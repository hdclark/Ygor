//YgorSerialize.cc - Helper routines for serializing data types to and from 
//  string or uint8_t memory blocks.
//
//NOTE: These routines are ignorant of machine-specific encodings, including the
// floating-point layout, endianness, and any differences in basic types between
// platforms. THEREFORE, the user of these routines must be aware of these issues.
//
// See existing software for some ideas to work around this. Here are some briefs:
// ---------------------------
//  - version consistency - Encode a version number in the serialized data. Use it
//                          to properly handle updated/outdated serialization 
//                          schemes.
//
//  - floating-point layout - Choose a specific double/float encoding scheme. 
//                            Using <limits> header, check that the platform uses
//                            the scheme whenever serialization/deserialization
//                            occurs. If needed, provide a translation routine to
//                            and from the chosen scheme.
//
//  - endianness - Choose a 'network endianness' scheme. Convert all necessary data 
//                 to and from the scheme. BSD socket implementations can help here.
// ---------------------------

#include <algorithm>
#include <cstdint>   //For uint8_t, uint32_t, etc..
#include <cstring>  //For memcpy().  
#include <limits>
#include <list>
#include <vector>
#include <memory>
#include <string>

#include "YgorMisc.h"
#include "YgorSerialize.h"

//#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
//#endif


//Error reporting function used internally.
static std::unique_ptr<uint8_t[]> 
FailWarn_or_Die(bool *OK, std::unique_ptr<uint8_t[]> in, const std::string &fail, const std::string &die){
    //If the user has not provided any extra channel or means of indicating an error, kill everything.
    if(OK == nullptr) FUNCERR(die);

    //Otherwise, issue a warning message, ensure the fail bit is set, and return.
    *OK = false;
    FUNCWARN(fail);
    return std::move(in);
}


template <class T> std::unique_ptr<uint8_t[]> 
SERIALIZE::Put(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T val){
    //Parameters:
    //  - OK     - Indicates success or failure. Do not use output if false.
    //  - in     - The block of memory holding the serialized data.
    //  - offset - The offset from in[0] in sizeof(uint8_t) = 1 = BYTES indicating the data. Will be incremented after read.
    //  - max    - The size of the memory block "in". Offset cannot legally exceed this number, or be less than 0.
    //  - val    - The value to place into memory.
    //
    //TODO: 
    //  - Pick an endianness. Enforce it during serialization/deserialization. (Use 'network endianness' ?)
    //  - 
    
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing", 
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if((*offset + sizeof(T)) > max){
        return FailWarn_or_Die(OK, std::move(in), "Desired value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                  "Desired value is beyond the bounds of the memory passed in. Unable to continue");
    }

    const auto ptrtomem = reinterpret_cast<T *>(in.get() + *offset);
    if(!ptrtomem){
        return FailWarn_or_Die(OK, std::move(in), "Region of memory indicated failed a reinterpret_cast. Indicating failure and continuing",
                                                  "Region of memory indicated failed a reinterpret_cast. Unable to continue");
    }

    //Copy the value to the specified location.
    *offset += sizeof(T);
    *ptrtomem = val;
    if(OK != nullptr) *OK = true;
    return std::move(in);
}

#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint8_t);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint16_t);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint32_t);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint64_t);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int8_t);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int16_t);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int32_t);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int64_t);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, char);

//On my 32bit machine, this is unique from the the others.  Need to figure out how to conditionally compile this...
// Alternatively - do not bother. Serializing std::time_t is going to lead to *many* more issues....
//    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::time_t);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, float);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, double);
#endif
namespace SERIALIZE {
    template <> std::unique_ptr<uint8_t[]>
    Put(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::string val){
        return SERIALIZE::Put_String(OK,std::move(in),offset,max,val);
    }
}

namespace SERIALIZE {
    template <> std::unique_ptr<uint8_t[]>
    Put(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const std::string &val){
        return SERIALIZE::Put_String(OK,std::move(in),offset,max,val);
    }
}

namespace SERIALIZE {
    template <> std::unique_ptr<uint8_t[]>
    Put(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const std::list<std::string> &val){
        return SERIALIZE::Put_Sequence_Container(OK,std::move(in),offset,max,val);
    }
}

namespace SERIALIZE {
    template <> std::unique_ptr<uint8_t[]>
    Put(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::list<std::string> val){
        return SERIALIZE::Put_Sequence_Container(OK,std::move(in),offset,max,val);
    }
}




template <class T> std::unique_ptr<uint8_t[]> 
SERIALIZE::Get(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T *val){
    //Parameters:
    //  - OK     - Indicates success or failure. Do not use output if false.
    //  - in     - The block of memory holding the serialized data.
    //  - offset - The offset from in[0] in sizeof(uint8_t) = 1 = BYTES indicating the data. Will be incremented after read.
    //  - max    - The size of the memory block "in". Offset cannot legally exceed this number, or be less than 0.
    //  - val    - The location to store the variable.
    //
    //TODO: 
    //  - Pick an endianness. Enforce it during serialization/deserialization. (Use 'network endianness' ?)
    //  - 

    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if((*offset + sizeof(T)) > max){
        return FailWarn_or_Die(OK, std::move(in), "Desired value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                  "Desired value is beyond the bounds of the memory passed in. Unable to continue");
    }else if(val == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed a nullptr instead of a location for storing the variable. Indicating failure and continuing",
                                                  "Passed a nullptr instead of a location for storing the variable. Unable to continue");
    }

    const auto ptrtovar = reinterpret_cast<T *>(in.get() + *offset);
    if(!ptrtovar){
        return FailWarn_or_Die(OK, std::move(in), "Region of memory indicated failed a reinterpret_cast. Indicating failure and continuing",
                                                  "Region of memory indicated failed a reinterpret_cast. Unable to continue");
    }

    //Copy the value to the specified location.
    *offset += sizeof(T);
    *val = *ptrtovar;
    if(OK != nullptr) *OK = true;
    return std::move(in);
}

#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint8_t *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint16_t *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint32_t *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, uint64_t *);
    
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int8_t *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int16_t *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int32_t *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, int64_t *);
   
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, char *);

//On my 32bit machine, this is unique from the the others.  Need to figure out how to conditionally compile this...
// Alternatively - do not bother. Serializing std::time_t is going to lead to *many* more issues....
//    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::time_t *);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, float *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, double *);
#endif
namespace SERIALIZE {
    template <> std::unique_ptr<uint8_t[]>
    Get(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::string *val){
        return SERIALIZE::Get_String(OK,std::move(in),offset,max,val);
    }
}

namespace SERIALIZE {
    template <> std::unique_ptr<uint8_t[]>
    Get(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::list<std::string> *val){
        return SERIALIZE::Get_Sequence_Container(OK,std::move(in),offset,max,val);
    }
}


const uint64_t SERIALIZE::max_vw_size = (1+2+8); //Theoretical maximum variable-width size.
const uint64_t SERIALIZE::max_fw_size = (1+2+8); //Theoretical maximum fixed-width size.
const uint64_t SERIALIZE::max_string_head_size = max_vw_size; //Theoretical maximum.


std::unique_ptr<uint8_t[]> 
SERIALIZE::Put_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t val){
    //This routine is used by the container serialization routines to write a number denoting
    // number of following bytes (ie. ~the number of things in the container).
    //
    //To keep memory footprint down, we use a type-escalation procedure. If `val' exceeds 
    // the maximum size of the smallest unsigned, fixed-width type, write the maximum size
    // and then iterate for the next type. If the number of items written exceeds 
    // std::numeric_limits<uint64_t>::max(), then we refuse to write anything (failure).
    //
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }

    bool l_OK;
    
    //Try writing the size into a single byte.
    {
      uint8_t shtl;
      if((*offset + sizeof(uint8_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }else if(val >= static_cast<uint64_t>(std::numeric_limits<uint8_t>::max())){
          shtl = std::numeric_limits<uint8_t>::max();
      }else{
          shtl = static_cast<uint8_t>(val);
      }
      in = SERIALIZE::Put<uint8_t>(&l_OK,std::move(in),offset,max,shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not write size value assuming uint8_t. Indicating failure and continuing",
                                                    "Could not write size value assuming uint8_t. Unable to continue");
      }else if(shtl != std::numeric_limits<uint8_t>::max()){ //Success.
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }

    //And then with two bytes.
    {
      uint16_t shtl;
      if((*offset + sizeof(uint16_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }else if(val >= static_cast<uint64_t>(std::numeric_limits<uint16_t>::max())){
          shtl = std::numeric_limits<uint16_t>::max();
      }else{
          shtl = static_cast<uint16_t>(val);
      }
      in = SERIALIZE::Put<uint16_t>(&l_OK,std::move(in),offset,max,shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not write size value assuming uint16_t. Indicating failure and continuing",
                                                    "Could not write size value assuming uint16_t. Unable to continue");
      }else if(shtl != std::numeric_limits<uint16_t>::max()){ //Success.
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }

    //And then with 8 bytes, because size is probably not an issue.
    {
      uint64_t shtl;
      if((*offset + sizeof(uint64_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }else if(val >= static_cast<uint64_t>(std::numeric_limits<uint64_t>::max())){
          shtl = std::numeric_limits<uint64_t>::max();
      }else{
          shtl = static_cast<uint64_t>(val);
      }
      in = SERIALIZE::Put<uint64_t>(&l_OK,std::move(in),offset,max,shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not write size value assuming uint64_t. Indicating failure and continuing",
                                                    "Could not write size value assuming uint64_t. Unable to continue");
      }else if(shtl != std::numeric_limits<uint64_t>::max()){ //Success.
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }


    //If we get here, there are more than std::numeric_limits<uint64_t>::max() items...
    //
    //Perhaps we could implement a uint128_t, but this is most likely overkill for many reasons.
    return FailWarn_or_Die(OK, std::move(in), "Unable to determine size. Indicating failure and continuing",
                                              "Unable to determine size. Unable to continue");
}

std::unique_ptr<uint8_t[]>
SERIALIZE::Put_Fixed_Width_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t val){
    //This routine is used to lay down a fixed-width size, often because it is being used
    // in several passes, perhaps when the ultimate width is not yet known but will be 
    // adjusted afterward.
    //
    //This routine is compatible with the variable-width routine. Get_Size(), can be 
    // used to get sizes produced by both Put_Size() and Put_Fixed_Width_Size().
    //
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }

    bool l_OK;

    //To ensure fixed width, and that we will be able to accomodate all sizes of the
    // variable-width routine, we will 'fake' that the size is large.
    uint8_t first = std::numeric_limits<uint8_t>::max();
    in = SERIALIZE::Put<uint8_t>(&l_OK,std::move(in),offset,max,first);

    uint16_t secnd = std::numeric_limits<uint16_t>::max();
    in = SERIALIZE::Put<uint16_t>(&l_OK,std::move(in),offset,max,secnd);


    //And then with 8 bytes we write the total size.
    {
      uint64_t shtl;
      if((*offset + sizeof(uint64_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }else if(val >= static_cast<uint64_t>(std::numeric_limits<uint64_t>::max())){
          shtl = std::numeric_limits<uint64_t>::max();
      }else{
          shtl = static_cast<uint64_t>(val);
      }
      in = SERIALIZE::Put<uint64_t>(&l_OK,std::move(in),offset,max,shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not write size value assuming uint64_t. Indicating failure and continuing",
                                                    "Could not write size value assuming uint64_t. Unable to continue");
      }else if(shtl != std::numeric_limits<uint64_t>::max()){ //Success.
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }

    //If we get here, there are more than std::numeric_limits<uint64_t>::max() items...
    //
    //Perhaps we could implement a uint128_t, but this is most likely overkill for many reasons.
    return FailWarn_or_Die(OK, std::move(in), "Unable to determine size. Indicating failure and continuing",
                                              "Unable to determine size. Unable to continue");
}

std::unique_ptr<uint8_t[]> 
SERIALIZE::Get_Size(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, uint64_t *val){
    //This routine is used by the container serialization routines to retrieve a number denoting
    // number of following bytes (ie. ~the number of things in the container).
    //
    //It is necessary to allow the possibility of holding many items - perhaps more than the type
    // can handle. (We will refuse to Put in such case, but we cannot refuse to Get - we can only 
    // safely fail!)
    //
    //We will generally refuse to Put when there are too many items, but often cannot guarantee 
    // the input on a Get. These functions implement a size escalation procedure to bloat the
    // size storage type in such a case. This helps reduce the storage size required when many
    // items are stored, but keeps costs down when few are stored.
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if(val == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed a nullptr instead of a location for storing the variable. Indicating failure and continuing",
                                                  "Passed a nullptr instead of a location for storing the variable. Unable to continue");
    }

    bool l_OK;

    //Try reading the size into a single byte.
    {
      uint8_t shtl;
      if((*offset + sizeof(uint8_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }
      in = SERIALIZE::Get<uint8_t>(&l_OK,std::move(in),offset,max,&shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not retrieve size value assuming uint8_t. Indicating failure and continuing",
                                                    "Could not retrieve size value assuming uint8_t. Unable to continue");
      }
      if(shtl != std::numeric_limits<uint8_t>::max()){ //Success.
          // *offset += sizeof(T); //Get() has already advanced the offset.
          *val = static_cast<uint64_t>(shtl);
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }

    //Try again with two bytes.
    {
      uint16_t shtl;
      if((*offset + sizeof(uint16_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }
      in = SERIALIZE::Get<uint16_t>(&l_OK,std::move(in),offset,max,&shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not retrieve size value assuming uint16_t. Indicating failure and continuing",
                                                    "Could not retrieve size value assuming uint16_t. Unable to continue");
      }
      if(shtl != std::numeric_limits<uint16_t>::max()){ //Success.
          // *offset += sizeof(T); //Get() has already advanced the offset.
          *val = static_cast<uint64_t>(shtl);
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }

    //Try again with eight bytes. Storage cost clearly doesn't matter anymore...
    {
      uint64_t shtl;
      if((*offset + sizeof(uint64_t)) > max){
          return FailWarn_or_Die(OK, std::move(in), "Size value is beyond the bounds of the memory passed in. Indicating failure and continuing",
                                                    "Size value is beyond the bounds of the memory passed in. Unable to continue");
      }
      in = SERIALIZE::Get<uint64_t>(&l_OK,std::move(in),offset,max,&shtl);
      if(l_OK == false){
          return FailWarn_or_Die(OK, std::move(in), "Could not retrieve size value assuming uint64_t. Indicating failure and continuing",
                                                    "Could not retrieve size value assuming uint64_t. Unable to continue");
      }
      if(shtl != std::numeric_limits<uint64_t>::max()){ //Success.
          // *offset += sizeof(T); //Get() has already advanced the offset.
          *val = static_cast<uint64_t>(shtl);
          if(OK != nullptr) *OK = true;
          return std::move(in);
      }
    }

    //If we get here, we are being signalled that an error has somehow occurred (or there are
    // indeed more than std::numeric_limits<uint64_t>::max() items...
    //
    //Perhaps we could implement a uint128_t, but this is most likely overkill for many reasons.
    return FailWarn_or_Die(OK, std::move(in), "Unable to determine size. Indicating failure and continuing",
                                              "Unable to determine size. Unable to continue");
}

//For writing buffers ala memcpy. This is a thin wrapper around memcpy
// that acts more like the other functions.
std::unique_ptr<uint8_t[]>
SERIALIZE::Put_Raw(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, void *raw, uint64_t num_bytes){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when buffer was expected. Indicating failure and continuing",
                                                  "Passed nullptr when buffer was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if(raw == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr raw. No data to copy. Indicating failure and continuing",
                                                  "Passed nullptr raw. No data to copy. Unable to continue");
    }else if((*offset + num_bytes) > max){
        return FailWarn_or_Die(OK, std::move(in), "Not enough memory available for copy. Indicating failure and continuing",
                                                  "Not enough memory available for copy. Unable to continue");
    }

    memcpy((void *)(in.get() + *offset), raw, num_bytes);
    *offset += num_bytes;

    if(OK != nullptr) *OK = true;
    return std::move(in);
}

std::unique_ptr<uint8_t[]>
SERIALIZE::Get_Raw(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, void *raw, uint64_t num_bytes){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if(raw == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr raw. No data to copy. Indicating failure and continuing",
                                                  "Passed nullptr raw. No data to copy. Unable to continue");
    }else if((*offset + num_bytes) > max){
        return FailWarn_or_Die(OK, std::move(in), "Not enough memory available for copy. Indicating failure and continuing",
                                                  "Not enough memory available for copy. Unable to continue");
    }

    memcpy(raw, (void *)(in.get() + *offset), num_bytes);
    *offset += num_bytes;

    if(OK != nullptr) *OK = true;
    return std::move(in);
}



//Used for verifying segments of (raw, buffer) data with a plain string.
std::unique_ptr<uint8_t[]>
SERIALIZE::Matches_At(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const std::string &seq){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if(seq.empty()){
        return FailWarn_or_Die(OK, std::move(in), "Passed an empty plain string. 'Matching' undefined. Indicating failure and continuing",
                                                  "Passed an empty plain string. 'Matching' undefined. Unable to continue");
    }else if((*offset + seq.size()) > max){
        //This may not be an error, but we cannot properly advance offset beyond max, so we are forced to fail.
        return FailWarn_or_Die(OK, std::move(in), "Not enough data for meaningful comparison. Indicating failure and continuing",
                                                  "Not enough data for meaningful comparison. Unable to continue");
    }

FUNCWARN("*********** This function has not been tested yet. Please verify it ********************");

    const std::string test(reinterpret_cast<char *>(in.get() + *offset), seq.size());

    //If they do not match, then *OK stays false.
    if(test != seq) return std::move(in);

    //Otherwise, it is a match. Advance offset and return.
    *offset += seq.size();
    if(OK != nullptr) *OK = true;
    return std::move(in);
}



template <class T> std::unique_ptr<uint8_t[]> 
SERIALIZE::Get_String(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::basic_string<T> *val){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if(val == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed a nullptr instead of a location for storing the variable. Indicating failure and continuing",
                                                  "Passed a nullptr instead of a location for storing the variable. Unable to continue");
    }

    bool l_OK;

    uint64_t str_size; //In bytes!
    in = SERIALIZE::Get_Size(&l_OK, std::move(in), offset, max, &str_size);
    if(l_OK == false){
        return FailWarn_or_Die(OK, std::move(in), "Could not get string size. Indicating failure and continuing",
                                                  "Could not get string size. Unable to continue");
    }else if((*offset + str_size) > max){
        return FailWarn_or_Die(OK, std::move(in), "String size indicates more elements than we have available. Indicating failure and continuing",
                                                  "String size indicates more elements than we have available. Unable to continue");
    }

    //Copy the data using the string assign() member. (Note: It takes 
    // the COUNT of T, not the size in bytes of T.)
    val->assign(reinterpret_cast<T *>(in.get() + *offset), str_size/sizeof(T));
    *offset += str_size;
    if(OK != nullptr) *OK = true;
    return std::move(in);
}
#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint8_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint16_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint32_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint64_t> *);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int8_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int16_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int32_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int64_t> *);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<char> *);
//    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<unsigned char> *);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<float> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<double> *);
#endif


template <class T> std::unique_ptr<uint8_t[]> 
SERIALIZE::Put_String(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, std::basic_string<T> val){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }

    bool l_OK;
    uint64_t str_size = val.size()*sizeof(T); //In bytes!
    in = SERIALIZE::Put_Size(&l_OK, std::move(in), offset, max, str_size);
    if(l_OK == false){
        return FailWarn_or_Die(OK, std::move(in), "Could not put string size. Indicating failure and continuing",
                                                  "Could not put string size. Unable to continue");
    }else if((*offset + str_size) > max){
        return FailWarn_or_Die(OK, std::move(in), "String size indicates more elements than we have space for. Indicating failure and continuing",
                                                  "String size indicates more elements than we have space for. Unable to continue");
    }

    //Copy the data. (Ditch the '\0' because we carry around a proper size.)
    in = SERIALIZE::Put_Raw(&l_OK, std::move(in), offset, max, (void *)(val.data()), str_size);
//    memcpy((void *)(in.get() + *offset), (void *)(val.data()), str_size);
//    *offset += str_size;
    if(OK != nullptr) *OK = true;
    return std::move(in);
}
#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint8_t>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint16_t>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint32_t>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<uint64_t>);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int8_t>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int16_t>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int32_t>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<int64_t>);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<char>);
//    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<unsigned char>);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<float>);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_String(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::basic_string<double>);
#endif


template <class T> std::unique_ptr<uint8_t[]> 
SERIALIZE::Get_Sequence_Container(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, T *val){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }else if(val == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed a nullptr instead of a location for storing the variable. Indicating failure and continuing",
                                                  "Passed a nullptr instead of a location for storing the variable. Unable to continue");
    }else if(!val->empty()){
        FUNCWARN("Passed a non-empty container. Assuming this is intentional. Appending data");
    }

    bool l_OK;
    uint64_t str_size; //In bytes!
    in = SERIALIZE::Get_Size(&l_OK, std::move(in), offset, max, &str_size);
    if(l_OK == false){
        return FailWarn_or_Die(OK, std::move(in), "Could not get string size. Indicating failure and continuing",
                                                  "Could not get string size. Unable to continue");
    }else if((*offset + str_size) > max){
        return FailWarn_or_Die(OK, std::move(in), "String size indicates more elements than we have available. Indicating failure and continuing",
                                                  "String size indicates more elements than we have available. Unable to continue");
    }

    typename T::value_type shtl;
    for(uint64_t i = 0; i < str_size; i+=sizeof(shtl)){
        in = SERIALIZE::Get<typename T::value_type>(&l_OK, std::move(in), offset, max, &shtl);
        if(l_OK == false){
            return FailWarn_or_Die(OK, std::move(in),"Could not get intrinsic for container. Indicating failure and continuing",
                                                     "Could not get intrinsic for container. Unable to continue");
        }
        val->push_back(shtl);
    }

    if(OK != nullptr) *OK = true;
    return std::move(in);
}
#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
    //Fill this in with anything which is needed.
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::list<uint8_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::list<double> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::list<std::string> *);

    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::vector<uint8_t> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::vector<double> *);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Get_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, std::vector<std::string> *);
#endif


template <class T> std::unique_ptr<uint8_t[]> 
SERIALIZE::Put_Sequence_Container(bool *OK, std::unique_ptr<uint8_t[]> in, uint64_t *offset, uint64_t max, const T &val){
    if(OK != nullptr) *OK = false;
    if(in == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr when data was expected. Indicating failure and continuing",
                                                  "Passed nullptr when data was expected. Unable to continue");
    }else if(offset == nullptr){
        return FailWarn_or_Die(OK, std::move(in), "Passed nullptr offset. No way to advance offset. Indicating failure and continuing",
                                                  "Passed nullptr offset. No way to advance offset. Unable to continue");
    }

    bool l_OK;
    uint64_t str_size = val.size()*sizeof(typename T::value_type); //In bytes!
    in = SERIALIZE::Put_Size(&l_OK, std::move(in), offset, max, str_size);
    if(l_OK == false){
        return FailWarn_or_Die(OK, std::move(in), "Could not put container size. Indicating failure and continuing",
                                                  "Could not put container size. Unable to continue");
    }else if((*offset + str_size) > max){
        return FailWarn_or_Die(OK, std::move(in), "Container size indicates more elements than we have space for. Indicating failure and continuing",
                                                  "Container size indicates more elements than we have space for. Unable to continue");
    }

    for(auto it = val.begin(); it != val.end(); ++it){
        in = SERIALIZE::Put<typename T::value_type>(&l_OK, std::move(in), offset, max, *it);
        if(l_OK == false){
            return FailWarn_or_Die(OK, std::move(in),"Could not put intrinsic within container. Indicating failure and continuing",
                                                     "Could not put intrinsic within container. Unable to continue");
        }
    }

    if(OK != nullptr) *OK = true;
    return std::move(in);
}
#ifndef YGORSERIALIZE_DISABLE_ALL_SPECIALIZATIONS
    //Fill this in with anything which is needed.
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, const std::list<uint8_t> &);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, const std::list<double> &);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, const std::list<std::string> &);
    
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, const std::vector<uint8_t> &);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, const std::vector<double> &);
    template std::unique_ptr<uint8_t[]> SERIALIZE::Put_Sequence_Container(bool *, std::unique_ptr<uint8_t[]>, uint64_t *, uint64_t, const std::vector<std::string> &);
#endif

