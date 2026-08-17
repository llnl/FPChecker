#ifndef MC_TIME_INFO_INCLUDE
#define MC_TIME_INFO_INCLUDE

#include "QS_Precision.hh"


class MC_Time_Info
{
public:
    int    cycle;
    qs_real initial_time;
    qs_real final_time;
    qs_real time;
    qs_real time_step;

    MC_Time_Info() : cycle(0), initial_time(qs_real(0.0)), final_time(), time(qs_real(0.0)), time_step(qs_real(1.0)) {}

};



#endif
