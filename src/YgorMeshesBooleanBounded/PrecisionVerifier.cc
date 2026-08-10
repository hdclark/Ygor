#include "StrictFloatingBuild.h"
#include "PrecisionVerifier.h"

namespace ygor::mesh_boolean::bounded {
static_assert(precision_verifier_v1 != 0, "precision verifier must be versioned");

#define YGOR_INSTANTIATE_PRECISION_VERIFIER(T, I) \
    template bool verify_precision_preflight( \
        const precision_preflight<T> &, const pending_invocation<T,I> &, \
        const precision_bootstrap_capabilities &); \
    template bool verify_precision_context( \
        const precision_context<T> &, const precision_preflight<T> &, \
        const boolean_context<T,I> &, const precision_runtime_capabilities &)

YGOR_INSTANTIATE_PRECISION_VERIFIER(float, std::uint32_t);
YGOR_INSTANTIATE_PRECISION_VERIFIER(float, std::uint64_t);
YGOR_INSTANTIATE_PRECISION_VERIFIER(double, std::uint32_t);
YGOR_INSTANTIATE_PRECISION_VERIFIER(double, std::uint64_t);
#undef YGOR_INSTANTIATE_PRECISION_VERIFIER
}
