//YgorTime.h
#ifndef YGOR_TIME_HDR_GRD_H
#define YGOR_TIME_HDR_GRD_H

#include <cstdint>
#include <ctime>
#include <memory>
#include <string>

#include "YgorDefinitions.h"
#include "YgorSerialize.h"

//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------- time_mark class - a simple class for holding timestamps --------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
//This class has (roughly, no less than) second-resolution timing. It uses epoch time (i.e. some arbitrary point in time) as the 
// reference point.
//
//The underlying representation may change. Do not use this for anything that has to properly account for daylight savings, or 
// leap seconds, or less-than-seconds inherent resolution. This class is mostly good for logging or timestamps. Do NOT depend on
// any particular treatment of, say, daylight savings time, etc..
//
class time_mark {
    private:
        //This holds the basic time info. It is sequential time since epoch ("Epoch time.").
        time_t When;

    public:
        //Constructors, destructor.
        time_mark();  //Sets the time as current time. (Do not change this!)
        time_mark(time_t);
        time_mark(const time_mark &);
        ~time_mark();

        //Methods.
        void Set_current_time(void); //Set to current time...
        void Set_unix_epoch(void); //Set to the Unix epoch...

        time_mark & operator=(const time_mark &);
        bool operator==(const time_mark &) const; //Compared only to the second.
        bool operator<(const time_mark &) const;
        bool operator>(const time_mark &) const;
        bool operator<=(const time_mark &) const; //Compared only to the second.
        bool operator>=(const time_mark &) const; //Compared only to the second.

        //Time-shift (of specified time_mark) factories.
        time_mark One_Day_Earlier_Than(const time_mark &T) const;
        time_mark One_Day_Later_Than(const time_mark &T) const;  

        //Time-shift (of *this) factories.
        time_mark Less_By_Seconds(int64_t) const;
        time_mark More_By_Seconds(int64_t) const;

        time_mark Same_Day_Earliest() const; //Earliest time_mark occuring on the same day.
        time_mark Same_Day_Latest() const;   //Latest   time_mark occuring on the same day.


        //In-place time shifts.
        void Regress_One_Day(); //aka "rewind"
        void Advance_One_Day();

        void Regress_By_Seconds(int64_t);
        void Advance_By_Seconds(int64_t);



        //Distance-based comparisons.
        bool Is_within_X_seconds(const time_mark &T, long int X) const; //Can be + or - from the event. Absolute distance from event in seconds.

        //These will return:  (T_in - T_this).
        time_t Diff_in_Seconds(const time_mark &T) const; //Can be either positive or negative. Resolution: 1sec.
        time_t Diff_in_Days(const time_mark &T) const; //Can be either positive or negative. Resolution: 1day.

        //Absolute comparisons.
        bool Have_same_day(const time_mark &T) const;  //NOTE: This is same day only (not same month, not same year!) Compares only the number.

        bool Occur_on_same_day(const time_mark &T) const;  //Same day. Same month and year too.


        //Input/Output functions.
        std::string Dump_as_string(void) const;       //Format: `date +%Y%m%d-%H%M%S`.
        std::string Dump_as_postgres_string(void) const; //Format: `date +%Y-%m-%d %H:%M:%S`.
        bool Read_from_string(const std::string &in, double *fractional_second = nullptr); //Format: `date +%Y%m%d-%H%M%S` but a little lenient.

        //Serialize (deeply) to buffer starting at *offset. See source for more info.
        std::unique_ptr<uint8_t[]> Serialize(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t *tot_size) const;
        //Deserialize (deeply) from buffer starting at *offset. See source for more info.
        std::unique_ptr<uint8_t[]> Deserialize(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t tot_size);

        //Serialization helpers.
        uint64_t Theo_Max_Serialization_Size(void) const;

};


#endif

