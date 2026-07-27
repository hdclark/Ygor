#include "StrictFloatingBuild.h"
#include "RelationReplay.h"

#include "RelationCodec.h"
#include "CandidateSourceEdgeRelations.h"
#include "EdgeFacetRelations.h"
#include "FacetFacetRelations.h"
#include "CoplanarRelationOverlay.h"

#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

void encode_digest(canonical_writer &writer,
                   const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

void encode_feature(canonical_writer &writer,
                    const relation_feature_key &feature) {
  encode_relation_feature_key(writer, feature);
}

void encode_diagnostic_without_digest(
    canonical_writer &writer, const relation_diagnostic_record &record) {
  writer.u64(record.id.ordinal());
  writer.u8(static_cast<std::uint8_t>(record.kind));
  writer.u8(static_cast<std::uint8_t>(record.severity));
  writer.u32(static_cast<std::uint32_t>(record.checkpoint));
  writer.u32(record.subcode);
  writer.boolean(record.has_candidate);
  writer.boolean(record.has_relation);
  writer.boolean(record.has_source_features);
  writer.boolean(record.has_numeric_evidence);
  writer.u64(record.candidate_ordinal);
  writer.u64(record.relation_ordinal);
  encode_feature(writer, record.first_feature);
  encode_feature(writer, record.second_feature);
  writer.u64(record.rounded_nominal_bits);
  writer.u64(record.lower_bits);
  writer.u64(record.upper_bits);
  writer.u64(record.margin_bits);
  writer.u8(static_cast<std::uint8_t>(record.bounded_sign));
  writer.u8(static_cast<std::uint8_t>(record.exact_relation));
  writer.u8(static_cast<std::uint8_t>(record.disposition));
  writer.u16(record.rounded_formula);
  writer.u16(record.exact_formula);
  writer.u64(record.trace_root);
  writer.u8(static_cast<std::uint8_t>(record.resource));
  writer.u64(record.resource_limit);
  writer.u64(record.resource_used);
  writer.u64(record.cancellation_progress);
  writer.u64(record.replay_checkpoint.ordinal());
  writer.u16(record.schema_version);
  writer.u16(record.reserved16);
  writer.u32(record.reserved32);
}

bounded_boolean_digest diagnostic_record_digest(
    const relation_diagnostic_record &record) {
  canonical_writer writer;
  writer.u32(0x44375259U); // YR7D
  writer.u16(contract_versions::relation_diagnostic_schema);
  encode_diagnostic_without_digest(writer, record);
  return sha256::digest(writer.bytes());
}

void encode_checkpoint_without_digest(
    canonical_writer &writer,
    const relation_replay_checkpoint_record &record) {
  writer.u64(record.id.ordinal());
  writer.u32(static_cast<std::uint32_t>(record.checkpoint));
  writer.u8(static_cast<std::uint8_t>(record.status));
  writer.u64(record.input_count);
  writer.u64(record.output_count);
  writer.u64(record.cumulative_work_units);
  writer.u16(record.schema_version);
  writer.u16(record.reserved16);
  writer.u32(record.reserved32);
}

bounded_boolean_digest replay_evidence_digest(
    const relation_replay_evidence &evidence) {
  canonical_writer writer;
  writer.u32(0x45375259U); // YR7E
  writer.u16(evidence.schema_version);
  writer.u16(evidence.policy_version);
  encode_digest(writer, evidence.input_equivalence_digest);
  encode_digest(writer, evidence.checkpoint_digest);
  encode_digest(writer, evidence.diagnostic_digest);
  encode_digest(writer, evidence.base_artifact_digest);
  writer.u64(evidence.checkpoint_count);
  writer.u64(evidence.diagnostic_count);
  writer.boolean(evidence.complete);
  writer.boolean(evidence.artifact_reconstructed);
  writer.boolean(evidence.primary_failure_present);
  writer.u8(evidence.reserved8);
  writer.u32(evidence.reserved32);
  return sha256::digest(writer.bytes());
}

template <class T, class I>
bounded_boolean_digest replay_input_equivalence_digest(
    const signed_feature_relations<T, I> &artifact) {
  canonical_writer writer;
  writer.u32(0x49375259U); // YR7I
  writer.u16(contract_versions::relation_replay_policy);
  writer.u8(static_cast<std::uint8_t>(artifact.operation()));
  writer.floating(artifact.residual_boundary());
  encode_digest(writer, artifact.context_digest());
  encode_digest(writer, artifact.precision_digest());
  encode_digest(writer, artifact.candidate_digest());
  encode_digest(writer, artifact.symbolic_policy_digest());
  const std::array<bounded_boolean_digest, 4> stage_digests{{
      artifact.source_edge_stage() ? artifact.source_edge_stage()->semantic_digest
                                  : bounded_boolean_digest{},
      artifact.source_edge_facet_stage()
          ? artifact.source_edge_facet_stage()->semantic_digest
          : bounded_boolean_digest{},
      artifact.source_facet_stage()
          ? artifact.source_facet_stage()->semantic_digest
          : bounded_boolean_digest{},
      artifact.coplanar_overlay_stage()
          ? artifact.coplanar_overlay_stage()->semantic_digest
          : bounded_boolean_digest{},
  }};
  for (const auto &digest : stage_digests)
    encode_digest(writer, digest);
  return sha256::digest(writer.bytes());
}

struct checkpoint_spec final {
  relation_checkpoint checkpoint;
  std::uint64_t input_count;
  std::uint64_t output_count;
  std::uint64_t work_units;
};

template <class T, class I>
std::array<checkpoint_spec, 17> replay_checkpoint_specs(
    const signed_feature_relations<T, I> &artifact) {
  const std::uint64_t stage_edge_count = artifact.source_edge_stage()
      ? static_cast<std::uint64_t>(artifact.source_edge_stage()->relations.size())
      : 0;
  const std::uint64_t stage_edge_facet_count = artifact.source_edge_facet_stage()
      ? static_cast<std::uint64_t>(artifact.source_edge_facet_stage()->relations.size())
      : 0;
  const std::uint64_t stage_facet_count = artifact.source_facet_stage()
      ? static_cast<std::uint64_t>(artifact.source_facet_stage()->relations.size())
      : 0;
  const std::uint64_t stage_overlay_count = artifact.coplanar_overlay_stage()
      ? static_cast<std::uint64_t>(artifact.coplanar_overlay_stage()->overlays.size())
      : 0;
  const auto &statistics = artifact.statistics();
  std::array<checkpoint_spec, 17> out{};
  std::size_t n = 0;
  const auto add = [&](relation_checkpoint checkpoint, std::uint64_t inputs,
                       std::uint64_t outputs, std::uint64_t work) {
    out[n++] = checkpoint_spec{checkpoint, inputs, outputs, work};
  };
  add(relation_checkpoint::predecessor_validation, 4, 4, 4);
  add(relation_checkpoint::edge_edge_evaluation, statistics.candidate_count,
      stage_edge_count, stage_edge_count);
  add(relation_checkpoint::edge_facet_evaluation, statistics.candidate_count,
      stage_edge_facet_count, stage_edge_facet_count);
  add(relation_checkpoint::facet_facet_evaluation, statistics.candidate_count,
      stage_facet_count, stage_facet_count);
  add(relation_checkpoint::coplanar_overlay_evaluation, stage_facet_count,
      stage_overlay_count, stage_overlay_count);
  add(relation_checkpoint::graph_finalization,
      statistics.request_proposal_count, statistics.unique_request_count,
      statistics.unique_request_count + statistics.dependency_count);
  add(relation_checkpoint::truth_record_assembly,
      statistics.bounded_primitive_count + statistics.exact_relation_count,
      static_cast<std::uint64_t>(artifact.truth_records().size()),
      static_cast<std::uint64_t>(artifact.truth_records().size()));
  add(relation_checkpoint::source_facet_region_evaluation,
      statistics.interval_evidence_count, statistics.source_facet_region_count,
      statistics.interval_evidence_count + statistics.source_facet_region_count);
  add(relation_checkpoint::construction_validation,
      statistics.public_relation_count + statistics.bookkeeping_relation_count,
      statistics.construction_count,
      statistics.construction_count + statistics.construction_ledger_count);
  add(relation_checkpoint::crossing_multiplicity,
      statistics.public_relation_count, statistics.crossing_record_count,
      statistics.crossing_record_count);
  add(relation_checkpoint::symbolic_matrix_lookup,
      statistics.symbolic_eligibility_count, statistics.symbolic_decision_count,
      statistics.symbolic_eligibility_count + statistics.symbolic_decision_count);
  add(relation_checkpoint::event_seed_and_disposition_reconciliation,
      statistics.candidate_count, statistics.event_seed_count,
      statistics.event_seed_count + statistics.candidate_count);
  add(relation_checkpoint::canonical_id_and_reference_remap,
      statistics.unique_request_count, statistics.unique_request_count,
      statistics.unique_request_count);
  add(relation_checkpoint::downstream_selection_boundary_audit,
      statistics.symbolic_decision_count, 0,
      statistics.symbolic_decision_count);
  add(relation_checkpoint::producer_verification,
      statistics.unique_request_count, statistics.unique_request_count,
      statistics.verifier_work_units);
  add(relation_checkpoint::canonical_encoding, statistics.persistent_bytes,
      statistics.persistent_bytes, 1);
  add(relation_checkpoint::resource_reconciliation,
      statistics.persistent_bytes, statistics.persistent_bytes, 1);
  return out;
}

bounded_boolean_digest chained_checkpoint_digest(
    const bounded_boolean_digest &previous,
    const bounded_boolean_digest &base_artifact_digest,
    const relation_replay_checkpoint_record &record) {
  canonical_writer writer;
  writer.u32(0x43375259U); // YR7C
  writer.u16(contract_versions::relation_replay_checkpoint_schema);
  encode_digest(writer, previous);
  encode_digest(writer, base_artifact_digest);
  encode_checkpoint_without_digest(writer, record);
  return sha256::digest(writer.bytes());
}

template <class T, class I>
std::vector<relation_replay_checkpoint_record> populate_replay_checkpoints(
    const signed_feature_relations<T, I> &artifact,
    const bounded_boolean_digest &input_digest,
    const bounded_boolean_digest &base_digest) {
  const auto specs = replay_checkpoint_specs(artifact);
  std::vector<relation_replay_checkpoint_record> records;
  records.reserve(specs.size());
  auto previous = input_digest;
  std::uint64_t cumulative_work = 0;
  for (std::size_t index = 0; index < specs.size(); ++index) {
    relation_replay_checkpoint_record record;
    record.id = relation_replay_checkpoint_id(index);
    record.checkpoint = specs[index].checkpoint;
    record.status = relation_replay_checkpoint_status::completed;
    record.input_count = specs[index].input_count;
    record.output_count = specs[index].output_count;
    if (!checked_add(cumulative_work, specs[index].work_units,
                     cumulative_work))
      cumulative_work = std::numeric_limits<std::uint64_t>::max();
    record.cumulative_work_units = cumulative_work;
    record.semantic_digest =
        chained_checkpoint_digest(previous, base_digest, record);
    previous = record.semantic_digest;
    records.push_back(record);
  }
  return records;
}

template <class T, class I>
std::vector<relation_diagnostic_record> populate_diagnostics(
    const signed_feature_relations<T, I> &artifact) {
  std::vector<relation_diagnostic_record> records;
  records.reserve(4);
  const auto add = [&](relation_diagnostic_kind kind,
                       relation_checkpoint checkpoint,
                       relation_subcode subcode,
                       relation_replay_checkpoint_id replay_checkpoint,
                       resource_kind resource = resource_kind::diagnostic_findings,
                       std::uint64_t limit = 0, std::uint64_t used = 0) {
    relation_diagnostic_record record;
    record.id = relation_diagnostic_id(records.size());
    record.kind = kind;
    record.severity = relation_diagnostic_severity::retained_finding;
    record.checkpoint = checkpoint;
    record.subcode = static_cast<std::uint32_t>(subcode);
    record.resource = resource;
    record.resource_limit = limit;
    record.resource_used = used;
    record.replay_checkpoint = replay_checkpoint;
    record.semantic_digest = diagnostic_record_digest(record);
    records.push_back(record);
  };
  add(relation_diagnostic_kind::owner_exclusion_audit,
      relation_checkpoint::producer_verification,
      relation_subcode::owner_in_semantics, relation_replay_checkpoint_id(14));
  add(relation_diagnostic_kind::selection_boundary_audit,
      relation_checkpoint::downstream_selection_boundary_audit,
      relation_subcode::symbolic_selection_boundary,
      relation_replay_checkpoint_id(13));
  add(relation_diagnostic_kind::replay_completeness_audit,
      relation_checkpoint::canonical_encoding,
      relation_subcode::digest_mismatch, relation_replay_checkpoint_id(15));
  add(relation_diagnostic_kind::resource_reconciliation_audit,
      relation_checkpoint::resource_reconciliation,
      relation_subcode::resource_preflight,
      relation_replay_checkpoint_id(16), resource_kind::persistent_bytes,
      artifact.statistics().persistent_bytes,
      artifact.statistics().persistent_bytes);
  return records;
}

} // namespace

std::vector<std::uint8_t> encode_relation_diagnostic_semantics(
    const std::vector<relation_diagnostic_record> &records) {
  canonical_writer writer;
  writer.u32(0x44375259U); // YR7D
  writer.u16(contract_versions::relation_diagnostic_schema);
  writer.u64(records.size());
  for (const auto &record : records) {
    encode_diagnostic_without_digest(writer, record);
    encode_digest(writer, record.semantic_digest);
  }
  return writer.take();
}

std::vector<std::uint8_t> encode_relation_replay_checkpoint_semantics(
    const std::vector<relation_replay_checkpoint_record> &records) {
  canonical_writer writer;
  writer.u32(0x43375259U); // YR7C
  writer.u16(contract_versions::relation_replay_checkpoint_schema);
  writer.u64(records.size());
  for (const auto &record : records) {
    encode_checkpoint_without_digest(writer, record);
    encode_digest(writer, record.semantic_digest);
  }
  return writer.take();
}

std::vector<std::uint8_t> encode_relation_replay_evidence_semantics(
    const relation_replay_evidence &evidence) {
  canonical_writer writer;
  writer.u32(0x45375259U); // YR7E
  writer.u16(evidence.schema_version);
  writer.u16(evidence.policy_version);
  encode_digest(writer, evidence.input_equivalence_digest);
  encode_digest(writer, evidence.checkpoint_digest);
  encode_digest(writer, evidence.diagnostic_digest);
  encode_digest(writer, evidence.base_artifact_digest);
  writer.u64(evidence.checkpoint_count);
  writer.u64(evidence.diagnostic_count);
  writer.boolean(evidence.complete);
  writer.boolean(evidence.artifact_reconstructed);
  writer.boolean(evidence.primary_failure_present);
  writer.u8(evidence.reserved8);
  encode_digest(writer, evidence.semantic_digest);
  writer.u32(evidence.reserved32);
  return writer.take();
}

bounded_boolean_digest relation_failure_replay_digest(
    const bounded_boolean_digest &invocation_replay_digest,
    const bounded_boolean_error &error) {
  canonical_writer writer;
  writer.u32(0x46375259U); // YR7F
  writer.u16(contract_versions::relation_replay_policy);
  encode_digest(writer, invocation_replay_digest);
  writer.u16(error.version);
  writer.u8(static_cast<std::uint8_t>(error.category));
  writer.u32(error.subcode);
  writer.u16(error.component);
  writer.u16(error.stage);
  writer.u32(error.checkpoint);
  encode_digest(writer, error.context_digest);
  writer.u8(error.witness_count);
  for (const auto witness : error.witnesses)
    writer.u64(witness);
  return sha256::digest(writer.bytes());
}

template <class T, class I>
bool relation_replay_bundle_builder<T, I>::build(
    signed_feature_relations<T, I> &artifact,
    const relation_capabilities &capabilities,
    bounded_boolean_error &error) {
  try {
    artifact.diagnostics_.clear();
    artifact.replay_checkpoints_.clear();
    artifact.replay_evidence_ = relation_replay_evidence{};
    artifact.statistics_.diagnostic_count = 0;
    artifact.statistics_.replay_checkpoint_count = 0;

    const auto input_digest = replay_input_equivalence_digest(artifact);
    auto projection = artifact;
    projection.diagnostics_.clear();
    projection.replay_checkpoints_.clear();
    projection.replay_evidence_ = relation_replay_evidence{};
    projection.statistics_.diagnostic_count = 0;
    projection.statistics_.replay_checkpoint_count = 0;
    projection.canonical_bytes_.clear();
    projection.digest_ = bounded_boolean_digest{};
    const auto base_digest =
        sha256::digest(encode_signed_feature_relations(projection));
    artifact.replay_checkpoints_ =
        populate_replay_checkpoints(artifact, input_digest, base_digest);
    artifact.diagnostics_ = populate_diagnostics(artifact);
    if (artifact.replay_checkpoints_.size() >
            capabilities.maximum_replay_checkpoints ||
        artifact.diagnostics_.size() >
            capabilities.maximum_diagnostics) {
      error = relation_error(relation_subcode::resource_preflight,
                             bounded_boolean_error_category::resource_limit,
                             "Component 07 replay evidence exceeds capacity",
                             relation_checkpoint::resource_reconciliation);
      return false;
    }

    artifact.statistics_.replay_checkpoint_count =
        artifact.replay_checkpoints_.size();
    artifact.statistics_.diagnostic_count = artifact.diagnostics_.size();
    artifact.replay_evidence_.input_equivalence_digest = input_digest;
    artifact.replay_evidence_.checkpoint_digest = sha256::digest(
        encode_relation_replay_checkpoint_semantics(
            artifact.replay_checkpoints_));
    artifact.replay_evidence_.diagnostic_digest = sha256::digest(
        encode_relation_diagnostic_semantics(artifact.diagnostics_));
    artifact.replay_evidence_.base_artifact_digest = base_digest;
    artifact.replay_evidence_.checkpoint_count =
        artifact.replay_checkpoints_.size();
    artifact.replay_evidence_.diagnostic_count = artifact.diagnostics_.size();
    artifact.replay_evidence_.complete = true;
    artifact.replay_evidence_.artifact_reconstructed = true;
    artifact.replay_evidence_.primary_failure_present = false;
    artifact.replay_evidence_.semantic_digest =
        replay_evidence_digest(artifact.replay_evidence_);
    return true;
  } catch (const std::bad_alloc &) {
    error = relation_error(relation_subcode::resource_preflight,
                           bounded_boolean_error_category::resource_limit,
                           "Component 07 replay evidence allocation failed",
                           relation_checkpoint::canonical_encoding);
    return false;
  } catch (...) {
    error = relation_error(relation_subcode::internal_invariant,
                           bounded_boolean_error_category::internal_invariant_error,
                           "Component 07 replay evidence assembly failed",
                           relation_checkpoint::canonical_encoding);
    return false;
  }
}

template <class T, class I>
bool verify_relation_replay_bundle(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = relation_error(subcode,
                           bounded_boolean_error_category::internal_invariant_error,
                           summary, relation_checkpoint::independent_verification);
    return false;
  };
  const auto &evidence = artifact.replay_evidence_;
  if (evidence.schema_version !=
          contract_versions::relation_replay_evidence_schema ||
      evidence.policy_version != contract_versions::relation_replay_policy ||
      evidence.reserved8 != 0 || evidence.reserved32 != 0 ||
      !evidence.complete || !evidence.artifact_reconstructed ||
      evidence.primary_failure_present ||
      evidence.checkpoint_count != artifact.replay_checkpoints_.size() ||
      evidence.diagnostic_count != artifact.diagnostics_.size() ||
      artifact.statistics_.replay_checkpoint_count !=
          artifact.replay_checkpoints_.size() ||
      artifact.statistics_.diagnostic_count != artifact.diagnostics_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 replay evidence header is inconsistent");

  if (artifact.replay_checkpoints_.size() != 17 ||
      artifact.diagnostics_.size() != 4)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 replay checkpoint or diagnostic set is incomplete");

  const auto input_digest = replay_input_equivalence_digest(artifact);
  auto projection = artifact;
  projection.diagnostics_.clear();
  projection.replay_checkpoints_.clear();
  projection.replay_evidence_ = relation_replay_evidence{};
  projection.statistics_.diagnostic_count = 0;
  projection.statistics_.replay_checkpoint_count = 0;
  projection.canonical_bytes_.clear();
  projection.digest_ = bounded_boolean_digest{};
  const auto base_digest =
      sha256::digest(encode_signed_feature_relations(projection));
  if (evidence.input_equivalence_digest != input_digest ||
      evidence.base_artifact_digest != base_digest)
    return fail(relation_subcode::digest_mismatch,
                "Component 07 replay input or base-artifact digest mismatch");

  const auto specs = replay_checkpoint_specs(artifact);
  auto previous = input_digest;
  std::uint64_t cumulative_work = 0;
  for (std::size_t index = 0; index < specs.size(); ++index) {
    const auto &record = artifact.replay_checkpoints_[index];
    if (!checked_add(cumulative_work, specs[index].work_units,
                     cumulative_work))
      return fail(relation_subcode::work_limit,
                  "Component 07 replay checkpoint work overflow");
    if (record.id.ordinal() != index ||
        record.checkpoint != specs[index].checkpoint ||
        record.status != relation_replay_checkpoint_status::completed ||
        record.input_count != specs[index].input_count ||
        record.output_count != specs[index].output_count ||
        record.cumulative_work_units != cumulative_work ||
        record.schema_version !=
            contract_versions::relation_replay_checkpoint_schema ||
        record.reserved16 != 0 || record.reserved32 != 0 ||
        record.semantic_digest !=
            chained_checkpoint_digest(previous, base_digest, record))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 replay checkpoint does not reconstruct");
    previous = record.semantic_digest;
  }
  const auto checkpoint_digest = sha256::digest(
      encode_relation_replay_checkpoint_semantics(
          artifact.replay_checkpoints_));
  if (checkpoint_digest != evidence.checkpoint_digest)
    return fail(relation_subcode::digest_mismatch,
                "Component 07 replay checkpoint digest mismatch");

  const std::array<relation_diagnostic_kind, 4> kinds{{
      relation_diagnostic_kind::owner_exclusion_audit,
      relation_diagnostic_kind::selection_boundary_audit,
      relation_diagnostic_kind::replay_completeness_audit,
      relation_diagnostic_kind::resource_reconciliation_audit}};
  const std::array<relation_checkpoint, 4> checkpoints{{
      relation_checkpoint::producer_verification,
      relation_checkpoint::downstream_selection_boundary_audit,
      relation_checkpoint::canonical_encoding,
      relation_checkpoint::resource_reconciliation}};
  const std::array<relation_subcode, 4> subcodes{{
      relation_subcode::owner_in_semantics,
      relation_subcode::symbolic_selection_boundary,
      relation_subcode::digest_mismatch,
      relation_subcode::resource_preflight}};
  const std::array<std::uint64_t, 4> checkpoint_ids{{14, 13, 15, 16}};
  for (std::size_t index = 0; index < artifact.diagnostics_.size(); ++index) {
    const auto &record = artifact.diagnostics_[index];
    const bool resource_record = index == 3;
    if (record.id.ordinal() != index || record.kind != kinds[index] ||
        record.severity !=
            relation_diagnostic_severity::retained_finding ||
        record.checkpoint != checkpoints[index] ||
        record.subcode != static_cast<std::uint32_t>(subcodes[index]) ||
        record.has_candidate || record.has_relation ||
        record.has_source_features || record.has_numeric_evidence ||
        record.candidate_ordinal != relation_invalid_ordinal ||
        record.relation_ordinal != relation_invalid_ordinal ||
        record.first_feature != relation_feature_key{} ||
        record.second_feature != relation_feature_key{} ||
        record.rounded_nominal_bits != 0 || record.lower_bits != 0 ||
        record.upper_bits != 0 || record.margin_bits != 0 ||
        record.bounded_sign != bounded_sign_status::invalid ||
        record.exact_relation != exact_relation_status::unavailable ||
        record.disposition != predicate_disposition::fail_invalid ||
        record.rounded_formula != 0 || record.exact_formula != 0 ||
        record.trace_root != 0 ||
        record.resource != (resource_record ? resource_kind::persistent_bytes
                                           : resource_kind::diagnostic_findings) ||
        record.resource_limit !=
            (resource_record ? artifact.statistics_.persistent_bytes : 0) ||
        record.resource_used !=
            (resource_record ? artifact.statistics_.persistent_bytes : 0) ||
        record.cancellation_progress != 0 ||
        record.replay_checkpoint.ordinal() != checkpoint_ids[index] ||
        record.schema_version != contract_versions::relation_diagnostic_schema ||
        record.reserved16 != 0 || record.reserved32 != 0 ||
        record.semantic_digest != diagnostic_record_digest(record))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 retained diagnostic does not reconstruct");
  }
  const auto diagnostic_digest = sha256::digest(
      encode_relation_diagnostic_semantics(artifact.diagnostics_));
  if (diagnostic_digest != evidence.diagnostic_digest ||
      replay_evidence_digest(evidence) != evidence.semantic_digest)
    return fail(relation_subcode::digest_mismatch,
                "Component 07 replay or diagnostic semantic digest mismatch");
  return true;
}


template struct relation_replay_bundle_builder<float, std::uint32_t>;
template struct relation_replay_bundle_builder<float, std::uint64_t>;
template struct relation_replay_bundle_builder<double, std::uint32_t>;
template struct relation_replay_bundle_builder<double, std::uint64_t>;

template bool verify_relation_replay_bundle<float, std::uint32_t>(
    const signed_feature_relations<float, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_relation_replay_bundle<float, std::uint64_t>(
    const signed_feature_relations<float, std::uint64_t> &,
    bounded_boolean_error &);
template bool verify_relation_replay_bundle<double, std::uint32_t>(
    const signed_feature_relations<double, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_relation_replay_bundle<double, std::uint64_t>(
    const signed_feature_relations<double, std::uint64_t> &,
    bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
