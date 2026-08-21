#include "SourceFacetRegionSegmentBuild.h"

namespace ygor::mesh_boolean::bounded {

bounded_boolean_error source_facet_region_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint) {
  const bool geometric =
      subcode == relation_subcode::source_facet_region_unresolved ||
      subcode == relation_subcode::source_facet_segment_order_unresolved ||
      subcode ==
          relation_subcode::source_facet_segment_partition_unresolved;
  return relation_error(
      subcode,
      geometric
          ? bounded_boolean_error_category::
                geometric_condition_exceeds_tolerance
          : bounded_boolean_error_category::internal_invariant_error,
      summary, checkpoint);
}

} // namespace ygor::mesh_boolean::bounded
