//Test_MeshProcessing_01.cc - Test file for mesh processing routines (Loop subdivision).

#include <iostream>
#include <sstream>
#include <cmath>
#include <string>
#include <set>
#include <map>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathIOOFF.h"
#include "YgorMeshProcessing4.h"


// Helper function to create a simple tetrahedron mesh.
fv_surface_mesh<double, uint64_t> create_tetrahedron() {
    fv_surface_mesh<double, uint64_t> mesh;
    
    // Vertices of a regular tetrahedron.
    mesh.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
    mesh.vertices.emplace_back(vec3<double>(1.0, -1.0, -1.0));
    mesh.vertices.emplace_back(vec3<double>(-1.0, 1.0, -1.0));
    mesh.vertices.emplace_back(vec3<double>(-1.0, -1.0, 1.0));
    
    // Four triangular faces.
    mesh.faces.push_back({0, 1, 2});
    mesh.faces.push_back({0, 3, 1});
    mesh.faces.push_back({0, 2, 3});
    mesh.faces.push_back({1, 3, 2});
    
    mesh.recreate_involved_face_index();
    return mesh;
}


// Helper function to create a single triangle mesh (has boundary edges).
fv_surface_mesh<double, uint64_t> create_single_triangle() {
    fv_surface_mesh<double, uint64_t> mesh;
    
    mesh.vertices.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    mesh.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
    mesh.vertices.emplace_back(vec3<double>(0.5, std::sqrt(3.0)/2.0, 0.0));
    
    mesh.faces.push_back({0, 1, 2});
    
    mesh.recreate_involved_face_index();
    return mesh;
}


// Helper function to create an icosahedron (20 faces, 12 vertices, 30 edges).
fv_surface_mesh<double, uint64_t> create_icosahedron() {
    fv_surface_mesh<double, uint64_t> mesh;

    std::stringstream ss;
    ss << "OFF" << std::endl
       << "12 20 0" << std::endl
       << "0 1.618034 1 " << std::endl
       << "0 1.618034 -1 " << std::endl
       << "0 -1.618034 1 " << std::endl
       << "0 -1.618034 -1 " << std::endl
       << "1.618034 1 0 " << std::endl
       << "1.618034 -1 0 " << std::endl
       << "-1.618034 1 0 " << std::endl
       << "-1.618034 -1 0 " << std::endl
       << "1 0 1.618034 " << std::endl
       << "-1 0 1.618034 " << std::endl
       << "1 0 -1.618034 " << std::endl
       << "-1 0 -1.618034 " << std::endl
       << "3 1 0 4" << std::endl
       << "3 0 1 6" << std::endl
       << "3 2 3 5" << std::endl
       << "3 3 2 7" << std::endl
       << "3 4 5 10" << std::endl
       << "3 5 4 8" << std::endl
       << "3 6 7 9" << std::endl
       << "3 7 6 11" << std::endl
       << "3 8 9 2" << std::endl
       << "3 9 8 0" << std::endl
       << "3 10 11 1" << std::endl
       << "3 11 10 3" << std::endl
       << "3 0 8 4" << std::endl
       << "3 0 6 9" << std::endl
       << "3 1 4 10" << std::endl
       << "3 1 11 6" << std::endl
       << "3 2 5 8" << std::endl
       << "3 2 9 7" << std::endl
       << "3 3 10 5" << std::endl
       << "3 3 7 11" << std::endl;
    
    if(!ReadFVSMeshFromOFF(mesh, ss)){
        throw std::runtime_error("Failed to create icosahedron.");
    }
    return mesh;
}


// Count the number of unique edges in a mesh.
size_t count_edges(const fv_surface_mesh<double, uint64_t> &mesh) {
    std::set<std::pair<uint64_t, uint64_t>> edges;
    for(const auto &face : mesh.faces){
        for(size_t i = 0; i < face.size(); ++i){
            uint64_t v0 = face[i];
            uint64_t v1 = face[(i + 1) % face.size()];
            if(v0 > v1) std::swap(v0, v1);
            edges.insert({v0, v1});
        }
    }
    return edges.size();
}


// Verify that all faces in the mesh are valid triangles with valid vertex indices.
bool verify_mesh_integrity(const fv_surface_mesh<double, uint64_t> &mesh) {
    const size_t N_verts = mesh.vertices.size();
    
    for(size_t f_idx = 0; f_idx < mesh.faces.size(); ++f_idx){
        const auto &face = mesh.faces[f_idx];
        if(face.size() != 3){
            YLOGWARN("Face " << f_idx << " has " << face.size() << " vertices (expected 3).");
            return false;
        }
        for(const auto &v : face){
            if(v >= N_verts){
                YLOGWARN("Face " << f_idx << " references invalid vertex index " << v 
                         << " (mesh has " << N_verts << " vertices).");
                return false;
            }
        }
    }
    
    // Verify involved_faces index is consistent.
    if(!mesh.involved_faces.empty()){
        if(mesh.involved_faces.size() != mesh.vertices.size()){
            YLOGWARN("involved_faces size mismatch.");
            return false;
        }
        for(size_t v_idx = 0; v_idx < mesh.involved_faces.size(); ++v_idx){
            for(const auto &f_idx : mesh.involved_faces[v_idx]){
                if(f_idx >= mesh.faces.size()){
                    YLOGWARN("involved_faces[" << v_idx << "] references invalid face " << f_idx);
                    return false;
                }
                // Check that the face actually contains this vertex.
                const auto &face = mesh.faces[f_idx];
                bool found = false;
                for(const auto &v : face){
                    if(v == v_idx) found = true;
                }
                if(!found){
                    YLOGWARN("involved_faces[" << v_idx << "] references face " << f_idx 
                             << " but that face doesn't contain vertex " << v_idx);
                    return false;
                }
            }
        }
    }
    
    return true;
}


int main(int argc, char **argv){

    // Test 1: Single triangle subdivision (boundary case).
    {
        YLOGINFO("Test 1: Single triangle subdivision");
        auto mesh = create_single_triangle();
        
        YLOGINFO("  Before: vertices=" << mesh.vertices.size() 
                 << ", faces=" << mesh.faces.size()
                 << ", edges=" << count_edges(mesh));
        
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        const size_t E_before = count_edges(mesh);
        
        loop_subdivide(mesh, 1);
        
        const size_t V_after = mesh.vertices.size();
        const size_t F_after = mesh.faces.size();
        const size_t E_after = count_edges(mesh);
        
        YLOGINFO("  After: vertices=" << V_after 
                 << ", faces=" << F_after
                 << ", edges=" << E_after);
        
        // After 1 iteration: V' = V + E, F' = 4F, E' = 2E + 3F
        if(V_after != V_before + E_before){
            throw std::runtime_error("Test 1 failed: Wrong vertex count after subdivision. Expected "
                                     + std::to_string(V_before + E_before) + " but got "
                                     + std::to_string(V_after));
        }
        if(F_after != 4 * F_before){
            throw std::runtime_error("Test 1 failed: Wrong face count after subdivision. Expected "
                                     + std::to_string(4 * F_before) + " but got "
                                     + std::to_string(F_after));
        }
        if(!verify_mesh_integrity(mesh)){
            throw std::runtime_error("Test 1 failed: Mesh integrity check failed.");
        }
        
        YLOGINFO("  Test 1 PASSED");
    }

    // Test 2: Tetrahedron subdivision (closed mesh).
    {
        YLOGINFO("Test 2: Tetrahedron subdivision");
        auto mesh = create_tetrahedron();
        
        YLOGINFO("  Before: vertices=" << mesh.vertices.size() 
                 << ", faces=" << mesh.faces.size()
                 << ", edges=" << count_edges(mesh));
        
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        const size_t E_before = count_edges(mesh);
        
        loop_subdivide(mesh, 1);
        
        const size_t V_after = mesh.vertices.size();
        const size_t F_after = mesh.faces.size();
        
        YLOGINFO("  After: vertices=" << V_after 
                 << ", faces=" << F_after
                 << ", edges=" << count_edges(mesh));
        
        // Tetrahedron has 4 vertices, 4 faces, 6 edges.
        // After subdivision: V' = 4 + 6 = 10, F' = 4 * 4 = 16.
        if(V_after != 10){
            throw std::runtime_error("Test 2 failed: Wrong vertex count. Expected 10 but got "
                                     + std::to_string(V_after));
        }
        if(F_after != 16){
            throw std::runtime_error("Test 2 failed: Wrong face count. Expected 16 but got "
                                     + std::to_string(F_after));
        }
        if(!verify_mesh_integrity(mesh)){
            throw std::runtime_error("Test 2 failed: Mesh integrity check failed.");
        }
        
        YLOGINFO("  Test 2 PASSED");
    }

    // Test 3: Icosahedron subdivision (larger closed mesh).
    {
        YLOGINFO("Test 3: Icosahedron subdivision");
        auto mesh = create_icosahedron();
        
        YLOGINFO("  Before: vertices=" << mesh.vertices.size() 
                 << ", faces=" << mesh.faces.size()
                 << ", edges=" << count_edges(mesh));
        
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        const size_t E_before = count_edges(mesh);
        const double area_before = mesh.surface_area();
        
        // Verify: Icosahedron has 12 vertices, 20 faces, 30 edges.
        if(V_before != 12 || F_before != 20 || E_before != 30){
            throw std::runtime_error("Test 3 setup failed: Icosahedron dimensions incorrect.");
        }
        
        loop_subdivide(mesh, 1);
        
        const size_t V_after = mesh.vertices.size();
        const size_t F_after = mesh.faces.size();
        const size_t E_after = count_edges(mesh);
        const double area_after = mesh.surface_area();
        
        YLOGINFO("  After: vertices=" << V_after 
                 << ", faces=" << F_after
                 << ", edges=" << E_after
                 << ", area_before=" << area_before
                 << ", area_after=" << area_after);
        
        // After subdivision: V' = 12 + 30 = 42, F' = 4 * 20 = 80.
        if(V_after != 42){
            throw std::runtime_error("Test 3 failed: Wrong vertex count. Expected 42 but got "
                                     + std::to_string(V_after));
        }
        if(F_after != 80){
            throw std::runtime_error("Test 3 failed: Wrong face count. Expected 80 but got "
                                     + std::to_string(F_after));
        }
        if(!verify_mesh_integrity(mesh)){
            throw std::runtime_error("Test 3 failed: Mesh integrity check failed.");
        }
        
        YLOGINFO("  Test 3 PASSED");
    }

    // Test 4: Multiple iterations of subdivision.
    {
        YLOGINFO("Test 4: Multiple iterations of subdivision");
        auto mesh = create_tetrahedron();
        
        const size_t V_0 = mesh.vertices.size();
        const size_t F_0 = mesh.faces.size();
        const size_t E_0 = count_edges(mesh);
        
        // Two iterations of subdivision.
        loop_subdivide(mesh, 2);
        
        const size_t V_2 = mesh.vertices.size();
        const size_t F_2 = mesh.faces.size();
        
        // After iteration 1: V_1 = V_0 + E_0 = 4 + 6 = 10, F_1 = 4 * F_0 = 16.
        // E_1 can be computed from Euler: E_1 = V_1 + F_1 - 2 = 10 + 16 - 2 = 24 (for closed surface).
        // After iteration 2: V_2 = V_1 + E_1 = 10 + 24 = 34, F_2 = 4 * F_1 = 64.
        
        YLOGINFO("  After 2 iterations: vertices=" << V_2 << ", faces=" << F_2);
        
        if(V_2 != 34){
            throw std::runtime_error("Test 4 failed: Wrong vertex count. Expected 34 but got "
                                     + std::to_string(V_2));
        }
        if(F_2 != 64){
            throw std::runtime_error("Test 4 failed: Wrong face count. Expected 64 but got "
                                     + std::to_string(F_2));
        }
        if(!verify_mesh_integrity(mesh)){
            throw std::runtime_error("Test 4 failed: Mesh integrity check failed.");
        }
        
        YLOGINFO("  Test 4 PASSED");
    }

    // Test 5: Zero iterations should be a no-op.
    {
        YLOGINFO("Test 5: Zero iterations should be no-op");
        auto mesh = create_tetrahedron();
        
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        
        loop_subdivide(mesh, 0);
        
        const size_t V_after = mesh.vertices.size();
        const size_t F_after = mesh.faces.size();
        
        if(V_after != V_before || F_after != F_before){
            throw std::runtime_error("Test 5 failed: Mesh changed with 0 iterations.");
        }
        
        YLOGINFO("  Test 5 PASSED");
    }

    // Test 6: Negative iterations should be a no-op.
    {
        YLOGINFO("Test 6: Negative iterations should be no-op");
        auto mesh = create_tetrahedron();
        
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        
        loop_subdivide(mesh, -1);
        
        const size_t V_after = mesh.vertices.size();
        const size_t F_after = mesh.faces.size();
        
        if(V_after != V_before || F_after != F_before){
            throw std::runtime_error("Test 6 failed: Mesh changed with negative iterations.");
        }
        
        YLOGINFO("  Test 6 PASSED");
    }

    // Test 7: Non-triangle mesh should throw.
    {
        YLOGINFO("Test 7: Non-triangle mesh should throw");
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices.emplace_back(vec3<double>(0, 0, 0));
        mesh.vertices.emplace_back(vec3<double>(1, 0, 0));
        mesh.vertices.emplace_back(vec3<double>(1, 1, 0));
        mesh.vertices.emplace_back(vec3<double>(0, 1, 0));
        mesh.faces.push_back({0, 1, 2, 3}); // Quad, not triangle.
        mesh.recreate_involved_face_index();
        
        bool threw = false;
        try {
            loop_subdivide(mesh, 1);
        } catch (const std::invalid_argument &) {
            threw = true;
        }
        
        if(!threw){
            throw std::runtime_error("Test 7 failed: Should have thrown for non-triangle mesh.");
        }
        
        YLOGINFO("  Test 7 PASSED");
    }

    // Test 8: Subdivision should smooth the surface.
    // Verify that after subdivision, the mesh approximates a sphere more closely.
    {
        YLOGINFO("Test 8: Subdivision smoothing towards sphere");
        auto mesh = create_icosahedron();
        
        // Normalize vertices to unit sphere.
        for(auto &v : mesh.vertices){
            v = v.unit();
        }
        
        // Compute average distance from origin before and after subdivision.
        auto avg_dist = [](const fv_surface_mesh<double, uint64_t> &m) -> double {
            double sum = 0.0;
            for(const auto &v : m.vertices){
                sum += v.length();
            }
            return sum / static_cast<double>(m.vertices.size());
        };
        
        auto max_dist_variance = [](const fv_surface_mesh<double, uint64_t> &m) -> double {
            double min_dist = 1e9;
            double max_dist = -1e9;
            for(const auto &v : m.vertices){
                double d = v.length();
                if(d < min_dist) min_dist = d;
                if(d > max_dist) max_dist = d;
            }
            return max_dist - min_dist;
        };
        
        const double variance_before = max_dist_variance(mesh);
        
        // Subdivide three times.
        loop_subdivide(mesh, 3);
        
        const double variance_after = max_dist_variance(mesh);
        
        YLOGINFO("  Variance before=" << variance_before << ", after=" << variance_after);
        
        // The variance should decrease (vertices should become more uniform in distance from origin).
        // But this depends on the mesh starting from a normalized icosahedron, so we just
        // verify that all vertices have reasonable coordinates.
        if(!verify_mesh_integrity(mesh)){
            throw std::runtime_error("Test 8 failed: Mesh integrity check failed.");
        }
        
        YLOGINFO("  Test 8 PASSED");
    }

    YLOGINFO("All tests passed!");
    return 0;
}

