#include "MeshBooleanIntersectionEventFixtures.h"
#include <iostream>
using namespace symbolic_test;
using namespace event_test;
namespace {
void encode_feature(canonical_encoder &e, const feature_ref &f) {
  e.byte(static_cast<std::uint8_t>(f.index()));
  std::visit([&](const auto &x) {
    using X = typename std::decay<decltype(x)>::type;
    if constexpr (std::is_same<X, original_vertex_ref>::value) {
      e.id(x.operand); e.id(x.vertex);
    } else if constexpr (std::is_same<X, facet_ref>::value) {
      e.id(x.operand); e.id(x.facet);
    } else {
      e.id(x);
    }
  }, f);
}
std::vector<std::uint8_t> encoded(const raw_source_incidence &x) {
  canonical_encoder e;
  encode_feature(e, x.source);
  e.byte(static_cast<std::uint8_t>(x.location));
  e.boolean(x.directed_edge_parameter.has_value());
  if (x.directed_edge_parameter) x.directed_edge_parameter->encode(e);
  e.boolean(x.local_side.has_value());
  if (x.local_side) e.byte(static_cast<std::uint8_t>(*x.local_side));
  e.byte(static_cast<std::uint8_t>(x.orientation));
  return e.bytes();
}
std::vector<std::uint8_t> encoded(const raw_curve_ownership &x) {
  canonical_encoder e;
  e.id(x.operand);
  encode_feature(e, x.source);
  e.byte(static_cast<std::uint8_t>(x.direction));
  e.u32(x.multiplicity);
  return e.bytes();
}
void comparator_equivalence() {
  std::vector<feature_ref> features{
      operand_a(), shell_id::from_canonical_value(2),
      edge_use_id::from_canonical_value(3), facet_id::from_canonical_value(4),
      original_vertex_ref{operand_b(), original_vertex_id::from_canonical_value(5)},
      facet_ref{operand_a(), facet_id::from_canonical_value(6)}};
  for (const auto &a : features) for (const auto &b : features) {
    canonical_encoder ea, eb;
    encode_feature(ea, a); encode_feature(eb, b);
    require(canonical_feature_less(a, b) == (ea.bytes() < eb.bytes()),
            "feature structural ordering matches bytes");
  }
  const std::vector<exact_scalar> parameters{
      exact_scalar(0), exact_scalar(1), exact_scalar(-1),
      exact_scalar(big_int(integer_sign::positive, big_uint(256)), big_uint(3)),
      exact_scalar(big_int(integer_sign::negative, big_uint(257)), big_uint(5))};
  std::vector<raw_source_incidence> incidences;
  for (const auto &feature : features)
    for (const auto &parameter : parameters) {
      incidences.push_back({feature,
                            local_incidence_location::directed_edge_open_interior,
                            parameter, exact_sign::negative,
                            orientation_parity::opposite});
      incidences.push_back({feature, local_incidence_location::facet_boundary,
                            std::nullopt, std::nullopt,
                            orientation_parity::agree});
    }
  for (const auto &a : incidences) for (const auto &b : incidences) {
    require(canonical_incidence_less(a, b) == (encoded(a) < encoded(b)),
            "incidence structural ordering matches bytes");
    require(canonical_incidence_equal(a, b) == (encoded(a) == encoded(b)),
            "incidence structural equality matches bytes");
  }
  std::vector<raw_curve_ownership> ownership;
  for (const auto &feature : features)
    for (auto direction : {orientation_parity::agree,
                           orientation_parity::opposite})
      ownership.push_back({operand_b(), feature, direction,
                           static_cast<std::uint32_t>(ownership.size() + 1)});
  for (const auto &a : ownership) for (const auto &b : ownership) {
    require(canonical_ownership_less(a, b) == (encoded(a) < encoded(b)),
            "ownership structural ordering matches bytes");
    require(canonical_ownership_equal(a, b) == (encoded(a) == encoded(b)),
            "ownership structural equality matches bytes");
  }
}
} // namespace
template <class T, class I>
void check_source_mappings(const raw_event_set<T, I> &events) {
  const auto &validated = *events.candidates->payload->validated->payload;
  auto event_point = [&](raw_event_id id) -> const exact_point3 & {
    for (const auto &point : events.points)
      if (point.id == id) return point.point;
    throw std::runtime_error("missing interval endpoint");
  };
  for (const auto &interval : events.intervals) {
    const auto &lower = event_point(interval.lower_point);
    const auto &upper = event_point(interval.upper_point);
    for (const auto &mapping : interval.source_intervals) {
      const auto &use = validated.edge_uses[mapping.edge_use.value_for_debug()];
      const auto &origin =
          validated.vertices[use.origin.value_for_debug()].exact_coordinate;
      const auto &destination =
          validated.vertices[use.destination.value_for_debug()].exact_coordinate;
      auto at = [&](const exact_scalar &t) {
        return origin + (destination - origin) * t;
      };
      if (mapping.orientation == orientation_parity::agree) {
        require(at(mapping.parameters.lower) == lower &&
                    at(mapping.parameters.upper) == upper,
                "agree source interval reconstruction");
      } else {
        require(at(mapping.parameters.upper) == lower &&
                    at(mapping.parameters.lower) == upper,
                "opposite source interval reconstruction");
      }
    }
  }
}
template <class T, class I> void run() {
  auto r = registry();
  for (int k = 1; k <= 3; ++k) {
    auto a = cube<T, I>(), b = cube<T, I>();
    translate(b, T(k) / T(4), T(1) / T(4), T(1) / T(4));
    auto c = context(a, b, r);
    auto e = discover_intersection_events(*c);
    require(e.has_value(), "generated events");
    std::uint64_t next = 0;
    for (const auto &x : e.value()->payload->classifications) {
      require(x.event_begin == next, "contiguous candidate ranges");
      next = x.event_end;
    }
    for (const auto &p : e.value()->payload->points)
      require(p.id.value_for_debug() < next, "dense event domain");
    check_source_mappings(*e.value()->payload);
  }

  std::vector<std::uint8_t> reference;
  for (std::uint32_t threads : {1U, 2U, 4U}) {
    auto a = cube<T, I>(), b = cube<T, I>();
    translate(b, T(1) / T(3), T(1) / T(4), T(1) / T(5));
    boolean_options options;
    options.execution.max_threads = threads;
    options.tracing.collect_noncanonical_timings = true;
    auto c = context(a, b, r, options);
    auto events = discover_intersection_events(*c);
    require(events.has_value(), "scheduled events");
    if (reference.empty()) reference = events.value()->payload->canonical_event_bytes;
    require(reference == events.value()->payload->canonical_event_bytes,
            "thread-count independent canonical events");
    const auto nonparallel = static_cast<std::uint64_t>(std::count_if(
        events.value()->payload->classifications.begin(),
        events.value()->payload->classifications.end(), [](const auto &entry) {
          return entry.plane_relation == event_plane_relation::nonparallel;
        }));
    const auto performance = c->performance();
    require(performance->stage(boolean_stage::intersection_events)
                .producer.value(
                    performance_counter::exact_carrier_polygon_tests) ==
                nonparallel * 2,
            "one cached carrier query per candidate facet");
    check_source_mappings(*events.value()->payload);
  }

  const std::vector<std::array<T, 2>> u = {
      {{T(0), T(0)}}, {{T(4), T(0)}}, {{T(4), T(4)}}, {{T(3), T(4)}},
      {{T(3), T(1)}}, {{T(1), T(1)}}, {{T(1), T(4)}}, {{T(0), T(4)}}};
  const std::vector<std::array<T, 2>> bar = {
      {{T(-1), T(2)}}, {{T(5), T(2)}}, {{T(5), T(3)}}, {{T(-1), T(3)}}};
  auto concave = extruded_ring<T, I>(u);
  auto crossing = extruded_ring<T, I>(bar);
  boolean_options oracle_options;
  oracle_options.verification = verification_level::exhaustive;
  oracle_options.tracing.collect_noncanonical_timings = true;
  auto oracle_context = context(concave, crossing, r, oracle_options);
  auto oracle_events = discover_intersection_events(*oracle_context);
  require(oracle_events.has_value(), "concave coplanar events");
  std::vector<exact_point2> exact_u, exact_bar;
  for (const auto &p : u)
    exact_u.push_back(point2(static_cast<std::int64_t>(p[0]),
                             static_cast<std::int64_t>(p[1])));
  for (const auto &p : bar)
    exact_bar.push_back(point2(static_cast<std::int64_t>(p[0]),
                               static_cast<std::int64_t>(p[1])));
  const auto expected_area = concave_intersection_area_oracle(exact_u, exact_bar);
  exact_scalar actual_area(0);
  std::size_t matching_regions = 0;
  for (const auto &region : oracle_events.value()->payload->regions) {
    if (region.interior_witness.z == exact_scalar(1)) {
      actual_area = actual_area + region.area;
      ++matching_regions;
    }
  }
  require(expected_area == exact_scalar(2), "independent concave oracle fixture");
  require(actual_area == expected_area, "concave overlay exact oracle area");
  require(matching_regions == 2, "disconnected concave overlap regions");
  const auto &event_verifier = oracle_context->performance()
      ->stage(boolean_stage::intersection_events).verifier;
  const auto exact_edge_tests = event_verifier.value(
      performance_counter::exact_coplanar_edge_tests);
  const auto edge_candidates = event_verifier.value(
      performance_counter::coplanar_edge_candidates);
  std::uint64_t exhaustive_edge_pairs = 0;
  const auto &validated =
      *oracle_events.value()->payload->candidates->payload->validated->payload;
  for (const auto &classification : oracle_events.value()->payload->classifications) {
    if (classification.plane_relation !=
            event_plane_relation::coincident_same_orientation &&
        classification.plane_relation !=
            event_plane_relation::coincident_opposite_orientation)
      continue;
    const auto &fa = validated.facet_geometry[
        classification.facets.operand_a_facet.value_for_debug()].ring2;
    const auto &fb = validated.facet_geometry[
        classification.facets.operand_b_facet.value_for_debug()].ring2;
    exhaustive_edge_pairs += fa.size() * fb.size();
  }
  require(exact_edge_tests > 0 && exact_edge_tests < exhaustive_edge_pairs,
          "coplanar verifier index reduces exact Cartesian edge tests");
  require(edge_candidates >= exact_edge_tests,
          "coplanar exact tests are limited to conservative candidates");
  require(event_verifier.resource(resource_kind::verifier_scratch_bytes) >
              oracle_events.value()->payload->points.size() *
                  sizeof(exact_point3),
           "coplanar verifier sweep storage is scratch-accounted");
  if constexpr (std::is_same<T, double>::value &&
                std::is_same<I, std::uint32_t>::value) {
    const auto type = raw_event_set_type_tag +
        (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8) +
        static_cast<std::uint64_t>(index_tag::uint32);
    const auto spec = r->specification(
        artifact_slot::raw_event_set, type, raw_event_set_schema,
        verification_level::mandatory);
    require(spec.has_value(), "event verifier resource specification");
    artifact_view view{oracle_context->owner(), artifact_slot::raw_event_set,
                       type, raw_event_set_schema, 1,
                       oracle_events.value()->artifact_digest,
                       oracle_events.value()->payload,
                       oracle_events.value()->payload.get()};
    const auto verify_with = [&](std::uint64_t scratch, std::uint64_t work) {
      resource_policy policy;
      policy.verifier_scratch_bytes = {false, scratch};
      policy.verifier_work = {false, work};
      resource_accountant accountant(policy);
      verification_environment_view env{
          oracle_context->owner(), oracle_context->replay().setup,
          oracle_context->contract().selected_operation(),
          &oracle_context->options(), coordinate_tag::binary64,
          index_tag::uint32, &oracle_context->kernel(), {}, &accountant,
          [] { return false; }};
      auto checked = r->verify(view, spec.value(), env);
      require(accountant.used(resource_kind::verifier_scratch_bytes) == 0 &&
                  accountant.used(resource_kind::verifier_work) == 0,
              "event verifier resource rollback");
      return checked;
    };
    const auto scratch = event_verifier.resource(
        resource_kind::verifier_scratch_bytes);
    const auto work = event_verifier.resource(resource_kind::verifier_work);
    auto exact = verify_with(scratch, work);
    require(exact.has_value() && exact.value().passed(),
            "event full verifier exact resource limits");
    auto short_scratch = verify_with(scratch - 1, work);
    require(!short_scratch.has_value() &&
                short_scratch.error().code == boolean_error_code::resource_limit,
            "event full verifier one-under scratch limit");
    auto short_work = verify_with(scratch, work - 1);
    require(!short_work.has_value() &&
                short_work.error().code == boolean_error_code::resource_limit,
            "event full verifier one-under work limit");

    std::uint32_t state = 0x9e3779b9U;
    for (std::size_t case_index = 0; case_index < 6; ++case_index) {
      auto left = cube<T, I>(), right = cube<T, I>();
      state = state * 1664525U + 1013904223U;
      const T x = case_index == 0
                      ? T(1)
                      : T(1 + (state & 7U)) / T(8);
      state = state * 1664525U + 1013904223U;
      const T y = T(state & 7U) / T(16);
      state = state * 1664525U + 1013904223U;
      const T z = T(state & 7U) / T(16);
      translate(right, x, y, z);
      boolean_options mandatory_options;
      mandatory_options.verification = verification_level::mandatory;
      auto mandatory_context = context(left, right, r, mandatory_options);
      auto mandatory = discover_intersection_events(*mandatory_context);
      require(mandatory.has_value(),
              "generated mandatory event verification");
      boolean_options exhaustive_options;
      exhaustive_options.verification = verification_level::exhaustive;
      auto exhaustive_context = context(left, right, r, exhaustive_options);
      auto exhaustive = discover_intersection_events(*exhaustive_context);
      require(exhaustive.has_value() &&
                  exhaustive.value()->payload->canonical_event_bytes ==
                      mandatory.value()->payload->canonical_event_bytes,
              "generated touching mandatory/exhaustive event differential");
    }
  }
}
int main() {
  try {
    comparator_equivalence();
    run<float, std::uint32_t>();
    run<float, std::uint64_t>();
    run<double, std::uint32_t>();
    run<double, std::uint64_t>();
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
