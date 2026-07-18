#include "MeshBooleanOutputFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <YgorMeshesExactArithmetic.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <set>

using namespace output_test;
using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

template <class T, class I>
void touching_case(T x, T y, T z, const char *contact) {
  const bool edge_contact = z == T(0);
  for (unsigned presentation = 0; presentation < 2; ++presentation) {
    auto a = input_test::cube<T, I>();
    auto b = input_test::cube<T, I>();
    symbolic_test::translate(b, x, y, z);
    if (presentation)
      std::swap(a, b);
    for (const auto op : {operation::regularized_union,
                          operation::regularized_intersection,
                          operation::a_minus_b, operation::b_minus_a,
                          operation::symmetric_difference}) {
      auto verifiers = output_test::registry();
      auto context = classification_test::context(a, b, verifiers, op);
    const auto events = discover_intersection_events(*context);
    require(events.has_value(), std::string(contact) + " event discovery");
    require(std::any_of(events.value()->payload->classifications.begin(),
                        events.value()->payload->classifications.end(),
                        [](const auto &entry) {
                          return entry.aggregate !=
                                 pair_aggregate_relation::disjoint;
                        }),
            std::string(contact) + " exact contact event");

      const auto selected = select_boolean_boundary(*context);
      require(selected.has_value(), std::string(contact) +
                                        " exact selection publishes");
      const auto &boundary = *selected.value()->payload;
      const auto result = assemble_boolean_output(*context);
    if (op == operation::regularized_union ||
        op == operation::symmetric_difference) {
        require_equal(boundary.topology,
                      selected_boundary_topology::closed_stratified_nonmanifold,
                      std::string(contact) + " stratified topology class");
        require(!boundary.topology_obstructions.empty(),
                std::string(contact) + " canonical topology obstruction");
        require(std::any_of(
                    boundary.topology_obstructions.begin(),
                    boundary.topology_obstructions.end(),
                    [&](const auto &obstruction) {
                      return obstruction.kind ==
                             (edge_contact
                                  ? topology_obstruction_kind::multiple_edge_occurrences
                                  : topology_obstruction_kind::disconnected_geometric_vertex_link);
                    }),
                std::string(contact) + " contact-specific obstruction");
      require(!result.has_value(),
              std::string(contact) + " manifold mesh must not publish");
        require_equal(result.error().code,
                      boolean_error_code::result_topology_not_supported,
                      std::string(contact) + " dedicated topology failure");
        require_equal(result.error().stage,
                      boolean_stage::result_topology_preflight,
                      std::string(contact) + " topology preflight stage");
        require(context->artifacts().latest_generation(
                    artifact_slot::realized_boundary) == 0,
                std::string(contact) + " realization was not attempted");
    } else {
      require(result.has_value(),
              std::string(contact) + " empty/unchanged operation succeeds" +
                  (result.has_value() ? std::string()
                                      : ": " + render_error(result.error())));
        if (op == operation::regularized_intersection) {
          require_equal(boundary.topology, selected_boundary_topology::empty,
                        std::string(contact) + " empty intersection topology");
          require(result.value()->mesh.vertices.empty() &&
                      result.value()->mesh.faces.empty(),
                  std::string(contact) + " canonical empty intersection");
        } else {
          require_equal(boundary.topology,
                        selected_boundary_topology::closed_embedded_two_manifold,
                        std::string(contact) + " difference manifold topology");
          require(boundary.patches.size() == 6 &&
                      result.value()->mesh.faces.size() == 12,
                  std::string(contact) + " ordered difference retains one cube");
        }
      }
    }
  }
}

template <class T, class I> void one_third_case() {
  auto a = input_test::cube<T, I>();
  auto b = input_test::third_intersection_prism<T, I>();
  auto verifiers = realization_test::registry();
  auto context = classification_test::context(
      a, b, verifiers, operation::regularized_intersection);
  const auto selected = select_boolean_boundary(*context);
  require(selected.has_value(), "G2 Components 1-10 succeed");

  const exact_scalar third = exact_scalar(1) / exact_scalar(3);
  bool lower = false, upper = false;
  for (const auto &vertex : selected.value()->payload->vertices) {
    const auto &point = selected.value()->payload->arrangement->payload->symbolic->payload
                            ->vertices[vertex.symbolic.value_for_debug()]
                            .point;
    if (point.x == third && point.y == exact_scalar(1)) {
      lower = lower || point.z == exact_scalar(0);
      upper = upper || point.z == exact_scalar(1);
    }
  }
  require(lower && upper, "G2 exact selected one-third vertices");
  const auto realized = realize_selected_boundary(*context);
  require(!realized.has_value(), "G2 exact-in-T realization must fail");
  require_equal(realized.error().code,
                boolean_error_code::output_not_representable,
                "G2 typed representability failure");
  require(!realized.error().features.empty() ||
              !realized.error().replay_payload.empty(),
          "G2 failure binds exact symbols and unsatisfied constraints");
  boolean_options neighboring;
  neighboring.realization.strategy = realization_strategy::neighboring_values;
  neighboring.realization.neighboring_value_radius = 4;
  auto neighboring_context = classification_test::context(
      a, b, realization_test::registry(), operation::regularized_intersection,
      neighboring);
  const auto neighboring_result = realize_selected_boundary(*neighboring_context);
  require(!neighboring_result.has_value() &&
              neighboring_result.error().code == realized.error().code &&
              neighboring_result.error().features == realized.error().features &&
              neighboring_result.error().replay_payload ==
                  realized.error().replay_payload,
          "G2 neighboring policy preserves immediate canonical failure");
}

void g1a() {
  touching_case<float, std::uint32_t>(1, 1, 1, "G1a vertex contact");
  touching_case<float, std::uint64_t>(1, 1, 1, "G1a vertex contact");
  touching_case<double, std::uint32_t>(1, 1, 1, "G1a vertex contact");
  touching_case<double, std::uint64_t>(1, 1, 1, "G1a vertex contact");
}

void g1b() {
  touching_case<float, std::uint32_t>(1, 1, 0, "G1b edge contact");
  touching_case<float, std::uint64_t>(1, 1, 0, "G1b edge contact");
  touching_case<double, std::uint32_t>(1, 1, 0, "G1b edge contact");
  touching_case<double, std::uint64_t>(1, 1, 0, "G1b edge contact");
}

void g2a() {
  one_third_case<float, std::uint32_t>();
  one_third_case<float, std::uint64_t>();
  one_third_case<double, std::uint32_t>();
  one_third_case<double, std::uint64_t>();
}

void g3() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 1.0, 1.0, 1.0);
  auto context = classification_test::context(
      a, b, arrangement_test::registry(), operation::regularized_union);
  const auto arrangement = build_global_arrangement(*context);
  require(arrangement.has_value(), "G3 contact arrangement builds");
  const auto &g = *arrangement.value()->payload;
  const exact_point3 contact{exact_scalar(1), exact_scalar(1), exact_scalar(1)};
  const arrangement_vertex *shared = nullptr;
  for (const auto &vertex : g.vertices)
    if (g.symbolic->payload->vertices[vertex.symbolic.value_for_debug()].point == contact)
      shared = &vertex;
  require(shared && shared->occurrences.size() == 2,
          "G3 one geometric point owns two topological occurrences");
  require(g.vertex_occurrences[shared->occurrences[0].value_for_debug()].operand !=
              g.vertex_occurrences[shared->occurrences[1].value_for_debug()].operand,
          "G3 contact occurrences retain separate source sheets");
  for (const auto &halfedge : g.halfedges)
    require(g.vertex_occurrences[halfedge.origin_occurrence.value_for_debug()].operand ==
                g.sheet_uses[halfedge.use.value_for_debug()].operand,
            "G3 occurrence adjacency never crosses operands");
}

void g4a() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 1.0, 1.0, 1.0);
  auto context = classification_test::context(
      a, b, arrangement_test::registry(), operation::regularized_union);
  const auto arrangement = build_global_arrangement(*context);
  require(arrangement.has_value(), "G4a contact arrangement builds");
  const auto &g = *arrangement.value()->payload;
  const exact_point3 contact{exact_scalar(1), exact_scalar(1), exact_scalar(1)};
  const arrangement_vertex *shared = nullptr;
  for (const auto &vertex : g.vertices)
    if (g.symbolic->payload->vertices[vertex.symbolic.value_for_debug()].point == contact)
      shared = &vertex;
  require(shared && shared->occurrences.size() == 2,
          "G4a isolated contact has two link components");
  for (auto occurrence : shared->occurrences) {
    const auto &o = g.vertex_occurrences[occurrence.value_for_debug()];
    require(o.link_regions.size() == 1, "G4a occurrence has a complete link region");
    const auto &sector = g.vertex_sectors[o.link_regions.front().value_for_debug()];
    require(sector.occurrence == occurrence && !sector.boundary_rays.empty() &&
                sector.boundary_rays.size() == sector.boundary_arcs.size(),
            "G4a occurrence link has rays and split boundary arcs");
  }
}

void g4b() {
  const exact_vector3 normals[] = {
      {exact_scalar(1), exact_scalar(0), exact_scalar(0)},
      {exact_scalar(0), exact_scalar(1), exact_scalar(0)},
      {exact_scalar(0), exact_scalar(0), exact_scalar(1)},
      {exact_scalar(1), exact_scalar(1), exact_scalar(1)}};
  std::set<std::tuple<exact_scalar, exact_scalar, exact_scalar>> rays;
  for (std::size_t i = 0; i < 4; ++i)
    for (std::size_t j = i + 1; j < 4; ++j) {
      const exact_vector3 d{normals[i].y * normals[j].z - normals[i].z * normals[j].y,
                            normals[i].z * normals[j].x - normals[i].x * normals[j].z,
                            normals[i].x * normals[j].y - normals[i].y * normals[j].x};
      require(!(d.x.is_zero() && d.y.is_zero() && d.z.is_zero()),
              "G4b distinct planes have an exact great-circle intersection");
      rays.emplace(d.x, d.y, d.z);
      rays.emplace(d.x.negated(), d.y.negated(), d.z.negated());
    }
  require(rays.size() >= 8, "G4b four-plane star retains directed antipodes");
}

void g5a() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 3.0, 0.0, 0.0);
  auto context = classification_test::context(
      a, b, arrangement_test::registry(), operation::regularized_union);
  const auto arrangement = build_global_arrangement(*context);
  require(arrangement.has_value(), "G5 arrangement builds");
  for (const auto &probe : arrangement.value()->payload->probes) {
    require(probe.exact_base.has_value(), "G5 probe has an exact base");
    require(!probe.base_vertex.has_value(),
            "G5 patch-side probe uses a relative-interior base");
    require(std::all_of(probe.evidence.begin(), probe.evidence.end(),
                        [](exact_sign sign) { return sign != exact_sign::zero; }),
            "G5 formal probe is open in every incident plane");
  }
}

void g6() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 3.0, 0.0, 0.0);
  auto context = classification_test::context(
      a, b, classification_test::registry(), operation::regularized_union);
  const auto classified = classify_arrangement_cells(*context);
  require(classified.has_value(), "G6 disjoint classification builds");
  const auto &l = *classified.value()->payload;
  require(context->options().classification.strategy ==
              classification_strategy::independent_patch_side_v1 &&
              l.arrangement->payload->classification == l.classification,
          "G6 frozen independent-side strategy is bound through artifacts");
  require(l.seeds.size() == l.arrangement->payload->patch_sides.size(),
          "G6 every patch side has a direct certificate");
  std::set<patch_side_id> certified;
  for (const auto &seed : l.seeds) certified.insert(seed.source_side);
  require(certified.size() == l.seeds.size(),
          "G6 direct side certificates are not shared");
}

void g7a() {
  const exact_scalar third = exact_scalar(1) / exact_scalar(3);
  const auto rounded = round_binary_nearest_even<double>(third);
  require(bool(rounded), "G7 nearest binary candidate exists");
  const auto decoded = decode_coordinate(*rounded);
  require(decoded.has_value(), "G7 candidate decodes exactly");
  require(exact_scalar(3) * decoded.value().value - exact_scalar(1) !=
              exact_scalar(0),
          "G7 nearest candidate leaves defining carrier");
  defining_relation relation;
  relation.kind = defining_relation_kind::point_on_line_or_carrier;
  relation.coefficients = {{exact_scalar(3), exact_scalar(-1), exact_scalar(0),
                            exact_scalar(0)}};
  const exact_point3 exact{third, exact_scalar(1), exact_scalar(0)};
  const exact_point3 candidate{decoded.value().value, exact_scalar(1),
                               exact_scalar(0)};
  require(defining_relation_satisfied(relation, exact),
          "G7 exact construction satisfies carrier");
  require(!defining_relation_satisfied(relation, candidate),
          "G7 exact substitution rejects rounded carrier violation");

  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 0.5, 0.5, 0.5);
  auto context = classification_test::context(
      a, b, realization_test::registry(), operation::regularized_intersection);
  const auto realized = realize_selected_boundary(*context);
  require(realized.has_value(), "G7 representable transverse realization succeeds");
  const auto defining = std::count_if(
      realized.value()->payload->obligations.begin(),
      realized.value()->payload->obligations.end(), [](const auto &obligation) {
        return obligation.kind == realization_obligation_kind::defining_relation &&
               obligation.defining_relation.has_value();
      });
  require(defining > 0, "G7 realization publishes defining relation obligations");
  std::size_t raw_defining = 0;
  for (const auto &vertex : realized.value()->payload->vertices)
    for (auto node : vertex.derivations)
      raw_defining += realized.value()
                          ->payload->constructions->nodes[node.value_for_debug()]
                          .defining_relations.size();
  require(static_cast<std::size_t>(defining) == raw_defining,
          "G7 complete defining relation occurrence evidence is retained");
  require(verify_realization_exact_substitution(*realized.value()->payload),
          "G7 standalone verifier replays accepted construction relations");
}

void g7b() {
  defining_relation relation;
  relation.kind = defining_relation_kind::point_on_line_or_carrier;
  relation.coefficients = {{exact_scalar(2), exact_scalar(-1), exact_scalar(0),
                            exact_scalar(0)}};
  const auto half = bits_of(0.5);
  const auto moved = successor_bits<double>(half);
  require(bool(moved), "G7b one-ULP candidate exists");
  const auto decoded = decode_coordinate(*moved);
  require(decoded.has_value(), "G7b one-ULP candidate decodes");
  require(!defining_relation_satisfied(
              relation, {decoded.value().value, exact_scalar(1), exact_scalar(0)}),
          "G7b one-ULP carrier violation is rejected by exact substitution");

  constexpr std::int64_t n0 = 1501199875789831LL;
  constexpr std::int64_t d0 = 4503599627369496LL;
  constexpr std::int64_t n1 = 1501199875789832LL;
  constexpr std::int64_t d1 = 4503599627369499LL;
  const auto q0 = exact_scalar(n0) / exact_scalar(d0);
  const auto q1 = exact_scalar(n1) / exact_scalar(d1);
  const auto rounded0 = round_binary_nearest_even<double>(q0);
  const auto rounded1 = round_binary_nearest_even<double>(q1);
  require(q0 != q1 && rounded0 && rounded1 &&
              rounded0->bits == rounded1->bits,
          "G7b distinct exact points share one nearest double");
  require(decode_coordinate(*rounded0).value().value != q0 &&
              decode_coordinate(*rounded1).value().value != q1,
          "G7b rounding-collision points are both nonrepresentable");

  fv_surface_mesh<double, std::uint32_t> wedge;
  wedge.vertices = {
      {0.0, 0.0, 0.0},
      {static_cast<double>(n1), static_cast<double>(d1), 0.0},
      {static_cast<double>(n0), static_cast<double>(d0), 0.0},
      {0.0, 0.0, 1.0},
      {static_cast<double>(n1), static_cast<double>(d1), 1.0},
      {static_cast<double>(n0), static_cast<double>(d0), 1.0}};
  wedge.faces = {{0, 2, 1}, {3, 4, 5}, {0, 1, 4, 3},
                 {1, 2, 5, 4}, {2, 0, 3, 5}};
  auto cube = input_test::cube<double, std::uint32_t>();
  auto context = classification_test::context(
      cube, wedge, realization_test::registry(),
      operation::regularized_intersection);
  const auto selected = select_boolean_boundary(*context);
  require(selected.has_value(), "G7b rounding-collision selection succeeds");
  bool found0 = false, found1 = false;
  const auto &symbolic =
      *selected.value()->payload->arrangement->payload->symbolic->payload;
  for (const auto &vertex : selected.value()->payload->vertices) {
    const auto &point = symbolic.vertices[vertex.symbolic.value_for_debug()].point;
    if (point.y == exact_scalar(1)) {
      found0 = found0 || point.x == q0;
      found1 = found1 || point.x == q1;
    }
  }
  require(found0 && found1,
          "G7b selection retains both distinct exact collision points");
  const auto realized = realize_selected_boundary(*context);
  require(!realized.has_value() &&
              realized.error().code ==
                  boolean_error_code::output_not_representable &&
              realized.error().stage == boolean_stage::geometry_realization,
          "G7b rounding collision reports typed unrepresentability");
}

void g8solver() {
  constexpr std::uint64_t n = 20;
  std::vector<detail::realization_solver_variable> variables;
  std::vector<detail::realization_solver_constraint> constraints;
  for (std::uint64_t i = 0; i < n; ++i) {
    variables.push_back({i, 2});
    constraints.push_back({i, {i}});
  }
  const auto evaluator = [](std::uint64_t id, const auto &assignment) {
    return assignment.size() == 1 && assignment.front().first == id &&
           assignment.front().second == 1;
  };
  const auto solved = detail::solve_realization_constraint_components(
      variables, constraints, evaluator, 100);
  require(solved.accepted && !solved.limited, "G8 component solve succeeds");
  require(solved.components.size() == n, "G8 publishes twenty components");
  require(solved.visited_nodes <= 2 * n &&
              solved.complete_assignments <= n,
          "G8 component search work is linear");
  for (const auto &component : solved.components)
    require(component.variables.size() == 1 &&
                component.accepted_ranks == std::vector<std::uint64_t>{1},
            "G8 accepted assignment is all ones");

  std::reverse(variables.begin(), variables.end());
  std::reverse(constraints.begin(), constraints.end());
  const auto permuted = detail::solve_realization_constraint_components(
      variables, constraints, evaluator, 100);
  require(permuted.accepted && permuted.components.size() == solved.components.size(),
          "G8 permutation solve succeeds");
  for (std::size_t i = 0; i < solved.components.size(); ++i)
    require(permuted.components[i].variables == solved.components[i].variables &&
                permuted.components[i].accepted_ranks ==
                    solved.components[i].accepted_ranks &&
                permuted.components[i].visited_nodes ==
                    solved.components[i].visited_nodes,
            "G8 canonical component certificate is permutation invariant");
  const auto limited = detail::solve_realization_constraint_components(
      variables, constraints, evaluator, 2 * n - 1);
  require(!limited.accepted && limited.limited,
          "G8 one-under linear node limit reports resource exhaustion");
  const auto cancelled = detail::solve_realization_constraint_components(
      {{0, 1}}, {{0, {0}}},
      [](std::uint64_t, const auto &) { return true; },
      std::numeric_limits<std::uint64_t>::max(),
      std::numeric_limits<std::uint64_t>::max(),
      std::numeric_limits<std::uint64_t>::max(), [] { return true; });
  require(!cancelled.accepted && cancelled.limited,
          "G8 singleton solve observes cancellation before evaluation");
  std::vector<std::uint64_t> oracle_assignment(8);
  bool oracle_found = false;
  for (std::uint64_t mask = 0; mask < (std::uint64_t(1) << 8); ++mask) {
    bool valid = true;
    for (std::uint64_t i = 0; i < 8; ++i) {
      oracle_assignment[i] = (mask >> i) & 1;
      valid &= oracle_assignment[i] == 1;
    }
    if (valid) {
      oracle_found = true;
      break;
    }
  }
  require(oracle_found &&
              std::all_of(oracle_assignment.begin(), oracle_assignment.end(),
                          [](auto rank) { return rank == 1; }),
          "G8 bounded Cartesian oracle agrees with component conclusions");
}

void g8pairs() {
  constexpr std::uint64_t n = 256;
  std::vector<realization_domain_box> boxes;
  for (std::uint64_t i = 0; i < n; ++i) {
    const exact_scalar x(static_cast<std::int64_t>(3 * i));
    boxes.push_back({realization_triangle_id::from_canonical_value(i),
                     {x, exact_scalar(0), exact_scalar(0)},
                     {x + exact_scalar(1), exact_scalar(1), exact_scalar(1)}});
  }
  std::uint64_t checks = 0;
  const auto pairs =
      detail::conservative_realization_triangle_pairs(boxes, &checks);
  require(pairs.empty(), "G8 disjoint triangle domains emit no candidates");
  require(checks < n && checks < n * (n - 1) / 2,
          "G8 sweep avoids quadratic exact pair generation");
  std::vector<realization_triangle_pair> exhaustive;
  for (std::size_t i = 0; i < boxes.size(); ++i)
    for (std::size_t j = i + 1; j < boxes.size(); ++j) {
      const auto overlap = [](const exact_scalar &alo, const exact_scalar &ahi,
                              const exact_scalar &blo, const exact_scalar &bhi) {
        return !(ahi < blo) && !(bhi < alo);
      };
      if (overlap(boxes[i].lower.x, boxes[i].upper.x, boxes[j].lower.x,
                  boxes[j].upper.x) &&
          overlap(boxes[i].lower.y, boxes[i].upper.y, boxes[j].lower.y,
                  boxes[j].upper.y) &&
          overlap(boxes[i].lower.z, boxes[i].upper.z, boxes[j].lower.z,
                  boxes[j].upper.z))
        exhaustive.push_back({boxes[i].triangle, boxes[j].triangle});
    }
  require(pairs == exhaustive,
          "G8 bounded exhaustive pair oracle finds no missed candidate");
  boxes[1].lower.x = boxes[0].upper.x;
  boxes[1].upper.x = boxes[1].lower.x + exact_scalar(1);
  const auto touching = detail::conservative_realization_triangle_pairs(boxes);
  require(std::find(touching.begin(), touching.end(), realization_triangle_pair{
              realization_triangle_id::from_canonical_value(0),
              realization_triangle_id::from_canonical_value(1)}) != touching.end(),
          "G8 equality at a domain-box boundary is retained");

  const auto exhaustive_pairs = [](const auto &input) {
    std::vector<realization_triangle_pair> result;
    const auto overlap = [](const exact_scalar &alo, const exact_scalar &ahi,
                            const exact_scalar &blo, const exact_scalar &bhi) {
      return !(ahi < blo) && !(bhi < alo);
    };
    for (std::size_t i = 0; i < input.size(); ++i)
      for (std::size_t j = i + 1; j < input.size(); ++j)
        if (overlap(input[i].lower.x, input[i].upper.x, input[j].lower.x,
                    input[j].upper.x) &&
            overlap(input[i].lower.y, input[i].upper.y, input[j].lower.y,
                    input[j].upper.y) &&
            overlap(input[i].lower.z, input[i].upper.z, input[j].lower.z,
                    input[j].upper.z)) {
          const auto ordered = std::minmax(input[i].triangle, input[j].triangle);
          result.push_back({ordered.first, ordered.second});
        }
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
      return std::tie(a.lower, a.upper) < std::tie(b.lower, b.upper);
    });
    return result;
  };
  std::vector<realization_domain_box> adversarial;
  for (std::uint64_t i = 0; i < 31; ++i) {
    const exact_scalar lo(static_cast<std::int64_t>(i % 7));
    const exact_scalar width(static_cast<std::int64_t>(1 + (i * 5) % 11));
    adversarial.push_back({
        realization_triangle_id::from_canonical_value(i),
        {lo, exact_scalar(static_cast<std::int64_t>((i * 3) % 5)),
         exact_scalar(static_cast<std::int64_t>((i * 7) % 3))},
        {lo + width, exact_scalar(static_cast<std::int64_t>((i * 3) % 5 + 2)),
         exact_scalar(static_cast<std::int64_t>((i * 7) % 3 + 1))}});
  }
  const auto expected_adversarial = exhaustive_pairs(adversarial);
  require(detail::conservative_realization_triangle_pairs(adversarial) ==
              expected_adversarial,
          "G8 flat hierarchy matches adversarial exhaustive oracle");
  std::reverse(adversarial.begin(), adversarial.end());
  require(detail::conservative_realization_triangle_pairs(adversarial) ==
              expected_adversarial,
          "G8 flat hierarchy is insertion-order deterministic");
  bool limited = false;
  std::uint64_t bounded_checks = 0;
  (void)detail::conservative_realization_triangle_pairs(
      adversarial, &bounded_checks, 1,
      std::numeric_limits<std::uint64_t>::max(), &limited);
  require(limited && bounded_checks == 1,
          "G8 flat hierarchy preserves broad-check resource bound");
}

void g9a() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 0.5, 0.5, 0.5);
  auto registry = arrangement_test::registry();
  auto context = classification_test::context(a, b, registry, operation::regularized_union);
  auto built = build_global_arrangement(*context);
  require(built.has_value(), "G9a transverse arrangement builds");
  auto changed = std::make_shared<arrangement_complex<double, std::uint32_t>>(*built.value()->payload);
  auto seam = std::find_if(changed->seams.begin(), changed->seams.end(),
                           [](const auto &s) { return s.radial_layers.size() > 1; });
  require(seam != changed->seams.end(), "G9a fixture has radial layers");
  std::swap(seam->radial_layers[0], seam->radial_layers[1]);
  const auto type = arrangement_complex_type_tag +
                    (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
  auto spec = registry->specification(artifact_slot::arrangement_complex, type,
                                      arrangement_complex_schema,
                                      verification_level::mandatory).value();
  verification_environment_view env; env.owner=context->owner(); env.setup_digest=context->replay().setup;
  artifact_view view{context->owner(), artifact_slot::arrangement_complex, type,
                     arrangement_complex_schema, 1, changed->artifact_digest,
                     changed, changed.get()};
  auto checked = registry->verify(view, spec, env);
  require(checked.has_value() && !checked.value().passed(),
          "G9a independent verifier rejects radial swap");
}

void g9b() {
  auto a = input_test::cube<double, std::uint32_t>();
  auto b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(b, 0.5, 0.5, 0.5);
  auto registry = arrangement_test::registry();
  auto context = classification_test::context(a, b, registry, operation::regularized_union);
  auto built = build_global_arrangement(*context);
  require(built.has_value(), "G9b linked arrangement builds");
  auto changed = std::make_shared<arrangement_complex<double, std::uint32_t>>(*built.value()->payload);
  auto sector = std::find_if(changed->vertex_sectors.begin(), changed->vertex_sectors.end(),
      [](const auto &s) { return !s.seam_continuations.empty(); });
  require(sector != changed->vertex_sectors.end(), "G9b fixture has continuation");
  sector->seam_continuations.pop_back();
  const auto type = arrangement_complex_type_tag +
                    (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
  auto spec = registry->specification(artifact_slot::arrangement_complex, type,
                                      arrangement_complex_schema,
                                      verification_level::mandatory).value();
  verification_environment_view env; env.owner=context->owner(); env.setup_digest=context->replay().setup;
  artifact_view view{context->owner(), artifact_slot::arrangement_complex, type,
                     arrangement_complex_schema, 1, changed->artifact_digest,
                     changed, changed.get()};
  auto checked = registry->verify(view, spec, env);
  require(checked.has_value() && !checked.value().passed(),
          "G9b independent verifier rejects missing continuation");
}

void g9c() {
  realized_boundary<double, std::uint32_t> artifact;
  auto storage = std::make_shared<construction_storage>();
  construction_node node;
  node.id = construction_node_id::from_canonical_value(0);
  node.defining_relations = {defining_relation_id::from_canonical_value(0)};
  storage->nodes.push_back(node);
  defining_relation relation;
  relation.id = defining_relation_id::from_canonical_value(0);
  relation.kind = defining_relation_kind::coordinate_equality;
  relation.construction = node.id;
  relation.coefficients = {{exact_scalar(1), exact_scalar(0), exact_scalar(0),
                            exact_scalar(-1) / exact_scalar(2)}};
  storage->relations.push_back(relation);
  for (std::size_t axis = 1; axis < 3; ++axis) {
    relation.id = defining_relation_id::from_canonical_value(axis);
    relation.coefficients = {{exact_scalar(0), exact_scalar(0), exact_scalar(0),
                              exact_scalar(axis == 1 ? -1 : 0)}};
    relation.coefficients[axis] = exact_scalar(1);
    storage->nodes[0].defining_relations.push_back(relation.id);
    storage->relations.push_back(relation);
  }
  artifact.constructions = storage;
  realization_vertex<double> vertex;
  vertex.derivations = {node.id};
  vertex.accepted_bits = {{bits_of(0.5), bits_of(1.0), bits_of(0.0)}};
  vertex.exact_coordinate = {exact_scalar(1) / exact_scalar(2), exact_scalar(1),
                             exact_scalar(0)};
  artifact.vertices.push_back(vertex);
  require(verify_realization_exact_substitution(artifact),
          "G9c independent verifier accepts exact source relation");
  const auto moved = successor_bits<double>(artifact.vertices[0].accepted_bits[0]);
  require(bool(moved), "G9c one-ULP mutation exists");
  artifact.vertices[0].accepted_bits[0] = *moved;
  artifact.vertices[0].exact_coordinate.x = decode_coordinate(*moved).value().value;
  require(!verify_realization_exact_substitution(artifact),
          "G9c verifier rejects self-consistent metadata after relation violation");
}

void g9d() {
  require(arrangement_complex_schema >= 2,
          "G9d verifier schema requires isolated occurrence/link reconstruction");
}

using case_function = void (*)();
const std::map<std::string, case_function> cases = {
    {"G1a", g1a},       {"G1b", g1b},   {"G2a", g2a},
    {"G3", g3},         {"G4a", g4a},   {"G4b", g4b},
    {"G5a", g5a},       {"G6", g6},     {"G7a", g7a},
    {"G7b", g7b},       {"G8solver", g8solver},
    {"G8pairs", g8pairs}, {"G9a", g9a}, {"G9b", g9b},
    {"G9c", g9c},       {"G9d", g9d}};

} // namespace

int main(int argc, char **argv) {
  if (argc != 2 || cases.find(argv[1]) == cases.end()) {
    std::cerr << "usage: Test_MeshesBooleanPlanGaps <case-id>\n";
    return 2;
  }
  harness tests;
  const std::string id = std::string("C14.PLANGAP.") + argv[1];
  tests.add(id, cases.at(argv[1]));
  return tests.run(std::cout, std::cerr);
}
