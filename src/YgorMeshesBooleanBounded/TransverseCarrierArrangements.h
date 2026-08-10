#pragma once

#include "SourceEdgeArrangements.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct transverse_carrier_proposal final {
  transverse_carrier_key key{};
  relation_construction_id construction{intersection_invalid_ordinal};
  feature_relation_id relation{intersection_invalid_ordinal};
  candidate_id candidate{intersection_invalid_ordinal};
  std::uint64_t relation_lineage = 0;
  bool designated_authority = false;
  bool support_consistent = false;
  bool orientation_consistent = false;
  bool residuals_accepted = false;
  bool precision_evidence_complete = false;
  bool coplanar = false;
};

struct carrier_membership_proposal final {
  transverse_carrier_key carrier{};
  intersection_occurrence_key occurrence_key{};
  event_occurrence_id occurrence{intersection_invalid_ordinal};
  event_id event{intersection_invalid_ordinal};
  relation_interval_evidence_id parameter{intersection_invalid_ordinal};
  std::uint64_t nominal_bits = 0;
  std::uint64_t lower_bits = 0;
  std::uint64_t upper_bits = 0;
  std::uint64_t parameter_lineage = 0;
  std::uint64_t relation_lineage = 0;
  feature_relation_id relation{intersection_invalid_ordinal};
  bool exact_equal_eligible = false;
  bool cluster_eligible = false;
  relation_source_facet_region_id first_region_evidence{intersection_invalid_ordinal};
  relation_source_facet_region_id second_region_evidence{intersection_invalid_ordinal};
  bool first_region_contains = false;
  bool second_region_contains = false;
};

struct transverse_relation_interval_proposal final {
  transverse_carrier_key carrier{};
  feature_relation_id relation{intersection_invalid_ordinal};
  std::uint64_t interval_lineage = 0;
  relation_interval_evidence_id start_parameter{intersection_invalid_ordinal};
  relation_interval_evidence_id end_parameter{intersection_invalid_ordinal};
  std::uint64_t start_lower_bits = 0;
  std::uint64_t start_upper_bits = 0;
  std::uint64_t end_lower_bits = 0;
  std::uint64_t end_upper_bits = 0;
  event_occurrence_id start_occurrence{intersection_invalid_ordinal};
  event_occurrence_id end_occurrence{intersection_invalid_ordinal};
  intersection_span_activation activation =
      intersection_span_activation::inactive;
  relation_source_facet_region_id first_region_evidence{intersection_invalid_ordinal};
  relation_source_facet_region_id second_region_evidence{intersection_invalid_ordinal};
  bool first_region_contains = false;
  bool second_region_contains = false;
  bool ownership_verified = false;
  bool start_closed = true;
  bool end_closed = true;
};

struct transverse_carrier_arrangement_tables final {
  std::vector<transverse_carrier_record> carriers{};
  std::vector<feature_relation_id> carrier_relation_provenance{};
  std::vector<candidate_id> carrier_candidate_provenance{};
  std::vector<carrier_membership_record> memberships{};
  std::vector<carrier_membership_id> carrier_membership_index{};
  std::vector<carrier_cluster_record> clusters{};
  std::vector<event_occurrence_id> cluster_occurrence_index{};
  std::vector<carrier_membership_id> cluster_membership_index{};
  std::vector<carrier_cluster_id> carrier_cluster_index{};
  std::vector<carrier_active_span_record> spans{};
  std::vector<carrier_active_span_id> carrier_span_index{};
  std::vector<feature_relation_id> span_relation_provenance{};
  std::vector<relation_source_facet_region_id> span_region_incidence{};
  std::vector<ordering_certificate_record> ordering_certificates{};
};

template <class T>
bool build_transverse_carrier_arrangements(
    const std::vector<transverse_carrier_proposal> &carrier_proposals,
    const std::vector<carrier_membership_proposal> &membership_proposals,
    const std::vector<transverse_relation_interval_proposal> &interval_proposals,
    transverse_carrier_arrangement_tables &tables,
    bounded_boolean_error &error);

template <class T>
bool verify_transverse_carrier_arrangements(
    const std::vector<transverse_carrier_proposal> &carrier_proposals,
    const std::vector<carrier_membership_proposal> &membership_proposals,
    const std::vector<transverse_relation_interval_proposal> &interval_proposals,
    const transverse_carrier_arrangement_tables &tables,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
