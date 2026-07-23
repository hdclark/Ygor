#include "SourceFacetRegionKernel.h"

namespace ygor::mesh_boolean::bounded {

bounded_boolean_error source_facet_region_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint) {
  const auto category =
      subcode == relation_subcode::source_facet_region_unresolved
          ? bounded_boolean_error_category::geometric_condition_exceeds_tolerance
          : bounded_boolean_error_category::internal_invariant_error;
  return relation_error(subcode, category, summary, checkpoint);
}

} // namespace ygor::mesh_boolean::bounded
