#include "StrictFloatingBuild.h"
#include "RelationCodec.h"

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
  writer.u64(artifact.symbolic_decisions_.size());
  for (const auto &record : artifact.symbolic_decisions_)
    encode_symbolic(writer, record);
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
  writer.u64(artifact.statistics_.symbolic_decision_count);
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

} // namespace ygor::mesh_boolean::bounded
