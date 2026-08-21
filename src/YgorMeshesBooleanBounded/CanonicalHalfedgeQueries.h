#pragma once

#include "CanonicalHalfedgeOperand.h"

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
class canonical_halfedge_query_view final {
public:
  canonical_halfedge_query_view(
      const canonical_halfedge_operand<T, I> &artifact,
      const context_owner_token &owner) noexcept
      : artifact_(artifact), owner_(owner) {}

  const canonical_manifold_vertex_record<T> *vertex(
      manifold_vertex_id id) const noexcept {
    return artifact_.vertex(id, owner_);
  }
  const canonical_manifold_triangle_record<T> *triangle(
      manifold_triangle_id id) const noexcept {
    return artifact_.triangle(id, owner_);
  }
  const canonical_manifold_halfedge_record *halfedge(
      manifold_halfedge_id id) const noexcept {
    return artifact_.halfedge(id, owner_);
  }
  const canonical_manifold_edge_record<T> *edge(
      manifold_edge_id id) const noexcept {
    return artifact_.edge(id, owner_);
  }
  bool valid_owner() const noexcept {
    return owner_.same_owner(artifact_.owner());
  }

private:
  const canonical_halfedge_operand<T, I> &artifact_;
  context_owner_token owner_;
};

} // namespace ygor::mesh_boolean::bounded

namespace ygor::mesh_boolean::bounded {

template <class T>
inline bool canonical_edge_is_source_feature(
    const canonical_manifold_edge_record<T> &edge) noexcept {
  return edge.edge_class == canonical_edge_class::source_edge &&
         edge.source_feature_owner;
}

template <class T>
inline bool canonical_edge_is_bookkeeping_only(
    const canonical_manifold_edge_record<T> &edge) noexcept {
  return edge.edge_class == canonical_edge_class::facet_internal_diagonal &&
         !edge.source_feature_owner && !edge.symbolic_contact_owner &&
         !edge.classification_barrier_inside_source_facet &&
         !edge.retained_surface_feature;
}

} // namespace ygor::mesh_boolean::bounded
