#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_traversal() {
  auto fixture = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  const auto &plans = fixture.artifact->count_plans();
  const auto expected_plans = fixture.artifact->primitive_table(bounded::operand_id::a).edges.size() +
                              fixture.artifact->primitive_table(bounded::operand_id::b).edges.size();
  require(plans.size() == expected_plans, "one count plan per canonical edge");
  std::uint64_t prefix = 0;
  for (std::uint64_t ordinal = 0; ordinal < plans.size(); ++ordinal) {
    const auto &plan = plans[ordinal];
    require(plan.plan_ordinal == ordinal && plan.output_prefix == prefix,
            "checked deterministic count-plan prefix");
    require(plan.retained_overlaps == plan.candidate_count,
            "count traversal retained-overlap reconciliation");
    prefix += plan.candidate_count;
  }
  require(prefix == fixture.artifact->candidates().size(),
          "count/emit exact materialization");
  const auto &stats = fixture.artifact->statistics();
  require(stats.role_node_tests == stats.role_emit_node_tests,
          "count and emit node-test reconciliation");
  require(stats.role_primitive_tests == stats.role_emit_primitive_tests,
          "count and emit primitive-test reconciliation");
}
} // namespace broad_phase_tests
