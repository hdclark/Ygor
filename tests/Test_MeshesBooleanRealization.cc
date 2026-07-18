#include "MeshBooleanRealizationFixtures.h"
#include <iostream>
using namespace realization_test;

int main() {
  try {
    auto r = realization_test::registry();
    auto a = cube<double, std::uint32_t>();
    auto b = cube<double, std::uint32_t>();
    translate(b, 3, 0, 0);
    auto c = classification_test::context(a, b, r, operation::regularized_union);
    auto result = realize_selected_boundary(*c);
    if (!result.has_value()) throw std::runtime_error(render_error(result.error()));
    const auto &x = *result.value()->payload;
    realization_oracle(x);
    require(result.value()->report.passed(), "realization verified");
    require(x.vertices.size() == 16 && x.triangles.size() == 24,
            "disjoint cubes realized and triangulated");
    require(std::all_of(x.axis_domains.begin(), x.axis_domains.end(),
                        [](const auto &domain) {
                          return domain.values.size() == 1;
                        }),
            "exact-in-T domains are singleton");
    require(x.search.visited_nodes == x.vertices.size() + x.components.size() &&
                std::all_of(x.component_transcripts.begin(),
                            x.component_transcripts.end(), [](const auto &t) {
                              return t.visited_nodes ==
                                         t.accepted_ranks.size() + 1 &&
                                     t.complete_assignments == 1;
                            }),
            "singleton realization preserves canonical transcript evidence");
    std::size_t raw_defining = 0, published_defining = 0;
    for (const auto &vertex : x.vertices)
      for (auto node : vertex.derivations)
        raw_defining += x.constructions->nodes[node.value_for_debug()]
                            .defining_relations.size();
    for (const auto &obligation : x.obligations)
      published_defining += obligation.kind ==
                            realization_obligation_kind::defining_relation;
    require(published_defining == raw_defining,
            "complete defining-relation occurrence evidence is retained");
    require(c->artifacts().latest_generation(artifact_slot::realized_boundary) == 1,
            "realization published");
    auto repeated = realize_selected_boundary(*c);
    require(repeated.has_value() && repeated.value().get() == result.value().get(),
            "realization idempotent");

    mutation_rejected(*r, *c, x, [](auto &v) { v.vertices.front().symbolic = symbolic_vertex_id::from_canonical_value(v.symbolic->payload->vertices.size()); }, "symbolic mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { ++v.vertices.front().accepted_bits[0].bits; }, "coordinate mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.axis_domains.front().values.front().rank++; }, "domain mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { std::swap(v.triangles.front().vertices[0], v.triangles.front().vertices[1]); }, "orientation mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.halfedges.front().next = v.halfedges.front().id; }, "topology mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.obligations.front().actual = realization_relation::disjoint; }, "obligation mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.search.complete_assignments = 0; }, "search mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.component_transcripts.front().visited_nodes++; }, "component transcript mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.certificate.solver_version = 1; }, "stale solver evidence rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.certificate.obligation_version = 2; }, "stale obligation evidence rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.pair_boxes.front().upper.x = exact_scalar(-1); }, "pair box mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.canonical_bytes.push_back(0); }, "canonical mutation rejected");
    mutation_rejected(*r, *c, x, [](auto &v) { v.artifact_bytes.push_back(0); }, "artifact mutation rejected");

    auto shell = cube<double, std::uint32_t>();
    for (auto &v : shell.vertices) {
      v.x *= 4;
      v.y *= 4;
      v.z *= 4;
    }
    auto column = cube<double, std::uint32_t>();
    for (auto &v : column.vertices) {
      v.x = v.x == 0 ? 1 : 3;
      v.y = v.y == 0 ? 1 : 3;
      v.z = v.z == 0 ? -1 : 5;
    }
    auto hole_context = classification_test::context(
        shell, column, realization_test::registry(), operation::a_minus_b);
    auto hole_arrangement = build_global_arrangement(*hole_context);
    if (!hole_arrangement.has_value())
      throw std::runtime_error(render_error(hole_arrangement.error()));
    require(hole_arrangement.value()->report.passed(),
            "annular global arrangement passes deep verification");
    require(std::any_of(
                hole_arrangement.value()->payload->patches.begin(),
                hole_arrangement.value()->payload->patches.end(),
                [](const auto &patch) { return !patch.holes.empty(); }),
            "through-column subtraction constructs a global patch with a hole");
    auto with_holes = realize_selected_boundary(*hole_context);
    if (!with_holes.has_value())
      throw std::runtime_error(render_error(with_holes.error()));
    const auto &hole_result = *with_holes.value()->payload;
    realization_oracle(hole_result);
    std::size_t holes = 0, expected_triangles = 0, bridges = 0;
    for (const auto &patch : hole_result.selected->payload->patches) {
      std::size_t boundary_vertices = 0;
      for (const auto cycle_id : patch.cycles) {
        const auto &cycle = hole_result.selected->payload->cycles[
            cycle_id.value_for_debug()];
        boundary_vertices += cycle.halfedges.size();
        holes += cycle.hole;
      }
      expected_triangles += boundary_vertices + 2 * (patch.cycles.size() - 1) - 2;
    }
    for (const auto &halfedge : hole_result.halfedges)
      bridges += halfedge.role == realization_edge_role::hole_bridge;
    require(holes > 0, "through-column subtraction selects a patch with a hole");
    require(hole_result.triangles.size() == expected_triangles,
            "hole triangulation has no Steiner vertices");
    require(bridges >= 2 * holes,
            "hole bridges retain their realization edge role");
    require(with_holes.value()->report.passed(),
            "hole triangulation passes deep verification");

    auto empty_context = classification_test::context(
        a, a, realization_test::registry(), operation::a_minus_b);
    auto empty = realize_selected_boundary(*empty_context);
    require(empty.has_value(), "empty realization succeeds");
    require(empty.value()->payload->vertices.empty() && empty.value()->payload->triangles.empty(),
            "empty realization artifact");
    realization_oracle(*empty.value()->payload);
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
