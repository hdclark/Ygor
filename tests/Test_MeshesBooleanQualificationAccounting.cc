#include "MeshBooleanInputTopologyFixtures.h"
#include "MeshBooleanOracles.h"
#include "MeshBooleanOutputFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationAccounting.h>
#include <YgorMeshesBooleanService.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace ygor;
using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

using coordinate_type = double;
using index_type = std::uint32_t;

boolean_service_options exact_mesh_options() {
  boolean_service_options options;
  options.product.backend.mode = backend_selection_mode::explicit_backend;
  options.product.backend.requested_backend = backend_id::experimental_exact_v1;
  options.product.backend.allow_experimental_backend = true;
  options.product.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  options.product.result.representation =
      result_representation::exact_in_T_mesh;
  options.product.realization.semantics =
      product_realization_semantics::exact_in_T;
  options.product.realization.search.strategy =
      realization_search_strategy::nearest_only;
  options.product.attributes.mode =
      attribute_transfer_mode::preserve_supported_with_report;
  return options;
}

digest tagged_digest(const char suffix, std::uint64_t ordinal = 0) {
  canonical_encoder encoder;
  encoder.u64(ordinal);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'A', 'T', '0', suffix}},
                       encoder.bytes());
}

qualification_dimension_key dimension(
    result_representation representation,
    std::string category = "non_box_intersection") {
  qualification_dimension_key value;
  value.backend = backend_id::experimental_exact_v1;
  value.representation = representation;
  value.preparation = preparation_mode::strict_validation;
  value.selected_operation = operation::regularized_union;
  value.coordinate = coordinate_tag::binary64;
  value.index = index_tag::uint32;
  value.toolchain_identifier = "p6.5-test-gcc-debug";
  value.geometry_category = std::move(category);
  return value;
}

qualification_guarded_probe_observation probe(
    std::string identifier, bool expected, bool observed,
    bool on_boundary = false) {
  qualification_guarded_probe_observation value;
  value.identifier = std::move(identifier);
  value.point_digest = tagged_digest('P', value.identifier.size());
  value.expected_occupied = expected;
  value.observed_occupied = observed;
  value.observed_on_boundary = on_boundary;
  auto made = make_qualification_guarded_probe_observation(std::move(value));
  require(made.has_value(), "guarded probe canonicalizes");
  return std::move(made.value());
}

qualification_case_observation success_case(
    std::string identifier, digest case_digest,
    qualification_dimension_key dimensions,
    qualification_success_verification verification,
    qualification_outcome expected) {
  qualification_case_observation observation;
  observation.identifier = std::move(identifier);
  observation.case_digest = case_digest;
  observation.dimensions = std::move(dimensions);
  observation.expected_outcomes = {expected};
  observation.published_success = true;
  observation.success_verification = std::move(verification);
  return observation;
}

qualification_case_observation failure_case(
    std::string identifier, product_error_code actual,
    std::vector<product_error_code> expected_codes) {
  qualification_case_observation observation;
  observation.identifier = std::move(identifier);
  observation.case_digest = tagged_digest('F', observation.identifier.size());
  observation.dimensions = dimension(result_representation::exact_stratified,
                                     "normalization_defect");
  observation.expected_outcomes = {
      qualification_outcome::expected_typed_failure};
  observation.expected_failure_codes = std::move(expected_codes);
  observation.failure = make_product_error(actual, "p6.5.synthetic_failure");
  return observation;
}

qualification_verification_check passed_check(qualification_check_kind kind,
                                               std::uint64_t ordinal) {
  qualification_verification_check value;
  value.kind = kind;
  value.state = qualification_check_state::passed;
  value.message_key = std::string("p6.5.synthetic.") +
                      qualification_check_kind_token(kind);
  value.evidence_digest = tagged_digest('C', ordinal);
  auto made = make_qualification_verification_check(std::move(value));
  require(made.has_value(), "synthetic verification check canonicalizes");
  return std::move(made.value());
}

qualification_success_verification synthetic_approximate_verification() {
  const auto mesh = input_test::box<coordinate_type, index_type>(0.0, 1.0);
  auto topology = reconstruct_qualification_mesh_topology(mesh);
  require(topology.has_value() && topology.value().passed(),
          "synthetic approximate topology reconstructs");
  qualification_success_verification verification;
  verification.mesh_published = true;
  verification.chain_reingestion_required = true;
  verification.representation =
      result_representation::certified_approximate_mesh;
  verification.exact_result_digest = tagged_digest('E', 1);
  verification.output_digest = tagged_digest('O', 1);
  verification.topology = std::move(topology.value());
  verification.probes = {probe("inside", true, true),
                         probe("outside", false, false)};
  const std::array<qualification_check_kind, 13> kinds{{
      qualification_check_kind::product_contract,
      qualification_check_kind::exact_result_binding,
      qualification_check_kind::representation_semantics,
      qualification_check_kind::strict_reingestion,
      qualification_check_kind::independent_topology,
      qualification_check_kind::certificate_replay,
      qualification_check_kind::guarded_occupancy,
      qualification_check_kind::embedding,
      qualification_check_kind::orientation,
      qualification_check_kind::shell_nesting,
      qualification_check_kind::attribute_mapping,
      qualification_check_kind::approximation_bounds,
      qualification_check_kind::chain_reingestion}};
  for (std::size_t i = 0; i != kinds.size(); ++i)
    verification.checks.push_back(passed_check(kinds[i], i));
  auto made = make_qualification_success_verification(std::move(verification));
  require(made.has_value(), "synthetic approximate verification canonicalizes");
  return std::move(made.value());
}

void independent_topology_contracts() {
  const auto cube = input_test::box<coordinate_type, index_type>(0.0, 1.0);
  auto topology = reconstruct_qualification_mesh_topology(cube);
  require(topology.has_value() && topology.value().passed() &&
              topology.value().vertices == 8 && topology.value().faces == 6 &&
              topology.value().edges == 12 &&
              topology.value().connected_components == 1 &&
              topology.value().euler_characteristic == 2,
          "independent topology reconstructs closed cube");
  auto replay = reconstruct_qualification_mesh_topology(cube);
  require(replay.has_value() &&
              replay.value().reconstruction_digest ==
                  topology.value().reconstruction_digest,
          "topology reconstruction is deterministic");

  auto open = cube;
  open.faces.pop_back();
  auto rejected = reconstruct_qualification_mesh_topology(open);
  require(rejected.has_value() && !rejected.value().passed() &&
              !rejected.value().closed_two_uses_per_edge,
          "open mesh is retained as failed evidence rather than discarded");
  auto reversed = cube;
  std::reverse(reversed.faces.front().begin(), reversed.faces.front().end());
  rejected = reconstruct_qualification_mesh_topology(reversed);
  require(rejected.has_value() && !rejected.value().passed() &&
              !rejected.value().opposite_edge_directions,
          "orientation inconsistency is independently reconstructed");
}

void real_success_reingestion_and_chain_feed() {
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  const auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  auto product = boolean_operation(a, b, operation::regularized_union,
                                   exact_mesh_options());
  require(product.has_value() && product.value()->mesh,
          "P6.5 exact mesh fixture publishes");

  std::uint64_t chain_feeds = 0;
  qualification_chain_mesh_consumer<coordinate_type, index_type> consumer =
      [&](const auto &mesh) -> product_status_or<bool> {
    ++chain_feeds;
    const auto independently_checked = independently_check_closed_mesh(mesh);
    return independently_checked.faces != 0;
  };
  auto verification = observe_qualification_product_success(
      *product.value(),
      std::make_shared<exact_kernel<coordinate_type>>(), output_test::registry(),
      {probe("occupied-cell", true, true),
       probe("empty-cell", false, false)},
      true, consumer);
  require(verification.has_value() && verification.value().topology.passed() &&
              chain_feeds == 1,
          "mesh success is re-ingested, reconstructed, probed and chain-fed");

  auto accounted = account_qualification_case(success_case(
      "p6.5-real-exact-success", tagged_digest('S', 1),
      dimension(result_representation::exact_in_T_mesh),
      std::move(verification.value()),
      qualification_outcome::verified_exact_success));
  require(accounted.has_value() &&
              accounted.value().outcome ==
                  qualification_outcome::verified_exact_success &&
              !accounted.value().blocking && !accounted.value().safe_failure &&
              accounted.value().false_success_reasons.empty() &&
              validate_qualification_case_accounting(accounted.value()).has_value(),
          "fully verified exact publication normalizes to exact success");

  auto missing_probes = observe_qualification_product_success(
      *product.value(),
      std::make_shared<exact_kernel<coordinate_type>>(), output_test::registry());
  require(missing_probes.has_value(), "missing probe evidence remains observable");
  auto incomplete = account_qualification_case(success_case(
      "p6.5-missing-probes", tagged_digest('S', 2),
      dimension(result_representation::exact_in_T_mesh),
      std::move(missing_probes.value()),
      qualification_outcome::verified_exact_success));
  require(incomplete.has_value() &&
              incomplete.value().outcome ==
                  qualification_outcome::verifier_disagreement &&
              incomplete.value().false_success_reasons.empty(),
          "unrun independent evidence blocks without inventing false success");
}

void false_success_detection() {
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  const auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  auto product = boolean_operation(a, b, operation::regularized_union,
                                   exact_mesh_options());
  require(product.has_value() && product.value()->mesh,
          "false-success fixture publishes");
  auto corrupted = *product.value();
  corrupted.mesh->exact_result_digest.bytes[0] ^= 1U;
  auto verification = observe_qualification_product_success(
      corrupted, std::make_shared<exact_kernel<coordinate_type>>(),
      output_test::registry(), {probe("occupied", true, true)});
  require(verification.has_value(), "corrupt publication produces evidence");
  auto accounted = account_qualification_case(success_case(
      "p6.5-stale-binding", tagged_digest('S', 3),
      dimension(result_representation::exact_in_T_mesh),
      std::move(verification.value()),
      qualification_outcome::verified_exact_success));
  require(accounted.has_value() &&
              accounted.value().outcome == qualification_outcome::false_success &&
              accounted.value().blocking &&
              std::find(accounted.value().false_success_reasons.begin(),
                        accounted.value().false_success_reasons.end(),
                        qualification_false_success_kind::
                            stale_exact_result_binding) !=
                  accounted.value().false_success_reasons.end(),
          "stale exact binding is a blocking false success");

  auto forged = accounted.value();
  forged.false_success_reasons.clear();
  require(!validate_qualification_case_accounting(forged).has_value(),
          "false-success reason removal fails closed");
  forged = accounted.value();
  forged.canonical_bytes.back() ^= 1U;
  require(!validate_qualification_case_accounting(forged).has_value(),
          "canonical accounting corruption fails closed");
}

void complete_outcome_taxonomy() {
  auto expected = account_qualification_case(failure_case(
      "expected", product_error_code::input_contract_error,
      {product_error_code::input_contract_error}));
  require(expected.has_value() &&
              expected.value().outcome ==
                  qualification_outcome::expected_typed_failure &&
              expected.value().safe_failure && !expected.value().blocking,
          "expected typed failure is normalized separately");

  auto unexpected = account_qualification_case(failure_case(
      "unexpected", product_error_code::normalization_failed,
      {product_error_code::input_contract_error}));
  require(unexpected.has_value() &&
              unexpected.value().outcome ==
                  qualification_outcome::unexpected_typed_failure &&
              unexpected.value().safe_failure && unexpected.value().blocking,
          "unexpected typed failure remains safe but blocking");

  auto resource_observation = failure_case(
      "resource", product_error_code::resource_limit,
      {product_error_code::resource_limit});
  resource_observation.timeout_or_resource_limit = true;
  auto resource = account_qualification_case(std::move(resource_observation));
  require(resource.has_value() &&
              resource.value().outcome ==
                  qualification_outcome::timeout_or_resource_limit,
          "resource exhaustion has its own normalized outcome");

  auto backend_observation = failure_case(
      "backend", product_error_code::backend_disagreement, {});
  backend_observation.backend_disagreement = true;
  auto backend = account_qualification_case(std::move(backend_observation));
  require(backend.has_value() &&
              backend.value().outcome ==
                  qualification_outcome::backend_disagreement,
          "backend disagreement has deterministic precedence");

  auto verifier_observation = failure_case(
      "verifier", product_error_code::verifier_disagreement, {});
  verifier_observation.verifier_disagreement = true;
  auto verifier = account_qualification_case(std::move(verifier_observation));
  require(verifier.has_value() &&
              verifier.value().outcome ==
                  qualification_outcome::verifier_disagreement,
          "verifier disagreement has deterministic precedence");

  qualification_case_observation nondeterministic;
  nondeterministic.identifier = "nondeterministic";
  nondeterministic.case_digest = tagged_digest('N', 1);
  nondeterministic.dimensions =
      dimension(result_representation::exact_stratified, "serialization_or_replay");
  nondeterministic.expected_outcomes = {
      qualification_outcome::verified_exact_success};
  nondeterministic.nondeterministic = true;
  auto nondeterminism =
      account_qualification_case(std::move(nondeterministic));
  require(nondeterminism.has_value() &&
              nondeterminism.value().outcome ==
                  qualification_outcome::nondeterministic_outcome,
          "nondeterminism is not collapsed into infrastructure failure");

  qualification_case_observation infrastructure;
  infrastructure.identifier = "infrastructure";
  infrastructure.case_digest = tagged_digest('I', 1);
  infrastructure.dimensions =
      dimension(result_representation::exact_stratified);
  infrastructure.expected_outcomes = {
      qualification_outcome::verified_exact_success};
  infrastructure.infrastructure_failure = true;
  auto infra = account_qualification_case(std::move(infrastructure));
  require(infra.has_value() &&
              infra.value().outcome ==
                  qualification_outcome::infrastructure_failure,
          "infrastructure failure remains explicit");

  auto approximate = account_qualification_case(success_case(
      "approximate", tagged_digest('A', 1),
      dimension(result_representation::certified_approximate_mesh,
                "thin_sliver_or_dense"),
      synthetic_approximate_verification(),
      qualification_outcome::verified_certified_approximate_success));
  require(approximate.has_value() &&
              approximate.value().outcome ==
                  qualification_outcome::verified_certified_approximate_success,
          "verified approximate success remains distinct from exact success");
}

void dimensioned_campaign_and_summary() {
  auto exact_failure = account_qualification_case(failure_case(
      "campaign-safe", product_error_code::input_contract_error,
      {product_error_code::input_contract_error}));
  auto approximate = account_qualification_case(success_case(
      "campaign-approx", tagged_digest('A', 2),
      dimension(result_representation::certified_approximate_mesh,
                "thin_sliver_or_dense"),
      synthetic_approximate_verification(),
      qualification_outcome::verified_certified_approximate_success));
  require(exact_failure.has_value() && approximate.has_value(),
          "campaign source records exist");
  auto passing = make_qualification_accounting_campaign(
      "p6.5-passing", {exact_failure.value(), approximate.value()});
  require(passing.has_value() && passing.value().total_case_count == 2 &&
              passing.value().safe_failure_count == 1 &&
              passing.value().false_success_count == 0 &&
              passing.value().blocking_issue_count == 0 &&
              passing.value().counts.size() == 2 &&
              qualification_false_success_gate_passes(passing.value()),
          "campaign counts safe failures by full dimensions");

  auto summary = make_qualification_result_summary_from_accounting(
      passing.value(), tagged_digest('M', 1), "p6.5-run", "test-commit",
      "2026-07-26T00:00:00Z", "2026-07-26T00:01:00Z");
  require(summary.has_value() && summary.value().counts.size() == 2 &&
              summary.value().false_success_count == 0 &&
              summary.value().blocking_issue_count == 0 &&
              validate_qualification_result_summary(summary.value()).has_value(),
          "dimensioned accounting bridges to canonical result summary");

  auto false_verification = synthetic_approximate_verification();
  for (auto &check : false_verification.checks)
    if (check.kind == qualification_check_kind::guarded_occupancy) {
      check.state = qualification_check_state::failed;
      check.evidence_digest = tagged_digest('X', 1);
      check.message_key = "p6.5.synthetic.guarded_failure";
    }
  false_verification.verification_digest = {};
  auto remade_false =
      make_qualification_success_verification(std::move(false_verification));
  require(remade_false.has_value(), "false verification recanonicalizes");
  auto false_record = account_qualification_case(success_case(
      "campaign-false", tagged_digest('X', 2),
      dimension(result_representation::certified_approximate_mesh),
      std::move(remade_false.value()),
      qualification_outcome::verified_certified_approximate_success));
  require(false_record.has_value() &&
              false_record.value().outcome == qualification_outcome::false_success,
          "campaign false-success source exists");
  auto blocked = make_qualification_accounting_campaign(
      "p6.5-blocked", {exact_failure.value(), false_record.value()});
  require(blocked.has_value() && blocked.value().false_success_count == 1 &&
              blocked.value().blocking_issue_count == 1 &&
              !qualification_false_success_gate_passes(blocked.value()),
          "false success is counted separately and blocks qualification");
  auto corrupted = blocked.value();
  corrupted.false_success_count = 0;
  require(!validate_qualification_accounting_campaign(corrupted).has_value(),
          "campaign count mutation fails closed");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.5.topology", independent_topology_contracts);
  tests.add("P6.5.real_success", real_success_reingestion_and_chain_feed);
  tests.add("P6.5.false_success", false_success_detection);
  tests.add("P6.5.taxonomy", complete_outcome_taxonomy);
  tests.add("P6.5.campaign", dimensioned_campaign_and_summary);
  return tests.run(std::cout, std::cerr);
}
