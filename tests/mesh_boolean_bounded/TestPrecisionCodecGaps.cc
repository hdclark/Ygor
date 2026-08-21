#include "YgorMeshesBooleanBounded/PrecisionVerifier.h"

#include <stdexcept>

namespace bounded = ygor::mesh_boolean::bounded;

namespace {
void require(bool value) { if (!value) throw std::runtime_error("Component 03 codec gap test failed"); }

bounded::bounded_scalar<double> scalar(const bounded::context_owner_token &owner, double value) {
    auto out = bounded::checked_bounded_singleton(owner, value);
    require(out.has_value());
    return *out.value();
}

bounded::bounded_point3<double> point(const bounded::context_owner_token &owner,
                                      double x, double y, double z) {
    bounded::bounded_point3<double> out;
    out.owner = owner; out.coordinates.owner = owner;
    out.coordinates.components = {{scalar(owner, x), scalar(owner, y), scalar(owner, z)}};
    require(bounded::bounded_operations_detail::compute_radial_error(out.coordinates));
    return out;
}

bounded::exact_relation_evidence relation(const bounded::context_owner_token &owner,
                                           bounded::exact_relation_status status) {
    bounded::exact_relation_evidence out;
    out.owner = owner; out.formula_code = static_cast<std::uint16_t>(
        bounded::exact_relation_formula_code::finite_scalar_comparison);
    out.status = status; out.capacity_used = 1;
    return out;
}

bounded::bounded_residual<double> residual(const bounded::context_owner_token &owner, double value) {
    bounded::bounded_residual<double> out;
    out.owner = owner; out.value = scalar(owner, value); out.scale = 1.0;
    out.comparison_boundary = 1.0; out.disposition = bounded::residual_disposition::pass;
    return out;
}
}

int main() {
    const auto owner = bounded::context_owner_token::create();
    const auto value = scalar(owner, 0.25);
    const auto value_bytes = bounded::encode_bounded_value(value);
    require(!bounded::decode_bounded_value<double>(value_bytes).has_value());
    auto decoded_value = bounded::decode_bounded_value<double>(value_bytes, owner);
    require(decoded_value.has_value() && bounded::verify_bounded_value(*decoded_value.value(), &owner));

    const auto directed = bounded::directed_multiply(0.1, 0.2);
    auto decoded_directed = bounded::decode_directed_result<double>(bounded::encode_directed_result(directed));
    require(decoded_directed.has_value() && bounded::verify_directed_result(*decoded_directed.value(), 0.1, 0.2));

    auto predicate = bounded::assemble_predicate_result(scalar(owner, 1.0),
        relation(owner, bounded::exact_relation_status::exact_positive));
    require(predicate.has_value());
    auto decoded_predicate = bounded::decode_predicate_result<double>(
        bounded::encode_predicate_result(*predicate.value()), owner);
    require(decoded_predicate.has_value() && bounded::verify_predicate_result(*decoded_predicate.value(), &owner));

    auto tie = bounded::condition_edge_plane(residual(owner, 0.0), residual(owner, 0.0),
        relation(owner, bounded::exact_relation_status::exact_zero),
        relation(owner, bounded::exact_relation_status::exact_zero),
        relation(owner, bounded::exact_relation_status::exact_positive),
        residual(owner, 0.0), residual(owner, 0.0), 0.0, 1.0);
    require(tie.has_value() && tie.value()->category == bounded::construction_category::exact_stored_coordinate_tie);
    auto decoded_tie = bounded::decode_construction<double>(bounded::encode_construction(*tie.value()), owner);
    require(decoded_tie.has_value() && bounded::verify_construction(*decoded_tie.value(), &owner));

    auto plane = bounded::bounded_plane_from_points(point(owner, 0.0, 0.0, 0.0),
        point(owner, 1.0, 0.0, 0.0), point(owner, 0.0, 1.0, 0.0));
    require(plane.has_value());
    auto projection = bounded::condition_projection(*plane.value(), point(owner, 0.25, 0.25, 2.0),
                                                     0.0, 1.0);
    require(projection.has_value() && projection.value()->has_constructed_point &&
            projection.value()->residual_after.disposition == bounded::residual_disposition::pass);

    const auto bound = bounded::point_bound_record(point(owner, 1.0, 2.0, 3.0));
    auto decoded_bound = bounded::decode_conservative_bound<double>(
        bounded::encode_conservative_bound(bound), owner);
    require(decoded_bound.has_value() && decoded_bound.value()->bound.owner.same_owner(owner));

    bounded::foreign_precision_provenance imported;
    imported.owner = owner; imported.prior_output_precision = 0.5; imported.inherited_precision = 0.1;
    imported.construction_uncertainty = 0.1; imported.cumulative_cleanup_displacement = 0.1;
    imported.serialization_contribution = 0.1; imported.construction_history_digest = {1};
    imported.prior_context_digest = {2}; imported.replay_lineage = {3};
    imported.publication_digest = {4}; imported.verified_publication_digest = {4};
    imported.verification_evidence = {5}; imported.publication_verified = true;
    auto decoded_import = bounded::decode_precision_import(bounded::encode_precision_import(imported), owner);
    require(decoded_import.has_value() && bounded::verify_precision_import_record(*decoded_import.value(), 1.0));

    bounded_boolean_error failure;
    failure.category = bounded_boolean_error_category::geometric_condition_exceeds_tolerance;
    failure.component = 3; failure.witness_count = 1; failure.witnesses[0] = 7;
    auto decoded_failure = bounded::decode_failure(bounded::encode_failure(failure));
    require(decoded_failure && bounded::verify_failure(*decoded_failure));
    return 0;
}
