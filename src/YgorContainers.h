//YgorContainers.h - A collection of some custom (niche?) containers. Most likely the contents of this file exist somewhere as more
//  mature implementations with well thought-out uses.

#ifndef YGOR_CUSTOM_CONTAINERS_HC_
#define YGOR_CUSTOM_CONTAINERS_HC_

#include <algorithm>
#include <atomic>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <cstddef>  // For std::ptrdiff_t

#include "YgorDefinitions.h"
#include "YgorMisc.h"            //Needed for function macros FUNCINFO, FUNCWARN, FUNCERR.
#include "YgorLog.h"

//----------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------- bimap -------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// Bimap - A bi-directional map. Useful for describing a set of items which relate to one another with one-to-one correspondance.
// This implementation 'piggybacks' on some simple STL containers to give a very crippled (but useable) container.
//
// This container is a poor-man's version of the Boost::Bimap library. Q: Why not just use Boost? A: To increase portability, this 
// will only depend on the C++ compiler's support of the STL. If performance or flexibility are desired, simply switch to Boost.
//
//Notes:
//   1) For simplicity in this implementation, please do NOT use this container with TA == TB.

template <class TA, class TB>   class bimap { 
    private:
        std::vector< std::pair<TA,TB> >  the_pairs;

    public:

        //Constructors.
        bimap();

        //Member functions.
        TA & operator[]( const TB &in );
        TB & operator[]( const TA &in );

        bimap & operator=(const std::map<TA,TB> &);
        bimap & operator=(const std::map<TB,TA> &);

        typename std::vector< std::pair<TA,TB> >::const_iterator  begin( void ) const;
        typename std::vector< std::pair<TA,TB> >::const_reverse_iterator  rbegin( void ) const;
        typename std::vector< std::pair<TA,TB> >::const_iterator  end( void ) const;
        typename std::vector< std::pair<TA,TB> >::const_reverse_iterator  rend( void ) const;
        
        typename std::vector< std::pair<TA,TB> >::const_iterator  find( const TA &in ) const;
        typename std::vector< std::pair<TA,TB> >::const_iterator  find( const TB &in ) const;

        //Given an element, these functions return a pointer to the next (cyclically.) Helps avoid a find/iterate/check procedure.
        template <class T> typename std::vector< std::pair<TA,TB> >::const_iterator  after( const T &in ) const;
        template <class T> typename std::vector< std::pair<TA,TB> >::const_iterator  before( const T &in ) const;

        //Given an element, these functions return the desired element (or the given element, if something fails.)
        template <class T> T get_next( const T &in ) const;
        template <class T> T get_previous( const T &in ) const;
   
        void order_on_first(void);
        void order_on_second(void);


        //  erase? 
        //  non-const iterators? (<-- Do I just need to remove the trailing const in the function def?)
        //  size() ?  ((needed?))
        //  empty() ?
};


//---------------------------------------------------------------------------------------------------------------------
//--------------------------- yspan: a non-owning sequence proxy object supporting stride -----------------------------
//---------------------------------------------------------------------------------------------------------------------
// This class is a non-owning object similar to std::span, but supporting runtime stride. The underlying sequence is
// treated as an opaque binary array to support heterogeneous sequences (e.g., [float, double, int, long int]).

template <class T>
class yspan {
    private:
        T* start;
        long int count; // the number of T present.
        long int stride_bytes; // in bytes, to support heterogeneous embeddings.

    public:
        using value_type = T;

        //Constructors.
        yspan();
        yspan(T* start, long int count, long int stride_bytes);
        yspan(const yspan &);

        //Operators.
        yspan & operator=(const yspan &);

        bool operator==(const yspan &) const;
        bool operator!=(const yspan &) const;
        bool operator< (const yspan &) const;

        // Accessors.
        T& operator[](long int);
        T& at(long int);

        long int size() const; // number of T included in yspan.
        long int stride() const; // number of bytes in the stride.

        bool empty() const;
        T& front();
        T& back();

        // Remove elements from the span, but leave them intact in the underlying storage.
        // NOTE: both pop_front() and pop_back() invalidate *all* iterators (begin() and end()).
        void pop_front();
        void pop_back();

        // Other members.
        void swap(yspan &); // Swaps the contents -- essentially a destructive operator=().

        // Iterators.
        struct iterator {
            public:
                using iterator_category = std::random_access_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = T;
                using pointer           = T*;
                using reference         = T&;

                iterator() : it_yspan(nullptr), it_n(-1L) {}
                iterator(yspan<T>* ys, long int n) : it_yspan(ys), it_n(n) {}

                reference operator*() const { return (*(this->it_yspan))[this->it_n]; }
                pointer operator->(){ return &((*(this->it_yspan))[this->it_n]); }
                bool operator<(const iterator &rhs){ 
                    return std::make_tuple(*(this->it_yspan), this->it_n)
                         < std::make_tuple(*(rhs.it_yspan), rhs.it_n);
                }

                // Prefix increment/decrement
                iterator& operator++(){ this->it_n++; return *this; }
                iterator& operator--(){ this->it_n--; return *this; }

                // Postfix increment/decrement
                iterator operator++(int){ iterator tmp = *this; ++(*this); return tmp; }
                iterator operator--(int){ iterator tmp = *this; --(*this); return tmp; }

                difference_type operator-(const iterator &rhs){ return static_cast<difference_type>(this->it_n - rhs.it_n); }
                iterator& operator+=(difference_type n){ this->it_n += n; return *this; }
                iterator& operator-=(difference_type n){ this->it_n -= n; return *this; }

                iterator operator+(difference_type n){ iterator out = *this; out.it_n += n; return out; }
                iterator operator-(difference_type n){ iterator out = *this; out.it_n -= n; return out; }

                friend bool operator==(const iterator& A, const iterator& B){
                    return (A.it_yspan == B.it_yspan)
                        && (A.it_n == B.it_n);
                };

                friend bool operator!=(const iterator& A, const iterator& B){
                    return !(A == B);
                };

            private:
                yspan<T>* it_yspan;
                long int it_n;
        };

        iterator begin(){
            return iterator(&(*this), 0L);
        }
        iterator end(){
            return iterator(&(*this), this->count);
        }
};


//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------- taskqueue ------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// Task Queue - A single-(additional)-threaded function evaluator. All tasks are performed in a std::thread, but are 
// performed sequentially. The utility of this container is that the queue is consumed by the thread asynchronously. The
// only interaction with the consuming thread is to push tasks into the queue (may block for small time) and intentionally
// blocking on events (such as queue being empty, completing current task, etc.).
//
// Each task is pushed onto the queue with an identical method: this->Queue(...). If a task is currently being performed, 
// then the task is queued until the previous task is complete. If no current task is being performed, execution happens 
// (nearly) immediately.
//
// This container is essentially a thread pool with a single thread consumer.
//
class taskqueue {
    private:
        //This is the basic queue, where tasks are pushed. There is probably no reason why it could not be templated.
        std::list<std::function<void (void)>> queue;
        std::thread queue_thread;

        //Long-blocking mutexes - these block while doing arbitrary computation and thus may block indefinately.
        std::mutex entire_queue_mutex;   //Gets flipped on when the first task is launched, off when the last is completed.
        std::mutex single_task_mutex;    //Gets flipped on when a single task is launched, off when it returns.

        //Short-blocking mutexes - these block during simple tasks like queue alterations and task (lambda) pushing.
        std::mutex queue_access_mutex;        //Gets flipped on when queue_thread is being used.
        std::mutex launching_mutex;           //Gets flipped on when launching the task thread initially, off after launch.
        std::mutex thread_has_launched_mutex; //Used to block the main thread until first item launches. Used to prevent 
                                              // 'sneaky' priority tasks prior to first task..

        //Callbacks which are checked/performed upon specific actions. Each comes with a mutex which locks access.
        // Note: these mutexs are simple locks which lock for access AND during execution, so they *may* lock forever.
        std::function<void (void)> Callback_Queue_Emptied;
        std::mutex callback_queue_emptied_mutex;

        //This will launch tasks. It is robust enough to handle being called when there is currently a task. It should not
        // be directly called without going through some wrapper functions. 
        //
        //Returns 'true' if there were no errors encountered. 
        bool Launch(void){
            this->launching_mutex.lock(); 

            //Check if the queue thread is currently doing something. If it is, we do not have to do anything.
            if(this->queue_thread.joinable() == 1){
                this->launching_mutex.unlock();
                return true; //Everything A-O-K.
            }
            if(!this->thread_has_launched_mutex.try_lock()){
                YLOGWARN("The 'thread has launched' mutex is locked. It is used to block on thread creation and should not remain locked. Bailing");
                this->launching_mutex.unlock();
                return false; //Something went wrong.
            }

            //This is the lambda function we will launch in another thread. It will iterate over the tasks (which can be popped
            // on in realtime) until they are all complete. It erases tasks which have been completed.
            //
            //NOTE: It is more convenient to lock the entire_queue_mutex just prior to calling this lambda. See below.
            auto lambda = [&](void) -> void {
                //Iterate over the queue. Note that we do not explicitly need to iterate because we 'gobble up' the list 
                // from the front by erasing tasks which we complete.
                this->queue_access_mutex.lock();    //Lock the queue PRIOR to allowing threads access by unblocking the launching thread.
                                                    //  This is desired so that we cannot have a 'fast' thread Queue() a priority task very quickly,
                                                    //  which will NOT be the same behaviour for other systems: uniformity is better to have!
                thread_has_launched_mutex.unlock(); //Unlock the 'thread has launched' mutexbecause we are now 'running free' in a thread.

                for(auto it = this->queue.begin(); it != this->queue.end();     ){  // ..and NOT: ++it){
                    //Check if the element queued still exists. If it does not, we remove it and continue on.
                    if(!(*it)){
                        YLOGWARN("Unable to perform task - the reference appears to be invalid. Removing it and continuing");
                        it = this->queue.erase(it);
                        continue;
                    }

                    //We copy, test for a lock, and remove this task from the queue. Then we relinquish queue control to the masses. 
                    auto f = std::move(*it);
                    if( !this->single_task_mutex.try_lock() ){
                        YLOGWARN("The single task mutex is currently locked. Is there already a thread performing these tasks? Bailing");
                        this->queue_access_mutex.unlock();
                        return;
                    }
                    this->queue.erase(it);
                    this->queue_access_mutex.unlock();

                    //Perform the task. We have a local copy, so the queue remains available to the masses.
                    f();
                    this->single_task_mutex.unlock();
    
                    //Get back control of the queue. Determine whether or not we can should continue. We do not
                    // assume any of our iterators are still valid.
                    this->queue_access_mutex.lock();
                    if(this->queue.empty()){
                        break;
                    }else{
                        it = this->queue.begin();
                    }
                }
                this->queue_access_mutex.unlock();

                //Perform the queue-end callback, if one exists.
                this->callback_queue_emptied_mutex.lock();
                if(this->Callback_Queue_Emptied){
                    this->Callback_Queue_Emptied();
                }
                this->callback_queue_emptied_mutex.unlock();
 
                //Detach this thread. Unlock the entire_queue_mutex, signifying we have consumed all tasks in the queue.
                this->queue_thread.detach();
                this->entire_queue_mutex.unlock();
                return;
            };
    
            //Lock the entire queue mutex. Launch the thread.
            if( !this->entire_queue_mutex.try_lock() ){
                YLOGWARN("Unable to initiate tasks - entire_queue_mutex is locked. Is another thread already running?");
                this->launching_mutex.unlock();
                this->thread_has_launched_mutex.unlock();
                return false;
            }
            this->queue_thread = std::thread( lambda );

            //Wait on the queue lock for the thread to begin. This guarantees us sequential operations upon Queue()'ing.
            this->thread_has_launched_mutex.lock(); //If the thread did not unlock, we would wait here forever!
            this->thread_has_launched_mutex.unlock();

            //Release the Launch() mutex so that waiting Launch()'ers can Launch().
            launching_mutex.unlock();
            return true;
        }

    public:

        //----- Constructors/Destructor.
        taskqueue(void){   this->queue.clear();          }
        ~taskqueue(void){  this->Wait_For_Empty_Queue(); }

        //----- Methods - Non-blocking.
        //This is the quintessential user function.
        bool Queue(std::function<void (void)> in){            //Insert the task in the back of the queue.
            this->queue_access_mutex.lock();
            this->queue.push_back(in);
            this->queue_access_mutex.unlock();
            return this->Launch();
        }

        bool Queue_Priority(std::function<void (void)> in){   //Insert the task in the front of the queue.
            this->queue_access_mutex.lock();
            this->queue.push_front(in);
            this->queue_access_mutex.unlock();
            return this->Launch();
        }

        decltype(queue) Acquire_Remaining_Queue(void){   //Passes ownership of remaining tasks out.
            this->queue_access_mutex.lock();
            decltype(queue) out(std::move(this->queue));
            this->queue.clear();
            this->queue_access_mutex.unlock();
            return out;
        }

        long int QueueSize(void){  //Returns the current size of the queue.
            long int l_size = 0;
            this->queue_access_mutex.lock();
            l_size = static_cast<long int>(this->queue.size());
            this->queue_access_mutex.unlock();
            return l_size;
        }

        //----- Callback functions.
        void Set_Callback_Queue_Emptied(std::function<void (void)> in){
            this->callback_queue_emptied_mutex.lock();
            this->Callback_Queue_Emptied = in;
            this->callback_queue_emptied_mutex.unlock();
            return;
        }

        //----- Methods - Blocking.
        //These let the user wait on tasks. Typically used to determine when to deallocate/destruct/destroy/terminate.
        void Wait_For_Current_Task(void){
            //Block on the current task mutex. Immediately unlock and return afterward.
            this->single_task_mutex.lock();
            this->single_task_mutex.unlock();
            return;
        }
        void Wait_For_Empty_Queue(void){
            //Block on the entire queue mutex. Immediately unlock and return afterward.
            this->entire_queue_mutex.lock();
            this->entire_queue_mutex.unlock();
            return;
        }
};

//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------ taskpool ------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// Task Pool - A multi-threaded function evaluator. A given task is pushed into a queue. At the other end of the queue, 
// there is a pool of threads executing the tasks (ie. 'consuming' them). The tasks are performed by whichever thread is
// free at a given point. If no threads are available, and there are less than N threads currently running, new threads 
// are pooled to handle the tasks. Consequently, when a thread finishes and finds no remaining tasks, it sepuku's itself
// from the pool.
//
// The order in which tasks are consumed is FIFO. Which thread they will be executed in is not known (and shouldn't matter
// because all threads are created equally).
//
class taskpool {
    private:
        std::atomic<long int> N; //The maximum number of threads in the pool. This can be adjusted by the user at runtime!

        //This is the basic queue, where tasks are pushed. There is probably no reason why it could not be templated.
        std::list<std::function<void (void)>> queue;
        std::list<std::thread> pool;  //Need to use a list because removal will not invalidate pointers.

        //Long-blocking mutexes - these block while doing arbitrary computation and thus may block indefinately.
        std::mutex entire_queue_mutex;   //Gets flipped on when the first task is launched, off when the last is completed.

        //Short-blocking mutexes - these block during simple tasks like queue alterations and task (lambda) pushing.
        std::mutex pool_access_mutex;           //Used for all operations involving the pool.
        std::mutex queue_access_mutex;          //Gets flipped on when queue_thread is being used.

//        std::mutex launching_mutex;           //Gets flipped on when launching the task thread initially, off after launch.
//        std::mutex thread_has_launched_mutex; //Used to block the main thread until first item launches. Used to prevent 
                                              // 'sneaky' priority tasks prior to first task..

        //Callbacks which are checked/performed upon specific actions. Each comes with a mutex which locks access.
        // Note: these mutexs are simple locks which lock for access AND during execution, so they *may* lock forever.
//        std::function<void (void)> Callback_Queue_Emptied;
//        std::mutex callback_queue_emptied_mutex;

        //This will launch a thread into the thread pool. The thread will eventually sepuku itself if it finds the queue
        // empty, so we do not need to track it (except that it will live in the list of threads).
        //
        //Returns 'true' if there were no errors encountered. 
        bool Launch_Thread(void){
            //Check if the entire queue mutex is locked or not. If not, lock it.
            this->entire_queue_mutex.try_lock();

            //This is the 'brains' of each thread. 
            auto lambda = [&](std::list<std::thread>::iterator self) -> void {

                //Chew through tasks from the queue.
                this->queue_access_mutex.lock();
                while(!this->queue.empty()){
                    auto it =  this->queue.begin();
                    if(!(*it)){
                        YLOGWARN("Unable to perform task - the reference appears to be invalid. Removing it and continuing");
                        it = this->queue.erase(it);
                        continue;
                    }

                    //We copy, test for a lock, and remove this task from the queue. Then we relinquish queue control to the masses. 
                    auto f = std::move(*it);
                    this->queue.erase(it);
                    this->queue_access_mutex.unlock();

                    //Perform the task. We have a local copy, so the queue remains available to the masses.
                    f();

                    //Get back control of the queue. 
                    this->queue_access_mutex.lock();
                }
                this->queue_access_mutex.unlock();

                //Sepuku. Remove self from the thread pool list.
                std::list<std::thread> underground_railroad;
                this->pool_access_mutex.lock();
                underground_railroad.push_back(std::move(*self));
                this->pool.erase(self);
                const auto alive_threads = static_cast<long int>(this->pool.size());
                this->pool_access_mutex.unlock();
                
                //Check if there are any more items in the queue (which may have been queued since earlier).
                //
                //NOTE: This is not watertight. Maybe we should use an atomic/special lock to determine when threads die...
                if(alive_threads == 0){
                    this->queue_access_mutex.lock();
                    if(this->queue.empty()) this->entire_queue_mutex.unlock();
                    this->queue_access_mutex.unlock();
                }

                //return true;
                //underground_railroad.clear();
                ((underground_railroad.begin()))->detach();
            };            

            this->pool_access_mutex.lock();
            this->pool.push_front(std::thread());
            const auto it = this->pool.begin();
            this->pool_access_mutex.unlock();

            *it = std::thread( lambda, it );

            //it->detach();
            return true;
        }

        bool Spawn_Thread_If_Needed(void){
            //Check if there are <N threads running. If so, spawn a new one.
            const auto n = this->N.load();
            this->pool_access_mutex.lock();
            const bool spawnthread = (static_cast<long int>(pool.size()) < n);
            this->pool_access_mutex.unlock();

            if(spawnthread){
                //YLOGINFO("We have not spawned the max number of threads. Launching thread now");
                return this->Launch_Thread();
            }else{
                //YLOGINFO("We have spawned the limit number of threads. Nothing to do, so leaving");
                return true;
            }
        }

    public:
        //----- Constructors/Destructor.
        taskpool(void){   
            this->queue.clear();
            const auto hwn = 2*std::thread::hardware_concurrency();
            const auto n   = static_cast<long int>((hwn > 0) ? hwn : 2);  //Fall back on two threads. Seems reasonable.
            this->N.store(n);
            //YLOGINFO("Using " << n << " threads in the pool");
        }
        ~taskpool(void){
            //YLOGINFO("Waiting for queue to empty");
            this->Wait_For_Empty_Queue();  //Wait for the tasks to finish to avoid memory loss.
        }

        //----- Methods - Non-blocking.
        //This is the quintessential user function.
        bool Queue(std::function<void (void)> in){            //Insert the task in the back of the queue.
            //Push the task into the queue. It may be consumed prior to launching a thread to execute it. This is OK!
            this->queue_access_mutex.lock();
            this->queue.push_back(in);
            this->queue_access_mutex.unlock();

            return this->Spawn_Thread_If_Needed();
        }

        //This function inserts the task into the front of the queue, where it will be consumed ASAP.
        bool Queue_Priority(std::function<void (void)> in){
            //Push the task into the queue. It may be consumed prior to launching a thread to execute it. This is OK!
            this->queue_access_mutex.lock();
            this->queue.push_front(in);
            this->queue_access_mutex.unlock();

            return this->Spawn_Thread_If_Needed();
        }

        decltype(queue) Acquire_Remaining_Queue(void){   //Passes ownership of remaining tasks out.
            this->queue_access_mutex.lock();
            decltype(queue) out(std::move(this->queue));
            this->queue.clear();
            this->queue_access_mutex.unlock();
            return out;
        }

        long int QueueSize(void){  //Returns the current size of the queue.
            this->queue_access_mutex.lock();
            const auto l_size = static_cast<long int>(this->queue.size());
            this->queue_access_mutex.unlock();
            return l_size;
        }

//        //----- Callback functions.
//        void Set_Callback_Queue_Emptied(std::function<void (void)> in){
//            this->callback_queue_emptied_mutex.lock();
//            this->Callback_Queue_Emptied = in;
//            this->callback_queue_emptied_mutex.unlock();
//            return;
//        }
//
//        //----- Methods - Blocking.
//        //These let the user wait on tasks. Typically used to determine when to deallocate/destruct/destroy/terminate.
//        void Wait_For_Current_Task(void){
//            //Block on the current task mutex. Immediately unlock and return afterward.
//            this->single_task_mutex.lock();
//            this->single_task_mutex.unlock();
//            return;
//        }

        void Wait_For_Empty_Queue(void){
            //Block on the entire queue mutex. Immediately unlock and return afterward.
            this->entire_queue_mutex.lock();
            this->entire_queue_mutex.unlock();
            return;
        }
};


#endif 
