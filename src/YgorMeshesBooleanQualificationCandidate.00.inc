#include "YgorMeshesBooleanQualificationCandidate.h"

#include <algorithm>
#include <array>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> gate_tag{{'Y','G','B','Q','C','G','0','1'}};
constexpr std::array<char, 8> case_tag{{'Y','G','B','Q','C','C','0','1'}};
constexpr std::array<char, 8> plan_tag{{'Y','G','B','Q','C','P','0','1'}};
constexpr std::array<char, 8> observation_tag{{'Y','G','B','Q','C','O','0','1'}};
constexpr std::array<char, 8> regression_tag{{'Y','G','B','Q','C','R','0','1'}};
constexpr std::array<char, 8> issue_tag{{'Y','G','B','Q','C','I','0','1'}};
constexpr std::array<char, 8> campaign_tag{{'Y','G','B','Q','C','M','0','1'}};

product_error candidate_error(const char *key) {
  return make_product_error(product_error_code::qualification_policy_violation,
                            key);
}

bool zero(const digest &value) noexcept { return value == digest{}; }

bool text(const std::string &value, bool allow_empty = false) noexcept {
  return (allow_empty || !value.empty()) && value.size() <= 4096U &&
         std::find(value.begin(), value.end(), '\0') == value.end();
}

template <class E> bool before_count(E value, E count) noexcept {
  return static_cast<unsigned>(value) < static_cast<unsigned>(count);
}

bool known(qualification_candidate_gate_kind value) noexcept {
  return before_count(value, qualification_candidate_gate_kind::count);
}
bool known(qualification_candidate_issue_kind value) noexcept {
  return before_count(value, qualification_candidate_issue_kind::count);
}
bool known(qualification_candidate_issue_disposition value) noexcept {
  return before_count(value, qualification_candidate_issue_disposition::count);
}
bool known(qualification_outcome value) noexcept {
  return value >= qualification_outcome::verified_exact_success &&
         value <= qualification_outcome::infrastructure_failure;
}
bool known(product_error_code value) noexcept {
  return value >= product_error_code::input_contract_error &&
         value <= product_error_code::verifier_disagreement;
}
bool known(backend_id value) noexcept {
  return value == backend_id::experimental_exact_v1 ||
         value == backend_id::independent_axis_aligned_box_v1;
}
bool known(result_representation value) noexcept {
  return value >= result_representation::exact_stratified &&
         value <= result_representation::certified_approximate_mesh;
}
bool known(preparation_mode value) noexcept {
  return value >= preparation_mode::strict_validation &&
         value <= preparation_mode::normalized;
}
bool known(operation value) noexcept {
  return value >= operation::regularized_union &&
         value <= operation::symmetric_difference;
}
bool known(coordinate_tag value) noexcept {
  return value >= coordinate_tag::binary32 && value <= coordinate_tag::binary64;
}
bool known(index_tag value) noexcept {
  return value >= index_tag::uint32 && value <= index_tag::uint64;
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

void encode_dimension(canonical_encoder &encoder,
                      const qualification_dimension_key &value) {
  encoder.u16(static_cast<std::uint16_t>(value.backend));
  encoder.byte(static_cast<std::uint8_t>(value.representation));
  encoder.byte(static_cast<std::uint8_t>(value.preparation));
  encoder.byte(static_cast<std::uint8_t>(value.selected_operation));
  encoder.byte(static_cast<std::uint8_t>(value.coordinate));
  encoder.byte(static_cast<std::uint8_t>(value.index));
  encoder.string(value.toolchain_identifier);
  encoder.string(value.geometry_category);
}

bool valid_dimension(const qualification_dimension_key &value) noexcept {
  return known(value.backend) && known(value.representation) &&
         known(value.preparation) && known(value.selected_operation) &&
         known(value.coordinate) && known(value.index) &&
         text(value.toolchain_identifier) && text(value.geometry_category);
}

void encode_gate(canonical_encoder &encoder,
                 const qualification_candidate_gate_binding &value,
                 bool include_digest) {
  encoder.u16(value.schema);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.string(value.identifier);
  encode_digest(encoder, value.plan_digest);
  encode_digest(encoder, value.report_digest);
  encode_digest(encoder, value.independent_validation_digest);
  encoder.boolean(value.complete);
  encoder.boolean(value.passed);
  if (include_digest)
    encode_digest(encoder, value.binding_digest);
}

digest gate_digest(const qualification_candidate_gate_binding &value) {
  canonical_encoder encoder;
  encode_gate(encoder, value, false);
