#include "MeshBooleanOutputFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationComparison.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace ygor;
using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

using coordinate_type = double;
using index_type = std::uint32_t;

class forced_disagreement_backend final
    : public boolean_backend<coordinate_type, index_type> {
  std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate_;

public:
  explicit forced_disagreement_backend(
      std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate)
      : delegate_(std::move(delegate)) {}

  const backend_identity &identity() const noexcept override {
    return delegate_->identity();
  }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::diagnostic_only;
  }
  product_status_or<backend_attempt<coordinate_type, index_type>>
  evaluate(const backend_request<coordinate_type, index_type> &request,
           const backend_execution_state &state) const override {
    return delegate_->evaluate(request, state);
  }
  product_status_or<bool>
  verify(const backend_request<coordinate_type, index_type> &request,
         const backend_attempt<coordinate_type, index_type> &attempt) const
      noexcept override {
    return delegate_->verify(request, attempt);
  }
  product_status_or<backend_comparison_record>
  compare(const backend_request<coordinate_type, index_type> &request,
          const backend_attempt<coordinate_type, index_type> &producer,
          const backend_attempt<coordinate_type, index_type> &comparator) const
      noexcept override {
    auto record = delegate_->compare(request, producer, comparator);
    if (!record.has_value())
      return record.error();
    record.value().outcome = backend_comparison_outcome::disagree;
    record.value().mismatched_cells = 1;
    record.value().exact_volume_matches = false;
    record.value().message_key = "qualification_comparison.test_forced";
    auto digest = backend_comparison_digest(record.value());
    if (!digest.has_value())
      return digest.error();
    record.value().report_digest = digest.value();
    return record.value();
  }
};

class unavailable_comparator final
    : public boolean_backend<coordinate_type, index_type> {
  backend_identity identity_;

public:
  unavailable_comparator() {
    auto reference =
        make_axis_aligned_box_reference_backend<coordinate_type, index_type>();
    require(reference.has_value(), "reference backend identity available");
    identity_ = reference.value()->identity();
  }
  const backend_identity &identity() const noexcept override { return identity_; }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::diagnostic_only;
  }
  product_status_or<backend_attempt<coordinate_type, index_type>>
  evaluate(const backend_request<coordinate_type, index_type> &,
           const backend_execution_state &) const override {
    return make_product_error(product_error_code::backend_unavailable,
                              "qualification_comparison.test_unavailable");
  }
  product_status_or<bool>
  verify(const backend_request<coordinate_type, index_type> &,
         const backend_attempt<coordinate_type, index_type> &) const noexcept
      override {
    return true;
  }
};

std::shared_ptr<const exact_kernel_services<coordinate_type>> kernel() {
  return std::make_shared<exact_kernel<coordinate_type>>();
}

std::shared_ptr<const verifier_service> verifiers() {
  return output_test::registry();
}

std::shared_ptr<backend_registry<coordinate_type, index_type>>
registry_with(std::shared_ptr<const boolean_backend<coordinate_type, index_type>>
                  comparator) {
  auto exact = make_experimental_exact_backend<coordinate_type, index_type>();
  require(exact.has_value(), "exact backend available");
  auto registry =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  require(registry->register_backend(exact.value()).has_value(),
          "register exact backend");
  require(registry->register_backend(std::move(comparator)).has_value(),
          "register comparator backend");
  require(registry->freeze().has_value(), "freeze comparison registry");
  return registry;
}

void workload_contract() {
  const auto a = make_qualification_backend_comparison_workload();
  const auto b = make_qualification_backend_comparison_workload();
  require_equal(a.size(), std::size_t(5), "frozen P6.4 workload size");
  require(qualification_backend_comparison_workload_digest(a) ==
              qualification_backend_comparison_workload_digest(b),
          "workload digest deterministic");
  for (std::size_t i = 0; i != a.size(); ++i) {
    require_equal(a[i].ordinal, static_cast<std::uint64_t>(i),
                  "workload ordinals canonical");
    require(a[i].case_digest == b[i].case_digest &&
                a[i].canonical_bytes == b[i].canonical_bytes,
            "workload cases deterministic");
    require(validate_qualification_backend_comparison_case(a[i]).has_value(),
            "workload case validates");
  }
  auto corrupted = a.front();
  corrupted.operand_a.maximum[0] = corrupted.operand_a.minimum[0];
  require(!validate_qualification_backend_comparison_case(corrupted).has_value(),
          "invalid box fails closed");
  corrupted = a.front();
  corrupted.selected_operation = static_cast<operation>(255);
  require(!validate_qualification_backend_comparison_case(corrupted).has_value(),
          "unknown operation fails closed");
}

void full_independent_campaign() {
  auto registry =
      make_default_backend_registry<coordinate_type, index_type>();
  require(registry.has_value(), "default diagnostic registry");
  const auto k = kernel();
  const auto v = verifiers();
  std::vector<qualification_backend_comparison_evidence> records;
  for (const auto &comparison_case :
       make_qualification_backend_comparison_workload()) {
    auto first = run_qualification_backend_comparison_case(
        comparison_case, *registry.value(), k, v);
    require(first.has_value(),
            first.has_value() ? "comparison run" : first.error().message_key);
    if (first.value().comparison.outcome !=
        backend_comparison_outcome::agree) {
      std::cerr << "P6.4 case=" << comparison_case.identifier
                << " outcome="
                << static_cast<unsigned>(first.value().comparison.outcome)
                << " cells=" << first.value().comparison.mismatched_cells
                << " volume=" << first.value().comparison.exact_volume_matches
                << " components="
                << first.value().comparison.component_count_matches
                << " bounds=" << first.value().comparison.output_bounds_match
                << " message=" << first.value().comparison.message_key;
      if (first.value().producer_attempt.evaluation_failure)
        std::cerr << " producer_failure="
                  << first.value().producer_attempt.evaluation_failure->message_key;
      if (first.value().producer_attempt.verification_failure)
        std::cerr << " producer_verify="
                  << first.value().producer_attempt.verification_failure->message_key;
      if (first.value().comparator_attempt.evaluation_failure)
        std::cerr << " comparator_failure="
                  << first.value().comparator_attempt.evaluation_failure->message_key;
      if (first.value().comparator_attempt.verification_failure)
        std::cerr << " comparator_verify="
                  << first.value().comparator_attempt.verification_failure->message_key;
      std::cerr << '\n';
    }
    require(first.value().comparison.outcome ==
                backend_comparison_outcome::agree,
            "independent backend agrees on declared workload");
    require(first.value().producer_attempt.evaluation_succeeded &&
                first.value().producer_attempt.verification_succeeded &&
                first.value().comparator_attempt.evaluation_succeeded &&
                first.value().comparator_attempt.verification_succeeded,
            "both backend attempts and verifiers succeed");
    require(!first.value().engine_options_canonical_bytes.empty() &&
                !first.value().product_options_canonical_bytes.empty(),
            "complete frozen request options retained");
    require(!first.value().producer_attempt.exact_result_canonical_bytes.empty() &&
                !first.value().producer_attempt.output_canonical_bytes.empty() &&
                !first.value().comparator_attempt.diagnostic_canonical_bytes.empty(),
            "all backend payloads retained");
    require(!first.value().probes.empty(), "exact guarded probes retained");
    require(first.value().probe_grid_shape[0] *
                    first.value().probe_grid_shape[1] *
                    first.value().probe_grid_shape[2] ==
                first.value().probes.size(),
            "guarded probe grid is complete");
    for (const auto &probe : first.value().probes)
      require(probe.classifications_match,
              "guarded exact probe classifications agree");
    require(first.value().differences.empty() && !first.value().blocking,
            "agreement has no material difference");
    records.push_back(std::move(first.value()));
  }
  auto campaign = make_qualification_backend_comparison_campaign(
      "p6.4-independent-axis-box-campaign", records);
  auto repeated_campaign = make_qualification_backend_comparison_campaign(
      "p6.4-independent-axis-box-campaign", std::move(records));
  require(campaign.has_value() && repeated_campaign.has_value() &&
              campaign.value().canonical_bytes ==
                  repeated_campaign.value().canonical_bytes &&
              campaign.value().campaign_digest ==
                  repeated_campaign.value().campaign_digest,
          "comparison campaign is canonically deterministic");
  require(campaign.value().complete &&
              campaign.value().agreement_count == 5 &&
              campaign.value().disagreement_count == 0 &&
              campaign.value().unsupported_count == 0 &&
              campaign.value().blocking_issue_count == 0 &&
              qualification_backend_comparison_gate_passes(campaign.value()),
          "complete independent comparison campaign passes qualification gate");
  require(validate_qualification_backend_comparison_campaign(campaign.value())
              .has_value(),
          "campaign validates");
  auto encoded_campaign =
      encode_qualification_backend_comparison_campaign(campaign.value());
  require(encoded_campaign.has_value() &&
              encoded_campaign.value() == campaign.value().canonical_bytes,
          "campaign artifact encoding is canonical");
  auto mutated_options = campaign.value().records.front();
  mutated_options.product_options_canonical_bytes.front() ^= 1U;
  require(!validate_qualification_backend_comparison_evidence(mutated_options)
               .has_value(),
          "request option mutation fails closed");
  auto truncated_probes = campaign.value().records.front();
  truncated_probes.probes.pop_back();
  require(!validate_qualification_backend_comparison_evidence(truncated_probes)
               .has_value(),
          "incomplete guarded probe grid fails closed");
  auto inconsistent_attempt = campaign.value().records.front();
  inconsistent_attempt.producer_attempt.verification_succeeded = false;
  require(!validate_qualification_backend_comparison_evidence(
               inconsistent_attempt)
               .has_value(),
          "inconsistent backend attempt outcome fails closed");
}

void disagreement_minimization_and_resolution() {
  auto reference =
      make_axis_aligned_box_reference_backend<coordinate_type, index_type>();
  require(reference.has_value(), "reference backend available");
  auto forced = registry_with(
      std::make_shared<forced_disagreement_backend>(reference.value()));
  const auto k = kernel();
  const auto v = verifiers();
  const auto source = make_qualification_backend_comparison_workload().front();
  auto evidence = run_qualification_backend_comparison_case(
      source, *forced, k, v);
  require(evidence.has_value() && evidence.value().blocking &&
              evidence.value().material_disagreement &&
              evidence.value().comparison.outcome ==
                  backend_comparison_outcome::disagree &&
              !evidence.value().resolution && !evidence.value().minimization,
          "unresolved material disagreement blocks qualification");

  auto minimization = minimize_qualification_backend_comparison_case(
      source,
      [&](const qualification_backend_comparison_case &candidate) {
        // The forced comparator disagrees independently of geometry.  Keep the
        // shrink predicate independent from the producer to exercise the
        // deterministic minimizer without rerunning an expensive exact Boolean
        // for every coordinate edit.
        return candidate.selected_operation == source.selected_operation &&
               validate_qualification_backend_comparison_case(candidate)
                   .has_value();
      },
      64);
  require(minimization.has_value() &&
              minimization.value().original_case_digest == source.case_digest &&
              !product_digest_is_zero(minimization.value().transcript_digest),
          "disagreement deterministically minimized and bound");

  qualification_disagreement_resolution resolution;
  resolution.producer = qualification_backend_assessment::correct;
  resolution.comparator = qualification_backend_assessment::incorrect;
  resolution.reviewer = "p6.4-test-reviewer";
  resolution.rationale =
      "The comparator was deliberately altered after an independently matching "
      "probe transcript; the producer retained the correct exact result.";
  canonical_encoder evidence_binding;
  evidence_binding.raw(evidence.value().evidence_digest.bytes.data(),
                       evidence.value().evidence_digest.bytes.size());
  evidence_binding.raw(minimization.value().transcript_digest.bytes.data(),
                       minimization.value().transcript_digest.bytes.size());
  resolution.evidence_digest = domain_digest(
      {{'Y', 'G', 'B', 'Q', 'C', 'T', '0', '1'}}, evidence_binding.bytes());
  auto resolved = resolve_qualification_backend_disagreement(
      std::move(evidence.value()), std::move(minimization.value()),
      std::move(resolution));
  require(resolved.has_value() && !resolved.value().blocking &&
              resolved.value().resolution && resolved.value().minimization &&
              validate_qualification_backend_comparison_evidence(resolved.value())
                  .has_value(),
          "classified minimized disagreement is preserved without remaining "
          "unexplained");

  auto mutated = resolved.value();
  mutated.minimization.reset();
  require(!validate_qualification_backend_comparison_evidence(mutated).has_value(),
          "resolved disagreement without minimization fails closed");
  require(resolved.value().minimization &&
              !resolved.value().minimization->edits.empty(),
          "forced disagreement produces a nontrivial shrink transcript");
  mutated = resolved.value();
  mutated.minimization->edits.front().after += 1;
  require(!validate_qualification_backend_comparison_evidence(mutated).has_value(),
          "mutated minimization edit fails closed");
  mutated = resolved.value();
  mutated.probes.front().midpoint = mutated.probes.front().open_cell_minimum;
  require(!validate_qualification_backend_comparison_evidence(mutated).has_value(),
          "unguarded probe mutation fails closed");
}

void failure_evidence_is_retained_and_blocking() {
  auto unavailable = registry_with(std::make_shared<unavailable_comparator>());
  const auto source = make_qualification_backend_comparison_workload().front();
  auto evidence = run_qualification_backend_comparison_case(
      source, *unavailable, kernel(), verifiers());
  require(evidence.has_value() && evidence.value().blocking &&
              evidence.value().comparison.outcome ==
                  backend_comparison_outcome::unsupported &&
              evidence.value().comparator_attempt.evaluation_failure &&
              evidence.value().comparator_attempt.evaluation_failure->code ==
                  product_error_code::backend_unavailable &&
              !evidence.value().differences.empty(),
          "typed comparator failure and semantic consequence are retained");
}

} // namespace

int main() {
  try {
    workload_contract();
    full_independent_campaign();
    disagreement_minimization_and_resolution();
    failure_evidence_is_retained_and_blocking();
    std::cout << "mesh Boolean P6.4 independent comparison qualification passed\n";
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << "mesh Boolean P6.4 comparison qualification failed: "
              << exception.what() << '\n';
    return 1;
  }
}
