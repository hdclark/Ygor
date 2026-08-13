#pragma once

#include "OutputTopologyTypes.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct output_topology_artifact_test_access;
template <class T, class I> class output_topology_builder;

// Immutable polygonal output complex published by Component 11 and consumed by
// Component 12. Every table is a canonical contiguous vector; variable-length
// member sets are stored as (begin,count) ranges into parallel dense index
// vectors. References point only to immutable predecessor storage and to
// stage-owned buffers whose lifetime covers Components 12-15.
template <class T, class I> class polygonal_output_complex final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  output_topology_provider_kind provider() const noexcept { return provider_; }
  output_topology_verification_disposition verification() const noexcept {
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
  const std::array<bounded_boolean_digest, 2> &manifold_digests() const noexcept {
    return manifold_digests_;
  }
  const bounded_boolean_digest &relation_digest() const noexcept {
    return relation_digest_;
  }
  const bounded_boolean_digest &intersection_digest() const noexcept {
    return intersection_digest_;
  }
  const bounded_boolean_digest &classification_digest() const noexcept {
    return classification_digest_;
  }
  const bounded_boolean_digest &retained_digest() const noexcept {
    return retained_digest_;
  }
  const std::array<bounded_boolean_digest, 20> &section_digests() const noexcept {
    return section_digests_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const output_topology_statistics &statistics() const noexcept {
    return statistics_;
  }

  const std::vector<output_incidence_audit_record> &incidence_audits()
      const noexcept { return incidence_audits_; }
  const std::vector<zero_measure_support_evidence_record> &
  zero_measure_supports() const noexcept { return zero_measure_supports_; }
  const std::vector<output_coordinate_reference_record> &coordinate_references()
      const noexcept { return coordinate_references_; }
  const std::vector<output_vertex_occurrence_record> &vertex_occurrences()
      const noexcept { return vertex_occurrences_; }
  const std::vector<output_face_region_record> &face_regions() const noexcept {
    return face_regions_;
  }
  const std::vector<region_member_record> &region_members() const noexcept {
    return region_members_;
  }
  const std::vector<continuation_consumption_record> &continuation_consumptions()
      const noexcept { return continuation_consumptions_; }
  const std::vector<boundary_dart_record> &boundary_darts() const noexcept {
    return boundary_darts_;
  }
  const std::vector<paired_output_edge_record> &paired_edges() const noexcept {
    return paired_edges_;
  }
  const std::vector<output_halfedge_record> &halfedges() const noexcept {
    return halfedges_;
  }
  const std::vector<halfedge_endpoint_fan_ref_record> &endpoint_fan_refs()
      const noexcept { return endpoint_fan_refs_; }
  const std::vector<face_cycle_record> &face_cycles() const noexcept {
    return face_cycles_;
  }
  const std::vector<cycle_halfedge_ref_record> &cycle_halfedge_refs()
      const noexcept { return cycle_halfedge_refs_; }
  const std::vector<contour_node_record> &contour_nodes() const noexcept {
    return contour_nodes_;
  }
  const std::vector<contour_witness_record> &contour_witnesses() const noexcept {
    return contour_witnesses_;
  }
  const std::vector<zero_measure_boundary_record> &zero_measure_boundaries()
      const noexcept { return zero_measure_boundaries_; }
  const std::vector<admissibility_evidence_record> &admissibility_evidence()
      const noexcept { return admissibility_evidence_; }
  const std::vector<carrier_balance_audit_record> &carrier_balance_audits()
      const noexcept { return carrier_balance_audits_; }
  const std::vector<vertex_link_evidence_record> &vertex_link_evidence()
      const noexcept { return vertex_link_evidence_; }

  // Dense reverse maps and member ranges.
  const std::vector<std::uint64_t> &audit_by_incidence() const noexcept {
    return audit_by_incidence_;
  }
  const std::vector<std::uint64_t> &halfedge_by_incidence() const noexcept {
    return halfedge_by_incidence_;
  }
  const std::vector<std::uint64_t> &occurrence_by_endpoint_domain() const noexcept {
    return occurrence_by_endpoint_domain_;
  }
  const std::vector<std::uint64_t> &region_by_retained_use() const noexcept {
    return region_by_retained_use_;
  }
  const std::vector<std::uint64_t> &region_member_index() const noexcept {
    return region_member_index_;
  }
  const std::vector<std::uint64_t> &cycle_halfedge_index() const noexcept {
    return cycle_halfedge_index_;
  }
  const std::vector<std::uint64_t> &outgoing_halfedges() const noexcept {
    return outgoing_halfedges_;
  }
  const std::vector<std::uint64_t> &carrier_balance_members() const noexcept {
    return carrier_balance_members_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

  const output_vertex_occurrence_record *vertex_occurrence(
      output_vertex_occurrence_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= vertex_occurrences_.size())
      return nullptr;
    return &vertex_occurrences_[id.ordinal()];
  }

  const output_halfedge_record *halfedge(
      output_halfedge_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= halfedges_.size())
      return nullptr;
    return &halfedges_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ =
      contract_versions::output_topology_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::output_topology_provider;
  std::uint16_t codec_version_ = contract_versions::output_topology_codec;
  std::uint16_t verifier_version_ = contract_versions::output_topology_verifier;
  output_topology_provider_kind provider_ =
      output_topology_provider_kind::paired_boundary_face_cycle_construction_v1;
  output_topology_verification_disposition verification_ =
      output_topology_verification_disposition::not_verified;
  boolean_operation operation_ = boolean_operation::set_union;
  context_owner_token owner_{};

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  std::array<bounded_boolean_digest, 2> manifold_digests_{};
  bounded_boolean_digest relation_digest_{};
  bounded_boolean_digest intersection_digest_{};
  bounded_boolean_digest classification_digest_{};
  bounded_boolean_digest retained_digest_{};
  std::array<bounded_boolean_digest, 20> section_digests_{};

  std::vector<output_incidence_audit_record> incidence_audits_;
  std::vector<zero_measure_support_evidence_record> zero_measure_supports_;
  std::vector<output_coordinate_reference_record> coordinate_references_;
  std::vector<output_vertex_occurrence_record> vertex_occurrences_;
  std::vector<output_face_region_record> face_regions_;
  std::vector<region_member_record> region_members_;
  std::vector<continuation_consumption_record> continuation_consumptions_;
  std::vector<boundary_dart_record> boundary_darts_;
  std::vector<paired_output_edge_record> paired_edges_;
  std::vector<output_halfedge_record> halfedges_;
  std::vector<halfedge_endpoint_fan_ref_record> endpoint_fan_refs_;
  std::vector<face_cycle_record> face_cycles_;
  std::vector<cycle_halfedge_ref_record> cycle_halfedge_refs_;
  std::vector<contour_node_record> contour_nodes_;
  std::vector<contour_witness_record> contour_witnesses_;
  std::vector<zero_measure_boundary_record> zero_measure_boundaries_;
  std::vector<admissibility_evidence_record> admissibility_evidence_;
  std::vector<carrier_balance_audit_record> carrier_balance_audits_;
  std::vector<vertex_link_evidence_record> vertex_link_evidence_;

  std::vector<std::uint64_t> audit_by_incidence_;
  std::vector<std::uint64_t> halfedge_by_incidence_;
  std::vector<std::uint64_t> occurrence_by_endpoint_domain_;
  std::vector<std::uint64_t> region_by_retained_use_;
  std::vector<std::uint64_t> region_member_index_;
  std::vector<std::uint64_t> cycle_halfedge_index_;
  std::vector<std::uint64_t> outgoing_halfedges_;
  std::vector<std::uint64_t> carrier_balance_members_;

  output_topology_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class output_topology_builder;
  friend struct output_topology_artifact_test_access;
};

// Narrow immutable views for Components 12, 15, diagnostics, and tests.
template <class T, class I> class polygonal_output_complex_view final {
public:
  explicit polygonal_output_complex_view(
      const polygonal_output_complex<T, I> &complex,
      const context_owner_token &owner) noexcept
      : complex_(&complex), owner_(owner) {}
  bool valid() const noexcept {
    return complex_ && complex_->owner().same_owner(owner_);
  }
  const polygonal_output_complex<T, I> *get() const noexcept {
    return valid() ? complex_ : nullptr;
  }
  std::uint64_t vertex_occurrence_count() const noexcept {
    return valid() ? complex_->vertex_occurrences().size() : 0;
  }
  std::uint64_t halfedge_count() const noexcept {
    return valid() ? complex_->halfedges().size() : 0;
  }
  std::uint64_t face_region_count() const noexcept {
    return valid() ? complex_->face_regions().size() : 0;
  }
  std::uint64_t face_cycle_count() const noexcept {
    return valid() ? complex_->face_cycles().size() : 0;
  }

private:
  const polygonal_output_complex<T, I> *complex_;
  context_owner_token owner_;
};

} // namespace ygor::mesh_boolean::bounded
