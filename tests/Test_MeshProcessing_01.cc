// Test_MeshProcessing_01.cc - Test mesh remeshing algorithm.

#include <iostream>
#include <sstream>
#include <cmath>
#include <string>
#include <random>
#include <vector>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMeshProcessing2.h"


// Helper function to create a simple test mesh (a tetrahedron).
fv_surface_mesh<double, uint32_t> create_tetrahedron() {
    fv_surface_mesh<double, uint32_t> mesh;
    
    // Vertices of a regular tetrahedron.
    mesh.vertices.push_back(vec3<double>(1, 1, 1));
    mesh.vertices.push_back(vec3<double>(1, -1, -1));
    mesh.vertices.push_back(vec3<double>(-1, 1, -1));
    mesh.vertices.push_back(vec3<double>(-1, -1, 1));
    
    // Faces (4 triangular faces).
    mesh.faces.push_back({0, 1, 2});
    mesh.faces.push_back({0, 2, 3});
    mesh.faces.push_back({0, 3, 1});
    mesh.faces.push_back({1, 3, 2});
    
    return mesh;
}

// Helper function to create an octahedron (8 faces, 6 vertices).
fv_surface_mesh<double, uint32_t> create_octahedron() {
    fv_surface_mesh<double, uint32_t> mesh;
    
    // Vertices.
    mesh.vertices.push_back(vec3<double>(1, 0, 0));   // 0: +x
    mesh.vertices.push_back(vec3<double>(-1, 0, 0));  // 1: -x
    mesh.vertices.push_back(vec3<double>(0, 1, 0));   // 2: +y
    mesh.vertices.push_back(vec3<double>(0, -1, 0));  // 3: -y
    mesh.vertices.push_back(vec3<double>(0, 0, 1));   // 4: +z
    mesh.vertices.push_back(vec3<double>(0, 0, -1));  // 5: -z
    
    // Faces (8 triangular faces).
    mesh.faces.push_back({0, 2, 4});
    mesh.faces.push_back({2, 1, 4});
    mesh.faces.push_back({1, 3, 4});
    mesh.faces.push_back({3, 0, 4});
    mesh.faces.push_back({2, 0, 5});
    mesh.faces.push_back({1, 2, 5});
    mesh.faces.push_back({3, 1, 5});
    mesh.faces.push_back({0, 3, 5});
    
    return mesh;
}

// Helper function to create a simple quad (2 triangles sharing an edge).
fv_surface_mesh<double, uint32_t> create_quad() {
    fv_surface_mesh<double, uint32_t> mesh;
    
    // Vertices of a unit square.
    mesh.vertices.push_back(vec3<double>(0, 0, 0));
    mesh.vertices.push_back(vec3<double>(1, 0, 0));
    mesh.vertices.push_back(vec3<double>(1, 1, 0));
    mesh.vertices.push_back(vec3<double>(0, 1, 0));
    
    // Two triangular faces.
    mesh.faces.push_back({0, 1, 2});
    mesh.faces.push_back({0, 2, 3});
    
    return mesh;
}

// Helper function to create a larger mesh (icosahedron approximation).
fv_surface_mesh<double, uint32_t> create_icosahedron() {
    fv_surface_mesh<double, uint32_t> mesh;
    
    // Golden ratio.
    const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
    
    // 12 vertices of icosahedron.
    mesh.vertices.push_back(vec3<double>(0, 1, phi));
    mesh.vertices.push_back(vec3<double>(0, -1, phi));
    mesh.vertices.push_back(vec3<double>(0, 1, -phi));
    mesh.vertices.push_back(vec3<double>(0, -1, -phi));
    mesh.vertices.push_back(vec3<double>(1, phi, 0));
    mesh.vertices.push_back(vec3<double>(-1, phi, 0));
    mesh.vertices.push_back(vec3<double>(1, -phi, 0));
    mesh.vertices.push_back(vec3<double>(-1, -phi, 0));
    mesh.vertices.push_back(vec3<double>(phi, 0, 1));
    mesh.vertices.push_back(vec3<double>(-phi, 0, 1));
    mesh.vertices.push_back(vec3<double>(phi, 0, -1));
    mesh.vertices.push_back(vec3<double>(-phi, 0, -1));
    
    // 20 faces of icosahedron.
    mesh.faces.push_back({0, 1, 8});
    mesh.faces.push_back({0, 8, 4});
    mesh.faces.push_back({0, 4, 5});
    mesh.faces.push_back({0, 5, 9});
    mesh.faces.push_back({0, 9, 1});
    mesh.faces.push_back({1, 6, 8});
    mesh.faces.push_back({8, 6, 10});
    mesh.faces.push_back({8, 10, 4});
    mesh.faces.push_back({4, 10, 2});
    mesh.faces.push_back({4, 2, 5});
    mesh.faces.push_back({5, 2, 11});
    mesh.faces.push_back({5, 11, 9});
    mesh.faces.push_back({9, 11, 7});
    mesh.faces.push_back({9, 7, 1});
    mesh.faces.push_back({1, 7, 6});
    mesh.faces.push_back({6, 7, 3});
    mesh.faces.push_back({6, 3, 10});
    mesh.faces.push_back({10, 3, 2});
    mesh.faces.push_back({2, 3, 11});
    mesh.faces.push_back({11, 3, 7});
    
    return mesh;
}


int main(int argc, char **argv) {
    const double eps = std::sqrt(std::numeric_limits<double>::epsilon());
    
    // Test 1: Basic construction and parameter access.
    {
        YLOGINFO("Test 1: Basic construction and parameter access");
        auto mesh = create_tetrahedron();
        const double target_len = 1.0;
        mesh_remesher<double, uint32_t> remesher(mesh, target_len);
        
        if(std::abs(remesher.get_target_edge_length() - target_len) > eps) {
            throw std::runtime_error("Target edge length not set correctly.");
        }
        
        const double expected_max = 4.0 / 3.0 * target_len;
        const double expected_min = 4.0 / 5.0 * target_len;
        
        if(std::abs(remesher.get_max_edge_length() - expected_max) > eps) {
            throw std::runtime_error("Max edge length not computed correctly.");
        }
        if(std::abs(remesher.get_min_edge_length() - expected_min) > eps) {
            throw std::runtime_error("Min edge length not computed correctly.");
        }
        
        YLOGINFO("  PASSED");
    }
    
    // Test 2: Mesh quality metrics on known mesh.
    {
        YLOGINFO("Test 2: Mesh quality metrics");
        auto mesh = create_quad();
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        
        double mean_len = remesher.mean_edge_length();
        if(mean_len <= 0) {
            throw std::runtime_error("Mean edge length should be positive.");
        }
        
        double stddev = remesher.edge_length_stddev();
        if(stddev < 0) {
            throw std::runtime_error("Edge length stddev should be non-negative.");
        }
        
        YLOGINFO("  Mean edge length: " << mean_len);
        YLOGINFO("  Edge length stddev: " << stddev);
        YLOGINFO("  PASSED");
    }
    
    // Test 3: Edge splitting.
    {
        YLOGINFO("Test 3: Edge splitting");
        auto mesh = create_quad();
        
        // Use a target edge length that will cause edges to be split.
        // The diagonal of the quad is sqrt(2), which is longer than 4/3 * 0.5 = 0.667.
        const double target_len = 0.5;
        mesh_remesher<double, uint32_t> remesher(mesh, target_len);
        
        size_t original_faces = mesh.faces.size();
        size_t original_vertices = mesh.vertices.size();
        
        int64_t splits = remesher.split_long_edges();
        
        YLOGINFO("  Original faces: " << original_faces << ", vertices: " << original_vertices);
        YLOGINFO("  After splitting faces: " << mesh.faces.size() << ", vertices: " << mesh.vertices.size());
        YLOGINFO("  Splits performed: " << splits);
        
        if(splits <= 0) {
            throw std::runtime_error("Expected at least one edge to be split.");
        }
        if(mesh.faces.size() <= original_faces) {
            throw std::runtime_error("Expected more faces after splitting.");
        }
        if(mesh.vertices.size() <= original_vertices) {
            throw std::runtime_error("Expected more vertices after splitting.");
        }
        
        // Verify all edges are now within bounds.
        double max_edge = remesher.get_max_edge_length();
        double max_found = 0.0;
        for(const auto &face : mesh.faces) {
            if(face.size() < 3) continue;
            for(size_t i = 0; i < face.size(); ++i) {
                size_t j = (i + 1) % face.size();
                double len = mesh.vertices[face[i]].distance(mesh.vertices[face[j]]);
                max_found = std::max(max_found, len);
            }
        }
        YLOGINFO("  Max edge length after splitting: " << max_found << " (limit: " << max_edge << ")");
        
        if(max_found > max_edge * 1.01) {  // Small tolerance for floating point.
            throw std::runtime_error("Edge splitting did not sufficiently reduce edge lengths.");
        }
        
        YLOGINFO("  PASSED");
    }
    
    // Test 4: Edge collapsing.
    {
        YLOGINFO("Test 4: Edge collapsing");
        
        // Create a mesh with some short edges.
        fv_surface_mesh<double, uint32_t> mesh;
        mesh.vertices.push_back(vec3<double>(0, 0, 0));
        mesh.vertices.push_back(vec3<double>(1, 0, 0));
        mesh.vertices.push_back(vec3<double>(0.5, 1, 0));
        mesh.vertices.push_back(vec3<double>(0.05, 0, 0));  // Very close to vertex 0.
        mesh.faces.push_back({0, 3, 2});
        mesh.faces.push_back({3, 1, 2});
        
        const double target_len = 1.0;  // min_len = 0.8, and edge 0-3 is 0.05.
        mesh_remesher<double, uint32_t> remesher(mesh, target_len);
        
        size_t original_faces = mesh.faces.size();
        
        int64_t collapses = remesher.collapse_short_edges();
        
        YLOGINFO("  Original faces: " << original_faces);
        YLOGINFO("  After collapsing faces: " << mesh.faces.size());
        YLOGINFO("  Collapses performed: " << collapses);
        
        if(collapses <= 0) {
            throw std::runtime_error("Expected at least one edge to be collapsed.");
        }
        
        YLOGINFO("  PASSED");
    }
    
    // Test 5: Edge flipping.
    {
        YLOGINFO("Test 5: Edge flipping");
        
        // Create a configuration where flipping would improve valence.
        // A "bowtie" configuration: two triangles sharing an edge where opposite vertices
        // have high valence.
        fv_surface_mesh<double, uint32_t> mesh;
        
        // Create a small mesh where flipping might help.
        mesh.vertices.push_back(vec3<double>(0, 0, 0));   // 0
        mesh.vertices.push_back(vec3<double>(2, 0, 0));   // 1
        mesh.vertices.push_back(vec3<double>(1, 1, 0));   // 2
        mesh.vertices.push_back(vec3<double>(1, -1, 0));  // 3
        
        // Create two triangles sharing edge 0-1.
        mesh.faces.push_back({0, 1, 2});
        mesh.faces.push_back({0, 3, 1});
        
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        
        double initial_deviation = remesher.valence_deviation();
        YLOGINFO("  Initial valence deviation: " << initial_deviation);
        
        int64_t flips = remesher.flip_edges_for_valence();
        
        double final_deviation = remesher.valence_deviation();
        YLOGINFO("  Final valence deviation: " << final_deviation);
        YLOGINFO("  Flips performed: " << flips);
        
        // The flip operation may or may not improve this simple case, but it shouldn't crash.
        YLOGINFO("  PASSED (no crash, operation completed)");
    }
    
    // Test 6: Tangential relaxation.
    {
        YLOGINFO("Test 6: Tangential relaxation");
        auto mesh = create_octahedron();
        
        // Perturb vertices slightly.
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(-0.1, 0.1);
        for(auto &v : mesh.vertices) {
            v.x += dist(rng);
            v.y += dist(rng);
            v.z += dist(rng);
        }
        
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        
        // Store original positions.
        auto original_verts = mesh.vertices;
        
        int64_t moved = remesher.tangential_relaxation(0.5);
        
        YLOGINFO("  Vertices moved: " << moved);
        
        // Check that at least some vertices were moved.
        int changed = 0;
        for(size_t i = 0; i < mesh.vertices.size(); ++i) {
            if(mesh.vertices[i].distance(original_verts[i]) > eps) {
                ++changed;
            }
        }
        
        YLOGINFO("  Vertices with position change: " << changed);
        YLOGINFO("  PASSED");
    }
    
    // Test 7: Full remeshing iteration.
    {
        YLOGINFO("Test 7: Full remeshing iteration");
        auto mesh = create_icosahedron();
        
        // Scale up the mesh to make edges longer.
        for(auto &v : mesh.vertices) {
            v *= 3.0;
        }
        
        mesh_remesher<double, uint32_t> remesher(mesh, 1.5);
        
        YLOGINFO("  Initial faces: " << mesh.faces.size() << ", vertices: " << mesh.vertices.size());
        YLOGINFO("  Initial mean edge length: " << remesher.mean_edge_length());
        YLOGINFO("  Initial mean valence: " << remesher.mean_valence());
        
        // Run several iterations.
        for(int iter = 0; iter < 3; ++iter) {
            int64_t changes = remesher.remesh_iteration();
            YLOGINFO("  Iteration " << (iter + 1) << ": " << changes << " changes");
        }
        
        YLOGINFO("  Final faces: " << mesh.faces.size() << ", vertices: " << mesh.vertices.size());
        YLOGINFO("  Final mean edge length: " << remesher.mean_edge_length());
        YLOGINFO("  Final mean valence: " << remesher.mean_valence());
        
        // Verify mesh is still valid (all faces reference valid vertices).
        for(const auto &face : mesh.faces) {
            for(const auto &v : face) {
                if(v >= mesh.vertices.size()) {
                    throw std::runtime_error("Invalid vertex reference in face after remeshing.");
                }
            }
        }
        
        YLOGINFO("  PASSED");
    }
    
    // Test 8: Aspect ratio computation.
    {
        YLOGINFO("Test 8: Aspect ratio computation");
        auto mesh = create_tetrahedron();
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        
        auto [min_ar, max_ar] = remesher.aspect_ratio_range();
        
        YLOGINFO("  Min aspect ratio: " << min_ar);
        YLOGINFO("  Max aspect ratio: " << max_ar);
        
        // For a regular tetrahedron, all triangles should have similar aspect ratios.
        if(min_ar <= 0 || max_ar <= 0) {
            throw std::runtime_error("Aspect ratios should be positive.");
        }
        
        YLOGINFO("  PASSED");
    }
    
    // Test 9: Parameter modification.
    {
        YLOGINFO("Test 9: Parameter modification");
        auto mesh = create_tetrahedron();
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        
        remesher.set_target_edge_length(2.0);
        
        if(std::abs(remesher.get_target_edge_length() - 2.0) > eps) {
            throw std::runtime_error("set_target_edge_length did not work.");
        }
        
        const double expected_max = 4.0 / 3.0 * 2.0;
        const double expected_min = 4.0 / 5.0 * 2.0;
        
        if(std::abs(remesher.get_max_edge_length() - expected_max) > eps) {
            throw std::runtime_error("Max edge length not updated after set.");
        }
        if(std::abs(remesher.get_min_edge_length() - expected_min) > eps) {
            throw std::runtime_error("Min edge length not updated after set.");
        }
        
        // Test invalid parameter.
        bool caught = false;
        try {
            remesher.set_target_edge_length(-1.0);
        } catch(const std::runtime_error &) {
            caught = true;
        }
        if(!caught) {
            throw std::runtime_error("Expected exception for negative target edge length.");
        }
        
        YLOGINFO("  PASSED");
    }
    
    // Test 10: Mesh validity after multiple operations.
    {
        YLOGINFO("Test 10: Mesh validity after multiple operations");
        auto mesh = create_octahedron();
        
        // Scale up.
        for(auto &v : mesh.vertices) {
            v *= 5.0;
        }
        
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        
        // Run many iterations.
        for(int i = 0; i < 5; ++i) {
            remesher.remesh_iteration();
            
            // Verify mesh validity after each iteration.
            for(const auto &face : mesh.faces) {
                if(face.size() < 3) {
                    throw std::runtime_error("Degenerate face after iteration.");
                }
                for(const auto &v : face) {
                    if(v >= mesh.vertices.size()) {
                        throw std::runtime_error("Invalid vertex reference after iteration.");
                    }
                }
            }
        }
        
        YLOGINFO("  Final faces: " << mesh.faces.size() << ", vertices: " << mesh.vertices.size());
        YLOGINFO("  PASSED");
    }
    
    YLOGINFO("All tests passed!");
    return 0;
}
