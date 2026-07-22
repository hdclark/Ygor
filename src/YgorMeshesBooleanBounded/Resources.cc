#include "StrictFloatingBuild.h"
#include "Resources.h"

#include <algorithm>
#include <utility>

namespace ygor::mesh_boolean::bounded {
resource_manager::resource_manager(const resource_policy&p)noexcept {
    for(auto &counter:counters_){counter.advisory=p.work_units.advisory;counter.hard=p.work_units.hard;}
    const std::pair<resource_kind,resource_limit_policy> limits[]{
        {resource_kind::persistent_bytes,p.persistent_bytes},{resource_kind::temporary_bytes,p.temporary_bytes},
        {resource_kind::source_vertices,p.source_vertices},{resource_kind::source_faces,p.source_faces},
        {resource_kind::source_indices,p.source_indices},{resource_kind::work_units,p.work_units},
        {resource_kind::precision_trace_nodes,p.precision_trace_nodes},
        {resource_kind::precision_ledger_records,p.precision_ledger_records},
        {resource_kind::budget_proposals,p.budget_proposals},
        {resource_kind::budget_reservations,p.budget_reservations},
        {resource_kind::budget_commits,p.budget_commits},
        {resource_kind::precision_verifier_work,p.precision_verifier_work}};
    for(const auto &entry:limits){auto &counter=counters_[static_cast<std::size_t>(entry.first)];counter.advisory=entry.second.advisory;counter.hard=entry.second.hard;}
}
std::optional<resource_reservation> resource_manager::reserve(resource_kind k,std::uint64_t amount)noexcept{std::lock_guard<std::mutex>l(mutex_);auto&c=counters_[static_cast<std::size_t>(k)];std::uint64_t live=0,next=0;if(!checked_add(c.reserved,c.committed,live)||!checked_add(live,amount,next)||next>c.hard)return std::nullopt;c.reserved+=amount;c.peak_live=std::max(c.peak_live,next);if(k==resource_kind::work_units)c.cumulative+=amount;return resource_reservation(this,k,amount);}
std::optional<resource_reservation> resource_manager::reserve(
    resource_kind k,std::uint64_t amount,const resource_cancellation_checkpoint&checkpoint)noexcept{
    if(checkpoint.cancellation_requested())return std::nullopt;
    auto reservation=reserve(k,amount);
    if(checkpoint.cancellation_requested()){if(reservation)reservation->release();return std::nullopt;}
    return reservation;
}
void resource_manager::release(resource_kind k,std::uint64_t amount)noexcept{std::lock_guard<std::mutex>l(mutex_);auto&c=counters_[static_cast<std::size_t>(k)];c.reserved=amount<=c.reserved?c.reserved-amount:0;}
bool resource_manager::commit(resource_kind k,std::uint64_t reserved,std::uint64_t used)noexcept{std::lock_guard<std::mutex>l(mutex_);auto&c=counters_[static_cast<std::size_t>(k)];if(used>reserved||reserved>c.reserved)return false;c.reserved-=reserved;c.committed+=used;return true;}
std::array<resource_counter,static_cast<std::size_t>(resource_kind::count)> resource_manager::snapshot()const noexcept{std::lock_guard<std::mutex>l(mutex_);return counters_;}
resource_reservation::resource_reservation(resource_reservation&&o)noexcept:owner_(o.owner_),kind_(o.kind_),amount_(o.amount_){o.owner_=nullptr;o.amount_=0;}
resource_reservation&resource_reservation::operator=(resource_reservation&&o)noexcept{if(this!=&o){release();owner_=o.owner_;kind_=o.kind_;amount_=o.amount_;o.owner_=nullptr;o.amount_=0;}return*this;}
resource_reservation::~resource_reservation(){release();}
bool resource_reservation::commit(std::uint64_t used)noexcept{if(!owner_||!owner_->commit(kind_,amount_,used))return false;owner_=nullptr;amount_=0;return true;}
void resource_reservation::release()noexcept{if(owner_)owner_->release(kind_,amount_);owner_=nullptr;amount_=0;}
}
