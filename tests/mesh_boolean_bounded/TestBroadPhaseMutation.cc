#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_mutation() {
  auto fixture = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  require(!fixture.artifact->candidates().empty(), "mutation fixture candidates");
  auto caps = capabilities(fixture.predecessor);
  bounded::broad_phase_verification_evidence evidence;
  bounded_boolean_error error;

  auto candidate_mutation =
      bounded::broad_phase_test_access::copy(*fixture.artifact);
  bounded::broad_phase_test_access::candidates(candidate_mutation)[0]
      .key.domain_policy_version = 2;
  require(!bounded::verify_canonical_candidate_stream(
              candidate_mutation, fixture.predecessor.context,
              *fixture.predecessor.precision, caps, evidence, error, false),
          "candidate semantic mutation rejected");

  auto hierarchy_mutation =
      bounded::broad_phase_test_access::copy(*fixture.artifact);
  auto &hierarchy = bounded::broad_phase_test_access::hierarchy(
      hierarchy_mutation, bounded::operand_id::a);
  require(!hierarchy.nodes.empty(), "mutation fixture hierarchy");
  hierarchy.nodes[hierarchy.root].subtree_primitive_count += 1;
  error = {};
  require(!bounded::verify_canonical_candidate_stream(
              hierarchy_mutation, fixture.predecessor.context,
              *fixture.predecessor.precision, caps, evidence, error, false),
          "hierarchy range mutation rejected");

  auto codec_mutation = bounded::broad_phase_test_access::copy(*fixture.artifact);
  bounded::broad_phase_test_access::canonical_bytes(codec_mutation).back() ^= 1;
  error = {};
  require(!bounded::verify_broad_phase_codec(codec_mutation, error),
          "canonical byte mutation rejected");
}
} // namespace broad_phase_tests
