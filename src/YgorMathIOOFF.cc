//YgorMathIOOFF.cc - Routines for reading and writing ASCII OFF ("Object File Format") files.
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

#include "YgorMathIOOFF.h"


// This routine writes a collection of points (or vertices) without any connectivity between them.
//
// NOTE: comments should not have newlines. This may be enforced in the future.
//
template <class T>
bool
WritePointsToOFF(std::vector<vec3<T>> points,
                 const std::string &filename,
                 const std::string &comment){
    
    //Check if the file exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    std::ofstream FO(filename, std::ios::out);
    if(!FO) return false;

    FO << "OFF" << std::endl;
    FO << std::endl;
    FO << "#" << comment << std::endl;
    FO << std::endl;

    FO << points.size() << " 0 0" << std::endl; //Number of vertices, faces, edges.  

    // Maximize precision of the vertices.
    const auto original_precision = FO.precision();
    FO.precision( std::numeric_limits<T>::max_digits10 );

    //Vertices.
    for(const auto &p : points){
        FO << p.x << " " << p.y << " " << p.z << std::endl;
    }
    FO << std::endl;

    // Reset the precision of the stream.
    FO.precision( original_precision );

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifndef YGORMATHIOOFF_DISABLE_ALL_SPECIALIZATIONS
    template bool WritePointsToOFF(std::vector<vec3<float >>, const std::string &, const std::string &);
    template bool WritePointsToOFF(std::vector<vec3<double>>, const std::string &, const std::string &);
#endif


// This routine writes a line_segment in OFF format.
//
// NOTE: This routine writes a line_segment as a triangle with zero area rather than a single edge connecting 
//       two vertices for compatibility purposes.
//
// NOTE: Comments should not have newlines. This may be enforced in the future.
//
template <class T>
bool
WriteLineSegmentToOFF(line_segment<T> ls,
                      const std::string &filename,
                      const std::string &comment){

    //Check if the file exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    std::ofstream FO(filename, std::ios::out);
    if(!FO) return false;

    FO << "OFF" << std::endl;
    FO << std::endl;
    FO << "# Note: this file contains a zero-area triangle that represents a line segment." << std::endl;
    FO << "#" << comment << std::endl;
    FO << std::endl;

    FO << "3 1 0" << std::endl; //Number of vertices, faces, edges (edges are ignorable).

    // Maximize precision of the vertices.
    const auto original_precision = FO.precision();
    FO.precision( std::numeric_limits<T>::max_digits10 );

    //Vertices.
    const auto R0 = ls.Get_R0();
    const auto R1 = ls.Get_R1();
    const auto R2 = (R0 + R1)*static_cast<T>(0.5); //The midpoint, to complete the triangle.
    FO << R0.x << " " << R0.y << " " << R0.z << std::endl; // "v0".
    FO << R1.x << " " << R1.y << " " << R1.z << std::endl; // "v1".
    FO << R2.x << " " << R2.y << " " << R2.z << std::endl; // "v2".
    FO << std::endl;

    // Reset the precision of the stream.
    FO.precision( original_precision );

    //Faces. (Only a single zero-area face in this case.)
    FO << "3 0 1 2" << std::endl; //Triangle with three vertices: (v0,v1,v2).
    FO << std::endl;

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifndef YGORMATHIOOFF_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteLineSegmentToOFF(line_segment<float >, const std::string &, const std::string &);
    template bool WriteLineSegmentToOFF(line_segment<double>, const std::string &, const std::string &);
#endif


// This routine reads an fv_surface_mesh from an OFF format stream.
//
// Note that OFF files can contain lines, unconnected vertices, and other non-polyhedron (non-manifold) elements.
// This routine does not validate or enforce manifoldness.
//
// Note that a subset of the full OFF format is supported; binary files, materials, colours, and other mesh attributes
// are NOT supported. Vertices must have three coordinates. Faces can comprise [1-inf) number of vertices.
template <class T, class I>
bool
ReadFVSMeshFromOFF(fv_surface_mesh<T,I> &fvsm,
                   std::istream &is ){

    bool dims_known = false;
    long long int N_verts = -1;
    long long int N_faces = -1;

    fvsm.vertices.clear();
    fvsm.faces.clear();

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

        // If the number of verts and faces are not yet known, seek this info before reading any other information.
        if(!dims_known){
            // The first line might contain merely "OFF", but it is not required. So we ignore anything that is not what we
            // need at a given moment. Note that out-of-order files will thus not be parsed correctly!
            if(split.size() != 3) continue;

            try{
                const auto v = std::stoll(split.at(0));
                const auto f = std::stoll(split.at(1));
                const auto e = std::stoll(split.at(2));
                // Allow #_of_edges to be anything for now, but still validate it is a number.
                if((v <= 0) || (f < 0) || (e < 0)){
                    continue;
                }
                N_verts = v;
                N_faces = f;
                dims_known = true;
                continue;

            }catch(const std::exception &){ 
                continue; 
            }

        // If dimensions are known, then try reading until we fulfill the vert/face/edge dimensions.
        }else{
            // Fill up the vertices first.
            if(static_cast<long long int>(fvsm.vertices.size()) != N_verts){
                if(split.size() != 3) continue; // Actually an error...
                try{
                    const auto x = std::stod(split.at(0));
                    const auto y = std::stod(split.at(1));
                    const auto z = std::stod(split.at(2));
                    fvsm.vertices.emplace_back( static_cast<T>(x), 
                                                static_cast<T>(y),
                                                static_cast<T>(z) );
                }catch(const std::exception &){ 
                    continue; 
                }
                continue;

            // Then fill up the faces.
            //
            // Note that faces are zero-indexed.
            }else if(static_cast<long long int>(fvsm.faces.size()) != N_faces){
                if(split.size() < 1) continue; // Actually an error...
                try{
                    const auto N = static_cast<long long int>(split.size());
                    const auto n = std::stoll(split.at(0)); // Number of verts for this face.
                    if(N != (n+1)) continue;
                    std::vector<I> shtl;
                    for(long int i = 1; i < N; ++i){
                        const auto x = std::stoll(split.at(i));
                        shtl.emplace_back( static_cast<I>(x) );
                    }

                    fvsm.faces.emplace_back(shtl);
                }catch(const std::exception &){ 
                    continue; 
                }
                continue;

            // Then fill up edges.
            }else{
                // TODO -- not needed at time of writing, and appear to rarely be used in the wild ...
            }

        } // Filling up verts and faces.
    } // While loop over lines in the file.
    //FUNCINFO("Read " << verts.size() << " of " << N_verts << " vertices and " << faces.size() << " of " << N_faces << " faces");

    // Verify the file was consistent.
    if( !dims_known ){
        FUNCWARN("Dimensions could not be determined");
        return false;
    }
    if( (static_cast<long long int>(fvsm.vertices.size()) != N_verts) ){
        FUNCWARN("Read " << static_cast<long long int>(fvsm.vertices.size()) << " vertices (should be " << N_verts << ")");
        return false;
    }
    if( (static_cast<long long int>(fvsm.faces.size()) != N_faces) ){
        FUNCWARN("Read " << static_cast<long long int>(fvsm.faces.size()) << " faces (should be " << N_faces << ")");
        return false;
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
#ifndef YGORMATHIOOFF_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromOFF(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<double, uint64_t> &, std::istream &);
#endif


// This routine writes an fv_surface_mesh to an OFF format stream.
//
// Note that metadata is currently not written.
template <class T, class I>
bool
WriteFVSMeshToOFF(const fv_surface_mesh<T,I> &fvsm,
                  std::ostream &os ){

    os << "OFF" << std::endl;
    os << fvsm.vertices.size() << " "
        << fvsm.faces.size() << " "
        << "0" << std::endl; // Number of edges.

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );
    for(const auto &v : fvsm.vertices){
        os << v.x << " "
            << v.y << " "
            << v.z << '\n';
    }
    // Reset the precision on the stream.
    os.precision( original_precision );

    // Emit faces.
    for(const auto &fv : fvsm.faces){
        if(fv.empty()) continue;

        os << fv.size(); // Preface lines with the number of vertices connected by this face.
        for(const auto &v_i : fv){
            os << " " << v_i;
        }
        os << '\n';
    }
    os.flush();

    return(!os.fail());
}
#ifndef YGORMATHIOOFF_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<float , uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<float , uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToOFF(const fv_surface_mesh<double, uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<double, uint64_t> &, std::ostream &);
#endif



// This routine reads an point_set from an OFF format stream.
//
// Note that OFF files can contain many irrelevant elements.
// This routine will extract only vertices and (optionally) normals.
//
// Note that a subset of the full OFF format is supported; binary files, materials, colours, and other mesh attributes
// are NOT supported. Vertices must have three coordinates.
template <class T>
bool
ReadPointSetFromOFF(point_set<T> &ps,
                      std::istream &is ){

    if(!is.good()){
        throw std::runtime_error("unable to read file.");
    }

    const auto reset = [&](){
        ps.points.clear();
        ps.normals.clear();
        ps.metadata.clear();
    };
    reset();

    bool dims_known = false;
    long long int N_verts = -1;

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

        // If the number of verts and faces are not yet known, seek this info before reading any other information.
        if(!dims_known){
            // The first line might contain merely "OFF", but it is not required. So we ignore anything that is not what we
            // need at a given moment. Note that out-of-order files will thus not be parsed correctly!
            if( !( (split.size() == 3) || (split.size() == 6) ) ) continue;

            try{
                const auto v = std::stoll(split.at(0));
                const auto f = std::stoll(split.at(1));
                const auto e = std::stoll(split.at(2));
                // Allow #_of_edges to be anything for now, but still validate it is a number.
                if((v <= 0) || (f < 0) || (e < 0)){
                    continue;
                }
                N_verts = v;
                dims_known = true;

                if(f != 0){
                    FUNCWARN("File contains a face -- refusing to parse as point set");
                    reset();
                    return false;
                }
                if(e != 0){
                    FUNCWARN("File contains an edge -- refusing to parse as point set");
                    reset();
                    return false;
                }
                continue;

            }catch(const std::exception &){ 
                //continue; 
                FUNCWARN("File contains invalid 'VFE' statement -- refusing to parse as point set");
                reset();
                return false;
            }

        // If dimensions are known, then try reading until we fulfill the vert count.
        }else{
            // Fill up vertices.
            if(static_cast<long long int>(ps.points.size()) != N_verts){
                if( !( (split.size() == 3) || (split.size() == 6) ) ) continue;
                try{
                    const auto x = std::stod(split[0]);
                    const auto y = std::stod(split[1]);
                    const auto z = std::stod(split[2]);
                    ps.points.emplace_back( static_cast<T>(x), 
                                            static_cast<T>(y),
                                            static_cast<T>(z) );
                    if(split.size() == 6){
                        const auto nx = std::stod(split[3]);
                        const auto ny = std::stod(split[4]);
                        const auto nz = std::stod(split[5]);
                        ps.normals.emplace_back( vec3<T>( static_cast<T>(nx), 
                                                          static_cast<T>(ny),
                                                          static_cast<T>(nz) ).unit() );
                    }
                }catch(const std::exception &){ 
                    //continue;
                    FUNCWARN("File contains invalid vertex statement -- refusing to parse as point set");
                    reset();
                    return false;
                }
                continue;

            // Then fill up edges.
            }else{
                // TODO -- not needed at time of writing, and appear to rarely be used in the wild ...
            }

        } // Filling up verts.
    } // While loop over lines in the file.

    // Verify the file was consistent.
    if( !dims_known ){
        FUNCWARN("Dimensions could not be determined");
        reset();
        return false;
    }
    if( (static_cast<long long int>(ps.points.size()) != N_verts) ){
        FUNCWARN("Read " << static_cast<long long int>(ps.points.size()) << " vertices (should be " << N_verts << ")");
        reset();
        return false;
    }
    if( !(ps.normals.empty()) 
    &&  (static_cast<long long int>(ps.normals.size()) != N_verts) ){
        FUNCWARN("Read " << static_cast<long long int>(ps.normals.size()) << " normals (should be 0 or " << N_verts << ")");
        reset();
        return false;
    }

    return true;
}
#ifndef YGORMATHIOOFF_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadPointSetFromOFF(point_set<float > &, std::istream &);
    template bool ReadPointSetFromOFF(point_set<double> &, std::istream &);
#endif


// This routine writes an point_set to an OFF format stream.
//
// Note that metadata is currently not written.
template <class T>
bool
WritePointSetToOFF(const point_set<T> &ps,
                     std::ostream &os ){

    if(ps.points.empty()){
        FUNCWARN("No vertices present. Refusing to write point set in OFF format");
        return false;
    }
    const bool has_normals = !ps.normals.empty();
    if(has_normals && (ps.points.size() != ps.normals.size())){
        FUNCWARN("Normals are inconsistent with vertices. Refusing to write point set in OFF format");
        return false;
    }

    os << "OFF" << std::endl;
    os << ps.points.size() << " "
       << "0 "              // Number of faces.
       << "0" << std::endl; // Number of edges.

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );

    if(!has_normals){
        for(const auto &p : ps.points){
            os << p.x << " "
               << p.y << " "
               << p.z << '\n';
        }
    }else{
        auto n_it = std::begin(ps.normals);
        const auto v_end = std::end(ps.points);
        const auto n_end = std::end(ps.normals);
        for(auto v_it = std::begin(ps.points); (v_it != v_end) && (n_it != n_end); ++v_it, ++n_it){
            os << v_it->x << " "
               << v_it->y << " "
               << v_it->z << " "
               << n_it->x << " "
               << n_it->y << " "
               << n_it->z << '\n';
        }
    }
    // Reset the precision on the stream.
    os.precision( original_precision );
    os.flush();

    return(!os.fail());
}
#ifndef YGORMATHIOOFF_DISABLE_ALL_SPECIALIZATIONS
    template bool WritePointSetToOFF(const point_set<float > &, std::ostream &);
    template bool WritePointSetToOFF(const point_set<double> &, std::ostream &);
#endif

