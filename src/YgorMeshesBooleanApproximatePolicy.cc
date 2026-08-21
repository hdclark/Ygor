#include "YgorMeshesBooleanApproximate.h"

namespace ygor {
namespace mesh_boolean {

digest approximate_realization_policy_digest(
    const product_realization_policy &policy) {
  const auto &a = policy.approximation;
  canonical_encoder encoder;
  encoder.u16(policy.schema);
  encoder.byte(static_cast<std::uint8_t>(policy.semantics));
  encoder.u16(policy.search.schema);
  encoder.byte(static_cast<std::uint8_t>(policy.search.strategy));
  encoder.u64(policy.search.max_candidates);
  encoder.u64(policy.search.max_candidate_evaluations);
  encoder.u64(policy.search.max_search_nodes);
  encoder.u64(policy.search.max_obligations);
  encoder.u64(policy.search.max_triangle_pairs);
  encoder.u64(policy.search.max_predicate_checks);
  encoder.u64(policy.search.max_verifier_work);
  encoder.u64(policy.search.max_verifier_records);
  encoder.u64(policy.search.max_verifier_bytes);
  encoder.u16(a.schema);
  encoder.boolean(a.enabled);
  encoder.byte(static_cast<std::uint8_t>(a.unit));
  encoder.floating(a.max_vertex_displacement);
  encoder.boolean(a.has_max_axis_displacement_x);
  encoder.floating(a.max_axis_displacement_x);
  encoder.boolean(a.has_max_axis_displacement_y);
  encoder.floating(a.max_axis_displacement_y);
  encoder.boolean(a.has_max_axis_displacement_z);
  encoder.floating(a.max_axis_displacement_z);
  encoder.floating(a.max_support_plane_deviation);
  encoder.boolean(a.allow_original_vertex_movement);
  encoder.u16(a.candidate_generation_version);
  encoder.u32(a.candidate_ulp_radius);
  encoder.floating(a.declared_model_tolerance);
  encoder.string(a.application_acceptance_metadata);
  return domain_digest({{'Y', 'G', 'B', 'A', 'P', 'P', '0', '1'}},
                       encoder.bytes());
}

} // namespace mesh_boolean
} // namespace ygor
