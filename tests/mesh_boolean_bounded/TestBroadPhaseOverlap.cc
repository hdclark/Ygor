#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
namespace {
bounded::canonical_bound3<scalar> bound(scalar lo, scalar hi) {
  bounded::canonical_bound3<scalar> result;
  result.axes[0] = *bounded::finite_interval<scalar>::create(lo, hi);
  result.axes[1] = *bounded::finite_interval<scalar>::create(0, 1);
  result.axes[2] = *bounded::finite_interval<scalar>::create(0, 1);
  return result;
}
}
void test_overlap() {
  const auto touching = bounded::classify_closed_bound_relation(bound(0, 1), bound(1, 2));
  require(!touching.definitely_separated &&
              touching.axes[0] == bounded::axis_overlap_category::closed_overlap,
          "closed AABBs retain endpoint contact");
  const auto separated = bounded::classify_closed_bound_relation(bound(0, 1), bound(2, 3));
  require(separated.definitely_separated &&
              separated.axes[0] ==
                  bounded::axis_overlap_category::left_definitely_below_right,
          "closed AABBs certify strict separation only");
}
} // namespace broad_phase_tests
