#include "YgorMeshesBooleanQualificationIngestion.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <new>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> artifact_content_tag{
    {'Y', 'G', 'B', 'Q', 'A', 'C', '0', '1'}};
constexpr std::array<char, 8> artifact_reference_tag{
    {'Y', 'G', 'B', 'Q', 'A', 'R', '0', '1'}};
constexpr std::array<char, 8> preparation_record_tag{
    {'Y', 'G', 'B', 'Q', 'P', 'R', '0', '1'}};
constexpr std::array<char, 8> case_record_tag{
    {'Y', 'G', 'B', 'Q', 'C', 'I', '0', '1'}};
constexpr std::array<char, 8> record_set_tag{
    {'Y', 'G', 'B', 'Q', 'I', 'S', '0', '1'}};
constexpr std::array<char, 8> category_coverage_tag{
    {'Y', 'G', 'B', 'Q', 'I', 'C', '0', '1'}};
constexpr std::array<char, 8> expected_outcome_tag{
    {'Y', 'G', 'B', 'Q', 'I', 'O', '0', '1'}};
constexpr std::array<char, 8> anonymized_summary_tag{
    {'Y', 'G', 'B', 'Q', 'I', 'A', '0', '1'}};
constexpr std::array<char, 8> manifest_tag{
    {'Y', 'G', 'B', 'Q', 'I', 'M', '0', '1'}};
constexpr std::array<char, 8> manifest_frame_tag{
    {'Y', 'G', 'B', 'Q', 'I', 'F', '0', '1'}};
constexpr std::array<char, 8> binding_category_tag{
    {'Y', 'G', 'B', 'Q', 'B', 'C', '0', '1'}};
constexpr std::array<char, 8> binding_outcome_tag{
    {'Y', 'G', 'B', 'Q', 'B', 'O', '0', '1'}};

template <class Enum> unsigned ordinal(Enum value) noexcept {
  return static_cast<unsigned>(value);
}

product_error ingestion_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

bool zero(const digest &value) noexcept { return value == digest{}; }

bool text(const std::string &value, bool allow_empty = false) noexcept {
  if ((!allow_empty && value.empty()) || value.size() > 1024U * 1024U)
    return false;
  return std::find(value.begin(), value.end(), '\0') == value.end();
}

bool known(qualification_cad_source_class value) noexcept {
  return ordinal(value) < ordinal(qualification_cad_source_class::count);
}
bool known(qualification_cad_artifact_kind value) noexcept {
  return ordinal(value) < ordinal(qualification_cad_artifact_kind::count);
}
bool known(qualification_cad_retrieval_kind value) noexcept {
  return ordinal(value) < ordinal(qualification_cad_retrieval_kind::count);
}
bool known(qualification_cad_content_digest_algorithm value) noexcept {
  return ordinal(value) <
         ordinal(qualification_cad_content_digest_algorithm::count);
}
bool known(qualification_corpus_source value) noexcept {
  return ordinal(value) <= ordinal(qualification_corpus_source::minimized_regression);
}
bool known(qualification_redistribution value) noexcept {
  return ordinal(value) <= ordinal(qualification_redistribution::private_digest_only);
}
bool known(model_unit value) noexcept {
  return ordinal(value) <= ordinal(model_unit::foot);
}
bool known(preparation_mode value) noexcept {
  return ordinal(value) <= ordinal(preparation_mode::normalized);
}
bool known(qualification_geometry_category value) noexcept {
  return ordinal(value) < ordinal(qualification_geometry_category::count);
}
bool known(operation value) noexcept {
  return ordinal(value) <= ordinal(operation::symmetric_difference);
}
bool known(qualification_operand_order value) noexcept {
  return ordinal(value) <= ordinal(qualification_operand_order::b_then_a);
}
bool known(result_representation value) noexcept {
  return ordinal(value) <=
         ordinal(result_representation::certified_approximate_mesh);
}
bool known(product_realization_semantics value) noexcept {
  return ordinal(value) <= ordinal(
      product_realization_semantics::certified_approximate_embedding_v1);
}
bool known(qualification_outcome value) noexcept {
  return ordinal(value) <= ordinal(qualification_outcome::infrastructure_failure);
}
bool known(product_error_code value) noexcept {
  return ordinal(value) <= ordinal(product_error_code::verifier_disagreement);
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

bool add_count(std::uint64_t &target, std::uint64_t value) noexcept {
  if (value > std::numeric_limits<std::uint64_t>::max() - target)
    return false;
  target += value;
  return true;
}

template <class Value, class Less, class Equal>
bool sort_unique(std::vector<Value> &values, Less less, Equal equal) {
  std::sort(values.begin(), values.end(), less);
  return std::adjacent_find(values.begin(), values.end(), equal) == values.end();
}

template <class Enum>
bool sort_unique_enum(std::vector<Enum> &values) {
  return sort_unique(
      values, [](Enum a, Enum b) { return ordinal(a) < ordinal(b); },
      [](Enum a, Enum b) { return a == b; });
}

bool valid_result_mode(const qualification_result_mode_binding &value) noexcept {
  if (!known(value.representation) || !known(value.semantics) ||
      zero(value.policy_digest))
    return false;
  if (value.representation == result_representation::exact_stratified)
    return value.semantics == product_realization_semantics::not_requested;
  if (value.representation == result_representation::exact_in_T_mesh)
    return value.semantics == product_realization_semantics::exact_in_T;
  return value.semantics ==
         product_realization_semantics::certified_approximate_embedding_v1;
}

bool valid_model_tolerance(
    const qualification_cad_model_tolerance &value) noexcept {
  if (!known(value.unit) || value.unit == model_unit::unspecified)
    return false;
  const std::uint64_t sign = value.binary64_bits >> 63U;
  const std::uint64_t exponent = (value.binary64_bits >> 52U) & 0x7ffU;
  return sign == 0 && exponent != 0x7ffU;
}

bool safe_repository_path(const std::string &path) noexcept {
  if (!text(path) || path.front() == '/' || path.back() == '/' ||
      path.find('\\') != std::string::npos)
    return false;
  std::size_t begin = 0;
  while (begin < path.size()) {
    const auto end = path.find('/', begin);
    const auto count = end == std::string::npos ? path.size() - begin
                                                : end - begin;
    if (count == 0)
      return false;
    const auto segment = path.substr(begin, count);
    if (segment == "." || segment == "..")
      return false;
    if (end == std::string::npos)
      break;
    begin = end + 1;
  }
  return true;
}

bool source_class_matches(const qualification_cad_case_record &record) noexcept {
  if (record.source == qualification_corpus_source::internally_generated_cad_like)
    return record.source_class ==
           qualification_cad_source_class::internally_generated_cad_like;
  if (record.source == qualification_corpus_source::licensed_external)
    return record.source_class !=
               qualification_cad_source_class::internally_generated_cad_like &&
           record.source_class !=
               qualification_cad_source_class::private_customer_workload;
  if (record.source == qualification_corpus_source::private_external)
    return record.source_class !=
           qualification_cad_source_class::internally_generated_cad_like;
  return false;
}

bool source_redistribution_matches(
    const qualification_cad_case_record &record) noexcept {
  if (record.source == qualification_corpus_source::internally_generated_cad_like)
    return record.redistribution !=
           qualification_redistribution::private_digest_only;
  if (record.source == qualification_corpus_source::private_external)
    return record.redistribution !=
           qualification_redistribution::repository_embedded;
  return record.source == qualification_corpus_source::licensed_external;
}

bool allowed_expected_outcome(qualification_outcome value) noexcept {
  return value == qualification_outcome::verified_exact_success ||
         value ==
             qualification_outcome::verified_certified_approximate_success ||
         value == qualification_outcome::expected_typed_failure ||
         value == qualification_outcome::timeout_or_resource_limit;
}

bool artifact_less(const qualification_cad_artifact_reference &a,
                   const qualification_cad_artifact_reference &b) noexcept {
  return std::tie(a.kind, a.identifier) < std::tie(b.kind, b.identifier);
}

bool operation_less(const qualification_operation_coverage &a,
                    const qualification_operation_coverage &b) noexcept {
  return a < b;
}

bool result_mode_less(const qualification_result_mode_binding &a,
                      const qualification_result_mode_binding &b) noexcept {
  return std::tie(a.representation, a.semantics, a.policy_digest) <
         std::tie(b.representation, b.semantics, b.policy_digest);
}

void encode_model_tolerance(canonical_encoder &encoder,
                            const qualification_cad_model_tolerance &value) {
  encoder.byte(static_cast<std::uint8_t>(value.unit));
  encoder.u64(value.binary64_bits);
}

void encode_result_mode(canonical_encoder &encoder,
                        const qualification_result_mode_binding &value) {
  encoder.byte(static_cast<std::uint8_t>(value.representation));
  encoder.byte(static_cast<std::uint8_t>(value.semantics));
  encode_digest(encoder, value.policy_digest);
}

void encode_artifact_payload(
    canonical_encoder &encoder,
    const qualification_cad_artifact_reference &value) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.byte(static_cast<std::uint8_t>(value.redistribution));
  encoder.byte(static_cast<std::uint8_t>(value.retrieval));
  encoder.byte(static_cast<std::uint8_t>(value.digest_algorithm));
  encoder.string(value.media_type);
  encoder.u64(value.byte_count);
  encode_digest(encoder, value.content_digest);
  encoder.string(value.repository_path);
  encoder.string(value.content_address);
  encoder.u64(value.retrieval_procedure.size());
  for (const auto &step : value.retrieval_procedure)
    encoder.string(step);
  encoder.boolean(value.retrieval_permitted);
}

void encode_artifact(canonical_encoder &encoder,
                     const qualification_cad_artifact_reference &value) {
  encode_artifact_payload(encoder, value);
  encode_digest(encoder, value.reference_digest);
}

void encode_preparation_payload(
    canonical_encoder &encoder,
    const qualification_cad_preparation_record &value) {
  encoder.u16(value.schema);
  encoder.byte(static_cast<std::uint8_t>(value.mode));
  encode_digest(encoder, value.policy_digest);
  encoder.boolean(value.attempted);
  encoder.boolean(value.succeeded);
  encode_digest(encoder, value.source_digest);
  encode_digest(encoder, value.output_digest);
  encode_digest(encoder, value.report_digest);
  encoder.u64(value.edit_count);
  encoder.boolean(value.strict_revalidation_passed);
}

void encode_preparation(canonical_encoder &encoder,
                        const qualification_cad_preparation_record &value) {
  encode_preparation_payload(encoder, value);
  encode_digest(encoder, value.record_digest);
}

void encode_case_payload(canonical_encoder &encoder,
                         const qualification_cad_case_record &value) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.source_class));
  encoder.string(value.source_or_generator_class);
  encoder.string(value.source_system);
  encoder.byte(static_cast<std::uint8_t>(value.source));
  encoder.byte(static_cast<std::uint8_t>(value.redistribution));
  encoder.string(value.license_or_provenance);
  encode_model_tolerance(encoder, value.intended_model_tolerance);
  encoder.u64(value.case_count);
  encode_preparation(encoder, value.preparation);
  encoder.u64(value.geometry_categories.size());
  for (const auto category : value.geometry_categories)
    encoder.byte(static_cast<std::uint8_t>(category));
  encoder.u64(value.operations.size());
  for (const auto &coverage : value.operations) {
    encoder.byte(static_cast<std::uint8_t>(coverage.selected_operation));
    encoder.byte(static_cast<std::uint8_t>(coverage.operand_order));
  }
  encoder.u64(value.result_modes.size());
  for (const auto &mode : value.result_modes)
    encode_result_mode(encoder, mode);
  encoder.u64(value.expected_outcomes.size());
  for (const auto outcome : value.expected_outcomes)
    encoder.byte(static_cast<std::uint8_t>(outcome));
  encoder.u64(value.expected_failure_codes.size());
  for (const auto code : value.expected_failure_codes)
    encoder.u16(static_cast<std::uint16_t>(code));
  encoder.u64(value.artifacts.size());
  for (const auto &artifact : value.artifacts)
    encode_artifact(encoder, artifact);
}

void encode_case(canonical_encoder &encoder,
                 const qualification_cad_case_record &value) {
  encode_case_payload(encoder, value);
  encode_digest(encoder, value.record_digest);
}

void encode_source_class_count(
    canonical_encoder &encoder,
    const qualification_cad_source_class_count &value) {
  encoder.byte(static_cast<std::uint8_t>(value.source_class));
  encoder.u64(value.count);
}

void encode_category_count(canonical_encoder &encoder,
                           const qualification_cad_category_count &value) {
  encoder.byte(static_cast<std::uint8_t>(value.category));
  encoder.u64(value.count);
}

void encode_outcome_count(
    canonical_encoder &encoder,
    const qualification_cad_expected_outcome_count &value) {
  encoder.byte(static_cast<std::uint8_t>(value.outcome));
  encoder.u64(value.count);
}

void encode_failure_count(canonical_encoder &encoder,
                          const qualification_cad_failure_code_count &value) {
  encoder.u16(static_cast<std::uint16_t>(value.code));
  encoder.u64(value.count);
}

void encode_summary_payload(
    canonical_encoder &encoder,
    const qualification_cad_anonymized_summary &value) {
  encoder.u16(value.schema);
  encoder.u64(value.total_case_count);
  encoder.u64(value.repository_embedded_case_count);
  encoder.u64(value.content_addressed_external_case_count);
  encoder.u64(value.private_digest_only_case_count);
  encoder.u64(value.source_classes.size());
  for (const auto &count : value.source_classes)
    encode_source_class_count(encoder, count);
  encoder.u64(value.geometry_categories.size());
  for (const auto &count : value.geometry_categories)
    encode_category_count(encoder, count);
  encoder.u64(value.expected_outcomes.size());
  for (const auto &count : value.expected_outcomes)
    encode_outcome_count(encoder, count);
  encoder.u64(value.expected_failure_codes.size());
  for (const auto &count : value.expected_failure_codes)
    encode_failure_count(encoder, count);
  encode_digest(encoder, value.source_record_set_digest);
}

void encode_summary(canonical_encoder &encoder,
                    const qualification_cad_anonymized_summary &value) {
  encode_summary_payload(encoder, value);
  encode_digest(encoder, value.summary_digest);
}

void encode_manifest_payload(canonical_encoder &encoder,
                             const qualification_cad_ingestion_manifest &value) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.version);
  encoder.u64(value.records.size());
  for (const auto &record : value.records)
    encode_case(encoder, record);
  encode_summary(encoder, value.anonymized_summary);
  encode_digest(encoder, value.record_set_digest);
  encode_digest(encoder, value.category_coverage_digest);
  encode_digest(encoder, value.expected_outcome_digest);
}

std::vector<std::uint8_t>
framed(const std::vector<std::uint8_t> &payload) {
  canonical_encoder encoder;
  encoder.raw(reinterpret_cast<const std::uint8_t *>(manifest_frame_tag.data()),
              manifest_frame_tag.size());
  encoder.u16(qualification_cad_ingestion_schema_version);
  encoder.u64(payload.size());
  encoder.raw(payload.data(), payload.size());
  return encoder.bytes();
}

bool preparation_contract(
    const qualification_cad_preparation_record &value) noexcept {
  if (value.schema != qualification_cad_ingestion_schema_version ||
      !known(value.mode) || zero(value.policy_digest) || !value.attempted ||
      zero(value.source_digest) || zero(value.report_digest))
    return false;
  if (value.succeeded) {
    if (zero(value.output_digest))
      return false;
    if (value.mode == preparation_mode::strict_validation &&
        (value.output_digest != value.source_digest || value.edit_count != 0 ||
         !value.strict_revalidation_passed))
      return false;
    if (value.mode == preparation_mode::diagnosis_only &&
        (value.output_digest != value.source_digest || value.edit_count != 0))
      return false;
    if (value.mode == preparation_mode::normalized &&
        !value.strict_revalidation_passed)
      return false;
  } else {
    if (!zero(value.output_digest) || value.strict_revalidation_passed)
      return false;
    if (value.mode == preparation_mode::diagnosis_only)
      return false;
  }
  return true;
}

bool artifact_contract(
    const qualification_cad_artifact_reference &value) noexcept {
  if (value.schema != qualification_cad_ingestion_schema_version ||
      !text(value.identifier) || !known(value.kind) ||
      !known(value.redistribution) || !known(value.retrieval) ||
      !known(value.digest_algorithm) || !text(value.media_type) ||
      value.byte_count == 0 || zero(value.content_digest))
    return false;
  const auto expected_address =
      qualification_cad_content_address(value.content_digest);
  if (value.redistribution == qualification_redistribution::repository_embedded)
    return value.retrieval ==
               qualification_cad_retrieval_kind::repository_path &&
           value.retrieval_permitted &&
           safe_repository_path(value.repository_path) &&
           value.content_address.empty() &&
           value.retrieval_procedure.empty();
  if (value.redistribution ==
      qualification_redistribution::content_addressed_external) {
    if (value.retrieval !=
            qualification_cad_retrieval_kind::content_addressed ||
        !value.retrieval_permitted || !value.repository_path.empty() ||
        value.content_address != expected_address ||
        value.retrieval_procedure.empty())
      return false;
    return std::all_of(value.retrieval_procedure.begin(),
                       value.retrieval_procedure.end(),
                       [](const std::string &step) { return text(step); });
  }
  return value.retrieval ==
             qualification_cad_retrieval_kind::unavailable_private &&
         !value.retrieval_permitted && value.repository_path.empty() &&
         value.content_address.empty() && value.retrieval_procedure.empty();
}

product_status_or<qualification_cad_anonymized_summary>
make_summary(const std::vector<qualification_cad_case_record> &records,
             const digest &record_set_digest) {
  qualification_cad_anonymized_summary summary;
  std::map<qualification_cad_source_class, std::uint64_t> source_counts;
  std::map<qualification_geometry_category, std::uint64_t> category_counts;
  std::map<qualification_outcome, std::uint64_t> outcome_counts;
  std::map<product_error_code, std::uint64_t> failure_counts;
  for (const auto &record : records) {
    if (!add_count(summary.total_case_count, record.case_count) ||
        !add_count(source_counts[record.source_class], record.case_count))
      return ingestion_error(product_error_code::resource_limit,
                             "qualification_cad.summary_count_overflow");
    std::uint64_t *redistribution_count = nullptr;
    if (record.redistribution ==
        qualification_redistribution::repository_embedded)
      redistribution_count = &summary.repository_embedded_case_count;
    else if (record.redistribution ==
             qualification_redistribution::content_addressed_external)
      redistribution_count = &summary.content_addressed_external_case_count;
    else
      redistribution_count = &summary.private_digest_only_case_count;
    if (!add_count(*redistribution_count, record.case_count))
      return ingestion_error(product_error_code::resource_limit,
                             "qualification_cad.summary_count_overflow");
    for (const auto category : record.geometry_categories)
      if (!add_count(category_counts[category], record.case_count))
        return ingestion_error(product_error_code::resource_limit,
                               "qualification_cad.summary_count_overflow");
    for (const auto outcome : record.expected_outcomes)
      if (!add_count(outcome_counts[outcome], record.case_count))
        return ingestion_error(product_error_code::resource_limit,
                               "qualification_cad.summary_count_overflow");
    for (const auto code : record.expected_failure_codes)
      if (!add_count(failure_counts[code], record.case_count))
        return ingestion_error(product_error_code::resource_limit,
                               "qualification_cad.summary_count_overflow");
  }
  for (const auto &entry : source_counts)
    summary.source_classes.push_back({entry.first, entry.second});
  for (const auto &entry : category_counts)
    summary.geometry_categories.push_back({entry.first, entry.second});
  for (const auto &entry : outcome_counts)
    summary.expected_outcomes.push_back({entry.first, entry.second});
  for (const auto &entry : failure_counts)
    summary.expected_failure_codes.push_back({entry.first, entry.second});
  summary.source_record_set_digest = record_set_digest;
  canonical_encoder encoded;
  encode_summary_payload(encoded, summary);
  summary.summary_digest = domain_digest(anonymized_summary_tag, encoded.bytes());
  return summary;
}

bool summary_equal(const qualification_cad_anonymized_summary &a,
                   const qualification_cad_anonymized_summary &b) {
  canonical_encoder encoded_a;
  canonical_encoder encoded_b;
  encode_summary(encoded_a, a);
  encode_summary(encoded_b, b);
  return encoded_a.bytes() == encoded_b.bytes();
}

class reader {
  const std::vector<std::uint8_t> &bytes_;
  const qualification_cad_ingestion_decode_limits &limits_;
  std::size_t at_ = 0;
  std::uint64_t total_strings_ = 0;

  void require(std::size_t count) {
    if (count > bytes_.size() - at_)
      throw std::runtime_error("truncated");
  }

public:
  reader(const std::vector<std::uint8_t> &bytes,
         const qualification_cad_ingestion_decode_limits &limits)
      : bytes_(bytes), limits_(limits) {}

  std::size_t remaining() const noexcept { return bytes_.size() - at_; }
  std::uint8_t byte() {
    require(1);
    return bytes_[at_++];
  }
  bool boolean() {
    const auto value = byte();
    if (value > 1)
      throw std::runtime_error("boolean");
    return value != 0;
  }
  std::uint16_t u16() {
    require(2);
    const auto value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes_[at_]) << 8U) |
        static_cast<std::uint16_t>(bytes_[at_ + 1]));
    at_ += 2;
    return value;
  }
  std::uint64_t u64() {
    std::uint64_t value = 0;
    for (int i = 0; i != 8; ++i)
      value = (value << 8U) | byte();
    return value;
  }
  std::uint64_t count() {
    const auto value = u64();
    if (value > limits_.max_vector_elements)
      throw std::runtime_error("vector_limit");
    return value;
  }
  std::string string() {
    const auto size = u64();
    if (size > limits_.max_string_bytes || size > remaining())
      throw std::runtime_error("string_limit");
    if (++total_strings_ > limits_.max_total_strings)
      throw std::runtime_error("total_string_limit");
    std::string result(
        reinterpret_cast<const char *>(bytes_.data() + at_),
        static_cast<std::size_t>(size));
    at_ += static_cast<std::size_t>(size);
    return result;
  }
  digest digest_value() {
    require(16);
    digest result;
    std::copy(bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
              bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + 16),
              result.bytes.begin());
    at_ += 16;
    return result;
  }
  std::vector<std::uint8_t> raw(std::size_t size) {
    require(size);
    std::vector<std::uint8_t> result(
        bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
        bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + size));
    at_ += size;
    return result;
  }
  const qualification_cad_ingestion_decode_limits &limits() const noexcept {
    return limits_;
  }
};

template <class Enum>
Enum read_enum(reader &input, bool (*is_known)(Enum) noexcept) {
  const auto value = static_cast<Enum>(input.byte());
  if (!is_known(value))
    throw std::runtime_error("unknown_enum");
  return value;
}

qualification_result_mode_binding read_result_mode(reader &input) {
  qualification_result_mode_binding result;
  result.representation = read_enum<result_representation>(input, known);
  result.semantics =
      read_enum<product_realization_semantics>(input, known);
  result.policy_digest = input.digest_value();
  return result;
}

qualification_cad_artifact_reference read_artifact(reader &input) {
  qualification_cad_artifact_reference result;
  result.schema = input.u16();
  result.identifier = input.string();
  result.kind = read_enum<qualification_cad_artifact_kind>(input, known);
  result.redistribution =
      read_enum<qualification_redistribution>(input, known);
  result.retrieval = read_enum<qualification_cad_retrieval_kind>(input, known);
  result.digest_algorithm =
      read_enum<qualification_cad_content_digest_algorithm>(input, known);
  result.media_type = input.string();
  result.byte_count = input.u64();
  result.content_digest = input.digest_value();
  result.repository_path = input.string();
  result.content_address = input.string();
  const auto step_count = input.count();
  if (step_count > input.limits().max_retrieval_steps_per_artifact)
    throw std::runtime_error("retrieval_step_limit");
  for (std::uint64_t i = 0; i != step_count; ++i)
    result.retrieval_procedure.push_back(input.string());
  result.retrieval_permitted = input.boolean();
  result.reference_digest = input.digest_value();
  return result;
}

qualification_cad_preparation_record read_preparation(reader &input) {
  qualification_cad_preparation_record result;
  result.schema = input.u16();
  result.mode = read_enum<preparation_mode>(input, known);
  result.policy_digest = input.digest_value();
  result.attempted = input.boolean();
  result.succeeded = input.boolean();
  result.source_digest = input.digest_value();
  result.output_digest = input.digest_value();
  result.report_digest = input.digest_value();
  result.edit_count = input.u64();
  result.strict_revalidation_passed = input.boolean();
  result.record_digest = input.digest_value();
  return result;
}

qualification_cad_case_record read_case(reader &input) {
  qualification_cad_case_record result;
  result.schema = input.u16();
  result.identifier = input.string();
  result.source_class =
      read_enum<qualification_cad_source_class>(input, known);
  result.source_or_generator_class = input.string();
  result.source_system = input.string();
  result.source = read_enum<qualification_corpus_source>(input, known);
  result.redistribution =
      read_enum<qualification_redistribution>(input, known);
  result.license_or_provenance = input.string();
  result.intended_model_tolerance.unit = read_enum<model_unit>(input, known);
  result.intended_model_tolerance.binary64_bits = input.u64();
  result.case_count = input.u64();
  result.preparation = read_preparation(input);
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i)
    result.geometry_categories.push_back(
        read_enum<qualification_geometry_category>(input, known));
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i) {
    qualification_operation_coverage coverage;
    coverage.selected_operation = read_enum<operation>(input, known);
    coverage.operand_order =
        read_enum<qualification_operand_order>(input, known);
    result.operations.push_back(coverage);
  }
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i)
    result.result_modes.push_back(read_result_mode(input));
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i)
    result.expected_outcomes.push_back(
        read_enum<qualification_outcome>(input, known));
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i) {
    const auto raw = static_cast<product_error_code>(input.u16());
    if (!known(raw))
      throw std::runtime_error("product_error_code");
    result.expected_failure_codes.push_back(raw);
  }
  const auto artifact_count = input.count();
  if (artifact_count > input.limits().max_artifacts_per_case)
    throw std::runtime_error("artifact_limit");
  for (std::uint64_t i = 0; i != artifact_count; ++i)
    result.artifacts.push_back(read_artifact(input));
  result.record_digest = input.digest_value();
  return result;
}

qualification_cad_anonymized_summary read_summary(reader &input) {
  qualification_cad_anonymized_summary result;
  result.schema = input.u16();
  result.total_case_count = input.u64();
  result.repository_embedded_case_count = input.u64();
  result.content_addressed_external_case_count = input.u64();
  result.private_digest_only_case_count = input.u64();
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i) {
    qualification_cad_source_class_count value;
    value.source_class =
        read_enum<qualification_cad_source_class>(input, known);
    value.count = input.u64();
    result.source_classes.push_back(value);
  }
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i) {
    qualification_cad_category_count value;
    value.category =
        read_enum<qualification_geometry_category>(input, known);
    value.count = input.u64();
    result.geometry_categories.push_back(value);
  }
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i) {
    qualification_cad_expected_outcome_count value;
    value.outcome = read_enum<qualification_outcome>(input, known);
    value.count = input.u64();
    result.expected_outcomes.push_back(value);
  }
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i) {
    qualification_cad_failure_code_count value;
    value.code = static_cast<product_error_code>(input.u16());
    if (!known(value.code))
      throw std::runtime_error("product_error_code");
    value.count = input.u64();
    result.expected_failure_codes.push_back(value);
  }
  result.source_record_set_digest = input.digest_value();
  result.summary_digest = input.digest_value();
  return result;
}

qualification_cad_ingestion_manifest read_manifest_payload(
    const std::vector<std::uint8_t> &payload,
    const qualification_cad_ingestion_decode_limits &limits) {
  reader input(payload, limits);
  qualification_cad_ingestion_manifest result;
  result.schema = input.u16();
  result.identifier = input.string();
  result.version = input.string();
  for (std::uint64_t i = 0, n = input.count(); i != n; ++i)
    result.records.push_back(read_case(input));
  result.anonymized_summary = read_summary(input);
  result.record_set_digest = input.digest_value();
  result.category_coverage_digest = input.digest_value();
  result.expected_outcome_digest = input.digest_value();
  result.manifest_digest = input.digest_value();
  if (input.remaining() != 0)
    throw std::runtime_error("trailing_data");
  return result;
}

std::vector<std::uint8_t> extract_payload(
    const std::vector<std::uint8_t> &bytes,
    const qualification_cad_ingestion_decode_limits &limits) {
  if (bytes.size() > limits.max_record_bytes)
    throw std::runtime_error("record_limit");
  reader input(bytes, limits);
  const auto observed_tag = input.raw(manifest_frame_tag.size());
  if (!std::equal(observed_tag.begin(), observed_tag.end(),
                  manifest_frame_tag.begin()))
    throw std::runtime_error("domain_tag");
  if (input.u16() != qualification_cad_ingestion_schema_version)
    throw std::runtime_error("schema");
  const auto payload_size = input.u64();
  if (payload_size != input.remaining() || payload_size > limits.max_record_bytes)
    throw std::runtime_error("payload_length");
  return input.raw(static_cast<std::size_t>(payload_size));
}

} // namespace

const char *qualification_cad_source_class_token(
    qualification_cad_source_class value) noexcept {
  static constexpr const char *tokens[] = {
      "analytic_cad_export",       "hand_edited_tessellation",
      "scan_derived",              "internally_generated_cad_like",
      "private_customer_workload", "other"};
  return known(value) ? tokens[ordinal(value)] : "unknown";
}

const char *qualification_cad_artifact_kind_token(
    qualification_cad_artifact_kind value) noexcept {
  static constexpr const char *tokens[] = {
      "operand_pair_bundle",          "source_model_bundle",
      "preparation_report",           "expected_outcome_evidence",
      "license_or_provenance_evidence", "compact_ci_representative"};
  return known(value) ? tokens[ordinal(value)] : "unknown";
}

const char *qualification_cad_retrieval_kind_token(
    qualification_cad_retrieval_kind value) noexcept {
  static constexpr const char *tokens[] = {
      "repository_path", "content_addressed", "unavailable_private"};
  return known(value) ? tokens[ordinal(value)] : "unknown";
}

digest qualification_cad_artifact_content_digest(
    const std::vector<std::uint8_t> &bytes) {
  return domain_digest(artifact_content_tag, bytes);
}

std::string qualification_cad_content_address(const digest &value) {
  return std::string("ygor-domain-md5-128-v1:") + value.hex();
}

product_status_or<qualification_cad_artifact_reference>
make_qualification_cad_artifact_reference(
    qualification_cad_artifact_reference value) {
  try {
    if (!artifact_contract(value))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.artifact_contract");
    canonical_encoder encoded;
    encode_artifact_payload(encoded, value);
    const auto computed = domain_digest(artifact_reference_tag, encoded.bytes());
    if (!zero(value.reference_digest) && value.reference_digest != computed)
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.artifact_stale_binding");
    value.reference_digest = computed;
    return value;
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.artifact_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.artifact_exception");
  }
}

product_status_or<bool> validate_qualification_cad_artifact_reference(
    const qualification_cad_artifact_reference &value) noexcept {
  try {
    auto made = make_qualification_cad_artifact_reference(value);
    if (!made.has_value())
      return made.error();
    canonical_encoder expected;
    canonical_encoder observed;
    encode_artifact(expected, made.value());
    encode_artifact(observed, value);
    if (expected.bytes() != observed.bytes())
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.artifact_not_canonical");
    return true;
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.artifact_validation_exception");
  }
}

product_status_or<bool> verify_qualification_cad_artifact_bytes(
    const qualification_cad_artifact_reference &reference,
    const std::vector<std::uint8_t> &bytes) noexcept {
  try {
    auto valid = validate_qualification_cad_artifact_reference(reference);
    if (!valid.has_value())
      return valid.error();
    if (bytes.size() != reference.byte_count)
      return ingestion_error(product_error_code::replay_mismatch,
                             "qualification_cad.artifact_size_mismatch");
    if (qualification_cad_artifact_content_digest(bytes) !=
        reference.content_digest)
      return ingestion_error(product_error_code::replay_mismatch,
                             "qualification_cad.artifact_digest_mismatch");
    return true;
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.artifact_verify_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
materialize_qualification_cad_artifact(
    const qualification_cad_artifact_reference &reference,
    const qualification_cad_artifact_loader &loader) {
  auto valid = validate_qualification_cad_artifact_reference(reference);
  if (!valid.has_value())
    return valid.error();
  if (!reference.retrieval_permitted ||
      reference.retrieval ==
          qualification_cad_retrieval_kind::unavailable_private)
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.private_retrieval_forbidden");
  if (!loader)
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.missing_artifact_loader");
  try {
    auto loaded = loader(reference);
    if (!loaded.has_value())
      return loaded.error();
    auto verified = verify_qualification_cad_artifact_bytes(reference,
                                                            loaded.value());
    if (!verified.has_value())
      return verified.error();
    return std::move(loaded.value());
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.materialize_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::internal_invariant_error,
                           "qualification_cad.materialize_loader_exception");
  }
}

product_status_or<qualification_cad_preparation_record>
make_qualification_cad_preparation_record(
    qualification_cad_preparation_record value) {
  try {
    if (!preparation_contract(value))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.preparation_contract");
    canonical_encoder encoded;
    encode_preparation_payload(encoded, value);
    const auto computed = domain_digest(preparation_record_tag, encoded.bytes());
    if (!zero(value.record_digest) && value.record_digest != computed)
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.preparation_stale_binding");
    value.record_digest = computed;
    return value;
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.preparation_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.preparation_exception");
  }
}

product_status_or<bool> validate_qualification_cad_preparation_record(
    const qualification_cad_preparation_record &value) noexcept {
  try {
    auto made = make_qualification_cad_preparation_record(value);
    if (!made.has_value())
      return made.error();
    canonical_encoder expected;
    canonical_encoder observed;
    encode_preparation(expected, made.value());
    encode_preparation(observed, value);
    if (expected.bytes() != observed.bytes())
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.preparation_not_canonical");
    return true;
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.preparation_validation_exception");
  }
}

product_status_or<qualification_cad_case_record>
make_qualification_cad_case_record(qualification_cad_case_record value) {
  try {
    if (value.schema != qualification_cad_ingestion_schema_version ||
        !text(value.identifier) || !known(value.source_class) ||
        !text(value.source_or_generator_class) || !text(value.source_system) ||
        !known(value.source) || !known(value.redistribution) ||
        !text(value.license_or_provenance) ||
        !valid_model_tolerance(value.intended_model_tolerance) ||
        value.case_count == 0 || !source_class_matches(value) ||
        !source_redistribution_matches(value))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_contract");

    auto preparation = make_qualification_cad_preparation_record(
        std::move(value.preparation));
    if (!preparation.has_value())
      return preparation.error();
    value.preparation = std::move(preparation.value());

    if (value.geometry_categories.empty() || value.operations.empty() ||
        value.result_modes.empty() || value.expected_outcomes.empty() ||
        value.artifacts.empty())
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_missing_coverage");

    for (const auto category : value.geometry_categories)
      if (!known(category))
        return ingestion_error(product_error_code::qualification_policy_violation,
                               "qualification_cad.case_unknown_category");
    if (!sort_unique_enum(value.geometry_categories))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_duplicate_category");

    for (const auto &coverage : value.operations)
      if (!known(coverage.selected_operation) ||
          !known(coverage.operand_order))
        return ingestion_error(product_error_code::qualification_policy_violation,
                               "qualification_cad.case_unknown_operation");
    if (!sort_unique(value.operations, operation_less,
                     [](const auto &a, const auto &b) { return a == b; }))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_duplicate_operation");

    for (const auto &mode : value.result_modes)
      if (!valid_result_mode(mode))
        return ingestion_error(product_error_code::qualification_policy_violation,
                               "qualification_cad.case_invalid_result_mode");
    if (!sort_unique(value.result_modes, result_mode_less,
                     [](const auto &a, const auto &b) {
                       return a.representation == b.representation &&
                              a.semantics == b.semantics &&
                              a.policy_digest == b.policy_digest;
                     }))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_duplicate_result_mode");

    for (const auto outcome : value.expected_outcomes)
      if (!known(outcome) || !allowed_expected_outcome(outcome))
        return ingestion_error(product_error_code::qualification_policy_violation,
                               "qualification_cad.case_invalid_expected_outcome");
    if (!sort_unique_enum(value.expected_outcomes))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_duplicate_expected_outcome");

    for (const auto code : value.expected_failure_codes)
      if (!known(code))
        return ingestion_error(product_error_code::qualification_policy_violation,
                               "qualification_cad.case_invalid_failure_code");
    if (!sort_unique_enum(value.expected_failure_codes))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_duplicate_failure_code");
    const bool expects_failure =
        std::find(value.expected_outcomes.begin(), value.expected_outcomes.end(),
                  qualification_outcome::expected_typed_failure) !=
        value.expected_outcomes.end();
    if (expects_failure != !value.expected_failure_codes.empty())
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_failure_vocabulary");
    if (!value.preparation.succeeded && !expects_failure)
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_failed_preparation_outcome");

    for (auto &artifact : value.artifacts) {
      auto made = make_qualification_cad_artifact_reference(std::move(artifact));
      if (!made.has_value())
        return made.error();
      artifact = std::move(made.value());
    }
    if (!sort_unique(value.artifacts, artifact_less,
                     [](const auto &a, const auto &b) {
                       return a.kind == b.kind && a.identifier == b.identifier;
                     }))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_duplicate_artifact");

    std::size_t primary_count = 0;
    std::size_t matching_primary_count = 0;
    std::size_t preparation_count = 0;
    std::size_t outcome_count = 0;
    std::size_t license_count = 0;
    std::size_t representative_count = 0;
    digest primary_digest;
    digest preparation_report_digest;
    for (const auto &artifact : value.artifacts) {
      if (artifact.kind ==
              qualification_cad_artifact_kind::operand_pair_bundle ||
          artifact.kind ==
              qualification_cad_artifact_kind::source_model_bundle) {
        ++primary_count;
        if (artifact.redistribution == value.redistribution) {
          ++matching_primary_count;
          primary_digest = artifact.content_digest;
        }
      } else if (artifact.kind ==
                 qualification_cad_artifact_kind::preparation_report) {
        ++preparation_count;
        preparation_report_digest = artifact.content_digest;
      } else if (artifact.kind ==
                 qualification_cad_artifact_kind::expected_outcome_evidence) {
        ++outcome_count;
      } else if (artifact.kind == qualification_cad_artifact_kind::
                                      license_or_provenance_evidence) {
        ++license_count;
      } else if (artifact.kind ==
                 qualification_cad_artifact_kind::compact_ci_representative) {
        ++representative_count;
        if (artifact.redistribution !=
                qualification_redistribution::repository_embedded ||
            artifact.byte_count > qualification_cad_ci_representative_byte_limit)
          return ingestion_error(
              product_error_code::qualification_policy_violation,
              "qualification_cad.case_invalid_ci_representative");
      }
    }
    if (primary_count == 0 || matching_primary_count != 1 ||
        zero(primary_digest) || primary_digest != value.preparation.source_digest ||
        preparation_count != 1 ||
        preparation_report_digest != value.preparation.report_digest ||
        outcome_count != 1 || license_count != 1 || representative_count > 1)
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_artifact_roles");
    if (value.redistribution !=
            qualification_redistribution::repository_embedded &&
        representative_count != 1)
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.case_missing_ci_representative");
    if (value.redistribution !=
        qualification_redistribution::repository_embedded) {
      const auto representative = std::find_if(
          value.artifacts.begin(), value.artifacts.end(), [](const auto &artifact) {
            return artifact.kind == qualification_cad_artifact_kind::
                                        compact_ci_representative;
          });
      if (representative != value.artifacts.end() &&
          representative->content_digest == primary_digest)
        return ingestion_error(product_error_code::qualification_policy_violation,
                               "qualification_cad.case_unsanitized_representative");
    }

    canonical_encoder encoded;
    encode_case_payload(encoded, value);
    const auto computed = domain_digest(case_record_tag, encoded.bytes());
    if (!zero(value.record_digest) && value.record_digest != computed)
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.case_stale_binding");
    value.record_digest = computed;
    return value;
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.case_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.case_exception");
  }
}

product_status_or<bool> validate_qualification_cad_case_record(
    const qualification_cad_case_record &value) noexcept {
  try {
    auto made = make_qualification_cad_case_record(value);
    if (!made.has_value())
      return made.error();
    canonical_encoder expected;
    canonical_encoder observed;
    encode_case(expected, made.value());
    encode_case(observed, value);
    if (expected.bytes() != observed.bytes())
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.case_not_canonical");
    return true;
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.case_validation_exception");
  }
}

product_status_or<qualification_cad_ingestion_manifest>
make_qualification_cad_ingestion_manifest(
    qualification_cad_ingestion_manifest value) {
  try {
    if (value.schema != qualification_cad_ingestion_schema_version ||
        !text(value.identifier) || !text(value.version) || value.records.empty())
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.manifest_contract");
    for (auto &record : value.records) {
      auto made = make_qualification_cad_case_record(std::move(record));
      if (!made.has_value())
        return made.error();
      record = std::move(made.value());
    }
    if (!sort_unique(value.records,
                     [](const auto &a, const auto &b) {
                       return a.identifier < b.identifier;
                     },
                     [](const auto &a, const auto &b) {
                       return a.identifier == b.identifier;
                     }))
      return ingestion_error(product_error_code::qualification_policy_violation,
                             "qualification_cad.manifest_duplicate_record");

    canonical_encoder records;
    records.u64(value.records.size());
    canonical_encoder categories;
    canonical_encoder outcomes;
    for (const auto &record : value.records) {
      records.string(record.identifier);
      encode_digest(records, record.record_digest);
      categories.string(record.identifier);
      categories.u64(record.geometry_categories.size());
      for (const auto category : record.geometry_categories)
        categories.byte(static_cast<std::uint8_t>(category));
      categories.u64(record.operations.size());
      for (const auto &coverage : record.operations) {
        categories.byte(static_cast<std::uint8_t>(coverage.selected_operation));
        categories.byte(static_cast<std::uint8_t>(coverage.operand_order));
      }
      outcomes.string(record.identifier);
      outcomes.u64(record.expected_outcomes.size());
      for (const auto outcome : record.expected_outcomes)
        outcomes.byte(static_cast<std::uint8_t>(outcome));
      outcomes.u64(record.expected_failure_codes.size());
      for (const auto code : record.expected_failure_codes)
        outcomes.u16(static_cast<std::uint16_t>(code));
      encode_digest(outcomes, record.preparation.record_digest);
    }
    const auto record_set_digest = domain_digest(record_set_tag, records.bytes());
    const auto category_digest =
        domain_digest(category_coverage_tag, categories.bytes());
    const auto outcome_digest = domain_digest(expected_outcome_tag, outcomes.bytes());
    auto summary = make_summary(value.records, record_set_digest);
    if (!summary.has_value())
      return summary.error();

    if ((!zero(value.record_set_digest) &&
         value.record_set_digest != record_set_digest) ||
        (!zero(value.category_coverage_digest) &&
         value.category_coverage_digest != category_digest) ||
        (!zero(value.expected_outcome_digest) &&
         value.expected_outcome_digest != outcome_digest) ||
        (!zero(value.anonymized_summary.summary_digest) &&
         !summary_equal(value.anonymized_summary, summary.value())))
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.manifest_stale_binding");

    value.record_set_digest = record_set_digest;
    value.category_coverage_digest = category_digest;
    value.expected_outcome_digest = outcome_digest;
    value.anonymized_summary = std::move(summary.value());
    canonical_encoder encoded;
    encode_manifest_payload(encoded, value);
    const auto computed = domain_digest(manifest_tag, encoded.bytes());
    if (!zero(value.manifest_digest) && value.manifest_digest != computed)
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.manifest_stale_binding");
    value.manifest_digest = computed;
    return value;
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.manifest_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.manifest_exception");
  }
}

product_status_or<bool> validate_qualification_cad_ingestion_manifest(
    const qualification_cad_ingestion_manifest &value) noexcept {
  try {
    auto made = make_qualification_cad_ingestion_manifest(value);
    if (!made.has_value())
      return made.error();
    canonical_encoder expected;
    canonical_encoder observed;
    encode_manifest_payload(expected, made.value());
    encode_digest(expected, made.value().manifest_digest);
    encode_manifest_payload(observed, value);
    encode_digest(observed, value.manifest_digest);
    if (expected.bytes() != observed.bytes())
      return ingestion_error(product_error_code::stale_binding,
                             "qualification_cad.manifest_not_canonical");
    return true;
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.manifest_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_cad_ingestion_manifest(
    const qualification_cad_ingestion_manifest &value) {
  auto valid = validate_qualification_cad_ingestion_manifest(value);
  if (!valid.has_value())
    return valid.error();
  try {
    canonical_encoder payload;
    encode_manifest_payload(payload, value);
    encode_digest(payload, value.manifest_digest);
    return framed(payload.bytes());
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.encode_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.encode_exception");
  }
}

product_status_or<qualification_cad_ingestion_manifest>
decode_qualification_cad_ingestion_manifest(
    const std::vector<std::uint8_t> &bytes,
    const qualification_cad_ingestion_decode_limits &limits) {
  try {
    auto payload = extract_payload(bytes, limits);
    auto decoded = read_manifest_payload(payload, limits);
    auto valid = validate_qualification_cad_ingestion_manifest(decoded);
    if (!valid.has_value())
      return valid.error();
    auto canonical = encode_qualification_cad_ingestion_manifest(decoded);
    if (!canonical.has_value())
      return canonical.error();
    if (canonical.value() != bytes)
      return ingestion_error(product_error_code::replay_mismatch,
                             "qualification_cad.decode_noncanonical");
    return decoded;
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.decode_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::replay_mismatch,
                           "qualification_cad.decode_rejected");
  }
}

product_status_or<std::vector<qualification_corpus_binding>>
make_qualification_cad_corpus_bindings(
    const qualification_cad_ingestion_manifest &manifest) noexcept {
  try {
    auto valid = validate_qualification_cad_ingestion_manifest(manifest);
    if (!valid.has_value())
      return valid.error();
    std::vector<qualification_corpus_binding> result;
    result.reserve(manifest.records.size());
    for (const auto &record : manifest.records) {
      qualification_corpus_binding binding;
      binding.identifier = manifest.identifier + "/" + record.identifier;
      binding.version = manifest.version;
      binding.source = record.source;
      binding.redistribution = record.redistribution;
      binding.license_or_provenance = record.license_or_provenance;
      binding.case_count = record.case_count;
      binding.corpus_digest = record.record_digest;
      canonical_encoder categories;
      categories.u64(record.geometry_categories.size());
      for (const auto category : record.geometry_categories)
        categories.byte(static_cast<std::uint8_t>(category));
      categories.u64(record.operations.size());
      for (const auto &coverage : record.operations) {
        categories.byte(static_cast<std::uint8_t>(coverage.selected_operation));
        categories.byte(static_cast<std::uint8_t>(coverage.operand_order));
      }
      binding.category_coverage_digest =
          domain_digest(binding_category_tag, categories.bytes());
      canonical_encoder outcomes;
      outcomes.u64(record.expected_outcomes.size());
      for (const auto outcome : record.expected_outcomes)
        outcomes.byte(static_cast<std::uint8_t>(outcome));
      outcomes.u64(record.expected_failure_codes.size());
      for (const auto code : record.expected_failure_codes)
        outcomes.u16(static_cast<std::uint16_t>(code));
      encode_digest(outcomes, record.preparation.record_digest);
      binding.expected_outcome_digest =
          domain_digest(binding_outcome_tag, outcomes.bytes());
      result.push_back(std::move(binding));
    }
    return result;
  } catch (const std::bad_alloc &) {
    return ingestion_error(product_error_code::resource_limit,
                           "qualification_cad.binding_allocation");
  } catch (...) {
    return ingestion_error(product_error_code::qualification_policy_violation,
                           "qualification_cad.binding_exception");
  }
}

} // namespace mesh_boolean
} // namespace ygor
