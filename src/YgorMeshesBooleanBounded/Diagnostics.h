#pragma once

#include "Errors.h"

#include <optional>
#include <vector>

namespace ygor::mesh_boolean::bounded {
class diagnostic_collector {
  public:
    explicit diagnostic_collector(std::size_t capacity):capacity_(capacity){secondary_.reserve(capacity);}
    void submit(bounded_boolean_error e){++candidate_count_;if(!primary_||error_key(e)<error_key(*primary_))primary_=e;if(secondary_.size()<capacity_)secondary_.push_back(std::move(e));else truncated_=true;}
    void observe_cancellation(bounded_boolean_error e)noexcept{if(!cancellation_)cancellation_=e;}
    const bounded_boolean_error*result()const noexcept{return cancellation_?&*cancellation_:(primary_?&*primary_:nullptr);}
    std::uint64_t candidate_count()const noexcept{return candidate_count_;}bool truncated()const noexcept{return truncated_;}
  private:std::size_t capacity_;std::uint64_t candidate_count_=0;bool truncated_=false;std::optional<bounded_boolean_error>primary_,cancellation_;std::vector<bounded_boolean_error>secondary_;
};
}
