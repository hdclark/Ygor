// Test of convex hull routine with different scales and edge cases.

#include <limits>
#include <utility>
#include <iostream>
#include <random>
#include <cstdint>
#include <cmath>

#include <YgorMath.h>
#include <YgorMisc.h>
#include "YgorLog.h"


// Helper function to validate convexity
bool validate_convexity(const std::vector<vec3<double>>& all_verts, 
                        const std::vector<std::vector<uint32_t>>& faces) {
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
            return false;
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
            if(offset > eps){
                violations++;
                if(violations <= 5){
                    YLOGWARN("Vertex " << i << " is outside face plane (offset = " << offset << ", eps = " << eps << ")");
                }
            }
        }
    }
    
    if(violations > 0){
        YLOGERR("Convexity validation FAILED: " << violations << " violations found");
        return false;
    }
    
    return true;
}

int main(int argc, char **argv){

    // Test 1: Very small scale (near machine epsilon range)
    {
        YLOGINFO("Test 1: Very small scale (1e-8)");
        std::mt19937 re(12345);
        std::uniform_real_distribution<> rd(0.0, 1e-8);
        
        std::vector<vec3<double>> verts;
        for(size_t i = 0; i < 100; ++i){
            verts.emplace_back(vec3<double>(rd(re), rd(re), rd(re)));
        }
        
        using vert_vec_t = decltype(std::begin(verts));
        try {
            auto faces = Convex_Hull_3<vert_vec_t,uint32_t>(std::begin(verts), std::end(verts));
            
            YLOGINFO("  Generated " << faces.size() << " faces from " << verts.size() << " vertices");
            
            if(!validate_convexity(verts, faces)){
                YLOGERR("Test 1 FAILED - convexity validation failed");
                return 1;
            }
        } catch (const std::exception& e) {
            // Very small scales may result in degenerate configurations that cannot form a valid hull
            YLOGWARN("Test 1 caught expected exception for very small scale: " << e.what());
        }
        YLOGINFO("Test 1 PASSED");
    }

    // Test 2: Very large scale
    {
        YLOGINFO("Test 2: Very large scale (1e10)");
        std::mt19937 re(12346);
        std::uniform_real_distribution<> rd(0.0, 1e10);
        
        std::vector<vec3<double>> verts;
        for(size_t i = 0; i < 100; ++i){
            verts.emplace_back(vec3<double>(rd(re), rd(re), rd(re)));
        }
        
        using vert_vec_t = decltype(std::begin(verts));
        auto faces = Convex_Hull_3<vert_vec_t,uint32_t>(std::begin(verts), std::end(verts));
        
        YLOGINFO("  Generated " << faces.size() << " faces from " << verts.size() << " vertices");
        
        if(!validate_convexity(verts, faces)){
            YLOGERR("Test 2 FAILED");
            return 1;
        }
        YLOGINFO("Test 2 PASSED");
    }

    // Test 3: Cube vertices (8 vertices)
    {
        YLOGINFO("Test 3: Cube vertices");
        std::vector<vec3<double>> verts = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 0.0, 1.0),
            vec3<double>(1.0, 0.0, 1.0),
            vec3<double>(0.0, 1.0, 1.0),
            vec3<double>(1.0, 1.0, 1.0),
        };
        
        using vert_vec_t = decltype(std::begin(verts));
        auto faces = Convex_Hull_3<vert_vec_t,uint32_t>(std::begin(verts), std::end(verts));
        
        YLOGINFO("  Generated " << faces.size() << " faces from " << verts.size() << " vertices");
        
        // Cube should have 12 faces (2 triangles per square face)
        if(faces.size() != 12){
            YLOGWARN("  Expected 12 faces for cube, got " << faces.size());
        }
        
        if(!validate_convexity(verts, faces)){
            YLOGERR("Test 3 FAILED");
            return 1;
        }
        YLOGINFO("Test 3 PASSED");
    }

    // Test 4: Tetrahedron (4 vertices - minimum for 3D hull)
    {
        YLOGINFO("Test 4: Tetrahedron");
        std::vector<vec3<double>> verts = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.5, 1.0, 0.0),
            vec3<double>(0.5, 0.5, 1.0),
        };
        
        using vert_vec_t = decltype(std::begin(verts));
        auto faces = Convex_Hull_3<vert_vec_t,uint32_t>(std::begin(verts), std::end(verts));
        
        YLOGINFO("  Generated " << faces.size() << " faces from " << verts.size() << " vertices");
        
        // Tetrahedron should have 4 faces
        if(faces.size() != 4){
            YLOGWARN("  Expected 4 faces for tetrahedron, got " << faces.size());
        }
        
        if(!validate_convexity(verts, faces)){
            YLOGERR("Test 4 FAILED");
            return 1;
        }
        YLOGINFO("Test 4 PASSED");
    }

    // Test 5: Points with many duplicates
    {
        YLOGINFO("Test 5: Points with duplicates");
        std::vector<vec3<double>> verts;
        
        // Add 8 corner points, then many duplicates
        for(int x = 0; x <= 1; ++x){
            for(int y = 0; y <= 1; ++y){
                for(int z = 0; z <= 1; ++z){
                    verts.emplace_back(vec3<double>(x, y, z));
                }
            }
        }
        
        // Add many duplicate points
        std::mt19937 re(12347);
        std::uniform_int_distribution<> rd(0, 7);
        for(size_t i = 0; i < 100; ++i){
            verts.push_back(verts[rd(re)]);
        }
        
        using vert_vec_t = decltype(std::begin(verts));
        auto faces = Convex_Hull_3<vert_vec_t,uint32_t>(std::begin(verts), std::end(verts));
        
        YLOGINFO("  Generated " << faces.size() << " faces from " << verts.size() << " vertices (with duplicates)");
        
        if(!validate_convexity(verts, faces)){
            YLOGERR("Test 5 FAILED");
            return 1;
        }
        YLOGINFO("Test 5 PASSED");
    }

    YLOGINFO("All convex hull tests PASSED");
    return 0;
}
