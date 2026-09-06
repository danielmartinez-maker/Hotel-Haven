#include "hh/assets/Hasset.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace hh::assets {
namespace {
constexpr std::array<std::byte, 8> kMagic{
    std::byte{'H'}, std::byte{'H'}, std::byte{'A'}, std::byte{'S'},
    std::byte{'S'}, std::byte{'E'}, std::byte{'T'}, std::byte{0}};
constexpr std::uint32_t kVersion = 1;

void append_u32(std::vector<std::byte>& out, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u) out.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}
void append_u64(std::vector<std::byte>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64u; shift += 8u) out.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}
void append_string(std::vector<std::byte>& out, std::string_view value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("hasset string too large");
    append_u32(out, static_cast<std::uint32_t>(value.size()));
    for (const unsigned char c : value) out.push_back(static_cast<std::byte>(c));
}
bool absolute_path_text(std::string_view value) {
    return std::filesystem::path(value).is_absolute() ||
           (value.size() >= 3 && ((value[0] >= 'A' && value[0] <= 'Z') || (value[0] >= 'a' && value[0] <= 'z')) && value[1] == ':' && (value[2] == '/' || value[2] == '\\'));
}

class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes) : bytes_(bytes) {}
    std::byte byte() {
        require(1);
        return bytes_[pos_++];
    }
    std::uint32_t u32() {
        require(4);
        std::uint32_t value = 0;
        for (unsigned shift = 0; shift < 32u; shift += 8u) value |= std::to_integer<std::uint32_t>(bytes_[pos_++]) << shift;
        return value;
    }
    std::uint64_t u64() {
        require(8);
        std::uint64_t value = 0;
        for (unsigned shift = 0; shift < 64u; shift += 8u) value |= std::to_integer<std::uint64_t>(bytes_[pos_++]) << shift;
        return value;
    }
    std::string string() {
        const auto size = u32();
        require(size);
        std::string out;
        out.reserve(size);
        for (std::uint32_t i = 0; i < size; ++i) out.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes_[pos_++])));
        return out;
    }
    std::vector<std::byte> payload(std::uint64_t size) {
        if (size > static_cast<std::uint64_t>(bytes_.size() - pos_)) throw std::runtime_error("truncated hasset payload");
        const auto count = static_cast<std::size_t>(size);
        std::vector<std::byte> out(bytes_.begin() + static_cast<std::ptrdiff_t>(pos_), bytes_.begin() + static_cast<std::ptrdiff_t>(pos_ + count));
        pos_ += count;
        return out;
    }
    bool finished() const noexcept { return pos_ == bytes_.size(); }
private:
    void require(std::size_t count) const {
        if (count > bytes_.size() - pos_) throw std::runtime_error("truncated hasset");
    }
    std::span<const std::byte> bytes_;
    std::size_t pos_{};
};
}

std::vector<std::byte> serialize_hasset(const HassetDocument& document) {
    if (absolute_path_text(document.source_path) || absolute_path_text(document.sidecar_path)) {
        throw std::runtime_error("hasset provenance paths must be repository-relative");
    }
    std::vector<std::byte> out;
    out.insert(out.end(), kMagic.begin(), kMagic.end());
    append_u32(out, kVersion);
    append_u32(out, static_cast<std::uint32_t>(document.type));
    append_string(out, document.asset_id);
    append_string(out, document.fingerprint);
    auto dependencies = document.dependencies;
    std::sort(dependencies.begin(), dependencies.end());
    dependencies.erase(std::unique(dependencies.begin(), dependencies.end()), dependencies.end());
    if (dependencies.size() > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("too many hasset dependencies");
    append_u32(out, static_cast<std::uint32_t>(dependencies.size()));
    for (const auto& dependency : dependencies) append_string(out, dependency);
    append_string(out, document.source_path);
    append_string(out, document.sidecar_path);
    append_u64(out, static_cast<std::uint64_t>(document.payload.size()));
    out.insert(out.end(), document.payload.begin(), document.payload.end());
    return out;
}

HassetDocument parse_hasset(std::span<const std::byte> bytes) {
    Reader reader(bytes);
    for (const auto expected : kMagic) if (reader.byte() != expected) throw std::runtime_error("invalid hasset magic");
    if (reader.u32() != kVersion) throw std::runtime_error("unsupported hasset version");
    const auto raw_type = reader.u32();
    if (raw_type > static_cast<std::uint32_t>(AssetType::AudioBank)) throw std::runtime_error("invalid hasset asset type");
    HassetDocument document;
    document.type = static_cast<AssetType>(raw_type);
    document.asset_id = reader.string();
    document.fingerprint = reader.string();
    const auto dependency_count = reader.u32();
    document.dependencies.reserve(dependency_count);
    for (std::uint32_t i = 0; i < dependency_count; ++i) document.dependencies.push_back(reader.string());
    document.source_path = reader.string();
    document.sidecar_path = reader.string();
    document.payload = reader.payload(reader.u64());
    if (!reader.finished()) throw std::runtime_error("trailing bytes in hasset");
    return document;
}
}
