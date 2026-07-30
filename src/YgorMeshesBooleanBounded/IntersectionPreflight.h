#pragma once

#include "EventNormalization.h"

namespace ygor::mesh_boolean::bounded {

struct intersection_preflight_plan final {
  intersection_resource_estimate estimate{};
  std::uint64_t construction_count = 0;
  std::uint64_t construction_ledger_count = 0;
  std::uint64_t seed_incidence_count = 0;
  std::uint64_t candidate_incidence_count = 0;
  std::uint64_t sort_comparison_bound = 0;
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
  return preflight_intersection_event_records(
      relations.event_seeds(), relations.constructions(),
      relations.construction_ledger().size(),
      relations.event_seed_incidence().size(),
      relations.event_seed_candidate_incidence().size(), capabilities, plan,
      error);
}

} // namespace ygor::mesh_boolean::bounded
