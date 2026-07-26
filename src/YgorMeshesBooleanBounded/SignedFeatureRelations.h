#pragma once

#include "CanonicalCandidateStream.h"
#include "RelationRequestGraph.h"
#include "SymbolicPerturbation.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T> struct candidate_source_edge_relation_stage;
template <class T> struct candidate_source_edge_facet_relation_stage;
template <class T> struct candidate_source_facet_relation_stage;
template <class T> struct candidate_coplanar_overlay_stage;
template <class T, class I> class relation_artifact_assembler;
struct relation_artifact_test_access;

struct relation_truth_record final {
  std::uint64_t rounded_nominal_bits = 0;
  bounded_sign_status bounded_sign = bounded_sign_status::invalid;
  exact_relation_status exact_relation = exact_relation_status::unavailable;
  predicate_disposition disposition = predicate_disposition::fail_invalid;
  std::uint16_t rounded_formula = 0;
  std::uint16_t exact_formula = 0;
  std::uint32_t reserved = 0;
};

struct relation_construction_record final {
  relation_construction_id id{0};
  relation_request_id producer{0};
  relation_construction_kind kind = relation_construction_kind::bounded_point;
  std::uint8_t component_count = 0;
  std::array<std::uint64_t, 6> nominal_bits{};
  std::array<std::uint64_t, 6> lower_bits{};
  std::array<std::uint64_t, 6> upper_bits{};
  std::uint64_t residual_truth_begin = 0;
  std::uint64_t residual_truth_count = 0;
  bool finite = false;
  bool tolerance_compatible = false;
  std::uint32_t reserved = 0;
};

struct feature_relation_record final {
  feature_relation_id id{0};
  relation_request_id producer{0};
  feature_relation_family family =
      feature_relation_family::source_vertex_source_facet;
  relation_record_scope scope = relation_record_scope::public_source_feature;
  feature_relation_status status = feature_relation_status::not_evaluated;
  std::uint64_t truth_begin = 0;
  std::uint64_t truth_count = 0;
  std::int32_t numeric_crossing_multiplicity = 0;
  std::uint32_t occurrence = 0;
  std::uint32_t reserved = 0;
};


struct relation_crossing_record final {
  feature_relation_id relation{0};
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
  std::uint32_t occurrence = 0;
  std::uint64_t source_fan_group = 0;
  std::uint32_t source_fan_group_size = 1;
  std::uint32_t source_fan_group_ordinal = 0;
  std::int8_t local_transition = 0;
  bool numeric_owner = false;
  bool source_fan_resolved = false;
  bool locally_conservative = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_event_seed_record final {
  relation_event_seed_id id{0};
  relation_event_seed_key key{};
  feature_relation_id source_relation{0};
  relation_construction_id construction{0};
  std::uint64_t incidence_begin = 0;
  std::uint64_t incidence_count = 0;
  bool distinct_occurrence_required = false;
  std::uint32_t reserved = 0;
};

struct relation_coplanar_event_lineage_record final {
  std::uint64_t contact_lineage = 0;
  std::uint8_t endpoint_role = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
};

struct relation_coplanar_event_occurrence_record final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t breakpoint_ordinal = 0;
  bool query_source_vertex_valid = false;
  std::uint64_t query_source_vertex = 0;
  std::vector<relation_coplanar_event_lineage_record> event_lineages;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_event_node_record final {
  relation_coplanar_event_node_id id{0};
  feature_relation_id overlay_relation{0};
  relation_construction_id representative{0};
  std::vector<relation_coplanar_event_occurrence_record> occurrences;
  std::uint8_t sheet_mask = 0;
  bool distinct_sheet_occurrences = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_arc_occurrence_record final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t interval_ordinal = 0;
  relation_coplanar_event_node_id start_node{0};
  relation_coplanar_event_node_id end_node{0};
  bool forward_along_source_edge = true;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_oriented_arc_record final {
  relation_coplanar_oriented_arc_id id{0};
  feature_relation_id overlay_relation{0};
  relation_coplanar_arc_kind kind =
      relation_coplanar_arc_kind::interior_boundary;
  relation_coplanar_event_node_id start_node{0};
  relation_coplanar_event_node_id end_node{0};
  std::vector<relation_coplanar_arc_occurrence_record> occurrences;
  std::vector<relation_request_id> overlap_lineages;
  std::uint8_t sheet_mask = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_overlap_component_record final {
  relation_coplanar_overlap_component_id id{0};
  feature_relation_id overlay_relation{0};
  relation_coplanar_component_kind kind =
      relation_coplanar_component_kind::isolated_point;
  std::vector<relation_coplanar_event_node_id> node_ids;
  std::vector<relation_coplanar_oriented_arc_id> arc_ids;
  std::uint8_t sheet_mask = 0;
  bool closed = false;
  bool distinct_sheet_occurrences = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_candidate_disposition_record final {
  relation_candidate_disposition_id id{0};
  candidate_id candidate{0};
  candidate_relation_disposition_kind disposition =
      candidate_relation_disposition_kind::no_public_relation;
  feature_relation_id public_relation{0};
  relation_request_id bookkeeping_request{0};
  std::uint32_t reserved = 0;
};

struct relation_verification_evidence final {
  relation_verifier_evidence_id id{0};
  std::uint16_t verifier_version = contract_versions::relation_verifier;
  bool graph_reconstructed = false;
  bool owner_exclusion_checked = false;
  bool selection_boundary_checked = false;
  bool candidate_dispositions_complete = false;
  std::uint64_t verifier_work_units = 0;
  bounded_boolean_digest semantic_digest{};
  std::uint32_t reserved = 0;
};

template <class T, class I> class signed_feature_relations final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t graph_policy_version() const noexcept {
    return graph_policy_version_;
  }
  std::uint16_t truth_policy_version() const noexcept {
    return truth_policy_version_;
  }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  relation_provider_kind provider() const noexcept { return provider_; }
  relation_verification_disposition verification() const noexcept {
    return verification_;
  }
  const context_owner_token &owner() const noexcept { return owner_; }
  const std::shared_ptr<const canonical_candidate_stream<T, I>> &candidates()
      const noexcept {
    return candidates_;
  }
  const relation_request_graph &request_graph() const noexcept {
    return request_graph_;
  }
  const std::shared_ptr<const candidate_source_edge_relation_stage<T>> &
  source_edge_stage() const noexcept { return source_edge_stage_; }
  const std::shared_ptr<const candidate_source_edge_facet_relation_stage<T>> &
  source_edge_facet_stage() const noexcept { return source_edge_facet_stage_; }
  const std::shared_ptr<const candidate_source_facet_relation_stage<T>> &
  source_facet_stage() const noexcept { return source_facet_stage_; }
  const std::shared_ptr<const candidate_coplanar_overlay_stage<T>> &
  coplanar_overlay_stage() const noexcept { return coplanar_overlay_stage_; }
  const std::vector<relation_truth_record> &truth_records() const noexcept {
    return truth_records_;
  }
  const std::vector<feature_relation_record> &relations() const noexcept {
    return relations_;
  }
  const std::vector<relation_construction_record> &constructions() const noexcept {
    return constructions_;
  }
  const std::vector<symbolic_eligibility_record> &symbolic_eligibility()
      const noexcept {
    return symbolic_eligibility_;
  }
  const std::vector<symbolic_relation_decision_record> &symbolic_decisions()
      const noexcept {
    return symbolic_decisions_;
  }
  const std::vector<relation_crossing_record> &crossings() const noexcept {
    return crossings_;
  }
  const std::vector<relation_event_seed_record> &event_seeds() const noexcept {
    return event_seeds_;
  }
  const std::vector<relation_coplanar_event_node_record> &
  coplanar_event_nodes() const noexcept { return coplanar_event_nodes_; }
  const std::vector<relation_coplanar_oriented_arc_record> &
  coplanar_oriented_arcs() const noexcept { return coplanar_oriented_arcs_; }
  const std::vector<relation_coplanar_overlap_component_record> &
  coplanar_overlap_components() const noexcept {
    return coplanar_overlap_components_;
  }
  const std::vector<relation_feature_key> &event_seed_incidence() const noexcept {
    return event_seed_incidence_;
  }
  const std::vector<relation_candidate_disposition_record> &
  candidate_dispositions() const noexcept {
    return candidate_dispositions_;
  }
  const relation_statistics &statistics() const noexcept { return statistics_; }
  const relation_verification_evidence &verification_evidence() const noexcept {
    return verification_evidence_;
  }
  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &candidate_digest() const noexcept {
    return candidate_digest_;
  }
  const bounded_boolean_digest &graph_digest() const noexcept {
    return graph_digest_;
  }
  boolean_operation operation() const noexcept { return operation_; }
  T residual_boundary() const noexcept { return residual_boundary_; }
  const bounded_boolean_digest &symbolic_policy_digest() const noexcept {
    return symbolic_policy_digest_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }

  const feature_relation_record *relation(
      feature_relation_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= relations_.size())
      return nullptr;
    return &relations_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ = contract_versions::relation_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::relation_provider;
  std::uint16_t graph_policy_version_ = contract_versions::relation_graph_policy;
  std::uint16_t truth_policy_version_ = contract_versions::relation_truth_policy;
  std::uint16_t codec_version_ = contract_versions::relation_codec;
  std::uint16_t verifier_version_ = contract_versions::relation_verifier;
  relation_provider_kind provider_ =
      relation_provider_kind::canonical_source_feature_relation_graph_v1;
  relation_verification_disposition verification_ =
      relation_verification_disposition::unverified;
  context_owner_token owner_{};
  std::shared_ptr<const canonical_candidate_stream<T, I>> candidates_;
  relation_request_graph request_graph_{};
  std::shared_ptr<const candidate_source_edge_relation_stage<T>> source_edge_stage_;
  std::shared_ptr<const candidate_source_edge_facet_relation_stage<T>> source_edge_facet_stage_;
  std::shared_ptr<const candidate_source_facet_relation_stage<T>> source_facet_stage_;
  std::shared_ptr<const candidate_coplanar_overlay_stage<T>> coplanar_overlay_stage_;
  std::vector<relation_truth_record> truth_records_;
  std::vector<feature_relation_record> relations_;
  std::vector<relation_construction_record> constructions_;
  std::vector<symbolic_eligibility_record> symbolic_eligibility_;
  std::vector<symbolic_relation_decision_record> symbolic_decisions_;
  std::vector<relation_crossing_record> crossings_;
  std::vector<relation_event_seed_record> event_seeds_;
  std::vector<relation_coplanar_event_node_record> coplanar_event_nodes_;
  std::vector<relation_coplanar_oriented_arc_record> coplanar_oriented_arcs_;
  std::vector<relation_coplanar_overlap_component_record>
      coplanar_overlap_components_;
  std::vector<relation_feature_key> event_seed_incidence_;
  std::vector<relation_candidate_disposition_record> candidate_dispositions_;
  relation_statistics statistics_{};
  relation_verification_evidence verification_evidence_{};
  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest candidate_digest_{};
  bounded_boolean_digest graph_digest_{};
  boolean_operation operation_ = boolean_operation::set_union;
  T residual_boundary_ = T(0);
  bounded_boolean_digest symbolic_policy_digest_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class relation_builder;
  template <class U, class J> friend class relation_artifact_assembler;
  friend struct relation_artifact_test_access;
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_signed_feature_relations(const signed_feature_relations<U, J> &);
  template <class U, class J>
  friend bool verify_relation_codec(
      const signed_feature_relations<U, J> &, bounded_boolean_error &);
  template <class U, class J>
  friend bool verify_signed_feature_relations(
      const signed_feature_relations<U, J> &, bounded_boolean_error &);
};

} // namespace ygor::mesh_boolean::bounded
