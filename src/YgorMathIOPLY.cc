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

#include "YgorMathIOPLY.h"


//This enum is used by the user to signal whether they want little- or big-endianness when the IO format
// can handle either (e.g., writing raw pixels, FITS files).
enum YgorMathIOPLYEndianness { 
    Little,   // i.e., least significant byte at lowest memory address.
    Big,      // i.e., most significant byte at lowest memory address.
    Default   // User unspecified: use the default or try to detect.
};

static inline
YgorMathIOPLYEndianness
Detect_Machine_Endianness(void){

    //Check if we are on a big-endian (i.e., "MSB") or little-endian ("LSB") machine. We do this by 
    // probing where a single bit resides in memory.
    //
    // NOTE: If endianness is not little or big, this routine throws! Feel free to add additional
    //       endian types if needed.
    //
    volatile uint64_t EndianScape = static_cast<uint64_t>(1); //Anything larger than 1 byte will suffice.
    volatile uint8_t *EndianCheck = reinterpret_cast<volatile uint8_t *>(&EndianScape);

    const bool UsingLittleEndian = (EndianCheck[0] == static_cast<uint8_t>(1)); // "LSB".
    const bool UsingBigEndian    = (EndianCheck[sizeof(uint64_t)-1] == static_cast<uint8_t>(1)); // "MSB".

    if(UsingLittleEndian){
        return YgorMathIOPLYEndianness::Little;
    }else if(UsingBigEndian){
        return YgorMathIOPLYEndianness::Big;
    }

    throw std::runtime_error("Cannot determine machine's endianness!");
    return YgorMathIOPLYEndianness::Default; //(You should never get here.)        
}


// This routine reads an fv_surface_mesh from an ASCII PLY format stream.
//
// Note that this routine does not validate or enforce manifoldness.
//
// Note that PLY files can not contain metadata.
//
template <class T, class I>
bool
ReadFVSMeshFromASCIIPLY(fv_surface_mesh<T,I> &fvsm,
                        std::istream &is ){

    const auto reset = [&](){
        fvsm.vertices.clear();
        fvsm.faces.clear();
        fvsm.involved_faces.clear();
        fvsm.metadata.clear();
    };
    reset();

    const auto literal_is_floating_point = [](const std::string& in) -> bool {
        // These widths are part of the specification, however we basically ignore them here
        // and only use them as a coherence check during parsing.
        return (in == "float") 
            || (in == "float32")
            || (in == "double")
            || (in == "double64");
    };

    const auto literal_is_integer = [](const std::string& in) -> bool {
        // These widths are part of the specification, however we basically ignore them here
        // and only use them as a coherence check during parsing.
        return (in == "char") 
            || (in == "char8") 
            || (in == "uchar")
            || (in == "uchar8")
            || (in == "short")
            || (in == "short16")
            || (in == "ushort")
            || (in == "ushort16")
            || (in == "int")
            || (in == "int32")
            || (in == "int64")
            || (in == "uint")
            || (in == "uint32")
            || (in == "uint64");
    };

    int parse_stage = 0;
    struct property_t {
        std::string name = "";
        std::string type = "";
        bool is_list = false;
    };
    struct element_t {
        std::string name = "";
        long int count = 0;
        std::vector<property_t> properties;
    };

    long int lineN = 1;
    std::list<element_t> elements;
    std::string line;
    while(std::getline(is, line)){
        ++lineN;
        if(line.empty()) continue;

        auto split = SplitStringToVector(line, "comment", 'd'); // Remove any comments on any lines.
        if(split.size() > 1) split.resize(1);
        split = SplitVector(split, ' ', 'd');
        //split = SplitVector(split, '\t', 'd');
        //split = SplitVector(split, ',', 'd');
        split.erase( std::remove_if(std::begin(split),
                                    std::end(split),
                                    [](const std::string &t) -> bool {
                                        return t.empty();
                                    }),
                     std::end(split) );

        if(split.empty()) continue; // Skip all empty lines.

        // Read the magic number.
        if(false){
        }else if( (parse_stage == 0) && (split.size() == 1) && (split.at(0) == "ply"_s)){
            ++parse_stage;

        // Read the version statement.
        }else if( (parse_stage == 1) && (split.size() == 3) && (split.at(0) == "format"_s) 
                                                            && (split.at(1) == "ascii"_s)
                                                            && (split.at(2) == "1.0"_s) ){
            ++parse_stage;

        // Read which elements are present and what their properties are.
        }else if( (parse_stage == 2) && (split.size() == 3) && (split.at(0) == "element"_s)){
            elements.emplace_back();
            elements.back().name = split.at(1);
            try{
                elements.back().count = std::stol(split.at(2));
            }catch(const std::exception &){ 
                FUNCWARN("Malformed element count encountered at line " << lineN << ". Refusing to continue");
                reset();
                return false;
            }

        }else if( (parse_stage == 2) && !elements.empty() && (3 == split.size()) && (split.at(0) == "property"_s)){
            elements.back().properties.emplace_back();
            elements.back().properties.back().type = split.at(1); // e.g., float
            elements.back().properties.back().name = split.at(2); // e.g., x
            elements.back().properties.back().is_list = false;

        }else if( (parse_stage == 2) && !elements.empty() && (5 == split.size()) && (split.at(0) == "property"_s) 
                                                                                 && (split.at(1) == "list"_s) ){
            if( !literal_is_integer(split.at(2)) ){
                FUNCWARN("Unsupported property list encountered. Refusing to continue");
                reset();
                return false;
            }
            elements.back().properties.emplace_back();
            elements.back().properties.back().type = split.at(3); // e.g., float
            elements.back().properties.back().name = split.at(4); // e.g., x
            elements.back().properties.back().is_list = true;

        }else if( (parse_stage == 2) && (split.size() == 1) && (split.at(0) == "end_header"_s)){
            ++parse_stage;
            break;

        }else{
            // Note: the following warning is helpful for debugging, but can pollute stdout with arbitrary code!
            FUNCWARN("Unanticipated line at line " << lineN << ". Refusing to continue.");
            reset();
            return false;
        }
    }

    // The rest of the file is now considered an unstructured stream of numbers. We read them in one-by-one because line
    // breaks can occur anywhere, but determine ahead of time which numbers are needed/supported.
    try{
        auto it = std::istream_iterator<std::string>(is); // Tokenizes the stream using whitespace.
        auto end = std::istream_iterator<std::string>();

        const auto get_another_T = [&]() -> T {
            if(it == end){
                throw std::runtime_error("Unable to read input stream. Refusing to continue");
            }
            return static_cast<T>( std::stod( *it++ ) );
        };

        const auto get_another_I = [&]() -> I {
            if(it == end){
                throw std::runtime_error("Unable to read input stream. Refusing to continue");
            }
            return static_cast<I>( std::stol( *it++ ) );
        };

        const auto get_list_T = [&]() -> std::vector<T> {
            // Lists are preceeded by a number describing the length.
            const auto N = get_another_I();
            if((N <= static_cast<I>(0)) || (static_cast<I>(10) < N)){
                throw std::runtime_error("List size invalid. Refusing to continue");
            }
            std::vector<T> out;
            out.reserve(N);
            for(auto i = static_cast<I>(0); i < N; ++i) out.push_back(get_another_T());
            return out;
        };

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

                for(long int i = 0; i < element.properties.size(); ++i){
                    if(false){
                    }else if( (index_x == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "x")
                          &&  literal_is_floating_point(element.properties[i].type) ){
                        index_x = i;
                    }else if( (index_y == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "y")
                          &&  literal_is_floating_point(element.properties[i].type) ){
                        index_y = i;
                    }else if( (index_z == -1)
                          &&  !element.properties[i].is_list
                          &&  (element.properties[i].name == "z")
                          &&  literal_is_floating_point(element.properties[i].type) ){
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
                    for(long int i = 0; i < element.properties.size(); ++i){
                        if(false){
                        }else if(i == index_x){
                            shtl.x = get_another_T();
                        }else if(i == index_y){
                            shtl.y = get_another_T();
                        }else if(i == index_z){
                            shtl.z = get_another_T();
                        }else if(element.properties[i].is_list){
                            get_list_T();
                        }else{
                            get_another_T();
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
                for(long int i = 0; i < element.properties.size(); ++i){

                    if(false){
                    }else if( (index_vs == -1)
                          &&  element.properties[i].is_list
                          &&  (  (element.properties[i].name == "vertex_index")
                              || (element.properties[i].name == "vertex_indices")
                              || (element.properties[i].name == "vertex-index")
                              || (element.properties[i].name == "vertex-indices") )
                          &&  literal_is_floating_point(element.properties[i].type) ){
                        index_vs = i;
                    }
                }
                if( index_vs == -1 ){
                    throw std::runtime_error("Unable to identify PLY face vertex_index property. Unable to continue");
                }

                // Read in all properties, disregarding those other than the face's connected vertices list.
                fvsm.vertices.reserve(element.count);
                for(long int n = 0; n < element.count; ++n){
                    for(long int i = 0; i < element.properties.size(); ++i){
                        if(false){
                        }else if(i == index_vs){
                            const auto l = get_list_T(); // Should already be zero-indexed.
                            std::vector<I> il;
                            il.reserve(l.size());
                            for(const auto& x : l) il.push_back( static_cast<I>(x) );
                            fvsm.faces.push_back(il);

                        }else if(element.properties[i].is_list){
                            get_list_T();
                        }else{
                            get_another_T();
                        }
                    }
                }

            // Unknown / unsupported elements.
            //
            // PLY files can contain many types of alternate geometry elements, so it is important to skip over them to
            // improve interoperability, even if it makes it harder to verify file contents were parsed correctly.
            }else{
                for(long int n = 0; n < element.count; ++n){
                    for(long int i = 0; i < element.properties.size(); ++i){
                        if(false){
                        }else if(element.properties[i].is_list){
                            get_list_T();
                        }else{
                            get_another_T();
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
    template bool ReadFVSMeshFromASCIIPLY(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromASCIIPLY(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromASCIIPLY(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromASCIIPLY(fv_surface_mesh<double, uint64_t> &, std::istream &);
#endif


// This routine writes an fv_surface_mesh to an PLY format stream.
//
// Note that PLY files do not support metadata.
//
template <class T, class I>
bool
WriteFVSMeshToASCIIPLY(const fv_surface_mesh<T,I> &fvsm,
                       std::ostream &os ){

    if( fvsm.vertices.empty() ){
        FUNCWARN("Refusing to write mesh containing zero vertices");
        return false;
    }

    os << "ply" << std::endl
       << "format ascii 1.0" << std::endl
       //<< "comment ... encode metadata here ..." << std::endl
       //<< "comment ... encode metadata here ..." << std::endl
       //<< "comment ... encode metadata here ..." << std::endl
       << "element vertex " << fvsm.vertices.size() << std::endl
       << "property float x" << std::endl
       << "property float y" << std::endl
       << "property float z" << std::endl
       << "element face " << fvsm.faces.size() << std::endl
       << "property list uchar double vertex_index" << std::endl
       << "end_header" << std::endl;

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );

    for(const auto &v : fvsm.vertices){
        os << v.x << " " << v.y << " " << v.z << std::endl;
    }
    for(const auto &fv : fvsm.faces){
        if(fv.empty()) continue;

        os << fv.size();
        for(const auto &i : fv){
            os << " " << i;
        }
        os << std::endl;
    }
    os.flush();
    os.precision( original_precision );

    return (!os.bad());
}
#ifndef YGORMATHIOPLY_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToASCIIPLY(const fv_surface_mesh<float , uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToASCIIPLY(const fv_surface_mesh<float , uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToASCIIPLY(const fv_surface_mesh<double, uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToASCIIPLY(const fv_surface_mesh<double, uint64_t> &, std::ostream &);
#endif


