// Test of convex hull routine.

#include <limits>
#include <utility>
#include <iostream>
#include <random>
#include <cstdint>

#include <YgorMath.h>
#include <YgorMathIOPLY.h>
#include <YgorMisc.h>
#include "YgorLog.h"


int main(int argc, char **argv){

    int64_t random_seed = 123456;
    std::mt19937 re( random_seed );
    std::uniform_real_distribution<> rd(0.0, 1.0); //Random distribution.

    std::vector<vec3<double>> all_verts;
    for(size_t i = 0; i < 50000; ++i){
        all_verts.emplace_back(vec3<double>(rd(re), rd(re), rd(re)));
    }

    // Shuffle order, so first vertices are unlikely to be present in the final hull.
    std::shuffle(std::begin(all_verts), std::end(all_verts), re);

    using vert_vec_t = decltype(std::begin(all_verts));
    auto faces = Convex_Hull_3<vert_vec_t,uint32_t>(std::begin(all_verts), std::end(all_verts));

    // Validate convexity: check that all vertices are on or inside all face planes
    // For a truly convex hull, every vertex should be on the same side (or on) each face plane
    YLOGINFO("Validating convexity of hull with " << faces.size() << " faces and " << all_verts.size() << " vertices");
    
    size_t violations = 0;
    const double eps = 1e-6; // Small tolerance for numerical errors
    
    for(const auto& face : faces){
        if(face.size() != 3){
            YLOGERR("Face does not have 3 vertices");
            return 1;
        }
        
        const auto& v_A = all_verts[face[0]];
        const auto& v_B = all_verts[face[1]];
        const auto& v_C = all_verts[face[2]];
        
        // Compute face normal (pointing outward for convex hull)
        const auto face_normal = (v_B - v_A).Cross(v_C - v_A);
        
        // Check all vertices against this face plane
        for(size_t i = 0; i < all_verts.size(); ++i){
            const auto& v = all_verts[i];
            const auto offset = face_normal.Dot(v - v_A);
            
            // For a convex hull, all vertices should be on or behind the face
            // (i.e., offset <= small_epsilon)
            if(offset > eps){
                violations++;
                if(violations <= 10){ // Only log first few violations
                    YLOGWARN("Vertex " << i << " is outside face plane (offset = " << offset << ")");
                }
            }
        }
    }
    
    if(violations > 0){
        YLOGERR("Convexity validation FAILED: " << violations << " violations found");
        return 1;
    }
    
    YLOGINFO("Convexity validation PASSED: hull is convex");

    // Dump the mesh for inspection.
    fv_surface_mesh<double, uint32_t> mesh;
    mesh.vertices = all_verts;
    mesh.faces = faces;
    std::ofstream of("convex_hull.ply");
    if(!WriteFVSMeshToPLY(mesh, of)){
        throw std::runtime_error("Unable to write file.");
    }

    return 0;
}


