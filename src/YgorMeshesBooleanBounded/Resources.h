#pragma once

#include "CheckedArithmetic.h"
#include "../YgorMeshesBooleanBounded.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>

namespace ygor::mesh_boolean::bounded {
struct resource_cancellation_checkpoint {
    const void *context = nullptr;
    bool (*requested)(const void *) noexcept = nullptr;
    bool cancellation_requested() const noexcept {
        return requested && requested(context);
    }
};
enum class resource_kind : std::uint8_t {
    persistent_bytes=0,temporary_bytes=1,source_vertices=2,source_faces=3,source_indices=4,
    source_rings=5,source_edges=6,source_triangles=7,source_halfedges=8,broad_phase_nodes=9,
    broad_phase_candidates=10,relations=11,symbolic_decisions=12,events=13,classification_groups=14,
    retained_uses=15,output_occurrences=16,output_carriers=17,output_halfedges=18,output_cycles=19,
    output_triangles=20,cleanup_actions=21,verification_findings=22,diagnostic_findings=23,
    diagnostic_bytes=24,replay_bytes=25,canonical_sort_records=26,task_descriptors=27,
    emergency_error_storage=28,work_units=29,precision_scale_records=30,bounded_values=31,
    directed_rounding_evidence=32,exact_expansion_limbs=33,precision_trace_nodes=34,
    precision_trace_parents=35,precision_ledger_records=36,budget_proposals=37,
    budget_reservations=38,budget_commits=39,finite_bounds=40,precision_import_records=41,
    precision_codec_bytes=42,precision_verifier_work=43,
    relation_requests=44,relation_dependencies=45,relation_consumers=46,
    relation_constructions=47,relation_overlays=48,relation_interval_partitions=49,
    relation_event_incidence=50,intersection_occurrences=51,intersection_incidence=52,
    intersection_memberships=53,intersection_clusters=54,intersection_intervals=55,
    intersection_carriers=56,intersection_overlaps=57,intersection_aggregates=58,
    intersection_descriptors=59,intersection_order_certificates=60,
    intersection_verifier_work=61,
    relation_request_records=62,relation_primitive_records=63,
    relation_family_records=64,relation_graph_edges=65,
    relation_region_workspace=66,relation_numerical_workspace=67,
    relation_overlay_records=68,relation_construction_records=69,
    relation_crossing_records=70,relation_symbolic_records=71,
    relation_seed_records=72,relation_disposition_records=73,
    relation_canonical_workspace=74,relation_private_buffers=75,
    relation_codec_evidence=76,relation_verifier_evidence=77,
    relation_persistent_artifact=78,
    classification_atoms=79,classification_sectors=80,
    classification_occurrences=81,classification_adjacency=82,
    classification_union_proposals=83,classification_quotient_edges=84,
    classification_seed_queries=85,classification_propagation_assignments=86,
    classification_side_labels=87,classification_shell_contributions=88,
    classification_verifier_evidence=89,
    selection_dispositions=90,selection_side_tuples=91,selection_sheet_cells=92,
    selection_owner_decisions=93,selection_multiplicity=94,selection_incidences=95,
    selection_continuations=96,selection_edge_occurrences=97,
    selection_vertex_occurrences=98,    selection_local_ports=99,
    selection_local_arcs=100,selection_carrier_balance=101,
    selection_feasibility=102,selection_verifier_evidence=103,
    output_topology_incidence_audits=104,output_topology_zero_measure=105,
    output_topology_coordinates=106,output_topology_vertex_occurrences=107,
    output_topology_regions=108,output_topology_region_members=109,
    output_topology_continuations=110,output_topology_darts=111,
    output_topology_pairs=112,output_topology_halfedges=113,
    output_topology_endpoint_fans=114,output_topology_successors=115,
    output_topology_cycles=116,output_topology_cycle_refs=117,
    output_topology_contours=118,output_topology_witnesses=119,
    output_topology_admissibility=120,output_topology_vertex_links=121,
    output_topology_carrier_audits=122,output_topology_verifier_evidence=123,
    output_triangulation_support_frames=124,output_triangulation_projected=125,
    output_triangulation_predicates=126,output_triangulation_diagonals=127,
    output_triangulation_internal_halfedges=128,output_triangulation_triangles=129,
    output_triangulation_corner_refs=130,output_triangulation_edge_use_refs=131,
    output_triangulation_residuals=132,output_triangulation_assignments=133,
    output_triangulation_certificates=134,output_triangulation_verifier_evidence=135,
    cleanup_vertex_slots=136,cleanup_edge_slots=137,cleanup_halfedge_slots=138,
    cleanup_triangle_slots=139,cleanup_component_records=140,
    cleanup_obligation_records=141,cleanup_action_records=142,
    cleanup_verifier_records=143,
    count=144
};
struct resource_counter { std::uint64_t hard=0,advisory=0,reserved=0,committed=0,peak_live=0,cumulative=0; };
class resource_manager;
class resource_reservation {
  public:
    resource_reservation() noexcept=default;resource_reservation(const resource_reservation&)=delete;resource_reservation&operator=(const resource_reservation&)=delete;
    resource_reservation(resource_reservation&& other) noexcept;resource_reservation&operator=(resource_reservation&& other) noexcept;~resource_reservation();
    bool commit(std::uint64_t used) noexcept;bool shrink(std::uint64_t amount) noexcept;void release() noexcept;std::uint64_t amount()const noexcept{return amount_;}
  private:
    resource_reservation(resource_manager *owner,resource_kind kind,std::uint64_t amount) noexcept:owner_(owner),kind_(kind),amount_(amount){}
    resource_manager *owner_=nullptr;resource_kind kind_=resource_kind::temporary_bytes;std::uint64_t amount_=0;friend class resource_manager;
};
class resource_manager {
  public:
    explicit resource_manager(const resource_policy &policy) noexcept;
    std::optional<resource_reservation> reserve(resource_kind kind,std::uint64_t amount) noexcept;
    std::optional<resource_reservation> reserve(resource_kind kind, std::uint64_t amount,
                                                const resource_cancellation_checkpoint &checkpoint) noexcept;
    std::array<resource_counter,static_cast<std::size_t>(resource_kind::count)> snapshot()const noexcept;
  private:
    void release(resource_kind,std::uint64_t)noexcept;bool commit(resource_kind,std::uint64_t,std::uint64_t)noexcept;
    mutable std::mutex mutex_;std::array<resource_counter,static_cast<std::size_t>(resource_kind::count)> counters_{};friend class resource_reservation;
};
} 
