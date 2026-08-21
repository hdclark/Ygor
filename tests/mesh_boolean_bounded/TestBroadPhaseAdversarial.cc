#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_adversarial() {
  auto face_contact = build(box(), box(1, 0, 0, 2, 1, 1));
  require(!face_contact.artifact->candidates().empty(),
          "closed broad phase retains exact face contact");
  for (const auto &candidate : face_contact.artifact->candidates()) {
    const auto &edge = face_contact.artifact
                           ->primitive_table(bounded::role_edge_operand(candidate.role))
                           .edges[candidate.edge.ordinal()];
    const auto &triangle = face_contact.artifact
                               ->primitive_table(
                                   bounded::role_triangle_operand(candidate.role))
                               .triangles[candidate.triangle.ordinal()];
    require(!bounded::classify_closed_bound_relation(edge.bound, triangle.bound)
                 .definitely_separated,
            "adversarial contact candidate has conservative bounds");
  }
  auto nextafter_gap = build(box(), box(1.0001, 0, 0, 2.0001, 1, 1));
  require(nextafter_gap.artifact->candidates().empty(),
          "strictly separated translated boxes are pruned");
}
} // namespace broad_phase_tests
