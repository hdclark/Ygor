#include "ToleranceBudget.h"

#include "CanonicalBytes.h"
#include "Sha256.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace ygor::mesh_boolean::bounded {

struct tolerance_budget_state final {
    context_owner_token owner;
    double tolerance = 0.0;
    resource_manager *resources = nullptr;
    resource_cancellation_checkpoint cancellation;
    std::map<std::uint64_t, double> totals;
    std::map<std::uint64_t, tolerance_budget_proposal> reservations;
    std::map<std::uint64_t, topology_authorization> authorizations;
    std::vector<tolerance_budget_commit> commits;
    std::uint64_t next_reservation_id = 0;
    std::uint64_t next_authorization_id = 0;
};

namespace {

bounded_boolean_error budget_error(std::uint32_t code) {
    bounded_boolean_error error;
    error.category = code == 31705 || code == 31805
        ? bounded_boolean_error_category::cancelled
        : code == 31706 || code == 31806
            ? bounded_boolean_error_category::resource_limit
            : bounded_boolean_error_category::cleanup_budget_exceeded;
    error.stage = 4;
    error.checkpoint = 17;
    error.component = 3;
    error.subcode = code;
    error.summary = "tolerance budget operation rejected";
    return error;
}

bool valid_cost(double cost) { return std::isfinite(cost) && cost >= 0.0; }

bool sum_up(double a, double b, double &out) {
    const auto sum = directed_add(a, b);
    if (!sum) return false;
    out = sum.value.upper;
    return true;
}

bool valid_operation(cleanup_operation_kind operation) {
    return operation >= cleanup_operation_kind::move_vertex &&
           operation <= cleanup_operation_kind::remove_component;
}

std::vector<std::uint8_t> authorization_digest(const topology_authorization &authorization) {
    canonical_writer writer;
    writer.u16(authorization.schema_version);
    writer.u64(authorization.id.ordinal());
    writer.u8(static_cast<std::uint8_t>(authorization.operation));
    writer.u64(authorization.transaction_owner);
    writer.sized_bytes(authorization.evidence);
    const auto digest = sha256::digest(writer.bytes());
    return {digest.bytes.begin(), digest.bytes.end()};
}

bool same_authorization(const topology_authorization &left,
                        const topology_authorization &right) {
    return left.schema_version == right.schema_version && left.id == right.id &&
           left.owner.same_owner(right.owner) && left.operation == right.operation &&
           left.transaction_owner == right.transaction_owner &&
           left.evidence == right.evidence && left.authentication == right.authentication;
}

bool valid_length_evidence(const budget_length_evidence &evidence,
                           const context_owner_token &owner,
                           std::uint64_t transaction_owner,
                           std::size_t lineage_count) {
    if (evidence.schema_version != 1 || !evidence.owner.same_owner(owner) ||
        evidence.transaction_owner != transaction_owner || evidence.evidence.empty() ||
        evidence.cumulative_lengths.size() != lineage_count) return false;
    return std::all_of(evidence.cumulative_lengths.begin(), evidence.cumulative_lengths.end(),
                       valid_cost);
}

bool valid_certificate(const tolerance_budget_proposal &proposal) {
    if (proposal.certificate.schema_version != 1 ||
        proposal.certificate.units != length_unit_kind::coordinate_length ||
        proposal.certificate.evidence.empty()) return false;
    switch (proposal.operation) {
    case cleanup_operation_kind::move_vertex:
        return proposal.certificate.kind == length_certificate_kind::vertex_displacement ||
               proposal.certificate.kind == length_certificate_kind::swept_displacement;
    case cleanup_operation_kind::collapse_feature:
        return proposal.certificate.kind == length_certificate_kind::feature_thickness_or_clearance ||
               proposal.certificate.kind == length_certificate_kind::swept_displacement;
    case cleanup_operation_kind::remove_patch:
        return proposal.certificate.kind == length_certificate_kind::patch_hausdorff_deviation ||
               proposal.certificate.kind == length_certificate_kind::feature_thickness_or_clearance;
    case cleanup_operation_kind::remove_component:
        return proposal.certificate.kind == length_certificate_kind::whole_component_deviation;
    }
    return false;
}

bool proposal_less(const tolerance_budget_proposal &left,
                   const tolerance_budget_proposal &right) {
    if (left.canonical_merge_key != right.canonical_merge_key)
        return left.canonical_merge_key < right.canonical_merge_key;
    return left.id < right.id;
}

bool verified_delta(double before, double after, double &delta) {
    if (!valid_cost(before) || !valid_cost(after) || after < before) return false;
    const auto difference = directed_subtract(after, before);
    if (!difference) return false;
    delta = difference.value.upper;
    return valid_cost(delta);
}

} // namespace

tolerance_budget::tolerance_budget(context_owner_token owner, double tolerance)
    : tolerance_budget(std::move(owner), tolerance, nullptr, {}) {}

tolerance_budget::tolerance_budget(context_owner_token owner, double tolerance,
                                   resource_manager *resources,
                                   resource_cancellation_checkpoint cancellation)
    : state_(std::make_shared<tolerance_budget_state>()) {
    state_->owner = std::move(owner);
    state_->tolerance = tolerance;
    state_->resources = resources;
    state_->cancellation = std::move(cancellation);
}

const context_owner_token &tolerance_budget::owner() const noexcept { return state_->owner; }
bool tolerance_budget::can_commit_stage() const noexcept { return state_->reservations.empty(); }

boolean_outcome<topology_authorization> tolerance_budget::authorize_topology(
    cleanup_operation_kind operation, std::uint64_t transaction_owner,
    std::vector<std::uint8_t> evidence) {
    if (!state_->owner.anchor || !valid_operation(operation) || transaction_owner == 0 ||
        evidence.empty())
        return boolean_outcome<topology_authorization>::failure(budget_error(31707));
    if (state_->cancellation.cancellation_requested())
        return boolean_outcome<topology_authorization>::failure(budget_error(31705));

    topology_authorization authorization;
    authorization.id = topology_authorization_id(state_->next_authorization_id);
    authorization.owner = state_->owner;
    authorization.operation = operation;
    authorization.transaction_owner = transaction_owner;
    authorization.evidence = std::move(evidence);
    authorization.authentication = authorization_digest(authorization);
    try {
        auto authorizations = state_->authorizations;
        authorizations.emplace(authorization.id.ordinal(), authorization);
        state_->authorizations.swap(authorizations);
        ++state_->next_authorization_id;
    } catch (...) {
        return boolean_outcome<topology_authorization>::failure(budget_error(31706));
    }
    return boolean_outcome<topology_authorization>::success(std::move(authorization));
}

tolerance_reservation::tolerance_reservation(std::shared_ptr<tolerance_budget_state> state,
                                             budget_reservation_id id,
                                             const tolerance_budget_proposal &proposal)
    : state_(std::move(state)), id_(id), proposal_(proposal.id),
      transaction_owner_(proposal.transaction_owner), lineages_(proposal.ordered_lineages),
      reserved_(proposal.requested_costs) {}

tolerance_reservation::tolerance_reservation(tolerance_reservation &&other) noexcept
    : state_(std::move(other.state_)), id_(other.id_), proposal_(other.proposal_),
      transaction_owner_(other.transaction_owner_), lineages_(std::move(other.lineages_)),
      reserved_(std::move(other.reserved_)),
      proposal_resource_(std::move(other.proposal_resource_)),
      reservation_resource_(std::move(other.reservation_resource_)) {}

tolerance_reservation &tolerance_reservation::operator=(tolerance_reservation &&other) noexcept {
    if (this != &other) {
        release();
        state_ = std::move(other.state_);
        id_ = other.id_;
        proposal_ = other.proposal_;
        transaction_owner_ = other.transaction_owner_;
        lineages_ = std::move(other.lineages_);
        reserved_ = std::move(other.reserved_);
        proposal_resource_ = std::move(other.proposal_resource_);
        reservation_resource_ = std::move(other.reservation_resource_);
    }
    return *this;
}

tolerance_reservation::~tolerance_reservation() { release(); }

void tolerance_reservation::release() noexcept {
    if (state_) state_->reservations.erase(id_.ordinal());
    reservation_resource_.reset();
    proposal_resource_.reset();
    state_.reset();
}

boolean_outcome<tolerance_reservation> tolerance_budget::reserve(
    const tolerance_budget_proposal &proposal) {
    if (state_->cancellation.cancellation_requested())
        return boolean_outcome<tolerance_reservation>::failure(budget_error(31705));
    if (!state_->owner.anchor || !valid_cost(state_->tolerance) ||
        !proposal.owner.same_owner(state_->owner) || proposal.transaction_owner == 0 ||
        proposal.ordered_lineages.empty() ||
        proposal.ordered_lineages.size() != proposal.requested_costs.size() ||
        !valid_cost(proposal.certificate.upper_length) || !valid_certificate(proposal) ||
        !valid_length_evidence(proposal.before_evidence, state_->owner,
                               proposal.transaction_owner, proposal.ordered_lineages.size()) ||
        proposal.authorization.operation != proposal.operation ||
        proposal.authorization.transaction_owner != proposal.transaction_owner ||
        proposal.authorization.authentication != authorization_digest(proposal.authorization))
        return boolean_outcome<tolerance_reservation>::failure(budget_error(31701));

    const auto issued = state_->authorizations.find(proposal.authorization.id.ordinal());
    if (issued == state_->authorizations.end() ||
        !same_authorization(issued->second, proposal.authorization))
        return boolean_outcome<tolerance_reservation>::failure(budget_error(31708));
    for (const auto &entry : state_->reservations)
        if (entry.second.id == proposal.id)
            return boolean_outcome<tolerance_reservation>::failure(budget_error(31704));
    for (const auto &entry : state_->commits)
        if (entry.proposal == proposal.id)
            return boolean_outcome<tolerance_reservation>::failure(budget_error(31704));

    for (std::size_t i = 0; i < proposal.ordered_lineages.size(); ++i) {
        if (!valid_cost(proposal.requested_costs[i]) ||
            proposal.requested_costs[i] > proposal.certificate.upper_length ||
            (i && !(proposal.ordered_lineages[i - 1] < proposal.ordered_lineages[i])))
            return boolean_outcome<tolerance_reservation>::failure(budget_error(31702));
        const auto existing = state_->totals.find(proposal.ordered_lineages[i].ordinal());
        double committed = existing == state_->totals.end() ? 0.0 : existing->second;
        for (const auto &active : state_->reservations) {
            const auto &reserved_proposal = active.second;
            const auto position = std::lower_bound(reserved_proposal.ordered_lineages.begin(),
                                                   reserved_proposal.ordered_lineages.end(),
                                                   proposal.ordered_lineages[i]);
            if (position != reserved_proposal.ordered_lineages.end() &&
                *position == proposal.ordered_lineages[i]) {
                const auto index = static_cast<std::size_t>(
                    position - reserved_proposal.ordered_lineages.begin());
                if (!sum_up(committed, reserved_proposal.requested_costs[index], committed))
                    return boolean_outcome<tolerance_reservation>::failure(budget_error(31703));
            }
        }
        double proposed = 0.0;
        if (!sum_up(committed, proposal.requested_costs[i], proposed) ||
            proposed > state_->tolerance)
            return boolean_outcome<tolerance_reservation>::failure(budget_error(31703));
    }

    std::optional<resource_reservation> proposal_resource;
    std::optional<resource_reservation> reservation_resource;
    if (state_->resources) {
        proposal_resource = state_->resources->reserve(resource_kind::budget_proposals, 1,
                                                        state_->cancellation);
        if (!proposal_resource)
            return boolean_outcome<tolerance_reservation>::failure(
                budget_error(state_->cancellation.cancellation_requested() ? 31705 : 31706));
        reservation_resource = state_->resources->reserve(resource_kind::budget_reservations, 1,
                                                           state_->cancellation);
        if (!reservation_resource)
            return boolean_outcome<tolerance_reservation>::failure(
                budget_error(state_->cancellation.cancellation_requested() ? 31705 : 31706));
    }

    const budget_reservation_id id(state_->next_reservation_id);
    try {
        auto reservations = state_->reservations;
        reservations.emplace(id.ordinal(), proposal);
        if (state_->cancellation.cancellation_requested())
            return boolean_outcome<tolerance_reservation>::failure(budget_error(31705));
        state_->reservations.swap(reservations);
    } catch (...) {
        return boolean_outcome<tolerance_reservation>::failure(budget_error(31706));
    }
    state_->authorizations.erase(issued);
    ++state_->next_reservation_id;
    tolerance_reservation result(state_, id, proposal);
    result.proposal_resource_ = std::move(proposal_resource);
    result.reservation_resource_ = std::move(reservation_resource);
    return boolean_outcome<tolerance_reservation>::success(std::move(result));
}

boolean_outcome<budget_commit_id> tolerance_budget::commit(
    tolerance_reservation &&reservation, std::vector<double> reported_actual_costs,
    budget_length_evidence after_evidence, std::uint64_t transaction_owner,
    const precision_ledger &ledger,
    std::vector<precision_ledger_entry_id> ledger_entries) {
    if (state_->cancellation.cancellation_requested())
        return boolean_outcome<budget_commit_id>::failure(budget_error(31805));
    if (reservation.state_ != state_ || reservation.transaction_owner_ != transaction_owner ||
        reported_actual_costs.size() != reservation.reserved_.size() ||
        ledger_entries.size() != reservation.reserved_.size() ||
        !ledger.owner().same_owner(state_->owner) ||
        !valid_length_evidence(after_evidence, state_->owner, transaction_owner,
                               reservation.reserved_.size()))
        return boolean_outcome<budget_commit_id>::failure(budget_error(31801));

    const auto live = state_->reservations.find(reservation.id_.ordinal());
    if (live == state_->reservations.end() ||
        (!state_->commits.empty() &&
         !proposal_less(state_->commits.back().proposal_record, live->second)))
        return boolean_outcome<budget_commit_id>::failure(budget_error(31803));

    const auto ledger_snapshot = ledger.snapshot();
    if (!ledger_snapshot || !ledger_snapshot->owner.same_owner(state_->owner))
        return boolean_outcome<budget_commit_id>::failure(budget_error(31804));
    std::set<std::uint64_t> linked_entries;
    std::vector<double> verified_costs(reported_actual_costs.size());
    std::vector<double> new_totals(reported_actual_costs.size());
    for (std::size_t i = 0; i < reported_actual_costs.size(); ++i) {
        const auto entry = ledger_entries[i].ordinal();
        if (entry >= ledger_snapshot->records.size() || !linked_entries.insert(entry).second ||
            !verified_delta(live->second.before_evidence.cumulative_lengths[i],
                            after_evidence.cumulative_lengths[i], verified_costs[i]) ||
            reported_actual_costs[i] != verified_costs[i] ||
            verified_costs[i] > reservation.reserved_[i])
            return boolean_outcome<budget_commit_id>::failure(budget_error(31802));
        const auto &record = ledger_snapshot->records[entry];
        if (!record.owner.same_owner(state_->owner) ||
            record.lineage != reservation.lineages_[i] ||
            record.contributors.current_cleanup != verified_costs[i] ||
            record.canonical_digest_contribution != precision_ledger_record_digest(record) ||
            !sum_up(state_->totals.count(reservation.lineages_[i].ordinal())
                        ? state_->totals.at(reservation.lineages_[i].ordinal()) : 0.0,
                    verified_costs[i], new_totals[i]) ||
            new_totals[i] > state_->tolerance)
            return boolean_outcome<budget_commit_id>::failure(budget_error(31804));
    }

    std::optional<resource_reservation> commit_resource;
    if (state_->resources) {
        commit_resource = state_->resources->reserve(resource_kind::budget_commits, 1,
                                                      state_->cancellation);
        if (!commit_resource)
            return boolean_outcome<budget_commit_id>::failure(
                budget_error(state_->cancellation.cancellation_requested() ? 31805 : 31806));
    }

    std::map<std::uint64_t, double> totals;
    std::vector<tolerance_budget_commit> commits;
    std::map<std::uint64_t, tolerance_budget_proposal> reservations;
    try {
        totals = state_->totals;
        commits = state_->commits;
        reservations = state_->reservations;
        const budget_commit_id id(commits.size());
        for (std::size_t i = 0; i < verified_costs.size(); ++i)
            totals[reservation.lineages_[i].ordinal()] = new_totals[i];
        tolerance_budget_commit record;
        record.id = id;
        record.proposal = reservation.proposal_;
        record.actual_costs = verified_costs;
        record.after_evidence = std::move(after_evidence);
        record.ledger_entries = std::move(ledger_entries);
        record.proposal_record = live->second;
        record.cumulative_totals = new_totals;
        commits.push_back(std::move(record));
        reservations.erase(reservation.id_.ordinal());
    } catch (...) {
        return boolean_outcome<budget_commit_id>::failure(budget_error(31806));
    }
    if (state_->cancellation.cancellation_requested())
        return boolean_outcome<budget_commit_id>::failure(budget_error(31805));

    if (commit_resource && !commit_resource->commit(1))
        return boolean_outcome<budget_commit_id>::failure(budget_error(31806));
    if (reservation.proposal_resource_ && !reservation.proposal_resource_->commit(1))
        return boolean_outcome<budget_commit_id>::failure(budget_error(31806));
    reservation.reservation_resource_.reset();
    state_->totals.swap(totals);
    state_->commits.swap(commits);
    state_->reservations.swap(reservations);
    reservation.state_.reset();
    return boolean_outcome<budget_commit_id>::success(state_->commits.back().id);
}

void tolerance_budget::rollback(budget_reservation_id id) noexcept {
    state_->reservations.erase(id.ordinal());
}

std::shared_ptr<const tolerance_budget_snapshot> tolerance_budget::snapshot() const {
    auto result = std::make_shared<tolerance_budget_snapshot>();
    result->owner = state_->owner;
    result->tolerance = state_->tolerance;
    result->commits = state_->commits;
    result->active_reservations = state_->reservations.size();
    for (const auto &reservation : state_->reservations)
        result->reservations.push_back(reservation.second);
    std::sort(result->reservations.begin(), result->reservations.end(), proposal_less);
    for (const auto &entry : state_->totals) {
        result->lineages.push_back({geometric_lineage_id(entry.first), entry.second});
        result->global_realized_displacement =
            std::max(result->global_realized_displacement, entry.second);
    }
    return result;
}

} // namespace ygor::mesh_boolean::bounded
