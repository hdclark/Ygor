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

static
std::string
encode_number_type(number_type format){
    std::string name;
    if(false){
    }else if( format == number_type::t_int8 ){     name = "char8";
    }else if( format == number_type::t_int16 ){    name = "short16";
    }else if( format == number_type::t_int32 ){    name = "int32";
    }else if( format == number_type::t_int64 ){    name = "int64";

    }else if( format == number_type::t_uint8 ){    name = "uchar8";
    }else if( format == number_type::t_uint16 ){   name = "ushort16";
    }else if( format == number_type::t_uint32 ){   name = "uint32";
    }else if( format == number_type::t_uint64 ){   name = "uint64";

    }else if( format == number_type::t_float32 ){  name = "float32";
    }else if( format == number_type::t_float64 ){  name = "double64";
    }else{
        throw std::logic_error("Unrecognized number_type format, cannot encode");
    }
    return name;
}

template <class T>
[[maybe_unused]]
T
read_as( std::istream &is,
         number_type format,
         bool read_as_binary ){
    T shtl;
    if(read_as_binary){
        if(false){
        }else if(format == number_type::t_float32){
            float val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (float32)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_float64){
            double val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (float64)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint8){
            uint8_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (uint8)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint16){
            uint16_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (uint16)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint32){
            uint32_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (uint32)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_uint64){
            uint64_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (uint64)");
            }
            shtl = static_cast<T>(val);


        }else if(format == number_type::t_int8){
            int8_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (int8)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_int16){
            int16_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (int16)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_int32){
            int32_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (int32)");
            }
            shtl = static_cast<T>(val);

        }else if(format == number_type::t_int64){
            int64_t val = 0;
            if(!is.read(reinterpret_cast<char*>(&(val)), sizeof(val))){
                throw std::runtime_error("Binary stream read error (int64)");
            }
            shtl = static_cast<T>(val);

        }else{
            throw std::logic_error("Binary stream: unrecognized number format");
        }

    }else{
        // Attempting to read the number directly as a T can cause nan and inf to be missed, so rely on more explicit
        // conversion.
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
              bool read_as_binary ){

    // Lists are preceeded by a number describing the length, so read it first.
    const auto N_list = read_as<T>(is, list_format, read_as_binary);
    if( (N_list <= static_cast<T>(0)) 
    ||  (static_cast<T>(50) < N_list) ){
        throw std::runtime_error("List size invalid. Refusing to continue");
    }
    std::vector<T> out;
    out.reserve(N_list);
    for(auto i = static_cast<T>(0); i < N_list; ++i){
        out.push_back( read_as<T>(is, format, read_as_binary) );
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


    int parse_stage = 0;
    std::optional<bool> is_binary_opt;
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
            //split = SplitVector(split, '\t', 'd');
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
            }else if( (1 <= split.size()) && (split.at(0) == "comment"_s)){
                // Note: Syntax should be:
                // |  # metadata: key = value
                // |  # base64 metadata: encoded_key = encoded_value
                const auto p_assign = line.find(" = ");
                const auto p_metadata = line.find("metadata: ");
                const auto p_base64 = line.find("base64 metadata: ");
                if( (p_assign == std::string::npos)
                ||  (p_metadata == std::string::npos) ) continue; // Is a non-metadata comment.

                // Determine the boundaries of the key and value.
                const auto value = line.substr(p_assign + 3);
                const auto key_offset = p_metadata + 10;
                const auto key = line.substr(key_offset, (p_assign - key_offset));

                // Decode using base64, if necessary.
                if( (p_base64 == std::string::npos)
                ||  (p_metadata < p_base64) ){ // If the base64 keyword appears in the metadata itself.
                    fvsm.metadata[key] = value;
                }else{
                    const auto decoded_key = Base64::DecodeToString(key);
                    const auto decoded_value = Base64::DecodeToString(value);
                    fvsm.metadata[decoded_key] = decoded_value;
                }

            // Read the magic number.
            }else if( (parse_stage == 0) && (split.size() == 1) && (split.at(0) == "ply"_s)){
                ++parse_stage;

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
                throw std::runtime_error( "Unanticipated line at line "_s
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
                long int index_x = -1;
                long int index_y = -1;
                long int index_z = -1;
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
                    }
                }
                if( (index_x == -1)
                ||  (index_y == -1)
                ||  (index_z == -1) ){
                    throw std::runtime_error("Unable to identify PLY vertex position properties. Unable to continue");
                }

                // Read in all properties, disregarding those other than the vertex positions.
                fvsm.vertices.reserve(element.count);
                vec3<T> shtl( static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) );;
                for(long int n = 0; n < element.count; ++n){
                    for(long int i = 0; i < N_props; ++i){
                        if(false){
                        }else if(i == index_x){
                            shtl.x = read_as<T>(is, element.properties[i].type, is_binary_opt.value());
                        }else if(i == index_y){
                            shtl.y = read_as<T>(is, element.properties[i].type, is_binary_opt.value());
                        }else if(i == index_z){
                            shtl.z = read_as<T>(is, element.properties[i].type, is_binary_opt.value());
                        }else if(element.properties[i].is_list){
                            read_list_as<I>(is, element.properties[i].list_type, element.properties[i].type, is_binary_opt.value());
                        }else{
                            read_as<T>(is, element.properties[i].type, is_binary_opt.value());
                        }
                    }
                    fvsm.vertices.push_back(shtl);
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
                            const auto l = read_list_as<I>(is, element.properties[i].list_type, element.properties[i].type, is_binary_opt.value());
                            fvsm.faces.push_back(l);

                        }else if(element.properties[i].is_list){
                            read_list_as<I>(is, element.properties[i].list_type, element.properties[i].type, is_binary_opt.value());
                        }else{
                            read_as<T>(is, element.properties[i].type, is_binary_opt.value());
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
                            read_list_as<I>(is, element.properties[i].list_type, element.properties[i].type, is_binary_opt.value());
                        }else{
                            read_as<T>(is, element.properties[i].type, is_binary_opt.value());
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

    // Used to determine when text must be base64 encoded.
    const auto needs_to_be_escaped = [](const std::string &in) -> bool {
        for(const auto &x : in){
            // Permit words/sentences but not characters that could potentially affect file interpretation.
            // Note that whitespace is significant and will not be altered.
            if( !std::isprint(x) 
                || (x == static_cast<unsigned char>('=')) ) return true;
        }
        return false;
    };

    if(os.bad()){
        FUNCWARN("Stream initially in invalid state. Refusing to continue");
        return false;
    }

    os << "ply\n";
    if(as_binary){
       os << "format binary_little_endian 1.0\n";
    }else{
       os << "format ascii 1.0\n";
    }

    // Emit metadata.
    {
        for(const auto &mp : fvsm.metadata){
            const auto key = mp.first;
            const auto value = mp.second;
            const bool must_encode = needs_to_be_escaped(key) || needs_to_be_escaped(value);
            if(must_encode){
                const auto encoded_key   = Base64::EncodeFromString(key);
                const auto encoded_value = Base64::EncodeFromString(value);
                os << "comment base64 metadata: " << encoded_key << " = " << encoded_value << "\n";
            }else{
                // If encoding is not needed then don't. It will make the data more accessible.
                os << "comment metadata: " << key << " = " << value << "\n";
            }
        }
    }

    os << "element vertex " << fvsm.vertices.size() << "\n"
       << "property float" << sizeof(T)*8 << " x\n"
       << "property float" << sizeof(T)*8 << " y\n"
       << "property float" << sizeof(T)*8 << " z\n";
    if(N_faces != 0){
        os << "element face " << N_faces << "\n"
           << "property list uchar int" << sizeof(I)*8 << " vertex_index\n";
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
        for(const auto &v : fvsm.vertices){
            if( !os.write(reinterpret_cast<const char*>(&(v.x)), sizeof(v.x))
            ||  !os.write(reinterpret_cast<const char*>(&(v.y)), sizeof(v.y))
            ||  !os.write(reinterpret_cast<const char*>(&(v.z)), sizeof(v.z)) ){
                FUNCWARN("Unable to write vertex to stream. Cannot continue");
                return false;
            }
        }
        for(const auto &fv : fvsm.faces){
            if(fv.empty()) continue;

            const auto N = static_cast<uint8_t>(fv.size());
            if( !os.write(reinterpret_cast<const char*>(&(N)), sizeof(N)) ){
                FUNCWARN("Unable to write facet list length to stream. Cannot continue");
                return false;
            }

            for(const auto &i : fv){
                if( !os.write(reinterpret_cast<const char*>(&(i)), sizeof(i)) ){
                    FUNCWARN("Unable to write facet list number. Cannot continue");
                    return false;
                }
            }
        }

    }else{
        for(const auto &v : fvsm.vertices){
            if(!(os << v.x << " " << v.y << " " << v.z << "\n")){
                FUNCWARN("Unable to write vertex to stream. Cannot continue");
                return false;
            }
        }
        for(const auto &fv : fvsm.faces){
            if(fv.empty()) continue;

            if(!( os << fv.size() )){
                FUNCWARN("Unable to write facet list length to stream. Cannot continue");
                return false;
            }
            for(const auto &i : fv){
                if(!( os << " " << i )){
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


