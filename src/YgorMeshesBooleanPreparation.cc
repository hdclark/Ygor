#include "YgorMeshesBooleanPreparation.h"

#include "YgorMeshesBooleanInputTopology.h"

#include <cstring>
#include <limits>
#include <stdexcept>

#if defined(__FAST_MATH__)
#error "Boolean preparation must not be compiled with fast-math"
#endif

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> record_tag{{'Y', 'G', 'B', 'P', 'R', 'P', '0', '1'}};
constexpr std::array<char, 8> input_tag{{'Y', 'G', 'B', 'P', 'I', 'N', '0', '1'}};
constexpr std::array<char, 8> policy_tag{{'Y', 'G', 'B', 'P', 'P', 'O', '0', '1'}};
constexpr std::array<char, 8> certificate_tag{{'Y', 'G', 'B', 'P', 'C', 'E', '0', '1'}};
constexpr std::array<char, 8> pair_tag{{'Y', 'G', 'B', 'P', 'P', 'R', '0', '1'}};
constexpr std::array<char, 8> options_tag{{'Y', 'G', 'B', 'P', 'O', 'P', '0', '1'}};

boolean_error preparation_error(preparation_validation_subcode subcode,
                                const char *key) {
  return make_error(boolean_error_code::input_contract_error,
                    boolean_stage::input_validation, key,
                    static_cast<std::uint32_t>(subcode));
}

void encode_digest(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}

bool nonzero(const digest &d) {
  for (auto b : d.bytes)
    if (b != 0) return true;
  return false;
}

coordinate_tag coordinate_type(bool is_float) {
  return is_float ? coordinate_tag::binary32 : coordinate_tag::binary64;
}

index_tag index_type(bool is_u32) {
  return is_u32 ? index_tag::uint32 : index_tag::uint64;
}

digest options_digest_for(const boolean_options &options) {
  auto bytes = encode_options(options);
  if (!bytes.has_value()) throw std::invalid_argument("invalid preparation options");
  return domain_digest(options_tag, bytes.value());
}

digest policy_digest_for(const strict_validation_policy &p,
                         const digest &options_digest) {
  canonical_encoder e;
  e.u16(p.schema);
  e.byte(static_cast<std::uint8_t>(p.verification));
  e.boolean(p.remove_unused_storage);
  encode_digest(e, options_digest);
  return domain_digest(policy_tag, e.bytes());
}

template <class T, class I>
std::vector<std::uint8_t> mesh_bytes(const fv_surface_mesh<T, I> &mesh) {
  canonical_encoder e;
  e.byte(sizeof(T));
  e.byte(sizeof(I));
  e.u64(mesh.vertices.size());
  for (const auto &v : mesh.vertices) {
    e.floating(v.x);
    e.floating(v.y);
    e.floating(v.z);
  }
  e.u64(mesh.faces.size());
  for (const auto &face : mesh.faces) {
    e.u64(face.size());
    for (I index : face) e.u64(static_cast<std::uint64_t>(index));
  }
  return e.bytes();
}

template <class T, class I>
digest mesh_digest(const fv_surface_mesh<T, I> &mesh) {
  return domain_digest(input_tag, mesh_bytes(mesh));
}

void encode_certificate_body(canonical_encoder &e,
                             const strict_validation_certificate &c) {
  e.u16(c.schema);
  e.u16(c.checker_version);
  e.byte(static_cast<std::uint8_t>(c.coordinate));
  e.byte(static_cast<std::uint8_t>(c.index));
  encode_digest(e, c.input_digest);
  encode_digest(e, c.prepared_digest);
  encode_digest(e, c.validation_options_digest);
  encode_digest(e, c.policy_digest);
  encode_digest(e, c.semantic_digest);
  encode_digest(e, c.validation_artifact_digest);
  encode_digest(e, c.validation_report_digest);
  encode_digest(e, c.invariant_set_digest);
  encode_digest(e, c.kernel_policy_digest);
  e.boolean(c.geometry_changed);
}

digest certificate_digest_for(const strict_validation_certificate &c) {
  canonical_encoder e;
  encode_certificate_body(e, c);
  return domain_digest(certificate_tag, e.bytes());
}

digest pair_digest(const digest &a, const digest &b, std::uint8_t field) {
  canonical_encoder e;
  e.byte(field);
  encode_digest(e, a);
  encode_digest(e, b);
  return domain_digest(pair_tag, e.bytes());
}

template <class T, class I>
status_or<bool> validate_bindings(const prepared_operand<T, I> &operand,
                                  const exact_kernel_services<T> *kernel,
                                  const boolean_options *requested) {
  const auto &p = operand.policy();
  const auto &c = operand.certificate();
  if (p.schema != prepared_operand_schema ||
      p.verification > verification_level::exhaustive ||
      p.remove_unused_storage || c.schema != prepared_operand_schema ||
      c.checker_version != 1 || c.geometry_changed)
    return preparation_error(preparation_validation_subcode::malformed_record,
                             "prepared_operand_contract");
  const auto expected_coordinate =
      coordinate_type(std::is_same<T, float>::value);
  const auto expected_index = index_type(std::is_same<I, std::uint32_t>::value);
  if (c.coordinate != expected_coordinate || c.index != expected_index)
    return preparation_error(preparation_validation_subcode::type_mismatch,
                             "prepared_operand_type");
  const auto input = mesh_digest(operand.mesh());
  if (c.input_digest != input || c.prepared_digest != input)
    return preparation_error(preparation_validation_subcode::stale_input_digest,
                             "prepared_operand_input_digest");
  if (!nonzero(c.validation_options_digest) ||
      c.policy_digest != policy_digest_for(p, c.validation_options_digest) ||
      (requested &&
       (p.verification != requested->verification ||
        c.validation_options_digest != options_digest_for(*requested))))
    return preparation_error(preparation_validation_subcode::stale_policy_digest,
                             "prepared_operand_policy_digest");
  if (!nonzero(c.invariant_set_digest))
    return preparation_error(preparation_validation_subcode::stale_report_digest,
                             "prepared_operand_report_digest");
  if (kernel) {
    const auto bytes = kernel->arithmetic_policy_bytes();
    const auto digest_value =
        domain_digest({{'Y', 'G', 'B', 'K', 'E', 'R', '0', '1'}}, bytes);
    if (c.kernel_policy_digest != digest_value)
      return preparation_error(preparation_validation_subcode::stale_policy_digest,
                               "prepared_operand_kernel_policy");
  }
  if (!nonzero(c.validation_artifact_digest) ||
      !nonzero(c.validation_report_digest) || !nonzero(c.semantic_digest))
    return preparation_error(preparation_validation_subcode::stale_report_digest,
                             "prepared_operand_report_binding");
  if (c.certificate_digest != certificate_digest_for(c))
    return preparation_error(preparation_validation_subcode::stale_certificate,
                             "prepared_operand_certificate");
  return true;
}

class reader {
  const std::vector<std::uint8_t> &bytes_;
  const prepared_operand_decode_limits &limits_;
  std::size_t at_ = 0;
  std::uint64_t indices_ = 0;

public:
  reader(const std::vector<std::uint8_t> &bytes,
         const prepared_operand_decode_limits &limits)
      : bytes_(bytes), limits_(limits) {
    if (bytes.size() > limits.max_record_bytes)
      throw std::length_error("prepared_operand_record_limit");
  }
  std::uint8_t byte() {
    if (at_ == bytes_.size()) throw std::runtime_error("prepared_operand_truncated");
    return bytes_[at_++];
  }
  std::uint16_t u16() {
    std::uint16_t value = 0;
    for (unsigned i = 0; i != 2; ++i) value = std::uint16_t((value << 8) | byte());
    return value;
  }
  std::uint32_t u32() {
    std::uint32_t value = 0;
    for (unsigned i = 0; i != 4; ++i) value = (value << 8) | byte();
    return value;
  }
  std::uint64_t u64() {
    std::uint64_t value = 0;
    for (unsigned i = 0; i != 8; ++i) value = (value << 8) | byte();
    return value;
  }
  bool boolean() {
    const auto value = byte();
    if (value > 1) throw std::runtime_error("prepared_operand_boolean");
    return value != 0;
  }
  digest digest_value() {
    digest value;
    for (auto &b : value.bytes) b = byte();
    return value;
  }
  void tag() {
    for (char expected : record_tag)
      if (byte() != static_cast<std::uint8_t>(expected))
        throw std::runtime_error("prepared_operand_tag");
  }
  std::uint64_t count(std::uint64_t limit, const char *key) {
    const auto value = u64();
    if (value > limit || value > std::numeric_limits<std::size_t>::max())
      throw std::length_error(key);
    return value;
  }
  void add_indices(std::uint64_t value) {
    if (value > limits_.max_face_indices - indices_)
      throw std::length_error("prepared_operand_index_limit");
    indices_ += value;
  }
  bool done() const noexcept { return at_ == bytes_.size(); }
  std::uint64_t remaining() const noexcept { return bytes_.size() - at_; }
};

template <class T> T floating(reader &r) {
  using bits_type = typename std::conditional<sizeof(T) == 4, std::uint32_t,
                                               std::uint64_t>::type;
  const bits_type bits = sizeof(T) == 4 ? static_cast<bits_type>(r.u32())
                                        : static_cast<bits_type>(r.u64());
  T value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

strict_validation_certificate decode_certificate(reader &r) {
  strict_validation_certificate c;
  c.schema = r.u16();
  c.checker_version = r.u16();
  c.coordinate = static_cast<coordinate_tag>(r.byte());
  c.index = static_cast<index_tag>(r.byte());
  c.input_digest = r.digest_value();
  c.prepared_digest = r.digest_value();
  c.validation_options_digest = r.digest_value();
  c.policy_digest = r.digest_value();
  c.semantic_digest = r.digest_value();
  c.validation_artifact_digest = r.digest_value();
  c.validation_report_digest = r.digest_value();
  c.invariant_set_digest = r.digest_value();
  c.kernel_policy_digest = r.digest_value();
  c.geometry_changed = r.boolean();
  c.certificate_digest = r.digest_value();
  return c;
}

} // namespace

template <class T, class I>
status_or<prepared_operand<T, I>> validate_operand_strict(
    const fv_surface_mesh<T, I> &source, const strict_validation_policy &policy,
    const boolean_options &options,
    std::shared_ptr<const exact_kernel_services<T>> kernel,
    std::shared_ptr<const verifier_service> verifiers,
    cancellation_source *cancel) {
  if (policy.schema != prepared_operand_schema || policy.remove_unused_storage ||
      policy.verification != options.verification)
    return preparation_error(preparation_validation_subcode::stale_policy_digest,
                             "strict_validation_policy");
  try {
    auto frozen = source;
    fv_surface_mesh<T, I> empty;
    auto context = make_boolean_context(frozen, empty, operation::regularized_union,
                                        options, kernel, verifiers, cancel);
    if (!context.has_value()) return context.error();
    auto validated = validate_operands(*context.value());
    if (!validated.has_value()) return validated.error();

    auto state = std::make_shared<typename prepared_operand<T, I>::state>();
    state->mesh = std::move(frozen);
    state->policy = policy;
    auto &certificate = state->certificate;
    certificate.coordinate = coordinate_type(std::is_same<T, float>::value);
    certificate.index = index_type(std::is_same<I, std::uint32_t>::value);
    certificate.input_digest = mesh_digest(state->mesh);
    certificate.prepared_digest = certificate.input_digest;
    certificate.validation_options_digest = options_digest_for(options);
    certificate.policy_digest =
        policy_digest_for(policy, certificate.validation_options_digest);
    certificate.semantic_digest =
        validated.value()->payload->operands[0].semantic_digest;
    certificate.validation_artifact_digest =
        validated.value()->payload->artifact_digest;
    certificate.validation_report_digest = validated.value()->report.report_digest;
    certificate.invariant_set_digest =
        validated.value()->report.invariant_set_digest;
    certificate.kernel_policy_digest = domain_digest(
        {{'Y', 'G', 'B', 'K', 'E', 'R', '0', '1'}},
        kernel->arithmetic_policy_bytes());
    certificate.certificate_digest = certificate_digest_for(certificate);
    return prepared_operand<T, I>(std::move(state));
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation,
                      "strict_validation_allocation");
  } catch (const std::exception &e) {
    auto error = make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::input_validation,
                            "strict_validation_exception");
    error.detail = e.what();
    return error;
  }
}

template <class T, class I>
status_or<std::vector<std::uint8_t>>
encode_prepared_operand(const prepared_operand<T, I> &operand) {
  try {
    auto valid = validate_bindings(
        operand, static_cast<const exact_kernel_services<T> *>(nullptr), nullptr);
    if (!valid.has_value()) return valid.error();
    canonical_encoder e;
    e.raw(reinterpret_cast<const std::uint8_t *>(record_tag.data()),
          record_tag.size());
    e.u16(prepared_operand_schema);
    e.u16(operand.policy().schema);
    e.byte(static_cast<std::uint8_t>(operand.policy().verification));
    e.boolean(operand.policy().remove_unused_storage);
    const auto &mesh = operand.mesh();
    e.u64(mesh.vertices.size());
    for (const auto &v : mesh.vertices) {
      e.floating(v.x);
      e.floating(v.y);
      e.floating(v.z);
    }
    e.u64(mesh.faces.size());
    for (const auto &face : mesh.faces) {
      e.u64(face.size());
      for (I index : face) e.u64(static_cast<std::uint64_t>(index));
    }
    encode_certificate_body(e, operand.certificate());
    encode_digest(e, operand.certificate().certificate_digest);
    return e.bytes();
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation,
                      "prepared_operand_encode_allocation");
  }
}

template <class T, class I>
status_or<prepared_operand<T, I>> decode_prepared_operand(
    const std::vector<std::uint8_t> &bytes,
    const prepared_operand_decode_limits &limits) {
  try {
    reader r(bytes, limits);
    r.tag();
    if (r.u16() != prepared_operand_schema)
      return preparation_error(preparation_validation_subcode::malformed_record,
                               "prepared_operand_schema");
    auto state = std::make_shared<typename prepared_operand<T, I>::state>();
    state->policy.schema = r.u16();
    state->policy.verification = static_cast<verification_level>(r.byte());
    state->policy.remove_unused_storage = r.boolean();
    const auto vertices = r.count(limits.max_vertices, "prepared_operand_vertex_limit");
    if (vertices > r.remaining() / (3U * sizeof(T)))
      throw std::runtime_error("prepared_operand_truncated_vertices");
    state->mesh.vertices.reserve(static_cast<std::size_t>(vertices));
    for (std::uint64_t i = 0; i != vertices; ++i)
      state->mesh.vertices.push_back(
          {floating<T>(r), floating<T>(r), floating<T>(r)});
    const auto faces = r.count(limits.max_faces, "prepared_operand_face_limit");
    if (faces > r.remaining() / sizeof(std::uint64_t))
      throw std::runtime_error("prepared_operand_truncated_faces");
    state->mesh.faces.reserve(static_cast<std::size_t>(faces));
    for (std::uint64_t i = 0; i != faces; ++i) {
      const auto count = r.count(limits.max_face_indices,
                                 "prepared_operand_index_limit");
      r.add_indices(count);
      if (count > r.remaining() / sizeof(std::uint64_t))
        throw std::runtime_error("prepared_operand_truncated_indices");
      std::vector<I> face;
      face.reserve(static_cast<std::size_t>(count));
      for (std::uint64_t j = 0; j != count; ++j) {
        const auto value = r.u64();
        if (value > std::numeric_limits<I>::max())
          return preparation_error(preparation_validation_subcode::type_mismatch,
                                   "prepared_operand_index_type");
        face.push_back(static_cast<I>(value));
      }
      state->mesh.faces.push_back(std::move(face));
    }
    state->certificate = decode_certificate(r);
    if (!r.done())
      return preparation_error(preparation_validation_subcode::malformed_record,
                               "prepared_operand_trailing_bytes");
    prepared_operand<T, I> result(std::move(state));
    auto valid = validate_bindings(
        result, static_cast<const exact_kernel_services<T> *>(nullptr), nullptr);
    if (!valid.has_value()) return valid.error();
    auto canonical = encode_prepared_operand(result);
    if (!canonical.has_value()) return canonical.error();
    if (canonical.value() != bytes)
      return preparation_error(preparation_validation_subcode::malformed_record,
                               "prepared_operand_noncanonical");
    return result;
  } catch (const std::length_error &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation,
                      "prepared_operand_decode_limit");
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation,
                      "prepared_operand_decode_allocation");
  } catch (const std::exception &) {
    return preparation_error(preparation_validation_subcode::malformed_record,
                             "prepared_operand_decode");
  }
}

template <class T, class I>
status_or<bool> verify_prepared_operand(
    const prepared_operand<T, I> &operand, const boolean_options &options,
    std::shared_ptr<const exact_kernel_services<T>> kernel,
    std::shared_ptr<const verifier_service> verifiers,
    cancellation_source *cancel) {
  auto bindings = validate_bindings(operand, kernel.get(), &options);
  if (!bindings.has_value()) return bindings.error();
  fv_surface_mesh<T, I> empty;
  auto context = make_boolean_context(operand.mesh(), empty,
                                      operation::regularized_union, options,
                                      kernel, verifiers, cancel);
  if (!context.has_value()) return context.error();
  auto validated = validate_operands(*context.value());
  if (!validated.has_value()) return validated.error();
  const auto &certificate = operand.certificate();
  if (validated.value()->payload->operands[0].semantic_digest !=
          certificate.semantic_digest ||
      validated.value()->payload->artifact_digest !=
          certificate.validation_artifact_digest ||
      validated.value()->report.report_digest !=
          certificate.validation_report_digest ||
      validated.value()->report.invariant_set_digest !=
          certificate.invariant_set_digest)
    return preparation_error(preparation_validation_subcode::stale_report_digest,
                             "prepared_operand_component2_binding");
  return true;
}

template <class T, class I>
status_or<std::unique_ptr<boolean_context<T, I>>> make_boolean_context(
    const prepared_operand<T, I> &a, const prepared_operand<T, I> &b,
    operation op, const boolean_options &options,
    std::shared_ptr<const exact_kernel_services<T>> kernel,
    std::shared_ptr<const verifier_service> verifiers,
    cancellation_source *cancel, diagnostic_consumer diagnostics) {
  auto valid_a = verify_prepared_operand(a, options, kernel, verifiers, cancel);
  if (!valid_a.has_value()) return valid_a.error();
  auto valid_b = verify_prepared_operand(b, options, kernel, verifiers, cancel);
  if (!valid_b.has_value()) return valid_b.error();
  auto owned_a = a.shared_mesh();
  auto owned_b = b.shared_mesh();
  auto context = make_boolean_context(*owned_a, *owned_b, op, options, kernel,
                                      verifiers, cancel, std::move(diagnostics));
  if (!context.has_value()) return context.error();
  context.value()->input_lifetimes_[0] = owned_a;
  context.value()->input_lifetimes_[1] = owned_b;
  auto validated = validate_operands(*context.value());
  if (!validated.has_value()) return validated.error();
  if (validated.value()->report.invariant_set_digest !=
          a.certificate().invariant_set_digest ||
      validated.value()->report.invariant_set_digest !=
          b.certificate().invariant_set_digest)
    return preparation_error(preparation_validation_subcode::stale_report_digest,
                             "prepared_operand_invariant_set");
  context_preparation_provenance provenance;
  provenance.input_digest = pair_digest(a.certificate().input_digest,
                                        b.certificate().input_digest, 0);
  provenance.prepared_digest = pair_digest(a.certificate().prepared_digest,
                                           b.certificate().prepared_digest, 1);
  provenance.policy_digest = pair_digest(a.certificate().policy_digest,
                                         b.certificate().policy_digest, 2);
  provenance.report_digest = pair_digest(a.certificate().validation_report_digest,
                                         b.certificate().validation_report_digest,
                                         3);
  context.value()->preparation_provenance_ = provenance;
  return std::move(context.value());
}

#define YGOR_PREPARATION_INSTANTIATE(T, I)                                      \
  template class prepared_operand<T, I>;                                       \
  template status_or<prepared_operand<T, I>> validate_operand_strict(          \
      const fv_surface_mesh<T, I> &, const strict_validation_policy &,          \
      const boolean_options &,                                                  \
      std::shared_ptr<const exact_kernel_services<T>>,                          \
      std::shared_ptr<const verifier_service>, cancellation_source *);          \
  template status_or<std::vector<std::uint8_t>> encode_prepared_operand(        \
      const prepared_operand<T, I> &);                                          \
  template status_or<prepared_operand<T, I>> decode_prepared_operand(           \
      const std::vector<std::uint8_t> &,                                        \
      const prepared_operand_decode_limits &);                                  \
  template status_or<bool> verify_prepared_operand(                             \
      const prepared_operand<T, I> &, const boolean_options &,                  \
      std::shared_ptr<const exact_kernel_services<T>>,                          \
      std::shared_ptr<const verifier_service>, cancellation_source *);          \
  template status_or<std::unique_ptr<boolean_context<T, I>>>                    \
  make_boolean_context(                                                         \
      const prepared_operand<T, I> &, const prepared_operand<T, I> &, operation,\
      const boolean_options &,                                                  \
      std::shared_ptr<const exact_kernel_services<T>>,                          \
      std::shared_ptr<const verifier_service>, cancellation_source *,           \
      diagnostic_consumer)

YGOR_PREPARATION_INSTANTIATE(float, std::uint32_t);
YGOR_PREPARATION_INSTANTIATE(float, std::uint64_t);
YGOR_PREPARATION_INSTANTIATE(double, std::uint32_t);
YGOR_PREPARATION_INSTANTIATE(double, std::uint64_t);
#undef YGOR_PREPARATION_INSTANTIATE

} // namespace mesh_boolean
} // namespace ygor
