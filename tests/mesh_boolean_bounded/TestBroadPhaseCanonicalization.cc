#include "BroadPhaseFixtures.h"

#include <algorithm>

namespace broad_phase_tests {
void test_canonicalization() {
  auto fixture = build(box(), box(0.25, 0.25, 0.25, 1.25, 1.25, 1.25));
  const auto &candidates = fixture.artifact->candidates();
  for (std::uint64_t ordinal = 0; ordinal < candidates.size(); ++ordinal) {
    require(candidates[ordinal].id.ordinal() == ordinal &&
                candidates[ordinal].ordinal == ordinal,
            "candidate ID assigned after complete-key sort");
    if (ordinal != 0)
      require(candidates[ordinal - 1].key < candidates[ordinal].key,
              "strict canonical candidate order and duplicate rejection");
  }
  std::uint64_t next = 0;
  for (std::uint64_t ordinal = 0;
       ordinal < fixture.artifact->partitions().size(); ++ordinal) {
    const auto &partition = fixture.artifact->partitions()[ordinal];
    require(partition.id.ordinal() == ordinal && partition.ordinal == ordinal &&
                partition.begin == next && partition.count != 0 &&
                partition.count <= bounded::broad_phase_partition_capacity_v1,
            "deterministic candidate partition");
    next += partition.count;
  }
  require(next == candidates.size(), "candidate partitions cover exact stream");
}
} // namespace broad_phase_tests
