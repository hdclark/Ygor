#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationSuites.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

std::uint64_t binary64_bits(double value) {
  std::uint64_t bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "binary64 size mismatch");
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

digest tagged_digest(char kind, std::uint64_t ordinal) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(kind));
  encoder.u64(ordinal);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'S', 'T', '0', '1'}},
                       encoder.bytes());
}

std::vector<qualification_preparation_suite_observation>
passing_preparation_observations(
    const qualification_profile_suite_plan &plan) {
  std::vector<qualification_preparation_suite_observation> result;
  std::uint64_t ordinal = 1;
  for (const auto &descriptor : plan.preparation_cases) {
    qualification_preparation_suite_observation observation;
    observation.source_digest = tagged_digest('S', ordinal);
    observation.independent_verification_digest = tagged_digest('V', ordinal);
    observation.strict_validation_ran = true;
    if (descriptor.kind ==
        qualification_preparation_case_kind::strict_validation) {
      observation.strict_validation_passed = true;
    } else if (descriptor.kind == qualification_preparation_case_kind::diagnosis) {
      observation.report_digest = tagged_digest('D', ordinal);
      observation.independent_report_verified = true;
    } else {
      observation.output_digest = tagged_digest('O', ordinal);
      observation.report_digest = tagged_digest('R', ordinal);
      observation.independent_report_verified = true;
      observation.prepared_operand_available = true;
      observation.prepared_operand_strictly_revalidated = true;
      observation.edit_count = 1;
      if (descriptor.coordinates_may_change) {
        observation.coordinates_changed = true;
        observation.displacement =
            normalization_displacement_claim::records_present;
        observation.displacement_record_count = 1;
        observation.tolerance_evidence.observed_maximum_binary64_bits =
            binary64_bits(0.005);
        observation.tolerance_evidence.independent_verification_digest =
            tagged_digest('B', ordinal);
        observation.tolerance_evidence.independently_verified = true;
      }
    }
    auto made = make_qualification_preparation_suite_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing preparation observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

std::vector<qualification_result_mode_suite_observation>
passing_result_observations(const qualification_profile_suite_plan &plan) {
  std::vector<qualification_result_mode_suite_observation> result;
  std::uint64_t ordinal = 1;
  for (const auto &descriptor : plan.result_mode_cases) {
    qualification_result_mode_suite_observation observation;
    observation.exact_result_digest = tagged_digest('E', ordinal);
    observation.independent_verification_digest = tagged_digest('V', ordinal);
    observation.exact_result_verified = true;
    observation.exact_result_retained = true;
    observation.representation_semantics_verified = true;
    observation.mesh_published = descriptor.mesh_required;
    if (descriptor.mesh_required) {
      observation.output_digest = tagged_digest('M', ordinal);
      observation.strict_reingestion_passed = true;
      observation.independent_topology_passed = true;
      observation.embedding_passed = true;
      observation.orientation_passed = true;
      observation.shell_nesting_passed = true;
      observation.certificate_replay_passed = true;
    }
    if (descriptor.approximation_bounds_required) {
      observation.approximation_bounds_passed = true;
      observation.displacement_evidence.observed_maximum_binary64_bits =
          binary64_bits(0.0075);
      observation.displacement_evidence.independent_verification_digest =
          tagged_digest('D', ordinal);
      observation.displacement_evidence.independently_verified = true;
      observation.support_plane_evidence.observed_maximum_binary64_bits =
          binary64_bits(0.004);
      observation.support_plane_evidence.independent_verification_digest =
          tagged_digest('P', ordinal);
      observation.support_plane_evidence.independently_verified = true;
    }
    auto made = make_qualification_result_mode_suite_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing result-mode observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

std::vector<qualification_attribute_suite_observation>
passing_attribute_observations(const qualification_profile_suite_plan &plan,
                               digest geometry_digest = tagged_digest('G', 1)) {
  std::vector<qualification_attribute_suite_observation> result;
  std::uint64_t ordinal = 1;
  const auto exact_result_digest = tagged_digest('E', 100);
  for (const auto &descriptor : plan.attribute_cases) {
    qualification_attribute_suite_observation observation;
    observation.exact_result_digest = exact_result_digest;
    observation.geometry_digest = geometry_digest;
    observation.report_digest = tagged_digest('A', ordinal);
    observation.independent_report_verified = true;
    observation.seam_evidence_verified = true;
    observation.conflict_evidence_verified = true;
    observation.omission_evidence_verified = true;
    observation.multi_source_mapping_verified = true;
    observation.downstream_source_query_verified = true;
    observation.downstream_source_query_digest = tagged_digest('Q', ordinal);
    observation.attribute_values_invariant_to_geometry = true;
    auto made = make_qualification_attribute_suite_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing attribute observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

void catalog_and_binding_contracts() {
  const auto plan = make_default_qualification_profile_suite_plan();
  require(validate_qualification_profile_suite_plan(plan).has_value(),
          "default P6.7 plan validates");
  require_equal(plan.preparation_cases.size(), std::size_t(11),
                "strict, diagnosis, and every repair class are independent");
  require_equal(plan.result_mode_cases.size(), std::size_t(3),
                "all three result representations are independent");
  require_equal(plan.attribute_cases.size(), std::size_t(23),
                "all advertised attribute and provenance policies are covered");

  std::set<normalization_operation> repairs;
  std::uint64_t strict = 0, diagnosis = 0, bounded_repairs = 0;
  for (const auto &entry : plan.preparation_cases) {
    require(entry.policy_digest != digest{} && entry.case_digest != digest{},
            "preparation case binds policy and identity");
    if (entry.kind == qualification_preparation_case_kind::strict_validation)
      ++strict;
    else if (entry.kind == qualification_preparation_case_kind::diagnosis)
      ++diagnosis;
    else {
      repairs.insert(*entry.operation);
      if (entry.coordinates_may_change) {
        ++bounded_repairs;
        require(entry.tolerance_unit == model_unit::millimetre &&
                    entry.model_tolerance_binary64_bits == binary64_bits(0.01),
                "geometry-changing repair declares an exact binary64 bound");
      }
    }
  }
  require(strict == 1 && diagnosis == 1 && repairs.size() == 9 &&
              bounded_repairs == 4,
          "preparation catalog separates strict, diagnosis, structural, and bounded repairs");

  auto bytes = encode_qualification_profile_suite_plan(plan);
  require(bytes.has_value() && bytes.value() == plan.canonical_bytes &&
              plan.plan_digest != digest{},
          "plan has stable canonical bytes and digest");
  auto replay = make_default_qualification_profile_suite_plan();
  require(replay.canonical_bytes == plan.canonical_bytes &&
              replay.plan_digest == plan.plan_digest,
          "default suite plan replays byte-identically");
}

void complete_report_contracts() {
  const auto plan = make_default_qualification_profile_suite_plan();
  auto preparation = passing_preparation_observations(plan);
  auto results = passing_result_observations(plan);
  auto attributes = passing_attribute_observations(plan);
  auto report = make_qualification_profile_suite_report(
      plan, preparation, results, attributes);
  require(report.has_value() && report.value().complete &&
              report.value().blocking_issue_count == 0 &&
              report.value().passed_preparation_cases ==
                  plan.preparation_cases.size() &&
              report.value().passed_result_mode_cases ==
                  plan.result_mode_cases.size() &&
              report.value().passed_attribute_cases ==
                  plan.attribute_cases.size() &&
              qualification_profile_suite_gate_passes(report.value(), plan),
          "complete independently checked suite report passes the P6.7 gate");
  require(validate_qualification_profile_suite_report(report.value(), plan)
              .has_value(),
          "complete report independently reconstructs");
  auto bytes = encode_qualification_profile_suite_report(report.value());
  require(bytes.has_value() && bytes.value() == report.value().canonical_bytes,
          "report exposes only canonical bound bytes");

  std::reverse(preparation.begin(), preparation.end());
  std::reverse(results.begin(), results.end());
  std::reverse(attributes.begin(), attributes.end());
  auto reordered = make_qualification_profile_suite_report(
      plan, std::move(preparation), std::move(results), std::move(attributes));
  require(reordered.has_value() &&
              reordered.value().canonical_bytes == report.value().canonical_bytes &&
              reordered.value().report_digest == report.value().report_digest,
          "observation arrival order cannot change the qualification report");
}

void preparation_fail_closed_contracts() {
  const auto plan = make_default_qualification_profile_suite_plan();
  const auto bounded = std::find_if(
      plan.preparation_cases.begin(), plan.preparation_cases.end(),
      [](const auto &entry) { return entry.coordinates_may_change; });
  require(bounded != plan.preparation_cases.end(),
          "bounded normalization case exists");

  qualification_preparation_suite_observation hidden;
  hidden.source_digest = tagged_digest('S', 700);
  hidden.independent_verification_digest = tagged_digest('V', 700);
  hidden.strict_validation_ran = true;
  hidden.output_digest = tagged_digest('O', 700);
  hidden.report_digest = tagged_digest('R', 700);
  hidden.independent_report_verified = true;
  hidden.prepared_operand_available = true;
  hidden.prepared_operand_strictly_revalidated = true;
  hidden.coordinates_changed = true;
  hidden.edit_count = 1;
  hidden.displacement = normalization_displacement_claim::records_present;
  hidden.displacement_record_count = 1;
  hidden.tolerance_evidence.observed_maximum_binary64_bits = binary64_bits(0.005);
  hidden.tolerance_evidence.independent_verification_digest =
      tagged_digest('B', 700);
  hidden.tolerance_evidence.independently_verified = true;
  hidden.tolerance_evidence.hidden_epsilon_used = true;
  require(!make_qualification_preparation_suite_observation(*bounded, hidden)
               .has_value(),
          "hidden normalization epsilon fails closed");

  hidden.tolerance_evidence.hidden_epsilon_used = false;
  hidden.tolerance_evidence.observed_maximum_binary64_bits = binary64_bits(0.02);
  require(!make_qualification_preparation_suite_observation(*bounded, hidden)
               .has_value(),
          "normalization displacement above the declared bound fails closed");

  auto passing = passing_preparation_observations(plan);
  auto stale_observation = passing.front();
  stale_observation.observation_digest.bytes[0] ^= 1U;
  require(!make_qualification_preparation_suite_observation(
               plan.preparation_cases.front(), stale_observation)
               .has_value(),
          "stale preparation observation digest fails closed");

  auto missing_independent = passing.front();
  missing_independent.independent_verification_digest = {};
  missing_independent.observation_digest = {};
  require(!make_qualification_preparation_suite_observation(
               plan.preparation_cases.front(), missing_independent)
               .has_value(),
          "strict validation requires replayable independent evidence");

  auto incomplete = plan;
  incomplete.preparation_cases.erase(
      std::find_if(incomplete.preparation_cases.begin(),
                   incomplete.preparation_cases.end(), [](const auto &entry) {
                     return entry.operation ==
                            normalization_operation::self_intersection_repair;
                   }));
  incomplete.canonical_bytes.clear();
  incomplete.plan_digest = {};
  require(!make_qualification_profile_suite_plan(std::move(incomplete))
               .has_value(),
          "omitting one repair class invalidates the suite plan");

  auto duplicate = plan;
  const auto first_repair = std::find_if(
      duplicate.preparation_cases.begin(), duplicate.preparation_cases.end(),
      [](const auto &entry) {
        return entry.kind == qualification_preparation_case_kind::repair;
      });
  require(first_repair != duplicate.preparation_cases.end(),
          "repair case exists for duplicate-coverage test");
  auto extra_repair = *first_repair;
  extra_repair.identifier += ".duplicate";
  extra_repair.case_digest = {};
  duplicate.preparation_cases.push_back(std::move(extra_repair));
  duplicate.canonical_bytes.clear();
  duplicate.plan_digest = {};
  require(!make_qualification_profile_suite_plan(std::move(duplicate))
               .has_value(),
          "duplicate repair coverage cannot inflate the frozen catalog");
}

void result_mode_fail_closed_contracts() {
  const auto plan = make_default_qualification_profile_suite_plan();
  const auto approximate = std::find_if(
      plan.result_mode_cases.begin(), plan.result_mode_cases.end(),
      [](const auto &entry) {
        return entry.representation ==
               result_representation::certified_approximate_mesh;
      });
  require(approximate != plan.result_mode_cases.end(),
          "approximate suite case exists");

  qualification_result_mode_suite_observation observation;
  observation.exact_result_digest = tagged_digest('E', 800);
  observation.output_digest = tagged_digest('M', 800);
  observation.independent_verification_digest = tagged_digest('V', 800);
  observation.exact_result_verified = true;
  observation.exact_result_retained = true;
  observation.mesh_published = true;
  observation.representation_semantics_verified = true;
  observation.strict_reingestion_passed = true;
  observation.independent_topology_passed = true;
  observation.embedding_passed = true;
  observation.orientation_passed = true;
  observation.shell_nesting_passed = true;
  observation.certificate_replay_passed = true;
  observation.approximation_bounds_passed = true;
  observation.displacement_evidence.observed_maximum_binary64_bits =
      binary64_bits(0.02);
  observation.displacement_evidence.independent_verification_digest =
      tagged_digest('D', 800);
  observation.displacement_evidence.independently_verified = true;
  observation.support_plane_evidence.observed_maximum_binary64_bits =
      binary64_bits(0.004);
  observation.support_plane_evidence.independent_verification_digest =
      tagged_digest('P', 800);
  observation.support_plane_evidence.independently_verified = true;
  require(!make_qualification_result_mode_suite_observation(*approximate,
                                                            observation)
               .has_value(),
          "approximate displacement above policy fails closed");

  observation.displacement_evidence.observed_maximum_binary64_bits =
      binary64_bits(0.0075);
  observation.certificate_replay_passed = false;
  require(!make_qualification_result_mode_suite_observation(*approximate,
                                                            observation)
               .has_value(),
          "missing certificate replay fails the approximate suite");

  observation.certificate_replay_passed = true;
  observation.displacement_evidence.independent_verification_digest = {};
  require(!make_qualification_result_mode_suite_observation(*approximate,
                                                            observation)
               .has_value(),
          "approximate bounds require a replayable independent checker digest");

  auto passing = passing_result_observations(plan);
  auto stale_observation = passing.front();
  stale_observation.observation_digest.bytes[0] ^= 1U;
  require(!make_qualification_result_mode_suite_observation(
               plan.result_mode_cases.front(), stale_observation)
               .has_value(),
          "stale result observation digest fails closed");

  auto stale = *approximate;
  stale.semantics = product_realization_semantics::exact_in_T;
  stale.policy_digest = {};
  stale.case_digest = {};
  require(!make_qualification_profile_suite_plan(
               qualification_profile_suite_plan{
                   qualification_suite_schema_version,
                   qualification_suite_checker_version,
                   "invalid-result-semantics-v1", plan.preparation_cases,
                   {stale}, plan.attribute_cases, {}, {}})
               .has_value(),
          "representation and realization semantics cannot be relabeled");
}

void attribute_and_geometry_invariance_contracts() {
  const auto plan = make_default_qualification_profile_suite_plan();
  const auto demanding = std::find_if(
      plan.attribute_cases.begin(), plan.attribute_cases.end(),
      [](const auto &entry) {
        return entry.require_seam_evidence &&
               entry.require_conflict_evidence &&
               entry.require_omission_evidence &&
               entry.require_multi_source_mapping &&
               entry.require_downstream_source_query;
      });
  require(demanding != plan.attribute_cases.end(),
          "combined seam/conflict/omission/query suite case exists");

  qualification_attribute_suite_observation missing;
  missing.exact_result_digest = tagged_digest('E', 900);
  missing.geometry_digest = tagged_digest('G', 900);
  missing.report_digest = tagged_digest('A', 900);
  missing.independent_report_verified = true;
  missing.attribute_values_invariant_to_geometry = true;
  require(!make_qualification_attribute_suite_observation(*demanding, missing)
               .has_value(),
          "missing advertised attribute evidence fails closed");

  auto passing_attributes = passing_attribute_observations(plan);
  auto stale_observation = passing_attributes.front();
  stale_observation.observation_digest.bytes[0] ^= 1U;
  require(!make_qualification_attribute_suite_observation(
               plan.attribute_cases.front(), stale_observation)
               .has_value(),
          "stale attribute observation digest fails closed");

  auto duplicate_plan = plan;
  auto extra_attribute = duplicate_plan.attribute_cases.front();
  extra_attribute.identifier += ".duplicate";
  extra_attribute.case_digest = {};
  duplicate_plan.attribute_cases.push_back(std::move(extra_attribute));
  duplicate_plan.canonical_bytes.clear();
  duplicate_plan.plan_digest = {};
  require(!make_qualification_profile_suite_plan(std::move(duplicate_plan))
               .has_value(),
          "duplicate attribute policy coverage cannot inflate the catalog");

  auto preparation = passing_preparation_observations(plan);
  auto results = passing_result_observations(plan);
  auto attributes = passing_attribute_observations(plan);
  require(attributes.size() > 1, "multiple attribute policies exist");
  attributes.back().geometry_digest = tagged_digest('G', 901);
  attributes.back().observation_digest = {};
  auto remade = make_qualification_attribute_suite_observation(
      plan.attribute_cases.back(), attributes.back());
  require(remade.has_value(), "individually valid alternate geometry binds");
  attributes.back() = std::move(remade.value());
  auto blocked = make_qualification_profile_suite_report(
      plan, std::move(preparation), std::move(results), std::move(attributes));
  require(blocked.has_value() && !blocked.value().complete &&
              blocked.value().blocking_issue_count == 1 &&
              !qualification_profile_suite_gate_passes(blocked.value(), plan) &&
              validate_qualification_profile_suite_report(blocked.value(), plan)
                  .has_value(),
          "attribute-value-dependent geometry is recorded as a blocking suite outcome");

  preparation = passing_preparation_observations(plan);
  results = passing_result_observations(plan);
  attributes = passing_attribute_observations(plan);
  attributes.back().exact_result_digest = tagged_digest('E', 901);
  attributes.back().observation_digest = {};
  remade = make_qualification_attribute_suite_observation(
      plan.attribute_cases.back(), attributes.back());
  require(remade.has_value(), "individually valid alternate exact result binds");
  attributes.back() = std::move(remade.value());
  blocked = make_qualification_profile_suite_report(
      plan, std::move(preparation), std::move(results), std::move(attributes));
  require(blocked.has_value() && !blocked.value().complete &&
              blocked.value().blocking_issue_count == 1 &&
              !qualification_profile_suite_gate_passes(blocked.value(), plan),
          "geometry invariance cannot compare different exact Boolean results");

  auto report = make_qualification_profile_suite_report(
      plan, passing_preparation_observations(plan),
      passing_result_observations(plan), passing_attribute_observations(plan));
  require(report.has_value(), "stale report source exists");
  auto stale_report = report.value();
  stale_report.report_digest.bytes[0] ^= 1U;
  require(!validate_qualification_profile_suite_report(stale_report, plan)
               .has_value(),
          "report digest mutation fails independent reconstruction");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.7.catalog", catalog_and_binding_contracts);
  tests.add("P6.7.report", complete_report_contracts);
  tests.add("P6.7.preparation", preparation_fail_closed_contracts);
  tests.add("P6.7.result_modes", result_mode_fail_closed_contracts);
  tests.add("P6.7.attributes", attribute_and_geometry_invariance_contracts);
  return tests.run(std::cout, std::cerr);
}
