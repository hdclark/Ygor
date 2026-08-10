#pragma once

#include "EventNormalization.h"
#include "CheckedArithmetic.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace ygor::mesh_boolean::bounded {

struct intersection_preflight_plan final {
  intersection_resource_estimate estimate{};
  std::uint64_t construction_count = 0;
  std::uint64_t construction_ledger_count = 0;
  std::uint64_t seed_incidence_count = 0;
  std::uint64_t candidate_incidence_count = 0;
  std::uint64_t sort_comparison_bound = 0;
  std::uint64_t source_edge_domain_count = 0;
  std::uint64_t source_triangle_count = 0;
  std::uint64_t ordering_certificate_count = 0;
  std::uint64_t canonical_byte_bound = 0;
};

bool preflight_intersection_event_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_construction_record> &constructions,
    std::uint64_t construction_ledger_count,
    std::uint64_t seed_incidence_count,
    std::uint64_t candidate_incidence_count,
    const intersection_capabilities &capabilities,
    intersection_preflight_plan &plan,
    bounded_boolean_error &error);

template <class T, class I>
bool preflight_intersection_events(
    const signed_feature_relations<T, I> &relations,
    const intersection_capabilities &capabilities,
    intersection_preflight_plan &plan,
    bounded_boolean_error &error) {
  if (!capabilities.owner.same_owner(relations.owner())) {
    error = intersection_error(intersection_subcode::wrong_owner,
                               bounded_boolean_error_category::input_contract_error,
                               "Component 08 preflight owner mismatch",
                               intersection_checkpoint::predecessor_validation);
    return false;
  }
  if (capabilities.provider_version != contract_versions::intersection_provider ||
      capabilities.semantic_policy_version !=
          contract_versions::intersection_semantic_policy ||
      capabilities.codec_version != contract_versions::intersection_codec ||
      capabilities.verifier_version != contract_versions::intersection_verifier) {
    error = intersection_error(intersection_subcode::unsupported_version,
                               bounded_boolean_error_category::input_contract_error,
                               "Component 08 capability version mismatch",
                               intersection_checkpoint::context_capability_validation);
    return false;
  }
  if (relations.schema_version() != contract_versions::relation_artifact_schema ||
      relations.provider_version() != contract_versions::relation_provider ||
      relations.codec_version() != contract_versions::relation_codec ||
      relations.verifier_version() != contract_versions::relation_verifier) {
    error = intersection_error(intersection_subcode::predecessor_mismatch,
                               bounded_boolean_error_category::input_contract_error,
                               "Component 08 rejected Component 07 version mismatch",
                               intersection_checkpoint::predecessor_validation);
    return false;
  }
  if (relations.verification() !=
      relation_verification_disposition::independently_verified) {
    error = intersection_error(
        intersection_subcode::predecessor_not_verified,
        bounded_boolean_error_category::input_contract_error,
        "Component 08 requires independently verified Component 07 input",
        intersection_checkpoint::predecessor_validation);
    return false;
  }
  if (!relations.candidates() ||
      !relations.candidates()->owner().same_owner(capabilities.owner)) {
    error = intersection_error(intersection_subcode::predecessor_mismatch,
                               bounded_boolean_error_category::input_contract_error,
                               "Component 08 candidate predecessor handshake failed",
                               intersection_checkpoint::predecessor_validation);
    return false;
  }
  if (!preflight_intersection_event_records(
          relations.event_seeds(), relations.constructions(),
          relations.construction_ledger().size(),
          relations.event_seed_incidence().size(),
          relations.event_seed_candidate_incidence().size(), capabilities,
          plan, error))
    return false;

  const auto fail_overflow = [&](const char *summary) {
    error = intersection_error(intersection_subcode::count_overflow,
                               bounded_boolean_error_category::index_overflow,
                               summary,
                               intersection_checkpoint::count_preflight);
    return false;
  };
  const auto fail_capacity = [&](const char *summary) {
    error = intersection_error(intersection_subcode::capacity_exceeded,
                               bounded_boolean_error_category::resource_limit,
                               summary,
                               intersection_checkpoint::count_preflight);
    return false;
  };
  const auto add = [&](std::uint64_t value, std::uint64_t &target,
                       const char *summary) {
    return checked_add<std::uint64_t>(target, value, target) ||
           fail_overflow(summary);
  };
  const auto multiply = [&](std::uint64_t a, std::uint64_t b,
                            std::uint64_t &out, const char *summary) {
    return checked_multiply<std::uint64_t>(a, b, out) ||
           fail_overflow(summary);
  };
  const auto vector_bytes = [&](std::uint64_t count, std::uint64_t width,
                                std::uint64_t &target,
                                const char *summary) {
    std::uint64_t bytes = 0;
    return multiply(count, width, bytes, summary) &&
           add(bytes, target, summary);
  };

  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto &table = relations.candidates()->primitive_table(operand);
    for (const auto &edge : table.edges) {
      if (edge.edge_class == canonical_edge_class::source_edge &&
          edge.source_feature_owner &&
          !add(1, plan.source_edge_domain_count,
               "Component 08 source-edge domain count overflowed"))
        return false;
    }
    if (!add(static_cast<std::uint64_t>(table.triangles.size()),
             plan.source_triangle_count,
             "Component 08 source-triangle count overflowed"))
      return false;
  }

  std::uint64_t source_edge_incidence = 0;
  for (const auto &feature : relations.event_seed_incidence()) {
    if (feature.kind == relation_feature_kind::source_edge &&
        !add(1, source_edge_incidence,
             "Component 08 source-edge incidence count overflowed"))
      return false;
  }
  plan.estimate.membership_count = source_edge_incidence;
  if (!add(static_cast<std::uint64_t>(relations.event_seeds().size()),
           plan.estimate.membership_count,
           "Component 08 membership upper bound overflowed"))
    return false;
  plan.estimate.cluster_count = plan.estimate.membership_count;
  plan.estimate.interval_count = plan.source_edge_domain_count;
  if (!add(plan.estimate.membership_count, plan.estimate.interval_count,
           "Component 08 interval upper bound overflowed"))
    return false;

  for (const auto &construction : relations.constructions()) {
    if (construction.kind == relation_construction_kind::bounded_carrier &&
        !add(1, plan.estimate.carrier_count,
             "Component 08 carrier count overflowed"))
      return false;
  }
  plan.estimate.overlap_count =
      static_cast<std::uint64_t>(relations.coplanar_overlap_components().size());
  if (!add(static_cast<std::uint64_t>(relations.coplanar_oriented_arcs().size()),
           plan.estimate.overlap_count,
           "Component 08 overlap count overflowed"))
    return false;

  plan.estimate.aggregate_count = plan.estimate.incidence_count;
  if (!add(plan.estimate.occurrence_count, plan.estimate.aggregate_count,
           "Component 08 aggregate count overflowed") ||
      !add(plan.estimate.interval_count, plan.estimate.aggregate_count,
           "Component 08 aggregate count overflowed") ||
      !add(plan.estimate.carrier_count, plan.estimate.aggregate_count,
           "Component 08 aggregate count overflowed") ||
      !add(plan.estimate.overlap_count, plan.estimate.aggregate_count,
           "Component 08 aggregate count overflowed"))
    return false;

  plan.estimate.descriptor_count = plan.source_edge_domain_count;
  std::uint64_t triangle_adjacencies = 0;
  if (!multiply(plan.source_triangle_count, std::uint64_t{3},
                triangle_adjacencies,
                "Component 08 source-topology descriptor count overflowed") ||
      !add(triangle_adjacencies, plan.estimate.descriptor_count,
           "Component 08 descriptor count overflowed"))
    return false;
  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto &table = relations.candidates()->primitive_table(operand);
    if (!add(static_cast<std::uint64_t>(table.edges.size()),
             plan.estimate.descriptor_count,
             "Component 08 descriptor count overflowed"))
      return false;
  }
  if (!add(plan.estimate.interval_count, plan.estimate.descriptor_count,
           "Component 08 descriptor count overflowed") ||
      !add(plan.estimate.cluster_count, plan.estimate.descriptor_count,
           "Component 08 descriptor count overflowed") ||
      !add(plan.estimate.occurrence_count, plan.estimate.descriptor_count,
           "Component 08 descriptor count overflowed") ||
      !add(plan.estimate.carrier_count, plan.estimate.descriptor_count,
           "Component 08 descriptor count overflowed") ||
      !add(plan.estimate.overlap_count, plan.estimate.descriptor_count,
           "Component 08 descriptor count overflowed"))
    return false;

  plan.ordering_certificate_count = plan.estimate.membership_count;
  if (!add(plan.estimate.carrier_count, plan.ordering_certificate_count,
           "Component 08 ordering-certificate count overflowed"))
    return false;

  if (plan.estimate.membership_count > capabilities.maximum_memberships)
    return fail_capacity("Component 08 membership capacity exceeded");
  if (plan.estimate.cluster_count > capabilities.maximum_clusters)
    return fail_capacity("Component 08 cluster capacity exceeded");
  if (plan.estimate.interval_count > capabilities.maximum_intervals)
    return fail_capacity("Component 08 interval capacity exceeded");
  if (plan.estimate.carrier_count > capabilities.maximum_carriers)
    return fail_capacity("Component 08 carrier capacity exceeded");
  if (plan.estimate.overlap_count > capabilities.maximum_overlaps)
    return fail_capacity("Component 08 overlap capacity exceeded");
  if (plan.estimate.aggregate_count > capabilities.maximum_aggregates)
    return fail_capacity("Component 08 aggregate capacity exceeded");
  if (plan.estimate.descriptor_count > capabilities.maximum_descriptors)
    return fail_capacity("Component 08 descriptor capacity exceeded");

  const auto add_persistent = [&](std::uint64_t count, std::uint64_t width,
                                  const char *summary) {
    return vector_bytes(count, width, plan.estimate.persistent_bytes, summary);
  };
  if (!add_persistent(plan.estimate.membership_count,
                      sizeof(source_edge_membership_record),
                      "Component 08 membership byte estimate overflowed") ||
      !add_persistent(plan.source_edge_domain_count,
                      sizeof(source_edge_sequence_record),
                      "Component 08 sequence byte estimate overflowed") ||
      !add_persistent(plan.estimate.cluster_count,
                      sizeof(source_edge_cluster_record),
                      "Component 08 cluster byte estimate overflowed") ||
      !add_persistent(plan.estimate.interval_count,
                      sizeof(source_edge_interval_record),
                      "Component 08 interval byte estimate overflowed") ||
      !add_persistent(plan.estimate.carrier_count,
                      sizeof(transverse_carrier_record) +
                          sizeof(carrier_membership_record) +
                          sizeof(carrier_cluster_record) +
                          sizeof(carrier_active_span_record),
                      "Component 08 carrier byte estimate overflowed") ||
      !add_persistent(plan.estimate.overlap_count,
                      sizeof(coplanar_support_record) +
                          sizeof(collinear_overlap_carrier_record) +
                          sizeof(coplanar_overlap_record) +
                          sizeof(coplanar_region_incidence_record),
                      "Component 08 overlap byte estimate overflowed") ||
      !add_persistent(plan.estimate.aggregate_count,
                      sizeof(crossing_aggregate_record) +
                          sizeof(contact_aggregate_record),
                      "Component 08 aggregate byte estimate overflowed") ||
      !add_persistent(plan.estimate.descriptor_count,
                      sizeof(intersection_descriptor_record),
                      "Component 08 descriptor byte estimate overflowed") ||
      !add_persistent(plan.ordering_certificate_count,
                      sizeof(ordering_certificate_record),
                      "Component 08 ordering byte estimate overflowed"))
    return false;

  std::uint64_t scratch_records = plan.estimate.membership_count;
  if (!add(plan.estimate.cluster_count, scratch_records,
           "Component 08 scratch count overflowed") ||
      !add(plan.estimate.interval_count, scratch_records,
           "Component 08 scratch count overflowed") ||
      !add(plan.estimate.aggregate_count, scratch_records,
           "Component 08 scratch count overflowed") ||
      !add(plan.estimate.descriptor_count, scratch_records,
           "Component 08 scratch count overflowed") ||
      !vector_bytes(scratch_records, sizeof(std::uint64_t) * 4,
                    plan.estimate.temporary_bytes,
                    "Component 08 scratch byte estimate overflowed"))
    return false;

  std::uint64_t pair_work = 0;
  if (!multiply(plan.estimate.membership_count,
                plan.estimate.membership_count, pair_work,
                "Component 08 pair-work estimate overflowed") ||
      !add(pair_work, plan.estimate.work_units,
           "Component 08 work estimate overflowed") ||
      !add(scratch_records, plan.estimate.work_units,
           "Component 08 work estimate overflowed"))
    return false;
  if (plan.estimate.work_units > capabilities.maximum_work_units)
    return fail_capacity("Component 08 work capacity exceeded");

  if (!multiply(plan.estimate.persistent_bytes, std::uint64_t{2},
                plan.canonical_byte_bound,
                "Component 08 canonical byte estimate overflowed") ||
      !add(std::uint64_t{4096}, plan.canonical_byte_bound,
           "Component 08 canonical byte estimate overflowed"))
    return false;
  if (plan.canonical_byte_bound > capabilities.maximum_canonical_bytes)
    return fail_capacity("Component 08 canonical-byte capacity exceeded");
  return true;
}

} // namespace ygor::mesh_boolean::bounded
