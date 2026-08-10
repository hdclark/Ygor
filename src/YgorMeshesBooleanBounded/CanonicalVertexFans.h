#pragma once

#include "CanonicalHalfedgeOperand.h"

namespace ygor::mesh_boolean::bounded {

inline std::uint64_t canonical_fan_next(
    const std::vector<canonical_manifold_halfedge_record> &halfedges,
    std::uint64_t halfedge) noexcept {
  if (halfedge >= halfedges.size())
    return canonical_invalid_ordinal;
  const auto previous = halfedges[halfedge].previous;
  if (previous >= halfedges.size())
    return canonical_invalid_ordinal;
  return halfedges[previous].pair;
}

inline std::uint64_t canonical_fan_previous(
    const std::vector<canonical_manifold_halfedge_record> &halfedges,
    std::uint64_t halfedge) noexcept {
  if (halfedge >= halfedges.size())
    return canonical_invalid_ordinal;
  const auto pair = halfedges[halfedge].pair;
  if (pair >= halfedges.size())
    return canonical_invalid_ordinal;
  return halfedges[pair].next;
}

} // namespace ygor::mesh_boolean::bounded
