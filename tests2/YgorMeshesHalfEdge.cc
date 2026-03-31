
#include <limits>
#include <utility>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <set>

#include <YgorMath.h>
#include <YgorMeshesHalfEdge.h>

#include "doctest/doctest.h"


namespace {

// Build a closed tetrahedron fv_surface_mesh for testing.
fv_surface_mesh<double, uint64_t> make_tetrahedron(){
    fv_surface_mesh<double, uint64_t> fvsm;
    fvsm.vertices = {
        vec3<double>(0.0, 0.0, 0.0),
        vec3<double>(1.0, 0.0, 0.0),
        vec3<double>(0.0, 1.0, 0.0),
        vec3<double>(0.0, 0.0, 1.0)
    };
    // Four faces, consistently oriented (outward-facing normals).
    fvsm.faces = {
        {0, 2, 1},
        {0, 1, 3},
        {1, 2, 3},
        {0, 3, 2}
    };
    return fvsm;
}

// Build a single triangle (open mesh with boundary).
fv_surface_mesh<double, uint64_t> make_single_triangle(){
    fv_surface_mesh<double, uint64_t> fvsm;
    fvsm.vertices = {
        vec3<double>(0.0, 0.0, 0.0),
        vec3<double>(1.0, 0.0, 0.0),
        vec3<double>(0.0, 1.0, 0.0)
    };
    fvsm.faces = {
        {0, 1, 2}
    };
    return fvsm;
}

// Build two triangles sharing an edge (open mesh with boundary).
fv_surface_mesh<double, uint64_t> make_two_triangles(){
    fv_surface_mesh<double, uint64_t> fvsm;
    fvsm.vertices = {
        vec3<double>(0.0, 0.0, 0.0),
        vec3<double>(1.0, 0.0, 0.0),
        vec3<double>(0.5, 1.0, 0.0),
        vec3<double>(0.5, -1.0, 0.0)
    };
    fvsm.faces = {
        {0, 1, 2},
        {1, 0, 3}
    };
    return fvsm;
}

} // namespace


TEST_CASE( "YgorMeshesHalfEdge" ){

    SUBCASE("default construction produces empty mesh"){
        he_surface_mesh<double, uint64_t> he;
        REQUIRE(he.num_vertices()  == 0UL);
        REQUIRE(he.num_faces()     == 0UL);
        REQUIRE(he.num_halfedges() == 0UL);
        REQUIRE(he.num_edges()     == 0UL);
        REQUIRE(he.vertices.empty());
        REQUIRE(he.halfedges.empty());
        REQUIRE(he.face_halfedges.empty());
    }

    SUBCASE("conversion from closed tetrahedron"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        REQUIRE(he.num_vertices()  == 4UL);
        REQUIRE(he.num_faces()     == 4UL);
        // Tetrahedron: 6 edges, each has 2 half-edges = 12 half-edges.
        REQUIRE(he.num_halfedges() == 12UL);
        REQUIRE(he.num_edges()     == 6UL);

        // Every half-edge should have a valid twin (closed mesh).
        for(uint64_t i = 0; i < he.num_halfedges(); ++i){
            REQUIRE(he.halfedges[i].twin != he_surface_mesh<double, uint64_t>::sentinel);
        }

        // No boundary vertices in a closed mesh.
        for(uint64_t v = 0; v < he.num_vertices(); ++v){
            REQUIRE_FALSE(he.is_boundary_vertex(v));
        }
    }

    SUBCASE("conversion from single triangle (boundary mesh)"){
        auto fvsm = make_single_triangle();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        REQUIRE(he.num_vertices()  == 3UL);
        REQUIRE(he.num_faces()     == 1UL);
        REQUIRE(he.num_halfedges() == 3UL);
        REQUIRE(he.num_edges()     == 3UL);

        // All edges are boundary (no twins).
        for(uint64_t i = 0; i < he.num_halfedges(); ++i){
            REQUIRE(he.halfedges[i].twin == he_surface_mesh<double, uint64_t>::sentinel);
        }

        // All vertices are boundary.
        for(uint64_t v = 0; v < he.num_vertices(); ++v){
            REQUIRE(he.is_boundary_vertex(v));
        }
    }

    SUBCASE("conversion from two triangles sharing an edge"){
        auto fvsm = make_two_triangles();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        REQUIRE(he.num_vertices()  == 4UL);
        REQUIRE(he.num_faces()     == 2UL);
        REQUIRE(he.num_halfedges() == 6UL);
        // 5 edges: 1 shared (interior) + 4 boundary.
        REQUIRE(he.num_edges()     == 5UL);
    }

    SUBCASE("face_vertices round-trip"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        for(uint64_t f = 0; f < he.num_faces(); ++f){
            const auto verts = he.face_vertices(f);
            REQUIRE(verts.size() == 3UL);
            // All vertex indices should be valid.
            for(const auto &v : verts){
                REQUIRE(v < he.num_vertices());
            }
        }
    }

    SUBCASE("vertex_faces returns correct number of incident faces"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        // Each vertex of a tetrahedron is incident to 3 faces.
        for(uint64_t v = 0; v < he.num_vertices(); ++v){
            const auto faces = he.vertex_faces(v);
            REQUIRE(faces.size() == 3UL);
        }
    }

    SUBCASE("vertex_neighbours returns correct neighbours"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        // Each vertex of a tetrahedron is connected to all 3 other vertices.
        for(uint64_t v = 0; v < he.num_vertices(); ++v){
            const auto nbrs = he.vertex_neighbours(v);
            REQUIRE(nbrs.size() == 3UL);
            // No self-references.
            for(const auto &n : nbrs){
                REQUIRE(n != v);
            }
        }
    }

    SUBCASE("face_neighbours returns adjacent faces"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        // Each face of a tetrahedron shares an edge with all 3 other faces.
        for(uint64_t f = 0; f < he.num_faces(); ++f){
            const auto nbrs = he.face_neighbours(f);
            REQUIRE(nbrs.size() == 3UL);
            // No self-references.
            for(const auto &n : nbrs){
                REQUIRE(n != f);
            }
        }
    }

    SUBCASE("face_neighbours for boundary mesh"){
        auto fvsm = make_two_triangles();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        // Each face should have exactly 1 adjacent face (the other triangle)
        // since the other edges are boundary.
        for(uint64_t f = 0; f < he.num_faces(); ++f){
            const auto nbrs = he.face_neighbours(f);
            REQUIRE(nbrs.size() == 1UL);
        }
    }

    SUBCASE("convert_to_fv_surface_mesh round-trip preserves geometry"){
        auto fvsm_orig = make_tetrahedron();
        fvsm_orig.metadata["key1"] = "val1";

        auto he = convert_fv_to_he_surface_mesh(fvsm_orig);
        auto fvsm_rt = he.convert_to_fv_surface_mesh();

        REQUIRE(fvsm_rt.vertices.size() == fvsm_orig.vertices.size());
        REQUIRE(fvsm_rt.faces.size()    == fvsm_orig.faces.size());
        REQUIRE(fvsm_rt.metadata        == fvsm_orig.metadata);

        // Each vertex should match exactly.
        for(size_t i = 0; i < fvsm_orig.vertices.size(); ++i){
            REQUIRE(fvsm_rt.vertices[i] == fvsm_orig.vertices[i]);
        }

        // Faces should contain the same vertex sets (order within a face may
        // differ due to the half-edge starting point).
        for(size_t f = 0; f < fvsm_orig.faces.size(); ++f){
            auto orig_sorted = fvsm_orig.faces[f];
            auto rt_sorted   = fvsm_rt.faces[f];
            std::sort(orig_sorted.begin(), orig_sorted.end());
            std::sort(rt_sorted.begin(),   rt_sorted.end());
            REQUIRE(orig_sorted == rt_sorted);
        }
    }

    SUBCASE("surface_area matches fv_surface_mesh"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        const auto sa_fv = fvsm.surface_area();
        const auto sa_he = he.surface_area();
        REQUIRE(std::abs(sa_fv - sa_he) < 1.0E-12);
    }

    SUBCASE("surface_area of single face"){
        auto fvsm = make_single_triangle();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        const auto sa = he.surface_area(0);
        // Triangle with vertices at origin, (1,0,0), (0,1,0) has area 0.5.
        REQUIRE(std::abs(sa - 0.5) < 1.0E-12);
    }

    SUBCASE("surface_area throws for invalid face index"){
        auto fvsm = make_single_triangle();
        auto he = convert_fv_to_he_surface_mesh(fvsm);
        REQUIRE_THROWS(he.surface_area(99));
    }

    SUBCASE("compute_vertex_normals produces unit normals"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        he.compute_vertex_normals();

        REQUIRE(he.vertex_normals.size() == he.vertices.size());
        for(const auto &vn : he.vertex_normals){
            REQUIRE(std::abs(vn.length() - 1.0) < 1.0E-6);
        }
    }

    SUBCASE("copy constructor and operator=="){
        auto fvsm = make_tetrahedron();
        auto he1 = convert_fv_to_he_surface_mesh(fvsm);

        auto he2(he1);
        REQUIRE(he1 == he2);
        REQUIRE_FALSE(he1 != he2);
    }

    SUBCASE("operator= and swap"){
        auto fvsm1 = make_tetrahedron();
        auto fvsm2 = make_single_triangle();
        auto he1 = convert_fv_to_he_surface_mesh(fvsm1);
        auto he2 = convert_fv_to_he_surface_mesh(fvsm2);

        auto he_copy1(he1);
        auto he_copy2(he2);

        he1.swap(he2);
        REQUIRE(he1 == he_copy2);
        REQUIRE(he2 == he_copy1);

        he_surface_mesh<double, uint64_t> he3;
        he3 = he_copy1;
        REQUIRE(he3 == he_copy1);
    }

    SUBCASE("pack and unpack RGBA32 colour"){
        he_surface_mesh<double, uint64_t> he;
        const std::array<uint8_t,4> colour = {{255, 128, 64, 32}};
        const auto packed = he.pack_RGBA32_colour(colour);
        const auto unpacked = he.unpack_RGBA32_colour(packed);
        REQUIRE(unpacked == colour);
    }

    SUBCASE("metadata operations"){
        auto fvsm = make_tetrahedron();
        fvsm.metadata["count"] = "42";
        fvsm.metadata["name"] = "test_mesh";

        auto he = convert_fv_to_he_surface_mesh(fvsm);

        REQUIRE(he.MetadataKeyPresent("count"));
        REQUIRE(he.MetadataKeyPresent("name"));
        REQUIRE_FALSE(he.MetadataKeyPresent("missing"));

        const auto val = he.GetMetadataValueAs<int32_t>("count");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == 42);
    }

    SUBCASE("vertex_normals and vertex_colours preserved in conversion"){
        auto fvsm = make_tetrahedron();
        fvsm.compute_vertex_normals();

        fvsm.vertex_colours.resize(fvsm.vertices.size());
        fvsm.vertex_colours[0] = 0xFF000000U;
        fvsm.vertex_colours[1] = 0x00FF0000U;
        fvsm.vertex_colours[2] = 0x0000FF00U;
        fvsm.vertex_colours[3] = 0x000000FFU;

        auto he = convert_fv_to_he_surface_mesh(fvsm);

        REQUIRE(he.vertex_normals.size() == fvsm.vertex_normals.size());
        REQUIRE(he.vertex_colours.size() == fvsm.vertex_colours.size());

        for(size_t i = 0; i < fvsm.vertex_normals.size(); ++i){
            REQUIRE(he.vertex_normals[i] == fvsm.vertex_normals[i]);
        }
        for(size_t i = 0; i < fvsm.vertex_colours.size(); ++i){
            REQUIRE(he.vertex_colours[i] == fvsm.vertex_colours[i]);
        }

        // And back.
        auto fvsm_rt = he.convert_to_fv_surface_mesh();
        REQUIRE(fvsm_rt.vertex_normals == fvsm.vertex_normals);
        REQUIRE(fvsm_rt.vertex_colours == fvsm.vertex_colours);
    }

    SUBCASE("Euler formula for closed mesh: V - E + F == 2"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        const auto V = static_cast<int64_t>(he.num_vertices());
        const auto E = static_cast<int64_t>(he.num_edges());
        const auto F = static_cast<int64_t>(he.num_faces());
        REQUIRE((V - E + F) == 2);
    }

    SUBCASE("half-edge next/prev cycle consistency"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        for(uint64_t i = 0; i < he.num_halfedges(); ++i){
            const auto &h = he.halfedges[i];
            // next of prev should be self.
            REQUIRE(he.halfedges[h.prev].next == i);
            // prev of next should be self.
            REQUIRE(he.halfedges[h.next].prev == i);
        }
    }

    SUBCASE("half-edge twin consistency"){
        auto fvsm = make_tetrahedron();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        for(uint64_t i = 0; i < he.num_halfedges(); ++i){
            const auto &h = he.halfedges[i];
            if(h.twin != he_surface_mesh<double, uint64_t>::sentinel){
                // Twin of twin should be self.
                REQUIRE(he.halfedges[h.twin].twin == i);
                // Twin should connect opposite vertices.
                const auto v_from = h.vertex;
                const auto v_to   = he.halfedges[h.next].vertex;
                const auto tw_from = he.halfedges[h.twin].vertex;
                const auto tw_to   = he.halfedges[he.halfedges[h.twin].next].vertex;
                REQUIRE(v_from == tw_to);
                REQUIRE(v_to   == tw_from);
            }
        }
    }

    SUBCASE("is_boundary_halfedge"){
        auto fvsm = make_single_triangle();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        for(uint64_t i = 0; i < he.num_halfedges(); ++i){
            REQUIRE(he.is_boundary_halfedge(i));
        }

        auto fvsm2 = make_tetrahedron();
        auto he2 = convert_fv_to_he_surface_mesh(fvsm2);
        for(uint64_t i = 0; i < he2.num_halfedges(); ++i){
            REQUIRE_FALSE(he2.is_boundary_halfedge(i));
        }
    }

    SUBCASE("float specialization compiles and works"){
        fv_surface_mesh<float, uint32_t> fvsm;
        fvsm.vertices = {
            vec3<float>(0.0f, 0.0f, 0.0f),
            vec3<float>(1.0f, 0.0f, 0.0f),
            vec3<float>(0.0f, 1.0f, 0.0f),
            vec3<float>(0.0f, 0.0f, 1.0f)
        };
        fvsm.faces = {
            {0U, 2U, 1U},
            {0U, 1U, 3U},
            {1U, 2U, 3U},
            {0U, 3U, 2U}
        };

        auto he = convert_fv_to_he_surface_mesh(fvsm);
        REQUIRE(he.num_vertices()  == 4U);
        REQUIRE(he.num_faces()     == 4U);
        REQUIRE(he.num_halfedges() == 12U);
        REQUIRE(he.num_edges()     == 6U);

        auto fvsm_rt = he.convert_to_fv_surface_mesh();
        REQUIRE(fvsm_rt.vertices.size() == 4U);
        REQUIRE(fvsm_rt.faces.size()    == 4U);
    }

    SUBCASE("vertex_faces and vertex_neighbours for boundary vertex"){
        auto fvsm = make_two_triangles();
        auto he = convert_fv_to_he_surface_mesh(fvsm);

        // Vertices 0 and 1 are each shared by 2 faces.
        for(uint64_t v : {0UL, 1UL}){
            const auto faces = he.vertex_faces(v);
            REQUIRE(faces.size() == 2UL);
        }

        // Vertices 2 and 3 are each in 1 face.
        for(uint64_t v : {2UL, 3UL}){
            const auto faces = he.vertex_faces(v);
            REQUIRE(faces.size() == 1UL);
        }
    }

    SUBCASE("non-manifold edge throws"){
        fv_surface_mesh<double, uint64_t> fvsm;
        fvsm.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.5, 1.0, 0.0),
            vec3<double>(0.5, -1.0, 0.0)
        };
        // Two faces with the same directed edge (0 -> 1).
        fvsm.faces = {
            {0, 1, 2},
            {0, 1, 3}
        };
        REQUIRE_THROWS(convert_fv_to_he_surface_mesh(fvsm));
    }

    SUBCASE("empty mesh converts without error"){
        fv_surface_mesh<double, uint64_t> fvsm;
        auto he = convert_fv_to_he_surface_mesh(fvsm);
        REQUIRE(he.num_vertices()  == 0UL);
        REQUIRE(he.num_faces()     == 0UL);
        REQUIRE(he.num_halfedges() == 0UL);

        auto fvsm_rt = he.convert_to_fv_surface_mesh();
        REQUIRE(fvsm_rt.vertices.empty());
        REQUIRE(fvsm_rt.faces.empty());
    }
}
