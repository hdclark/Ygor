#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_INGESTION_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_INGESTION_H_

#include "YgorMeshesBooleanQualificationCorpus.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_cad_ingestion_schema_version = 1;
constexpr std::uint64_t qualification_cad_ci_representative_byte_limit =
    4ULL * 1024ULL * 1024ULL;

enum class qualification_cad_source_class : std::uint8_t {
  analytic_cad_export,
  hand_edited_tessellation,
  scan_derived,
  internally_generated_cad_like,
  private_customer_workload,
  other,
  count
};

enum class qualification_cad_artifact_kind : std::uint8_t {
  operand_pair_bundle,
  source_model_bundle,
  preparation_report,
  expected_outcome_evidence,
  license_or_provenance_evidence,
  compact_ci_representative,
  count
};

enum class qualification_cad_retrieval_kind : std::uint8_t {
  repository_path,
  content_addressed,
  unavailable_private,
  count
};

enum class qualification_cad_content_digest_algorithm : std::uint8_t {
  ygor_domain_md5_128_v1,
  count
};

struct qualification_cad_model_tolerance {
  model_unit unit = model_unit::unspecified;
  std::uint64_t binary64_bits = 0;
};

struct qualification_cad_artifact_reference {
  std::uint16_t schema = qualification_cad_ingestion_schema_version;
  std::string identifier;
  qualification_cad_artifact_kind kind =
      qualification_cad_artifact_kind::operand_pair_bundle;
  qualification_redistribution redistribution =
      qualification_redistribution::repository_embedded;
  qualification_cad_retrieval_kind retrieval =
      qualification_cad_retrieval_kind::repository_path;
  qualification_cad_content_digest_algorithm digest_algorithm =
      qualification_cad_content_digest_algorithm::ygor_domain_md5_128_v1;
  std::string media_type;
  std::uint64_t byte_count = 0;
  digest content_digest;
  std::string repository_path;
  std::string content_address;
  std::vector<std::string> retrieval_procedure;
  bool retrieval_permitted = true;
  digest reference_digest;
};

struct qualification_cad_preparation_record {
  std::uint16_t schema = qualification_cad_ingestion_schema_version;
  preparation_mode mode = preparation_mode::strict_validation;
  digest policy_digest;
  bool attempted = true;
  bool succeeded = true;
  digest source_digest;
  digest output_digest;
  digest report_digest;
  std::uint64_t edit_count = 0;
  bool strict_revalidation_passed = true;
  digest record_digest;
};

struct qualification_cad_case_record {
  std::uint16_t schema = qualification_cad_ingestion_schema_version;
  std::string identifier;
  qualification_cad_source_class source_class =
      qualification_cad_source_class::internally_generated_cad_like;
  std::string source_or_generator_class;
  std::string source_system;
  qualification_corpus_source source =
      qualification_corpus_source::internally_generated_cad_like;
  qualification_redistribution redistribution =
      qualification_redistribution::repository_embedded;
  std::string license_or_provenance;
  qualification_cad_model_tolerance intended_model_tolerance;
  std::uint64_t case_count = 0;
  qualification_cad_preparation_record preparation;
  std::vector<qualification_geometry_category> geometry_categories;
  std::vector<qualification_operation_coverage> operations;
  std::vector<qualification_result_mode_binding> result_modes;
  std::vector<qualification_outcome> expected_outcomes;
  std::vector<product_error_code> expected_failure_codes;
  std::vector<qualification_cad_artifact_reference> artifacts;
  digest record_digest;
};

struct qualification_cad_source_class_count {
  qualification_cad_source_class source_class =
      qualification_cad_source_class::internally_generated_cad_like;
  std::uint64_t count = 0;
};

struct qualification_cad_category_count {
  qualification_geometry_category category =
      qualification_geometry_category::non_box_intersection;
  std::uint64_t count = 0;
};

struct qualification_cad_expected_outcome_count {
  qualification_outcome outcome =
      qualification_outcome::expected_typed_failure;
  std::uint64_t count = 0;
};

struct qualification_cad_failure_code_count {
  product_error_code code = product_error_code::input_contract_error;
  std::uint64_t count = 0;
};

struct qualification_cad_anonymized_summary {
  std::uint16_t schema = qualification_cad_ingestion_schema_version;
  std::uint64_t total_case_count = 0;
  std::uint64_t repository_embedded_case_count = 0;
  std::uint64_t content_addressed_external_case_count = 0;
  std::uint64_t private_digest_only_case_count = 0;
  std::vector<qualification_cad_source_class_count> source_classes;
  std::vector<qualification_cad_category_count> geometry_categories;
  std::vector<qualification_cad_expected_outcome_count> expected_outcomes;
  std::vector<qualification_cad_failure_code_count> expected_failure_codes;
  digest source_record_set_digest;
  digest summary_digest;
};

struct qualification_cad_ingestion_manifest {
  std::uint16_t schema = qualification_cad_ingestion_schema_version;
  std::string identifier;
  std::string version;
  std::vector<qualification_cad_case_record> records;
  qualification_cad_anonymized_summary anonymized_summary;
  digest record_set_digest;
  digest category_coverage_digest;
  digest expected_outcome_digest;
  digest manifest_digest;
};

struct qualification_cad_ingestion_decode_limits {
  std::uint64_t max_record_bytes = 64ULL * 1024ULL * 1024ULL;
  std::uint64_t max_string_bytes = 4ULL * 1024ULL * 1024ULL;
  std::uint64_t max_vector_elements = 1000000;
  std::uint64_t max_total_strings = 4000000;
  std::uint64_t max_artifacts_per_case = 4096;
  std::uint64_t max_retrieval_steps_per_artifact = 256;
};

using qualification_cad_artifact_loader = std::function<
    product_status_or<std::vector<std::uint8_t>>(
        const qualification_cad_artifact_reference &)>;

const char *qualification_cad_source_class_token(
    qualification_cad_source_class) noexcept;
const char *qualification_cad_artifact_kind_token(
    qualification_cad_artifact_kind) noexcept;
const char *qualification_cad_retrieval_kind_token(
    qualification_cad_retrieval_kind) noexcept;

// The artifact address is a versioned, deterministic string derived from the
// frozen in-tree digest type. It contains no transport or credential data.
digest qualification_cad_artifact_content_digest(
    const std::vector<std::uint8_t> &);
std::string qualification_cad_content_address(const digest &);

product_status_or<qualification_cad_artifact_reference>
make_qualification_cad_artifact_reference(
    qualification_cad_artifact_reference);
product_status_or<bool> validate_qualification_cad_artifact_reference(
    const qualification_cad_artifact_reference &) noexcept;
product_status_or<bool> verify_qualification_cad_artifact_bytes(
    const qualification_cad_artifact_reference &,
    const std::vector<std::uint8_t> &) noexcept;
product_status_or<std::vector<std::uint8_t>>
materialize_qualification_cad_artifact(
    const qualification_cad_artifact_reference &,
    const qualification_cad_artifact_loader &);

product_status_or<qualification_cad_preparation_record>
make_qualification_cad_preparation_record(
    qualification_cad_preparation_record);
product_status_or<bool> validate_qualification_cad_preparation_record(
    const qualification_cad_preparation_record &) noexcept;

product_status_or<qualification_cad_case_record>
make_qualification_cad_case_record(qualification_cad_case_record);
product_status_or<bool> validate_qualification_cad_case_record(
    const qualification_cad_case_record &) noexcept;

product_status_or<qualification_cad_ingestion_manifest>
make_qualification_cad_ingestion_manifest(
    qualification_cad_ingestion_manifest);
product_status_or<bool> validate_qualification_cad_ingestion_manifest(
    const qualification_cad_ingestion_manifest &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_cad_ingestion_manifest(
    const qualification_cad_ingestion_manifest &);
product_status_or<qualification_cad_ingestion_manifest>
decode_qualification_cad_ingestion_manifest(
    const std::vector<std::uint8_t> &,
    const qualification_cad_ingestion_decode_limits & = {});

product_status_or<std::vector<qualification_corpus_binding>>
make_qualification_cad_corpus_bindings(
    const qualification_cad_ingestion_manifest &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
