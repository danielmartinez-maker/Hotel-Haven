#include "hh/assets/Cooker.h"
#include "hh/assets/Hasset.h"
#include "hh/assets/Json.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <span>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace hh::assets {
namespace {
using CookState = std::map<std::string, std::string, std::less<>>;

std::vector<std::byte> read_binary(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read export payload: " + path.string());
    std::vector<char> chars((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail()) throw std::runtime_error("failed reading export payload: " + path.string());
    std::vector<std::byte> bytes;
    bytes.reserve(chars.size());
    for (const unsigned char c : chars) bytes.push_back(static_cast<std::byte>(c));
    return bytes;
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read cook state: " + path.string());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string escape_json(std::string_view value) {
    std::string out = "\"";
    for (const char c : value) {
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out.push_back(c);
    }
    out.push_back('"');
    return out;
}

CookState load_state(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return {};
    const auto root = parse_json(read_text(path));
    if (root.at("schema").as_number() != 1.0) throw std::runtime_error("unsupported cook state schema");
    CookState state;
    for (const auto& [id, value] : root.at("assets").as_object()) state.emplace(id, value.as_string());
    return state;
}

std::string serialize_state(const CookState& state) {
    std::string out = "{\"assets\":{";
    bool first = true;
    for (const auto& [id, fingerprint] : state) {
        if (!first) out.push_back(',');
        first = false;
        out += escape_json(id); out.push_back(':'); out += escape_json(fingerprint);
    }
    out += "},\"schema\":1}";
    return out;
}

void atomic_replace(const std::filesystem::path& temp, const std::filesystem::path& output) {
#ifdef _WIN32
    if (!MoveFileExW(temp.c_str(), output.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const auto code = GetLastError();
        throw std::runtime_error("atomic replace failed with Win32 error " + std::to_string(code));
    }
#else
    std::filesystem::rename(temp, output);
#endif
}

void write_bytes_atomic(const std::filesystem::path& output, std::span<const std::byte> bytes) {
    std::filesystem::create_directories(output.parent_path());
    auto temp = output; temp += ".tmp";
    try {
        {
            std::ofstream stream(temp, std::ios::binary | std::ios::trunc);
            if (!stream) throw std::runtime_error("cannot create temporary output: " + temp.string());
            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            stream.flush();
            if (!stream) throw std::runtime_error("failed writing temporary output: " + temp.string());
        }
        atomic_replace(temp, output);
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove(temp, ec);
        throw;
    }
}

void write_text_atomic(const std::filesystem::path& output, std::string_view text) {
    write_bytes_atomic(output, std::as_bytes(std::span(text.data(), text.size())));
}

std::string repo_relative(const std::filesystem::path& path, const std::filesystem::path& root) {
    const auto absolute_path = std::filesystem::absolute(path).lexically_normal();
    const auto absolute_root = std::filesystem::absolute(root).lexically_normal();
    const auto relative = absolute_path.lexically_relative(absolute_root);
    if (relative.empty() || (!relative.empty() && *relative.begin() == "..")) {
        throw std::runtime_error("asset provenance path escapes repository root: " + absolute_path.string());
    }
    return relative.generic_string();
}

void validate_cookable(const AssetRecord& record) {
    for (const auto& diagnostic : validate_metadata(record.metadata)) {
        if (is_release_blocking(diagnostic.severity)) throw std::runtime_error(diagnostic.code + ": " + diagnostic.message);
    }
    if (record.metadata.asset_id.find('/') != std::string::npos || record.metadata.asset_id.find('\\') != std::string::npos) {
        throw std::runtime_error("asset_id cannot contain path separators");
    }
}

CookResult cook_internal(
    const AssetCatalog& catalog,
    const DependencyGraph& graph,
    std::string_view asset_id,
    const CookOptions& options,
    CookState& state,
    bool force) {
    const auto& record = catalog.by_id(asset_id);
    validate_cookable(record);
    const auto fingerprint = compute_fingerprint(record, catalog, graph, options.fingerprint);
    const auto output = options.cooked_root / (record.metadata.asset_id + ".hasset");
    const auto existing = state.find(record.metadata.asset_id);
    if (!force && existing != state.end() && existing->second == fingerprint && std::filesystem::exists(output)) {
        return {record.metadata.asset_id, false, output, fingerprint};
    }

    HassetDocument document;
    document.type = record.metadata.asset_type;
    document.asset_id = record.metadata.asset_id;
    document.fingerprint = fingerprint;
    document.dependencies = record.metadata.dependencies;
    document.source_path = repo_relative(record.source_path, options.repository_root);
    document.sidecar_path = repo_relative(record.sidecar_path, options.repository_root);
    document.payload = read_binary(record.export_path);
    const auto bytes = serialize_hasset(document);
    write_bytes_atomic(output, bytes);
    state[record.metadata.asset_id] = fingerprint;
    write_text_atomic(options.cooked_root / ".cook-state.json", serialize_state(state));
    return {record.metadata.asset_id, true, output, fingerprint};
}
}

CookResult cook_one(const AssetCatalog& catalog, const DependencyGraph& graph, std::string_view asset_id, const CookOptions& options) {
    auto state = load_state(options.cooked_root / ".cook-state.json");
    return cook_internal(catalog, graph, asset_id, options, state, false);
}

std::vector<CookResult> cook_all(const AssetCatalog& catalog, const DependencyGraph& graph, const CookOptions& options) {
    auto state = load_state(options.cooked_root / ".cook-state.json");
    std::vector<CookResult> results;
    for (const auto& id : graph.topological_order()) results.push_back(cook_internal(catalog, graph, id, options, state, true));
    return results;
}

std::vector<CookResult> cook_changed(const AssetCatalog& catalog, const DependencyGraph& graph, const CookOptions& options) {
    auto state = load_state(options.cooked_root / ".cook-state.json");
    std::vector<CookResult> results;
    for (const auto& id : graph.topological_order()) results.push_back(cook_internal(catalog, graph, id, options, state, false));
    return results;
}
}
