#include "MeshBooleanAnalyticFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <iostream>

using namespace output_test;
using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

int main() {
  harness tests;
  tests.add("C14.MUT.output.detectors", [] {
    auto a = input_test::box<double, std::uint32_t>(0, 1);
    auto b = input_test::box<double, std::uint32_t>(3, 4);
    auto verifiers = output_test::registry();
    auto context = classification_test::context(
        a, b, verifiers, operation::regularized_union);
    const auto result = assemble_boolean_output_artifact(*context);
    require(result.has_value(), "mutation source assembles");
    const auto &source = *result.value()->payload;
    const auto type = assembled_output_type_tag +
        (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
    const auto spec = verifiers->specification(
        artifact_slot::assembled_output, type, assembled_output_schema,
        verification_level::mandatory).value();
    verification_environment_view env{
        context->owner(), context->replay().setup,
        context->contract().selected_operation(), &context->options(),
        coordinate_tag::binary64, index_tag::uint32, &context->kernel(), {},
        &context->accountant(), [&] { return context->cancelled(); }};
    const auto mutation_fails_at = [&](auto mutate, invariant_code expected,
                                       const char *message) {
      auto changed =
          std::make_shared<assembled_output<double, std::uint32_t>>(source);
      mutate(*changed);
      artifact_view view{context->owner(), artifact_slot::assembled_output,
                         type, assembled_output_schema, 1,
                         changed->artifact_digest, changed, changed.get()};
      auto checked = verifiers->verify(view, spec, env);
      require(checked.has_value() && !checked.value().passed(), message);
      bool saw_failure = false;
      for (const auto &result : checked.value().results) {
        if (!saw_failure && result.status == check_status::failed) {
          require(result.code == expected, message);
          saw_failure = true;
        } else if (saw_failure) {
          require(result.status == check_status::not_run_due_to_prior_failure,
                  message);
        } else {
          require(result.status == check_status::passed, message);
        }
      }
      require(saw_failure, message);
    };
    mutation_fails_at(
        [](auto &value) { value.owner = context_owner_token{}; },
        invariant_code::output_binding, "binding detector runs first");
    mutation_fails_at(
        [](auto &value) {
          std::swap(value.mesh.faces[0][0], value.mesh.faces[0][1]);
        },
        invariant_code::output_topology, "reversed face detector family");
    mutation_fails_at(
        [](auto &value) { value.mesh.involved_faces[0].clear(); },
        invariant_code::output_mappings,
        "reverse incidence detector family");
    mutation_fails_at(
        [](auto &value) { ++value.certificate.topology_digest.bytes[0]; },
        invariant_code::output_certificate, "certificate detector family");
    mutation_fails_at(
        [](auto &value) { value.canonical_bytes.push_back(0); },
        invariant_code::output_canonical_encoding,
        "canonical encoding detector family");
  });
  return tests.run(std::cout, std::cerr);
}
