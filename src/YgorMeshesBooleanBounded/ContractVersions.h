#pragma once

#include <cstdint>

namespace ygor::mesh_boolean::bounded {
struct contract_versions final {
    static constexpr std::uint16_t public_api = 1;
    static constexpr std::uint16_t type_profile = 1;
    static constexpr std::uint16_t source_read = 1;
    static constexpr std::uint16_t source_snapshot = 1;
    static constexpr std::uint16_t source_bundle = 1;
    static constexpr std::uint16_t canonical_bytes = 1;
    static constexpr std::uint16_t sha256 = 1;
    static constexpr std::uint16_t policies = 1;
    static constexpr std::uint16_t truth_table = 1;
    static constexpr std::uint16_t symbolic_policy = 1;
    static constexpr std::uint16_t identities = 1;
    static constexpr std::uint16_t errors = 1;
    static constexpr std::uint16_t resources = 1;
    static constexpr std::uint16_t cancellation = 1;
    static constexpr std::uint16_t replay = 1;
    static constexpr std::uint16_t platform = 1;
    static constexpr std::uint16_t context = 1;
    static constexpr std::uint16_t transaction = 1;
    static constexpr std::uint16_t verifier = 1;
    static constexpr std::uint16_t precision_bootstrap_schema = 1;
    static constexpr std::uint16_t precision_bootstrap_provider = 1;
    static constexpr std::uint16_t precision_scalar_profile = 1;
    static constexpr std::uint16_t floating_bits = 1;
    static constexpr std::uint16_t directed_rounding = 1;
    static constexpr std::uint16_t finite_interval_schema = 1;
    static constexpr std::uint16_t finite_interval_provider = 1;
    static constexpr std::uint16_t exact_expansion_core = 1;
    static constexpr std::uint16_t exact_expansion_adapter = 1;
    static constexpr std::uint16_t rounded_operation_graphs = 1;
    static constexpr std::uint16_t exact_relation_formulas = 2;
    static constexpr std::uint16_t bounded_values = 1;
    static constexpr std::uint16_t predicate_truth_layers = 1;
    static constexpr std::uint16_t construction_conditioning = 1;
    static constexpr std::uint16_t precision_trace = 1;
    static constexpr std::uint16_t precision_ledger = 1;
    static constexpr std::uint16_t tolerance_certificates = 1;
    static constexpr std::uint16_t tolerance_budget = 1;
    static constexpr std::uint16_t precision_import = 1;
    static constexpr std::uint16_t precision_codec = 1;
    static constexpr std::uint16_t precision_verifier = 1;
    static constexpr std::uint16_t input_validation_provider = 1;
    static constexpr std::uint16_t normalized_ring_schema = 1;
    static constexpr std::uint16_t source_incidence_schema = 1;
    static constexpr std::uint16_t vertex_link_schema = 1;
    static constexpr std::uint16_t source_topology_canonicalizer = 1;
    static constexpr std::uint16_t presentation_correspondence_schema = 1;
    static constexpr std::uint16_t input_facet_geometry_schema = 1;
    static constexpr std::uint16_t input_facet_geometry_provider = 1;
    static constexpr std::uint16_t coherent_realization_schema = 1;
    static constexpr std::uint16_t coherent_realization_provider = 1;
    static constexpr std::uint16_t input_geometry_relations = 1;
    static constexpr std::uint16_t shell_pair_relation_schema = 1;
    static constexpr std::uint16_t shell_pair_relation_provider = 1;
    static constexpr std::uint16_t shell_semantics_schema = 1;
    static constexpr std::uint16_t shell_semantics_provider = 1;
    static constexpr std::uint16_t input_geometry_assessment_schema = 1;
    static constexpr std::uint16_t input_geometry_assessment_provider = 1;
    static constexpr std::uint16_t validated_operand = 1;
    static constexpr std::uint16_t validated_operand_codec = 1;
    static constexpr std::uint16_t validated_operand_verifier = 1;
    static constexpr std::uint16_t source_triangulation_provider = 1;
    static constexpr std::uint16_t source_triangulation_policy = 1;
    static constexpr std::uint16_t facet_geometry_basis_ref = 1;
    static constexpr std::uint16_t projected_source_workspace = 1;
    static constexpr std::uint16_t bounded_source_polygon_kernel = 1;
    static constexpr std::uint16_t source_ear_reference_trace = 1;
    static constexpr std::uint16_t source_ear_dependency_trace = 1;
    static constexpr std::uint16_t source_triangle_schema = 1;
    static constexpr std::uint16_t source_triangle_edge_use_schema = 1;
    static constexpr std::uint16_t source_internal_diagonal_schema = 1;
    static constexpr std::uint16_t source_facet_coverage_schema = 1;
    static constexpr std::uint16_t source_verifier_witness_schema = 1;
    static constexpr std::uint16_t source_triangle_complex = 1;
    static constexpr std::uint16_t source_triangle_complex_codec = 1;
    static constexpr std::uint16_t source_triangle_complex_verifier = 1;
    static constexpr std::uint16_t canonical_halfedge_vertex_schema = 1;
    static constexpr std::uint16_t canonical_halfedge_triangle_schema = 1;
    static constexpr std::uint16_t canonical_halfedge_halfedge_schema = 1;
    static constexpr std::uint16_t canonical_halfedge_pairing_policy = 1;
    static constexpr std::uint16_t canonical_halfedge_geometry_attachment_schema = 1;
    static constexpr std::uint16_t canonical_halfedge_bound_formula = 1;
    static constexpr std::uint16_t canonical_halfedge_operand_schema = 1;
    static constexpr std::uint16_t canonical_halfedge_provider = 1;
    static constexpr std::uint16_t canonical_halfedge_policy = 1;
    static constexpr std::uint16_t canonical_halfedge_codec = 1;
    static constexpr std::uint16_t canonical_halfedge_verifier = 1;
    static constexpr std::uint16_t canonical_source_manifolds_schema = 1;

    // Component 06 broad-phase collision enumeration.
    static constexpr std::uint16_t broad_phase_artifact_schema = 1;
    static constexpr std::uint16_t broad_phase_provider = 1;
    static constexpr std::uint16_t broad_phase_candidate_domain_policy = 1;
    static constexpr std::uint16_t broad_phase_primitive_schema = 1;
    static constexpr std::uint16_t broad_phase_axis_key_schema = 1;
    static constexpr std::uint16_t broad_phase_dense_rank_schema = 1;
    static constexpr std::uint16_t broad_phase_rank_morton_schema = 1;
    static constexpr std::uint16_t broad_phase_hierarchy_node_schema = 1;
    static constexpr std::uint16_t broad_phase_count_plan_schema = 1;
    static constexpr std::uint16_t broad_phase_witness_schema = 1;
    static constexpr std::uint16_t broad_phase_candidate_key_schema = 1;
    static constexpr std::uint16_t broad_phase_candidate_record_schema = 1;
    static constexpr std::uint16_t broad_phase_partition_schema = 1;
    static constexpr std::uint16_t broad_phase_statistics_schema = 1;
    static constexpr std::uint16_t broad_phase_evidence_schema = 1;
    static constexpr std::uint16_t broad_phase_codec = 1;
    static constexpr std::uint16_t broad_phase_replay = 1;
    static constexpr std::uint16_t broad_phase_verifier = 1;
    static constexpr std::uint16_t broad_phase_leaf_layout_policy = 1;
    static constexpr std::uint16_t broad_phase_spatial_order_policy = 1;
    static constexpr std::uint16_t broad_phase_node_id_policy = 1;
    static constexpr std::uint16_t broad_phase_traversal_policy = 1;
    static constexpr std::uint16_t broad_phase_candidate_order_policy = 1;
    static constexpr std::uint16_t broad_phase_duplicate_policy = 1;
    static constexpr std::uint16_t broad_phase_encoding_policy = 1;

    // Component 07 canonical relation graph and symbolic perturbation.
    static constexpr std::uint16_t relation_artifact_schema = 5;
    static constexpr std::uint16_t relation_provider = 1;
    static constexpr std::uint16_t relation_graph_policy = 1;
    static constexpr std::uint16_t relation_family_precedence_policy = 1;
    static constexpr std::uint16_t relation_truth_policy = 1;
    static constexpr std::uint16_t relation_primitive_formula_binding = 1;
    static constexpr std::uint16_t relation_source_facet_region_schema = 1;
    static constexpr std::uint16_t relation_source_facet_region_policy = 1;
    static constexpr std::uint16_t relation_source_facet_segment_schema = 1;
    static constexpr std::uint16_t relation_source_facet_segment_policy = 1;
    static constexpr std::uint16_t relation_source_facet_segment_witness_policy = 1;
    static constexpr std::uint16_t relation_triangle_local_reconciliation_schema = 1;
    static constexpr std::uint16_t relation_alternative_triangulation_semantics_policy = 1;
    static constexpr std::uint16_t relation_source_edge_edge_schema = 1;
    static constexpr std::uint16_t relation_source_edge_edge_policy = 1;
    static constexpr std::uint16_t relation_source_edge_facet_schema = 1;
    static constexpr std::uint16_t relation_source_edge_facet_policy = 1;
    static constexpr std::uint16_t relation_source_edge_facet_stage_schema = 1;
    static constexpr std::uint16_t relation_source_edge_facet_stage_policy = 1;
    static constexpr std::uint16_t relation_source_facet_facet_schema = 1;
    static constexpr std::uint16_t relation_source_facet_facet_policy = 1;
    static constexpr std::uint16_t relation_source_facet_facet_stage_schema = 1;
    static constexpr std::uint16_t relation_source_facet_facet_stage_policy = 1;
    static constexpr std::uint16_t relation_coplanar_overlay_schema = 3;
    static constexpr std::uint16_t relation_coplanar_overlay_policy = 1;
    static constexpr std::uint16_t relation_coplanar_overlay_stage_schema = 1;
    static constexpr std::uint16_t relation_coplanar_overlay_stage_policy = 1;
    static constexpr std::uint16_t relation_coplanar_topology_schema = 1;
    static constexpr std::uint16_t relation_coplanar_topology_policy = 1;
    static constexpr std::uint16_t relation_feature_key_schema = 1;
    static constexpr std::uint16_t relation_request_key_schema = 1;
    static constexpr std::uint16_t relation_event_seed_key_schema = 1;
    static constexpr std::uint16_t relation_request_graph_schema = 1;
    static constexpr std::uint16_t relation_construction_schema = 1;
    static constexpr std::uint16_t relation_symbolic_eligibility_schema = 1;
    static constexpr std::uint16_t relation_symbolic_decision_schema = 1;
    static constexpr std::uint16_t relation_event_seed_schema = 1;
    static constexpr std::uint16_t relation_candidate_disposition_schema = 1;
    static constexpr std::uint16_t relation_owner_exclusion_policy = 1;
    static constexpr std::uint16_t relation_selection_boundary_policy = 1;
    static constexpr std::uint16_t relation_codec = 6;
    static constexpr std::uint16_t relation_verifier = 6;

    static constexpr std::uint16_t input_facet_geometry = input_facet_geometry_provider;
    static constexpr std::uint16_t coherent_realization = coherent_realization_provider;
    static constexpr std::uint16_t shell_semantics = shell_semantics_provider;
};
enum class operand_id : std::uint8_t { a = 0, b = 1 };
enum class stage_id : std::uint16_t {
    public_entry = 1, source_capture = 2, context_preflight = 3,
    precision_bootstrap = 4, publication = 20, qualification = 21,
    execution_service = 22, input_validation_a = 23, input_validation_b = 24,
    source_triangulation_a = 25, source_triangulation_b = 26,
    canonical_halfedge_a = 27, canonical_halfedge_b = 28,
    broad_phase = 29, relation_kernel = 30,
};
enum class precision_checkpoint : std::uint32_t {
    pending_context_validation = 1, source_bit_scan = 2, non_finite_rejection = 3,
    scale_derivation = 4, machine_floor_derivation = 5, precision_normalization = 6,
    preflight_encoding = 7, frozen_context_handshake = 8, context_construction = 9,
    source_import = 10, rounded_operation = 11, exact_relation = 12,
    predicate_assembly = 13, construction_conditioning = 14, trace_finalization = 15,
    ledger_snapshot = 16, tolerance_reservation = 17, tolerance_commit = 18,
    conservative_bounds = 19, prior_precision_import = 20, independent_verification = 21,
    canonical_encoding = 22, publication_commit = 23,
};
} // namespace ygor::mesh_boolean::bounded
