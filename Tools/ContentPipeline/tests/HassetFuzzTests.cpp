#include "Test.h"
#include "hh/assets/Hasset.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace hh::assets;
namespace {
std::uint64_t next_random(std::uint64_t& state) noexcept {
    state ^= state << 13u;
    state ^= state >> 7u;
    state ^= state << 17u;
    return state;
}

HassetDocument fuzz_seed_document() {
    HassetDocument d;
    d.type = AssetType::StaticMesh;
    d.asset_id = "asset.fuzz.seed";
    d.fingerprint = "0123456789abcdef0123456789abcdef";
    d.dependencies = {"asset.dep.a", "asset.dep.b"};
    d.source_path = "Art/Source/Fuzz/Seed.blend";
    d.sidecar_path = "Art/Exports/Fuzz/Seed.glb.asset.json";
    d.payload.assign(128, std::byte{0x5a});
    return d;
}

void parse_may_reject(const std::vector<std::byte>& bytes) {
    try {
        static_cast<void>(parse_hasset(bytes));
    } catch (const std::runtime_error&) {
        // Malformed input is expected to fail closed with a normal parse error.
    }
}
}

HH_TEST("hasset parser survives four thousand deterministic hostile byte buffers") {
    std::uint64_t state = 0x484156454e484153ull;
    for (int case_index = 0; case_index < 4000; ++case_index) {
        const std::size_t size = static_cast<std::size_t>(next_random(state) % 1025u);
        std::vector<std::byte> bytes(size);
        for (auto& value : bytes) value = static_cast<std::byte>(next_random(state) & 0xffu);
        parse_may_reject(bytes);
    }
}

HH_TEST("hasset parser survives four thousand structured binary mutations") {
    const auto canonical = serialize_hasset(fuzz_seed_document());
    std::uint64_t state = 0x5041525345524655ull;
    for (int case_index = 0; case_index < 4000; ++case_index) {
        auto bytes = canonical;
        const int mutations = 1 + static_cast<int>(next_random(state) % 12u);
        for (int mutation = 0; mutation < mutations && !bytes.empty(); ++mutation) {
            const auto index = static_cast<std::size_t>(next_random(state) % bytes.size());
            bytes[index] = static_cast<std::byte>(next_random(state) & 0xffu);
        }
        if ((next_random(state) & 3u) == 0u && !bytes.empty()) {
            bytes.resize(static_cast<std::size_t>(next_random(state) % bytes.size()));
        }
        parse_may_reject(bytes);
    }
}
