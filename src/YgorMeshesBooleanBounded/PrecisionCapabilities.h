#pragma once

#include "ConstructionConditioning.h"
#include "ConservativeBounds.h"
#include "PrecisionImport.h"
#include "PrecisionContext.h"
#include "ToleranceBudget.h"

#include <memory>

namespace ygor::mesh_boolean::bounded {

struct precision_read_capability final {
    context_owner_token owner{};
    const precision_ledger *ledger = nullptr;
    const tolerance_budget *budget = nullptr;
    bool valid_for(const context_owner_token &expected) const noexcept {
        if (!bounded_operations_detail::owner_bound(expected) || !owner.same_owner(expected) ||
            !ledger || !budget || !ledger->owner().same_owner(expected)) return false;
        const auto current_budget = budget->snapshot();
        return current_budget && current_budget->owner.same_owner(expected);
    }
    std::shared_ptr<const precision_ledger_snapshot> ledger_snapshot() const {
        return ledger ? ledger->snapshot() : nullptr;
    }
    std::shared_ptr<const tolerance_budget_snapshot> budget_snapshot() const {
        return budget ? budget->snapshot() : nullptr;
    }
    double output_precision() const noexcept {
        const auto current = ledger ? ledger->snapshot() : nullptr;
        return current && current->owner.same_owner(owner) ? current->global_output_precision : 0.0;
    }
};

template<class T>
struct bounded_arithmetic_capability final {
    context_owner_token owner{};
    boolean_outcome<bounded_scalar<T>> singleton(T value) const {
        return checked_bounded_singleton(owner, value);
    }
    boolean_outcome<bounded_scalar<T>> add(const bounded_scalar<T> &a,
                                           const bounded_scalar<T> &b) const {
        if (!a.identity.owner.same_owner(owner) || !b.identity.owner.same_owner(owner))
            return boolean_outcome<bounded_scalar<T>>::failure(
                bounded_operations_detail::arithmetic_error(31401));
        return bounded_add(a, b);
    }
    boolean_outcome<bounded_scalar<T>> subtract(const bounded_scalar<T> &a,
                                                const bounded_scalar<T> &b) const {
        if (!a.identity.owner.same_owner(owner) || !b.identity.owner.same_owner(owner))
            return boolean_outcome<bounded_scalar<T>>::failure(
                bounded_operations_detail::arithmetic_error(31401));
        return bounded_subtract(a, b);
    }
    boolean_outcome<bounded_scalar<T>> multiply(const bounded_scalar<T> &a,
                                                const bounded_scalar<T> &b) const {
        if (!a.identity.owner.same_owner(owner) || !b.identity.owner.same_owner(owner))
            return boolean_outcome<bounded_scalar<T>>::failure(
                bounded_operations_detail::arithmetic_error(31401));
        return bounded_multiply(a, b);
    }
    boolean_outcome<bounded_scalar<T>> divide(const bounded_scalar<T> &a,
                                              const bounded_scalar<T> &b) const {
        if (!a.identity.owner.same_owner(owner) || !b.identity.owner.same_owner(owner))
            return boolean_outcome<bounded_scalar<T>>::failure(
                bounded_operations_detail::arithmetic_error(31401));
        return bounded_divide(a, b);
    }
    boolean_outcome<predicate_result<T>> predicate(
        bounded_scalar<T> value, exact_relation_evidence exact,
        bool alternate_available = false) const {
        if (!value.identity.owner.same_owner(owner) || !exact.owner.same_owner(owner))
            return boolean_outcome<predicate_result<T>>::failure(
                bounded_operations_detail::arithmetic_error(31402));
        return assemble_predicate_result(std::move(value), std::move(exact), alternate_available);
    }
};

template<class T>
struct conservative_bounds_capability final {
    context_owner_token owner{};
    bool overlap(const bounded_aabb3<T> &a, const bounded_aabb3<T> &b) const noexcept {
        return a.owner.same_owner(owner) && b.owner.same_owner(owner) && bounds_overlap_closed(a, b);
    }
    boolean_outcome<T> distance_squared_lower(const bounded_aabb3<T> &a,
                                               const bounded_aabb3<T> &b) const {
        if (!a.owner.same_owner(owner) || !b.owner.same_owner(owner))
            return boolean_outcome<T>::failure(
                bounded_operations_detail::arithmetic_error(31403));
        return squared_distance_lower_bound(a, b);
    }
};

struct precision_mutation_capability final {
    context_owner_token owner{};
    precision_ledger *ledger = nullptr;
    tolerance_budget *budget = nullptr;
    bool valid_for(const context_owner_token &expected) const noexcept {
        if (!bounded_operations_detail::owner_bound(expected) || !owner.same_owner(expected) ||
            !ledger || !budget || !ledger->owner().same_owner(expected)) return false;
        const auto current_budget = budget->snapshot();
        return current_budget && current_budget->owner.same_owner(expected);
    }
    boolean_outcome<topology_authorization> authorize_topology(
        cleanup_operation_kind operation, std::uint64_t transaction_owner,
        std::vector<std::uint8_t> evidence) const {
        if (!valid_for(owner))
            return boolean_outcome<topology_authorization>::failure(
                bounded_operations_detail::arithmetic_error(31405));
        return budget->authorize_topology(operation, transaction_owner, std::move(evidence));
    }
};

template<class T>
struct component_03_capabilities final {
    bounded_arithmetic_capability<T> arithmetic;
    conservative_bounds_capability<T> bounds;
    precision_read_capability read;
    precision_mutation_capability mutation;
};

template<class T>
struct component_02_precision_capabilities final {
    context_owner_token owner{};
    bounded_arithmetic_capability<T> arithmetic{};
    conservative_bounds_capability<T> bounds{};
    template<class I>
    boolean_outcome<source_bounded_value_batch<T>> import_source(
        const precision_context<T> &context, const immutable_source_mesh<T,I> &source) const {
        if (!context.owned_by(owner))
            return boolean_outcome<source_bounded_value_batch<T>>::failure(
                bounded_operations_detail::arithmetic_error(31405));
        return import_source_bounded_values(context, source);
    }
    boolean_outcome<bounded_plane3<T>> plane(const bounded_point3<T> &a,
                                              const bounded_point3<T> &b,
                                              const bounded_point3<T> &c) const {
        if (!a.owner.same_owner(owner) || !b.owner.same_owner(owner) || !c.owner.same_owner(owner))
            return boolean_outcome<bounded_plane3<T>>::failure(
                bounded_operations_detail::arithmetic_error(31405));
        return bounded_plane_from_points(a, b, c);
    }
};

template<class T>
struct component_04_precision_capabilities final {
    bounded_arithmetic_capability<T> arithmetic{};
    conservative_bounds_capability<T> bounds{};
};

template<class T>
using component_06_precision_capabilities = conservative_bounds_capability<T>;

template<class T>
using component_07_precision_capabilities = bounded_arithmetic_capability<T>;

struct component_13_precision_capabilities final {
    precision_read_capability read{};
    precision_mutation_capability mutation{};
    bool valid_for(const context_owner_token &owner) const noexcept {
        return read.valid_for(owner) && mutation.valid_for(owner);
    }
};

using component_15_precision_capabilities = precision_read_capability;

template<class T>
using component_16_precision_capabilities = bounded_arithmetic_capability<T>;

template<class T>
struct component_17_precision_capabilities final {
    context_owner_token owner{};
    template<class U>
    boolean_outcome<committed_bounded_scalar<U>> finalize(
        precision_ledger &ledger, const local_precision_trace &trace,
        local_trace_node_id root, precision_trace_id trace_id,
        bounded_value_id value, provenance_id provenance,
        geometric_lineage_id lineage, const bounded_scalar<U> &local,
        std::vector<precision_ledger_entry_id> parents = {}) const {
        if (!owner.anchor || !ledger.owner().same_owner(owner) || !trace.owner().same_owner(owner))
            return boolean_outcome<committed_bounded_scalar<U>>::failure(
                bounded_operations_detail::arithmetic_error(31406));
        return finalize_and_publish_bounded_scalar(
            ledger, trace, root, trace_id, value, provenance, lineage, local,
            std::move(parents));
    }
};

template<class T>
struct named_precision_consumer_capabilities final {
    component_02_precision_capabilities<T> component_02;
    component_04_precision_capabilities<T> component_04;
    component_06_precision_capabilities<T> component_06;
    component_07_precision_capabilities<T> component_07;
    component_13_precision_capabilities component_13;
    component_15_precision_capabilities component_15;
    component_16_precision_capabilities<T> component_16;
    component_17_precision_capabilities<T> component_17;
};

template<class T>
boolean_outcome<component_03_capabilities<T>> make_precision_capabilities(
    const context_owner_token &owner, precision_ledger &ledger, tolerance_budget &budget) {
    const auto budget_snapshot = budget.snapshot();
    if (!bounded_operations_detail::owner_bound(owner) || !ledger.owner().same_owner(owner) ||
        !budget_snapshot || !budget_snapshot->owner.same_owner(owner))
        return boolean_outcome<component_03_capabilities<T>>::failure(
            bounded_operations_detail::arithmetic_error(31404));
    component_03_capabilities<T> out{{owner}, {owner}, {owner, &ledger, &budget},
                                     {owner, &ledger, &budget}};
    return boolean_outcome<component_03_capabilities<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<named_precision_consumer_capabilities<T>> make_named_precision_capabilities(
    const context_owner_token &owner, precision_ledger &ledger, tolerance_budget &budget) {
    auto base = make_precision_capabilities<T>(owner, ledger, budget);
    if (!base.has_value())
        return boolean_outcome<named_precision_consumer_capabilities<T>>::failure(*base.error());
    named_precision_consumer_capabilities<T> out{
        {owner, {owner}, {owner}},
        {{owner}, {owner}},
        {owner},
        {owner},
        {{owner, &ledger, &budget}, {owner, &ledger, &budget}},
        {owner, &ledger, &budget},
        {owner},
        {owner}};
    return boolean_outcome<named_precision_consumer_capabilities<T>>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
