#include "MeshBooleanAnalyticFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <iostream>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

int main() {
  harness tests;
  tests.add("C14.META.schedule.bytes", [] {
    boolean_options serial;
    serial.execution.max_threads = 1;
    boolean_options parallel;
    parallel.execution.max_threads = 4;
    const auto a = run_box_operation<double, std::uint32_t>(
        0, 1, 3, 4, operation::regularized_union, serial);
    const auto b = run_box_operation<double, std::uint32_t>(
        0, 1, 3, 4, operation::regularized_union, parallel);
    require_equal(a->payload->canonical_bytes, b->payload->canonical_bytes,
                  "thread schedule preserves canonical output");
  });
  tests.add("C14.META.commutative.geometry", [] {
    const auto ab = run_box_operation<double, std::uint32_t>(
        0, 1, 3, 4, operation::regularized_union);
    const auto ba = run_box_operation<double, std::uint32_t>(
        3, 4, 0, 1, operation::regularized_union);
    require_equal(sorted_coordinates(ab->payload->mesh),
                  sorted_coordinates(ba->payload->mesh),
                  "union operand swap preserves exact coordinates");
  });
  tests.add("C14.META.directed.swap", [] {
    const auto ab = run_box_operation<double, std::uint32_t>(
        0, 2, 1, 3, operation::a_minus_b);
    const auto ba = run_box_operation<double, std::uint32_t>(
        1, 3, 0, 2, operation::b_minus_a);
    require_equal(sorted_coordinates(ab->payload->mesh),
                  sorted_coordinates(ba->payload->mesh),
                  "directed difference remap preserves geometry");
  });
  tests.add("C14.META.power_two_translation", [] {
    const auto base = run_box_operation<double, std::uint32_t>(
        0, 1, 3, 4, operation::regularized_union);
    const auto transformed = run_box_operation<double, std::uint32_t>(
        8, 10, 14, 16, operation::regularized_union);
    auto expected = sorted_coordinates(base->payload->mesh);
    for (auto &p : expected)
      for (auto &coordinate : p)
        coordinate = coordinate * 2 + 8;
    require_equal(expected, sorted_coordinates(transformed->payload->mesh),
                  "exact affine transform preserves boundary");
  });
  return tests.run(std::cout, std::cerr);
}
