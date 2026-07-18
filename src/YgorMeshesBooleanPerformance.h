#pragma once
#ifndef YGOR_MESHES_BOOLEAN_PERFORMANCE_H_
#define YGOR_MESHES_BOOLEAN_PERFORMANCE_H_

#include "YgorMeshesBooleanContract.h"
#include <chrono>
#include <map>

namespace ygor { namespace mesh_boolean {

class performance_collector {
  struct bucket {
    std::uint64_t nanoseconds = 0;
    performance_counter_snapshot counters;
  };
  mutable std::mutex mutex_;
  std::array<std::array<std::map<std::uint64_t, bucket>, 2>,
             static_cast<std::size_t>(boolean_stage::final_verification) + 1>
      buckets_;

public:
  void merge(boolean_stage, performance_role, std::uint64_t, std::uint64_t,
             const performance_counter_snapshot &) noexcept;
  std::shared_ptr<const performance_snapshot> snapshot() const;
};

struct performance_task_context {
  performance_collector *collector = nullptr;
  boolean_stage stage = boolean_stage::context_setup;
  performance_role role = performance_role::producer;
};
struct performance_measurement {
  performance_counter_snapshot counters;
  std::uint64_t nanoseconds = 0;
  bool valid = false;
};

performance_task_context capture_performance_task_context() noexcept;
void performance_count(performance_counter, std::uint64_t = 1) noexcept;
void performance_max(performance_counter, std::uint64_t) noexcept;
void performance_count_resource(resource_kind, std::uint64_t) noexcept;

class performance_scope {
  struct state;
  std::unique_ptr<state> state_;

public:
  performance_scope(performance_collector *, boolean_stage, performance_role,
                    std::uint64_t task_key = 0, bool timed = true);
  performance_scope(performance_scope &&) noexcept;
  performance_scope &operator=(performance_scope &&) noexcept;
  performance_scope(const performance_scope &) = delete;
  performance_scope &operator=(const performance_scope &) = delete;
  ~performance_scope();
  void finish() noexcept;
  performance_measurement finish_without_merge() noexcept;
};

} }
#endif
