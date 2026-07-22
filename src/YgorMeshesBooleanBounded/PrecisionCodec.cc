#include "StrictFloatingBuild.h"
#include "PrecisionCodec.h"

namespace ygor::mesh_boolean::bounded {
static_assert(precision_codec_v1 != 0, "precision codec must be versioned");
namespace {
constexpr std::uint64_t maximum_records = 1U << 20;

bool read_sized(canonical_reader &reader, std::vector<std::uint8_t> &value) {
    return reader.sized_bytes(value, reader.remaining());
}
template<class Id>
void write_ids(canonical_writer &writer, const std::vector<Id> &values) {
    writer.u64(values.size());
    for (const auto value : values) writer.u64(value.ordinal());
}
void write_u64s(canonical_writer &writer, const std::vector<std::uint64_t> &values) {
    writer.u64(values.size());
    for (const auto value : values) writer.u64(value);
}
template<class Id>
bool read_ids(canonical_reader &reader, std::vector<Id> &values) {
    std::uint64_t count = 0;
    if (!reader.u64(count) || count > maximum_records || count > reader.remaining() / 8) return false;
    values.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0, value = 0; i < count; ++i) {
        if (!reader.u64(value)) return false;
        values.emplace_back(value);
    }
    return true;
}
bool read_u64s(canonical_reader &reader, std::vector<std::uint64_t> &values) {
    std::uint64_t count = 0;
    if (!reader.u64(count) || count > maximum_records || count > reader.remaining() / 8) return false;
    values.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0, value = 0; i < count; ++i) {
        if (!reader.u64(value)) return false;
        values.push_back(value);
    }
    return true;
}
void write_trace_key(canonical_writer &writer, const precision_trace_key &key) {
    writer.u16(key.operation_code); writer.u16(key.exact_formula_code);
    write_u64s(writer, key.parents); writer.sized_bytes(key.result_bytes);
    writer.sized_bytes(key.contributor_bytes); write_u64s(writer, key.provenance);
}
bool read_trace_key(canonical_reader &reader, precision_trace_key &key) {
    return reader.u16(key.operation_code) && reader.u16(key.exact_formula_code) &&
           read_u64s(reader, key.parents) && read_sized(reader, key.result_bytes) &&
           read_sized(reader, key.contributor_bytes) && read_u64s(reader, key.provenance);
}
void write_length_evidence(canonical_writer &writer, const budget_length_evidence &value) {
    writer.u16(value.schema_version); writer.u64(value.transaction_owner);
    writer.u64(value.cumulative_lengths.size());
    for (double length : value.cumulative_lengths) writer.floating(length);
    writer.sized_bytes(value.evidence);
}
bool read_length_evidence(canonical_reader &reader, budget_length_evidence &value) {
    std::uint64_t count = 0;
    if (!reader.u16(value.schema_version) || !reader.u64(value.transaction_owner) ||
        !reader.u64(count) || count > maximum_records) return false;
    value.cumulative_lengths.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
        double length = 0.0;
        if (!reader.floating(length)) return false;
        value.cumulative_lengths.push_back(length);
    }
    return read_sized(reader, value.evidence);
}
void write_authorization(canonical_writer &writer, const topology_authorization &value) {
    writer.u16(value.schema_version); writer.u64(value.id.ordinal());
    writer.u8(static_cast<std::uint8_t>(value.operation)); writer.u64(value.transaction_owner);
    writer.sized_bytes(value.evidence); writer.sized_bytes(value.authentication);
}
bool read_authorization(canonical_reader &reader, topology_authorization &value) {
    std::uint64_t id = 0; std::uint8_t operation = 0;
    if (!reader.u16(value.schema_version) || !reader.u64(id) || !reader.u8(operation) ||
        !reader.u64(value.transaction_owner) || !read_sized(reader, value.evidence) ||
        !read_sized(reader, value.authentication)) return false;
    value.id = topology_authorization_id(id);
    value.operation = static_cast<cleanup_operation_kind>(operation);
    return true;
}
void write_proposal(canonical_writer &writer, const tolerance_budget_proposal &value) {
    writer.u64(value.id.ordinal()); writer.u8(static_cast<std::uint8_t>(value.operation));
    write_ids(writer, value.ordered_lineages); writer.u64(value.requested_costs.size());
    for (const double cost : value.requested_costs) writer.floating(cost);
    writer.u16(value.certificate.schema_version); writer.u64(value.certificate.id.ordinal());
    writer.u8(static_cast<std::uint8_t>(value.certificate.kind));
    writer.u8(static_cast<std::uint8_t>(value.certificate.units));
    writer.floating(value.certificate.upper_length); writer.sized_bytes(value.certificate.evidence);
    write_length_evidence(writer, value.before_evidence);
    write_authorization(writer, value.authorization); writer.u64(value.transaction_owner);
    writer.sized_bytes(value.canonical_merge_key);
}
bool read_proposal(canonical_reader &reader, tolerance_budget_proposal &value) {
    std::uint64_t id = 0, count = 0, certificate = 0;
    std::uint8_t operation = 0, kind = 0, units = 0;
    if (!reader.u64(id) || !reader.u8(operation) || !read_ids(reader, value.ordered_lineages) ||
        !reader.u64(count) || count > maximum_records || count > reader.remaining() / sizeof(double)) return false;
    value.id = budget_proposal_id(id); value.operation = static_cast<cleanup_operation_kind>(operation);
    value.requested_costs.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
        double cost = 0.0; if (!reader.floating(cost)) return false; value.requested_costs.push_back(cost);
    }
    if (!reader.u16(value.certificate.schema_version) || !reader.u64(certificate) ||
        !reader.u8(kind) || !reader.u8(units) || !reader.floating(value.certificate.upper_length) ||
        !read_sized(reader, value.certificate.evidence) ||
        !read_length_evidence(reader, value.before_evidence) ||
        !read_authorization(reader, value.authorization) || !reader.u64(value.transaction_owner) ||
        !read_sized(reader, value.canonical_merge_key)) return false;
    value.certificate.id = displacement_certificate_id(certificate);
    value.certificate.kind = static_cast<length_certificate_kind>(kind);
    value.certificate.units = static_cast<length_unit_kind>(units);
    return true;
}
bool read_state_header(canonical_reader &reader, std::uint32_t expected, std::uint16_t &schema) {
    std::uint32_t magic = 0; std::uint16_t codec = 0;
    return reader.u32(magic) && magic == expected && reader.u16(codec) &&
           codec == precision_state_codec_v1 && reader.u16(schema) && schema == 1;
}
}

std::vector<std::uint8_t> encode_precision_trace(const precision_trace_snapshot &snapshot) {
    canonical_writer writer;
    writer.u32(0x54504759U); writer.u16(precision_state_codec_v1); writer.u16(snapshot.schema_version);
    writer.u64(snapshot.id.ordinal()); writer.u64(snapshot.root.ordinal()); writer.u64(snapshot.nodes.size());
    for (const auto &node : snapshot.nodes) {
        writer.u64(node.id.ordinal()); write_trace_key(writer, node.key);
        precision_codec_detail::encode_contributors(writer, node.contributors);
    }
    return precision_codec_detail::seal(writer.take());
}

boolean_outcome<precision_trace_snapshot> decode_precision_trace(const std::vector<std::uint8_t> &bytes) {
    std::vector<std::uint8_t> payload;
    if (!precision_codec_detail::unseal(bytes, payload)) return boolean_outcome<precision_trace_snapshot>::failure(
        precision_codec_error(30032, "malformed precision trace encoding"));
    canonical_reader reader(payload); precision_trace_snapshot out; std::uint64_t id = 0, root = 0, count = 0;
    if (!read_state_header(reader, 0x54504759U, out.schema_version) || !reader.u64(id) ||
        !reader.u64(root) || !reader.u64(count) || count > maximum_records)
        return boolean_outcome<precision_trace_snapshot>::failure(
            precision_codec_error(30032, "malformed precision trace encoding"));
    out.id = precision_trace_id(id); out.root = precision_trace_node_id(root);
    out.nodes.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0, node_id = 0; i < count; ++i) {
        committed_precision_trace_node node;
        if (!reader.u64(node_id) || !read_trace_key(reader, node.key) ||
            !precision_codec_detail::read_contributors(reader, node.contributors))
            return boolean_outcome<precision_trace_snapshot>::failure(
                precision_codec_error(30032, "malformed precision trace encoding"));
        node.id = precision_trace_node_id(node_id); out.nodes.push_back(std::move(node));
    }
    if (!reader.complete()) return boolean_outcome<precision_trace_snapshot>::failure(
        precision_codec_error(30032, "malformed precision trace encoding"));
    return boolean_outcome<precision_trace_snapshot>::success(std::move(out));
}

std::vector<std::uint8_t> encode_precision_ledger(const precision_ledger_snapshot &snapshot) {
    canonical_writer writer;
    writer.u32(0x4c504759U); writer.u16(precision_state_codec_v1); writer.u16(snapshot.schema_version);
    writer.u64(snapshot.records.size());
    for (const auto &record : snapshot.records) {
        writer.u16(record.schema_version); writer.u16(record.provider_version); writer.u64(record.id.ordinal());
        writer.u64(record.result.ordinal()); writer.u64(record.lineage.ordinal());
        writer.u16(record.operation_code); writer.u16(record.exact_formula_code);
        write_ids(writer, record.ordered_parents); write_ids(writer, record.source_provenance);
        writer.sized_bytes(record.rounded_nominal_bits); writer.sized_bytes(record.enclosure_bits);
        precision_codec_detail::encode_contributors(writer, record.contributors);
        writer.floating(record.no_motion_uncertainty); writer.floating(record.cumulative_displacement);
        writer.floating(record.lineage_precision); writer.boolean(record.exact_tie);
        writer.boolean(record.within_tolerance); writer.u64(record.trace.ordinal());
        writer.sized_bytes(record.replay_identity); writer.sized_bytes(record.canonical_digest_contribution);
    }
    writer.u64(snapshot.lineages.size());
    for (const auto &lineage : snapshot.lineages) {
        writer.u64(lineage.lineage.ordinal()); writer.floating(lineage.no_motion_uncertainty);
        writer.floating(lineage.cumulative_displacement); writer.floating(lineage.precision);
    }
    writer.floating(snapshot.global_output_precision);
    return precision_codec_detail::seal(writer.take());
}

boolean_outcome<precision_ledger_snapshot> decode_precision_ledger(const std::vector<std::uint8_t> &bytes) {
    std::vector<std::uint8_t> payload;
    if (!precision_codec_detail::unseal(bytes, payload)) return boolean_outcome<precision_ledger_snapshot>::failure(
        precision_codec_error(30033, "malformed precision ledger encoding"));
    canonical_reader reader(payload); precision_ledger_snapshot out; std::uint64_t count = 0;
    if (!read_state_header(reader, 0x4c504759U, out.schema_version) || !reader.u64(count) || count > maximum_records)
        return boolean_outcome<precision_ledger_snapshot>::failure(
            precision_codec_error(30033, "malformed precision ledger encoding"));
    out.records.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
        precision_ledger_record record; std::uint64_t id = 0, result = 0, lineage = 0, trace = 0;
        if (!reader.u16(record.schema_version) || !reader.u16(record.provider_version) || !reader.u64(id) ||
            !reader.u64(result) || !reader.u64(lineage) || !reader.u16(record.operation_code) ||
            !reader.u16(record.exact_formula_code) || !read_ids(reader, record.ordered_parents) ||
            !read_ids(reader, record.source_provenance) || !read_sized(reader, record.rounded_nominal_bits) ||
            !read_sized(reader, record.enclosure_bits) ||
            !precision_codec_detail::read_contributors(reader, record.contributors) ||
            !reader.floating(record.no_motion_uncertainty) ||
            !reader.floating(record.cumulative_displacement) || !reader.floating(record.lineage_precision) ||
            !reader.boolean(record.exact_tie) || !reader.boolean(record.within_tolerance) ||
            !reader.u64(trace) || !read_sized(reader, record.replay_identity) ||
            !read_sized(reader, record.canonical_digest_contribution))
            return boolean_outcome<precision_ledger_snapshot>::failure(
                precision_codec_error(30033, "malformed precision ledger encoding"));
        record.id = precision_ledger_entry_id(id); record.result = bounded_value_id(result);
        record.lineage = geometric_lineage_id(lineage); record.trace = precision_trace_id(trace);
        out.records.push_back(std::move(record));
    }
    if (!reader.u64(count) || count > maximum_records) return boolean_outcome<precision_ledger_snapshot>::failure(
        precision_codec_error(30033, "malformed precision ledger encoding"));
    out.lineages.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0, lineage = 0; i < count; ++i) {
        lineage_precision_total total;
        if (!reader.u64(lineage) || !reader.floating(total.no_motion_uncertainty) ||
            !reader.floating(total.cumulative_displacement) || !reader.floating(total.precision))
            return boolean_outcome<precision_ledger_snapshot>::failure(
                precision_codec_error(30033, "malformed precision ledger encoding"));
        total.lineage = geometric_lineage_id(lineage); out.lineages.push_back(total);
    }
    if (!reader.floating(out.global_output_precision) || !reader.complete())
        return boolean_outcome<precision_ledger_snapshot>::failure(
            precision_codec_error(30033, "malformed precision ledger encoding"));
    return boolean_outcome<precision_ledger_snapshot>::success(std::move(out));
}

std::vector<std::uint8_t> encode_tolerance_budget(const tolerance_budget_snapshot &snapshot) {
    canonical_writer writer;
    writer.u32(0x42504759U); writer.u16(precision_state_codec_v1); writer.u16(snapshot.schema_version);
    writer.floating(snapshot.tolerance); writer.u64(snapshot.lineages.size());
    for (const auto &lineage : snapshot.lineages) {
        writer.u64(lineage.lineage.ordinal()); writer.floating(lineage.committed_displacement);
    }
    writer.u64(snapshot.commits.size());
    for (const auto &commit : snapshot.commits) {
        writer.u64(commit.id.ordinal()); writer.u64(commit.proposal.ordinal());
        writer.u64(commit.actual_costs.size()); for (double value : commit.actual_costs) writer.floating(value);
        write_length_evidence(writer, commit.after_evidence); write_ids(writer, commit.ledger_entries);
        write_proposal(writer, commit.proposal_record);
        writer.u64(commit.cumulative_totals.size()); for (double value : commit.cumulative_totals) writer.floating(value);
    }
    writer.u64(snapshot.reservations.size());
    for (const auto &proposal : snapshot.reservations) write_proposal(writer, proposal);
    writer.floating(snapshot.global_realized_displacement); writer.u64(snapshot.active_reservations);
    return precision_codec_detail::seal(writer.take());
}

boolean_outcome<tolerance_budget_snapshot> decode_tolerance_budget(const std::vector<std::uint8_t> &bytes) {
    std::vector<std::uint8_t> payload;
    if (!precision_codec_detail::unseal(bytes, payload)) return boolean_outcome<tolerance_budget_snapshot>::failure(
        precision_codec_error(30034, "malformed tolerance budget encoding"));
    canonical_reader reader(payload); tolerance_budget_snapshot out; std::uint64_t count = 0;
    if (!read_state_header(reader, 0x42504759U, out.schema_version) || !reader.floating(out.tolerance) ||
        !reader.u64(count) || count > maximum_records)
        return boolean_outcome<tolerance_budget_snapshot>::failure(
            precision_codec_error(30034, "malformed tolerance budget encoding"));
    for (std::uint64_t i = 0, lineage = 0; i < count; ++i) {
        double total = 0.0; if (!reader.u64(lineage) || !reader.floating(total))
            return boolean_outcome<tolerance_budget_snapshot>::failure(
                precision_codec_error(30034, "malformed tolerance budget encoding"));
        out.lineages.push_back({geometric_lineage_id(lineage), total});
    }
    if (!reader.u64(count) || count > maximum_records) return boolean_outcome<tolerance_budget_snapshot>::failure(
        precision_codec_error(30034, "malformed tolerance budget encoding"));
    for (std::uint64_t i = 0; i < count; ++i) {
        tolerance_budget_commit commit; std::uint64_t id = 0, proposal = 0, costs = 0;
        if (!reader.u64(id) || !reader.u64(proposal) || !reader.u64(costs) || costs > maximum_records) return boolean_outcome<tolerance_budget_snapshot>::failure(
            precision_codec_error(30034, "malformed tolerance budget encoding"));
        commit.id = budget_commit_id(id); commit.proposal = budget_proposal_id(proposal);
        for (std::uint64_t j = 0; j < costs; ++j) { double value = 0.0; if (!reader.floating(value)) return boolean_outcome<tolerance_budget_snapshot>::failure(precision_codec_error(30034, "malformed tolerance budget encoding")); commit.actual_costs.push_back(value); }
        if (!read_length_evidence(reader, commit.after_evidence) ||
            !read_ids(reader, commit.ledger_entries) || !read_proposal(reader, commit.proposal_record) ||
            !reader.u64(costs) || costs > maximum_records) return boolean_outcome<tolerance_budget_snapshot>::failure(
                precision_codec_error(30034, "malformed tolerance budget encoding"));
        for (std::uint64_t j = 0; j < costs; ++j) { double value = 0.0; if (!reader.floating(value)) return boolean_outcome<tolerance_budget_snapshot>::failure(precision_codec_error(30034, "malformed tolerance budget encoding")); commit.cumulative_totals.push_back(value); }
        out.commits.push_back(std::move(commit));
    }
    if (!reader.u64(count) || count > maximum_records) return boolean_outcome<tolerance_budget_snapshot>::failure(precision_codec_error(30034, "malformed tolerance budget encoding"));
    for (std::uint64_t i = 0; i < count; ++i) { tolerance_budget_proposal proposal; if (!read_proposal(reader, proposal)) return boolean_outcome<tolerance_budget_snapshot>::failure(precision_codec_error(30034, "malformed tolerance budget encoding")); out.reservations.push_back(std::move(proposal)); }
    std::uint64_t active = 0;
    if (!reader.floating(out.global_realized_displacement) || !reader.u64(active) ||
        active != out.reservations.size() || !reader.complete()) return boolean_outcome<tolerance_budget_snapshot>::failure(precision_codec_error(30034, "malformed tolerance budget encoding"));
    out.active_reservations = static_cast<std::size_t>(active);
    return boolean_outcome<tolerance_budget_snapshot>::success(std::move(out));
}

std::vector<std::uint8_t> encode_precision_import(const foreign_precision_provenance &record) {
    canonical_writer writer;
    writer.u32(0x49504759U); writer.u16(precision_state_codec_v1); writer.u16(record.schema_version);
    precision_codec_detail::write_owner_marker(writer, record.owner);
    writer.floating(record.prior_output_precision); writer.floating(record.inherited_precision);
    writer.floating(record.construction_uncertainty); writer.floating(record.cumulative_cleanup_displacement);
    writer.floating(record.serialization_contribution); writer.sized_bytes(record.construction_history_digest);
    writer.sized_bytes(record.prior_context_digest); writer.sized_bytes(record.replay_lineage);
    writer.sized_bytes(record.publication_digest); writer.sized_bytes(record.verified_publication_digest);
    writer.sized_bytes(record.verification_evidence);
    writer.boolean(record.publication_verified); return precision_codec_detail::seal(writer.take());
}

namespace {
boolean_outcome<foreign_precision_provenance> decode_precision_import_with_owner(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner) {
    std::vector<std::uint8_t> payload;
    if (!precision_codec_detail::unseal(bytes, payload)) return boolean_outcome<foreign_precision_provenance>::failure(
        precision_codec_error(30035, "malformed precision import encoding"));
    canonical_reader reader(payload); foreign_precision_provenance out;
    if (!read_state_header(reader, 0x49504759U, out.schema_version) ||
        !precision_codec_detail::read_owner_marker(reader, owner, out.owner) ||
        !reader.floating(out.prior_output_precision) || !reader.floating(out.inherited_precision) ||
        !reader.floating(out.construction_uncertainty) || !reader.floating(out.cumulative_cleanup_displacement) ||
        !reader.floating(out.serialization_contribution) || !read_sized(reader, out.construction_history_digest) ||
        !read_sized(reader, out.prior_context_digest) || !read_sized(reader, out.replay_lineage) ||
        !read_sized(reader, out.publication_digest) || !read_sized(reader, out.verified_publication_digest) ||
        !read_sized(reader, out.verification_evidence) ||
        !reader.boolean(out.publication_verified) || !reader.complete())
        return boolean_outcome<foreign_precision_provenance>::failure(
            precision_codec_error(30035, "malformed precision import encoding"));
    return boolean_outcome<foreign_precision_provenance>::success(std::move(out));
}
}

boolean_outcome<foreign_precision_provenance> decode_precision_import(const std::vector<std::uint8_t> &bytes) {
    return decode_precision_import_with_owner(bytes, {});
}

boolean_outcome<foreign_precision_provenance> decode_precision_import(
    const std::vector<std::uint8_t> &bytes, const context_owner_token &owner) {
    return decode_precision_import_with_owner(bytes, owner);
}

#define YGOR_INSTANTIATE_PRECISION_CODEC(T) \
    template std::vector<std::uint8_t> encode_precision_preflight(const precision_preflight<T> &); \
    template boolean_outcome<precision_preflight<T>> decode_precision_preflight( \
        const std::vector<std::uint8_t> &); \
    template std::vector<std::uint8_t> encode_precision_context(const precision_context<T> &); \
    template boolean_outcome<decoded_precision_context<T>> decode_precision_context( \
        const std::vector<std::uint8_t> &); \
    template std::vector<std::uint8_t> encode_bounded_value(const bounded_scalar<T> &); \
    template boolean_outcome<bounded_scalar<T>> decode_bounded_value( \
        const std::vector<std::uint8_t> &, const context_owner_token &); \
    template std::vector<std::uint8_t> encode_directed_result(const directed_operation_result<T> &); \
    template boolean_outcome<directed_operation_result<T>> decode_directed_result( \
        const std::vector<std::uint8_t> &); \
    template std::vector<std::uint8_t> encode_predicate_result(const predicate_result<T> &); \
    template boolean_outcome<predicate_result<T>> decode_predicate_result( \
        const std::vector<std::uint8_t> &, const context_owner_token &); \
    template std::vector<std::uint8_t> encode_construction(const construction_conditioning<T> &); \
    template boolean_outcome<construction_conditioning<T>> decode_construction( \
        const std::vector<std::uint8_t> &, const context_owner_token &); \
    template std::vector<std::uint8_t> encode_conservative_bound(const conservative_bound_record<T> &); \
    template boolean_outcome<conservative_bound_record<T>> decode_conservative_bound( \
        const std::vector<std::uint8_t> &, const context_owner_token &)

YGOR_INSTANTIATE_PRECISION_CODEC(float);
YGOR_INSTANTIATE_PRECISION_CODEC(double);
#undef YGOR_INSTANTIATE_PRECISION_CODEC
}
