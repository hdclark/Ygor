#pragma once
#include "MeshBooleanSymbolicRegistryFixtures.h"
#include <YgorMeshesBooleanLocalRefinement.h>
namespace local_test {
using namespace symbolic_test;
inline std::shared_ptr<verifier_registry> registry() {
  auto r = std::make_shared<verifier_registry>();
  for (auto c : {coordinate_tag::binary32, coordinate_tag::binary64})
    for (auto i : {index_tag::uint32, index_tag::uint64}) {
      require(register_input_topology_verifier(*r, c, i).has_value(),
              "input verifier");
      require(register_broad_phase_verifier(*r, c, i).has_value(),
              "broad verifier");
      require(register_intersection_events_verifier(*r, c, i).has_value(),
              "event verifier");
      require(register_symbolic_registry_verifier(*r, c, i).has_value(),
              "symbolic verifier");
      require(register_local_refinement_verifier(*r, c, i).has_value(),
              "local verifier");
    }
  require(r->freeze().has_value(), "freeze");
  return r;
}

inline void exhaustive_oracle(const local_refinement &facet) {
  std::vector<bool> halfedge_seen(facet.halfedges.size());
  exact_scalar patch_area(0);
  for (const auto &walk : facet.walks) {
    exact_scalar area(0);
    for (auto halfedge_ref : walk.halfedges) {
      const auto index = halfedge_ref.id.value_for_debug();
      require(index < facet.halfedges.size() && !halfedge_seen[index],
              "oracle halfedge partition");
      halfedge_seen[index] = true;
      const auto &halfedge = facet.halfedges[index];
      const auto &origin =
          facet.vertices[halfedge.origin.id.value_for_debug()].projected;
      const auto &destination =
          facet.vertices[halfedge.destination.id.value_for_debug()].projected;
      area = area + origin.x * destination.y - origin.y * destination.x;
    }
    require(area == walk.signed_double_area, "oracle walk area");
  }
  require(std::all_of(halfedge_seen.begin(), halfedge_seen.end(),
                      [](bool seen) { return seen; }),
          "oracle complete walk permutation");
  for (std::size_t i = 0; i < facet.edges.size(); ++i)
    for (std::size_t j = i + 1; j < facet.edges.size(); ++j) {
      const auto &first = facet.edges[i];
      const auto &second = facet.edges[j];
      exact_segment2 a{
          facet.vertices[first.lower.id.value_for_debug()].projected,
          facet.vertices[first.upper.id.value_for_debug()].projected};
      exact_segment2 b{
          facet.vertices[second.lower.id.value_for_debug()].projected,
          facet.vertices[second.upper.id.value_for_debug()].projected};
      const auto relation = relate_segments(a, b);
      require(relation.point_kind != segment_point_kind::proper_crossing &&
                  relation.dimension != intersection_dimension::segment,
              "oracle atom interiors disjoint");
    }
  for (const auto &patch : facet.patches)
    patch_area = patch_area + patch.signed_double_area;
  require(patch_area == facet.certificate.source_double_area,
          "oracle patch area");
  require(facet.vertices.size() + facet.faces.size() ==
              facet.edges.size() + 1 + facet.certificate.components,
          "oracle Euler identity");
}
} // namespace local_test
