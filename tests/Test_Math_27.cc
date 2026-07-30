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
    
    // Compute scale-aware epsilon based on bounding box
    vec3<double> bbox_min(std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity());
    vec3<double> bbox_max(-std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity());
    
    for(const auto& v : all_verts){
        if(v.isfinite()){
            bbox_min.x = std::min(bbox_min.x, v.x);
            bbox_min.y = std::min(bbox_min.y, v.y);
            bbox_min.z = std::min(bbox_min.z, v.z);
            bbox_max.x = std::max(bbox_max.x, v.x);
            bbox_max.y = std::max(bbox_max.y, v.y);
            bbox_max.z = std::max(bbox_max.z, v.z);
        }
    }
    
    const auto bbox_diag = bbox_max - bbox_min;
    const auto bbox_size = bbox_diag.length();
    const auto rel_eps = std::sqrt(std::numeric_limits<double>::epsilon());
    const auto abs_eps = 100.0 * std::numeric_limits<double>::epsilon();
    const double eps = (bbox_size > abs_eps) ? (bbox_size * rel_eps) : abs_eps;
    
    size_t violations = 0;
    
    for(const auto& face : faces){
        if(face.size() != 3){
            YLOGERR("Face does not have 3 vertices");
            return 1;
        }
        
        const auto& v_A = all_verts[face[0]];
        const auto& v_B = all_verts[face[1]];
        const auto& v_C = all_verts[face[2]];
        
        // Compute normalized face normal to make offset independent of triangle size
        const auto face_normal_unnorm = (v_B - v_A).Cross(v_C - v_A);
        const auto face_normal = face_normal_unnorm.unit();
        
        if(!face_normal.isfinite()) continue;
        
        // Check all vertices against this face plane
        for(size_t i = 0; i < all_verts.size(); ++i){
            const auto& v = all_verts[i];
            const auto offset = face_normal.Dot(v - v_A);
            
            // For a convex hull, all vertices should be on or behind the face
            // (i.e., offset <= eps)
            if(offset > eps){
                violations++;
                if(violations <= 10){ // Only log first few violations
                    YLOGWARN("Vertex " << i << " is outside face plane (offset = " << offset << ", eps = " << eps << ")");
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


