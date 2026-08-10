#pragma once

#include "PrecisionContext.h"
#include "ConservativeBounds.h"
#include "PrecisionImport.h"
#include "ToleranceBudget.h"
#include "ConstructionConditioning.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr std::uint16_t precision_state_codec_v1 = 1;

template<class T>
struct decoded_precision_context {
    std::uint16_t schema_version = 0;
    std::uint16_t provider_version = 0;
    std::uint16_t scalar_profile_version = 0;
    std::uint16_t arithmetic_profile_version = 0;
    std::uint8_t scalar_bytes = 0;
    std::uint8_t index_bytes = 0;
    T tolerance = T(0);
    T declared_input_precision_a = T(0);
    T declared_input_precision_b = T(0);
    T effective_input_precision_a = T(0);
    T effective_input_precision_b = T(0);
    T required_machine_floor = T(0);
    bool ordinary_success_eligible = false;
    precision_scale_descriptor<T> operand_a{};
    precision_scale_descriptor<T> operand_b{};
    precision_scale_descriptor<T> global{};
    bounded_boolean_digest source_digest{};
    bounded_boolean_digest replay_digest{};
    bounded_boolean_digest boolean_context_digest{};
    bounded_boolean_digest preflight_digest{};
    bounded_boolean_digest digest{};
};

inline bounded_boolean_error precision_codec_error(std::uint32_t subcode, const char *summary) {
    return precision_bootstrap_error(subcode, bounded_boolean_error_category::input_contract_error,
                                     22, summary);
}

namespace precision_codec_detail {

template<class T>
bool valid_scale(const precision_scale_descriptor<T> &scale) noexcept {
    if (scale.has_values != (scale.coordinate_count != 0) || scale.coordinate_count % 3 != 0 ||
        !finite_bits(scale.maximum_absolute) || !finite_bits(scale.smallest_nonzero) ||
        !finite_bits(scale.machine_floor) || scale.maximum_absolute < T(0) ||
        scale.smallest_nonzero < T(0) || scale.machine_floor < T(0)) return false;

    T maximum_absolute = T(0);
    T largest_span = T(0);
    for (const auto &axis : scale.axis) {
        if (!finite_bits(axis.minimum) || !finite_bits(axis.maximum) ||
            !finite_bits(axis.maximum_absolute) || !finite_bits(axis.span) ||
            finite_numeric_less(axis.maximum, axis.minimum) || axis.maximum_absolute < T(0) ||
            axis.span < T(0)) return false;
        const T expected_absolute = std::max(std::fabs(axis.minimum), std::fabs(axis.maximum));
        const T expected_span = static_cast<T>(axis.maximum - axis.minimum);
        if (!finite_bits(expected_span) || axis.maximum_absolute != expected_absolute ||
            axis.span != expected_span || axis.all_identical != (axis.minimum == axis.maximum)) return false;
        maximum_absolute = std::max(maximum_absolute, axis.maximum_absolute);
        largest_span = std::max(largest_span, axis.span);
    }
    if (!scale.has_values) {
        return maximum_absolute == T(0) && scale.maximum_absolute == T(0) &&
               scale.smallest_nonzero == T(0) && scale.machine_floor == T(0) &&
               !scale.has_positive_zero && !scale.has_negative_zero && !scale.has_subnormal &&
               !scale.has_normal && !scale.mixed_magnitude && !scale.large_translation &&
               scale.normalization_exponent == 0;
    }
    if (scale.maximum_absolute != maximum_absolute ||
        ((scale.has_subnormal || scale.has_normal) != (scale.smallest_nonzero != T(0)))) return false;
    std::int16_t expected_exponent = 0;
    if (scale.maximum_absolute != T(0)) {
        const int normalized = -std::ilogb(scale.maximum_absolute);
        expected_exponent = static_cast<std::int16_t>(
            std::max<int>(std::numeric_limits<std::int16_t>::min(),
                          std::min<int>(std::numeric_limits<std::int16_t>::max(), normalized)));
    }
    const bool mixed = scale.smallest_nonzero != T(0) &&
        std::ilogb(scale.maximum_absolute) - std::ilogb(scale.smallest_nonzero) >
            std::numeric_limits<T>::digits;
    const bool translated = largest_span != T(0) && scale.maximum_absolute / T(2) > largest_span;
    return scale.normalization_exponent == expected_exponent && scale.mixed_magnitude == mixed &&
           scale.large_translation == translated;
}

template<class T>
bool valid_precision_scalars(T tolerance, T declared_a, T declared_b, T effective_a,
                             T effective_b, T floor, bool eligible) noexcept {
    if (!finite_bits(tolerance) || !finite_bits(declared_a) || !finite_bits(declared_b) ||
        !finite_bits(effective_a) || !finite_bits(effective_b) || !finite_bits(floor) ||
        tolerance < T(0) || declared_a < T(0) || declared_b < T(0) || effective_a < declared_a ||
        effective_b < declared_b || floor < T(0)) return false;
    return eligible == (effective_a <= tolerance && effective_b <= tolerance && floor <= tolerance);
}

inline bool read_digest(canonical_reader &reader, bounded_boolean_digest &digest) {
    std::vector<std::uint8_t> bytes;
    if (!reader.fixed_bytes(digest.bytes.size(), bytes)) return false;
    std::copy(bytes.begin(), bytes.end(), digest.bytes.begin());
    return true;
}

template<class T>
bool read_scale(canonical_reader &reader, precision_scale_descriptor<T> &scale) {
    if (!reader.u64(scale.coordinate_count) || !reader.boolean(scale.has_values)) return false;
    for (auto &axis : scale.axis) {
        if (!reader.floating(axis.minimum) || !reader.floating(axis.maximum) ||
            !reader.floating(axis.maximum_absolute) || !reader.floating(axis.span) ||
            !reader.boolean(axis.all_identical)) return false;
    }
    std::uint16_t exponent = 0;
    if (!reader.floating(scale.maximum_absolute) || !reader.floating(scale.smallest_nonzero) ||
        !reader.floating(scale.machine_floor) ||
        !reader.boolean(scale.has_positive_zero) || !reader.boolean(scale.has_negative_zero) ||
        !reader.boolean(scale.has_subnormal) || !reader.boolean(scale.has_normal) ||
        !reader.boolean(scale.mixed_magnitude) || !reader.boolean(scale.large_translation) ||
        !reader.u16(exponent) || !read_digest(reader, scale.source_digest)) return false;
    scale.normalization_exponent = static_cast<std::int16_t>(exponent);
    return true;
}

template<class T>
bool read_header(canonical_reader &reader, std::uint32_t expected_magic,
                 std::uint16_t &schema, std::uint16_t &provider,
                 std::uint16_t &scalar_profile, std::uint16_t &arithmetic_profile,
                 std::uint8_t &scalar_bytes, std::uint8_t &index_bytes) {
    std::uint32_t magic = 0;
    std::uint16_t codec = 0;
    return reader.u32(magic) && magic == expected_magic && reader.u16(codec) &&
           codec == precision_codec_v1 && reader.u16(schema) && reader.u16(provider) &&
           reader.u16(scalar_profile) && reader.u16(arithmetic_profile) &&
           reader.u8(scalar_bytes) && scalar_bytes == sizeof(T) &&
           reader.u8(index_bytes) && (index_bytes == 4 || index_bytes == 8);
}

} // namespace precision_codec_detail

template<class T>
std::vector<std::uint8_t> encode_precision_preflight(const precision_preflight<T> &record) {
    return precision_detail::encode_preflight_payload(record);
}

template<class T>
boolean_outcome<precision_preflight<T>> decode_precision_preflight(
    const std::vector<std::uint8_t> &bytes) {
    canonical_reader reader(bytes);
    precision_preflight<T> out;
    if (!precision_codec_detail::read_header<T>(reader, 0x33504759U,
            out.schema_version, out.provider_version, out.scalar_profile_version,
            out.arithmetic_profile_version, out.scalar_bytes, out.index_bytes) ||
        !reader.u32(out.reserved) ||
        !precision_codec_detail::read_scale(reader, out.operand_a) ||
        !precision_codec_detail::read_scale(reader, out.operand_b) ||
        !precision_codec_detail::read_scale(reader, out.global) ||
        !reader.floating(out.tolerance) ||
        !reader.floating(out.declared_input_precision_a) ||
        !reader.floating(out.declared_input_precision_b) ||
        !reader.floating(out.effective_input_precision_a) ||
        !reader.floating(out.effective_input_precision_b) ||
        !reader.floating(out.required_machine_floor) ||
        !reader.boolean(out.ordinary_success_eligible) ||
        !precision_codec_detail::read_digest(reader, out.source_digest) ||
        !precision_codec_detail::read_digest(reader, out.replay_digest) || !reader.complete()) {
        return boolean_outcome<precision_preflight<T>>::failure(
            precision_codec_error(30020, "malformed precision preflight encoding"));
    }
    if (out.schema_version != precision_bootstrap_schema_v1 ||
        out.provider_version != precision_bootstrap_provider_v1 ||
        out.scalar_profile_version != precision_scalar_profile_v1 ||
        out.arithmetic_profile_version != precision_arithmetic_profile_v1 || out.reserved != 0) {
        return boolean_outcome<precision_preflight<T>>::failure(
            precision_codec_error(30021, "unsupported precision preflight version"));
    }
    if (!precision_codec_detail::valid_scale(out.operand_a) ||
        !precision_codec_detail::valid_scale(out.operand_b) ||
        !precision_codec_detail::valid_scale(out.global) ||
        !precision_codec_detail::valid_precision_scalars(
            out.tolerance, out.declared_input_precision_a, out.declared_input_precision_b,
            out.effective_input_precision_a, out.effective_input_precision_b,
            out.required_machine_floor, out.ordinary_success_eligible)) {
        return boolean_outcome<precision_preflight<T>>::failure(
            precision_codec_error(30020, "semantically invalid precision preflight encoding"));
    }
    out.canonical_bytes = bytes;
    out.digest = sha256::digest(bytes);
    return boolean_outcome<precision_preflight<T>>::success(std::move(out));
}

template<class T>
std::vector<std::uint8_t> encode_precision_context(const precision_context<T> &context) {
    return precision_detail::encode_context_payload(context);
}

template<class T>
boolean_outcome<decoded_precision_context<T>> decode_precision_context(
    const std::vector<std::uint8_t> &bytes) {
    canonical_reader reader(bytes);
    decoded_precision_context<T> out;
    if (!precision_codec_detail::read_header<T>(reader, 0x33434759U,
            out.schema_version, out.provider_version, out.scalar_profile_version,
            out.arithmetic_profile_version, out.scalar_bytes, out.index_bytes) ||
        !reader.floating(out.tolerance) ||
        !reader.floating(out.declared_input_precision_a) ||
        !reader.floating(out.declared_input_precision_b) ||
        !reader.floating(out.effective_input_precision_a) ||
        !reader.floating(out.effective_input_precision_b) ||
        !reader.floating(out.required_machine_floor) ||
        !reader.boolean(out.ordinary_success_eligible) ||
        !precision_codec_detail::read_scale(reader, out.operand_a) ||
        !precision_codec_detail::read_scale(reader, out.operand_b) ||
        !precision_codec_detail::read_scale(reader, out.global) ||
        !precision_codec_detail::read_digest(reader, out.source_digest) ||
        !precision_codec_detail::read_digest(reader, out.replay_digest) ||
        !precision_codec_detail::read_digest(reader, out.boolean_context_digest) ||
        !precision_codec_detail::read_digest(reader, out.preflight_digest) || !reader.complete()) {
        return boolean_outcome<decoded_precision_context<T>>::failure(
            precision_codec_error(30022, "malformed precision context encoding"));
    }
    if (out.schema_version != precision_context_schema_v1 ||
        out.provider_version != precision_context_provider_v1 ||
        out.scalar_profile_version != precision_scalar_profile_v1 ||
        out.arithmetic_profile_version != precision_arithmetic_profile_v1) {
        return boolean_outcome<decoded_precision_context<T>>::failure(
            precision_codec_error(30023, "unsupported precision context version"));
    }
    if (!precision_codec_detail::valid_scale(out.operand_a) ||
        !precision_codec_detail::valid_scale(out.operand_b) ||
        !precision_codec_detail::valid_scale(out.global) ||
        !precision_codec_detail::valid_precision_scalars(
            out.tolerance, out.declared_input_precision_a, out.declared_input_precision_b,
            out.effective_input_precision_a, out.effective_input_precision_b,
            out.required_machine_floor, out.ordinary_success_eligible)) {
        return boolean_outcome<decoded_precision_context<T>>::failure(
            precision_codec_error(30022, "semantically invalid precision context encoding"));
    }
    out.digest = sha256::digest(bytes);
    return boolean_outcome<decoded_precision_context<T>>::success(std::move(out));
}

std::vector<std::uint8_t> encode_precision_trace(const precision_trace_snapshot &snapshot);
boolean_outcome<precision_trace_snapshot> decode_precision_trace(
    const std::vector<std::uint8_t> &bytes);

std::vector<std::uint8_t> encode_precision_ledger(const precision_ledger_snapshot &snapshot);
boolean_outcome<precision_ledger_snapshot> decode_precision_ledger(
    const std::vector<std::uint8_t> &bytes);

std::vector<std::uint8_t> encode_tolerance_budget(const tolerance_budget_snapshot &snapshot);
boolean_outcome<tolerance_budget_snapshot> decode_tolerance_budget(
    const std::vector<std::uint8_t> &bytes);

std::vector<std::uint8_t> encode_precision_import(const foreign_precision_provenance &record);
boolean_outcome<foreign_precision_provenance> decode_precision_import(
    const std::vector<std::uint8_t> &bytes);
boolean_outcome<foreign_precision_provenance> decode_precision_import(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner);

namespace precision_codec_detail {
inline std::vector<std::uint8_t> seal(std::vector<std::uint8_t> payload) {
    const auto digest = sha256::digest(payload);
    payload.insert(payload.end(), digest.bytes.begin(), digest.bytes.end());
    return payload;
}
inline bool unseal(const std::vector<std::uint8_t> &bytes, std::vector<std::uint8_t> &payload) {
    bounded_boolean_digest expected;
    if (bytes.size() < expected.bytes.size()) return false;
    payload.assign(bytes.begin(), bytes.end() - expected.bytes.size());
    std::copy(bytes.end() - expected.bytes.size(), bytes.end(), expected.bytes.begin());
    return sha256::digest(payload) == expected;
}
inline void encode_contributors(canonical_writer &writer, const uncertainty_contributors &value) {
    writer.floating(value.inherited_a); writer.floating(value.inherited_b);
    writer.floating(value.machine_floor); writer.floating(value.construction);
    writer.floating(value.conditioning); writer.floating(value.conversion);
    writer.floating(value.prior_cleanup); writer.floating(value.current_cleanup);
}
inline bool read_contributors(canonical_reader &reader, uncertainty_contributors &value) {
    return reader.floating(value.inherited_a) && reader.floating(value.inherited_b) &&
           reader.floating(value.machine_floor) && reader.floating(value.construction) &&
           reader.floating(value.conditioning) && reader.floating(value.conversion) &&
           reader.floating(value.prior_cleanup) && reader.floating(value.current_cleanup);
}

inline void write_owner_marker(canonical_writer &writer, const context_owner_token &owner) {
    writer.boolean(static_cast<bool>(owner.anchor));
}
inline bool read_owner_marker(canonical_reader &reader, const context_owner_token &owner,
                              context_owner_token &out) {
    bool bound = false;
    if (!reader.boolean(bound) || bound != static_cast<bool>(owner.anchor)) return false;
    out = owner;
    return true;
}
template<class T>
void write_scalar(canonical_writer &writer, const bounded_scalar<T> &value) {
    writer.floating(value.rounded_nominal);
    writer.floating(value.uncertainty_enclosure.lower());
    writer.floating(value.uncertainty_enclosure.upper());
    writer.u16(value.identity.schema_version); writer.u16(value.identity.provider_version);
    write_owner_marker(writer, value.identity.owner);
    writer.u64(value.identity.value.ordinal()); writer.u64(value.identity.provenance.ordinal());
    writer.u64(value.identity.lineage.ordinal()); writer.u64(value.identity.ledger_entry.ordinal());
    writer.u64(value.identity.trace_root); writer.u8(static_cast<std::uint8_t>(value.identity.publication));
    encode_contributors(writer, value.contributors);
}
template<class T>
bool read_scalar(canonical_reader &reader, const context_owner_token &owner, bounded_scalar<T> &value) {
    T lower{}, upper{}; std::uint64_t id = 0, provenance = 0, lineage = 0, ledger = 0;
    std::uint8_t publication = 0;
    if (!reader.floating(value.rounded_nominal) || !reader.floating(lower) || !reader.floating(upper) ||
        !reader.u16(value.identity.schema_version) || !reader.u16(value.identity.provider_version) ||
        !read_owner_marker(reader, owner, value.identity.owner) || !reader.u64(id) ||
        !reader.u64(provenance) || !reader.u64(lineage) || !reader.u64(ledger) ||
        !reader.u64(value.identity.trace_root) || !reader.u8(publication) ||
        !read_contributors(reader, value.contributors)) return false;
    const auto interval = finite_interval<T>::create(lower, upper);
    if (!interval) return false;
    value.uncertainty_enclosure = *interval;
    value.identity.value = bounded_value_id(id); value.identity.provenance = provenance_id(provenance);
    value.identity.lineage = geometric_lineage_id(lineage);
    value.identity.ledger_entry = precision_ledger_entry_id(ledger);
    value.identity.publication = static_cast<bounded_publication_state>(publication);
    return true;
}
template<class T>
void write_residual(canonical_writer &writer, const bounded_residual<T> &value) {
    write_owner_marker(writer, value.owner); write_scalar(writer, value.value);
    writer.floating(value.scale); writer.floating(value.comparison_boundary);
    writer.u8(static_cast<std::uint8_t>(value.disposition)); encode_contributors(writer, value.contributors);
}
template<class T>
bool read_residual(canonical_reader &reader, const context_owner_token &owner, bounded_residual<T> &value) {
    std::uint8_t disposition = 0;
    if (!read_owner_marker(reader, owner, value.owner) || !read_scalar(reader, owner, value.value) ||
        !reader.floating(value.scale) || !reader.floating(value.comparison_boundary) ||
        !reader.u8(disposition) || !read_contributors(reader, value.contributors)) return false;
    value.disposition = static_cast<residual_disposition>(disposition); return true;
}
template<class T>
void write_point(canonical_writer &writer, const bounded_point3<T> &value) {
    write_owner_marker(writer, value.owner); write_owner_marker(writer, value.coordinates.owner);
    for (const auto &component : value.coordinates.components) write_scalar(writer, component);
    writer.floating(value.coordinates.radial_error_upper); writer.u64(value.provenance.ordinal());
    writer.u64(value.lineage.ordinal());
}
template<class T>
bool read_point(canonical_reader &reader, const context_owner_token &owner, bounded_point3<T> &value) {
    std::uint64_t provenance = 0, lineage = 0;
    if (!read_owner_marker(reader, owner, value.owner) ||
        !read_owner_marker(reader, owner, value.coordinates.owner)) return false;
    for (auto &component : value.coordinates.components)
        if (!read_scalar(reader, owner, component)) return false;
    if (!reader.floating(value.coordinates.radial_error_upper) || !reader.u64(provenance) ||
        !reader.u64(lineage)) return false;
    value.provenance = provenance_id(provenance); value.lineage = geometric_lineage_id(lineage); return true;
}
} // namespace precision_codec_detail

template<class T>
std::vector<std::uint8_t> encode_bounded_value(const bounded_scalar<T> &value) {
    canonical_writer writer; writer.u32(0x56504759U); writer.u16(precision_state_codec_v1);
    writer.u8(sizeof(T)); precision_codec_detail::write_scalar(writer, value);
    return precision_codec_detail::seal(writer.take());
}

template<class T>
boolean_outcome<bounded_scalar<T>> decode_bounded_value(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner = {}) {
    std::vector<std::uint8_t> payload; bounded_scalar<T> out;
    if (!precision_codec_detail::unseal(bytes, payload))
        return boolean_outcome<bounded_scalar<T>>::failure(precision_codec_error(30036, "malformed bounded value encoding"));
    canonical_reader input(payload); std::uint32_t magic = 0; std::uint16_t codec = 0; std::uint8_t scalar = 0;
    if (!input.u32(magic) || magic != 0x56504759U || !input.u16(codec) || codec != precision_state_codec_v1 ||
        !input.u8(scalar) || scalar != sizeof(T) || !precision_codec_detail::read_scalar(input, owner, out) ||
        !input.complete() || !bounded_operations_detail::bounded_scalar_valid(out) ||
        !bounded_operations_detail::finite_contributors(out.contributors))
        return boolean_outcome<bounded_scalar<T>>::failure(precision_codec_error(30036, "malformed bounded value encoding"));
    return boolean_outcome<bounded_scalar<T>>::success(std::move(out));
}
template<class T> std::vector<std::uint8_t> encode_bounded_scalar(const bounded_scalar<T> &value) {
    return encode_bounded_value(value);
}
template<class T> boolean_outcome<bounded_scalar<T>> decode_bounded_scalar(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner = {}) {
    return decode_bounded_value<T>(bytes, owner);
}

template<class T>
std::vector<std::uint8_t> encode_directed_result(const directed_operation_result<T> &value) {
    canonical_writer writer; writer.u32(0x44504759U); writer.u16(precision_state_codec_v1); writer.u8(sizeof(T));
    writer.u8(static_cast<std::uint8_t>(value.status)); writer.floating(value.value.rounded);
    writer.floating(value.value.lower); writer.floating(value.value.upper);
    writer.u8(static_cast<std::uint8_t>(value.value.exactness));
    writer.u8(static_cast<std::uint8_t>(value.value.direction));
    writer.u8(static_cast<std::uint8_t>(value.value.operation)); writer.u64(value.value.residual_evidence);
    return precision_codec_detail::seal(writer.take());
}

template<class T>
boolean_outcome<directed_operation_result<T>> decode_directed_result(const std::vector<std::uint8_t> &bytes) {
    std::vector<std::uint8_t> payload; directed_operation_result<T> out;
    if (!precision_codec_detail::unseal(bytes, payload))
        return boolean_outcome<directed_operation_result<T>>::failure(precision_codec_error(30037, "malformed directed result encoding"));
    canonical_reader reader(payload); std::uint32_t magic = 0; std::uint16_t codec = 0; std::uint8_t scalar = 0;
    std::uint8_t status = 0, exactness = 0, direction = 0, operation = 0;
    if (!reader.u32(magic) || magic != 0x44504759U || !reader.u16(codec) || codec != precision_state_codec_v1 ||
        !reader.u8(scalar) || scalar != sizeof(T) || !reader.u8(status) || !reader.floating(out.value.rounded) ||
        !reader.floating(out.value.lower) || !reader.floating(out.value.upper) || !reader.u8(exactness) ||
        !reader.u8(direction) || !reader.u8(operation) || !reader.u64(out.value.residual_evidence) || !reader.complete())
        return boolean_outcome<directed_operation_result<T>>::failure(precision_codec_error(30037, "malformed directed result encoding"));
    out.status = static_cast<numeric_status>(status); out.value.exactness = static_cast<rounding_exactness>(exactness);
    out.value.direction = static_cast<residual_sign>(direction);
    out.value.operation = static_cast<rounded_operation_code>(operation);
    return boolean_outcome<directed_operation_result<T>>::success(out);
}
template<class T> std::vector<std::uint8_t> encode_directed_operation_result(
    const directed_operation_result<T> &value) { return encode_directed_result(value); }
template<class T> boolean_outcome<directed_operation_result<T>> decode_directed_operation_result(
    const std::vector<std::uint8_t> &bytes) { return decode_directed_result<T>(bytes); }

inline std::vector<std::uint8_t> encode_exact_relation(const exact_relation_evidence &value) {
    canonical_writer writer; writer.u32(0x45504759U); writer.u16(precision_state_codec_v1);
    writer.u16(value.schema_version); writer.u16(value.formula_code);
    precision_codec_detail::write_owner_marker(writer, value.owner); writer.u64(value.id.ordinal());
    writer.u8(static_cast<std::uint8_t>(value.status));
    writer.u32(static_cast<std::uint32_t>(value.normalization_exponent)); writer.u32(value.capacity_used);
    writer.u64(value.ordered_inputs.size());
    for (const auto input : value.ordered_inputs) writer.u64(input.ordinal());
    return precision_codec_detail::seal(writer.take());
}

inline boolean_outcome<exact_relation_evidence> decode_exact_relation(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner = {}) {
    std::vector<std::uint8_t> payload; exact_relation_evidence out;
    if (!precision_codec_detail::unseal(bytes, payload))
        return boolean_outcome<exact_relation_evidence>::failure(precision_codec_error(30038, "malformed exact relation encoding"));
    canonical_reader reader(payload); std::uint32_t magic = 0, exponent = 0; std::uint16_t codec = 0;
    std::uint64_t id = 0, count = 0; std::uint8_t status = 0;
    if (!reader.u32(magic) || magic != 0x45504759U || !reader.u16(codec) || codec != precision_state_codec_v1 ||
        !reader.u16(out.schema_version) || !reader.u16(out.formula_code) ||
        !precision_codec_detail::read_owner_marker(reader, owner, out.owner) || !reader.u64(id) ||
        !reader.u8(status) || !reader.u32(exponent) || !reader.u32(out.capacity_used) ||
        !reader.u64(count) || count > (1U << 20) || count > reader.remaining() / 8)
        return boolean_outcome<exact_relation_evidence>::failure(precision_codec_error(30038, "malformed exact relation encoding"));
    out.id = exact_relation_id(id); out.status = static_cast<exact_relation_status>(status);
    out.normalization_exponent = static_cast<std::int32_t>(exponent);
    for (std::uint64_t i = 0, input = 0; i < count; ++i) {
        if (!reader.u64(input)) return boolean_outcome<exact_relation_evidence>::failure(
            precision_codec_error(30038, "malformed exact relation encoding"));
        out.ordered_inputs.emplace_back(input);
    }
    if (!reader.complete()) return boolean_outcome<exact_relation_evidence>::failure(
        precision_codec_error(30038, "malformed exact relation encoding"));
    return boolean_outcome<exact_relation_evidence>::success(std::move(out));
}

template<class T>
std::vector<std::uint8_t> encode_predicate_result(const predicate_result<T> &value) {
    canonical_writer writer; writer.u32(0x52504759U); writer.u16(precision_state_codec_v1); writer.u8(sizeof(T));
    writer.u16(value.schema_version); precision_codec_detail::write_owner_marker(writer, value.owner);
    writer.sized_bytes(encode_bounded_value(value.rounded_and_bounded));
    writer.u8(static_cast<std::uint8_t>(value.bounded_sign));
    writer.sized_bytes(encode_exact_relation(value.exact_relation));
    writer.u8(static_cast<std::uint8_t>(value.disposition)); writer.floating(value.separation_margin);
    writer.floating(value.uncertainty_width); precision_codec_detail::encode_contributors(writer, value.contributors);
    writer.u64(value.trace_root); return precision_codec_detail::seal(writer.take());
}

template<class T>
boolean_outcome<predicate_result<T>> decode_predicate_result(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner = {}) {
    std::vector<std::uint8_t> payload, scalar_bytes, exact_bytes; predicate_result<T> out;
    if (!precision_codec_detail::unseal(bytes, payload))
        return boolean_outcome<predicate_result<T>>::failure(precision_codec_error(30039, "malformed predicate encoding"));
    canonical_reader reader(payload); std::uint32_t magic = 0; std::uint16_t codec = 0; std::uint8_t scalar = 0, sign = 0, disposition = 0;
    if (!reader.u32(magic) || magic != 0x52504759U || !reader.u16(codec) || codec != precision_state_codec_v1 ||
        !reader.u8(scalar) || scalar != sizeof(T) || !reader.u16(out.schema_version) ||
        !precision_codec_detail::read_owner_marker(reader, owner, out.owner) ||
        !reader.sized_bytes(scalar_bytes, reader.remaining()) || !reader.u8(sign) ||
        !reader.sized_bytes(exact_bytes, reader.remaining()) || !reader.u8(disposition) ||
        !reader.floating(out.separation_margin) || !reader.floating(out.uncertainty_width) ||
        !precision_codec_detail::read_contributors(reader, out.contributors) || !reader.u64(out.trace_root) || !reader.complete())
        return boolean_outcome<predicate_result<T>>::failure(precision_codec_error(30039, "malformed predicate encoding"));
    auto scalar_value = decode_bounded_value<T>(scalar_bytes, owner); auto exact = decode_exact_relation(exact_bytes, owner);
    if (!scalar_value.has_value() || !exact.has_value())
        return boolean_outcome<predicate_result<T>>::failure(precision_codec_error(30039, "malformed predicate encoding"));
    out.rounded_and_bounded = std::move(*scalar_value.value()); out.exact_relation = std::move(*exact.value());
    out.bounded_sign = static_cast<bounded_sign_status>(sign);
    out.disposition = static_cast<predicate_disposition>(disposition);
    return boolean_outcome<predicate_result<T>>::success(std::move(out));
}

inline std::vector<std::uint8_t> encode_failure(const bounded_boolean_error &error) {
    canonical_writer writer; writer.u32(0x46504759U); writer.u16(precision_state_codec_v1);
    writer.u16(error.version); writer.u8(static_cast<std::uint8_t>(error.category)); writer.u32(error.subcode);
    writer.u16(error.component); writer.u16(error.stage); writer.u32(error.checkpoint);
    for (const auto byte : error.context_digest.bytes) writer.u8(byte);
    for (const auto byte : error.replay_digest.bytes) writer.u8(byte);
    writer.u8(error.witness_count); for (const auto witness : error.witnesses) writer.u64(witness);
    return precision_codec_detail::seal(writer.take());
}

class decoded_failure_result final {
  public:
    decoded_failure_result() = default;
    explicit decoded_failure_result(bounded_boolean_error value) : value_(std::move(value)) {}
    bool has_value() const noexcept { return value_.has_value(); }
    explicit operator bool() const noexcept { return has_value(); }
    bounded_boolean_error *value() noexcept { return value_ ? &*value_ : nullptr; }
    const bounded_boolean_error *value() const noexcept { return value_ ? &*value_ : nullptr; }
    bounded_boolean_error &operator*() noexcept { return *value_; }
    const bounded_boolean_error &operator*() const noexcept { return *value_; }
  private:
    std::optional<bounded_boolean_error> value_;
};

inline decoded_failure_result decode_failure(const std::vector<std::uint8_t> &bytes) {
    std::vector<std::uint8_t> payload; bounded_boolean_error out; std::vector<std::uint8_t> digest;
    if (!precision_codec_detail::unseal(bytes, payload))
        return {};
    canonical_reader reader(payload); std::uint32_t magic = 0; std::uint16_t codec = 0; std::uint8_t category = 0;
    if (!reader.u32(magic) || magic != 0x46504759U || !reader.u16(codec) || codec != precision_state_codec_v1 ||
        !reader.u16(out.version) || !reader.u8(category) || !reader.u32(out.subcode) ||
        !reader.u16(out.component) || !reader.u16(out.stage) || !reader.u32(out.checkpoint) ||
        !reader.fixed_bytes(out.context_digest.bytes.size(), digest))
        return {};
    std::copy(digest.begin(), digest.end(), out.context_digest.bytes.begin());
    if (!reader.fixed_bytes(out.replay_digest.bytes.size(), digest))
        return {};
    std::copy(digest.begin(), digest.end(), out.replay_digest.bytes.begin());
    if (!reader.u8(out.witness_count) || out.witness_count > out.witnesses.size())
        return {};
    for (auto &witness : out.witnesses) if (!reader.u64(witness))
        return {};
    if (!reader.complete()) return {};
    out.category = static_cast<bounded_boolean_error_category>(category);
    out.summary = "decoded canonical bounded Boolean failure";
    return decoded_failure_result(out);
}
inline std::vector<std::uint8_t> encode_bounded_failure(const bounded_boolean_error &error) {
    return encode_failure(error);
}
inline decoded_failure_result decode_bounded_failure(
    const std::vector<std::uint8_t> &bytes) { return decode_failure(bytes); }

template<class T>
std::vector<std::uint8_t> encode_construction(const construction_conditioning<T> &value) {
    canonical_writer writer; writer.u32(0x4e504759U); writer.u16(precision_state_codec_v1); writer.u8(sizeof(T));
    writer.u16(value.schema_version); precision_codec_detail::write_owner_marker(writer, value.owner);
    writer.u64(value.id.ordinal()); writer.u8(static_cast<std::uint8_t>(value.kind));
    writer.u16(value.rounded_graph_code); writer.u16(value.exact_formula_code);
    precision_codec_detail::write_owner_marker(writer, value.parameter.owner);
    precision_codec_detail::write_scalar(writer, value.parameter.value);
    writer.u8(static_cast<std::uint8_t>(value.parameter.carrier));
    writer.u8(static_cast<std::uint8_t>(value.parameter.endpoints));
    writer.u8(static_cast<std::uint8_t>(value.parameter.domain)); writer.floating(value.parameter.domain_margin);
    writer.floating(value.denominator.lower()); writer.floating(value.denominator.upper());
    writer.sized_bytes(encode_exact_relation(value.denominator_relation));
    precision_codec_detail::write_residual(writer, value.carrier_residual);
    precision_codec_detail::write_residual(writer, value.support_residual);
    writer.boolean(value.has_constructed_point);
    if (value.has_constructed_point) {
        precision_codec_detail::write_point(writer, value.constructed_point);
        precision_codec_detail::write_residual(writer, value.residual_after);
    }
    precision_codec_detail::encode_contributors(writer, value.contributors);
    writer.floating(value.available_tolerance); writer.floating(value.required_precision);
    writer.floating(value.amplification_upper); writer.boolean(value.cancellation_detected);
    writer.u8(static_cast<std::uint8_t>(value.category)); writer.u64(value.trace_root);
    return precision_codec_detail::seal(writer.take());
}

template<class T>
boolean_outcome<construction_conditioning<T>> decode_construction(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner = {}) {
    std::vector<std::uint8_t> payload, exact_bytes; construction_conditioning<T> out;
    if (!precision_codec_detail::unseal(bytes, payload))
        return boolean_outcome<construction_conditioning<T>>::failure(precision_codec_error(30041, "malformed construction encoding"));
    canonical_reader reader(payload); std::uint32_t magic = 0; std::uint16_t codec = 0; std::uint8_t scalar = 0;
    std::uint64_t id = 0; std::uint8_t kind = 0, carrier = 0, endpoints = 0, domain = 0, category = 0;
    T lower{}, upper{};
    if (!reader.u32(magic) || magic != 0x4e504759U || !reader.u16(codec) || codec != precision_state_codec_v1 ||
        !reader.u8(scalar) || scalar != sizeof(T) || !reader.u16(out.schema_version) ||
        !precision_codec_detail::read_owner_marker(reader, owner, out.owner) || !reader.u64(id) ||
        !reader.u8(kind) || !reader.u16(out.rounded_graph_code) || !reader.u16(out.exact_formula_code) ||
        !precision_codec_detail::read_owner_marker(reader, owner, out.parameter.owner) ||
        !precision_codec_detail::read_scalar(reader, owner, out.parameter.value) || !reader.u8(carrier) ||
        !reader.u8(endpoints) || !reader.u8(domain) || !reader.floating(out.parameter.domain_margin) ||
        !reader.floating(lower) || !reader.floating(upper) || !reader.sized_bytes(exact_bytes, reader.remaining()) ||
        !precision_codec_detail::read_residual(reader, owner, out.carrier_residual) ||
        !precision_codec_detail::read_residual(reader, owner, out.support_residual) ||
        !reader.boolean(out.has_constructed_point))
        return boolean_outcome<construction_conditioning<T>>::failure(precision_codec_error(30041, "malformed construction encoding"));
    if (out.has_constructed_point &&
        (!precision_codec_detail::read_point(reader, owner, out.constructed_point) ||
         !precision_codec_detail::read_residual(reader, owner, out.residual_after)))
        return boolean_outcome<construction_conditioning<T>>::failure(precision_codec_error(30041, "malformed construction encoding"));
    if (!precision_codec_detail::read_contributors(reader, out.contributors) ||
        !reader.floating(out.available_tolerance) || !reader.floating(out.required_precision) ||
        !reader.floating(out.amplification_upper) || !reader.boolean(out.cancellation_detected) ||
        !reader.u8(category) || !reader.u64(out.trace_root) || !reader.complete())
        return boolean_outcome<construction_conditioning<T>>::failure(precision_codec_error(30041, "malformed construction encoding"));
    const auto denominator = finite_interval<T>::create(lower, upper);
    auto exact = decode_exact_relation(exact_bytes, owner);
    if (!denominator || !exact.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(
        precision_codec_error(30041, "malformed construction encoding"));
    out.id = construction_id(id); out.kind = static_cast<construction_kind>(kind);
    out.parameter.carrier = static_cast<parameter_carrier>(carrier);
    out.parameter.endpoints = static_cast<endpoint_convention>(endpoints);
    out.parameter.domain = static_cast<parameter_domain_status>(domain);
    out.denominator = *denominator; out.denominator_relation = std::move(*exact.value());
    out.category = static_cast<construction_category>(category);
    return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
}

template<class T>
std::vector<std::uint8_t> encode_conservative_bound(
    const conservative_bound_record<T> &record) {
    canonical_writer writer;
    writer.u32(0x42424759U); writer.u16(precision_state_codec_v1);
    writer.u16(record.schema_version); writer.u8(sizeof(T));
    precision_codec_detail::write_owner_marker(writer, record.bound.owner);
    writer.u64(record.bound.id.ordinal());
    for (const auto &axis : record.bound.axes) {
        writer.floating(axis.lower()); writer.floating(axis.upper());
    }
    writer.u64(record.bound.provenance.ordinal()); writer.u64(record.bound.lineage.ordinal());
    precision_codec_detail::encode_contributors(writer, record.bound.inflation);
    writer.u64(record.ordered_provenance.size());
    for (const auto value : record.ordered_provenance) writer.u64(value.ordinal());
    writer.u64(record.ordered_lineages.size());
    for (const auto value : record.ordered_lineages) writer.u64(value.ordinal());
    return precision_codec_detail::seal(writer.take());
}

template<class T>
boolean_outcome<conservative_bound_record<T>> decode_conservative_bound(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner = {}) {
    std::vector<std::uint8_t> payload;
    if (!precision_codec_detail::unseal(bytes, payload))
        return boolean_outcome<conservative_bound_record<T>>::failure(
            precision_codec_error(30030, "malformed conservative bound encoding"));
    canonical_reader reader(payload);
    conservative_bound_record<T> out;
    std::uint32_t magic = 0; std::uint16_t codec = 0; std::uint8_t scalar_bytes = 0;
    std::uint64_t id = 0, provenance = 0, lineage = 0, count = 0;
    if (!reader.u32(magic) || magic != 0x42424759U || !reader.u16(codec) ||
        codec != precision_state_codec_v1 || !reader.u16(out.schema_version) ||
        out.schema_version != 1 || !reader.u8(scalar_bytes) || scalar_bytes != sizeof(T) ||
        !precision_codec_detail::read_owner_marker(reader, owner, out.bound.owner) ||
        !reader.u64(id))
        return boolean_outcome<conservative_bound_record<T>>::failure(
            precision_codec_error(30030, "malformed conservative bound encoding"));
    out.bound.id = finite_bound_id(id);
    for (auto &axis : out.bound.axes) {
        T lower{}, upper{};
        if (!reader.floating(lower) || !reader.floating(upper))
            return boolean_outcome<conservative_bound_record<T>>::failure(
                precision_codec_error(30030, "malformed conservative bound encoding"));
        const auto interval = finite_interval<T>::create(lower, upper);
        if (!interval)
            return boolean_outcome<conservative_bound_record<T>>::failure(
                precision_codec_error(30031, "invalid conservative bound interval"));
        axis = *interval;
    }
    if (!reader.u64(provenance) || !reader.u64(lineage) ||
        !precision_codec_detail::read_contributors(reader, out.bound.inflation) ||
        !reader.u64(count) || count > reader.remaining() / 8)
        return boolean_outcome<conservative_bound_record<T>>::failure(
            precision_codec_error(30030, "malformed conservative bound encoding"));
    out.bound.provenance = provenance_id(provenance); out.bound.lineage = geometric_lineage_id(lineage);
    out.ordered_provenance.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0, value = 0; i < count; ++i) {
        if (!reader.u64(value)) return boolean_outcome<conservative_bound_record<T>>::failure(
            precision_codec_error(30030, "malformed conservative bound encoding"));
        out.ordered_provenance.emplace_back(value);
    }
    if (!reader.u64(count) || count > reader.remaining() / 8)
        return boolean_outcome<conservative_bound_record<T>>::failure(
            precision_codec_error(30030, "malformed conservative bound encoding"));
    out.ordered_lineages.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0, value = 0; i < count; ++i) {
        if (!reader.u64(value)) return boolean_outcome<conservative_bound_record<T>>::failure(
            precision_codec_error(30030, "malformed conservative bound encoding"));
        out.ordered_lineages.emplace_back(value);
    }
    if (!reader.complete()) return boolean_outcome<conservative_bound_record<T>>::failure(
        precision_codec_error(30030, "malformed conservative bound encoding"));
    return boolean_outcome<conservative_bound_record<T>>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
