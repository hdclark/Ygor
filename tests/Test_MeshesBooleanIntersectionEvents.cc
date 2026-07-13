#include "MeshBooleanSymbolicRegistryFixtures.h"
#include <iostream>
using namespace symbolic_test;
int main() {
  try {
    auto r = registry();
    auto a = cube<double, std::uint32_t>(), b = cube<double, std::uint32_t>();
    translate(b, 0.5, 0.5, 0.5);
    auto c = context(a, b, r);
    auto e = discover_intersection_events(*c);
    require(e.has_value(), "events");
    require(e.value()->report.passed(), "verified");
    require(e.value()->payload->classifications.size() ==
                e.value()->payload->candidates->payload->candidates.size(),
            "ledger");
    require(!e.value()->payload->intervals.empty(), "transverse intervals");
    require(e.value()->payload->constructions &&
                !e.value()->payload->constructions->nodes.empty(),
            "construction storage retained");
    for (std::size_t i = 0;
         i < e.value()->payload->constructions->nodes.size(); ++i) {
      const auto &node = e.value()->payload->constructions->nodes[i];
      require(node.id.value_for_debug() == i, "dense construction IDs");
      for (auto child : node.children)
        require(child.value_for_debug() < i, "child-before-parent DAG");
    }
    for (const auto &classification : e.value()->payload->classifications)
      if (classification.aggregate == pair_aggregate_relation::disjoint)
        require(classification.carrier_begin == classification.carrier_end,
                "disjoint candidate has no carrier");
    for (const auto &p : e.value()->payload->points) {
      for (const auto &incidence : p.incidences)
        if (incidence.directed_edge_parameter)
          require(!(incidence.directed_edge_parameter.value() < exact_scalar(0)) &&
                      !(exact_scalar(1) < incidence.directed_edge_parameter.value()),
                  "no before-origin or after-destination edge incidence");
    }
    for (const auto &q : e.value()->payload->intervals) {
      require(q.carrier_parameters.lower < q.carrier_parameters.upper,
               "positive interval");
      require(!q.ownership.empty(), "explicit interval ownership");
    }
    auto equal_context = context(a, a, r);
    auto equal = discover_intersection_events(*equal_context);
    require(equal.has_value(), "equal coplanar events");
    require(!equal.value()->payload->regions.empty(), "coplanar regions");
    for (std::size_t i = 0; i < equal.value()->payload->points.size(); ++i)
      for (std::size_t j = i + 1; j < equal.value()->payload->points.size(); ++j)
        require(equal.value()->payload->points[i].candidate !=
                    equal.value()->payload->points[j].candidate ||
                    !(equal.value()->payload->points[i].point ==
                      equal.value()->payload->points[j].point),
                "candidate-local exact point normalization");
    for (const auto &region : equal.value()->payload->regions) {
      require(exact_scalar(0) < region.area, "positive region area");
      require(!region.boundary_cycles.empty() &&
                  region.boundary_cycles.front().vertices.size() >= 3,
               "closed region boundary");
      require(region.boundary_cycles.front().role == region_cycle_role::outer,
              "outer cycle role");
      for (const auto &cycle : region.boundary_cycles)
        require(cycle.vertices.size() == cycle.intervals.size() &&
                    cycle.ownership.size() == cycle.intervals.size(),
                  "boundary interval references");
    }
    std::size_t mapped_source_intervals = 0;
    for (const auto &interval : equal.value()->payload->intervals)
      mapped_source_intervals += interval.source_intervals.size();
    require(mapped_source_intervals != 0,
            "coplanar boundaries retain source parameter intervals");
    auto corrupt_events =
        std::make_shared<raw_event_set<double, std::uint32_t>>(
            *equal.value()->payload);
    corrupt_events->regions.front().boundary_cycles.front().role =
        region_cycle_role::hole;
    const auto event_type = raw_event_set_type_tag +
        (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
    auto event_spec = r->specification(artifact_slot::raw_event_set, event_type,
                                      raw_event_set_schema,
                                      verification_level::mandatory).value();
    verification_environment_view event_env;
    event_env.owner = equal_context->owner();
    event_env.setup_digest = equal_context->replay().setup;
    event_env.coordinate = coordinate_tag::binary64;
    event_env.index = index_tag::uint32;
    event_env.exact_kernel = &equal_context->kernel();
    event_env.accountant = &equal_context->accountant();
    artifact_view event_view{equal_context->owner(), artifact_slot::raw_event_set,
                             event_type, raw_event_set_schema, 1,
                             equal.value()->artifact_digest, corrupt_events,
                             corrupt_events.get()};
    auto rejected_event = r->verify(event_view, event_spec, event_env);
    require(rejected_event.has_value() && !rejected_event.value().passed(),
            "cycle role mutation detected");
    auto verify_mutation_rejected = [&](const std::shared_ptr<
                                             raw_event_set<double, std::uint32_t>> &mutated,
                                         const char *message) {
      artifact_view view{equal_context->owner(), artifact_slot::raw_event_set,
                         event_type, raw_event_set_schema, 1,
                         equal.value()->artifact_digest, mutated, mutated.get()};
      auto result = r->verify(view, event_spec, event_env);
      require(result.has_value() && !result.value().passed(), message);
    };
    auto mutate_events = [&](auto mutate, const char *message) {
      auto changed = std::make_shared<raw_event_set<double, std::uint32_t>>(
          *equal.value()->payload);
      mutate(*changed);
      verify_mutation_rejected(changed, message);
    };
    mutate_events(
        [](auto &events) { ++events.classifications.front().event_end; },
        "candidate event range mutation detected");
    mutate_events(
        [](auto &events) {
          events.classifications.front().aggregate =
              pair_aggregate_relation::disjoint;
        },
        "candidate aggregate mutation detected");
    mutate_events(
        [](auto &events) {
          events.points.front().point.x = events.points.front().point.x +
                                          exact_scalar(1);
        },
        "point coordinate mutation detected");
    mutate_events(
        [](auto &events) {
          events.regions.front().area = events.regions.front().area +
                                        exact_scalar(1);
        },
        "region area mutation detected");
    mutate_events(
        [](auto &events) {
          events.regions.front().interior_witness.x =
              events.regions.front().interior_witness.x + exact_scalar(8);
        },
        "region witness mutation detected");
    const auto &fixture_cycle = equal.value()->payload->regions.front()
                                    .boundary_cycles.front();
    const auto fixture_interval_id = fixture_cycle.intervals.front();
    auto fixture_interval = std::find_if(
        equal.value()->payload->intervals.begin(),
        equal.value()->payload->intervals.end(),
        [&](const auto &x) { return x.id == fixture_interval_id; });
    require(fixture_interval != equal.value()->payload->intervals.end(),
            "boundary mutation interval fixture");
    const auto fixture_interval_index = static_cast<std::size_t>(std::distance(
        equal.value()->payload->intervals.begin(), fixture_interval));
    auto edge_owner = std::find_if(
        fixture_interval->ownership.begin(), fixture_interval->ownership.end(),
        [](const auto &owner) {
          return std::holds_alternative<edge_use_id>(owner.source);
        });
    require(edge_owner != fixture_interval->ownership.end(),
            "boundary edge ownership fixture");
    const auto edge_owner_index = static_cast<std::size_t>(
        std::distance(fixture_interval->ownership.begin(), edge_owner));

    auto bad_endpoint = std::make_shared<raw_event_set<double, std::uint32_t>>(
        *equal.value()->payload);
    bad_endpoint->intervals[fixture_interval_index].upper_point =
        bad_endpoint->intervals[fixture_interval_index].lower_point;
    verify_mutation_rejected(bad_endpoint,
                              "boundary endpoint mutation detected");

    mutate_events(
        [&](auto &events) {
          require(!events.intervals[fixture_interval_index].source_intervals.empty(),
                  "source parameter mutation fixture");
          events.intervals[fixture_interval_index]
              .source_intervals.front().parameters.lower = exact_scalar(-1);
        },
        "source interval parameter mutation detected");

    auto mutate_mirrored_owner = [&](auto mutate, const char *message) {
      auto changed = std::make_shared<raw_event_set<double, std::uint32_t>>(
          *equal.value()->payload);
      mutate(changed->intervals[fixture_interval_index]
                 .ownership[edge_owner_index]);
      mutate(changed->regions.front().boundary_cycles.front()
                 .ownership.front()[edge_owner_index]);
      verify_mutation_rejected(changed, message);
    };
    mutate_mirrored_owner(
        [&](auto &owner) {
          auto id = std::get<edge_use_id>(owner.source).value_for_debug();
          owner.source = edge_use_id::from_canonical_value(
              (id + 1) % equal.value()->payload->candidates->payload->validated
                             ->payload->edge_uses.size());
        },
        "boundary ownership source mutation detected");
    mutate_mirrored_owner(
        [](auto &owner) {
          owner.direction = owner.direction == orientation_parity::agree
                                ? orientation_parity::opposite
                                : orientation_parity::agree;
        },
        "boundary ownership direction mutation detected");
    mutate_mirrored_owner(
        [](auto &owner) { ++owner.multiplicity; },
        "boundary ownership multiplicity mutation detected");
    auto missing_incidence =
        std::make_shared<raw_event_set<double, std::uint32_t>>(
            *equal.value()->payload);
    require(!missing_incidence->points.front().incidences.empty(),
            "point incidence fixture");
    missing_incidence->points.front().incidences.pop_back();
    artifact_view incidence_view{
        equal_context->owner(), artifact_slot::raw_event_set, event_type,
        raw_event_set_schema, 1, equal.value()->artifact_digest,
        missing_incidence, missing_incidence.get()};
    auto rejected_incidence = r->verify(incidence_view, event_spec, event_env);
    require(rejected_incidence.has_value() && !rejected_incidence.value().passed(),
            "missing topology incidence independently detected");
    auto bad_construction =
        std::make_shared<raw_event_set<double, std::uint32_t>>(
            *equal.value()->payload);
    require(!bad_construction->points.front().derivations.empty(),
            "construction mutation fixture");
    bad_construction->points.front().derivations.front().construction =
        construction_node_id::from_canonical_value(
            bad_construction->constructions->nodes.size());
    artifact_view construction_view{
        equal_context->owner(), artifact_slot::raw_event_set, event_type,
        raw_event_set_schema, 1, equal.value()->artifact_digest,
        bad_construction, bad_construction.get()};
    auto rejected_construction =
        r->verify(construction_view, event_spec, event_env);
    require(rejected_construction.has_value() &&
                !rejected_construction.value().passed(),
            "invalid construction reference detected");
    auto d = cube<double, std::uint32_t>();
    translate(d, 3, 0, 0);
    auto c2 = context(a, d, r);
    auto x = discover_intersection_events(*c2);
    require(x.has_value(), "disjoint");
    require(x.value()->payload->points.empty() &&
                x.value()->payload->intervals.empty(),
            "no disjoint events");
    const auto event_count = e.value()->payload->points.size() +
                             e.value()->payload->intervals.size() +
                             e.value()->payload->regions.size();
    boolean_options at_limit;
    at_limit.resources.raw_events = {false, event_count};
    auto at_context = context(a, b, r, at_limit);
    require(discover_intersection_events(*at_context).has_value(),
            "raw event exact limit");
    boolean_options one_over;
    one_over.resources.raw_events = {false, event_count - 1};
    auto over_context = context(a, b, r, one_over);
    auto over = discover_intersection_events(*over_context);
    require(!over.has_value() &&
                over.error().code == boolean_error_code::resource_limit &&
                over.error().stage == boolean_stage::intersection_events &&
                over_context->accountant().used(resource_kind::raw_events) == 0,
            "raw event one-over rolls back");
    cancellation_source cancelled;
    auto cancelled_context = context(a, b, r, boolean_options{}, &cancelled);
    cancelled.cancel();
    auto stopped = discover_intersection_events(*cancelled_context);
    require(!stopped.has_value() &&
                stopped.error().code == boolean_error_code::resource_limit &&
                stopped.error().stage == boolean_stage::intersection_events &&
                cancelled_context->accountant().used(resource_kind::raw_events) == 0,
            "event cancellation rolls back");
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
