#include "MeshBooleanLocalRefinementFixtures.h"
#include <iostream>
using namespace local_test;
int main() {
  try {
    auto r = local_test::registry();
    auto a = cube<double, std::uint32_t>(), b = cube<double, std::uint32_t>();
    translate(b, 0.5, 0.5, 0.5);
    auto c = context(a, b, r);
    auto result = refine_source_facets(*c);
    if (!result.has_value())
      throw std::runtime_error(render_error(result.error()));
    const auto &x = *result.value()->payload;
    require(result.value()->report.passed(), "verified");
    require(x.facets.size() == x.validated->payload->facets.size(),
            "one refinement per facet");
    require(!x.shared_edges.empty(), "shared semantic edges");
    require(c->artifacts().latest_generation(
                artifact_slot::refined_facet_patches) == 1,
            "refinement atomically published in artifact catalog");
    auto repeated = refine_source_facets(*c);
    require(repeated.has_value() && repeated.value().get() == result.value().get(),
            "refinement publication is idempotent");
    std::uint64_t patches = 0;
    for (const auto &f : x.facets) {
      require(f.certificate.halfedges == 2 * f.certificate.edges,
              "paired halfedges");
      require(f.certificate.vertices - f.certificate.edges +
                      f.certificate.faces ==
                  1 + f.certificate.components,
              "Euler certificate");
      require(f.certificate.patch_double_area ==
                  f.certificate.source_double_area,
              "area certificate");
      require(!f.source_boundary.empty(), "source chains");
      for (const auto &patch : f.patches)
        require(patch.id.owner == c->owner() && patch.id.facet == f.facet,
                "patch reference owner and facet binding");
      for (const auto &chain : f.source_boundary)
        require(!chain.edges.empty(), "source chain covered");
      patches += f.patches.size();
    }
    require(patches > 0, "positive patches");

    const auto type = refined_facet_patches_type_tag +
                      (static_cast<std::uint64_t>(coordinate_tag::binary64)
                       << 8);
    auto spec = r->specification(artifact_slot::refined_facet_patches, type,
                                 refined_facet_patches_schema,
                                 verification_level::mandatory)
                    .value();
    verification_environment_view environment;
    environment.owner = c->owner();
    environment.setup_digest = c->replay().setup;
    environment.coordinate = coordinate_tag::binary64;
    environment.index = index_tag::uint32;
    environment.exact_kernel = &c->kernel();
    environment.accountant = &c->accountant();
    auto mutation_rejected = [&](auto mutate, const char *message) {
      auto changed = std::make_shared<
          refined_facet_patches<double, std::uint32_t>>(x);
      mutate(*changed);
      artifact_view view{c->owner(), artifact_slot::refined_facet_patches,
                         type, refined_facet_patches_schema, 1,
                         x.artifact_digest, changed, changed.get()};
      auto checked = r->verify(view, spec, environment);
      if (!checked.has_value())
        throw std::runtime_error(std::string(message) + ": " +
                                 render_error(checked.error()));
      if (checked.value().passed())
        throw std::runtime_error(std::string(message) + ": verifier passed");
    };
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().vertices.front().symbolic =
              symbolic_vertex_id::from_canonical_value(
                  artifact.symbolic->payload->vertices.size());
        },
        "symbolic vertex mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          auto &halfedge = artifact.facets.front().halfedges.front();
          halfedge.twin = halfedge.id;
        },
        "twin mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().walks.front().signed_double_area =
              exact_scalar(0);
        },
        "walk area mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().certificate.components++;
        },
        "Euler mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.shared_edges.front().occurrences.clear();
        },
        "shared edge mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().point_incidences.front().used_by_edge = false;
        },
        "point incidence mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().source_boundary.front().forward =
              !artifact.facets.front().source_boundary.front().forward;
        },
        "source map mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().certificate.source_domain_faces++;
        },
        "expanded certificate mutation rejected");
    mutation_rejected(
        [](auto &artifact) {
          artifact.facets.front().patches.front().id.owner =
              context_owner_token{};
        },
        "patch owner mutation rejected");
    mutation_rejected(
        [](auto &artifact) { artifact.canonical_bytes.push_back(0); },
        "canonical encoding mutation rejected");
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
