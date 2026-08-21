#pragma once

#include "CleanupTypes.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct cleanup_artifact_test_access;
template <class T, class I> class cleanup_builder;

// Immutable cleaned triangle manifold published by Component 13 and consumed by
// Component 14. Every entity is a triangle-manifold entity: reciprocal pairs,
// exactly two edge uses, three distinct triangle corners, one closed fan per
// vertex occurrence, and no residual cell or cleanup obligation remains.
template <class T> class cleaned_triangle_manifold final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  cleanup_provider_kind provider() const noexcept { return provider_; }
  cleanup_verification_disposition verification() const noexcept {
    return verification_;
  }
  boolean_operation operation() const noexcept { return operation_; }
  final_cleanup_status status() const noexcept { return status_; }
  const context_owner_token &owner() const noexcept { return owner_; }

  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &polygonal_digest() const noexcept {
    return polygonal_digest_;
  }
  const bounded_boolean_digest &triangulated_digest() const noexcept {
    return triangulated_digest_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const cleanup_statistics &statistics() const noexcept { return statistics_; }

  const std::vector<cleaned_vertex_record> &vertices() const noexcept {
    return vertices_;
  }
  const std::vector<cleaned_paired_edge_record> &paired_edges() const noexcept {
    return paired_edges_;
  }
  const std::vector<cleaned_halfedge_record> &halfedges() const noexcept {
    return halfedges_;
  }
  const std::vector<cleaned_triangle_record> &triangles() const noexcept {
    return triangles_;
  }
  const std::vector<cleaned_component_record> &components() const noexcept {
    return components_;
  }
  const std::vector<cleanup_obligation_disposition_record> &
  obligation_dispositions() const noexcept { return obligation_dispositions_; }
  const std::vector<cleanup_action_record> &actions() const noexcept {
    return actions_;
  }

  // Dense reverse maps and ranges.
  const std::vector<std::uint64_t> &vertex_by_occurrence() const noexcept {
    return vertex_by_occurrence_;
  }
  const std::vector<std::uint64_t> &triangle_by_source() const noexcept {
    return triangle_by_source_;
  }
  const std::vector<std::uint64_t> &link_index() const noexcept {
    return link_index_;
  }
  const std::vector<std::uint64_t> &component_triangle_index() const noexcept {
    return component_triangle_index_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }

  const cleaned_vertex_record *vertex(
      cleaned_vertex_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= vertices_.size())
      return nullptr;
    return &vertices_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ = contract_versions::cleanup_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::cleanup_provider;
  std::uint16_t codec_version_ = contract_versions::cleanup_codec;
  std::uint16_t verifier_version_ = contract_versions::cleanup_verifier;
  cleanup_provider_kind provider_ =
      cleanup_provider_kind::append_only_generation_checked_cleanup_complex_v1;
  cleanup_verification_disposition verification_ =
      cleanup_verification_disposition::not_verified;
  boolean_operation operation_ = boolean_operation::set_union;
  final_cleanup_status status_ = final_cleanup_status::invalid;
  context_owner_token owner_{};

  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest polygonal_digest_{};
  bounded_boolean_digest triangulated_digest_{};

  std::vector<cleaned_vertex_record> vertices_;
  std::vector<cleaned_paired_edge_record> paired_edges_;
  std::vector<cleaned_halfedge_record> halfedges_;
  std::vector<cleaned_triangle_record> triangles_;
  std::vector<cleaned_component_record> components_;
  std::vector<cleanup_obligation_disposition_record> obligation_dispositions_;
  std::vector<cleanup_action_record> actions_;

  std::vector<std::uint64_t> vertex_by_occurrence_;
  std::vector<std::uint64_t> triangle_by_source_;
  std::vector<std::uint64_t> link_index_;
  std::vector<std::uint64_t> component_triangle_index_;

  cleanup_statistics statistics_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class cleanup_builder;
  friend struct cleanup_artifact_test_access;
};

} // namespace ygor::mesh_boolean::bounded
