#include "YgorMeshesBooleanBounded/PrecisionVerifier.h"

#include <iostream>
#include <stdexcept>

namespace bounded = ygor::mesh_boolean::bounded;

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

bounded::precision_trace_snapshot make_trace(bool reverse) {
    bounded::local_precision_trace trace(bounded::context_owner_token::create());
    bounded::precision_trace_key a; a.operation_code = 1; a.result_bytes = {1};
    bounded::precision_trace_key b; b.operation_code = 1; b.result_bytes = {2};
    bounded::local_trace_node_id aid{0}, bid{0};
    if (reverse) { bid = trace.append(b); aid = trace.append(a); }
    else { aid = trace.append(a); bid = trace.append(b); }
    bounded::precision_trace_key sum; sum.operation_code = 4;
    sum.parents = {aid.ordinal, bid.ordinal}; sum.result_bytes = {3};
    auto result = bounded::finalize_precision_trace(trace, trace.append(sum), bounded::precision_trace_id(4));
    require(result.has_value(), "trace finalization");
    return std::move(*result.value());
}

bounded::precision_ledger_record ledger_record(const bounded::context_owner_token &owner,
                                                std::uint64_t id, double uncertainty, double movement,
                                                 std::uint64_t lineage = 7) {
    bounded::precision_ledger_record out;
    out.owner = owner;
    out.id = bounded::precision_ledger_entry_id(id);
    out.result = bounded::bounded_value_id(id + 10);
    out.lineage = bounded::geometric_lineage_id(lineage);
    out.contributors.machine_floor = uncertainty;
    out.contributors.current_cleanup = movement;
    out.no_motion_uncertainty = uncertainty;
    out.cumulative_displacement = movement;
    return out;
}

bounded::tolerance_budget_proposal proposal(bounded::tolerance_budget &budget,
                                             const bounded::context_owner_token &owner,
                                             std::uint64_t id, double cost) {
    bounded::tolerance_budget_proposal out;
    out.id = bounded::budget_proposal_id(id);
    out.ordered_lineages = {bounded::geometric_lineage_id(3)};
    out.requested_costs = {cost};
    out.certificate.id = bounded::displacement_certificate_id(id);
    out.certificate.upper_length = cost;
    out.certificate.evidence = {1, 2};
    out.owner = owner;
    out.transaction_owner = 9;
    out.before_evidence.owner = owner;
    out.before_evidence.transaction_owner = 9;
    out.before_evidence.cumulative_lengths = {0.0};
    out.before_evidence.evidence = {2};
    auto authorization = budget.authorize_topology(out.operation, 9, {3});
    require(authorization.has_value(), "budget topology authorization");
    out.authorization = std::move(*authorization.value());
    return out;
}

bounded::budget_length_evidence after(const bounded::context_owner_token &owner, double length) {
    bounded::budget_length_evidence out;
    out.owner = owner;
    out.transaction_owner = 9;
    out.cumulative_lengths = {length};
    out.evidence = {4};
    return out;
}

bounded::bounded_point3<double> point(double x, std::uint64_t provenance, std::uint64_t lineage) {
    bounded::bounded_point3<double> out;
    const auto owner = bounded::context_owner_token::create();
    out.provenance = bounded::provenance_id(provenance);
    out.lineage = bounded::geometric_lineage_id(lineage);
    for (unsigned axis = 0; axis < 3; ++axis) {
        auto scalar = bounded::bounded_singleton(owner, axis == 0 ? x : 0.0);
        require(scalar.has_value(), "bounded point scalar");
        out.coordinates.components[axis] = std::move(*scalar.value());
    }
    out.coordinates.components[0].contributors.machine_floor = 0.1;
    return out;
}

void test_trace() {
    const auto forward = make_trace(false);
    const auto reverse = make_trace(true);
    require(bounded::encode_precision_trace(forward) == bounded::encode_precision_trace(reverse),
            "trace must use recursive canonical parent keys");
    require(bounded::verify_precision_trace(forward), "trace verifier");
    auto decoded = bounded::decode_precision_trace(bounded::encode_precision_trace(forward));
    require(decoded.has_value() && bounded::verify_precision_trace(*decoded.value()) &&
            bounded::encode_precision_trace(*decoded.value()) == bounded::encode_precision_trace(forward),
            "trace canonical roundtrip");
    auto parent_mutation = forward;
    parent_mutation.nodes.back().key.parents[0] = parent_mutation.nodes.back().id.ordinal();
    require(!bounded::verify_precision_trace(parent_mutation), "trace parent mutation");
    auto bytes = bounded::encode_precision_trace(forward);
    bytes[bytes.size() - 1] ^= 1;
    require(!bounded::decode_precision_trace(bytes).has_value(), "trace digest mutation");
}

void test_ledger() {
    const auto owner = bounded::context_owner_token::create();
    bounded::precision_ledger ledger(owner, 1.0);
    auto wrong = ledger_record(owner, 0, 0.1, 0.1);
    wrong.owner = bounded::context_owner_token::create();
    require(!ledger.append(wrong).has_value() && ledger.snapshot()->records.empty(),
            "ledger owner rejection is atomic");
    require(ledger.append(ledger_record(owner, 0, 0.1, 0.2)).has_value(), "ledger append");
    auto second = ledger_record(owner, 1, 0.2, 0.1);
    second.ordered_parents.push_back(bounded::precision_ledger_entry_id(0));
    require(ledger.append(second).has_value(), "ledger parent append");
    auto snapshot = *ledger.snapshot();
    require(bounded::verify_precision_ledger(snapshot, 1.0, &owner), "ledger reconstruction");
    auto total_mutation = snapshot; total_mutation.lineages[0].precision -= 0.01;
    require(!bounded::verify_precision_ledger(total_mutation, 1.0), "ledger total mutation");
    auto digest_mutation = snapshot; digest_mutation.records[0].canonical_digest_contribution[0] ^= 1;
    require(!bounded::verify_precision_ledger(digest_mutation, 1.0), "ledger digest mutation");
    auto bytes = bounded::encode_precision_ledger(snapshot);
    auto decoded = bounded::decode_precision_ledger(bytes);
    require(decoded.has_value() && bounded::verify_precision_ledger(*decoded.value(), 1.0) &&
            bounded::encode_precision_ledger(*decoded.value()) == bytes, "ledger canonical roundtrip");
    bytes[20] ^= 1;
    require(!bounded::decode_precision_ledger(bytes).has_value(), "ledger codec mutation");
}

void test_budget() {
    const auto owner = bounded::context_owner_token::create();
    bounded::tolerance_budget budget(owner, 1.0);
    bounded::precision_ledger ledger(owner, 1.0);
    auto first = budget.reserve(proposal(budget, owner, 1, 0.7));
    require(first.has_value(), "first reservation");
    require(!budget.reserve(proposal(budget, owner, 2, 0.4)).has_value(), "active reservation overbooking");
    require(ledger.append(ledger_record(owner, 0, 0.0, 0.6, 3)).has_value(), "budget ledger update");
    require(budget.commit(std::move(*first.value()), {0.6}, after(owner, 0.6), 9, ledger,
                          {bounded::precision_ledger_entry_id(0)}).has_value(), "budget commit");
    auto snapshot = *budget.snapshot();
    require(bounded::verify_tolerance_budget(snapshot, *ledger.snapshot(), &owner), "budget reconstruction");
    auto decoded = bounded::decode_tolerance_budget(bounded::encode_tolerance_budget(snapshot));
    require(decoded.has_value() && bounded::verify_tolerance_budget(*decoded.value()),
            "budget canonical roundtrip");
    auto cost_mutation = snapshot; cost_mutation.commits[0].actual_costs[0] = 0.5;
    require(!bounded::verify_tolerance_budget(cost_mutation), "budget cost mutation");
    auto invalid = proposal(budget, owner, 3, 0.1);
    invalid.certificate.units = static_cast<bounded::length_unit_kind>(9);
    require(!budget.reserve(invalid).has_value(), "budget unit validation");

    bounded::tolerance_reservation orphan;
    {
        const auto temporary_owner = bounded::context_owner_token::create();
        bounded::tolerance_budget temporary(temporary_owner, 1.0);
        auto reservation = temporary.reserve(proposal(temporary, temporary_owner, 8, 0.1));
        require(reservation.has_value(), "temporary reservation");
        orphan = std::move(*reservation.value());
    }
    require(orphan.active(), "reservation owns shared rollback state");
    orphan = bounded::tolerance_reservation{};

    resource_policy policy = resource_policy::conservative_defaults();
    policy.budget_proposals = {1, 1};
    policy.budget_reservations = {1, 1};
    policy.budget_commits = {1, 1};
    bounded::resource_manager resources(policy);
    bool cancelled = false;
    bounded::resource_cancellation_checkpoint checkpoint{
        &cancelled, [](const void *state) noexcept {
            return *static_cast<const bool *>(state);
        }};
    const auto resource_owner = bounded::context_owner_token::create();
    bounded::tolerance_budget resource_budget(resource_owner, 1.0, &resources, checkpoint);
    auto cancelled_proposal = proposal(resource_budget, resource_owner, 9, 0.25);
    cancelled = true;
    require(!resource_budget.reserve(cancelled_proposal).has_value() &&
            resource_budget.snapshot()->commits.empty() &&
            resource_budget.snapshot()->active_reservations == 0,
            "cancelled budget checkpoint has zero publication");
    const auto cancelled_resources = resources.snapshot();
    require(cancelled_resources[static_cast<std::size_t>(bounded::resource_kind::budget_proposals)].reserved == 0 &&
            cancelled_resources[static_cast<std::size_t>(bounded::resource_kind::budget_reservations)].reserved == 0,
            "cancelled budget checkpoint releases resources");

    cancelled = false;
    {
        auto rollback_reservation = resource_budget.reserve(
            proposal(resource_budget, resource_owner, 10, 1.0));
        require(rollback_reservation.has_value(), "budget resource exact hard boundary");
    }
    const auto rolled_back_resources = resources.snapshot();
    require(resource_budget.snapshot()->active_reservations == 0 &&
            rolled_back_resources[static_cast<std::size_t>(bounded::resource_kind::budget_proposals)].reserved == 0 &&
            rolled_back_resources[static_cast<std::size_t>(bounded::resource_kind::budget_reservations)].reserved == 0,
            "budget reservation rollback has zero publication");
}

void test_bounds_and_import() {
    const auto a = bounded::point_bound_record(point(0.0, 4, 5), bounded::finite_bound_id(1));
    const auto b = bounded::point_bound_record(point(2.0, 6, 7), bounded::finite_bound_id(2));
    auto joined = bounded::union_bound_records(a, b, bounded::finite_bound_id(3));
    require(joined.has_value() && joined.value()->ordered_provenance.size() == 2 &&
            joined.value()->ordered_lineages.size() == 2 &&
            bounded::verify_union_bound(*joined.value(), a, b), "bound contributor lineage preservation");
    auto narrowed = *joined.value();
    narrowed.bound.axes[0] = *bounded::finite_interval<double>::create(0.5, 2.0);
    require(!bounded::verify_union_bound(narrowed, a, b), "AABB narrowing mutation");
    require(!bounded::intersect_bounds(a.bound, b.bound, bounded::finite_bound_id(4)).has_value(),
            "intersection cannot fabricate proof");
    auto bytes = bounded::encode_conservative_bound(*joined.value());
    auto decoded_bound = bounded::decode_conservative_bound<double>(bytes);
    require(decoded_bound.has_value() && bounded::verify_union_bound(*decoded_bound.value(), a, b) &&
            bounded::encode_conservative_bound(*decoded_bound.value()) == bytes,
            "bound canonical roundtrip");

    bounded::foreign_precision_provenance imported;
    imported.prior_output_precision = 0.5;
    imported.inherited_precision = 0.1;
    imported.construction_uncertainty = 0.1;
    imported.cumulative_cleanup_displacement = 0.1;
    imported.serialization_contribution = 0.1;
    imported.construction_history_digest = {1};
    imported.prior_context_digest = {2};
    imported.publication_verified = true;
    require(bounded::verify_precision_import_record(imported, 1.0), "import reconstruction");
    auto import_bytes = bounded::encode_precision_import(imported);
    auto decoded_import = bounded::decode_precision_import(import_bytes);
    require(decoded_import.has_value() && bounded::verify_precision_import_record(*decoded_import.value(), 1.0) &&
            bounded::encode_precision_import(*decoded_import.value()) == import_bytes,
            "import canonical roundtrip");
    import_bytes[import_bytes.size() - 2] ^= 1;
    require(!bounded::decode_precision_import(import_bytes).has_value(), "import digest mutation");
}
}

int main() {
    try {
        test_trace(); test_ledger(); test_budget(); test_bounds_and_import();
        std::cout << "Component 03 precision state hardening tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
