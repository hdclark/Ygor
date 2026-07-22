#pragma once

#include "CanonicalHalfedgeOperand.h"
#include "FloatingBits.h"

#include <array>
#include <cstddef>
#include <optional>

namespace ygor::mesh_boolean::bounded {

template <class T>
inline std::optional<canonical_bound3<T>> canonical_vertex_bound(
    const source_triangle_vertex_ref<T> &source) noexcept {
  canonical_bound3<T> out;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto interval = finite_interval<T>::create(source.lower[axis], source.upper[axis]);
    if (!interval || !interval->contains(from_bits<T>(source.nominal_bits[axis])))
      return std::nullopt;
    out.axes[axis] = *interval;
  }
  return out.valid() ? std::optional<canonical_bound3<T>>(out) : std::nullopt;
}

template <class T>
inline canonical_bound3<T> canonical_edge_bound(
    const canonical_manifold_vertex_record<T> &a,
    const canonical_manifold_vertex_record<T> &b) {
  return canonical_bound_hull(a.bound, b.bound);
}

template <class T>
inline canonical_bound3<T> canonical_triangle_bound(
    const canonical_manifold_vertex_record<T> &a,
    const canonical_manifold_vertex_record<T> &b,
    const canonical_manifold_vertex_record<T> &c) {
  return canonical_bound_hull(canonical_bound_hull(a.bound, b.bound), c.bound);
}

} // namespace ygor::mesh_boolean::bounded
