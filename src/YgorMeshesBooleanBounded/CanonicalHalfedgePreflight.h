#pragma once

#include "CanonicalHalfedgeOperand.h"
#include "CheckedArithmetic.h"
#include "SourceTriangleComplex.h"
#include "ValidatedOperand.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct canonical_halfedge_preflight_counts final {
  std::uint64_t represented_vertices = 0;
  std::uint64_t triangles = 0;
  std::uint64_t halfedges = 0;
  std::uint64_t boundary_uses = 0;
  std::uint64_t diagonal_uses = 0;
  std::uint64_t source_edges = 0;
  std::uint64_t internal_diagonals = 0;
  std::uint64_t edges = 0;
  std::uint64_t work_units = 0;
  std::uint64_t estimated_persistent_bytes = 0;
  std::uint64_t estimated_temporary_bytes = 0;
};

template <class T, class I>
inline bool canonical_halfedge_preflight(
    const validated_operand<T, I> &operand,
    const source_triangle_complex<T, I> &triangles,
    const canonical_halfedge_capabilities &capabilities,
    canonical_halfedge_preflight_counts &out) noexcept {
  out = {};
  out.triangles = triangles.triangles().size();
  if (!checked_multiply(out.triangles, std::uint64_t{3}, out.halfedges))
    return false;
  out.source_edges = operand.edges().size();
  out.internal_diagonals = triangles.diagonals().size();
  if (!checked_add(out.source_edges, out.internal_diagonals, out.edges))
    return false;
  for (const auto &use : triangles.edge_uses()) {
    if (use.role == source_triangle_edge_role::source_boundary)
      ++out.boundary_uses;
    else if (use.role == source_triangle_edge_role::facet_internal_diagonal)
      ++out.diagonal_uses;
    else
      return false;
  }
  std::uint64_t twice_source_edges = 0, twice_diagonals = 0,
                twice_edges = 0, total_uses = 0;
  if (!checked_multiply(out.source_edges, std::uint64_t{2}, twice_source_edges) ||
      !checked_multiply(out.internal_diagonals, std::uint64_t{2}, twice_diagonals) ||
      !checked_multiply(out.edges, std::uint64_t{2}, twice_edges) ||
      !checked_add(out.boundary_uses, out.diagonal_uses, total_uses) ||
      out.boundary_uses != twice_source_edges ||
      out.diagonal_uses != twice_diagonals ||
      out.halfedges != total_uses || out.halfedges != twice_edges)
    return false;
  std::vector<bool> represented(operand.vertices().size(), false);
  for (const auto &triangle : triangles.triangles())
    for (auto vertex : triangle.vertices) {
      if (vertex >= represented.size())
        return false;
      represented[vertex] = true;
    }
  for (bool value : represented)
    out.represented_vertices += value ? 1 : 0;
  if (out.represented_vertices > capabilities.maximum_entities ||
      out.triangles > capabilities.maximum_entities ||
      out.halfedges > capabilities.maximum_entities ||
      out.edges > capabilities.maximum_entities)
    return false;
  std::uint64_t work = 0;
  if (!checked_add(out.represented_vertices, out.triangles, work) ||
      !checked_add(work, out.halfedges, work) ||
      !checked_add(work, out.edges, work) ||
      !checked_add(work, out.halfedges, work))
    return false;
  out.work_units = work;
  if (out.work_units > capabilities.maximum_work_units)
    return false;
  std::uint64_t bytes = 0, add = 0;
  auto add_records = [&](std::uint64_t count, std::uint64_t size) {
    return checked_multiply(count, size, add) && checked_add(bytes, add, bytes);
  };
  if (!add_records(out.represented_vertices,
                   sizeof(canonical_manifold_vertex_record<T>)) ||
      !add_records(out.triangles,
                   sizeof(canonical_manifold_triangle_record<T>)) ||
      !add_records(out.halfedges,
                   sizeof(canonical_manifold_halfedge_record)) ||
      !add_records(out.edges, sizeof(canonical_manifold_edge_record<T>)) ||
      !add_records(out.represented_vertices,
                   sizeof(canonical_vertex_fan_record)))
    return false;
  out.estimated_persistent_bytes = bytes;
  if (!checked_multiply(out.halfedges, std::uint64_t{96},
                        out.estimated_temporary_bytes))
    return false;
  return true;
}

} // namespace ygor::mesh_boolean::bounded
