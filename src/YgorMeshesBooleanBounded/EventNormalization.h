#pragma once

#include "CanonicalIntersectionComplex.h"
#include "SignedFeatureRelations.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct normalized_event_seed_proposal final {
  relation_event_seed_id seed{0};
  relation_event_seed_key seed_key{};
  intersection_event_key event_key{};
  intersection_occurrence_key occurrence_key{};
  feature_relation_id relation{0};
  relation_construction_id construction{0};
  relation_feature_key accepted_source_vertex{};
  intersection_range declared_construction_witnesses{};
  intersection_range declared_incidence{};
  intersection_range declared_candidate_incidence{};
  intersection_membership_role expected_membership_role =
      intersection_membership_role::interior;
  intersection_carrier_role expected_carrier_role =
      intersection_carrier_role::none;
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
  bool designated_authority = false;
  bool duplicate_consumer = false;
  bool distinct_occurrence_required = false;
  bool precision_evidence_complete = false;
  std::uint16_t schema_version =
      contract_versions::intersection_seed_binding_schema;
  std::uint16_t reserved = 0;
};

bool normalize_event_seed_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_construction_record> &constructions,
    std::vector<normalized_event_seed_proposal> &proposals,
    bounded_boolean_error &error);

template <class T, class I>
bool normalize_event_seeds(
    const signed_feature_relations<T, I> &relations,
    const context_owner_token &owner,
    std::vector<normalized_event_seed_proposal> &proposals,
    bounded_boolean_error &error) {
  if (!owner.same_owner(relations.owner())) {
    error = intersection_error(intersection_subcode::wrong_owner,
                               bounded_boolean_error_category::input_contract_error,
                               "Component 08 seed normalization owner mismatch",
                               intersection_checkpoint::seed_normalization);
    return false;
  }
  if (relations.verification() !=
      relation_verification_disposition::independently_verified) {
    error = intersection_error(
        intersection_subcode::predecessor_not_verified,
        bounded_boolean_error_category::input_contract_error,
        "Component 08 requires an independently verified Component 07 artifact",
        intersection_checkpoint::seed_normalization);
    return false;
  }
  return normalize_event_seed_records(relations.event_seeds(),
                                      relations.constructions(), proposals,
                                      error);
}

} // namespace ygor::mesh_boolean::bounded
