#pragma once

#include "TransverseCarrierArrangements.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct coplanar_support_proposal final {
  coplanar_support_key key{};
  feature_relation_id relation{intersection_invalid_ordinal};
  candidate_id candidate{intersection_invalid_ordinal};
  bounded_boolean_digest first_source_facet_semantic_digest{};
  bounded_boolean_digest second_source_facet_semantic_digest{};
  std::vector<relation_feature_key> original_boundary_edges{};
  bool designated_authority = false;
  bool support_consistent = false;
  bool orientation_consistent = false;
  bool symbolic_policy_consistent = false;
  bool precision_evidence_complete = false;
};

struct collinear_overlap_carrier_proposal final {
  collinear_overlap_carrier_key key{};
  relation_construction_id first_parameter_interval{intersection_invalid_ordinal};
  relation_construction_id second_parameter_interval{intersection_invalid_ordinal};
  relation_interval_evidence_id first_parameter_evidence{intersection_invalid_ordinal};
  relation_interval_evidence_id second_parameter_evidence{intersection_invalid_ordinal};
  std::uint64_t first_lower_bits = 0;
  std::uint64_t first_upper_bits = 0;
  std::uint64_t second_lower_bits = 0;
  std::uint64_t second_upper_bits = 0;
  parameter_domain_status first_domain = parameter_domain_status::invalid;
  parameter_domain_status second_domain = parameter_domain_status::invalid;
  event_occurrence_id start_occurrence{intersection_invalid_ordinal};
  event_occurrence_id end_occurrence{intersection_invalid_ordinal};
  intersection_occurrence_key start_occurrence_key{};
  intersection_occurrence_key end_occurrence_key{};
  relation_feature_key start_source_vertex{};
  relation_feature_key end_source_vertex{};
  feature_relation_id relation{intersection_invalid_ordinal};
  candidate_id candidate{intersection_invalid_ordinal};
  std::vector<relation_feature_key> source_provenance{};
  bool first_original_source_edge = false;
  bool second_original_source_edge = false;
  bool first_direction_valid = false;
  bool second_direction_valid = false;
  bool parameter_correspondence_verified = false;
  bool endpoint_ownership_verified = false;
  bool half_open_first = false;
  bool half_open_second = false;
  bool half_open_policy_consistent = false;
  bool exact_zero_length = false;
  bool definitely_positive_length = false;
  bool separate_sheet_required = false;
};

struct coplanar_overlap_component_proposal final {
  coplanar_overlap_key key{};
  relation_coplanar_overlap_component_id component07_component{
      intersection_invalid_ordinal};
  feature_relation_id relation{intersection_invalid_ordinal};
  std::vector<event_occurrence_id> boundary_events{};
  std::vector<collinear_overlap_carrier_key> boundary_carriers{};
  bool closed = false;
  bool distinct_sheet_occurrences = false;
  bool zero_measure = false;
  bool component_assembly_complete = false;
};

struct coplanar_region_incidence_proposal final {
  coplanar_support_key support{};
  coplanar_overlap_key component{};
  relation_feature_key first_facet{};
  relation_feature_key second_facet{};
  relation_feature_key first_triangle{};
  relation_feature_key second_triangle{};
  coplanar_region_classification classification =
      coplanar_region_classification::point_contact;
  feature_relation_status relation_status =
      feature_relation_status::not_evaluated;
  operand_id symbolic_owner = operand_id::a;
  std::uint8_t sheet_mask = 0;
  std::vector<event_occurrence_id> boundary_events{};
  std::vector<collinear_overlap_carrier_key> boundary_carriers{};
  std::vector<relation_feature_key> coverage_witnesses{};
  bounded_boolean_digest source_facet_semantic_digest{};
  bool coverage_complete = false;
  bool internal_diagonals_coverage_only = false;
  bool source_facet_semantics_verified = false;
};

struct coplanar_carrier_arrangement_tables final {
  std::vector<coplanar_support_record> supports{};
  std::vector<feature_relation_id> support_relation_provenance{};
  std::vector<candidate_id> support_candidate_provenance{};
  std::vector<relation_feature_key> support_original_boundary_edge_index{};
  std::vector<event_occurrence_id> support_boundary_event_index{};
  std::vector<collinear_overlap_carrier_id> support_boundary_carrier_index{};
  std::vector<coplanar_overlap_record_id> support_overlap_index{};
  std::vector<coplanar_region_incidence_id> support_region_index{};

  std::vector<collinear_overlap_carrier_record> carriers{};
  std::vector<feature_relation_id> carrier_relation_provenance{};
  std::vector<candidate_id> carrier_candidate_provenance{};
  std::vector<relation_feature_key> carrier_source_provenance{};

  std::vector<coplanar_overlap_record> overlaps{};
  std::vector<event_occurrence_id> overlap_boundary_event_index{};
  std::vector<collinear_overlap_carrier_id> overlap_boundary_carrier_index{};
  std::vector<feature_relation_id> overlap_relation_provenance{};

  std::vector<coplanar_region_incidence_record> regions{};
  std::vector<event_occurrence_id> region_boundary_event_index{};
  std::vector<collinear_overlap_carrier_id> region_boundary_carrier_index{};
  std::vector<relation_feature_key> region_coverage_witness_index{};
};

template <class T>
bool build_coplanar_carrier_arrangements(
    const std::vector<coplanar_support_proposal> &support_proposals,
    const std::vector<collinear_overlap_carrier_proposal> &carrier_proposals,
    const std::vector<coplanar_overlap_component_proposal> &overlap_proposals,
    const std::vector<coplanar_region_incidence_proposal> &region_proposals,
    coplanar_carrier_arrangement_tables &tables,
    bounded_boolean_error &error);

template <class T>
bool verify_coplanar_carrier_arrangements(
    const std::vector<coplanar_support_proposal> &support_proposals,
    const std::vector<collinear_overlap_carrier_proposal> &carrier_proposals,
    const std::vector<coplanar_overlap_component_proposal> &overlap_proposals,
    const std::vector<coplanar_region_incidence_proposal> &region_proposals,
    const coplanar_carrier_arrangement_tables &tables,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
