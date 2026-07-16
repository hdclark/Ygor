#include "MeshBooleanSymbolicRegistryFixtures.h"
#include <iostream>
using namespace symbolic_test;
void reconciliation_collision_property() {
  symbolic_reconciliation_request a, b, equal;
  a.facet = b.facet = facet_id::from_canonical_value(1);
  a.first_curve = b.first_curve = symbolic_curve_id::from_canonical_value(2);
  a.second_curve = b.second_curve = symbolic_curve_id::from_canonical_value(3);
  a.point = {exact_scalar(1), exact_scalar(2), exact_scalar(3)};
  b.point = {exact_scalar(1), exact_scalar(2), exact_scalar(4)};
  a.canonical_key = b.canonical_key = digest{};
  equal = a;
  require(!reconciliation_request_equal(a, b),
          "digest collision does not merge reconciliation requests");
  require(reconciliation_request_less(a, b) != reconciliation_request_less(b, a),
          "digest collision has deterministic structural order");
  require(reconciliation_request_equal(a, equal),
          "equal reconciliation requests merge structurally");
}
template <class T, class I> void run() {
  auto r = registry();
  auto a = cube<T, I>(), b = cube<T, I>();
  translate(b, T(1) / T(2), T(1) / T(2), T(1) / T(2));
  boolean_options baseline_options;
  baseline_options.verification = verification_level::exhaustive;
  auto c = context(a, b, r, baseline_options);
  auto s = build_symbolic_complex(*c);
  require(s.has_value(), "registry property");
  const auto &x = *s.value()->payload;
  for (std::size_t i = 1; i < x.vertices.size(); ++i)
    require(
        lexicographic_compare(x.vertices[i - 1].point, x.vertices[i].point) < 0,
        "canonical point order");
  for (const auto &m : x.raw_points)
    require(m.symbolic.value_for_debug() < x.vertices.size(),
            "raw mapping range");
  for (const auto &m : x.raw_intervals)
    require(!m.atomic_intervals.empty(), "interval coverage");
  for (const auto &order : x.angular_orders)
    require(!order.groups.empty(), "angular order coverage");
  for (const auto &order : x.radial_orders)
    require(!order.groups.empty(), "radial order coverage");
  const auto expected = x.canonical_symbolic_bytes;
  for (auto threads : {1U, 2U, 4U}) {
    for (unsigned repetition = 0; repetition != 4; ++repetition) {
      boolean_options options;
      options.verification = verification_level::exhaustive;
      options.execution.max_threads = threads;
      auto scheduled_context = context(a, b, r, options);
      auto scheduled = build_symbolic_complex(*scheduled_context);
      require(scheduled.has_value(), "scheduled registry build");
      require(scheduled.value()->payload->canonical_symbolic_bytes == expected,
              "schedule-independent canonical merge");
      require(scheduled.value()->report.passed(),
              "scheduled exhaustive verification");
    }
  }
}
int main() {
  try {
    reconciliation_collision_property();
    run<float, std::uint32_t>();
    run<float, std::uint64_t>();
    run<double, std::uint32_t>();
    run<double, std::uint64_t>();
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
