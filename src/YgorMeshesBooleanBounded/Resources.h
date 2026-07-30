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
    intersection_verifier_work=61,count=62
};
struct resource_counter { std::uint64_t hard=0,advisory=0,reserved=0,committed=0,peak_live=0,cumulative=0; };
class resource_manager;
class resource_reservation {
  public:
    resource_reservation() noexcept=default;resource_reservation(const resource_reservation&)=delete;resource_reservation&operator=(const resource_reservation&)=delete;
    resource_reservation(resource_reservation&& other) noexcept;resource_reservation&operator=(resource_reservation&& other) noexcept;~resource_reservation();
    bool commit(std::uint64_t used) noexcept;void release() noexcept;std::uint64_t amount()const noexcept{return amount_;}
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
