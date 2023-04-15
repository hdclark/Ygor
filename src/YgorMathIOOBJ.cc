//YgorMathIOOBJ.cc - Routines for reading and writing simple (ascii) OBJ ("Wavefront Object") files.
//
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorLog.h"
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
template <class T, class I>
bool
ReadFVSMeshFromOBJ(fv_surface_mesh<T,I> &fvsm,
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

    std::string line;
    while(std::getline(is, line)){
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
            if( !( (split.size() == 4) || (split.size() == 5) ) ){
                YLOGWARN("File contains unknown vertex statement -- refusing to parse as surface mesh");
                reset();
                return false;
            }
            try{
                const auto x = std::stod(split.at(1));
                const auto y = std::stod(split.at(2));
                const auto z = std::stod(split.at(3));
                if(split.size() == 5) std::stod(split.at(4)); // Optional weight term. Not supported here.
                fvsm.vertices.emplace_back( static_cast<T>(x), 
                                            static_cast<T>(y),
                                            static_cast<T>(z) );
            }catch(const std::exception &){ 
                //continue;
                YLOGWARN("File contains invalid vertex statement -- refusing to parse as surface mesh");
                reset();
                return false;
            }

        // Add a vertex normal.
        }else if(split[0] == "vn"_s){
            if(split.size() != 4){
                YLOGWARN("File contains unknown vertex normal statement -- refusing to parse as surface mesh");
                reset();
                return false;
            }
            try{
                const auto nx = std::stod(split[1]);
                const auto ny = std::stod(split[2]);
                const auto nz = std::stod(split[3]);
                fvsm.vertex_normals.emplace_back( vec3<T>( static_cast<T>(nx), 
                                                           static_cast<T>(ny),
                                                           static_cast<T>(nz) ).unit() );
            }catch(const std::exception &){ 
                YLOGWARN("File contains invalid vertex normal statement -- refusing to parse as surface mesh");
                reset();
                return false;
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
                    YLOGWARN("This file is invalid; line indexes should never be zero");
                    reset();
                    return false;
                }

                // Convert to absolute vertices.
                const auto abs_x = (0LL < x) ? x : (static_cast<intmax_t>(fvsm.vertices.size()) + x + 1LL);
                const auto abs_y = (0LL < y) ? y : (static_cast<intmax_t>(fvsm.vertices.size()) + y + 1LL);

                // Write as a line segment, duplicating the vertices.
                //fvsm.faces.emplace_back( fvsm.vertices.at(abs_x), fvsm.vertices.at(abs_y) );

                // Record as indices.
                const std::vector<I> faces = {{ static_cast<I>(abs_x - 1LL),
                                                static_cast<I>(abs_y - 1LL) }};
                fvsm.faces.emplace_back( faces );
            }catch(const std::exception &){ 
                YLOGWARN("File contains invalid line (ie edge) statement -- refusing to parse as surface mesh");
                reset();
                return false;
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
                        YLOGWARN("This file is invalid; face indexes should never be zero in OBJ files");
                        reset();
                        return false;
                    }

                    // Convert to absolute vertices.
                    const auto abs_x = (0LL < x) ? x : (static_cast<intmax_t>(fvsm.vertices.size()) + x + 1LL);

                    // Record as indices.
                    fi.emplace_back( static_cast<I>(abs_x - 1LL) );
                }
                fvsm.faces.emplace_back(fi);
            }catch(const std::exception &){ 
                YLOGWARN("File contains invalid face statement -- refusing to parse as surface mesh");
                reset();
                return false;
            }
        }

    }

    // Verify that the indices are reasonable.
    {
        const auto N_verts = fvsm.vertices.size();
        if(N_verts == 0){
            YLOGWARN("No vertices detected -- refusing to parse as surface mesh");
            reset();
            return false;
        }
        for(const auto &fv : fvsm.faces){
            if(fv.empty()){
                YLOGWARN("Encountered face with zero involved vertices");
                reset();
                return false;
            }
            for(const auto &v_i : fv){
                if((v_i < 0) || (N_verts <= v_i)){
                    YLOGWARN("Face references non-existent vertex (" << v_i << ", but N_verts = " << N_verts << ")");
                    reset();
                    return false;
                }
            }
        }
    }

    // Verify that vertex normals are reasonable.
    {
        const bool has_normals = !fvsm.vertex_normals.empty();
        if(has_normals && (fvsm.vertices.size() != fvsm.vertex_normals.size())){
            YLOGWARN("Vertex normals are inconsistent with vertices");
            reset();
            return false;
        }
    }

    // Create the involved_faces index.
    try{
        fvsm.recreate_involved_face_index();
    }catch(const std::exception &){ 
        YLOGWARN("Failed to recreate involved_faces index");
        reset();
        return false;
    }

    return true;
}
#ifndef YGORMATHIOOBJ_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOBJ(fv_surface_mesh<double, uint64_t> &, std::istream &);
#endif


// This routine writes an fv_surface_mesh to an OBJ format stream.
//
// Note that metadata is currently not written.
template <class T, class I>
bool
WriteFVSMeshToOBJ(const fv_surface_mesh<T,I> &fvsm,
                  std::ostream &os,
                  bool use_relative_indexing){

    const bool has_normals = !fvsm.vertex_normals.empty();
    if(has_normals && (fvsm.vertices.size() != fvsm.vertex_normals.size())){
        YLOGWARN("Vertex normals are inconsistent with vertices. Refusing to write point set in OBJ format");
        return false;
    }

    if(fvsm.vertices.empty()){
        YLOGWARN("No vertices detected -- refusing to write surface mesh");
        return false;
    }

    if(!( os << "# Wavefront OBJ file.\n" )){
        YLOGWARN("Unable to write to stream. Cannot continue");
        return false;
    }

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );
    for(const auto &v : fvsm.vertices){
        os << "v "
            << v.x << " "
            << v.y << " "
            << v.z << '\n';
    }
    for(const auto &vn : fvsm.vertex_normals){
        os << "vn "
            << vn.x << " "
            << vn.y << " "
            << vn.z << '\n';
    }

    // Reset the precision on the stream.
    os.precision( original_precision );

    // Emit faces.
    const auto N_verts = static_cast<intmax_t>(fvsm.vertices.size());
    for(const auto &fv : fvsm.faces){
        if(fv.empty()) continue;

        os << "f";
        for(const auto &v_i : fv){
            const auto v_adj = use_relative_indexing ? static_cast<intmax_t>(v_i) - N_verts
                                                     : static_cast<intmax_t>(v_i) + 1LL;
            os << " " << v_adj;
        }
        os << "\n";
    }
    os.flush();

    return(!os.bad());
}
#ifndef YGORMATHIOOBJ_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint32_t> &, std::ostream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<float , uint64_t> &, std::ostream &, bool);

    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint32_t> &, std::ostream &, bool);
    template bool WriteFVSMeshToOBJ(const fv_surface_mesh<double, uint64_t> &, std::ostream &, bool);
#endif



// This routine reads an point_set from an OBJ format stream.
//
// Note that a subset of the full OBJ format is supported; materials, colours, and other mesh attributes are NOT
// supported. Vertices must have three coordinates. Normals are supported and are optional.
//
// Note that the presence of edges/lines or faces will cause the read to fail since the file describes a polyhedron and
// not a point set.
//
template <class T>
bool
ReadPointSetFromOBJ(point_set<T> &ps,
                   std::istream &is ){

    if(!is.good()){
        throw std::runtime_error("unable to read file.");
    }

    const auto reset = [&](){
        ps.points.clear();
        ps.normals.clear();
        ps.colours.clear();
        ps.metadata.clear();
    };
    reset();

    std::string line;
    while(std::getline(is, line)){
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

        if(false){
        }else if(split[0] == "v"_s){ // Vertex.
            if( !( (split.size() == 4) || (split.size() == 5) ) ){
                YLOGWARN("File contains unknown vertex statement -- refusing to parse as point set");
                reset();
                return false;
            }
            try{
                const auto x = std::stod(split[1]);
                const auto y = std::stod(split[2]);
                const auto z = std::stod(split[3]);
                if(split.size() == 5) std::stod(split.at(4)); // Optional weight term. Not supported here.
                ps.points.emplace_back( static_cast<T>(x), 
                                        static_cast<T>(y),
                                        static_cast<T>(z) );
            }catch(const std::exception &){ 
                //continue;
                YLOGWARN("File contains invalid vertex statement -- refusing to parse as point set");
                reset();
                return false;
            }

        }else if(split[0] == "vn"_s){ // Vertex normal.
            if(split.size() != 4){
                YLOGWARN("File contains unknown vertex normal statement -- refusing to parse as point set");
                reset();
                return false;
            }
            try{
                const auto nx = std::stod(split[1]);
                const auto ny = std::stod(split[2]);
                const auto nz = std::stod(split[3]);
                ps.normals.emplace_back( vec3<T>( static_cast<T>(nx), 
                                                  static_cast<T>(ny),
                                                  static_cast<T>(nz) ).unit() );
            }catch(const std::exception &){ 
                //continue; 
                YLOGWARN("File contains invalid vertex normal statement -- refusing to parse as point set");
                reset();
                return false;
            }

        }else if( split[0] == "l"_s ){
            YLOGWARN("File contains an edge -- refusing to parse as point set");
            reset();
            return false;

        }else if( split[0] == "f"_s ){
            YLOGWARN("File contains a face -- refusing to parse as point set");
            reset();
            return false;
        }
    }

    if(ps.points.empty()){
        YLOGWARN("No vertices detected -- refusing to parse as point set");
        reset();
        return false;
    }
    if(!ps.normals.empty() && (ps.points.size() != ps.normals.size())){
        YLOGWARN("Inconsistent number of vertices and normals -- refusing to parse as point set");
        reset();
        return false;
    }

    return true;
}
#ifndef YGORMATHIOOBJ_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadPointSetFromOBJ(point_set<float > &, std::istream &);
    template bool ReadPointSetFromOBJ(point_set<double> &, std::istream &);
#endif


// This routine writes an point_set to an OBJ format stream.
//
// Note that metadata is currently not written.
template <class T>
bool
WritePointSetToOBJ(const point_set<T> &ps,
                  std::ostream &os){

    if(ps.points.empty()){
        YLOGWARN("No vertices present. Refusing to write point set in OBJ format");
        return false;
    }
    const bool has_normals = !ps.normals.empty();
    if(has_normals && (ps.points.size() != ps.normals.size())){
        YLOGWARN("Normals are inconsistent with vertices. Refusing to write point set in OBJ format");
        return false;
    }

    os << "# Wavefront OBJ file." << std::endl;

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );
    for(const auto &v : ps.points){
        os << "v "
           << v.x << " "
           << v.y << " "
           << v.z << '\n';
    }
    for(const auto &vn : ps.normals){
        os << "vn "
           << vn.x << " "
           << vn.y << " "
           << vn.z << '\n';
    }

    // Reset the precision on the stream.
    os.precision( original_precision );
    os.flush();

    return(!os.bad());
}
#ifndef YGORMATHIOOBJ_DISABLE_ALL_SPECIALIZATIONS
    template bool WritePointSetToOBJ(const point_set<float > &, std::ostream &);
    template bool WritePointSetToOBJ(const point_set<double> &, std::ostream &);
#endif

