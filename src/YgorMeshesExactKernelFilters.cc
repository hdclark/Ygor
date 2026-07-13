#include "YgorMeshesExactKernel.h"
#include <cfenv>
#if defined(__FAST_MATH__)
#error "Exact predicate filters must not be compiled with fast-math"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Exact predicate filters must not assume finite-only arithmetic"
#endif
// Schema 1 deliberately routes all signs through the exact fallback. This is a
// certified strict filter: it never accepts an unproved floating-point sign.
namespace ygor { namespace mesh_boolean { namespace exact_filter_policy {
const std::uint16_t orient2d_proof_version=0;
const std::uint16_t orient3d_proof_version=0;
} } }
