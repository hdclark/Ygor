//This program tests the taskpool class. It launches a bunch of pointless tasks and lets
// the user observe the variation in completion time/ordering. It also ensures the threads
// are able to safely 'sepuku' themselves when they are no longer needed.

#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <cstdint>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorContainers.h"


int main(int argc, char **argv){


    taskpool tasks;
    YLOGINFO("Queuing in order: [A,B,C,D,E,F,G,H]");

    tasks.Queue( [](void) -> void { 
        YLOGINFO("Beginning task: A");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 + 23 )) );
        YLOGINFO("Completed task: A");
        return; 
    });
    tasks.Queue( [](void) -> void {
        YLOGINFO("Beginning task: B");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 2 * 1E3  + 10 )) );
        YLOGINFO("Completed task: B");
        return;
    });
    tasks.Queue( [](void) -> void {  
        YLOGINFO("Beginning task: C");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 0 * 1E3 + 56 )) );
        YLOGINFO("Completed task: C");
        return; 
    });
    tasks.Queue( [](void) -> void {  
        YLOGINFO("Beginning task: D");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 2 * 1E3 + 10 )) );
        YLOGINFO("Completed task: D");
        return; 
    });
    tasks.Queue( [](void) -> void {  
        YLOGINFO("Beginning task: E");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 0 * 1E3 + 56 )) );
        YLOGINFO("Completed task: E");
        return; 
    });
    tasks.Queue( [](void) -> void {         
        YLOGINFO("Beginning task: F");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 3 * 1E3 + 89 )) );
        YLOGINFO("Completed task: F");
        return; 
    });
    tasks.Queue( [](void) -> void {         
        YLOGINFO("Beginning task: G");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 2 * 1E3 + 5 )) );
        YLOGINFO("Completed task: G");
        return; 
    });
    tasks.Queue( [](void) -> void {         
        YLOGINFO("Beginning task: H");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 + 1 )) );
        YLOGINFO("Completed task: H");
        return; 
    });

//    YLOGINFO("Should see order: [A,B,C,D,E,F,G,H]"); 

    YLOGINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    YLOGINFO("Queue is empty");

    tasks.Queue( [](void) -> void {
        YLOGINFO("Beginning task: I");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 + 23 )) );
        YLOGINFO("Completed task: I");
        return;
    });

    YLOGINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    YLOGINFO("Queue is empty");

    tasks.Queue( [](void) -> void {
        YLOGINFO("Beginning task: J");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 + 23 )) );
        YLOGINFO("Completed task: J");
        return;
    });
    tasks.Queue( [](void) -> void {
        YLOGINFO("Beginning task: K");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 2 * 1E3  + 10 )) );
        YLOGINFO("Completed task: K");
        return;
    });
    tasks.Queue( [](void) -> void {
        YLOGINFO("Beginning task: L");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 0 * 1E3 + 56 )) );
        YLOGINFO("Completed task: L");
        return;
    });
    tasks.Queue( [](void) -> void {
        YLOGINFO("Beginning task: M");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 2 * 1E3 + 10 )) );
        YLOGINFO("Completed task: M");
        return;
    });

    YLOGINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    YLOGINFO("Queue is empty");


    return 0;
};
