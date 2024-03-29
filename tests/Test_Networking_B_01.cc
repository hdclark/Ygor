
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <cstdint>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorContainers.h"


int64_t Slow_Nth_Fibonacci(int64_t count){
    if(count <  0) YLOGERR("Cannot compute that!");
    if(count == 0) return 0;
    if(count == 1) return 1;
    if(count == 2) return 1;
    return Slow_Nth_Fibonacci(count - 2) + Slow_Nth_Fibonacci(count - 1);
}


int main(int argc, char **argv){

//    YLOGINFO( "Main thread:  35th number is " << Slow_Nth_Fibonacci(  35) ); 

    taskqueue tasks;
    YLOGINFO("Queuing in order: [A,C,D,E,B]");

    tasks.Queue( [](void) -> void { 
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 )) );
        YLOGINFO( "Task: A");  //Should *block* until this task begins.
        return; 
    });
    tasks.Queue_Priority( [](void) -> void {
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 )) );
        YLOGINFO( "Task: C");  //Gets placed right after A.
        return;
    });
    tasks.Queue( [](void) -> void {  
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 )) );
        YLOGINFO( "Task: D"); //Gets placed after C.
        return; 
    });
    tasks.Queue( [](void) -> void {  
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 )) );
        YLOGINFO( "Task: E"); //Gets placed after D.
        return; 
    });
    tasks.Queue_Priority( [](void) -> void {  
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<int64_t>( 1 * 1E3 )) );
        YLOGINFO( "Task: B"); //Gets placed right after A, jumping in front of C.
        return; 
    });

    YLOGINFO("Should see order: [A,B,C,D,E]"); 

    YLOGINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    YLOGINFO("Queue is empty");

    return 0;
};
