//YgorPerformance.cc - Simple routines to help estimate performance.  
//
//  NOTE: These are (currently) not high-quality. They simply help DRY principle.

#include <iostream>
#include <sys/time.h>
#include <sstream>
#include <ctime>

#include "YgorMisc.h"
#include "YgorString.h"
#include "YgorPerformance.h"


//------------------------------------------------------------------------------------------------------
//------------------------------------------- Timing Routines ------------------------------------------
//------------------------------------------------------------------------------------------------------
double YgorPerformance_Get_Time(void){
    //Returns the number of seconds from an arbitrary point in the past (~Linux Epoch?)
    // For all purposes, it should be subtracted from another call at a later time to get a dt.
    //
    //Granularity of dt's should be around 10 microsecs AT BEST.
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + (tv.tv_usec * 1E-6);
}

void YgorPerformance_dt_from_last(void){
    //This routine is a simple, one-function interface. Call once to set the time. Call additional
    // times to get the dt since the last call.
    //
    //NOTE: This call is NOT threadsafe!
    static double YgorPerformance_dt_from_last_t = 0.0; //Will only be set to 0.0 the first time!

    if(YgorPerformance_dt_from_last_t == 0.0){
        std::cerr << "Initialized timing marker." << std::endl;
    }else{
        const auto t2 = YgorPerformance_Get_Time();
        std::cerr << "Time since last call: " << t2 - YgorPerformance_dt_from_last_t << " seconds " << std::endl;
    }
    YgorPerformance_dt_from_last_t = YgorPerformance_Get_Time();
    return;
}

/*
void NOT_TO_BE_CALLED_BY_USER_YgorPerformance_dt_from_last(void){
    static double NOT_TO_BE_CALLED_BY_USER_YgorPerformance_dt_from_last_t = 0.0; //Will only be set to 0.0 the first time!

    if(YgorPerformance_dt_from_last_t == 0.0){
        YgorPerformance_dt_from_last_t = YgorPerformance_Get_Time();
    }else{
        const auto t2 = YgorPerformance_Get_Time();
        std::cout << "Time since last call: " << t2 - NOT_TO_BE_CALLED_BY_USER_YgorPerformance_dt_from_last_t << " seconds " << std::endl;
        YgorPerformance_dt_from_last_t = YgorPerformance_Get_Time();
    }
    return;
}
*/

