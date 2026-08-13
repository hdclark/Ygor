#include "ClassificationFixtures.h"

#include <utility>

namespace classification_tests {

std::string diagnostic(const bounded_boolean_error &error) {
  return std::string(error.summary) + " [subcode " +
         std::to_string(error.subcode) + ", checkpoint " +
         std::to_string(error.checkpoint) + "]";
}

classification_fixture build_classification_fixture(const mesh_type &a,
                                                    const mesh_type &b,
                                                    boolean_operation operation) {
  (void)operation; // Classification is operation-neutral in V1; the pipeline is
                   // built with the fixture's intersection context.
  classification_fixture fixture;
  fixture.broad = broad_phase_tests::build(
      a, b, bounded::source_triangulation_provider_kind::indexed_dependency_v1,
      true, bounded_execution_mode::serial_v1, 1);

  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::relation_capabilities relation_caps;
  relation_caps.owner = fixture.broad.predecessor.context.owner;
  relation_caps.resources = &resources;
  auto relations = bounded::build_signed_feature_relations(
      fixture.broad.predecessor.context, *fixture.broad.predecessor.precision,
      fixture.broad.artifact, relation_caps);
  require(relations.has_value(), "classification relation build failed");
  fixture.relations = *relations.value();

  bounded::intersection_capabilities intersection_caps;
  intersection_caps.owner = fixture.broad.predecessor.context.owner;
  intersection_caps.resources = &resources;
  auto intersections = bounded::build_canonical_intersection_complex(
      fixture.broad.predecessor.context, *fixture.broad.predecessor.precision,
      fixture.relations, intersection_caps);
  require(intersections.has_value(), "classification intersection build failed");
  fixture.intersections = *intersections.value();

  bounded::classification_capabilities classification_caps;
  classification_caps.owner = fixture.broad.predecessor.context.owner;
  classification_caps.resources = &resources;
  auto classification = bounded::build_classification_complex(
      fixture.broad.predecessor.context, *fixture.broad.predecessor.precision,
      fixture.broad.predecessor.manifolds, fixture.relations,
      fixture.intersections, classification_caps);
  if (!classification.has_value())
    throw std::runtime_error("classification build failed: " +
                             diagnostic(*classification.error()));
  fixture.classification = *classification.value();
  return fixture;
}

classification_fixture box_fixture(scalar x0, scalar y0, scalar z0, scalar x1,
                                   scalar y1, scalar z1, scalar X0, scalar Y0,
                                   scalar Z0, scalar X1, scalar Y1, scalar Z1,
                                   boolean_operation operation) {
  return build_classification_fixture(
      broad_phase_tests::box(x0, y0, z0, x1, y1, z1),
      broad_phase_tests::box(X0, Y0, Z0, X1, Y1, Z1), operation);
}

} // namespace classification_tests
