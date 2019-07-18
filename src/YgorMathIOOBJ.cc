//YgorMathIOOBJ.cc - Routines for reading and writing simple (ascii) OBJ ("Wavefront Object") files.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorString.h"

#include "YgorMathIOOBJ.h"


// This routine reads an fv_surface_mesh from an OBJ format stream.
//
// Note that this routine does not validate or enforce manifoldness.
//
// Note that a subset of the full OBJ format is supported; materials, colours, and other mesh attributes
// are NOT supported. Vertices must have three coordinates. Faces can comprise [1-inf) number of vertices.
//
// Note that Wavefront OBJ files are 1-indexed. This routine transforms all lines and faces to 0-indexing.
//
template <class S, class T, class I>
bool
ReadFVSMeshFromOBJ(fv_surface_mesh<T,I> &fvsm,
                   S &ios ){

    fvsm.vertices.clear();
    fvsm.faces.clear();
    fvsm.involved_faces.clear();

    std::string line;
    while(std::getline(ios, line)){
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

        // Add a new vertex.
        if(false){
        }else if(split.at(0) == "v"_s){
            if(split.size() != 4) continue; // Actually an error...
            try{
                const auto x = std::stod(split.at(1));
                const auto y = std::stod(split.at(2));
                const auto z = std::stod(split.at(3));
                fvsm.vertices.emplace_back( static_cast<T>(x), 
                                            static_cast<T>(y),
                                            static_cast<T>(z) );
            }catch(const std::exception &){ 
                continue; 
            }

        // Add a new line (aka edge).
        //
        // Note that lines are treated as faces in this routine.
        //
        // Note that lines do not duplicate the edges of faces (unless they do in the input mesh).
        // This routine treats them separately, so depending on the input data lines may or may not be an edge of a face.
        }else if(split.at(0) == "l"_s){
            if(split.size() != 3) continue; // Actually an error...
            try{
                // Read in endpoint vertex indices (i.e., integers).
                // These can be absolute (starting from 1 -- the first vertex) or relative (starting from -1 -- the
                // last vertex).
                const auto x = std::stoll(split.at(1));
                const auto y = std::stoll(split.at(2));
                if((x == 0LL) || (y == 0LL)){
                    FUNCERR("This file is invalid; line indexes should never be zero");
                }

                // Convert to absolute vertices.
                const auto abs_x = (0LL < x) ? x : (static_cast<long long int>(fvsm.vertices.size()) + x + 1LL);
                const auto abs_y = (0LL < y) ? y : (static_cast<long long int>(fvsm.vertices.size()) + y + 1LL);

                // Write as a line segment, duplicating the vertices.
                //fvsm.faces.emplace_back( fvsm.vertices.at(abs_x), fvsm.vertices.at(abs_y) );

                // Record as indices.
                const std::vector<I> faces = {{ static_cast<I>(abs_x - 1LL),
                                                static_cast<I>(abs_y - 1LL) }};
                fvsm.faces.emplace_back( faces );
            }catch(const std::exception &){ 
                continue; 
            }

        // Add a new face.
        }else if(split.at(0) == "f"_s){
            if(split.size() < 4) continue; // Actually an error...
            try{
                std::vector<I> fi;
                for(size_t i = 1; i < split.size(); ++i){
                    // Read in endpoint vertex indices (i.e., integers).
                    // These can be absolute (starting from 1 -- the first vertex) or relative (starting from -1 -- the
                    // last vertex).
                    const auto x = std::stoll(split.at(i));
                    if(x == 0LL){
                        FUNCERR("This file is invalid; face indexes should never be zero");
                    }

                    // Convert to absolute vertices.
                    const auto abs_x = (0LL < x) ? x : (static_cast<long long int>(fvsm.vertices.size()) + x + 1LL);

                    // Record as indices.
                    fi.emplace_back( static_cast<I>(abs_x - 1LL) );
                }
                fvsm.faces.emplace_back(fi);
            }catch(const std::exception &){ 
                continue; 
            }
        }

    }

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
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint32_t> &, std::ifstream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint64_t> &, std::ifstream &);

    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint32_t> &, std::ifstream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint64_t> &, std::ifstream &);

    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint32_t> &, std::stringstream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint64_t> &, std::stringstream &);

    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint32_t> &, std::stringstream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint64_t> &, std::stringstream &);
#endif


// This routine writes an fv_surface_mesh to an OBJ format stream.
//
// Note that metadata is currently not written.
template <class S, class T, class I>
bool
WriteFVSMeshToOBJ(const fv_surface_mesh<T,I> &fvsm,
                  S &ios, // a stream
                  bool use_relative_indexing){

    ios << "# Wavefront OBJ file." << std::endl;

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = ios.precision();
    ios.precision( std::numeric_limits<T>::digits10 + 1 );
    for(const auto &v : fvsm.vertices){
        ios << "v "
            << v.x << " "
            << v.y << " "
            << v.z << '\n';
    }

    // Reset the precision on the stream.
    ios.precision( original_precision );

    // Emit faces.
    const auto N_verts = static_cast<long long int>(fvsm.vertices.size());
    for(const auto &fv : fvsm.faces){
        if(fv.empty()) continue;

        ios << "f";
        for(const auto &v_i : fv){
            const auto v_adj = use_relative_indexing ? static_cast<long long int>(v_i) - N_verts
                                                     : static_cast<long long int>(v_i) + 1LL;
            ios << " " << v_adj;
        }
        ios << "\n";
    }
    ios.flush();

    return(!ios.fail());
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint32_t> &, std::ostream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint64_t> &, std::ostream &, bool);

    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint32_t> &, std::ostream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint64_t> &, std::ostream &, bool);

    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint32_t> &, std::ofstream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint64_t> &, std::ofstream &, bool);

    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint32_t> &, std::ofstream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint64_t> &, std::ofstream &, bool);

    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint32_t> &, std::stringstream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint64_t> &, std::stringstream &, bool);

    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint32_t> &, std::stringstream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint64_t> &, std::stringstream &, bool);
#endif

