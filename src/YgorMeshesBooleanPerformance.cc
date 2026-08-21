#include "YgorMeshesBooleanPerformance.h"
#include <algorithm>

namespace ygor { namespace mesh_boolean {
namespace {
struct active_performance {
  performance_collector *collector = nullptr;
  boolean_stage stage = boolean_stage::context_setup;
  performance_role role = performance_role::producer;
  std::uint64_t task_key = 0;
  performance_counter_snapshot counters;
  std::chrono::steady_clock::time_point started;
  std::uint64_t elapsed_nanoseconds = 0;
  bool timed = false, running = false;
};
thread_local active_performance *active = nullptr;

void add_saturating(std::uint64_t &to, std::uint64_t value) noexcept {
  to = value > std::numeric_limits<std::uint64_t>::max() - to
           ? std::numeric_limits<std::uint64_t>::max()
           : to + value;
}
}

struct performance_scope::state {
  active_performance local;
  active_performance *prior = nullptr;
  bool finished = false;
};

void performance_collector::merge(
    boolean_stage stage, performance_role role, std::uint64_t task_key,
    std::uint64_t nanoseconds,
    const performance_counter_snapshot &counters) noexcept {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &bucket = buckets_[static_cast<std::size_t>(stage)]
                           [static_cast<std::size_t>(role)][task_key];
    add_saturating(bucket.nanoseconds, nanoseconds);
    for (std::size_t i = 0; i < bucket.counters.values.size(); ++i) {
      const auto counter = static_cast<performance_counter>(i);
      if (counter == performance_counter::max_numerator_limbs ||
          counter == performance_counter::max_denominator_limbs)
        bucket.counters.values[i] =
            std::max(bucket.counters.values[i], counters.values[i]);
      else
        add_saturating(bucket.counters.values[i], counters.values[i]);
    }
    for (std::size_t i = 0; i < bucket.counters.resources.size(); ++i)
      add_saturating(bucket.counters.resources[i], counters.resources[i]);
  } catch (...) {
    // Diagnostics are non-authoritative and must not change the Boolean result.
  }
}

std::shared_ptr<const performance_snapshot>
performance_collector::snapshot() const {
  auto result = std::make_shared<performance_snapshot>();
  result->collected = true;
  std::lock_guard<std::mutex> lock(mutex_);
  for (std::size_t stage = 0; stage < buckets_.size(); ++stage)
    for (std::size_t role = 0; role < 2; ++role)
      for (const auto &task : buckets_[stage][role]) {
        auto &out = result->stages[stage];
        auto &duration = role == 0 ? out.producer_nanoseconds
                                  : out.verifier_nanoseconds;
        auto &counters = role == 0 ? out.producer : out.verifier;
        add_saturating(duration, task.second.nanoseconds);
        for (std::size_t i = 0; i < counters.values.size(); ++i) {
          const auto counter = static_cast<performance_counter>(i);
          if (counter == performance_counter::max_numerator_limbs ||
              counter == performance_counter::max_denominator_limbs)
            counters.values[i] =
                std::max(counters.values[i], task.second.counters.values[i]);
          else
            add_saturating(counters.values[i], task.second.counters.values[i]);
        }
        for (std::size_t i = 0; i < counters.resources.size(); ++i)
          add_saturating(counters.resources[i],
                         task.second.counters.resources[i]);
      }
  return result;
}

performance_task_context capture_performance_task_context() noexcept {
  return active ? performance_task_context{active->collector, active->stage,
                                            active->role}
                : performance_task_context{};
}
void performance_count(performance_counter counter, std::uint64_t value) noexcept {
  if (active)
    add_saturating(active->counters.values[static_cast<std::size_t>(counter)],
                   value);
}
void performance_max(performance_counter counter, std::uint64_t value) noexcept {
  if (active) {
    auto &current = active->counters.values[static_cast<std::size_t>(counter)];
    current = std::max(current, value);
  }
}
void performance_count_resource(resource_kind kind, std::uint64_t value) noexcept {
  if (active)
    add_saturating(active->counters.resources[static_cast<std::size_t>(kind)],
                   value);
}

performance_scope::performance_scope(performance_collector *collector,
                                     boolean_stage stage,
                                     performance_role role,
                                     std::uint64_t task_key, bool timed) {
  if (!collector)
    return;
  try {
    state_.reset(new state);
  } catch (...) {
    return;
  }
  state_->local.collector = collector;
  state_->local.stage = stage;
  state_->local.role = role;
  state_->local.task_key = task_key;
  state_->prior = active;
  const auto now = std::chrono::steady_clock::now();
  if (active && active->running) {
    add_saturating(active->elapsed_nanoseconds,
                   static_cast<std::uint64_t>(
                       std::chrono::duration_cast<std::chrono::nanoseconds>(
                           now - active->started)
                           .count()));
    active->running = false;
  }
  state_->local.timed = timed;
  state_->local.running = timed;
  state_->local.started = now;
  active = &state_->local;
}
performance_scope::performance_scope(performance_scope &&) noexcept = default;
performance_scope &performance_scope::operator=(performance_scope &&) noexcept =
    default;
performance_scope::~performance_scope() { finish(); }
performance_measurement performance_scope::finish_without_merge() noexcept {
  performance_measurement result;
  if (!state_ || state_->finished)
    return result;
  const auto now = std::chrono::steady_clock::now();
  if (state_->local.running)
    add_saturating(state_->local.elapsed_nanoseconds,
                   static_cast<std::uint64_t>(
                       std::chrono::duration_cast<std::chrono::nanoseconds>(
                           now - state_->local.started)
                           .count()));
  active = state_->prior;
  if (active && active->timed) {
    active->started = now;
    active->running = true;
  }
  result.counters = state_->local.counters;
  result.nanoseconds = state_->local.elapsed_nanoseconds;
  result.valid = true;
  state_->finished = true;
  return result;
}
void performance_scope::finish() noexcept {
  if (!state_ || state_->finished)
    return;
  auto *collector = state_->local.collector;
  const auto stage = state_->local.stage;
  const auto role = state_->local.role;
  const auto task_key = state_->local.task_key;
  const auto result = finish_without_merge();
  if (result.valid)
    collector->merge(stage, role, task_key, result.nanoseconds,
                     result.counters);
}

} }
