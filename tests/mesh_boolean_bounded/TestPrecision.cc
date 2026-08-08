#include "YgorMeshesBooleanBounded/BoundedOperations.h"
#include "YgorMeshesBooleanBounded/ConservativeBounds.h"
#include "YgorMeshesBooleanBounded/ConstructionConditioning.h"
#include "YgorMeshesBooleanBounded/ExactFloatExpansion.h"
#include "YgorMeshesBooleanBounded/FloatingBits.h"
#include "YgorMeshesBooleanBounded/PrecisionCodec.h"
#include "YgorMeshesBooleanBounded/PrecisionImport.h"
#include "YgorMeshesBooleanBounded/PrecisionLedger.h"
#include "YgorMeshesBooleanBounded/PrecisionTrace.h"
#include "YgorMeshesBooleanBounded/PrecisionVerifier.h"
#include "YgorMeshesBooleanBounded/ToleranceBudget.h"
#include "qualification/ExactFloatImport.h"
#include "qualification/ExactGeometryOracle.h"

#include <array>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
namespace qualification = ygor::mesh_boolean::qualification;

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

const bounded::context_owner_token context_owner = bounded::context_owner_token::create();

template<class T>
T unwrap(bounded::boolean_outcome<T> outcome, const char *message) {
    require(outcome.has_value() && outcome.value() != nullptr, message);
    return std::move(*outcome.value());
}

template<class T>
qualification::ExactRational exact(T value) {
    return qualification::import_exact(value).value;
}

bounded::exact_relation_status relation_from_sign(int sign) {
    return sign < 0 ? bounded::exact_relation_status::exact_negative
         : sign > 0 ? bounded::exact_relation_status::exact_positive
                    : bounded::exact_relation_status::exact_zero;
}

template<class T>
void test_floating_bits() {
    using traits = bounded::floating_bits_traits<T>;
    const T negative_zero = bounded::from_bits<T>(traits::sign_mask);
    require(bounded::to_bits(negative_zero) == traits::sign_mask, "floating bits signed zero roundtrip");
    require(bounded::negative_zero(negative_zero) && !bounded::negative_zero(T(0)),
            "floating bits distinguishes signed zero");
    require(bounded::finite_bits(std::numeric_limits<T>::max()) &&
            !bounded::finite_bits(std::numeric_limits<T>::infinity()), "floating bits finiteness");
    require(bounded::finite_total_less(negative_zero, T(0)) &&
            !bounded::finite_numeric_less(negative_zero, T(0)), "signed zero total and numeric order");

    T adjacent = T(0);
    require(bounded::next_up_finite(negative_zero, adjacent) &&
            adjacent == std::numeric_limits<T>::denorm_min(), "next up from negative zero");
    require(bounded::next_down_finite(T(0), adjacent) &&
            adjacent == -std::numeric_limits<T>::denorm_min(), "next down from positive zero");
    require(!bounded::next_up_finite(std::numeric_limits<T>::max(), adjacent) &&
            !bounded::next_down_finite(-std::numeric_limits<T>::max(), adjacent),
            "finite adjacency rejects range endpoints");
    require(bounded::unbiased_exponent(T(1)) == 0 &&
            bounded::unbiased_exponent(T(0)) == std::numeric_limits<int>::min(),
            "unbiased exponent normal and zero");

    const auto imported_negative_zero = qualification::import_exact(negative_zero);
    require(imported_negative_zero.negative_zero && imported_negative_zero.is_zero(),
            "qualification exact import preserves negative zero");
    const T sample = std::nextafter(T(1), T(2));
    require(qualification::import_exact(sample).source_bits == bounded::to_bits(sample),
            "qualification exact import agrees on source bits");
}

template<class T>
void require_encloses(const bounded::directed_operation_result<T> &result,
                      const qualification::ExactRational &expected,
                      const char *message) {
    require(result && exact(result.value.lower) <= expected && expected <= exact(result.value.upper), message);
}

template<class T>
void test_directed_rounding() {
    auto add_exact = bounded::directed_add(T(1), T(2));
    require(add_exact && add_exact.value.rounded == T(3) &&
            add_exact.value.exactness == bounded::rounding_exactness::exact,
            "directed exact add");
    const T epsilon = std::numeric_limits<T>::epsilon();
    auto add_inexact = bounded::directed_add(T(1), epsilon / T(2));
    require_encloses(add_inexact, exact(T(1)) + exact(epsilon / T(2)), "directed inexact add enclosure");
    require(add_inexact.value.exactness == bounded::rounding_exactness::inexact_direction_known &&
            add_inexact.value.direction == bounded::residual_sign::positive,
            "directed inexact add direction");

    auto subtract_exact = bounded::directed_subtract(T(3), T(1));
    require(subtract_exact && subtract_exact.value.rounded == T(2) &&
            subtract_exact.value.exactness == bounded::rounding_exactness::exact,
            "directed exact subtract");
    auto subtract_inexact = bounded::directed_subtract(T(1), epsilon / T(4));
    require_encloses(subtract_inexact, exact(T(1)) - exact(epsilon / T(4)),
                     "directed inexact subtract enclosure");
    require(subtract_inexact.value.direction == bounded::residual_sign::negative,
            "directed inexact subtract direction");

    auto multiply_exact = bounded::directed_multiply(T(3), T(4));
    require(multiply_exact && multiply_exact.value.rounded == T(12) &&
            multiply_exact.value.exactness == bounded::rounding_exactness::exact,
            "directed exact multiply");
    const T multiplier = std::nextafter(T(1), T(2));
    auto multiply_inexact = bounded::directed_multiply(multiplier, multiplier);
    require_encloses(multiply_inexact, exact(multiplier) * exact(multiplier),
                     "directed inexact multiply enclosure");
    require(multiply_inexact.value.exactness == bounded::rounding_exactness::inexact_direction_known,
            "directed inexact multiply exactness");

    auto divide_exact = bounded::directed_divide(T(6), T(2));
    require(divide_exact && divide_exact.value.rounded == T(3) &&
            divide_exact.value.exactness == bounded::rounding_exactness::exact,
            "directed exact divide");
    auto divide_inexact = bounded::directed_divide(T(1), T(3));
    require_encloses(divide_inexact, exact(T(1)) / exact(T(3)),
                     "directed inexact divide enclosure");
    require(divide_inexact.value.exactness == bounded::rounding_exactness::inexact_direction_known,
            "directed inexact divide exactness");
    require(bounded::directed_divide(T(1), T(0)).status == bounded::numeric_status::division_by_zero,
            "directed divide rejects zero");

    const T maximum = std::numeric_limits<T>::max();
    require(bounded::directed_add(maximum, maximum).status == bounded::numeric_status::non_finite_result,
            "directed add rejects overflow");
    require(bounded::directed_add(maximum, T(1)).status == bounded::numeric_status::outward_step_unavailable,
            "directed add reports unavailable max-finite outward step");
    require(bounded::directed_multiply(maximum, T(1)) &&
            bounded::directed_multiply(maximum, T(1)).value.rounded == maximum,
            "directed multiply exact at max finite");
    require(bounded::directed_multiply(maximum, T(2)).status == bounded::numeric_status::non_finite_result,
            "directed multiply rejects max-finite overflow");
}

template<class T>
void test_intervals() {
    require(!bounded::finite_interval<T>::create(T(2), T(1)) &&
            !bounded::finite_interval<T>::checked_singleton(std::numeric_limits<T>::infinity()),
            "finite interval rejects invalid endpoints");
    const auto zero = *bounded::finite_interval<T>::create(T(0), T(0));
    require(zero.contains_zero() && zero.contains(T(0)) && !zero.is_singleton() &&
            bounded::negative_zero(zero.lower()) && !std::signbit(zero.upper()),
            "finite interval canonical signed-zero enclosure");
    const auto a = *bounded::finite_interval<T>::create(T(1), T(2));
    const auto b = *bounded::finite_interval<T>::create(T(3), T(4));
    auto sum = bounded::interval_add(a, b);
    auto difference = bounded::interval_subtract(b, a);
    auto product = bounded::interval_multiply(a, b);
    auto quotient = bounded::interval_divide(b, a);
    require(sum && sum.value->contains(T(4)) && sum.value->contains(T(6)), "interval add enclosure");
    require(difference && difference.value->contains(T(1)) && difference.value->contains(T(3)),
            "interval subtract enclosure");
    require(product && product.value->contains(T(3)) && product.value->contains(T(8)),
            "interval multiply enclosure");
    require(quotient && quotient.value->contains(T(1.5)) && quotient.value->contains(T(4)),
            "interval divide enclosure");
    require(bounded::interval_divide(a, zero).status == bounded::numeric_status::denominator_contains_zero,
            "interval divide rejects denominator containing zero");
    bounded::interval_intersection_proof invalid_proof{};
    require(bounded::interval_intersection(a, b, invalid_proof).status == bounded::numeric_status::invalid_argument,
            "interval intersection requires proof");
}

template<class T>
qualification::ExactPoint2 exact_point2(const std::array<T, 2> &point) {
    return {exact(point[0]), exact(point[1])};
}

template<class T>
qualification::ExactPoint3 exact_point3(const std::array<T, 3> &point) {
    return {exact(point[0]), exact(point[1]), exact(point[2])};
}

template<class T>
void test_exact_geometry() {
    const T one_up = std::nextafter(T(1), T(2));
    const std::array<T, 4> matrix2{{T(1), T(1), T(1), one_up}};
    const std::array<std::array<qualification::ExactRational, 2>, 2> oracle2{{
        {{exact(matrix2[0]), exact(matrix2[1])}}, {{exact(matrix2[2]), exact(matrix2[3])}}}};
    const auto determinant2 = bounded::exact_determinant_2x2(matrix2);
    require(determinant2.evaluation_status == bounded::numeric_status::success &&
            determinant2.status == relation_from_sign(
                qualification::ExactGeometryOracle::determinant_2x2(oracle2).sign()),
            "exact 2x2 determinant agrees with oracle");

    const std::array<T, 9> matrix3{{T(1), T(2), T(3), T(0), T(1), T(4), T(5), T(6), T(0)}};
    std::array<std::array<qualification::ExactRational, 3>, 3> oracle3{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column)
            oracle3[row][column] = exact(matrix3[row * 3 + column]);
    const auto determinant3 = bounded::exact_determinant_3x3(matrix3);
    require(determinant3.status == relation_from_sign(
                qualification::ExactGeometryOracle::determinant_3x3(oracle3).sign()),
            "exact 3x3 determinant agrees with oracle");

    const std::array<T, 2> a2{{T(0), T(0)}};
    const std::array<T, 2> b2{{T(1), T(1)}};
    const std::array<T, 2> c2{{T(2), std::nextafter(T(2), T(3))}};
    const auto orientation2 = bounded::exact_orient_2d(a2, b2, c2);
    require(orientation2.status == relation_from_sign(qualification::ExactGeometryOracle::orientation_2d(
                exact_point2(a2), exact_point2(b2), exact_point2(c2))),
            "exact 2D orientation agrees with oracle");
    const std::array<T, 2> collinear{{T(2), T(2)}};
    require(bounded::exact_orient_2d(a2, b2, collinear).status == bounded::exact_relation_status::exact_zero,
            "exact 2D orientation detects tie");

    const std::array<T, 3> a3{{T(0), T(0), T(0)}};
    const std::array<T, 3> b3{{T(1), T(0), T(0)}};
    const std::array<T, 3> c3{{T(0), T(1), T(0)}};
    const std::array<T, 3> d3{{T(0), T(0), one_up}};
    const auto orientation3 = bounded::exact_orient_3d(a3, b3, c3, d3);
    require(orientation3.status == relation_from_sign(qualification::ExactGeometryOracle::orientation_3d(
                exact_point3(a3), exact_point3(b3), exact_point3(c3), exact_point3(d3))),
            "exact 3D orientation agrees with oracle");
}

bounded::bounded_scalar<double> scalar(double nominal, double lower, double upper) {
    bounded::bounded_scalar<double> out;
    out.rounded_nominal = nominal;
    out.uncertainty_enclosure = *bounded::finite_interval<double>::create(lower, upper);
    out.identity.owner = context_owner;
    return out;
}

bounded::bounded_scalar<double> scalar(double value) {
    return unwrap(bounded::bounded_singleton(context_owner, value), "bounded scalar fixture");
}

bounded::bounded_vec3<double> vector(double x, double y, double z) {
    bounded::bounded_vec3<double> out;
    out.owner = context_owner;
    out.components = {{scalar(x), scalar(y), scalar(z)}};
    return out;
}

bounded::bounded_point3<double> point(double x, double y, double z,
                                       std::uint64_t provenance = 0,
                                       std::uint64_t lineage = 0) {
    bounded::bounded_point3<double> out;
    out.owner = context_owner;
    out.coordinates = vector(x, y, z);
    out.provenance = bounded::provenance_id(provenance);
    out.lineage = bounded::geometric_lineage_id(lineage);
    return out;
}

bounded::bounded_residual<double> residual(
    double value, double comparison_boundary = 0.0,
    bounded::residual_disposition disposition = bounded::residual_disposition::pass) {
    bounded::bounded_residual<double> out;
    out.owner = context_owner;
    out.value = scalar(value);
    out.scale = 1.0;
    out.comparison_boundary = comparison_boundary;
    out.disposition = disposition;
    return out;
}

void test_truth_layers_and_bounded_geometry() {
    auto positive = scalar(2.0, 1.5, 2.5);
    positive.identity.trace_root = 17;
    positive.contributors.conditioning = 0.25;
    bounded::exact_relation_evidence exact_positive;
    exact_positive.owner = context_owner;
    exact_positive.status = bounded::exact_relation_status::exact_positive;
    const auto predicate = unwrap(bounded::assemble_predicate_result(positive, exact_positive),
                                  "predicate result assembly");
    require(predicate.rounded_and_bounded.rounded_nominal == 2.0 &&
            predicate.bounded_sign == bounded::bounded_sign_status::definitely_positive &&
            predicate.exact_relation.status == bounded::exact_relation_status::exact_positive &&
            predicate.disposition == bounded::predicate_disposition::accept_numeric_sign &&
            predicate.trace_root == 17 && predicate.separation_margin == 1.5,
            "predicate preserves rounded, exact, and enclosure truth layers");
    require(bounded::assemble_predicate_disposition(bounded::bounded_sign_status::overlaps_boundary,
                bounded::exact_relation_status::exact_zero) ==
                bounded::predicate_disposition::retain_tie_for_consumer_eligibility,
            "predicate exact tie disposition");
    require(bounded::assemble_predicate_disposition(bounded::bounded_sign_status::overlaps_boundary,
                bounded::exact_relation_status::exact_positive, true) ==
                bounded::predicate_disposition::try_permitted_alternate,
            "predicate uncertain alternate disposition");
    require(bounded::assemble_predicate_disposition(bounded::bounded_sign_status::definitely_negative,
                bounded::exact_relation_status::exact_positive) == bounded::predicate_disposition::fail_invalid,
            "predicate rejects contradictory truth layers");

    auto a = scalar(2.0, 1.5, 2.5);
    auto b = scalar(3.0, 2.5, 3.5);
    a.contributors.inherited_a = 0.1;
    b.contributors.machine_floor = 0.2;
    auto sum = bounded::bounded_add(a, b);
    auto difference = bounded::bounded_subtract(b, a);
    auto product = bounded::bounded_multiply(a, b);
    auto quotient = bounded::bounded_divide(b, a);
    require(sum.has_value() && sum.value()->uncertainty_enclosure.contains(4.0) &&
            sum.value()->uncertainty_enclosure.contains(6.0) &&
            sum.value()->contributors.machine_floor == 0.2, "bounded scalar add");
    require(difference.has_value() && difference.value()->rounded_nominal == 1.0,
            "bounded scalar subtract");
    require(product.has_value() && product.value()->rounded_nominal == 6.0, "bounded scalar multiply");
    require(quotient.has_value() && quotient.value()->rounded_nominal == 1.5, "bounded scalar divide");
    auto zero_denominator = scalar(0.0, -1.0, 1.0);
    require(!bounded::bounded_divide(a, zero_denominator).has_value(),
            "bounded scalar divide rejects denominator zero enclosure");

    const auto x = vector(1.0, 0.0, 0.0);
    const auto y = vector(0.0, 1.0, 0.0);
    auto dot = bounded::bounded_dot3(x, y);
    auto cross = bounded::bounded_cross3(x, y);
    require(dot.has_value() && dot.value()->rounded_nominal == 0.0, "bounded vector dot");
    require(cross.has_value() && cross.value()->components[2].rounded_nominal == 1.0,
            "bounded vector cross");

    const auto p0 = point(0.0, 0.0, 0.0, 4, 5);
    const auto p1 = point(1.0, 0.0, 0.0);
    const auto p2 = point(0.0, 1.0, 0.0);
    auto plane = bounded::bounded_plane_from_points(p0, p1, p2);
    require(plane.has_value() && plane.value()->normal.components[2].rounded_nominal == 1.0 &&
            plane.value()->normal_sq.rounded_nominal == 1.0, "bounded plane from points");
    auto residual = bounded::bounded_plane_residual(*plane.value(), point(0.25, 0.25, 2.0), 0.5);
    require(residual.has_value() && residual.value()->disposition == bounded::residual_disposition::fail,
            "bounded plane residual");
    auto projection = bounded::bounded_project_onto_plane(*plane.value(), point(0.25, 0.25, 2.0));
    require(projection.has_value() && projection.value()->coordinates.components[2].rounded_nominal == 0.0,
            "bounded plane projection");
    require(!bounded::bounded_plane_from_points(p0, p1, point(2.0, 0.0, 0.0)).has_value(),
            "bounded plane rejects collinear points");

    bounded::bounded_parameter<double> parameter;
    parameter.owner = context_owner;
    parameter.value = scalar(0.25);
    auto interpolated = bounded::bounded_interpolate_from_a(p0, p1, parameter);
    require(interpolated.has_value() &&
            interpolated.value()->coordinates.components[0].rounded_nominal == 0.25 &&
            interpolated.value()->provenance == p0.provenance, "bounded point interpolation");
}

void test_conditioning() {
    const auto at_a = residual(1.0, 1.0);
    const auto at_b = residual(-1.0, 1.0);
    bounded::exact_relation_evidence denominator;
    denominator.owner = context_owner;
    denominator.formula_code = 7;
    denominator.status = bounded::exact_relation_status::exact_positive;
    bounded::exact_relation_evidence endpoint;
    endpoint.owner = context_owner;
    endpoint.formula_code = 8;
    endpoint.status = bounded::exact_relation_status::exact_positive;
    bounded::exact_relation_evidence coplanar;
    coplanar.owner = context_owner;
    coplanar.status = bounded::exact_relation_status::exact_positive;
    const auto carrier = residual(0.0);
    const auto support = residual(0.0);
    auto stable = bounded::condition_edge_plane(at_a, at_b, denominator, endpoint, coplanar,
                                                 carrier, support, 0.1, 1.0,
                                                 bounded::construction_id(3));
    require(stable.has_value() && stable.value()->parameter.value.rounded_nominal == 0.5 &&
            stable.value()->parameter.domain == bounded::parameter_domain_status::stable_interior &&
            stable.value()->category == bounded::construction_category::stable_interior &&
            stable.value()->denominator_relation.formula_code == 7 &&
            stable.value()->carrier_residual.disposition == bounded::residual_disposition::pass &&
            stable.value()->support_residual.disposition == bounded::residual_disposition::pass,
            "edge-plane stable conditioning");
    require(bounded::classify_construction(stable.value()->parameter, stable.value()->denominator,
                endpoint.status, bounded::exact_relation_status::exact_zero, 0.1, 1.0) ==
                bounded::construction_category::coplanar_or_coincident,
            "conditioning coplanar classification");
    auto zero = bounded::finite_interval<double>::singleton(0.0);
    require(bounded::classify_construction(stable.value()->parameter, zero,
                bounded::exact_relation_status::exact_zero, bounded::exact_relation_status::exact_positive,
                0.1, 1.0) == bounded::construction_category::exact_stored_coordinate_tie,
            "conditioning denominator-zero exact endpoint");
    require(bounded::classify_construction(stable.value()->parameter, stable.value()->denominator,
                endpoint.status, coplanar.status, 2.0, 1.0) ==
                bounded::construction_category::ill_conditioned,
            "conditioning precision exceeds tolerance");
}

void test_trace_finalization() {
    const auto owner = bounded::context_owner_token::create();
    bounded::local_precision_trace trace(owner);
    bounded::precision_trace_key source;
    source.operation_code = 1;
    source.result_bytes = {1};
    const auto first = trace.append(source);
    const auto duplicate = trace.append(source);
    bounded::precision_trace_key root_key;
    root_key.operation_code = 4;
    root_key.parents = {first.ordinal, duplicate.ordinal};
    const auto root = trace.append(root_key);
    auto finalized = bounded::finalize_precision_trace(trace, root, bounded::precision_trace_id(9));
    require(finalized.has_value() && finalized.value()->nodes.size() == 2 &&
            finalized.value()->nodes.back().key.parents[0] == finalized.value()->nodes.back().key.parents[1] &&
            finalized.value()->owner.same_owner(owner), "precision trace canonicalizes duplicate parents");
    require(!bounded::finalize_precision_trace(trace, bounded::local_trace_node_id{99},
                                               bounded::precision_trace_id(10)).has_value(),
            "precision trace rejects mutated root");

    bounded::local_precision_trace forward(owner);
    bounded::precision_trace_key bad;
    bad.parents = {1};
    const auto bad_root = forward.append(bad);
    forward.append(source);
    require(!bounded::finalize_precision_trace(forward, bad_root, bounded::precision_trace_id(11)).has_value(),
            "precision trace rejects forward-parent mutation");
}

bounded::precision_ledger_record ledger_record(const bounded::context_owner_token &owner,
                                                 std::uint64_t id, std::uint64_t lineage,
                                                 double no_motion, double displacement) {
    bounded::precision_ledger_record record;
    record.owner = owner;
    record.id = bounded::precision_ledger_entry_id(id);
    record.result = bounded::bounded_value_id(id);
    record.lineage = bounded::geometric_lineage_id(lineage);
    record.contributors.machine_floor = no_motion;
    record.contributors.current_cleanup = displacement;
    record.no_motion_uncertainty = no_motion;
    record.cumulative_displacement = displacement;
    return record;
}

void test_precision_ledger() {
    const auto owner = bounded::context_owner_token::create();
    bounded::precision_ledger ledger(owner, 1.0);
    require(ledger.append(ledger_record(owner, 0, 4, 0.1, 0.2)).has_value(), "precision ledger first append");
    auto second = ledger_record(owner, 1, 4, 0.2, 0.1);
    second.ordered_parents.push_back(bounded::precision_ledger_entry_id(0));
    require(ledger.append(second).has_value(), "precision ledger second append");
    auto third = ledger_record(owner, 2, 7, 0.7, 0.5);
    third.ordered_parents.push_back(bounded::precision_ledger_entry_id(1));
    require(ledger.append(third).has_value(), "precision ledger cross-lineage append");
    const auto snapshot = ledger.snapshot();
    require(snapshot->records[1].lineage_precision >= snapshot->records[0].lineage_precision &&
            snapshot->lineages[0].precision >= snapshot->records[1].lineage_precision,
            "precision ledger monotonic lineage aggregation");
    require(snapshot->global_output_precision >= snapshot->lineages[0].precision &&
            !snapshot->records[2].within_tolerance, "precision ledger global maximum and tolerance");
    auto invalid = ledger_record(owner, 3, 7, 0.0, 0.0);
    invalid.ordered_parents.push_back(bounded::precision_ledger_entry_id(3));
    require(!ledger.append(invalid).has_value(), "precision ledger rejects nonpreceding parent");
}

bounded::tolerance_budget_proposal proposal(bounded::tolerance_budget &budget,
                                             const bounded::context_owner_token &owner,
                                             std::uint64_t id, double cost,
                                             std::uint64_t transaction = 12) {
    bounded::tolerance_budget_proposal out;
    out.id = bounded::budget_proposal_id(id);
    out.ordered_lineages = {bounded::geometric_lineage_id(2)};
    out.requested_costs = {cost};
    out.certificate.id = bounded::displacement_certificate_id(id);
    out.certificate.upper_length = cost;
    out.certificate.evidence = {1};
    out.owner = owner;
    out.transaction_owner = transaction;
    out.before_evidence.owner = owner;
    out.before_evidence.transaction_owner = transaction;
    out.before_evidence.cumulative_lengths = {0.0};
    out.before_evidence.evidence = {2};
    out.authorization = unwrap(budget.authorize_topology(out.operation, transaction, {3}),
                               "topology authorization fixture");
    return out;
}

bounded::budget_length_evidence after_evidence(const bounded::context_owner_token &owner,
                                                double cumulative,
                                                std::uint64_t transaction = 12) {
    bounded::budget_length_evidence out;
    out.owner = owner;
    out.transaction_owner = transaction;
    out.cumulative_lengths = {cumulative};
    out.evidence = {9};
    return out;
}

void test_tolerance_budget() {
    const auto owner = bounded::context_owner_token::create();
    bounded::precision_ledger ledger(owner, 1.0);
    bounded::tolerance_budget budget(owner, 1.0);
    {
        auto reservation = budget.reserve(proposal(budget, owner, 0, 0.4));
        require(reservation.has_value() && budget.snapshot()->active_reservations == 1,
                "tolerance budget reservation");
    }
    require(budget.snapshot()->active_reservations == 0 && budget.can_commit_stage(),
            "tolerance budget reservation rollback");

    auto reservation = budget.reserve(proposal(budget, owner, 1, 0.4));
    require(reservation.has_value(), "tolerance budget commit reservation");
    require(ledger.append(ledger_record(owner, 0, 2, 0.0, 0.25)).has_value(),
            "tolerance budget linked ledger update");
    auto committed = budget.commit(std::move(*reservation.value()), {0.25},
                                   after_evidence(owner, 0.25), 12, ledger,
                                   {bounded::precision_ledger_entry_id(0)});
    require(committed.has_value() && budget.snapshot()->global_realized_displacement == 0.25 &&
            budget.can_commit_stage(), "tolerance budget commit actual cost");

    auto boundary = budget.reserve(proposal(budget, owner, 2, 0.75));
    require(boundary.has_value(), "tolerance budget accepts exact boundary");
    require(ledger.append(ledger_record(owner, 1, 2, 0.0, 0.75)).has_value(),
            "tolerance budget boundary ledger update");
    require(budget.commit(std::move(*boundary.value()), {0.75}, after_evidence(owner, 0.75),
                          12, ledger, {bounded::precision_ledger_entry_id(1)}).has_value(),
            "tolerance budget commits exact boundary");
    require(!budget.reserve(proposal(budget, owner, 3, std::numeric_limits<double>::epsilon())).has_value(),
            "tolerance budget rejects cost beyond boundary");

    auto invalid = proposal(budget, owner, 4, 0.0);
    invalid.authorization.authentication[0] ^= 1;
    require(!budget.reserve(invalid).has_value(), "tolerance budget rejects invalid certificate");
    auto wrong_owner = budget.reserve(proposal(budget, owner, 5, 0.0, 44));
    require(wrong_owner.has_value() &&
            !budget.commit(std::move(*wrong_owner.value()), {0.0},
                           after_evidence(owner, 0.0, 44), 45, ledger,
                           {bounded::precision_ledger_entry_id(1)}).has_value(),
            "tolerance budget rejects wrong transaction owner");

    bounded::tolerance_budget under_report_budget(owner, 1.0);
    const auto rollback_before = under_report_budget.snapshot()->commits.size();
    auto under_report = under_report_budget.reserve(
        proposal(under_report_budget, owner, 6, 0.25));
    require(under_report.has_value(), "tolerance budget under-report reservation");
    require(!under_report_budget.commit(std::move(*under_report.value()), {0.0},
                           after_evidence(owner, 0.25), 12, ledger,
                           {bounded::precision_ledger_entry_id(0)}).has_value() &&
            under_report_budget.snapshot()->commits.size() == rollback_before,
            "tolerance budget independently rejects zero under-report");
}

void test_conservative_bounds() {
    auto first = bounded::point_bound(point(0.0, 0.0, 0.0), bounded::finite_bound_id(1));
    auto touching = bounded::point_bound(point(0.0, 0.0, 0.0), bounded::finite_bound_id(2));
    auto separated = bounded::point_bound(point(2.0, 0.0, 0.0), bounded::finite_bound_id(3));
    require(bounded::bounds_overlap_closed(first, touching) &&
            !bounded::bounds_definitely_separated(first, touching), "conservative bounds closed contact");
    require(bounded::bounds_definitely_separated(first, separated), "conservative bounds separation");
    auto distance = bounded::squared_distance_lower_bound(first, separated);
    require(distance.has_value() && *distance.value() == 4.0, "conservative bounds distance lower bound");
    auto inflated = bounded::inflate_bound(first, 1.0, bounded::finite_bound_id(4));
    require(inflated.has_value() && inflated.value()->axes[0].contains(-1.0) &&
            inflated.value()->axes[0].contains(1.0) &&
            inflated.value()->inflation.current_cleanup == 1.0, "conservative bounds inflation");
    require(!bounded::inflate_bound(first, -1.0).has_value(), "conservative bounds reject negative inflation");
    auto contact_after_inflation = bounded::inflate_bound(separated, 1.0);
    require(contact_after_inflation.has_value() &&
            bounded::bounds_overlap_closed(*inflated.value(), *contact_after_inflation.value()),
            "conservative bounds retain inflated contact");
}

void test_precision_import_and_affine() {
    bounded::foreign_precision_provenance prior;
    prior.owner = context_owner;
    prior.prior_output_precision = 0.6;
    prior.inherited_precision = 0.1;
    prior.construction_uncertainty = 0.2;
    prior.cumulative_cleanup_displacement = 0.1;
    prior.serialization_contribution = 0.1;
    prior.construction_history_digest = {1};
    prior.prior_context_digest = {2};
    prior.publication_digest = {3};
    prior.verified_publication_digest = prior.publication_digest;
    prior.verification_evidence = {4};
    prior.publication_verified = true;
    auto imported = bounded::verify_precision_import(prior, 1.0);
    require(imported.has_value() && imported.value()->inherited_a == 0.1 &&
            imported.value()->inherited_b > 0.0 && imported.value()->inherited_b <= 0.1 &&
            imported.value()->construction == 0.2 && imported.value()->prior_cleanup == 0.1 &&
            imported.value()->conversion == 0.1, "prior precision import");
    prior.publication_verified = false;
    require(!bounded::verify_precision_import(prior, 1.0).has_value(),
            "prior precision import rejects unverified publication");

    bounded::bounded_affine3<double> transform;
    transform.owner = context_owner;
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column)
            transform.linear[row][column] = scalar(row == column ? 2.0 : 0.0);
        transform.translation[row] = scalar(static_cast<double>(row + 1));
    }
    auto transformed = bounded::apply_affine(transform, point(1.0, 2.0, 3.0, 5, 6));
    require(transformed.has_value() && transformed.value()->coordinates.components[0].rounded_nominal == 3.0 &&
            transformed.value()->coordinates.components[1].rounded_nominal == 6.0 &&
            transformed.value()->coordinates.components[2].rounded_nominal == 9.0 &&
            transformed.value()->lineage == bounded::geometric_lineage_id(6), "bounded affine transform");
    const std::array<bounded::bounded_scalar<double>, 3> translation{{
        scalar(-1.0), scalar(2.0), scalar(3.0)}};
    auto translated = bounded::translate_point(point(1.0, 2.0, 3.0), translation);
    require(translated.has_value() && translated.value()->coordinates.components[0].rounded_nominal == 0.0 &&
            translated.value()->coordinates.components[2].rounded_nominal == 6.0,
            "bounded point translation");
}

template<class T, class I>
void test_bootstrap_codec_and_frozen_context() {
    fv_surface_mesh<T, I> a;
    fv_surface_mesh<T, I> b;
    a.vertices.emplace_back(T(-0.0), std::numeric_limits<T>::denorm_min(), T(1));
    b.vertices.emplace_back(T(2), T(3), T(4));
    bounded_boolean_options<T> options;
    options.tolerance = T(1);
    auto pending = bounded::build_pending_invocation(a, b, boolean_operation::intersection, options);
    require(pending.has_value(), "precision bootstrap pending invocation");
    auto preflight = bounded::preflight_precision(*pending.value());
    require(preflight.has_value(), "precision bootstrap finite source");
    require(preflight.value()->operand_a.has_negative_zero,
            "precision bootstrap signed zero flag");
    require(preflight.value()->operand_a.has_subnormal,
            "precision bootstrap subnormal flag");
    require(preflight.value()->global.has_normal,
            "precision bootstrap normal flag");
    require(preflight.value()->ordinary_success_eligible,
            "precision bootstrap tolerance eligibility");
    require(bounded::verify_precision_preflight(*preflight.value(), *pending.value()),
            "independent precision preflight verifier");

    const auto encoded_preflight = bounded::encode_precision_preflight(*preflight.value());
    auto decoded_preflight = bounded::decode_precision_preflight<T>(encoded_preflight);
    require(decoded_preflight.has_value() && decoded_preflight.value()->digest == preflight.value()->digest,
            "precision preflight codec roundtrip");
    auto mutated_preflight = encoded_preflight;
    mutated_preflight[0] ^= 1;
    require(!bounded::decode_precision_preflight<T>(mutated_preflight).has_value(),
            "precision preflight codec rejects mutation");

    const auto bootstrap = bounded::make_precision_bootstrap_record(*preflight.value());
    auto context = bounded::finalize_context(std::move(*pending.value()), bootstrap);
    require(context.has_value(), "boolean context precision handshake");
    bounded::precision_runtime_capabilities runtime;
    runtime.expected_owner = &context.value()->owner;
    auto frozen = bounded::build_precision_context(*preflight.value(), *context.value(), runtime);
    require(frozen.has_value() && bounded::verify_precision_context(
                **frozen.value(), *preflight.value(), *context.value(), runtime),
            "frozen precision context independent verifier");
    bounded::precision_context_view<T> view(**frozen.value(), context.value()->owner);
    require(view.valid() && view.tolerance() == T(1) && view.digest() != nullptr,
            "frozen precision context owner view");
    auto imported = bounded::import_source_bounded_values(
        **frozen.value(), context.value()->sources->a);
    require(imported.has_value() && imported.value()->points.size() == 1 &&
            imported.value()->trace.nodes().size() == 3 &&
            imported.value()->points[0].value.coordinates.radial_error_upper ==
                (*frozen.value())->effective_input_precision_a() &&
            imported.value()->points[0].value.coordinates.components[0].identity.provenance ==
                imported.value()->points[0].value.provenance,
            "frozen source import publishes stable identities, enclosures, and local traces");

    const auto encoded_context = bounded::encode_precision_context(**frozen.value());
    auto decoded_context = bounded::decode_precision_context<T>(encoded_context);
    require(decoded_context.has_value() && decoded_context.value()->digest == (*frozen.value())->digest(),
            "precision context codec roundtrip");
    auto mutated_context = encoded_context;
    mutated_context.pop_back();
    require(!bounded::decode_precision_context<T>(mutated_context).has_value(),
            "precision context codec rejects mutation");

    const auto wrong_owner = bounded::context_owner_token::create();
    bounded::precision_runtime_capabilities wrong_runtime;
    wrong_runtime.expected_owner = &wrong_owner;
    require(!bounded::verify_precision_context(**frozen.value(), *preflight.value(),
                                               *context.value(), wrong_runtime) &&
            !bounded::verify_precision_context_owner(**frozen.value(), wrong_owner),
            "frozen precision context rejects wrong owner");
    bounded::precision_context_view<T> wrong_view(**frozen.value(), wrong_owner);
    require(!wrong_view.valid(), "precision context wrong-owner view invalid");

    fv_surface_mesh<T, I> nonfinite;
    nonfinite.vertices.emplace_back(std::numeric_limits<T>::infinity(), T(0), T(0));
    auto bad_pending = bounded::build_pending_invocation(nonfinite, b, boolean_operation::set_union, options);
    require(bad_pending.has_value(), "nonfinite source reaches precision preflight");
    auto rejected = bounded::preflight_precision(*bad_pending.value());
    require(!rejected.has_value() && rejected.error()->subcode == 30005,
            "precision bootstrap rejects nonfinite coordinate");
}

} // namespace

int main() {
    try {
        require(std::fesetenv(FE_DFL_ENV) == 0 && std::fesetround(FE_TONEAREST) == 0,
                "establish qualified floating environment");
        test_floating_bits<float>();
        test_floating_bits<double>();
        test_directed_rounding<float>();
        test_directed_rounding<double>();
        test_intervals<float>();
        test_intervals<double>();
        test_exact_geometry<float>();
        test_exact_geometry<double>();
        test_truth_layers_and_bounded_geometry();
        test_conditioning();
        test_trace_finalization();
        test_precision_ledger();
        test_tolerance_budget();
        test_conservative_bounds();
        test_precision_import_and_affine();
        test_bootstrap_codec_and_frozen_context<float, std::uint32_t>();
        test_bootstrap_codec_and_frozen_context<double, std::uint64_t>();
        std::cout << "Component 03 precision tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
