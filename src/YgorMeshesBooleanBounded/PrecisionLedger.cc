#include "PrecisionLedger.h"

#include <cmath>
#include <set>

namespace ygor::mesh_boolean::bounded {
namespace {
bounded_boolean_error ledger_error(std::uint32_t code) {
    bounded_boolean_error e;
    e.category = bounded_boolean_error_category::internal_invariant_error;
    e.stage = 4; e.checkpoint = 16; e.component = 3; e.subcode = code;
    e.summary = "precision ledger append failed";
    return e;
}
bool finite_nonnegative(double value) { return std::isfinite(value) && value >= 0.0; }
bool add_up(double a, double b, double &out) {
    const auto result = directed_add(a, b);
    if (!result) return false;
    out = result.value.upper;
    return true;
}
bool contributors_complete(const precision_ledger_record &record) {
    double no_motion = 0.0;
    const double no_motion_terms[]{record.contributors.inherited_a,
                                   record.contributors.inherited_b,
                                   record.contributors.machine_floor,
                                   record.contributors.construction,
                                   record.contributors.conditioning,
                                   record.contributors.conversion};
    for (const auto term : no_motion_terms)
        if (!finite_nonnegative(term) || !add_up(no_motion, term, no_motion)) return false;
    double displacement = 0.0;
    return finite_nonnegative(record.contributors.prior_cleanup) &&
           finite_nonnegative(record.contributors.current_cleanup) &&
           add_up(record.contributors.prior_cleanup, record.contributors.current_cleanup, displacement) &&
            record.no_motion_uncertainty == no_motion &&
            record.cumulative_displacement == displacement;
}
}

boolean_outcome<precision_ledger_entry_id> precision_ledger::append(precision_ledger_record record) {
    if (!owner_.anchor || !finite_nonnegative(tolerance_) ||
        record.schema_version != 1 || record.provider_version != 1 ||
        !record.owner.anchor || !record.owner.same_owner(owner_) ||
        !registered_rounded_operation(static_cast<rounded_operation_code>(record.operation_code)) ||
        !valid_exact_formula_code(record.exact_formula_code) ||
        record.id.ordinal() != records_.size() || !finite_nonnegative(record.no_motion_uncertainty) ||
        !finite_nonnegative(record.cumulative_displacement) || !contributors_complete(record))
        return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31601));
    std::set<std::uint64_t> parents;
    for (const auto parent : record.ordered_parents) {
        if (parent.ordinal() >= record.id.ordinal())
            return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31602));
        if (!parents.insert(parent.ordinal()).second)
            return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31602));
    }
    for (std::size_t i = 1; i < record.source_provenance.size(); ++i)
        if (!(record.source_provenance[i - 1] < record.source_provenance[i]))
            return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31604));
    for (const auto &existing : records_)
        if (existing.result == record.result)
            return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31605));

    auto new_totals = totals_;
    auto new_records = records_;
    auto &total = new_totals[record.lineage.ordinal()];
    total.lineage = record.lineage;
    double no_motion = 0.0, displacement = 0.0, precision = 0.0;
    if (!add_up(total.no_motion_uncertainty, record.no_motion_uncertainty, no_motion) ||
        !add_up(total.cumulative_displacement, record.cumulative_displacement, displacement) ||
        !add_up(no_motion, displacement, precision))
        return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31603));
    for (const auto parent : record.ordered_parents)
        precision = std::max(precision, records_[parent.ordinal()].lineage_precision);
    total.no_motion_uncertainty = no_motion;
    total.cumulative_displacement = displacement;
    total.precision = precision;
    record.lineage_precision = precision;
    record.within_tolerance = precision <= tolerance_;
    record.owner = owner_;
    const auto digest = precision_ledger_record_digest(record);
    if (!record.canonical_digest_contribution.empty() &&
        record.canonical_digest_contribution != digest)
        return boolean_outcome<precision_ledger_entry_id>::failure(ledger_error(31606));
    record.canonical_digest_contribution = digest;
    new_records.push_back(std::move(record));
    totals_.swap(new_totals);
    records_.swap(new_records);
    return boolean_outcome<precision_ledger_entry_id>::success(records_.back().id);
}

std::shared_ptr<const precision_ledger_snapshot> precision_ledger::snapshot() const {
    auto result = std::make_shared<precision_ledger_snapshot>();
    result->owner = owner_;
    result->records = records_;
    result->lineages.reserve(totals_.size());
    for (const auto &entry : totals_) {
        result->lineages.push_back(entry.second);
        result->global_output_precision = std::max(result->global_output_precision, entry.second.precision);
    }
    return result;
}

} // namespace ygor::mesh_boolean::bounded
