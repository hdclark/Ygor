#pragma once

#include "CanonicalCandidateStream.h"
#include "ConstructionConditioning.h"
#include "RelationRequestGraph.h"
#include "RelationExecutionAuthorityTypes.h"
#include "SourceFacetRegionKernel.h"
#include "SymbolicPerturbation.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T> struct candidate_source_edge_relation_stage;
template <class T> struct candidate_source_edge_facet_relation_stage;
template <class T> struct source_vertex_facet_evaluated_stage;
template <class T> struct candidate_source_facet_relation_stage;
template <class T> struct candidate_coplanar_overlay_stage;
template <class T, class I> class relation_artifact_assembler;
struct relation_artifact_test_access;
struct relation_artifact_internal_access;

struct relation_imported_geometry_record final {
  relation_imported_geometry_id id{0};
  relation_request_id producer{0};
  relation_feature_key feature{};
  relation_record_scope scope = relation_record_scope::public_source_feature;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_truth_record final {
  std::uint64_t rounded_nominal_bits = 0;
  std::uint64_t lower_bits = 0;
  std::uint64_t upper_bits = 0;
  std::uint64_t separation_margin_bits = 0;
  std::uint64_t uncertainty_width_bits = 0;
  std::array<std::uint64_t, 8> contributor_bits{};
  std::uint64_t bounded_value = 0;
  std::uint64_t source_provenance = 0;
  std::uint64_t geometric_lineage = 0;
  std::uint64_t precision_ledger_entry = 0;
  std::uint64_t trace_root = 0;
  std::uint64_t exact_evidence = 0;
  std::uint64_t exact_trace_root = 0;
  std::array<std::uint64_t, 24> exact_ordered_inputs{};
  bounded_sign_status bounded_sign = bounded_sign_status::invalid;
  exact_relation_status exact_relation = exact_relation_status::unavailable;
  predicate_disposition disposition = predicate_disposition::fail_invalid;
  std::uint16_t rounded_formula = 0;
  std::uint16_t exact_formula = 0;
  std::uint16_t bounded_schema_version = 0;
  std::uint16_t bounded_provider_version = 0;
  std::uint16_t exact_schema_version = 0;
  std::uint16_t exact_ordered_input_count = 0;
  std::int32_t exact_normalization_exponent = 0;
  std::uint32_t exact_capacity_used = 0;
  std::uint32_t exact_capacity_limit = 0;
  bounded_publication_state bounded_publication =
      bounded_publication_state::transaction_local;
  bool alternate_formulation_available = false;
  std::uint16_t schema_version = contract_versions::relation_truth_policy;
  std::uint32_t reserved = 0;
};

struct relation_bounded_primitive_record final {
  relation_bounded_primitive_id id{0};
  relation_request_id producer{0};
  feature_relation_id source_relation{0};
  std::uint32_t truth_ordinal = 0;
  std::uint64_t rounded_nominal_bits = 0;
  bounded_sign_status bounded_sign = bounded_sign_status::invalid;
  predicate_disposition disposition = predicate_disposition::fail_invalid;
  std::uint16_t rounded_formula = 0;
  relation_truth_record evidence{};
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_exact_relation_record final {
  relation_exact_relation_id id{0};
  relation_request_id producer{0};
  feature_relation_id source_relation{0};
  std::uint32_t truth_ordinal = 0;
  exact_relation_status status = exact_relation_status::unavailable;
  std::uint16_t exact_formula = 0;
  relation_truth_record evidence{};
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_truth_lineage_record final {
  relation_truth_lineage_id id{0};
  feature_relation_id source_relation{0};
  std::uint32_t truth_ordinal = 0;
  relation_bounded_primitive_id bounded_primitive{0};
  relation_exact_relation_id exact_relation{0};
  bool has_exact_relation = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_interval_evidence_record final {
  relation_interval_evidence_id id{0};
  relation_request_id producer{0};
  feature_relation_id source_relation{0};
  relation_interval_evidence_kind kind =
      relation_interval_evidence_kind::source_edge_first_parameter;
  std::uint32_t occurrence = 0;
  std::uint8_t component = 0;
  bool has_rounded_nominal = false;
  bool has_parameter_metadata = false;
  bool within_authorized_boundary = false;
  std::uint64_t rounded_nominal_bits = 0;
  std::uint64_t lower_bits = 0;
  std::uint64_t upper_bits = 0;
  parameter_domain_status domain = parameter_domain_status::invalid;
  std::uint64_t domain_margin_bits = 0;
  exact_relation_status exact_zero = exact_relation_status::unavailable;
  exact_relation_status exact_one = exact_relation_status::unavailable;
  std::array<std::uint64_t, 8> contributor_bits{};
  std::uint64_t trace_root = 0;
  rounded_operation_code issued_operation = rounded_operation_code::invalid;
  std::uint64_t issued_value = 0;
  std::uint64_t issued_ledger_entry = 0;
  std::vector<std::uint64_t> issued_parent_values;
  std::vector<std::uint64_t> issued_parent_trace_roots;
  std::vector<std::uint64_t> issued_parent_ledger_entries;
  std::vector<std::uint8_t> issued_operation_evidence;
  std::uint64_t comparison_boundary_bits = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T> struct relation_source_facet_region_record final {
  relation_source_facet_region_id id{0};
  relation_request_id producer{0};
  feature_relation_id source_relation{0};
  relation_source_facet_region_kind kind =
      relation_source_facet_region_kind::edge_facet_event;
  std::uint32_t occurrence = 0;
  std::uint8_t query_component_count = 0;
  bool query_source_identity_valid = false;
  std::array<std::uint64_t, 3> query_nominal_bits{};
  std::array<std::uint64_t, 3> query_lower_bits{};
  std::array<std::uint64_t, 3> query_upper_bits{};
  source_facet_point_region_record<T> region{};
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_construction_record final {
  relation_construction_id id{0};
  relation_request_id producer{0};
  feature_relation_id source_relation{0};
  relation_construction_kind kind = relation_construction_kind::bounded_point;
  relation_construction_precedence precedence =
      relation_construction_precedence::verification_witness;
  relation_construction_coordinate_space coordinate_space =
      relation_construction_coordinate_space::world_3d;
  relation_construction_compatibility_disposition compatibility =
      relation_construction_compatibility_disposition::authoritative;
  std::uint8_t component_count = 0;
  std::uint8_t projection_axis = 3;
  relation_feature_key authoritative_source_feature{};
  std::array<std::uint64_t, 6> nominal_bits{};
  std::array<std::uint64_t, 6> lower_bits{};
  std::array<std::uint64_t, 6> upper_bits{};
  std::uint64_t source_provenance = 0;
  std::uint64_t geometric_lineage = 0;
  std::array<relation_feature_key, 2> defining_features{};
  std::uint8_t defining_feature_count = 0;
  std::uint16_t formula_version = 0;
  std::uint16_t schema_version = contract_versions::relation_construction_schema;
  std::uint64_t defining_dependency_begin = 0;
  std::uint64_t defining_dependency_count = 0;
  std::uint64_t radial_error_upper_bits = 0;
  std::array<std::uint64_t, 6> axis_error_upper_bits{};
  std::uint64_t denominator_lower_bits = 0;
  std::uint64_t denominator_upper_bits = 0;
  std::uint64_t conditioning_lower_bits = 0;
  rounded_operation_code operation = rounded_operation_code::invalid;
  construction_category conditioning = construction_category::invalid;
  construction_tolerance_disposition tolerance =
      construction_tolerance_disposition::invalid;
  std::vector<std::uint64_t> ordered_bounded_inputs;
  std::vector<std::uint8_t> certificate_evidence;
  std::uint64_t precision_trace_root = 0;
  bool accepted_source_vertex = false;
  bool finite = false;
  bool tolerance_compatible = false;
  bool precision_evidence_complete = false;
  std::uint64_t tolerance_boundary_bits = 0;
  std::uint64_t residual_truth_begin = 0;
  std::uint64_t residual_truth_count = 0;
  std::uint64_t interval_evidence_begin = 0;
  std::uint64_t interval_evidence_count = 0;
  std::uint64_t source_facet_region_begin = 0;
  std::uint64_t source_facet_region_count = 0;
  std::uint64_t consumer_begin = 0;
  std::uint64_t consumer_count = 0;
  std::uint64_t ledger_begin = 0;
  std::uint64_t ledger_count = 0;
  std::uint32_t reserved = 0;
};

struct relation_construction_ledger_record final {
  relation_construction_ledger_id id{0};
  relation_construction_id construction{0};
  feature_relation_id source_relation{0};
  relation_construction_precedence precedence =
      relation_construction_precedence::verification_witness;
  relation_construction_coordinate_space coordinate_space =
      relation_construction_coordinate_space::world_3d;
  relation_construction_compatibility_disposition compatibility =
      relation_construction_compatibility_disposition::compatible_witness;
  std::uint8_t component_count = 0;
  std::uint8_t projection_axis = 3;
  std::uint32_t occurrence = 0;
  std::array<std::uint64_t, 6> nominal_bits{};
  std::array<std::uint64_t, 6> lower_bits{};
  std::array<std::uint64_t, 6> upper_bits{};
  std::uint64_t source_provenance = 0;
  std::uint64_t geometric_lineage = 0;
  std::array<relation_feature_key, 2> defining_features{};
  std::uint8_t defining_feature_count = 0;
  std::uint16_t formula_version = 0;
  std::uint16_t schema_version =
      contract_versions::relation_construction_ledger_schema;
  std::uint64_t precision_trace_root = 0;
  std::array<std::uint64_t, 6> axis_error_upper_bits{};
  std::uint64_t radial_error_upper_bits = 0;
  std::uint64_t denominator_lower_bits = 0;
  std::uint64_t denominator_upper_bits = 0;
  std::uint64_t conditioning_lower_bits = 0;
  rounded_operation_code operation = rounded_operation_code::invalid;
  construction_category conditioning = construction_category::invalid;
  construction_tolerance_disposition tolerance =
      construction_tolerance_disposition::invalid;
  std::vector<std::uint64_t> ordered_bounded_inputs;
  std::vector<std::uint8_t> certificate_evidence;
  bool accepted_source_vertex = false;
  bool finite = false;
  bool tolerance_compatible = false;
  bool authoritative_entry = false;
  bool lineage_compatible = false;
  bool enclosure_compatible = false;
  bool parameter_compatible = false;
  bool residual_compatible = false;
  bool precision_evidence_complete = false;
  std::uint64_t tolerance_boundary_bits = 0;
  std::uint64_t truth_begin = 0;
  std::uint64_t truth_count = 0;
  std::uint64_t interval_evidence_begin = 0;
  std::uint64_t interval_evidence_count = 0;
  std::uint64_t source_facet_region_begin = 0;
  std::uint64_t source_facet_region_count = 0;
  std::uint32_t reserved = 0;
};

struct feature_relation_record final {
  feature_relation_id id{0};
  relation_request_id producer{0};
  feature_relation_family family =
      feature_relation_family::source_vertex_source_facet;
  relation_record_scope scope = relation_record_scope::public_source_feature;
  feature_relation_status status = feature_relation_status::not_evaluated;
  std::uint64_t truth_begin = 0;
  std::uint64_t truth_count = 0;
  std::int32_t numeric_crossing_multiplicity = 0;
  std::uint32_t occurrence = 0;
  std::uint32_t reserved = 0;
};


struct relation_crossing_record final {
  feature_relation_id relation{0};
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
  std::uint32_t occurrence = 0;
  std::uint64_t source_fan_group = 0;
  std::uint32_t source_fan_group_size = 1;
  std::uint32_t source_fan_group_ordinal = 0;
  std::int8_t local_transition = 0;
  bool numeric_owner = false;
  bool source_fan_resolved = false;
  bool locally_conservative = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_event_seed_record final {
  relation_event_seed_id id{0};
  relation_event_seed_key key{};
  feature_relation_id source_relation{0};
  relation_construction_id construction{0};
  feature_relation_status contact_status =
      feature_relation_status::not_evaluated;
  relation_contact_dimension contact_dimension = relation_contact_dimension::none;
  relation_construction_kind construction_kind =
      relation_construction_kind::bounded_point;
  relation_feature_key accepted_source_vertex{};
  bool accepted_source_vertex_reused = false;
  bool has_symbolic_decision = false;
  symbolic_relation_decision_id symbolic_decision{0};
  std::uint64_t symbolic_rule_ordinal = 0;
  std::uint64_t symbolic_exchange_rule_ordinal = 0;
  symbolic_relation_subject_kind symbolic_subject_kind =
      symbolic_relation_subject_kind::relation;
  std::uint64_t symbolic_subject_ordinal = 0;
  std::uint32_t symbolic_occurrence_rank = 0;
  symbolic_relation_side conceptual_side = symbolic_relation_side::coincident;
  symbolic_offset_disposition conceptual_order =
      symbolic_offset_disposition::coincident;
  symbolic_contact_class symbolic_contact =
      symbolic_contact_class::point_contact;
  symbolic_expected_disposition symbolic_expected =
      symbolic_expected_disposition::classification_only;
  symbolic_explanation_code symbolic_explanation =
      symbolic_explanation_code::exact_vertex_tie;
  std::uint16_t symbolic_tie_key_schema = 0;
  operand_id coincident_owner_rank = operand_id::a;
  bool symbolic_owner_rank_eligible = false;
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
  std::uint64_t truth_begin = 0;
  std::uint64_t truth_count = 0;
  std::uint64_t construction_ledger_begin = 0;
  std::uint64_t construction_ledger_count = 0;
  std::uint64_t incidence_begin = 0;
  std::uint64_t incidence_count = 0;
  std::uint64_t candidate_incidence_begin = 0;
  std::uint64_t candidate_incidence_count = 0;
  bool precision_evidence_complete = false;
  bool distinct_occurrence_required = false;
  std::uint16_t schema_version = contract_versions::relation_event_seed_schema;
  std::uint32_t reserved = 0;
};

struct relation_event_seed_candidate_incidence_record final {
  relation_event_seed_incidence_id id{0};
  relation_event_seed_id seed{0};
  candidate_id candidate{0};
  relation_candidate_disposition_id disposition{0};
  relation_feature_key candidate_edge{};
  relation_feature_key source_triangle{};
  std::array<std::uint64_t, 2> edge_halfedges{};
  std::array<std::uint64_t, 3> triangle_halfedges{};
  bool internal_diagonal_witness = false;
  bool source_feature_owner = false;
  std::uint16_t schema_version =
      contract_versions::relation_event_seed_incidence_schema;
  std::uint32_t reserved = 0;
};

struct relation_coplanar_event_lineage_record final {
  relation_request_id request{0};
  std::uint64_t contact_lineage = 0;
  std::uint8_t endpoint_role = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
};

struct relation_coplanar_event_occurrence_record final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t breakpoint_ordinal = 0;
  relation_feature_key source_edge{};
  bool query_source_vertex_valid = false;
  std::uint64_t query_source_vertex = 0;
  relation_feature_key endpoint_source_vertex{};
  std::vector<relation_coplanar_event_lineage_record> event_lineages;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_event_node_record final {
  relation_coplanar_event_node_id id{0};
  feature_relation_id overlay_relation{0};
  relation_construction_id representative{0};
  std::vector<relation_coplanar_event_occurrence_record> occurrences;
  std::uint8_t sheet_mask = 0;
  bool distinct_sheet_occurrences = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_arc_occurrence_record final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t interval_ordinal = 0;
  relation_coplanar_event_node_id start_node{0};
  relation_coplanar_event_node_id end_node{0};
  relation_feature_key source_edge{};
  std::uint64_t source_edge_lineage = 0;
  std::array<std::uint64_t, 2> endpoint_nominal_bits{};
  std::array<std::uint64_t, 2> endpoint_lower_bits{};
  std::array<std::uint64_t, 2> endpoint_upper_bits{};
  std::array<parameter_domain_status, 2> endpoint_domains{
      parameter_domain_status::invalid, parameter_domain_status::invalid};
  bool forward_along_source_edge = true;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_oriented_arc_record final {
  relation_coplanar_oriented_arc_id id{0};
  feature_relation_id overlay_relation{0};
  relation_coplanar_arc_kind kind =
      relation_coplanar_arc_kind::interior_boundary;
  relation_coplanar_event_node_id start_node{0};
  relation_coplanar_event_node_id end_node{0};
  std::vector<relation_coplanar_arc_occurrence_record> occurrences;
  std::vector<relation_request_id> overlap_lineages;
  std::array<relation_feature_key, 2> source_edge_pair{};
  std::uint8_t source_edge_count = 0;
  std::uint64_t arc_lineage = 0;
  std::uint8_t sheet_mask = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_overlap_component_record final {
  relation_coplanar_overlap_component_id id{0};
  feature_relation_id overlay_relation{0};
  relation_coplanar_component_kind kind =
      relation_coplanar_component_kind::isolated_point;
  std::vector<relation_coplanar_event_node_id> node_ids;
  std::vector<relation_coplanar_oriented_arc_id> arc_ids;
  std::uint64_t component_lineage = 0;
  operand_id half_open_owner = operand_id::a;
  std::uint8_t sheet_mask = 0;
  bool closed = false;
  bool distinct_sheet_occurrences = false;
  bool zero_measure = true;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_partition_coverage_record final {
  relation_feature_key source_edge{};
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t breakpoint_count = 0;
  std::uint64_t interior_interval_count = 0;
  std::uint64_t outside_interval_count = 0;
  std::uint64_t original_edge_overlap_interval_count = 0;
  bool complete_boundary_contact_set = false;
  bool triangle_reconciliation_complete = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_coplanar_support_record final {
  relation_coplanar_support_id id{0};
  feature_relation_id overlay_relation{0};
  std::array<relation_feature_key, 2> support_facets{};
  relation_coplanar_orientation orientation =
      relation_coplanar_orientation::same;
  relation_coplanar_classification classification =
      relation_coplanar_classification::disjoint;
  std::array<occupied_side, 2> material_sides{
      occupied_side::negative, occupied_side::negative};
  operand_id half_open_owner = operand_id::a;
  std::uint64_t support_lineage = 0;
  std::array<std::vector<relation_feature_key>, 2> original_boundary_edges;
  std::vector<relation_coplanar_partition_coverage_record> partition_coverage;
  bool complete_boundary_pair_coverage = false;
  bool complete_vertex_coverage = false;
  bool complete_boundary_partition_coverage = false;
  bool complete_event_lineage = false;
  bool complete_authorized_arc_coverage = false;
  bool complete_overlap_component_assembly = false;
  bool distinct_sheet_occurrences = false;
  bool zero_measure = true;
  std::uint16_t schema_version =
      contract_versions::relation_coplanar_topology_schema;
  std::uint32_t reserved32 = 0;
};

struct relation_candidate_disposition_record final {
  relation_candidate_disposition_id id{0};
  candidate_id candidate{0};
  candidate_relation_disposition_kind disposition =
      candidate_relation_disposition_kind::primitive_dependency_only;
  feature_relation_id public_relation{0};
  relation_request_id bookkeeping_request{0};
  std::uint64_t relation_begin = 0;
  std::uint64_t relation_count = 0;
  std::uint64_t event_seed_begin = 0;
  std::uint64_t event_seed_count = 0;
  std::uint16_t coverage_flags = 0;
  bool coverage_complete = false;
  std::uint16_t schema_version =
      contract_versions::relation_candidate_disposition_schema;
  std::uint32_t reserved = 0;
};

struct relation_triangle_local_reconciliation_record final {
  relation_triangle_local_reconciliation_id id{0};
  candidate_id candidate{0};
  relation_request_id bookkeeping_request{0};
  relation_request_id public_composite_request{0};
  feature_relation_id public_relation{0};
  relation_feature_key discovery_edge{};
  relation_feature_key discovery_triangle{};
  relation_feature_key owning_source_facet{};
  relation_feature_key opposite_source_facet{};
  std::array<std::uint64_t, 2> edge_halfedges{};
  std::array<std::uint64_t, 3> triangle_halfedges{};
  triangle_local_reconciliation_disposition disposition =
      triangle_local_reconciliation_disposition::no_public_relation;
  triangle_local_no_public_reason no_public_reason =
      triangle_local_no_public_reason::not_applicable;
  bool internal_diagonal = false;
  bool source_feature_owner = false;
  bool symbolic_contact_owner = false;
  bool classification_barrier = false;
  bool retained_surface_feature = false;
  bool complete = false;
  std::uint16_t schema_version =
      contract_versions::relation_triangle_local_publication_schema;
  std::uint32_t reserved = 0;
};

struct relation_transverse_carrier_membership_record final {
  relation_transverse_carrier_membership_id id{0};
  feature_relation_id carrier_relation{0};
  relation_construction_id carrier_construction{0};
  feature_relation_id member_relation{0};
  relation_construction_id point_construction{0};
  relation_event_seed_id seed{0};
  std::uint32_t occurrence = 0;
  relation_interval_evidence_id parameter{0};
  std::array<relation_interval_evidence_id, 3> point_carrier_residuals{
      relation_interval_evidence_id{0}, relation_interval_evidence_id{0},
      relation_interval_evidence_id{0}};
  relation_source_facet_region_id first_region{0};
  relation_source_facet_region_id second_region{0};
  std::uint64_t parameter_lineage = 0;
  std::uint64_t carrier_lineage = 0;
  std::uint64_t event_lineage = 0;
  std::int32_t numeric_crossing = 0;
  std::int8_t local_transition = 0;
  relation_carrier_transition transition =
      relation_carrier_transition::tangent;
  operand_id half_open_owner = operand_id::a;
  bool numeric_owner = false;
  bool finite = false;
  bool conditioning_accepted = false;
  bool residuals_accepted = false;
  bool regions_complete = false;
  bool precision_evidence_complete = false;
  std::uint16_t schema_version =
      contract_versions::relation_transverse_carrier_membership_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_source_edge_domain_record final {
  relation_feature_key source_edge{};
  relation_feature_key start_vertex{};
  relation_feature_key end_vertex{};
  std::uint64_t canonical_edge = 0;
  std::uint16_t schema_version =
      contract_versions::relation_downstream_topology_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_source_vertex_fan_record final {
  relation_feature_key source_vertex{};
  std::vector<relation_feature_key> ordered_facets;
  std::uint64_t canonical_vertex = 0;
  std::uint16_t schema_version =
      contract_versions::relation_downstream_topology_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_source_edge_adjacency_record final {
  relation_feature_key edge{};
  relation_feature_key first_facet{};
  relation_feature_key second_facet{};
  std::uint64_t canonical_edge = 0;
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  bool source_feature_owner = false;
  bool bookkeeping_only = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version =
      contract_versions::relation_downstream_topology_schema;
  std::uint32_t reserved32 = 0;
};

struct relation_source_topology_record final {
  operand_id operand = operand_id::a;
  std::uint64_t source_triangle_count = 0;
  std::uint64_t canonical_edge_count = 0;
  bounded_boolean_digest source_semantic_digest{};
  bounded_boolean_digest exact_topology_digest{};
  std::vector<relation_source_edge_domain_record> source_edges;
  std::vector<relation_source_vertex_fan_record> vertex_fans;
  std::vector<relation_source_edge_adjacency_record> edge_adjacencies;
  std::uint16_t schema_version =
      contract_versions::relation_downstream_topology_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_transverse_carrier_support_record final {
  feature_relation_id relation{0};
  relation_construction_id construction{0};
  relation_feature_key first_facet{};
  relation_feature_key second_facet{};
  std::uint64_t expected_membership_count = 0;
  bool support_consistent = false;
  bool orientation_consistent = false;
  bool residuals_accepted = false;
  bool precision_evidence_complete = false;
  std::uint16_t schema_version =
      contract_versions::relation_transverse_support_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_candidate_partition_record final {
  relation_candidate_partition_id id{0};
  candidate_partition_id source_partition{0};
  std::uint64_t candidate_begin = 0;
  std::uint64_t candidate_count = 0;
  std::uint64_t disposition_begin = 0;
  std::uint64_t disposition_count = 0;
  std::uint64_t relation_begin = 0;
  std::uint64_t relation_count = 0;
  std::uint64_t event_seed_begin = 0;
  std::uint64_t event_seed_count = 0;
  std::uint64_t maximum_records = 0;
  std::uint16_t schema_version =
      contract_versions::relation_candidate_partition_schema;
  std::uint32_t reserved = 0;
};

struct relation_verification_evidence final {
  relation_verifier_evidence_id id{0};
  std::uint16_t verifier_version = contract_versions::relation_verifier;
  bool graph_reconstructed = false;
  bool owner_exclusion_checked = false;
  bool selection_boundary_checked = false;
  bool candidate_dispositions_complete = false;
  std::uint64_t verifier_work_units = 0;
  bounded_boolean_digest semantic_digest{};
  std::uint32_t reserved = 0;
};

struct relation_predecessor_commitment_record final {
  relation_predecessor_component component =
      relation_predecessor_component::component_01_context;
  std::uint16_t schema_version = 0;
  std::uint16_t provider_version = 0;
  std::uint16_t policy_version = 0;
  std::uint16_t codec_version = 0;
  std::uint16_t verifier_version = 0;
  bool independently_verified = false;
  std::uint8_t reserved8 = 0;
  bounded_boolean_digest artifact_digest_a{};
  bounded_boolean_digest artifact_digest_b{};
  bounded_boolean_digest semantic_digest_a{};
  bounded_boolean_digest semantic_digest_b{};
  bounded_boolean_digest exact_digest_a{};
  bounded_boolean_digest exact_digest_b{};
  bounded_boolean_digest policy_digest{};
  std::uint16_t commitment_schema =
      contract_versions::relation_predecessor_commitment_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_resource_evidence_record final {
  relation_resource_evidence_id id{0};
  relation_resource_domain domain = relation_resource_domain::requests;
  resource_kind resource = resource_kind::relation_request_records;
  std::uint64_t required_limit = 0;
  std::uint64_t preflight_reserved = 0;
  std::uint64_t closed_authority_bound = 0;
  std::uint64_t reconciled_used = 0;
  std::uint64_t witness_ordinal = relation_invalid_ordinal;
  bool closed_authority_exact = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version =
      contract_versions::relation_resource_evidence_schema;
  std::uint32_t reserved32 = 0;
};

struct relation_section_digest_record final {
  relation_section_domain domain =
      relation_section_domain::header_and_predecessors;
  std::uint16_t layout_version =
      contract_versions::relation_section_digest_layout;
  std::uint32_t reserved32 = 0;
  bounded_boolean_digest digest{};
  friend bool operator==(const relation_section_digest_record &a,
                         const relation_section_digest_record &b) noexcept {
    return a.domain == b.domain && a.layout_version == b.layout_version &&
           a.reserved32 == b.reserved32 && a.digest == b.digest;
  }
  friend bool operator!=(const relation_section_digest_record &a,
                         const relation_section_digest_record &b) noexcept {
    return !(a == b);
  }
};

struct relation_diagnostic_record final {
  relation_diagnostic_id id{0};
  relation_diagnostic_kind kind =
      relation_diagnostic_kind::replay_completeness_audit;
  relation_diagnostic_severity severity =
      relation_diagnostic_severity::retained_finding;
  relation_checkpoint checkpoint = relation_checkpoint::producer_verification;
  std::uint32_t subcode = 0;
  bool has_candidate = false;
  bool has_relation = false;
  bool has_source_features = false;
  bool has_numeric_evidence = false;
  std::uint64_t candidate_ordinal = relation_invalid_ordinal;
  std::uint64_t relation_ordinal = relation_invalid_ordinal;
  relation_feature_key first_feature{};
  relation_feature_key second_feature{};
  std::uint64_t rounded_nominal_bits = 0;
  std::uint64_t lower_bits = 0;
  std::uint64_t upper_bits = 0;
  std::uint64_t margin_bits = 0;
  bounded_sign_status bounded_sign = bounded_sign_status::invalid;
  exact_relation_status exact_relation = exact_relation_status::unavailable;
  predicate_disposition disposition = predicate_disposition::fail_invalid;
  std::uint16_t rounded_formula = 0;
  std::uint16_t exact_formula = 0;
  std::uint64_t trace_root = 0;
  resource_kind resource = resource_kind::diagnostic_findings;
  std::uint64_t resource_limit = 0;
  std::uint64_t resource_used = 0;
  std::uint64_t cancellation_progress = 0;
  relation_replay_checkpoint_id replay_checkpoint{0};
  bounded_boolean_digest semantic_digest{};
  std::uint16_t schema_version = contract_versions::relation_diagnostic_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_replay_checkpoint_record final {
  relation_replay_checkpoint_id id{0};
  relation_checkpoint checkpoint = relation_checkpoint::predecessor_validation;
  relation_replay_checkpoint_status status =
      relation_replay_checkpoint_status::completed;
  std::uint64_t input_count = 0;
  std::uint64_t output_count = 0;
  std::uint64_t cumulative_work_units = 0;
  bounded_boolean_digest semantic_digest{};
  std::uint16_t schema_version =
      contract_versions::relation_replay_checkpoint_schema;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct relation_replay_evidence final {
  std::uint16_t schema_version =
      contract_versions::relation_replay_evidence_schema;
  std::uint16_t policy_version = contract_versions::relation_replay_policy;
  bounded_boolean_digest input_equivalence_digest{};
  bounded_boolean_digest checkpoint_digest{};
  bounded_boolean_digest diagnostic_digest{};
  bounded_boolean_digest base_artifact_digest{};
  std::uint64_t checkpoint_count = 0;
  std::uint64_t diagnostic_count = 0;
  bool complete = false;
  bool artifact_reconstructed = false;
  bool primary_failure_present = false;
  std::uint8_t reserved8 = 0;
  bounded_boolean_digest semantic_digest{};
  std::uint32_t reserved32 = 0;
};

template <class T, class I> class signed_feature_relations final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t graph_policy_version() const noexcept {
    return graph_policy_version_;
  }
  std::uint16_t truth_policy_version() const noexcept {
    return truth_policy_version_;
  }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  relation_provider_kind provider() const noexcept { return provider_; }
  relation_verification_disposition verification() const noexcept {
    return verification_;
  }
  const context_owner_token &owner() const noexcept { return owner_; }
  const relation_request_graph &request_graph() const noexcept {
    return request_graph_;
  }
  const relation_execution_authority &execution_authority() const noexcept {
    return execution_authority_;
  }
  const std::vector<relation_imported_geometry_record> &
  imported_geometry() const noexcept { return imported_geometry_; }
  const std::vector<relation_bounded_primitive_record> &
  bounded_primitives() const noexcept { return bounded_primitives_; }
  const std::vector<relation_exact_relation_record> &
  exact_relations() const noexcept { return exact_relations_; }
  const std::vector<relation_truth_lineage_record> &
  truth_lineage() const noexcept { return truth_lineage_; }
  const std::vector<relation_interval_evidence_record> &
  interval_evidence() const noexcept { return interval_evidence_; }
  const std::vector<relation_source_facet_region_record<T>> &
  source_facet_regions() const noexcept { return source_facet_regions_; }
  const std::vector<relation_truth_record> &truth_records() const noexcept {
    return truth_records_;
  }

  const std::vector<feature_relation_record> &relations() const noexcept {
    return relations_;
  }
  const std::vector<relation_construction_record> &constructions() const noexcept {
    return constructions_;
  }
  const std::vector<relation_construction_ledger_record> &
  construction_ledger() const noexcept {
    return construction_ledger_;
  }
  const std::vector<symbolic_eligibility_record> &symbolic_eligibility()
      const noexcept {
    return symbolic_eligibility_;
  }
  const std::vector<symbolic_relation_decision_record> &symbolic_decisions()
      const noexcept {
    return symbolic_decisions_;
  }
  const std::vector<relation_crossing_record> &crossings() const noexcept {
    return crossings_;
  }
  const std::vector<relation_event_seed_record> &event_seeds() const noexcept {
    return event_seeds_;
  }
  const std::vector<relation_coplanar_event_node_record> &
  coplanar_event_nodes() const noexcept { return coplanar_event_nodes_; }
  const std::vector<relation_coplanar_oriented_arc_record> &
  coplanar_oriented_arcs() const noexcept { return coplanar_oriented_arcs_; }
  const std::vector<relation_coplanar_overlap_component_record> &
  coplanar_overlap_components() const noexcept {
    return coplanar_overlap_components_;
  }
  const std::vector<relation_coplanar_support_record> &
  coplanar_supports() const noexcept { return coplanar_supports_; }
  const std::vector<relation_feature_key> &event_seed_incidence() const noexcept {
    return event_seed_incidence_;
  }
  const std::vector<relation_event_seed_candidate_incidence_record> &
  event_seed_candidate_incidence() const noexcept {
    return event_seed_candidate_incidence_;
  }
  const std::vector<relation_candidate_disposition_record> &
  candidate_dispositions() const noexcept {
    return candidate_dispositions_;
  }
  const std::vector<relation_triangle_local_reconciliation_record> &
  triangle_local_reconciliation() const noexcept {
    return triangle_local_reconciliation_;
  }
  const std::vector<relation_transverse_carrier_membership_record> &
  transverse_carrier_memberships() const noexcept {
    return transverse_carrier_memberships_;
  }
  const std::array<relation_source_topology_record, 2> &
  source_topology() const noexcept { return source_topology_; }
  const std::vector<relation_transverse_carrier_support_record> &
  transverse_carrier_supports() const noexcept {
    return transverse_carrier_supports_;
  }
  const std::vector<feature_relation_id> &candidate_relation_coverage()
      const noexcept { return candidate_relation_coverage_; }
  const std::vector<relation_event_seed_id> &candidate_event_seed_coverage()
      const noexcept { return candidate_event_seed_coverage_; }
  const std::vector<relation_candidate_partition_record> &candidate_partitions()
      const noexcept { return candidate_partitions_; }
  const relation_statistics &statistics() const noexcept { return statistics_; }
  const relation_verification_evidence &verification_evidence() const noexcept {
    return verification_evidence_;
  }
  const std::vector<relation_diagnostic_record> &diagnostics() const noexcept {
    return diagnostics_;
  }
  const std::vector<relation_replay_checkpoint_record> &replay_checkpoints()
      const noexcept {
    return replay_checkpoints_;
  }
  const relation_replay_evidence &replay_evidence() const noexcept {
    return replay_evidence_;
  }
  const std::array<relation_predecessor_commitment_record, 6> &
  predecessor_commitments() const noexcept { return predecessor_commitments_; }
  const std::vector<relation_resource_evidence_record> &resource_evidence()
      const noexcept { return resource_evidence_; }
  const std::array<relation_section_digest_record, 10> &section_digests()
      const noexcept { return section_digests_; }
  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &candidate_digest() const noexcept {
    return candidate_digest_;
  }
  const bounded_boolean_digest &graph_digest() const noexcept {
    return graph_digest_;
  }
  boolean_operation operation() const noexcept { return operation_; }
  T residual_boundary() const noexcept { return residual_boundary_; }
  const bounded_boolean_digest &symbolic_policy_digest() const noexcept {
    return symbolic_policy_digest_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }

  const feature_relation_record *relation(
      feature_relation_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= relations_.size())
      return nullptr;
    return &relations_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ = contract_versions::relation_artifact_schema;
  std::uint16_t provider_version_ = contract_versions::relation_provider;
  std::uint16_t graph_policy_version_ = contract_versions::relation_graph_policy;
  std::uint16_t truth_policy_version_ = contract_versions::relation_truth_policy;
  std::uint16_t codec_version_ = contract_versions::relation_codec;
  std::uint16_t verifier_version_ = contract_versions::relation_verifier;
  relation_provider_kind provider_ =
      relation_provider_kind::canonical_source_feature_relation_graph_v1;
  relation_verification_disposition verification_ =
      relation_verification_disposition::unverified;
  context_owner_token owner_{};
  std::shared_ptr<const canonical_candidate_stream<T, I>> candidates_;
  relation_request_graph request_graph_{};
  relation_execution_authority execution_authority_{};
  std::shared_ptr<const candidate_source_edge_relation_stage<T>> source_edge_stage_;
  std::shared_ptr<const candidate_source_edge_facet_relation_stage<T>> source_edge_facet_stage_;
  std::shared_ptr<const candidate_source_facet_relation_stage<T>> source_facet_stage_;
  std::shared_ptr<const candidate_coplanar_overlay_stage<T>> coplanar_overlay_stage_;
  std::vector<relation_imported_geometry_record> imported_geometry_;
  std::vector<relation_bounded_primitive_record> bounded_primitives_;
  std::vector<relation_exact_relation_record> exact_relations_;
  std::vector<relation_truth_lineage_record> truth_lineage_;
  std::vector<relation_interval_evidence_record> interval_evidence_;
  std::vector<relation_source_facet_region_record<T>> source_facet_regions_;
  std::vector<relation_truth_record> truth_records_;
  std::vector<feature_relation_record> relations_;
  std::vector<relation_construction_record> constructions_;
  std::vector<relation_construction_ledger_record> construction_ledger_;
  std::vector<symbolic_eligibility_record> symbolic_eligibility_;
  std::vector<symbolic_relation_decision_record> symbolic_decisions_;
  std::vector<relation_crossing_record> crossings_;
  std::vector<relation_event_seed_record> event_seeds_;
  std::vector<relation_coplanar_event_node_record> coplanar_event_nodes_;
  std::vector<relation_coplanar_oriented_arc_record> coplanar_oriented_arcs_;
  std::vector<relation_coplanar_overlap_component_record>
      coplanar_overlap_components_;
  std::vector<relation_coplanar_support_record> coplanar_supports_;
  std::vector<relation_feature_key> event_seed_incidence_;
  std::vector<relation_event_seed_candidate_incidence_record>
      event_seed_candidate_incidence_;
  std::vector<relation_candidate_disposition_record> candidate_dispositions_;
  std::vector<relation_triangle_local_reconciliation_record>
      triangle_local_reconciliation_;
  std::vector<relation_transverse_carrier_membership_record>
      transverse_carrier_memberships_;
  std::array<relation_source_topology_record, 2> source_topology_{};
  std::vector<relation_transverse_carrier_support_record>
      transverse_carrier_supports_;
  std::vector<feature_relation_id> candidate_relation_coverage_;
  std::vector<relation_event_seed_id> candidate_event_seed_coverage_;
  std::vector<relation_candidate_partition_record> candidate_partitions_;
  relation_statistics statistics_{};
  relation_verification_evidence verification_evidence_{};
  std::vector<relation_diagnostic_record> diagnostics_;
  std::vector<relation_replay_checkpoint_record> replay_checkpoints_;
  relation_replay_evidence replay_evidence_{};
  std::array<relation_predecessor_commitment_record, 6>
      predecessor_commitments_{};
  std::vector<relation_resource_evidence_record> resource_evidence_;
  std::array<relation_section_digest_record, 10> section_digests_{};
  bounded_boolean_digest context_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest candidate_digest_{};
  bounded_boolean_digest graph_digest_{};
  boolean_operation operation_ = boolean_operation::set_union;
  T residual_boundary_ = T(0);
  bounded_boolean_digest symbolic_policy_digest_{};
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class relation_builder;
  template <class U, class J> friend class relation_artifact_assembler;
  friend struct relation_artifact_test_access;
  friend struct relation_artifact_internal_access;
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_signed_feature_relations(const signed_feature_relations<U, J> &);
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_relation_unframed_payload(
      const signed_feature_relations<U, J> &,
      std::array<std::size_t, 11> *);
  template <class U, class J>
  friend std::array<std::vector<std::uint8_t>, 10>
  build_relation_section_payloads(const signed_feature_relations<U, J> &);
  template <class U, class J>
  friend bool refresh_relation_section_digests(
      signed_feature_relations<U, J> &) noexcept;
  template <class U, class J>
  friend bool verify_relation_codec(
      const signed_feature_relations<U, J> &, bounded_boolean_error &);
  template <class U, class J>
  friend bool verify_signed_feature_relations(
      const signed_feature_relations<U, J> &, bounded_boolean_error &);
  template <class U, class J>
  friend struct relation_replay_bundle_builder;
  template <class U, class J>
  friend bool verify_relation_replay_bundle(
      const signed_feature_relations<U, J> &, bounded_boolean_error &);
};

} // namespace ygor::mesh_boolean::bounded
