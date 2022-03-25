//YgorMathIOPLY.cc - Routines for reading and writing simple PLY files.
//
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorString.h"
#include "YgorBase64.h"   //Used for metadata serialization.
#include "YgorIO.h"

#include "YgorMathIOPLY.h"

// This awkward routine reads a number from a stream in either binary or text mode.
// The type is specified by the user, and is read as that type. But it is converted to type T after reading.
enum class number_type {
    other = 0,
    t_int8,
    t_uint8,
    t_int16,
    t_uint16,
    t_int32,
    t_int64,
    t_uint32,
    t_uint64,
    t_float32,
    t_float64
};

static
bool
number_type_is_floating_point(number_type format){
    return (format == number_type::t_float32)
        || (format == number_type::t_float64);
}

static
bool
number_type_is_integer(number_type format){
    return (format == number_type::t_int8)
        || (format == number_type::t_int16)
        || (format == number_type::t_int32)
        || (format == number_type::t_int64)
        || (format == number_type::t_uint8)
        || (format == number_type::t_uint16)
        || (format == number_type::t_uint32)
        || (format == number_type::t_uint64);
}

static
number_type
decode_number_type(const std::string& in){
    // These widths are part of the specification, however we basically ignore them here
    // and only use them as a coherence check during parsing.
    number_type t = number_type::other;
    if(false){
    }else if( (in == "float")
          ||  (in == "float32") ){    t = number_type::t_float32;
    }else if( (in == "double")
          ||  (in == "double64")
          ||  (in == "float64") ){    t = number_type::t_float64;

    }else if( (in == "uchar")
          ||  (in == "uchar8")
          ||  (in == "uint8") ){      t = number_type::t_uint8;
    }else if( (in == "ushort")
          ||  (in == "ushort16")
          ||  (in == "uint16") ){     t = number_type::t_uint16;
    }else if( (in == "uint")
          ||  (in == "uint32") ){     t = number_type::t_uint32;
    }else if( (in == "ulong")
          ||  (in == "ulong64")
          ||  (in == "uint64") ){     t = number_type::t_uint64;

    }else if( (in == "char")
          ||  (in == "char8")
          ||  (in == "int8") ){       t = number_type::t_int8;
    }else if( (in == "short")
          ||  (in == "short16")
          ||  (in == "int16") ){      t = number_type::t_int16;
    }else if( (in == "int")
          ||  (in == "int32") ){      t = number_type::t_int32;
    }else if( (in == "long")
          ||  (in == "long64")
          ||  (in == "int64") ){      t = number_type::t_int64;

    }else{
        throw std::logic_error("Unrecognized number_type format, cannot decode");
    }
    return t;
}

template <class T>
inline
std::string
encode_number_type(){
    std::string name;
    if(false){
    }else if( std::is_same<T, int8_t   >::value ){  name = "char8";
    }else if( std::is_same<T, int16_t  >::value ){  name = "short16";
    }else if( std::is_same<T, int32_t  >::value ){  name = "int32";
    }else if( std::is_same<T, int64_t  >::value ){  name = "int64";
                                     
    }else if( std::is_same<T, uint8_t  >::value ){  name = "uchar8";
    }else if( std::is_same<T, uint16_t >::value ){  name = "ushort16";
    }else if( std::is_same<T, uint32_t >::value ){  name = "uint32";
    }else if( std::is_same<T, uint64_t >::value ){  name = "uint64";
                                     
    }else if( std::is_same<T, float    >::value ){  name = "float32";
    }else if( std::is_same<T, double   >::value ){  name = "double64";
    }else{
        throw std::logic_error("Unrecognized number_type format, cannot encode");
    }
    return name;
}

// Implmentation function for endian-aware binary reads. This function converts the runtime endian parameter to a
// compilation-time template parameter as needed for upstream routine.
template <class T>
[[maybe_unused]]
T
endian_binary_read_as( std::istream &is,
                       YgorEndianness stream_endianness){
    T shtl;
    if(false){
    }else if( (stream_endianness == YgorEndianness::Little)
          &&  !ygor::io::read_binary<T,YgorEndianness::Little>(is, shtl) ){
        throw std::runtime_error("Binary stream read error (little-endian)");
    }else if( (stream_endianness == YgorEndianness::Big)
          &&  !ygor::io::read_binary<T,YgorEndianness::Big>(is, shtl) ){
        throw std::runtime_error("Binary stream read error (big-endian)");
    }
    return shtl;
}

template <class T>
[[maybe_unused]]
T
read_as( std::istream &is,
         number_type format,
         bool read_as_binary,
         YgorEndianness stream_endianness){

    T shtl;
    if(read_as_binary){

        if(false){
        }else if(format == number_type::t_float32){
            auto val = endian_binary_read_as<float>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_float64){
            auto val = endian_binary_read_as<double>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint8){
            auto val = endian_binary_read_as<uint8_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint16){
            auto val = endian_binary_read_as<uint16_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint32){
            auto val = endian_binary_read_as<uint32_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint64){
            auto val = endian_binary_read_as<uint64_t>(is,stream_endianness);
            shtl = static_cast<T>(val);


        }else if(format == number_type::t_int8){
            auto val = endian_binary_read_as<int8_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_int16){
            auto val = endian_binary_read_as<int16_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_int32){
            auto val = endian_binary_read_as<int32_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_int64){
            auto val = endian_binary_read_as<int64_t>(is,stream_endianness);
            shtl = static_cast<T>(val);

        }else{
            throw std::logic_error("Binary stream: unrecognized number format");
       }

    }else{
        // Attempting to read the number directly as a T can cause nan and inf to be missed, so rely on more explicit
        // (but slower) conversion.
        std::string w;
        if(!(is >> w)){
            throw std::runtime_error("Text stream read error");
        }
        if(number_type_is_floating_point(format)){
            shtl = static_cast<T>(std::stod(w));
        }else{
            shtl = static_cast<T>(std::stoll(w));
        }
    }
    return shtl;
}

// Note: the following type (T) should be integer.
template <class T>
[[maybe_unused]]
std::vector<T>
read_list_as( std::istream &is,
              number_type list_format,
              number_type format,
              bool read_as_binary,
              YgorEndianness stream_endianness){

    // Lists are preceeded by a number describing the length, so read it first.
    const auto N_list = read_as<T>(is, list_format, read_as_binary, stream_endianness);
    if( (N_list <= static_cast<T>(0)) 
    ||  (static_cast<T>(50) < N_list) ){
        throw std::runtime_error("List size invalid. Refusing to continue");
    }
    std::vector<T> out;
    out.reserve(N_list);
    for(auto i = static_cast<T>(0); i < N_list; ++i){
        out.push_back( read_as<T>(is, format, read_as_binary, stream_endianness) );
    }
    return out;
}


template <class T, class I>
bool
ReadFVSMeshFromPLY(fv_surface_mesh<T,I> &fvsm,
                   std::istream &is ){

    const auto reset = [&](){
        fvsm.vertices.clear();
        fvsm.vertex_normals.clear();
        fvsm.vertex_colours.clear();
        fvsm.faces.clear();
        fvsm.involved_faces.clear();
        fvsm.metadata.clear();
    };
    reset();

    // Check the magic number, which is required for either ASCII or binary files, before proceeding.
    try{
        char a, b, c;
        if( !is.get(a)
        ||  !is.get(b)
        ||  !is.get(c) ){
            throw std::runtime_error("Unable to read from stream");
        }
        if( ((a != 'p') && (a != 'P')) 
        ||  ((b != 'l') && (b != 'L'))
        ||  ((c != 'y') && (c != 'Y')) ){
            throw std::runtime_error("Missing 'ply' magic number");
        }
    }catch(const std::exception& e){
        FUNCWARN(e.what());
        reset();
        return false;
    }

    int parse_stage = 1;
    std::optional<bool> is_binary_opt;
    YgorEndianness stream_endianness = YgorEndianness::Little;

    struct property_t {
        std::string name = "";
        number_type type = number_type::other;

        bool is_list = false;
        // The type of the list length number. Usually uint8..
        number_type list_type = number_type::other;
    };
    struct element_t {
        std::string name = "";
        long int count = 0;
        std::vector<property_t> properties;
    };

    long int lineN = 1;
    std::list<element_t> elements;
    std::string line;
    try{
        while(std::getline(is, line)){
            ++lineN;
            if(line.empty()) continue;

            //auto split = SplitStringToVector(line, "comment", 'd'); // Remove any comments on any lines.
            //if(split.size() > 1) split.resize(1);
            auto split = SplitStringToVector(line, ' ', 'd');
            split = SplitVector(split, '\r', 'd'); // in case on Windows and the stream is in binary mode.
            //split = SplitVector(split, ',', 'd');
            split.erase( std::remove_if(std::begin(split),
                                        std::end(split),
                                        [](const std::string &t) -> bool {
                                            return t.empty();
                                        }),
                         std::end(split) );

            if(split.empty()) continue; // Skip all empty lines.

            // Handle metadata comments anywhere in the header.
            if(false){
            }else if( (1 <= split.size()) && (split.at(0) == "obj_info"_s)){
                // Handle 'obj_info' metadata statements.
                const auto p_space = line.find(" ");
                if( p_space == std::string::npos ) continue;
                fvsm.metadata["ObjInfo"] = line.substr(p_space + 1);

            }else if( (1 <= split.size()) && (split.at(0) == "comment"_s)){
                // Handle metadata packed into a comment line.
                auto kvp_opt = decode_metadata_kv_pair(line);
                if(kvp_opt){
                    fvsm.metadata.insert(kvp_opt.value());
                }else{
                    continue;
                }

            // Read the version statement.
            }else if( (parse_stage == 1) && (split.size() == 3) && (split.at(0) == "format"_s) 
                                                                && (split.at(1) == "ascii"_s)
                                                                && (split.at(2) == "1.0"_s) ){
                is_binary_opt = false;
                ++parse_stage;

            }else if( (parse_stage == 1) && (split.size() == 3) && (split.at(0) == "format"_s) 
                                                                && (split.at(1) == "binary_little_endian"_s)
                                                                && (split.at(2) == "1.0"_s) ){
                is_binary_opt = true;
                stream_endianness = YgorEndianness::Little;
                ++parse_stage;

            }else if( (parse_stage == 1) && (split.size() == 3) && (split.at(0) == "format"_s) 
                                                                && (split.at(1) == "binary_big_endian"_s)
                                                                && (split.at(2) == "1.0"_s) ){
                is_binary_opt = true;
                stream_endianness = YgorEndianness::Big;
                ++parse_stage;

            // Read which elements are present and what their properties are.
            }else if( (parse_stage == 2) && (split.size() == 3) && (split.at(0) == "element"_s)){
                elements.emplace_back();
                elements.back().name = split.at(1);
                try{
                    elements.back().count = std::stol(split.at(2));
                }catch(const std::exception &){ 
                    throw std::runtime_error( "Malformed element count encountered at line "_s
                                              + std::to_string(lineN) + ". Refusing to continue");
                }

            }else if( (parse_stage == 2) && !elements.empty() && (3 == split.size()) && (split.at(0) == "property"_s)){
                elements.back().properties.emplace_back();
                elements.back().properties.back().type = decode_number_type( split.at(1) ); // e.g., float
                elements.back().properties.back().name = split.at(2); // e.g., x
                elements.back().properties.back().is_list = false;

            }else if( (parse_stage == 2) && !elements.empty() && (5 == split.size()) && (split.at(0) == "property"_s) 
                                                                                     && (split.at(1) == "list"_s) ){
                elements.back().properties.emplace_back();
                elements.back().properties.back().list_type = decode_number_type( split.at(2) ); // e.g., uchar
                elements.back().properties.back().type = decode_number_type( split.at(3) ); // e.g., float
                elements.back().properties.back().name = split.at(4); // e.g., x
                elements.back().properties.back().is_list = true;

                if( !number_type_is_integer( elements.back().properties.back().list_type ) ){
                    throw std::runtime_error("Unsupported property list encountered. Refusing to continue");
                }

            }else if( (parse_stage == 2) && (split.size() == 1) && (split.at(0) == "end_header"_s)){
                ++parse_stage;
                break;

            }else{
                throw std::runtime_error( "Unanticipated data on line "_s
                                          + std::to_string(lineN) + ". Refusing to continue");
            }
        }
    }catch(const std::exception& e){
        FUNCWARN(e.what());
        reset();
        return false;
    }

    if(!is_binary_opt){
        reset();
        throw std::logic_error("Binary format status not detected.");
    }

    // Attempt to convert the stream to a binary stream, if necessary on this system.

    // ... TODO ...


    // The rest of the file is now considered an unstructured stream of numbers. We read them in one-by-one because line
    // breaks can occur anywhere, but determine ahead of time which numbers are needed/supported.
    try{

        // Parse elements in order specified.
        for(const auto& element : elements){

            // Vertex elements.
            //
            // Note: This element could be augmented to extract normals, colour, or other standard properties.
            if(false){
            }else if( (element.name == "vertex")
                  ||  (element.name == "vertices") ){
                long int index_x = -1; // vertices
                long int index_y = -1;
                long int index_z = -1;
                long int index_nx = -1; // vertex normals
                long int index_ny = -1;
                long int index_nz = -1;
                const auto N_props = static_cast<long int>(element.properties.size());

                for(long int i = 0; i < N_props; ++i){
                    if(false){
                    }else if( (index_x == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "x")
                          &&  number_type_is_floating_point(element.properties[i].type) ){
                        index_x = i;
                    }else if( (index_y == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "y")
                          &&  number_type_is_floating_point(element.properties[i].type) ){
                        index_y = i;
                    }else if( (index_z == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "z")
                          &&  number_type_is_floating_point(element.properties[i].type) ){
                        index_z = i;

                    }else if( (index_nx == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "nx")
                          &&  number_type_is_floating_point(element.properties[i].type) ){
                        index_nx = i;
                    }else if( (index_ny == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "ny")
                          &&  number_type_is_floating_point(element.properties[i].type) ){
                        index_ny = i;
                    }else if( (index_nz == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "nz")
                          &&  number_type_is_floating_point(element.properties[i].type) ){
                        index_nz = i;
                    }
                }
                if( (index_x == -1)
                ||  (index_y == -1)
                ||  (index_z == -1) ){
                    throw std::runtime_error("Unable to identify PLY vertex position properties. Unable to continue");
                }
                const bool has_normal = (index_nx != -1) || (index_ny != -1) || (index_nz != -1);
                if( has_normal ){
                    if( (index_nx == -1) // Ensure all normal components are accounted for.
                    ||  (index_ny == -1)
                    ||  (index_nz == -1) ){
                        throw std::runtime_error("Unable to identify PLY vertex normal properties. Unable to continue");
                    }
                }

                // Read in all properties, disregarding those other than the vertex positions.
                fvsm.vertices.reserve(element.count);
                vec3<T> v_shtl( static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) );
                vec3<T> n_shtl( static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) );
                for(long int n = 0; n < element.count; ++n){
                    for(long int i = 0; i < N_props; ++i){
                        if(false){
                        }else if(i == index_x){
                            v_shtl.x = read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);
                        }else if(i == index_y){
                            v_shtl.y = read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);
                        }else if(i == index_z){
                            v_shtl.z = read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);

                        }else if(i == index_nx){
                            n_shtl.x = read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);
                        }else if(i == index_ny){
                            n_shtl.y = read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);
                        }else if(i == index_nz){
                            n_shtl.z = read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);

                        }else if(element.properties[i].is_list){
                            read_list_as<I>(is, element.properties[i].list_type,
                                                element.properties[i].type,
                                                is_binary_opt.value(),
                                                stream_endianness);
                        }else{
                            read_as<T>(is, element.properties[i].type, is_binary_opt.value(), stream_endianness);
                        }
                    }
                    fvsm.vertices.push_back(v_shtl);
                    if(has_normal) fvsm.vertex_normals.push_back(n_shtl.unit());
                }

            // Face elements.
            }else if( (element.name == "face")
                  ||  (element.name == "faces")
                  ||  (element.name == "facet")
                  ||  (element.name == "facets") ){
                long int index_vs = -1;
                const auto N_props = static_cast<long int>(element.properties.size());
                for(long int i = 0; i < N_props; ++i){

                    if(false){
                    }else if( (index_vs == -1)
                          &&  element.properties[i].is_list
                          &&  (  (element.properties[i].name == "vertex_index")
                              || (element.properties[i].name == "vertex_indices")
                              || (element.properties[i].name == "vertex-index")
                              || (element.properties[i].name == "vertex-indices") )
                          &&  number_type_is_integer(element.properties[i].list_type)
                          &&  number_type_is_integer(element.properties[i].type) ){
                        index_vs = i;
                    }
                }
                if( index_vs == -1 ){
                    throw std::runtime_error("Unable to identify PLY face vertex_index property. Unable to continue");
                }

                // Read in all properties, disregarding those other than the face's connected vertices list.
                fvsm.vertices.reserve(element.count);
                for(long int n = 0; n < element.count; ++n){
                    for(long int i = 0; i < N_props; ++i){
                        if(false){
                        }else if(i == index_vs){
                            // Note: list should already be zero-indexed.
                            const auto l = read_list_as<I>(is, element.properties[i].list_type,
                                                               element.properties[i].type,
                                                               is_binary_opt.value(),
                                                               stream_endianness);
                            fvsm.faces.push_back(l);

                        }else if(element.properties[i].is_list){
                            read_list_as<I>(is, element.properties[i].list_type,
                                                element.properties[i].type,
                                                is_binary_opt.value(),
                                                stream_endianness);
                        }else{
                            read_as<T>(is, element.properties[i].type,
                                           is_binary_opt.value(),
                                           stream_endianness);
                        }
                    }
                }

            // Unknown / unsupported elements.
            //
            // PLY files can contain many types of alternate geometry elements, so it is important to skip over them to
            // improve interoperability, even if it makes it harder to verify file contents were parsed correctly.
            }else{
                for(long int n = 0; n < element.count; ++n){
                    const auto N_props = static_cast<long int>(element.properties.size());
                    for(long int i = 0; i < N_props; ++i){
                        if(false){
                        }else if(element.properties[i].is_list){
                            read_list_as<I>(is, element.properties[i].list_type,
                                                element.properties[i].type,
                                                is_binary_opt.value(),
                                                stream_endianness);
                        }else{
                            read_as<T>(is, element.properties[i].type,
                                           is_binary_opt.value(),
                                           stream_endianness);
                        }
                    }
                }
            }

        }


        // Verify that the indices are reasonable.
        {
            const auto N_verts = fvsm.vertices.size();
            if(N_verts == 0){
                throw std::runtime_error("No vertices present. Refusing to continue");
            }
            if( !fvsm.vertex_normals.empty()
            &&  (fvsm.vertex_normals.size() != fvsm.vertices.size())){
                throw std::runtime_error("Inconsistent number of vertices and normals -- refusing to parse as surface mesh");
            }
            for(const auto &fv : fvsm.faces){
                if(fv.empty()){
                    throw std::runtime_error("Encountered face with zero involved vertices");
                }
                for(const auto &v_i : fv){
                    if((v_i < 0) || (N_verts <= v_i)){
                        std::stringstream ss;
                        ss << "Face references non-existent vertex (" << v_i << ", but N_verts = " << N_verts << ")";
                        throw std::runtime_error(ss.str().c_str());
                    }
                }
            }
        }

        // Create the involved_faces index.
        fvsm.recreate_involved_face_index();

    }catch(const std::exception &e){ 
        FUNCWARN(e.what());
        reset();
        return false;
    }

    return true;
}
#ifndef YGORMATHIOPLY_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromPLY(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromPLY(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromPLY(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromPLY(fv_surface_mesh<double, uint64_t> &, std::istream &);
#endif


template <class T, class I>
bool
WriteFVSMeshToPLY(const fv_surface_mesh<T,I> &fvsm,
                  std::ostream &os,
                  bool as_binary ){

    size_t N_faces = 0;
    for(const auto &fv : fvsm.faces){
        if(!fv.empty()) ++N_faces;
        if(static_cast<size_t>(255) < fv.size()){
            throw std::runtime_error("Face contains more than 2^8-1 (=255) vertices. This is not currently supported.");
            // It would be easy to support this, but it currently is not to minimize file space needed.
            // Also, it seems unusual for a single face to have 255 corners for most purposes.
        }
    }

    if(fvsm.vertices.empty()){
        FUNCWARN("No vertices to write. Refusing to continue");
        return false;
    }
    const size_t N_verts = fvsm.vertices.size();

    const bool has_normals = !fvsm.vertex_normals.empty();
    if( has_normals
    &&  (fvsm.vertex_normals.size() != fvsm.vertices.size())){
        throw std::runtime_error("Inconsistent number of vertices and normals -- refusing to write PLY file");
    }

    // To minimize storage requirements and improve portability, we use the smallest integer possible for faces.
    //
    // NOTE: use_int8_t and use_int16_t are disabled to maximize portability. They work, but Meshlab/VCGlib doesn't
    //       honour them, so we disable them until a better workaround is found.
    const bool use_int8_t  = false; //(N_verts < std::numeric_limits<int8_t >::max());
    const bool use_int16_t = false; //(N_verts < std::numeric_limits<int16_t>::max());
    const bool use_int32_t = (N_verts < std::numeric_limits<int32_t>::max());

    if(os.bad()){
        FUNCWARN("Stream initially in invalid state. Refusing to continue");
        return false;
    }

   // For simplicity, we will always write little-endian binaries, regardless of whether the host is little- or
   // big-endian. If needed, this can be made a function parameter.
    constexpr YgorEndianness write_endianness = YgorEndianness::Little;

    os << "ply\n";
    if(as_binary){
       os << "format binary_little_endian 1.0\n";
    }else{
       os << "format ascii 1.0\n";
    }

    // Emit metadata.
    for(const auto &mp : fvsm.metadata){
        os << "comment " << encode_metadata_kv_pair(mp) << "\n";
    }

    os << "element vertex " << fvsm.vertices.size() << "\n"
       << "property float" << (sizeof(T)*8) << " x\n"
       << "property float" << (sizeof(T)*8) << " y\n"
       << "property float" << (sizeof(T)*8) << " z\n";
    if(has_normals){
        os << "property float" << (sizeof(T)*8) << " nx\n"
           << "property float" << (sizeof(T)*8) << " ny\n"
           << "property float" << (sizeof(T)*8) << " nz\n";
    }
    if(N_faces != 0){
        std::string face_enc_type = encode_number_type<I>();
        face_enc_type = (use_int32_t ? encode_number_type<int32_t>() : face_enc_type);
        face_enc_type = (use_int16_t ? encode_number_type<int16_t>() : face_enc_type);
        face_enc_type = (use_int8_t  ? encode_number_type<int8_t >() : face_enc_type);

        os << "element face " << N_faces << "\n"
           << "property list uchar " << face_enc_type << " vertex_index\n";
    }
    os << "end_header\n";
    if(os.bad()){
        FUNCWARN("Unable to write header. Refusing to continue");
        return false;
    }

    // Attempt to switch to a binary stream, if applicable.

    // ... TODO ...


    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );

    if(as_binary){
        for(size_t i = 0; i < N_verts; ++i){
            if( !ygor::io::write_binary<T,write_endianness>(os, fvsm.vertices[i].x)
            ||  !ygor::io::write_binary<T,write_endianness>(os, fvsm.vertices[i].y)
            ||  !ygor::io::write_binary<T,write_endianness>(os, fvsm.vertices[i].z) ){
                FUNCWARN("Unable to write vertex to stream. Cannot continue");
                return false;
            }
            if( has_normals ){
                if( !ygor::io::write_binary<T,write_endianness>(os, fvsm.vertex_normals[i].x)
                ||  !ygor::io::write_binary<T,write_endianness>(os, fvsm.vertex_normals[i].y)
                ||  !ygor::io::write_binary<T,write_endianness>(os, fvsm.vertex_normals[i].z) ){
                    FUNCWARN("Unable to write vertex normals to stream. Cannot continue");
                    return false;
                }
            }
        }
        for(const auto &fv : fvsm.faces){
            if(fv.empty()) continue;

            const auto N = static_cast<uint8_t>(fv.size());
            if( !ygor::io::write_binary<uint8_t,write_endianness>(os, N) ){
                FUNCWARN("Unable to write facet list length to stream. Cannot continue");
                return false;
            }

            for(const auto &i : fv){
                if(false){
                }else if(use_int8_t){
                    if( !ygor::io::write_binary<int8_t ,write_endianness>(os, static_cast<int8_t >(i)) ){
                        FUNCWARN("Unable to write facet list number. Cannot continue");
                        return false;
                    }
                }else if(use_int16_t){
                    if( !ygor::io::write_binary<int16_t,write_endianness>(os, static_cast<int16_t>(i)) ){
                        FUNCWARN("Unable to write facet list number. Cannot continue");
                        return false;
                    }

                }else if(use_int32_t){
                    if( !ygor::io::write_binary<int32_t,write_endianness>(os, static_cast<int32_t>(i)) ){
                        FUNCWARN("Unable to write facet list number. Cannot continue");
                        return false;
                    }
                }else{
                    if( !ygor::io::write_binary<I,write_endianness>(os, i) ){
                        FUNCWARN("Unable to write facet list number. Cannot continue");
                        return false;
                    }
                }
            }
        }

    }else{
        for(size_t i = 0; i < N_verts; ++i){
            if(!(os << fvsm.vertices[i].x << " "
                    << fvsm.vertices[i].y << " "
                    << fvsm.vertices[i].z << "\n")){
                FUNCWARN("Unable to write vertex to stream. Cannot continue");
                return false;
            }
            if( has_normals ){
                if(!(os << fvsm.vertex_normals[i].x << " "
                        << fvsm.vertex_normals[i].y << " "
                        << fvsm.vertex_normals[i].z << "\n")){
                    FUNCWARN("Unable to write vertex normals to stream. Cannot continue");
                    return false;
                }
            }
        }
        for(const auto &fv : fvsm.faces){
            if(fv.empty()) continue;

            if(!( os << fv.size() )){
                FUNCWARN("Unable to write facet list length to stream. Cannot continue");
                return false;
            }
            for(const auto &i : fv){
                // Cast to avoid a uint8_t face being treated as a char by the stream.
                if(!( os << " " << static_cast<uint64_t>(i) )){
                    FUNCWARN("Unable to write facet list number. Cannot continue");
                    return false;
                }
            }
            if(!( os << "\n" )){
                FUNCWARN("Unable to write facet newline. Cannot continue");
                return false;
            }
        }
    }
    os.flush();
    os.precision( original_precision );

    return (!os.bad());
}
#ifndef YGORMATHIOPLY_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToPLY(const fv_surface_mesh<float , uint32_t> &, std::ostream &, bool);
    template bool WriteFVSMeshToPLY(const fv_surface_mesh<float , uint64_t> &, std::ostream &, bool);

    template bool WriteFVSMeshToPLY(const fv_surface_mesh<double, uint32_t> &, std::ostream &, bool);
    template bool WriteFVSMeshToPLY(const fv_surface_mesh<double, uint64_t> &, std::ostream &, bool);
#endif


