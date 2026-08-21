#include "MeshBooleanInputTopologyFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanPreparation.h>
#include <YgorMeshesBooleanQualificationGeneration.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <string>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

template <class T, class I>
bool strict_valid(const fv_surface_mesh<T, I> &mesh) {
  auto registry = input_test::registry();
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  return validate_operand_strict(mesh, strict_validation_policy{},
                                 boolean_options{}, kernel, verifier)
      .has_value();
}

template <class T, class I>
void materializes(const qualification_case_recipe &recipe) {
  auto made = materialize_qualification_case<T, I>(recipe);
  require(made.has_value(), "recipe materializes in required specialization");
  require(!made.value().first.vertices.empty() &&
              !made.value().second.vertices.empty(),
          "materialized operands are nonempty");
  require(qualification_materialized_mesh_digest(made.value().first) != digest{} &&
              qualification_materialized_mesh_digest(made.value().second) !=
                  digest{},
          "materialized meshes are canonically digest-bound");
}

digest observation_digest(const qualification_case_recipe &recipe) {
  canonical_encoder encoder;
  encoder.raw(recipe.case_digest.bytes.data(), recipe.case_digest.bytes.size());
  return domain_digest({{'Y', 'G', 'B', 'Q', 'O', 'B', '0', '1'}},
                       encoder.bytes());
}

void descriptor_and_materialization_contracts() {
  const auto descriptors = qualification_generator_descriptors();
  require_equal(descriptors.size(), std::size_t(16),
                "all P6.2 pair-family recipes are executable");
  std::set<std::string> identifiers;
  std::uint64_t generated = 0, cad_like = 0;
  for (const auto &descriptor : descriptors) {
    require(identifiers.insert(descriptor.identifier).second,
            "generator identifiers are unique");
    require(descriptor.schema == qualification_generation_schema_version &&
                descriptor.recipe_digest != digest{} &&
                descriptor.implementation_digest != digest{} &&
                descriptor.case_count != 0,
            "generator descriptor is fully bound");
    if (descriptor.corpus_kind ==
        qualification_corpus_record_kind::generated_pair_family)
      generated += descriptor.case_count;
    else if (descriptor.corpus_kind ==
             qualification_corpus_record_kind::cad_like_pair_family)
      cad_like += descriptor.case_count;
    else
      require(false, "pair descriptor has pair-family corpus kind");

    for (const auto ordinal :
         {descriptor.first_case_ordinal,
          descriptor.first_case_ordinal + descriptor.case_count - 1}) {
      auto first = make_qualification_case_recipe(
          descriptor.identifier, descriptor.version, ordinal);
      auto replay = make_qualification_case_recipe(
          descriptor.identifier, descriptor.version, ordinal);
      require(first.has_value() && replay.has_value(),
              "descriptor ordinal materializes");
      require(first.value().case_digest == replay.value().case_digest,
              "generator replay is deterministic");
      auto first_bytes = encode_qualification_case_recipe(first.value());
      auto replay_bytes = encode_qualification_case_recipe(replay.value());
      require(first_bytes.has_value() && replay_bytes.has_value() &&
                  first_bytes.value() == replay_bytes.value(),
              "generator canonical bytes replay exactly");
      materializes<float, std::uint32_t>(first.value());
      materializes<float, std::uint64_t>(first.value());
      materializes<double, std::uint32_t>(first.value());
      materializes<double, std::uint64_t>(first.value());
    }
  }
  require_equal(generated, std::uint64_t(10500),
                "all 10,500 construction-known ordinals are executable");
  require_equal(cad_like, std::uint64_t(1100),
                "all 1,100 CAD-like ordinals are executable");

  auto halfspace = make_qualification_case_recipe(
      "exact-halfspace-skew-convex-v1", "1", 0);
  require(halfspace.has_value() &&
              halfspace.value().operand_a_halfspaces.size() == 6 &&
              halfspace.value().operand_b_halfspaces.size() == 6,
          "halfspace polytope recipes retain all exact construction planes");
}

void representative_operands_pass_strict_validation() {
  for (const auto &descriptor : qualification_generator_descriptors()) {
    auto recipe = make_qualification_case_recipe(descriptor.identifier,
                                                 descriptor.version, 0);
    require(recipe.has_value(), "representative recipe exists");
    auto materialized =
        materialize_qualification_case<double, std::uint32_t>(recipe.value());
    require(materialized.has_value(), "representative recipe materializes");
    require(strict_valid(materialized.value().first) ==
                recipe.value().expectation.strict_operand_a_expected_valid,
            "operand A strict-validity expectation is executable");
    require(strict_valid(materialized.value().second) ==
                recipe.value().expectation.strict_operand_b_expected_valid,
            "operand B strict-validity expectation is executable");
  }
}

void valid_and_invalid_fuzz_contracts() {
  for (unsigned raw = 0;
       raw < static_cast<unsigned>(qualification_valid_mutation::count); ++raw) {
    const auto mutation = static_cast<qualification_valid_mutation>(raw);
    auto first = make_qualification_valid_fuzz_case(mutation, 0x12340000U + raw);
    auto replay = make_qualification_valid_fuzz_case(mutation, 0x12340000U + raw);
    require(first.has_value() && replay.has_value() &&
                first.value().case_digest == replay.value().case_digest,
            "valid fuzz construction is deterministic");
    require(first.value().expectation.strict_operand_a_expected_valid &&
                first.value().expectation.strict_operand_b_expected_valid &&
                first.value().expectation.defect ==
                    qualification_defect_label::none,
            "valid fuzz families do not smuggle preparation defects");
  }

  for (unsigned raw = 1;
       raw < static_cast<unsigned>(qualification_defect_label::count); ++raw) {
    const auto defect = static_cast<qualification_defect_label>(raw);
    auto recipe = make_qualification_invalid_fuzz_case(defect, 0x98760000U + raw);
    require(recipe.has_value() && recipe.value().expectation.defect == defect &&
                recipe.value().expectation.normalization_report_required,
            "invalid fuzz case retains its exact controlled defect label");

    qualification_preparation_observation observation;
    observation.strict_operand_a_valid =
        recipe.value().expectation.strict_operand_a_expected_valid;
    observation.strict_operand_b_valid =
        recipe.value().expectation.strict_operand_b_expected_valid;
    observation.normalization_attempted = true;
    observation.normalization_succeeded =
        defect != qualification_defect_label::overlapping_shells &&
        defect != qualification_defect_label::self_intersection;
    observation.reported_defect = defect;
    observation.normalization_edit_count =
        observation.normalization_succeeded ? 1 : 0;
    observation.normalization_report_digest = observation_digest(recipe.value());
    observation.prepared_operand_strictly_valid =
        observation.normalization_succeeded;
    require(validate_qualification_preparation_observation(recipe.value(),
                                                           observation)
                .has_value(),
            "controlled defect observation binds report and strict revalidation");

    auto stale = observation;
    stale.reported_defect = qualification_defect_label::none;
    require(!validate_qualification_preparation_observation(recipe.value(), stale)
                 .has_value(),
            "mislabeled preparation observations fail closed");
  }
}

qualification_chain_step_output<double, std::uint32_t>
synthetic_success(const fv_surface_mesh<double, std::uint32_t> &first,
                  const qualification_chain_step_recipe &step,
                  std::uint64_t call) {
  qualification_chain_step_output<double, std::uint32_t> output;
  canonical_encoder encoder;
  encoder.u64(call);
  encoder.byte(static_cast<std::uint8_t>(step.selected_operation));
  output.exact_result_digest =
      domain_digest({{'Y', 'G', 'B', 'Q', 'S', 'Y', '0', '1'}},
                    encoder.bytes());
  output.outcome =
      step.requested_result == result_representation::certified_approximate_mesh
          ? qualification_outcome::verified_certified_approximate_success
          : qualification_outcome::verified_exact_success;
  if (step.requested_result != result_representation::exact_stratified)
    output.realized_mesh = first;
  return output;
}

void chain_execution_contracts() {
  const std::array<const char *, 5> chains{{
      "chain-mixed-csg-reingestion-v1",
      "chain-cavity-coplanar-v1",
      "chain-subdivision-association-v1",
      "chain-explicit-preparation-boundary-v1",
      "chain-transactional-failure-replay-v1"}};
  for (const auto *identifier : chains) {
    auto recipe =
        make_qualification_operation_chain_recipe(identifier, "1", 0);
    require(recipe.has_value() && recipe.value().steps.size() >= 5,
            "bound chain recipe has at least five steps");
    auto replay =
        make_qualification_operation_chain_recipe(identifier, "1", 0);
    require(replay.has_value() &&
                replay.value().chain_digest == recipe.value().chain_digest,
            "chain definition replays deterministically");

    std::uint64_t calls = 0, reingestions = 0;
    qualification_chain_step_executor<double, std::uint32_t> executor =
        [&](const auto &first, const auto &, const auto &step)
        -> product_status_or<
            qualification_chain_step_output<double, std::uint32_t>> {
      ++calls;
      if (!step.expected_failure_codes.empty())
        return make_product_error(step.expected_failure_codes.front(),
                                  "synthetic.expected_failure");
      return synthetic_success(first, step, calls);
    };
    qualification_chain_reingestor<double, std::uint32_t> reingestor =
        [&](const auto &mesh) -> product_status_or<bool> {
      ++reingestions;
      return !mesh.vertices.empty() && !mesh.faces.empty();
    };
    auto executed = execute_qualification_operation_chain(
        recipe.value(), executor, reingestor);
    require(executed.has_value() && executed.value().transcript_digest != digest{} &&
                executed.value().steps.size() == recipe.value().steps.size() &&
                !executed.value().retained_exact_results.empty(),
            "chain harness retains exact results and complete transcript");
    require(reingestions != 0,
            "successful mesh results and legal subdivisions are re-ingested");
    if (std::string(identifier) ==
        "chain-transactional-failure-replay-v1") {
      const auto failure = std::find_if(
          executed.value().steps.begin(), executed.value().steps.end(),
          [](const auto &record) { return record.failure_code.has_value(); });
      require(failure != executed.value().steps.end() &&
                  failure->state_unchanged_after_failure,
              "expected chain failure preserves the prior accumulator");
    }
  }
}

void minimization_and_promotion_contracts() {
  auto source = make_qualification_invalid_fuzz_case(
      qualification_defect_label::duplicate_facet, 77);
  require(source.has_value(), "minimization source exists");
  qualification_failure_provenance provenance;
  provenance.operand_b_faces = {0};
  provenance.operand_b_vertices = {0};
  auto bound = make_qualification_failure_provenance(std::move(provenance));
  require(bound.has_value(), "failure provenance canonicalizes");
  const auto reproduces = [](const qualification_case_recipe &candidate) {
    if (candidate.expectation.defect !=
        qualification_defect_label::duplicate_facet)
      return false;
    const auto &faces = candidate.operand_b.faces;
    for (std::size_t i = 0; i != faces.size(); ++i)
      for (std::size_t j = i + 1; j != faces.size(); ++j)
        if (faces[i] == faces[j])
          return true;
    return false;
  };
  auto first = minimize_qualification_case(source.value(), bound.value(),
                                           reproduces, 256);
  auto replay = minimize_qualification_case(source.value(), bound.value(),
                                            reproduces, 256);
  require(first.has_value() && replay.has_value() &&
              first.value().transcript_digest == replay.value().transcript_digest &&
              first.value().minimized.case_digest ==
                  replay.value().minimized.case_digest,
          "provenance-guided minimization is deterministic");
  require(first.value().attempts != 0 && !first.value().edits.empty(),
          "minimizer records attempted and accepted edits");

  auto promotion = make_qualification_regression_promotion(first.value());
  require(promotion.has_value() && promotion.value().artifact_digest != digest{} &&
              !promotion.value().canonical_case_bytes.empty(),
          "minimized failure emits a permanent-regression promotion artifact");
  std::uint64_t sink_calls = 0;
  qualification_regression_sink sink =
      [&](const qualification_regression_promotion &) -> product_status_or<bool> {
    ++sink_calls;
    return true;
  };
  require(promote_qualification_regression(promotion.value(), sink).has_value() &&
              sink_calls == 1,
          "validated promotion is published atomically through the sink");
  auto stale = promotion.value();
  stale.artifact_digest.bytes[0] ^= 1U;
  require(!promote_qualification_regression(stale, sink).has_value() &&
              sink_calls == 1,
          "stale promotion artifacts fail before reaching the sink");
}

void fail_closed_contracts() {
  auto recipe = make_qualification_case_recipe(
      "exact-profile-extrusion-concave-v1", "1", 0);
  require(recipe.has_value(), "fail-closed source exists");
  auto stale = recipe.value();
  stale.ordinal = 1200;
  stale.case_digest = {};
  require(!validate_qualification_case_recipe(stale).has_value(),
          "out-of-range recipe ordinal fails closed");

  auto edited_ordinal = recipe.value();
  edited_ordinal.operand_a.vertices.front().coordinate.front().value =
      edited_ordinal.operand_a.vertices.front().coordinate.front().value +
      exact_rational(1);
  edited_ordinal.case_digest = {};
  require(!validate_qualification_case_recipe(edited_ordinal).has_value(),
          "an edited mesh cannot masquerade as a frozen inventory ordinal");

  auto derived = make_qualification_valid_fuzz_case(
      qualification_valid_mutation::alternate_subdivision, 11);
  require(derived.has_value(), "derived fail-closed source exists");
  auto unbound_derived = derived.value();
  unbound_derived.parent_case_digest = {};
  unbound_derived.case_digest = {};
  require(!validate_qualification_case_recipe(unbound_derived).has_value(),
          "derived cases require their parent-case digest");

  auto chain = make_qualification_operation_chain_recipe(
      "chain-mixed-csg-reingestion-v1", "1", 0);
  require(chain.has_value(), "fail-closed chain exists");
  auto short_chain = chain.value();
  short_chain.steps.resize(4);
  short_chain.chain_digest = {};
  require(!validate_qualification_operation_chain_recipe(short_chain).has_value(),
          "short operation chain fails closed");

  auto reinterpreted_chain = chain.value();
  reinterpreted_chain.steps.front().preparation = preparation_mode::normalized;
  reinterpreted_chain.chain_digest = {};
  require(!validate_qualification_operation_chain_recipe(reinterpreted_chain)
               .has_value(),
          "normalization boundaries are bound to their declared chain family");

  qualification_failure_provenance duplicate;
  duplicate.operand_a_faces = {1, 1};
  require(!make_qualification_failure_provenance(duplicate).has_value(),
          "duplicate provenance indices fail closed");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.3.generators.descriptors",
            descriptor_and_materialization_contracts);
  tests.add("P6.3.generators.strict_validation",
            representative_operands_pass_strict_validation);
  tests.add("P6.3.fuzz.valid_invalid", valid_and_invalid_fuzz_contracts);
  tests.add("P6.3.chains.execution", chain_execution_contracts);
  tests.add("P6.3.minimization.promotion",
            minimization_and_promotion_contracts);
  tests.add("P6.3.contracts.fail_closed", fail_closed_contracts);
  return tests.run(std::cout, std::cerr);
}
