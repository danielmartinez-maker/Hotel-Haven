if(NOT DEFINED HH_COOKED_ASSET_ROOT OR HH_COOKED_ASSET_ROOT STREQUAL "")
    message(FATAL_ERROR "HH_COOKED_ASSET_ROOT is required")
endif()
if(NOT DEFINED HH_RUNTIME_ASSET_ROOT OR HH_RUNTIME_ASSET_ROOT STREQUAL "")
    message(FATAL_ERROR "HH_RUNTIME_ASSET_ROOT is required")
endif()

# Manifest v2 declares 800 gameplay assets; 745 are runtime mesh assets.
# The source-to-cooked ID audit runs before this staging target in CI.
set(expected_runtime_assets 745)
file(GLOB cooked_assets "${HH_COOKED_ASSET_ROOT}/*.hasset")
list(SORT cooked_assets)
list(LENGTH cooked_assets cooked_count)
if(NOT cooked_count EQUAL expected_runtime_assets)
    message(FATAL_ERROR
        "Expected exactly ${expected_runtime_assets} cooked .hasset files in ${HH_COOKED_ASSET_ROOT}; found ${cooked_count}")
endif()

file(REMOVE_RECURSE "${HH_RUNTIME_ASSET_ROOT}")
file(MAKE_DIRECTORY "${HH_RUNTIME_ASSET_ROOT}")
foreach(asset IN LISTS cooked_assets)
    file(COPY "${asset}" DESTINATION "${HH_RUNTIME_ASSET_ROOT}")
endforeach()

file(GLOB staged_assets "${HH_RUNTIME_ASSET_ROOT}/*.hasset")
list(LENGTH staged_assets staged_count)
if(NOT staged_count EQUAL expected_runtime_assets)
    message(FATAL_ERROR
        "Runtime asset staging produced ${staged_count} files instead of ${expected_runtime_assets}")
endif()

message(STATUS
    "Staged ${staged_count} renderer-safe cooked assets to ${HH_RUNTIME_ASSET_ROOT}")
