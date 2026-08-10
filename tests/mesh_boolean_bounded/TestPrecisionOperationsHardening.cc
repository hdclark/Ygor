#include "YgorMeshesBooleanBounded/PrecisionCapabilities.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace bounded = ygor::mesh_boolean::bounded;

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

bounded::bounded_scalar<double> singleton(const bounded::context_owner_token &owner, double value) {
    auto result = bounded::checked_bounded_singleton(owner, value);
    require(result.has_value(), "checked singleton setup");
    return std::move(*result.value());
}

bounded::bounded_residual<double> passing_residual(const bounded::context_owner_token &owner,
                                                   double value) {
    bounded::bounded_residual<double> out;
    out.owner = owner;
    out.value = singleton(owner, value);
    out.scale = 1.0;
    out.comparison_boundary = 1.0;
    out.disposition = bounded::residual_disposition::pass;
    return out;
}

bounded::exact_relation_evidence relation(const bounded::context_owner_token &owner,
                                          bounded::exact_relation_status status) {
    bounded::exact_relation_evidence out;
    out.owner = owner;
    out.status = status;
    out.formula_code = 7;
    return out;
}

void test_arithmetic_invariants_and_owners() {
    const auto owner = bounded::context_owner_token::create();
    const auto other = bounded::context_owner_token::create();
    require(!bounded::checked_bounded_singleton(owner,
                std::numeric_limits<double>::infinity()).has_value(),
            "checked singleton rejects infinity");

    auto a = singleton(owner, 2.0);
    auto b = singleton(owner, 3.0);
    a.contributors.inherited_b = 0.125;
    b.contributors.machine_floor = 0.25;
    b.contributors.conversion = 0.5;
    auto product = bounded::bounded_multiply(a, b);
    require(product.has_value(), "multiply with contributors succeeds");
    require(product.value()->contributors.inherited_b >= 0.125 &&
            product.value()->contributors.machine_floor >= 0.25 &&
            product.value()->contributors.conversion >= 0.5,
            "multiply propagates every contributor");

    auto wrong_owner = singleton(other, 1.0);
    require(!bounded::bounded_add(a, wrong_owner).has_value(), "cross-owner arithmetic rejected");
    auto malformed = a;
    malformed.rounded_nominal = 9.0;
    require(!bounded::bounded_add(malformed, b).has_value(),
            "nominal outside enclosure rejected");

    bounded::bounded_vec3<double> left;
    bounded::bounded_vec3<double> right;
    left.owner = owner;
    right.owner = owner;
    for (unsigned axis = 0; axis < 3; ++axis) {
        left.components[axis] = singleton(owner, 1.0);
        right.components[axis] = singleton(owner, 1.0);
    }
    left.components[0].uncertainty_enclosure =
        *bounded::finite_interval<double>::create(0.0, 2.0);
    auto sum = bounded::bounded_vector_add(left, right);
    require(sum.has_value() && sum.value()->radial_error_upper >= 1.0,
            "vector operation computes conservative radial error");
}

void test_predicate_and_conditioning_fail_closed() {
    const auto owner = bounded::context_owner_token::create();
    auto positive = singleton(owner, 1.0);
    require(!bounded::assemble_predicate_result(
                positive, relation(owner, bounded::exact_relation_status::exact_negative)).has_value(),
            "predicate contradiction is typed failure");
    auto predicate = bounded::assemble_predicate_result(
        positive, relation(owner, bounded::exact_relation_status::exact_positive));
    require(predicate.has_value() &&
            predicate.value()->disposition == bounded::predicate_disposition::accept_numeric_sign,
            "consistent predicate succeeds");

    auto at_a = passing_residual(owner, 1.0);
    auto at_b = passing_residual(owner, -1.0);
    auto carrier = passing_residual(owner, 0.0);
    auto support = passing_residual(owner, 0.0);
    auto conditioned = bounded::condition_edge_plane(
        at_a, at_b,
        relation(owner, bounded::exact_relation_status::exact_positive),
        relation(owner, bounded::exact_relation_status::exact_positive),
        relation(owner, bounded::exact_relation_status::exact_positive),
        carrier, support, 0.1, 1.0, bounded::construction_id(3));
    require(conditioned.has_value() &&
            conditioned.value()->denominator_relation.status ==
                bounded::exact_relation_status::exact_positive &&
            conditioned.value()->parameter.domain == bounded::parameter_domain_status::stable_interior,
            "conditioning publishes complete correctly attributed evidence");

    support.disposition = bounded::residual_disposition::invalid;
    require(!bounded::condition_edge_plane(
                at_a, at_b,
                relation(owner, bounded::exact_relation_status::exact_positive),
                relation(owner, bounded::exact_relation_status::exact_positive),
                relation(owner, bounded::exact_relation_status::exact_positive),
                carrier, support, 0.1, 1.0).has_value(),
            "conditioning rejects incomplete residual evidence");
}

void test_import_and_live_capabilities() {
    const auto owner = bounded::context_owner_token::create();
    bounded::foreign_precision_provenance prior;
    prior.owner = owner;
    prior.prior_output_precision = 0.6;
    prior.inherited_precision = 0.1;
    prior.construction_uncertainty = 0.2;
    prior.cumulative_cleanup_displacement = 0.1;
    prior.serialization_contribution = 0.1;
    prior.construction_history_digest = {1};
    prior.prior_context_digest = {2};
    prior.publication_digest = {3};
    prior.verified_publication_digest = {3};
    prior.verification_evidence = {4};
    prior.publication_verified = true;
    auto imported = bounded::verify_precision_import(prior, 1.0);
    require(imported.has_value() && imported.value()->inherited_b >= 0.09,
            "import carries unexplained prior precision");
    prior.verified_publication_digest = {9};
    require(!bounded::verify_precision_import(prior, 1.0).has_value(),
            "bare publication flag cannot verify import");

    bounded::precision_ledger ledger(owner, 1.0);
    bounded::tolerance_budget budget(owner, 1.0);
    auto capabilities = bounded::make_precision_capabilities<double>(owner, ledger, budget);
    require(capabilities.has_value() && capabilities.value()->read.valid_for(owner),
            "capabilities validate ledger and budget owners");
    require(capabilities.value()->read.output_precision() == 0.0, "initial live precision");
    bounded::precision_ledger_record record;
    record.owner = owner;
    record.id = bounded::precision_ledger_entry_id(0);
    record.result = bounded::bounded_value_id(1);
    record.lineage = bounded::geometric_lineage_id(1);
    record.no_motion_uncertainty = 0.25;
    record.contributors.machine_floor = 0.25;
    require(ledger.append(record).has_value() &&
            capabilities.value()->read.output_precision() >= 0.25,
            "read capability does not retain stale snapshots");
    auto ownerless = record;
    ownerless.id = bounded::precision_ledger_entry_id(1);
    ownerless.result = bounded::bounded_value_id(2);
    ownerless.owner = {};
    require(!ledger.append(ownerless).has_value(), "ledger rejects ownerless publication");

    bounded::precision_ledger wrong_ledger(bounded::context_owner_token::create(), 1.0);
    require(!bounded::make_precision_capabilities<double>(owner, wrong_ledger, budget).has_value(),
            "capability factory rejects wrong ledger owner");
}

void test_traced_publication_and_squared_norm() {
    const auto owner = bounded::context_owner_token::create();
    bounded::local_precision_trace trace(owner);
    bounded::precision_trace_key source_a;
    source_a.operation_code = static_cast<std::uint16_t>(bounded::rounded_operation_code::source_import);
    source_a.result_bytes = {1};
    bounded::precision_trace_key source_b = source_a;
    source_b.result_bytes = {2};
    const auto a_root = trace.append(source_a);
    const auto b_root = trace.append(source_b);
    auto a = singleton(owner, 2.0);
    auto b = singleton(owner, 3.0);
    a.identity.trace_root = a_root.ordinal;
    b.identity.trace_root = b_root.ordinal;
    a.identity.provenance = bounded::provenance_id(4);
    b.identity.provenance = bounded::provenance_id(4);
    a.identity.lineage = bounded::geometric_lineage_id(7);
    b.identity.lineage = bounded::geometric_lineage_id(7);
    auto product = bounded::bounded_multiply(trace, a, b);
    require(product.has_value(), "traced multiply succeeds");
    bounded::precision_ledger ledger(owner, 10.0);
    auto published = bounded::finalize_and_publish_bounded_scalar(
        ledger, trace, bounded::local_trace_node_id{product.value()->identity.trace_root},
        bounded::precision_trace_id(3), bounded::bounded_value_id(9),
        bounded::provenance_id(4), bounded::geometric_lineage_id(7), *product.value());
    require(published.has_value() &&
            published.value()->value->identity.publication == bounded::bounded_publication_state::committed &&
            published.value()->value->identity.ledger_entry == bounded::precision_ledger_entry_id(0) &&
            ledger.snapshot()->records.size() == 1,
            "trace finalization and ledger publication are connected");

    bounded::bounded_vec3<double> uncertain;
    uncertain.owner = owner;
    for (auto &component : uncertain.components) {
        component = singleton(owner, 0.0);
        component.uncertainty_enclosure = *bounded::finite_interval<double>::create(-1.0, 1.0);
    }
    auto norm = bounded::bounded_squared_norm(uncertain);
    require(norm.has_value() && norm.value()->uncertainty_enclosure.lower() == 0.0,
            "squared norm uses verified opaque nonnegative evidence");
}

} // namespace

int main() {
    try {
        test_arithmetic_invariants_and_owners();
        test_predicate_and_conditioning_fail_closed();
        test_import_and_live_capabilities();
        test_traced_publication_and_squared_norm();
        std::cout << "Component 03 precision hardening tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
