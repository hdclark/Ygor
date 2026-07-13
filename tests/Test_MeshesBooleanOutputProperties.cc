#include "MeshBooleanOutputFixtures.h"
#include <iostream>
using namespace output_test;

template <class T, class I> void qualification() {
  auto a = input_test::box<T, I>(T(0), T(1));
  auto b = input_test::box<T, I>(T(3), T(4));
  boolean_options one;
  one.execution.max_threads = 1;
  auto c1 = classification_test::context(a, b, output_test::registry(),
                                         operation::regularized_union, one);
  auto x = assemble_boolean_output_artifact(*c1);
  require(x.has_value(), "single-thread output");
  output_oracle(*x.value()->payload);
  boolean_options many;
  many.execution.max_threads = 4;
  auto c4 = classification_test::context(a, b, output_test::registry(),
                                         operation::regularized_union, many);
  auto y = assemble_boolean_output_artifact(*c4);
  require(y.has_value(), "multi-thread output");
  require(x.value()->payload->canonical_bytes ==
              y.value()->payload->canonical_bytes,
          "output schedule determinism");
  require(x.value()->payload->mesh.faces == y.value()->payload->mesh.faces,
          "public face determinism");
  cancellation_source stop;
  auto cancelled_context = classification_test::context(
      a, b, output_test::registry(), operation::regularized_union,
      boolean_options{}, &stop);
  stop.cancel();
  auto cancelled = assemble_boolean_output_artifact(*cancelled_context);
  require(!cancelled.has_value() &&
              cancelled.error().code == boolean_error_code::resource_limit,
          "output cancellation category");
  require(cancelled_context->artifacts().latest_generation(
              artifact_slot::assembled_output) == 0,
          "output cancellation rollback");
  const auto count = x.value()->payload->mesh.vertices.size();
  boolean_options exact;
  exact.resources.output_vertices = {false, count};
  auto exact_context = classification_test::context(
      a, b, output_test::registry(), operation::regularized_union, exact);
  auto accepted = assemble_boolean_output_artifact(*exact_context);
  require(accepted.has_value(), "exact output vertex limit");
  boolean_options short_limit;
  short_limit.resources.output_vertices = {false, count - 1};
  auto short_context = classification_test::context(
      a, b, output_test::registry(), operation::regularized_union, short_limit);
  auto rejected = assemble_boolean_output_artifact(*short_context);
  require(!rejected.has_value() &&
              rejected.error().code == boolean_error_code::resource_limit,
          "one-under output limit");
  require(
      short_context->artifacts().latest_generation(
          artifact_slot::assembled_output) == 0 &&
          short_context->accountant().used(resource_kind::output_vertices) == 0,
      "output resource rollback");
}

int main() {
  try {
    boolean_options unsupported_provenance;
    unsupported_provenance.output.include_compact_provenance = true;
    require(!validate_options(unsupported_provenance).has_value(),
            "unimplemented provenance policy rejected at setup");
    require(index_capacity_accepts<std::uint32_t>({0, 0}), "zero capacity");
    require(index_capacity_accepts<std::uint32_t>(
                {0, std::uint64_t(UINT32_MAX) + 1}),
            "uint32 maximum addressable count");
    require(!index_capacity_accepts<std::uint32_t>(
                {0, std::uint64_t(UINT32_MAX) + 2}),
            "uint32 one over capacity");
    require(index_capacity_accepts<std::uint64_t>({1, 0}),
            "uint64 maximum addressable count");
    require(!index_capacity_accepts<std::uint64_t>({1, 1}),
            "uint64 one over capacity");
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
