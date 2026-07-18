#include "MeshBooleanSymbolicRegistryFixtures.h"
#include <iostream>
using namespace symbolic_test;
void reconciliation_collision_property() {
  symbolic_reconciliation_request a, b, equal;
  a.facet = b.facet = facet_id::from_canonical_value(1);
  a.first_curve = b.first_curve = symbolic_curve_id::from_canonical_value(2);
  a.second_curve = b.second_curve = symbolic_curve_id::from_canonical_value(3);
  a.point = {exact_scalar(1), exact_scalar(2), exact_scalar(3)};
  b.point = {exact_scalar(1), exact_scalar(2), exact_scalar(4)};
  a.canonical_key = b.canonical_key = digest{};
  equal = a;
  require(!reconciliation_request_equal(a, b),
          "digest collision does not merge reconciliation requests");
  require(reconciliation_request_less(a, b) != reconciliation_request_less(b, a),
          "digest collision has deterministic structural order");
  require(reconciliation_request_equal(a, equal),
          "equal reconciliation requests merge structurally");
}
template <class T, class I> void run() {
  auto r = registry();
  auto a = cube<T, I>(), b = cube<T, I>();
  translate(b, T(1) / T(2), T(1) / T(2), T(1) / T(2));
  boolean_options baseline_options;
  baseline_options.verification = verification_level::exhaustive;
  auto c = context(a, b, r, baseline_options);
  auto s = build_symbolic_complex(*c);
  require(s.has_value(), "registry property");
  const auto &x = *s.value()->payload;
  for (std::size_t i = 1; i < x.vertices.size(); ++i)
    require(
        lexicographic_compare(x.vertices[i - 1].point, x.vertices[i].point) < 0,
        "canonical point order");
  for (const auto &m : x.raw_points)
    require(m.symbolic.value_for_debug() < x.vertices.size(),
            "raw mapping range");
  for (const auto &m : x.raw_intervals)
    require(!m.atomic_intervals.empty(), "interval coverage");
  for (const auto &order : x.angular_orders)
    require(!order.groups.empty(), "angular order coverage");
  for (const auto &order : x.radial_orders)
    require(!order.groups.empty(), "radial order coverage");
  const auto expected = x.canonical_symbolic_bytes;
  const auto curve = std::find_if(x.curves.begin(), x.curves.end(),
                                  [](const auto &value) {
    return value.kind == symbolic_curve_kind::atomic_interval && value.lower &&
           value.upper && !value.raw_intervals.empty();
  });
  require(curve != x.curves.end(), "atomic curve for reconciliation audit");
  const auto *raw = find_raw_interval(*x.raw_events->payload,
                                      curve->raw_intervals.front());
  require(raw != nullptr, "raw interval for reconciliation audit");
  symbolic_reconciliation_request no_progress;
  no_progress.prior_digest = s.value()->artifact_digest;
  no_progress.prior_generation = s.value()->generation;
  no_progress.facet = raw->facets.operand_a_facet;
  no_progress.first_curve = curve->id;
  no_progress.second_curve = curve->id;
  no_progress.point = x.vertices[curve->lower->value_for_debug()].point;
  canonical_encoder request_key;
  request_key.id(no_progress.facet);
  request_key.id(no_progress.first_curve);
  request_key.id(no_progress.second_curve);
  encode(request_key, no_progress.point.x);
  encode(request_key, no_progress.point.y);
  encode(request_key, no_progress.point.z);
  no_progress.canonical_key = domain_digest(
      {{'Y', 'G', 'B', 'R', 'E', 'C', '0', '7'}}, request_key.bytes());
  auto rejected = reconcile_symbolic_complex(*c, s.value(), {no_progress});
  require(!rejected.has_value() &&
              rejected.error().code ==
                  boolean_error_code::internal_invariant_error &&
              c->artifacts().latest_generation(artifact_slot::symbolic_complex) ==
                  s.value()->generation &&
              c->accountant().used(resource_kind::reconciliation_requests) == 0 &&
              c->accountant().used(resource_kind::successor_generations) == 0,
          "already satisfied reconciliation cannot publish a successor");
  boolean_options mandatory_options;
  mandatory_options.verification = verification_level::mandatory;
  mandatory_options.tracing.collect_noncanonical_timings = true;
  auto mandatory_context = context(a, b, r, mandatory_options);
  auto mandatory = build_symbolic_complex(*mandatory_context);
  require(mandatory.has_value() &&
              mandatory.value()->payload->canonical_symbolic_bytes == expected,
           "mandatory sorted verifier preserves canonical artifact");
  const auto &verifier_counters = mandatory_context->performance()
      ->stage(boolean_stage::symbolic_registry).verifier;
  const auto point_candidates = verifier_counters.value(
      performance_counter::symbolic_point_candidates);
  const auto endpoint_incidences = verifier_counters.value(
      performance_counter::symbolic_endpoint_incidences);
  require(point_candidates > 0 &&
              point_candidates <= x.raw_intervals.size() * x.vertices.size(),
          "symbolic point index bounds interval point candidates");
  require(endpoint_incidences > 0 &&
              endpoint_incidences < x.angular_orders.size() * x.curves.size(),
          "symbolic endpoint index avoids angular order by curve scan");
  require(verifier_counters.resource(resource_kind::verifier_scratch_bytes) >
              x.vertices.size() * sizeof(exact_point3),
          "symbolic verifier indices are scratch-accounted");
  constexpr auto coordinate = std::is_same<T, double>::value
                                  ? coordinate_tag::binary64
                                  : coordinate_tag::binary32;
  constexpr auto index = std::is_same<I, std::uint64_t>::value
                             ? index_tag::uint64
                             : index_tag::uint32;
  const auto symbolic_type = symbolic_complex_type_tag +
      (static_cast<std::uint64_t>(coordinate) << 8) +
      static_cast<std::uint64_t>(index);
  auto mandatory_spec = r->specification(
      artifact_slot::symbolic_complex, symbolic_type, symbolic_complex_schema,
      verification_level::mandatory);
  require(mandatory_spec.has_value(), "symbolic resource specification");
  artifact_view mandatory_view{
      mandatory_context->owner(), artifact_slot::symbolic_complex,
      symbolic_type, symbolic_complex_schema, 1, mandatory.value()->artifact_digest,
      mandatory.value()->payload, mandatory.value()->payload.get()};
  for (const auto &entry : std::array<std::pair<resource_kind, std::uint64_t>, 2>{{
           {resource_kind::verifier_work,
            verifier_counters.resource(resource_kind::verifier_work)},
           {resource_kind::verifier_scratch_bytes,
            verifier_counters.resource(resource_kind::verifier_scratch_bytes)}}}) {
    require(entry.second > 0, "symbolic verifier resource measured");
    auto verify_limit = [&](std::uint64_t limit) {
      resource_policy policy;
      if (entry.first == resource_kind::verifier_work)
        policy.verifier_work = {false, limit};
      else
        policy.verifier_scratch_bytes = {false, limit};
      resource_accountant accountant(policy);
      verification_environment_view env{
          mandatory_context->owner(), mandatory_context->replay().setup,
          mandatory_context->contract().selected_operation(),
          &mandatory_context->options(), coordinate, index,
          &mandatory_context->kernel(), {}, &accountant, [] { return false; }};
      auto checked = r->verify(mandatory_view, mandatory_spec.value(), env);
      require(accountant.used(resource_kind::verifier_work) == 0 &&
                  accountant.used(resource_kind::verifier_scratch_bytes) == 0,
              "symbolic verifier resource rollback");
      return checked;
    };
    auto exact = verify_limit(entry.second);
    require(exact.has_value() && exact.value().passed(),
            "symbolic verifier exact resource admission");
    auto short_result = verify_limit(entry.second - 1);
    require(!short_result.has_value() &&
                short_result.error().code == boolean_error_code::resource_limit,
            "symbolic verifier one-under resource admission");
  }
  auto exhaustive_spec = r->specification(
      artifact_slot::symbolic_complex, symbolic_type, symbolic_complex_schema,
      verification_level::exhaustive);
  require(exhaustive_spec.has_value(), "symbolic exhaustive specification");
  auto exhaustive_options = mandatory_context->options();
  exhaustive_options.verification = verification_level::exhaustive;
  resource_accountant exhaustive_accountant(exhaustive_options.resources);
  verification_environment_view exhaustive_env{
      mandatory_context->owner(), mandatory_context->replay().setup,
      mandatory_context->contract().selected_operation(), &exhaustive_options,
      coordinate, index, &mandatory_context->kernel(), {},
      &exhaustive_accountant, [] { return false; }};
  const auto same_bytes = mandatory.value()->payload->canonical_symbolic_bytes;
  auto exhaustive_report =
      r->verify(mandatory_view, exhaustive_spec.value(), exhaustive_env);
  require(exhaustive_report.has_value() && exhaustive_report.value().passed() &&
              exhaustive_report.value().results.size() ==
                  exhaustive_spec.value().required_invariants.size() &&
              mandatory.value()->payload->canonical_symbolic_bytes == same_bytes,
          "same symbolic artifact passes mandatory and exhaustive verification");
  for (auto threads : {1U, 2U, 4U}) {
    for (unsigned repetition = 0; repetition != 4; ++repetition) {
      boolean_options options;
      options.verification = verification_level::exhaustive;
      options.execution.max_threads = threads;
      auto scheduled_context = context(a, b, r, options);
      auto scheduled = build_symbolic_complex(*scheduled_context);
      require(scheduled.has_value(), "scheduled registry build");
      require(scheduled.value()->payload->canonical_symbolic_bytes == expected,
              "schedule-independent canonical merge");
      require(scheduled.value()->report.passed(),
              "scheduled exhaustive verification");
    }
  }
}
int main() {
  try {
    reconciliation_collision_property();
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
