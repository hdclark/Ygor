#include "YgorMeshesBooleanQualification.h"

#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> manifest_record_tag{
    {'Y', 'G', 'B', 'Q', 'M', 'F', '0', '1'}};
constexpr std::array<char, 8> manifest_material_tag{
    {'Y', 'G', 'B', 'Q', 'M', 'T', '0', '1'}};
constexpr std::array<char, 8> manifest_digest_tag{
    {'Y', 'G', 'B', 'Q', 'M', 'D', '0', '1'}};
constexpr std::array<char, 8> summary_record_tag{
    {'Y', 'G', 'B', 'Q', 'R', 'S', '0', '1'}};
constexpr std::array<char, 8> summary_digest_tag{
    {'Y', 'G', 'B', 'Q', 'R', 'D', '0', '1'}};
constexpr std::array<char, 8> report_record_tag{
    {'Y', 'G', 'B', 'Q', 'R', 'P', '0', '1'}};
constexpr std::array<char, 8> report_digest_tag{
    {'Y', 'G', 'B', 'Q', 'P', 'D', '0', '1'}};
constexpr std::array<char, 8> markdown_digest_tag{
    {'Y', 'G', 'B', 'Q', 'M', 'K', '0', '1'}};
constexpr std::array<char, 8> compatibility_change_tag{
    {'Y', 'G', 'B', 'Q', 'C', 'H', '0', '1'}};

product_error qerror(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

bool digest_zero(const digest &d) noexcept { return d == digest{}; }

bool valid_text(const std::string &s, bool allow_empty = false) noexcept {
  if ((!allow_empty && s.empty()) || s.size() > 1024U * 1024U)
    return false;
  return std::find(s.begin(), s.end(), '\0') == s.end();
}

bool known(backend_id v) noexcept {
  return static_cast<unsigned>(v) <=
             static_cast<unsigned>(backend_id::independent_axis_aligned_box_v1) &&
         static_cast<unsigned>(v) != 0;
}
bool known(result_representation v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(
             result_representation::certified_approximate_mesh);
}
bool known(product_realization_semantics v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(
             product_realization_semantics::certified_approximate_embedding_v1);
}
bool known(preparation_mode v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(preparation_mode::normalized);
}
bool known(model_unit v) noexcept {
  return static_cast<unsigned>(v) <= static_cast<unsigned>(model_unit::foot);
}
bool known(coordinate_tag v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(coordinate_tag::binary64);
}
bool known(index_tag v) noexcept {
  return static_cast<unsigned>(v) <= static_cast<unsigned>(index_tag::uint64);
}
bool known(operation v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(operation::symmetric_difference);
}
bool known(qualification_outcome v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_outcome::infrastructure_failure);
}
bool known(qualification_architecture v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_architecture::other_64_bit);
}
bool known(qualification_standard_library v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_standard_library::other);
}
bool known(qualification_floating_point_mode v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_floating_point_mode::
                                   strict_controlled_rounding_matrix);
}
bool known(qualification_corpus_source v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_corpus_source::minimized_regression);
}
bool known(qualification_redistribution v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_redistribution::private_digest_only);
}
bool known(qualification_threshold_relation v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_threshold_relation::equal);
}
bool known(qualification_report_decision v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_report_decision::revoked);
}
bool known(qualification_report_section_kind v) noexcept {
  return static_cast<unsigned>(v) <
         static_cast<unsigned>(qualification_report_section_kind::count);
}
bool known(qualification_artifact_kind v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(qualification_artifact_kind::other);
}

void encode_digest(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}

void encode_backend_version(canonical_encoder &e, const backend_version &v) {
  e.u16(v.major);
  e.u16(v.minor);
  e.u16(v.patch);
}

void encode_backend_identity(canonical_encoder &e, const backend_identity &v) {
  e.u16(v.schema);
  e.u16(static_cast<std::uint16_t>(v.id));
  encode_backend_version(e, v.adapter_version);
  e.string(v.build_identifier);
  e.u16(v.capabilities.schema);
  e.u64(v.capabilities.bits);
  encode_digest(e, v.capability_digest);
  e.byte(static_cast<std::uint8_t>(v.maturity));
}

void encode_backend(canonical_encoder &e,
                    const qualification_backend_binding &v) {
  encode_backend_identity(e, v.identity);
  e.string(v.implementation_name);
  e.string(v.implementation_version);
  encode_digest(e, v.adapter_source_digest);
  encode_digest(e, v.adapter_binary_digest);
  encode_digest(e, v.dependency_digest);
  e.boolean(v.diagnostic_only);
}

void encode_result_mode(canonical_encoder &e,
                        const qualification_result_mode_binding &v) {
  e.byte(static_cast<std::uint8_t>(v.representation));
  e.byte(static_cast<std::uint8_t>(v.semantics));
  encode_digest(e, v.policy_digest);
}

void encode_preparation(canonical_encoder &e,
                        const qualification_preparation_binding &v) {
  e.byte(static_cast<std::uint8_t>(v.mode));
  e.byte(static_cast<std::uint8_t>(v.tolerance_unit));
  e.u64(v.model_tolerance_binary64_bits);
  encode_digest(e, v.policy_digest);
}

void encode_type(canonical_encoder &e, const qualification_type_binding &v) {
  e.byte(static_cast<std::uint8_t>(v.coordinate));
  e.byte(static_cast<std::uint8_t>(v.index));
}

void encode_toolchain(canonical_encoder &e,
                      const qualification_toolchain_binding &v) {
  e.string(v.identifier);
  e.string(v.compiler_name);
  e.string(v.compiler_version);
  e.byte(static_cast<std::uint8_t>(v.standard_library));
  e.string(v.standard_library_version);
  e.byte(static_cast<std::uint8_t>(v.architecture));
  e.string(v.operating_system);
  e.string(v.operating_system_version);
  e.string(v.target_triple);
  e.string(v.build_type);
  e.byte(static_cast<std::uint8_t>(v.floating_point_mode));
  e.u64(v.compile_flags.size());
  for (const auto &flag : v.compile_flags)
    e.string(flag);
  encode_digest(e, v.environment_digest);
}

void encode_verifier(canonical_encoder &e,
                     const qualification_verifier_binding &v) {
  e.string(v.identifier);
  e.string(v.version);
  encode_digest(e, v.implementation_digest);
  e.boolean(v.mandatory);
}

void encode_corpus(canonical_encoder &e,
                   const qualification_corpus_binding &v) {
  e.string(v.identifier);
  e.string(v.version);
  e.byte(static_cast<std::uint8_t>(v.source));
  e.byte(static_cast<std::uint8_t>(v.redistribution));
  e.string(v.license_or_provenance);
  e.u64(v.case_count);
  encode_digest(e, v.corpus_digest);
  encode_digest(e, v.category_coverage_digest);
  encode_digest(e, v.expected_outcome_digest);
}

void encode_generator(canonical_encoder &e,
                      const qualification_generator_binding &v) {
  e.string(v.identifier);
  e.string(v.version);
  encode_digest(e, v.implementation_digest);
  e.u64(v.first_seed);
  e.u64(v.last_seed);
  encode_digest(e, v.parameter_range_digest);
}

void encode_fuzz(canonical_encoder &e,
                 const qualification_fuzz_campaign_binding &v) {
  e.string(v.identifier);
  e.string(v.engine);
  e.string(v.engine_version);
  encode_digest(e, v.dictionary_digest);
  encode_digest(e, v.mutator_digest);
  encode_digest(e, v.seed_set_digest);
  e.u64(v.duration_seconds);
  e.u32(v.worker_count);
}

void encode_chain(canonical_encoder &e,
                  const qualification_chain_binding &v) {
  e.string(v.identifier);
  e.string(v.version);
  encode_digest(e, v.definition_digest);
  e.u64(v.chain_count);
  e.u32(v.minimum_steps);
  e.u32(v.maximum_steps);
}

void encode_resource(canonical_encoder &e,
                     const qualification_resource_binding &v) {
  e.string(v.identifier);
  encode_digest(e, v.policy_digest);
  e.u64(v.wall_timeout_milliseconds);
  e.u64(v.authoritative_byte_limit);
  e.u64(v.work_unit_limit);
  e.u64(v.cancellation_latency_limit_milliseconds);
}

void encode_performance(canonical_encoder &e,
                        const qualification_performance_protocol &v) {
  e.string(v.identifier);
  e.string(v.version);
  e.string(v.hardware_identifier);
  encode_digest(e, v.hardware_digest);
  encode_digest(e, v.measurement_protocol_digest);
  e.u32(v.warmup_runs);
  e.u32(v.measured_runs);
  e.boolean(v.controlled_exclusive_host);
}

void encode_threshold(canonical_encoder &e,
                      const qualification_threshold &v) {
  e.string(v.metric);
  e.byte(static_cast<std::uint8_t>(v.relation));
  e.u64(v.numerator);
  e.u64(v.denominator);
  e.string(v.unit);
  e.boolean(v.blocking);
}

void encode_compatibility(canonical_encoder &e,
                          const qualification_compatibility_review &v) {
  encode_digest(e, v.prior_manifest_digest);
  encode_digest(e, v.reviewed_change_digest);
  e.string(v.reviewer);
  e.string(v.rationale);
  encode_digest(e, v.evidence_digest);
  e.boolean(v.approved);
}

template <class V, class F>
void encode_vector(canonical_encoder &e, const std::vector<V> &values, F f) {
  e.u64(values.size());
  for (const auto &value : values)
    f(e, value);
}

void encode_string_vector(canonical_encoder &e,
                          const std::vector<std::string> &values) {
  e.u64(values.size());
  for (const auto &value : values)
    e.string(value);
}

void encode_manifest_material(canonical_encoder &e,
                              const qualification_campaign_manifest &m) {
  e.u16(m.schema);
  e.string(m.identifier);
  e.string(m.workload_profile);
  e.string(m.repository_commit);
  encode_digest(e, m.repository_tree_digest);
  e.boolean(m.repository_dirty);
  e.string(m.created_utc);
  encode_vector(e, m.backends, encode_backend);
  encode_vector(e, m.result_modes, encode_result_mode);
  encode_vector(e, m.preparation_policies, encode_preparation);
  encode_vector(e, m.type_specializations, encode_type);
  encode_vector(e, m.toolchains, encode_toolchain);
  encode_vector(e, m.verifiers, encode_verifier);
  encode_vector(e, m.corpora, encode_corpus);
  encode_vector(e, m.generators, encode_generator);
  encode_vector(e, m.fuzz_campaigns, encode_fuzz);
  encode_vector(e, m.operation_chains, encode_chain);
  encode_vector(e, m.resource_policies, encode_resource);
  encode_performance(e, m.performance);
  encode_vector(e, m.thresholds, encode_threshold);
  encode_string_vector(e, m.exclusions);
  encode_string_vector(e, m.known_limitations);
}

void encode_manifest_digest_payload(canonical_encoder &e,
                                    const qualification_campaign_manifest &m) {
  encode_manifest_material(e, m);
  encode_vector(e, m.compatibility_reviews, encode_compatibility);
  encode_digest(e, m.material_binding_digest);
}

std::vector<std::uint8_t>
record_bytes(const std::array<char, 8> &tag, std::uint16_t schema,
             const std::vector<std::uint8_t> &payload) {
  canonical_encoder e;
  e.raw(reinterpret_cast<const std::uint8_t *>(tag.data()), tag.size());
  e.u16(schema);
  e.u64(payload.size());
  e.raw(payload.data(), payload.size());
  return e.bytes();
}

class reader {
  const std::vector<std::uint8_t> &bytes_;
  const qualification_decode_limits &limits_;
  std::size_t at_ = 0;
  std::uint64_t total_strings_ = 0;

  void require(std::size_t n) {
    if (n > bytes_.size() - at_)
      throw std::runtime_error("truncated");
  }

public:
  reader(const std::vector<std::uint8_t> &bytes,
         const qualification_decode_limits &limits)
      : bytes_(bytes), limits_(limits) {}

  std::size_t remaining() const noexcept { return bytes_.size() - at_; }
  std::uint8_t byte() {
    require(1);
    return bytes_[at_++];
  }
  bool boolean() {
    const auto v = byte();
    if (v > 1)
      throw std::runtime_error("boolean");
    return v != 0;
  }
  std::uint16_t u16() {
    require(2);
    const auto value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes_[at_]) << 8) |
        static_cast<std::uint16_t>(bytes_[at_ + 1]));
    at_ += 2;
    return value;
  }
  std::uint32_t u32() {
    std::uint32_t value = 0;
    for (int i = 0; i != 4; ++i)
      value = (value << 8) | byte();
    return value;
  }
  std::uint64_t u64() {
    std::uint64_t value = 0;
    for (int i = 0; i != 8; ++i)
      value = (value << 8) | byte();
    return value;
  }
  std::uint64_t count() {
    const auto value = u64();
    if (value > limits_.max_vector_elements)
      throw std::runtime_error("vector_limit");
    return value;
  }
  std::string string() {
    const auto n = u64();
    if (n > limits_.max_string_bytes || n > remaining())
      throw std::runtime_error("string_limit");
    if (++total_strings_ > limits_.max_total_strings)
      throw std::runtime_error("total_string_limit");
    std::string result(
        reinterpret_cast<const char *>(bytes_.data() + at_),
        static_cast<std::size_t>(n));
    at_ += static_cast<std::size_t>(n);
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
  std::vector<std::uint8_t> raw(std::size_t n) {
    require(n);
    std::vector<std::uint8_t> result(
        bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
        bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + n));
    at_ += n;
    return result;
  }
};

template <class E> E read_enum(reader &r, bool (*is_known)(E) noexcept) {
  const auto value = static_cast<E>(r.byte());
  if (!is_known(value))
    throw std::runtime_error("unknown_enum");
  return value;
}

std::vector<std::uint8_t>
extract_payload(const std::vector<std::uint8_t> &bytes,
                const std::array<char, 8> &tag, std::uint16_t schema,
                const qualification_decode_limits &limits) {
  if (bytes.size() > limits.max_record_bytes)
    throw std::runtime_error("record_limit");
  reader r(bytes, limits);
  const auto observed_tag = r.raw(tag.size());
  if (!std::equal(observed_tag.begin(), observed_tag.end(), tag.begin()))
    throw std::runtime_error("domain_tag");
  if (r.u16() != schema)
    throw std::runtime_error("schema");
  const auto n = r.u64();
  if (n != r.remaining() || n > limits.max_record_bytes)
    throw std::runtime_error("payload_length");
  return r.raw(static_cast<std::size_t>(n));
}

backend_identity read_backend_identity(reader &r) {
  backend_identity result;
  result.schema = r.u16();
  const auto raw_id = static_cast<backend_id>(r.u16());
  if (!known(raw_id))
    throw std::runtime_error("backend_id");
  result.id = raw_id;
  result.adapter_version.major = r.u16();
  result.adapter_version.minor = r.u16();
  result.adapter_version.patch = r.u16();
  result.build_identifier = r.string();
  result.capabilities.schema = r.u16();
  result.capabilities.bits = r.u64();
  result.capability_digest = r.digest_value();
  const auto maturity = static_cast<backend_maturity>(r.byte());
  if (static_cast<unsigned>(maturity) >
      static_cast<unsigned>(backend_maturity::deprecated))
    throw std::runtime_error("backend_maturity");
  result.maturity = maturity;
  return result;
}

qualification_backend_binding read_backend(reader &r) {
  qualification_backend_binding result;
  result.identity = read_backend_identity(r);
  result.implementation_name = r.string();
  result.implementation_version = r.string();
  result.adapter_source_digest = r.digest_value();
  result.adapter_binary_digest = r.digest_value();
  result.dependency_digest = r.digest_value();
  result.diagnostic_only = r.boolean();
  return result;
}

qualification_result_mode_binding read_result_mode(reader &r) {
  qualification_result_mode_binding result;
  result.representation = read_enum<result_representation>(r, known);
  result.semantics = read_enum<product_realization_semantics>(r, known);
  result.policy_digest = r.digest_value();
  return result;
}

qualification_preparation_binding read_preparation(reader &r) {
  qualification_preparation_binding result;
  result.mode = read_enum<preparation_mode>(r, known);
  result.tolerance_unit = read_enum<model_unit>(r, known);
  result.model_tolerance_binary64_bits = r.u64();
  result.policy_digest = r.digest_value();
  return result;
}

qualification_type_binding read_type(reader &r) {
  qualification_type_binding result;
  result.coordinate = read_enum<coordinate_tag>(r, known);
  result.index = read_enum<index_tag>(r, known);
  return result;
}

qualification_toolchain_binding read_toolchain(reader &r) {
  qualification_toolchain_binding result;
  result.identifier = r.string();
  result.compiler_name = r.string();
  result.compiler_version = r.string();
  result.standard_library =
      read_enum<qualification_standard_library>(r, known);
  result.standard_library_version = r.string();
  result.architecture = read_enum<qualification_architecture>(r, known);
  result.operating_system = r.string();
  result.operating_system_version = r.string();
  result.target_triple = r.string();
  result.build_type = r.string();
  result.floating_point_mode =
      read_enum<qualification_floating_point_mode>(r, known);
  const auto n = r.count();
  result.compile_flags.reserve(static_cast<std::size_t>(n));
  for (std::uint64_t i = 0; i != n; ++i)
    result.compile_flags.push_back(r.string());
  result.environment_digest = r.digest_value();
  return result;
}

qualification_verifier_binding read_verifier(reader &r) {
  qualification_verifier_binding result;
  result.identifier = r.string();
  result.version = r.string();
  result.implementation_digest = r.digest_value();
  result.mandatory = r.boolean();
  return result;
}

qualification_corpus_binding read_corpus(reader &r) {
  qualification_corpus_binding result;
  result.identifier = r.string();
  result.version = r.string();
  result.source = read_enum<qualification_corpus_source>(r, known);
  result.redistribution = read_enum<qualification_redistribution>(r, known);
  result.license_or_provenance = r.string();
  result.case_count = r.u64();
  result.corpus_digest = r.digest_value();
  result.category_coverage_digest = r.digest_value();
  result.expected_outcome_digest = r.digest_value();
  return result;
}

qualification_generator_binding read_generator(reader &r) {
  qualification_generator_binding result;
  result.identifier = r.string();
  result.version = r.string();
  result.implementation_digest = r.digest_value();
  result.first_seed = r.u64();
  result.last_seed = r.u64();
  result.parameter_range_digest = r.digest_value();
  return result;
}

qualification_fuzz_campaign_binding read_fuzz(reader &r) {
  qualification_fuzz_campaign_binding result;
  result.identifier = r.string();
  result.engine = r.string();
  result.engine_version = r.string();
  result.dictionary_digest = r.digest_value();
  result.mutator_digest = r.digest_value();
  result.seed_set_digest = r.digest_value();
  result.duration_seconds = r.u64();
  result.worker_count = r.u32();
  return result;
}

qualification_chain_binding read_chain(reader &r) {
  qualification_chain_binding result;
  result.identifier = r.string();
  result.version = r.string();
  result.definition_digest = r.digest_value();
  result.chain_count = r.u64();
  result.minimum_steps = r.u32();
  result.maximum_steps = r.u32();
  return result;
}

qualification_resource_binding read_resource(reader &r) {
  qualification_resource_binding result;
  result.identifier = r.string();
  result.policy_digest = r.digest_value();
  result.wall_timeout_milliseconds = r.u64();
  result.authoritative_byte_limit = r.u64();
  result.work_unit_limit = r.u64();
  result.cancellation_latency_limit_milliseconds = r.u64();
  return result;
}

qualification_performance_protocol read_performance(reader &r) {
  qualification_performance_protocol result;
  result.identifier = r.string();
  result.version = r.string();
  result.hardware_identifier = r.string();
  result.hardware_digest = r.digest_value();
  result.measurement_protocol_digest = r.digest_value();
  result.warmup_runs = r.u32();
  result.measured_runs = r.u32();
  result.controlled_exclusive_host = r.boolean();
  return result;
}

qualification_threshold read_threshold(reader &r) {
  qualification_threshold result;
  result.metric = r.string();
  result.relation = read_enum<qualification_threshold_relation>(r, known);
  result.numerator = r.u64();
  result.denominator = r.u64();
  result.unit = r.string();
  result.blocking = r.boolean();
  return result;
}

qualification_compatibility_review read_compatibility(reader &r) {
  qualification_compatibility_review result;
  result.prior_manifest_digest = r.digest_value();
  result.reviewed_change_digest = r.digest_value();
  result.reviewer = r.string();
  result.rationale = r.string();
  result.evidence_digest = r.digest_value();
  result.approved = r.boolean();
  return result;
}

template <class V, class F>
std::vector<V> read_vector(reader &r, F f) {
  const auto n = r.count();
  std::vector<V> result;
  result.reserve(static_cast<std::size_t>(n));
  for (std::uint64_t i = 0; i != n; ++i)
    result.push_back(f(r));
  return result;
}

std::vector<std::string> read_string_vector(reader &r) {
  const auto n = r.count();
  std::vector<std::string> result;
  result.reserve(static_cast<std::size_t>(n));
  for (std::uint64_t i = 0; i != n; ++i)
    result.push_back(r.string());
  return result;
}

template <class V, class Less>
bool strictly_sorted(const std::vector<V> &values, Less less) noexcept {
  for (std::size_t i = 1; i < values.size(); ++i)
    if (!less(values[i - 1], values[i]))
      return false;
  return true;
}

bool backend_less(const qualification_backend_binding &a,
                  const qualification_backend_binding &b) noexcept {
  return std::tie(a.identity.id, a.identity.adapter_version.major,
                  a.identity.adapter_version.minor,
                  a.identity.adapter_version.patch,
                  a.identity.build_identifier, a.identity.capability_digest,
                  a.implementation_name, a.implementation_version,
                  a.adapter_source_digest, a.adapter_binary_digest,
                  a.dependency_digest, a.diagnostic_only) <
         std::tie(b.identity.id, b.identity.adapter_version.major,
                  b.identity.adapter_version.minor,
                  b.identity.adapter_version.patch,
                  b.identity.build_identifier, b.identity.capability_digest,
                  b.implementation_name, b.implementation_version,
                  b.adapter_source_digest, b.adapter_binary_digest,
                  b.dependency_digest, b.diagnostic_only);
}

bool result_mode_less(const qualification_result_mode_binding &a,
                      const qualification_result_mode_binding &b) noexcept {
  return std::tie(a.representation, a.semantics, a.policy_digest) <
         std::tie(b.representation, b.semantics, b.policy_digest);
}

bool preparation_less(const qualification_preparation_binding &a,
                      const qualification_preparation_binding &b) noexcept {
  return std::tie(a.mode, a.tolerance_unit,
                  a.model_tolerance_binary64_bits, a.policy_digest) <
         std::tie(b.mode, b.tolerance_unit,
                  b.model_tolerance_binary64_bits, b.policy_digest);
}

bool type_less(const qualification_type_binding &a,
               const qualification_type_binding &b) noexcept {
  return std::tie(a.coordinate, a.index) < std::tie(b.coordinate, b.index);
}

bool toolchain_less(const qualification_toolchain_binding &a,
                    const qualification_toolchain_binding &b) noexcept {
  return std::tie(a.identifier, a.compiler_name, a.compiler_version,
                  a.standard_library, a.standard_library_version,
                  a.architecture, a.operating_system,
                  a.operating_system_version, a.target_triple, a.build_type,
                  a.floating_point_mode, a.compile_flags,
                  a.environment_digest) <
         std::tie(b.identifier, b.compiler_name, b.compiler_version,
                  b.standard_library, b.standard_library_version,
                  b.architecture, b.operating_system,
                  b.operating_system_version, b.target_triple, b.build_type,
                  b.floating_point_mode, b.compile_flags,
                  b.environment_digest);
}

bool verifier_less(const qualification_verifier_binding &a,
                   const qualification_verifier_binding &b) noexcept {
  return std::tie(a.identifier, a.version, a.implementation_digest,
                  a.mandatory) <
         std::tie(b.identifier, b.version, b.implementation_digest,
                  b.mandatory);
}

bool corpus_less(const qualification_corpus_binding &a,
                 const qualification_corpus_binding &b) noexcept {
  return std::tie(a.identifier, a.version, a.source, a.redistribution,
                  a.license_or_provenance, a.case_count, a.corpus_digest,
                  a.category_coverage_digest, a.expected_outcome_digest) <
         std::tie(b.identifier, b.version, b.source, b.redistribution,
                  b.license_or_provenance, b.case_count, b.corpus_digest,
                  b.category_coverage_digest, b.expected_outcome_digest);
}

bool generator_less(const qualification_generator_binding &a,
                    const qualification_generator_binding &b) noexcept {
  return std::tie(a.identifier, a.version, a.implementation_digest,
                  a.first_seed, a.last_seed, a.parameter_range_digest) <
         std::tie(b.identifier, b.version, b.implementation_digest,
                  b.first_seed, b.last_seed, b.parameter_range_digest);
}

bool fuzz_less(const qualification_fuzz_campaign_binding &a,
               const qualification_fuzz_campaign_binding &b) noexcept {
  return std::tie(a.identifier, a.engine, a.engine_version,
                  a.dictionary_digest, a.mutator_digest, a.seed_set_digest,
                  a.duration_seconds, a.worker_count) <
         std::tie(b.identifier, b.engine, b.engine_version,
                  b.dictionary_digest, b.mutator_digest, b.seed_set_digest,
                  b.duration_seconds, b.worker_count);
}

bool chain_less(const qualification_chain_binding &a,
                const qualification_chain_binding &b) noexcept {
  return std::tie(a.identifier, a.version, a.definition_digest,
                  a.chain_count, a.minimum_steps, a.maximum_steps) <
         std::tie(b.identifier, b.version, b.definition_digest,
                  b.chain_count, b.minimum_steps, b.maximum_steps);
}

bool resource_less(const qualification_resource_binding &a,
                   const qualification_resource_binding &b) noexcept {
  return std::tie(a.identifier, a.policy_digest,
                  a.wall_timeout_milliseconds,
                  a.authoritative_byte_limit, a.work_unit_limit,
                  a.cancellation_latency_limit_milliseconds) <
         std::tie(b.identifier, b.policy_digest,
                  b.wall_timeout_milliseconds,
                  b.authoritative_byte_limit, b.work_unit_limit,
                  b.cancellation_latency_limit_milliseconds);
}

bool threshold_less(const qualification_threshold &a,
                    const qualification_threshold &b) noexcept {
  return std::tie(a.metric, a.relation, a.numerator, a.denominator, a.unit,
                  a.blocking) <
         std::tie(b.metric, b.relation, b.numerator, b.denominator, b.unit,
                  b.blocking);
}

bool compatibility_less(const qualification_compatibility_review &a,
                        const qualification_compatibility_review &b) noexcept {
  return std::tie(a.prior_manifest_digest, a.reviewed_change_digest,
                  a.reviewer, a.rationale, a.evidence_digest, a.approved) <
         std::tie(b.prior_manifest_digest, b.reviewed_change_digest,
                  b.reviewer, b.rationale, b.evidence_digest, b.approved);
}

void canonicalize_manifest(qualification_campaign_manifest &m) {
  std::sort(m.backends.begin(), m.backends.end(), backend_less);
  std::sort(m.result_modes.begin(), m.result_modes.end(), result_mode_less);
  std::sort(m.preparation_policies.begin(), m.preparation_policies.end(),
            preparation_less);
  std::sort(m.type_specializations.begin(), m.type_specializations.end(),
            type_less);
  for (auto &toolchain : m.toolchains)
    std::sort(toolchain.compile_flags.begin(), toolchain.compile_flags.end());
  std::sort(m.toolchains.begin(), m.toolchains.end(), toolchain_less);
  std::sort(m.verifiers.begin(), m.verifiers.end(), verifier_less);
  std::sort(m.corpora.begin(), m.corpora.end(), corpus_less);
  std::sort(m.generators.begin(), m.generators.end(), generator_less);
  std::sort(m.fuzz_campaigns.begin(), m.fuzz_campaigns.end(), fuzz_less);
  std::sort(m.operation_chains.begin(), m.operation_chains.end(), chain_less);
  std::sort(m.resource_policies.begin(), m.resource_policies.end(),
            resource_less);
  std::sort(m.thresholds.begin(), m.thresholds.end(), threshold_less);
  std::sort(m.exclusions.begin(), m.exclusions.end());
  std::sort(m.known_limitations.begin(), m.known_limitations.end());
  std::sort(m.compatibility_reviews.begin(), m.compatibility_reviews.end(),
            compatibility_less);
}

bool valid_backend(const qualification_backend_binding &v) noexcept {
  return validate_backend_identity(v.identity).has_value() &&
         valid_text(v.implementation_name) &&
         valid_text(v.implementation_version) &&
         !digest_zero(v.adapter_source_digest) &&
         !digest_zero(v.adapter_binary_digest) &&
         !digest_zero(v.dependency_digest);
}

bool valid_result_mode(const qualification_result_mode_binding &v) noexcept {
  if (!known(v.representation) || !known(v.semantics) ||
      digest_zero(v.policy_digest))
    return false;
  if (v.representation == result_representation::exact_stratified)
    return v.semantics == product_realization_semantics::not_requested;
  if (v.representation == result_representation::exact_in_T_mesh)
    return v.semantics == product_realization_semantics::exact_in_T;
  return v.semantics ==
         product_realization_semantics::certified_approximate_embedding_v1;
}

bool valid_preparation(const qualification_preparation_binding &v) noexcept {
  if (!known(v.mode) || !known(v.tolerance_unit) || digest_zero(v.policy_digest))
    return false;
  if (v.mode == preparation_mode::strict_validation)
    return v.tolerance_unit == model_unit::unspecified &&
           v.model_tolerance_binary64_bits == 0;
  return true;
}

bool valid_type(const qualification_type_binding &v) noexcept {
  return known(v.coordinate) && known(v.index);
}

bool valid_toolchain(const qualification_toolchain_binding &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.compiler_name) &&
         valid_text(v.compiler_version) && known(v.standard_library) &&
         valid_text(v.standard_library_version) && known(v.architecture) &&
         valid_text(v.operating_system) &&
         valid_text(v.operating_system_version) && valid_text(v.target_triple) &&
         valid_text(v.build_type) && known(v.floating_point_mode) &&
         strictly_sorted(v.compile_flags, std::less<std::string>{}) &&
         std::all_of(v.compile_flags.begin(), v.compile_flags.end(),
                     [](const std::string &s) { return valid_text(s); }) &&
         !digest_zero(v.environment_digest);
}

bool valid_verifier(const qualification_verifier_binding &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.version) &&
         !digest_zero(v.implementation_digest);
}

bool valid_corpus(const qualification_corpus_binding &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.version) && known(v.source) &&
         known(v.redistribution) && valid_text(v.license_or_provenance) &&
         v.case_count != 0 && !digest_zero(v.corpus_digest) &&
         !digest_zero(v.category_coverage_digest) &&
         !digest_zero(v.expected_outcome_digest);
}

bool valid_generator(const qualification_generator_binding &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.version) &&
         !digest_zero(v.implementation_digest) && v.first_seed <= v.last_seed &&
         !digest_zero(v.parameter_range_digest);
}

bool valid_fuzz(const qualification_fuzz_campaign_binding &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.engine) &&
         valid_text(v.engine_version) && !digest_zero(v.dictionary_digest) &&
         !digest_zero(v.mutator_digest) && !digest_zero(v.seed_set_digest) &&
         v.duration_seconds != 0 && v.worker_count != 0;
}

bool valid_chain(const qualification_chain_binding &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.version) &&
         !digest_zero(v.definition_digest) && v.chain_count != 0 &&
         v.minimum_steps >= 5 && v.minimum_steps <= v.maximum_steps;
}

bool valid_resource(const qualification_resource_binding &v) noexcept {
  return valid_text(v.identifier) && !digest_zero(v.policy_digest) &&
         v.wall_timeout_milliseconds != 0 &&
         v.cancellation_latency_limit_milliseconds != 0;
}

bool valid_performance(const qualification_performance_protocol &v) noexcept {
  return valid_text(v.identifier) && valid_text(v.version) &&
         valid_text(v.hardware_identifier) && !digest_zero(v.hardware_digest) &&
         !digest_zero(v.measurement_protocol_digest) && v.measured_runs != 0;
}

bool valid_threshold(const qualification_threshold &v) noexcept {
  return valid_text(v.metric) && known(v.relation) && v.denominator != 0 &&
         valid_text(v.unit);
}

bool valid_compatibility(const qualification_compatibility_review &v) noexcept {
  return !digest_zero(v.prior_manifest_digest) &&
         !digest_zero(v.reviewed_change_digest) && valid_text(v.reviewer) &&
         valid_text(v.rationale) && !digest_zero(v.evidence_digest);
}

bool valid_string_vector(const std::vector<std::string> &values) noexcept {
  return strictly_sorted(values, std::less<std::string>{}) &&
         std::all_of(values.begin(), values.end(), [](const std::string &s) {
           return valid_text(s);
         });
}

product_status_or<bool>
validate_manifest_body(const qualification_campaign_manifest &m) noexcept {
  if (m.schema != qualification_campaign_schema_version ||
      !valid_text(m.identifier) || !valid_text(m.workload_profile) ||
      !valid_text(m.repository_commit) ||
      digest_zero(m.repository_tree_digest) || m.repository_dirty ||
      !valid_text(m.created_utc))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_campaign.identity");
  if (m.backends.empty() || m.result_modes.empty() ||
      m.preparation_policies.empty() || m.type_specializations.empty() ||
      m.toolchains.empty() || m.verifiers.empty() || m.corpora.empty() ||
      m.generators.empty() || m.fuzz_campaigns.empty() ||
      m.operation_chains.empty() || m.resource_policies.empty() ||
      m.thresholds.empty())
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_campaign.required_bindings");
  if (!strictly_sorted(m.backends, backend_less) ||
      !strictly_sorted(m.result_modes, result_mode_less) ||
      !strictly_sorted(m.preparation_policies, preparation_less) ||
      !strictly_sorted(m.type_specializations, type_less) ||
      !strictly_sorted(m.toolchains, toolchain_less) ||
      !strictly_sorted(m.verifiers, verifier_less) ||
      !strictly_sorted(m.corpora, corpus_less) ||
      !strictly_sorted(m.generators, generator_less) ||
      !strictly_sorted(m.fuzz_campaigns, fuzz_less) ||
      !strictly_sorted(m.operation_chains, chain_less) ||
      !strictly_sorted(m.resource_policies, resource_less) ||
      !strictly_sorted(m.thresholds, threshold_less) ||
      !strictly_sorted(m.compatibility_reviews, compatibility_less) ||
      !valid_string_vector(m.exclusions) ||
      !valid_string_vector(m.known_limitations))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_campaign.canonical_order");
  if (!std::all_of(m.backends.begin(), m.backends.end(), valid_backend) ||
      !std::all_of(m.result_modes.begin(), m.result_modes.end(),
                   valid_result_mode) ||
      !std::all_of(m.preparation_policies.begin(),
                   m.preparation_policies.end(), valid_preparation) ||
      !std::all_of(m.type_specializations.begin(),
                   m.type_specializations.end(), valid_type) ||
      !std::all_of(m.toolchains.begin(), m.toolchains.end(), valid_toolchain) ||
      !std::all_of(m.verifiers.begin(), m.verifiers.end(), valid_verifier) ||
      !std::all_of(m.corpora.begin(), m.corpora.end(), valid_corpus) ||
      !std::all_of(m.generators.begin(), m.generators.end(), valid_generator) ||
      !std::all_of(m.fuzz_campaigns.begin(), m.fuzz_campaigns.end(),
                   valid_fuzz) ||
      !std::all_of(m.operation_chains.begin(), m.operation_chains.end(),
                   valid_chain) ||
      !std::all_of(m.resource_policies.begin(), m.resource_policies.end(),
                   valid_resource) ||
      !std::all_of(m.thresholds.begin(), m.thresholds.end(), valid_threshold) ||
      !std::all_of(m.compatibility_reviews.begin(),
                   m.compatibility_reviews.end(), valid_compatibility) ||
      !valid_performance(m.performance))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_campaign.binding");
  canonical_encoder material;
  encode_manifest_material(material, m);
  const auto expected_material =
      domain_digest(manifest_material_tag, material.bytes());
  if (m.material_binding_digest != expected_material)
    return qerror(product_error_code::stale_binding,
                  "qualification_campaign.material_digest");
  canonical_encoder manifest_payload;
  encode_manifest_digest_payload(manifest_payload, m);
  const auto expected_manifest =
      domain_digest(manifest_digest_tag, manifest_payload.bytes());
  if (m.manifest_digest != expected_manifest)
    return qerror(product_error_code::stale_binding,
                  "qualification_campaign.manifest_digest");
  return true;
}

void encode_dimension(canonical_encoder &e,
                      const qualification_dimension_key &v) {
  e.u16(static_cast<std::uint16_t>(v.backend));
  e.byte(static_cast<std::uint8_t>(v.representation));
  e.byte(static_cast<std::uint8_t>(v.preparation));
  e.byte(static_cast<std::uint8_t>(v.selected_operation));
  e.byte(static_cast<std::uint8_t>(v.coordinate));
  e.byte(static_cast<std::uint8_t>(v.index));
  e.string(v.toolchain_identifier);
  e.string(v.geometry_category);
}

void encode_outcome_count(canonical_encoder &e,
                          const qualification_outcome_count &v) {
  encode_dimension(e, v.dimensions);
  e.byte(static_cast<std::uint8_t>(v.outcome));
  e.u64(v.count);
}

void encode_artifact(canonical_encoder &e,
                     const qualification_artifact_reference &v) {
  e.byte(static_cast<std::uint8_t>(v.kind));
  e.string(v.identifier);
  e.string(v.location);
  encode_digest(e, v.content_digest);
  e.u64(v.byte_count);
}

void encode_summary_digest_payload(canonical_encoder &e,
                                   const qualification_result_summary &s) {
  e.u16(s.schema);
  encode_digest(e, s.manifest_digest);
  e.string(s.run_identifier);
  e.string(s.repository_commit);
  e.string(s.started_utc);
  e.string(s.finished_utc);
  e.boolean(s.complete);
  encode_vector(e, s.counts, encode_outcome_count);
  encode_vector(e, s.artifacts, encode_artifact);
  e.u64(s.blocking_issue_count);
  e.u64(s.false_success_count);
}

qualification_dimension_key read_dimension(reader &r) {
  qualification_dimension_key result;
  const auto backend = static_cast<backend_id>(r.u16());
  if (!known(backend))
    throw std::runtime_error("backend");
  result.backend = backend;
  result.representation = read_enum<result_representation>(r, known);
  result.preparation = read_enum<preparation_mode>(r, known);
  result.selected_operation = read_enum<operation>(r, known);
  result.coordinate = read_enum<coordinate_tag>(r, known);
  result.index = read_enum<index_tag>(r, known);
  result.toolchain_identifier = r.string();
  result.geometry_category = r.string();
  return result;
}

qualification_outcome_count read_outcome_count(reader &r) {
  qualification_outcome_count result;
  result.dimensions = read_dimension(r);
  result.outcome = read_enum<qualification_outcome>(r, known);
  result.count = r.u64();
  return result;
}

qualification_artifact_reference read_artifact(reader &r) {
  qualification_artifact_reference result;
  result.kind = read_enum<qualification_artifact_kind>(r, known);
  result.identifier = r.string();
  result.location = r.string();
  result.content_digest = r.digest_value();
  result.byte_count = r.u64();
  return result;
}

bool outcome_count_less(const qualification_outcome_count &a,
                        const qualification_outcome_count &b) noexcept {
  if (a.dimensions < b.dimensions)
    return true;
  if (b.dimensions < a.dimensions)
    return false;
  return static_cast<unsigned>(a.outcome) < static_cast<unsigned>(b.outcome);
}

bool artifact_less(const qualification_artifact_reference &a,
                   const qualification_artifact_reference &b) noexcept {
  return std::tie(a.kind, a.identifier, a.location, a.content_digest,
                  a.byte_count) <
         std::tie(b.kind, b.identifier, b.location, b.content_digest,
                  b.byte_count);
}

bool blocking_outcome(qualification_outcome v) noexcept {
  return v != qualification_outcome::verified_exact_success &&
         v != qualification_outcome::verified_certified_approximate_success &&
         v != qualification_outcome::expected_typed_failure;
}

bool checked_accumulate(std::uint64_t &sum, std::uint64_t value) noexcept {
  if (value > std::numeric_limits<std::uint64_t>::max() - sum)
    return false;
  sum += value;
  return true;
}

void canonicalize_summary(qualification_result_summary &s) {
  std::sort(s.counts.begin(), s.counts.end(), outcome_count_less);
  std::sort(s.artifacts.begin(), s.artifacts.end(), artifact_less);
}

product_status_or<bool>
validate_summary_body(const qualification_result_summary &s) noexcept {
  if (s.schema != qualification_result_summary_schema_version ||
      digest_zero(s.manifest_digest) || !valid_text(s.run_identifier) ||
      !valid_text(s.repository_commit) || !valid_text(s.started_utc) ||
      !valid_text(s.finished_utc) || s.counts.empty())
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_summary.identity");
  if (!strictly_sorted(s.counts, outcome_count_less) ||
      !strictly_sorted(s.artifacts, artifact_less))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_summary.canonical_order");
  std::uint64_t blocking = 0;
  std::uint64_t false_success = 0;
  for (const auto &entry : s.counts) {
    const auto &d = entry.dimensions;
    if (!known(d.backend) || !known(d.representation) ||
        !known(d.preparation) || !known(d.selected_operation) ||
        !known(d.coordinate) || !known(d.index) ||
        !valid_text(d.toolchain_identifier) ||
        !valid_text(d.geometry_category) || !known(entry.outcome) ||
        entry.count == 0)
      return qerror(product_error_code::qualification_policy_violation,
                    "qualification_summary.count");
    if (blocking_outcome(entry.outcome) && !checked_accumulate(blocking, entry.count))
      return qerror(product_error_code::resource_limit,
                    "qualification_summary.blocking_overflow");
    if (entry.outcome == qualification_outcome::false_success &&
        !checked_accumulate(false_success, entry.count))
      return qerror(product_error_code::resource_limit,
                    "qualification_summary.false_success_overflow");
  }
  for (const auto &artifact : s.artifacts)
    if (!known(artifact.kind) || !valid_text(artifact.identifier) ||
        !valid_text(artifact.location) || digest_zero(artifact.content_digest) ||
        artifact.byte_count == 0)
      return qerror(product_error_code::qualification_policy_violation,
                    "qualification_summary.artifact");
  if (s.blocking_issue_count != blocking ||
      s.false_success_count != false_success)
    return qerror(product_error_code::stale_binding,
                  "qualification_summary.accounting");
  canonical_encoder payload;
  encode_summary_digest_payload(payload, s);
  if (s.summary_digest != domain_digest(summary_digest_tag, payload.bytes()))
    return qerror(product_error_code::stale_binding,
                  "qualification_summary.digest");
  return true;
}

const char *decision_name(qualification_report_decision d) noexcept {
  switch (d) {
  case qualification_report_decision::candidate:
    return "candidate";
  case qualification_report_decision::qualified:
    return "qualified";
  case qualification_report_decision::rejected:
    return "rejected";
  case qualification_report_decision::revoked:
    return "revoked";
  }
  return "unknown";
}

void encode_section(canonical_encoder &e,
                    const qualification_report_section &s) {
  e.byte(static_cast<std::uint8_t>(s.kind));
  e.string(s.heading);
  encode_string_vector(e, s.lines);
}

void encode_report_digest_payload(canonical_encoder &e,
                                  const qualification_human_report &r) {
  e.u16(r.schema);
  encode_digest(e, r.manifest_digest);
  encode_digest(e, r.summary_digest);
  e.string(r.title);
  e.byte(static_cast<std::uint8_t>(r.decision));
  e.string(r.claim_scope);
  e.string(r.generated_utc);
  e.u64(r.blocking_issue_count);
  e.u64(r.false_success_count);
  encode_vector(e, r.sections, encode_section);
  encode_digest(e, r.markdown_digest);
}

qualification_report_section read_section(reader &r) {
  qualification_report_section result;
  result.kind = read_enum<qualification_report_section_kind>(r, known);
  result.heading = r.string();
  result.lines = read_string_vector(r);
  return result;
}

bool section_less(const qualification_report_section &a,
                  const qualification_report_section &b) noexcept {
  return static_cast<unsigned>(a.kind) < static_cast<unsigned>(b.kind);
}

void canonicalize_report(qualification_human_report &r) {
  std::sort(r.sections.begin(), r.sections.end(), section_less);
}

std::string render_report_impl(const qualification_human_report &r) {
  std::ostringstream out;
  out << "# " << r.title << "\n\n";
  out << "- Decision: `" << decision_name(r.decision) << "`\n";
  out << "- Claim scope: " << r.claim_scope << "\n";
  out << "- Generated UTC: `" << r.generated_utc << "`\n";
  out << "- Campaign manifest: `" << r.manifest_digest.hex() << "`\n";
  out << "- Machine summary: `" << r.summary_digest.hex() << "`\n";
  out << "- Blocking issues: " << r.blocking_issue_count << "\n";
  out << "- False successes: " << r.false_success_count << "\n\n";
  for (const auto &section : r.sections) {
    out << "## " << section.heading << "\n\n";
    for (const auto &line : section.lines)
      out << line << "\n";
    out << "\n";
  }
  return out.str();
}

product_status_or<bool>
validate_report_body(const qualification_human_report &r) noexcept {
  if (r.schema != qualification_human_report_schema_version ||
      digest_zero(r.manifest_digest) || digest_zero(r.summary_digest) ||
      !valid_text(r.title) || !known(r.decision) ||
      !valid_text(r.claim_scope) || !valid_text(r.generated_utc))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_report.identity");
  if (r.sections.size() !=
          static_cast<std::size_t>(qualification_report_section_kind::count) ||
      !strictly_sorted(r.sections, section_less))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_report.sections");
  for (std::size_t i = 0; i != r.sections.size(); ++i) {
    const auto &section = r.sections[i];
    if (static_cast<unsigned>(section.kind) != i ||
        !valid_text(section.heading) || section.lines.empty() ||
        !std::all_of(section.lines.begin(), section.lines.end(),
                     [](const std::string &line) {
                       return valid_text(line);
                     }))
      return qerror(product_error_code::qualification_policy_violation,
                    "qualification_report.section_content");
  }
  if (r.decision == qualification_report_decision::qualified &&
      (r.blocking_issue_count != 0 || r.false_success_count != 0))
    return qerror(product_error_code::qualification_policy_violation,
                  "qualification_report.qualified_with_blockers");
  const auto markdown = render_report_impl(r);
  canonical_encoder markdown_bytes;
  markdown_bytes.string(markdown);
  if (r.markdown_digest !=
      domain_digest(markdown_digest_tag, markdown_bytes.bytes()))
    return qerror(product_error_code::stale_binding,
                  "qualification_report.markdown_digest");
  canonical_encoder payload;
  encode_report_digest_payload(payload, r);
  if (r.report_digest != domain_digest(report_digest_tag, payload.bytes()))
    return qerror(product_error_code::stale_binding,
                  "qualification_report.digest");
  return true;
}

} // namespace

product_status_or<qualification_campaign_manifest>
make_qualification_campaign_manifest(qualification_campaign_manifest m) {
  try {
    canonicalize_manifest(m);
    canonical_encoder material;
    encode_manifest_material(material, m);
    m.material_binding_digest =
        domain_digest(manifest_material_tag, material.bytes());
    canonical_encoder manifest_payload;
    encode_manifest_digest_payload(manifest_payload, m);
    m.manifest_digest =
        domain_digest(manifest_digest_tag, manifest_payload.bytes());
    auto valid = validate_manifest_body(m);
    if (!valid.has_value())
      return valid.error();
    return m;
  } catch (const std::bad_alloc &) {
    return qerror(product_error_code::resource_limit,
                  "qualification_campaign.allocation");
  } catch (const std::exception &x) {
    auto error = qerror(product_error_code::qualification_policy_violation,
                        "qualification_campaign.make");
    error.detail = x.what();
    return error;
  }
}

product_status_or<bool>
validate_qualification_campaign_manifest(
    const qualification_campaign_manifest &m) noexcept {
  try {
    return validate_manifest_body(m);
  } catch (...) {
    return qerror(product_error_code::internal_invariant_error,
                  "qualification_campaign.validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_campaign_manifest(
    const qualification_campaign_manifest &m) {
  auto valid = validate_qualification_campaign_manifest(m);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder payload;
  encode_manifest_digest_payload(payload, m);
  encode_digest(payload, m.manifest_digest);
  return record_bytes(manifest_record_tag, m.schema, payload.bytes());
}

product_status_or<qualification_campaign_manifest>
decode_qualification_campaign_manifest(
    const std::vector<std::uint8_t> &bytes,
    const qualification_decode_limits &limits) {
  try {
    const auto payload = extract_payload(
        bytes, manifest_record_tag, qualification_campaign_schema_version,
        limits);
    reader r(payload, limits);
    qualification_campaign_manifest m;
    m.schema = r.u16();
    m.identifier = r.string();
    m.workload_profile = r.string();
    m.repository_commit = r.string();
    m.repository_tree_digest = r.digest_value();
    m.repository_dirty = r.boolean();
    m.created_utc = r.string();
    m.backends = read_vector<qualification_backend_binding>(r, read_backend);
    m.result_modes =
        read_vector<qualification_result_mode_binding>(r, read_result_mode);
    m.preparation_policies =
        read_vector<qualification_preparation_binding>(r, read_preparation);
    m.type_specializations =
        read_vector<qualification_type_binding>(r, read_type);
    m.toolchains =
        read_vector<qualification_toolchain_binding>(r, read_toolchain);
    m.verifiers =
        read_vector<qualification_verifier_binding>(r, read_verifier);
    m.corpora = read_vector<qualification_corpus_binding>(r, read_corpus);
    m.generators =
        read_vector<qualification_generator_binding>(r, read_generator);
    m.fuzz_campaigns =
        read_vector<qualification_fuzz_campaign_binding>(r, read_fuzz);
    m.operation_chains =
        read_vector<qualification_chain_binding>(r, read_chain);
    m.resource_policies =
        read_vector<qualification_resource_binding>(r, read_resource);
    m.performance = read_performance(r);
    m.thresholds =
        read_vector<qualification_threshold>(r, read_threshold);
    m.exclusions = read_string_vector(r);
    m.known_limitations = read_string_vector(r);
    m.compatibility_reviews =
        read_vector<qualification_compatibility_review>(r, read_compatibility);
    m.material_binding_digest = r.digest_value();
    m.manifest_digest = r.digest_value();
    if (r.remaining() != 0)
      throw std::runtime_error("trailing_bytes");
    auto valid = validate_qualification_campaign_manifest(m);
    if (!valid.has_value())
      return valid.error();
    auto encoded = encode_qualification_campaign_manifest(m);
    if (!encoded.has_value() || encoded.value() != bytes)
      return qerror(product_error_code::input_contract_error,
                    "qualification_campaign.noncanonical");
    return m;
  } catch (const std::bad_alloc &) {
    return qerror(product_error_code::resource_limit,
                  "qualification_campaign.decode_allocation");
  } catch (const std::exception &x) {
    auto error = qerror(product_error_code::input_contract_error,
                        "qualification_campaign.decode");
    error.detail = x.what();
    return error;
  }
}

product_status_or<qualification_result_summary>
make_qualification_result_summary(qualification_result_summary s) {
  try {
    canonicalize_summary(s);
    std::uint64_t blocking = 0;
    std::uint64_t false_success = 0;
    for (const auto &entry : s.counts) {
      if (blocking_outcome(entry.outcome) &&
          !checked_accumulate(blocking, entry.count))
        return qerror(product_error_code::resource_limit,
                      "qualification_summary.blocking_overflow");
      if (entry.outcome == qualification_outcome::false_success &&
          !checked_accumulate(false_success, entry.count))
        return qerror(product_error_code::resource_limit,
                      "qualification_summary.false_success_overflow");
    }
    s.blocking_issue_count = blocking;
    s.false_success_count = false_success;
    canonical_encoder payload;
    encode_summary_digest_payload(payload, s);
    s.summary_digest = domain_digest(summary_digest_tag, payload.bytes());
    auto valid = validate_summary_body(s);
    if (!valid.has_value())
      return valid.error();
    return s;
  } catch (const std::bad_alloc &) {
    return qerror(product_error_code::resource_limit,
                  "qualification_summary.allocation");
  } catch (const std::exception &x) {
    auto error = qerror(product_error_code::qualification_policy_violation,
                        "qualification_summary.make");
    error.detail = x.what();
    return error;
  }
}

product_status_or<bool>
validate_qualification_result_summary(
    const qualification_result_summary &s) noexcept {
  try {
    return validate_summary_body(s);
  } catch (...) {
    return qerror(product_error_code::internal_invariant_error,
                  "qualification_summary.validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_result_summary(const qualification_result_summary &s) {
  auto valid = validate_qualification_result_summary(s);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder payload;
  encode_summary_digest_payload(payload, s);
  encode_digest(payload, s.summary_digest);
  return record_bytes(summary_record_tag, s.schema, payload.bytes());
}

product_status_or<qualification_result_summary>
decode_qualification_result_summary(
    const std::vector<std::uint8_t> &bytes,
    const qualification_decode_limits &limits) {
  try {
    const auto payload = extract_payload(
        bytes, summary_record_tag, qualification_result_summary_schema_version,
        limits);
    reader r(payload, limits);
    qualification_result_summary s;
    s.schema = r.u16();
    s.manifest_digest = r.digest_value();
    s.run_identifier = r.string();
    s.repository_commit = r.string();
    s.started_utc = r.string();
    s.finished_utc = r.string();
    s.complete = r.boolean();
    s.counts = read_vector<qualification_outcome_count>(r, read_outcome_count);
    s.artifacts =
        read_vector<qualification_artifact_reference>(r, read_artifact);
    s.blocking_issue_count = r.u64();
    s.false_success_count = r.u64();
    s.summary_digest = r.digest_value();
    if (r.remaining() != 0)
      throw std::runtime_error("trailing_bytes");
    auto valid = validate_qualification_result_summary(s);
    if (!valid.has_value())
      return valid.error();
    auto encoded = encode_qualification_result_summary(s);
    if (!encoded.has_value() || encoded.value() != bytes)
      return qerror(product_error_code::input_contract_error,
                    "qualification_summary.noncanonical");
    return s;
  } catch (const std::bad_alloc &) {
    return qerror(product_error_code::resource_limit,
                  "qualification_summary.decode_allocation");
  } catch (const std::exception &x) {
    auto error = qerror(product_error_code::input_contract_error,
                        "qualification_summary.decode");
    error.detail = x.what();
    return error;
  }
}

product_status_or<qualification_human_report>
make_qualification_human_report(qualification_human_report r) {
  try {
    canonicalize_report(r);
    const auto markdown = render_report_impl(r);
    canonical_encoder markdown_bytes;
    markdown_bytes.string(markdown);
    r.markdown_digest =
        domain_digest(markdown_digest_tag, markdown_bytes.bytes());
    canonical_encoder payload;
    encode_report_digest_payload(payload, r);
    r.report_digest = domain_digest(report_digest_tag, payload.bytes());
    auto valid = validate_report_body(r);
    if (!valid.has_value())
      return valid.error();
    return r;
  } catch (const std::bad_alloc &) {
    return qerror(product_error_code::resource_limit,
                  "qualification_report.allocation");
  } catch (const std::exception &x) {
    auto error = qerror(product_error_code::qualification_policy_violation,
                        "qualification_report.make");
    error.detail = x.what();
    return error;
  }
}

product_status_or<bool>
validate_qualification_human_report(
    const qualification_human_report &r) noexcept {
  try {
    return validate_report_body(r);
  } catch (...) {
    return qerror(product_error_code::internal_invariant_error,
                  "qualification_report.validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_human_report(const qualification_human_report &r) {
  auto valid = validate_qualification_human_report(r);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder payload;
  encode_report_digest_payload(payload, r);
  encode_digest(payload, r.report_digest);
  return record_bytes(report_record_tag, r.schema, payload.bytes());
}

product_status_or<qualification_human_report>
decode_qualification_human_report(
    const std::vector<std::uint8_t> &bytes,
    const qualification_decode_limits &limits) {
  try {
    const auto payload = extract_payload(
        bytes, report_record_tag, qualification_human_report_schema_version,
        limits);
    reader r(payload, limits);
    qualification_human_report report;
    report.schema = r.u16();
    report.manifest_digest = r.digest_value();
    report.summary_digest = r.digest_value();
    report.title = r.string();
    report.decision = read_enum<qualification_report_decision>(r, known);
    report.claim_scope = r.string();
    report.generated_utc = r.string();
    report.blocking_issue_count = r.u64();
    report.false_success_count = r.u64();
    report.sections =
        read_vector<qualification_report_section>(r, read_section);
    report.markdown_digest = r.digest_value();
    report.report_digest = r.digest_value();
    if (r.remaining() != 0)
      throw std::runtime_error("trailing_bytes");
    auto valid = validate_qualification_human_report(report);
    if (!valid.has_value())
      return valid.error();
    auto encoded = encode_qualification_human_report(report);
    if (!encoded.has_value() || encoded.value() != bytes)
      return qerror(product_error_code::input_contract_error,
                    "qualification_report.noncanonical");
    return report;
  } catch (const std::bad_alloc &) {
    return qerror(product_error_code::resource_limit,
                  "qualification_report.decode_allocation");
  } catch (const std::exception &x) {
    auto error = qerror(product_error_code::input_contract_error,
                        "qualification_report.decode");
    error.detail = x.what();
    return error;
  }
}

product_status_or<std::string>
render_qualification_report_markdown(const qualification_human_report &r) {
  auto valid = validate_qualification_human_report(r);
  if (!valid.has_value())
    return valid.error();
  return render_report_impl(r);
}

digest qualification_material_change_digest(const digest &prior_material,
                                             const digest &next_material) {
  canonical_encoder e;
  encode_digest(e, prior_material);
  encode_digest(e, next_material);
  return domain_digest(compatibility_change_tag, e.bytes());
}

bool qualification_claim_remains_valid(
    const qualification_campaign_manifest &prior,
    const qualification_campaign_manifest &next) noexcept {
  if (!validate_qualification_campaign_manifest(prior).has_value() ||
      !validate_qualification_campaign_manifest(next).has_value())
    return false;
  if (prior.material_binding_digest == next.material_binding_digest)
    return true;
  const auto change = qualification_material_change_digest(
      prior.material_binding_digest, next.material_binding_digest);
  return std::any_of(
      next.compatibility_reviews.begin(), next.compatibility_reviews.end(),
      [&](const qualification_compatibility_review &review) {
        return review.approved &&
               review.prior_manifest_digest == prior.manifest_digest &&
               review.reviewed_change_digest == change &&
               !digest_zero(review.evidence_digest);
      });
}

product_status_or<qualification_evidence_binding>
make_qualification_evidence_binding(
    const qualification_campaign_manifest &manifest,
    const qualification_result_summary &summary,
    const qualification_human_report &report) noexcept {
  try {
    auto mv = validate_qualification_campaign_manifest(manifest);
    if (!mv.has_value())
      return mv.error();
    auto sv = validate_qualification_result_summary(summary);
    if (!sv.has_value())
      return sv.error();
    auto rv = validate_qualification_human_report(report);
    if (!rv.has_value())
      return rv.error();
    if (summary.manifest_digest != manifest.manifest_digest ||
        summary.repository_commit != manifest.repository_commit ||
        !summary.complete || report.manifest_digest != manifest.manifest_digest ||
        report.summary_digest != summary.summary_digest ||
        report.blocking_issue_count != summary.blocking_issue_count ||
        report.false_success_count != summary.false_success_count ||
        report.decision != qualification_report_decision::qualified ||
        summary.blocking_issue_count != 0 || summary.false_success_count != 0)
      return qerror(product_error_code::qualification_policy_violation,
                    "qualification_evidence.cross_binding");
    qualification_evidence_binding evidence;
    evidence.campaign_manifest_digest = manifest.manifest_digest;
    evidence.result_summary_digest = summary.summary_digest;
    evidence.human_report_digest = report.report_digest;
    return evidence;
  } catch (...) {
    return qerror(product_error_code::internal_invariant_error,
                  "qualification_evidence.exception");
  }
}

} // namespace mesh_boolean
} // namespace ygor
