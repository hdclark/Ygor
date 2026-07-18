#pragma once
#ifndef YGOR_MESHES_BOOLEAN_EXECUTOR_H_
#define YGOR_MESHES_BOOLEAN_EXECUTOR_H_

#include "YgorMeshesBooleanContract.h"
#include "YgorMeshesBooleanPerformance.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace ygor { namespace mesh_boolean {

struct deterministic_task {
  std::uint64_t key = 0;
  std::function<status_or<bool>(cancellation_token)> work;
};

class deterministic_executor {
  struct run_state {
    std::vector<std::optional<boolean_error>> errors;
    std::vector<performance_measurement> measurements;
    std::vector<cancellation_source> task_cancellations;
    std::size_t admitted = 0;
    std::size_t completed = 0;
    std::uint64_t failure_key = std::numeric_limits<std::uint64_t>::max();
    bool cancelled = false;
  };

  struct queued_task {
    std::uint64_t key = 0;
    std::size_t slot = 0;
    std::function<status_or<bool>(cancellation_token)> work;
    performance_task_context performance;
    std::shared_ptr<run_state> state;
  };

  execution_policy policy_;
  mutable std::mutex mutex_;
  mutable std::mutex run_mutex_;
  mutable std::condition_variable work_ready_;
  mutable std::condition_variable state_changed_;
  mutable std::deque<queued_task> queue_;
  std::vector<std::thread> workers_;
  bool stopping_ = false;

  static status_or<bool> invoke(queued_task &task,
                                performance_measurement &measurement) {
    performance_scope performance(task.performance.collector,
                                  task.performance.stage,
                                  task.performance.role, task.key, false);
    try {
      auto result =
          task.work(task.state->task_cancellations[task.slot].token());
      measurement = performance.finish_without_merge();
      return result;
    } catch (const std::bad_alloc &) {
      measurement = performance.finish_without_merge();
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::context_setup, "allocation");
    } catch (const std::exception &e) {
      measurement = performance.finish_without_merge();
      auto error = make_error(boolean_error_code::internal_invariant_error,
                              boolean_stage::context_setup, "task_exception");
      error.detail = e.what();
      return error;
    } catch (...) {
      measurement = performance.finish_without_merge();
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::context_setup,
                        "task_exception_unknown");
    }
  }

  void worker_loop() {
    for (;;) {
      queued_task task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        work_ready_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
        if (stopping_ && queue_.empty()) return;
        task = std::move(queue_.front());
        queue_.pop_front();
        state_changed_.notify_all();

        if (task.state->cancelled || task.key > task.state->failure_key) {
          ++task.state->completed;
          state_changed_.notify_all();
          continue;
        }
      }

      performance_measurement measurement;
      auto result = invoke(task, measurement);
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!task.state->measurements.empty())
          task.state->measurements[task.slot] = std::move(measurement);
        if (!result.has_value()) {
          task.state->errors[task.slot] = std::move(result.error());
          task.state->failure_key =
              std::min(task.state->failure_key, task.key);
          for (std::size_t i = task.slot + 1;
               i < task.state->task_cancellations.size(); ++i)
            task.state->task_cancellations[i].cancel();
        }
        ++task.state->completed;
      }
      state_changed_.notify_all();
    }
  }

  static boolean_error executor_error(boolean_error_code code,
                                      const char *message) {
    return make_error(code, boolean_stage::context_setup, message);
  }

public:
  explicit deterministic_executor(execution_policy policy) : policy_(policy) {
    workers_.reserve(policy_.max_threads);
    try {
      for (std::uint32_t i = 0; i < policy_.max_threads; ++i)
        workers_.emplace_back([this] { worker_loop(); });
    } catch (...) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
      }
      work_ready_.notify_all();
      for (auto &worker : workers_)
        if (worker.joinable()) worker.join();
      throw;
    }
  }

  ~deterministic_executor() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stopping_ = true;
    }
    work_ready_.notify_all();
    for (auto &worker : workers_)
      if (worker.joinable()) worker.join();
  }

  deterministic_executor(const deterministic_executor &) = delete;
  deterministic_executor &operator=(const deterministic_executor &) = delete;

  const execution_policy &policy() const { return policy_; }

  status_or<bool> run(std::vector<deterministic_task> tasks,
                      cancellation_token cancel) const {
    std::lock_guard<std::mutex> single_run(run_mutex_);
    if (cancel.cancelled())
      return executor_error(boolean_error_code::resource_limit, "cancelled");
    if (tasks.empty()) return true;

    std::shared_ptr<run_state> state;
    try {
      std::sort(tasks.begin(), tasks.end(), [](const auto &a, const auto &b) {
        return a.key < b.key;
      });
      for (std::size_t i = 1; i < tasks.size(); ++i)
        if (tasks[i - 1].key == tasks[i].key)
          return executor_error(boolean_error_code::internal_invariant_error,
                                "duplicate_task_key");

      state = std::make_shared<run_state>();
      state->errors.resize(tasks.size());
      state->task_cancellations.resize(tasks.size());
      const auto performance = capture_performance_task_context();
      if (performance.collector) state->measurements.resize(tasks.size());
      std::size_t next = 0;
      std::unique_lock<std::mutex> lock(mutex_);
      while (next < tasks.size()) {
        if (cancel.cancelled()) {
          state->cancelled = true;
          for (auto &source : state->task_cancellations) source.cancel();
        }
        if (state->cancelled ||
            state->failure_key != std::numeric_limits<std::uint64_t>::max())
          break;
        if (queue_.size() < policy_.max_queued_tasks) {
          auto task = std::move(tasks[next]);
          queue_.push_back({task.key, next, std::move(task.work), performance,
                            state});
          ++state->admitted;
          ++next;
          work_ready_.notify_one();
          continue;
        }
        state_changed_.wait_for(lock, std::chrono::milliseconds(1));
      }

      while (state->completed != state->admitted) {
        if (cancel.cancelled()) {
          state->cancelled = true;
          for (auto &source : state->task_cancellations) source.cancel();
        }
        state_changed_.wait_for(lock, std::chrono::milliseconds(1));
      }
      if (cancel.cancelled()) {
        state->cancelled = true;
        for (auto &source : state->task_cancellations) source.cancel();
      }
      lock.unlock();

      const auto first_failure = std::find_if(
          state->errors.begin(), state->errors.end(),
          [](const auto &error) { return error.has_value(); });
      const auto merge_count = state->cancelled
                                   ? std::size_t(0)
                                   : static_cast<std::size_t>(
                                         first_failure == state->errors.end()
                                             ? state->errors.size()
                                             : first_failure - state->errors.begin() + 1);
      if (performance.collector)
        for (std::size_t i = 0; i < merge_count; ++i)
          if (state->measurements[i].valid)
            performance.collector->merge(
                performance.stage, performance.role, tasks[i].key,
                state->measurements[i].nanoseconds,
                state->measurements[i].counters);
      for (const auto &error : state->errors)
        if (error) return *error;
      if (state->cancelled)
        return executor_error(boolean_error_code::resource_limit, "cancelled");
      return true;
    } catch (const std::bad_alloc &) {
      if (state) {
        std::unique_lock<std::mutex> lock(mutex_);
        state->cancelled = true;
        for (auto &source : state->task_cancellations) source.cancel();
        work_ready_.notify_all();
        state_changed_.wait(lock, [&] {
          return state->completed == state->admitted;
        });
      }
      return executor_error(boolean_error_code::resource_limit, "allocation");
    } catch (const std::exception &e) {
      if (state) {
        std::unique_lock<std::mutex> lock(mutex_);
        state->cancelled = true;
        for (auto &source : state->task_cancellations) source.cancel();
        work_ready_.notify_all();
        state_changed_.wait(lock, [&] {
          return state->completed == state->admitted;
        });
      }
      auto error = executor_error(boolean_error_code::internal_invariant_error,
                                  "executor_exception");
      error.detail = e.what();
      return error;
    } catch (...) {
      if (state) {
        std::unique_lock<std::mutex> lock(mutex_);
        state->cancelled = true;
        for (auto &source : state->task_cancellations) source.cancel();
        work_ready_.notify_all();
        state_changed_.wait(lock, [&] {
          return state->completed == state->admitted;
        });
      }
      return executor_error(boolean_error_code::internal_invariant_error,
                            "executor_exception_unknown");
    }
  }
};

} }
#endif
