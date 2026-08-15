#include "MeshBooleanPerformanceSupport.h"
#include "MeshBooleanTestHarness.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

struct frozen_baseline {
  const char *fixture;
  operation op;
  const char *output_semantic;
  std::uint64_t broad_candidates, event_points, selected_patches;
  std::uint64_t output_vertices, output_faces, canonical_bytes;
};

const frozen_baseline baselines[] = {
    {"B0", operation::regularized_union, "0cffa01f0f8f4af4966790098976fb10", 0, 0, 12, 16, 24, 60486},
    {"B1", operation::regularized_union, "0cffa01f0f8f4af4966790098976fb10", 0, 0, 12, 16, 24, 60486},
    {"B2", operation::regularized_union, "e980c207c226f010726939446f75ad01", 6, 12, 12, 20, 36, 96626},
    {"B3", operation::regularized_union, "2123bfa67e69548aace2c76dd730ded2", 20, 48, 14, 16, 28, 69356},
    {"B4", operation::regularized_union, "80a53395c0aa7157a7fa4a9b74197c8f", 4, 8, 11, 16, 28, 74601},
    {"B5", operation::a_minus_b, "b0bdb9015e4d8717c35247fb48f6cc6b", 8, 16, 10, 16, 32, 88467},
    {"B6", operation::regularized_union, "0d68d6893beb86becac050d42ab1f403", 0, 0, 6, 8, 12, 30172},
    {"B7", operation::regularized_union, "6f1f52709bd05f6aa5438db3697c0883", 4, 8, 17, 24, 40, 104623},
    {"B8", operation::regularized_union, "5228fdb8248569e4ee4149e408da8721", 12, 28, 14, 20, 36, 103790},
};

performance_observation run(const frozen_baseline &baseline,
                             std::uint32_t threads,
                             predicate_execution_policy predicates=predicate_execution_policy::automatic,
                             formal_ray_index_execution_policy ray_index=formal_ray_index_execution_policy::accelerated) {
  struct policy_scope {
    predicate_execution_policy previous;
    explicit policy_scope(predicate_execution_policy policy):previous(exact_filter_policy::exchange_test_execution_policy(policy)){}
    ~policy_scope(){exact_filter_policy::exchange_test_execution_policy(previous);}
  } policy(predicates);
  struct ray_policy_scope {
    formal_ray_index_execution_policy previous;
    explicit ray_policy_scope(formal_ray_index_execution_policy policy):previous(formal_ray_index_policy::exchange_test_execution_policy(policy)){}
    ~ray_policy_scope(){formal_ray_index_policy::exchange_test_execution_policy(previous);}
  } ray_policy(ray_index);
  boolean_options options;
  options.execution.max_threads = threads;
  options.verification = verification_level::mandatory;
  options.tracing.collect_noncanonical_timings = true;
  return observe_performance_fixture<double, std::uint32_t>(
      baseline.fixture, 1, baseline.op, options);
}

void require_frozen(const frozen_baseline &baseline,
                    const performance_observation &observed) {
  require_equal(observed.typed_outcome(), std::string("success"),
                "frozen typed outcome");
  require_equal(observed.output_identity.hex(),
                std::string(baseline.output_semantic),
                "frozen canonical output identity");
  require_equal(observed.canonical_output_bytes.size(),
                static_cast<std::size_t>(baseline.canonical_bytes),
                "frozen canonical output byte count");
  require_equal(observed.counters.broad_final_candidates,
                baseline.broad_candidates, "frozen broad-phase candidates");
  require_equal(observed.counters.event_points, baseline.event_points,
                "frozen event point count");
  require_equal(observed.counters.selected_patches,
                baseline.selected_patches, "frozen selected patch count");
  require_equal(observed.counters.output_vertices, baseline.output_vertices,
                "frozen output vertex count");
  require_equal(observed.counters.output_faces, baseline.output_faces,
                "frozen output face count");
  require_equal(observed.producer_counters.value(
                    performance_counter::realization_exact_pair_checks),
                observed.counters.realization_pair_candidates,
                "exact realization checks follow conservative candidates");
  require_equal(
      observed.producer_counters.value(
          performance_counter::realization_solver_nodes),
      std::uint64_t(0), "exact-in-T singleton path bypasses generic DFS");
  const auto output_digest = std::find_if(
      observed.semantic_digests.begin(), observed.semantic_digests.end(),
      [](const auto &entry) { return entry.first == "output"; });
  require(output_digest != observed.semantic_digests.end(),
          "output stage semantic digest is exposed");
  require_equal(output_digest->second.hex(),
                std::string(baseline.output_semantic),
                "frozen output stage semantic digest");
}

void check_baseline(const frozen_baseline &baseline) {
  const auto single = run(baseline, 1);
  const auto two_threads = run(baseline, 2);
  const auto maximum_threads = run(baseline, 4);
  const auto exact_only = run(baseline, 1, predicate_execution_policy::exact_only);
  const auto exhaustive = run(baseline, 1, predicate_execution_policy::automatic,
                              formal_ray_index_execution_policy::exhaustive);
  require_equal(exhaustive.semantic_digests.size(), single.semantic_digests.size(),
                "exhaustive/indexed semantic stage count");
  for (std::size_t i = 0; i < single.semantic_digests.size(); ++i)
    require_equal(exhaustive.semantic_digests[i], single.semantic_digests[i],
                  std::string("exhaustive/indexed semantic identity at ") +
                      single.semantic_digests[i].first);
  require_equal(exhaustive.canonical_output_bytes, single.canonical_output_bytes,
                "exhaustive/indexed complete canonical output bytes");
  require_frozen(baseline, single);
  auto require_thread_identity = [&](const performance_observation &parallel) {
    require_frozen(baseline, parallel);
    require_equal(parallel.typed_outcome(), single.typed_outcome(),
                  "thread-count typed outcome identity");
    require_equal(parallel.semantic_digests, single.semantic_digests,
                  "thread-count stage semantic identity");
    require_equal(parallel.canonical_output_bytes, single.canonical_output_bytes,
                  "thread-count complete canonical output bytes");
    require(parallel.counters == single.counters,
            "thread-count deterministic artifact counters");
    require_equal(parallel.producer_counters.values,
                  single.producer_counters.values,
                  "thread-count producer performance counters");
    require_equal(parallel.verifier_counters.values,
                  single.verifier_counters.values,
                  "thread-count verifier performance counters");
    require_equal(parallel.producer_counters.resources,
                  single.producer_counters.resources,
                  "thread-count producer resource counters");
    require_equal(parallel.verifier_counters.resources,
                  single.verifier_counters.resources,
                  "thread-count verifier resource counters");
  };
  require_thread_identity(two_threads);
  require_thread_identity(maximum_threads);
  require_equal(exact_only.typed_outcome(), single.typed_outcome(),
                "filter-on/off typed outcome identity");
  require_equal(exact_only.semantic_digests, single.semantic_digests,
                "filter-on/off stage semantic identity");
  require_equal(exact_only.canonical_output_bytes, single.canonical_output_bytes,
                "filter-on/off complete canonical output bytes");
  require(exact_only.counters == single.counters,
          "filter-on/off deterministic artifact counters");
}

template <class T, class I>
void require_same_mesh(const fv_surface_mesh<T, I> &a,
                       const fv_surface_mesh<T, I> &b) {
  require_equal(a.faces, b.faces, "fixture face bytes are deterministic");
  require_equal(a.vertices.size(), b.vertices.size(),
                "fixture vertex count is deterministic");
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    require(a.vertices[i].x == b.vertices[i].x &&
                a.vertices[i].y == b.vertices[i].y &&
                a.vertices[i].z == b.vertices[i].z,
            "fixture coordinate bits are deterministic");
}

template <class T, class I>
void require_closed_oriented(const fv_surface_mesh<T, I> &mesh) {
  std::map<std::pair<I, I>, std::pair<std::uint64_t, std::int64_t>> edges;
  for (const auto &face : mesh.faces) {
    require(face.size() >= 3, "fixture face has at least three vertices");
    for (std::size_t i = 0; i < face.size(); ++i) {
      const auto from = face[i], to = face[(i + 1) % face.size()];
      require(from != to && static_cast<std::size_t>(from) < mesh.vertices.size() &&
                  static_cast<std::size_t>(to) < mesh.vertices.size(),
              "fixture edge has valid distinct endpoints");
      const auto key = std::minmax(from, to);
      auto &uses = edges[{key.first, key.second}];
      ++uses.first;
      uses.second += from < to ? 1 : -1;
    }
  }
  for (const auto &edge : edges)
    require(edge.second.first == 2 && edge.second.second == 0,
            "fixture is a closed consistently oriented B-rep");
}

void check_fixture_families() {
  for (const auto *name : performance_fixture_names())
    for (std::uint32_t size = 1; size <= 3; ++size) {
      const auto first = make_performance_fixture<double, std::uint32_t>(name, size);
      const auto second = make_performance_fixture<double, std::uint32_t>(name, size);
      require_same_mesh(first.a, second.a);
      require_same_mesh(first.b, second.b);
      require_closed_oriented(first.a);
      require_closed_oriented(first.b);
    }
}

} // namespace

int main() {
  harness tests;
  tests.add("C14.Performance.fixture_families",
            [] { check_fixture_families(); });
  for (const auto &baseline : baselines)
    tests.add(std::string("C14.Performance.") + baseline.fixture,
              [baseline] { check_baseline(baseline); });
  return tests.run(std::cout, std::cerr);
}
