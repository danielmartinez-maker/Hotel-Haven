#include "hh/assets/Fingerprint.h"
#include "hh/assets/Hash.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace hh::assets {
namespace {
void append_u64(std::vector<std::byte>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64u; shift += 8u) out.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}
void append_field(std::vector<std::byte>& out, std::string_view value) {
    append_u64(out, static_cast<std::uint64_t>(value.size()));
    const auto bytes = std::as_bytes(std::span(value.data(), value.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}
void append_record_hashes(std::vector<std::byte>& out, const AssetRecord& record) {
    append_field(out, record.metadata.asset_id);
    append_field(out, hash_file(record.source_path));
    append_field(out, hash_file(record.sidecar_path));
    append_field(out, hash_file(record.export_path));
}
}

std::string compute_fingerprint(
    const AssetRecord& record,
    const AssetCatalog& catalog,
    const DependencyGraph& graph,
    const FingerprintSettings& settings) {
    std::vector<std::byte> stream;
    append_field(stream, "HMG-070-FINGERPRINT-V1");
    append_record_hashes(stream, record);
    append_field(stream, settings.importer_version);
    append_field(stream, settings.cooker_version);
    append_field(stream, settings.compression_settings);
    append_field(stream, settings.platform_target);
    for (const auto& dependency_id : graph.dependencies_of(record.metadata.asset_id, true)) {
        append_record_hashes(stream, catalog.by_id(dependency_id));
    }
    return sha256_hex(stream);
}
}
