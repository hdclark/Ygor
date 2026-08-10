#pragma once

#include "SourceTriangulationTypes.h"
#include "ValidatedOperand.h"

#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct source_triangle_complex_test_access;

template <class T, class I> class source_triangle_complex final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t policy_version() const noexcept { return policy_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  operand_id operand() const noexcept { return operand_; }
  const context_owner_token &owner() const noexcept { return owner_; }
  const bounded_boolean_digest &predecessor_digest() const noexcept {
    return predecessor_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const auto &vertices() const noexcept { return vertices_; }
  const auto &facets() const noexcept { return facets_; }
  const auto &triangles() const noexcept { return triangles_; }
  const auto &edge_uses() const noexcept { return edge_uses_; }
  const auto &diagonals() const noexcept { return diagonals_; }
  const auto &source_edge_feature_range() const noexcept {
    return source_edge_feature_range_;
  }
  const auto &facet_internal_diagonal_range() const noexcept {
    return facet_internal_diagonal_range_;
  }
  const auto &source_directed_use_to_edge_use() const noexcept {
    return source_directed_use_to_edge_use_;
  }
  const auto &source_vertex_to_triangles() const noexcept {
    return source_vertex_to_triangles_;
  }
  const auto &trace() const noexcept { return trace_; }
  const source_triangulation_statistics &statistics() const noexcept {
    return statistics_;
  }
  const std::vector<std::uint8_t> &semantic_bytes() const noexcept {
    return semantic_bytes_;
  }
  const std::vector<std::uint8_t> &exact_bytes() const noexcept {
    return exact_bytes_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }
  const bounded_boolean_digest &source_semantic_digest() const noexcept {
    return source_semantic_digest_;
  }
  const bounded_boolean_digest &exact_triangulation_digest() const noexcept {
    return exact_triangulation_digest_;
  }
  const bounded_boolean_digest &replay_presentation_digest() const noexcept {
    return replay_presentation_digest_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }

private:
  std::uint16_t schema_version_ = contract_versions::source_triangle_complex;
  std::uint16_t provider_version_ = contract_versions::source_triangulation_provider;
  std::uint16_t policy_version_ = contract_versions::source_triangulation_policy;
  std::uint16_t codec_version_ = contract_versions::source_triangle_complex_codec;
  std::uint16_t verifier_version_ = contract_versions::source_triangle_complex_verifier;
  operand_id operand_ = operand_id::a;
  context_owner_token owner_{};
  bounded_boolean_digest predecessor_digest_{};
  bounded_boolean_digest precision_digest_{};
  std::shared_ptr<const validated_operand<T, I>> predecessor_;
  std::vector<source_triangle_vertex_ref<T>> vertices_;
  std::vector<source_facet_triangulation_record> facets_;
  std::vector<source_triangle_record<T>> triangles_;
  std::vector<source_triangle_edge_use> edge_uses_;
  std::vector<source_internal_diagonal_record> diagonals_;
  std::vector<std::uint64_t> source_edge_feature_range_;
  std::vector<std::uint64_t> facet_internal_diagonal_range_;
  std::vector<std::uint64_t> source_directed_use_to_edge_use_;
  std::vector<std::vector<std::uint64_t>> source_vertex_to_triangles_;
  std::vector<source_ear_trace_step> trace_;
  source_triangulation_statistics statistics_{};
  std::vector<std::uint8_t> semantic_bytes_;
  std::vector<std::uint8_t> exact_bytes_;
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest source_semantic_digest_{};
  bounded_boolean_digest exact_triangulation_digest_{};
  bounded_boolean_digest replay_presentation_digest_{};
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class source_triangulation_builder;
  template <class U, class J>
  friend bool verify_source_triangle_complex(
      const source_triangle_complex<U, J> &,
      const validated_operand<U, J> &, const precision_context<U> &);
  friend struct source_triangle_complex_test_access;
};

} // namespace ygor::mesh_boolean::bounded
