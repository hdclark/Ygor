#include "MeshBooleanSymbolicRegistryFixtures.h"
#include <iostream>
using namespace symbolic_test;
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
