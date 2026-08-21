#include "StrictFloatingBuild.h"
#include "PrecisionContext.h"

namespace ygor::mesh_boolean::bounded {
static_assert(precision_context_schema_v1 != 0, "precision context schema must be versioned");

#define YGOR_INSTANTIATE_PRECISION_CONTEXT(T, I) \
    template boolean_outcome<std::shared_ptr<const precision_context<T>>> build_precision_context( \
        const precision_preflight<T> &, const boolean_context<T,I> &, \
        const precision_runtime_capabilities &)

YGOR_INSTANTIATE_PRECISION_CONTEXT(float, std::uint32_t);
YGOR_INSTANTIATE_PRECISION_CONTEXT(float, std::uint64_t);
YGOR_INSTANTIATE_PRECISION_CONTEXT(double, std::uint32_t);
YGOR_INSTANTIATE_PRECISION_CONTEXT(double, std::uint64_t);
#undef YGOR_INSTANTIATE_PRECISION_CONTEXT
}
