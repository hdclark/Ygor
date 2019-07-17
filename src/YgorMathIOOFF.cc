//YgorMathIOOFF.cc - Routines for reading and writing ASCII OFF ("Object File Format") files.
//
#include <iostream>
#include <sstream>
#include <vector>

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
    FO.precision( std::numeric_limits<T>::digits10 + 1 );

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
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
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
    FO.precision( std::numeric_limits<T>::digits10 + 1 );

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
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
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
template <class S, class T, class I>
bool
ReadFVSMeshFromOFF(fv_surface_mesh<T,I> &fvsm,
                   S &ios ){

    bool dims_known = false;
    long long int N_verts = -1;
    long long int N_faces = -1;

    fvsm.vertices.clear();
    fvsm.faces.clear();

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
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<float , uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<float , uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromOFF(fv_surface_mesh<double, uint32_t> &, std::istream &);
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<double, uint64_t> &, std::istream &);

    template bool ReadFVSMeshFromOFF(fv_surface_mesh<float , uint32_t> &, std::stringstream &);
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<float , uint64_t> &, std::stringstream &);

    template bool ReadFVSMeshFromOFF(fv_surface_mesh<double, uint32_t> &, std::stringstream &);
    template bool ReadFVSMeshFromOFF(fv_surface_mesh<double, uint64_t> &, std::stringstream &);
#endif


// This routine writes an fv_surface_mesh to an OFF format stream.
//
// Note that metadata is currently not written.
template <class S, class T, class I>
bool
WriteFVSMeshToOFF(const fv_surface_mesh<T,I> &fvsm,
                  S &ios ){ // a stream.

    ios << "OFF" << std::endl;
    ios << fvsm.vertices.size() << " "
        << fvsm.faces.size() << " "
        << "0" << std::endl; // Number of edges.

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = ios.precision();
    ios.precision( std::numeric_limits<T>::digits10 + 1 );
    for(const auto &v : fvsm.vertices){
        ios << v.x << " "
            << v.y << " "
            << v.z << '\n';
    }
    // Reset the precision on the stream.
    ios.precision( original_precision );

    // Emit faces.
    for(const auto &fv : fvsm.faces){
        if(fv.empty()) continue;

        ios << fv.size(); // Preface lines with the number of vertices connected by this face.
        for(const auto &v_i : fv){
            ios << " " << v_i;
        }
        ios << '\n';
    }
    ios.flush();

    return(!ios.fail());
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<float , uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<float , uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToOFF(const fv_surface_mesh<double, uint32_t> &, std::ostream &);
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<double, uint64_t> &, std::ostream &);

    template bool WriteFVSMeshToOFF(const fv_surface_mesh<float , uint32_t> &, std::stringstream &);
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<float , uint64_t> &, std::stringstream &);

    template bool WriteFVSMeshToOFF(const fv_surface_mesh<double, uint32_t> &, std::stringstream &);
    template bool WriteFVSMeshToOFF(const fv_surface_mesh<double, uint64_t> &, std::stringstream &);
#endif
