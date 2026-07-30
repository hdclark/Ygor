#pragma once

#include "IntersectionKeys.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct intersection_artifact_test_access;
template <class T, class I> class intersection_builder;

struct intersection_range final {
  std::uint64_t begin = 0;
  std::uint64_t count = 0;
};

struct bounded_point_reference final {
  bounded_point_reference_kind kind =
      bounded_point_reference_kind::constructed_point;
  relation_feature_key source_vertex{};
  relation_construction_id construction{0};
  relation_construction_ledger_id precision_ledger{0};
  std::uint16_t schema_version = contract_versions::intersection_event_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct intersection_event_record final {
  event_id id{0};
  intersection_event_key key{};
  bounded_point_reference point{};
  intersection_range construction_witnesses{};
  intersection_range occurrences{};
  intersection_range seed_bindings{};
  intersection_range incidence{};
  intersection_range crossing_aggregates{};
  intersection_range contact_aggregates{};
  std::uint16_t schema_version = contract_versions::intersection_event_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct intersection_occurrence_record final {
  event_occurrence_id id{0};
  event_id event{0};
  intersection_occurrence_key key{};
  intersection_range seed_bindings{};
  intersection_range incidence{};
  intersection_range source_edge_memberships{};
  intersection_range carrier_memberships{};
  intersection_range cluster_memberships{};
  intersection_range aggregate_contributions{};
  intersection_range descriptors{};
  bool may_share_output_coordinate = true;
  bool topology_separate = false;
  bool local_cluster_compatible = false;
  bool requires_contact_separation = false;
  std::uint16_t schema_version = contract_versions::intersection_occurrence_schema;
  std::uint16_t reserved16 = 0;
};

struct event_seed_binding_record final {
  event_seed_binding_id id{0};
  relation_event_seed_id seed{0};
  relation_event_seed_key seed_key{};
  std::uint64_t canonical_seed_ordinal = 0;
  event_id event{0};
  event_occurrence_id occurrence{0};
  feature_relation_id relation{0};
  relation_construction_id construction{0};
  relation_feature_key accepted_source_vertex{};
  intersection_range incidence{};
  intersection_range candidate_incidence{};
  intersection_membership_role expected_membership_role =
      intersection_membership_role::interior;
  intersection_carrier_role expected_carrier_role =
      intersection_carrier_role::none;
  bool designated_authority = false;
  bool duplicate_consumer = false;
  bool compatibility_verified = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version =
      contract_versions::intersection_seed_binding_schema;
  std::uint32_t reserved32 = 0;
};

struct event_incidence_record final {
  event_incidence_id id{0};
  event_incidence_key key{};
  event_id event{0};
  event_occurrence_id occurrence{0};
  event_seed_binding_id seed_binding{0};
  event_incidence_kind kind = event_incidence_kind::source_vertex;
  relation_feature_key feature{};
  feature_relation_id relation{intersection_invalid_ordinal};
  candidate_id candidate{intersection_invalid_ordinal};
  std::uint64_t payload_primary = 0;
  std::uint64_t payload_secondary = 0;
  std::uint32_t payload_occurrence = 0;
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  std::int8_t orientation = 0;
  bool source_feature_owner = false;
  bool bookkeeping_only = false;
  std::uint16_t schema_version = contract_versions::intersection_incidence_schema;
  std::uint16_t reserved16 = 0;
};

struct source_feature_incidence_range_record final {
  event_incidence_kind kind = event_incidence_kind::source_vertex;
  relation_feature_key feature{};
  intersection_range incidence{};
  std::uint16_t schema_version = contract_versions::intersection_incidence_schema;
  std::uint16_t reserved16 = 0;
};

struct oriented_halfedge_incidence_range_record final {
  operand_id operand = operand_id::a;
  std::uint64_t halfedge = 0;
  intersection_range incidence{};
  std::uint16_t schema_version = contract_versions::intersection_incidence_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct source_edge_membership_record final {
  source_edge_membership_id id{0};
  source_edge_membership_key key{};
  event_occurrence_id occurrence{0};
  event_id event{0};
  relation_interval_evidence_id parameter{0};
  intersection_range contributions{};
  intersection_range incident_facet_uses{};
  ordering_certificate_id ordering_certificate{intersection_invalid_ordinal};
  bool exact_equal_eligible = false;
  bool cluster_eligible = false;
  bool internal_diagonal_discovery = false;
  bool bookkeeping_only = false;
  std::uint16_t schema_version =
      contract_versions::intersection_source_edge_membership_schema;
  std::uint16_t reserved16 = 0;
};

struct source_edge_endpoint_sentinel_record final {
  source_edge_sentinel_side side = source_edge_sentinel_side::start;
  relation_feature_key source_vertex{};
  relation_feature_key source_edge{};
};

struct source_edge_sequence_record final {
  source_edge_sequence_id id{0};
  relation_feature_key source_edge{};
  source_edge_endpoint_sentinel_record start{};
  source_edge_endpoint_sentinel_record end{};
  intersection_range clusters{};
  intersection_range memberships{};
  intersection_range intervals{};
  intersection_range aggregates{};
  intersection_range descriptors{};
  bounded_boolean_digest sequence_digest{};
  std::uint64_t comparison_count = 0;
  bool canonical_forward = true;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version =
      contract_versions::intersection_source_edge_sequence_schema;
  std::uint32_t reserved32 = 0;
};

struct source_edge_cluster_record final {
  source_edge_cluster_id id{0};
  source_edge_sequence_id sequence{0};
  source_edge_cluster_key key{};
  intersection_range member_occurrences{};
  intersection_range membership_ids{};
  intersection_range contributions{};
  source_edge_cluster_id predecessor{intersection_invalid_ordinal};
  source_edge_cluster_id successor{intersection_invalid_ordinal};
  ordering_certificate_id ordering_certificate{intersection_invalid_ordinal};
  bool shared_output_coordinate = false;
  bool separate_output_occurrences = false;
  std::uint16_t schema_version = contract_versions::intersection_cluster_schema;
};

struct source_edge_interval_record final {
  source_edge_interval_id id{0};
  source_edge_sequence_id sequence{0};
  source_edge_interval_key key{};
  relation_interval_evidence_id left_parameter{intersection_invalid_ordinal};
  relation_interval_evidence_id right_parameter{intersection_invalid_ordinal};
  intersection_interval_length length_disposition =
      intersection_interval_length::definitely_positive;
  std::int32_t left_crossing_delta = 0;
  std::int32_t right_crossing_delta = 0;
  std::int32_t accumulated_crossing = 0;
  bool propagation_allowed = false;
  bool retention_allowed = false;
  bool split_required = false;
  bool duplicate_required = false;
  intersection_range provenance{};
  intersection_range descriptors{};
  std::uint16_t schema_version =
      contract_versions::intersection_source_edge_interval_schema;
  std::uint16_t reserved16 = 0;
};

struct transverse_carrier_record final {
  transverse_carrier_id id{0};
  transverse_carrier_key key{};
  relation_construction_id construction{0};
  intersection_range relation_provenance{};
  intersection_range candidate_provenance{};
  intersection_range memberships{};
  intersection_range clusters{};
  intersection_range active_spans{};
  intersection_range region_incidence{};
  intersection_range aggregates{};
  intersection_range descriptors{};
  bounded_boolean_digest carrier_digest{};
  std::uint16_t schema_version = contract_versions::intersection_carrier_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct carrier_membership_record final {
  carrier_membership_id id{0};
  transverse_carrier_id carrier{0};
  event_occurrence_id occurrence{0};
  event_id event{0};
  relation_interval_evidence_id parameter{0};
  std::uint64_t relation_lineage = 0;
  ordering_certificate_id ordering_certificate{intersection_invalid_ordinal};
  std::uint16_t schema_version =
      contract_versions::intersection_carrier_membership_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct carrier_cluster_record final {
  carrier_cluster_id id{0};
  transverse_carrier_id carrier{0};
  intersection_cluster_equivalence equivalence =
      intersection_cluster_equivalence::exact_parameter_coincidence;
  intersection_range occurrence_members{};
  intersection_range membership_members{};
  carrier_cluster_id predecessor{intersection_invalid_ordinal};
  carrier_cluster_id successor{intersection_invalid_ordinal};
  ordering_certificate_id ordering_certificate{intersection_invalid_ordinal};
  bool shared_output_coordinate = false;
  bool separate_output_occurrences = false;
  std::uint16_t schema_version = contract_versions::intersection_cluster_schema;
};

struct carrier_active_span_record final {
  carrier_active_span_id id{0};
  transverse_carrier_id carrier{0};
  carrier_cluster_id left{0};
  carrier_cluster_id right{0};
  intersection_span_activation activation =
      intersection_span_activation::inactive;
  std::uint64_t relation_interval_lineage = 0;
  intersection_range region_incidence{};
  intersection_range provenance{};
  bool classification_cut = false;
  bool contact_delimiter = false;
  bool output_edge_allowed = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version = contract_versions::intersection_carrier_schema;
};

struct coplanar_support_record final {
  coplanar_support_id id{0};
  relation_feature_key first_facet{};
  relation_feature_key second_facet{};
  std::uint64_t support_lineage = 0;
  bool opposite_orientation = false;
  operand_id symbolic_owner = operand_id::a;
  intersection_range boundary_carriers{};
  intersection_range overlap_components{};
  intersection_range region_incidence{};
  intersection_range provenance{};
  std::uint16_t schema_version = contract_versions::intersection_overlap_schema;
  std::uint16_t reserved16 = 0;
};

struct collinear_overlap_carrier_record final {
  collinear_overlap_carrier_id id{0};
  collinear_overlap_carrier_key key{};
  relation_construction_id first_parameter_interval{0};
  relation_construction_id second_parameter_interval{0};
  event_occurrence_id start_occurrence{0};
  event_occurrence_id end_occurrence{0};
  operand_id symbolic_owner = operand_id::a;
  bool half_open_first = false;
  bool half_open_second = false;
  bool separate_sheet_required = false;
  std::uint8_t reserved8 = 0;
  intersection_range provenance{};
  intersection_range contributions{};
  intersection_range descriptors{};
  std::uint16_t schema_version = contract_versions::intersection_overlap_schema;
  std::uint16_t reserved16 = 0;
};

struct coplanar_region_incidence_record final {
  coplanar_region_incidence_id id{0};
  coplanar_support_id support{0};
  relation_feature_key first_facet{};
  relation_feature_key second_facet{};
  relation_feature_key first_triangle{};
  relation_feature_key second_triangle{};
  std::uint64_t component_lineage = 0;
  feature_relation_status relation_status =
      feature_relation_status::not_evaluated;
  operand_id symbolic_owner = operand_id::a;
  intersection_range boundary_events{};
  intersection_range boundary_carriers{};
  bounded_boolean_digest source_facet_semantic_digest{};
  std::uint16_t schema_version = contract_versions::intersection_overlap_schema;
  std::uint16_t reserved16 = 0;
};

struct crossing_aggregate_record final {
  crossing_aggregate_id id{0};
  intersection_aggregate_locus locus =
      intersection_aggregate_locus::event_occurrence;
  std::uint64_t locus_ordinal = 0;
  std::int64_t numeric_signed_sum = 0;
  std::int32_t symbolic_signed_sum = 0;
  operand_id symbolic_owner = operand_id::a;
  intersection_range members{};
  intersection_range facet_subtotals{};
  intersection_range shell_subtotals{};
  bool conserved = false;
  std::uint16_t schema_version = contract_versions::intersection_aggregate_schema;
  std::uint8_t reserved8 = 0;
};

struct contact_aggregate_record final {
  contact_aggregate_id id{0};
  intersection_aggregate_locus locus =
      intersection_aggregate_locus::event_occurrence;
  std::uint64_t locus_ordinal = 0;
  feature_relation_status contact_status = feature_relation_status::point_contact;
  relation_contact_dimension contact_dimension = relation_contact_dimension::point;
  intersection_range members{};
  bool zero_net_retained = false;
  bool tangent_retained = false;
  bool coincidence_retained = false;
  bool reconstructed = false;
  std::uint16_t schema_version = contract_versions::intersection_aggregate_schema;
  std::uint16_t reserved16 = 0;
};

struct intersection_descriptor_record final {
  intersection_descriptor_id id{0};
  intersection_descriptor_key key{};
  std::int32_t signed_crossing_delta = 0;
  operand_id symbolic_owner = operand_id::a;
  std::uint64_t symbolic_rule_ordinal = 0;
  intersection_range provenance{};
  bool continuation_allowed = false;
  bool occurrence_separation_required = false;
  bool classification_consumable = false;
  bool selection_consumable = false;
  bool topology_consumable = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version = contract_versions::intersection_descriptor_schema;
};

struct ordering_certificate_record final {
  ordering_certificate_id id{0};
  intersection_order_disposition disposition =
      intersection_order_disposition::invalid;
  relation_interval_evidence_id first_parameter{intersection_invalid_ordinal};
  relation_interval_evidence_id second_parameter{intersection_invalid_ordinal};
  std::uint64_t exact_evidence_lineage = 0;
  std::uint64_t comparison_evidence_lineage = 0;
  bool topology_safe = false;
  std::uint16_t policy_version =
      contract_versions::intersection_bounded_ordering_policy;
  std::uint8_t reserved8 = 0;
};

struct intersection_verification_evidence final {
  std::uint16_t schema_version =
      contract_versions::intersection_exhaustive_evidence_schema;
  std::uint16_t verifier_version = contract_versions::intersection_verifier;
  bool seed_regrouped = false;
  bool incidence_reconstructed = false;
  bool arrangements_reconstructed = false;
  bool descriptors_reconstructed = false;
  bool exhaustive_mode = false;
  std::uint8_t reserved8 = 0;
  bounded_boolean_digest reconstructed_digest{};
  std::uint64_t work_units = 0;
};

struct intersection_diagnostic_record final {
  intersection_diagnostic_id id{0};
  intersection_checkpoint checkpoint =
      intersection_checkpoint::context_capability_validation;
  intersection_subcode subcode = intersection_subcode::internal_invariant;
  std::array<std::uint64_t, 4> witnesses{};
  std::uint8_t witness_count = 0;
  bool retained_finding = true;
  std::uint16_t schema_version = contract_versions::intersection_diagnostic_schema;
  std::uint32_t reserved32 = 0;
};

struct intersection_replay_checkpoint_record final {
  intersection_replay_checkpoint_id id{0};
  intersection_checkpoint checkpoint =
      intersection_checkpoint::context_capability_validation;
  bounded_boolean_digest input_digest{};
  bounded_boolean_digest output_digest{};
  std::uint64_t input_count = 0;
  std::uint64_t output_count = 0;
  std::uint64_t cumulative_work_units = 0;
  std::uint16_t schema_version = contract_versions::intersection_replay_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T, class I> class canonical_intersection_complex final {
public:
  const context_owner_token &owner() const noexcept { return owner_; }
  boolean_operation operation() const noexcept { return operation_; }
  intersection_provider_kind provider() const noexcept { return provider_; }
  intersection_verification_disposition verification() const noexcept {
    return verification_;
  }
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t semantic_policy_version() const noexcept {
    return semantic_policy_version_;
  }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }

  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &relation_digest() const noexcept {
    return relation_digest_;
  }
  const std::array<bounded_boolean_digest, 2> &source_semantic_digests()
      const noexcept { return source_semantic_digests_; }
  const std::array<bounded_boolean_digest, 2> &exact_triangulation_digests()
      const noexcept { return exact_triangulation_digests_; }
  const std::array<bounded_boolean_digest, 8> &section_digests() const noexcept {
    return section_digests_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const intersection_statistics &statistics() const noexcept {
    return statistics_;
  }
  const intersection_verification_evidence &verification_evidence()
      const noexcept { return verification_evidence_; }

  const std::vector<intersection_event_record> &events() const noexcept {
    return events_;
  }
  const std::vector<intersection_occurrence_record> &occurrences() const noexcept {
    return occurrences_;
  }
  const std::vector<event_seed_binding_record> &seed_bindings() const noexcept {
    return seed_bindings_;
  }
  const std::vector<event_incidence_record> &incidence() const noexcept {
    return incidence_;
  }
  const std::vector<event_incidence_id> &incidence_by_event() const noexcept {
    return incidence_by_event_;
  }
  const std::vector<event_incidence_id> &incidence_by_occurrence() const noexcept {
    return incidence_by_occurrence_;
  }
  const std::vector<event_incidence_id> &incidence_by_seed() const noexcept {
    return incidence_by_seed_;
  }
  const std::vector<event_incidence_id> &incidence_by_seed_candidate()
      const noexcept { return incidence_by_seed_candidate_; }
  const std::vector<event_incidence_id> &incidence_by_relation() const noexcept {
    return incidence_by_relation_;
  }
  const std::vector<intersection_range> &relation_incidence_ranges()
      const noexcept { return relation_incidence_ranges_; }
  const std::vector<event_incidence_id> &incidence_by_candidate() const noexcept {
    return incidence_by_candidate_;
  }
  const std::vector<intersection_range> &candidate_incidence_ranges()
      const noexcept { return candidate_incidence_ranges_; }
  const std::vector<event_incidence_id> &incidence_by_source_feature() const noexcept {
    return incidence_by_source_feature_;
  }
  const std::vector<source_feature_incidence_range_record> &
  source_feature_incidence_ranges() const noexcept {
    return source_feature_incidence_ranges_;
  }
  const std::vector<event_incidence_id> &incidence_by_halfedge() const noexcept {
    return incidence_by_halfedge_;
  }
  const std::vector<oriented_halfedge_incidence_range_record> &
  halfedge_incidence_ranges() const noexcept {
    return halfedge_incidence_ranges_;
  }
  const std::vector<source_edge_membership_record> &source_edge_memberships()
      const noexcept { return source_edge_memberships_; }
  const std::vector<source_edge_sequence_record> &source_edge_sequences()
      const noexcept { return source_edge_sequences_; }
  const std::vector<source_edge_cluster_record> &source_edge_clusters()
      const noexcept { return source_edge_clusters_; }
  const std::vector<source_edge_interval_record> &source_edge_intervals()
      const noexcept { return source_edge_intervals_; }
  const std::vector<transverse_carrier_record> &transverse_carriers()
      const noexcept { return transverse_carriers_; }
  const std::vector<carrier_membership_record> &carrier_memberships()
      const noexcept { return carrier_memberships_; }
  const std::vector<carrier_cluster_record> &carrier_clusters() const noexcept {
    return carrier_clusters_;
  }
  const std::vector<carrier_active_span_record> &carrier_active_spans()
      const noexcept { return carrier_active_spans_; }
  const std::vector<coplanar_support_record> &coplanar_supports() const noexcept {
    return coplanar_supports_;
  }
  const std::vector<collinear_overlap_carrier_record> &overlap_carriers()
      const noexcept { return overlap_carriers_; }
  const std::vector<coplanar_region_incidence_record> &coplanar_region_incidence()
      const noexcept { return coplanar_region_incidence_; }
  const std::vector<crossing_aggregate_record> &crossing_aggregates()
      const noexcept { return crossing_aggregates_; }
  const std::vector<contact_aggregate_record> &contact_aggregates()
      const noexcept { return contact_aggregates_; }
  const std::vector<intersection_descriptor_record> &descriptors() const noexcept {
    return descriptors_;
  }
  const std::vector<ordering_certificate_record> &ordering_certificates()
      const noexcept { return ordering_certificates_; }
  const std::vector<intersection_diagnostic_record> &diagnostics() const noexcept {
    return diagnostics_;
  }
  const std::vector<intersection_replay_checkpoint_record> &replay_checkpoints()
      const noexcept { return replay_checkpoints_; }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

  const intersection_event_record *event(
      event_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= events_.size())
      return nullptr;
    return &events_[id.ordinal()];
  }
  const intersection_occurrence_record *occurrence(
      event_occurrence_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= occurrences_.size())
      return nullptr;
    return &occurrences_[id.ordinal()];
  }

private:
  context_owner_token owner_{};
  boolean_operation operation_ = boolean_operation::set_union;
  intersection_provider_kind provider_ =
      intersection_provider_kind::canonical_lineage_event_arrangement_v1;
  intersection_verification_disposition verification_ =
      intersection_verification_disposition::not_verified;
  std::uint16_t schema_version_ = contract_versions::intersection_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::intersection_provider;
  std::uint16_t semantic_policy_version_ =
      contract_versions::intersection_semantic_policy;
  std::uint16_t codec_version_ = contract_versions::intersection_codec;
  std::uint16_t verifier_version_ = contract_versions::intersection_verifier;
  std::uint32_t reserved_ = 0;

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest relation_digest_{};
  std::array<bounded_boolean_digest, 2> source_semantic_digests_{};
  std::array<bounded_boolean_digest, 2> exact_triangulation_digests_{};
  std::array<bounded_boolean_digest, 8> section_digests_{};

  std::vector<intersection_event_record> events_{};
  std::vector<intersection_occurrence_record> occurrences_{};
  std::vector<event_seed_binding_record> seed_bindings_{};
  std::vector<event_incidence_record> incidence_{};
  std::vector<event_incidence_id> incidence_by_event_{};
  std::vector<event_incidence_id> incidence_by_occurrence_{};
  std::vector<event_incidence_id> incidence_by_seed_{};
  std::vector<event_incidence_id> incidence_by_seed_candidate_{};
  std::vector<event_incidence_id> incidence_by_relation_{};
  std::vector<intersection_range> relation_incidence_ranges_{};
  std::vector<event_incidence_id> incidence_by_candidate_{};
  std::vector<intersection_range> candidate_incidence_ranges_{};
  std::vector<event_incidence_id> incidence_by_source_feature_{};
  std::vector<source_feature_incidence_range_record>
      source_feature_incidence_ranges_{};
  std::vector<event_incidence_id> incidence_by_halfedge_{};
  std::vector<oriented_halfedge_incidence_range_record>
      halfedge_incidence_ranges_{};
  std::vector<source_edge_membership_record> source_edge_memberships_{};
  std::vector<source_edge_sequence_record> source_edge_sequences_{};
  std::vector<source_edge_cluster_record> source_edge_clusters_{};
  std::vector<source_edge_interval_record> source_edge_intervals_{};
  std::vector<transverse_carrier_record> transverse_carriers_{};
  std::vector<carrier_membership_record> carrier_memberships_{};
  std::vector<carrier_cluster_record> carrier_clusters_{};
  std::vector<carrier_active_span_record> carrier_active_spans_{};
  std::vector<coplanar_support_record> coplanar_supports_{};
  std::vector<collinear_overlap_carrier_record> overlap_carriers_{};
  std::vector<coplanar_region_incidence_record> coplanar_region_incidence_{};
  std::vector<crossing_aggregate_record> crossing_aggregates_{};
  std::vector<contact_aggregate_record> contact_aggregates_{};
  std::vector<intersection_descriptor_record> descriptors_{};
  std::vector<ordering_certificate_record> ordering_certificates_{};
  std::vector<intersection_diagnostic_record> diagnostics_{};
  std::vector<intersection_replay_checkpoint_record> replay_checkpoints_{};

  intersection_statistics statistics_{};
  intersection_verification_evidence verification_evidence_{};
  std::vector<std::uint8_t> canonical_bytes_{};
  bounded_boolean_digest digest_{};

  friend struct intersection_artifact_test_access;
  template <class U, class J> friend class intersection_builder;
};

} // namespace ygor::mesh_boolean::bounded
