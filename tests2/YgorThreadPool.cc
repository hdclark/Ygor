#include <atomic>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>
#include <mutex>

#include <YgorThreadPool.h>

#include "doctest/doctest.h"


TEST_CASE( "work_queue" ){

    SUBCASE("basic task execution"){
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool(2);
            pool.submit_task([&counter](){ ++counter; });
            pool.submit_task([&counter](){ ++counter; });
            pool.submit_task([&counter](){ ++counter; });
        }
        REQUIRE(counter.load() == 3);
    }

    SUBCASE("single worker sequential FIFO order"){
        std::vector<int> results;
        std::mutex results_mutex;
        {
            work_queue<std::function<void()>> pool(1);
            for(int i = 0; i < 10; ++i){
                pool.submit_task([i, &results, &results_mutex](){
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(i);
                });
            }
        }
        REQUIRE(results.size() == 10);
        for(int i = 0; i < 10; ++i){
            REQUIRE(results[static_cast<size_t>(i)] == i);
        }
    }

    SUBCASE("handles std::exception without crashing"){
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool(2);
            pool.submit_task([](){ throw std::runtime_error("test"); });
            pool.submit_task([&counter](){ ++counter; });
        }
        REQUIRE(counter.load() == 1);
    }

    SUBCASE("handles non-std exceptions without crashing"){
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool(2);
            pool.submit_task([](){ throw 42; });
            pool.submit_task([&counter](){ ++counter; });
        }
        REQUIRE(counter.load() == 1);
    }

    SUBCASE("clear_tasks removes pending tasks"){
        std::atomic<int> counter{0};
        work_queue<std::function<void()>> pool(1);

        // Submit a blocking task to hold the single worker.
        std::atomic<bool> blocker{true};
        std::atomic<bool> started{false};
        pool.submit_task([&blocker, &started](){
            started.store(true);
            while(blocker.load()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

        // Wait for the blocking task to start.
        while(!started.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Submit tasks that should be clearable (worker is busy).
        pool.submit_task([&counter](){ ++counter; });
        pool.submit_task([&counter](){ ++counter; });
        pool.submit_task([&counter](){ ++counter; });

        auto cleared = pool.clear_tasks();
        REQUIRE(cleared.size() == 3);

        // Release the blocker so the destructor can join.
        blocker.store(false);
    }

    SUBCASE("many tasks with many workers"){
        const int N = 1000;
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool(8);
            for(int i = 0; i < N; ++i){
                pool.submit_task([&counter](){ ++counter; });
            }
        }
        REQUIRE(counter.load() == N);
    }

    SUBCASE("handles empty function gracefully"){
        {
            work_queue<std::function<void()>> pool(2);
            std::function<void()> empty_fn;
            pool.submit_task(empty_fn);
        }
        // Should not crash.
        REQUIRE(true);
    }

    SUBCASE("default constructor uses hardware concurrency"){
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool;
            pool.submit_task([&counter](){ ++counter; });
        }
        REQUIRE(counter.load() == 1);
    }

    SUBCASE("zero workers defaults to hardware concurrency"){
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool(0);
            pool.submit_task([&counter](){ ++counter; });
        }
        REQUIRE(counter.load() == 1);
    }

    SUBCASE("tasks submitted from within tasks"){
        std::atomic<int> counter{0};
        {
            work_queue<std::function<void()>> pool(2);
            pool.submit_task([&pool, &counter](){
                ++counter;
                pool.submit_task([&counter](){ ++counter; });
            });
        }
        REQUIRE(counter.load() == 2);
    }
}
