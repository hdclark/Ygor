#include "MeshBooleanGlobalArrangementFixtures.h"

#include <iostream>

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
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
