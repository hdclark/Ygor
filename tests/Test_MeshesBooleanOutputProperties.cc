#include "MeshBooleanOutputFixtures.h"
#include <iostream>
using namespace output_test;

namespace {
std::vector<std::uint8_t> legacy_rotation(
    const std::array<std::vector<std::uint8_t>, 3> &tokens,
    std::size_t rotation) {
  canonical_encoder e;
  for (std::size_t n = 0; n < 3; ++n)
    e.byte_string(tokens[(n + rotation) % 3]);
  return e.bytes();
}
std::size_t structural_rotation(
    const std::array<std::vector<std::uint8_t>, 3> &tokens) {
  auto less = [&](std::size_t a, std::size_t b) {
    for (std::size_t n = 0; n < 3; ++n) {
      const auto &x = tokens[(n + a) % 3], &y = tokens[(n + b) % 3];
      if (x.size() != y.size()) return x.size() < y.size();
      if (x != y) return x < y;
    }
    return false;
  };
  std::size_t best = 0;
  for (std::size_t candidate = 1; candidate < 3; ++candidate)
    if (less(candidate, best)) best = candidate;
  return best;
}
void rotation_equivalence() {
  std::uint32_t state = 0x9e3779b9U;
  for (std::size_t sample = 0; sample < 256; ++sample) {
    std::array<std::vector<std::uint8_t>, 3> tokens;
    for (auto &token : tokens) {
      state = state * 1664525U + 1013904223U;
      token.resize((state >> 27) + (sample % 3));
      for (auto &byte : token) {
        state = state * 1664525U + 1013904223U;
        byte = static_cast<std::uint8_t>(state >> 24);
      }
    }
    std::array<std::vector<std::uint8_t>, 3> encoded;
    for (std::size_t rotation = 0; rotation < 3; ++rotation)
      encoded[rotation] = legacy_rotation(tokens, rotation);
    const auto legacy = static_cast<std::size_t>(
        std::min_element(encoded.begin(), encoded.end()) - encoded.begin());
    const auto structural = structural_rotation(tokens);
    require(structural == legacy,
            "structural face rotation matches canonical-byte ordering");
    canonical_encoder direct;
    direct.u64(encoded[structural].size());
    for (std::size_t n = 0; n < 3; ++n)
      direct.byte_string(tokens[(n + structural) % 3]);
    canonical_encoder nested;
    nested.byte_string(encoded[legacy]);
    require(direct.bytes() == nested.bytes(),
            "direct ring encoding matches legacy nested encoding");
  }
}
} // namespace

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
    rotation_equivalence();
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
