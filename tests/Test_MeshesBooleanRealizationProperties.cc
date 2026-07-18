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
  auto shrunk = *x.value()->payload;
  if (!shrunk.pair_boxes.empty()) {
    shrunk.pair_boxes.front().upper = shrunk.pair_boxes.front().lower;
    require(!verify_realization_constraint_evidence(shrunk),
            "independent verifier rejects a nonconservative domain box");
  }

  boolean_options neighboring;
  neighboring.realization.strategy = realization_strategy::neighboring_values;
  neighboring.realization.neighboring_value_radius = 1;
  auto cn = classification_test::context(a, b, realization_test::registry(), operation::regularized_union, neighboring);
  auto n = realize_selected_boundary(*cn);
  require(n.has_value(), "neighbor-domain realization");
  realization_oracle(*n.value()->payload);
  require(std::all_of(n.value()->payload->axis_domains.begin(),
                      n.value()->payload->axis_domains.end(),
                      [](const auto &domain) {
                        return domain.values.size() == 1;
                      }),
          "neighboring policy cannot expand exact-in-T domains");
  require(n.value()->payload->search.visited_nodes ==
              n.value()->payload->vertices.size() +
                  n.value()->payload->components.size(),
          "neighboring singleton preserves canonical transcript evidence");

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
  require(attempts == x.value()->payload->vertices.size() +
                          x.value()->payload->components.size(),
          "singleton realization preserves canonical attempt evidence");
  boolean_options exact_attempts;
  exact_attempts.resources.realization_attempts = {false, attempts};
  auto exact_attempt_context = classification_test::context(
      a, b, realization_test::registry(), operation::regularized_union,
      exact_attempts);
  require(realize_selected_boundary(*exact_attempt_context).has_value(),
          "exact singleton transcript-node limit");
  require(exact_attempt_context->accountant().used(
              resource_kind::realization_attempts) == 0,
          "singleton path charges no actual DFS attempts");
  boolean_options short_attempts;
  short_attempts.resources.realization_attempts = {false, attempts - 1};
  auto short_attempt_context = classification_test::context(
      a, b, realization_test::registry(), operation::regularized_union,
      short_attempts);
  const auto short_attempt = realize_selected_boundary(*short_attempt_context);
  require(!short_attempt.has_value() &&
              short_attempt.error().code == boolean_error_code::resource_limit,
          "one-under singleton transcript-node limit rejects before publication");

  if constexpr (std::is_same<T, double>::value &&
                std::is_same<I, std::uint32_t>::value) {
    std::uint64_t hierarchy_checks = 0;
    require(detail::conservative_realization_triangle_pairs(
                x.value()->payload->pair_boxes, &hierarchy_checks) ==
                x.value()->payload->pair_candidates,
            "realization hierarchy fixture reconstructs candidates");
    const auto pair_checks = std::max<std::uint64_t>(
        hierarchy_checks, x.value()->payload->pair_candidates.size());
    require(pair_checks > 0, "realization pair-check fixture");
    boolean_options pair_exact;
    pair_exact.resources.realization_pair_checks = {false, pair_checks};
    auto pair_context = classification_test::context(
        a, b, realization_test::registry(), operation::regularized_union,
        pair_exact);
    require(realize_selected_boundary(*pair_context).has_value(),
            "exact realization pair-check limit");
    boolean_options pair_short;
    pair_short.resources.realization_pair_checks = {false, pair_checks - 1};
    auto pair_short_context = classification_test::context(
        a, b, realization_test::registry(), operation::regularized_union,
        pair_short);
    const auto pair_rejected = realize_selected_boundary(*pair_short_context);
    require(!pair_rejected.has_value() &&
                pair_rejected.error().code == boolean_error_code::resource_limit,
            "one-under realization pair-check rejection");

    const auto components = x.value()->payload->components.size();
    const auto trail = std::max_element(
        x.value()->payload->components.begin(),
        x.value()->payload->components.end(), [](const auto &lhs, const auto &rhs) {
          return lhs.variables.size() < rhs.variables.size();
        })->variables.size();
    boolean_options component_short;
    component_short.resources.realization_components = {false, components - 1};
    auto component_context = classification_test::context(
        a, b, realization_test::registry(), operation::regularized_union,
        component_short);
    const auto component_rejected = realize_selected_boundary(*component_context);
    require(!component_rejected.has_value() &&
                component_rejected.error().code ==
                    boolean_error_code::resource_limit,
            "one-under realization component rejection");
    boolean_options trail_short;
    trail_short.resources.realization_solver_trail = {false, trail - 1};
    auto trail_context = classification_test::context(
        a, b, realization_test::registry(), operation::regularized_union,
        trail_short);
    const auto trail_rejected = realize_selected_boundary(*trail_context);
    require(!trail_rejected.has_value() &&
                trail_rejected.error().code == boolean_error_code::resource_limit,
            "one-under realization trail rejection");
  }
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
