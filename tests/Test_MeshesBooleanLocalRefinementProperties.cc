#include "MeshBooleanLocalRefinementFixtures.h"
#include <iostream>
using namespace local_test;
int main() {
  try {
    auto r = local_test::registry();
    auto a = cube<double, std::uint32_t>(), b = cube<double, std::uint32_t>();
    translate(b, 3, 0, 0);
    boolean_options one;
    one.execution.max_threads = 1;
    auto c1 = context(a, b, r, one);
    auto x = refine_source_facets(*c1);
    require(x.has_value(), "single-thread refinement");
    boolean_options many;
    many.execution.max_threads = 4;
    auto c2 = context(a, b, r, many);
    auto y = refine_source_facets(*c2);
    require(y.has_value(), "multi-thread refinement");
    require(x.value()->payload->canonical_bytes ==
                y.value()->payload->canonical_bytes,
            "schedule-independent canonical bytes");
    for (const auto &f : x.value()->payload->facets)
      require(f.patches.size() == 1 &&
                  f.certificate.source_domain_faces == 1 &&
                  f.certificate.artificial_edges == 0,
              "untouched facet identity patch");

    std::uint64_t vertex_count = 0;
    for (const auto &facet : x.value()->payload->facets)
      vertex_count += facet.vertices.size();
    boolean_options limited;
    limited.resources.local_vertices = {false, vertex_count};
    auto at_limit_context = context(a, b, r, limited);
    require(refine_source_facets(*at_limit_context).has_value(),
            "local vertex exact limit");
    limited.resources.local_vertices = {false, vertex_count - 1};
    auto over_limit_context = context(a, b, r, limited);
    auto over_limit = refine_source_facets(*over_limit_context);
    require(!over_limit.has_value() &&
                over_limit.error().code == boolean_error_code::resource_limit &&
                over_limit_context->accountant().used(
                    resource_kind::local_vertices) == 0 &&
                over_limit_context->accountant().used(
                    resource_kind::local_atomic_edges) == 0,
            "local vertex one-over rolls back all local reservations");

    cancellation_source cancelled;
    auto cancelled_context = context(a, b, r, boolean_options{}, &cancelled);
    cancelled.cancel();
    auto stopped = refine_source_facets(*cancelled_context);
    require(!stopped.has_value() &&
                stopped.error().code == boolean_error_code::resource_limit &&
                cancelled_context->accountant().used(
                    resource_kind::local_vertices) == 0,
            "local cancellation publishes nothing");

    for (double offset : {0.125, 0.25, 0.5, 0.75, 1.25}) {
      auto generated_a = cube<double, std::uint32_t>();
      auto generated_b = cube<double, std::uint32_t>();
      translate(generated_b, offset, offset / 2.0, offset / 4.0);
      boolean_options generated_options;
      generated_options.execution.max_threads =
          offset == 0.5 ? 4U : 1U;
      auto generated_context =
          context(generated_a, generated_b, r, generated_options);
      auto generated = refine_source_facets(*generated_context);
      if (!generated.has_value())
        throw std::runtime_error("generated arrangement offset " +
                                 std::to_string(offset) + ": " +
                                 render_error(generated.error()));
      for (const auto &facet : generated.value()->payload->facets)
        exhaustive_oracle(facet);
    }
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
