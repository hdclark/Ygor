//YgorMeshProcessing4.cc - Routines for mesh processing.
//
#include <cstdint>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <algorithm>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"

#include "YgorMeshProcessing4.h"


// Perform Loop subdivision on a triangle mesh.
//
// This implements Charles Loop's subdivision scheme (1987) for triangle meshes.
// The algorithm:
// 1. Creates new edge vertices at edge midpoints (with weighted positions).
// 2. Updates original vertex positions using neighborhood information.
// 3. Creates four new triangles from each original triangle.
//
// For interior edges, the new vertex position is:
//   v_new = (3/8) * (v0 + v1) + (1/8) * (v2 + v3)
//   where v0, v1 are the edge endpoints and v2, v3 are the opposite vertices.
//
// For boundary edges, the new vertex position is:
//   v_new = (1/2) * (v0 + v1)
//
// For interior vertices, the new position is:
//   v_new = (1 - n*beta) * v + beta * sum(neighbors)
//   where n is the valence and beta = (1/n) * (5/8 - (3/8 + (1/4)*cos(2*pi/n))^2)
//
// For boundary vertices, the new position is:
//   v_new = (3/4) * v + (1/8) * (v_left + v_right)
//   where v_left and v_right are the two boundary neighbors.
//
template <class T, class I>
void
loop_subdivide(fv_surface_mesh<T,I> &fvsm,
               int64_t iterations){

    if(iterations <= 0) return;

    // Validate that all faces are triangles.
    for(const auto &face : fvsm.faces){
        if(face.size() != 3){
            throw std::invalid_argument("Loop subdivision only supports triangle meshes. Found face with "
                                        + std::to_string(face.size()) + " vertices.");
        }
    }

    // Validate mesh consistency.
    const bool has_normals = !fvsm.vertex_normals.empty();
    const bool has_colours = !fvsm.vertex_colours.empty();
    if(has_normals && (fvsm.vertices.size() != fvsm.vertex_normals.size())){
        throw std::invalid_argument("Vertex normals are inconsistent with vertices.");
    }
    if(has_colours && (fvsm.vertices.size() != fvsm.vertex_colours.size())){
        throw std::invalid_argument("Vertex colours are inconsistent with vertices.");
    }

    for(int64_t iter = 0; iter < iterations; ++iter){
        const auto N_orig_verts = static_cast<I>(fvsm.vertices.size());
        const auto N_orig_faces = fvsm.faces.size();

        // Ensure we have an up-to-date index of involved faces.
        if(fvsm.involved_faces.size() != fvsm.vertices.size()){
            fvsm.recreate_involved_face_index();
        }

        // Helper: Create a canonical edge key from two vertex indices.
        // The edge key is always ordered (min, max) to ensure consistency.
        auto make_edge_key = [](I v0, I v1) -> std::pair<I, I> {
            return (v0 < v1) ? std::make_pair(v0, v1) : std::make_pair(v1, v0);
        };

        // Build edge-to-face adjacency map.
        // For each edge, store the list of faces that contain it.
        std::map<std::pair<I, I>, std::vector<I>> edge_faces;
        for(I f_idx = 0; f_idx < static_cast<I>(N_orig_faces); ++f_idx){
            const auto &face = fvsm.faces[f_idx];
            for(size_t j = 0; j < 3; ++j){
                auto edge_key = make_edge_key(face[j], face[(j + 1) % 3]);
                edge_faces[edge_key].emplace_back(f_idx);
            }
        }

        // Map from edge to new vertex index.
        std::map<std::pair<I, I>, I> edge_to_new_vert;

        // Create new edge vertices.
        // For each unique edge, compute the new vertex position and add it.
        for(const auto &ef : edge_faces){
            const auto &edge_key = ef.first;
            const auto &faces_on_edge = ef.second;
            const I v0 = edge_key.first;
            const I v1 = edge_key.second;

            vec3<T> new_pos;
            vec3<T> new_normal;
            uint32_t new_colour = 0;

            if(faces_on_edge.size() == 2){
                // Interior edge: use Loop's weighting.
                // Find the two opposite vertices.
                I v2 = static_cast<I>(-1);
                I v3 = static_cast<I>(-1);

                for(size_t fi = 0; fi < 2; ++fi){
                    const auto &face = fvsm.faces[faces_on_edge[fi]];
                    for(size_t j = 0; j < 3; ++j){
                        if(face[j] != v0 && face[j] != v1){
                            if(fi == 0) v2 = face[j];
                            else v3 = face[j];
                            break;
                        }
                    }
                }

                // new_pos = (3/8) * (v0 + v1) + (1/8) * (v2 + v3)
                const T w_edge = static_cast<T>(3.0 / 8.0);
                const T w_opp = static_cast<T>(1.0 / 8.0);
                new_pos = fvsm.vertices[v0] * w_edge
                        + fvsm.vertices[v1] * w_edge
                        + fvsm.vertices[v2] * w_opp
                        + fvsm.vertices[v3] * w_opp;

                if(has_normals){
                    new_normal = (fvsm.vertex_normals[v0] * w_edge
                                + fvsm.vertex_normals[v1] * w_edge
                                + fvsm.vertex_normals[v2] * w_opp
                                + fvsm.vertex_normals[v3] * w_opp).unit();
                }
                if(has_colours){
                    // Simple average for colours.
                    auto c0 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v0]);
                    auto c1 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v1]);
                    auto c2 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v2]);
                    auto c3 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v3]);
                    std::array<uint8_t, 4> c_new;
                    for(size_t ci = 0; ci < 4; ++ci){
                        c_new[ci] = static_cast<uint8_t>(
                            static_cast<T>(c0[ci]) * w_edge
                          + static_cast<T>(c1[ci]) * w_edge
                          + static_cast<T>(c2[ci]) * w_opp
                          + static_cast<T>(c3[ci]) * w_opp);
                    }
                    new_colour = fvsm.pack_RGBA32_colour(c_new);
                }
            }else{
                // Boundary edge: use simple midpoint.
                new_pos = (fvsm.vertices[v0] + fvsm.vertices[v1]) * static_cast<T>(0.5);

                if(has_normals){
                    new_normal = (fvsm.vertex_normals[v0] + fvsm.vertex_normals[v1]).unit();
                }
                if(has_colours){
                    auto c0 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v0]);
                    auto c1 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v1]);
                    std::array<uint8_t, 4> c_new;
                    for(size_t ci = 0; ci < 4; ++ci){
                        c_new[ci] = static_cast<uint8_t>((static_cast<int>(c0[ci]) + static_cast<int>(c1[ci])) / 2);
                    }
                    new_colour = fvsm.pack_RGBA32_colour(c_new);
                }
            }

            // Add the new vertex.
            I new_vert_idx = static_cast<I>(fvsm.vertices.size());
            fvsm.vertices.emplace_back(new_pos);
            if(has_normals){
                fvsm.vertex_normals.emplace_back(new_normal);
            }
            if(has_colours){
                fvsm.vertex_colours.emplace_back(new_colour);
            }

            edge_to_new_vert[edge_key] = new_vert_idx;
        }

        // Determine boundary vertices (vertices on boundary edges).
        std::set<I> boundary_verts;
        // Also build boundary edge map for boundary vertex position update.
        std::map<I, std::vector<I>> boundary_neighbors; // boundary_neighbors[v] = list of boundary edge neighbors
        for(const auto &ef : edge_faces){
            if(ef.second.size() == 1){
                // This is a boundary edge.
                boundary_verts.insert(ef.first.first);
                boundary_verts.insert(ef.first.second);
                boundary_neighbors[ef.first.first].emplace_back(ef.first.second);
                boundary_neighbors[ef.first.second].emplace_back(ef.first.first);
            }
        }

        // Compute updated positions for original vertices.
        std::vector<vec3<T>> updated_positions(N_orig_verts);
        std::vector<vec3<T>> updated_normals;
        std::vector<uint32_t> updated_colours;
        if(has_normals) updated_normals.resize(N_orig_verts);
        if(has_colours) updated_colours.resize(N_orig_verts);

        for(I v_idx = 0; v_idx < N_orig_verts; ++v_idx){
            if(boundary_verts.count(v_idx) > 0){
                // Boundary vertex update.
                // v_new = (3/4) * v + (1/8) * (v_left + v_right)
                const auto &bn = boundary_neighbors[v_idx];
                if(bn.size() >= 2){
                    const T w_center = static_cast<T>(3.0 / 4.0);
                    const T w_neighbor = static_cast<T>(1.0 / 8.0);
                    updated_positions[v_idx] = fvsm.vertices[v_idx] * w_center
                                             + fvsm.vertices[bn[0]] * w_neighbor
                                             + fvsm.vertices[bn[1]] * w_neighbor;
                    if(has_normals){
                        updated_normals[v_idx] = (fvsm.vertex_normals[v_idx] * w_center
                                                + fvsm.vertex_normals[bn[0]] * w_neighbor
                                                + fvsm.vertex_normals[bn[1]] * w_neighbor).unit();
                    }
                    if(has_colours){
                        auto c0 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v_idx]);
                        auto c1 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[bn[0]]);
                        auto c2 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[bn[1]]);
                        std::array<uint8_t, 4> c_new;
                        for(size_t ci = 0; ci < 4; ++ci){
                            c_new[ci] = static_cast<uint8_t>(
                                static_cast<T>(c0[ci]) * w_center
                              + static_cast<T>(c1[ci]) * w_neighbor
                              + static_cast<T>(c2[ci]) * w_neighbor);
                        }
                        updated_colours[v_idx] = fvsm.pack_RGBA32_colour(c_new);
                    }
                }else{
                    // Corner case: vertex has fewer than 2 boundary neighbors (isolated or degenerate).
                    updated_positions[v_idx] = fvsm.vertices[v_idx];
                    if(has_normals) updated_normals[v_idx] = fvsm.vertex_normals[v_idx];
                    if(has_colours) updated_colours[v_idx] = fvsm.vertex_colours[v_idx];
                }
            }else{
                // Interior vertex update.
                // Collect all neighboring vertices using involved_faces.
                std::set<I> neighbors;
                for(const auto &f_idx : fvsm.involved_faces[v_idx]){
                    for(const auto &v : fvsm.faces[f_idx]){
                        if(v != v_idx) neighbors.insert(v);
                    }
                }

                const size_t n = neighbors.size();
                if(n == 0){
                    // Isolated vertex.
                    updated_positions[v_idx] = fvsm.vertices[v_idx];
                    if(has_normals) updated_normals[v_idx] = fvsm.vertex_normals[v_idx];
                    if(has_colours) updated_colours[v_idx] = fvsm.vertex_colours[v_idx];
                }else{
                    // Compute beta using Loop's formula.
                    // beta = (1/n) * (5/8 - (3/8 + (1/4)*cos(2*pi/n))^2)
                    const T pi = static_cast<T>(3.14159265358979323846);
                    const T n_t = static_cast<T>(n);
                    const T inner = static_cast<T>(3.0 / 8.0) + static_cast<T>(1.0 / 4.0) * std::cos(static_cast<T>(2.0) * pi / n_t);
                    const T beta = (static_cast<T>(1.0) / n_t) * (static_cast<T>(5.0 / 8.0) - inner * inner);

                    // v_new = (1 - n*beta) * v + beta * sum(neighbors)
                    const T w_center = static_cast<T>(1.0) - n_t * beta;
                    vec3<T> neighbor_sum(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
                    vec3<T> normal_sum(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
                    std::array<T, 4> colour_sum = {0, 0, 0, 0};

                    for(const auto &nb : neighbors){
                        neighbor_sum = neighbor_sum + fvsm.vertices[nb];
                        if(has_normals) normal_sum = normal_sum + fvsm.vertex_normals[nb];
                        if(has_colours){
                            auto c = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[nb]);
                            for(size_t ci = 0; ci < 4; ++ci){
                                colour_sum[ci] += static_cast<T>(c[ci]);
                            }
                        }
                    }

                    updated_positions[v_idx] = fvsm.vertices[v_idx] * w_center + neighbor_sum * beta;
                    if(has_normals){
                        updated_normals[v_idx] = (fvsm.vertex_normals[v_idx] * w_center + normal_sum * beta).unit();
                    }
                    if(has_colours){
                        auto c0 = fvsm.unpack_RGBA32_colour(fvsm.vertex_colours[v_idx]);
                        std::array<uint8_t, 4> c_new;
                        for(size_t ci = 0; ci < 4; ++ci){
                            c_new[ci] = static_cast<uint8_t>(
                                static_cast<T>(c0[ci]) * w_center + colour_sum[ci] * beta);
                        }
                        updated_colours[v_idx] = fvsm.pack_RGBA32_colour(c_new);
                    }
                }
            }
        }

        // Apply the updated positions to original vertices.
        for(I v_idx = 0; v_idx < N_orig_verts; ++v_idx){
            fvsm.vertices[v_idx] = updated_positions[v_idx];
            if(has_normals) fvsm.vertex_normals[v_idx] = updated_normals[v_idx];
            if(has_colours) fvsm.vertex_colours[v_idx] = updated_colours[v_idx];
        }

        // Create new faces.
        // For each original triangle (v0, v1, v2), create 4 new triangles:
        //   - (v0, e01, e20)
        //   - (v1, e12, e01)
        //   - (v2, e20, e12)
        //   - (e01, e12, e20)
        // where e01 is the edge vertex between v0 and v1, etc.
        std::vector<std::vector<I>> new_faces;
        new_faces.reserve(N_orig_faces * 4);

        for(size_t f_idx = 0; f_idx < N_orig_faces; ++f_idx){
            const auto &face = fvsm.faces[f_idx];
            const I v0 = face[0];
            const I v1 = face[1];
            const I v2 = face[2];

            const I e01 = edge_to_new_vert.at(make_edge_key(v0, v1));
            const I e12 = edge_to_new_vert.at(make_edge_key(v1, v2));
            const I e20 = edge_to_new_vert.at(make_edge_key(v2, v0));

            // Corner triangles.
            new_faces.push_back({v0, e01, e20});
            new_faces.push_back({v1, e12, e01});
            new_faces.push_back({v2, e20, e12});
            // Center triangle.
            new_faces.push_back({e01, e12, e20});
        }

        // Replace old faces with new faces.
        fvsm.faces = std::move(new_faces);

        // Invalidate and rebuild involved_faces index.
        fvsm.involved_faces.clear();
    }

    // Rebuild the involved_faces index for the final mesh.
    fvsm.recreate_involved_face_index();
}

#ifndef YGORMESHPROCESSING4_DISABLE_ALL_SPECIALIZATIONS
    template void loop_subdivide(fv_surface_mesh<float , uint32_t> &, int64_t);
    template void loop_subdivide(fv_surface_mesh<float , uint64_t> &, int64_t);

    template void loop_subdivide(fv_surface_mesh<double, uint32_t> &, int64_t);
    template void loop_subdivide(fv_surface_mesh<double, uint64_t> &, int64_t);
#endif

