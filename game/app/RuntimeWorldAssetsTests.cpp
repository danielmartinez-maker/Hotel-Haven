#include "RuntimeWorldAssets.h"

#include "hh/assets/Hasset.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool value, const char* message) {
  if (!value)
    throw std::runtime_error(message);
}

void appendU32(std::vector<std::byte>& bytes, std::uint32_t value) {
  for (unsigned shift = 0; shift < 32u; shift += 8u)
    bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

template <typename T>
void appendValue(std::vector<std::byte>& bytes, const T& value) {
  const auto* first = reinterpret_cast<const std::byte*>(&value);
  bytes.insert(bytes.end(), first, first + sizeof(T));
}

std::vector<std::byte> makeMeshGlb(float alpha) {
  std::vector<std::byte> binary;
  const std::array<float, 9> positions{
      -1.0f, 0.0f, -0.5f,
       1.0f, 0.0f,  0.5f,
       0.0f, 2.0f,  0.0f,
  };
  for (const float value : positions)
    appendValue(binary, value);
  const std::size_t indexOffset = binary.size();
  const std::array<std::uint16_t, 3> indices{0u, 1u, 2u};
  for (const auto value : indices)
    appendValue(binary, value);
  while ((binary.size() % 4u) != 0u)
    binary.push_back(std::byte{0});

  std::string json =
      "{"
      "\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
      "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":6}"
      "],"
      "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}"
      "],"
      "\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1," +
      std::to_string(alpha) + "]}}],"
      "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1,\"material\":0}]}],"
      "\"nodes\":[{\"mesh\":0}],"
      "\"scenes\":[{\"nodes\":[0]}],\"scene\":0"
      "}";
  while ((json.size() % 4u) != 0u)
    json.push_back(' ');

  const auto jsonLength = static_cast<std::uint32_t>(json.size());
  const auto binaryLength = static_cast<std::uint32_t>(binary.size());
  std::vector<std::byte> glb;
  appendU32(glb, 0x46546c67u);
  appendU32(glb, 2u);
  appendU32(glb, 12u + 8u + jsonLength + 8u + binaryLength);
  appendU32(glb, jsonLength);
  appendU32(glb, 0x4e4f534au);
  for (const char c : json)
    glb.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
  appendU32(glb, binaryLength);
  appendU32(glb, 0x004e4942u);
  glb.insert(glb.end(), binary.begin(), binary.end());
  return glb;
}

std::vector<std::byte> makeHasset(
    const std::string& id,
    float alpha = 1.0f,
    hh::assets::AssetType type = hh::assets::AssetType::StaticMesh) {
  hh::assets::HassetDocument document;
  document.type = type;
  document.asset_id = id;
  document.fingerprint = "runtime-world-test";
  document.source_path = "Art/Exports/" + id + ".glb";
  document.sidecar_path = "Art/Exports/" + id + ".asset.json";
  document.payload = makeMeshGlb(alpha);
  return hh::assets::serialize_hasset(document);
}

void writeBytes(const std::filesystem::path& path,
                const std::vector<std::byte>& bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output)
    throw std::runtime_error("cannot create runtime test asset");
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!output)
    throw std::runtime_error("cannot write runtime test asset");
}

} // namespace

int main() {
  try {
    hh::renderer::RuntimeAssetRegistry registry;
    const std::array<const char*, 9> ids{
        "HH_A030", "HH_A113", "HH_A121", "HH_A126", "HH_A166",
        "HH_A169", "HH_A171", "HH_A186", "HH_A396"};
    for (const char* id : ids)
      (void)registry.addHasset(makeHasset(id, std::string(id) == "HH_A171" ? 0.55f : 1.0f));

    const hh::client::WorldAssetSet assets =
        hh::client::worldAssetsFromRegistry(registry);
    require(assets.guestBed.has_value(), "guest bed binding missing");
    require(assets.guestBed->handle == registry.resolve("HH_A113"),
            "guest bed handle mismatch");
    require(assets.guestBed->localBounds.min.x == -1.0f &&
                assets.guestBed->localBounds.max.x == 1.0f,
            "decoded local bounds not propagated");
    require(assets.guestBed->localBounds.max.y == 2.0f,
            "decoded vertical local bounds not propagated");
    require(!assets.guestBed->translucent,
            "opaque material incorrectly marked translucent");
    require(assets.showerGlass->translucent,
            "alpha material did not mark world asset translucent");
    require(assets.pottedPlant->handle == registry.resolve("HH_A396"),
            "potted plant handle mismatch");

    const auto root = std::filesystem::temp_directory_path() /
                      "hotel-haven-runtime-world-assets";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    for (const char* id : ids) {
      writeBytes(root / (std::string(id) + ".hasset"),
                 makeHasset(id, std::string(id) == "HH_A171" ? 0.55f : 1.0f));
    }
    writeBytes(root / "HH_A451.hasset",
               makeHasset("HH_A451", 1.0f, hh::assets::AssetType::SkinnedMesh));

    hh::renderer::RuntimeAssetRegistry startupRegistry;
    const hh::client::WorldAssetSet startupAssets =
        hh::client::loadWorldAssetsFromDirectory(startupRegistry, root);
    require(startupRegistry.size() == 10u,
            "startup did not load the complete cooked asset directory");
    require(startupAssets.guestBed->handle == startupRegistry.resolve("HH_A113"),
            "startup asset set did not bind guest bed handle");
    require(startupRegistry.resolve("HH_A451").value < startupRegistry.size(),
            "startup rejected a cooked skinned bind-pose mesh");
    std::filesystem::remove_all(root);

    std::cout << "Runtime registry world asset bridge passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
