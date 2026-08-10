#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_codec_replay() {
  auto fixture = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  auto caps = capabilities(fixture.predecessor);
  auto decoded = bounded::decode_canonical_candidate_stream(
      fixture.artifact->canonical_bytes(), fixture.predecessor.context,
      *fixture.predecessor.precision, fixture.predecessor.manifolds, caps);
  require(decoded.has_value(), "canonical candidate stream decode");
  require((*decoded.value())->canonical_bytes() ==
              fixture.artifact->canonical_bytes() &&
              (*decoded.value())->digest() == fixture.artifact->digest(),
          "canonical candidate replay round trip");

  auto corrupt = fixture.artifact->canonical_bytes();
  require(!corrupt.empty(), "canonical candidate stream nonempty");
  corrupt.back() ^= 1;
  auto rejected = bounded::decode_canonical_candidate_stream(
      corrupt, fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.predecessor.manifolds, caps);
  require(!rejected.has_value(), "canonical candidate stream corruption rejected");
}
} // namespace broad_phase_tests
