#pragma once

#include "SignedFeatureRelations.h"

namespace ygor::mesh_boolean::bounded {

// Component 07 implementation access. This header is intentionally excluded
// from all downstream production headers and sources.
struct relation_artifact_internal_access final {
  template <class T, class I>
  static const auto &predecessor_candidates(
      const signed_feature_relations<T, I> &artifact) noexcept {
    return artifact.candidates_;
  }

  template <class T, class I>
  static const auto &source_edge_stage(
      const signed_feature_relations<T, I> &artifact) noexcept {
    return artifact.source_edge_stage_;
  }

  template <class T, class I>
  static const auto &source_edge_facet_stage(
      const signed_feature_relations<T, I> &artifact) noexcept {
    return artifact.source_edge_facet_stage_;
  }

  template <class T, class I>
  static const auto &source_facet_stage(
      const signed_feature_relations<T, I> &artifact) noexcept {
    return artifact.source_facet_stage_;
  }

  template <class T, class I>
  static const auto &coplanar_overlay_stage(
      const signed_feature_relations<T, I> &artifact) noexcept {
    return artifact.coplanar_overlay_stage_;
  }
};

} // namespace ygor::mesh_boolean::bounded
