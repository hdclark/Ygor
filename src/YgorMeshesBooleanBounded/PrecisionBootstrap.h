#pragma once

#include "Context.h"
#include "FloatingBits.h"
#include "InvocationSourcesVerifier.h"
#include "PlatformQualification.h"

#include <array>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr std::uint16_t precision_bootstrap_schema_v1 = 1;
inline constexpr std::uint16_t precision_bootstrap_provider_v1 = 1;
inline constexpr std::uint16_t precision_scalar_profile_v1 = 1;
inline constexpr std::uint16_t precision_arithmetic_profile_v1 = 1;
inline constexpr std::uint16_t precision_codec_v1 = 1;

struct precision_bootstrap_capabilities {
#ifdef YGOR_MESH_BOOLEAN_STRICT_FP_BUILD
    bool strict_build = YGOR_MESH_BOOLEAN_STRICT_FP_BUILD == 1;
#else
    bool strict_build = false;
#endif
    bool require_nearest_even = true;
    std::uint16_t version = 1;
    std::uint32_t reserved = 0;
};

template<class T>
struct precision_axis_scale {
    T minimum = T(0);
    T maximum = T(0);
    T maximum_absolute = T(0);
    T span = T(0);
    bool all_identical = true;
};

template<class T>
struct precision_scale_descriptor {
    std::array<precision_axis_scale<T>, 3> axis{};
    T maximum_absolute = T(0);
    T smallest_nonzero = T(0);
    T machine_floor = T(0);
    std::uint64_t coordinate_count = 0;
    bool has_values = false;
    bool has_positive_zero = false;
    bool has_negative_zero = false;
    bool has_subnormal = false;
    bool has_normal = false;
    bool mixed_magnitude = false;
    bool large_translation = false;
    std::int16_t normalization_exponent = 0;
    bounded_boolean_digest source_digest{};
};

template<class T>
struct precision_preflight {
    std::uint16_t schema_version = precision_bootstrap_schema_v1;
    std::uint16_t provider_version = precision_bootstrap_provider_v1;
    std::uint16_t scalar_profile_version = precision_scalar_profile_v1;
    std::uint16_t arithmetic_profile_version = precision_arithmetic_profile_v1;
    std::uint8_t scalar_bytes = sizeof(T);
    std::uint8_t index_bytes = 0;
    std::uint32_t reserved = 0;
    precision_scale_descriptor<T> operand_a{};
    precision_scale_descriptor<T> operand_b{};
    precision_scale_descriptor<T> global{};
    T tolerance = T(0);
    T declared_input_precision_a = T(0);
    T declared_input_precision_b = T(0);
    T effective_input_precision_a = T(0);
    T effective_input_precision_b = T(0);
    T required_machine_floor = T(0);
    bool ordinary_success_eligible = false;
    bounded_boolean_digest source_digest{};
    bounded_boolean_digest replay_digest{};
    std::vector<std::uint8_t> canonical_bytes;
    bounded_boolean_digest digest{};
};

template<class T, class I>
class pending_boolean_context_view {
  public:
    explicit pending_boolean_context_view(const pending_invocation<T,I> &pending) : pending_(pending) {}
    pending_boolean_context_view(pending_invocation<T,I> &&) = delete;
    const immutable_invocation_sources<T,I> &sources() const noexcept { return pending_.sources; }
    const bounded_boolean_options<T> &options() const noexcept { return pending_.options; }
    boolean_operation operation() const noexcept { return pending_.operation; }
    const bounded_boolean_digest &replay_digest() const noexcept { return pending_.replay_digest; }
    const pending_invocation<T,I> &underlying() const noexcept { return pending_; }
  private:
    pending_invocation<T,I> pending_;
};

inline bounded_boolean_error precision_bootstrap_error(
    std::uint32_t subcode,
    bounded_boolean_error_category category,
    std::uint32_t checkpoint,
    const char *summary) {
    bounded_boolean_error error;
    error.category = category;
    error.subcode = subcode;
    error.component = 3;
    error.stage = static_cast<std::uint16_t>(stage_id::precision_bootstrap);
    error.checkpoint = checkpoint;
    error.summary = summary;
    return error;
}

namespace precision_detail {

template<class T, class I>
bool runtime_precision_profile_qualified() noexcept {
    return supported_type_profile<T,I>() && runtime_floating_profile_qualified<T>() &&
           std::fegetround() == FE_TONEAREST;
}

template<class T>
T from_source_bits(scalar_bits_t<T> bits) noexcept {
    return from_bits<T>(bits);
}

template<class T>
T adjacent_gap(T value) noexcept {
    if (value == T(0)) return std::numeric_limits<T>::denorm_min();
    T adjacent = T(0);
    const bool outward = std::signbit(value) ? next_down_finite(value, adjacent)
                                             : next_up_finite(value, adjacent);
    if (!outward) {
        const bool inward = std::signbit(value) ? next_up_finite(value, adjacent)
                                                : next_down_finite(value, adjacent);
        if (!inward) return T(0);
    }
    const T gap = std::fabs(adjacent - value);
    return finite_bits(gap) ? gap : T(0);
}

template<class T>
bool checked_nonnegative_add(T a, T b, T &out) noexcept {
    if (!std::isfinite(a) || !std::isfinite(b) || a < T(0) || b < T(0)) return false;
    if (a > std::numeric_limits<T>::max() - b) return false;
    out = static_cast<T>(a + b);
    return std::isfinite(out);
}

template<class T>
bool finish_scale(precision_scale_descriptor<T> &out) noexcept {
    if (!out.has_values) return true;
    T largest_span = T(0);
    for (auto &axis : out.axis) {
        if (axis.maximum > T(0) && axis.minimum < T(0) &&
            axis.maximum > std::numeric_limits<T>::max() + axis.minimum) return false;
        axis.span = static_cast<T>(axis.maximum - axis.minimum);
        if (!std::isfinite(axis.span)) return false;
        largest_span = std::max(largest_span, axis.span);
    }
    if (out.maximum_absolute != T(0)) {
        const int exponent = std::ilogb(out.maximum_absolute);
        const int normalized = -exponent;
        out.normalization_exponent = static_cast<std::int16_t>(
            std::max<int>(std::numeric_limits<std::int16_t>::min(),
                          std::min<int>(std::numeric_limits<std::int16_t>::max(), normalized)));
    }
    if (out.smallest_nonzero != T(0)) {
        out.mixed_magnitude = std::ilogb(out.maximum_absolute) - std::ilogb(out.smallest_nonzero) >
                              std::numeric_limits<T>::digits;
    }
    out.large_translation = largest_span != T(0) && out.maximum_absolute / T(2) > largest_span;
    return true;
}

template<class T>
void encode_scale(canonical_writer &writer, const precision_scale_descriptor<T> &scale);

template<class T>
std::vector<std::uint8_t> encode_preflight_payload(const precision_preflight<T> &record);

template<class T, class I>
bool reconstruct_scale_checked(const immutable_source_mesh<T,I> *first,
                               const immutable_source_mesh<T,I> *second,
                               const bounded_boolean_digest &digest,
                               precision_scale_descriptor<T> &out) noexcept {
    out = {};
    out.source_digest = digest;
    std::size_t position = 0;
    const auto consume = [&](const immutable_source_mesh<T,I> *source) {
        if (!source) return true;
        for (const auto raw : source->coordinate_bits()) {
            const T value = from_bits<T>(raw);
            if (!finite_bits(value)) return false;
            const std::size_t axis_index = position % 3;
            auto &axis = out.axis[axis_index];
            if (!out.has_values || position < 3) {
                axis.minimum = value;
                axis.maximum = value;
            } else {
                axis.minimum = std::min(axis.minimum, value);
                axis.maximum = std::max(axis.maximum, value);
            }
            const T absolute = std::fabs(value);
            axis.maximum_absolute = std::max(axis.maximum_absolute, absolute);
            out.maximum_absolute = std::max(out.maximum_absolute, absolute);
            using traits = floating_bits_traits<T>;
            const auto magnitude = raw & ~traits::sign_mask;
            const auto exponent = raw & traits::exponent_mask;
            const auto fraction = raw & traits::fraction_mask;
            if (magnitude == 0) {
                out.has_negative_zero = out.has_negative_zero || (raw & traits::sign_mask) != 0;
                out.has_positive_zero = out.has_positive_zero || (raw & traits::sign_mask) == 0;
            } else {
                if (out.smallest_nonzero == T(0) || absolute < out.smallest_nonzero)
                    out.smallest_nonzero = absolute;
                out.has_subnormal = out.has_subnormal || (exponent == 0 && fraction != 0);
                out.has_normal = out.has_normal || exponent != 0;
            }
            T adjacent = T(0);
            const bool outward = std::signbit(value) ? next_down_finite(value, adjacent)
                                                     : next_up_finite(value, adjacent);
            if (!outward) {
                const bool inward = std::signbit(value) ? next_up_finite(value, adjacent)
                                                        : next_down_finite(value, adjacent);
                if (!inward) return false;
            }
            const T gap = std::fabs(adjacent - value);
            if (!finite_bits(gap)) return false;
            out.machine_floor = std::max(out.machine_floor, gap);
            out.has_values = true;
            ++position;
        }
        return true;
    };
    if (!consume(first) || !consume(second)) return false;
    out.coordinate_count = position;
    if (!out.has_values) return true;
    for (auto &axis : out.axis) axis.all_identical = axis.minimum == axis.maximum;
    return finish_scale(out);
}

template<class T>
bool exact_scale_equal(const precision_scale_descriptor<T> &left,
                       const precision_scale_descriptor<T> &right) {
    canonical_writer a;
    canonical_writer b;
    encode_scale(a, left);
    encode_scale(b, right);
    return a.bytes() == b.bytes();
}

template<class T, class I>
bool verify_preflight_against_frozen_sources(
    const precision_preflight<T> &record,
    const immutable_invocation_sources<T,I> &sources,
    const bounded_boolean_options<T> &options,
    const bounded_boolean_digest &replay_digest) {
    if (record.schema_version != precision_bootstrap_schema_v1 ||
        record.provider_version != precision_bootstrap_provider_v1 ||
        record.scalar_profile_version != precision_scalar_profile_v1 ||
        record.arithmetic_profile_version != precision_arithmetic_profile_v1 ||
        record.scalar_bytes != sizeof(T) || record.index_bytes != sizeof(I) || record.reserved != 0 ||
        !runtime_precision_profile_qualified<T,I>()) return false;

    precision_scale_descriptor<T> a, b, global;
    if (!reconstruct_scale_checked(&sources.a, static_cast<const immutable_source_mesh<T,I> *>(nullptr),
                                   sources.a.digest(), a) ||
        !reconstruct_scale_checked(&sources.b, static_cast<const immutable_source_mesh<T,I> *>(nullptr),
                                   sources.b.digest(), b) ||
        !reconstruct_scale_checked(&sources.a, &sources.b, sources.digest, global) ||
        !exact_scale_equal(record.operand_a, a) || !exact_scale_equal(record.operand_b, b) ||
        !exact_scale_equal(record.global, global)) return false;

    T effective_a = T(0), effective_b = T(0);
    if (!checked_nonnegative_add(options.input_precision_a, a.machine_floor, effective_a) ||
        !checked_nonnegative_add(options.input_precision_b, b.machine_floor, effective_b)) return false;
    const T floor = std::max(a.machine_floor, b.machine_floor);
    const bool eligible = effective_a <= options.tolerance && effective_b <= options.tolerance &&
                          floor <= options.tolerance;
    const auto bytes = encode_preflight_payload(record);
    return record.tolerance == options.tolerance &&
           record.declared_input_precision_a == options.input_precision_a &&
           record.declared_input_precision_b == options.input_precision_b &&
           record.effective_input_precision_a == effective_a &&
           record.effective_input_precision_b == effective_b &&
           record.required_machine_floor == floor &&
           record.ordinary_success_eligible == eligible && record.source_digest == sources.digest &&
           record.replay_digest == replay_digest && record.canonical_bytes == bytes &&
           record.digest == sha256::digest(bytes);
}

template<class T, class I>
boolean_outcome<precision_scale_descriptor<T>> scan_scale(const immutable_source_mesh<T,I> &source) {
    precision_scale_descriptor<T> out;
    out.source_digest = source.digest();
    const auto &bits = source.coordinate_bits();
    out.coordinate_count = bits.size();
    for (std::size_t position = 0; position < bits.size(); ++position) {
        const auto raw = bits[position];
        const T value = from_source_bits<T>(raw);
        if (!finite_bits(value)) {
            auto error = precision_bootstrap_error(30005,
                bounded_boolean_error_category::input_contract_error, 3,
                "non-finite source coordinate");
            error.witnesses[0] = static_cast<std::uint64_t>(source.operand());
            error.witnesses[1] = position;
            error.witness_count = 2;
            return boolean_outcome<precision_scale_descriptor<T>>::failure(error);
        }
        const std::size_t axis_index = position % 3;
        auto &axis = out.axis[axis_index];
        if (!out.has_values || position < 3) {
            axis.minimum = value;
            axis.maximum = value;
        } else {
            axis.minimum = std::min(axis.minimum, value);
            axis.maximum = std::max(axis.maximum, value);
        }
        const T absolute = std::fabs(value);
        axis.maximum_absolute = std::max(axis.maximum_absolute, absolute);
        out.maximum_absolute = std::max(out.maximum_absolute, absolute);
        using traits = floating_bits_traits<T>;
        const auto magnitude_bits = raw & ~traits::sign_mask;
        const auto exponent_bits = raw & traits::exponent_mask;
        const auto fraction_bits = raw & traits::fraction_mask;
        if (magnitude_bits == 0) {
            out.has_negative_zero = out.has_negative_zero || (raw & traits::sign_mask) != 0;
            out.has_positive_zero = out.has_positive_zero || (raw & traits::sign_mask) == 0;
        } else {
            if (out.smallest_nonzero == T(0) || absolute < out.smallest_nonzero) out.smallest_nonzero = absolute;
            out.has_subnormal = out.has_subnormal || (exponent_bits == 0 && fraction_bits != 0);
            out.has_normal = out.has_normal || exponent_bits != 0;
        }
        out.machine_floor = std::max(out.machine_floor, adjacent_gap(value));
        out.has_values = true;
    }
    for (auto &axis : out.axis) axis.all_identical = !out.has_values || axis.minimum == axis.maximum;
    if (!finish_scale(out)) return boolean_outcome<precision_scale_descriptor<T>>::failure(
        precision_bootstrap_error(30007, bounded_boolean_error_category::invalid_tolerance, 4,
                                  "source scale is not finitely representable"));
    return boolean_outcome<precision_scale_descriptor<T>>::success(std::move(out));
}

template<class T>
void encode_digest(canonical_writer &writer, const bounded_boolean_digest &digest) {
    for (std::uint8_t byte : digest.bytes) writer.u8(byte);
}

template<class T>
void encode_scale(canonical_writer &writer, const precision_scale_descriptor<T> &scale) {
    writer.u64(scale.coordinate_count);
    writer.boolean(scale.has_values);
    for (const auto &axis : scale.axis) {
        writer.floating(axis.minimum);
        writer.floating(axis.maximum);
        writer.floating(axis.maximum_absolute);
        writer.floating(axis.span);
        writer.boolean(axis.all_identical);
    }
    writer.floating(scale.maximum_absolute);
    writer.floating(scale.smallest_nonzero);
    writer.floating(scale.machine_floor);
    writer.boolean(scale.has_positive_zero);
    writer.boolean(scale.has_negative_zero);
    writer.boolean(scale.has_subnormal);
    writer.boolean(scale.has_normal);
    writer.boolean(scale.mixed_magnitude);
    writer.boolean(scale.large_translation);
    writer.u16(static_cast<std::uint16_t>(scale.normalization_exponent));
    encode_digest<T>(writer, scale.source_digest);
}

template<class T>
std::vector<std::uint8_t> encode_preflight_payload(const precision_preflight<T> &record) {
    canonical_writer writer;
    writer.u32(0x33504759U); // YGP3
    writer.u16(precision_codec_v1);
    writer.u16(record.schema_version);
    writer.u16(record.provider_version);
    writer.u16(record.scalar_profile_version);
    writer.u16(record.arithmetic_profile_version);
    writer.u8(record.scalar_bytes);
    writer.u8(record.index_bytes);
    writer.u32(record.reserved);
    encode_scale(writer, record.operand_a);
    encode_scale(writer, record.operand_b);
    encode_scale(writer, record.global);
    writer.floating(record.tolerance);
    writer.floating(record.declared_input_precision_a);
    writer.floating(record.declared_input_precision_b);
    writer.floating(record.effective_input_precision_a);
    writer.floating(record.effective_input_precision_b);
    writer.floating(record.required_machine_floor);
    writer.boolean(record.ordinary_success_eligible);
    encode_digest<T>(writer, record.source_digest);
    encode_digest<T>(writer, record.replay_digest);
    return writer.take();
}

template<class T, class I>
boolean_outcome<precision_preflight<T>> preflight_impl(
    const pending_invocation<T,I> &pending,
    const precision_bootstrap_capabilities &capabilities) {
    static_assert(std::is_same_v<T,float> || std::is_same_v<T,double>);
    static_assert(std::is_same_v<I,std::uint32_t> || std::is_same_v<I,std::uint64_t>);
    if (capabilities.version != 1 || capabilities.reserved != 0 || !capabilities.strict_build ||
        !precision_detail::runtime_precision_profile_qualified<T,I>() ||
        (capabilities.require_nearest_even && std::fegetround() != FE_TONEAREST)) {
        return boolean_outcome<precision_preflight<T>>::failure(
            precision_bootstrap_error(30001, bounded_boolean_error_category::unsupported_platform, 1,
                                      "precision floating-point profile is unavailable"));
    }
    if (!verify_pending(pending) || !verify_invocation_sources(pending.sources)) {
        return boolean_outcome<precision_preflight<T>>::failure(
            precision_bootstrap_error(30002, bounded_boolean_error_category::internal_invariant_error, 1,
                                      "pending precision input failed verification"));
    }
    auto a = scan_scale(pending.sources.a);
    if (!a.has_value()) return boolean_outcome<precision_preflight<T>>::failure(*a.error());
    auto b = scan_scale(pending.sources.b);
    if (!b.has_value()) return boolean_outcome<precision_preflight<T>>::failure(*b.error());

    precision_preflight<T> out;
    out.index_bytes = sizeof(I);
    out.operand_a = std::move(*a.value());
    out.operand_b = std::move(*b.value());
    out.tolerance = pending.options.tolerance;
    out.declared_input_precision_a = pending.options.input_precision_a;
    out.declared_input_precision_b = pending.options.input_precision_b;
    out.required_machine_floor = std::max(out.operand_a.machine_floor, out.operand_b.machine_floor);
    if (!checked_nonnegative_add(out.declared_input_precision_a, out.operand_a.machine_floor,
                                 out.effective_input_precision_a) ||
        !checked_nonnegative_add(out.declared_input_precision_b, out.operand_b.machine_floor,
                                 out.effective_input_precision_b)) {
        return boolean_outcome<precision_preflight<T>>::failure(
            precision_bootstrap_error(30007, bounded_boolean_error_category::invalid_tolerance, 5,
                                      "effective input precision is not representable"));
    }

    // Re-scan the ordered concatenation for a true global descriptor.
    out.global = out.operand_a;
    out.global.source_digest = pending.sources.digest;
    out.global.coordinate_count = out.operand_a.coordinate_count + out.operand_b.coordinate_count;
    if (!out.operand_a.has_values) out.global = out.operand_b;
    else if (out.operand_b.has_values) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            out.global.axis[axis].minimum = std::min(out.operand_a.axis[axis].minimum, out.operand_b.axis[axis].minimum);
            out.global.axis[axis].maximum = std::max(out.operand_a.axis[axis].maximum, out.operand_b.axis[axis].maximum);
            out.global.axis[axis].maximum_absolute = std::max(out.operand_a.axis[axis].maximum_absolute, out.operand_b.axis[axis].maximum_absolute);
            out.global.axis[axis].all_identical = out.global.axis[axis].minimum == out.global.axis[axis].maximum;
        }
        out.global.maximum_absolute = std::max(out.operand_a.maximum_absolute, out.operand_b.maximum_absolute);
        out.global.smallest_nonzero = out.operand_a.smallest_nonzero == T(0) ? out.operand_b.smallest_nonzero :
            (out.operand_b.smallest_nonzero == T(0) ? out.operand_a.smallest_nonzero : std::min(out.operand_a.smallest_nonzero, out.operand_b.smallest_nonzero));
        out.global.machine_floor = out.required_machine_floor;
        out.global.has_values = true;
        out.global.has_positive_zero = out.operand_a.has_positive_zero || out.operand_b.has_positive_zero;
        out.global.has_negative_zero = out.operand_a.has_negative_zero || out.operand_b.has_negative_zero;
        out.global.has_subnormal = out.operand_a.has_subnormal || out.operand_b.has_subnormal;
        out.global.has_normal = out.operand_a.has_normal || out.operand_b.has_normal;
    }
    out.global.source_digest = pending.sources.digest;
    out.global.coordinate_count = out.operand_a.coordinate_count + out.operand_b.coordinate_count;
    if (!finish_scale(out.global)) return boolean_outcome<precision_preflight<T>>::failure(
        precision_bootstrap_error(30007, bounded_boolean_error_category::invalid_tolerance, 4,
                                  "global source scale is not finitely representable"));

    out.ordinary_success_eligible = out.effective_input_precision_a <= out.tolerance &&
                                    out.effective_input_precision_b <= out.tolerance &&
                                    out.required_machine_floor <= out.tolerance;
    out.source_digest = pending.sources.digest;
    out.replay_digest = pending.replay_digest;
    out.canonical_bytes = encode_preflight_payload(out);
    out.digest = sha256::digest(out.canonical_bytes);
    return boolean_outcome<precision_preflight<T>>::success(std::move(out));
}

} // namespace precision_detail

template<class T, class I>
boolean_outcome<precision_preflight<T>> preflight_precision(
    const pending_invocation<T,I> &pending,
    const precision_bootstrap_capabilities &capabilities = {}) {
    return precision_detail::preflight_impl(pending, capabilities);
}

template<class T, class I>
boolean_outcome<precision_preflight<T>> preflight_precision(
    const pending_boolean_context_view<T,I> &pending,
    const precision_bootstrap_capabilities &capabilities = {}) {
    return precision_detail::preflight_impl(pending.underlying(), capabilities);
}

template<class T>
precision_bootstrap_record<T> make_precision_bootstrap_record(const precision_preflight<T> &preflight) {
    precision_bootstrap_record<T> out;
    out.version = 1;
    out.machine_floor = preflight.required_machine_floor;
    out.input_precision_a = preflight.declared_input_precision_a;
    out.input_precision_b = preflight.declared_input_precision_b;
    out.source_digest = preflight.source_digest;
    return out;
}

} // namespace ygor::mesh_boolean::bounded
