#include <YgorMeshesBooleanExecutor.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace ygor::mesh_boolean;

static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

static cancellation_token token() {
  static cancellation_source source;
  return source.token();
}

static void empty_and_ordered_success() {
  deterministic_executor executor({3, 2});
  require(executor.run({}, token()).has_value(), "empty run");
  std::mutex mutex;
  std::vector<std::uint64_t> finished;
  std::vector<deterministic_task> tasks;
  for (auto key : {9ULL, 1ULL, 5ULL, 3ULL})
    tasks.push_back({key, [&, key](cancellation_token) -> status_or<bool> {
      std::lock_guard<std::mutex> lock(mutex);
      finished.push_back(key);
      return true;
    }});
  require(executor.run(std::move(tasks), token()).has_value(), "successful run");
  std::sort(finished.begin(), finished.end());
  require((finished == std::vector<std::uint64_t>{1, 3, 5, 9}),
          "all tasks executed");
}

static void duplicate_keys_are_rejected_before_launch() {
  deterministic_executor executor({2, 1});
  std::atomic<unsigned> started{0};
  std::vector<deterministic_task> tasks;
  for (auto key : {4ULL, 2ULL, 4ULL})
    tasks.push_back({key, [&](cancellation_token) -> status_or<bool> {
      ++started;
      return true;
    }});
  auto result = executor.run(std::move(tasks), token());
  require(!result.has_value(), "duplicate keys rejected");
  require(result.error().code == boolean_error_code::internal_invariant_error &&
              result.error().message_key == "duplicate_task_key",
          "duplicate key error");
  require(started == 0, "duplicates launch no work");
}

static void lowest_key_failure_wins() {
  deterministic_executor executor({3, 8});
  std::vector<deterministic_task> tasks;
  tasks.push_back({30, [](cancellation_token) -> status_or<bool> {
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::input_validation, "key_30", 30);
  }});
  tasks.push_back({10, [](cancellation_token) -> status_or<bool> {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto error = make_error(boolean_error_code::output_not_representable,
                            boolean_stage::geometry_realization, "key_10", 10);
    error.detail = "preserved provenance";
    return error;
  }});
  tasks.push_back({20, [](cancellation_token) -> status_or<bool> {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::intersection_events, "key_20", 20);
  }});
  auto result = executor.run(std::move(tasks), token());
  require(!result.has_value(), "task failure returned");
  require(result.error().message_key == "key_10" &&
              result.error().subcode == 10 &&
              result.error().stage == boolean_stage::geometry_realization &&
              result.error().detail == "preserved provenance",
          "lowest canonical failure and provenance");
}

static void exceptions_are_typed_and_executor_is_reusable() {
  deterministic_executor executor({3, 3});
  std::vector<deterministic_task> tasks;
  tasks.push_back({3, [](cancellation_token) -> status_or<bool> { throw 7; }});
  tasks.push_back({2, [](cancellation_token) -> status_or<bool> {
    throw std::runtime_error("sentinel");
  }});
  tasks.push_back({1, [](cancellation_token) -> status_or<bool> { throw std::bad_alloc(); }});
  auto result = executor.run(std::move(tasks), token());
  require(!result.has_value(), "exception translated");
  require(result.error().code == boolean_error_code::resource_limit &&
              result.error().message_key == "allocation",
          "lowest-key allocation selected");
  std::vector<deterministic_task> retry;
  retry.push_back({1, [](cancellation_token) -> status_or<bool> { return true; }});
  require(executor.run(std::move(retry), token()).has_value(),
          "executor reusable after exception");
}

static std::uint64_t failed_run_counter(std::uint32_t threads) {
  performance_collector collector;
  performance_scope stage(&collector, boolean_stage::intersection_events,
                          performance_role::producer);
  deterministic_executor executor({threads, 8});
  std::vector<deterministic_task> tasks;
  for (std::uint64_t key = 1; key <= 4; ++key)
    tasks.push_back({key, [key](cancellation_token) -> status_or<bool> {
      performance_count(performance_counter::exact_equality_checks, key);
      if (key == 1)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::intersection_events, "failed_task",
                        static_cast<std::uint32_t>(key));
    }});
  auto result = executor.run(std::move(tasks), token());
  require(!result.has_value() && result.error().subcode == 1,
          "counter test canonical failure");
  stage.finish();
  return collector.snapshot()
      ->stage(boolean_stage::intersection_events)
      .producer.value(performance_counter::exact_equality_checks);
}

static void failure_counters_are_schedule_independent() {
  require(failed_run_counter(1) == 1, "serial failed-run counter");
  require(failed_run_counter(4) == 1, "parallel failed-run counter");
}

static void failure_cancels_higher_key_active_work() {
  deterministic_executor executor({2, 2});
  std::atomic<bool> higher_observed{false};
  std::vector<deterministic_task> tasks;
  tasks.push_back({1, [](cancellation_token) -> status_or<bool> {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::intersection_events, "lower_failure");
  }});
  tasks.push_back({2, [&](cancellation_token task_cancel) -> status_or<bool> {
    while (!task_cancel.cancelled()) std::this_thread::yield();
    higher_observed = true;
    return true;
  }});
  auto result = executor.run(std::move(tasks), token());
  require(!result.has_value() && result.error().message_key == "lower_failure",
          "lower failure retained after worker cancellation");
  require(higher_observed, "higher-key active task observed failure cancellation");
}

static void cancellation_at_launch() {
  deterministic_executor executor({2, 1});
  cancellation_source source;
  source.cancel();
  std::atomic<unsigned> started{0};
  std::vector<deterministic_task> tasks;
  tasks.push_back({1, [&](cancellation_token) -> status_or<bool> {
    ++started;
    return true;
  }});
  auto result = executor.run(std::move(tasks), source.token());
  require(!result.has_value() && result.error().message_key == "cancelled",
          "launch cancellation result");
  require(started == 0, "launch cancellation starts no work");
}

static void cancellation_stops_saturated_queue_and_joins() {
  deterministic_executor executor({2, 1});
  cancellation_source source;
  std::mutex mutex;
  std::condition_variable changed;
  bool release = false;
  std::atomic<unsigned> started{0}, finished{0};
  std::atomic<unsigned> observed_cancellation{0};
  std::vector<deterministic_task> tasks;
  for (std::uint64_t key = 0; key < 20; ++key)
    tasks.push_back({key, [&](cancellation_token task_cancel) -> status_or<bool> {
      ++started;
      changed.notify_all();
      std::unique_lock<std::mutex> lock(mutex);
      changed.wait(lock, [&] { return release || task_cancel.cancelled(); });
      if (task_cancel.cancelled()) ++observed_cancellation;
      ++finished;
      return true;
    }});

  status_or<bool> result = true;
  std::thread caller([&] {
    result = executor.run(std::move(tasks), source.token());
  });
  {
    std::unique_lock<std::mutex> lock(mutex);
    require(changed.wait_for(lock, std::chrono::seconds(2),
                             [&] { return started == 2; }),
            "workers reached saturation");
  }
  source.cancel();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  {
    std::lock_guard<std::mutex> lock(mutex);
    release = true;
  }
  changed.notify_all();
  caller.join();
  require(!result.has_value() && result.error().message_key == "cancelled",
          "mid-stage cancellation result");
  require(started == 2, "queued and unlaunched work cancelled");
  require(finished == started, "active tasks joined before return");
  require(observed_cancellation == started,
          "active tasks observe executor cancellation");
}

int main() {
  struct test {
    const char *name;
    void (*run)();
  } tests[] = {{"empty_and_ordered_success", empty_and_ordered_success},
               {"duplicate_keys", duplicate_keys_are_rejected_before_launch},
               {"lowest_key_failure", lowest_key_failure_wins},
               {"exceptions", exceptions_are_typed_and_executor_is_reusable},
               {"failure_counters", failure_counters_are_schedule_independent},
               {"failure_cancellation", failure_cancels_higher_key_active_work},
               {"cancellation_at_launch", cancellation_at_launch},
               {"cancellation_mid_stage",
                cancellation_stops_saturated_queue_and_joins}};
  int failures = 0;
  for (const auto &test : tests) {
    try {
      test.run();
      std::cout << "PASS " << test.name << '\n';
    } catch (const std::exception &e) {
      ++failures;
      std::cerr << "FAIL " << test.name << ": " << e.what() << '\n';
    }
  }
  return failures ? 1 : 0;
}
