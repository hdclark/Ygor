#include "MeshBooleanAnalyticFixtures.h"
#include "MeshBooleanTestConfig.h"
#include "MeshBooleanTestHarness.h"

#include <iostream>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

int main() {
  harness tests;
  const auto config = load_test_config();
  tests.add("C14.FUZZ.generated.disjoint_boxes", [config] {
    for (std::size_t ordinal = 0; ordinal < config.generated_cases; ++ordinal) {
      auto stream = derive_stream(config, "C14.FUZZ.generated.disjoint_boxes",
                                  ordinal, "coordinates");
      const auto origin = static_cast<int>(next_random(stream) % 17U) - 8;
      const auto width = static_cast<int>(next_random(stream) % 3U) + 1;
      const auto gap = static_cast<int>(next_random(stream) % 3U) + 1;
      const double lo = origin;
      const double hi = origin + width;
      const double other_lo = hi + gap;
      const double other_hi = other_lo + width;
      const auto first = run_box_operation<double, std::uint32_t>(
          lo, hi, other_lo, other_hi, operation::regularized_union);
      const auto second = run_box_operation<double, std::uint32_t>(
          lo, hi, other_lo, other_hi, operation::regularized_union);
      require_equal(first->payload->canonical_bytes,
                    second->payload->canonical_bytes,
                    "generated case replays deterministically");
      const auto topology = independently_check_closed_mesh(first->payload->mesh);
      require_equal(topology.components, std::uint64_t(2),
                    "generated disjoint union has two components");
    }
  });
  return tests.run(std::cout, std::cerr);
}
