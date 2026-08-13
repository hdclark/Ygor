#include "RelationTypeMatrixFixtures.h"
#include "qualification/RelationExactOracle.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
namespace qualification = ygor::mesh_boolean::qualification;
namespace rtm = relation_type_matrix;
using rtm::require;

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

// ---------------------------------------------------------------------------
// Full float/double x uint32/uint64 artifact coverage.
// ---------------------------------------------------------------------------

template <class T, class I>
void verify_type_index_profile() {
  auto a = rtm::typed_box<T, I>(T(0), T(0), T(0), T(1), T(1), T(1));
  auto b = rtm::typed_box<T, I>(T(0.25), T(0.25), T(1.0), T(0.75), T(0.75),
                                T(2.0));

  auto serial = rtm::build_typed<T, I>(a, b, bounded_execution_mode::serial_v1, 1);
  auto parallel = rtm::build_typed<T, I>(
      a, b, bounded_execution_mode::deterministic_parallel_v1, 2);
  const auto first = rtm::build_typed_relation(serial);
  const auto second = rtm::build_typed_relation(parallel);
  if (!first.success || !second.success)
    throw std::runtime_error("type/index profile failed: " +
                             rtm::describe_attempt(first) + " / " +
                             rtm::describe_attempt(second));
  require(!first.artifact->relations().empty() &&
              !first.artifact->constructions().empty() &&
              !first.artifact->event_seeds().empty(),
          "type/index profile publishes a complete non-empty artifact");
  require(first.artifact->canonical_bytes() ==
                  second.artifact->canonical_bytes() &&
              first.artifact->digest() == second.artifact->digest(),
          "serial and deterministic-parallel profiles publish identical bytes");

  // Independent exact oracle for every compute-once source-edge relation.
  const auto &edge_stage =
      bounded::relation_artifact_test_access::source_edge_stage(*first.artifact);
  require(edge_stage != nullptr,
          "type/index profile requires the source-edge stage");
  for (const auto &record : edge_stage->relations) {
    std::string failure;
    require(qualification::exact_source_edge_record_agrees(record, failure),
            failure.c_str());
  }
  const auto &facet_stage =
      bounded::relation_artifact_test_access::source_facet_stage(*first.artifact);
  require(facet_stage != nullptr,
          "type/index profile requires the facet/facet stage");
  for (const auto &record : facet_stage->relations) {
    std::string failure;
    require(qualification::exact_facet_facet_record_agrees(record, failure),
            failure.c_str());
  }
}

void test_type_index_matrix() {
  verify_type_index_profile<float, std::uint32_t>();
  verify_type_index_profile<float, std::uint64_t>();
  verify_type_index_profile<double, std::uint32_t>();
  verify_type_index_profile<double, std::uint64_t>();
}

// ---------------------------------------------------------------------------
// Forced alternate schedules and worker counts must publish byte-identical
// canonical artifacts and select the same primary failure.
// ---------------------------------------------------------------------------

void test_alternate_schedules_and_merges() {
  using scalar = double;
  using index_type = std::uint32_t;
  auto a = rtm::typed_box<scalar, index_type>(0, 0, 0, 1, 1, 1);
  auto b = rtm::typed_box<scalar, index_type>(0.25, 0.25, 1.0, 0.75, 0.75, 2.0);

  auto single = rtm::build_typed<scalar, index_type>(
      a, b, bounded_execution_mode::serial_v1, 1);
  auto two = rtm::build_typed<scalar, index_type>(
      a, b, bounded_execution_mode::deterministic_parallel_v1, 2);
  auto maximum = rtm::build_typed<scalar, index_type>(
      a, b, bounded_execution_mode::deterministic_parallel_v1, 7);

  const auto single_result = rtm::build_typed_relation(single);
  const auto two_result = rtm::build_typed_relation(two);
  const auto maximum_result = rtm::build_typed_relation(maximum);
  require(single_result.success && two_result.success && maximum_result.success,
          "alternate schedules publish successful artifacts");
  require(single_result.artifact->canonical_bytes() ==
                  two_result.artifact->canonical_bytes() &&
              two_result.artifact->canonical_bytes() ==
                  maximum_result.artifact->canonical_bytes() &&
              single_result.artifact->digest() ==
                  maximum_result.artifact->digest(),
          "worker count 1/2/7 publish byte-identical canonical artifacts");

  // A coincident-sheet fixture legitimately fails with a typed resource
  // error; every schedule must select the identical primary failure.
  auto coincident = rtm::build_typed<scalar, index_type>(
      a, a, bounded_execution_mode::serial_v1, 1);
  auto coincident_parallel = rtm::build_typed<scalar, index_type>(
      a, a, bounded_execution_mode::deterministic_parallel_v1, 7);
  const auto coincident_serial = rtm::build_typed_relation(coincident);
  const auto coincident_worker7 = rtm::build_typed_relation(coincident_parallel);
  require(coincident_serial.success == coincident_worker7.success,
          "coincident-sheet schedules agree on success or failure");
  if (!coincident_serial.success) {
    require(coincident_serial.error.category ==
                    coincident_worker7.error.category &&
                coincident_serial.error.subcode ==
                    coincident_worker7.error.subcode &&
                coincident_serial.error.checkpoint ==
                    coincident_worker7.error.checkpoint,
            "alternate schedules select the same primary failure");
  }
}

// ---------------------------------------------------------------------------
// Independent source-fan crossing conservation.
// ---------------------------------------------------------------------------

void test_fan_conservation() {
  auto a = rtm::typed_box<double, std::uint32_t>(0, 0, 0, 1, 1, 1);
  auto b = rtm::typed_box<double, std::uint32_t>(0.5, 0.5, 0.5, 1.5, 1.5, 1.5);
  auto fixture = rtm::build_typed<double, std::uint32_t>(
      a, b, bounded_execution_mode::serial_v1, 1);
  const auto attempt = rtm::build_typed_relation(fixture);
  require(attempt.success, "fan-conservation fixture publishes an artifact");

  const auto &crossings = attempt.artifact->crossings();
  std::vector<std::uint64_t> groups;
  groups.reserve(crossings.size());
  for (const auto &crossing : crossings)
    groups.push_back(crossing.source_fan_group);
  std::sort(groups.begin(), groups.end());
  groups.erase(std::unique(groups.begin(), groups.end()), groups.end());

  std::size_t resolved = 0;
  std::size_t nonzero_groups = 0;
  for (const auto group : groups) {
    std::int64_t total = 0;
    std::size_t members = 0;
    std::size_t owners = 0;
    std::size_t symbolic_carriers = 0;
    std::uint32_t expected_size = 0;
    for (const auto &crossing : crossings) {
      if (crossing.source_fan_group != group)
        continue;
      require(crossing.source_fan_resolved && crossing.locally_conservative,
              "every published crossing is fan-resolved and locally conservative");
      if (members == 0)
        expected_size = crossing.source_fan_group_size;
      require(crossing.source_fan_group_size == expected_size &&
                  crossing.source_fan_group_ordinal == members,
              "fan group membership and ordinals are complete and ordered");
      total += crossing.numeric_crossing;
      owners += crossing.numeric_owner ? 1U : 0U;
      symbolic_carriers += crossing.symbolic_crossing != 0 ? 1U : 0U;
      ++members;
    }
    require(members == expected_size,
            "fan group size matches its published membership");
    require(total >= -1 && total <= 1,
            "closed two-manifold fan crossing total is -1, 0, or +1");
    require(owners == (total != 0 ? 1U : 0U),
            "each nonzero fan total has exactly one numeric owner");
    require(symbolic_carriers <= 1,
            "symbolic crossing is carried on at most one canonical occurrence");
    ++resolved;
    nonzero_groups += total != 0 ? 1U : 0U;
  }
  require(resolved == groups.size(),
          "every source-fan group conserves its crossing total");
  require(nonzero_groups != 0,
          "qualification fixture exercises nonzero fan crossing totals");
}

// ---------------------------------------------------------------------------
// Production-path selection-boundary instrumentation.
// ---------------------------------------------------------------------------

void test_selection_boundary() {
  const auto is_classification_only = [](bounded::symbolic_expected_disposition value) {
    return value ==
               bounded::symbolic_expected_disposition::classification_only ||
           value == bounded::symbolic_expected_disposition::half_open_ownership ||
           value ==
               bounded::symbolic_expected_disposition::occurrence_separation ||
           value ==
               bounded::symbolic_expected_disposition::coincident_owner_eligibility;
  };
  const auto check = [&](const auto &artifact) {
    require(artifact->verification_evidence().selection_boundary_checked,
            "the independent verifier audited the downstream selection boundary");
    for (const auto &decision : artifact->symbolic_decisions()) {
      require(is_classification_only(decision.expected_disposition),
              "symbolic decisions publish no final retain/discard/suppress state");
      require(decision.nominal_geometry_unchanged,
              "symbolic decisions never change nominal geometry");
    }
    for (const auto &seed : artifact->event_seeds()) {
      if (!seed.has_symbolic_decision)
        continue;
      require(is_classification_only(seed.symbolic_expected),
              "event-seed symbolic disposition carries no final selection");
    }
  };

  auto a = rtm::typed_box<double, std::uint32_t>(0, 0, 0, 1, 1, 1);
  auto b = rtm::typed_box<double, std::uint32_t>(0.25, 0.25, 1.0, 0.75, 0.75,
                                                 2.0);
  auto fixture = rtm::build_typed<double, std::uint32_t>(
      a, b, bounded_execution_mode::serial_v1, 1);
  const auto attempt = rtm::build_typed_relation(fixture);
  require(attempt.success, "selection-boundary fixture publishes an artifact");
  check(attempt.artifact);
  require(!attempt.artifact->symbolic_decisions().empty(),
          "selection-boundary fixture publishes real symbolic decisions");

  // Equal coincident sheets legitimately fail with a typed resource error;
  // the selection-boundary audit still holds for any artifact that publishes.
  auto equal = rtm::build_typed<double, std::uint32_t>(
      a, a, bounded_execution_mode::serial_v1, 1);
  const auto equal_attempt = rtm::build_typed_relation(equal);
  if (equal_attempt.success)
    check(equal_attempt.artifact);
  else
    require(equal_attempt.error.category !=
                bounded_boolean_error_category::internal_invariant_error,
            "equal sheets fail closed with a typed error");
}

// ---------------------------------------------------------------------------
// Durable deterministic fuzz campaign with a deterministic shrinker.
// ---------------------------------------------------------------------------

struct fuzz_case final {
  double x0 = 0;
  double y0 = 0;
  double z0 = 0;
  double x1 = 1;
  double y1 = 1;
  double z1 = 1;
};

std::string describe(const fuzz_case &value) {
  std::ostringstream stream;
  stream << '[' << value.x0 << ',' << value.y0 << ',' << value.z0 << " -> "
         << value.x1 << ',' << value.y1 << ',' << value.z1 << ']';
  return stream.str();
}

fuzz_case shrink(fuzz_case value) {
  const auto half = [](double coordinate) {
    return coordinate == 0.0 ? coordinate : coordinate / 2.0;
  };
  for (std::size_t pass = 0; pass < 6; ++pass) {
    value.x0 = half(value.x0);
    value.y0 = half(value.y0);
    value.z0 = half(value.z0);
    value.x1 = 1.0 + half(value.x1 - 1.0);
    value.y1 = 1.0 + half(value.y1 - 1.0);
    value.z1 = 1.0 + half(value.z1 - 1.0);
  }
  return value;
}

void test_fuzz_and_shrink() {
  const std::array<fuzz_case, 10> cases{{
      {4.0, 4.0, 4.0, 5.0, 5.0, 5.0},
      {0.5, 0.25, 0.125, 1.5, 1.25, 1.125},
      {0.25, 0.25, 0.25, 0.75, 0.75, 0.75},
      {0.25, 0.25, 1.0, 0.75, 0.75, 2.0},
      {1.0, 1.0, 0.25, 2.0, 2.0, 0.75},
      {-0.5, -0.5, -0.5, 0.5, 0.5, 0.5},
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
      {0.125, -0.25, 0.375, 1.125, 0.75, 1.375},
      {0.5, 0.0, 0.5, 1.5, 1.0, 1.5},
      {0.0, 0.5, 0.0, 1.0, 1.5, 1.0},
  }};

  for (std::size_t index = 0; index < cases.size(); ++index) {
    const auto &test_case = cases[index];
    auto a = rtm::typed_box<double, std::uint32_t>(0, 0, 0, 1, 1, 1);
    auto b = rtm::typed_box<double, std::uint32_t>(
        test_case.x0, test_case.y0, test_case.z0, test_case.x1, test_case.y1,
        test_case.z1);
    auto serial = rtm::build_typed<double, std::uint32_t>(
        a, b, bounded_execution_mode::serial_v1, 1);
    auto parallel = rtm::build_typed<double, std::uint32_t>(
        a, b, bounded_execution_mode::deterministic_parallel_v1, 2);
    const auto serial_result = rtm::build_typed_relation(serial);
    const auto parallel_result = rtm::build_typed_relation(parallel);
    if (serial_result.success != parallel_result.success) {
      const auto shrunk = shrink(test_case);
      throw std::runtime_error(
          "fuzz success parity mismatch for case " + std::to_string(index) +
          " input " + describe(test_case) + " shrunk " + describe(shrunk));
    }
    if (serial_result.success) {
      require(serial_result.artifact->canonical_bytes() ==
                  parallel_result.artifact->canonical_bytes(),
              "fuzz campaign publishes byte-identical artifacts across schedules");
      continue;
    }
    require(serial_result.error.category == parallel_result.error.category &&
                serial_result.error.subcode == parallel_result.error.subcode &&
                serial_result.error.checkpoint == parallel_result.error.checkpoint,
            "fuzz campaign selects the same primary failure across schedules");
  }

  const fuzz_case synthetic{8.0, -4.0, 2.0, 9.0, -3.0, 3.0};
  const auto first = shrink(synthetic);
  const auto second = shrink(synthetic);
  require(describe(first) == describe(second) &&
              std::abs(first.x0) < std::abs(synthetic.x0),
          "fuzz shrinker is deterministic and reduces the witness");
}

} // namespace

int main() {
  try {
    test_type_index_matrix();
    test_alternate_schedules_and_merges();
    test_fan_conservation();
    test_selection_boundary();
    test_fuzz_and_shrink();
    std::cout << "Component 07 type/index, scheduling, fan, boundary, and fuzz qualification passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
