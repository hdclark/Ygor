#pragma once

#include "PrecisionCodec.h"
#include "ExactFloatExpansion.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr std::uint16_t precision_verifier_v1 = 1;

namespace precision_verifier_detail {

inline bool finite_contributors(const uncertainty_contributors &value) {
    const double values[]{value.inherited_a, value.inherited_b, value.machine_floor,
                          value.construction, value.conditioning, value.conversion,
                          value.prior_cleanup, value.current_cleanup};
    for (const double item : values) if (!std::isfinite(item) || item < 0.0) return false;
    return true;
}
template<class T>
bool same_computed_scalar(const bounded_scalar<T> &a, const bounded_scalar<T> &b) {
    if (to_bits(a.rounded_nominal) != to_bits(b.rounded_nominal) ||
        to_bits(a.uncertainty_enclosure.lower()) != to_bits(b.uncertainty_enclosure.lower()) ||
        to_bits(a.uncertainty_enclosure.upper()) != to_bits(b.uncertainty_enclosure.upper())) return false;
    canonical_writer left, right;
    precision_codec_detail::encode_contributors(left, a.contributors);
    precision_codec_detail::encode_contributors(right, b.contributors);
    return left.bytes() == right.bytes();
}

inline bool outward_add(double a, double b, double &result) {
    const auto value = directed_add(a, b);
    if (!value) return false;
    result = value.value.upper;
    return true;
}

inline void trace_structural_bytes(canonical_writer &writer,
                                   const committed_precision_trace_node &node,
                                   const std::vector<std::vector<std::uint8_t>> &parents) {
    writer.u16(node.key.operation_code); writer.u16(node.key.exact_formula_code);
    writer.u64(node.key.parents.size());
    for (const auto parent : node.key.parents) writer.sized_bytes(parents[parent]);
    writer.sized_bytes(node.key.result_bytes); writer.sized_bytes(node.key.contributor_bytes);
    writer.u64(node.key.provenance.size()); for (auto value : node.key.provenance) writer.u64(value);
    precision_codec_detail::encode_contributors(writer, node.contributors);
}

inline bool valid_trace_operation(const precision_trace_key &key) {
    const auto descriptor = rounded_operation_descriptor_for(
        static_cast<rounded_operation_code>(key.operation_code));
    if (descriptor.code == rounded_operation_code::invalid ||
        !valid_exact_formula_code(key.exact_formula_code) ||
        key.parents.size() < descriptor.minimum_arity || key.parents.size() > descriptor.maximum_arity)
        return false;
    for (std::size_t i = 1; i < key.provenance.size(); ++i)
        if (key.provenance[i - 1] >= key.provenance[i]) return false;
    return true;
}

inline bool valid_budget_proposal(const tolerance_budget_proposal &proposal) {
    if (proposal.ordered_lineages.empty() ||
        proposal.ordered_lineages.size() != proposal.requested_costs.size() ||
        proposal.certificate.schema_version != 1 ||
        proposal.certificate.units != length_unit_kind::coordinate_length ||
        proposal.certificate.evidence.empty() || proposal.transaction_owner == 0 ||
        !std::isfinite(proposal.certificate.upper_length) || proposal.certificate.upper_length < 0.0)
        return false;
    if (proposal.before_evidence.schema_version != 1 ||
        proposal.before_evidence.transaction_owner != proposal.transaction_owner ||
        proposal.before_evidence.cumulative_lengths.size() != proposal.ordered_lineages.size() ||
        proposal.before_evidence.evidence.empty() || proposal.authorization.schema_version != 1 ||
        proposal.authorization.operation != proposal.operation ||
        proposal.authorization.transaction_owner != proposal.transaction_owner ||
        proposal.authorization.evidence.empty()) return false;
    if (proposal.owner.anchor &&
        (!proposal.before_evidence.owner.same_owner(proposal.owner) ||
         !proposal.authorization.owner.same_owner(proposal.owner))) return false;
    canonical_writer authorization_writer;
    authorization_writer.u16(proposal.authorization.schema_version);
    authorization_writer.u64(proposal.authorization.id.ordinal());
    authorization_writer.u8(static_cast<std::uint8_t>(proposal.authorization.operation));
    authorization_writer.u64(proposal.authorization.transaction_owner);
    authorization_writer.sized_bytes(proposal.authorization.evidence);
    const auto authorization_digest = sha256::digest(authorization_writer.bytes());
    if (proposal.authorization.authentication !=
        std::vector<std::uint8_t>(authorization_digest.bytes.begin(), authorization_digest.bytes.end()))
        return false;
    bool compatible = false;
    switch (proposal.operation) {
    case cleanup_operation_kind::move_vertex:
        compatible = proposal.certificate.kind == length_certificate_kind::vertex_displacement ||
                     proposal.certificate.kind == length_certificate_kind::swept_displacement; break;
    case cleanup_operation_kind::collapse_feature:
        compatible = proposal.certificate.kind == length_certificate_kind::feature_thickness_or_clearance ||
                     proposal.certificate.kind == length_certificate_kind::swept_displacement; break;
    case cleanup_operation_kind::remove_patch:
        compatible = proposal.certificate.kind == length_certificate_kind::patch_hausdorff_deviation ||
                     proposal.certificate.kind == length_certificate_kind::feature_thickness_or_clearance; break;
    case cleanup_operation_kind::remove_component:
        compatible = proposal.certificate.kind == length_certificate_kind::whole_component_deviation; break;
    }
    if (!compatible) return false;
    for (std::size_t i = 0; i < proposal.requested_costs.size(); ++i)
        if (!std::isfinite(proposal.requested_costs[i]) || proposal.requested_costs[i] < 0.0 ||
            !std::isfinite(proposal.before_evidence.cumulative_lengths[i]) ||
            proposal.before_evidence.cumulative_lengths[i] < 0.0 ||
            proposal.requested_costs[i] > proposal.certificate.upper_length ||
            (i && !(proposal.ordered_lineages[i - 1] < proposal.ordered_lineages[i]))) return false;
    return true;
}

template<class T>
bool exact_scale_equal(const precision_scale_descriptor<T> &left,
                       const precision_scale_descriptor<T> &right) {
    canonical_writer a;
    canonical_writer b;
    precision_detail::encode_scale(a, left);
    precision_detail::encode_scale(b, right);
    return a.bytes() == b.bytes();
}

template<class T, class I>
bool reconstruct_scale(const immutable_source_mesh<T,I> *first,
                       const immutable_source_mesh<T,I> *second,
                       const bounded_boolean_digest &digest,
                       precision_scale_descriptor<T> &out) {
    out = {};
    out.source_digest = digest;
    std::size_t position = 0;
    auto consume = [&](const immutable_source_mesh<T,I> *source) {
        if (!source) return true;
        for (scalar_bits_t<T> raw : source->coordinate_bits()) {
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
            const auto magnitude_bits = raw & ~traits::sign_mask;
            const auto exponent_bits = raw & traits::exponent_mask;
            const auto fraction_bits = raw & traits::fraction_mask;
            if (magnitude_bits == 0) {
                if ((raw & traits::sign_mask) != 0) out.has_negative_zero = true;
                else out.has_positive_zero = true;
            } else {
                if (out.smallest_nonzero == T(0) || absolute < out.smallest_nonzero)
                    out.smallest_nonzero = absolute;
                if (exponent_bits == 0 && fraction_bits != 0) out.has_subnormal = true;
                if (exponent_bits != 0) out.has_normal = true;
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
    T largest_span = T(0);
    for (auto &axis : out.axis) {
        if (axis.maximum > T(0) && axis.minimum < T(0) &&
            axis.maximum > std::numeric_limits<T>::max() + axis.minimum) return false;
        axis.span = static_cast<T>(axis.maximum - axis.minimum);
        if (!std::isfinite(axis.span)) return false;
        axis.all_identical = axis.minimum == axis.maximum;
        largest_span = std::max(largest_span, axis.span);
    }
    if (out.maximum_absolute != T(0)) {
        const int normalized = -std::ilogb(out.maximum_absolute);
        out.normalization_exponent = static_cast<std::int16_t>(
            std::max<int>(std::numeric_limits<std::int16_t>::min(),
                          std::min<int>(std::numeric_limits<std::int16_t>::max(), normalized)));
    }
    if (out.smallest_nonzero != T(0)) {
        out.mixed_magnitude = std::ilogb(out.maximum_absolute) -
                              std::ilogb(out.smallest_nonzero) > std::numeric_limits<T>::digits;
    }
    out.large_translation = largest_span != T(0) && out.maximum_absolute / T(2) > largest_span;
    return true;
}

template<class T>
bool nonnegative_sum(T a, T b, T &result) {
    if (!std::isfinite(a) || !std::isfinite(b) || a < T(0) || b < T(0) ||
        a > std::numeric_limits<T>::max() - b) return false;
    result = static_cast<T>(a + b);
    return std::isfinite(result);
}

template<class T>
bool decoded_context_equal(const decoded_precision_context<T> &decoded,
                           const precision_context<T> &context) {
    return decoded.schema_version == context.schema_version() &&
           decoded.provider_version == context.provider_version() &&
           decoded.scalar_profile_version == context.scalar_profile_version() &&
           decoded.arithmetic_profile_version == context.arithmetic_profile_version() &&
           decoded.scalar_bytes == context.scalar_bytes() && decoded.index_bytes == context.index_bytes() &&
           decoded.tolerance == context.tolerance() &&
           decoded.declared_input_precision_a == context.declared_input_precision_a() &&
           decoded.declared_input_precision_b == context.declared_input_precision_b() &&
           decoded.effective_input_precision_a == context.effective_input_precision_a() &&
           decoded.effective_input_precision_b == context.effective_input_precision_b() &&
           decoded.required_machine_floor == context.required_machine_floor() &&
           decoded.ordinary_success_eligible == context.ordinary_success_eligible() &&
           exact_scale_equal(decoded.operand_a, context.operand_a_scale()) &&
           exact_scale_equal(decoded.operand_b, context.operand_b_scale()) &&
           exact_scale_equal(decoded.global, context.global_scale()) &&
           decoded.source_digest == context.source_digest() &&
           decoded.replay_digest == context.replay_digest() &&
           decoded.boolean_context_digest == context.boolean_context_digest() &&
           decoded.preflight_digest == context.preflight_digest() &&
           decoded.digest == context.digest();
}

template<class T>
bool decoded_preflight_equal(const precision_preflight<T> &decoded,
                             const precision_preflight<T> &record) {
    return decoded.schema_version == record.schema_version &&
           decoded.provider_version == record.provider_version &&
           decoded.scalar_profile_version == record.scalar_profile_version &&
           decoded.arithmetic_profile_version == record.arithmetic_profile_version &&
           decoded.scalar_bytes == record.scalar_bytes && decoded.index_bytes == record.index_bytes &&
           decoded.reserved == record.reserved &&
           exact_scale_equal(decoded.operand_a, record.operand_a) &&
           exact_scale_equal(decoded.operand_b, record.operand_b) &&
           exact_scale_equal(decoded.global, record.global) && decoded.tolerance == record.tolerance &&
           decoded.declared_input_precision_a == record.declared_input_precision_a &&
           decoded.declared_input_precision_b == record.declared_input_precision_b &&
           decoded.effective_input_precision_a == record.effective_input_precision_a &&
           decoded.effective_input_precision_b == record.effective_input_precision_b &&
           decoded.required_machine_floor == record.required_machine_floor &&
           decoded.ordinary_success_eligible == record.ordinary_success_eligible &&
           decoded.source_digest == record.source_digest && decoded.replay_digest == record.replay_digest &&
           decoded.canonical_bytes == record.canonical_bytes && decoded.digest == record.digest;
}

} // namespace precision_verifier_detail

template<class T>
bool verify_bounded_value(const bounded_scalar<T> &value,
                          const context_owner_token *expected_owner = nullptr) {
    if (!bounded_operations_detail::bounded_scalar_valid(value) ||
        !precision_verifier_detail::finite_contributors(value.contributors) ||
        value.identity.schema_version != 1 || value.identity.provider_version != 1 ||
        value.identity.publication < bounded_publication_state::transaction_local ||
        value.identity.publication > bounded_publication_state::invalid) return false;
    return !expected_owner || value.identity.owner.same_owner(*expected_owner);
}

template<class T>
bool verify_directed_result(const directed_operation_result<T> &result) {
    if (result.status != numeric_status::success) return result.value.exactness == rounding_exactness::invalid;
    const auto &value = result.value;
    if (!finite_bits(value.rounded) || !finite_bits(value.lower) || !finite_bits(value.upper) ||
        value.lower > value.rounded || value.rounded > value.upper ||
        value.operation < rounded_operation_code::source_import ||
        value.operation > rounded_operation_code::aabb_intersection) return false;
    if (value.exactness == rounding_exactness::exact)
        return value.direction == residual_sign::zero && value.lower == value.rounded && value.upper == value.rounded;
    if (value.exactness == rounding_exactness::inexact_direction_known) {
        T adjacent = T(0);
        if (value.direction == residual_sign::positive)
            return next_up_finite(value.rounded, adjacent) && value.lower == value.rounded && value.upper == adjacent;
        if (value.direction == residual_sign::negative)
            return next_down_finite(value.rounded, adjacent) && value.lower == adjacent && value.upper == value.rounded;
        return false;
    }
    if (value.exactness != rounding_exactness::direction_unavailable ||
        value.direction != residual_sign::unavailable) return false;
    T lower = T(0), upper = T(0);
    return next_down_finite(value.rounded, lower) && next_up_finite(value.rounded, upper) &&
           value.lower == lower && value.upper == upper;
}

template<class T>
bool verify_directed_result(const directed_operation_result<T> &result, T a, T b) {
    directed_operation_result<T> expected;
    switch (result.value.operation) {
    case rounded_operation_code::add: expected = directed_add(a, b); break;
    case rounded_operation_code::subtract: expected = directed_subtract(a, b); break;
    case rounded_operation_code::multiply: expected = directed_multiply(a, b); break;
    case rounded_operation_code::divide: expected = directed_divide(a, b); break;
    default: return false;
    }
    return verify_directed_result(result) && encode_directed_result(result) == encode_directed_result(expected);
}

inline bool verify_exact_relation(const exact_relation_evidence &relation,
                                  const context_owner_token *expected_owner = nullptr) {
    if (relation.schema_version != 1 || relation.status < exact_relation_status::exact_negative ||
        relation.status > exact_relation_status::invalid ||
        !valid_exact_formula_code(relation.formula_code) ||
        (expected_owner && !relation.owner.same_owner(*expected_owner))) return false;
    if (relation.status != exact_relation_status::unavailable && relation.status != exact_relation_status::invalid &&
        (relation.formula_code == 0 || relation.capacity_used == 0)) return false;
    return true;
}

template<class T>
bool verify_exact_relation(const exact_relation_evidence &relation,
                           const std::vector<T> &inputs,
                           const context_owner_token *expected_owner = nullptr) {
    if (!verify_exact_relation(relation, expected_owner)) return false;
    exact_relation_record reconstructed;
    const auto formula = static_cast<exact_relation_formula_code>(relation.formula_code);
    if (formula == exact_relation_formula_code::finite_scalar_comparison && inputs.size() == 2)
        reconstructed = exact_scalar_comparison(inputs[0], inputs[1]);
    else if (formula == exact_relation_formula_code::sum_residual && inputs.size() == 3)
        reconstructed = exact_sum_residual(inputs[0], inputs[1], inputs[2]);
    else if (formula == exact_relation_formula_code::difference_residual && inputs.size() == 3)
        reconstructed = exact_difference_residual(inputs[0], inputs[1], inputs[2]);
    else if (formula == exact_relation_formula_code::product_residual && inputs.size() == 3)
        reconstructed = exact_product_residual(inputs[0], inputs[1], inputs[2]);
    else if (formula == exact_relation_formula_code::division_quotient_residual_numerator && inputs.size() == 3)
        reconstructed = exact_division_residual_numerator(inputs[0], inputs[1], inputs[2]);
    else if (formula == exact_relation_formula_code::determinant_2x2 && inputs.size() == 4) {
        std::array<T, 4> matrix{}; std::copy(inputs.begin(), inputs.end(), matrix.begin());
        reconstructed = exact_determinant_2x2(matrix);
    } else if (formula == exact_relation_formula_code::determinant_3x3 && inputs.size() == 9) {
        std::array<T, 9> matrix{}; std::copy(inputs.begin(), inputs.end(), matrix.begin());
        reconstructed = exact_determinant<T, 3>(matrix, formula);
    } else if ((formula == exact_relation_formula_code::orient_2d ||
                formula == exact_relation_formula_code::collinearity_2d) && inputs.size() == 6) {
        const std::array<T, 2> a{{inputs[0], inputs[1]}};
        const std::array<T, 2> b{{inputs[2], inputs[3]}};
        const std::array<T, 2> c{{inputs[4], inputs[5]}};
        reconstructed = exact_orient_2d(a, b, c);
    } else if ((formula == exact_relation_formula_code::orient_3d ||
                formula == exact_relation_formula_code::coplanarity_3d ||
                formula == exact_relation_formula_code::plane_numerator) && inputs.size() == 12) {
        const std::array<T, 3> a{{inputs[0], inputs[1], inputs[2]}};
        const std::array<T, 3> b{{inputs[3], inputs[4], inputs[5]}};
        const std::array<T, 3> c{{inputs[6], inputs[7], inputs[8]}};
        const std::array<T, 3> d{{inputs[9], inputs[10], inputs[11]}};
        reconstructed = exact_orient_3d(a, b, c, d);
    } else if (formula == exact_relation_formula_code::plane_point_residual_3d &&
               inputs.size() == 12) {
        const std::array<T, 3> a{{inputs[0], inputs[1], inputs[2]}};
        const std::array<T, 3> b{{inputs[3], inputs[4], inputs[5]}};
        const std::array<T, 3> c{{inputs[6], inputs[7], inputs[8]}};
        const std::array<T, 3> d{{inputs[9], inputs[10], inputs[11]}};
        reconstructed = exact_plane_point_residual_3d(a, b, c, d);
    } else if ((formula == exact_relation_formula_code::plane_normal_parallel_3d ||
                formula == exact_relation_formula_code::plane_normal_dot_3d) &&
               inputs.size() == 18) {
        const std::array<T, 3> a0{{inputs[0], inputs[1], inputs[2]}};
        const std::array<T, 3> a1{{inputs[3], inputs[4], inputs[5]}};
        const std::array<T, 3> a2{{inputs[6], inputs[7], inputs[8]}};
        const std::array<T, 3> b0{{inputs[9], inputs[10], inputs[11]}};
        const std::array<T, 3> b1{{inputs[12], inputs[13], inputs[14]}};
        const std::array<T, 3> b2{{inputs[15], inputs[16], inputs[17]}};
        reconstructed = formula == exact_relation_formula_code::plane_normal_parallel_3d
            ? exact_plane_normals_parallel_3d(a0, a1, a2, b0, b1, b2)
            : exact_plane_normal_dot_3d(a0, a1, a2, b0, b1, b2);
    } else return false;
    return reconstructed.status == relation.status &&
           reconstructed.normalization_exponent == relation.normalization_exponent &&
           reconstructed.capacity_used == relation.capacity_used;
}

template<class T>
bool verify_predicate_result(const predicate_result<T> &result,
                             const context_owner_token *expected_owner = nullptr) {
    if (result.schema_version != 1 || !verify_bounded_value(result.rounded_and_bounded, expected_owner) ||
        !verify_exact_relation(result.exact_relation, expected_owner) ||
        !result.owner.same_owner(result.rounded_and_bounded.identity.owner) ||
        result.bounded_sign != classify_bounded_sign(result.rounded_and_bounded.uncertainty_enclosure) ||
        result.disposition != assemble_predicate_disposition(result.bounded_sign,
            result.exact_relation.status, result.disposition == predicate_disposition::try_permitted_alternate) ||
        result.trace_root != result.rounded_and_bounded.identity.trace_root) return false;
    const auto &interval = result.rounded_and_bounded.uncertainty_enclosure;
    const auto width = directed_subtract(interval.upper(), interval.lower());
    if (!width || result.uncertainty_width != width.value.upper) return false;
    const T margin = result.bounded_sign == bounded_sign_status::definitely_negative ? -interval.upper()
        : result.bounded_sign == bounded_sign_status::definitely_positive ? interval.lower() : T(0);
    return result.separation_margin == margin;
}

template<class T>
bool verify_construction(const construction_conditioning<T> &value,
                         const context_owner_token *expected_owner = nullptr) {
    if (value.schema_version != 1 || !bounded_operations_detail::owner_bound(value.owner) ||
        (expected_owner && !value.owner.same_owner(*expected_owner)) ||
        !finite_bits(value.required_precision) || !finite_bits(value.available_tolerance) ||
        !finite_bits(value.amplification_upper) || value.required_precision < T(0) ||
        value.available_tolerance < T(0) || value.amplification_upper < T(0) ||
        !verify_exact_relation(value.denominator_relation, &value.owner) ||
        !bounded_operations_detail::bounded_scalar_valid(value.parameter.value) ||
        !value.parameter.owner.same_owner(value.owner) ||
        !precision_verifier_detail::finite_contributors(value.contributors)) return false;
    if (value.kind != construction_kind::projection) {
        const auto category = classify_construction(value.parameter, value.denominator,
            value.parameter.domain == parameter_domain_status::stable_endpoint
                ? exact_relation_status::exact_zero : exact_relation_status::unavailable,
            value.category == construction_category::coplanar_or_coincident
                ? exact_relation_status::exact_zero : exact_relation_status::unavailable,
            value.required_precision, value.available_tolerance);
        if (value.category != construction_category::exact_stored_coordinate_tie && category != value.category)
            return false;
    }
    if (value.has_constructed_point) {
        if (!value.constructed_point.owner.same_owner(value.owner) ||
            !value.residual_after.owner.same_owner(value.owner) ||
            value.residual_after.disposition != residual_disposition::pass) return false;
        for (const auto &component : value.constructed_point.coordinates.components)
            if (!verify_bounded_value(component, &value.owner)) return false;
    }
    return true;
}

inline bool verify_failure(const bounded_boolean_error &error) {
    return error.version == 1 &&
           error.category >= bounded_boolean_error_category::input_contract_error &&
           error.category <= bounded_boolean_error_category::internal_invariant_error &&
           error.component != 0 && error.witness_count <= error.witnesses.size();
}

inline bool verify_precision_trace(const precision_trace_snapshot &snapshot,
                                   const context_owner_token *expected_owner = nullptr) {
    if (snapshot.schema_version != 1 || snapshot.nodes.empty() ||
        snapshot.root.ordinal() >= snapshot.nodes.size() ||
        (expected_owner && !snapshot.owner.same_owner(*expected_owner))) return false;
    std::vector<std::vector<std::uint8_t>> structural(snapshot.nodes.size());
    std::vector<bounded_scalar<double>> typed(snapshot.nodes.size());
    std::vector<bool> has_typed(snapshot.nodes.size(), false);
    std::vector<std::uint64_t> depth(snapshot.nodes.size(), 0);
    for (std::size_t i = 0; i < snapshot.nodes.size(); ++i) {
        const auto &node = snapshot.nodes[i];
        if (node.id.ordinal() != i || !precision_verifier_detail::valid_trace_operation(node.key) ||
            !precision_verifier_detail::finite_contributors(node.contributors)) return false;
        for (const auto parent : node.key.parents) {
            if (parent >= i) return false;
            depth[i] = std::max(depth[i], depth[parent] + 1);
        }
        canonical_writer writer;
        precision_verifier_detail::trace_structural_bytes(writer, node, structural);
        structural[i] = writer.take();
        if (i && (depth[i] < depth[i - 1] ||
            (depth[i] == depth[i - 1] && !(structural[i - 1] < structural[i])))) return false;
        auto decoded = decode_bounded_value<double>(node.key.result_bytes,
            expected_owner ? *expected_owner : context_owner_token{});
        if (decoded.has_value()) {
            typed[i] = std::move(*decoded.value()); has_typed[i] = true;
            if (!verify_bounded_value(typed[i], expected_owner)) return false;
            boolean_outcome<bounded_scalar<double>> reconstructed =
                boolean_outcome<bounded_scalar<double>>::success(typed[i]);
            const auto operation = static_cast<rounded_operation_code>(node.key.operation_code);
            if (node.key.parents.size() == 2 && has_typed[node.key.parents[0]] && has_typed[node.key.parents[1]]) {
                const auto &a = typed[node.key.parents[0]], &b = typed[node.key.parents[1]];
                if (operation == rounded_operation_code::add) reconstructed = bounded_add(a, b);
                else if (operation == rounded_operation_code::subtract) reconstructed = bounded_subtract(a, b);
                else if (operation == rounded_operation_code::multiply) reconstructed = bounded_multiply(a, b);
                else if (operation == rounded_operation_code::divide) reconstructed = bounded_divide(a, b);
            } else if (operation == rounded_operation_code::negate && has_typed[node.key.parents[0]]) {
                reconstructed = bounded_negate(typed[node.key.parents[0]]);
            }
            if (!reconstructed.has_value() ||
                !precision_verifier_detail::same_computed_scalar(*reconstructed.value(), typed[i])) return false;
        } else if (expected_owner) return false;
    }
    std::vector<bool> reachable(snapshot.nodes.size(), false);
    std::vector<std::uint64_t> stack{snapshot.root.ordinal()};
    while (!stack.empty()) {
        const auto id = stack.back(); stack.pop_back();
        if (reachable[id]) continue;
        reachable[id] = true;
        for (const auto parent : snapshot.nodes[id].key.parents) stack.push_back(parent);
    }
    return std::find(reachable.begin(), reachable.end(), false) == reachable.end();
}

inline bool verify_precision_ledger(const precision_ledger_snapshot &snapshot, double tolerance,
                                    const context_owner_token *expected_owner = nullptr) {
    if (snapshot.schema_version != 1 || !std::isfinite(tolerance) || tolerance < 0.0 ||
        (expected_owner && !snapshot.owner.same_owner(*expected_owner))) return false;
    std::map<std::uint64_t, lineage_precision_total> totals;
    std::set<std::uint64_t> results;
    for (std::size_t i = 0; i < snapshot.records.size(); ++i) {
        const auto &record = snapshot.records[i];
        if (record.schema_version != 1 || record.provider_version != 1 || record.id.ordinal() != i ||
            !registered_rounded_operation(static_cast<rounded_operation_code>(record.operation_code)) ||
            !valid_exact_formula_code(record.exact_formula_code) ||
            (record.owner.anchor && !record.owner.same_owner(snapshot.owner)) ||
            !results.insert(record.result.ordinal()).second ||
            !precision_verifier_detail::finite_contributors(record.contributors)) return false;
        std::set<std::uint64_t> parents;
        for (const auto parent : record.ordered_parents)
            if (parent.ordinal() >= i || !parents.insert(parent.ordinal()).second) return false;
        for (std::size_t j = 1; j < record.source_provenance.size(); ++j)
            if (!(record.source_provenance[j - 1] < record.source_provenance[j])) return false;
        double no_motion = 0.0, displacement = 0.0;
        const double no_motion_terms[]{record.contributors.inherited_a, record.contributors.inherited_b,
            record.contributors.machine_floor, record.contributors.construction,
            record.contributors.conditioning, record.contributors.conversion};
        for (double value : no_motion_terms)
            if (!precision_verifier_detail::outward_add(no_motion, value, no_motion)) return false;
        if (!precision_verifier_detail::outward_add(record.contributors.prior_cleanup,
                record.contributors.current_cleanup, displacement) ||
            record.no_motion_uncertainty != no_motion || record.cumulative_displacement != displacement)
            return false;
        auto &total = totals[record.lineage.ordinal()]; total.lineage = record.lineage;
        if (!precision_verifier_detail::outward_add(total.no_motion_uncertainty, no_motion,
                total.no_motion_uncertainty) ||
            !precision_verifier_detail::outward_add(total.cumulative_displacement, displacement,
                total.cumulative_displacement) ||
            !precision_verifier_detail::outward_add(total.no_motion_uncertainty,
                total.cumulative_displacement, total.precision)) return false;
        for (const auto parent : record.ordered_parents)
            total.precision = std::max(total.precision, snapshot.records[parent.ordinal()].lineage_precision);
        if (record.lineage_precision != total.precision ||
            record.within_tolerance != (total.precision <= tolerance) ||
            record.canonical_digest_contribution != precision_ledger_record_digest(record)) return false;
    }
    if (snapshot.lineages.size() != totals.size()) return false;
    double global = 0.0; std::size_t index = 0;
    for (const auto &entry : totals) {
        const auto &stored = snapshot.lineages[index++];
        if (stored.lineage.ordinal() != entry.first ||
            stored.no_motion_uncertainty != entry.second.no_motion_uncertainty ||
            stored.cumulative_displacement != entry.second.cumulative_displacement ||
            stored.precision != entry.second.precision) return false;
        global = std::max(global, entry.second.precision);
    }
    return snapshot.global_output_precision == global;
}

inline bool verify_tolerance_budget(const tolerance_budget_snapshot &snapshot,
                                    const context_owner_token *expected_owner = nullptr,
                                    bool require_no_active_reservations = true) {
    if (snapshot.schema_version != 1 || !std::isfinite(snapshot.tolerance) || snapshot.tolerance < 0.0 ||
        snapshot.active_reservations != snapshot.reservations.size() ||
        (require_no_active_reservations && snapshot.active_reservations != 0) ||
        (expected_owner && !snapshot.owner.same_owner(*expected_owner))) return false;
    std::map<std::uint64_t, double> totals;
    std::set<std::uint64_t> proposals;
    for (std::size_t i = 0; i < snapshot.commits.size(); ++i) {
        const auto &commit = snapshot.commits[i]; const auto &proposal = commit.proposal_record;
        if (commit.id.ordinal() != i || commit.proposal != proposal.id ||
            !proposals.insert(commit.proposal.ordinal()).second ||
            !precision_verifier_detail::valid_budget_proposal(proposal) ||
            commit.actual_costs.size() != proposal.requested_costs.size() ||
            commit.cumulative_totals.size() != commit.actual_costs.size() ||
            commit.ledger_entries.size() != commit.actual_costs.size() ||
            commit.after_evidence.schema_version != 1 ||
            commit.after_evidence.transaction_owner != proposal.transaction_owner ||
            commit.after_evidence.cumulative_lengths.size() != commit.actual_costs.size() ||
            commit.after_evidence.evidence.empty()) return false;
        if (proposal.owner.anchor && !commit.after_evidence.owner.same_owner(proposal.owner)) return false;
        if (i) {
            const auto &prior = snapshot.commits[i - 1].proposal_record;
            if (proposal.canonical_merge_key < prior.canonical_merge_key ||
                (proposal.canonical_merge_key == prior.canonical_merge_key && !(prior.id < proposal.id))) return false;
        }
        std::set<std::uint64_t> linked_entries;
        for (std::size_t j = 0; j < commit.actual_costs.size(); ++j) {
            const double actual = commit.actual_costs[j]; double next = 0.0;
            double verified = 0.0;
            const double before = proposal.before_evidence.cumulative_lengths[j];
            const double after = commit.after_evidence.cumulative_lengths[j];
            const auto difference = directed_subtract(after, before);
            if (!difference) return false;
            verified = difference.value.upper;
            if (!std::isfinite(actual) || actual < 0.0 || actual != verified ||
                actual > proposal.requested_costs[j] ||
                !linked_entries.insert(commit.ledger_entries[j].ordinal()).second ||
                !precision_verifier_detail::outward_add(totals[proposal.ordered_lineages[j].ordinal()],
                    actual, next) || next > snapshot.tolerance || commit.cumulative_totals[j] != next) return false;
            totals[proposal.ordered_lineages[j].ordinal()] = next;
        }
    }
    std::map<std::uint64_t, double> reserved = totals;
    for (std::size_t reservation_index = 0; reservation_index < snapshot.reservations.size(); ++reservation_index) {
        const auto &proposal = snapshot.reservations[reservation_index];
        if (reservation_index) {
            const auto &prior = snapshot.reservations[reservation_index - 1];
            if (proposal.canonical_merge_key < prior.canonical_merge_key ||
                (proposal.canonical_merge_key == prior.canonical_merge_key && !(prior.id < proposal.id))) return false;
        }
        if (!proposals.insert(proposal.id.ordinal()).second ||
            !precision_verifier_detail::valid_budget_proposal(proposal)) return false;
        for (std::size_t i = 0; i < proposal.requested_costs.size(); ++i) {
            double next = 0.0;
            if (!precision_verifier_detail::outward_add(reserved[proposal.ordered_lineages[i].ordinal()],
                    proposal.requested_costs[i], next) || next > snapshot.tolerance) return false;
            reserved[proposal.ordered_lineages[i].ordinal()] = next;
        }
    }
    if (snapshot.lineages.size() != totals.size()) return false;
    double global = 0.0; std::size_t index = 0;
    for (const auto &entry : totals) {
        if (snapshot.lineages[index].lineage.ordinal() != entry.first ||
            snapshot.lineages[index].committed_displacement != entry.second) return false;
        global = std::max(global, entry.second); ++index;
    }
    return snapshot.global_realized_displacement == global;
}

inline bool verify_tolerance_budget(const tolerance_budget_snapshot &snapshot,
                                    const precision_ledger_snapshot &ledger,
                                    const context_owner_token *expected_owner = nullptr,
                                    bool require_no_active_reservations = true) {
    if (!verify_tolerance_budget(snapshot, expected_owner, require_no_active_reservations) ||
        (expected_owner && !ledger.owner.same_owner(*expected_owner))) return false;
    for (const auto &commit : snapshot.commits) {
        for (std::size_t i = 0; i < commit.ledger_entries.size(); ++i) {
            const auto id = commit.ledger_entries[i].ordinal();
            if (id >= ledger.records.size()) return false;
            const auto &record = ledger.records[id];
            if (record.lineage != commit.proposal_record.ordered_lineages[i] ||
                record.contributors.current_cleanup != commit.actual_costs[i] ||
                record.canonical_digest_contribution != precision_ledger_record_digest(record))
                return false;
        }
    }
    return true;
}

template<class T>
bool verify_conservative_bound(const conservative_bound_record<T> &record,
                               const context_owner_token *expected_owner = nullptr) {
    if (record.schema_version != 1 || !precision_verifier_detail::finite_contributors(record.bound.inflation))
        return false;
    if (expected_owner && !record.bound.owner.same_owner(*expected_owner)) return false;
    for (const auto &axis : record.bound.axes)
        if (!finite_bits(axis.lower()) || !finite_bits(axis.upper()) ||
            finite_numeric_less(axis.upper(), axis.lower())) return false;
    for (std::size_t i = 1; i < record.ordered_provenance.size(); ++i)
        if (!(record.ordered_provenance[i - 1] < record.ordered_provenance[i])) return false;
    for (std::size_t i = 1; i < record.ordered_lineages.size(); ++i)
        if (!(record.ordered_lineages[i - 1] < record.ordered_lineages[i])) return false;
    if (record.ordered_provenance.empty() || record.ordered_lineages.empty()) return false;
    if (record.ordered_provenance.size() == 1 && record.bound.provenance != record.ordered_provenance[0]) return false;
    if (record.ordered_provenance.size() > 1 && record.bound.provenance.ordinal() != 0) return false;
    if (record.ordered_lineages.size() == 1 && record.bound.lineage != record.ordered_lineages[0]) return false;
    if (record.ordered_lineages.size() > 1 && record.bound.lineage.ordinal() != 0) return false;
    return true;
}

template<class T>
bool verify_point_bound(const conservative_bound_record<T> &record,
                        const bounded_point3<T> &point) {
    if (!verify_conservative_bound(record) || record.ordered_provenance.size() != 1 ||
        record.ordered_lineages.size() != 1 || record.ordered_provenance[0] != point.provenance ||
        record.ordered_lineages[0] != point.lineage) return false;
    uncertainty_contributors contributors;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto &expected = point.coordinates.components[axis].uncertainty_enclosure;
        const auto &stored = record.bound.axes[axis];
        if (to_bits(expected.lower()) != to_bits(stored.lower()) ||
            to_bits(expected.upper()) != to_bits(stored.upper())) return false;
        uncertainty_contributors next;
        const auto &value = point.coordinates.components[axis].contributors;
        if (!precision_verifier_detail::outward_add(contributors.inherited_a, value.inherited_a, next.inherited_a) ||
            !precision_verifier_detail::outward_add(contributors.inherited_b, value.inherited_b, next.inherited_b) ||
            !precision_verifier_detail::outward_add(contributors.machine_floor, value.machine_floor, next.machine_floor) ||
            !precision_verifier_detail::outward_add(contributors.construction, value.construction, next.construction) ||
            !precision_verifier_detail::outward_add(contributors.conditioning, value.conditioning, next.conditioning) ||
            !precision_verifier_detail::outward_add(contributors.conversion, value.conversion, next.conversion) ||
            !precision_verifier_detail::outward_add(contributors.prior_cleanup, value.prior_cleanup, next.prior_cleanup) ||
            !precision_verifier_detail::outward_add(contributors.current_cleanup, value.current_cleanup, next.current_cleanup)) return false;
        contributors = next;
    }
    canonical_writer expected, stored;
    precision_codec_detail::encode_contributors(expected, contributors);
    precision_codec_detail::encode_contributors(stored, record.bound.inflation);
    return expected.bytes() == stored.bytes();
}

template<class T>
bool verify_union_bound(const conservative_bound_record<T> &record,
                        const conservative_bound_record<T> &a,
                        const conservative_bound_record<T> &b) {
    if (!verify_conservative_bound(record) || !verify_conservative_bound(a) ||
        !verify_conservative_bound(b)) return false;
    if (static_cast<bool>(a.bound.owner.anchor) != static_cast<bool>(b.bound.owner.anchor) ||
        static_cast<bool>(record.bound.owner.anchor) != static_cast<bool>(a.bound.owner.anchor) ||
        (a.bound.owner.anchor && (!a.bound.owner.same_owner(b.bound.owner) ||
                                 !record.bound.owner.same_owner(a.bound.owner)))) return false;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const T lower = finite_numeric_less(b.bound.axes[axis].lower(), a.bound.axes[axis].lower())
            ? b.bound.axes[axis].lower() : a.bound.axes[axis].lower();
        const T upper = finite_numeric_less(a.bound.axes[axis].upper(), b.bound.axes[axis].upper())
            ? b.bound.axes[axis].upper() : a.bound.axes[axis].upper();
        if (to_bits(lower) != to_bits(record.bound.axes[axis].lower()) ||
            to_bits(upper) != to_bits(record.bound.axes[axis].upper())) return false;
    }
    auto provenance = a.ordered_provenance; auto lineages = a.ordered_lineages;
    for (const auto value : b.ordered_provenance) {
        const auto position = std::lower_bound(provenance.begin(), provenance.end(), value);
        if (position == provenance.end() || *position != value) provenance.insert(position, value);
    }
    for (const auto value : b.ordered_lineages) {
        const auto position = std::lower_bound(lineages.begin(), lineages.end(), value);
        if (position == lineages.end() || *position != value) lineages.insert(position, value);
    }
    if (record.ordered_provenance != provenance || record.ordered_lineages != lineages) return false;
    uncertainty_contributors contributors;
    const auto &left = a.bound.inflation; const auto &right = b.bound.inflation;
    if (!precision_verifier_detail::outward_add(left.inherited_a, right.inherited_a, contributors.inherited_a) ||
        !precision_verifier_detail::outward_add(left.inherited_b, right.inherited_b, contributors.inherited_b) ||
        !precision_verifier_detail::outward_add(left.machine_floor, right.machine_floor, contributors.machine_floor) ||
        !precision_verifier_detail::outward_add(left.construction, right.construction, contributors.construction) ||
        !precision_verifier_detail::outward_add(left.conditioning, right.conditioning, contributors.conditioning) ||
        !precision_verifier_detail::outward_add(left.conversion, right.conversion, contributors.conversion) ||
        !precision_verifier_detail::outward_add(left.prior_cleanup, right.prior_cleanup, contributors.prior_cleanup) ||
        !precision_verifier_detail::outward_add(left.current_cleanup, right.current_cleanup, contributors.current_cleanup)) return false;
    canonical_writer expected, stored;
    precision_codec_detail::encode_contributors(expected, contributors);
    precision_codec_detail::encode_contributors(stored, record.bound.inflation);
    return expected.bytes() == stored.bytes();
}

template<class T>
bool verify_precision_import_record(const foreign_precision_provenance &record, T tolerance) {
    if (record.schema_version != 1 || !std::isfinite(tolerance) || tolerance < T(0) ||
        !record.publication_verified || record.construction_history_digest.empty() ||
        record.prior_context_digest.empty() || record.prior_output_precision > tolerance) return false;
    const double values[]{record.prior_output_precision, record.inherited_precision,
        record.construction_uncertainty, record.cumulative_cleanup_displacement,
        record.serialization_contribution};
    for (double value : values) if (!std::isfinite(value) || value < 0.0) return false;
    double reconstructed = 0.0;
    for (double value : {record.inherited_precision, record.construction_uncertainty,
                         record.cumulative_cleanup_displacement, record.serialization_contribution})
        if (!precision_verifier_detail::outward_add(reconstructed, value, reconstructed) ||
            reconstructed > record.prior_output_precision) return false;
    if (record.owner.anchor) {
        if (record.publication_digest.empty() || record.verification_evidence.empty() ||
            record.publication_digest != record.verified_publication_digest) return false;
        return verify_precision_import(record, tolerance).has_value();
    }
    return true;
}

template<class T, class I>
bool verify_precision_preflight(
    const precision_preflight<T> &record,
    const pending_invocation<T,I> &pending,
    const precision_bootstrap_capabilities &capabilities = {}) {
    if (record.schema_version != precision_bootstrap_schema_v1 ||
        record.provider_version != precision_bootstrap_provider_v1 ||
        record.scalar_profile_version != precision_scalar_profile_v1 ||
        record.arithmetic_profile_version != precision_arithmetic_profile_v1 ||
        record.scalar_bytes != sizeof(T) || record.index_bytes != sizeof(I) || record.reserved != 0 ||
        capabilities.version != 1 || capabilities.reserved != 0 || !capabilities.strict_build ||
        !precision_detail::runtime_precision_profile_qualified<T,I>() ||
        (capabilities.require_nearest_even && std::fegetround() != FE_TONEAREST) ||
        !verify_pending(pending) || !verify_invocation_sources(pending.sources)) return false;

    precision_scale_descriptor<T> a, b, global;
    if (!precision_verifier_detail::reconstruct_scale(&pending.sources.a,
            static_cast<const immutable_source_mesh<T,I> *>(nullptr), pending.sources.a.digest(), a) ||
        !precision_verifier_detail::reconstruct_scale(&pending.sources.b,
            static_cast<const immutable_source_mesh<T,I> *>(nullptr), pending.sources.b.digest(), b) ||
        !precision_verifier_detail::reconstruct_scale(&pending.sources.a, &pending.sources.b,
                                                       pending.sources.digest, global) ||
        !precision_verifier_detail::exact_scale_equal(record.operand_a, a) ||
        !precision_verifier_detail::exact_scale_equal(record.operand_b, b) ||
        !precision_verifier_detail::exact_scale_equal(record.global, global)) return false;

    T effective_a = T(0), effective_b = T(0);
    if (!precision_verifier_detail::nonnegative_sum(pending.options.input_precision_a,
                                                     a.machine_floor, effective_a) ||
        !precision_verifier_detail::nonnegative_sum(pending.options.input_precision_b,
                                                     b.machine_floor, effective_b)) return false;
    const T floor = std::max(a.machine_floor, b.machine_floor);
    const bool eligible = effective_a <= pending.options.tolerance &&
                          effective_b <= pending.options.tolerance && floor <= pending.options.tolerance;
    if (record.tolerance != pending.options.tolerance ||
        record.declared_input_precision_a != pending.options.input_precision_a ||
        record.declared_input_precision_b != pending.options.input_precision_b ||
        record.effective_input_precision_a != effective_a ||
        record.effective_input_precision_b != effective_b ||
        record.required_machine_floor != floor ||
        record.ordinary_success_eligible != eligible ||
        record.source_digest != pending.sources.digest ||
        record.replay_digest != pending.replay_digest) return false;

    const auto bytes = precision_detail::encode_preflight_payload(record);
    if (record.canonical_bytes != bytes || record.digest != sha256::digest(bytes)) return false;
    auto decoded = decode_precision_preflight<T>(bytes);
    return decoded.has_value() &&
           precision_verifier_detail::decoded_preflight_equal(*decoded.value(), record);
}

template<class T, class I>
bool verify_precision_preflight(
    const precision_preflight<T> &record,
    const pending_boolean_context_view<T,I> &pending,
    const precision_bootstrap_capabilities &capabilities = {}) {
    return verify_precision_preflight(record, pending.underlying(), capabilities);
}

template<class T, class I>
bool verify_precision_context(
    const precision_context<T> &precision,
    const precision_preflight<T> &preflight,
    const boolean_context<T,I> &context,
    const precision_runtime_capabilities &capabilities = {}) {
    if (precision.schema_version() != precision_context_schema_v1 ||
        precision.provider_version() != precision_context_provider_v1 ||
        precision.scalar_profile_version() != precision_scalar_profile_v1 ||
        precision.arithmetic_profile_version() != precision_arithmetic_profile_v1 ||
        precision.scalar_bytes() != sizeof(T) || precision.index_bytes() != sizeof(I) ||
        preflight.schema_version != precision_bootstrap_schema_v1 ||
        preflight.provider_version != precision_bootstrap_provider_v1 ||
        preflight.scalar_profile_version != precision_scalar_profile_v1 ||
        preflight.arithmetic_profile_version != precision_arithmetic_profile_v1 ||
        preflight.scalar_bytes != sizeof(T) || preflight.index_bytes != sizeof(I) ||
        preflight.reserved != 0 || context.precision.version != 1 ||
        capabilities.version != 1 || capabilities.provider_version != precision_context_provider_v1 ||
        capabilities.reserved != 0 || !context.owner.anchor || !precision.owned_by(context.owner) ||
        (capabilities.expected_owner && !precision.owned_by(*capabilities.expected_owner)) ||
        !precision_detail::runtime_precision_profile_qualified<T,I>() ||
        (capabilities.require_nearest_even && std::fegetround() != FE_TONEAREST) ||
        !verify_context(context) || !context.sources) return false;

    precision_scale_descriptor<T> a, b, global;
    if (!precision_verifier_detail::reconstruct_scale(&context.sources->a,
            static_cast<const immutable_source_mesh<T,I> *>(nullptr), context.sources->a.digest(), a) ||
        !precision_verifier_detail::reconstruct_scale(&context.sources->b,
            static_cast<const immutable_source_mesh<T,I> *>(nullptr), context.sources->b.digest(), b) ||
        !precision_verifier_detail::reconstruct_scale(&context.sources->a, &context.sources->b,
                                                       context.sources->digest, global)) return false;

    T effective_a = T(0), effective_b = T(0);
    if (!precision_verifier_detail::nonnegative_sum(context.options.input_precision_a,
                                                     a.machine_floor, effective_a) ||
        !precision_verifier_detail::nonnegative_sum(context.options.input_precision_b,
                                                     b.machine_floor, effective_b)) return false;
    const T floor = std::max(a.machine_floor, b.machine_floor);
    const bool eligible = effective_a <= context.options.tolerance &&
                          effective_b <= context.options.tolerance && floor <= context.options.tolerance;
    if (!eligible || !precision.ordinary_success_eligible() ||
        precision.tolerance() != context.options.tolerance ||
        precision.declared_input_precision_a() != context.options.input_precision_a ||
        precision.declared_input_precision_b() != context.options.input_precision_b ||
        precision.effective_input_precision_a() != effective_a ||
        precision.effective_input_precision_b() != effective_b ||
        precision.required_machine_floor() != floor ||
        !precision_verifier_detail::exact_scale_equal(precision.operand_a_scale(), a) ||
        !precision_verifier_detail::exact_scale_equal(precision.operand_b_scale(), b) ||
        !precision_verifier_detail::exact_scale_equal(precision.global_scale(), global) ||
        precision.source_digest() != context.input_digest ||
        precision.replay_digest() != context.replay_digest ||
        precision.boolean_context_digest() != context.context_digest ||
        precision.preflight_digest() != preflight.digest) return false;

    // The frozen context has no replay bytes, so verify all preflight fields against
    // independently reconstructed frozen data and its already-bound canonical digest.
    if (preflight.source_digest != context.input_digest ||
        preflight.replay_digest != context.replay_digest ||
        preflight.tolerance != context.options.tolerance ||
        preflight.declared_input_precision_a != context.options.input_precision_a ||
        preflight.declared_input_precision_b != context.options.input_precision_b ||
        preflight.effective_input_precision_a != effective_a ||
        preflight.effective_input_precision_b != effective_b ||
        preflight.required_machine_floor != floor ||
        preflight.ordinary_success_eligible != eligible ||
        preflight.canonical_bytes != precision_detail::encode_preflight_payload(preflight) ||
        preflight.digest != sha256::digest(preflight.canonical_bytes) ||
        !precision_detail::verify_preflight_against_frozen_sources(
            preflight, *context.sources, context.options, context.replay_digest)) return false;

    const auto bytes = precision_detail::encode_context_payload(precision);
    if (precision.canonical_bytes() != bytes || precision.digest() != sha256::digest(bytes)) return false;
    auto decoded = decode_precision_context<T>(bytes);
    return decoded.has_value() &&
           precision_verifier_detail::decoded_context_equal(*decoded.value(), precision);
}

template<class T>
bool verify_precision_context_owner(const precision_context<T> &context,
                                    const context_owner_token &owner) noexcept {
    return context.schema_version() == precision_context_schema_v1 &&
           context.provider_version() == precision_context_provider_v1 && context.owned_by(owner);
}

} // namespace ygor::mesh_boolean::bounded
