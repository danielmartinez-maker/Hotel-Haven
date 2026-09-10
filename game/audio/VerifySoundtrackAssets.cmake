if(NOT DEFINED HH_SOUNDTRACK_DIR)
  message(FATAL_ERROR "HH_SOUNDTRACK_DIR is required")
endif()

set(HH_SOUNDTRACK_ASSETS
  "Morning_in_the_Atrium.mp3|4323801|af78af388491faf33449efad632dfc6d09f53406d3ccf05cbad91dace933e69d"
  "Sunlight_on_Marble.mp3|4330697|cc0018a839e3fa9b0f0775b128a9774d8cb5d13802682bda4317b80347199460"
  "The_Concierge_Desk.mp3|4342609|89b2308da2b26d58f7186516c592810e83660ec86944707f0885aa94057939b7"
  "First_Light_on_Marble.mp3|4377091|06031b117eb1b1af6bd6c7f0cf72d955d07c176f38c06df97e32f577a3e51823"
)

foreach(entry IN LISTS HH_SOUNDTRACK_ASSETS)
  string(REPLACE "|" ";" fields "${entry}")
  list(GET fields 0 filename)
  list(GET fields 1 expected_size)
  list(GET fields 2 expected_sha256)
  set(path "${HH_SOUNDTRACK_DIR}/${filename}")
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Missing soundtrack asset: ${path}")
  endif()
  file(SIZE "${path}" actual_size)
  if(NOT actual_size EQUAL expected_size)
    message(FATAL_ERROR
      "Unexpected size for ${filename}: ${actual_size}, expected ${expected_size}")
  endif()
  file(SHA256 "${path}" actual_sha256)
  if(NOT actual_sha256 STREQUAL expected_sha256)
    message(FATAL_ERROR
      "Unexpected SHA-256 for ${filename}: ${actual_sha256}, expected ${expected_sha256}")
  endif()
endforeach()

set(manifest "${HH_SOUNDTRACK_DIR}/soundtrack.json")
if(NOT EXISTS "${manifest}")
  message(FATAL_ERROR "Missing soundtrack manifest: ${manifest}")
endif()
file(READ "${manifest}" manifest_content)
foreach(filename
    Morning_in_the_Atrium.mp3
    Sunlight_on_Marble.mp3
    The_Concierge_Desk.mp3
    First_Light_on_Marble.mp3)
  string(FIND "${manifest_content}" "${filename}" position)
  if(position EQUAL -1)
    message(FATAL_ERROR "Manifest does not reference ${filename}")
  endif()
endforeach()

message(STATUS "Hotel Haven soundtrack assets verified")
