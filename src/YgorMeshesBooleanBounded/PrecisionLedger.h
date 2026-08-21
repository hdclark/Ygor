#pragma once

#include "BoundedOperations.h"
#include "CanonicalBytes.h"
#include "PrecisionTrace.h"
#include "Sha256.h"

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct precision_ledger_record final {
    std::uint16_t schema_version = 1;
    std::uint16_t provider_version = 1;
    context_owner_token owner{};
    precision_ledger_entry_id id{0};
    bounded_value_id result{0};
    geometric_lineage_id lineage{0};
    std::uint16_t operation_code = static_cast<std::uint16_t>(rounded_operation_code::source_import);
    std::uint16_t exact_formula_code = 0;
    std::vector<precision_ledger_entry_id> ordered_parents;
    std::vector<provenance_id> source_provenance;
    std::vector<std::uint8_t> rounded_nominal_bits;
    std::vector<std::uint8_t> enclosure_bits;
    uncertainty_contributors contributors{};
    double no_motion_uncertainty = 0.0;
    double cumulative_displacement = 0.0;
    double lineage_precision = 0.0;
    bool exact_tie = false;
    bool within_tolerance = false;
    precision_trace_id trace{0};
    std::vector<std::uint8_t> replay_identity;
    std::vector<std::uint8_t> canonical_digest_contribution;
};

inline std::vector<std::uint8_t> precision_ledger_record_digest(
    const precision_ledger_record &record) {
    canonical_writer writer;
    writer.u16(record.schema_version); writer.u16(record.provider_version);
    writer.u64(record.id.ordinal()); writer.u64(record.result.ordinal()); writer.u64(record.lineage.ordinal());
    writer.u16(record.operation_code); writer.u16(record.exact_formula_code);
    writer.u64(record.ordered_parents.size());
    for (const auto value : record.ordered_parents) writer.u64(value.ordinal());
    writer.u64(record.source_provenance.size());
    for (const auto value : record.source_provenance) writer.u64(value.ordinal());
    writer.sized_bytes(record.rounded_nominal_bits); writer.sized_bytes(record.enclosure_bits);
    const double contributors[]{record.contributors.inherited_a, record.contributors.inherited_b,
        record.contributors.machine_floor, record.contributors.construction,
        record.contributors.conditioning, record.contributors.conversion,
        record.contributors.prior_cleanup, record.contributors.current_cleanup};
    for (double value : contributors) writer.floating(value);
    writer.floating(record.no_motion_uncertainty); writer.floating(record.cumulative_displacement);
    writer.floating(record.lineage_precision); writer.boolean(record.exact_tie);
    writer.boolean(record.within_tolerance); writer.u64(record.trace.ordinal());
    writer.sized_bytes(record.replay_identity);
    const auto digest = sha256::digest(writer.bytes());
    return {digest.bytes.begin(), digest.bytes.end()};
}

struct lineage_precision_total final {
    geometric_lineage_id lineage{0};
    double no_motion_uncertainty = 0.0;
    double cumulative_displacement = 0.0;
    double precision = 0.0;
};

struct precision_ledger_snapshot final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    std::vector<precision_ledger_record> records;
    std::vector<lineage_precision_total> lineages;
    double global_output_precision = 0.0;
};

class precision_ledger final {
  public:
    precision_ledger(context_owner_token owner, double tolerance)
        : owner_(std::move(owner)), tolerance_(tolerance) {}
    boolean_outcome<precision_ledger_entry_id> append(precision_ledger_record record);
    std::shared_ptr<const precision_ledger_snapshot> snapshot() const;
    const context_owner_token &owner() const noexcept { return owner_; }
    double tolerance() const noexcept { return tolerance_; }
  private:
    context_owner_token owner_;
    double tolerance_ = 0.0;
    std::vector<precision_ledger_record> records_;
    std::map<std::uint64_t, lineage_precision_total> totals_;
};

template<class T>
struct committed_bounded_scalar final {
    published_bounded<bounded_scalar<T>> value;
    precision_trace_snapshot trace;
    precision_ledger_entry_id ledger_entry{0};
};

template<class T>
boolean_outcome<committed_bounded_scalar<T>> finalize_and_publish_bounded_scalar(
    precision_ledger &ledger, const local_precision_trace &local_trace,
    local_trace_node_id root, precision_trace_id trace_id,
    bounded_value_id value_id, provenance_id provenance,
    geometric_lineage_id lineage, const bounded_scalar<T> &local_value,
    std::vector<precision_ledger_entry_id> ordered_parents = {}) {
    if (!ledger.owner().anchor || !local_trace.owner().anchor ||
        !ledger.owner().same_owner(local_trace.owner()) ||
        !local_value.identity.owner.anchor ||
        !ledger.owner().same_owner(local_value.identity.owner) ||
        local_value.identity.publication != bounded_publication_state::transaction_local ||
        local_value.identity.trace_root != root.ordinal) {
        return boolean_outcome<committed_bounded_scalar<T>>::failure(
            bounded_operations_detail::arithmetic_error(31607));
    }
    auto finalized = finalize_precision_trace(local_trace, root, trace_id);
    if (!finalized.has_value())
        return boolean_outcome<committed_bounded_scalar<T>>::failure(*finalized.error());
    if (finalized.value()->nodes.empty())
        return boolean_outcome<committed_bounded_scalar<T>>::failure(
            bounded_operations_detail::arithmetic_error(31608));

    precision_ledger_record record;
    const auto snapshot = ledger.snapshot();
    record.owner = ledger.owner();
    record.id = precision_ledger_entry_id(snapshot->records.size());
    record.result = value_id;
    record.lineage = lineage;
    const auto &root_node = finalized.value()->nodes[finalized.value()->root.ordinal()];
    record.operation_code = root_node.key.operation_code;
    record.exact_formula_code = root_node.key.exact_formula_code;
    record.ordered_parents = std::move(ordered_parents);
    record.source_provenance.reserve(root_node.key.provenance.size() + 1);
    for (const auto source : root_node.key.provenance)
        record.source_provenance.push_back(provenance_id(source));
    if (provenance.ordinal() != 0) record.source_provenance.push_back(provenance);
    std::sort(record.source_provenance.begin(), record.source_provenance.end());
    record.source_provenance.erase(
        std::unique(record.source_provenance.begin(), record.source_provenance.end()),
        record.source_provenance.end());
    canonical_writer nominal;
    nominal.floating(local_value.rounded_nominal);
    record.rounded_nominal_bits = nominal.take();
    canonical_writer enclosure;
    enclosure.floating(local_value.uncertainty_enclosure.lower());
    enclosure.floating(local_value.uncertainty_enclosure.upper());
    record.enclosure_bits = enclosure.take();
    record.contributors = local_value.contributors;
    record.trace = trace_id;
    const double no_motion_terms[]{record.contributors.inherited_a,
        record.contributors.inherited_b, record.contributors.machine_floor,
        record.contributors.construction, record.contributors.conditioning,
        record.contributors.conversion};
    for (const double term : no_motion_terms) {
        const auto sum = directed_add(record.no_motion_uncertainty, term);
        if (!sum)
            return boolean_outcome<committed_bounded_scalar<T>>::failure(
                bounded_operations_detail::arithmetic_error(31609));
        record.no_motion_uncertainty = sum.value.upper;
    }
    const auto displacement = directed_add(record.contributors.prior_cleanup,
                                           record.contributors.current_cleanup);
    if (!displacement)
        return boolean_outcome<committed_bounded_scalar<T>>::failure(
            bounded_operations_detail::arithmetic_error(31609));
    record.cumulative_displacement = displacement.value.upper;
    auto appended = ledger.append(std::move(record));
    if (!appended.has_value())
        return boolean_outcome<committed_bounded_scalar<T>>::failure(*appended.error());

    auto published = std::make_shared<bounded_scalar<T>>(local_value);
    published->identity.owner = ledger.owner();
    published->identity.value = value_id;
    published->identity.provenance = provenance;
    published->identity.lineage = lineage;
    published->identity.ledger_entry = *appended.value();
    published->identity.trace_root = finalized.value()->root.ordinal();
    published->identity.publication = bounded_publication_state::committed;
    committed_bounded_scalar<T> out;
    out.value = std::move(published);
    out.trace = std::move(*finalized.value());
    out.ledger_entry = *appended.value();
    return boolean_outcome<committed_bounded_scalar<T>>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
