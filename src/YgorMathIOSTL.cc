//YgorMathIOSTL.cc - Routines for reading and writing simple (ascii) STL ("Wavefront Object") files.
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

    return(!os.fail());
}
#ifndef YGORMATHIOSTL_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<float , uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<float , uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<double, uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToASCIISTL(const fv_surface_mesh<double, uint64_t> &, std::ostream &);
#endif

