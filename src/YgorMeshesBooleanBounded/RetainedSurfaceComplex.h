#pragma once

#include "SelectionTypes.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct selection_artifact_test_access;
template <class T, class I> class selection_builder;

// Immutable retained-surface complex published by Component 10 and consumed by
// Component 11. Every table is a canonical contiguous vector; variable-length
// member sets are stored as (begin,count) ranges into parallel dense index
// vectors. References point only to immutable predecessor storage and to
// stage-owned buffers whose lifetime covers Components 11-15.
template <class T, class I> class retained_surface_complex final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  selection_provider_kind provider() const noexcept { return provider_; }
  selection_verification_disposition verification() const noexcept {
    return verification_;
  }
  boolean_operation operation() const noexcept { return operation_; }
  const context_owner_token &owner() const noexcept { return owner_; }

  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const std::array<bounded_boolean_digest, 2> &manifold_digests()
      const noexcept { return manifold_digests_; }
  const bounded_boolean_digest &relation_digest() const noexcept {
    return relation_digest_;
  }
  const bounded_boolean_digest &intersection_digest() const noexcept {
    return intersection_digest_;
  }
  const bounded_boolean_digest &classification_digest() const noexcept {
    return classification_digest_;
  }
  const std::array<bounded_boolean_digest, 10> &section_digests()
      const noexcept { return section_digests_; }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const selection_statistics &statistics() const noexcept { return statistics_; }

  const std::vector<selection_disposition_record> &dispositions() const noexcept {
    return dispositions_;
  }
  const std::vector<selection_side_tuple_record> &side_tuples() const noexcept {
    return side_tuples_;
  }
  const std::vector<selection_truth_record> &truth_records() const noexcept {
    return truth_records_;
  }
  const std::vector<coincidence_sheet_cell_record> &sheet_cells() const noexcept {
    return sheet_cells_;
  }
  const std::vector<coincidence_member_record> &coincidence_members()
      const noexcept { return coincidence_members_; }
  const std::vector<owner_decision_record> &owner_decisions() const noexcept {
    return owner_decisions_;
  }
  const std::vector<suppression_record> &suppressions() const noexcept {
    return suppressions_;
  }
  const std::vector<multiplicity_record> &multiplicities() const noexcept {
    return multiplicities_;
  }
  const std::vector<retained_surface_use_record> &retained_uses() const noexcept {
    return retained_uses_;
  }
  const std::vector<retained_use_provenance_record> &retained_provenance()
      const noexcept { return retained_provenance_; }
  const std::vector<retained_incidence_record> &incidences() const noexcept {
    return incidences_;
  }
  const std::vector<continuation_record> &continuations() const noexcept {
    return continuations_;
  }
  const std::vector<edge_mate_group_record> &edge_mate_groups() const noexcept {
    return edge_mate_groups_;
  }
  const std::vector<planned_edge_occurrence_record> &planned_edges() const noexcept {
    return planned_edges_;
  }
  const std::vector<carrier_balance_record> &carrier_balances() const noexcept {
    return carrier_balances_;
  }
  const std::vector<endpoint_domain_record> &endpoint_domains() const noexcept {
    return endpoint_domains_;
  }
  const std::vector<local_port_record> &local_ports() const noexcept {
    return local_ports_;
  }
  const std::vector<face_corner_arc_record> &face_corner_arcs() const noexcept {
    return face_corner_arcs_;
  }
  const std::vector<edge_mate_arc_record> &edge_mate_arcs() const noexcept {
    return edge_mate_arcs_;
  }
  const std::vector<local_link_component_record> &link_components() const noexcept {
    return link_components_;
  }
  const std::vector<vertex_occurrence_requirement_record> &
  vertex_occurrences() const noexcept { return vertex_occurrences_; }

  // Dense reverse maps and member ranges.
  const std::vector<std::uint64_t> &disposition_by_atom() const noexcept {
    return disposition_by_atom_;
  }
  const std::vector<std::uint64_t> &retained_use_by_atom() const noexcept {
    return retained_use_by_atom_;
  }
  const std::vector<std::uint64_t> &cell_members() const noexcept {
    return cell_members_;
  }
  const std::vector<std::uint64_t> &balance_members() const noexcept {
    return balance_members_;
  }
  const std::vector<std::uint64_t> &link_cycle_ports() const noexcept {
    return link_cycle_ports_;
  }
  const std::vector<std::uint64_t> &incidence_by_retained_use() const noexcept {
    return incidence_by_retained_use_;
  }
  const std::vector<std::uint64_t> &port_by_endpoint_domain() const noexcept {
    return port_by_endpoint_domain_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

  const retained_surface_use_record *retained_use(
      retained_surface_use_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= retained_uses_.size())
      return nullptr;
    return &retained_uses_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ = contract_versions::selection_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::selection_provider;
  std::uint16_t codec_version_ = contract_versions::selection_codec;
  std::uint16_t verifier_version_ = contract_versions::selection_verifier;
  selection_provider_kind provider_ =
      selection_provider_kind::side_truth_and_lineage_selection_v1;
  selection_verification_disposition verification_ =
      selection_verification_disposition::not_verified;
  boolean_operation operation_ = boolean_operation::set_union;
  context_owner_token owner_{};

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  std::array<bounded_boolean_digest, 2> manifold_digests_{};
  bounded_boolean_digest relation_digest_{};
  bounded_boolean_digest intersection_digest_{};
  bounded_boolean_digest classification_digest_{};
  std::array<bounded_boolean_digest, 10> section_digests_{};

  std::vector<selection_disposition_record> dispositions_;
  std::vector<selection_side_tuple_record> side_tuples_;
  std::vector<selection_truth_record> truth_records_;
  std::vector<coincidence_sheet_cell_record> sheet_cells_;
  std::vector<coincidence_member_record> coincidence_members_;
  std::vector<owner_decision_record> owner_decisions_;
  std::vector<suppression_record> suppressions_;
  std::vector<multiplicity_record> multiplicities_;
  std::vector<retained_surface_use_record> retained_uses_;
  std::vector<retained_use_provenance_record> retained_provenance_;
  std::vector<retained_incidence_record> incidences_;
  std::vector<continuation_record> continuations_;
  std::vector<edge_mate_group_record> edge_mate_groups_;
  std::vector<planned_edge_occurrence_record> planned_edges_;
  std::vector<carrier_balance_record> carrier_balances_;
  std::vector<endpoint_domain_record> endpoint_domains_;
  std::vector<local_port_record> local_ports_;
  std::vector<face_corner_arc_record> face_corner_arcs_;
  std::vector<edge_mate_arc_record> edge_mate_arcs_;
  std::vector<local_link_component_record> link_components_;
  std::vector<vertex_occurrence_requirement_record> vertex_occurrences_;

  std::vector<std::uint64_t> disposition_by_atom_;
  std::vector<std::uint64_t> retained_use_by_atom_;
  std::vector<std::uint64_t> cell_members_;
  std::vector<std::uint64_t> balance_members_;
  std::vector<std::uint64_t> link_cycle_ports_;
  std::vector<std::uint64_t> incidence_by_retained_use_;
  std::vector<std::uint64_t> port_by_endpoint_domain_;

  selection_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class selection_builder;
  friend struct selection_artifact_test_access;
};

} // namespace ygor::mesh_boolean::bounded
