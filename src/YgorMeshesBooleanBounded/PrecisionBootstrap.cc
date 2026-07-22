#include "StrictFloatingBuild.h"
#include "PrecisionBootstrap.h"

namespace ygor::mesh_boolean::bounded {
static_assert(precision_bootstrap_schema_v1 != 0, "precision bootstrap schema must be versioned");

#define YGOR_INSTANTIATE_PRECISION_BOOTSTRAP(T, I) \
    template boolean_outcome<precision_preflight<T>> preflight_precision( \
        const pending_invocation<T,I> &, const precision_bootstrap_capabilities &); \
    template boolean_outcome<precision_preflight<T>> preflight_precision( \
        const pending_boolean_context_view<T,I> &, const precision_bootstrap_capabilities &)

YGOR_INSTANTIATE_PRECISION_BOOTSTRAP(float, std::uint32_t);
YGOR_INSTANTIATE_PRECISION_BOOTSTRAP(float, std::uint64_t);
YGOR_INSTANTIATE_PRECISION_BOOTSTRAP(double, std::uint32_t);
YGOR_INSTANTIATE_PRECISION_BOOTSTRAP(double, std::uint64_t);
#undef YGOR_INSTANTIATE_PRECISION_BOOTSTRAP
}
