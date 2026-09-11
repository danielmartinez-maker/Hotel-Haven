#include "hh/frontend/InspectorModel.h"
#include <cstdint>
namespace hh::frontend {
// Platform renderers consume the structured inspector model; no gameplay inference lives here.
static_assert(sizeof(EntityId) == sizeof(std::uint64_t));
} // namespace hh::frontend
