#include "CoplanarRelationOverlay.h"
#include "StrictFloatingBuild.h"

namespace ygor::mesh_boolean::bounded {
namespace {
static_assert(strict_floating_build_enabled(),
              "Component 07 coplanar overlay requires strict floating point");
}
} // namespace ygor::mesh_boolean::bounded
