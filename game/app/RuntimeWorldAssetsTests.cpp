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

std::uint32_t assetNumber(std::string_view id) {
  if (!id.starts_with("HH_A") || id.size() <= 4)
    throw std::runtime_error("unexpected test asset id");
  return static_cast<std::uint32_t>(std::stoul(std::string(id.substr(4))));
}

hh::assets::AssetType testAssetType(std::string_view id) {
  const auto number = assetNumber(id);
  return id == "HH_A012" || id == "HH_A057" || number >= 451u
             ? hh::assets::AssetType::SkinnedMesh
             : hh::assets::AssetType::StaticMesh;
}

} // namespace

int main() {
  try {
    const auto required = hh::client::requiredWorldAssetIds();
    require(required.size() == 64u,
            "world presentation dependency set should contain 64 assets");

    hh::renderer::RuntimeAssetRegistry registry;
    for (const std::string_view id : required) {
      const float alpha = id == "HH_A171" ? 0.55f : 1.0f;
      (void)registry.addHasset(
          makeHasset(std::string(id), alpha, testAssetType(id)));
    }

    const hh::client::WorldAssetSet assets =
        hh::client::worldAssetsFromRegistry(registry);
    require(assets.queenBed.has_value(), "queen bed binding missing");
    require(assets.singleBed->handle == registry.resolve("HH_A111"),
            "single bed binding mismatch");
    require(assets.doubleBed->handle == registry.resolve("HH_A112"),
            "double bed binding mismatch");
    require(assets.queenBed->handle == registry.resolve("HH_A113"),
            "queen bed handle mismatch");
    require(assets.kingBed->handle == registry.resolve("HH_A114"),
            "king bed binding mismatch");
    require(assets.twinBedLeft->handle == registry.resolve("HH_A115"),
            "left twin binding mismatch");
    require(assets.twinBedRight->handle == registry.resolve("HH_A116"),
            "right twin binding mismatch");
    require(assets.queenBed->localBounds.min.x == -1.0f &&
                assets.queenBed->localBounds.max.x == 1.0f,
            "decoded local bounds not propagated");
    require(assets.queenBed->localBounds.max.y == 2.0f,
            "decoded vertical local bounds not propagated");
    require(!assets.queenBed->translucent,
            "opaque material incorrectly marked translucent");
    require(assets.showerGlass->translucent,
            "alpha material did not mark world asset translucent");
    require(assets.standardGuestDoor->handle == registry.resolve("HH_A012"),
            "guest door binding mismatch");
    require(assets.lobbyEntranceDoor->handle == registry.resolve("HH_A057"),
            "lobby entrance binding mismatch");
    require(assets.bedsideLamp->handle == registry.resolve("HH_A125"),
            "bedside lamp binding mismatch");
    require(assets.deskChair->handle == registry.resolve("HH_A129"),
            "desk chair binding mismatch");
    require(assets.guestArmchair->handle == registry.resolve("HH_A131"),
            "guest armchair binding mismatch");
    require(assets.luggageBench->handle == registry.resolve("HH_A140"),
            "luggage bench binding mismatch");
    require(assets.wardrobe->handle == registry.resolve("HH_A141"),
            "wardrobe binding mismatch");
    require(assets.wallTelevision->handle == registry.resolve("HH_A153"),
            "wall television binding mismatch");
    require(assets.lobbySofa->handle == registry.resolve("HH_A194"),
            "lobby sofa binding mismatch");
    require(assets.lobbyArmchair->handle == registry.resolve("HH_A197"),
            "lobby armchair binding mismatch");
    require(assets.lobbyCoffeeTable->handle == registry.resolve("HH_A200"),
            "lobby coffee table binding mismatch");
    require(assets.cleaningSupplyCabinet->handle == registry.resolve("HH_A302"),
            "supply cabinet binding mismatch");
    require(assets.staffLockerBank->handle == registry.resolve("HH_A338"),
            "staff locker binding mismatch");
    require(assets.staffBench->handle == registry.resolve("HH_A339"),
            "staff bench binding mismatch");
    require(assets.pottedPlant->handle == registry.resolve("HH_A396"),
            "potted plant handle mismatch");
    require(assets.guestCharacters.front()->handle ==
                registry.resolve("HH_A451") &&
                assets.guestCharacters.back()->handle ==
                registry.resolve("HH_A480"),
            "guest character range was not fully bound");
    require(assets.receptionistCharacters.front()->handle ==
                registry.resolve("HH_A481"),
            "receptionist character binding missing");
    require(assets.housekeeperCharacters.front()->handle ==
                registry.resolve("HH_A487"),
            "housekeeper character binding missing");
    require(assets.maintenanceCharacters.front()->handle ==
                registry.resolve("HH_A495"),
            "maintenance character binding missing");

    const auto root = std::filesystem::temp_directory_path() /
                      "hotel-haven-runtime-world-assets";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    for (const std::string_view id : required) {
      const float alpha = id == "HH_A171" ? 0.55f : 1.0f;
      writeBytes(root / (std::string(id) + ".hasset"),
                 makeHasset(std::string(id), alpha, testAssetType(id)));
    }

    // A shipping package contains all 500 assets, but startup should decode only
    // the world-view dependency set rather than eagerly materializing every GLB.
    writeBytes(root / "HH_A001.hasset",
               makeHasset("HH_A001", 1.0f,
                          hh::assets::AssetType::StaticMesh));

    hh::renderer::RuntimeAssetRegistry startupRegistry;
    const hh::client::WorldAssetSet startupAssets =
        hh::client::loadWorldAssetsFromDirectory(startupRegistry, root);
    require(startupRegistry.size() == required.size(),
            "startup decoded assets outside the world dependency set");
    require(startupAssets.queenBed->handle ==
                startupRegistry.resolve("HH_A113"),
            "startup asset set did not bind queen bed handle");
    require(startupRegistry.resolve("HH_A451").value < startupRegistry.size(),
            "startup rejected a cooked skinned bind-pose character");

    bool strayLoaded = true;
    try {
      (void)startupRegistry.resolve("HH_A001");
    } catch (const std::runtime_error&) {
      strayLoaded = false;
    }
    require(!strayLoaded,
            "startup eagerly decoded an unrelated package asset");
    std::filesystem::remove_all(root);

    std::cout << "Selective runtime registry world asset bridge passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
