//YgorSerialize.h - Helper routines for serializing data types to and from 
//  string or uint8_t memory blocks.
//
//NOTE: These routines MAY or MAY NOT insert extra data for containers and
// classes. Do NOT mix these routines with other techniques (eg. memcpy or 
// other direct read methods) unless you are certain that no hints are 
// being used.
//
//NOTE: These routines do not serialize enough information to *fully*
// construct most objects. The metadata about whether data is a list or a
// vector or an array are up to the user. These routines are essentially
// to be used as *tools* for implementing your own Serialize()/Deserialize()
// member functions!
//

#include <string>
#include <memory>

#include "YgorMisc.h"

namespace SERIALIZE {

    //Basic, safe serialization routines. These store the intrinsic types, like ints, chars, and doubles.
    template <class T> std::unique_ptr<uint8_t[]> 
    Put_Intrinsic(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T val);

    template <class T> std::unique_ptr<uint8_t[]> 
    Get_Intrinsic(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T *val);

    //Used for storing metadata about containers and arrays of data. These will probably just be used internally, 
    // but are available for the user if needed.
    std::unique_ptr<uint8_t[]> 
    Put_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t val);

    std::unique_ptr<uint8_t[]> 
    Get_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t *val);


    //For all types of strings, including std::string (==std::basic_string<char>)
    template <class T> std::unique_ptr<uint8_t[]> 
    Get_String(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::basic_string<T> *val);

    template <class T> std::unique_ptr<uint8_t[]> 
    Put_String(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::basic_string<T> val);


    //For storing std::list, std::vector, std::array, std::forward_list, std::deque, etc.. of intrinsic types.
    // Anything with size(), begin(), and end() members, and with T::value_type being intrinsic is OK.
    template <class T> std::unique_ptr<uint8_t[]> 
    Get_Intrinsic_Sequence_Container(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T *val);

    template <class T> std::unique_ptr<uint8_t[]> 
    Put_Intrinsic_Sequence_Container(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const T &val);






} //namespace SERIALIZE
