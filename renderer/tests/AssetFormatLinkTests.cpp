#include "TestFramework.h"
#include "hh/assets/Hasset.h"

#include <string>

TEST_CASE("renderer links runtime hasset format without cooker") {
    hh::assets::HassetDocument document;
    document.asset_id = "HH_A001";

    const auto bytes = hh::assets::serialize_hasset(document);
    const auto roundTrip = hh::assets::parse_hasset(bytes);

    EXPECT_EQ(roundTrip.asset_id, std::string("HH_A001"));
}
