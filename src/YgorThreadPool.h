//YgorThreadPool.h.

#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <list>
#include <type_traits>


// Multi-threaded work queue for offloading processing tasks.
//
// Note that if there is a single thread, then work is processed sequentially in FIFO order.

template<class T>
class work_queue {

  private:
    std::list<T> queue;
    std::mutex queue_mutex;

    std::condition_variable new_task_notifier;
    std::atomic<bool> should_quit = false;
    std::list<std::thread> worker_threads;

    std::condition_variable end_task_notifier;

  public:

    work_queue(unsigned int n_workers = std::thread::hardware_concurrency()){
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        
        // Exercise the condition variables and mutexes, ensuring they are initialized by the implementation.
        // This should effectively evaluate to a no-op, but also helps suppress false-positive warning messages in
        // Valgrind's DRD tool, i.e., 'not a condition variable', and other tools.
        this->new_task_notifier.notify_all(); // No threads waiting, so nothing to notify.
        this->end_task_notifier.notify_all(); // No threads waiting, so nothing to notify.
        this->new_task_notifier.wait_for(lock, std::chrono::nanoseconds(1) ); // No notifiers, so no signal to receive.
        this->end_task_notifier.wait_for(lock, std::chrono::nanoseconds(1) ); // No notifiers, so no signal to receive.

        auto l_n_workers = (n_workers == 0U) ? std::thread::hardware_concurrency()
                                             : n_workers;
        l_n_workers = (l_n_workers == 0U) ? 2U : l_n_workers;


        for(unsigned int i = 0; i < l_n_workers; ++i){
            this->worker_threads.emplace_back(
                [this](){
                    // Continually check the queue and wait on the condition variable.
                    while(true){

                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        while( !this->should_quit.load()
                               && this->queue.empty() ){

                            // Waiting releases the lock, which allows for work to be submitted.
                            //
                            // Note: spurious notifications are OK, since the queue will be empty and the worker will return to
                            // waiting on the condition variable.
                            this->new_task_notifier.wait_for(lock, std::chrono::seconds(2) );
                        }

                        if(this->queue.empty()){
                            return;
                        }

                        // Assume ownership of only the first item in the queue (FIFO).
                        auto task = std::move(this->queue.front());
                        this->queue.pop_front();

                        lock.unlock();

                        // Perform the work.
                        try{
                            if constexpr(std::is_constructible_v<bool, const T &>){
                                if(task) task();
                            }else{
                                task();
                            }
                        }catch(const std::exception &){}
                         catch(...){};

                        this->end_task_notifier.notify_one();
                    }
                }
            );
        }
    }

    void submit_task(T f){
        {
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            this->queue.push_back(std::move(f));
        }
        // Note: notifying without the mutex held avoids the situation where a notified thread wakes and immediately
        // blocks waiting for the mutex to be released. This is safe; condition_variable::notify_one does not require
        // the associated mutex to be held.
        this->new_task_notifier.notify_one();
        return;
    }

    std::list<T> clear_tasks(){
        std::list<T> out;
        std::lock_guard<std::mutex> lock(this->queue_mutex);
        out.swap( this->queue );
        return out;
    }

    ~work_queue(){
        // We can either cancel outstanding tasks or wait for all queued tasks to be completed.
        // Since there is a mechanism to clear queued tasks that have not been acquired by worker threads,
        // it is least-surprising to wait for all queued tasks to be completed before destructing.
        //
        // We rely on a condition variable to signal when tasks are completed, but fallback on occasional polling in
        // case there are any races to avoid waiting forever.
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            while( !this->queue.empty() ){

                // Waiting releases the lock while waiting, which still allows for outstanding work to be completed.
                //
                // We also periodically wake up in case there is a signalling race. For longer-running tasks, this will
                // hopefully be an insignificant amount of extra processing.
                this->end_task_notifier.wait_for(lock, std::chrono::milliseconds(2000) );
            }

            this->should_quit.store(true);
        }
        this->new_task_notifier.notify_all(); // notify threads to wake up and 'notice' they need to terminate.
        for(auto &wt : this->worker_threads) wt.join();
    }
};

