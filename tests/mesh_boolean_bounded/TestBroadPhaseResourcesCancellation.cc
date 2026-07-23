#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_resources_cancellation() {
  auto predecessor = build_predecessors(
      box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  auto caps = capabilities(predecessor);
  bounded_boolean_cancellation_source source;
  auto token = source.token();
  source.request_cancel(77);
  caps.cancellation = &token;
  auto cancelled = bounded::build_canonical_candidate_stream(
      predecessor.context, *predecessor.precision, predecessor.manifolds, caps);
  require(!cancelled.has_value() &&
              cancelled.error()->category ==
                  bounded_boolean_error_category::cancelled,
          "broad-phase cancellation before allocation");

  caps = capabilities(predecessor);
  caps.maximum_candidates = 1;
  auto limited = bounded::build_canonical_candidate_stream(
      predecessor.context, *predecessor.precision, predecessor.manifolds, caps);
  require(!limited.has_value() &&
              limited.error()->category ==
                  bounded_boolean_error_category::resource_limit,
          "broad-phase exact candidate resource limit");
}
} // namespace broad_phase_tests
