#pragma once

#include "DeterministicExecutionTypes.h"
#include "Outcome.h"
#include "PlatformQualification.h"

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// The deterministic bounded FIFO worker service. `execute_parallel` runs a
// contiguous logical item range through either the serial reference (zero
// workers) or a bounded pool of background workers, then reduces per-task
// results in canonical task order so the outcome is independent of scheduling.
//
// WorkerFn   : Result worker(std::uint64_t begin, std::uint64_t end,
//                            const bounded_boolean_cancellation_token &)
// ReduceFn   : Result reduce(Result accumulator, const Result &next)
// The reduce is applied strictly in ascending task order; it must be a
// deterministic associative combine for the serial reference to equal the
// parallel result.
template <class Result, class WorkerFn, class ReduceFn>
boolean_outcome<Result> execute_parallel(
    std::uint64_t item_count, WorkerFn worker, ReduceFn reduce,
    Result identity, execution_capabilities capabilities) {
  const auto fail = [&](execution_subcode subcode,
                        bounded_boolean_error_category category,
                        const char *summary, execution_checkpoint checkpoint) {
    return boolean_outcome<Result>::failure(
        execution_error(subcode, category, summary, checkpoint));
  };

  if (capabilities.provider_version != contract_versions::execution_provider ||
      capabilities.codec_version != contract_versions::execution_codec ||
      capabilities.verifier_version != contract_versions::execution_verifier)
    return fail(execution_subcode::unsupported_version,
                bounded_boolean_error_category::input_contract_error,
                "execution capability version mismatch",
                execution_checkpoint::capability_validation);
  if (item_count > capabilities.maximum_items)
    return fail(execution_subcode::task_count_limit,
                bounded_boolean_error_category::resource_limit,
                "execution item count exceeds the configured limit",
                execution_checkpoint::plan_construction);
  if (execution_cancelled(capabilities,
                          execution_checkpoint::capability_validation))
    return fail(execution_subcode::cancelled,
                bounded_boolean_error_category::cancelled,
                "execution cancelled during capability validation",
                execution_checkpoint::cancellation);

  const bounded_boolean_cancellation_token cancellation =
      capabilities.cancellation ? *capabilities.cancellation
                                : bounded_boolean_cancellation_token{};

  // Determine the actual worker count.
  std::uint32_t actual_workers = 0;
  if (capabilities.maximum_workers > 0) {
    std::uint32_t desired = capabilities.requested_workers;
    if (desired == 0) {
      const unsigned hw = std::thread::hardware_concurrency();
      desired = static_cast<std::uint32_t>(
          std::min<std::uint64_t>(capabilities.maximum_workers,
                                  std::max<unsigned>(1, hw)));
    } else {
      desired = std::min(desired, capabilities.maximum_workers);
    }
    actual_workers = desired;
  }

  if (actual_workers == 0 || item_count <= 1) {
    Result result = worker(0, item_count, cancellation);
    return boolean_outcome<Result>::success(std::move(result));
  }

  // Partition into contiguous tasks (one per worker is the V1 grain).
  const std::uint64_t task_count = std::min<std::uint64_t>(
      static_cast<std::uint64_t>(actual_workers), item_count);
  std::vector<std::uint64_t> begins(task_count);
  std::vector<std::uint64_t> ends(task_count);
  for (std::uint64_t t = 0; t < task_count; ++t) {
    begins[t] = (item_count * t) / task_count;
    ends[t] = (item_count * (t + 1)) / task_count;
  }

  std::vector<std::optional<Result>> results(task_count);
  std::vector<bounded_boolean_error> errors(task_count);
  std::vector<bool> succeeded(task_count, false);

  std::mutex mutex;
  std::condition_variable condition;
  std::uint64_t next_task = 0;
  std::uint64_t completed = 0;
  bool cancel_signal = false;
  bool shutdown = false;

  auto run_tasks = [&]() {
    floating_environment_guard environment;
    const bool qualified = environment.qualified();
    while (true) {
      std::uint64_t task = execution_invalid_ordinal;
      {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [&]() {
          return cancel_signal || shutdown || next_task < task_count;
        });
        if (cancel_signal || shutdown)
          return;
        task = next_task++;
      }
      if (!qualified) {
        std::lock_guard<std::mutex> lock(mutex);
        errors[task] = execution_error(
            execution_subcode::floating_environment_failure,
            bounded_boolean_error_category::unsupported_platform,
            "worker floating environment is unqualified",
            execution_checkpoint::worker_creation);
        ++completed;
        condition.notify_all();
        continue;
      }
      try {
        Result result = worker(begins[task], ends[task], cancellation);
        std::lock_guard<std::mutex> lock(mutex);
        results[task] = std::move(result);
        succeeded[task] = true;
      } catch (const std::bad_alloc &) {
        std::lock_guard<std::mutex> lock(mutex);
        errors[task] = execution_error(
            execution_subcode::reservation_failure,
            bounded_boolean_error_category::resource_limit,
            "worker task allocation failed",
            execution_checkpoint::task_dispatch);
      } catch (...) {
        std::lock_guard<std::mutex> lock(mutex);
        errors[task] = execution_error(
            execution_subcode::task_exception,
            bounded_boolean_error_category::internal_invariant_error,
            "worker task threw an unexpected exception",
            execution_checkpoint::task_dispatch);
      }
      std::lock_guard<std::mutex> lock(mutex);
      ++completed;
      if (cancellation.cancellation_requested())
        cancel_signal = true;
      condition.notify_all();
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(task_count);
  try {
    for (std::uint64_t t = 0; t < task_count; ++t)
      threads.emplace_back(run_tasks);
  } catch (const std::exception &) {
    std::lock_guard<std::mutex> lock(mutex);
    shutdown = true;
    condition.notify_all();
    for (auto &thread : threads)
      thread.join();
    return fail(execution_subcode::worker_creation_failure,
                bounded_boolean_error_category::resource_limit,
                "worker creation failed", execution_checkpoint::worker_creation);
  }

  {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&]() {
      return completed == task_count || cancel_signal;
    });
  }
  {
    std::lock_guard<std::mutex> lock(mutex);
    shutdown = true;
    condition.notify_all();
  }
  for (auto &thread : threads)
    thread.join();

  if (cancel_signal)
    return fail(execution_subcode::cancelled,
                bounded_boolean_error_category::cancelled,
                "execution cancelled during task dispatch",
                execution_checkpoint::cancellation);

  // Reconcile and reduce in canonical task order.
  for (std::uint64_t t = 0; t < task_count; ++t)
    if (!succeeded[t])
      return boolean_outcome<Result>::failure(
          errors[t].subcode != 0
              ? errors[t]
              : execution_error(execution_subcode::missing_output_record,
                                bounded_boolean_error_category::internal_invariant_error,
                                "worker task produced no output",
                                execution_checkpoint::canonical_merge));
  Result accumulator = identity;
  for (std::uint64_t t = 0; t < task_count; ++t)
    accumulator = reduce(std::move(accumulator), *results[t]);
  return boolean_outcome<Result>::success(std::move(accumulator));
}

} // namespace ygor::mesh_boolean::bounded
