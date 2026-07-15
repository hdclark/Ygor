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
    output_test::mutation_rejected(
        *verifiers, *context, source,
        [](auto &value) { std::swap(value.mesh.faces[0][0], value.mesh.faces[0][1]); },
        "reversed face detector");
    output_test::mutation_rejected(
        *verifiers, *context, source,
        [](auto &value) { value.mesh.involved_faces[0].clear(); },
        "reverse incidence detector");
    output_test::mutation_rejected(
        *verifiers, *context, source,
        [](auto &value) { ++value.certificate.topology_digest.bytes[0]; },
        "certificate detector");
    output_test::mutation_rejected(
        *verifiers, *context, source,
        [](auto &value) { value.canonical_bytes.push_back(0); },
        "canonical encoding detector");
  });
  return tests.run(std::cout, std::cerr);
}
