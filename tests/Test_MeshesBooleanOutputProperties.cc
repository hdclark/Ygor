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
template <class T, class I>
status_or<verification_report>
verify_with_work_limit(const assembled_output<T, I> &artifact,
                       boolean_context<T, I> &context,
                       verifier_registry &registry, std::uint64_t limit,
                       verification_level level) {
  resource_policy policy;
  policy.verifier_work = {false, limit};
  resource_accountant accountant(policy);
  const auto type = assembled_output_type_tag +
                    (static_cast<std::uint64_t>(coordinate_type<T>()) << 8) +
                    static_cast<std::uint64_t>(index_type<I>());
  auto spec = registry.specification(artifact_slot::assembled_output, type,
                                     assembled_output_schema,
                                     level);
  require(spec.has_value(), "output resource specification");
  std::shared_ptr<const void> lifetime(&artifact, [](const void *) {});
  artifact_view view{context.owner(), artifact_slot::assembled_output,
                     type, assembled_output_schema, 1,
                     artifact.artifact_digest, lifetime, &artifact};
  auto verify_options = context.options();
  verify_options.verification = level;
  verification_environment_view env{
      context.owner(), context.replay().setup,
      context.contract().selected_operation(), &verify_options,
      coordinate_type<T>(), index_type<I>(), &context.kernel(), {},
      &accountant, [&] { return context.cancelled(); }};
  return registry.verify(view, spec.value(), env);
}
std::uint64_t output_verifier_work(std::size_t components) {
  using T = double;
  using I = std::uint32_t;
  fv_surface_mesh<T, I> a, b;
  for (std::size_t i = 0; i < components; ++i)
    input_test::append(a, input_test::box<T, I>(T(3 * i), T(3 * i + 1)));
  auto verifiers = output_test::registry();
  boolean_options options;
  auto context = classification_test::context(
      a, b, verifiers, operation::regularized_union, options);
  auto output = assemble_boolean_output_artifact(*context);
  require(output.has_value(), "output resource fixture");
  const auto faces = output.value()->payload->mesh.faces.size();
  const auto vertices = output.value()->payload->mesh.vertices.size();
  const std::uint64_t incidences = 6 * faces;
  std::uint64_t levels = 1;
  for (auto count = incidences; count > 1; count = count / 2 + count % 2)
    ++levels;
  const std::uint64_t work = incidences * levels + faces + vertices + 1;
  auto exact = verify_with_work_limit(*output.value()->payload, *context,
                                      *verifiers, work,
                                      verification_level::mandatory);
  require(exact.has_value() && exact.value().passed(),
          "exact output verifier work limit passes");
  auto short_limit = verify_with_work_limit(*output.value()->payload, *context,
                                            *verifiers, work - 1,
                                            verification_level::mandatory);
  require(!short_limit.has_value() &&
              short_limit.error().code == boolean_error_code::resource_limit,
          "one-under output verifier work limit is rejected");
  const auto exhaustive_work = work + vertices * 32U * 32U;
  const auto bytes = output.value()->payload->canonical_bytes;
  auto exhaustive = verify_with_work_limit(
      *output.value()->payload, *context, *verifiers, exhaustive_work,
      verification_level::exhaustive);
  require(exhaustive.has_value() && exhaustive.value().passed() &&
              output.value()->payload->canonical_bytes == bytes,
          "same output artifact passes bounded exhaustive verifier");
  auto exhaustive_short = verify_with_work_limit(
      *output.value()->payload, *context, *verifiers, exhaustive_work - 1,
      verification_level::exhaustive);
  require(!exhaustive_short.has_value() &&
              exhaustive_short.error().code ==
                  boolean_error_code::resource_limit,
          "exhaustive output oracle is separately work-admitted");
  return work;
}
void output_resource_scaling() {
  const auto one = output_verifier_work(1);
  const auto two = output_verifier_work(2);
  require(two < 3 * one,
          "output link verifier work scales below quadratic doubling");
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
    output_resource_scaling();
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
