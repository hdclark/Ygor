#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_provenance() {
  auto fixture = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  bounded::candidate_stream_view<scalar, index_type> view(
      fixture.artifact, fixture.predecessor.context.owner);
  require(view.valid() && view.size() == fixture.artifact->candidates().size(),
          "owner-checked candidate stream view");
  for (std::uint64_t ordinal = 0; ordinal < view.size(); ++ordinal) {
    const auto *candidate = view.candidate(ordinal);
    require(candidate && candidate->id.ordinal() == ordinal,
            "candidate checked random access");
    const auto *edge = view.edge(*candidate);
    const auto *triangle = view.triangle(*candidate);
    const auto *witness = view.witness(candidate->witness);
    require(edge && triangle && witness,
            "candidate provenance recovery");
    require(edge->semantic_key == candidate->key.edge &&
                triangle->semantic_key == candidate->key.triangle &&
                witness->key.candidate == candidate->key,
            "candidate provenance semantic identity");
    require(edge->geometry_attachment_digest ==
                fixture.artifact->primitive_table(edge->operand)
                    .geometry_attachment_digest &&
                triangle->precision_attachment_digest ==
                    fixture.artifact->primitive_table(triangle->operand)
                        .precision_attachment_digest,
            "candidate precision and geometry attachment recovery");
  }
  bounded::candidate_stream_view<scalar, index_type> wrong(
      fixture.artifact, bounded::context_owner_token::create());
  require(!wrong.valid() && wrong.candidate(0) == nullptr,
          "cross-context candidate view rejected");
}
} // namespace broad_phase_tests
