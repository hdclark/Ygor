#pragma once

#include "CanonicalHalfedgeOperand.h"

#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I> class canonical_source_manifolds final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  const context_owner_token &owner() const noexcept { return owner_; }
  const std::shared_ptr<const canonical_halfedge_operand<T, I>> &a() const noexcept {
    return a_;
  }
  const std::shared_ptr<const canonical_halfedge_operand<T, I>> &b() const noexcept {
    return b_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }

private:
  std::uint16_t schema_version_ =
      contract_versions::canonical_source_manifolds_schema;
  context_owner_token owner_{};
  std::shared_ptr<const canonical_halfedge_operand<T, I>> a_;
  std::shared_ptr<const canonical_halfedge_operand<T, I>> b_;
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J>
  friend boolean_outcome<std::shared_ptr<const canonical_source_manifolds<U, J>>>
  build_canonical_source_manifolds(
      std::shared_ptr<const validated_operand<U, J>>,
      std::shared_ptr<const validated_operand<U, J>>,
      std::shared_ptr<const source_triangle_complex<U, J>>,
      std::shared_ptr<const source_triangle_complex<U, J>>,
      const boolean_context<U, J> &, const precision_context<U> &,
      canonical_halfedge_capabilities);
};

} // namespace ygor::mesh_boolean::bounded
