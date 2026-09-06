#include "hh/assets/Cli.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Cooker.h"
#include "hh/assets/DependencyGraph.h"
#include "hh/assets/Exporter.h"
#include "hh/assets/Fingerprint.h"
#include "hh/assets/Metadata.h"
#include <filesystem>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace hh::assets {
namespace {
std::filesystem::path find_repository_root() {
    auto current = std::filesystem::absolute(std::filesystem::current_path()).lexically_normal();
    for (;;) {
        if (std::filesystem::is_directory(current / "Art") && std::filesystem::is_directory(current / "Tools/ContentPipeline")) return current;
        const auto parent = current.parent_path();
        if (parent == current || parent.empty()) break;
        current = parent;
    }
    throw std::runtime_error("cannot locate Hotel Haven repository root (expected Art and Tools/ContentPipeline)");
}

FingerprintSettings default_fingerprint_settings() {
    return {"hmg070-importer-v1", "hmg070-cooker-v1", "none", "windows-x64"};
}

CookOptions cook_options(const std::filesystem::path& root) {
    return {root, root / "Build/CookedAssets", default_fingerprint_settings()};
}

bool blocking(const std::vector<Diagnostic>& diagnostics, std::ostream& err, std::string_view id) {
    bool failed = false;
    for (const auto& diagnostic : diagnostics) {
        const char* severity = "MINOR";
        switch (diagnostic.severity) {
        case Severity::Blocker: severity = "BLOCKER"; break;
        case Severity::Critical: severity = "CRITICAL"; break;
        case Severity::Major: severity = "MAJOR"; break;
        case Severity::Minor: severity = "MINOR"; break;
        }
        err << severity << ' ' << id << ' ' << diagnostic.code << ": " << diagnostic.message << '\n';
        failed = failed || is_release_blocking(diagnostic.severity);
    }
    return failed;
}

std::string join(const std::vector<std::string>& values) {
    if (values.empty()) return "(none)";
    std::string out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) out += ", ";
        out += values[i];
    }
    return out;
}

void usage(std::ostream& err) {
    err << "usage:\n"
        << "  asset validate <asset-id|path>\n"
        << "  asset export <asset-id|path>\n"
        << "  asset cook <asset-id|path>\n"
        << "  asset cook --changed\n"
        << "  asset cook --all\n"
        << "  asset inspect <asset-id>\n"
        << "  asset deps <asset-id>\n"
        << "  asset audit --milestone <name>\n";
}

const AssetRecord& resolve_for_cli(const AssetCatalog& catalog, std::string_view id_or_path, const std::filesystem::path& root) {
    if (catalog.records().find(id_or_path) != catalog.records().end()) return catalog.by_id(id_or_path);
    auto path = std::filesystem::path(id_or_path);
    if (path.is_relative()) path = root / path;
    return catalog.resolve(std::filesystem::absolute(path).lexically_normal().string());
}

int validate_command(std::string_view target, const std::filesystem::path& root, std::ostream& out, std::ostream& err) {
    auto target_path = std::filesystem::path(target);
    if (target_path.is_relative()) target_path = root / target_path;
    if (std::filesystem::is_directory(target_path)) {
        const auto catalog = AssetCatalog::scan(target_path);
        static_cast<void>(DependencyGraph::build(catalog));
        bool failed = false;
        for (const auto& [id, record] : catalog.records()) failed = blocking(validate_metadata(record.metadata), err, id) || failed;
        if (!failed) out << "validated " << catalog.size() << " asset" << (catalog.size() == 1 ? "" : "s") << '\n';
        return failed ? 1 : 0;
    }
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    static_cast<void>(DependencyGraph::build(catalog));
    const auto& record = resolve_for_cli(catalog, target, root);
    const bool failed = blocking(validate_metadata(record.metadata), err, record.metadata.asset_id);
    if (!failed) out << "validated " << record.metadata.asset_id << '\n';
    return failed ? 1 : 0;
}

int audit_command(std::string_view milestone, const std::filesystem::path& root, std::ostream& out, std::ostream& err) {
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    static_cast<void>(DependencyGraph::build(catalog));
    bool failed = false;
    std::size_t matched = 0;
    for (const auto& [id, record] : catalog.records()) {
        const auto& m = record.metadata;
        if (!m.milestone || *m.milestone != milestone) continue;
        ++matched;
        failed = blocking(validate_metadata(m), err, id) || failed;
        auto require_field = [&](const std::optional<std::string>& field, const char* name) {
            if (!field || field->empty()) { err << "MAJOR " << id << " audit.missing." << name << ": required for milestone audit\n"; failed = true; }
        };
        require_field(m.content_owner, "content_owner");
        require_field(m.technical_reviewer, "technical_reviewer");
        require_field(m.art_reviewer, "art_reviewer");
        require_field(m.dependent_feature_owner, "dependent_feature_owner");
        if (!m.lifecycle_state || *m.lifecycle_state != LifecycleState::ReleaseReady) {
            err << "MAJOR " << id << " audit.lifecycle: asset must be RELEASE_READY\n";
            failed = true;
        }
    }
    if (!failed) out << "audited " << matched << " asset" << (matched == 1 ? "" : "s") << " for milestone " << milestone << '\n';
    return failed ? 1 : 0;
}
}

int run_asset_cli(std::span<const std::string_view> args, std::ostream& out, std::ostream& err) {
    try {
        if (args.empty()) { usage(err); return 2; }
        const auto root = find_repository_root();
        const auto command = args[0];
        if (command == "validate") {
            if (args.size() != 2) { usage(err); return 2; }
            return validate_command(args[1], root, out, err);
        }
        const auto catalog = AssetCatalog::scan(root / "Art/Exports");
        const auto graph = DependencyGraph::build(catalog);
        if (command == "export") {
            if (args.size() != 2) { usage(err); return 2; }
            const auto& record = resolve_for_cli(catalog, args[1], root);
            const auto result = export_asset(record, root);
            if (result.success) out << result.message << '\n'; else err << result.message << '\n';
            return result.success ? 0 : (result.exit_code == 0 ? 1 : result.exit_code);
        }
        if (command == "cook") {
            if (args.size() != 2) { usage(err); return 2; }
            const auto options = cook_options(root);
            if (args[1] == "--changed") {
                const auto results = cook_changed(catalog, graph, options);
                std::size_t count = 0; for (const auto& result : results) if (result.cooked) ++count;
                out << "cooked " << count << " changed asset" << (count == 1 ? "" : "s") << '\n';
                return 0;
            }
            if (args[1] == "--all") {
                const auto results = cook_all(catalog, graph, options);
                out << "cooked " << results.size() << " asset" << (results.size() == 1 ? "" : "s") << '\n';
                return 0;
            }
            const auto& record = resolve_for_cli(catalog, args[1], root);
            const auto result = cook_one(catalog, graph, record.metadata.asset_id, options);
            out << (result.cooked ? "cooked " : "unchanged ") << result.asset_id << " -> " << result.output.generic_string() << '\n';
            return 0;
        }
        if (command == "inspect") {
            if (args.size() != 2) { usage(err); return 2; }
            const auto& record = catalog.by_id(args[1]);
            out << "asset_id: " << record.metadata.asset_id << '\n';
            out << "asset_type: " << to_string(record.metadata.asset_type) << '\n';
            out << "metadata: " << canonicalize_metadata(record.metadata) << '\n';
            out << "fingerprint: " << compute_fingerprint(record, catalog, graph, default_fingerprint_settings()) << '\n';
            out << "output: " << (root / "Build/CookedAssets" / (record.metadata.asset_id + ".hasset")).generic_string() << '\n';
            out << "direct_dependencies: " << join(graph.dependencies_of(record.metadata.asset_id, false)) << '\n';
            return 0;
        }
        if (command == "deps") {
            if (args.size() != 2) { usage(err); return 2; }
            static_cast<void>(catalog.by_id(args[1]));
            out << "direct_dependencies: " << join(graph.dependencies_of(args[1], false)) << '\n';
            out << "transitive_dependencies: " << join(graph.dependencies_of(args[1], true)) << '\n';
            out << "direct_dependents: " << join(graph.dependents_of(args[1], false)) << '\n';
            out << "transitive_dependents: " << join(graph.dependents_of(args[1], true)) << '\n';
            return 0;
        }
        if (command == "audit") {
            if (args.size() != 3 || args[1] != "--milestone") { usage(err); return 2; }
            return audit_command(args[2], root, out, err);
        }
        usage(err);
        return 2;
    } catch (const std::exception& exception) {
        err << "error: " << exception.what() << '\n';
        return 1;
    }
}
}
