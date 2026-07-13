#include "MeshBooleanRealizationFixtures.h"
#include <iostream>
using namespace realization_test;

template <class T, class I> void qualification() {
  auto a = input_test::box<T, I>(T(0), T(1));
  auto b = input_test::box<T, I>(T(3), T(4));
  boolean_options one;
  one.execution.max_threads = 1;
  auto c1 = classification_test::context(a, b, realization_test::registry(), operation::regularized_union, one);
  auto x = realize_selected_boundary(*c1);
  require(x.has_value(), "single-thread realization");
  realization_oracle(*x.value()->payload);
  boolean_options many;
  many.execution.max_threads = 4;
  auto c4 = classification_test::context(a, b, realization_test::registry(), operation::regularized_union, many);
  auto y = realize_selected_boundary(*c4);
  require(y.has_value(), "multi-thread realization");
  require(x.value()->payload->canonical_bytes == y.value()->payload->canonical_bytes,
          "serial realization schedule determinism");

  boolean_options neighboring;
  neighboring.realization.strategy = realization_strategy::neighboring_values;
  neighboring.realization.neighboring_value_radius = 1;
  auto cn = classification_test::context(a, b, realization_test::registry(), operation::regularized_union, neighboring);
  auto n = realize_selected_boundary(*cn);
  require(n.has_value(), "neighbor-domain realization");
  realization_oracle(*n.value()->payload);
  for (const auto &v : n.value()->payload->vertices)
    if (!v.preserved_source)
      for (std::size_t axis = 0; axis < 3; ++axis)
        require(n.value()->payload->axis_domains[3 * v.id.value_for_debug() + axis].values.size() >= 2,
                "neighboring exact domain expanded");

  cancellation_source stop;
  auto cancelled_context = classification_test::context(a, b, realization_test::registry(),
      operation::regularized_union, boolean_options{}, &stop);
  stop.cancel();
  auto cancelled = realize_selected_boundary(*cancelled_context);
  require(!cancelled.has_value() && cancelled.error().code == boolean_error_code::resource_limit,
          "realization cancellation category");
  require(cancelled_context->artifacts().latest_generation(artifact_slot::realized_boundary) == 0,
          "realization cancellation rollback");

  const auto attempts = x.value()->payload->search.visited_nodes;
  require(attempts > 0, "realization attempts fixture");
  boolean_options exact;
  exact.resources.realization_attempts = {false, attempts};
  auto exact_context = classification_test::context(a, b, realization_test::registry(), operation::regularized_union, exact);
  auto accepted = realize_selected_boundary(*exact_context);
  require(accepted.has_value(), "exact realization attempt limit");
  require(exact_context->accountant().used(resource_kind::realization_attempts) == attempts,
          "realization attempts charged");
  boolean_options short_limit;
  short_limit.resources.realization_attempts = {false, attempts - 1};
  auto short_context = classification_test::context(a, b, realization_test::registry(), operation::regularized_union, short_limit);
  auto rejected = realize_selected_boundary(*short_context);
  require(!rejected.has_value() && rejected.error().code == boolean_error_code::resource_limit,
          "one-under realization attempt rejection");
  require(short_context->artifacts().latest_generation(artifact_slot::realized_boundary) == 0 &&
          short_context->accountant().used(resource_kind::realization_attempts) == 0,
          "realization resource rollback");
}

int main() {
  try {
    qualification<float, std::uint32_t>();
    qualification<float, std::uint64_t>();
    qualification<double, std::uint32_t>();
    qualification<double, std::uint64_t>();
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
