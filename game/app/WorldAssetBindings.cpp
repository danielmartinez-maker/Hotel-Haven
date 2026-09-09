#include "WorldView.h"

namespace hh::client {

WorldAssetSet resolveWorldAssets(const WorldAssetResolver &resolver) {
  WorldAssetSet assets;
  assets.straightStair = resolver("HH_A030");
  assets.guestBed = resolver("HH_A113");
  assets.nightstand = resolver("HH_A121");
  assets.guestDesk = resolver("HH_A126");
  assets.bathroomVanity = resolver("HH_A166");
  assets.bathroomToilet = resolver("HH_A169");
  assets.showerGlass = resolver("HH_A171");
  assets.receptionDesk = resolver("HH_A186");
  assets.pottedPlant = resolver("HH_A396");
  return assets;
}

} // namespace hh::client
