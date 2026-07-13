#include "MeshBooleanSymbolicRegistryFixtures.h"
#include <iostream>
using namespace symbolic_test;
int main() {
  try {
    auto r = registry();
    auto a = cube<double, std::uint32_t>(), b = cube<double, std::uint32_t>();
    translate(b, 0.5, 0.5, 0.5);
    auto c = context(a, b, r);
    auto s = build_symbolic_complex(*c);
    if (!s.has_value())
      throw std::runtime_error("symbolic: " + render_error(s.error()));
    require(s.value()->report.passed(), "verified");
    const auto &x = *s.value()->payload;
    require(x.constructions.get() ==
                x.raw_events->payload->constructions.get(),
            "construction storage identity retained");
    require(x.original_vertices.size() == x.validated->payload->vertices.size(),
            "all originals mapped");
    require(x.raw_points.size() == x.raw_events->payload->points.size(),
            "all raw points mapped");
    require(x.edge_sequences.size() == x.validated->payload->edges.size(),
            "all edges sequenced");
    for (const auto &e : x.edge_sequences) {
      require(e.parameters.front() == exact_scalar(0) &&
                  e.parameters.back() == exact_scalar(1),
              "edge endpoints");
      for (std::size_t i = 1; i < e.parameters.size(); ++i)
        require(e.parameters[i - 1] < e.parameters[i], "strict edge order");
    }
    require(x.raw_regions.size() == x.raw_events->payload->regions.size(),
            "all regions mapped");
    for (const auto &region : x.raw_regions)
      for (const auto &cycle : region.boundary_cycles) {
        require(cycle.role == region_cycle_role::outer,
                "symbolic cycle role");
        require(cycle.vertices.size() == cycle.boundary_intervals.size(),
                "symbolic boundary interval references");
        for (const auto &atoms : cycle.boundary_intervals)
          require(!atoms.empty(), "boundary interval atomization");
      }
    for (const auto &m : x.raw_intervals)
      require(!m.atomic_intervals.empty(), "raw interval atomized");
    for (const auto &curve : x.curves)
      if (curve.kind == symbolic_curve_kind::atomic_interval)
        require(!curve.ownership.empty(), "atomic ownership retained");
    require(x.directed_edge_views.size() == x.validated->payload->edge_uses.size(),
            "all directed edge views");
    for (const auto &view : x.directed_edge_views) {
      const auto &use = x.validated->payload->edge_uses[view.edge_use.value_for_debug()];
      const auto &twin = x.directed_edge_views[use.twin.value_for_debug()];
      require(view.sequence == twin.sequence && view.forward != twin.forward,
              "twin views reverse one sequence");
    }
    for (const auto &curve : x.curves) {
      if (curve.kind != symbolic_curve_kind::carrier) continue;
      const auto &d = curve.carrier.direction;
      if (!d.x.is_zero())
        require(d.x == exact_scalar(1) && curve.carrier.anchor.x.is_zero(),
                "canonical x carrier");
      else if (!d.y.is_zero())
        require(d.y == exact_scalar(1) && curve.carrier.anchor.y.is_zero(),
                "canonical y carrier");
      else
        require(d.z == exact_scalar(1) && curve.carrier.anchor.z.is_zero(),
                "canonical z carrier");
    }
    require(!x.angular_orders.empty(), "planar angular orders published");
    require(!x.radial_orders.empty(), "carrier radial orders published");
    for (const auto &order : x.angular_orders) {
      require(order.kind == symbolic_order_kind::planar_angular &&
                  !order.groups.empty(), "angular order schema");
      for (const auto &group : order.groups)
        require(!group.curves.empty(), "angular equality group");
    }
    for (const auto &order : x.radial_orders) {
      require(order.kind == symbolic_order_kind::carrier_radial &&
                  !order.groups.empty(), "radial order schema");
      for (const auto &group : order.groups)
        require(!group.facets.empty(), "radial equality group");
    }
    for (const auto &vertex : x.vertices)
      for (auto use_id : vertex.edge_uses) {
        const auto &use = x.validated->payload->edge_uses[use_id.value_for_debug()];
        require(std::binary_search(vertex.edge_uses.begin(), vertex.edge_uses.end(),
                                   use.twin), "closed twin incidence");
        require(std::binary_search(vertex.undirected_edges.begin(),
                                   vertex.undirected_edges.end(), use.edge),
                "closed edge incidence");
      }
    for (const auto &mapping : x.raw_points) {
      const auto raw = std::find_if(x.raw_events->payload->points.begin(),
                                    x.raw_events->payload->points.end(),
                                    [&](const auto &point) {
                                      return point.id == mapping.source;
                                    });
      require(raw != x.raw_events->payload->points.end(), "mapped raw point");
      const auto &vertex = x.vertices[mapping.symbolic.value_for_debug()];
      for (const auto &derivation : raw->derivations)
        require(std::binary_search(vertex.constructions.begin(),
                                   vertex.constructions.end(),
                                   derivation.construction),
                "point construction provenance retained");
    }
    auto mutated = std::make_shared<symbolic_complex<double, std::uint32_t>>(x);
    require(!mutated->directed_edge_views.empty(), "view mutation fixture");
    mutated->directed_edge_views.front().forward =
        !mutated->directed_edge_views.front().forward;
    const auto symbolic_type = symbolic_complex_type_tag +
        (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
    auto symbolic_spec = r->specification(
        artifact_slot::symbolic_complex, symbolic_type, symbolic_complex_schema,
        verification_level::mandatory).value();
    verification_environment_view symbolic_env;
    symbolic_env.owner = c->owner();
    symbolic_env.setup_digest = c->replay().setup;
    symbolic_env.coordinate = coordinate_tag::binary64;
    symbolic_env.index = index_tag::uint32;
    symbolic_env.exact_kernel = &c->kernel();
    symbolic_env.accountant = &c->accountant();
    artifact_view symbolic_view{c->owner(), artifact_slot::symbolic_complex,
        symbolic_type, symbolic_complex_schema, 1, x.artifact_digest, mutated,
        mutated.get()};
    auto rejected = r->verify(symbolic_view, symbolic_spec, symbolic_env);
    require(rejected.has_value() && !rejected.value().passed(),
            "independent twin-view mutation check");
    auto verify_symbolic_mutation = [&](auto mutate, const char *message) {
      auto changed =
          std::make_shared<symbolic_complex<double, std::uint32_t>>(x);
      mutate(*changed);
      artifact_view view{c->owner(), artifact_slot::symbolic_complex,
          symbolic_type, symbolic_complex_schema, 1, x.artifact_digest, changed,
          changed.get()};
      auto result = r->verify(view, symbolic_spec, symbolic_env);
      require(result.has_value() && !result.value().passed(), message);
    };
    verify_symbolic_mutation(
        [](auto &complex) {
          complex.vertices.front().id = symbolic_vertex_id::from_canonical_value(
              complex.vertices.size());
        },
        "symbolic vertex ID mutation rejected");
    verify_symbolic_mutation(
        [](auto &complex) {
          complex.vertices.front().point.x = complex.vertices.front().point.x +
                                             exact_scalar(1);
        },
        "symbolic point mutation rejected");
    verify_symbolic_mutation(
        [](auto &complex) {
          complex.original_vertices.front().symbolic =
              symbolic_vertex_id::from_canonical_value(complex.vertices.size());
        },
        "original vertex mapping mutation rejected");
    verify_symbolic_mutation(
        [](auto &complex) {
          require(complex.edge_sequences.front().parameters.size() >= 2,
                  "edge sequence mutation fixture");
          complex.edge_sequences.front().parameters.back() = exact_scalar(0);
        },
        "edge sequence parameter mutation rejected");
    verify_symbolic_mutation(
        [](auto &complex) {
          auto interval = std::find_if(complex.curves.begin(), complex.curves.end(),
              [](const auto &curve) {
                return curve.kind == symbolic_curve_kind::atomic_interval;
              });
          require(interval != complex.curves.end() && interval->lower,
                  "atomic interval mutation fixture");
          interval->upper = interval->lower;
        },
        "atomic interval endpoint mutation rejected");
    verify_symbolic_mutation(
        [](auto &complex) {
          require(!complex.raw_intervals.empty(),
                  "raw interval mapping mutation fixture");
          complex.raw_intervals.front().atomic_intervals.clear();
        },
        "raw interval mapping mutation rejected");
    auto missing_class = std::make_shared<symbolic_complex<double, std::uint32_t>>(x);
    missing_class->vertices.pop_back();
    artifact_view class_view{c->owner(), artifact_slot::symbolic_complex,
        symbolic_type, symbolic_complex_schema, 1, x.artifact_digest,
        missing_class, missing_class.get()};
    auto rejected_class = r->verify(class_view, symbolic_spec, symbolic_env);
    require(rejected_class.has_value() && !rejected_class.value().passed(),
            "independent equivalence reconstruction detects missing class");
    auto bad_order = std::make_shared<symbolic_complex<double, std::uint32_t>>(x);
    require(!bad_order->angular_orders.empty(), "order mutation fixture");
    bad_order->angular_orders.front().groups.front().curves.clear();
    artifact_view order_view{c->owner(), artifact_slot::symbolic_complex,
        symbolic_type, symbolic_complex_schema, 1, x.artifact_digest,
        bad_order, bad_order.get()};
    auto rejected_order = r->verify(order_view, symbolic_spec, symbolic_env);
    require(rejected_order.has_value() && !rejected_order.value().passed(),
            "angular order mutation rejected");
    verify_symbolic_mutation(
        [](auto &complex) {
          require(!complex.radial_orders.empty(), "radial order mutation fixture");
          complex.radial_orders.front().groups.front().facets.clear();
        },
        "radial order mutation rejected");
    auto equal_context = context(a, a, r);
    auto equal = build_symbolic_complex(*equal_context);
    require(equal.has_value(), "equal cube symbolic complex");
    const auto &coplanar = *equal.value()->payload;
    require(!coplanar.raw_regions.empty(), "equal cube raw regions");
    for (const auto &region : coplanar.raw_regions) {
      require(!region.boundary_cycles.empty(), "coplanar region cycles");
      require(region.boundary_cycles.front().role == region_cycle_role::outer,
              "coplanar outer cycle role");
      for (const auto &cycle : region.boundary_cycles) {
        require(cycle.vertices.size() == cycle.boundary_intervals.size(),
                "coplanar boundary mapping size");
        for (const auto &atoms : cycle.boundary_intervals)
          require(!atoms.empty(), "coplanar boundary atoms");
      }
    }
    auto repeated_context = context(a, a, r);
    auto repeated = build_symbolic_complex(*repeated_context);
    require(repeated.has_value() &&
                repeated.value()->payload->canonical_symbolic_bytes ==
                    coplanar.canonical_symbolic_bytes,
            "equal cube symbolic determinism");
    boolean_options vertex_limit;
    vertex_limit.resources.symbolic_vertices = {false, x.vertices.size()};
    vertex_limit.resources.symbolic_curves = {false, x.curves.size()};
    auto limited_context = context(a, b, r, vertex_limit);
    require(build_symbolic_complex(*limited_context).has_value(),
            "symbolic exact limits");
    boolean_options one_over = vertex_limit;
    one_over.resources.symbolic_vertices = {false, x.vertices.size() - 1};
    auto over_context = context(a, b, r, one_over);
    auto over = build_symbolic_complex(*over_context);
    require(!over.has_value() &&
                over.error().code == boolean_error_code::resource_limit &&
                over.error().stage == boolean_stage::symbolic_registry &&
                over_context->accountant().used(resource_kind::symbolic_vertices) == 0 &&
                over_context->accountant().used(resource_kind::symbolic_curves) == 0,
            "symbolic one-over rolls back");
    cancellation_source cancelled;
    auto cancelled_context = context(a, b, r, boolean_options{}, &cancelled);
    cancelled.cancel();
    auto stopped = build_symbolic_complex(*cancelled_context);
    require(!stopped.has_value() &&
                stopped.error().code == boolean_error_code::resource_limit &&
                stopped.error().stage == boolean_stage::symbolic_registry &&
                cancelled_context->accountant().used(resource_kind::symbolic_vertices) == 0,
            "symbolic cancellation rolls back");
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
