#include "WorldView.h"

#include <array>

namespace hh::client {
namespace {

constexpr std::string_view kStraightStair = "HH_A030";
constexpr std::string_view kStandardGuestDoor = "HH_A012";
constexpr std::string_view kGuestBed = "HH_A113";
constexpr std::string_view kNightstand = "HH_A121";
constexpr std::string_view kBedsideLamp = "HH_A125";
constexpr std::string_view kGuestDesk = "HH_A126";
constexpr std::string_view kDeskChair = "HH_A129";
constexpr std::string_view kBathroomVanity = "HH_A166";
constexpr std::string_view kBathroomToilet = "HH_A169";
constexpr std::string_view kShowerGlass = "HH_A171";
constexpr std::string_view kReceptionDesk = "HH_A186";
constexpr std::string_view kCleaningSupplyCabinet = "HH_A302";
constexpr std::string_view kPottedPlant = "HH_A396";

constexpr std::array<std::string_view, WorldAssetSet::GuestCharacterVariantCount>
    kGuestCharacterIds{{
        "HH_A451", "HH_A452", "HH_A453", "HH_A454", "HH_A455",
        "HH_A456", "HH_A457", "HH_A458", "HH_A459", "HH_A460",
        "HH_A461", "HH_A462", "HH_A463", "HH_A464", "HH_A465",
        "HH_A466", "HH_A467", "HH_A468", "HH_A469", "HH_A470",
        "HH_A471", "HH_A472", "HH_A473", "HH_A474", "HH_A475",
        "HH_A476", "HH_A477", "HH_A478", "HH_A479", "HH_A480",
    }};

constexpr std::array<std::string_view, WorldAssetSet::ReceptionistVariantCount>
    kReceptionistCharacterIds{{"HH_A481", "HH_A482"}};
constexpr std::array<std::string_view, WorldAssetSet::HousekeeperVariantCount>
    kHousekeeperCharacterIds{{"HH_A487", "HH_A488"}};
constexpr std::array<std::string_view, WorldAssetSet::MaintenanceVariantCount>
    kMaintenanceCharacterIds{{"HH_A495", "HH_A496"}};

constexpr std::size_t kCoreAssetCount = 13;
constexpr std::size_t kRequiredAssetCount =
    kCoreAssetCount + kGuestCharacterIds.size() +
    kReceptionistCharacterIds.size() + kHousekeeperCharacterIds.size() +
    kMaintenanceCharacterIds.size();

constexpr auto makeRequiredAssetIds() {
  std::array<std::string_view, kRequiredAssetCount> ids{{
      kStraightStair,
      kStandardGuestDoor,
      kGuestBed,
      kNightstand,
      kBedsideLamp,
      kGuestDesk,
      kDeskChair,
      kBathroomVanity,
      kBathroomToilet,
      kShowerGlass,
      kReceptionDesk,
      kCleaningSupplyCabinet,
      kPottedPlant,
  }};
  std::size_t index = kCoreAssetCount;
  for (const auto id : kGuestCharacterIds)
    ids[index++] = id;
  for (const auto id : kReceptionistCharacterIds)
    ids[index++] = id;
  for (const auto id : kHousekeeperCharacterIds)
    ids[index++] = id;
  for (const auto id : kMaintenanceCharacterIds)
    ids[index++] = id;
  return ids;
}

constexpr auto kRequiredAssetIds = makeRequiredAssetIds();

template <std::size_t N>
void resolveCharacterSet(
    std::array<std::optional<WorldAssetVisual>, N> &destination,
    const std::array<std::string_view, N> &ids,
    const WorldAssetResolver &resolver) {
  for (std::size_t index = 0; index < ids.size(); ++index)
    destination[index] = resolver(ids[index]);
}

} // namespace

std::span<const std::string_view> requiredWorldAssetIds() noexcept {
  return kRequiredAssetIds;
}

WorldAssetSet resolveWorldAssets(const WorldAssetResolver &resolver) {
  WorldAssetSet assets;
  assets.straightStair = resolver(kStraightStair);
  assets.standardGuestDoor = resolver(kStandardGuestDoor);
  assets.guestBed = resolver(kGuestBed);
  assets.nightstand = resolver(kNightstand);
  assets.bedsideLamp = resolver(kBedsideLamp);
  assets.guestDesk = resolver(kGuestDesk);
  assets.deskChair = resolver(kDeskChair);
  assets.bathroomVanity = resolver(kBathroomVanity);
  assets.bathroomToilet = resolver(kBathroomToilet);
  assets.showerGlass = resolver(kShowerGlass);
  assets.receptionDesk = resolver(kReceptionDesk);
  assets.cleaningSupplyCabinet = resolver(kCleaningSupplyCabinet);
  assets.pottedPlant = resolver(kPottedPlant);

  resolveCharacterSet(assets.guestCharacters, kGuestCharacterIds, resolver);
  resolveCharacterSet(
      assets.receptionistCharacters, kReceptionistCharacterIds, resolver);
  resolveCharacterSet(
      assets.housekeeperCharacters, kHousekeeperCharacterIds, resolver);
  resolveCharacterSet(
      assets.maintenanceCharacters, kMaintenanceCharacterIds, resolver);
  return assets;
}

} // namespace hh::client
