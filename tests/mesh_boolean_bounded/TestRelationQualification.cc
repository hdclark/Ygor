#include "BroadPhaseFixtures.h"
#include "qualification/RelationExactOracle.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/RelationQueries.h"
#include "YgorMeshesBooleanBounded/RelationReplay.h"

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
using broad_phase_tests::built_fixture;
using broad_phase_tests::diagnostic;
using broad_phase_tests::require;

namespace {

bounded::relation_capabilities capabilities(
    built_fixture &fixture, bounded::resource_manager &resources) {
  bounded::relation_capabilities result;
  result.owner = fixture.predecessor.context.owner;
  result.resources = &resources;
  return result;
}

using artifact_ptr = std::shared_ptr<
    const bounded::signed_feature_relations<double, std::uint32_t>>;

struct relation_attempt final {
  artifact_ptr artifact;
  bounded_boolean_error error{};
  bool success = false;
};

relation_attempt build_relation(built_fixture &fixture) {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  auto outcome = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities(fixture, resources));
  relation_attempt result;
  result.success = outcome.has_value();
  if (result.success)
    result.artifact = *outcome.value();
  else
    result.error = *outcome.error();
  return result;
}

void verify_exact_edge_oracle(
    const bounded::signed_feature_relations<double, std::uint32_t> &artifact) {
  require(artifact.source_edge_stage() != nullptr,
          "exact relation oracle requires the source-edge stage");
  std::size_t checked = 0;
  for (const auto &record : artifact.source_edge_stage()->relations) {
    std::string failure;
    require(qualification::exact_source_edge_record_agrees(record, failure),
            failure.c_str());
    ++checked;
  }
  require(checked == artifact.source_edge_stage()->evaluation_count,
          "exact relation oracle covers every compute-once edge relation");
}

struct component08_event final {
  bounded::relation_event_seed_id seed{0};
  bounded::feature_relation_id relation{0};
  bounded::relation_construction_id construction{0};
  bounded::relation_event_seed_key equivalence_key{};
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  bounded::operand_id half_open_owner = bounded::operand_id::a;
  std::uint32_t occurrence = 0;
  std::uint64_t source_incidence_count = 0;
  std::uint64_t candidate_incidence_count = 0;
  bool distinct_occurrence_required = false;
};

std::vector<component08_event> component08_registry_test_double(
    const bounded::signed_feature_relations_view<double, std::uint32_t> &view) {
  require(view.valid_owner() && view.statistics() != nullptr,
          "Component 08 test double requires a valid relation view");
  std::vector<component08_event> events;
  events.reserve(static_cast<std::size_t>(view.statistics()->event_seed_count));
  for (std::uint64_t ordinal = 0;
       ordinal < view.statistics()->event_seed_count; ++ordinal) {
    const auto seed_id = bounded::relation_event_seed_id(ordinal);
    const auto *seed = view.event_seed(seed_id);
    require(seed != nullptr && seed->id == seed_id &&
                view.relation(seed->source_relation) != nullptr &&
                view.construction(seed->construction) != nullptr,
            "Component 08 consumes only published relation and construction IDs");
    const auto incidence = view.event_incidence(seed_id);
    const auto candidate_incidence = view.event_candidate_incidence(seed_id);
    require(incidence && candidate_incidence &&
                incidence.count == seed->incidence_count &&
                candidate_incidence.count == seed->candidate_incidence_count,
            "Component 08 consumes complete source and candidate incidence without geometry");
    for (std::uint64_t i = 0; i < candidate_incidence.count; ++i) {
      const auto &record = candidate_incidence[i];
      require(record.seed == seed_id &&
                  view.candidate_disposition(record.disposition) != nullptr,
              "Component 08 candidate incidence resolves through canonical IDs");
    }
    events.push_back(component08_event{
        seed->id, seed->source_relation, seed->construction, seed->key,
        seed->numeric_crossing, seed->symbolic_crossing, seed->half_open_owner,
        seed->key.occurrence, incidence.count, candidate_incidence.count,
        seed->distinct_occurrence_required});
  }
  return events;
}

struct component09_atom final {
  bounded::relation_event_seed_id seed{0};
  bounded::side_occupancy sides{false, false, false, false};
  std::uint32_t occurrence = 0;
  bool distinct_occurrence_required = false;
};

std::vector<component09_atom> component09_classification_test_double(
    const std::vector<component08_event> &events) {
  std::vector<component09_atom> atoms;
  atoms.reserve(events.size());
  for (const auto &event : events) {
    const auto signed_crossing = event.numeric_crossing +
                                 static_cast<std::int32_t>(event.symbolic_crossing);
    component09_atom atom;
    atom.seed = event.seed;
    atom.occurrence = event.occurrence;
    atom.distinct_occurrence_required = event.distinct_occurrence_required;
    if (event.half_open_owner == bounded::operand_id::a) {
      atom.sides.a_negative = signed_crossing > 0;
      atom.sides.a_positive = signed_crossing < 0;
    } else {
      atom.sides.b_negative = signed_crossing > 0;
      atom.sides.b_positive = signed_crossing < 0;
    }
    atoms.push_back(atom);
  }
  return atoms;
}

struct component10_selection final {
  bounded::relation_event_seed_id seed{0};
  bounded::truth_cell result{};
  std::uint32_t occurrence = 0;
};

std::vector<component10_selection> component10_selection_test_double(
    const std::vector<component09_atom> &atoms, boolean_operation operation,
    std::uint64_t &truth_service_calls) {
  std::vector<component10_selection> selections;
  selections.reserve(atoms.size());
  for (const auto &atom : atoms) {
    ++truth_service_calls;
    selections.push_back(component10_selection{
        atom.seed, bounded::evaluate_truth(operation, atom.sides),
        atom.occurrence});
  }
  return selections;
}

void test_downstream_handoff_boundary() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0));
  const auto attempt = build_relation(fixture);
  if (!attempt.success)
    throw std::runtime_error(diagnostic(attempt.error));

  bounded::signed_feature_relations_view<double, std::uint32_t> view(
      *attempt.artifact, fixture.predecessor.context.owner);
  require(view.valid_owner() &&
              view.schema_version() ==
                  bounded::contract_versions::relation_artifact_schema &&
              view.artifact_digest() &&
              *view.artifact_digest() == attempt.artifact->digest(),
          "downstream view exposes the verified owner-bound artifact handshake");
  bounded::signed_feature_relations_view<double, std::uint32_t> wrong_view(
      *attempt.artifact, bounded::context_owner_token::create());
  require(!wrong_view.valid_owner() &&
              wrong_view.event_seed(bounded::relation_event_seed_id(0)) ==
                  nullptr &&
              wrong_view.artifact_digest() == nullptr,
          "downstream view rejects wrong-owner access before dereference");

  std::uint64_t truth_service_calls = 0;
  const auto events = component08_registry_test_double(view);
  require(truth_service_calls == 0,
          "Component 08 performs no final truth-table selection");
  const auto atoms = component09_classification_test_double(events);
  require(truth_service_calls == 0 && atoms.size() == events.size(),
          "Component 09 derives side labels without final selection");

  for (std::uint8_t operation_value = 1; operation_value <= 5;
       ++operation_value) {
    const auto before = truth_service_calls;
    const auto selected = component10_selection_test_double(
        atoms, static_cast<boolean_operation>(operation_value),
        truth_service_calls);
    require(selected.size() == atoms.size() &&
                truth_service_calls - before == atoms.size(),
            "Component 10 alone invokes the final truth-table service");
    for (const auto &selection : selected)
      require(selection.result.orientation ==
                      bounded::boundary_orientation::not_applicable ||
                  selection.result.retain,
              "Component 10 derives orientation only for retained boundaries");
  }

  for (std::uint64_t request = 0;
       request < attempt.artifact->request_graph().requests.size(); ++request) {
    const auto request_id = bounded::relation_request_id(request);
    require(view.request(request_id) != nullptr &&
                view.dependencies(request_id) &&
                view.reverse_consumers(request_id) &&
                view.candidate_witnesses(request_id),
            "downstream view exposes complete checked graph ranges");
  }
  for (std::uint64_t relation = 0;
       relation < attempt.artifact->relations().size(); ++relation)
    require(static_cast<bool>(view.relation_truths(bounded::feature_relation_id(relation))),
            "downstream view exposes complete relation truth ranges");
  for (std::uint64_t construction = 0;
       construction < attempt.artifact->constructions().size(); ++construction) {
    const auto id = bounded::relation_construction_id(construction);
    require(view.construction_residual_truths(id) &&
                view.construction_intervals(id) &&
                view.construction_regions(id) &&
                view.construction_witnesses(id),
            "downstream view exposes construction precision lineage without arithmetic");
  }
}

std::uint64_t ceil_log2(std::uint64_t value) {
  std::uint64_t result = 0;
  std::uint64_t power = 1;
  while (power < value) {
    power <<= 1U;
    ++result;
  }
  return result;
}

void verify_structural_gates(
    const bounded::signed_feature_relations<double, std::uint32_t> &artifact) {
  const auto &statistics = artifact.statistics();
  require(statistics.unique_request_count ==
                  artifact.request_graph().requests.size() &&
              statistics.unique_request_count <=
                  statistics.request_proposal_count &&
              artifact.source_edge_stage()->evaluation_count ==
                  artifact.source_edge_stage()->relations.size() &&
              artifact.source_edge_facet_stage()->evaluation_count ==
                  artifact.source_edge_facet_stage()->relations.size() &&
              artifact.source_facet_stage()->evaluation_count ==
                  artifact.source_facet_stage()->relations.size() &&
              artifact.coplanar_overlay_stage()->evaluation_count ==
                  artifact.coplanar_overlay_stage()->overlays.size(),
          "structural gate proves one evaluation per canonical producer");

  const auto requests = statistics.unique_request_count;
  const auto comparison_ceiling =
      64U + 64U * requests * ceil_log2(requests + 1U);
  require(statistics.sort_comparisons <= comparison_ceiling,
          "request grouping remains within the reviewed O(R log R) comparison ceiling");

  std::uint64_t published_records =
      statistics.unique_request_count + statistics.dependency_count +
      statistics.reverse_consumer_count + statistics.candidate_witness_count +
      statistics.imported_geometry_count + statistics.bounded_primitive_count +
      statistics.exact_relation_count + statistics.truth_lineage_count +
      statistics.interval_evidence_count +
      statistics.source_facet_region_count + statistics.public_relation_count +
      statistics.bookkeeping_relation_count + statistics.construction_count +
      statistics.construction_ledger_count +
      statistics.symbolic_eligibility_count +
      statistics.symbolic_decision_count + statistics.crossing_record_count +
      statistics.event_seed_count +
      statistics.event_seed_candidate_incidence_count +
      statistics.candidate_relation_coverage_count +
      statistics.candidate_seed_coverage_count +
      statistics.candidate_partition_count + statistics.diagnostic_count +
      statistics.replay_checkpoint_count;
  require(statistics.persistent_bytes <=
              65536U + published_records * 8192U,
          "persistent memory remains proportional to published relation evidence");
}

struct box_case final {
  double x0 = 0;
  double y0 = 0;
  double z0 = 0;
  double x1 = 1;
  double y1 = 1;
  double z1 = 1;
};

std::string describe(const box_case &value) {
  std::ostringstream stream;
  stream << '[' << value.x0 << ',' << value.y0 << ',' << value.z0 << " -> "
         << value.x1 << ',' << value.y1 << ',' << value.z1 << ']';
  return stream.str();
}

box_case shrink_failure(box_case value) {
  const auto shrink = [](double coordinate) {
    if (coordinate == 0.0)
      return coordinate;
    return coordinate / 2.0;
  };
  for (std::size_t pass = 0; pass < 8; ++pass) {
    value.x0 = shrink(value.x0);
    value.y0 = shrink(value.y0);
    value.z0 = shrink(value.z0);
    value.x1 = 1.0 + shrink(value.x1 - 1.0);
    value.y1 = 1.0 + shrink(value.y1 - 1.0);
    value.z1 = 1.0 + shrink(value.z1 - 1.0);
  }
  return value;
}

void test_deterministic_campaign_and_structural_gates() {
  const std::array<box_case, 8> cases{{
      {4.0, 4.0, 4.0, 5.0, 5.0, 5.0},
      {0.5, 0.25, 0.125, 1.5, 1.25, 1.125},
      {0.25, 0.25, 0.25, 0.75, 0.75, 0.75},
      {0.25, 0.25, 1.0, 0.75, 0.75, 2.0},
      {1.0, 1.0, 0.25, 2.0, 2.0, 0.75},
      {-0.5, -0.5, -0.5, 0.5, 0.5, 0.5},
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
      {0.125, -0.25, 0.375, 1.125, 0.75, 1.375},
  }};

  std::size_t exact_successes = 0;
  for (std::size_t index = 0; index < cases.size(); ++index) {
    const auto &test_case = cases[index];
    auto serial_fixture = broad_phase_tests::build(
        broad_phase_tests::box(),
        broad_phase_tests::box(test_case.x0, test_case.y0, test_case.z0,
                               test_case.x1, test_case.y1, test_case.z1),
        bounded::source_triangulation_provider_kind::indexed_dependency_v1,
        true, bounded_execution_mode::serial_v1, 1);
    auto parallel_fixture = broad_phase_tests::build(
        broad_phase_tests::box(),
        broad_phase_tests::box(test_case.x0, test_case.y0, test_case.z0,
                               test_case.x1, test_case.y1, test_case.z1),
        bounded::source_triangulation_provider_kind::indexed_dependency_v1,
        true, bounded_execution_mode::deterministic_parallel_v1, 7);
    const auto serial = build_relation(serial_fixture);
    const auto parallel = build_relation(parallel_fixture);
    if (serial.success != parallel.success) {
      const auto shrunk = shrink_failure(test_case);
      throw std::runtime_error(
          "deterministic campaign success mismatch for case " +
          std::to_string(index) + " input " + describe(test_case) +
          " shrunk " + describe(shrunk));
    }
    if (!serial.success) {
      require(serial.error.category == parallel.error.category &&
                  serial.error.subcode == parallel.error.subcode &&
                  serial.error.checkpoint == parallel.error.checkpoint,
              "serial and deterministic-parallel profiles select the same primary failure");
      continue;
    }
    if (serial.artifact->canonical_bytes() != parallel.artifact->canonical_bytes()) {
      std::size_t mismatch = 0;
      const auto common = std::min(serial.artifact->canonical_bytes().size(),
                                   parallel.artifact->canonical_bytes().size());
      while (mismatch < common &&
             serial.artifact->canonical_bytes()[mismatch] ==
                 parallel.artifact->canonical_bytes()[mismatch])
        ++mismatch;
      throw std::runtime_error(
          "serial/parallel canonical byte mismatch for case " +
          std::to_string(index) + " at byte " + std::to_string(mismatch) +
          " sizes " +
          std::to_string(serial.artifact->canonical_bytes().size()) + "/" +
          std::to_string(parallel.artifact->canonical_bytes().size()) +
          " context=" + std::to_string(serial.artifact->context_digest() == parallel.artifact->context_digest()) +
          " precision=" + std::to_string(serial.artifact->precision_digest() == parallel.artifact->precision_digest()) +
          " candidate=" + std::to_string(serial.artifact->candidate_digest() == parallel.artifact->candidate_digest()) +
          " graph=" + std::to_string(serial.artifact->graph_digest() == parallel.artifact->graph_digest()));
    }
    require(serial.artifact->digest() == parallel.artifact->digest(),
            "serial and deterministic-parallel profiles publish identical artifact digests");
    require(bounded::encode_relation_diagnostic_semantics(
                serial.artifact->diagnostics()) ==
                bounded::encode_relation_diagnostic_semantics(
                    parallel.artifact->diagnostics()),
            "serial and deterministic-parallel profiles publish identical diagnostics");
    require(serial.artifact->replay_evidence().input_equivalence_digest ==
                parallel.artifact->replay_evidence().input_equivalence_digest,
            "serial and deterministic-parallel profiles publish identical input equivalence");
    verify_exact_edge_oracle(*serial.artifact);
    verify_structural_gates(*serial.artifact);
    ++exact_successes;
  }
  require(exact_successes >= 4,
          "deterministic qualification campaign exercises successful exact-oracle cases");

  const box_case synthetic{8.0, -4.0, 2.0, 9.0, -3.0, 3.0};
  const auto first = shrink_failure(synthetic);
  const auto second = shrink_failure(synthetic);
  require(describe(first) == describe(second) &&
              std::abs(first.x0) < std::abs(synthetic.x0),
          "qualification shrinker is deterministic and reduces the witness");
}

} // namespace

int main() {
  try {
    test_downstream_handoff_boundary();
    test_deterministic_campaign_and_structural_gates();
    std::cout << "Component 07 exact-oracle, handoff, and campaign qualification passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
