#include "YgorMeshesBooleanQualificationAccounting.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> probe_tag{{'Y', 'G', 'B', 'Q', 'P', 'R', '0', '1'}};
constexpr std::array<char, 8> topology_tag{{'Y', 'G', 'B', 'Q', 'T', 'R', '0', '1'}};
constexpr std::array<char, 8> verification_tag{{'Y', 'G', 'B', 'Q', 'V', 'R', '0', '1'}};
constexpr std::array<char, 8> observation_tag{{'Y', 'G', 'B', 'Q', 'O', 'B', '0', '1'}};
constexpr std::array<char, 8> accounting_tag{{'Y', 'G', 'B', 'Q', 'A', 'C', '0', '1'}};
constexpr std::array<char, 8> accounting_record_tag{{'Y', 'G', 'B', 'Q', 'A', 'R', '0', '1'}};
constexpr std::array<char, 8> campaign_tag{{'Y', 'G', 'B', 'Q', 'A', 'M', '0', '1'}};
constexpr std::array<char, 8> campaign_record_tag{{'Y', 'G', 'B', 'Q', 'A', 'B', '0', '1'}};
constexpr std::array<char, 8> check_evidence_tag{{'Y', 'G', 'B', 'Q', 'C', 'K', '0', '1'}};

template <class E> unsigned ordinal(E value) noexcept {
  return static_cast<unsigned>(value);
}

bool known(qualification_check_state value) noexcept {
  return ordinal(value) <= ordinal(qualification_check_state::failed);
}
bool known(qualification_check_kind value) noexcept {
  return ordinal(value) <= ordinal(qualification_check_kind::chain_reingestion);
}
bool known(qualification_false_success_kind value) noexcept {
  return ordinal(value) <=
         ordinal(qualification_false_success_kind::approximation_bound_violation);
}
bool known(qualification_outcome value) noexcept {
  return ordinal(value) <= ordinal(qualification_outcome::infrastructure_failure);
}
bool known(product_error_code value) noexcept {
  return ordinal(value) <= ordinal(product_error_code::verifier_disagreement);
}
bool known(backend_id value) noexcept {
  return value == backend_id::experimental_exact_v1 ||
         value == backend_id::independent_axis_aligned_box_v1;
}
bool known(result_representation value) noexcept {
  return ordinal(value) <=
         ordinal(result_representation::certified_approximate_mesh);
}
bool known(preparation_mode value) noexcept {
  return ordinal(value) <= ordinal(preparation_mode::normalized);
}
bool known(operation value) noexcept {
  return ordinal(value) <= ordinal(operation::symmetric_difference);
}
bool known(coordinate_tag value) noexcept {
  return ordinal(value) <= ordinal(coordinate_tag::binary64);
}
bool known(index_tag value) noexcept {
  return ordinal(value) <= ordinal(index_tag::uint64);
}

bool text(const std::string &value, bool allow_empty = false) noexcept {
  return (allow_empty || !value.empty()) && value.size() <= 1024U * 1024U &&
         std::find(value.begin(), value.end(), '\0') == value.end();
}
bool zero(const digest &value) noexcept { return value == digest{}; }
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

void encode_product_error(canonical_encoder &encoder,
                          const product_error &value) {
  encoder.u16(value.schema);
  encoder.u16(static_cast<std::uint16_t>(value.code));
  encoder.u32(value.subcode);
  encoder.string(value.message_key);
  encoder.string(value.detail);
  encoder.boolean(static_cast<bool>(value.backend));
  if (value.backend) {
    encoder.u16(static_cast<std::uint16_t>(value.backend->id));
    encoder.u16(value.backend->adapter_version.major);
    encoder.u16(value.backend->adapter_version.minor);
    encoder.u16(value.backend->adapter_version.patch);
    encoder.string(value.backend->build_identifier);
    encode_digest(encoder, value.backend->capability_digest);
    encoder.byte(static_cast<std::uint8_t>(value.backend->maturity));
  }
  encode_digest(encoder, value.replay_binding_digest);
}

bool valid_error(const product_error &value) noexcept {
  return value.schema == product_contract_schema_version && known(value.code) &&
         text(value.message_key) && text(value.detail, true) &&
         (!value.backend || validate_backend_identity(*value.backend).has_value());
}

void encode_check(canonical_encoder &encoder,
                  const qualification_verification_check &value) {
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.byte(static_cast<std::uint8_t>(value.state));
  encoder.string(value.message_key);
  encode_digest(encoder, value.evidence_digest);
}

void encode_probe(canonical_encoder &encoder,
                  const qualification_guarded_probe_observation &value,
                  bool include_digest = true) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encode_digest(encoder, value.point_digest);
  encoder.boolean(value.expected_occupied);
  encoder.boolean(value.observed_occupied);
  encoder.boolean(value.observed_on_boundary);
  if (include_digest)
    encode_digest(encoder, value.probe_digest);
}

void encode_topology(canonical_encoder &encoder,
                     const qualification_mesh_topology_reconstruction &value,
                     bool include_digest = true) {
  encoder.u16(value.schema);
  encoder.u64(value.vertices);
  encoder.u64(value.faces);
  encoder.u64(value.edges);
  encoder.u64(value.connected_components);
  encoder.u64(static_cast<std::uint64_t>(value.euler_characteristic));
  encoder.boolean(value.face_rings_valid);
  encoder.boolean(value.indices_in_range);
  encoder.boolean(value.every_vertex_referenced);
  encoder.boolean(value.closed_two_uses_per_edge);
  encoder.boolean(value.opposite_edge_directions);
  encode_digest(encoder, value.incidence_digest);
  if (include_digest)
    encode_digest(encoder, value.reconstruction_digest);
}

void encode_verification(canonical_encoder &encoder,
                         const qualification_success_verification &value,
                         bool include_digest = true) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.boolean(value.mesh_published);
  encoder.boolean(value.chain_reingestion_required);
  encoder.byte(static_cast<std::uint8_t>(value.representation));
  encode_digest(encoder, value.exact_result_digest);
  encode_digest(encoder, value.output_digest);
  encode_topology(encoder, value.topology);
  encoder.u64(value.probes.size());
  for (const auto &probe : value.probes)
    encode_probe(encoder, probe);
  encoder.u64(value.checks.size());
  for (const auto &check : value.checks)
    encode_check(encoder, check);
  if (include_digest)
    encode_digest(encoder, value.verification_digest);
}

void encode_observation(canonical_encoder &encoder,
                        const qualification_case_observation &value,
                        bool include_digest = true) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encode_digest(encoder, value.case_digest);
  encode_dimension(encoder, value.dimensions);
  encoder.u64(value.expected_outcomes.size());
  for (const auto outcome : value.expected_outcomes)
    encoder.byte(static_cast<std::uint8_t>(outcome));
  encoder.u64(value.expected_failure_codes.size());
  for (const auto code : value.expected_failure_codes)
    encoder.u16(static_cast<std::uint16_t>(code));
  encoder.boolean(value.published_success);
  encoder.boolean(static_cast<bool>(value.failure));
  if (value.failure)
    encode_product_error(encoder, *value.failure);
  encoder.boolean(value.backend_disagreement);
  encoder.boolean(value.verifier_disagreement);
  encoder.boolean(value.nondeterministic);
  encoder.boolean(value.timeout_or_resource_limit);
  encoder.boolean(value.infrastructure_failure);
  encoder.boolean(static_cast<bool>(value.success_verification));
  if (value.success_verification)
    encode_verification(encoder, *value.success_verification);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_accounting(canonical_encoder &encoder,
                       const qualification_case_accounting &value,
                       bool include_digest = true) {
  encoder.u16(value.schema);
  encode_observation(encoder, value.observation);
  encoder.byte(static_cast<std::uint8_t>(value.outcome));
  encoder.boolean(value.blocking);
  encoder.boolean(value.safe_failure);
  encoder.u64(value.false_success_reasons.size());
  for (const auto reason : value.false_success_reasons)
    encoder.byte(static_cast<std::uint8_t>(reason));
  if (include_digest)
    encode_digest(encoder, value.accounting_digest);
}

std::vector<std::uint8_t> framed(const std::array<char, 8> &tag,
                                 std::uint16_t schema,
                                 const std::vector<std::uint8_t> &payload) {
  canonical_encoder encoder;
  encoder.raw(reinterpret_cast<const std::uint8_t *>(tag.data()), tag.size());
  encoder.u16(schema);
  encoder.u64(payload.size());
  encoder.raw(payload.data(), payload.size());
  return encoder.bytes();
}

bool check_less(const qualification_verification_check &a,
                const qualification_verification_check &b) noexcept {
  return ordinal(a.kind) < ordinal(b.kind);
}
bool probe_less(const qualification_guarded_probe_observation &a,
                const qualification_guarded_probe_observation &b) noexcept {
  return std::tie(a.identifier, a.point_digest) <
         std::tie(b.identifier, b.point_digest);
}
bool accounting_less(const qualification_case_accounting &a,
                     const qualification_case_accounting &b) noexcept {
  return std::tie(a.observation.identifier, a.observation.case_digest,
                  a.accounting_digest) <
         std::tie(b.observation.identifier, b.observation.case_digest,
                  b.accounting_digest);
}
bool count_less(const qualification_outcome_count &a,
                const qualification_outcome_count &b) noexcept {
  if (a.dimensions < b.dimensions)
    return true;
  if (b.dimensions < a.dimensions)
    return false;
  return ordinal(a.outcome) < ordinal(b.outcome);
}

template <class Values, class Less>
bool strictly_sorted(const Values &values, Less less) noexcept {
  return std::adjacent_find(
             values.begin(), values.end(),
             [&](const auto &a, const auto &b) { return !less(a, b); }) ==
         values.end();
}

bool counts_equal(const std::vector<qualification_outcome_count> &a,
                  const std::vector<qualification_outcome_count> &b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i != a.size(); ++i)
    if (!(a[i].dimensions == b[i].dimensions) ||
        a[i].outcome != b[i].outcome || a[i].count != b[i].count)
      return false;
  return true;
}

bool blocking_outcome(qualification_outcome value) noexcept {
  return value != qualification_outcome::verified_exact_success &&
         value !=
             qualification_outcome::verified_certified_approximate_success &&
         value != qualification_outcome::expected_typed_failure;
}
bool safe_failure_outcome(qualification_outcome value) noexcept {
  return value == qualification_outcome::expected_typed_failure ||
         value == qualification_outcome::unexpected_typed_failure ||
         value == qualification_outcome::timeout_or_resource_limit;
}

std::optional<qualification_false_success_kind>
false_success_reason(qualification_check_kind kind) noexcept {
  switch (kind) {
  case qualification_check_kind::product_contract:
  case qualification_check_kind::representation_semantics:
    return qualification_false_success_kind::semantic_mislabeling;
  case qualification_check_kind::exact_result_binding:
    return qualification_false_success_kind::stale_exact_result_binding;
  case qualification_check_kind::strict_reingestion:
    return qualification_false_success_kind::strict_reingestion_rejected;
  case qualification_check_kind::independent_topology:
    return qualification_false_success_kind::incorrect_topology;
  case qualification_check_kind::certificate_replay:
    return qualification_false_success_kind::certificate_replay_failed;
  case qualification_check_kind::guarded_occupancy:
    return qualification_false_success_kind::incorrect_occupancy;
  case qualification_check_kind::embedding:
    return qualification_false_success_kind::incorrect_embedding;
  case qualification_check_kind::orientation:
    return qualification_false_success_kind::incorrect_orientation;
  case qualification_check_kind::shell_nesting:
    return qualification_false_success_kind::incorrect_shell_nesting;
  case qualification_check_kind::attribute_mapping:
    return qualification_false_success_kind::incorrect_attribute_mapping;
  case qualification_check_kind::approximation_bounds:
    return qualification_false_success_kind::approximation_bound_violation;
  case qualification_check_kind::chain_reingestion:
    return std::nullopt;
  }
  return std::nullopt;
}

bool required_check(const qualification_success_verification &verification,
                    qualification_check_kind kind) noexcept {
  if (kind == qualification_check_kind::product_contract ||
      kind == qualification_check_kind::exact_result_binding ||
      kind == qualification_check_kind::representation_semantics ||
      kind == qualification_check_kind::attribute_mapping)
    return true;
  if (!verification.mesh_published)
    return false;
  if (kind == qualification_check_kind::strict_reingestion ||
      kind == qualification_check_kind::independent_topology ||
      kind == qualification_check_kind::certificate_replay ||
      kind == qualification_check_kind::guarded_occupancy ||
      kind == qualification_check_kind::embedding ||
      kind == qualification_check_kind::orientation ||
      kind == qualification_check_kind::shell_nesting)
    return true;
  if (kind == qualification_check_kind::approximation_bounds)
    return verification.representation ==
           result_representation::certified_approximate_mesh;
  if (kind == qualification_check_kind::chain_reingestion)
    return verification.chain_reingestion_required;
  return false;
}

const qualification_verification_check *
find_check(const qualification_success_verification &verification,
           qualification_check_kind kind) noexcept {
  const auto found = std::lower_bound(
      verification.checks.begin(), verification.checks.end(), kind,
      [](const qualification_verification_check &check,
         qualification_check_kind target) {
        return ordinal(check.kind) < ordinal(target);
      });
  return found != verification.checks.end() && found->kind == kind ? &*found
                                                                  : nullptr;
}

qualification_outcome successful_outcome(result_representation value) noexcept {
  return value == result_representation::certified_approximate_mesh
             ? qualification_outcome::verified_certified_approximate_success
             : qualification_outcome::verified_exact_success;
}

bool expected_outcome(const qualification_case_observation &observation,
                      qualification_outcome outcome) noexcept {
  return std::binary_search(observation.expected_outcomes.begin(),
                            observation.expected_outcomes.end(), outcome,
                            [](qualification_outcome a,
                               qualification_outcome b) {
                              return ordinal(a) < ordinal(b);
                            });
}

bool expected_failure(const qualification_case_observation &observation,
                      product_error_code code) noexcept {
  return std::binary_search(observation.expected_failure_codes.begin(),
                            observation.expected_failure_codes.end(), code,
                            [](product_error_code a, product_error_code b) {
                              return ordinal(a) < ordinal(b);
                            });
}

bool add(std::uint64_t &target, std::uint64_t value) noexcept {
  if (value > std::numeric_limits<std::uint64_t>::max() - target)
    return false;
  target += value;
  return true;
}

} // namespace

namespace qualification_accounting_detail {
product_error accounting_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

digest mesh_observation_digest(const digest &basis, const char *key) {
  canonical_encoder encoder;
  encode_digest(encoder, basis);
  encoder.string(key ? key : "qualification_accounting.unspecified");
  return domain_digest(check_evidence_tag, encoder.bytes());
}

product_status_or<qualification_mesh_topology_reconstruction>
canonicalize_topology_reconstruction(
    qualification_mesh_topology_reconstruction value) {
  if (value.schema != qualification_accounting_schema_version ||
      zero(value.incidence_digest))
    return accounting_error(product_error_code::qualification_policy_violation,
                            "qualification_accounting.topology_contract");
  canonical_encoder encoder;
  encode_topology(encoder, value, false);
  const auto computed = domain_digest(topology_tag, encoder.bytes());
  if (!zero(value.reconstruction_digest) &&
      value.reconstruction_digest != computed)
    return accounting_error(product_error_code::stale_binding,
                            "qualification_accounting.topology_digest");
  value.reconstruction_digest = computed;
  return value;
}
} // namespace qualification_accounting_detail

const char *qualification_check_kind_token(
    qualification_check_kind value) noexcept {
  static const char *const names[]{
      "product_contract",       "exact_result_binding",
      "representation_semantics", "strict_reingestion",
      "independent_topology",   "certificate_replay",
      "guarded_occupancy",      "embedding",
      "orientation",            "shell_nesting",
      "attribute_mapping",      "approximation_bounds",
      "chain_reingestion"};
  return known(value) ? names[ordinal(value)] : "invalid";
}

const char *qualification_false_success_kind_token(
    qualification_false_success_kind value) noexcept {
  static const char *const names[]{
      "semantic_mislabeling",
      "stale_exact_result_binding",
      "strict_reingestion_rejected",
      "incorrect_topology",
      "certificate_replay_failed",
      "incorrect_occupancy",
      "incorrect_embedding",
      "incorrect_orientation",
      "incorrect_shell_nesting",
      "incorrect_attribute_mapping",
      "approximation_bound_violation"};
  return known(value) ? names[ordinal(value)] : "invalid";
}

product_status_or<qualification_verification_check>
make_qualification_verification_check(qualification_verification_check value) {
  if (!known(value.kind) || !known(value.state) || !text(value.message_key) ||
      zero(value.evidence_digest))
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.check_contract");
  return value;
}

product_status_or<qualification_guarded_probe_observation>
make_qualification_guarded_probe_observation(
    qualification_guarded_probe_observation value) {
  if (value.schema != qualification_accounting_schema_version ||
      !text(value.identifier) || zero(value.point_digest))
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.probe_contract");
  canonical_encoder encoder;
  encode_probe(encoder, value, false);
  const auto computed = domain_digest(probe_tag, encoder.bytes());
  if (!zero(value.probe_digest) && value.probe_digest != computed)
    return qualification_accounting_detail::accounting_error(
        product_error_code::stale_binding,
        "qualification_accounting.probe_digest");
  value.probe_digest = computed;
  return value;
}

product_status_or<qualification_success_verification>
make_qualification_success_verification(
    qualification_success_verification value) {
  if (value.schema != qualification_accounting_schema_version ||
      value.checker_version != qualification_accounting_checker_version ||
      !known(value.representation) || zero(value.exact_result_digest) ||
      zero(value.output_digest) ||
      (value.mesh_published &&
       value.representation == result_representation::exact_stratified) ||
      (!value.mesh_published &&
       value.representation != result_representation::exact_stratified) ||
      (value.chain_reingestion_required && !value.mesh_published))
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.verification_contract");

  if (value.mesh_published) {
    auto topology = qualification_accounting_detail::
        canonicalize_topology_reconstruction(std::move(value.topology));
    if (!topology.has_value())
      return topology.error();
    value.topology = std::move(topology.value());
  }
  for (auto &probe : value.probes) {
    auto made = make_qualification_guarded_probe_observation(std::move(probe));
    if (!made.has_value())
      return made.error();
    probe = std::move(made.value());
  }
  std::sort(value.probes.begin(), value.probes.end(), probe_less);
  if (std::adjacent_find(value.probes.begin(), value.probes.end(),
                         [](const auto &a, const auto &b) {
                           return a.identifier == b.identifier;
                         }) != value.probes.end())
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.duplicate_probe");
  for (auto &check : value.checks) {
    auto made = make_qualification_verification_check(std::move(check));
    if (!made.has_value())
      return made.error();
    check = std::move(made.value());
  }
  std::sort(value.checks.begin(), value.checks.end(), check_less);
  if (std::adjacent_find(value.checks.begin(), value.checks.end(),
                         [](const auto &a, const auto &b) {
                           return a.kind == b.kind;
                         }) != value.checks.end())
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.duplicate_check");
  for (unsigned raw = ordinal(qualification_check_kind::product_contract);
       raw <= ordinal(qualification_check_kind::chain_reingestion); ++raw) {
    const auto kind = static_cast<qualification_check_kind>(raw);
    if (required_check(value, kind) && !find_check(value, kind))
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.missing_required_check");
  }
  canonical_encoder encoder;
  encode_verification(encoder, value, false);
  const auto computed = domain_digest(verification_tag, encoder.bytes());
  if (!zero(value.verification_digest) && value.verification_digest != computed)
    return qualification_accounting_detail::accounting_error(
        product_error_code::stale_binding,
        "qualification_accounting.verification_digest");
  value.verification_digest = computed;
  return value;
}

product_status_or<bool> validate_qualification_success_verification(
    const qualification_success_verification &value) noexcept {
  try {
    if (!strictly_sorted(value.checks, check_less) ||
        !strictly_sorted(value.probes, probe_less))
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.verification_order");
    auto made = make_qualification_success_verification(value);
    if (!made.has_value())
      return made.error();
    if (made.value().verification_digest != value.verification_digest ||
        made.value().checks.size() != value.checks.size() ||
        made.value().probes.size() != value.probes.size())
      return qualification_accounting_detail::accounting_error(
          product_error_code::stale_binding,
          "qualification_accounting.verification_not_canonical");
    return true;
  } catch (...) {
    return qualification_accounting_detail::accounting_error(
        product_error_code::internal_invariant_error,
        "qualification_accounting.verification_exception");
  }
}

product_status_or<qualification_case_observation>
make_qualification_case_observation(qualification_case_observation value) {
  if (value.schema != qualification_accounting_schema_version ||
      !text(value.identifier) || zero(value.case_digest) ||
      !valid_dimension(value.dimensions) || value.expected_outcomes.empty() ||
      (value.published_success != static_cast<bool>(value.success_verification)) ||
      (value.published_success && value.failure) ||
      (!value.published_success && !value.failure &&
       !value.infrastructure_failure && !value.backend_disagreement &&
       !value.verifier_disagreement && !value.nondeterministic &&
       !value.timeout_or_resource_limit))
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.observation_contract");
  for (const auto outcome : value.expected_outcomes)
    if (!known(outcome))
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.expected_outcome");
  std::sort(value.expected_outcomes.begin(), value.expected_outcomes.end(),
            [](auto a, auto b) { return ordinal(a) < ordinal(b); });
  if (std::adjacent_find(value.expected_outcomes.begin(),
                         value.expected_outcomes.end()) !=
      value.expected_outcomes.end())
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.duplicate_expected_outcome");
  for (const auto code : value.expected_failure_codes)
    if (!known(code))
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.expected_failure_code");
  std::sort(value.expected_failure_codes.begin(),
            value.expected_failure_codes.end(),
            [](auto a, auto b) { return ordinal(a) < ordinal(b); });
  if (std::adjacent_find(value.expected_failure_codes.begin(),
                         value.expected_failure_codes.end()) !=
      value.expected_failure_codes.end())
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.duplicate_expected_failure");
  if (value.failure && !valid_error(*value.failure))
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.failure_contract");
  if (value.success_verification) {
    auto made =
        make_qualification_success_verification(std::move(*value.success_verification));
    if (!made.has_value())
      return made.error();
    if (made.value().representation != value.dimensions.representation)
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.dimension_representation_mismatch");
    value.success_verification = std::move(made.value());
  }
  canonical_encoder encoder;
  encode_observation(encoder, value, false);
  const auto computed = domain_digest(observation_tag, encoder.bytes());
  if (!zero(value.observation_digest) && value.observation_digest != computed)
    return qualification_accounting_detail::accounting_error(
        product_error_code::stale_binding,
        "qualification_accounting.observation_digest");
  value.observation_digest = computed;
  return value;
}

product_status_or<bool> validate_qualification_case_observation(
    const qualification_case_observation &value) noexcept {
  try {
    if (value.success_verification) {
      auto nested = validate_qualification_success_verification(
          *value.success_verification);
      if (!nested.has_value())
        return nested.error();
    }
    auto made = make_qualification_case_observation(value);
    if (!made.has_value())
      return made.error();
    if (made.value().observation_digest != value.observation_digest ||
        made.value().expected_outcomes != value.expected_outcomes ||
        made.value().expected_failure_codes != value.expected_failure_codes)
      return qualification_accounting_detail::accounting_error(
          product_error_code::stale_binding,
          "qualification_accounting.observation_not_canonical");
    return true;
  } catch (...) {
    return qualification_accounting_detail::accounting_error(
        product_error_code::internal_invariant_error,
        "qualification_accounting.observation_exception");
  }
}

product_status_or<qualification_case_accounting>
account_qualification_case(qualification_case_observation observation) {
  auto canonical = make_qualification_case_observation(std::move(observation));
  if (!canonical.has_value())
    return canonical.error();

  qualification_case_accounting result;
  result.observation = std::move(canonical.value());
  if (result.observation.published_success) {
    const auto &verification = *result.observation.success_verification;
    bool incomplete = result.observation.verifier_disagreement;
    for (const auto &check : verification.checks) {
      if (!required_check(verification, check.kind))
        continue;
      if (check.state == qualification_check_state::failed) {
        const auto reason = false_success_reason(check.kind);
        if (reason)
          result.false_success_reasons.push_back(*reason);
        else
          incomplete = true;
      } else if (check.state == qualification_check_state::not_run) {
        incomplete = true;
      }
    }
    std::sort(result.false_success_reasons.begin(),
              result.false_success_reasons.end(),
              [](auto a, auto b) { return ordinal(a) < ordinal(b); });
    result.false_success_reasons.erase(
        std::unique(result.false_success_reasons.begin(),
                    result.false_success_reasons.end()),
        result.false_success_reasons.end());
    if (!result.false_success_reasons.empty())
      result.outcome = qualification_outcome::false_success;
    else if (result.observation.infrastructure_failure)
      result.outcome = qualification_outcome::infrastructure_failure;
    else if (result.observation.nondeterministic)
      result.outcome = qualification_outcome::nondeterministic_outcome;
    else if (incomplete)
      result.outcome = qualification_outcome::verifier_disagreement;
    else if (result.observation.backend_disagreement)
      result.outcome = qualification_outcome::backend_disagreement;
    else
      result.outcome = successful_outcome(verification.representation);

    if ((result.outcome == qualification_outcome::verified_exact_success ||
         result.outcome ==
             qualification_outcome::verified_certified_approximate_success) &&
        !expected_outcome(result.observation, result.outcome))
      result.outcome = qualification_outcome::verifier_disagreement;
  } else if (result.observation.infrastructure_failure) {
    result.outcome = qualification_outcome::infrastructure_failure;
  } else if (result.observation.nondeterministic) {
    result.outcome = qualification_outcome::nondeterministic_outcome;
  } else if (result.observation.verifier_disagreement) {
    result.outcome = qualification_outcome::verifier_disagreement;
  } else if (result.observation.backend_disagreement) {
    result.outcome = qualification_outcome::backend_disagreement;
  } else if (result.observation.timeout_or_resource_limit ||
             (result.observation.failure &&
              result.observation.failure->code ==
                  product_error_code::resource_limit)) {
    result.outcome = qualification_outcome::timeout_or_resource_limit;
  } else if (result.observation.failure &&
             expected_failure(result.observation,
                              result.observation.failure->code)) {
    result.outcome = qualification_outcome::expected_typed_failure;
  } else if (result.observation.failure) {
    result.outcome = qualification_outcome::unexpected_typed_failure;
  } else {
    result.outcome = qualification_outcome::infrastructure_failure;
  }
  result.blocking = blocking_outcome(result.outcome);
  result.safe_failure = safe_failure_outcome(result.outcome);

  canonical_encoder digest_payload;
  encode_accounting(digest_payload, result, false);
  result.accounting_digest = domain_digest(accounting_tag, digest_payload.bytes());
  canonical_encoder record_payload;
  encode_accounting(record_payload, result, true);
  result.canonical_bytes = framed(accounting_record_tag, result.schema,
                                  record_payload.bytes());
  return result;
}

product_status_or<bool> validate_qualification_case_accounting(
    const qualification_case_accounting &value) noexcept {
  try {
    auto observation_valid =
        validate_qualification_case_observation(value.observation);
    if (!observation_valid.has_value())
      return observation_valid.error();
    if (value.schema != qualification_accounting_schema_version ||
        !known(value.outcome) ||
        !std::is_sorted(value.false_success_reasons.begin(),
                        value.false_success_reasons.end(),
                        [](auto a, auto b) { return ordinal(a) < ordinal(b); }) ||
        std::adjacent_find(value.false_success_reasons.begin(),
                           value.false_success_reasons.end()) !=
            value.false_success_reasons.end())
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.record_contract");
    for (const auto reason : value.false_success_reasons)
      if (!known(reason))
        return qualification_accounting_detail::accounting_error(
            product_error_code::qualification_policy_violation,
            "qualification_accounting.record_reason");
    auto remade = account_qualification_case(value.observation);
    if (!remade.has_value())
      return remade.error();
    if (remade.value().outcome != value.outcome ||
        remade.value().blocking != value.blocking ||
        remade.value().safe_failure != value.safe_failure ||
        remade.value().false_success_reasons != value.false_success_reasons ||
        remade.value().canonical_bytes != value.canonical_bytes ||
        remade.value().accounting_digest != value.accounting_digest)
      return qualification_accounting_detail::accounting_error(
          product_error_code::stale_binding,
          "qualification_accounting.record_binding");
    return true;
  } catch (...) {
    return qualification_accounting_detail::accounting_error(
        product_error_code::internal_invariant_error,
        "qualification_accounting.record_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_case_accounting(
    const qualification_case_accounting &value) {
  auto valid = validate_qualification_case_accounting(value);
  if (!valid.has_value())
    return valid.error();
  return value.canonical_bytes;
}

product_status_or<qualification_accounting_campaign>
make_qualification_accounting_campaign(
    std::string identifier, std::vector<qualification_case_accounting> records,
    bool complete) {
  if (!text(identifier) || records.empty())
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.campaign_identity");
  for (const auto &record : records) {
    auto valid = validate_qualification_case_accounting(record);
    if (!valid.has_value())
      return valid.error();
  }
  std::sort(records.begin(), records.end(), accounting_less);
  if (std::adjacent_find(records.begin(), records.end(),
                         [](const auto &a, const auto &b) {
                           return a.observation.identifier ==
                                      b.observation.identifier &&
                                  a.observation.case_digest ==
                                      b.observation.case_digest;
                         }) != records.end())
    return qualification_accounting_detail::accounting_error(
        product_error_code::qualification_policy_violation,
        "qualification_accounting.duplicate_case");

  qualification_accounting_campaign campaign;
  campaign.identifier = std::move(identifier);
  campaign.records = std::move(records);
  campaign.complete = complete;
  std::map<std::pair<qualification_dimension_key, qualification_outcome>,
           std::uint64_t>
      counts;
  for (const auto &record : campaign.records) {
    if (!add(campaign.total_case_count, 1) ||
        (record.safe_failure && !add(campaign.safe_failure_count, 1)) ||
        (record.outcome == qualification_outcome::false_success &&
         !add(campaign.false_success_count, 1)) ||
        (record.blocking && !add(campaign.blocking_issue_count, 1)))
      return qualification_accounting_detail::accounting_error(
          product_error_code::resource_limit,
          "qualification_accounting.campaign_count_overflow");
    auto &count = counts[{record.observation.dimensions, record.outcome}];
    if (!add(count, 1))
      return qualification_accounting_detail::accounting_error(
          product_error_code::resource_limit,
          "qualification_accounting.dimension_count_overflow");
  }
  for (const auto &entry : counts)
    campaign.counts.push_back(
        {entry.first.first, entry.first.second, entry.second});
  std::sort(campaign.counts.begin(), campaign.counts.end(), count_less);

  canonical_encoder payload;
  payload.u16(campaign.schema);
  payload.u32(campaign.checker_version);
  payload.string(campaign.identifier);
  payload.u64(campaign.records.size());
  for (const auto &record : campaign.records)
    encode_digest(payload, record.accounting_digest);
  payload.u64(campaign.counts.size());
  for (const auto &count : campaign.counts) {
    encode_dimension(payload, count.dimensions);
    payload.byte(static_cast<std::uint8_t>(count.outcome));
    payload.u64(count.count);
  }
  payload.u64(campaign.total_case_count);
  payload.u64(campaign.safe_failure_count);
  payload.u64(campaign.false_success_count);
  payload.u64(campaign.blocking_issue_count);
  payload.boolean(campaign.complete);
  campaign.campaign_digest = domain_digest(campaign_tag, payload.bytes());
  canonical_encoder record_payload;
  record_payload.raw(payload.bytes().data(), payload.bytes().size());
  encode_digest(record_payload, campaign.campaign_digest);
  campaign.canonical_bytes =
      framed(campaign_record_tag, campaign.schema, record_payload.bytes());
  return campaign;
}

product_status_or<bool> validate_qualification_accounting_campaign(
    const qualification_accounting_campaign &value) noexcept {
  try {
    if (value.schema != qualification_accounting_schema_version ||
        value.checker_version != qualification_accounting_checker_version ||
        !text(value.identifier) || value.records.empty() ||
        !strictly_sorted(value.records, accounting_less) ||
        !strictly_sorted(value.counts, count_less))
      return qualification_accounting_detail::accounting_error(
          product_error_code::qualification_policy_violation,
          "qualification_accounting.campaign_contract");
    for (const auto &record : value.records) {
      auto valid = validate_qualification_case_accounting(record);
      if (!valid.has_value())
        return valid.error();
    }
    auto remade = make_qualification_accounting_campaign(
        value.identifier, value.records, value.complete);
    if (!remade.has_value())
      return remade.error();
    if (!counts_equal(remade.value().counts, value.counts) ||
        remade.value().total_case_count != value.total_case_count ||
        remade.value().safe_failure_count != value.safe_failure_count ||
        remade.value().false_success_count != value.false_success_count ||
        remade.value().blocking_issue_count != value.blocking_issue_count ||
        remade.value().campaign_digest != value.campaign_digest ||
        remade.value().canonical_bytes != value.canonical_bytes)
      return qualification_accounting_detail::accounting_error(
          product_error_code::stale_binding,
          "qualification_accounting.campaign_binding");
    return true;
  } catch (...) {
    return qualification_accounting_detail::accounting_error(
        product_error_code::internal_invariant_error,
        "qualification_accounting.campaign_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_accounting_campaign(
    const qualification_accounting_campaign &value) {
  auto valid = validate_qualification_accounting_campaign(value);
  if (!valid.has_value())
    return valid.error();
  return value.canonical_bytes;
}

bool qualification_false_success_gate_passes(
    const qualification_accounting_campaign &value) noexcept {
  return validate_qualification_accounting_campaign(value).has_value() &&
         value.complete && value.false_success_count == 0 &&
         value.blocking_issue_count == 0;
}

product_status_or<qualification_result_summary>
make_qualification_result_summary_from_accounting(
    const qualification_accounting_campaign &campaign, digest manifest_digest,
    std::string run_identifier, std::string repository_commit,
    std::string started_utc, std::string finished_utc,
    std::vector<qualification_artifact_reference> artifacts) {
  auto valid = validate_qualification_accounting_campaign(campaign);
  if (!valid.has_value())
    return valid.error();
  qualification_result_summary summary;
  summary.manifest_digest = manifest_digest;
  summary.run_identifier = std::move(run_identifier);
  summary.repository_commit = std::move(repository_commit);
  summary.started_utc = std::move(started_utc);
  summary.finished_utc = std::move(finished_utc);
  summary.complete = campaign.complete;
  summary.counts = campaign.counts;
  summary.artifacts = std::move(artifacts);
  summary.blocking_issue_count = campaign.blocking_issue_count;
  summary.false_success_count = campaign.false_success_count;
  return make_qualification_result_summary(std::move(summary));
}

} // namespace mesh_boolean
} // namespace ygor
