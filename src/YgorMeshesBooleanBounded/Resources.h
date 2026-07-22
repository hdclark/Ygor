#pragma once

#include "CheckedArithmetic.h"
#include "../YgorMeshesBooleanBounded.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>

namespace ygor::mesh_boolean::bounded {
enum class resource_kind : std::uint8_t {
    persistent_bytes=0,temporary_bytes=1,source_vertices=2,source_faces=3,source_indices=4,
    source_rings=5,source_edges=6,source_triangles=7,source_halfedges=8,broad_phase_nodes=9,
    broad_phase_candidates=10,relations=11,symbolic_decisions=12,events=13,classification_groups=14,
    retained_uses=15,output_occurrences=16,output_carriers=17,output_halfedges=18,output_cycles=19,
    output_triangles=20,cleanup_actions=21,verification_findings=22,diagnostic_findings=23,
    diagnostic_bytes=24,replay_bytes=25,canonical_sort_records=26,task_descriptors=27,
    emergency_error_storage=28,work_units=29,count=30
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
    std::array<resource_counter,static_cast<std::size_t>(resource_kind::count)> snapshot()const noexcept;
  private:
    void release(resource_kind,std::uint64_t)noexcept;bool commit(resource_kind,std::uint64_t,std::uint64_t)noexcept;
    mutable std::mutex mutex_;std::array<resource_counter,static_cast<std::size_t>(resource_kind::count)> counters_{};friend class resource_reservation;
};
}
