#include "StrictFloatingBuild.h"
#include "RelationCodec.h"
#include "CoplanarRelationOverlay.h"

namespace ygor::mesh_boolean::bounded {
namespace {

void encode_digest(canonical_writer &writer,
                   const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

void encode_truth(canonical_writer &writer,
                  const relation_truth_record &record) {
  writer.u64(record.rounded_nominal_bits);
  writer.u8(static_cast<std::uint8_t>(record.bounded_sign));
  writer.u8(static_cast<std::uint8_t>(record.exact_relation));
  writer.u8(static_cast<std::uint8_t>(record.disposition));
  writer.u16(record.rounded_formula);
  writer.u16(record.exact_formula);
  writer.u32(record.reserved);
}


void encode_eligibility(canonical_writer &writer,
                        const symbolic_eligibility_record &record) {
  encode_relation_request_key(writer, record.request);
  writer.u8(static_cast<std::uint8_t>(record.exact_relation));
  writer.u8(static_cast<std::uint8_t>(record.reason));
  writer.u16(record.evidence_formula_version);
  writer.boolean(record.exact_lineage_tie);
  writer.boolean(record.representational_tie_evidence);
  writer.boolean(record.structural_category_eligible);
  writer.boolean(record.tolerance_compatible);
  writer.boolean(record.rounded_nominal_zero);
  writer.boolean(record.inherited_uncertainty);
  writer.boolean(record.separated_realizations_possible);
  writer.boolean(record.owner_is_original_source_feature);
  writer.u8(record.reserved8);
  writer.u32(record.reserved);
}

void encode_symbolic(canonical_writer &writer,
                     const symbolic_relation_decision_record &record) {
  writer.u64(record.id.ordinal());
  encode_relation_request_key(writer, record.request);
  writer.u8(static_cast<std::uint8_t>(record.operation));
  writer.u8(static_cast<std::uint8_t>(record.acting_operand));
  writer.u8(static_cast<std::uint8_t>(record.matrix_family));
  writer.u8(static_cast<std::uint8_t>(record.orientation));
  writer.u64(record.stable_rule_ordinal);
  writer.u8(record.feature_priority);
  writer.u8(static_cast<std::uint8_t>(record.half_open_owner));
  writer.u8(static_cast<std::uint8_t>(record.symbolic_crossing_contribution));
  writer.u8(static_cast<std::uint8_t>(record.coincident_owner_rank));
  writer.u8(static_cast<std::uint8_t>(record.conceptual_side));
  writer.boolean(record.occurrence_separation_required);
  writer.boolean(record.nominal_geometry_unchanged);
  writer.u32(record.reserved);
}

} // namespace

template <class T, class I>
std::vector<std::uint8_t>
encode_signed_feature_relations(const signed_feature_relations<T, I> &artifact) {
  canonical_writer writer;
  writer.u32(0x37465259U); // YRF7
  writer.u16(artifact.schema_version_);
  writer.u16(artifact.provider_version_);
  writer.u16(artifact.graph_policy_version_);
  writer.u16(artifact.truth_policy_version_);
  writer.u16(artifact.codec_version_);
  writer.u16(artifact.verifier_version_);
  writer.u8(static_cast<std::uint8_t>(artifact.provider_));
  writer.u8(static_cast<std::uint8_t>(artifact.verification_));
  encode_digest(writer, artifact.context_digest_);
  encode_digest(writer, artifact.precision_digest_);
  encode_digest(writer, artifact.candidate_digest_);
  encode_digest(writer, artifact.graph_digest_);
  writer.u8(static_cast<std::uint8_t>(artifact.operation_));
  writer.floating(artifact.residual_boundary_);
  encode_digest(writer, artifact.symbolic_policy_digest_);
  writer.boolean(static_cast<bool>(artifact.source_edge_stage_));
  if (artifact.source_edge_stage_)
    writer.sized_bytes(encode_candidate_source_edge_relation_semantics(
        *artifact.source_edge_stage_));
  writer.boolean(static_cast<bool>(artifact.source_edge_facet_stage_));
  if (artifact.source_edge_facet_stage_)
    writer.sized_bytes(encode_candidate_source_edge_facet_relation_semantics(
        *artifact.source_edge_facet_stage_));
  writer.boolean(static_cast<bool>(artifact.source_facet_stage_));
  if (artifact.source_facet_stage_)
    writer.sized_bytes(encode_candidate_source_facet_relation_semantics(
        *artifact.source_facet_stage_));
  writer.boolean(static_cast<bool>(artifact.coplanar_overlay_stage_));
  if (artifact.coplanar_overlay_stage_)
    writer.sized_bytes(encode_candidate_coplanar_overlay_semantics(
        *artifact.coplanar_overlay_stage_));
  const auto graph_bytes =
      encode_relation_request_graph_semantics(artifact.request_graph_);
  writer.sized_bytes(graph_bytes);
  writer.u64(artifact.truth_records_.size());
  for (const auto &record : artifact.truth_records_)
    encode_truth(writer, record);
  writer.u64(artifact.relations_.size());
  for (const auto &record : artifact.relations_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.family));
    writer.u8(static_cast<std::uint8_t>(record.scope));
    writer.u8(static_cast<std::uint8_t>(record.status));
    writer.u64(record.truth_begin);
    writer.u64(record.truth_count);
    writer.u32(static_cast<std::uint32_t>(record.numeric_crossing_multiplicity));
    writer.u32(record.occurrence);
    writer.u32(record.reserved);
  }
  writer.u64(artifact.constructions_.size());
  for (const auto &record : artifact.constructions_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.kind));
    writer.u8(record.component_count);
    for (const auto bits : record.nominal_bits) writer.u64(bits);
    for (const auto bits : record.lower_bits) writer.u64(bits);
    for (const auto bits : record.upper_bits) writer.u64(bits);
    writer.u64(record.residual_truth_begin);
    writer.u64(record.residual_truth_count);
    writer.boolean(record.finite);
    writer.boolean(record.tolerance_compatible);
    writer.u32(record.reserved);
  }
  writer.u64(artifact.symbolic_eligibility_.size());
  for (const auto &record : artifact.symbolic_eligibility_)
    encode_eligibility(writer, record);
  writer.u64(artifact.symbolic_decisions_.size());
  for (const auto &record : artifact.symbolic_decisions_)
    encode_symbolic(writer, record);
  writer.u64(artifact.crossings_.size());
  for (const auto &record : artifact.crossings_) {
    writer.u64(record.relation.ordinal());
    writer.u32(static_cast<std::uint32_t>(record.numeric_crossing));
    writer.u8(static_cast<std::uint8_t>(record.symbolic_crossing));
    writer.u8(static_cast<std::uint8_t>(record.half_open_owner));
    writer.u32(record.occurrence);
    writer.u64(record.source_fan_group);
    writer.u32(record.source_fan_group_size);
    writer.u32(record.source_fan_group_ordinal);
    writer.u8(static_cast<std::uint8_t>(record.local_transition));
    writer.boolean(record.numeric_owner);
    writer.boolean(record.source_fan_resolved);
    writer.boolean(record.locally_conservative);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  writer.u64(artifact.event_seeds_.size());
  for (const auto &record : artifact.event_seeds_) {
    writer.u64(record.id.ordinal());
    encode_relation_event_seed_key(writer, record.key);
    writer.u64(record.source_relation.ordinal());
    writer.u64(record.construction.ordinal());
    writer.u64(record.incidence_begin);
    writer.u64(record.incidence_count);
    writer.boolean(record.distinct_occurrence_required);
    writer.u32(record.reserved);
  }
  writer.u64(artifact.event_seed_incidence_.size());
  for (const auto &feature : artifact.event_seed_incidence_)
    encode_relation_feature_key(writer, feature);
  writer.u64(artifact.candidate_dispositions_.size());
  for (const auto &record : artifact.candidate_dispositions_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.candidate.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.disposition));
    writer.u64(record.public_relation.ordinal());
    writer.u64(record.bookkeeping_request.ordinal());
    writer.u32(record.reserved);
  }
  writer.u64(artifact.statistics_.candidate_count);
  writer.u64(artifact.statistics_.request_proposal_count);
  writer.u64(artifact.statistics_.unique_request_count);
  writer.u64(artifact.statistics_.dependency_count);
  writer.u64(artifact.statistics_.reverse_consumer_count);
  writer.u64(artifact.statistics_.candidate_witness_count);
  writer.u64(artifact.statistics_.public_relation_count);
  writer.u64(artifact.statistics_.bookkeeping_relation_count);
  writer.u64(artifact.statistics_.construction_count);
  writer.u64(artifact.statistics_.symbolic_eligibility_count);
  writer.u64(artifact.statistics_.symbolic_decision_count);
  writer.u64(artifact.statistics_.crossing_record_count);
  writer.u64(artifact.statistics_.event_seed_count);
  writer.u64(artifact.statistics_.sort_comparisons);
  writer.u64(artifact.statistics_.verifier_work_units);
  writer.u64(artifact.statistics_.persistent_bytes);
  writer.u64(artifact.verification_evidence_.id.ordinal());
  writer.u16(artifact.verification_evidence_.verifier_version);
  writer.boolean(artifact.verification_evidence_.graph_reconstructed);
  writer.boolean(artifact.verification_evidence_.owner_exclusion_checked);
  writer.boolean(artifact.verification_evidence_.selection_boundary_checked);
  writer.boolean(
      artifact.verification_evidence_.candidate_dispositions_complete);
  writer.u64(artifact.verification_evidence_.verifier_work_units);
  encode_digest(writer, artifact.verification_evidence_.semantic_digest);
  writer.u32(artifact.verification_evidence_.reserved);
  return writer.take();
}


namespace relation_codec_detail {

inline bool read_digest(canonical_reader &reader,
                        bounded_boolean_digest &digest) {
  for (auto &byte : digest.bytes)
    if (!reader.u8(byte))
      return false;
  return true;
}

inline bool read_feature_key(canonical_reader &reader) {
  std::uint8_t operand = 0, kind = 0;
  std::uint64_t primary = 0, secondary = 0;
  std::uint32_t occurrence = 0;
  std::uint16_t schema = 0;
  return reader.u8(operand) && reader.u8(kind) && reader.u64(primary) &&
         reader.u64(secondary) && reader.u32(occurrence) &&
         reader.u16(schema);
}

inline bool read_request_key(canonical_reader &reader) {
  bounded_boolean_digest semantic_namespace{};
  std::uint8_t family = 0, scope = 0;
  std::uint64_t directed_use = 0;
  std::uint16_t formula = 0, policy = 0;
  std::uint32_t occurrence = 0, reserved = 0;
  return read_digest(reader, semantic_namespace) && reader.u8(family) &&
         reader.u8(scope) && read_feature_key(reader) &&
         read_feature_key(reader) && reader.u64(directed_use) &&
         reader.u16(formula) && reader.u16(policy) &&
         reader.u32(occurrence) && reader.u32(reserved);
}

inline bool read_event_seed_key(canonical_reader &reader) {
  bounded_boolean_digest semantic_namespace{};
  std::uint8_t family = 0;
  std::uint32_t occurrence = 0;
  std::uint16_t policy = 0, reserved = 0;
  return read_digest(reader, semantic_namespace) && reader.u8(family) &&
         read_feature_key(reader) && read_feature_key(reader) &&
         reader.u32(occurrence) && reader.u16(policy) &&
         reader.u16(reserved);
}

inline bool count_fits(canonical_reader &reader, std::uint64_t count,
                       std::uint64_t maximum,
                       std::uint64_t minimum_record_bytes) noexcept {
  return count <= maximum &&
         (minimum_record_bytes == 0 ||
          count <= static_cast<std::uint64_t>(reader.remaining()) /
                       minimum_record_bytes);
}

inline bool read_truth_record(canonical_reader &reader) {
  std::uint64_t nominal = 0;
  std::uint8_t bounded = 0, exact = 0, disposition = 0;
  std::uint16_t rounded_formula = 0, exact_formula = 0;
  std::uint32_t reserved = 0;
  return reader.u64(nominal) && reader.u8(bounded) && reader.u8(exact) &&
         reader.u8(disposition) && reader.u16(rounded_formula) &&
         reader.u16(exact_formula) && reader.u32(reserved);
}

inline bool read_relation_record(canonical_reader &reader) {
  std::uint64_t id = 0, producer = 0, truth_begin = 0, truth_count = 0;
  std::uint8_t family = 0, scope = 0, status = 0;
  std::uint32_t crossing = 0, occurrence = 0, reserved = 0;
  return reader.u64(id) && reader.u64(producer) && reader.u8(family) &&
         reader.u8(scope) && reader.u8(status) && reader.u64(truth_begin) &&
         reader.u64(truth_count) && reader.u32(crossing) &&
         reader.u32(occurrence) && reader.u32(reserved);
}

inline bool read_construction_record(canonical_reader &reader) {
  std::uint64_t id = 0, producer = 0, value = 0;
  std::uint8_t kind = 0, component_count = 0;
  std::uint64_t residual_begin = 0, residual_count = 0;
  bool finite = false, tolerance = false;
  std::uint32_t reserved = 0;
  if (!reader.u64(id) || !reader.u64(producer) || !reader.u8(kind) ||
      !reader.u8(component_count))
    return false;
  for (std::size_t i = 0; i < 18; ++i)
    if (!reader.u64(value))
      return false;
  return reader.u64(residual_begin) && reader.u64(residual_count) &&
         reader.boolean(finite) && reader.boolean(tolerance) &&
         reader.u32(reserved);
}

inline bool read_eligibility_record(canonical_reader &reader) {
  std::uint8_t exact = 0, reason = 0, reserved8 = 0;
  std::uint16_t formula = 0;
  bool flag = false;
  std::uint32_t reserved = 0;
  if (!read_request_key(reader) || !reader.u8(exact) ||
      !reader.u8(reason) || !reader.u16(formula))
    return false;
  for (std::size_t i = 0; i < 8; ++i)
    if (!reader.boolean(flag))
      return false;
  return reader.u8(reserved8) && reader.u32(reserved);
}

inline bool read_symbolic_record(canonical_reader &reader) {
  std::uint64_t id = 0, rule = 0;
  std::uint8_t value = 0;
  bool flag = false;
  std::uint32_t reserved = 0;
  if (!reader.u64(id) || !read_request_key(reader))
    return false;
  for (std::size_t i = 0; i < 4; ++i)
    if (!reader.u8(value))
      return false;
  if (!reader.u64(rule))
    return false;
  for (std::size_t i = 0; i < 5; ++i)
    if (!reader.u8(value))
      return false;
  return reader.boolean(flag) && reader.boolean(flag) &&
         reader.u32(reserved);
}

inline bool read_crossing_record(canonical_reader &reader) {
  std::uint64_t relation = 0, group = 0;
  std::uint32_t numeric = 0, occurrence = 0, group_size = 0,
                group_ordinal = 0, reserved32 = 0;
  std::uint8_t symbolic = 0, owner = 0, local_transition = 0;
  bool numeric_owner = false, resolved = false, conservative = false;
  std::uint16_t reserved16 = 0;
  return reader.u64(relation) && reader.u32(numeric) &&
         reader.u8(symbolic) && reader.u8(owner) &&
         reader.u32(occurrence) && reader.u64(group) &&
         reader.u32(group_size) && reader.u32(group_ordinal) &&
         reader.u8(local_transition) && reader.boolean(numeric_owner) &&
         reader.boolean(resolved) && reader.boolean(conservative) &&
         reader.u16(reserved16) && reader.u32(reserved32);
}

inline bool read_event_seed_record(canonical_reader &reader) {
  std::uint64_t value = 0;
  bool distinct = false;
  std::uint32_t reserved = 0;
  return reader.u64(value) && read_event_seed_key(reader) &&
         reader.u64(value) && reader.u64(value) && reader.u64(value) &&
         reader.u64(value) && reader.boolean(distinct) &&
         reader.u32(reserved);
}

inline bool read_candidate_disposition_record(canonical_reader &reader) {
  std::uint64_t value = 0;
  std::uint8_t disposition = 0;
  std::uint32_t reserved = 0;
  return reader.u64(value) && reader.u64(value) &&
         reader.u8(disposition) && reader.u64(value) &&
         reader.u64(value) && reader.u32(reserved);
}

} // namespace relation_codec_detail

template <class T>
bool parse_relation_artifact_envelope(
    const std::vector<std::uint8_t> &bytes,
    const relation_capabilities &capabilities,
    relation_artifact_envelope<T> &envelope,
    bounded_boolean_error &error) {
  using namespace relation_codec_detail;
  envelope = relation_artifact_envelope<T>{};
  const auto codec_failure = [&](relation_subcode subcode,
                                 bounded_boolean_error_category category,
                                 const char *summary) {
    error = relation_error(subcode, category, summary,
                           relation_checkpoint::canonical_encoding);
    return false;
  };
  if (bytes.size() > capabilities.maximum_canonical_bytes)
    return codec_failure(relation_subcode::resource_preflight,
                         bounded_boolean_error_category::resource_limit,
                         "Component 07 encoded artifact exceeds configured limit");

  canonical_reader reader(bytes);
  std::uint32_t magic = 0;
  std::uint8_t provider = 0, verification = 0, operation = 0;
  if (!reader.u32(magic) || magic != 0x37465259U ||
      !reader.u16(envelope.schema_version) ||
      !reader.u16(envelope.provider_version) ||
      !reader.u16(envelope.graph_policy_version) ||
      !reader.u16(envelope.truth_policy_version) ||
      !reader.u16(envelope.codec_version) ||
      !reader.u16(envelope.verifier_version) || !reader.u8(provider) ||
      !reader.u8(verification) ||
      !read_digest(reader, envelope.context_digest) ||
      !read_digest(reader, envelope.precision_digest) ||
      !read_digest(reader, envelope.candidate_digest) ||
      !read_digest(reader, envelope.graph_digest) || reader.u8(operation) == false ||
      !reader.floating(envelope.residual_boundary) ||
      !read_digest(reader, envelope.symbolic_policy_digest))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 encoded artifact header is malformed");

  envelope.provider = static_cast<relation_provider_kind>(provider);
  envelope.verification =
      static_cast<relation_verification_disposition>(verification);
  envelope.operation = static_cast<boolean_operation>(operation);
  if (envelope.schema_version != contract_versions::relation_artifact_schema ||
      envelope.provider_version != contract_versions::relation_provider ||
      envelope.graph_policy_version !=
          contract_versions::relation_graph_policy ||
      envelope.truth_policy_version !=
          contract_versions::relation_truth_policy ||
      envelope.codec_version != contract_versions::relation_codec ||
      envelope.verifier_version != contract_versions::relation_verifier ||
      envelope.provider !=
          relation_provider_kind::canonical_source_feature_relation_graph_v1 ||
      envelope.verification !=
          relation_verification_disposition::independently_verified)
    return codec_failure(relation_subcode::unsupported_version,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 encoded artifact uses an unsupported contract");

  canonical_writer detailed_writer;
  for (std::size_t stage = 0; stage < envelope.detailed_stage_present.size();
       ++stage) {
    bool present = false;
    std::vector<std::uint8_t> section;
    if (!reader.boolean(present) ||
        (present && !reader.sized_bytes(section,
                                        capabilities.maximum_canonical_bytes)))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 detailed-stage section is malformed");
    envelope.detailed_stage_present[stage] = present;
    detailed_writer.boolean(present);
    if (present)
      detailed_writer.sized_bytes(section);
  }
  envelope.detailed_stage_digest = sha256::digest(detailed_writer.take());

  std::vector<std::uint8_t> graph_section;
  if (!reader.sized_bytes(graph_section, capabilities.maximum_canonical_bytes))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 request-graph section is malformed");
  envelope.graph_section_digest = sha256::digest(graph_section);

  if (!reader.u64(envelope.truth_count) ||
      !count_fits(reader, envelope.truth_count,
                  capabilities.maximum_dependencies, 19))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 truth table count is malformed");
  for (std::uint64_t i = 0; i < envelope.truth_count; ++i)
    if (!read_truth_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 truth table is truncated");

  if (!reader.u64(envelope.relation_count) ||
      !count_fits(reader, envelope.relation_count,
                  capabilities.maximum_relations, 47))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 relation table count is malformed");
  for (std::uint64_t i = 0; i < envelope.relation_count; ++i)
    if (!read_relation_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 relation table is truncated");

  if (!reader.u64(envelope.construction_count) ||
      !count_fits(reader, envelope.construction_count,
                  capabilities.maximum_constructions, 184))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 construction table count is malformed");
  for (std::uint64_t i = 0; i < envelope.construction_count; ++i)
    if (!read_construction_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 construction table is truncated");

  if (!reader.u64(envelope.symbolic_eligibility_count) ||
      !count_fits(reader, envelope.symbolic_eligibility_count,
                  capabilities.maximum_symbolic_decisions, 117))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 symbolic eligibility count is malformed");
  for (std::uint64_t i = 0; i < envelope.symbolic_eligibility_count; ++i)
    if (!read_eligibility_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 symbolic eligibility table is truncated");

  if (!reader.u64(envelope.symbolic_decision_count) ||
      !count_fits(reader, envelope.symbolic_decision_count,
                  capabilities.maximum_symbolic_decisions, 131))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 symbolic decision count is malformed");
  for (std::uint64_t i = 0; i < envelope.symbolic_decision_count; ++i)
    if (!read_symbolic_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 symbolic decision table is truncated");

  if (!reader.u64(envelope.crossing_count) ||
      !count_fits(reader, envelope.crossing_count,
                  capabilities.maximum_relations, 44))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 crossing table count is malformed");
  for (std::uint64_t i = 0; i < envelope.crossing_count; ++i)
    if (!read_crossing_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 crossing table is truncated");

  if (!reader.u64(envelope.event_seed_count) ||
      !count_fits(reader, envelope.event_seed_count,
                  capabilities.maximum_event_seeds, 134))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 event-seed count is malformed");
  for (std::uint64_t i = 0; i < envelope.event_seed_count; ++i)
    if (!read_event_seed_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 event-seed table is truncated");

  if (!reader.u64(envelope.incidence_count) ||
      !count_fits(reader, envelope.incidence_count,
                  capabilities.maximum_consumers, 24))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 event incidence count is malformed");
  for (std::uint64_t i = 0; i < envelope.incidence_count; ++i)
    if (!read_feature_key(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 event incidence table is truncated");

  if (!reader.u64(envelope.candidate_disposition_count) ||
      !count_fits(reader, envelope.candidate_disposition_count,
                  capabilities.maximum_relations, 37))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 candidate disposition count is malformed");
  for (std::uint64_t i = 0; i < envelope.candidate_disposition_count; ++i)
    if (!read_candidate_disposition_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 candidate disposition table is truncated");

  auto &statistics = envelope.statistics;
  if (!reader.u64(statistics.candidate_count) ||
      !reader.u64(statistics.request_proposal_count) ||
      !reader.u64(statistics.unique_request_count) ||
      !reader.u64(statistics.dependency_count) ||
      !reader.u64(statistics.reverse_consumer_count) ||
      !reader.u64(statistics.candidate_witness_count) ||
      !reader.u64(statistics.public_relation_count) ||
      !reader.u64(statistics.bookkeeping_relation_count) ||
      !reader.u64(statistics.construction_count) ||
      !reader.u64(statistics.symbolic_eligibility_count) ||
      !reader.u64(statistics.symbolic_decision_count) ||
      !reader.u64(statistics.crossing_record_count) ||
      !reader.u64(statistics.event_seed_count) ||
      !reader.u64(statistics.sort_comparisons) ||
      !reader.u64(statistics.verifier_work_units) ||
      !reader.u64(statistics.persistent_bytes))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 statistics section is truncated");

  std::uint64_t evidence_id = 0, evidence_work = 0;
  std::uint16_t evidence_version = 0;
  bool evidence_flag = false;
  bounded_boolean_digest evidence_digest{};
  std::uint32_t evidence_reserved = 0;
  if (!reader.u64(evidence_id) || !reader.u16(evidence_version))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 verifier evidence is truncated");
  for (std::size_t i = 0; i < 4; ++i)
    if (!reader.boolean(evidence_flag))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 verifier evidence flags are malformed");
  if (!reader.u64(evidence_work) || !read_digest(reader, evidence_digest) ||
      !reader.u32(evidence_reserved) || !reader.complete())
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 encoded artifact has malformed or trailing data");

  if (statistics.candidate_count != envelope.candidate_disposition_count ||
      statistics.public_relation_count + statistics.bookkeeping_relation_count !=
          envelope.relation_count ||
      statistics.construction_count != envelope.construction_count ||
      statistics.symbolic_eligibility_count !=
          envelope.symbolic_eligibility_count ||
      statistics.symbolic_decision_count != envelope.symbolic_decision_count ||
      statistics.crossing_record_count != envelope.crossing_count ||
      statistics.event_seed_count != envelope.event_seed_count ||
      evidence_id != 0 ||
      evidence_version != contract_versions::relation_verifier ||
      evidence_reserved != 0)
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 encoded artifact counts or evidence are inconsistent");
  return true;
}

template <class T, class I>
bool verify_relation_codec(const signed_feature_relations<T, I> &artifact,
                           bounded_boolean_error &error) {
  const auto encoded = encode_signed_feature_relations(artifact);
  if (encoded != artifact.canonical_bytes_ ||
      sha256::digest(encoded) != artifact.digest_) {
    error = relation_error(relation_subcode::digest_mismatch,
                           bounded_boolean_error_category::internal_invariant_error,
                           "Component 07 canonical bytes or digest mismatch",
                           relation_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

template std::vector<std::uint8_t>
encode_signed_feature_relations<float, std::uint32_t>(
    const signed_feature_relations<float, std::uint32_t> &);
template std::vector<std::uint8_t>
encode_signed_feature_relations<float, std::uint64_t>(
    const signed_feature_relations<float, std::uint64_t> &);
template std::vector<std::uint8_t>
encode_signed_feature_relations<double, std::uint32_t>(
    const signed_feature_relations<double, std::uint32_t> &);
template std::vector<std::uint8_t>
encode_signed_feature_relations<double, std::uint64_t>(
    const signed_feature_relations<double, std::uint64_t> &);

template bool verify_relation_codec<float, std::uint32_t>(
    const signed_feature_relations<float, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_relation_codec<float, std::uint64_t>(
    const signed_feature_relations<float, std::uint64_t> &,
    bounded_boolean_error &);
template bool verify_relation_codec<double, std::uint32_t>(
    const signed_feature_relations<double, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_relation_codec<double, std::uint64_t>(
    const signed_feature_relations<double, std::uint64_t> &,
    bounded_boolean_error &);

template bool parse_relation_artifact_envelope<float>(
    const std::vector<std::uint8_t> &, const relation_capabilities &,
    relation_artifact_envelope<float> &, bounded_boolean_error &);
template bool parse_relation_artifact_envelope<double>(
    const std::vector<std::uint8_t> &, const relation_capabilities &,
    relation_artifact_envelope<double> &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
