//This program tests the taskpool class. It launches a bunch of pointless tasks and lets
// the user observe the variation in completion time/ordering. It also ensures the threads
// are able to safely 'sepuku' themselves when they are no longer needed.

#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

#include "YgorMisc.h"
#include "YgorContainers.h"


int main(int argc, char **argv){


    taskpool tasks;
    FUNCINFO("Queuing in order: [A,B,C,D,E,F,G,H]");

    tasks.Queue( [](void) -> void { 
        FUNCINFO("Beginning task: A");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 + 23 )) );
        FUNCINFO("Completed task: A");
        return; 
    });
    tasks.Queue( [](void) -> void {
        FUNCINFO("Beginning task: B");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 2 * 1E3  + 10 )) );
        FUNCINFO("Completed task: B");
        return;
    });
    tasks.Queue( [](void) -> void {  
        FUNCINFO("Beginning task: C");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 0 * 1E3 + 56 )) );
        FUNCINFO("Completed task: C");
        return; 
    });
    tasks.Queue( [](void) -> void {  
        FUNCINFO("Beginning task: D");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 2 * 1E3 + 10 )) );
        FUNCINFO("Completed task: D");
        return; 
    });
    tasks.Queue( [](void) -> void {  
        FUNCINFO("Beginning task: E");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 0 * 1E3 + 56 )) );
        FUNCINFO("Completed task: E");
        return; 
    });
    tasks.Queue( [](void) -> void {         
        FUNCINFO("Beginning task: F");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 3 * 1E3 + 89 )) );
        FUNCINFO("Completed task: F");
        return; 
    });
    tasks.Queue( [](void) -> void {         
        FUNCINFO("Beginning task: G");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 2 * 1E3 + 5 )) );
        FUNCINFO("Completed task: G");
        return; 
    });
    tasks.Queue( [](void) -> void {         
        FUNCINFO("Beginning task: H");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 + 1 )) );
        FUNCINFO("Completed task: H");
        return; 
    });

//    FUNCINFO("Should see order: [A,B,C,D,E,F,G,H]"); 

    FUNCINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    FUNCINFO("Queue is empty");

    tasks.Queue( [](void) -> void {
        FUNCINFO("Beginning task: I");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 + 23 )) );
        FUNCINFO("Completed task: I");
        return;
    });

    FUNCINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    FUNCINFO("Queue is empty");

    tasks.Queue( [](void) -> void {
        FUNCINFO("Beginning task: J");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 + 23 )) );
        FUNCINFO("Completed task: J");
        return;
    });
    tasks.Queue( [](void) -> void {
        FUNCINFO("Beginning task: K");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 2 * 1E3  + 10 )) );
        FUNCINFO("Completed task: K");
        return;
    });
    tasks.Queue( [](void) -> void {
        FUNCINFO("Beginning task: L");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 0 * 1E3 + 56 )) );
        FUNCINFO("Completed task: L");
        return;
    });
    tasks.Queue( [](void) -> void {
        FUNCINFO("Beginning task: M");
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 2 * 1E3 + 10 )) );
        FUNCINFO("Completed task: M");
        return;
    });

    FUNCINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    FUNCINFO("Queue is empty");


    return 0;
};
