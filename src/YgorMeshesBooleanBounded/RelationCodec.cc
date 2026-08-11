#include "StrictFloatingBuild.h"
#include "RelationCodec.h"
#include "RelationReplay.h"
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
  encode_relation_truth_record(writer, record);
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
  encode_symbolic_rule_key(writer, record.rule_key);
  encode_symbolic_rule_key(writer, record.exchanged_rule_key);
  writer.u8(static_cast<std::uint8_t>(record.subject_kind));
  writer.u64(record.subject_ordinal);
  writer.u8(static_cast<std::uint8_t>(record.operation));
  writer.u8(static_cast<std::uint8_t>(record.acting_operand));
  writer.u8(static_cast<std::uint8_t>(record.matrix_family));
  writer.u8(static_cast<std::uint8_t>(record.orientation));
  writer.u64(record.stable_rule_ordinal);
  writer.u64(record.exchange_rule_ordinal);
  writer.u8(record.feature_priority);
  writer.u8(static_cast<std::uint8_t>(record.half_open_owner));
  writer.u8(static_cast<std::uint8_t>(record.symbolic_crossing_contribution));
  writer.u8(static_cast<std::uint8_t>(record.coincident_owner_rank));
  writer.u8(static_cast<std::uint8_t>(record.conceptual_side));
  writer.u8(static_cast<std::uint8_t>(record.conceptual_order));
  writer.u8(static_cast<std::uint8_t>(record.contact_class));
  writer.u8(static_cast<std::uint8_t>(record.expected_disposition));
  writer.u16(static_cast<std::uint16_t>(record.explanation));
  encode_symbolic_tie_key_description(writer, record.tie_key);
  writer.u16(record.tie_key_schema);
  writer.boolean(record.owner_rank_eligible);
  writer.boolean(record.occurrence_separation_required);
  writer.boolean(record.nominal_geometry_unchanged);
  writer.u8(record.reserved8);
  writer.u16(record.schema_version);
  writer.u32(record.reserved);
}

void encode_predecessor_commitment(
    canonical_writer &writer,
    const relation_predecessor_commitment_record &record) {
  writer.u8(static_cast<std::uint8_t>(record.component));
  writer.u16(record.schema_version);
  writer.u16(record.provider_version);
  writer.u16(record.policy_version);
  writer.u16(record.codec_version);
  writer.u16(record.verifier_version);
  writer.boolean(record.independently_verified);
  writer.u8(record.reserved8);
  encode_digest(writer, record.artifact_digest_a);
  encode_digest(writer, record.artifact_digest_b);
  encode_digest(writer, record.semantic_digest_a);
  encode_digest(writer, record.semantic_digest_b);
  encode_digest(writer, record.exact_digest_a);
  encode_digest(writer, record.exact_digest_b);
  encode_digest(writer, record.policy_digest);
  writer.u16(record.commitment_schema);
  writer.u16(record.reserved16);
  writer.u32(record.reserved32);
}

void encode_resource_evidence(canonical_writer &writer,
                              const relation_resource_evidence_record &record) {
  writer.u64(record.id.ordinal());
  writer.u8(static_cast<std::uint8_t>(record.domain));
  writer.u8(static_cast<std::uint8_t>(record.resource));
  writer.u64(record.required_limit);
  writer.u64(record.preflight_reserved);
  writer.u64(record.closed_authority_bound);
  writer.u64(record.reconciled_used);
  writer.u64(record.witness_ordinal);
  writer.boolean(record.closed_authority_exact);
  writer.u8(record.reserved8);
  writer.u16(record.schema_version);
  writer.u32(record.reserved32);
}

} // namespace

template <class T, class I>
std::vector<std::uint8_t> encode_relation_unframed_payload(
    const signed_feature_relations<T, I> &artifact,
    std::array<std::size_t, 11> *boundaries = nullptr) {
  canonical_writer writer;
  if (boundaries)
    (*boundaries)[0] = 0;
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
  writer.u16(contract_versions::relation_predecessor_commitment_schema);
  writer.u16(static_cast<std::uint16_t>(artifact.predecessor_commitments_.size()));
  for (const auto &record : artifact.predecessor_commitments_)
    encode_predecessor_commitment(writer, record);
  if (boundaries)
    (*boundaries)[1] = writer.bytes().size();
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
  writer.sized_bytes(encode_relation_execution_authority_semantics(
      artifact.execution_authority_));
  const auto graph_bytes =
      encode_relation_request_graph_semantics(artifact.request_graph_);
  writer.sized_bytes(graph_bytes);
  if (boundaries)
    (*boundaries)[2] = writer.bytes().size();
  writer.u64(artifact.imported_geometry_.size());
  for (const auto &record : artifact.imported_geometry_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    encode_relation_feature_key(writer, record.feature);
    writer.u8(static_cast<std::uint8_t>(record.scope));
    writer.u8(record.reserved8);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  writer.u64(artifact.bounded_primitives_.size());
  for (const auto &record : artifact.bounded_primitives_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u32(record.truth_ordinal);
    writer.u64(record.rounded_nominal_bits);
    writer.u8(static_cast<std::uint8_t>(record.bounded_sign));
    writer.u8(static_cast<std::uint8_t>(record.disposition));
    writer.u16(record.rounded_formula);
    encode_truth(writer, record.evidence);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  if (boundaries)
    (*boundaries)[3] = writer.bytes().size();
  writer.u64(artifact.exact_relations_.size());
  for (const auto &record : artifact.exact_relations_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u32(record.truth_ordinal);
    writer.u8(static_cast<std::uint8_t>(record.status));
    writer.u16(record.exact_formula);
    encode_truth(writer, record.evidence);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  writer.u64(artifact.truth_lineage_.size());
  for (const auto &record : artifact.truth_lineage_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u32(record.truth_ordinal);
    writer.u64(record.bounded_primitive.ordinal());
    writer.u64(record.exact_relation.ordinal());
    writer.boolean(record.has_exact_relation);
    writer.u8(record.reserved8);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  if (boundaries)
    (*boundaries)[4] = writer.bytes().size();
  writer.u16(contract_versions::relation_interval_evidence_schema);
  writer.u16(contract_versions::relation_source_facet_region_publication_schema);
  writer.u64(artifact.interval_evidence_.size());
  for (const auto &record : artifact.interval_evidence_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.kind));
    writer.u32(record.occurrence);
    writer.u8(record.component);
    writer.boolean(record.has_rounded_nominal);
    writer.boolean(record.has_parameter_metadata);
    writer.boolean(record.within_authorized_boundary);
    writer.u64(record.rounded_nominal_bits);
    writer.u64(record.lower_bits);
    writer.u64(record.upper_bits);
    writer.u8(static_cast<std::uint8_t>(record.domain));
    writer.u64(record.domain_margin_bits);
    writer.u8(static_cast<std::uint8_t>(record.exact_zero));
    writer.u8(static_cast<std::uint8_t>(record.exact_one));
    for (const auto bits : record.contributor_bits)
      writer.u64(bits);
    writer.u64(record.trace_root);
    writer.u16(static_cast<std::uint16_t>(record.issued_operation));
    writer.u64(record.issued_value);
    writer.u64(record.issued_ledger_entry);
    writer.u64(record.issued_parent_values.size());
    for (const auto parent : record.issued_parent_values) writer.u64(parent);
    writer.u64(record.issued_parent_trace_roots.size());
    for (const auto parent : record.issued_parent_trace_roots) writer.u64(parent);
    writer.u64(record.issued_parent_ledger_entries.size());
    for (const auto parent : record.issued_parent_ledger_entries) writer.u64(parent);
    writer.sized_bytes(record.issued_operation_evidence);
    writer.u64(record.comparison_boundary_bits);
    writer.u8(record.reserved8);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  writer.u64(artifact.source_facet_regions_.size());
  for (const auto &record : artifact.source_facet_regions_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.kind));
    writer.u32(record.occurrence);
    writer.u8(record.query_component_count);
    writer.boolean(record.query_source_identity_valid);
    for (const auto bits : record.query_nominal_bits) writer.u64(bits);
    for (const auto bits : record.query_lower_bits) writer.u64(bits);
    for (const auto bits : record.query_upper_bits) writer.u64(bits);
    source_edge_facet_detail::encode_region(writer, record.region);
    writer.u8(record.reserved8);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
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
  if (boundaries)
    (*boundaries)[5] = writer.bytes().size();
  writer.u16(contract_versions::relation_construction_schema);
  writer.u16(contract_versions::relation_construction_registry_policy);
  writer.u16(contract_versions::relation_construction_ledger_schema);
  writer.u64(artifact.constructions_.size());
  for (const auto &record : artifact.constructions_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.producer.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.kind));
    writer.u8(static_cast<std::uint8_t>(record.precedence));
    writer.u8(static_cast<std::uint8_t>(record.coordinate_space));
    writer.u8(static_cast<std::uint8_t>(record.compatibility));
    writer.u8(record.component_count);
    writer.u8(record.projection_axis);
    encode_relation_feature_key(writer, record.authoritative_source_feature);
    for (const auto bits : record.nominal_bits) writer.u64(bits);
    for (const auto bits : record.lower_bits) writer.u64(bits);
    for (const auto bits : record.upper_bits) writer.u64(bits);
    writer.u64(record.source_provenance);
    writer.u64(record.geometric_lineage);
    for (const auto &feature : record.defining_features)
      encode_relation_feature_key(writer, feature);
    writer.u8(record.defining_feature_count);
    writer.u16(record.formula_version);
    writer.u16(record.schema_version);
    writer.u64(record.defining_dependency_begin);
    writer.u64(record.defining_dependency_count);
    writer.u64(record.radial_error_upper_bits);
    for (const auto bits : record.axis_error_upper_bits) writer.u64(bits);
    writer.u64(record.denominator_lower_bits);
    writer.u64(record.denominator_upper_bits);
    writer.u64(record.conditioning_lower_bits);
    writer.u16(static_cast<std::uint16_t>(record.operation));
    writer.u8(static_cast<std::uint8_t>(record.conditioning));
    writer.u8(static_cast<std::uint8_t>(record.tolerance));
    writer.u64(record.ordered_bounded_inputs.size());
    for (const auto input : record.ordered_bounded_inputs) writer.u64(input);
    writer.sized_bytes(record.certificate_evidence);
    writer.u64(record.precision_trace_root);
    writer.boolean(record.accepted_source_vertex);
    writer.boolean(record.finite);
    writer.boolean(record.tolerance_compatible);
    writer.boolean(record.precision_evidence_complete);
    writer.u64(record.tolerance_boundary_bits);
    writer.u64(record.residual_truth_begin);
    writer.u64(record.residual_truth_count);
    writer.u64(record.interval_evidence_begin);
    writer.u64(record.interval_evidence_count);
    writer.u64(record.source_facet_region_begin);
    writer.u64(record.source_facet_region_count);
    writer.u64(record.consumer_begin);
    writer.u64(record.consumer_count);
    writer.u64(record.ledger_begin);
    writer.u64(record.ledger_count);
    writer.u32(record.reserved);
  }
  writer.u64(artifact.construction_ledger_.size());
  for (const auto &record : artifact.construction_ledger_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.construction.ordinal());
    writer.u64(record.source_relation.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.precedence));
    writer.u8(static_cast<std::uint8_t>(record.coordinate_space));
    writer.u8(static_cast<std::uint8_t>(record.compatibility));
    writer.u8(record.component_count);
    writer.u8(record.projection_axis);
    writer.u32(record.occurrence);
    for (const auto bits : record.nominal_bits) writer.u64(bits);
    for (const auto bits : record.lower_bits) writer.u64(bits);
    for (const auto bits : record.upper_bits) writer.u64(bits);
    writer.u64(record.source_provenance);
    writer.u64(record.geometric_lineage);
    for (const auto &feature : record.defining_features)
      encode_relation_feature_key(writer, feature);
    writer.u8(record.defining_feature_count);
    writer.u16(record.formula_version);
    writer.u16(record.schema_version);
    writer.u64(record.precision_trace_root);
    for (const auto bits : record.axis_error_upper_bits) writer.u64(bits);
    writer.u64(record.radial_error_upper_bits);
    writer.u64(record.denominator_lower_bits);
    writer.u64(record.denominator_upper_bits);
    writer.u64(record.conditioning_lower_bits);
    writer.u16(static_cast<std::uint16_t>(record.operation));
    writer.u8(static_cast<std::uint8_t>(record.conditioning));
    writer.u8(static_cast<std::uint8_t>(record.tolerance));
    writer.u64(record.ordered_bounded_inputs.size());
    for (const auto input : record.ordered_bounded_inputs) writer.u64(input);
    writer.sized_bytes(record.certificate_evidence);
    writer.boolean(record.accepted_source_vertex);
    writer.boolean(record.finite);
    writer.boolean(record.tolerance_compatible);
    writer.boolean(record.authoritative_entry);
    writer.boolean(record.lineage_compatible);
    writer.boolean(record.enclosure_compatible);
    writer.boolean(record.parameter_compatible);
    writer.boolean(record.residual_compatible);
    writer.boolean(record.precision_evidence_complete);
    writer.u64(record.tolerance_boundary_bits);
    writer.u64(record.truth_begin);
    writer.u64(record.truth_count);
    writer.u64(record.interval_evidence_begin);
    writer.u64(record.interval_evidence_count);
    writer.u64(record.source_facet_region_begin);
    writer.u64(record.source_facet_region_count);
    writer.u32(record.reserved);
  }
  writer.u16(contract_versions::relation_coplanar_topology_schema);
  writer.u16(contract_versions::relation_coplanar_topology_policy);
  writer.u64(artifact.coplanar_event_nodes_.size());
  for (const auto &record : artifact.coplanar_event_nodes_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.overlay_relation.ordinal());
    writer.u64(record.representative.ordinal());
    writer.u64(record.occurrences.size());
    for (const auto &occurrence : record.occurrences) {
      writer.u8(occurrence.polygon);
      writer.u64(occurrence.edge_ordinal);
      writer.u64(occurrence.breakpoint_ordinal);
      writer.boolean(occurrence.query_source_vertex_valid);
      writer.u64(occurrence.query_source_vertex);
      writer.u64(occurrence.event_lineages.size());
      for (const auto &lineage : occurrence.event_lineages) {
        writer.u64(lineage.contact_lineage);
        writer.u8(lineage.endpoint_role);
        writer.u8(lineage.reserved8);
        writer.u16(lineage.reserved16);
      }
      writer.u8(occurrence.reserved8);
      writer.u16(occurrence.reserved16);
      writer.u32(occurrence.reserved32);
    }
    writer.u8(record.sheet_mask);
    writer.boolean(record.distinct_sheet_occurrences);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  writer.u64(artifact.coplanar_oriented_arcs_.size());
  for (const auto &record : artifact.coplanar_oriented_arcs_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.overlay_relation.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.kind));
    writer.u64(record.start_node.ordinal());
    writer.u64(record.end_node.ordinal());
    writer.u64(record.occurrences.size());
    for (const auto &occurrence : record.occurrences) {
      writer.u8(occurrence.polygon);
      writer.u64(occurrence.edge_ordinal);
      writer.u64(occurrence.interval_ordinal);
      writer.u64(occurrence.start_node.ordinal());
      writer.u64(occurrence.end_node.ordinal());
      writer.boolean(occurrence.forward_along_source_edge);
      writer.u8(occurrence.reserved8);
      writer.u16(occurrence.reserved16);
      writer.u32(occurrence.reserved32);
    }
    writer.u64(record.overlap_lineages.size());
    for (const auto lineage : record.overlap_lineages)
      writer.u64(lineage.ordinal());
    writer.u8(record.sheet_mask);
    writer.u8(record.reserved8);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  writer.u64(artifact.coplanar_overlap_components_.size());
  for (const auto &record : artifact.coplanar_overlap_components_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.overlay_relation.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.kind));
    writer.u64(record.node_ids.size());
    for (const auto node : record.node_ids)
      writer.u64(node.ordinal());
    writer.u64(record.arc_ids.size());
    for (const auto arc : record.arc_ids)
      writer.u64(arc.ordinal());
    writer.u8(record.sheet_mask);
    writer.boolean(record.closed);
    writer.boolean(record.distinct_sheet_occurrences);
    writer.u8(record.reserved8);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  if (boundaries)
    (*boundaries)[6] = writer.bytes().size();
  writer.u64(artifact.symbolic_eligibility_.size());
  for (const auto &record : artifact.symbolic_eligibility_)
    encode_eligibility(writer, record);
  writer.u64(artifact.symbolic_decisions_.size());
  for (const auto &record : artifact.symbolic_decisions_)
    encode_symbolic(writer, record);
  if (boundaries)
    (*boundaries)[7] = writer.bytes().size();
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
  if (boundaries)
    (*boundaries)[8] = writer.bytes().size();
  writer.u64(artifact.event_seeds_.size());
  for (const auto &record : artifact.event_seeds_) {
    writer.u64(record.id.ordinal());
    encode_relation_event_seed_key(writer, record.key);
    writer.u64(record.source_relation.ordinal());
    writer.u64(record.construction.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.contact_status));
    writer.u8(static_cast<std::uint8_t>(record.contact_dimension));
    writer.u8(static_cast<std::uint8_t>(record.construction_kind));
    encode_relation_feature_key(writer, record.accepted_source_vertex);
    writer.boolean(record.accepted_source_vertex_reused);
    writer.boolean(record.has_symbolic_decision);
    writer.u64(record.symbolic_decision.ordinal());
    writer.u64(record.symbolic_rule_ordinal);
    writer.u64(record.symbolic_exchange_rule_ordinal);
    writer.u8(static_cast<std::uint8_t>(record.symbolic_subject_kind));
    writer.u64(record.symbolic_subject_ordinal);
    writer.u32(record.symbolic_occurrence_rank);
    writer.u8(static_cast<std::uint8_t>(record.conceptual_side));
    writer.u8(static_cast<std::uint8_t>(record.conceptual_order));
    writer.u8(static_cast<std::uint8_t>(record.symbolic_contact));
    writer.u8(static_cast<std::uint8_t>(record.symbolic_expected));
    writer.u16(static_cast<std::uint16_t>(record.symbolic_explanation));
    writer.u16(record.symbolic_tie_key_schema);
    writer.u8(static_cast<std::uint8_t>(record.coincident_owner_rank));
    writer.boolean(record.symbolic_owner_rank_eligible);
    writer.u32(static_cast<std::uint32_t>(record.numeric_crossing));
    writer.u8(static_cast<std::uint8_t>(record.symbolic_crossing));
    writer.u8(static_cast<std::uint8_t>(record.half_open_owner));
    writer.u64(record.truth_begin);
    writer.u64(record.truth_count);
    writer.u64(record.construction_ledger_begin);
    writer.u64(record.construction_ledger_count);
    writer.u64(record.incidence_begin);
    writer.u64(record.incidence_count);
    writer.u64(record.candidate_incidence_begin);
    writer.u64(record.candidate_incidence_count);
    writer.boolean(record.precision_evidence_complete);
    writer.boolean(record.distinct_occurrence_required);
    writer.u16(record.schema_version);
    writer.u32(record.reserved);
  }
  writer.u64(artifact.event_seed_incidence_.size());
  for (const auto &feature : artifact.event_seed_incidence_)
    encode_relation_feature_key(writer, feature);
  writer.u64(artifact.event_seed_candidate_incidence_.size());
  for (const auto &record : artifact.event_seed_candidate_incidence_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.seed.ordinal());
    writer.u64(record.candidate.ordinal());
    writer.u64(record.disposition.ordinal());
    encode_relation_feature_key(writer, record.candidate_edge);
    encode_relation_feature_key(writer, record.source_triangle);
    for (const auto halfedge : record.edge_halfedges)
      writer.u64(halfedge);
    for (const auto halfedge : record.triangle_halfedges)
      writer.u64(halfedge);
    writer.boolean(record.internal_diagonal_witness);
    writer.boolean(record.source_feature_owner);
    writer.u16(record.schema_version);
    writer.u32(record.reserved);
  }
  canonical_writer reconciliation_writer;
  reconciliation_writer.u16(
      contract_versions::relation_triangle_local_publication_schema);
  reconciliation_writer.u64(artifact.triangle_local_reconciliation_.size());
  for (const auto &record : artifact.triangle_local_reconciliation_) {
    reconciliation_writer.u64(record.id.ordinal());
    reconciliation_writer.u64(record.candidate.ordinal());
    reconciliation_writer.u64(record.bookkeeping_request.ordinal());
    reconciliation_writer.u64(record.public_composite_request.ordinal());
    reconciliation_writer.u64(record.public_relation.ordinal());
    encode_relation_feature_key(reconciliation_writer, record.discovery_edge);
    encode_relation_feature_key(reconciliation_writer, record.discovery_triangle);
    encode_relation_feature_key(reconciliation_writer, record.owning_source_facet);
    encode_relation_feature_key(reconciliation_writer, record.opposite_source_facet);
    for (const auto value : record.edge_halfedges)
      reconciliation_writer.u64(value);
    for (const auto value : record.triangle_halfedges)
      reconciliation_writer.u64(value);
    reconciliation_writer.u8(static_cast<std::uint8_t>(record.disposition));
    reconciliation_writer.u8(static_cast<std::uint8_t>(record.no_public_reason));
    reconciliation_writer.boolean(record.internal_diagonal);
    reconciliation_writer.boolean(record.source_feature_owner);
    reconciliation_writer.boolean(record.symbolic_contact_owner);
    reconciliation_writer.boolean(record.classification_barrier);
    reconciliation_writer.boolean(record.retained_surface_feature);
    reconciliation_writer.boolean(record.complete);
    reconciliation_writer.u16(record.schema_version);
    reconciliation_writer.u32(record.reserved);
  }
  writer.sized_bytes(reconciliation_writer.take());
  writer.u16(contract_versions::relation_transverse_carrier_membership_schema);
  writer.u64(artifact.transverse_carrier_memberships_.size());
  for (const auto &record : artifact.transverse_carrier_memberships_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.carrier_relation.ordinal());
    writer.u64(record.carrier_construction.ordinal());
    writer.u64(record.member_relation.ordinal());
    writer.u64(record.point_construction.ordinal());
    writer.u64(record.seed.ordinal());
    writer.u32(record.occurrence);
    writer.u64(record.parameter.ordinal());
    for (const auto residual : record.point_carrier_residuals)
      writer.u64(residual.ordinal());
    writer.u64(record.first_region.ordinal());
    writer.u64(record.second_region.ordinal());
    writer.u64(record.parameter_lineage);
    writer.u64(record.carrier_lineage);
    writer.u64(record.event_lineage);
    writer.u32(static_cast<std::uint32_t>(record.numeric_crossing));
    writer.u8(static_cast<std::uint8_t>(record.local_transition));
    writer.u8(static_cast<std::uint8_t>(record.transition));
    writer.u8(static_cast<std::uint8_t>(record.half_open_owner));
    writer.boolean(record.numeric_owner);
    writer.boolean(record.finite);
    writer.boolean(record.conditioning_accepted);
    writer.boolean(record.residuals_accepted);
    writer.boolean(record.regions_complete);
    writer.boolean(record.precision_evidence_complete);
    writer.u16(record.schema_version);
    writer.u16(record.reserved16);
    writer.u32(record.reserved32);
  }
  canonical_writer handoff_writer;
  handoff_writer.u16(contract_versions::relation_downstream_topology_schema);
  for (const auto &topology : artifact.source_topology_) {
    handoff_writer.u8(static_cast<std::uint8_t>(topology.operand));
    handoff_writer.u64(topology.source_triangle_count);
    handoff_writer.u64(topology.canonical_edge_count);
    encode_digest(handoff_writer, topology.source_semantic_digest);
    encode_digest(handoff_writer, topology.exact_topology_digest);
    handoff_writer.u64(topology.source_edges.size());
    for (const auto &edge : topology.source_edges) {
      encode_relation_feature_key(handoff_writer, edge.source_edge);
      encode_relation_feature_key(handoff_writer, edge.start_vertex);
      encode_relation_feature_key(handoff_writer, edge.end_vertex);
      handoff_writer.u64(edge.canonical_edge);
      handoff_writer.u16(edge.schema_version);
      handoff_writer.u16(edge.reserved16);
      handoff_writer.u32(edge.reserved32);
    }
    handoff_writer.u64(topology.vertex_fans.size());
    for (const auto &fan : topology.vertex_fans) {
      encode_relation_feature_key(handoff_writer, fan.source_vertex);
      handoff_writer.u64(fan.ordered_facets.size());
      for (const auto &facet : fan.ordered_facets)
        encode_relation_feature_key(handoff_writer, facet);
      handoff_writer.u64(fan.canonical_vertex);
      handoff_writer.u16(fan.schema_version);
      handoff_writer.u16(fan.reserved16);
      handoff_writer.u32(fan.reserved32);
    }
    handoff_writer.u64(topology.edge_adjacencies.size());
    for (const auto &edge : topology.edge_adjacencies) {
      encode_relation_feature_key(handoff_writer, edge.edge);
      encode_relation_feature_key(handoff_writer, edge.first_facet);
      encode_relation_feature_key(handoff_writer, edge.second_facet);
      handoff_writer.u64(edge.canonical_edge);
      handoff_writer.u8(static_cast<std::uint8_t>(edge.edge_class));
      handoff_writer.boolean(edge.source_feature_owner);
      handoff_writer.boolean(edge.bookkeeping_only);
      handoff_writer.u8(edge.reserved8);
      handoff_writer.u16(edge.schema_version);
      handoff_writer.u32(edge.reserved32);
    }
    handoff_writer.u16(topology.schema_version);
    handoff_writer.u16(topology.reserved16);
    handoff_writer.u32(topology.reserved32);
  }
  handoff_writer.u16(contract_versions::relation_transverse_support_schema);
  handoff_writer.u64(artifact.transverse_carrier_supports_.size());
  for (const auto &support : artifact.transverse_carrier_supports_) {
    handoff_writer.u64(support.relation.ordinal());
    handoff_writer.u64(support.construction.ordinal());
    encode_relation_feature_key(handoff_writer, support.first_facet);
    encode_relation_feature_key(handoff_writer, support.second_facet);
    handoff_writer.u64(support.expected_membership_count);
    handoff_writer.boolean(support.support_consistent);
    handoff_writer.boolean(support.orientation_consistent);
    handoff_writer.boolean(support.residuals_accepted);
    handoff_writer.boolean(support.precision_evidence_complete);
    handoff_writer.u16(support.schema_version);
    handoff_writer.u16(support.reserved16);
    handoff_writer.u32(support.reserved32);
  }
  writer.sized_bytes(handoff_writer.take());
  writer.u64(artifact.candidate_dispositions_.size());
  for (const auto &record : artifact.candidate_dispositions_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.candidate.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.disposition));
    writer.u64(record.public_relation.ordinal());
    writer.u64(record.bookkeeping_request.ordinal());
    writer.u64(record.relation_begin);
    writer.u64(record.relation_count);
    writer.u64(record.event_seed_begin);
    writer.u64(record.event_seed_count);
    writer.u16(record.coverage_flags);
    writer.boolean(record.coverage_complete);
    writer.u16(record.schema_version);
    writer.u32(record.reserved);
  }
  writer.u64(artifact.candidate_relation_coverage_.size());
  for (const auto relation : artifact.candidate_relation_coverage_)
    writer.u64(relation.ordinal());
  writer.u64(artifact.candidate_event_seed_coverage_.size());
  for (const auto seed : artifact.candidate_event_seed_coverage_)
    writer.u64(seed.ordinal());
  writer.u64(artifact.candidate_partitions_.size());
  for (const auto &record : artifact.candidate_partitions_) {
    writer.u64(record.id.ordinal());
    writer.u64(record.source_partition.ordinal());
    writer.u64(record.candidate_begin);
    writer.u64(record.candidate_count);
    writer.u64(record.disposition_begin);
    writer.u64(record.disposition_count);
    writer.u64(record.relation_begin);
    writer.u64(record.relation_count);
    writer.u64(record.event_seed_begin);
    writer.u64(record.event_seed_count);
    writer.u64(record.maximum_records);
    writer.u16(record.schema_version);
    writer.u32(record.reserved);
  }
  if (boundaries)
    (*boundaries)[9] = writer.bytes().size();
  writer.u64(artifact.statistics_.candidate_count);
  writer.u64(artifact.statistics_.request_proposal_count);
  writer.u64(artifact.statistics_.unique_request_count);
  writer.u64(artifact.statistics_.dependency_count);
  writer.u64(artifact.statistics_.reverse_consumer_count);
  writer.u64(artifact.statistics_.candidate_witness_count);
  writer.u64(artifact.statistics_.imported_geometry_count);
  writer.u64(artifact.statistics_.bounded_primitive_count);
  writer.u64(artifact.statistics_.exact_relation_count);
  writer.u64(artifact.statistics_.truth_lineage_count);
  writer.u64(artifact.statistics_.interval_evidence_count);
  writer.u64(artifact.statistics_.source_facet_region_count);
  writer.u64(artifact.statistics_.public_relation_count);
  writer.u64(artifact.statistics_.bookkeeping_relation_count);
  writer.u64(artifact.statistics_.construction_count);
  writer.u64(artifact.statistics_.construction_ledger_count);
  writer.u64(artifact.statistics_.coplanar_event_node_count);
  writer.u64(artifact.statistics_.coplanar_oriented_arc_count);
  writer.u64(artifact.statistics_.coplanar_overlap_component_count);
  writer.u64(artifact.statistics_.symbolic_eligibility_count);
  writer.u64(artifact.statistics_.symbolic_decision_count);
  writer.u64(artifact.statistics_.crossing_record_count);
  writer.u64(artifact.statistics_.event_seed_count);
  writer.u64(artifact.statistics_.event_seed_candidate_incidence_count);
  writer.u64(artifact.statistics_.candidate_relation_coverage_count);
  writer.u64(artifact.statistics_.candidate_seed_coverage_count);
  writer.u64(artifact.statistics_.candidate_partition_count);
  writer.u64(artifact.statistics_.triangle_local_reconciliation_count);
  writer.u64(artifact.statistics_.transverse_carrier_membership_count);
  writer.u64(artifact.statistics_.diagnostic_count);
  writer.u64(artifact.statistics_.replay_checkpoint_count);
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
  writer.u16(contract_versions::relation_resource_evidence_schema);
  writer.u16(static_cast<std::uint16_t>(artifact.resource_evidence_.size()));
  for (const auto &record : artifact.resource_evidence_)
    encode_resource_evidence(writer, record);
  writer.sized_bytes(
      encode_relation_diagnostic_semantics(artifact.diagnostics_));
  writer.sized_bytes(encode_relation_replay_checkpoint_semantics(
      artifact.replay_checkpoints_));
  writer.sized_bytes(
      encode_relation_replay_evidence_semantics(artifact.replay_evidence_));
  if (boundaries)
    (*boundaries)[10] = writer.bytes().size();
  return writer.take();
}

template <class T, class I>
std::array<std::vector<std::uint8_t>, 10> build_relation_section_payloads(
    const signed_feature_relations<T, I> &artifact) {
  std::array<std::size_t, 11> boundaries{};
  const auto payload = encode_relation_unframed_payload(artifact, &boundaries);
  std::array<std::vector<std::uint8_t>, 10> sections;
  for (std::size_t i = 0; i < sections.size(); ++i) {
    const auto begin = boundaries[i];
    const auto end = boundaries[i + 1];
    sections[i].assign(payload.begin() + begin, payload.begin() + end);
  }
  return sections;
}

template <class T, class I>
bool refresh_relation_section_digests(
    signed_feature_relations<T, I> &artifact) noexcept {
  try {
    const auto sections = build_relation_section_payloads(artifact);
    for (std::size_t i = 0; i < sections.size(); ++i) {
      artifact.section_digests_[i].domain =
          static_cast<relation_section_domain>(i + 1);
      artifact.section_digests_[i].layout_version =
          contract_versions::relation_section_digest_layout;
      artifact.section_digests_[i].reserved32 = 0;
      artifact.section_digests_[i].digest = sha256::digest(sections[i]);
    }
    return true;
  } catch (...) {
    return false;
  }
}

template <class T, class I>
std::vector<std::uint8_t>
encode_signed_feature_relations(const signed_feature_relations<T, I> &artifact) {
  const auto sections = build_relation_section_payloads(artifact);
  canonical_writer writer;
  writer.u32(0x46375259U); // YR7F framed artifact.
  writer.u16(contract_versions::relation_codec);
  writer.u16(static_cast<std::uint16_t>(sections.size()));
  writer.u32(0);
  for (std::size_t i = 0; i < sections.size(); ++i) {
    const auto &record = artifact.section_digests_[i];
    writer.u16(static_cast<std::uint16_t>(record.domain));
    writer.u16(record.layout_version);
    writer.u32(record.reserved32);
    encode_digest(writer, record.digest);
    writer.sized_bytes(sections[i]);
  }
  const auto digest = sha256::digest(writer.bytes());
  encode_digest(writer, digest);
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

inline bool read_feature_key(canonical_reader &reader, relation_feature_key *out) {
  relation_feature_key key;
  std::uint8_t operand = 0, kind = 0;
  if (!reader.u8(operand) || !reader.u8(kind) || !reader.u64(key.primary) ||
      !reader.u64(key.secondary) || !reader.u32(key.occurrence) ||
      !reader.u16(key.schema_version))
    return false;
  key.operand = static_cast<operand_id>(operand);
  key.kind = static_cast<relation_feature_kind>(kind);
  const bool none = key.kind == relation_feature_kind::none &&
                    key.primary == 0 && key.secondary == 0 &&
                    key.occurrence == 0 &&
                    key.schema_version ==
                        contract_versions::relation_feature_key_schema;
  if (!none && !valid_relation_feature_key(key)) return false;
  if (out) *out = key;
  return true;
}

inline bool read_feature_key(canonical_reader &reader) {
  return read_feature_key(reader, nullptr);
}

inline bool read_request_key(canonical_reader &reader) {
  std::uint8_t family = 0, scope = 0;
  std::uint32_t reserved = 0;
  relation_request_key key;
  if (!read_digest(reader, key.semantic_namespace) || !reader.u8(family) ||
      !reader.u8(scope) || !read_feature_key(reader, &key.first) ||
      !read_feature_key(reader, &key.second) || !reader.u64(key.directed_use) ||
      !reader.u16(key.formula_version) || !reader.u16(key.policy_version) ||
      !reader.u32(key.occurrence_discriminator) || !reader.u32(reserved) ||
      reserved != 0)
    return false;
  key.family = static_cast<relation_request_family>(family);
  key.scope = static_cast<relation_record_scope>(scope);
  return valid_relation_request_key(key);
}

inline bool read_symbolic_rule_key(canonical_reader &reader) {
  std::uint8_t byte = 0;
  std::uint16_t version = 0, reserved = 0;
  for (std::size_t i = 0; i < 8; ++i)
    if (!reader.u8(byte))
      return false;
  return reader.u16(version) && reader.u16(reserved);
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

inline bool read_imported_geometry_record(canonical_reader &reader) {
  std::uint64_t id = 0, producer = 0;
  std::uint8_t scope = 0, reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
  return reader.u64(id) && reader.u64(producer) && read_feature_key(reader) &&
         reader.u8(scope) && reader.u8(reserved8) &&
         reader.u16(reserved16) && reader.u32(reserved32);
}

inline bool read_truth_record(canonical_reader &reader) {
  std::uint16_t schema = 0;
  std::array<std::uint64_t, 44> values{};
  std::uint8_t bounded = 0, exact = 0, disposition = 0, publication = 0;
  std::uint16_t rounded_formula = 0, exact_formula = 0, bounded_schema = 0,
                bounded_provider = 0, exact_schema = 0, input_count = 0;
  std::uint32_t normalization = 0, capacity_used = 0, capacity_limit = 0,
                reserved = 0;
  bool alternate = false;
  if (!reader.u16(schema) || schema != contract_versions::relation_truth_policy)
    return false;
  for (auto &value : values)
    if (!reader.u64(value)) return false;
  if (!reader.u8(bounded) ||
      bounded < static_cast<std::uint8_t>(bounded_sign_status::definitely_negative) ||
      bounded > static_cast<std::uint8_t>(bounded_sign_status::definitely_positive) ||
      !reader.u8(exact) ||
      exact < static_cast<std::uint8_t>(exact_relation_status::exact_negative) ||
      exact > static_cast<std::uint8_t>(exact_relation_status::unavailable) ||
      !reader.u8(disposition) ||
      disposition < static_cast<std::uint8_t>(predicate_disposition::accept_numeric_sign) ||
      disposition > static_cast<std::uint8_t>(predicate_disposition::fail_invalid) ||
      !reader.u16(rounded_formula) ||
      !registered_rounded_operation(
          static_cast<rounded_operation_code>(rounded_formula)) ||
      !reader.u16(exact_formula) || !valid_exact_formula_code(exact_formula) ||
      !reader.u16(bounded_schema) ||
      bounded_schema != contract_versions::bounded_values ||
      !reader.u16(bounded_provider) ||
      bounded_provider != contract_versions::finite_interval_provider ||
      !reader.u16(exact_schema) ||
      exact_schema != contract_versions::predicate_truth_layers ||
      !reader.u16(input_count) || input_count > 24 ||
      !reader.u32(normalization) || !reader.u32(capacity_used) ||
      !reader.u32(capacity_limit) || capacity_used > capacity_limit ||
      !reader.u8(publication) ||
      publication < static_cast<std::uint8_t>(bounded_publication_state::transaction_local) ||
      publication > static_cast<std::uint8_t>(bounded_publication_state::committed) ||
      !reader.boolean(alternate) || !reader.u32(reserved) || reserved != 0)
    return false;
  const bool requested =
      exact_formula != static_cast<std::uint16_t>(exact_relation_formula_code::invalid);
  for (std::size_t i = 20 + input_count; i < values.size(); ++i)
    if (values[i] != 0) return false;
  return requested
             ? (values[18] != 0 && values[19] != 0 && input_count != 0 &&
                exact != static_cast<std::uint8_t>(exact_relation_status::unavailable))
             : (values[18] == 0 && values[19] == 0 && input_count == 0 &&
                exact == static_cast<std::uint8_t>(exact_relation_status::unavailable));
}

inline bool read_bounded_primitive_record(canonical_reader &reader) {
  std::uint64_t id = 0, producer = 0, source = 0, nominal = 0;
  std::uint32_t truth = 0, reserved32 = 0;
  std::uint8_t bounded = 0, disposition = 0;
  std::uint16_t formula = 0, reserved16 = 0;
  return reader.u64(id) && reader.u64(producer) && reader.u64(source) &&
         reader.u32(truth) && reader.u64(nominal) && reader.u8(bounded) &&
           bounded >= static_cast<std::uint8_t>(bounded_sign_status::definitely_negative) &&
           bounded <= static_cast<std::uint8_t>(bounded_sign_status::definitely_positive) &&
           reader.u8(disposition) &&
           disposition >= static_cast<std::uint8_t>(predicate_disposition::accept_numeric_sign) &&
           disposition < static_cast<std::uint8_t>(predicate_disposition::fail_invalid) &&
           reader.u16(formula) && registered_rounded_operation(
               static_cast<rounded_operation_code>(formula)) &&
           read_truth_record(reader) &&
         reader.u16(reserved16) && reserved16 == 0 &&
         reader.u32(reserved32) && reserved32 == 0;
}

inline bool read_exact_relation_record(canonical_reader &reader) {
  std::uint64_t id = 0, producer = 0, source = 0;
  std::uint32_t truth = 0, reserved32 = 0;
  std::uint8_t status = 0;
  std::uint16_t formula = 0, reserved16 = 0;
  return reader.u64(id) && reader.u64(producer) && reader.u64(source) &&
           reader.u32(truth) && reader.u8(status) &&
           status >= static_cast<std::uint8_t>(exact_relation_status::exact_negative) &&
           status <= static_cast<std::uint8_t>(exact_relation_status::exact_positive) &&
           reader.u16(formula) && valid_exact_formula_code(formula) && formula != 0 &&
           read_truth_record(reader) &&
         reader.u16(reserved16) && reserved16 == 0 &&
         reader.u32(reserved32) && reserved32 == 0;
}

inline bool read_truth_lineage_record(canonical_reader &reader) {
  std::uint64_t id = 0, source = 0, bounded = 0, exact = 0;
  std::uint32_t truth = 0, reserved32 = 0;
  bool has_exact = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  return reader.u64(id) && reader.u64(source) && reader.u32(truth) &&
         reader.u64(bounded) && reader.u64(exact) &&
         reader.boolean(has_exact) && reader.u8(reserved8) &&
         reader.u16(reserved16) && reader.u32(reserved32);
}

inline bool read_interval_evidence_record(canonical_reader &reader) {
  std::uint64_t value = 0, count = 0;
  std::uint32_t occurrence = 0, reserved32 = 0;
  std::uint16_t reserved16 = 0;
  std::uint8_t byte = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u8(byte) || !reader.u32(occurrence) || !reader.u8(byte))
    return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (!reader.boolean(flag)) return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u8(byte) || !reader.u64(value) || !reader.u8(byte) ||
      !reader.u8(byte))
    return false;
  for (std::size_t i = 0; i < 8; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u64(value) || !reader.u16(reserved16) ||
      !reader.u64(value) || !reader.u64(value))
    return false;
  for (std::size_t identity = 0; identity < 3; ++identity) {
    if (!reader.u64(count) || count > 64)
      return false;
    for (std::uint64_t parent = 0; parent < count; ++parent)
      if (!reader.u64(value)) return false;
  }
  std::vector<std::uint8_t> evidence;
  return reader.sized_bytes(evidence, std::uint64_t{1} << 24U) &&
         reader.u64(value) && reader.u8(byte) &&
          reader.u16(reserved16) && reader.u32(reserved32);
}

template <class T>
bool read_source_orientation(canonical_reader &reader) {
  T value = T(0);
  std::uint8_t byte = 0;
  std::uint16_t formula = 0;
  return reader.floating(value) && reader.floating(value) &&
         reader.u8(byte) && reader.u8(byte) && reader.u16(formula);
}

template <class T>
bool read_source_facet_region(canonical_reader &reader,
                              const relation_capabilities &capabilities) {
  std::uint16_t schema = 0, policy = 0;
  std::uint8_t byte = 0;
  std::uint64_t value = 0, count = 0;
  std::uint32_t reserved = 0;
  bool flag = false;
  if (!reader.u16(schema) || !reader.u16(policy) || !reader.u8(byte) ||
      !reader.u64(value) || !reader.u64(value) || !reader.u8(byte))
    return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (!reader.boolean(flag)) return false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(count) ||
      !count_fits(reader, count, capabilities.maximum_dependencies, 8))
    return false;
  for (std::uint64_t i = 0; i < count; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u64(count) ||
      !count_fits(reader, count, capabilities.maximum_dependencies, 24))
    return false;
  for (std::uint64_t i = 0; i < count; ++i)
    for (std::size_t component = 0; component < 3; ++component)
      if (!reader.u64(value)) return false;
  if (!read_source_orientation<T>(reader) || !reader.u64(count) ||
      !count_fits(reader, count, capabilities.maximum_dependencies,
                  2 * sizeof(T) + 4))
    return false;
  for (std::uint64_t i = 0; i < count; ++i)
    if (!read_source_orientation<T>(reader)) return false;
  return reader.u32(reserved);
}

template <class T>
bool read_source_facet_region_record(
    canonical_reader &reader, const relation_capabilities &capabilities) {
  std::uint64_t value = 0;
  std::uint32_t occurrence = 0, reserved32 = 0;
  std::uint16_t reserved16 = 0;
  std::uint8_t byte = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u8(byte) || !reader.u32(occurrence) || !reader.u8(byte) ||
      !reader.boolean(flag))
    return false;
  for (std::size_t i = 0; i < 9; ++i)
    if (!reader.u64(value)) return false;
  return read_source_facet_region<T>(reader, capabilities) &&
         reader.u8(byte) && reader.u16(reserved16) &&
         reader.u32(reserved32);
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

inline bool read_transverse_membership_record(canonical_reader &reader,
                                              std::uint64_t expected_id) {
  constexpr auto invalid = std::numeric_limits<std::uint64_t>::max();
  std::array<std::uint64_t, 6> identities{};
  std::uint64_t parameter = 0, value = 0;
  std::uint32_t occurrence = 0, numeric = 0, reserved32 = 0;
  std::uint16_t schema = 0, reserved16 = 0;
  std::uint8_t local = 0, transition = 0, owner = 0;
  bool flag = false;
  for (auto &identity : identities)
    if (!reader.u64(identity)) return false;
  if (identities[0] != expected_id ||
      std::find(identities.begin(), identities.end(), invalid) !=
          identities.end() ||
      !reader.u32(occurrence) || !reader.u64(parameter) || parameter == invalid)
    return false;
  for (std::size_t i = 0; i < 8; ++i)
    if (!reader.u64(value) || value == invalid) return false;
  if (!reader.u32(numeric) ||
      static_cast<std::int32_t>(numeric) < -1 ||
      static_cast<std::int32_t>(numeric) > 1 ||
      !reader.u8(local) || (local != 0 && local != 1 && local != 0xffU) ||
      !reader.u8(transition) ||
      transition < static_cast<std::uint8_t>(relation_carrier_transition::entering) ||
      transition > static_cast<std::uint8_t>(relation_carrier_transition::tangent) ||
      !reader.u8(owner) ||
      (owner != static_cast<std::uint8_t>(operand_id::a) &&
       owner != static_cast<std::uint8_t>(operand_id::b)))
    return false;
  for (std::size_t i = 0; i < 6; ++i)
    if (!reader.boolean(flag)) return false;
  return reader.u16(schema) &&
         schema == contract_versions::relation_transverse_carrier_membership_schema &&
         reader.u16(reserved16) && reserved16 == 0 &&
         reader.u32(reserved32) && reserved32 == 0;
}

inline bool read_construction_record(canonical_reader &reader) {
  std::uint64_t value = 0, input_count = 0;
  std::vector<std::uint8_t> certificate;
  std::uint8_t kind = 0, precedence = 0, space = 0, compatibility = 0,
               components = 0, projection = 0, defining_count = 0,
               conditioning = 0, tolerance = 0;
  std::uint16_t formula = 0, schema = 0, operation = 0;
  std::uint32_t reserved = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u8(kind) || !reader.u8(precedence) || !reader.u8(space) ||
       !reader.u8(compatibility) || !reader.u8(components) ||
       !reader.u8(projection) ||
       !read_feature_key(reader))
    return false;
  if (kind < 1 || kind > 4 || precedence < 1 || precedence > 6 ||
      space < 1 || space > 2 || compatibility < 1 || compatibility > 2 ||
      !((kind == static_cast<std::uint8_t>(relation_construction_kind::bounded_point) &&
         (components == 2 || components == 3)) ||
        (kind == static_cast<std::uint8_t>(relation_construction_kind::bounded_carrier) &&
         components == 6)) ||
      (space == static_cast<std::uint8_t>(relation_construction_coordinate_space::world_3d)
           ? projection != 3
           : projection > 2 || components != 2))
    return false;
  for (std::size_t i = 0; i < 18; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u64(value) || !reader.u64(value)) return false;
  if (!read_feature_key(reader) || !read_feature_key(reader) ||
       !reader.u8(defining_count) || defining_count == 0 || defining_count > 2 ||
       !reader.u16(formula) || formula == 0 || !reader.u16(schema) ||
       schema != contract_versions::relation_construction_schema)
    return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (!reader.u64(value)) return false;
  for (std::size_t i = 0; i < 6; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u16(operation) ||
      !registered_rounded_operation(
          static_cast<rounded_operation_code>(operation)) ||
      !reader.u8(conditioning) || conditioning < 1 || conditioning > 6 ||
      !reader.u8(tolerance) || tolerance < 1 || tolerance > 2 ||
      !reader.u64(input_count) || input_count == 0 || input_count > 64)
    return false;
  for (std::uint64_t input = 0; input < input_count; ++input)
    if (!reader.u64(value) || value == 0) return false;
  if (!reader.sized_bytes(certificate, std::uint64_t{1} << 24U) ||
      certificate.empty() || !reader.u64(value) || value == 0)
    return false;
  for (std::size_t i = 0; i < 4; ++i)
    if (!reader.boolean(flag)) return false;
  if (!reader.u64(value)) return false;
  for (std::size_t i = 0; i < 10; ++i)
    if (!reader.u64(value)) return false;
  return reader.u32(reserved) && reserved == 0;
}

inline bool read_construction_ledger_record(canonical_reader &reader) {
  std::uint64_t value = 0, input_count = 0;
  std::vector<std::uint8_t> certificate;
  std::uint8_t precedence = 0, space = 0, compatibility = 0,
               components = 0, projection = 0, defining_count = 0,
               conditioning = 0, tolerance = 0;
  std::uint16_t formula = 0, schema = 0, operation = 0;
  std::uint32_t occurrence = 0, reserved = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u8(precedence) || !reader.u8(space) ||
      !reader.u8(compatibility) || !reader.u8(components) ||
      !reader.u8(projection) || !reader.u32(occurrence))
    return false;
  if (precedence < 1 || precedence > 6 || space < 1 || space > 2 ||
      compatibility < 1 || compatibility > 2 ||
      (space == static_cast<std::uint8_t>(relation_construction_coordinate_space::world_3d)
           ? (projection != 3 || (components != 3 && components != 6))
           : (projection > 2 || components != 2)))
    return false;
  for (std::size_t i = 0; i < 18; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u64(value) || !reader.u64(value)) return false;
  if (!read_feature_key(reader) || !read_feature_key(reader) ||
       !reader.u8(defining_count) || defining_count == 0 || defining_count > 2 ||
       !reader.u16(formula) || formula == 0 || !reader.u16(schema) ||
       schema != contract_versions::relation_construction_ledger_schema ||
       !reader.u64(value) || value == 0)
    return false;
  for (std::size_t i = 0; i < 6; ++i)
    if (!reader.u64(value)) return false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u64(value) || !reader.u16(operation) ||
      !registered_rounded_operation(
          static_cast<rounded_operation_code>(operation)) ||
      !reader.u8(conditioning) || conditioning < 1 || conditioning > 6 ||
      !reader.u8(tolerance) || tolerance < 1 || tolerance > 2 ||
      !reader.u64(input_count) || input_count == 0 || input_count > 64)
    return false;
  for (std::uint64_t input = 0; input < input_count; ++input)
    if (!reader.u64(value) || value == 0) return false;
  if (!reader.sized_bytes(certificate, std::uint64_t{1} << 24U) ||
      certificate.empty())
    return false;
  for (std::size_t i = 0; i < 9; ++i)
    if (!reader.boolean(flag)) return false;
  if (!reader.u64(value)) return false;
  for (std::size_t i = 0; i < 6; ++i)
    if (!reader.u64(value)) return false;
  return reader.u32(reserved) && reserved == 0;
}

inline bool read_coplanar_event_node_record(
    canonical_reader &reader, const relation_capabilities &capabilities) {
  std::uint64_t value = 0, occurrence_count = 0, lineage_count = 0;
  std::uint8_t byte = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u64(occurrence_count) ||
      !count_fits(reader, occurrence_count, capabilities.maximum_dependencies,
                  39))
    return false;
  for (std::uint64_t occurrence = 0; occurrence < occurrence_count;
       ++occurrence) {
    if (!reader.u8(byte) || !reader.u64(value) || !reader.u64(value) ||
        !reader.boolean(flag) || !reader.u64(value) ||
        !reader.u64(lineage_count) ||
        !count_fits(reader, lineage_count, capabilities.maximum_dependencies,
                    12))
      return false;
    for (std::uint64_t lineage = 0; lineage < lineage_count; ++lineage)
      if (!reader.u64(value) || !reader.u8(byte) || !reader.u8(byte) ||
          !reader.u16(reserved16))
        return false;
    if (!reader.u8(byte) || !reader.u16(reserved16) ||
        !reader.u32(reserved32))
      return false;
  }
  return reader.u8(byte) && reader.boolean(flag) &&
         reader.u16(reserved16) && reader.u32(reserved32);
}

inline bool read_coplanar_oriented_arc_record(
    canonical_reader &reader, const relation_capabilities &capabilities) {
  std::uint64_t value = 0, occurrence_count = 0, lineage_count = 0;
  std::uint8_t byte = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u8(byte) ||
      !reader.u64(value) || !reader.u64(value) ||
      !reader.u64(occurrence_count) ||
      !count_fits(reader, occurrence_count, capabilities.maximum_dependencies,
                  41))
    return false;
  for (std::uint64_t occurrence = 0; occurrence < occurrence_count;
       ++occurrence)
    if (!reader.u8(byte) || !reader.u64(value) || !reader.u64(value) ||
        !reader.u64(value) || !reader.u64(value) || !reader.boolean(flag) ||
        !reader.u8(byte) || !reader.u16(reserved16) ||
        !reader.u32(reserved32))
      return false;
  if (!reader.u64(lineage_count) ||
      !count_fits(reader, lineage_count, capabilities.maximum_dependencies, 8))
    return false;
  for (std::uint64_t lineage = 0; lineage < lineage_count; ++lineage)
    if (!reader.u64(value))
      return false;
  return reader.u8(byte) && reader.u8(byte) && reader.u16(reserved16) &&
         reader.u32(reserved32);
}

inline bool read_coplanar_overlap_component_record(
    canonical_reader &reader, const relation_capabilities &capabilities) {
  std::uint64_t value = 0, node_count = 0, arc_count = 0;
  std::uint8_t byte = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u8(byte) ||
      !reader.u64(node_count) ||
      !count_fits(reader, node_count, capabilities.maximum_dependencies, 8))
    return false;
  for (std::uint64_t node = 0; node < node_count; ++node)
    if (!reader.u64(value))
      return false;
  if (!reader.u64(arc_count) ||
      !count_fits(reader, arc_count, capabilities.maximum_dependencies, 8))
    return false;
  for (std::uint64_t arc = 0; arc < arc_count; ++arc)
    if (!reader.u64(value))
      return false;
  return reader.u8(byte) && reader.boolean(flag) && reader.boolean(flag) &&
         reader.u8(byte) && reader.u16(reserved16) &&
         reader.u32(reserved32);
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

inline bool read_symbolic_tie_key_description(canonical_reader &reader) {
  std::uint8_t byte = 0;
  std::uint16_t value16 = 0;
  for (std::size_t component = 0;
       component < symbolic_tie_key_component_count; ++component)
    if (!reader.u8(byte))
      return false;
  return reader.u8(byte) && reader.u8(byte) && reader.u16(value16) &&
         reader.u16(value16);
}

inline bool read_symbolic_record(canonical_reader &reader) {
  std::uint64_t id = 0, value64 = 0;
  std::uint8_t value = 0;
  bool flag = false;
  std::uint16_t value16 = 0;
  std::uint32_t reserved = 0;
  if (!reader.u64(id) || !read_request_key(reader) ||
      !read_symbolic_rule_key(reader) ||
      !read_symbolic_rule_key(reader) || !reader.u8(value) ||
      !reader.u64(value64))
    return false;
  for (std::size_t i = 0; i < 4; ++i)
    if (!reader.u8(value))
      return false;
  if (!reader.u64(value64) || !reader.u64(value64))
    return false;
  for (std::size_t i = 0; i < 8; ++i)
    if (!reader.u8(value))
      return false;
  return reader.u16(value16) &&
         read_symbolic_tie_key_description(reader) && reader.u16(value16) &&
         reader.boolean(flag) && reader.boolean(flag) &&
         reader.boolean(flag) && reader.u8(value) && reader.u16(value16) &&
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
  std::uint32_t numeric = 0, rank = 0, reserved = 0;
  std::uint16_t schema = 0;
  std::uint8_t byte = 0;
  bool flag = false;
  return reader.u64(value) && read_event_seed_key(reader) &&
         reader.u64(value) && reader.u64(value) && reader.u8(byte) &&
         reader.u8(byte) && reader.u8(byte) && read_feature_key(reader) &&
         reader.boolean(flag) && reader.boolean(flag) && reader.u64(value) &&
         reader.u64(value) && reader.u64(value) && reader.u8(byte) &&
         reader.u64(value) && reader.u32(rank) && reader.u8(byte) &&
         reader.u8(byte) && reader.u8(byte) && reader.u8(byte) &&
         reader.u16(schema) && reader.u16(schema) && reader.u8(byte) &&
         reader.boolean(flag) &&
         reader.u32(numeric) && reader.u8(byte) && reader.u8(byte) &&
         reader.u64(value) && reader.u64(value) && reader.u64(value) &&
         reader.u64(value) && reader.u64(value) && reader.u64(value) &&
         reader.u64(value) && reader.u64(value) && reader.boolean(flag) &&
         reader.boolean(flag) && reader.u16(schema) && reader.u32(reserved);
}

inline bool read_event_seed_candidate_incidence_record(
    canonical_reader &reader) {
  std::uint64_t value = 0;
  std::uint16_t schema = 0;
  std::uint32_t reserved = 0;
  bool flag = false;
  if (!reader.u64(value) || !reader.u64(value) || !reader.u64(value) ||
      !reader.u64(value) || !read_feature_key(reader) ||
      !read_feature_key(reader))
    return false;
  for (std::size_t i = 0; i < 5; ++i)
    if (!reader.u64(value))
      return false;
  return reader.boolean(flag) && reader.boolean(flag) &&
         reader.u16(schema) && reader.u32(reserved);
}

inline bool read_candidate_disposition_record(canonical_reader &reader) {
  std::uint64_t value = 0;
  std::uint16_t flags = 0, schema = 0;
  std::uint8_t disposition = 0;
  bool complete = false;
  std::uint32_t reserved = 0;
  return reader.u64(value) && reader.u64(value) &&
         reader.u8(disposition) && reader.u64(value) &&
         reader.u64(value) && reader.u64(value) && reader.u64(value) &&
         reader.u64(value) && reader.u64(value) && reader.u16(flags) &&
         reader.boolean(complete) && reader.u16(schema) &&
         reader.u32(reserved);
}

inline bool read_candidate_partition_record(canonical_reader &reader) {
  std::uint64_t value = 0;
  std::uint16_t schema = 0;
  std::uint32_t reserved = 0;
  for (std::size_t i = 0; i < 11; ++i)
    if (!reader.u64(value))
      return false;
  return reader.u16(schema) && reader.u32(reserved);
}

inline bool read_diagnostic_section(
    const std::vector<std::uint8_t> &bytes,
    const relation_capabilities &capabilities, std::uint16_t &schema,
    std::uint64_t &count, bounded_boolean_digest &digest) {
  canonical_reader reader(bytes);
  std::uint32_t magic = 0;
  if (!reader.u32(magic) || magic != 0x44375259U ||
      !reader.u16(schema) ||
      schema != contract_versions::relation_diagnostic_schema ||
      !reader.u64(count) || count > capabilities.maximum_diagnostics)
    return false;
  for (std::uint64_t ordinal = 0; ordinal < count; ++ordinal) {
    std::uint64_t id = 0, value64 = 0;
    std::uint32_t checkpoint = 0, subcode = 0, reserved32 = 0;
    std::uint16_t value16 = 0, reserved16 = 0;
    std::uint8_t kind = 0, severity = 0, value8 = 0;
    bool flag = false;
    if (!reader.u64(id) || id != ordinal || !reader.u8(kind) ||
        kind < static_cast<std::uint8_t>(
                   relation_diagnostic_kind::owner_exclusion_audit) ||
        kind > static_cast<std::uint8_t>(
                   relation_diagnostic_kind::maximum_exact_capacity) ||
        !reader.u8(severity) ||
        severity < static_cast<std::uint8_t>(
                       relation_diagnostic_severity::retained_finding) ||
        severity > static_cast<std::uint8_t>(
                       relation_diagnostic_severity::failure) ||
        !reader.u32(checkpoint) ||
        checkpoint < static_cast<std::uint32_t>(
                         relation_checkpoint::context_policy_capability_validation) ||
        checkpoint > static_cast<std::uint32_t>(
                         relation_checkpoint::transaction_commit) ||
        !reader.u32(subcode))
      return false;
    for (std::size_t i = 0; i < 4; ++i)
      if (!reader.boolean(flag))
        return false;
    if (!reader.u64(value64) || !reader.u64(value64) ||
        !read_feature_key(reader) || !read_feature_key(reader))
      return false;
    for (std::size_t i = 0; i < 4; ++i)
      if (!reader.u64(value64))
        return false;
    for (std::size_t i = 0; i < 3; ++i)
      if (!reader.u8(value8))
        return false;
    if (!reader.u16(value16) || !reader.u16(value16) ||
        !reader.u64(value64) || !reader.u8(value8))
      return false;
    for (std::size_t i = 0; i < 4; ++i)
      if (!reader.u64(value64))
        return false;
    bounded_boolean_digest record_digest{};
    if (!reader.u16(value16) ||
        value16 != contract_versions::relation_diagnostic_schema ||
        !reader.u16(reserved16) || reserved16 != 0 ||
        !reader.u32(reserved32) || reserved32 != 0 ||
        !read_digest(reader, record_digest))
      return false;
  }
  if (!reader.complete())
    return false;
  digest = sha256::digest(bytes);
  return true;
}

inline bool read_replay_checkpoint_section(
    const std::vector<std::uint8_t> &bytes,
    const relation_capabilities &capabilities, std::uint16_t &schema,
    std::uint64_t &count, bounded_boolean_digest &digest) {
  canonical_reader reader(bytes);
  std::uint32_t magic = 0;
  if (!reader.u32(magic) || magic != 0x43375259U ||
      !reader.u16(schema) ||
      schema != contract_versions::relation_replay_checkpoint_schema ||
      !reader.u64(count) || count > capabilities.maximum_replay_checkpoints)
    return false;
  std::uint64_t previous_work = 0;
  for (std::uint64_t ordinal = 0; ordinal < count; ++ordinal) {
    std::uint64_t id = 0, input_count = 0, output_count = 0, work = 0;
    std::uint32_t checkpoint = 0, reserved32 = 0;
    std::uint16_t record_schema = 0, reserved16 = 0;
    std::uint8_t status = 0;
    bounded_boolean_digest record_digest{};
    if (!reader.u64(id) || id != ordinal || !reader.u32(checkpoint) ||
        checkpoint < static_cast<std::uint32_t>(
                         relation_checkpoint::context_policy_capability_validation) ||
        checkpoint > static_cast<std::uint32_t>(
                         relation_checkpoint::transaction_commit) ||
        !reader.u8(status) ||
        status < static_cast<std::uint8_t>(
                     relation_replay_checkpoint_status::completed) ||
        status > static_cast<std::uint8_t>(
                     relation_replay_checkpoint_status::cancelled) ||
        !reader.u64(input_count) || !reader.u64(output_count) ||
        !reader.u64(work) || work < previous_work ||
        !reader.u16(record_schema) ||
        record_schema != contract_versions::relation_replay_checkpoint_schema ||
        !reader.u16(reserved16) || reserved16 != 0 ||
        !reader.u32(reserved32) || reserved32 != 0 ||
        !read_digest(reader, record_digest))
      return false;
    previous_work = work;
  }
  if (!reader.complete())
    return false;
  digest = sha256::digest(bytes);
  return true;
}

inline bool read_replay_evidence_section(
    const std::vector<std::uint8_t> &bytes, std::uint16_t &schema,
    std::uint16_t &policy, std::uint64_t &checkpoint_count,
    std::uint64_t &diagnostic_count,
    const bounded_boolean_digest &expected_checkpoint_digest,
    const bounded_boolean_digest &expected_diagnostic_digest,
    bounded_boolean_digest &section_digest) {
  canonical_reader reader(bytes);
  std::uint32_t magic = 0, reserved32 = 0;
  bounded_boolean_digest input_digest{}, checkpoint_digest{},
      diagnostic_digest{}, base_digest{}, semantic_digest{};
  bool complete = false, reconstructed = false, primary_failure = false;
  std::uint8_t reserved8 = 0;
  if (!reader.u32(magic) || magic != 0x45375259U ||
      !reader.u16(schema) ||
      schema != contract_versions::relation_replay_evidence_schema ||
      !reader.u16(policy) || policy != contract_versions::relation_replay_policy ||
      !read_digest(reader, input_digest) ||
      !read_digest(reader, checkpoint_digest) ||
      !read_digest(reader, diagnostic_digest) ||
      !read_digest(reader, base_digest) ||
      !reader.u64(checkpoint_count) || !reader.u64(diagnostic_count) ||
      !reader.boolean(complete) || !reader.boolean(reconstructed) ||
      !reader.boolean(primary_failure) || !reader.u8(reserved8) ||
      reserved8 != 0 || !read_digest(reader, semantic_digest) ||
      !reader.u32(reserved32) || reserved32 != 0 || !reader.complete() ||
      !complete || !reconstructed || primary_failure ||
      checkpoint_digest != expected_checkpoint_digest ||
      diagnostic_digest != expected_diagnostic_digest)
    return false;
  section_digest = sha256::digest(bytes);
  return true;
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

  if (bytes.size() < 12 + 10 * 48 + 32)
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 framed artifact is truncated");
  canonical_reader frame_reader(bytes);
  std::uint32_t frame_magic = 0, frame_reserved = 0;
  std::uint16_t frame_codec = 0, frame_count = 0;
  if (!frame_reader.u32(frame_magic) || !frame_reader.u16(frame_codec) ||
      !frame_reader.u16(frame_count) || !frame_reader.u32(frame_reserved) ||
      frame_magic != 0x46375259U ||
      frame_codec != contract_versions::relation_codec ||
      frame_count != envelope.section_digests.size() || frame_reserved != 0)
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 framed artifact header is malformed");
  std::vector<std::uint8_t> semantic_bytes;
  semantic_bytes.reserve(bytes.size());
  for (std::size_t i = 0; i < envelope.section_digests.size(); ++i) {
    auto &record = envelope.section_digests[i];
    std::uint16_t domain = 0;
    std::vector<std::uint8_t> payload;
    if (!frame_reader.u16(domain) ||
        !frame_reader.u16(record.layout_version) ||
        !frame_reader.u32(record.reserved32) ||
        !read_digest(frame_reader, record.digest) ||
        !frame_reader.sized_bytes(payload, capabilities.maximum_canonical_bytes) ||
        domain != i + 1 ||
        record.layout_version !=
            contract_versions::relation_section_digest_layout ||
        record.reserved32 != 0 || sha256::digest(payload) != record.digest)
      return codec_failure(relation_subcode::digest_mismatch,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 framed section digest mismatch");
    record.domain = static_cast<relation_section_domain>(domain);
    std::uint64_t combined = 0;
    if (!checked_add<std::uint64_t>(semantic_bytes.size(), payload.size(),
                                    combined) ||
        combined > capabilities.maximum_canonical_bytes)
      return codec_failure(relation_subcode::resource_preflight,
                           bounded_boolean_error_category::resource_limit,
                           "Component 07 framed section bytes exceed limit");
    semantic_bytes.insert(semantic_bytes.end(), payload.begin(), payload.end());
  }
  bounded_boolean_digest framed_digest{};
  if (!read_digest(frame_reader, framed_digest) || !frame_reader.complete())
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 framed artifact has trailing or missing bytes");
  sha256 framed_hash;
  framed_hash.update(bytes.data(), bytes.size() - framed_digest.bytes.size());
  if (framed_hash.finish() != framed_digest)
    return codec_failure(relation_subcode::digest_mismatch,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 complete framed artifact digest mismatch");

  canonical_reader reader(semantic_bytes);
  std::uint32_t magic = 0;
  std::uint8_t provider = 0, verification = 0, operation = 0;
  if (!reader.u32(magic) || magic != 0x37465259U ||
      !reader.u16(envelope.schema_version) ||
      !reader.u16(envelope.provider_version) ||
      !reader.u16(envelope.graph_policy_version) ||
      !reader.u16(envelope.truth_policy_version) ||
      !reader.u16(envelope.codec_version) ||
      !reader.u16(envelope.verifier_version) || !reader.u8(provider) ||
      !reader.u8(verification))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 encoded artifact header is malformed");
  if (!read_digest(reader, envelope.context_digest) ||
      !read_digest(reader, envelope.precision_digest) ||
      !read_digest(reader, envelope.candidate_digest) ||
      !read_digest(reader, envelope.graph_digest) || reader.u8(operation) == false ||
      !reader.floating(envelope.residual_boundary) ||
      !read_digest(reader, envelope.symbolic_policy_digest))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 encoded artifact header is malformed");
  std::uint16_t commitment_schema = 0, commitment_count = 0;
  if (!reader.u16(commitment_schema) || !reader.u16(commitment_count) ||
      commitment_schema !=
          contract_versions::relation_predecessor_commitment_schema ||
      commitment_count != envelope.predecessor_commitments.size())
    return codec_failure(relation_subcode::unsupported_version,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 predecessor commitment section is unsupported");
  for (std::size_t i = 0; i < envelope.predecessor_commitments.size(); ++i) {
    auto &record = envelope.predecessor_commitments[i];
    std::uint8_t component = 0;
    if (!reader.u8(component) || !reader.u16(record.schema_version) ||
        !reader.u16(record.provider_version) ||
        !reader.u16(record.policy_version) ||
        !reader.u16(record.codec_version) ||
        !reader.u16(record.verifier_version) ||
        !reader.boolean(record.independently_verified) ||
        !reader.u8(record.reserved8) ||
        !read_digest(reader, record.artifact_digest_a) ||
        !read_digest(reader, record.artifact_digest_b) ||
        !read_digest(reader, record.semantic_digest_a) ||
        !read_digest(reader, record.semantic_digest_b) ||
        !read_digest(reader, record.exact_digest_a) ||
        !read_digest(reader, record.exact_digest_b) ||
        !read_digest(reader, record.policy_digest) ||
        !reader.u16(record.commitment_schema) ||
        !reader.u16(record.reserved16) || !reader.u32(record.reserved32) ||
        component != i + 1 || !record.independently_verified ||
        record.commitment_schema != commitment_schema ||
        record.reserved8 != 0 || record.reserved16 != 0 ||
        record.reserved32 != 0)
      return codec_failure(relation_subcode::predecessor_mismatch,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 predecessor commitment payload is malformed");
    record.component = static_cast<relation_predecessor_component>(component);
  }

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

  std::vector<std::uint8_t> execution_authority_section;
  if (!reader.sized_bytes(execution_authority_section,
                          capabilities.maximum_canonical_bytes))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 execution-authority section is malformed");
  envelope.execution_authority_digest =
      sha256::digest(execution_authority_section);

  std::vector<std::uint8_t> graph_section;
  if (!reader.sized_bytes(graph_section, capabilities.maximum_canonical_bytes))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 request-graph section is malformed");
  envelope.graph_section_digest = sha256::digest(graph_section);

  if (!reader.u64(envelope.imported_geometry_count) ||
      !count_fits(reader, envelope.imported_geometry_count,
                  capabilities.maximum_relations, 48))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 imported geometry count is malformed");
  for (std::uint64_t i = 0; i < envelope.imported_geometry_count; ++i)
    if (!read_imported_geometry_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 imported geometry table is truncated");

  if (!reader.u64(envelope.bounded_primitive_count) ||
      !count_fits(reader, envelope.bounded_primitive_count,
                  capabilities.maximum_dependencies, 46))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 bounded primitive count is malformed");
  for (std::uint64_t i = 0; i < envelope.bounded_primitive_count; ++i)
    if (!read_bounded_primitive_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 bounded primitive table is truncated");

  if (!reader.u64(envelope.exact_relation_count) ||
      !count_fits(reader, envelope.exact_relation_count,
                  capabilities.maximum_dependencies, 37))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 exact relation count is malformed");
  for (std::uint64_t i = 0; i < envelope.exact_relation_count; ++i)
    if (!read_exact_relation_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 exact relation table is truncated");

  if (!reader.u64(envelope.truth_lineage_count) ||
      !count_fits(reader, envelope.truth_lineage_count,
                  capabilities.maximum_dependencies, 44))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 truth lineage count is malformed");
  for (std::uint64_t i = 0; i < envelope.truth_lineage_count; ++i)
    if (!read_truth_lineage_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 truth lineage table is truncated");

  if (!reader.u16(envelope.interval_evidence_schema) ||
      !reader.u16(envelope.source_facet_region_schema) ||
      envelope.interval_evidence_schema !=
          contract_versions::relation_interval_evidence_schema ||
      envelope.source_facet_region_schema !=
          contract_versions::relation_source_facet_region_publication_schema)
    return codec_failure(relation_subcode::unsupported_version,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 family-04 evidence section version is unsupported");

  if (!reader.u64(envelope.interval_evidence_count) ||
      !count_fits(reader, envelope.interval_evidence_count,
                  capabilities.maximum_interval_evidence, 150))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 interval-evidence count is malformed");
  for (std::uint64_t i = 0; i < envelope.interval_evidence_count; ++i)
    if (!read_interval_evidence_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 interval-evidence table is truncated");

  if (!reader.u64(envelope.source_facet_region_count) ||
      !count_fits(reader, envelope.source_facet_region_count,
                  capabilities.maximum_region_records, 140))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 source-facet-region count is malformed");
  for (std::uint64_t i = 0; i < envelope.source_facet_region_count; ++i)
    if (!read_source_facet_region_record<T>(reader, capabilities))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 source-facet-region table is truncated");

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

  if (!reader.u16(envelope.construction_schema) ||
      !reader.u16(envelope.construction_registry_policy) ||
      !reader.u16(envelope.construction_ledger_schema) ||
      envelope.construction_schema !=
          contract_versions::relation_construction_schema ||
      envelope.construction_registry_policy !=
          contract_versions::relation_construction_registry_policy ||
      envelope.construction_ledger_schema !=
          contract_versions::relation_construction_ledger_schema)
    return codec_failure(relation_subcode::unsupported_version,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 construction section version is unsupported");
  if (!reader.u64(envelope.construction_count) ||
      !count_fits(reader, envelope.construction_count,
                  capabilities.maximum_constructions, 320))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 construction table count is malformed");
  for (std::uint64_t i = 0; i < envelope.construction_count; ++i)
    if (!read_construction_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 construction table is truncated");
  if (!reader.u64(envelope.construction_ledger_count) ||
      !count_fits(reader, envelope.construction_ledger_count,
                  capabilities.maximum_construction_ledger, 260))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 construction-ledger count is malformed");
  for (std::uint64_t i = 0; i < envelope.construction_ledger_count; ++i)
    if (!read_construction_ledger_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 construction ledger is truncated");

  if (!reader.u16(envelope.coplanar_topology_schema) ||
      !reader.u16(envelope.coplanar_topology_policy) ||
      envelope.coplanar_topology_schema !=
          contract_versions::relation_coplanar_topology_schema ||
      envelope.coplanar_topology_policy !=
          contract_versions::relation_coplanar_topology_policy)
    return codec_failure(relation_subcode::unsupported_version,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 coplanar topology section version is unsupported");
  if (!reader.u64(envelope.coplanar_event_node_count) ||
      !count_fits(reader, envelope.coplanar_event_node_count,
                  capabilities.maximum_relations, 32))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 coplanar event-node count is malformed");
  for (std::uint64_t i = 0; i < envelope.coplanar_event_node_count; ++i)
    if (!read_coplanar_event_node_record(reader, capabilities))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 coplanar event-node table is truncated");

  if (!reader.u64(envelope.coplanar_oriented_arc_count) ||
      !count_fits(reader, envelope.coplanar_oriented_arc_count,
                  capabilities.maximum_relations, 41))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 coplanar oriented-arc count is malformed");
  for (std::uint64_t i = 0; i < envelope.coplanar_oriented_arc_count; ++i)
    if (!read_coplanar_oriented_arc_record(reader, capabilities))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 coplanar oriented-arc table is truncated");

  if (!reader.u64(envelope.coplanar_overlap_component_count) ||
      !count_fits(reader, envelope.coplanar_overlap_component_count,
                  capabilities.maximum_relations, 33))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 coplanar component count is malformed");
  for (std::uint64_t i = 0;
       i < envelope.coplanar_overlap_component_count; ++i)
    if (!read_coplanar_overlap_component_record(reader, capabilities))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 coplanar component table is truncated");

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

  if (!reader.u64(envelope.event_seed_candidate_incidence_count) ||
      !count_fits(reader, envelope.event_seed_candidate_incidence_count,
                  capabilities.maximum_event_seed_incidence, 104))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 event-seed candidate incidence count is malformed");
  for (std::uint64_t i = 0;
       i < envelope.event_seed_candidate_incidence_count; ++i)
    if (!read_event_seed_candidate_incidence_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 event-seed candidate incidence table is truncated");

  std::vector<std::uint8_t> reconciliation_section;
  if (!reader.sized_bytes(reconciliation_section,
                          capabilities.maximum_canonical_bytes))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 triangle-local reconciliation section is malformed");
  envelope.triangle_local_reconciliation_digest =
      sha256::digest(reconciliation_section);
  {
    canonical_reader reconciliation_reader(reconciliation_section);
    std::uint16_t schema = 0;
    if (!reconciliation_reader.u16(schema) ||
        schema !=
            contract_versions::relation_triangle_local_publication_schema ||
        !reconciliation_reader.u64(
            envelope.triangle_local_reconciliation_count) ||
        envelope.triangle_local_reconciliation_count >
            capabilities.maximum_relations)
      return codec_failure(
          relation_subcode::codec_error,
          bounded_boolean_error_category::input_contract_error,
          "Component 07 triangle-local reconciliation header is malformed");
  }

  std::uint16_t transverse_schema = 0;
  if (!reader.u16(transverse_schema) ||
      transverse_schema !=
          contract_versions::relation_transverse_carrier_membership_schema ||
      !reader.u64(envelope.transverse_carrier_membership_count) ||
      !count_fits(reader, envelope.transverse_carrier_membership_count,
                  capabilities.maximum_transverse_memberships, 137))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 transverse membership header is malformed");
  for (std::uint64_t i = 0;
       i < envelope.transverse_carrier_membership_count; ++i)
    if (!read_transverse_membership_record(reader, i))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                            "Component 07 transverse membership table is truncated");

  std::vector<std::uint8_t> downstream_handoff;
  if (!reader.sized_bytes(downstream_handoff,
                          capabilities.maximum_canonical_bytes) ||
      downstream_handoff.empty())
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 downstream handoff section is malformed");

  if (!reader.u64(envelope.candidate_disposition_count) ||
      !count_fits(reader, envelope.candidate_disposition_count,
                  capabilities.maximum_relations, 71))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 candidate disposition count is malformed");
  for (std::uint64_t i = 0; i < envelope.candidate_disposition_count; ++i)
    if (!read_candidate_disposition_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 candidate disposition table is truncated");

  if (!reader.u64(envelope.candidate_relation_coverage_count) ||
      !count_fits(reader, envelope.candidate_relation_coverage_count,
                  capabilities.maximum_candidate_coverage, 8))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 candidate relation coverage count is malformed");
  for (std::uint64_t i = 0; i < envelope.candidate_relation_coverage_count; ++i) {
    std::uint64_t value = 0;
    if (!reader.u64(value))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 candidate relation coverage is truncated");
  }
  if (!reader.u64(envelope.candidate_seed_coverage_count) ||
      !count_fits(reader, envelope.candidate_seed_coverage_count,
                  capabilities.maximum_candidate_coverage, 8))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 candidate seed coverage count is malformed");
  for (std::uint64_t i = 0; i < envelope.candidate_seed_coverage_count; ++i) {
    std::uint64_t value = 0;
    if (!reader.u64(value))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 candidate seed coverage is truncated");
  }
  if (!reader.u64(envelope.candidate_partition_count) ||
      !count_fits(reader, envelope.candidate_partition_count,
                  capabilities.maximum_relations, 92))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 candidate partition count is malformed");
  for (std::uint64_t i = 0; i < envelope.candidate_partition_count; ++i)
    if (!read_candidate_partition_record(reader))
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 candidate partition table is truncated");

  auto &statistics = envelope.statistics;
  if (!reader.u64(statistics.candidate_count) ||
      !reader.u64(statistics.request_proposal_count) ||
      !reader.u64(statistics.unique_request_count) ||
      !reader.u64(statistics.dependency_count) ||
      !reader.u64(statistics.reverse_consumer_count) ||
      !reader.u64(statistics.candidate_witness_count) ||
      !reader.u64(statistics.imported_geometry_count) ||
      !reader.u64(statistics.bounded_primitive_count) ||
      !reader.u64(statistics.exact_relation_count) ||
      !reader.u64(statistics.truth_lineage_count) ||
      !reader.u64(statistics.interval_evidence_count) ||
      !reader.u64(statistics.source_facet_region_count) ||
      !reader.u64(statistics.public_relation_count) ||
      !reader.u64(statistics.bookkeeping_relation_count) ||
      !reader.u64(statistics.construction_count) ||
      !reader.u64(statistics.construction_ledger_count) ||
      !reader.u64(statistics.coplanar_event_node_count) ||
      !reader.u64(statistics.coplanar_oriented_arc_count) ||
      !reader.u64(statistics.coplanar_overlap_component_count) ||
      !reader.u64(statistics.symbolic_eligibility_count) ||
      !reader.u64(statistics.symbolic_decision_count) ||
      !reader.u64(statistics.crossing_record_count) ||
      !reader.u64(statistics.event_seed_count) ||
      !reader.u64(statistics.event_seed_candidate_incidence_count) ||
      !reader.u64(statistics.candidate_relation_coverage_count) ||
      !reader.u64(statistics.candidate_seed_coverage_count) ||
       !reader.u64(statistics.candidate_partition_count) ||
       !reader.u64(statistics.triangle_local_reconciliation_count) ||
       !reader.u64(statistics.transverse_carrier_membership_count) ||
      !reader.u64(statistics.diagnostic_count) ||
      !reader.u64(statistics.replay_checkpoint_count) ||
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
      !reader.u32(evidence_reserved))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 verifier evidence is malformed");

  std::uint16_t resource_schema = 0, resource_count = 0;
  if (!reader.u16(resource_schema) || !reader.u16(resource_count) ||
      resource_schema != contract_versions::relation_resource_evidence_schema ||
      resource_count != 17)
    return codec_failure(relation_subcode::unsupported_version,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 resource evidence section is unsupported");
  envelope.resource_evidence_count = resource_count;
  for (std::uint64_t i = 0; i < resource_count; ++i) {
    std::uint64_t id = 0, required = 0, reserved = 0, closed = 0, used = 0,
                  witness = 0;
    std::uint8_t domain = 0, resource = 0, reserved8 = 0;
    std::uint16_t schema = 0;
    std::uint32_t reserved32 = 0;
    bool exact = false;
    if (!reader.u64(id) || !reader.u8(domain) || !reader.u8(resource) ||
        !reader.u64(required) || !reader.u64(reserved) ||
        !reader.u64(closed) || !reader.u64(used) ||
        !reader.u64(witness) || !reader.boolean(exact) ||
        !reader.u8(reserved8) || !reader.u16(schema) ||
        !reader.u32(reserved32) || id != i || domain != i + 1 ||
        resource >= static_cast<std::uint8_t>(resource_kind::count) ||
        reserved != required || used > required ||
        schema != resource_schema || reserved8 != 0 || reserved32 != 0)
      return codec_failure(relation_subcode::codec_error,
                           bounded_boolean_error_category::input_contract_error,
                           "Component 07 resource evidence payload is malformed");
  }

  std::vector<std::uint8_t> diagnostic_bytes, checkpoint_bytes, replay_bytes;
  if (!reader.sized_bytes(diagnostic_bytes,
                          capabilities.maximum_canonical_bytes) ||
      !reader.sized_bytes(checkpoint_bytes,
                          capabilities.maximum_canonical_bytes) ||
      !reader.sized_bytes(replay_bytes,
                          capabilities.maximum_canonical_bytes) ||
      !reader.complete() ||
      !read_diagnostic_section(
          diagnostic_bytes, capabilities, envelope.diagnostic_schema,
          envelope.diagnostic_count, envelope.diagnostic_digest) ||
      !read_replay_checkpoint_section(
          checkpoint_bytes, capabilities, envelope.replay_checkpoint_schema,
          envelope.replay_checkpoint_count,
          envelope.replay_checkpoint_digest) ||
      !read_replay_evidence_section(
          replay_bytes, envelope.replay_evidence_schema,
          envelope.replay_policy_version, envelope.replay_checkpoint_count,
          envelope.diagnostic_count, envelope.replay_checkpoint_digest,
          envelope.diagnostic_digest, envelope.replay_evidence_digest))
    return codec_failure(relation_subcode::codec_error,
                         bounded_boolean_error_category::input_contract_error,
                         "Component 07 replay or diagnostic section is malformed");

  if (statistics.candidate_count != envelope.candidate_disposition_count ||
      statistics.imported_geometry_count != envelope.imported_geometry_count ||
      statistics.bounded_primitive_count != envelope.bounded_primitive_count ||
      statistics.exact_relation_count != envelope.exact_relation_count ||
      statistics.truth_lineage_count != envelope.truth_lineage_count ||
      statistics.interval_evidence_count != envelope.interval_evidence_count ||
      statistics.source_facet_region_count !=
          envelope.source_facet_region_count ||
      statistics.public_relation_count + statistics.bookkeeping_relation_count !=
          envelope.relation_count ||
      statistics.construction_count != envelope.construction_count ||
      statistics.construction_ledger_count !=
          envelope.construction_ledger_count ||
      statistics.coplanar_event_node_count !=
          envelope.coplanar_event_node_count ||
      statistics.coplanar_oriented_arc_count !=
          envelope.coplanar_oriented_arc_count ||
      statistics.coplanar_overlap_component_count !=
          envelope.coplanar_overlap_component_count ||
      statistics.symbolic_eligibility_count !=
          envelope.symbolic_eligibility_count ||
      statistics.symbolic_decision_count != envelope.symbolic_decision_count ||
      statistics.crossing_record_count != envelope.crossing_count ||
      statistics.event_seed_count != envelope.event_seed_count ||
      statistics.event_seed_candidate_incidence_count !=
          envelope.event_seed_candidate_incidence_count ||
      statistics.candidate_relation_coverage_count !=
          envelope.candidate_relation_coverage_count ||
      statistics.candidate_seed_coverage_count !=
          envelope.candidate_seed_coverage_count ||
      statistics.candidate_partition_count !=
          envelope.candidate_partition_count ||
      statistics.triangle_local_reconciliation_count !=
          envelope.triangle_local_reconciliation_count ||
      statistics.transverse_carrier_membership_count !=
          envelope.transverse_carrier_membership_count ||
      statistics.diagnostic_count != envelope.diagnostic_count ||
      statistics.replay_checkpoint_count !=
          envelope.replay_checkpoint_count ||
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
  auto reconstructed = artifact;
  if (!refresh_relation_section_digests(reconstructed) ||
      reconstructed.section_digests_ != artifact.section_digests_) {
    error = relation_error(relation_subcode::digest_mismatch,
                           bounded_boolean_error_category::internal_invariant_error,
                           "Component 07 section digest table mismatch",
                           relation_checkpoint::canonical_encoding);
    return false;
  }
  const auto encoded = encode_signed_feature_relations(reconstructed);
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

template bool refresh_relation_section_digests<float, std::uint32_t>(
    signed_feature_relations<float, std::uint32_t> &) noexcept;
template bool refresh_relation_section_digests<float, std::uint64_t>(
    signed_feature_relations<float, std::uint64_t> &) noexcept;
template bool refresh_relation_section_digests<double, std::uint32_t>(
    signed_feature_relations<double, std::uint32_t> &) noexcept;
template bool refresh_relation_section_digests<double, std::uint64_t>(
    signed_feature_relations<double, std::uint64_t> &) noexcept;

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
