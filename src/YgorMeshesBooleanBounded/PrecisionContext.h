#pragma once

#include "ContextVerifier.h"
#include "BoundedValues.h"
#include "PrecisionBootstrap.h"
#include "PrecisionTrace.h"

#include <cfenv>
#include <memory>

namespace ygor::mesh_boolean::bounded {

inline constexpr std::uint16_t precision_context_schema_v1 = 1;
inline constexpr std::uint16_t precision_context_provider_v1 = 1;

struct precision_runtime_capabilities {
    std::uint16_t version = 1;
    std::uint16_t provider_version = precision_context_provider_v1;
    const context_owner_token *expected_owner = nullptr;
    bool require_nearest_even = true;
    std::uint32_t reserved = 0;
};

template<class T>
class precision_context {
  public:
    std::uint16_t schema_version() const noexcept { return schema_version_; }
    std::uint16_t provider_version() const noexcept { return provider_version_; }
    std::uint16_t scalar_profile_version() const noexcept { return scalar_profile_version_; }
    std::uint16_t arithmetic_profile_version() const noexcept { return arithmetic_profile_version_; }
    std::uint8_t scalar_bytes() const noexcept { return scalar_bytes_; }
    std::uint8_t index_bytes() const noexcept { return index_bytes_; }
    T tolerance() const noexcept { return tolerance_; }
    T declared_input_precision_a() const noexcept { return declared_input_precision_a_; }
    T declared_input_precision_b() const noexcept { return declared_input_precision_b_; }
    T effective_input_precision_a() const noexcept { return effective_input_precision_a_; }
    T effective_input_precision_b() const noexcept { return effective_input_precision_b_; }
    T required_machine_floor() const noexcept { return required_machine_floor_; }
    bool ordinary_success_eligible() const noexcept { return ordinary_success_eligible_; }
    const precision_scale_descriptor<T> &operand_a_scale() const noexcept { return operand_a_; }
    const precision_scale_descriptor<T> &operand_b_scale() const noexcept { return operand_b_; }
    const precision_scale_descriptor<T> &global_scale() const noexcept { return global_; }
    const bounded_boolean_digest &source_digest() const noexcept { return source_digest_; }
    const bounded_boolean_digest &replay_digest() const noexcept { return replay_digest_; }
    const bounded_boolean_digest &boolean_context_digest() const noexcept { return boolean_context_digest_; }
    const bounded_boolean_digest &preflight_digest() const noexcept { return preflight_digest_; }
    const bounded_boolean_digest &digest() const noexcept { return digest_; }
    const std::vector<std::uint8_t> &canonical_bytes() const noexcept { return canonical_bytes_; }
    const context_owner_token &owner() const noexcept { return owner_; }
    bool owned_by(const context_owner_token &owner) const noexcept {
        return owner_.anchor && owner_.same_owner(owner);
    }

  private:
    std::uint16_t schema_version_ = precision_context_schema_v1;
    std::uint16_t provider_version_ = precision_context_provider_v1;
    std::uint16_t scalar_profile_version_ = precision_scalar_profile_v1;
    std::uint16_t arithmetic_profile_version_ = precision_arithmetic_profile_v1;
    std::uint8_t scalar_bytes_ = sizeof(T);
    std::uint8_t index_bytes_ = 0;
    T tolerance_ = T(0);
    T declared_input_precision_a_ = T(0);
    T declared_input_precision_b_ = T(0);
    T effective_input_precision_a_ = T(0);
    T effective_input_precision_b_ = T(0);
    T required_machine_floor_ = T(0);
    bool ordinary_success_eligible_ = false;
    precision_scale_descriptor<T> operand_a_{};
    precision_scale_descriptor<T> operand_b_{};
    precision_scale_descriptor<T> global_{};
    context_owner_token owner_{};
    bounded_boolean_digest source_digest_{};
    bounded_boolean_digest replay_digest_{};
    bounded_boolean_digest boolean_context_digest_{};
    bounded_boolean_digest preflight_digest_{};
    std::vector<std::uint8_t> canonical_bytes_;
    bounded_boolean_digest digest_{};

    template<class U, class I>
    friend boolean_outcome<std::shared_ptr<const precision_context<U>>> build_precision_context(
        const precision_preflight<U> &, const boolean_context<U,I> &,
        const precision_runtime_capabilities &);
};

template<class T>
class precision_context_view {
  public:
    precision_context_view(const precision_context<T> &context,
                           const context_owner_token &owner) noexcept
        : context_(&context), owner_(owner) {}
    precision_context_view(precision_context<T> &&,
                           const context_owner_token &) = delete;
    bool valid() const noexcept { return context_ && context_->owned_by(owner_); }
    T tolerance() const noexcept { return valid() ? context_->tolerance() : T(0); }
    T machine_floor() const noexcept { return valid() ? context_->required_machine_floor() : T(0); }
    T effective_input_precision(operand_id operand) const noexcept {
        if (!valid()) return T(0);
        return operand == operand_id::a ? context_->effective_input_precision_a()
                                        : context_->effective_input_precision_b();
    }
    bool ordinary_success_eligible() const noexcept {
        return valid() && context_->ordinary_success_eligible();
    }
    const bounded_boolean_digest *digest() const noexcept {
        return valid() ? &context_->digest() : nullptr;
    }
  private:
    const precision_context<T> *context_;
    context_owner_token owner_;
};

template<class T>
struct source_bounded_value_batch final {
    context_owner_token owner{};
    operand_id operand = operand_id::a;
    local_precision_trace trace;
    std::vector<local_bounded_value<bounded_point3<T>>> points;

    source_bounded_value_batch(context_owner_token owner_token, operand_id source_operand)
        : owner(std::move(owner_token)), operand(source_operand), trace(owner) {}
};

namespace precision_detail {

inline std::uint64_t source_identity_base(operand_id operand) noexcept {
    return operand == operand_id::a ? 0 : (std::uint64_t(1) << 63);
}

template<class T>
void encode_source_scalar_trace(precision_trace_key &key, T nominal,
                                const finite_interval<T> &enclosure,
                                const uncertainty_contributors &contributors) {
    canonical_writer result;
    result.floating(nominal);
    result.floating(enclosure.lower());
    result.floating(enclosure.upper());
    key.result_bytes = result.take();
    canonical_writer causes;
    causes.floating(contributors.inherited_a); causes.floating(contributors.inherited_b);
    causes.floating(contributors.machine_floor); causes.floating(contributors.construction);
    causes.floating(contributors.conditioning); causes.floating(contributors.conversion);
    causes.floating(contributors.prior_cleanup); causes.floating(contributors.current_cleanup);
    key.contributor_bytes = causes.take();
}

} // namespace precision_detail

template<class T, class I>
boolean_outcome<source_bounded_value_batch<T>> import_source_bounded_values(
    const precision_context<T> &context, const immutable_source_mesh<T,I> &source) {
    if (!context.owner().anchor || !context.ordinary_success_eligible() || !verify_source(source) ||
        source.coordinate_bits().size() % 3 != 0) {
        return boolean_outcome<source_bounded_value_batch<T>>::failure(
            precision_bootstrap_error(30011, bounded_boolean_error_category::internal_invariant_error,
                                      10, "source bounded-value import rejected"));
    }
    const auto &scale = source.operand() == operand_id::a
        ? context.operand_a_scale() : context.operand_b_scale();
    if (scale.source_digest != source.digest()) {
        return boolean_outcome<source_bounded_value_batch<T>>::failure(
            precision_bootstrap_error(30012, bounded_boolean_error_category::internal_invariant_error,
                                      10, "source bounded-value digest mismatch"));
    }
    const T radial = source.operand() == operand_id::a
        ? context.effective_input_precision_a() : context.effective_input_precision_b();
    const T declared = source.operand() == operand_id::a
        ? context.declared_input_precision_a() : context.declared_input_precision_b();
    if (!finite_bits(radial) || radial < T(0) || !finite_bits(declared) || declared < T(0) ||
        source.vertex_count() > (std::numeric_limits<std::uint64_t>::max() >> 2)) {
        return boolean_outcome<source_bounded_value_batch<T>>::failure(
            precision_bootstrap_error(30013, bounded_boolean_error_category::invalid_tolerance,
                                      10, "source enclosure is not representable"));
    }

    source_bounded_value_batch<T> out(context.owner(), source.operand());
    out.points.reserve(source.vertex_count());
    const std::uint64_t base = precision_detail::source_identity_base(source.operand());
    for (std::size_t vertex = 0; vertex < source.vertex_count(); ++vertex) {
        bounded_point3<T> point;
        point.owner = context.owner();
        point.coordinates.owner = context.owner();
        point.provenance = provenance_id(base + vertex);
        point.lineage = geometric_lineage_id(base + vertex);
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const T nominal = from_bits<T>(source.coordinate_bits()[vertex * 3 + axis]);
            const auto lower = directed_subtract(nominal, radial);
            const auto upper = directed_add(nominal, radial);
            if (!lower || !upper) {
                return boolean_outcome<source_bounded_value_batch<T>>::failure(
                    precision_bootstrap_error(30013, bounded_boolean_error_category::invalid_tolerance,
                                              10, "source enclosure is not representable"));
            }
            auto enclosure = finite_interval<T>::create(lower.value.lower, upper.value.upper);
            if (!enclosure) {
                return boolean_outcome<source_bounded_value_batch<T>>::failure(
                    precision_bootstrap_error(30013, bounded_boolean_error_category::invalid_tolerance,
                                              10, "source enclosure is not representable"));
            }
            auto &component = point.coordinates.components[axis];
            component.rounded_nominal = nominal;
            component.uncertainty_enclosure = *enclosure;
            component.identity.owner = context.owner();
            component.identity.value = bounded_value_id(base + vertex * 4 + axis);
            component.identity.provenance = point.provenance;
            component.identity.lineage = point.lineage;
            component.contributors.machine_floor = static_cast<double>(scale.machine_floor);
            if (source.operand() == operand_id::a)
                component.contributors.inherited_a = static_cast<double>(declared);
            else
                component.contributors.inherited_b = static_cast<double>(declared);
            precision_trace_key key;
            key.operation_code = static_cast<std::uint16_t>(rounded_operation_code::source_import);
            key.provenance = {point.provenance.ordinal()};
            precision_detail::encode_source_scalar_trace(key, nominal, *enclosure,
                                                         component.contributors);
            const auto node = out.trace.append(std::move(key), component.contributors);
            component.identity.trace_root = node.ordinal;
        }
        point.coordinates.radial_error_upper = radial;
        local_bounded_value<bounded_point3<T>> local;
        local.owner = context.owner();
        local.local_id = task_local_id<bounded_value_tag>{static_cast<std::uint64_t>(vertex)};
        local.trace_node = task_local_id<precision_trace_node_tag>{
            point.coordinates.components[2].identity.trace_root};
        local.value = std::move(point);
        out.points.push_back(std::move(local));
    }
    return boolean_outcome<source_bounded_value_batch<T>>::success(std::move(out));
}

namespace precision_detail {

template<class T>
std::vector<std::uint8_t> encode_context_payload(const precision_context<T> &context) {
    canonical_writer writer;
    writer.u32(0x33434759U); // YGC3
    writer.u16(precision_codec_v1);
    writer.u16(context.schema_version());
    writer.u16(context.provider_version());
    writer.u16(context.scalar_profile_version());
    writer.u16(context.arithmetic_profile_version());
    writer.u8(context.scalar_bytes());
    writer.u8(context.index_bytes());
    writer.floating(context.tolerance());
    writer.floating(context.declared_input_precision_a());
    writer.floating(context.declared_input_precision_b());
    writer.floating(context.effective_input_precision_a());
    writer.floating(context.effective_input_precision_b());
    writer.floating(context.required_machine_floor());
    writer.boolean(context.ordinary_success_eligible());
    encode_scale(writer, context.operand_a_scale());
    encode_scale(writer, context.operand_b_scale());
    encode_scale(writer, context.global_scale());
    encode_digest<T>(writer, context.source_digest());
    encode_digest<T>(writer, context.replay_digest());
    encode_digest<T>(writer, context.boolean_context_digest());
    encode_digest<T>(writer, context.preflight_digest());
    return writer.take();
}

} // namespace precision_detail

template<class T, class I>
boolean_outcome<std::shared_ptr<const precision_context<T>>> build_precision_context(
    const precision_preflight<T> &preflight,
    const boolean_context<T,I> &context,
    const precision_runtime_capabilities &capabilities = {}) {
    if (capabilities.version != 1 || capabilities.provider_version != precision_context_provider_v1 ||
        capabilities.reserved != 0 ||
        !precision_detail::runtime_precision_profile_qualified<T,I>() ||
        (capabilities.require_nearest_even && std::fegetround() != FE_TONEAREST)) {
        return boolean_outcome<std::shared_ptr<const precision_context<T>>>::failure(
            precision_bootstrap_error(30001, bounded_boolean_error_category::unsupported_platform, 8,
                                      "precision runtime profile is unavailable"));
    }
    if (!context.owner.anchor || (capabilities.expected_owner &&
        !context.owner.same_owner(*capabilities.expected_owner))) {
        return boolean_outcome<std::shared_ptr<const precision_context<T>>>::failure(
            precision_bootstrap_error(30010, bounded_boolean_error_category::internal_invariant_error, 8,
                                      "precision context owner mismatch"));
    }
    if (!verify_context(context) || !context.sources || context.precision.version != 1 ||
        preflight.schema_version != precision_bootstrap_schema_v1 ||
        preflight.provider_version != precision_bootstrap_provider_v1 ||
        preflight.scalar_profile_version != precision_scalar_profile_v1 ||
        preflight.arithmetic_profile_version != precision_arithmetic_profile_v1 ||
        preflight.scalar_bytes != sizeof(T) || preflight.index_bytes != sizeof(I) ||
        preflight.reserved != 0 || !preflight.ordinary_success_eligible ||
        preflight.source_digest != context.input_digest ||
        preflight.replay_digest != context.replay_digest ||
        preflight.tolerance != context.options.tolerance ||
        preflight.declared_input_precision_a != context.options.input_precision_a ||
        preflight.declared_input_precision_b != context.options.input_precision_b ||
        preflight.required_machine_floor != context.precision.machine_floor ||
        preflight.source_digest != context.precision.source_digest ||
        preflight.canonical_bytes != precision_detail::encode_preflight_payload(preflight) ||
        preflight.digest != sha256::digest(preflight.canonical_bytes) ||
        !precision_detail::verify_preflight_against_frozen_sources(
            preflight, *context.sources, context.options, context.replay_digest)) {
        return boolean_outcome<std::shared_ptr<const precision_context<T>>>::failure(
            precision_bootstrap_error(30009, bounded_boolean_error_category::internal_invariant_error, 8,
                                      "precision bootstrap/frozen-context mismatch"));
    }

    auto result = std::make_shared<precision_context<T>>();
    result->index_bytes_ = preflight.index_bytes;
    result->tolerance_ = preflight.tolerance;
    result->declared_input_precision_a_ = preflight.declared_input_precision_a;
    result->declared_input_precision_b_ = preflight.declared_input_precision_b;
    result->effective_input_precision_a_ = preflight.effective_input_precision_a;
    result->effective_input_precision_b_ = preflight.effective_input_precision_b;
    result->required_machine_floor_ = preflight.required_machine_floor;
    result->ordinary_success_eligible_ = preflight.ordinary_success_eligible;
    result->operand_a_ = preflight.operand_a;
    result->operand_b_ = preflight.operand_b;
    result->global_ = preflight.global;
    result->owner_ = context.owner;
    result->source_digest_ = context.input_digest;
    result->replay_digest_ = context.replay_digest;
    result->boolean_context_digest_ = context.context_digest;
    result->preflight_digest_ = preflight.digest;
    result->canonical_bytes_ = precision_detail::encode_context_payload(*result);
    result->digest_ = sha256::digest(result->canonical_bytes_);
    return boolean_outcome<std::shared_ptr<const precision_context<T>>>::success(std::move(result));
}

} // namespace ygor::mesh_boolean::bounded
