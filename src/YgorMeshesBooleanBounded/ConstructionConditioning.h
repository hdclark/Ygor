#pragma once

#include "BoundedOperations.h"
#include "PredicateResults.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

struct construction_tag;
using construction_id = strong_id<construction_tag>;

enum class construction_kind : std::uint8_t {
    edge_plane = 1, edge_face = 2, carrier = 3, interpolation = 4, projection = 5
};
enum class construction_category : std::uint8_t {
    stable_interior = 1,
    stable_endpoint = 2,
    exact_stored_coordinate_tie = 3,
    coplanar_or_coincident = 4,
    near_parallel_bounded = 5,
    ill_conditioned = 6,
    invalid = 7
};

enum class construction_tolerance_disposition : std::uint8_t {
    accepted = 1, rejected = 2, invalid = 3
};

template<class T>
struct construction_issued_output_evidence final {
    bounded_value_identity identity{};
    T rounded_nominal = T(0);
    finite_interval<T> enclosure = finite_interval<T>::singleton(T(0));
    uncertainty_contributors contributors{};
};

template<class T>
struct construction_operation_certificate final {
    std::uint16_t schema_version = contract_versions::construction_conditioning;
    context_owner_token owner{};
    rounded_operation_code operation = rounded_operation_code::invalid;
    std::uint16_t formula_version = 0;
    std::vector<bounded_operation_parent_identity> ordered_inputs;
    std::array<construction_issued_output_evidence<T>, 6> issued_outputs{};
    std::array<T, 6> axis_error_upper{};
    std::uint8_t component_count = 0;
    T radial_error_upper = T(0);
    finite_interval<T> denominator = finite_interval<T>::singleton(T(0));
    T conditioning_lower = T(0);
    construction_category conditioning = construction_category::invalid;
    construction_tolerance_disposition tolerance =
        construction_tolerance_disposition::invalid;
    T tolerance_boundary = T(0);
    bool complete = false;
};

template<class T>
bool valid_construction_operation_certificate(
    const construction_operation_certificate<T> &value) noexcept {
    if (value.schema_version != contract_versions::construction_conditioning ||
        !value.owner.anchor ||
        !registered_rounded_operation(value.operation) ||
        value.formula_version == 0 || value.component_count == 0 ||
        value.component_count > value.axis_error_upper.size() ||
        !value.complete ||
        !finite_bits(value.denominator.lower()) ||
        !finite_bits(value.denominator.upper()) ||
        finite_numeric_less(value.denominator.upper(), value.denominator.lower()) ||
        !finite_bits(value.conditioning_lower) || value.conditioning_lower < T(0) ||
        value.conditioning == construction_category::invalid ||
        value.tolerance == construction_tolerance_disposition::invalid ||
        !finite_bits(value.tolerance_boundary) || value.tolerance_boundary < T(0) ||
        !finite_bits(value.radial_error_upper) || value.radial_error_upper < T(0))
        return false;
    for (const auto &input : value.ordered_inputs)
        if (input.value.ordinal() == 0 || input.trace_root == 0 ||
            input.ledger_entry.ordinal() == 0)
            return false;
    for (std::size_t component = 0; component < value.component_count; ++component) {
        if (!finite_bits(value.axis_error_upper[component]) ||
            value.axis_error_upper[component] < T(0))
            return false;
        const auto &issued = value.issued_outputs[component];
        if (!issued.identity.owner.same_owner(value.owner) ||
            !bounded_operations_detail::bounded_operation_lineage_valid(
                issued.identity) ||
            !finite_bits(issued.rounded_nominal) ||
            !finite_bits(issued.enclosure.lower()) ||
            !finite_bits(issued.enclosure.upper()) ||
            !issued.enclosure.contains(issued.rounded_nominal) ||
            !bounded_operations_detail::finite_contributors(
                issued.contributors))
            return false;
    }
    return true;
}

template<class T>
bool construction_certificate_matches_outputs(
    const construction_operation_certificate<T> &certificate,
    const std::vector<const bounded_scalar<T> *> &outputs) noexcept {
    if (!valid_construction_operation_certificate(certificate) ||
        outputs.size() != certificate.component_count)
        return false;
    for (std::size_t component = 0; component < outputs.size(); ++component) {
        const auto *output = outputs[component];
        const auto &issued = certificate.issued_outputs[component];
        const auto &expected = issued.identity;
        if (!output || !output->identity.owner.same_owner(certificate.owner) ||
            output->rounded_nominal != issued.rounded_nominal ||
            output->uncertainty_enclosure.lower() != issued.enclosure.lower() ||
            output->uncertainty_enclosure.upper() != issued.enclosure.upper() ||
            output->identity.schema_version != expected.schema_version ||
            output->identity.provider_version != expected.provider_version ||
            output->identity.value != expected.value ||
            output->identity.provenance != expected.provenance ||
            output->identity.lineage != expected.lineage ||
            output->identity.ledger_entry != expected.ledger_entry ||
            output->identity.trace_root != expected.trace_root ||
            output->identity.operation != expected.operation ||
            output->identity.ordered_parent_values !=
                expected.ordered_parent_values ||
            output->identity.ordered_parent_trace_roots !=
                expected.ordered_parent_trace_roots ||
            output->identity.ordered_parent_ledger_entries !=
                expected.ordered_parent_ledger_entries)
            return false;
        const double actual[]{output->contributors.inherited_a,
            output->contributors.inherited_b, output->contributors.machine_floor,
            output->contributors.construction, output->contributors.conditioning,
            output->contributors.conversion, output->contributors.prior_cleanup,
            output->contributors.current_cleanup};
        const double retained[]{issued.contributors.inherited_a,
            issued.contributors.inherited_b, issued.contributors.machine_floor,
            issued.contributors.construction, issued.contributors.conditioning,
            issued.contributors.conversion, issued.contributors.prior_cleanup,
            issued.contributors.current_cleanup};
        for (std::size_t field = 0; field < 8; ++field)
            if (actual[field] != retained[field])
                return false;
    }
    return true;
}

template<class T>
void encode_construction_operation_certificate(
    canonical_writer &writer,
    const construction_operation_certificate<T> &value) {
    writer.u16(value.schema_version);
    writer.u16(static_cast<std::uint16_t>(value.operation));
    writer.u16(value.formula_version);
    writer.u64(value.ordered_inputs.size());
    for (const auto &input : value.ordered_inputs) {
        writer.u64(input.value.ordinal());
        writer.u64(input.trace_root);
        writer.u64(input.ledger_entry.ordinal());
    }
    writer.u8(value.component_count);
    for (std::size_t component = 0; component < value.component_count; ++component) {
        const auto &issued = value.issued_outputs[component];
        const auto &identity = issued.identity;
        writer.u16(identity.schema_version);
        writer.u16(identity.provider_version);
        writer.u64(identity.value.ordinal());
        writer.u64(identity.provenance.ordinal());
        writer.u64(identity.lineage.ordinal());
        writer.u64(identity.ledger_entry.ordinal());
        writer.u64(identity.trace_root);
        writer.u16(static_cast<std::uint16_t>(identity.operation));
        writer.u64(identity.ordered_parent_values.size());
        for (std::size_t parent = 0;
             parent < identity.ordered_parent_values.size(); ++parent) {
            writer.u64(identity.ordered_parent_values[parent].ordinal());
            writer.u64(identity.ordered_parent_trace_roots[parent]);
            writer.u64(identity.ordered_parent_ledger_entries[parent].ordinal());
        }
        writer.u8(static_cast<std::uint8_t>(identity.publication));
        writer.floating(issued.rounded_nominal);
        writer.floating(issued.enclosure.lower());
        writer.floating(issued.enclosure.upper());
        const double contributors[]{issued.contributors.inherited_a,
            issued.contributors.inherited_b, issued.contributors.machine_floor,
            issued.contributors.construction, issued.contributors.conditioning,
            issued.contributors.conversion, issued.contributors.prior_cleanup,
            issued.contributors.current_cleanup};
        for (const auto contributor : contributors) writer.floating(contributor);
    }
    for (const auto error : value.axis_error_upper) writer.floating(error);
    writer.floating(value.radial_error_upper);
    writer.floating(value.denominator.lower());
    writer.floating(value.denominator.upper());
    writer.floating(value.conditioning_lower);
    writer.u8(static_cast<std::uint8_t>(value.conditioning));
    writer.u8(static_cast<std::uint8_t>(value.tolerance));
    writer.floating(value.tolerance_boundary);
    writer.boolean(value.complete);
}

template<class T>
boolean_outcome<construction_operation_certificate<T>>
certify_construction_components(
    rounded_operation_code operation, std::uint16_t formula_version,
    const context_owner_token &owner,
    const std::vector<const bounded_scalar<T> *> &outputs, T radial_error_upper,
    const std::vector<const bounded_scalar<T> *> &ordered_inputs,
    const finite_interval<T> &denominator, construction_category conditioning,
    construction_tolerance_disposition tolerance, T tolerance_boundary) {
    using certificate_type = construction_operation_certificate<T>;
    if (!owner.anchor || !registered_rounded_operation(operation) ||
        formula_version == 0 || outputs.empty() || outputs.size() > 6 ||
        ordered_inputs.empty() || !finite_bits(radial_error_upper) ||
        radial_error_upper < T(0) || !finite_bits(denominator.lower()) ||
        !finite_bits(denominator.upper()) ||
        finite_numeric_less(denominator.upper(), denominator.lower()) ||
        conditioning == construction_category::invalid ||
        tolerance == construction_tolerance_disposition::invalid ||
        !finite_bits(tolerance_boundary) || tolerance_boundary < T(0))
        return boolean_outcome<certificate_type>::failure(
            bounded_operations_detail::arithmetic_error(31312));

    certificate_type out;
    out.owner = owner;
    out.operation = operation;
    out.formula_version = formula_version;
    out.denominator = denominator;
    out.conditioning = conditioning;
    out.tolerance = tolerance;
    out.tolerance_boundary = tolerance_boundary;
    for (const auto *input : ordered_inputs) {
        if (!input || !bounded_operations_detail::bounded_scalar_valid(*input) ||
            !input->identity.owner.same_owner(owner) ||
            !bounded_operations_detail::bounded_operation_lineage_valid(
                input->identity))
            return boolean_outcome<certificate_type>::failure(
                bounded_operations_detail::arithmetic_error(31313));
        out.ordered_inputs.push_back(
            {input->identity.value, input->identity.trace_root,
             input->identity.ledger_entry});
    }
    out.component_count = static_cast<std::uint8_t>(outputs.size());
    out.radial_error_upper = radial_error_upper;
    for (std::size_t component = 0; component < outputs.size(); ++component) {
        const auto *output = outputs[component];
        if (!output || !bounded_operations_detail::bounded_scalar_valid(*output) ||
            !output->identity.owner.same_owner(owner) ||
            !bounded_operations_detail::bounded_operation_lineage_valid(
                output->identity))
            return boolean_outcome<certificate_type>::failure(
                bounded_operations_detail::arithmetic_error(31314));
        auto &issued = out.issued_outputs[component];
        issued.identity = output->identity;
        issued.rounded_nominal = output->rounded_nominal;
        issued.enclosure = output->uncertainty_enclosure;
        issued.contributors = output->contributors;
        const auto below = directed_subtract(output->rounded_nominal,
                                             output->uncertainty_enclosure.lower());
        const auto above = directed_subtract(output->uncertainty_enclosure.upper(),
                                             output->rounded_nominal);
        if (!below || !above)
            return boolean_outcome<certificate_type>::failure(
                bounded_operations_detail::arithmetic_error(31314));
        out.axis_error_upper[component] =
            std::max(below.value.upper, above.value.upper);
    }
    if (!denominator.contains_zero())
        out.conditioning_lower = denominator.lower() > T(0)
            ? denominator.lower() : -denominator.upper();
    out.complete = true;
    if (!valid_construction_operation_certificate(out) ||
        !construction_certificate_matches_outputs(out, outputs))
        return boolean_outcome<certificate_type>::failure(
            bounded_operations_detail::arithmetic_error(31316));
    return boolean_outcome<certificate_type>::success(std::move(out));
}

template<class T>
boolean_outcome<construction_operation_certificate<T>>
certify_construction_operation(
    rounded_operation_code operation, std::uint16_t formula_version,
    const bounded_point3<T> &result,
    const std::vector<const bounded_scalar<T> *> &ordered_inputs,
    const finite_interval<T> &denominator, construction_category conditioning,
    construction_tolerance_disposition tolerance, T tolerance_boundary) {
    std::vector<const bounded_scalar<T> *> outputs;
    outputs.reserve(3);
    for (const auto &component : result.coordinates.components)
        outputs.push_back(&component);
    return certify_construction_components(
        operation, formula_version, result.owner, outputs,
        result.coordinates.radial_error_upper, ordered_inputs, denominator,
        conditioning, tolerance, tolerance_boundary);
}

template<class T>
struct construction_conditioning final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    construction_id id{0};
    construction_kind kind = construction_kind::edge_plane;
    std::uint16_t rounded_graph_code = 0;
    std::uint16_t exact_formula_code = 0;
    bounded_parameter<T> parameter{};
    finite_interval<T> denominator{};
    exact_relation_evidence denominator_relation{};
    bounded_residual<T> carrier_residual{};
    bounded_residual<T> support_residual{};
    bounded_point3<T> constructed_point{};
    bounded_residual<T> residual_after{};
    bool has_constructed_point = false;
    uncertainty_contributors contributors{};
    T available_tolerance{};
    T required_precision{};
    T amplification_upper{};
    bool cancellation_detected = false;
    construction_category category = construction_category::invalid;
    std::uint64_t trace_root = 0;
};

template<class T>
construction_category classify_construction(const bounded_parameter<T> &parameter,
                                             const finite_interval<T> &denominator,
                                             exact_relation_status endpoint_relation,
                                             exact_relation_status coplanar_relation,
                                             T required_precision,
                                             T available_tolerance) noexcept {
    if (!std::isfinite(required_precision) || !std::isfinite(available_tolerance) ||
        required_precision < T(0) || available_tolerance < T(0) ||
        finite_numeric_less(denominator.upper(), denominator.lower()))
        return construction_category::invalid;
    if (coplanar_relation == exact_relation_status::exact_zero)
        return construction_category::coplanar_or_coincident;
    if (denominator.contains_zero())
        return endpoint_relation == exact_relation_status::exact_zero
            ? construction_category::exact_stored_coordinate_tie
            : construction_category::ill_conditioned;
    if (required_precision > available_tolerance) return construction_category::ill_conditioned;
    if (parameter.value.uncertainty_enclosure.lower() > T(0) &&
        parameter.value.uncertainty_enclosure.upper() < T(1))
        return construction_category::stable_interior;
    if (endpoint_relation == exact_relation_status::exact_zero)
        return construction_category::stable_endpoint;
    if (parameter.value.uncertainty_enclosure.lower() >= T(0) &&
        parameter.value.uncertainty_enclosure.upper() <= T(1))
        return construction_category::near_parallel_bounded;
    return construction_category::ill_conditioned;
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_edge_plane(
    const bounded_residual<T> &at_a,
    const bounded_residual<T> &at_b,
    exact_relation_evidence denominator_relation,
    exact_relation_evidence endpoint_relation,
    exact_relation_evidence coplanar_relation,
    bounded_residual<T> carrier_residual,
    bounded_residual<T> support_residual,
    T required_precision,
    T available_tolerance,
    construction_id id = construction_id(0)) {
    using namespace bounded_operations_detail;
    const auto &owner = at_a.owner;
    if (!owner_bound(owner) || !same_bound_owner(owner, at_b.owner) ||
        !same_bound_owner(owner, denominator_relation.owner) ||
        !same_bound_owner(owner, endpoint_relation.owner) ||
        !same_bound_owner(owner, coplanar_relation.owner) ||
        !same_bound_owner(owner, carrier_residual.owner) ||
        !same_bound_owner(owner, support_residual.owner) ||
        !same_bound_owner(owner, at_a.value.identity.owner) ||
        !same_bound_owner(owner, at_b.value.identity.owner) ||
        !same_bound_owner(owner, carrier_residual.value.identity.owner) ||
        !same_bound_owner(owner, support_residual.value.identity.owner) ||
        !bounded_scalar_valid(at_a.value) || !bounded_scalar_valid(at_b.value) ||
        !bounded_scalar_valid(carrier_residual.value) || !bounded_scalar_valid(support_residual.value) ||
        !finite_bits(at_a.scale) || !finite_bits(at_b.scale) ||
        !finite_bits(at_a.comparison_boundary) || !finite_bits(at_b.comparison_boundary) ||
        !finite_bits(carrier_residual.scale) || !finite_bits(support_residual.scale) ||
        !finite_bits(carrier_residual.comparison_boundary) ||
        !finite_bits(support_residual.comparison_boundary) ||
        !finite_bits(required_precision) || !finite_bits(available_tolerance) ||
        required_precision < T(0) || available_tolerance < T(0) ||
        endpoint_relation.status == exact_relation_status::invalid ||
        coplanar_relation.status == exact_relation_status::invalid ||
        carrier_residual.disposition != residual_disposition::pass ||
        support_residual.disposition != residual_disposition::pass)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31301));
    auto denominator = bounded_subtract(at_a.value, at_b.value);
    if (!denominator.has_value())
        return boolean_outcome<construction_conditioning<T>>::failure(*denominator.error());
    const auto denominator_sign = classify_bounded_sign(denominator.value()->uncertainty_enclosure);
    if ((denominator_sign == bounded_sign_status::definitely_negative &&
         (denominator_relation.status == exact_relation_status::exact_positive ||
          denominator_relation.status == exact_relation_status::exact_zero)) ||
        (denominator_sign == bounded_sign_status::definitely_positive &&
         (denominator_relation.status == exact_relation_status::exact_negative ||
          denominator_relation.status == exact_relation_status::exact_zero)) ||
        denominator_relation.status == exact_relation_status::invalid)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31302));
    if ((endpoint_relation.status == exact_relation_status::exact_zero &&
         !at_a.value.uncertainty_enclosure.contains_zero() &&
         !at_b.value.uncertainty_enclosure.contains_zero()) ||
        (coplanar_relation.status == exact_relation_status::exact_zero &&
         (!at_a.value.uncertainty_enclosure.contains_zero() ||
          !at_b.value.uncertainty_enclosure.contains_zero())))
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31302));
    construction_conditioning<T> out;
    out.owner = owner;
    out.id = id;
    out.rounded_graph_code = 1;
    out.exact_formula_code = denominator_relation.formula_code;
    out.denominator = denominator.value()->uncertainty_enclosure;
    out.denominator_relation = std::move(denominator_relation);
    out.carrier_residual = std::move(carrier_residual);
    out.support_residual = std::move(support_residual);
    out.required_precision = required_precision;
    out.available_tolerance = available_tolerance;
    out.parameter.owner = owner;
    if (coplanar_relation.status == exact_relation_status::exact_zero ||
        endpoint_relation.status == exact_relation_status::exact_zero) {
        const T endpoint = at_a.value.uncertainty_enclosure.contains_zero() ? T(0) : T(1);
        auto endpoint_value = checked_bounded_singleton(owner, endpoint);
        if (!endpoint_value.has_value())
            return boolean_outcome<construction_conditioning<T>>::failure(*endpoint_value.error());
        out.parameter.value = std::move(*endpoint_value.value());
        out.parameter.domain = endpoint_relation.status == exact_relation_status::exact_zero
            ? parameter_domain_status::stable_endpoint : parameter_domain_status::overlaps_boundary;
        out.cancellation_detected = out.denominator.contains_zero();
        out.category = coplanar_relation.status == exact_relation_status::exact_zero
            ? construction_category::coplanar_or_coincident
            : construction_category::exact_stored_coordinate_tie;
        if (!sum_contributors(at_a.contributors, at_b.contributors, out.contributors) ||
            !sum_contributors(out.contributors, out.carrier_residual.contributors, out.contributors) ||
            !sum_contributors(out.contributors, out.support_residual.contributors, out.contributors))
            return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31304));
        return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
    }
    auto parameter_value = bounded_divide(at_a.value, *denominator.value());
    if (!parameter_value.has_value())
        return boolean_outcome<construction_conditioning<T>>::failure(*parameter_value.error());
    out.parameter.value = std::move(*parameter_value.value());
    const auto &i = out.parameter.value.uncertainty_enclosure;
    out.parameter.domain = i.lower() > T(0) && i.upper() < T(1)
        ? parameter_domain_status::stable_interior
        : (i.lower() >= T(0) && i.upper() <= T(1)
            ? (endpoint_relation.status == exact_relation_status::exact_zero
                ? parameter_domain_status::stable_endpoint
                : parameter_domain_status::overlaps_boundary)
            : parameter_domain_status::outside);
    out.parameter.domain_margin = i.lower() > T(0) && i.upper() < T(1)
        ? std::min(i.lower(), T(1) - i.upper()) : T(0);
    if (out.parameter.domain == parameter_domain_status::outside ||
        out.parameter.domain == parameter_domain_status::overlaps_boundary)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31303));
    if (!sum_contributors(at_a.contributors, at_b.contributors, out.contributors) ||
        !sum_contributors(out.contributors, out.carrier_residual.contributors, out.contributors) ||
        !sum_contributors(out.contributors, out.support_residual.contributors, out.contributors))
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31304));
    const T denominator_distance = denominator_sign == bounded_sign_status::definitely_positive
        ? out.denominator.lower() : -out.denominator.upper();
    const auto amplification = directed_divide(T(1), denominator_distance);
    if (!amplification)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31305));
    out.amplification_upper = amplification.value.upper;
    out.cancellation_detected = out.denominator.contains_zero();
    out.category = classify_construction(out.parameter, out.denominator,
                                         endpoint_relation.status, coplanar_relation.status,
                                         required_precision, available_tolerance);
    if (out.category == construction_category::invalid)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31306));
    return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<construction_conditioning<T>> construct_edge_plane(
    const bounded_point3<T> &a, const bounded_point3<T> &b, const bounded_plane3<T> &plane,
    exact_relation_evidence denominator_relation, exact_relation_evidence endpoint_relation,
    exact_relation_evidence coplanar_relation, T required_precision, T available_tolerance,
    construction_id id = construction_id(0)) {
    auto at_a = bounded_plane_residual(plane, a, available_tolerance);
    auto at_b = bounded_plane_residual(plane, b, available_tolerance);
    if (!at_a.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*at_a.error());
    if (!at_b.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*at_b.error());
    auto carrier = *at_a.value(); auto support = *at_b.value();
    carrier.comparison_boundary = std::max(std::fabs(carrier.value.uncertainty_enclosure.lower()),
                                           std::fabs(carrier.value.uncertainty_enclosure.upper()));
    support.comparison_boundary = std::max(std::fabs(support.value.uncertainty_enclosure.lower()),
                                           std::fabs(support.value.uncertainty_enclosure.upper()));
    carrier.disposition = residual_disposition::pass; support.disposition = residual_disposition::pass;
    auto conditioned = condition_edge_plane(*at_a.value(), *at_b.value(),
        std::move(denominator_relation), std::move(endpoint_relation), std::move(coplanar_relation),
        std::move(carrier), std::move(support), required_precision, available_tolerance, id);
    if (!conditioned.has_value()) return conditioned;
    if (conditioned.value()->category == construction_category::coplanar_or_coincident)
        return conditioned;
    auto point = bounded_interpolate_from_a(a, b, conditioned.value()->parameter);
    if (!point.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*point.error());
    auto after = bounded_plane_residual(plane, *point.value(), available_tolerance);
    if (!after.has_value() || after.value()->disposition != residual_disposition::pass)
        return boolean_outcome<construction_conditioning<T>>::failure(
            bounded_operations_detail::arithmetic_error(31307));
    conditioned.value()->constructed_point = std::move(*point.value());
    conditioned.value()->residual_after = std::move(*after.value());
    conditioned.value()->carrier_residual = conditioned.value()->residual_after;
    conditioned.value()->support_residual = conditioned.value()->residual_after;
    conditioned.value()->has_constructed_point = true;
    return conditioned;
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_edge_plane(
    const bounded_point3<T> &a, const bounded_point3<T> &b, const bounded_plane3<T> &plane,
    exact_relation_evidence denominator_relation, exact_relation_evidence endpoint_relation,
    exact_relation_evidence coplanar_relation, T required_precision, T available_tolerance,
    construction_id id = construction_id(0)) {
    return construct_edge_plane(a, b, plane, std::move(denominator_relation),
        std::move(endpoint_relation), std::move(coplanar_relation), required_precision,
        available_tolerance, id);
}

template<class T>
boolean_outcome<construction_conditioning<T>> construct_edge_face(
    const bounded_point3<T> &a, const bounded_point3<T> &b,
    const bounded_point3<T> &face_a, const bounded_point3<T> &face_b,
    const bounded_point3<T> &face_c, exact_relation_evidence denominator_relation,
    exact_relation_evidence endpoint_relation, exact_relation_evidence coplanar_relation,
    T required_precision, T available_tolerance, construction_id id = construction_id(0)) {
    auto plane = bounded_plane_from_points(face_a, face_b, face_c);
    if (!plane.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*plane.error());
    auto result = construct_edge_plane(a, b, *plane.value(), std::move(denominator_relation),
        std::move(endpoint_relation), std::move(coplanar_relation), required_precision,
        available_tolerance, id);
    if (result.has_value()) result.value()->kind = construction_kind::edge_face;
    return result;
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_edge_face(
    const bounded_point3<T> &a, const bounded_point3<T> &b,
    const bounded_point3<T> &face_a, const bounded_point3<T> &face_b,
    const bounded_point3<T> &face_c, exact_relation_evidence denominator_relation,
    exact_relation_evidence endpoint_relation, exact_relation_evidence coplanar_relation,
    T required_precision, T available_tolerance, construction_id id = construction_id(0)) {
    return construct_edge_face(a, b, face_a, face_b, face_c, std::move(denominator_relation),
        std::move(endpoint_relation), std::move(coplanar_relation), required_precision,
        available_tolerance, id);
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_projection(
    const bounded_plane3<T> &plane, const bounded_point3<T> &point,
    T required_precision, T available_tolerance, construction_id id = construction_id(0)) {
    using namespace bounded_operations_detail;
    if (!same_bound_owner(plane.owner, point.owner) || !finite_bits(required_precision) ||
        !finite_bits(available_tolerance) || required_precision < T(0) || available_tolerance < T(0))
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31308));
    auto before = bounded_plane_residual(plane, point, available_tolerance);
    auto projected = bounded_project_onto_plane(plane, point);
    if (!before.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*before.error());
    if (!projected.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*projected.error());
    auto projection_parameter = bounded_divide(before.value()->value, plane.normal_sq);
    if (!projection_parameter.has_value())
        return boolean_outcome<construction_conditioning<T>>::failure(*projection_parameter.error());
    auto after = bounded_plane_residual(plane, *projected.value(), available_tolerance);
    if (!after.has_value() || after.value()->disposition != residual_disposition::pass)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31309));
    construction_conditioning<T> out;
    out.owner = plane.owner; out.id = id; out.kind = construction_kind::projection;
    out.rounded_graph_code = 2; out.denominator = plane.normal_sq.uncertainty_enclosure;
    out.parameter.owner = plane.owner; out.parameter.value = std::move(*projection_parameter.value());
    out.parameter.carrier = parameter_carrier::face;
    out.parameter.domain = parameter_domain_status::stable_interior;
    out.denominator_relation.owner = plane.owner;
    out.denominator_relation.status = exact_relation_status::unavailable;
    out.carrier_residual = *before.value(); out.support_residual = *after.value();
    out.residual_after = *after.value(); out.constructed_point = std::move(*projected.value());
    out.has_constructed_point = true; out.required_precision = required_precision;
    out.available_tolerance = available_tolerance; out.category = required_precision <= available_tolerance
        ? construction_category::stable_interior : construction_category::ill_conditioned;
    out.contributors = out.constructed_point.coordinates.components[0].contributors;
    for (unsigned axis = 1; axis < 3; ++axis)
        if (!sum_contributors(out.contributors,
                out.constructed_point.coordinates.components[axis].contributors, out.contributors))
            return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31310));
    const auto amplification = directed_divide(T(1), out.denominator.lower());
    if (!amplification) return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31311));
    out.amplification_upper = amplification.value.upper;
    return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
