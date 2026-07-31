#include "MeshBooleanGlobalArrangementFixtures.h"

#include <cmath>
#include <iostream>
#include <random>

using namespace arrangement_test;

namespace {

using mesh = fv_surface_mesh<double, std::uint32_t>;

mesh box(double x0, double x1, double y0, double y1, double z0, double z1) {
  auto result = cube<double, std::uint32_t>();
  for (auto &v : result.vertices) {
    v.x = v.x == 0.0 ? x0 : x1;
    v.y = v.y == 0.0 ? y0 : y1;
    v.z = v.z == 0.0 ? z0 : z1;
  }
  return result;
}

void rotate_rings(mesh &value) {
  for (std::size_t i = 0; i < value.faces.size(); ++i) {
    auto &face = value.faces[i];
    std::rotate(face.begin(), face.begin() + (i % face.size()), face.end());
  }
  std::reverse(value.faces.begin(), value.faces.end());
}

auto build_witness_fixture(mesh &shell, mesh &cutters,
                           std::shared_ptr<verifier_registry> verifiers) {
  boolean_options options;
  options.execution.max_threads = 1;
  options.tracing.collect_noncanonical_timings = true;
  auto ctx = context(shell, cutters, std::move(verifiers), options);
  auto result = build_global_arrangement(*ctx);
  if (!result.has_value())
    throw std::runtime_error(render_error(result.error()));
  require(result.value()->report.passed(),
          "exact patch witnesses pass independent verification");
  return std::make_pair(std::move(ctx), std::move(result.value()));
}

void require_cached_witness_work(
    const boolean_context<double, std::uint32_t> &ctx,
    const arrangement_complex<double, std::uint32_t> &arrangement) {
  std::uint64_t maximum_once_per_patch = 0;
  for (const auto &patch : arrangement.patches) {
    const auto axis = dominant_projection(patch.plane);
    std::vector<exact_scalar> xs;
    auto add_ring = [&](const auto &ring) {
      for (auto vertex : ring) {
        const auto symbolic = arrangement.vertices[vertex.value_for_debug()].symbolic;
        xs.push_back(project(arrangement.symbolic->payload
                                 ->vertices[symbolic.value_for_debug()].point,
                             axis)
                         .x);
      }
    };
    add_ring(patch.outer);
    for (const auto &hole : patch.holes)
      add_ring(hole);
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    maximum_once_per_patch += xs.empty() ? 0 : xs.size() - 1;

    const auto &negative = arrangement.probes[2 * patch.id.value_for_debug()];
    const auto &positive = arrangement.probes[2 * patch.id.value_for_debug() + 1];
    require(negative.exact_base == positive.exact_base,
            "both patch sides reuse one exact witness");
  }
  const auto counters =
      ctx.performance()->stage(boolean_stage::global_arrangement).producer;
  require(counters.value(performance_counter::patch_witness_slabs) > 0,
          "exact vertical decomposition is exercised");
  require(counters.value(performance_counter::patch_witness_slabs) <=
              maximum_once_per_patch,
          "slabs are visited at most once per global patch");
  require(counters.value(performance_counter::patch_witness_crossings) > 0,
          "incremental active edges produce exact crossings");
}

void exact_patch_witness_regressions() {
  auto shell = box(0.0, 8.0, 0.0, 8.0, 0.0, 4.0);
  auto cutters = box(1.0, 3.75, 1.0, 7.0, -1.0, 5.0);
  const auto second = box(4.25, 7.0, 1.0, 7.0, -1.0, 5.0);
  input_test::append(cutters, second);

  auto built = build_witness_fixture(shell, cutters, arrangement_test::registry());
  const auto &arrangement = *built.second->payload;
  structural_oracle(arrangement);
  require(std::any_of(arrangement.patches.begin(), arrangement.patches.end(),
                      [](const auto &patch) { return patch.holes.size() >= 2; }),
          "multiple through-columns produce a multi-hole global patch");
  require_cached_witness_work(*built.first, arrangement);

  auto permuted_shell = shell;
  auto permuted_cutters = cutters;
  rotate_rings(permuted_shell);
  rotate_rings(permuted_cutters);
  auto permuted =
      build_witness_fixture(permuted_shell, permuted_cutters,
                            arrangement_test::registry());
  require(permuted.second->payload->canonical_bytes == arrangement.canonical_bytes,
          "ring-order permutations preserve exact probes and canonical bytes");

  const double epsilon = 1.0 / 1024.0;
  auto thin_shell = box(0.0, 8.0, 0.0, 8.0, 0.0, 4.0);
  auto thin_column =
      box(epsilon, 8.0 - epsilon, epsilon, 8.0 - epsilon, -1.0, 5.0);
  auto thin = build_witness_fixture(thin_shell, thin_column,
                                    arrangement_test::registry());
  require(std::any_of(thin.second->payload->patches.begin(),
                      thin.second->payload->patches.end(),
                      [](const auto &patch) { return !patch.holes.empty(); }),
          "dyadically thin corridor remains an exact annular patch");
  require_cached_witness_work(*thin.first, *thin.second->payload);
}

mesh bipyramid(std::size_t valence) {
  constexpr double pi = 3.141592653589793238462643383279502884;
  mesh result;
  result.vertices.push_back({0.0, 0.0, 2.0});
  result.vertices.push_back({0.0, 0.0, -2.0});
  for (std::size_t i = 0; i < valence; ++i) {
    const auto angle = 2.0 * pi * static_cast<double>(i) /
                       static_cast<double>(valence);
    result.vertices.push_back({std::cos(angle), std::sin(angle), 0.0});
  }
  for (std::size_t i = 0; i < valence; ++i) {
    const auto current = static_cast<std::uint32_t>(2 + i);
    const auto next = static_cast<std::uint32_t>(2 + (i + 1) % valence);
    result.faces.push_back({0, current, next});
    result.faces.push_back({1, next, current});
  }
  return result;
}

void high_valence_verifier_work() {
  constexpr std::size_t valence = 64;
  auto shell = bipyramid(valence);
  mesh empty;
  auto verifiers = arrangement_test::registry();
  boolean_options options;
  options.execution.max_threads = 1;
  options.tracing.collect_noncanonical_timings = true;
  auto ctx = context(shell, empty, verifiers, options);
  auto built = build_global_arrangement(*ctx);
  if (!built.has_value())
    throw std::runtime_error(render_error(built.error()));

  const auto &arrangement = *built.value()->payload;
  std::uint64_t candidates = 0, comparison_bound = 0;
  std::size_t maximum_valence = 0;
  for (const auto &occurrence : arrangement.vertex_occurrences) {
    std::set<global_atomic_edge_id> incident;
    for (auto halfedge : occurrence.incident_halfedges)
      incident.insert(
          arrangement.halfedges[halfedge.value_for_debug()].edge);
    const auto count = incident.size();
    maximum_valence = std::max(maximum_valence, count);
    candidates += count;
    std::uint64_t levels = 0;
    for (std::size_t width = 1; width < count; width *= 2) ++levels;
    comparison_bound += 2 * count * levels;
  }
  require(maximum_valence >= valence,
          "high-valence occurrence reaches verifier direction sort");
  const auto counters =
      ctx->performance()->stage(boolean_stage::global_arrangement).verifier;
  require(counters.value(performance_counter::link_direction_candidates) ==
              candidates,
          "verifier visits each high-valence direction once");
  require(counters.value(
              performance_counter::link_direction_sort_comparisons) <=
              comparison_bound,
          "verifier direction comparisons obey n log n admission bound");
  require(counters.value(performance_counter::exact_link_direction_tests) <=
              candidates,
          "mandatory verifier exact-confirms only equal-key runs");
}

} // namespace

int main() {
  try {
    auto r = arrangement_test::registry();
    auto a = cube<double, std::uint32_t>();
    auto b = cube<double, std::uint32_t>();
    translate(b, 3, 0, 0);
    boolean_options one;
    one.execution.max_threads = 1;
    one.tracing.collect_noncanonical_timings = true;
    auto c1 = context(a, b, r, one);
    auto x = build_global_arrangement(*c1);
    require(x.has_value(), "single schedule");
    boolean_options many;
    many.execution.max_threads = 4;
    auto c2 = context(a, b, r, many);
    auto y = build_global_arrangement(*c2);
    require(y.has_value(), "parallel schedule");
    require(x.value()->payload->canonical_bytes == y.value()->payload->canonical_bytes,
            "schedule canonical");
    structural_oracle(*x.value()->payload);
    const auto counters =
        c1->performance()->stage(boolean_stage::global_arrangement).producer;
    require(counters.value(performance_counter::global_incidence_records) ==
                x.value()->payload->halfedges.size(),
            "one flat incidence record per halfedge");
    require(counters.value(performance_counter::global_index_lookups) > 0,
            "dense incidence lookups exercised");
    require(counters.value(performance_counter::exact_link_direction_tests) ==
                counters.value(performance_counter::link_direction_candidates),
            "all local direction candidates checked exactly");
    const auto verifier_counters =
        c1->performance()->stage(boolean_stage::global_arrangement).verifier;
    require(verifier_counters.value(performance_counter::global_index_lookups) > 0 &&
                verifier_counters.value(performance_counter::exact_link_direction_tests) <=
                    verifier_counters.value(performance_counter::link_direction_candidates) &&
                verifier_counters.value(
                    performance_counter::link_direction_sort_comparisons) > 0,
            "verifier uses indexed incidence and sorted exact directions");
    require(verifier_counters.resource(resource_kind::verifier_scratch_bytes) > 0 &&
                verifier_counters.resource(resource_kind::verifier_work) > 0,
            "global verifier scratch and work are accounted");

    const auto type = arrangement_complex_type_tag +
        (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8) +
        static_cast<std::uint64_t>(index_tag::uint32);
    const auto spec = r->specification(artifact_slot::arrangement_complex, type,
                                      arrangement_complex_schema,
                                      verification_level::mandatory);
    require(spec.has_value(), "global verifier resource specification");
    const auto direct_verify = [&](resource_policy policy) {
      resource_accountant accountant(policy);
      const auto &artifact = *x.value()->payload;
      artifact_view view{c1->owner(), artifact_slot::arrangement_complex, type,
                         arrangement_complex_schema, 1, artifact.artifact_digest,
                         x.value()->payload, x.value()->payload.get()};
      verification_environment_view env{
          c1->owner(), c1->replay().setup, c1->contract().selected_operation(),
          &c1->options(), coordinate_tag::binary64, index_tag::uint32,
          &c1->kernel(), {}, &accountant, [&] { return c1->cancelled(); }};
      auto checked = r->verify(view, spec.value(), env);
      require(accountant.used(resource_kind::verifier_scratch_bytes) == 0 &&
                  accountant.used(resource_kind::verifier_work) == 0,
              "global verifier scoped resource rollback");
      return checked;
    };
    resource_policy exact_verifier;
    exact_verifier.verifier_scratch_bytes = {
        false, verifier_counters.resource(resource_kind::verifier_scratch_bytes)};
    exact_verifier.verifier_work = {
        false, verifier_counters.resource(resource_kind::verifier_work)};
    const auto verifier_exact = direct_verify(exact_verifier);
    require(verifier_exact.has_value() && verifier_exact.value().passed(),
            "global verifier passes at exact scratch/work limits");
    auto short_work = exact_verifier;
    short_work.verifier_work.value--;
    const auto verifier_short = direct_verify(short_work);
    require(!verifier_short.has_value() &&
                verifier_short.error().code == boolean_error_code::resource_limit &&
                verifier_short.error().stage == boolean_stage::global_arrangement,
            "global verifier one-under work limit");
    auto short_scratch = exact_verifier;
    short_scratch.verifier_scratch_bytes.value--;
    const auto scratch_short = direct_verify(short_scratch);
    require(!scratch_short.has_value() &&
                scratch_short.error().code == boolean_error_code::resource_limit &&
                scratch_short.error().stage == boolean_stage::global_arrangement,
            "global verifier one-under scratch limit");

    cancellation_source stop;
    auto cc = context(a, b, r, boolean_options{}, &stop);
    stop.cancel();
    auto cancelled = build_global_arrangement(*cc);
    require(!cancelled.has_value() &&
                cancelled.error().code == boolean_error_code::resource_limit &&
                cc->artifacts().latest_generation(
                    artifact_slot::arrangement_complex) == 0,
            "cancel rollback");

    auto crossing_a = cube<double, std::uint32_t>();
    auto crossing_b = cube<double, std::uint32_t>();
    translate(crossing_b, 0.5, 0.5, 0.5);
    auto crossing_context = context(crossing_a, crossing_b, r);
    auto crossing = build_global_arrangement(*crossing_context);
    if (!crossing.has_value())
      throw std::runtime_error(render_error(crossing.error()));
    structural_oracle(*crossing.value()->payload);
    require(!crossing.value()->payload->seams.empty(),
            "transverse seams retained");

    auto same_a = cube<double, std::uint32_t>();
    auto same_b = cube<double, std::uint32_t>();
    auto same_context = context(same_a, same_b, r);
    auto same = build_global_arrangement(*same_context);
    if (!same.has_value())
      throw std::runtime_error(render_error(same.error()));
    structural_oracle(*same.value()->payload);
    require(!same.value()->payload->coincident_groups.empty(),
            "coincident sheets retained");
    for (const auto &group : same.value()->payload->coincident_groups)
      require(group.members.size() == 2, "separate coincidence members");

    const auto semantic_check = [&](const auto &published, const auto &ctx,
                                     verification_level level, auto mutate) {
      auto changed = std::make_shared<arrangement_complex<double, std::uint32_t>>(
          *published->payload);
      mutate(*changed);
      const auto level_spec = r->specification(
          artifact_slot::arrangement_complex, type, arrangement_complex_schema,
          level);
      require(level_spec.has_value(), "differential verifier specification");
      artifact_view view{ctx->owner(), artifact_slot::arrangement_complex, type,
                         arrangement_complex_schema, 1,
                         published->artifact_digest, changed, changed.get()};
      resource_accountant accountant(resource_policy{});
      verification_environment_view env{
          ctx->owner(), ctx->replay().setup, ctx->contract().selected_operation(),
          &ctx->options(), coordinate_tag::binary64, index_tag::uint32,
          &ctx->kernel(), {}, &accountant, [] { return false; }};
      auto checked = r->verify(view, level_spec.value(), env);
      require(accountant.used(resource_kind::verifier_scratch_bytes) == 0 &&
                  accountant.used(resource_kind::verifier_work) == 0,
              "differential verifier rollback");
      return checked;
    };
    const auto unchanged = [](auto &) {};
    const auto mandatory_valid = semantic_check(
        x.value(), c1, verification_level::mandatory, unchanged);
    const auto exhaustive_valid = semantic_check(
        x.value(), c1, verification_level::exhaustive, unchanged);
    require(mandatory_valid.has_value() && exhaustive_valid.has_value() &&
                mandatory_valid.value().passed() &&
                exhaustive_valid.value().passed(),
            "bounded exhaustive direction oracle agrees on valid artifact");
    std::mt19937 random(0x080b11u);
    for (std::size_t trial = 0; trial < 48; ++trial) {
      const auto mutation = random() % 10;
      const auto mutate = [mutation](auto &changed) {
        switch (mutation) {
        case 0: changed.refined_digest.bytes[0] ^= 1; break;
        case 1: changed.local_maps.front().global_fragments.front() ^= 1; break;
        case 2: changed.halfedges.front().sheet_mate = changed.halfedges.front().id; break;
        case 3: changed.seams.clear(); break;
        case 4: changed.coincident_groups.clear(); break;
        case 5: changed.transitions.front().region_crossing =
                    !changed.transitions.front().region_crossing; break;
        case 6: changed.canonical_bytes.push_back(0); break;
        case 7: changed.vertex_occurrences.front().local_germs.clear(); break;
        case 8: changed.link_rays.front().antipode = changed.link_rays.front().id; break;
        default: changed.probes.front().evidence.front() = exact_sign::zero; break;
        }
      };
      const auto &fixture = mutation == 3 ? crossing : mutation == 4 ? same : x;
      const auto &fixture_context =
          mutation == 3 ? crossing_context : mutation == 4 ? same_context : c1;
      const auto mandatory = semantic_check(
          fixture.value(), fixture_context, verification_level::mandatory, mutate);
      const auto exhaustive = semantic_check(
          fixture.value(), fixture_context, verification_level::exhaustive, mutate);
      require(mandatory.has_value() && exhaustive.has_value() &&
                  mandatory.value().outcome == exhaustive.value().outcome &&
                  mandatory.value().results.size() == exhaustive.value().results.size(),
              "mandatory/exhaustive semantic differential outcome");
      bool causal_failure = false;
      for (std::size_t i = 0; i < mandatory.value().results.size(); ++i) {
        require(mandatory.value().results[i].code ==
                    exhaustive.value().results[i].code &&
                    mandatory.value().results[i].status ==
                    exhaustive.value().results[i].status,
                "mandatory/exhaustive deterministic invariant result");
        const auto status = mandatory.value().results[i].status;
        if (!causal_failure && status == check_status::failed)
          causal_failure = true;
        else
          require(status == (causal_failure
                                 ? check_status::not_run_due_to_prior_failure
                                 : check_status::passed),
                  "ordered checks stop after one causal failure");
      }
      require(causal_failure && !mandatory.value().passed(),
              "mutation produces one causal verifier failure");
    }

    auto partial_a = cube<double, std::uint32_t>();
    auto partial_b = cube<double, std::uint32_t>();
    translate(partial_b, 0.5, 0, 0);
    auto partial_context = context(partial_a, partial_b, r);
    auto partial = build_global_arrangement(*partial_context);
    if (!partial.has_value())
      throw std::runtime_error(render_error(partial.error()));
    structural_oracle(*partial.value()->payload);
    require(!partial.value()->payload->coincident_groups.empty(),
            "partial coplanar common atoms");

    boolean_options limited;
    limited.resources.probe_descriptors = {false, 1};
    auto limited_context = context(a, b, r, limited);
    auto over = build_global_arrangement(*limited_context);
    require(!over.has_value() &&
                over.error().code == boolean_error_code::resource_limit &&
                limited_context->artifacts().latest_generation(
                    artifact_slot::arrangement_complex) == 0,
            "probe resource rollback");

    exact_patch_witness_regressions();
    high_valence_verifier_work();
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
