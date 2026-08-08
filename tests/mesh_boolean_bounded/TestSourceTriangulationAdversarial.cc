#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_adversarial() {
  auto fixture = make_fixture(box<double, std::uint32_t>());
  bounded::source_triangulation_capabilities limited;
  limited.owner = fixture.context.owner;
  limited.resources = fixture.resources.get();
  limited.maximum_ring_size = 3;
  auto rejected = bounded::triangulate_source_operand(
      fixture.operand, fixture.context, *fixture.precision, limited);
  require(!rejected.has_value() &&
              rejected.error()->category == bounded_boolean_error_category::resource_limit,
          "ring-size limit fails deterministically");
  bounded_boolean_cancellation_source cancellation;
  cancellation.request_cancel();
  bounded::source_triangulation_capabilities cancelled;
  cancelled.owner = fixture.context.owner;
  cancelled.resources = fixture.resources.get();
  auto token = cancellation.token();
  cancelled.cancellation = &token;
  auto stopped = bounded::triangulate_source_operand(
      fixture.operand, fixture.context, *fixture.precision, cancelled);
  require(!stopped.has_value() &&
              stopped.error()->category == bounded_boolean_error_category::cancelled,
          "pre-cancelled triangulation publishes nothing");
}
