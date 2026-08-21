#include "StrictFloatingBuild.h"
#include "SourceEdgeRelationKernel.h"

namespace ygor::mesh_boolean::bounded {

bounded_boolean_error source_edge_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint) {
  const bool geometric =
      subcode == relation_subcode::source_edge_direction_degenerate ||
      subcode == relation_subcode::source_edge_support_unresolved ||
      subcode == relation_subcode::source_edge_parameter_unresolved ||
      subcode == relation_subcode::source_edge_residual_rejected;
  return relation_error(
      subcode,
      geometric
          ? bounded_boolean_error_category::
                geometric_condition_exceeds_tolerance
          : bounded_boolean_error_category::internal_invariant_error,
      summary, checkpoint);
}

} // namespace ygor::mesh_boolean::bounded
