
#include <limits>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "fv_surface_mesh constructors" ){
    fv_surface_mesh<double, uint32_t> mesh1;
    fv_surface_mesh<double, uint32_t> mesh2;

    SUBCASE("default constructor gives an empty mesh"){
        REQUIRE( mesh1.vertices.size() == 0 );
        REQUIRE( mesh1.faces.size() == 0 );

        REQUIRE( mesh1.vertices == mesh2.vertices );
        REQUIRE( mesh1.faces == mesh2.faces );
    }
}

TEST_CASE( "fv_surface_mesh member functions" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    const vec3<double> p1(1.0, 0.0, 0.0);
    const vec3<double> p2(0.0, 1.0, 0.0);
    const vec3<double> p3(0.0, 0.0, 1.0);
    const vec3<double> p4(0.0, 0.0, 0.0);
    const vec3<double> p5(1.0, 0.0, 1.0);
    const vec3<double> p6(1.0, 1.0, 0.0);

    fv_surface_mesh<double, uint32_t> mesh1;
    mesh1.vertices = {{ p1, p3, p4 }};
    mesh1.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

    SUBCASE("operator=="){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};
        REQUIRE( mesh1 == mesh2 );

        // Member 'involved_faces' should not impact equality since it is a derived structure that may be in an
        // indeterminate state (i.e., it can be generated on-demand whenever needed, and may not have been generated
        // recently).
        mesh1.involved_faces.clear();
        mesh2.recreate_involved_face_index();
        REQUIRE( mesh1 == mesh2 );
        mesh2.involved_faces.clear();

        // Metadata *is* significant for equality.
        mesh2.metadata["new key"] = "new value";
        REQUIRE( mesh1 != mesh2 );
    }

    SUBCASE("surface_area"){
        REQUIRE( mesh1.surface_area() == 0.5 );
    }

    SUBCASE("recreate_involved_face_index"){
        REQUIRE( mesh1.involved_faces.size() == 0 );
        mesh1.recreate_involved_face_index();
        REQUIRE( mesh1.involved_faces.size() == 3 );
    }

    SUBCASE("merge_duplicate_vertices"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p1, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

        REQUIRE( mesh2.vertices.size() == 3 );
        REQUIRE( mesh2.faces.size() == 1 );
        mesh2.merge_duplicate_vertices();
        REQUIRE( mesh2.vertices.size() == 2 );
        REQUIRE( mesh2.faces.size() == 0 );  // Degenerate case where the face collapses.

        fv_surface_mesh<double, uint32_t> mesh3;
        mesh3.vertices = {{ p1, p1, p4, p5 }};
        mesh3.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2), static_cast<uint32_t>(3) }};

        REQUIRE( mesh3.vertices.size() == 4 );
        REQUIRE( mesh3.faces.size() == 1 );
        mesh3.merge_duplicate_vertices();
        REQUIRE( mesh3.vertices.size() == 3 );
        REQUIRE( mesh3.faces.size() == 1 );   // Facet still has non-zero area.
    }

    SUBCASE("convert_to_triangles"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4, p5 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2), static_cast<uint32_t>(3) }};

        REQUIRE( mesh2.vertices.size() == 4 );
        REQUIRE( mesh2.faces.size() == 1 );
        mesh2.convert_to_triangles();
        REQUIRE( mesh2.vertices.size() == 4 );
        REQUIRE( mesh2.faces.size() == 2 );
    }

    SUBCASE("remove_disconnected_vertices"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4, p5 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

        REQUIRE( mesh2.vertices.size() == 4 );
        REQUIRE( mesh2.faces.size() == 1 );
        mesh2.remove_disconnected_vertices();
        REQUIRE( mesh2.vertices.size() == 3 );
        REQUIRE( mesh2.faces.size() == 1 );
    }

    SUBCASE("sample_surface_randomly"){
        const long int random_seed = 123456;

        SUBCASE("degenerate surface (single-vertex)"){
            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ p1 }};
            mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(0), static_cast<uint32_t>(0) }};

            double surface_area_per_sample = 1.0;
            REQUIRE_THROWS(mesh2.sample_surface_randomly(surface_area_per_sample, random_seed));
        }

        SUBCASE("degenerate surface (single-edge)"){
            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ p1, p2 }};
            mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(0) }};

            double surface_area_per_sample = 1.0;
            REQUIRE_THROWS(mesh2.sample_surface_randomly(surface_area_per_sample, random_seed));
        }

        SUBCASE("degenerate surface (small face)"){
            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ p1, p2, p4 }};
            mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

            double surface_area_per_sample = 10.0; // Total surface area = 0.50.
            // NOTE: The following should *not* throw, since it might be valid to require 0 samples, but it might emit a
            // warning to stdout.
            point_set<double> ps = mesh2.sample_surface_randomly(surface_area_per_sample, random_seed);
            REQUIRE( ps.points.size() == 0 ); // Number of samples should be exact.
        }

        SUBCASE("single triangular facet"){
            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ p1, p2, p4 }};
            mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

            double surface_area_per_sample = 0.001; // Total surface area = 0.50.
            point_set<double> ps = mesh2.sample_surface_randomly(surface_area_per_sample, random_seed);
            REQUIRE( ps.points.size() == 500 ); // Number of samples should be exact.

            // Ensure all points are within the bounds of the face.
            plane<double> PA( (vec3<double>( 1.0,  0.0,  0.0)).unit(), vec3<double>(0.0 - eps, 0.0, 0.0) );
            plane<double> PB( (vec3<double>( 0.0,  1.0,  0.0)).unit(), vec3<double>(0.0, 0.0 - eps, 0.0) );
            plane<double> PC( (vec3<double>(-1.0, -1.0,  0.0)).unit(), vec3<double>(0.5 + eps, 0.5 + eps, 0.0) );
            plane<double> PD( (vec3<double>( 0.0,  0.0,  1.0)).unit(), vec3<double>(0.0, 0.0, 0.0 - eps) );
            plane<double> PE( (vec3<double>( 0.0,  0.0, -1.0)).unit(), vec3<double>(0.0, 0.0, 0.0 + eps) );
            for(const auto& p : ps.points){
                REQUIRE(PA.Is_Point_Above_Plane(p));
                REQUIRE(PB.Is_Point_Above_Plane(p));
                REQUIRE(PC.Is_Point_Above_Plane(p));
                REQUIRE(PD.Is_Point_Above_Plane(p));
                REQUIRE(PE.Is_Point_Above_Plane(p));
            }
        }

        SUBCASE("two equal-area triangular facets"){
            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ p1, p2, p4, p6 }};
            mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                           { static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(3) }};

            double surface_area_per_sample = 0.001; // Total surface area = 1.0.
            point_set<double> ps = mesh2.sample_surface_randomly(surface_area_per_sample, random_seed);
            REQUIRE( ps.points.size() == 1000 ); // Number of samples should be exact.

            // Ensure all points are within the bounds of the faces.
            plane<double> PA( (vec3<double>( 1.0,  0.0,  0.0)).unit(), vec3<double>(0.0 - eps, 0.0, 0.0) );
            plane<double> PB( (vec3<double>( 0.0,  1.0,  0.0)).unit(), vec3<double>(0.0, 0.0 - eps, 0.0) );
            plane<double> PC( (vec3<double>(-1.0,  0.0,  0.0)).unit(), vec3<double>(1.0 + eps, 0.0, 0.0) );
            plane<double> PD( (vec3<double>( 0.0, -1.0,  0.0)).unit(), vec3<double>(0.0, 1.0 - eps, 0.0) );
            plane<double> PE( (vec3<double>( 0.0,  0.0,  1.0)).unit(), vec3<double>(0.0, 0.0, 0.0 - eps) );
            plane<double> PF( (vec3<double>( 0.0,  0.0, -1.0)).unit(), vec3<double>(0.0, 0.0, 0.0 + eps) );
            for(const auto& p : ps.points){
                REQUIRE(PA.Is_Point_Above_Plane(p));
                REQUIRE(PB.Is_Point_Above_Plane(p));
                REQUIRE(PC.Is_Point_Above_Plane(p));
                REQUIRE(PD.Is_Point_Above_Plane(p));
                REQUIRE(PE.Is_Point_Above_Plane(p));
                REQUIRE(PF.Is_Point_Above_Plane(p));
            }

            // Ensure both faces are being sampled by counting the number of samples within each face.
            //
            // Note: The following is a statistical check, so it might fail.
            //       Both faces have equal area, so each has equal likelihood of being selected (~coin flip).
            //       The binomial equation provides likelihood estimates.
            //       The std. dev. of the binomial eqn. in this case is sqrt(1000 * 0.5 * (1.0 - 0.5)) ~= 15.8.
            //       So we should expect at least 400 samples in each triangle, since it is more than 6 std. dev.s away
            //       from the mean, though it *is* possible that this will (extremely rarely) fail.
            plane<double> PG( (vec3<double>(-1.0, -1.0,  0.0)).unit(), vec3<double>(0.5 + eps, 0.5 + eps, 0.0) );
            size_t N = 0;
            for(const auto& p : ps.points){
                N += (PG.Is_Point_Above_Plane(p)) ? 0 : 1;
            }
            const size_t diff = (N < 500) ? (500 - N) : (N - 500);
            INFO("Samples in the lower triangle = " << N);
            REQUIRE(diff < 400); // ~25 std. dev.s. If encountered, this almost certainly indicates bias.
            REQUIRE(diff < 300); // ~19 std. dev.s. If encountered, this almost certainly indicates bias.
            REQUIRE(diff < 200); // ~12 std. dev.s. If encountered, this almost certainly indicates bias.
            REQUIRE(diff < 100); // ~6 std. dev.s. If encountered, this could indicate bias.
            WARN_MESSAGE(diff < 48, "3 std. dev. statistical test failed");
            WARN_MESSAGE(diff < 32, "2 std. dev. statistical test failed");
            WARN_MESSAGE(diff < 16, "1 std. dev. statistical test failed");
        }
    }
}

