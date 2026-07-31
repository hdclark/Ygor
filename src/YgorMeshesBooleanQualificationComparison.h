#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_COMPARISON_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_COMPARISON_H_

#include "YgorMeshesBooleanBackend.h"
#include "YgorMeshesBooleanQualification.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_backend_comparison_schema_version = 1;
constexpr std::uint32_t qualification_backend_comparison_checker_version = 1;
constexpr const char *qualification_axis_box_comparison_profile =
    "axis_aligned_box_pair_v1";

enum class qualification_backend_assessment : std::uint8_t {
  correct,
  incorrect,
  unsupported,
  policy_different,
  unresolved
};

enum class qualification_semantic_difference_kind : std::uint8_t {
  producer_evaluation_failure,
  comparator_evaluation_failure,
  producer_verification_failure,
  comparator_verification_failure,
  occupancy,
  exact_volume,
  connected_components,
  output_bounds,
  representation_unavailable,
  comparison_unsupported
};

struct qualification_axis_aligned_box {
  std::array<std::int64_t, 3> minimum{{0, 0, 0}};
  std::array<std::int64_t, 3> maximum{{1, 1, 1}};
};

struct qualification_backend_comparison_case {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  std::string identifier;
  std::string workload_profile = qualification_axis_box_comparison_profile;
  std::uint64_t ordinal = 0;
  operation selected_operation = operation::regularized_union;
  qualification_axis_aligned_box operand_a;
  qualification_axis_aligned_box operand_b;
  backend_request_limits limits;
  std::vector<std::uint8_t> canonical_bytes;
  digest case_digest;
};

struct qualification_backend_attempt_evidence {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  backend_identity backend;
  backend_adapter_role role = backend_adapter_role::producer;
  digest request_digest;
  bool evaluation_succeeded = false;
  bool verification_succeeded = false;
  std::optional<product_error> evaluation_failure;
  std::optional<product_error> verification_failure;
  std::vector<std::uint8_t> attempt_canonical_bytes;
  std::vector<std::uint8_t> exact_result_canonical_bytes;
  std::vector<std::uint8_t> realization_canonical_bytes;
  std::vector<std::uint8_t> output_canonical_bytes;
  std::vector<std::uint8_t> diagnostic_canonical_bytes;
  digest payload_digest;
  digest evidence_digest;
};

struct qualification_guarded_probe_evidence {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  std::array<std::uint64_t, 3> cell_index{{0, 0, 0}};
  exact_point3 open_cell_minimum;
  exact_point3 open_cell_maximum;
  exact_point3 midpoint;
  bool comparator_occupied = false;
  solid_point_kind producer_classification = solid_point_kind::outside;
  bool producer_occupied = false;
  bool classifications_match = false;
  digest probe_digest;
};

struct qualification_semantic_difference {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  qualification_semantic_difference_kind kind =
      qualification_semantic_difference_kind::occupancy;
  bool material = true;
  std::optional<std::array<std::uint64_t, 3>> cell_index;
  digest expected_digest;
  digest observed_digest;
  std::string message_key;
  digest difference_digest;
};

struct qualification_comparison_minimization_edit {
  std::uint8_t operand = 0;
  std::uint8_t axis = 0;
  bool maximum = false;
  std::int64_t before = 0;
  std::int64_t after = 0;
  digest before_digest;
  digest after_digest;
};

struct qualification_comparison_minimization {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  digest original_case_digest;
  qualification_backend_comparison_case minimized_case;
  std::uint64_t attempts = 0;
  std::vector<qualification_comparison_minimization_edit> edits;
  digest transcript_digest;
};

struct qualification_disagreement_resolution {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  qualification_backend_assessment producer =
      qualification_backend_assessment::unresolved;
  qualification_backend_assessment comparator =
      qualification_backend_assessment::unresolved;
  std::string reviewer;
  std::string rationale;
  digest evidence_digest;
  digest resolution_digest;
};

struct qualification_backend_comparison_evidence {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  std::uint32_t checker_version =
      qualification_backend_comparison_checker_version;
  qualification_backend_comparison_case comparison_case;
  std::vector<std::uint8_t> engine_options_canonical_bytes;
  std::vector<std::uint8_t> product_options_canonical_bytes;
  std::array<std::uint64_t, 3> probe_grid_shape{{0, 0, 0}};
  qualification_backend_attempt_evidence producer_attempt;
  qualification_backend_attempt_evidence comparator_attempt;
  backend_comparison_record comparison;
  std::vector<qualification_guarded_probe_evidence> probes;
  std::vector<qualification_semantic_difference> differences;
  std::optional<qualification_comparison_minimization> minimization;
  std::optional<qualification_disagreement_resolution> resolution;
  bool material_disagreement = false;
  bool blocking = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest evidence_digest;
};

struct qualification_backend_comparison_campaign {
  std::uint16_t schema = qualification_backend_comparison_schema_version;
  std::uint32_t checker_version =
      qualification_backend_comparison_checker_version;
  std::string identifier;
  std::string workload_profile = qualification_axis_box_comparison_profile;
  digest workload_digest;
  backend_identity producer;
  backend_identity comparator;
  std::vector<qualification_backend_comparison_evidence> records;
  std::uint64_t agreement_count = 0;
  std::uint64_t disagreement_count = 0;
  std::uint64_t unsupported_count = 0;
  std::uint64_t blocking_issue_count = 0;
  bool complete = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest campaign_digest;
};

using qualification_backend_comparison_reproducer =
    std::function<bool(const qualification_backend_comparison_case &)>;

const char *qualification_backend_assessment_token(
    qualification_backend_assessment) noexcept;

std::vector<qualification_backend_comparison_case>
make_qualification_backend_comparison_workload();

product_status_or<qualification_backend_comparison_case>
make_qualification_backend_comparison_case(
    qualification_backend_comparison_case);
product_status_or<bool> validate_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &);

digest qualification_backend_comparison_workload_digest(
    const std::vector<qualification_backend_comparison_case> &);

product_status_or<qualification_comparison_minimization>
minimize_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &,
    const qualification_backend_comparison_reproducer &,
    std::uint64_t attempt_limit = 256);

product_status_or<qualification_backend_comparison_evidence>
resolve_qualification_backend_disagreement(
    qualification_backend_comparison_evidence,
    qualification_comparison_minimization,
    qualification_disagreement_resolution);

product_status_or<bool> validate_qualification_backend_comparison_evidence(
    const qualification_backend_comparison_evidence &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_backend_comparison_evidence(
    const qualification_backend_comparison_evidence &);

product_status_or<qualification_backend_comparison_campaign>
make_qualification_backend_comparison_campaign(
    std::string,
    std::vector<qualification_backend_comparison_evidence>);
product_status_or<bool> validate_qualification_backend_comparison_campaign(
    const qualification_backend_comparison_campaign &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_backend_comparison_campaign(
    const qualification_backend_comparison_campaign &);
bool qualification_backend_comparison_gate_passes(
    const qualification_backend_comparison_campaign &) noexcept;

template <class T, class I>
product_status_or<qualification_backend_comparison_evidence>
run_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &,
    const backend_registry<T, I> &,
    std::shared_ptr<const exact_kernel_services<T>>,
    std::shared_ptr<const verifier_service>,
    std::shared_ptr<cancellation_source> = {}, diagnostic_consumer = {},
    deterministic_executor_factory = {});

#define YGOR_QUALIFICATION_COMPARISON_EXTERN(T, I)                             \
  extern template product_status_or<                                           \
      qualification_backend_comparison_evidence>                               \
  run_qualification_backend_comparison_case(                                   \
      const qualification_backend_comparison_case &,                           \
      const backend_registry<T, I> &,                                           \
      std::shared_ptr<const exact_kernel_services<T>>,                          \
      std::shared_ptr<const verifier_service>,                                  \
      std::shared_ptr<cancellation_source>, diagnostic_consumer,                \
      deterministic_executor_factory)
YGOR_QUALIFICATION_COMPARISON_EXTERN(float, std::uint32_t);
YGOR_QUALIFICATION_COMPARISON_EXTERN(float, std::uint64_t);
YGOR_QUALIFICATION_COMPARISON_EXTERN(double, std::uint32_t);
YGOR_QUALIFICATION_COMPARISON_EXTERN(double, std::uint64_t);
#undef YGOR_QUALIFICATION_COMPARISON_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
