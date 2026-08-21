#pragma once

#include "OutputTriangulationTypes.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct output_triangulation_artifact_test_access;
template <class T, class I> class output_triangulation_builder;

// Immutable triangulated output complex published by Component 12 and consumed
// by Component 13. All Component 11 output occurrences, coordinates, and
// boundary halfedges are referenced unchanged; Component 12 owns only internal
// diagonals, internal halfedges, triangles, residuals, and evidence.
template <class T, class I> class triangulated_output_complex final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  output_triangulation_provider_kind provider() const noexcept {
    return provider_;
  }
  triangulation_verification_disposition verification() const noexcept {
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
  const bounded_boolean_digest &intersection_digest() const noexcept {
    return intersection_digest_;
  }
  const bounded_boolean_digest &retained_digest() const noexcept {
    return retained_digest_;
  }
  const bounded_boolean_digest &polygonal_digest() const noexcept {
    return polygonal_digest_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const output_triangulation_statistics &statistics() const noexcept {
    return statistics_;
  }

  const std::vector<support_frame_record> &support_frames() const noexcept {
    return support_frames_;
  }
  const std::vector<projected_occurrence_record> &projected_occurrences()
      const noexcept { return projected_occurrences_; }
  const std::vector<planar_predicate_evidence_record> &predicate_evidence()
      const noexcept { return predicate_evidence_; }
  const std::vector<orientation_escalation_record> &escalations() const noexcept {
    return escalations_;
  }
  const std::vector<internal_diagonal_record> &diagonals() const noexcept {
    return diagonals_;
  }
  const std::vector<triangulation_halfedge_record> &internal_halfedges()
      const noexcept { return internal_halfedges_; }
  const std::vector<output_triangle_record> &triangles() const noexcept {
    return triangles_;
  }
  const std::vector<triangle_corner_ref_record> &corner_refs() const noexcept {
    return corner_refs_;
  }
  const std::vector<triangle_edge_use_ref_record> &edge_use_refs() const noexcept {
    return edge_use_refs_;
  }
  const std::vector<boundary_assignment_record> &boundary_assignments()
      const noexcept { return boundary_assignments_; }
  const std::vector<cleanup_handoff_length_certificate_record> &
  cleanup_certificates() const noexcept { return cleanup_certificates_; }
  const std::vector<coverage_certificate_record> &coverage_certificates()
      const noexcept { return coverage_certificates_; }

  // Dense reverse maps and region ranges.
  const std::vector<std::uint64_t> &boundary_assignment_by_halfedge()
      const noexcept { return boundary_assignment_by_halfedge_; }
  const std::vector<std::uint64_t> &region_triangle_index() const noexcept {
    return region_triangle_index_;
  }
  const std::vector<std::uint64_t> &region_diagonal_index() const noexcept {
    return region_diagonal_index_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

  const output_triangle_record *triangle(
      output_triangle_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= triangles_.size())
      return nullptr;
    return &triangles_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ =
      contract_versions::output_triangulation_artifact_schema;
  std::uint16_t provider_version_ =
      contract_versions::output_triangulation_provider;
  std::uint16_t codec_version_ = contract_versions::output_triangulation_codec;
  std::uint16_t verifier_version_ =
      contract_versions::output_triangulation_verifier;
  output_triangulation_provider_kind provider_ =
      output_triangulation_provider_kind::bounded_indexed_ear_decomposition_v1;
  triangulation_verification_disposition verification_ =
      triangulation_verification_disposition::not_verified;
  boolean_operation operation_ = boolean_operation::set_union;
  context_owner_token owner_{};

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  std::array<bounded_boolean_digest, 2> manifold_digests_{};
  bounded_boolean_digest intersection_digest_{};
  bounded_boolean_digest retained_digest_{};
  bounded_boolean_digest polygonal_digest_{};

  std::vector<support_frame_record> support_frames_;
  std::vector<projected_occurrence_record> projected_occurrences_;
  std::vector<planar_predicate_evidence_record> predicate_evidence_;
  std::vector<orientation_escalation_record> escalations_;
  std::vector<internal_diagonal_record> diagonals_;
  std::vector<triangulation_halfedge_record> internal_halfedges_;
  std::vector<output_triangle_record> triangles_;
  std::vector<triangle_corner_ref_record> corner_refs_;
  std::vector<triangle_edge_use_ref_record> edge_use_refs_;
  std::vector<boundary_assignment_record> boundary_assignments_;
  std::vector<cleanup_handoff_length_certificate_record> cleanup_certificates_;
  std::vector<coverage_certificate_record> coverage_certificates_;

  std::vector<std::uint64_t> boundary_assignment_by_halfedge_;
  std::vector<std::uint64_t> region_triangle_index_;
  std::vector<std::uint64_t> region_diagonal_index_;

  output_triangulation_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class output_triangulation_builder;
  friend struct output_triangulation_artifact_test_access;
};

// Narrow immutable views for Component 13, diagnostics, and tests.
template <class T, class I> class triangulated_output_complex_view final {
public:
  explicit triangulated_output_complex_view(
      const triangulated_output_complex<T, I> &complex,
      const context_owner_token &owner) noexcept
      : complex_(&complex), owner_(owner) {}
  bool valid() const noexcept {
    return complex_ && complex_->owner().same_owner(owner_);
  }
  const triangulated_output_complex<T, I> *get() const noexcept {
    return valid() ? complex_ : nullptr;
  }
  std::uint64_t triangle_count() const noexcept {
    return valid() ? complex_->triangles().size() : 0;
  }
  std::uint64_t internal_diagonal_count() const noexcept {
    return valid() ? complex_->diagonals().size() : 0;
  }
  std::uint64_t cleanup_obligation_count() const noexcept {
    return valid() ? complex_->cleanup_certificates().size() : 0;
  }

private:
  const triangulated_output_complex<T, I> *complex_;
  context_owner_token owner_;
};

} // namespace ygor::mesh_boolean::bounded
