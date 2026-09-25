#include "WorldView.h"

#include <array>

namespace hh::client {
namespace {

constexpr std::string_view kStraightStair = "HH_A030";
constexpr std::string_view kStandardGuestDoor = "HH_A012";
constexpr std::string_view kLobbyEntranceDoor = "HH_A057";
constexpr std::string_view kSingleBed = "HH_A111";
constexpr std::string_view kDoubleBed = "HH_A112";
constexpr std::string_view kQueenBed = "HH_A113";
constexpr std::string_view kKingBed = "HH_A114";
constexpr std::string_view kTwinBedLeft = "HH_A115";
constexpr std::string_view kTwinBedRight = "HH_A116";
constexpr std::string_view kNightstand = "HH_A121";
constexpr std::string_view kBedsideLamp = "HH_A125";
constexpr std::string_view kGuestDesk = "HH_A126";
constexpr std::string_view kDeskChair = "HH_A129";
constexpr std::string_view kGuestArmchair = "HH_A131";
constexpr std::string_view kLuggageBench = "HH_A140";
constexpr std::string_view kWardrobe = "HH_A141";
constexpr std::string_view kTvConsole = "HH_A151";
constexpr std::string_view kWallTelevision = "HH_A153";
constexpr std::string_view kBathroomVanity = "HH_A166";
constexpr std::string_view kBathroomToilet = "HH_A169";
constexpr std::string_view kShowerGlass = "HH_A171";
constexpr std::string_view kReceptionDesk = "HH_A186";
constexpr std::string_view kLobbySofa = "HH_A194";
constexpr std::string_view kLobbyArmchair = "HH_A197";
constexpr std::string_view kLobbyCoffeeTable = "HH_A200";
constexpr std::string_view kLobbyFloorLamp = "HH_A205";
constexpr std::string_view kLobbyPlanter = "HH_A207";
constexpr std::string_view kLuggageCart = "HH_A211";
constexpr std::string_view kHousekeepingCart = "HH_A291";
constexpr std::string_view kUtilityCart = "HH_A320";
constexpr std::string_view kCleaningSupplyCabinet = "HH_A302";
constexpr std::string_view kStaffLockerBank = "HH_A338";
constexpr std::string_view kStaffBench = "HH_A339";
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

constexpr std::size_t kCoreAssetCount = 34;
constexpr std::size_t kRequiredAssetCount =
    kCoreAssetCount + kGuestCharacterIds.size() +
    kReceptionistCharacterIds.size() + kHousekeeperCharacterIds.size() +
    kMaintenanceCharacterIds.size();

constexpr auto makeRequiredAssetIds() {
  std::array<std::string_view, kRequiredAssetCount> ids{{
      kStraightStair,
      kStandardGuestDoor,
      kLobbyEntranceDoor,
      kSingleBed,
      kDoubleBed,
      kQueenBed,
      kKingBed,
      kTwinBedLeft,
      kTwinBedRight,
      kNightstand,
      kBedsideLamp,
      kGuestDesk,
      kDeskChair,
      kGuestArmchair,
      kLuggageBench,
      kWardrobe,
      kTvConsole,
      kWallTelevision,
      kBathroomVanity,
      kBathroomToilet,
      kShowerGlass,
      kReceptionDesk,
      kLobbySofa,
      kLobbyArmchair,
      kLobbyCoffeeTable,
      kLobbyFloorLamp,
      kLobbyPlanter,
      kLuggageCart,
      kHousekeepingCart,
      kUtilityCart,
      kCleaningSupplyCabinet,
      kStaffLockerBank,
      kStaffBench,
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

constexpr std::array<std::string_view, 52> kRuntimePresentationAssetIds{{
    "HH_A511", "HH_A512", "HH_A513", "HH_A539", "HH_A540",
    "HH_A551", "HH_A552", "HH_A553", "HH_A554", "HH_A557",
    "HH_A558", "HH_A559", "HH_A560", "HH_A561", "HH_A562",
    "HH_A563", "HH_A564", "HH_A565", "HH_A566", "HH_A568",
    "HH_A569", "HH_A570", "HH_A571", "HH_A572", "HH_A575",
    "HH_A577", "HH_A578", "HH_A579", "HH_A580", "HH_A581",
    "HH_A582", "HH_A583", "HH_A584", "HH_A585", "HH_A586",
    "HH_A593", "HH_A594", "HH_A601", "HH_A602", "HH_A603",
    "HH_A604", "HH_A605", "HH_A608", "HH_A609", "HH_A610",
    "HH_A611", "HH_A614", "HH_A615", "HH_A619", "HH_A623",
    "HH_A643", "HH_A644",
}};

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

std::span<const std::string_view> runtimeWorldPresentationAssetIds() noexcept {
  return kRuntimePresentationAssetIds;
}

WorldAssetSet resolveWorldAssets(const WorldAssetResolver &resolver) {
  WorldAssetSet assets;
  assets.straightStair = resolver(kStraightStair);
  assets.standardGuestDoor = resolver(kStandardGuestDoor);
  assets.lobbyEntranceDoor = resolver(kLobbyEntranceDoor);
  assets.singleBed = resolver(kSingleBed);
  assets.doubleBed = resolver(kDoubleBed);
  assets.queenBed = resolver(kQueenBed);
  assets.kingBed = resolver(kKingBed);
  assets.twinBedLeft = resolver(kTwinBedLeft);
  assets.twinBedRight = resolver(kTwinBedRight);
  assets.nightstand = resolver(kNightstand);
  assets.bedsideLamp = resolver(kBedsideLamp);
  assets.guestDesk = resolver(kGuestDesk);
  assets.deskChair = resolver(kDeskChair);
  assets.guestArmchair = resolver(kGuestArmchair);
  assets.luggageBench = resolver(kLuggageBench);
  assets.wardrobe = resolver(kWardrobe);
  assets.tvConsole = resolver(kTvConsole);
  assets.wallTelevision = resolver(kWallTelevision);
  assets.bathroomVanity = resolver(kBathroomVanity);
  assets.bathroomToilet = resolver(kBathroomToilet);
  assets.showerGlass = resolver(kShowerGlass);
  assets.receptionDesk = resolver(kReceptionDesk);
  assets.lobbySofa = resolver(kLobbySofa);
  assets.lobbyArmchair = resolver(kLobbyArmchair);
  assets.lobbyCoffeeTable = resolver(kLobbyCoffeeTable);
  assets.lobbyFloorLamp = resolver(kLobbyFloorLamp);
  assets.lobbyPlanter = resolver(kLobbyPlanter);
  assets.luggageCart = resolver(kLuggageCart);
  assets.housekeepingCart = resolver(kHousekeepingCart);
  assets.utilityCart = resolver(kUtilityCart);
  assets.cleaningSupplyCabinet = resolver(kCleaningSupplyCabinet);
  assets.staffLockerBank = resolver(kStaffLockerBank);
  assets.staffBench = resolver(kStaffBench);
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

const WorldAssetVisual* findWorldAsset(
    const WorldAssetSet& assets,
    std::string_view assetId) noexcept {
  const auto found = assets.catalog.find(std::string(assetId));
  return found == assets.catalog.end() ? nullptr : &found->second;
}

} // namespace hh::client
