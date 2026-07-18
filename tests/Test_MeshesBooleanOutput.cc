#include "MeshBooleanOutputFixtures.h"
#include <iostream>
using namespace output_test;

int main() {
  try {
    auto r = output_test::registry();
    auto a = cube<double, std::uint32_t>();
    auto b = cube<double, std::uint32_t>();
    translate(b, 3, 0, 0);
    auto c =
        classification_test::context(a, b, r, operation::regularized_union);
    auto result = assemble_boolean_output_artifact(*c);
    if (!result.has_value())
      throw std::runtime_error(render_error(result.error()));
    const auto &x = *result.value()->payload;
    output_oracle(x);
    require(result.value()->report.passed(), "output verified");
    require(x.mesh.vertices.size() == 16 && x.mesh.faces.size() == 24 &&
                x.components.size() == 2,
            "disconnected cubes assembled");
    auto public_result = assemble_boolean_output(*c);
    require(public_result.has_value(), "public output result");
    require(public_result.value()->mesh.faces == x.mesh.faces &&
                public_result.value()->certificate,
            "public wrapper audit data");
    require(public_result.value()->mesh.vertices.data() ==
                    x.mesh.vertices.data() &&
                public_result.value()->mesh.faces.data() == x.mesh.faces.data(),
            "public result reuses published mesh storage");
    auto repeated = assemble_boolean_output_artifact(*c);
    require(repeated.has_value() &&
                repeated.value().get() == result.value().get(),
            "output idempotent");

    mutation_rejected(
        *r, *c, x,
        [](auto &v) { v.mesh.vertices.front().x = -v.mesh.vertices.front().x; },
        "coordinate mutation rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) {
                        std::swap(v.mesh.faces.front()[0],
                                  v.mesh.faces.front()[1]);
                      },
                      "ring reversal rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) { v.mesh.faces.front().pop_back(); },
                      "face length rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) { v.mesh.involved_faces.front().clear(); },
                      "reverse incidence mutation rejected");
    mutation_rejected(
        *r, *c, x,
        [](auto &v) {
          std::swap(v.mesh.faces.front()[0], v.mesh.faces.front()[1]);
          std::swap(v.faces.front().public_vertices[0],
                    v.faces.front().public_vertices[1]);
          for (auto &incidence : v.mesh.involved_faces) incidence.clear();
          for (std::size_t face = 0; face < v.mesh.faces.size(); ++face)
            for (auto vertex : v.mesh.faces[face])
              v.mesh.involved_faces[vertex].push_back(
                  static_cast<std::uint32_t>(face));
        },
        "co-mutated face mapping and incidence rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) {
                        v.mesh.vertex_normals.push_back({0, 0, 1});
                      },
                      "normal mutation rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) { v.mesh.vertex_colours.push_back(0); },
                      "colour mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.mesh.metadata["x"] = "y"; },
                      "metadata mutation rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) { v.faces.front().cyclic_rotation = 3; },
                      "mapping mutation rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) { v.components.front().first_face++; },
                      "component mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.certificate.faces++; },
                      "certificate mutation rejected");
    mutation_rejected(
        *r, *c, x,
        [](auto &v) { ++v.topology_authorization.policy_digest.bytes.front(); },
        "topology authorization mutation rejected");
    mutation_rejected(
        *r, *c, x,
        [](auto &v) { ++v.certificate.topology_digest.bytes.front(); },
        "topology digest mutation rejected");
    mutation_rejected(
        *r, *c, x,
        [](auto &v) { ++v.certificate.mapping_digest.bytes.front(); },
        "mapping digest mutation rejected");
    mutation_rejected(*r, *c, x,
                      [](auto &v) { v.canonical_bytes.push_back(0); },
                      "canonical mutation rejected");

    auto emitted = x.mesh;
    auto other = cube<double, std::uint32_t>();
    translate(other, 10, 0, 0);
    auto reingest = classification_test::context(
        emitted, other, output_test::registry(), operation::regularized_union);
    auto valid = validate_operands(*reingest);
    require(valid.has_value(), "output re-ingests without repair");
    auto empty_context = classification_test::context(
        a, a, output_test::registry(), operation::a_minus_b);
    auto empty = assemble_boolean_output_artifact(*empty_context);
    require(empty.has_value(), "empty output succeeds");
    output_oracle(*empty.value()->payload);
    require(empty.value()->payload->mesh.vertices.empty() &&
                empty.value()->payload->mesh.faces.empty() &&
                empty.value()->payload->components.empty(),
            "all empty public fields");
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
