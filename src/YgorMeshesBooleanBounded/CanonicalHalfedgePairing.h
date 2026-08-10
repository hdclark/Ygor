#pragma once

#include "CanonicalHalfedgeOperand.h"

#include <tuple>

namespace ygor::mesh_boolean::bounded {

struct canonical_pairing_proposal final {
  canonical_edge_key pairing_key{};
  canonical_halfedge_key halfedge_key{};
  std::uint64_t halfedge = 0;
  std::uint64_t origin = 0;
  std::uint64_t destination = 0;
  std::uint64_t source_origin = 0;
  std::uint64_t source_destination = 0;
  std::uint64_t triangle = 0;
  std::uint8_t local_slot = 0;
  std::uint64_t predecessor_edge_use = 0;
  std::uint64_t facet = 0;
  std::uint64_t shell = 0;
  std::uint64_t source_directed_use = canonical_invalid_ordinal;

  friend bool operator<(const canonical_pairing_proposal &a,
                        const canonical_pairing_proposal &b) noexcept {
    return std::tie(a.pairing_key, a.origin, a.destination, a.halfedge_key,
                    a.local_slot, a.predecessor_edge_use) <
           std::tie(b.pairing_key, b.origin, b.destination, b.halfedge_key,
                    b.local_slot, b.predecessor_edge_use);
  }
};

inline canonical_edge_key canonical_pairing_key(
    operand_id operand, const source_triangle_edge_use &use) noexcept {
  canonical_edge_key key;
  key.operand = operand;
  if (use.role == source_triangle_edge_role::source_boundary) {
    key.edge_class = canonical_edge_class::source_edge;
    key.primary = use.source_undirected_edge;
    key.secondary = 0;
  } else {
    key.edge_class = canonical_edge_class::facet_internal_diagonal;
    key.primary = use.facet;
    key.secondary = use.diagonal;
  }
  return key;
}

} // namespace ygor::mesh_boolean::bounded
