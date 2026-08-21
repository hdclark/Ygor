#include "BroadPhaseFixtures.h"
#include "GoldenRelationsV1.h"
#include "qualification/RelationExactOracle.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/RelationQueries.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
namespace qualification = ygor::mesh_boolean::qualification;
using broad_phase_tests::built_fixture;
using broad_phase_tests::diagnostic;
using broad_phase_tests::require;

namespace ygor::mesh_boolean::bounded {

struct relation_artifact_test_access final {
  template <class T, class I>
  static const auto &source_edge_stage(
      const signed_feature_relations<T, I> &artifact) {
    return artifact.source_edge_stage_;
  }

  template <class T, class I>
  static const auto &source_facet_stage(
      const signed_feature_relations<T, I> &artifact) {
    return artifact.source_facet_stage_;
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace {

using scalar = double;
using index_type = std::uint32_t;
using artifact_ptr = std::shared_ptr<
    const bounded::signed_feature_relations<scalar, index_type>>;

bounded::relation_capabilities capabilities(
    built_fixture &fixture, bounded::resource_manager &resources) {
  bounded::relation_capabilities result;
  result.owner = fixture.predecessor.context.owner;
  result.resources = &resources;
  return result;
}

artifact_ptr build_relation(built_fixture &fixture) {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  auto outcome = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities(fixture, resources));
  if (!outcome.has_value())
    throw std::runtime_error(diagnostic(*outcome.error()));
  return *outcome.value();
}

// A minimal exact-support snapshot for oracle unit fixtures.  Only the rounded
// nominal bits participate in exact classification; the enclosures are pinned
// to the nominal for this focused oracle unit.
bounded::bounded_geometry_snapshot3<double>
snapshot3(double x, double y, double z) {
  bounded::bounded_geometry_snapshot3<double> result;
  result.rounded = {x, y, z};
  result.lower = {x, y, z};
  result.upper = {x, y, z};
  return result;
}

bounded::source_facet_source_facet_relation_record<double>
make_facet_facet_record(std::array<std::array<double, 3>, 3> first,
                        std::array<std::array<double, 3>, 3> second) {
  bounded::source_facet_source_facet_relation_record<double> record;
  for (std::size_t point = 0; point < 3; ++point) {
    record.support_points[0][point] =
        snapshot3(first[point][0], first[point][1], first[point][2]);
    record.support_points[1][point] =
        snapshot3(second[point][0], second[point][1], second[point][2]);
  }
  return record;
}

void test_exact_facet_facet_unit() {
  // Four support classifications derived entirely from integer support points,
  // so the exact answer is unique and hand-auditable.
  const auto transverse = make_facet_facet_record(
      {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}},
      {{{0, 0, 0}, {1, 0, 1}, {0, 1, 0}}});
  const auto parallel_separated = make_facet_facet_record(
      {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}},
      {{{0, 0, 2}, {1, 0, 2}, {0, 1, 2}}});
  const auto coplanar_same = make_facet_facet_record(
      {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}},
      {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}});
  const auto coplanar_opposite = make_facet_facet_record(
      {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}},
      {{{1, 0, 0}, {0, 0, 0}, {0, 1, 0}}});

  require(qualification::classify_facet_facet_exact(transverse).support ==
              bounded::source_facet_support_relation_class::transverse,
          "exact oracle classifies transverse support");
  require(qualification::classify_facet_facet_exact(parallel_separated).support ==
              bounded::source_facet_support_relation_class::parallel_separated,
          "exact oracle classifies parallel separated support");
  const auto same = qualification::classify_facet_facet_exact(coplanar_same);
  require(same.support ==
                  bounded::source_facet_support_relation_class::
                      coplanar_same_orientation &&
              same.coplanar && same.same_orientation,
          "exact oracle classifies coplanar same-orientation support");
  const auto opposite =
      qualification::classify_facet_facet_exact(coplanar_opposite);
  require(opposite.support ==
                  bounded::source_facet_support_relation_class::
                      coplanar_opposite_orientation &&
              opposite.coplanar && !opposite.same_orientation,
          "exact oracle classifies coplanar opposite-orientation support");
}

void verify_facet_facet_oracle(const artifact_ptr &artifact,
                               bool require_coplanar = false) {
  const auto &stage =
      bounded::relation_artifact_test_access::source_facet_stage(*artifact);
  require(stage != nullptr,
          "exact facet/facet oracle requires the facet/facet stage");
  std::size_t checked = 0;
  std::size_t coplanar = 0;
  for (const auto &record : stage->relations) {
    std::string failure;
    require(qualification::exact_facet_facet_record_agrees(record, failure),
            failure.c_str());
    coplanar +=
        record.classification ==
                    bounded::source_facet_support_relation_class::
                        coplanar_same_orientation ||
                record.classification ==
                    bounded::source_facet_support_relation_class::
                        coplanar_opposite_orientation
            ? 1U
            : 0U;
    ++checked;
  }
  require(checked == stage->evaluation_count && checked != 0,
          "exact facet/facet oracle covers every compute-once support relation");
  if (require_coplanar)
    require(coplanar != 0,
            "facet/facet oracle exercises coplanar support classification");
}

void verify_edge_edge_oracle(const artifact_ptr &artifact) {
  const auto &stage =
      bounded::relation_artifact_test_access::source_edge_stage(*artifact);
  require(stage != nullptr,
          "exact edge/edge oracle requires the source-edge stage");
  std::size_t checked = 0;
  for (const auto &record : stage->relations) {
    std::string failure;
    require(qualification::exact_source_edge_record_agrees(record, failure),
            failure.c_str());
    ++checked;
  }
  require(checked == stage->evaluation_count,
          "exact edge/edge oracle covers every compute-once edge relation");
}

void verify_oracle_families(const artifact_ptr &artifact) {
  verify_edge_edge_oracle(artifact);
  verify_facet_facet_oracle(artifact);
}

void test_exact_oracle_integration() {
  // Proper transverse overlap also produces a coplanar opposite-orientation
  // containment support between the touching top/bottom faces, so this single
  // fixture exercises both the transverse and coplanar facet/facet families.
  auto overlap = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0));
  const auto artifact = build_relation(overlap);
  verify_edge_edge_oracle(artifact);
  verify_facet_facet_oracle(artifact, /*require_coplanar=*/true);

  // Equal coincident shells may exceed the conservative resource budget and
  // are therefore a legitimate typed-failure case, never an internal
  // invariant violation.  When they do publish, the exact oracle must still
  // cover every compute-once relation.
  const auto verify_equal = []() {
    auto fixture = broad_phase_tests::build(broad_phase_tests::box(),
                                            broad_phase_tests::box());
    bounded::resource_manager resources(
        resource_policy::conservative_defaults());
    auto outcome = bounded::build_signed_feature_relations(
        fixture.predecessor.context, *fixture.predecessor.precision,
        fixture.artifact, capabilities(fixture, resources));
    if (outcome.has_value()) {
      verify_oracle_families(*outcome.value());
      return;
    }
    require(outcome.error()->category !=
                bounded_boolean_error_category::internal_invariant_error,
            "coincident shells fail closed with a typed error, never an internal invariant");
  };
  verify_equal();
}

std::string digest_hex(const bounded_boolean_digest &digest) {
  static const char *digits = "0123456789abcdef";
  std::string result;
  result.reserve(digest.bytes.size() * 2);
  for (const auto byte : digest.bytes) {
    result.push_back(digits[byte >> 4U]);
    result.push_back(digits[byte & 0x0FU]);
  }
  return result;
}

void test_golden_artifact() {
  const auto build_golden = []() {
    auto fixture = broad_phase_tests::build(
        broad_phase_tests::box(),
        broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0),
        bounded::source_triangulation_provider_kind::indexed_dependency_v1,
        true, bounded_execution_mode::serial_v1, 1);
    return build_relation(fixture);
  };
  const auto first = build_golden();
  const auto second = build_golden();
  require(first->canonical_bytes() == second->canonical_bytes() &&
              first->digest() == second->digest(),
          "golden fixture is byte-identical across independent rebuilds");

  const bool golden_known =
      std::memcmp(first->digest().bytes.data(),
                  qualification::golden_relation_artifact_digest_v1,
                  first->digest().bytes.size()) == 0;
  if (!golden_known) {
    std::ostringstream stream;
    stream << "golden relation artifact digest drifted to " << digest_hex(first->digest());
    throw std::runtime_error(stream.str());
  }

  // Decode-by-rebuild must reproduce the committed golden bytes.
  auto decode_fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0));
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      first->canonical_bytes(), decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, decode_resources));
  if (!decoded.has_value())
    throw std::runtime_error(diagnostic(*decoded.error()));
  require((*decoded.value())->canonical_bytes() == first->canonical_bytes(),
          "golden artifact decodes to identical canonical bytes");
}

} // namespace

int main() {
  try {
    test_exact_facet_facet_unit();
    test_exact_oracle_integration();
    test_golden_artifact();
    std::cout << "Component 07 exact-oracle and golden-artifact qualification passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
