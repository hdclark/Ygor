#include "YgorMeshesBooleanBackend.h"

#include "YgorMeshesBooleanBroadPhase.h"
#include "YgorMeshesBooleanCellClassification.h"
#include "YgorMeshesBooleanGlobalArrangement.h"
#include "YgorMeshesBooleanInputTopology.h"
#include "YgorMeshesBooleanIntersectionEvents.h"
#include "YgorMeshesBooleanLocalRefinement.h"
#include "YgorMeshesBooleanOutput.h"
#include "YgorMeshesBooleanRealization.h"
#include "YgorMeshesBooleanSelection.h"
#include "YgorMeshesBooleanSymbolicRegistry.h"
#include "YgorMeshesExactKernel.h"

#include <algorithm>
#include <array>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#if defined(__FAST_MATH__)
#error "Boolean backend adapters must not be compiled with fast-math"
#endif

namespace ygor {
namespace mesh_boolean {

namespace {
#include "YgorMeshesBooleanBackendCommon.inc"
#include "YgorMeshesBooleanBackendReference.inc"
#include "YgorMeshesBooleanBackendAdapters.inc"
} // namespace
#include "YgorMeshesBooleanBackendAPI.inc"

} // namespace mesh_boolean
} // namespace ygor
