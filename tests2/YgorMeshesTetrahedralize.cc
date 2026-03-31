//YgorMeshesTetrahedralize.cc - Tests for tetrahedral mesh generation from surface meshes.
//

#include <cmath>
#include <sstream>
#include <string>
#include <set>
#include <stdexcept>

#include <YgorMath.h>
#include <YgorMath_Samples.h>
#include <YgorMisc.h>
#include <YgorLog.h>
#include <YgorMeshesTetrahedralize.h>

#include "doctest/doctest.h"


// Helper: compute the signed volume of a single tetrahedron.
template <class T, class I>
static T tet_signed_volume(const fv_tet_mesh<T, I> &mesh, size_t ti){
    const auto &tet = mesh.tetrahedra.at(ti);
    const auto &a = mesh.vertices.at(tet[0]);
    const auto &b = mesh.vertices.at(tet[1]);
    const auto &c = mesh.vertices.at(tet[2]);
    const auto &d = mesh.vertices.at(tet[3]);
    return (b - a).Dot( (c - a).Cross(d - a) ) / static_cast<T>(6);
}


// Helper: verify basic tet mesh integrity.
template <class T, class I>
static bool verify_tet_mesh_integrity(const fv_tet_mesh<T, I> &mesh){
    const auto N_v = mesh.vertices.size();
    for(const auto &tet : mesh.tetrahedra){
        std::set<I> vis;
        for(const auto &idx : tet){
            if(static_cast<size_t>(idx) >= N_v) return false;
            vis.insert(idx);
        }
        // Each tet must have 4 distinct vertices.
        if(vis.size() != 4) return false;
    }
    return true;
}


// Helper: compute tet quality using the normalized mean-ratio metric:
//   Q = 12 * (3V)^(2/3) / (sum of squared edge lengths).
// Returns a value in (0, 1], where 1 is a regular tetrahedron.
template <class T, class I>
static T tet_quality(const fv_tet_mesh<T, I> &mesh, size_t ti){
    const auto &tet = mesh.tetrahedra.at(ti);
    const auto &a = mesh.vertices.at(tet[0]);
    const auto &b = mesh.vertices.at(tet[1]);
    const auto &c = mesh.vertices.at(tet[2]);
    const auto &d = mesh.vertices.at(tet[3]);

    // Edge lengths.
    const T lab = (b - a).length();
    const T lac = (c - a).length();
    const T lad = (d - a).length();
    const T lbc = (c - b).length();
    const T lbd = (d - b).length();
    const T lcd = (d - c).length();

    // Volume.
    const T vol = std::abs((b - a).Dot( (c - a).Cross(d - a) )) / static_cast<T>(6);
    if(vol < std::numeric_limits<T>::epsilon()) return static_cast<T>(0);

    // Surface area of the 4 faces.
    const T s1 = (b - a).Cross(c - a).length() * static_cast<T>(0.5);
    const T s2 = (b - a).Cross(d - a).length() * static_cast<T>(0.5);
    const T s3 = (c - a).Cross(d - a).length() * static_cast<T>(0.5);
    const T s4 = (c - b).Cross(d - b).length() * static_cast<T>(0.5);
    const T total_area = s1 + s2 + s3 + s4;

    // Insphere radius: r = 3V / A.
    const T r_in = static_cast<T>(3) * vol / total_area;

    // Circumsphere radius approximation using edge lengths.
    // For the exact formula: R = (abc) / (8V) where a,b,c are areas... but simpler:
    // Use the quality measure: Q = 12 * (3*V)^(2/3) / (sum of squared edge lengths).
    // This is a well-known FEM quality metric normalized to [0,1] (1 = regular tet).
    const T sum_sq_edges = lab*lab + lac*lac + lad*lad + lbc*lbc + lbd*lbd + lcd*lcd;
    const T cbrt_vol = std::cbrt(static_cast<T>(3) * vol);
    const T quality = static_cast<T>(12) * cbrt_vol * cbrt_vol / sum_sq_edges;

    return quality;
}


// =================================================================
// Tests for fv_tet_mesh class.
// =================================================================
TEST_CASE( "fv_tet_mesh basic operations" ){

    SUBCASE("default construction produces empty mesh"){
        fv_tet_mesh<double, uint64_t> mesh;
        REQUIRE( mesh.vertices.empty() );
        REQUIRE( mesh.tetrahedra.empty() );
        REQUIRE( mesh.metadata.empty() );
    }

    SUBCASE("copy construction preserves all data"){
        fv_tet_mesh<double, uint64_t> m1;
        m1.vertices.push_back( vec3<double>(0.0, 0.0, 0.0) );
        m1.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        m1.vertices.push_back( vec3<double>(0.0, 1.0, 0.0) );
        m1.vertices.push_back( vec3<double>(0.0, 0.0, 1.0) );
        m1.tetrahedra.push_back({{0, 1, 2, 3}});
        m1.metadata["key"] = "val";

        fv_tet_mesh<double, uint64_t> m2(m1);
        REQUIRE( m2.vertices.size() == 4 );
        REQUIRE( m2.tetrahedra.size() == 1 );
        REQUIRE( m2.metadata.at("key") == "val" );
        REQUIRE( m2 == m1 );
    }

    SUBCASE("assignment operator"){
        fv_tet_mesh<double, uint64_t> m1;
        m1.vertices.push_back( vec3<double>(0.0, 0.0, 0.0) );
        m1.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        m1.vertices.push_back( vec3<double>(0.0, 1.0, 0.0) );
        m1.vertices.push_back( vec3<double>(0.0, 0.0, 1.0) );
        m1.tetrahedra.push_back({{0, 1, 2, 3}});

        fv_tet_mesh<double, uint64_t> m2;
        m2 = m1;
        REQUIRE( m2 == m1 );
    }

    SUBCASE("equality and inequality operators"){
        fv_tet_mesh<double, uint64_t> m1, m2;
        REQUIRE( m1 == m2 );
        REQUIRE_FALSE( m1 != m2 );

        m1.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        REQUIRE( m1 != m2 );
    }

    SUBCASE("swap"){
        fv_tet_mesh<double, uint64_t> m1, m2;
        m1.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        m2.metadata["k"] = "v";

        m1.swap(m2);
        REQUIRE( m1.vertices.empty() );
        REQUIRE( m1.metadata.at("k") == "v" );
        REQUIRE( m2.vertices.size() == 1 );
        REQUIRE( m2.metadata.empty() );
    }

    SUBCASE("volume of a single tet"){
        fv_tet_mesh<double, uint64_t> mesh;
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 1.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 1.0) );
        mesh.tetrahedra.push_back({{0, 1, 2, 3}});

        // Expected volume = 1/6.
        const auto vol = mesh.volume();
        REQUIRE( std::abs(vol - 1.0/6.0) < 1.0e-10 );
    }

    SUBCASE("volume of a specific tet by index"){
        fv_tet_mesh<double, uint64_t> mesh;
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(2.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 2.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 2.0) );
        mesh.tetrahedra.push_back({{0, 1, 2, 3}});

        // Expected volume = 8/6 = 4/3.
        const auto vol = mesh.volume(0);
        REQUIRE( std::abs(vol - 4.0/3.0) < 1.0e-10 );
    }

    SUBCASE("volume throws for invalid index"){
        fv_tet_mesh<double, uint64_t> mesh;
        REQUIRE_THROWS_AS( mesh.volume(0), std::invalid_argument );
    }

    SUBCASE("remove_disconnected_vertices"){
        fv_tet_mesh<double, uint64_t> mesh;
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(99.0, 99.0, 99.0) ); // Disconnected.
        mesh.vertices.push_back( vec3<double>(0.0, 1.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 1.0) );
        mesh.tetrahedra.push_back({{0, 1, 3, 4}});

        mesh.remove_disconnected_vertices();
        REQUIRE( mesh.vertices.size() == 4 );
        REQUIRE( mesh.tetrahedra.size() == 1 );
        REQUIRE( verify_tet_mesh_integrity(mesh) );
    }

    SUBCASE("merge_duplicate_vertices"){
        fv_tet_mesh<double, uint64_t> mesh;
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(1.0, 0.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 1.0, 0.0) );
        mesh.vertices.push_back( vec3<double>(0.0, 0.0, 1.0) );
        // Add a near-duplicate of vertex 0.
        mesh.vertices.push_back( vec3<double>(1.0e-10, 0.0, 0.0) );
        mesh.tetrahedra.push_back({{0, 1, 2, 3}});
        mesh.tetrahedra.push_back({{4, 1, 2, 3}}); // Uses the duplicate.

        mesh.merge_duplicate_vertices(1.0e-6);
        REQUIRE( mesh.vertices.size() == 4 );
        // Both tets should now reference the same vertex.
        REQUIRE( mesh.tetrahedra[0][0] == mesh.tetrahedra[1][0] );
    }

    SUBCASE("MetadataKeyPresent and GetMetadataValueAs"){
        fv_tet_mesh<double, uint64_t> mesh;
        mesh.metadata["answer"] = "42";

        REQUIRE( mesh.MetadataKeyPresent("answer") );
        REQUIRE_FALSE( mesh.MetadataKeyPresent("missing") );

        const auto val = mesh.GetMetadataValueAs<double>("answer");
        REQUIRE( val.has_value() );
        REQUIRE( std::abs(val.value() - 42.0) < 1.0e-10 );
    }
}


// =================================================================
// Tests for tetrahedral_mesh_from_surface_mesh.
// =================================================================
TEST_CASE( "tetrahedral_mesh_from_surface_mesh" ){

    SUBCASE("basic tetrahedron: produces non-empty conforming mesh"){
        const auto surface = fv_surface_mesh_tetrahedron();
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 3);

        REQUIRE( !tet_mesh.vertices.empty() );
        REQUIRE( !tet_mesh.tetrahedra.empty() );
        REQUIRE( verify_tet_mesh_integrity(tet_mesh) );
    }

    SUBCASE("icosahedron: produces non-empty conforming mesh"){
        const auto surface = fv_surface_mesh_icosahedron();
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 3);

        REQUIRE( !tet_mesh.vertices.empty() );
        REQUIRE( !tet_mesh.tetrahedra.empty() );
        REQUIRE( verify_tet_mesh_integrity(tet_mesh) );
    }

    SUBCASE("octahedron: produces non-empty conforming mesh"){
        const auto surface = fv_surface_mesh_octahedron();
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 3);

        REQUIRE( !tet_mesh.vertices.empty() );
        REQUIRE( !tet_mesh.tetrahedra.empty() );
        REQUIRE( verify_tet_mesh_integrity(tet_mesh) );
    }

    SUBCASE("increasing depth increases mesh resolution"){
        const auto surface = fv_surface_mesh_icosahedron();
        const auto tm_lo = tetrahedral_mesh_from_surface_mesh(surface, 2);
        const auto tm_hi = tetrahedral_mesh_from_surface_mesh(surface, 3);

        REQUIRE( tm_hi.tetrahedra.size() > tm_lo.tetrahedra.size() );
        REQUIRE( tm_hi.vertices.size() > tm_lo.vertices.size() );
    }

    SUBCASE("total volume is positive and approximates analytic volume"){
        // The icosahedron from samples has edge length a = 2.
        // Volume of a regular icosahedron: V = (5/12)(3 + sqrt(5)) * a^3.
        const auto surface = fv_surface_mesh_icosahedron();
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 4);

        const auto total_vol = tet_mesh.volume();
        REQUIRE( total_vol > 0.0 );

        const double a = 2.0;
        const double expected_vol = (5.0 / 12.0) * (3.0 + std::sqrt(5.0)) * a * a * a;
        // The tet mesh volume should approximate this. With octree discretization
        // it will be approximate. Check it's within 25% of the expected value.
        REQUIRE( total_vol > expected_vol * 0.75 );
        REQUIRE( total_vol < expected_vol * 1.25 );
    }

    SUBCASE("all tetrahedra have non-zero volume"){
        const auto surface = fv_surface_mesh_icosahedron();
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 3);

        for(size_t i = 0; i < tet_mesh.tetrahedra.size(); ++i){
            const auto vol = std::abs(tet_signed_volume(tet_mesh, i));
            REQUIRE( vol > 0.0 );
        }
    }

    SUBCASE("tet quality suitable for FEM (quality metric above threshold)"){
        const auto surface = fv_surface_mesh_icosahedron();
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 3);

        // Check that the minimum quality is above a threshold.
        // A quality > 0.05 is considered acceptable for FEM in this test.
        double min_quality = 1.0;
        for(size_t i = 0; i < tet_mesh.tetrahedra.size(); ++i){
            const auto q = tet_quality(tet_mesh, i);
            min_quality = std::min(min_quality, static_cast<double>(q));
        }

        // The body-centred decomposition of cubes yields tets with known quality bounds.
        REQUIRE( min_quality > 0.05 );
    }

    SUBCASE("invalid max_depth throws"){
        const auto surface = fv_surface_mesh_icosahedron();
        REQUIRE_THROWS_AS( tetrahedral_mesh_from_surface_mesh(surface, 0), std::invalid_argument );
        REQUIRE_THROWS_AS( tetrahedral_mesh_from_surface_mesh(surface, 11), std::invalid_argument );
    }

    SUBCASE("negative boundary_scale throws"){
        const auto surface = fv_surface_mesh_icosahedron();
        REQUIRE_THROWS_AS( tetrahedral_mesh_from_surface_mesh(surface, 3, -1.0), std::invalid_argument );
    }

    SUBCASE("empty surface mesh throws"){
        fv_surface_mesh<double, uint64_t> empty_surface;
        REQUIRE_THROWS_AS( tetrahedral_mesh_from_surface_mesh(empty_surface, 3), std::invalid_argument );
    }

    SUBCASE("mesh with quads is handled (auto-triangulated)"){
        auto surface = fv_surface_mesh_icosahedron();
        // Add a quad face (will be auto-triangulated).
        if(surface.vertices.size() >= 4){
            surface.faces.push_back({0, 1, 2, 3});
        }
        // Should not throw; quads are triangulated internally.
        const auto tet_mesh = tetrahedral_mesh_from_surface_mesh(surface, 3);
        REQUIRE( !tet_mesh.tetrahedra.empty() );
    }
}
