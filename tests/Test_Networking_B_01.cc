
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorContainers.h"


long int Slow_Nth_Fibonacci(long int count){
    if(count <  0) FUNCERR("Cannot compute that!");
    if(count == 0) return 0;
    if(count == 1) return 1;
    if(count == 2) return 1;
    return Slow_Nth_Fibonacci(count - 2) + Slow_Nth_Fibonacci(count - 1);
}


int main(int argc, char **argv){

//    FUNCINFO( "Main thread:  35th number is " << Slow_Nth_Fibonacci(  35) ); 

    taskqueue tasks;
    FUNCINFO("Queuing in order: [A,C,D,E,B]");

    tasks.Queue( [](void) -> void { 
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 )) );
        FUNCINFO( "Task: A");  //Should *block* until this task begins.
        return; 
    });
    tasks.Queue_Priority( [](void) -> void {
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 )) );
        FUNCINFO( "Task: C");  //Gets placed right after A.
        return;
    });
    tasks.Queue( [](void) -> void {  
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 )) );
        FUNCINFO( "Task: D"); //Gets placed after C.
        return; 
    });
    tasks.Queue( [](void) -> void {  
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 )) );
        FUNCINFO( "Task: E"); //Gets placed after D.
        return; 
    });
    tasks.Queue_Priority( [](void) -> void {  
        std::this_thread::sleep_for( std::chrono::milliseconds( static_cast<long int>( 1 * 1E3 )) );
        FUNCINFO( "Task: B"); //Gets placed right after A, jumping in front of C.
        return; 
    });

    FUNCINFO("Should see order: [A,B,C,D,E]"); 

    FUNCINFO("Waiting now for queue to empty");
    tasks.Wait_For_Empty_Queue();
    FUNCINFO("Queue is empty");

    return 0;
};
