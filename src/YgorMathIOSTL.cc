//YgorMathIOSTL.cc - Routines for reading and writing simple STL ("Stereolithography") files.
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

#include "YgorMathIOSTL.h"


//This enum is used by the user to signal whether they want little- or big-endianness when the IO format
// can handle either (e.g., writing raw pixels, FITS files).
enum YgorMathIOSTLEndianness { 
    Little,   // i.e., least significant byte at lowest memory address.
    Big,      // i.e., most significant byte at lowest memory address.
    Default   // User unspecified: use the default or try to detect.
};

static inline
YgorMathIOSTLEndianness
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
        return YgorMathIOSTLEndianness::Little;
    }else if(UsingBigEndian){
        return YgorMathIOSTLEndianness::Big;
    }

    throw std::runtime_error("Cannot determine machine's endianness!");
    return YgorMathIOSTLEndianness::Default; //(You should never get here.)        
}


// This routine reads an fv_surface_mesh from an ASCII STL format stream.
//
// Note that this routine does not validate or enforce manifoldness.
//
// Note that STL files can not contain metadata.
//
template <class T, class I>
bool
ReadFVSMeshFromASCIISTL(fv_surface_mesh<T,I> &fvsm,
                        std::istream &is ){

    fvsm.vertices.clear();
    fvsm.faces.clear();
    fvsm.involved_faces.clear();

    std::vector< vec3<T> > vectors; // Buffer for unit vector and vertices.

    long int lineN = 1;
    std::string line;
    while(std::getline(is, line)){
        ++lineN;
        if(line.empty()) continue;

        auto split = SplitStringToVector(line, '#', 'd'); // Remove any comments on any lines.
        if(split.size() > 1) split.resize(1);
        split = SplitVector(split, ' ', 'd');
        split = SplitVector(split, '\t', 'd');
        //split = SplitVector(split, ',', 'd');
        split.erase( std::remove_if(std::begin(split),
                                    std::end(split),
                                    [](const std::string &t) -> bool {
                                        return t.empty();
                                    }),
                     std::end(split) );

        if(split.empty()) continue;

        // The following file parser effectively consists of a whitelist of the allowed states and line reads.
        // While the ASCII STL format *appears* to be somewhat flexible (e.g., support for arbitrary N-gon faces), it in
        // fact only permits a single linear format without any optional items. (Normal vectors can be invalid, but they
        // still must be present.)
        if(false){
        }else if( (vectors.size() == 0) && ((split.size() == 1) || (split.size() == 2)) && (split.at(0) == "solid"_s)){
            // Do nothing.

        // Extract a normal vector.
        }else if( (vectors.size() == 0) && (split.size() == 5) && (split.at(0) == "facet"_s) && (split.at(1) == "normal"_s)){
            try{
                const auto x = std::stod(split.at(2));
                const auto y = std::stod(split.at(3));
                const auto z = std::stod(split.at(4));

                vectors.emplace_back( static_cast<T>(x), 
                                      static_cast<T>(y),
                                      static_cast<T>(z) );
            }catch(const std::exception &){ 
                continue; 
            }

        }else if( (vectors.size() == 1) && (split.size() == 2) && (split.at(0) == "outer"_s) && (split.at(1) == "loop"_s)){
            // Do nothing.

        // Extract a vertex.
        }else if( ( (vectors.size() == 1) || (vectors.size() == 2) || (vectors.size() == 3) ) && (split.size() == 4) && (split.at(0) == "vertex"_s) ){
            // Do nothing.
            try{
                const auto x = std::stod(split.at(1));
                const auto y = std::stod(split.at(2));
                const auto z = std::stod(split.at(3));

                vectors.emplace_back( static_cast<T>(x), 
                                      static_cast<T>(y),
                                      static_cast<T>(z) );
            }catch(const std::exception &){ 
                continue; 
            }

        }else if( (vectors.size() == 4) && (split.size() == 1) && (split.at(0) == "endloop"_s)){
            fvsm.vertices.emplace_back( vectors[1] ); // Ignore the first vector, which holds a unit normal that we disregard.
            fvsm.vertices.emplace_back( vectors[2] );
            fvsm.vertices.emplace_back( vectors[3] );

            const auto v_offset = static_cast<long long int>(fvsm.vertices.size()) - 3LL;
            const std::vector<I> faces = {{ static_cast<I>(v_offset + 0LL),
                                            static_cast<I>(v_offset + 1LL),
                                            static_cast<I>(v_offset + 2LL) }};
            fvsm.faces.emplace_back( faces );
            vectors.clear();

        }else if( (vectors.size() == 0) && (split.size() == 1) && (split.at(0) == "endfacet"_s)){
            // Do nothing.

        }else if( (vectors.size() == 0) && ((split.size() == 1) || (split.size() == 2)) && (split.at(0) == "endsolid"_s)){
            // Do nothing.

        }else{
            FUNCWARN("Unanticipated line '" << line << "' (number " << lineN << "). Refusing to continue.");
            return false;
        }
    }

    // Connect duplicate vertices.
    //
    // Note: STL only requires single precision vertices, so we can safely ignore greater precision below 
    //       a reasonable threshold.
    fvsm.merge_duplicate_vertices( static_cast<T>(1E-5) );

    // Verify that the indices are reasonable.
    {
        const auto N_verts = fvsm.vertices.size();
        for(const auto &fv : fvsm.faces){
            if(fv.empty()){
                FUNCWARN("Encountered face with zero involved vertices");
                return false;
            }
            for(const auto &v_i : fv){
                if((v_i < 0) || (N_verts <= v_i)){
                    FUNCWARN("Face references non-existent vertex (" << v_i << ", but N_verts = " << N_verts << ")");
                    return false;
                }
            }
        }
    }

    // Create the involved_faces index.
    try{
        fvsm.recreate_involved_face_index();
    }catch(const std::exception &){ 
        FUNCWARN("Failed to recreate involved_faces index");
        return false;
    }

    return true;
}
#ifndef YGORMATHIOSTL_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromASCIISTL(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromASCIISTL(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromASCIISTL(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromASCIISTL(fv_surface_mesh<double, uint64_t> &, std::istream &);
#endif


// This routine writes an fv_surface_mesh to an STL format stream.
//
// Note that STL files do not support metadata.
//
template <class T, class I>
bool
WriteFVSMeshToASCIISTL(const fv_surface_mesh<T,I> &fvsm,
                       std::ostream &os ){

    os << "solid a" << std::endl;

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::digits10 + 1 );

    // Enable scientific floating-point output, as per the STL specification.
    os << std::scientific;

    // Emit faces one at a time.
    const auto N_verts = static_cast<long long int>(fvsm.vertices.size());
    for(const auto &fv : fvsm.faces){
        if(fv.empty()) continue;

        if(fv.size() != 3){
            throw std::runtime_error("Found non-triangle face; STL files only support triangles.");
        }
        const auto v_A = fvsm.vertices.at( static_cast<size_t>(fv[0]) );
        const auto v_B = fvsm.vertices.at( static_cast<size_t>(fv[1]) );
        const auto v_C = fvsm.vertices.at( static_cast<size_t>(fv[2]) );

        // Compute face normal.
        //
        // Note that it must be consistent with the normal derived from vertices.
        //const auto N = (v_B - v_A).Cross(v_C - v_A).unit();

        // Disregard the normal, forcing the reader to compute up-to-date normals as needed.
        const auto N = vec3<T>( static_cast<T>(0),
                                static_cast<T>(0),
                                static_cast<T>(0) );

        os << "  facet normal " << N.x << " " << N.y << " " << N.z << "\n";
        os << "    outer loop\n";
        os << "      vertex " << v_A.x << " " << v_A.y << " " << v_A.z << "\n";
        os << "      vertex " << v_B.x << " " << v_B.y << " " << v_B.z << "\n";
        os << "      vertex " << v_C.x << " " << v_C.y << " " << v_C.z << "\n";
        os << "    endloop\n";
        os << "  endfacet\n";
    }
    os << "endsolid a" << std::endl;
    os.flush();

    return (!os.fail());
}
#ifndef YGORMATHIOSTL_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<float , uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<float , uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<double, uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<double, uint64_t> &, std::ostream &);
#endif


// This routine reads an fv_surface_mesh from a binary STL format stream.
//
// Note that this routine does not validate or enforce manifoldness.
//
// Note that STL files can not contain metadata.
//
template <class T, class I>
bool
ReadFVSMeshFromBinarySTL(fv_surface_mesh<T,I> &fvsm,
                         std::istream &is ){

    // Work out the endianness of this machine.
    //
    // The STL file (implicitly?) requires little-endian writes.
    const auto MachineEndianness = Detect_Machine_Endianness();

    auto ReadUint16 = [&](void){
        uint16_t j = 0;
        unsigned char *c = reinterpret_cast<unsigned char *>(&j); //c[0] to c[sizeof(uint16_t)-1] only!
        if( false ){
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Big ){
            //Reverse the order of the bytes read from the stream.
            for(size_t i = 0; i <= (sizeof(uint16_t)-1); ++i){
                is.get( reinterpret_cast<char *>(&c[sizeof(uint16_t)-1-i]), sizeof(char) );
            }
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Little ){
            //Keep the byte order unaltered.
            is.read( reinterpret_cast<char *>( &c[0] ), sizeof(uint16_t) );
        }else{
            throw std::logic_error("Unable to determine machine endianness. Cannot continue.");
        }
        return j;
    };

    auto ReadUint32 = [&](void){
        uint32_t j = 0;
        unsigned char *c = reinterpret_cast<unsigned char *>(&j); //c[0] to c[sizeof(T)-1] only!
        if( false ){
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Big ){
            //Reverse the order of the bytes read from the stream.
            for(size_t i = 0; i <= (sizeof(uint32_t)-1); ++i){
                is.get( reinterpret_cast<char *>(&c[sizeof(uint32_t)-1-i]), sizeof(char) );
            }
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Little ){
            //Keep the byte order unaltered.
            is.read( reinterpret_cast<char *>( &c[0] ), sizeof(uint32_t) );
        }else{
            throw std::logic_error("Unable to determine machine endianness. Cannot continue.");
        }
        return j;
    };

    auto ReadVec3T = [&](void){
        // Verify we conform to standard types.
        //
        // The STL format requires IEEE-formatted 32 bit floats. If this computer does not implement it,
        // we cannot read the file correctly.
        if(!std::numeric_limits<float>::is_iec559) throw std::runtime_error("Refusing to read non-IEEE754 floating point values.");
        if((sizeof(float)) != 4) throw std::runtime_error("Float is not 32 bit. Refusing to continue.");

        auto v_pack = std::vector<float>{{ static_cast<float>(0), 
                                           static_cast<float>(0),
                                           static_cast<float>(0) }};
        for(float &x : v_pack){
            unsigned char *c = reinterpret_cast<unsigned char *>(&x); //c[0] to c[sizeof(float)-1] only!
            if( false ){
            }else if( MachineEndianness == YgorMathIOSTLEndianness::Big ){
                //Reverse the order of the bytes read from the stream.
                for(size_t i = 0; i <= (sizeof(float)-1); ++i){
                    is.get( reinterpret_cast<char *>(&c[sizeof(float)-1-i]), sizeof(char) );
                }
            }else if( MachineEndianness == YgorMathIOSTLEndianness::Little ){
                //Keep the byte order unaltered.
                is.read( reinterpret_cast<char *>( &c[0] ), sizeof(float) );
            }else{
                throw std::logic_error("Unable to determine machine endianness. Cannot continue.");
            }
        }

        return vec3<T>( static_cast<T>(v_pack[0]),
                        static_cast<T>(v_pack[1]),
                        static_cast<T>(v_pack[2]) );
    };

    fvsm.vertices.clear();
    fvsm.faces.clear();
    fvsm.involved_faces.clear();

    std::vector< vec3<T> > vectors; // Buffer for unit vector and vertices.

    // Skip the first 80 bytes, which can contain anything.
    is.seekg( 80, is.cur );

    // Read the number of faces (all assumed to be triangles).
    const auto N_faces = static_cast<I>( ReadUint32() );

    if((N_faces <= 0) || (220'000'000 < N_faces)){ 
        // Approx 10 GiB limit. Seems reasonable (for now), given that the file is fully loaded into memory. A uint32_t
        // can go up to 4'294'967'295, which would consume ~200 GiB. The chosen limit here is 1/20th the maximum to both
        // reduce the risk of a memory DOS, and reduce the risk of a large, non-STL file being misinterpretted as an STL
        // file.
        FUNCWARN("File is either not an STL file, or is too large to handle. Refusing to proceed");
        return false;
    }
    
    for(I i = 0; i < N_faces; ++i){
        //const auto normal = ReadVec3T();
        ReadVec3T(); // Read and discard the normal.
        const auto v_A = ReadVec3T();
        const auto v_B = ReadVec3T();
        const auto v_C = ReadVec3T();
        const auto N_attr = ReadUint16();

        if(N_attr != static_cast<uint16_t>(0)){
            FUNCWARN("This routine cannot handle attributes. Refusing to continue");
            return false;
        }
        if(is.fail()){
            FUNCWARN("File prematurely ended. File is either not an STL file, or is damaged");
            return false;
        }

        fvsm.vertices.emplace_back(v_A);
        fvsm.vertices.emplace_back(v_B);
        fvsm.vertices.emplace_back(v_C);

        const auto v_offset = static_cast<long long int>(fvsm.vertices.size()) - 3LL;
        const std::vector<I> faces = {{ static_cast<I>(v_offset + 0LL),
                                        static_cast<I>(v_offset + 1LL),
                                        static_cast<I>(v_offset + 2LL) }};
        fvsm.faces.emplace_back( faces );
    }
    if(is.peek() != std::char_traits<char>::eof()){
        FUNCWARN("All faces have been read, but EOF has not been reached. File may be invalid. Refusing to continue");
        // This could potentially indicate non-triangular faces have been encountered (and not handled correctly), the
        // file is corrupt, or there are additional file elements that this reader does not implement. It is safest to
        // assume the worst and refuse to continue.
        return false;
    }

    // Connect duplicate vertices.
    //
    // Note: STL only requires single precision vertices, so we can safely ignore greater precision below 
    //       a reasonable threshold.
    fvsm.merge_duplicate_vertices( static_cast<T>(1E-5) );

    // Verify that the indices are reasonable.
    {
        const auto N_verts = fvsm.vertices.size();
        for(const auto &fv : fvsm.faces){
            if(fv.empty()){
                FUNCWARN("Encountered face with zero involved vertices");
                return false;
            }
            for(const auto &v_i : fv){
                if((v_i < 0) || (N_verts <= v_i)){
                    FUNCWARN("Face references non-existent vertex (" << v_i << ", but N_verts = " << N_verts << ")");
                    return false;
                }
            }
        }
    }

    // Create the involved_faces index.
    try{
        fvsm.recreate_involved_face_index();
    }catch(const std::exception &){ 
        FUNCWARN("Failed to recreate involved_faces index");
        return false;
    }

    return true;
}
#ifndef YGORMATHIOSTL_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromBinarySTL(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromBinarySTL(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromBinarySTL(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromBinarySTL(fv_surface_mesh<double, uint64_t> &, std::istream &);
#endif


// This routine writes an fv_surface_mesh to an STL format stream.
//
// Note that STL files do not support metadata.
//
template <class T, class I>
bool
WriteFVSMeshToBinarySTL(const fv_surface_mesh<T,I> &fvsm,
                        std::ostream &os ){

    // Work out the endianness of this machine.
    //
    // The STL file (implicitly?) requires little-endian writes.
    const auto MachineEndianness = Detect_Machine_Endianness();

    auto WriteUint16 = [&](uint16_t j){
        unsigned char *c = reinterpret_cast<unsigned char *>(&j); //c[0] to c[sizeof(uint16_t)-1] only!
        if( false ){
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Big ){
            //Reverse the order of the bytes written to the stream.
            for(size_t i = 0; i <= (sizeof(uint16_t)-1); ++i) os.put(c[sizeof(uint16_t)-1-i]);
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Little ){
            //Keep the byte order unaltered.
            os.write( reinterpret_cast<char *>( &c[0] ), sizeof(uint16_t) );
        }else{
            throw std::logic_error("Unable to determine machine endianness. Cannot continue.");
        }
        return;
    };

    auto WriteUint32 = [&](uint32_t j){
        unsigned char *c = reinterpret_cast<unsigned char *>(&j); //c[0] to c[sizeof(T)-1] only!
        if( false ){
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Big ){
            //Reverse the order of the bytes written to the stream.
            for(size_t i = 0; i <= (sizeof(uint32_t)-1); ++i) os.put(c[sizeof(uint32_t)-1-i]);
        }else if( MachineEndianness == YgorMathIOSTLEndianness::Little ){
            //Keep the byte order unaltered.
            os.write( reinterpret_cast<char *>( &c[0] ), sizeof(uint32_t) );
        }else{
            throw std::logic_error("Unable to determine machine endianness. Cannot continue.");
        }
        return;
    };

    auto WriteVec3T = [&](vec3<T> v){
        // Verify we conform to standard types.
        //
        // The STL format requires IEEE-formatted 32 bit floats.
        if(!std::numeric_limits<float>::is_iec559) throw std::runtime_error("Refusing to write non-IEEE754 floating point values.");
        if((sizeof(float)) != 4) throw std::runtime_error("Float is not 32 bit. Refusing to continue.");

        auto v_pack = std::vector<float>{{ static_cast<float>(v.x), 
                                           static_cast<float>(v.y),
                                           static_cast<float>(v.z) }};
        for(float &x : v_pack){
            unsigned char *c = reinterpret_cast<unsigned char *>(&x); //c[0] to c[sizeof(float)-1] only!
            if( false ){
            }else if( MachineEndianness == YgorMathIOSTLEndianness::Big ){
                //Reverse the order of the bytes written to the stream.
                for(size_t i = 0; i <= (sizeof(float)-1); ++i) os.put(c[sizeof(float)-1-i]);
            }else if( MachineEndianness == YgorMathIOSTLEndianness::Little ){
                //Keep the byte order unaltered.
                os.write( reinterpret_cast<char *>( &c[0] ), sizeof(float) );
            }else{
                throw std::logic_error("Unable to determine machine endianness. Cannot continue.");
            }
        }
        return;
    };

    // Header. The contents are arbitrary but should not begin with 'solid' since readers may then assume this is an
    // ASCII STL file. Conversely, the header should ideally inform users what the file contains...
    os << "STL";
    for(size_t i = 3; i < 80; ++i) os << '\0';

    // Emit the number of faces.
    auto N_faces = static_cast<uint32_t>(fvsm.faces.size());
    WriteUint32(N_faces);

    // Emit the faces one at a time.
    for(const auto &fv : fvsm.faces){
        if(fv.size() != 3){
            throw std::runtime_error("Found non-triangle face; STL files only support triangles.");
        }
        auto v_A = fvsm.vertices.at( static_cast<size_t>(fv[0]) );
        auto v_B = fvsm.vertices.at( static_cast<size_t>(fv[1]) );
        auto v_C = fvsm.vertices.at( static_cast<size_t>(fv[2]) );

        // Compute face normal.
        //
        // Note that it must be consistent with the normal derived from vertices.
        //const auto N = (v_B - v_A).Cross(v_C - v_A).unit();

        // Disregard the normal, forcing the reader to compute up-to-date normals as needed.
        const auto N = vec3<T>( static_cast<T>(0),
                                static_cast<T>(0),
                                static_cast<T>(0) );

        // Emit normal.
        WriteVec3T(N);

        // Emit vertices.
        WriteVec3T(v_A);
        WriteVec3T(v_B);
        WriteVec3T(v_C);

        // Emit a dummy 2-byte 'attribute byte count' that should be zero.
        uint16_t dummy = 0;
        WriteUint16(dummy);
    }
    os.flush();

    return(!os.fail());
}
#ifndef YGORMATHIOSTL_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToBinarySTL(const fv_surface_mesh<float , uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToBinarySTL(const fv_surface_mesh<float , uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToBinarySTL(const fv_surface_mesh<double, uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToBinarySTL(const fv_surface_mesh<double, uint64_t> &, std::ostream &);
#endif

