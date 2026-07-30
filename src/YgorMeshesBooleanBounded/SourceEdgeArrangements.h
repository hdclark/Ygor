#pragma once

#include "EventIncidence.h"
#include "FiniteInterval.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct source_edge_domain_record final {
  relation_feature_key source_edge{};
  relation_feature_key start_vertex{};
  relation_feature_key end_vertex{};
};

struct source_edge_membership_proposal final {
  source_edge_membership_key key{};
  event_occurrence_id occurrence{intersection_invalid_ordinal};
  event_id event{intersection_invalid_ordinal};
  relation_interval_evidence_id parameter{intersection_invalid_ordinal};
  std::uint64_t nominal_bits = 0;
  std::uint64_t lower_bits = 0;
  std::uint64_t upper_bits = 0;
  parameter_domain_status domain = parameter_domain_status::invalid;
  exact_relation_status exact_zero = exact_relation_status::unavailable;
  exact_relation_status exact_one = exact_relation_status::unavailable;
  intersection_range contributions{};
  intersection_range incident_facet_uses{};
  bool exact_equal_eligible = false;
  bool cluster_eligible = false;
  bool internal_diagonal_discovery = false;
  bool bookkeeping_only = false;
};

struct source_edge_arrangement_tables final {
  std::vector<source_edge_membership_record> memberships{};
  std::vector<source_edge_membership_id> membership_sequence_index{};
  std::vector<source_edge_sequence_record> sequences{};
  std::vector<source_edge_cluster_record> clusters{};
  std::vector<event_occurrence_id> cluster_occurrence_index{};
  std::vector<source_edge_membership_id> cluster_membership_index{};
  std::vector<source_edge_cluster_id> sequence_cluster_index{};
  std::vector<source_edge_interval_record> intervals{};
  std::vector<source_edge_interval_id> sequence_interval_index{};
  std::vector<ordering_certificate_record> ordering_certificates{};
};

bool collect_source_edge_membership_proposals(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_interval_evidence_record> &interval_evidence,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    std::vector<source_edge_membership_proposal> &proposals,
    bounded_boolean_error &error);

template <class T>
bool build_source_edge_arrangements(
    const std::vector<source_edge_domain_record> &domains,
    const std::vector<source_edge_membership_proposal> &proposals,
    source_edge_arrangement_tables &tables,
    bounded_boolean_error &error);

template <class T>
bool verify_source_edge_arrangements(
    const std::vector<source_edge_domain_record> &domains,
    const std::vector<source_edge_membership_proposal> &proposals,
    const source_edge_arrangement_tables &tables,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
