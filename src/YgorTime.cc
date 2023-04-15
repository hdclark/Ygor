//YgorTime.cc - Simple routines for dealing with time.

#include <algorithm>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <regex>
#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorSerialize.h"
#include "YgorString.h"
#include "YgorTime.h"

//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------- time_mark class - a simple class for holding timestamps --------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
time_mark::time_mark(void){ 
    this->Set_current_time();
}

time_mark::time_mark(time_t t) : When(t) {}

time_mark::time_mark(const time_mark &T) : When(T.When) { }

time_mark::~time_mark(void){ }

void time_mark::Set_current_time(void){
    std::time(&this->When);
}

void time_mark::Set_unix_epoch(void){
    this->When = static_cast<time_t>(0);
}

std::string time_mark::Dump_as_string(void) const {
    //NOTE: Months and hours are 1-12, days are 1-31, but minutes and seconds are 0-59.
    //NOTE: In the future, update to use the std::put_time and std::get_time commands.
    //NOTE: If this needs to be changed, update this->Theo_Max_Serialization_Size. Consider making a new serialization version.
    struct tm lt;
#if !defined(_WIN32) && !defined(_WIN64)
    if(localtime_r(&this->When, &lt) == nullptr) YLOGERR("localtime_r produced an error - unable to continue");
//    if(gmtime_r(&this->When, &lt) == nullptr) YLOGERR("localtime_r produced an error - unable to continue");
#else
    if(localtime_s(&lt, &this->When) != 0) YLOGERR("localtime_s produced an error - unable to continue");
#endif
    std::stringstream ss("");

    //The previously-preferred way. It was easy to parse because of all the landmarks, but made little sense for most
    // practical purposes (including funneling date strings through bash, which was painful).
    //
    //Format: year/month/day-hour:minute:second
    //Format: `date +%Y\/%m\/%d-%H:%M:%S`  (slightly different because all numbers have preceeding 0's if needed. Will interoperate, I think) 
    //ss << (lt.tm_year+1900) << "/" << (lt.tm_mon+1)  << "/" << lt.tm_mday   << "-";
    //ss << lt.tm_hour        << ":" << lt.tm_min      << ":" << lt.tm_sec;

    //The presently-preferred way. Fixed-width numbers with a simple separator for the human.
    //Format: `date +%Y%m%d-%H%M%S` or "YearMonthDay-HourMinuteSecond". Example: "20131105-130535"
    ss << (lt.tm_year + 1900);
    ss << ((lt.tm_mon + 1) < 10 ? "0" : "") << (lt.tm_mon + 1);
    ss << (lt.tm_mday < 10 ? "0" : "") << lt.tm_mday << "-";
    ss << (lt.tm_hour < 10 ? "0" : "") << lt.tm_hour;
    ss << (lt.tm_min < 10 ? "0" : "") << lt.tm_min;
    ss << (lt.tm_sec < 10 ? "0" : "") << lt.tm_sec;
    return ss.str();
}

std::string time_mark::Dump_as_postgres_string(void) const {
    //NOTE: Months and hours are 1-12, days are 1-31, but minutes and seconds are 0-59.
    //NOTE: In the future, update to use the std::put_time and std::get_time commands.
    //NOTE: If this needs to be changed, update this->Theo_Max_Serialization_Size. Consider making a new serialization version.
    struct tm lt;
#if !defined(_WIN32) && !defined(_WIN64)
    if(localtime_r(&this->When, &lt) == nullptr) YLOGERR("localtime_r produced an error - unable to continue");
#else
    if(localtime_s(&lt, &this->When) != 0) YLOGERR("localtime_s produced an error - unable to continue");
#endif
    std::stringstream ss("");

    //The postgres-displayed (or at least psql) way. Fixed-width numbers with lots of landmarks and a space.
    //Format: `date +%Y-%m-%d %H:%M:%S` or "Year-Month-Day Hour:Minute:Second". Example: "2013-11-30 13:05:35"
    //
    //Try avoid this format because it contains an annoying space and lots of annoying landmarks.
    ss << (lt.tm_year + 1900) << "-";
    ss << ((lt.tm_mon + 1) < 10 ? "0" : "") << (lt.tm_mon + 1) << "-";
    ss << (lt.tm_mday < 10 ? "0" : "") << lt.tm_mday << " ";
    ss << (lt.tm_hour < 10 ? "0" : "") << lt.tm_hour << ":";
    ss << (lt.tm_min < 10 ? "0" : "") << lt.tm_min << ":";
    ss << (lt.tm_sec < 10 ? "0" : "") << lt.tm_sec;
    return ss.str();
}



//This routine is used in this->Read_from_string(...). It properly handles a fixed-width
// scheme, like where 'Jan. 25th, 2014 - 1:01:01 am' is represented as: '20140125-010101'.
//
//Returns true iff the string held a date/time in the afformentioned format (and nothing
// else). 
static bool Glean_date_time_from_string(const std::string &in, struct tm *ttm){
    if(ttm == nullptr) throw std::logic_error("Passed in an invalid struct tm pointer. This is a programming error");
    if(in.size() != 15) return false; //Fixed-width!
    std::string shtl;

    //Set the unused (or possibly irrelevant/unknown) flags to -1, as per:
    // "The mktime function converts the broken-down time, expressed as local time, in
    //  the structure pointed to by timeptr into a calendar time value with the same
    //  encoding as that of the values returned by the time function. The original
    //  values of the tm_wday and tm_yday components of the structure are ignored, and
    //  the original values of the other components are not restricted to the ranges
    //  indicated above. On successful completion, the values of the tm_wday and
    //  tm_yday components of the structure are set appropriately, and the other
    //  components are set to represent the specified calendar time, but with their
    //  values forced to the ranges indicated above; the final value of tm_mdayis not
    //  set until tm_mon and tm_year are determined."
    ttm->tm_isdst = ttm->tm_wday = ttm->tm_yday = -1;

    //Substrings:
    // Sample string:  `2  0  1  4  0  1  2  5  -  1  2  3  4   5  6'
    // Character num:   0  1  2  3  4  5  6  7  8  9 10  11 12  13 14
    //                  ----------  ----  ----     ----  -----  -----
    //                      Y        M     D        H     M     S

    shtl = in.substr(0,4);
    if(!Is_String_An_X<int>(shtl)) return false;
    ttm->tm_year = (stringtoX<int>(shtl) - 1900);
    shtl = Remove_Preceeding_Chars(in.substr(4,2),"0");
    if(!Is_String_An_X<int>(shtl)) return false;
    ttm->tm_mon = (stringtoX<int>(shtl) - 1);
    shtl = Remove_Preceeding_Chars(in.substr(6,2),"0");
    if(!Is_String_An_X<int>(shtl)) return false;
    ttm->tm_mday = stringtoX<int>(shtl);

    if(in.substr(8,1) != "-") return false;

    shtl = Remove_Preceeding_Chars(in.substr(9,2),"0");
    if(!Is_String_An_X<int>(shtl)) return false;
    ttm->tm_hour = stringtoX<int>(shtl);
    shtl = Remove_Preceeding_Chars(in.substr(11,2),"0");
    if(!Is_String_An_X<int>(shtl)) return false;
    ttm->tm_min = stringtoX<int>(shtl);
    shtl = Remove_Preceeding_Chars(in.substr(13,2),"0");
    if(!Is_String_An_X<int>(shtl)) return false;
    ttm->tm_sec = stringtoX<int>(shtl);

    return true;
}


bool time_mark::Read_from_string(const std::string &in, double *fractional_second){
    struct tm lt;
    lt.tm_year = lt.tm_mon = lt.tm_mday = lt.tm_hour = lt.tm_min = lt.tm_sec = -1;
    if(fractional_second != nullptr){
        *fractional_second = 0.0;
    }

    //We also make sure no other struct members remain untouched. This caused an annoying
    // bug which altered read times during the call to mktime(...).
    lt.tm_isdst = -1; //Indicates that info about daylight saving's time should be looked up during mktime(...) call.
    lt.tm_wday = lt.tm_yday = -1; //Stores the day of the week and day of the year.

    const auto extract_numbers = [](const std::string &src, const std::string &regex_str){
        std::vector<int64_t> numbers;
        try{
            auto tokens = GetAllRegex2(src, regex_str);
            for(auto &s : tokens){
                // Strip preceeding zero characters.
                while(1 < s.size()){ // leave a single 0, if input has multiple.
                    if(s[0] == '0'){
                        s = std::string( std::next(std::begin(s)), std::end(s) );
                    }else{
                        break;
                    }
                }
                // Attempt to convert to an integer.
                try{
                    numbers.emplace_back(std::stol(s));
                }catch(const std::exception &){}
            }
        }catch(const std::exception &){}
        return numbers;
    };
    const auto extract_year = [](int64_t y){
        if(isininc(70,y,99)){ // 2-digit shorthand for 1900's.
            // Do nothing; already the number of years since 1900.
        }else if(isininc(0,y,69)){ // 2-digit shorthand for 2000's.
            y = y + 100;
        }else if(1900 <= y){ // Assume fully specified, like '2021' or '1923'.
            y = y - 1900;
        }else{ // Otherwise, consider ambiguous.
            y = -1;
        }
        return y;
    };
         
    const std::string dig24 = "([[:digit:]]{2,4})";
    const std::string dig12 = "([[:digit:]]{1,2})";
    const std::string frac  = "[.]?([[:digit:]]*)";
    const std::string d_sep = R"***([-_/,.\ ]*)***";
    const std::string t_sep = R"***([-_/,.\: ]*)***";

    //The fixed-width format, like: '20140125-012345'. 
    // date '+%Y%m%_d-%_H%_M%_S'
    if(Glean_date_time_from_string(in,&lt)){

    // Regex-based parsing. Slow and awkward, but available cross-platform and support extraction of fractional seconds.
    // YY-MM-DD HH-MM-SS{.SSSSSS}
    }else if(auto n = extract_numbers(in, "^[[:space:]]*"_s + dig24 + d_sep + dig12 + d_sep + dig12 + d_sep + dig12 + t_sep + dig12 + t_sep + dig12 + frac + "[[:space:]]*$");
          (6 <= n.size()) && (0 < extract_year(n[0])) // year
                          && isininc(1,n[1],12)       // month
                          && isininc(1,n[2],31)       // day of month
                          && isininc(0,n[3],23)       // hour
                          && isininc(0,n[4],59)       // minute
                          && isininc(0,n[5],60) ){    // second
        lt.tm_year = extract_year(n[0]);
        lt.tm_mon  = n[1] - 1;
        lt.tm_mday = n[2];
        lt.tm_hour = n[3];
        lt.tm_min  = n[4];
        lt.tm_sec  = n[5];

        //Attempt to extract fractional seconds, if present.
        // Note: in the future, this can all get replaced by std::chrono (awaiting C++20).
        if( (fractional_second != nullptr)
        &&  (7 == n.size()) ){
            try{
                *fractional_second = std::stod( "0."_s + std::to_string(n[6]) );
            }catch(const std::exception &){}
        }

    // HH-MM-SS{.SSSSSS}
    }else if(auto n = extract_numbers(in, "^[[:space:]]*"_s + dig12 + t_sep + dig12 + t_sep + dig12 + frac + "[[:space:]]*$");
          (3 <= n.size()) && isininc(0,n[0],23)    // hour
                          && isininc(0,n[1],59)    // minute
                          && isininc(0,n[2],60) ){ // second
        lt.tm_hour = n[0];
        lt.tm_min  = n[1];
        lt.tm_sec  = n[2];
        lt.tm_year = lt.tm_mon = lt.tm_mday = 0;

        //Attempt to extract fractional seconds, if present.
        // Note: in the future, this can all get replaced by std::chrono (awaiting C++20).
        if( (fractional_second != nullptr)
        &&  (4 == n.size()) ){
            try{
                *fractional_second = std::stod( "0."_s + std::to_string(n[3]) );
            }catch(const std::exception &){}
        }

#if !defined(_WIN32) && !defined(_WIN64)
    //The preferred format: `date +%Y%m%d-%H%M%S` or "YearMonthDay-HourMinuteSecond". Example: "20131105-130535"
    // Note: that strptime has a lot of trouble with preceeding '0's! It will not properly handle fixed-width
    //       date and time!
    }else if(strptime(in.c_str(), "%Y%m%d-%H%M%S", &lt) != nullptr){
    }else if(strptime(in.c_str(), "%Y%Om%d-%H%M%S", &lt) != nullptr){

    //The previously preferred format: `date +%Y\/%m\/%d-%H:%M:%S`. Regex for matching it: 
    // ([0-9]{2,4})[/]([0-9]{1,2})[/]([0-9]{1,2})[-]([0-9]{1,2})[:]([0-9]{1,2})[:]([0-9]{1,2})
    // Example: '2013/07/16-21:19:13'
    }else if(strptime(in.c_str(), "%Y/%m/%d-%H:%M:%S", &lt) != nullptr){ //NOTE: Does not handle month or day = '0'...

    //Other misc. common formats.
    }else if(strptime(in.c_str(), "%Y-%m-%d", &lt) != nullptr){
        lt.tm_hour = lt.tm_min = lt.tm_sec = 0;
    }else if(strptime(in.c_str(), "%Y %m %d", &lt) != nullptr){
        lt.tm_hour = lt.tm_min = lt.tm_sec = 0;
    }else if(strptime(in.c_str(), "%H:%M:%S", &lt) != nullptr){ // No date info, only time.
        lt.tm_year = lt.tm_mon = lt.tm_mday = 0;

    //The style of Salivary Flow measurement data. Format: '17-Mar-2009' There is no time info here.
    }else if(strptime(in.c_str(), "%d-%b-%Y", &lt) != nullptr){
        lt.tm_hour = lt.tm_min = lt.tm_sec = 0;

    //The format used as an example in the strpcopy manual. Seems nice enough...
    // Example: '6 Dec 2001 12:33:45'
    }else if(strptime(in.c_str(), "%d %b %Y %H:%M:%S", &lt) != nullptr){

#endif // !defined(_WIN32) && !defined(_WIN64)

    // YY-MM-DD
    }else if(auto n = extract_numbers(in, "^[[:space:]]*"_s + dig24 + d_sep + dig12 + d_sep + dig12 + "[[:space:]]*$");
          (3 == n.size()) && (0 < extract_year(n[0])) // year
                          && isininc(1,n[1],12)       // month
                          && isininc(1,n[2],31) ){    // day of month
        lt.tm_year = extract_year(n[0]);
        lt.tm_mon  = n[1] - 1;
        lt.tm_mday = n[2];
        lt.tm_hour = lt.tm_min = lt.tm_sec = 0;

    //}else if( ....        <---add more here. Consider just piping to shell to use GNU date, which handles all sorts of neat formats automagically...
    }else{
        //We did not understand format. This is NOT an error, just a failure to parse. 
        // We pass back a false; whether the client considers this a failure or not is
        // up to them. 
        //
        //Do NOT issue a blanket warning because parsing something to see if it is a
        // parseable date is a valid strategy for determining if it is a date/time!
        //YLOGWARN("Could not make sense of the stringified date/time");
        return false;
    }

    //Verify no parameter has been left unset.   
    if((lt.tm_year == -1) || (lt.tm_mon == -1) || (lt.tm_mday == -1) || (lt.tm_hour == -1) || (lt.tm_min == -1) || (lt.tm_sec == -1)){
        //This legitimately warrants a warning. It may indicate a programming error or bug.
        YLOGWARN("Time string reading failed for some unknown reason. Perhaps the string parsed but wasn't a valid date?");
        return false;
    }
    this->When = mktime(&lt);
    if(this->When == -1) return false;
    return true;
}

time_mark & time_mark::operator=(const time_mark &rhs){
    if(this == &rhs) return *this;
    this->When = rhs.When;
    return *this;
}
bool time_mark::operator==(const time_mark &rhs) const {
    return (this->When == rhs.When);
}
bool time_mark::operator<(const time_mark &rhs) const {
    return (this->When < rhs.When);
}
bool time_mark::operator>(const time_mark &rhs) const {
    return (this->When > rhs.When);
}
bool time_mark::operator<=(const time_mark &rhs) const {
    return (this->When <= rhs.When);
}
bool time_mark::operator>=(const time_mark &rhs) const {
    return (this->When >= rhs.When);
}

time_mark time_mark::One_Day_Earlier_Than(const time_mark &T) const {
    time_mark out(T);
    out.When -= 24*60*60; 
    return out;
}
time_mark time_mark::One_Day_Later_Than(const time_mark &T) const { 
    time_mark out(T);
    out.When += 24*60*60;
    return out;
}

time_mark time_mark::Less_By_Seconds(int64_t dt) const {
    time_mark out(*this);
    out.When -= dt;
    return out;
}
time_mark time_mark::More_By_Seconds(int64_t dt) const {
    time_mark out(*this);
    out.When += dt;
    return out;
}

//Earliest time_mark occuring on the same day.
time_mark time_mark::Same_Day_Earliest(void) const {
    time_mark out(*this);
    struct tm A;
#if !defined(_WIN32) && !defined(_WIN64)
    if(localtime_r(&out.When, &A) == nullptr) YLOGERR("localtime_r produced an error - unable to continue")
#else
    if(localtime_s(&A, &out.When) != 0) YLOGERR("localtime_s produced an error - unable to continue")
#endif
    if((A.tm_year == -1) || (A.tm_mon == -1) || (A.tm_mday == -1)
          || (A.tm_hour == -1) || (A.tm_min == -1) || (A.tm_sec  == -1) ){
        YLOGERR("Cannot determine earliest same-day time from input");
    }

    //Set the hour and minutes to their minimum value.
    A.tm_hour = 0;
    A.tm_min  = 0;
    A.tm_sec  = 0;

    out.When = mktime(&A);
    if(out.When == -1){
        YLOGERR("Could not produce the earliest same-day time for unknown reasons");
    }
    return out;
}

time_mark time_mark::Same_Day_Latest(void) const {
    time_mark out(*this);
    out.Advance_One_Day();
    out = out.Same_Day_Earliest();
    out.Regress_By_Seconds(1);
    return out;
}


void time_mark::Regress_One_Day(void){ //aka "rewind"
    *this = this->One_Day_Earlier_Than(*this);
    return;
}
void time_mark::Advance_One_Day(void){
    *this = this->One_Day_Later_Than(*this);
    return;
}

void time_mark::Regress_By_Seconds(int64_t dt){
    this->When -= static_cast<time_t>(dt);
    return;
}
void time_mark::Advance_By_Seconds(int64_t dt){
    this->When += static_cast<time_t>(dt);
    return;
}


bool time_mark::Is_within_X_seconds(const time_mark &in, int64_t X) const {
    const auto A = static_cast<int64_t>(this->When);
    const auto B = static_cast<int64_t>(in.When);
    return (YGORABS(A - B) <= X);
}

time_t time_mark::Diff_in_Seconds(const time_mark &in) const {
    //Returns:  (in.sec - this->sec). If input is further in the future, result will be positive.
    //Note: This difference does not include fractional seconds. It has resolution of 1s.
    return in.When - this->When;
}
time_t time_mark::Diff_in_Days(const time_mark &in) const {
    //Returns:  (in.days - this->days). If input is further in the future, result will be positive.
    //Note: This difference does not include fractional days. It has resolution of 1day.
    const auto dsec = static_cast<double>(this->Diff_in_Seconds(in));
    const auto spd  = 60.0*60.0*24.0; //"Seconds per day"
    return static_cast<time_t>(dsec/spd);
}

bool time_mark::Have_same_day(const time_mark &in) const {  //Compares ONLY the day (the number) - not the year, month, etc..
    struct tm A,B;
#if !defined(_WIN32) && !defined(_WIN64)
    if(localtime_r(&this->When, &A) == nullptr) YLOGERR("localtime_r produced an error - unable to continue")
    if(localtime_r(&in.When, &B)    == nullptr) YLOGERR("localtime_r produced an error - unable to continue")
#else
    if(localtime_s(&A, &this->When) != 0) YLOGERR("localtime_s produced an error - unable to continue")
    if(localtime_s(&B, &in.When)    != 0) YLOGERR("localtime_s produced an error - unable to continue")
#endif
    if((A.tm_mday == -1) || (B.tm_mday == -1)) return false; //Error in the data.
    return A.tm_mday == B.tm_mday;
}

bool time_mark::Occur_on_same_day(const time_mark &in) const { 
    struct tm A,B;
#if !defined(_WIN32) && !defined(_WIN64)
    if(localtime_r(&this->When, &A) == nullptr) YLOGERR("localtime_r produced an error - unable to continue")
    if(localtime_r(&in.When, &B)    == nullptr) YLOGERR("localtime_r produced an error - unable to continue")
#else
    if(localtime_s(&A, &this->When) != 0) YLOGERR("localtime_s produced an error - unable to continue")
    if(localtime_s(&B, &in.When)    != 0) YLOGERR("localtime_s produced an error - unable to continue")
#endif

    if((A.tm_mday == -1) || (B.tm_mday == -1) 
       || (A.tm_mon == -1) || (B.tm_mon == -1)
       || (A.tm_year == -1) || (B.tm_year == -1) ) return false; //Error in the data.
    return (A.tm_mday == B.tm_mday) && (A.tm_mon == B.tm_mon) && (A.tm_year == B.tm_year);
}



//Serialize (deeply) to buffer starting at *offset. Buffer can be nullptr or passed in, but 
// other params must be appropriately set (if applicable) and always NOT-nullptr. On 
// failure, *OK set to false.
//
// Input:
//
//   - *OK       - Optional - Used to indicate success or failure. In absence, program is 
//                             terminated on failure.
//
//   - *in       - Optional - If provided, serialization is deposited into buffer (starting
//                             at *offset), *offset is advanced to the back of the 
//                             serialized data, and *buf_size is treated as the maximum 
//                             size of the buffer (and is not altered).
//                             If not provided, an appropriate amount of space will be 
//                             allocated using the default allocator prior to serialization.
//
//   - *offset   - Required - Used by the user to tell where to begin serialization (often
//                             at 0) and by this routine to tell the user where the next
//                             available byte is after serialization is complete.
//                             If "in" buffer is nullptr, *offset is set to 0 prior to
//                             serialization.
//
//   - *buf_size - Required - If "in" buffer is non-nullptr, this must hold the number of
//                             allocated bytes, and it will not be altered. 
//                             If "in" buffer is nullptr, this will afterward hold the 
//                             number of allocated bytes (which may be >= the amount of 
//                             data serialized).
//
//NOTE: The buffer may be larger than the serialized content, thus allowing many serialized 
// instances in one large buffer. The semantics of this are up to the user.
std::unique_ptr<uint8_t[]> 
time_mark::Serialize(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t *buf_size) const {
    if(OK != nullptr) *OK = false;
    SERIALIZE_WARNFAIL_OR_DIE((buf_size == nullptr), OK, "Passed an invalid *buf_size.",in);
    SERIALIZE_WARNFAIL_OR_DIE((offset == nullptr), OK, "Passed an invalid *offset.",in);

    //If the buffer is not yet allocated, allocate an appropriately-sized buffer.
    if(in == nullptr){
        *buf_size = this->Theo_Max_Serialization_Size();
        in = std::make_unique<uint8_t[]>( *buf_size );
        *offset = 0;
    }
    
    bool l_OK;

    /*
    //------ <Version 1 @ 20130905> -------
    //Verify that the double representation is standard.
    l_OK = (std::numeric_limits<double>::is_iec559 &&
           (std::numeric_limits<double>::max_exponent == 1024) &&
           (std::numeric_limits<double>::digits == 53) &&
           (std::numeric_limits<double>::radix == 2));
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Non-standard platform double representation.",std::move(in));

    //Version number.
    const uint64_t version = 1;
    in = SERIALIZE::Put_Size(&l_OK,std::move(in),offset,*buf_size, version);
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Put version.",std::move(in));

    //When.
    in = SERIALIZE::Put(&l_OK,std::move(in),offset,*buf_size, this->When);
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Put this->When.",std::move(in));
    //------ </Version 1 @ 20130905> -------
    */

    //------ <Version 2 @ 20131017> -------
    //Verify that the double representation is standard.
    l_OK = (std::numeric_limits<double>::is_iec559 &&
           (std::numeric_limits<double>::max_exponent == 1024) &&
           (std::numeric_limits<double>::digits == 53) &&
           (std::numeric_limits<double>::radix == 2));
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Non-standard platform double representation.",in);

    //Version number.
    const uint64_t version = 2;
    in = SERIALIZE::Put_Size(&l_OK,std::move(in),offset,*buf_size, version);
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Put version.",in);

    //When (in std::string format).
    const auto stringified = this->Dump_as_string();
    in = SERIALIZE::Put(&l_OK,std::move(in),offset,*buf_size, stringified);
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Put stringified this->When.",in);
    //------ </Version 2 @ 20131017> -------

    if(OK != nullptr) *OK = true;
    return in;
}

//Deserialize (deeply) from buffer starting at *offset. Contrary to the counterpart
// Serialize function, this function requires a valid buffer to deserialize from.
// Thus, all parameters (except *OK) are required.
//
//This routine will overwrite *this.
//
//NOTE: The buffer may be larger than the serialized content, thus allowing many 
// serialized instances in one large buffer. The semantics of this are up to the user.
// This routine will only deserialize the data beginning at *offset.
std::unique_ptr<uint8_t[]> 
time_mark::Deserialize(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t buf_size){
    if(OK != nullptr) *OK = false;
    SERIALIZE_WARNFAIL_OR_DIE((offset == nullptr), OK, "Passed an invalid *offset.",in);

    bool l_OK;
    uint64_t version;
    in = SERIALIZE::Get_Size(&l_OK,std::move(in),offset,buf_size, &(version));
    SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Get version.",in);

    if(version == 1){
        /*
        //------ <Version 1 @ 20130905> -------
        //Verify that the double representation is standard.
        l_OK = (std::numeric_limits<double>::is_iec559 &&
               (std::numeric_limits<double>::max_exponent == 1024) &&
               (std::numeric_limits<double>::digits == 53) &&
               (std::numeric_limits<double>::radix == 2));
        SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Non-standard platform double representation.",std::move(in));
        //When. This version uses raw std::time_t. Bad: different on 32 and 64 bit test machines.
        in = SERIALIZE::Get(&l_OK,std::move(in),offset,buf_size, &(this->When));
        SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Get this->When.",std::move(in));
        //------ </Version 1 @ 20130905> -------
        */
        l_OK = false;
        SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Can't handle v1 serialization. Functionality was removed.",in);

    }else if(version == 2){
        //------ <Version 2 @ 20131017> -------
        //Verify that the double representation is standard.
        l_OK = (std::numeric_limits<double>::is_iec559 &&
               (std::numeric_limits<double>::max_exponent == 1024) &&
               (std::numeric_limits<double>::digits == 53) &&
               (std::numeric_limits<double>::radix == 2));
        SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Non-standard platform double representation.",in);

        //When (in std::string format).
        std::string stringified;
        in = SERIALIZE::Get(&l_OK,std::move(in),offset,buf_size, &stringified);
        SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to Get stringified this->When.",in);

        l_OK = this->Read_from_string(stringified);
        SERIALIZE_WARNFAIL_OR_DIE(!l_OK,OK,"Unable to interpret stringified this->When as valid date.",in);
        //------ </Version 2 @ 20131017> -------

    }else{
        SERIALIZE_WARNFAIL_OR_DIE(false,OK,"Version not recognized.",in);
    }

    if(OK != nullptr) *OK = true;
    return in;
}

//Returns the (maximum) number of bytes required to serialize *this (at this moment).
// Alteration of *any* of the members will probably void the estimate. Re-compute if 
// uncertain.
//
//This procedure depends strongly on the serialization procedure. Remember to keep it 
// up to date and versioned!
uint64_t time_mark::Theo_Max_Serialization_Size(void) const {
    uint64_t max_tot_size = 0;
    /*
    //------ <Version 1.0 @ 20130905> -------
    max_tot_size += SERIALIZE::max_vw_size; //Version stamp. 
    max_tot_size += sizeof(std::time_t); //Raw this->When value. 
    //------ </Version 1.0 @ 20130905> -------
    */
    //------ <Version 2 @ 20131017> -------
    max_tot_size += SERIALIZE::max_vw_size; //Version stamp. 
    max_tot_size += 15; //Size of stringified this->When. 
    //------ </Version 2 @ 20131017> -------

    return max_tot_size;
}

