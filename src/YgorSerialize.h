//YgorSerialize.h - Helper routines for serializing data types to and from 
//  string or uint8_t memory blocks.
//
//NOTE: These routines MAY or MAY NOT insert extra data for containers and
// classes. Do NOT mix these routines with other techniques (eg. memcpy or 
// other direct read methods) unless you are certain that no hints are 
// being used.
//
//NOTE: If data was encoded using a 'PUT(..)', ensure it is decoded using
// the accompanying 'GET(...)'. Extra information is placed between objects
// which make simple memcpy() unable to safely reconstruct the data!
//
//NOTE: These routines do not serialize enough information to *fully*
// construct most objects. The metadata about whether data is a list or a
// vector or an array are up to the user. These routines are essentially
// to be used as *tools* for implementing your own Serialize()/Deserialize()
// member functions!
//

#ifndef YGOR_SERIALIZE_HDR_GRD_H
#define YGOR_SERIALIZE_HDR_GRD_H

#include <string>
#include <memory>
#include <cstdint>   //For uint8_t, uint32_t, etc..

#include "YgorMisc.h"


//This macro function simplifies error reporting in many use-cases of the
// particular style of serialization here.
//
// What it does: An indicator bool (l_OK) is checked. If TRUE, then a bool 
// pointer for propagating errors is checked for validity. If not valid,
// die. If valid, emit a warning and propagate the failure to the caller.
//   
//   if(l_OK){
//        if(OK == nullptr){
//            FUNCERR(failmsg << " Cannot continue");
//        }else{  
//            FUNCWARN(failmsg << " Bailing"); 
//            return std::move(in);  
//        }
//    }
//
// To use:
//   buf = SERIALIZE::Put(&l_OK, std::move(buf),....);
//   SERIALIZE_FUNCWARN_OR_DIE(!l_OK,OK,"Could not Put ...", std::move(buf));
//
// or:
//  ..... Some_Function(bool *OK, std::unique_ptr in, uint64_t *avar, ...){
//      SERIALIZE_FUNCWARN_OR_DIE((avar == nullptr),OK,"Encountered a nullptr at ...",std::move(in));
//
#ifndef SERIALIZE_WARNFAIL_OR_DIE
    #define SERIALIZE_WARNFAIL_OR_DIE(l_OK,OK,failmsg,pntr)  if((l_OK)){ \
        if((OK) == nullptr){  FUNCERR(failmsg << " Cannot continue"); \
        }else{  FUNCWARN(failmsg << " Bailing"); return (pntr);  } \
    }
#endif


namespace SERIALIZE {

    //These routines are the *core* routines which the user should use. They 
    // will seamlessly handle intrinsic types (integer, char, floating point) 
    // and more complex types which require writing extra metainfo. Through
    // template specialization, this all happens behind the scenes.
    template <class T> std::unique_ptr<uint8_t[]> 
    Put(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T val);

    template <class T> std::unique_ptr<uint8_t[]> 
    Get(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T *val);

    //Used for safely storing the sizes of things, up to a maximum of the
    // max uint64_t - 1. To save space, the amount of bytes used is variable.

    extern const uint64_t max_vw_size; //Theoretical maximum variable-width size.
    extern const uint64_t max_fw_size; //Theoretical maximum fixed-width size.

    extern const uint64_t max_string_head_size; //Theoretical maximum.

    std::unique_ptr<uint8_t[]> 
    Put_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t val);

    std::unique_ptr<uint8_t[]>
    Put_Fixed_Width_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t val);

    std::unique_ptr<uint8_t[]> 
    Get_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t *val);

    //Used for verifying parts of data with a plain string.
    std::unique_ptr<uint8_t[]>
    Matches_At(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const std::string &seq);


    //For writing buffers ala memcpy. This is a thin wrapper around memcpy
    // that acts more like the other functions.
    std::unique_ptr<uint8_t[]>
    Put_Raw(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, void *raw, uint64_t num_bytes);

    std::unique_ptr<uint8_t[]>
    Get_Raw(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, void *raw, uint64_t num_bytes);


    //-----------------------------------------------------------------
    //Below routines may disappear. If they are needed, try adding the 
    // required functionality to the 'PUT(...)' and 'GET(...)' above.
    //-----------------------------------------------------------------


    //For all types of strings, including std::string (==std::basic_string<char>)
    template <class T> std::unique_ptr<uint8_t[]> 
    Get_String(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::basic_string<T> *val);

    template <class T> std::unique_ptr<uint8_t[]> 
    Put_String(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::basic_string<T> val);


    //For storing std::list, std::vector, std::array, std::forward_list, std::deque, etc.. of intrinsic types.
    // Anything with size(), begin(), and end() members, and with T::value_type being intrinsic is OK.
    template <class T> std::unique_ptr<uint8_t[]> 
    Get_Sequence_Container(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T *val);

    template <class T> std::unique_ptr<uint8_t[]> 
    Put_Sequence_Container(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const T &val);


} //namespace SERIALIZE

#endif //YGOR_SERIALIZE_HDR_GRD_H
