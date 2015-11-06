//YgorPerformance.h
#ifndef YGOR_PERFORMANCE_HDR_GRD_H
#define YGOR_PERFORMANCE_HDR_GRD_H

#include <ctime>

//------------------------------------------------------------------------------------------------------
//------------------------------------------- Timing Routines ------------------------------------------
//------------------------------------------------------------------------------------------------------
double YgorPerformance_Get_Time(void);
void YgorPerformance_dt_from_last(void);

#ifndef YGORPERF_TIMESTAMP_DT_IS_DEFINED
    #define YGORPERF_TIMESTAMP_DT_IS_DEFINED
    #define YGORPERF_TIMESTAMP_DT       {std::cerr << "--(T) File: " << __FILE__; \
                                        std::cerr << ", Line: " << __LINE__; \
                                        std::cerr << " "; \
                                        YgorPerformance_dt_from_last(); \
                                        std::cerr.flush(); }
#endif


#endif

