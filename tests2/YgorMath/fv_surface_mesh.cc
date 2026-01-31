
#include <limits>
#include <utility>
#include <iostream>
#include <random>
#include <cstdint>
#include <algorithm>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "fv_surface_mesh class" ){
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

    SUBCASE("default constructor gives an empty mesh"){
        fv_surface_mesh<double, uint32_t> mesh1;
        fv_surface_mesh<double, uint32_t> mesh2;

        REQUIRE( mesh1.vertices.size() == 0 );
        REQUIRE( mesh1.vertex_normals.size() == 0 );
        REQUIRE( mesh1.vertex_colours.size() == 0 );
        REQUIRE( mesh1.faces.size() == 0 );

        REQUIRE( mesh1.vertices == mesh2.vertices );
        REQUIRE( mesh1.vertex_normals == mesh2.vertex_normals );
        REQUIRE( mesh1.vertex_colours == mesh2.vertex_colours );
        REQUIRE( mesh1.faces == mesh2.faces );
    }

    SUBCASE("copy constructor and operator="){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4, p5 }};
        mesh2.vertex_normals = {{ p1, p2, p3, p4 }};
        mesh2.vertex_colours = {{ static_cast<uint32_t>(123456789),
                                  static_cast<uint32_t>(234567890),
                                  static_cast<uint32_t>(345678901),
                                  static_cast<uint32_t>(456789012) }};
        mesh2.metadata["new key"] = "new value";
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                       { static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(3) }};
        mesh2.recreate_involved_face_index();

        fv_surface_mesh<double, uint32_t> mesh3(mesh2);
        fv_surface_mesh<double, uint32_t> mesh4 = mesh2;
        REQUIRE(mesh2 == mesh2);
        REQUIRE(mesh3 == mesh2);
        REQUIRE(mesh4 == mesh2);
        REQUIRE(mesh4 == mesh3);
    }

    SUBCASE("operator== and operator!="){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};
        REQUIRE( mesh1 == mesh1 );
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

    SUBCASE("swap"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p2, p3 }};
        mesh2.faces = {{ static_cast<uint32_t>(2), static_cast<uint32_t>(1), static_cast<uint32_t>(0) }};

        fv_surface_mesh<double, uint32_t> mesh3 = mesh1;
        fv_surface_mesh<double, uint32_t> mesh4 = mesh2;

        REQUIRE( mesh1 == mesh3 );
        REQUIRE( mesh2 == mesh4 );
        REQUIRE( mesh1 != mesh4 );
        REQUIRE( mesh2 != mesh3 );
        mesh3.swap(mesh4);
        REQUIRE( mesh1 != mesh3 );
        REQUIRE( mesh2 != mesh4 );
        REQUIRE( mesh1 == mesh4 );
        REQUIRE( mesh2 == mesh3 );
    }

    SUBCASE("pack_RGBA32_colour and unpack_RGBA32_colour"){
        fv_surface_mesh<double, uint32_t> mesh2;

        const auto R1 = static_cast<uint8_t>(0x12);
        const auto G1 = static_cast<uint8_t>(0x34);
        const auto B1 = static_cast<uint8_t>(0x56);
        const auto A1 = static_cast<uint8_t>(0x78);

        const auto p1 = mesh2.pack_RGBA32_colour( {{ R1, G1, B1, A1 }} );
        REQUIRE(p1 == static_cast<uint32_t>(0x12345678));

        const auto u1 = mesh2.unpack_RGBA32_colour(p1);
        REQUIRE( u1.at(0) == R1 );
        REQUIRE( u1.at(1) == G1 );
        REQUIRE( u1.at(2) == B1 );
        REQUIRE( u1.at(3) == A1 );
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
        mesh2.vertex_normals = {{ p1, p2, p3 }};
        mesh2.vertex_colours = {{ static_cast<uint32_t>(123456789),
                                  static_cast<uint32_t>(234567890),
                                  static_cast<uint32_t>(345678901) }};
        mesh2.faces = {{ static_cast<uint32_t>(0),
                         static_cast<uint32_t>(1),
                         static_cast<uint32_t>(2) }};

        REQUIRE( mesh2.vertices.size() == 3 );
        REQUIRE( mesh2.vertex_normals.size() == 3 );
        REQUIRE( mesh2.vertex_colours.size() == 3 );
        REQUIRE( mesh2.faces.size() == 1 );
        mesh2.merge_duplicate_vertices();
        REQUIRE( mesh2.vertices.size() == 2 );
        REQUIRE( mesh2.vertex_normals.size() == 2 );
        REQUIRE( mesh2.vertex_colours.size() == 2 );
        REQUIRE( mesh2.faces.size() == 0 );  // Degenerate case where the face collapses.

        fv_surface_mesh<double, uint32_t> mesh3;
        mesh3.vertices = {{ p1, p1, p4, p5 }};
        mesh3.vertex_normals = {{ p1, p2, p3, p4 }};
        mesh3.vertex_colours = {{ static_cast<uint32_t>(123456789),
                                  static_cast<uint32_t>(234567890),
                                  static_cast<uint32_t>(345678901),
                                  static_cast<uint32_t>(456789012) }};
        mesh3.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2), static_cast<uint32_t>(3) }};

        REQUIRE( mesh3.vertices.size() == 4 );
        REQUIRE( mesh3.vertex_normals.size() == 4 );
        REQUIRE( mesh3.vertex_colours.size() == 4 );
        REQUIRE( mesh3.faces.size() == 1 );
        mesh3.merge_duplicate_vertices();
        REQUIRE( mesh3.vertices.size() == 3 );
        REQUIRE( mesh3.vertex_normals.size() == 3 );
        REQUIRE( mesh3.vertex_colours.size() == 3 );
        REQUIRE( mesh3.faces.size() == 1 );   // Facet still has non-zero area.

        SUBCASE("throws when vertex normals are inconsistent"){
            mesh3.vertex_normals.push_back(p1);
            REQUIRE_THROWS( mesh3.merge_duplicate_vertices() );

            mesh3.vertex_normals.pop_back();
            mesh3.vertex_normals.pop_back();
            REQUIRE_THROWS( mesh3.merge_duplicate_vertices() );
        }

        SUBCASE("throws when vertex colours are inconsistent"){
            mesh3.vertex_colours.push_back(static_cast<uint32_t>(123456789));
            REQUIRE_THROWS( mesh3.merge_duplicate_vertices() );

            mesh3.vertex_colours.pop_back();
            mesh3.vertex_colours.pop_back();
            REQUIRE_THROWS( mesh3.merge_duplicate_vertices() );
        }

        SUBCASE("can proceed when attributes are missing"){
            fv_surface_mesh<double, uint32_t> mesh4;
            mesh4.vertices = {{ p1, p1, p4, p5 }};
            mesh4.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2), static_cast<uint32_t>(3) }};

            REQUIRE( mesh4.vertices.size() == 4 );
            REQUIRE( mesh4.vertex_normals.size() == 0 );
            REQUIRE( mesh4.vertex_colours.size() == 0 );
            REQUIRE( mesh4.faces.size() == 1 );
            mesh4.merge_duplicate_vertices();
            REQUIRE( mesh4.vertices.size() == 3 );
            REQUIRE( mesh4.vertex_normals.size() == 0 );
            REQUIRE( mesh4.vertex_colours.size() == 0 );
            REQUIRE( mesh4.faces.size() == 1 );   // Facet still has non-zero area.
        }
    }

    SUBCASE("convert_to_triangles"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4, p5 }};
        mesh2.vertex_normals = {{ p1, p2, p3, p4 }};
        mesh2.vertex_colours = {{ static_cast<uint32_t>(123456789),
                                  static_cast<uint32_t>(234567890),
                                  static_cast<uint32_t>(345678901),
                                  static_cast<uint32_t>(456789012) }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2), static_cast<uint32_t>(3) }};

        REQUIRE( mesh2.vertices.size() == 4 );
        REQUIRE( mesh2.vertex_normals.size() == 4 );
        REQUIRE( mesh2.vertex_colours.size() == 4 );
        REQUIRE( mesh2.faces.size() == 1 );
        mesh2.convert_to_triangles();
        REQUIRE( mesh2.vertices.size() == 4 );
        REQUIRE( mesh2.vertex_normals.size() == 4 );
        REQUIRE( mesh2.vertex_colours.size() == 4 );
        REQUIRE( mesh2.faces.size() == 2 );
    }

    SUBCASE("remove_disconnected_vertices"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4, p5 }};
        mesh2.vertex_normals = {{ p1, p2, p3, p4 }};
        mesh2.vertex_colours = {{ static_cast<uint32_t>(123456789),
                                  static_cast<uint32_t>(234567890),
                                  static_cast<uint32_t>(345678901),
                                  static_cast<uint32_t>(456789012) }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

        REQUIRE( mesh2.vertices.size() == 4 );
        REQUIRE( mesh2.vertex_normals.size() == 4 );
        REQUIRE( mesh2.vertex_colours.size() == 4 );
        REQUIRE( mesh2.faces.size() == 1 );
        mesh2.remove_disconnected_vertices();
        REQUIRE( mesh2.vertices.size() == 3 );
        REQUIRE( mesh2.vertex_normals.size() == 3 );
        REQUIRE( mesh2.vertex_colours.size() == 3 );
        REQUIRE( mesh2.faces.size() == 1 );

        SUBCASE("throws when vertex normals are inconsistent"){
            mesh2.vertex_normals.push_back(p1);
            REQUIRE_THROWS( mesh2.remove_disconnected_vertices() );

            mesh2.vertex_normals.pop_back();
            mesh2.vertex_normals.pop_back();
            REQUIRE_THROWS( mesh2.remove_disconnected_vertices() );
        }

        SUBCASE("throws when vertex colours are inconsistent"){
            mesh2.vertex_colours.push_back(static_cast<uint32_t>(123456789));
            REQUIRE_THROWS( mesh2.remove_disconnected_vertices() );

            mesh2.vertex_colours.pop_back();
            mesh2.vertex_colours.pop_back();
            REQUIRE_THROWS( mesh2.remove_disconnected_vertices() );
        }

        SUBCASE("can proceed when attributes are missing"){
            fv_surface_mesh<double, uint32_t> mesh4;
            mesh4.vertices = {{ p1, p1, p4, p5 }};
            mesh4.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) }};

            REQUIRE( mesh4.vertices.size() == 4 );
            REQUIRE( mesh4.vertex_normals.size() == 0 );
            REQUIRE( mesh4.vertex_colours.size() == 0 );
            REQUIRE( mesh4.faces.size() == 1 );
            mesh4.remove_disconnected_vertices();
            REQUIRE( mesh4.vertices.size() == 3 );
            REQUIRE( mesh4.vertex_normals.size() == 0 );
            REQUIRE( mesh4.vertex_colours.size() == 0 );
            REQUIRE( mesh4.faces.size() == 1 );   // Facet still has non-zero area.
        }
    }

    SUBCASE("sample_surface_randomly"){
        const int64_t random_seed = 123456;

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

    SUBCASE("convert_to_point_set"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p3, p4, p5 }};
        mesh2.vertex_normals = {{ p1, p2, p3, p4 }};
        mesh2.vertex_colours = {{ static_cast<uint32_t>(123456789),
                                  static_cast<uint32_t>(234567890),
                                  static_cast<uint32_t>(345678901),
                                  static_cast<uint32_t>(456789012) }};
        mesh2.metadata["new key"] = "new value";

        SUBCASE("throws if conversion would be lossy"){
            mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                           { static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(3) }};
            mesh2.recreate_involved_face_index();
            REQUIRE_THROWS(mesh2.convert_to_point_set());
        }

        SUBCASE("conversion is lossless and leaves mesh in default state"){
            fv_surface_mesh<double, uint32_t> mesh3 = mesh2;
            point_set<double> p = mesh3.convert_to_point_set();

            // The point_set should steal member data.
            REQUIRE(p.points == mesh2.vertices);
            REQUIRE(p.metadata == mesh2.metadata);
            REQUIRE(p.normals == mesh2.vertex_normals);
            REQUIRE(p.colours == mesh2.vertex_colours);

            // The mesh should be reset to default state.
            fv_surface_mesh<double, uint32_t> mesh4;
            REQUIRE(mesh3 == mesh4);
        }
    }

    SUBCASE("simplify_inner_triangles"){
        SUBCASE("cube with redundant middle surface vertices"){
            // Make a cube with a single vertex in the middle of each face (i.e., 6 verts) and all 8 corners.
            //
            //    c1                              c5
            //      o----------------------------o
            //      |`.                          |`.
            //      |  `.               m13      |  `.
            //      |    `.              x       |    `.
            //      |      `.                    |      `.
            //      |        `. c7               |        `. c6
            //      |          o----------------------------o
            //      |          |                 |          |
            //      |          |  m10            |          |
            //      |          |   x             |          |
            //      |          |                 |          |
            //      |    m08   |                 |   m09    |
            //      |     x    |                 |    x     |
            //      |          |                 |          |
            //      |          |            m11  |          |
            //      |          |             x   |          |
            //      |          |                 |          |
            //   c3 o----------|-----------------o c0       |
            //       `.        |                  `.        |
            //         `.      |                    `.      |
            //           `.    |         x            `.    |
            //             `.  |        m12             `.  |
            //               `.|                          `.|
            //                 o----------------------------o
            //               c2                              c4
            // 
            const vec3<double> c0(1.0, 0.0, 0.0);
            const vec3<double> c1(0.0, 1.0, 0.0);
            const vec3<double> c2(0.0, 0.0, 1.0);
            const vec3<double> c3(0.0, 0.0, 0.0);
            const vec3<double> c4(1.0, 0.0, 1.0);
            const vec3<double> c5(1.0, 1.0, 0.0);
            const vec3<double> c6(1.0, 1.0, 1.0);
            const vec3<double> c7(0.0, 1.0, 1.0);

            const vec3<double> m08 = (c1 + c2 + c3 + c7) * 0.25; // == m1237  
            const vec3<double> m09 = (c0 + c4 + c5 + c6) * 0.25; // == m0456 
            const vec3<double> m10 = (c0 + c1 + c3 + c5) * 0.25; // == m0135 
            const vec3<double> m11 = (c2 + c4 + c6 + c7) * 0.25; // == m2467 
            const vec3<double> m12 = (c0 + c2 + c3 + c4) * 0.25; // == m0234 
            const vec3<double> m13 = (c1 + c5 + c6 + c7) * 0.25; // == m1567 

            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ c0, c1, c2, c3, c4, c5, c6, c7,
                                m08, m09, m10, m11, m12, m13 }};
            mesh2.faces = {{ static_cast<uint32_t>(1), static_cast<uint32_t>(3), static_cast<uint32_t>(8) },
                           { static_cast<uint32_t>(3), static_cast<uint32_t>(2), static_cast<uint32_t>(8) },
                           { static_cast<uint32_t>(2), static_cast<uint32_t>(7), static_cast<uint32_t>(8) },
                           { static_cast<uint32_t>(7), static_cast<uint32_t>(1), static_cast<uint32_t>(8) },

                           { static_cast<uint32_t>(0), static_cast<uint32_t>(4), static_cast<uint32_t>(9) },
                           { static_cast<uint32_t>(4), static_cast<uint32_t>(6), static_cast<uint32_t>(9) },
                           { static_cast<uint32_t>(6), static_cast<uint32_t>(5), static_cast<uint32_t>(9) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(0), static_cast<uint32_t>(9) },

                           { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(10) },
                           { static_cast<uint32_t>(3), static_cast<uint32_t>(1), static_cast<uint32_t>(10) },
                           { static_cast<uint32_t>(1), static_cast<uint32_t>(5), static_cast<uint32_t>(10) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(0), static_cast<uint32_t>(10) },

                           { static_cast<uint32_t>(2), static_cast<uint32_t>(4), static_cast<uint32_t>(11) },
                           { static_cast<uint32_t>(4), static_cast<uint32_t>(6), static_cast<uint32_t>(11) },
                           { static_cast<uint32_t>(6), static_cast<uint32_t>(7), static_cast<uint32_t>(11) },
                           { static_cast<uint32_t>(7), static_cast<uint32_t>(2), static_cast<uint32_t>(11) },

                           { static_cast<uint32_t>(3), static_cast<uint32_t>(0), static_cast<uint32_t>(12) },
                           { static_cast<uint32_t>(0), static_cast<uint32_t>(4), static_cast<uint32_t>(12) },
                           { static_cast<uint32_t>(4), static_cast<uint32_t>(2), static_cast<uint32_t>(12) },
                           { static_cast<uint32_t>(2), static_cast<uint32_t>(3), static_cast<uint32_t>(12) },

                           { static_cast<uint32_t>(7), static_cast<uint32_t>(6), static_cast<uint32_t>(13) },
                           { static_cast<uint32_t>(6), static_cast<uint32_t>(5), static_cast<uint32_t>(13) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(1), static_cast<uint32_t>(13) },
                           { static_cast<uint32_t>(1), static_cast<uint32_t>(7), static_cast<uint32_t>(13) }};

            SUBCASE("with restrictive criteria, only flat, redundant faces are simplified"){
                const double dist_tolerance = 0.0001;
                const double min_angle = std::acos(0.95);

                // Each non-boundary vertex removal also removes two faces.
                const size_t expected_verts = mesh2.vertices.size() - 6UL;
                const size_t expected_faces = mesh2.faces.size() - 6UL * 2UL;

                mesh2.simplify_inner_triangles(dist_tolerance, min_angle);
                REQUIRE(mesh2.vertices.size() == expected_verts);
                REQUIRE(mesh2.faces.size() == expected_faces);
            }

            SUBCASE("nonmanifoldness is protected against when underconstrained"){
                const double dist_tolerance = 100.0;
                const double min_angle = std::acos(-0.8);

                // Should reduce down to a tetrahedron, but not become degenerate.
                const size_t expected_verts = 4UL;
                const size_t expected_faces = 4UL;

                mesh2.simplify_inner_triangles(dist_tolerance, min_angle);
                REQUIRE(mesh2.vertices.size() == expected_verts);
                REQUIRE(mesh2.faces.size() == expected_faces);
            }
        }

        SUBCASE("cube with only corner vertices"){
            // Make a cube with only 8 corner vertices.
            //
            //    c1                              c5
            //      o----------------------------o
            //      |`.                          |`.
            //      |  `.                        |  `.
            //      |    `.                      |    `.
            //      |      `.                    |      `.
            //      |        `. c7               |        `. c6
            //      |          o----------------------------o
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //      |          |                 |          |
            //   c3 o----------|-----------------o c0       |
            //       `.        |                  `.        |
            //         `.      |                    `.      |
            //           `.    |                      `.    |
            //             `.  |                        `.  |
            //               `.|                          `.|
            //                 o----------------------------o
            //               c2                              c4
            // 
            const vec3<double> c0(1.0, 0.0, 0.0);
            const vec3<double> c1(0.0, 1.0, 0.0);
            const vec3<double> c2(0.0, 0.0, 1.0);
            const vec3<double> c3(0.0, 0.0, 0.0);
            const vec3<double> c4(1.0, 0.0, 1.0);
            const vec3<double> c5(1.0, 1.0, 0.0);
            const vec3<double> c6(1.0, 1.0, 1.0);
            const vec3<double> c7(0.0, 1.0, 1.0);

            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ c0, c1, c2, c3, c4, c5, c6, c7 }};
            mesh2.faces = {{ static_cast<uint32_t>(3), static_cast<uint32_t>(2), static_cast<uint32_t>(7) },
                           { static_cast<uint32_t>(7), static_cast<uint32_t>(1), static_cast<uint32_t>(3) },

                           { static_cast<uint32_t>(2), static_cast<uint32_t>(4), static_cast<uint32_t>(6) },
                           { static_cast<uint32_t>(6), static_cast<uint32_t>(7), static_cast<uint32_t>(2) },

                           { static_cast<uint32_t>(4), static_cast<uint32_t>(0), static_cast<uint32_t>(5) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(6), static_cast<uint32_t>(4) },

                           { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1) },
                           { static_cast<uint32_t>(1), static_cast<uint32_t>(5), static_cast<uint32_t>(0) },

                           { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(2) },
                           { static_cast<uint32_t>(2), static_cast<uint32_t>(4), static_cast<uint32_t>(0) },

                           { static_cast<uint32_t>(7), static_cast<uint32_t>(6), static_cast<uint32_t>(5) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(1), static_cast<uint32_t>(7) }};

            SUBCASE("with restrictive criteria, corner vertices are not simplified"){
                const double dist_tolerance = 0.0001;
                const double min_angle = std::acos(0.95);

                // There should be no vertices that satisfy the simplification criteria in this case.
                const size_t expected_verts = mesh2.vertices.size();
                const size_t expected_faces = mesh2.faces.size();

                mesh2.simplify_inner_triangles(dist_tolerance, min_angle);
                REQUIRE(mesh2.vertices.size() == expected_verts);
                REQUIRE(mesh2.faces.size() == expected_faces);
            }

            SUBCASE("nonmanifoldness is protected against when underconstrained"){
                const double dist_tolerance = 100.0;
                const double min_angle = std::acos(-0.8);

                // Should reduce down to a tetrahedron, but not become degenerate.
                const size_t expected_verts = 4UL;
                const size_t expected_faces = 4UL;

                mesh2.simplify_inner_triangles(dist_tolerance, min_angle);
                REQUIRE(mesh2.vertices.size() == expected_verts);
                REQUIRE(mesh2.faces.size() == expected_faces);
            }
        }

        SUBCASE("cube with only corner vertices and a boundary"){
            // Make a cube with only 8 corner vertices.
            //
            //    c1                              c5
            //      o----------------------------o
            //      |`.                          |`.
            //      |  `.                        |  `.
            //      |    `.                      |    `.
            //      |      `.                    |      `.
            //      |        `. c7               |        `. c6
            //      |          o++++++++++++++++++++++++++++o
            //      |          ++++++++++++++++++++++++++++++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //      |          ++                |         ++
            //   c3 o----------++----------------o c0      ++
            //       `.        ++                 `.       ++
            //         `.      ++                   `.     ++
            //           `.    ++                     `.   ++
            //             `.  ++        (boundary)     `. ++
            //               `.++++++++++++++++++++++++++++++
            //                 o++++++++++++++++++++++++++++o
            //               c2                              c4
            // 
            const vec3<double> c0(1.0, 0.0, 0.0);
            const vec3<double> c1(0.0, 1.0, 0.0);
            const vec3<double> c2(0.0, 0.0, 1.0);
            const vec3<double> c3(0.0, 0.0, 0.0);
            const vec3<double> c4(1.0, 0.0, 1.0);
            const vec3<double> c5(1.0, 1.0, 0.0);
            const vec3<double> c6(1.0, 1.0, 1.0);
            const vec3<double> c7(0.0, 1.0, 1.0);

            fv_surface_mesh<double, uint32_t> mesh2;
            mesh2.vertices = {{ c0, c1, c2, c3, c4, c5, c6, c7 }};
            mesh2.faces = {{ static_cast<uint32_t>(3), static_cast<uint32_t>(2), static_cast<uint32_t>(7) },
                           { static_cast<uint32_t>(7), static_cast<uint32_t>(1), static_cast<uint32_t>(3) },

                           { static_cast<uint32_t>(4), static_cast<uint32_t>(0), static_cast<uint32_t>(5) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(6), static_cast<uint32_t>(4) },

                           { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1) },
                           { static_cast<uint32_t>(1), static_cast<uint32_t>(5), static_cast<uint32_t>(0) },

                           { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(2) },
                           { static_cast<uint32_t>(2), static_cast<uint32_t>(4), static_cast<uint32_t>(0) },

                           { static_cast<uint32_t>(7), static_cast<uint32_t>(6), static_cast<uint32_t>(5) },
                           { static_cast<uint32_t>(5), static_cast<uint32_t>(1), static_cast<uint32_t>(7) }};

            SUBCASE("under restrictive criteria, corner vertices are not simplified"){
                const double dist_tolerance = 0.0001;
                const double min_angle = std::acos(0.95);

                // There should be no vertices that satisfy the simplification criteria in this case.
                const size_t expected_verts = mesh2.vertices.size();
                const size_t expected_faces = mesh2.faces.size();

                mesh2.simplify_inner_triangles(dist_tolerance, min_angle);
                REQUIRE(mesh2.vertices.size() == expected_verts);
                REQUIRE(mesh2.faces.size() == expected_faces);
            }

            SUBCASE("boundary becomes shrink-wrapped when underconstrained"){
                const double dist_tolerance = 100.0;
                const double min_angle = std::acos(-0.8);

                // The exact behaviour around boundaries isn't specified.
                // However, *some* simplification should still occur in this case.
                // Should reduce down to a flat square.
                const size_t expected_vert_lower_limit = 4UL;
                const size_t expected_face_lower_limit = 2UL;
                const size_t orig_verts = mesh2.vertices.size();
                const size_t orig_faces = mesh2.faces.size();

                mesh2.simplify_inner_triangles(dist_tolerance, min_angle);
                REQUIRE(expected_vert_lower_limit <= mesh2.vertices.size());
                REQUIRE(expected_face_lower_limit <= mesh2.faces.size());

                REQUIRE(mesh2.vertices.size() < orig_verts);
                REQUIRE(mesh2.faces.size() < orig_faces);

                // Regardless of how much simplification happens, every vert removed should remove two faces.
                const auto diff_verts = orig_verts - mesh2.vertices.size();
                const auto diff_faces = orig_faces - mesh2.faces.size();
                REQUIRE(2UL * diff_verts == diff_faces);
            }
        }
    }
}

TEST_CASE( "Convex_Hull" ){
    SUBCASE("a valid seed tetrahedron can reliably be found, regardless of input orientation"){
        int64_t random_seed = 123456;
        std::mt19937 re( random_seed );
        std::uniform_real_distribution<> rd(0.0, 1.0); //Random distribution.

        std::vector<vec3<double>> all_verts {{
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)) }};

        // The following monstrosity iterates over the allowed permutations of 4 vertices selected.
        // It iterates allowed combinations from 'N-pick-M' where N = 6 and M = 4.
        //
        // This test is done to ensure that whatever order the points are in, a positive-volume seed tetrahedron
        // can be found.
        const int64_t N_all_verts = all_verts.size();
        const int64_t N_to_test = 4;

        std::vector<bool> v(N_all_verts);
        std::fill(v.begin(), v.begin() + N_to_test, true);

        do{
            std::vector<int64_t> indices;
            for(int64_t i = 0; i < N_all_verts; ++i){
                if(v[i] == true) indices.push_back(i);
            }
            if(indices.size() != N_to_test) throw std::runtime_error("Insufficient data (1)");

            std::sort( std::begin(indices), std::end(indices) );
            do{
                std::vector< vec3<double> > vert_v;
                for(const auto &j : indices){
                    vert_v.emplace_back( all_verts.at(j) );
                }

                if(vert_v.size() != N_to_test) throw std::runtime_error("Insufficient data (2)");

                using vert_vec_t = decltype(std::begin(all_verts));
                const auto faces = Convex_Hull_3<vert_vec_t,uint32_t>( std::begin(vert_v), std::end(vert_v));

            }while(std::next_permutation(std::begin(indices), std::end(indices)));
        }while(std::prev_permutation(v.begin(), v.end()));
    }

    SUBCASE("minimal hull problem correctly solved"){
        std::vector<vec3<double>> all_verts {{
            vec3<double>(1.0, 0.0, 0.0),     // }
            vec3<double>(0.0, 1.0, 0.0),     // }
            vec3<double>(0.0, 0.0, 1.0),     // }-- Seed hull
            vec3<double>(0.0, 0.0, 0.0),     // }
            vec3<double>(0.1, 0.1, 0.1),     // Inside the hull, should be ignored.
            vec3<double>(1.5, 1.5, 0.5) }};  // Outside the hull, should extend the hull.

        using vert_vec_t = decltype(std::begin(all_verts));
        const auto faces = Convex_Hull_3<vert_vec_t,uint32_t>( std::begin(all_verts), std::end(all_verts));
        REQUIRE( faces.size() == 6 );
    }

    SUBCASE("seed tetrahedron can handle degeneracy in the first few vertices"){
        std::vector<vec3<double>> all_verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),

            vec3<double>(0.0, 0.0, 1.0),
            vec3<double>(1.0, 0.0, 1.0),
            vec3<double>(1.0, 1.0, 1.0),
            vec3<double>(0.0, 1.0, 1.0) }};

        using vert_vec_t = decltype(std::begin(all_verts));
        const auto faces = Convex_Hull_3<vert_vec_t,uint32_t>( std::begin(all_verts), std::end(all_verts));
        REQUIRE( faces.size() == 12 );
    }

    SUBCASE("handles case with only degenerate data"){
        std::vector<vec3<double>> all_verts {{
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0) }};

        using vert_vec_t = decltype(std::begin(all_verts));
        REQUIRE_THROWS( Convex_Hull_3<vert_vec_t,uint32_t>( std::begin(all_verts), std::end(all_verts)) );
    }

    SUBCASE("a seed tetrahedron can reliably be found"){
        int64_t random_seed = 123456;
        std::mt19937 re( random_seed );
        std::uniform_real_distribution<> rd(0.01, 0.99); //Random distribution.

        std::vector<vec3<double>> all_verts {{
            vec3<double>(0.0, 0.0, 0.0), // Outer vertices.
            vec3<double>(0.0, 0.0, 1.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 1.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 1.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(1.0, 1.0, 1.0) }};
        for(size_t i = 0; i < 100; ++i){ // Inner vertices, randomly distributed.
            all_verts.emplace_back(vec3<double>(rd(re), rd(re), rd(re)));
        }

        // Shuffle order, so first vertices are unlikely to be present in the final hull.
        std::shuffle(std::begin(all_verts), std::end(all_verts), re);

        using vert_vec_t = decltype(std::begin(all_verts));
        const auto faces = Convex_Hull_3<vert_vec_t,uint32_t>( std::begin(all_verts), std::end(all_verts));
        REQUIRE( faces.size() == 12 );
    }
}


TEST_CASE( "Delaunay_Triangulation_2 function" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("fewer than 3 vertices returns empty mesh"){
        // 0 vertices
        std::vector<vec3<double>> verts0;
        const auto mesh0 = Delaunay_Triangulation_2<double, uint32_t>(verts0);
        REQUIRE( mesh0.vertices.size() == 0 );
        REQUIRE( mesh0.faces.size() == 0 );

        // 1 vertex
        std::vector<vec3<double>> verts1 {{ vec3<double>(0.0, 0.0, 0.0) }};
        const auto mesh1 = Delaunay_Triangulation_2<double, uint32_t>(verts1);
        REQUIRE( mesh1.vertices.size() == 0 );
        REQUIRE( mesh1.faces.size() == 0 );

        // 2 vertices
        std::vector<vec3<double>> verts2 {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0) }};
        const auto mesh2 = Delaunay_Triangulation_2<double, uint32_t>(verts2);
        REQUIRE( mesh2.vertices.size() == 0 );
        REQUIRE( mesh2.faces.size() == 0 );
    }

    SUBCASE("exactly 3 vertices produces exactly 1 triangle"){
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.5, 1.0, 0.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 3 );
        REQUIRE( mesh.faces.size() == 1 );
        REQUIRE( mesh.faces[0].size() == 3 );
    }

    SUBCASE("4 vertices forming a square produces 2 triangles"){
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 4 );
        REQUIRE( mesh.faces.size() == 2 );
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
        }
    }

    SUBCASE("5 vertices with one in center produces 4 triangles"){
        // Square with center point.
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.5, 0.5, 0.0) }};  // center
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 5 );
        REQUIRE( mesh.faces.size() == 4 );
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
        }
    }

    SUBCASE("regular grid produces valid triangulation"){
        // A 3x3 grid of points = 9 vertices.
        std::vector<vec3<double>> verts;
        for(int i = 0; i < 3; ++i){
            for(int j = 0; j < 3; ++j){
                verts.emplace_back(vec3<double>(static_cast<double>(i), static_cast<double>(j), 0.0));
            }
        }
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 9 );
        // For a 3x3 grid (2x2 cells), we expect 2*2*2 = 8 triangles.
        REQUIRE( mesh.faces.size() == 8 );
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
        }
    }

    SUBCASE("all triangles have valid indices"){
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.5, 0.5, 0.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
            for(const auto &idx : f){
                REQUIRE( idx < mesh.vertices.size() );
            }
        }
    }

    SUBCASE("triangles have non-zero area"){
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.5, 0.5, 0.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        for(const auto &f : mesh.faces){
            const auto &A = mesh.vertices[f[0]];
            const auto &B = mesh.vertices[f[1]];
            const auto &C = mesh.vertices[f[2]];
            // Compute triangle area using cross product.
            const auto cross = (B - A).Cross(C - A);
            const auto area = cross.length() / 2.0;
            REQUIRE( area > eps );
        }
    }

    SUBCASE("Delaunay property is satisfied (empty circumcircle)"){
        // The Delaunay property: no vertex is strictly inside the circumcircle of any triangle.
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.5, 0.5, 0.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);

        // Use the same tolerance strategy as the implementation.
        const double machine_eps = std::sqrt( std::numeric_limits<double>::epsilon() );

        // Helper to compute circumcircle.
        auto compute_circumcircle = [&machine_eps](const vec3<double> &A, const vec3<double> &B, const vec3<double> &C)
            -> std::tuple<double, double, double> {
            const double D = 2.0 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
            const double max_coord = std::max({std::abs(A.x), std::abs(B.x), std::abs(C.x),
                                               std::abs(A.y), std::abs(B.y), std::abs(C.y), 1.0});
            const double det_eps = machine_eps * max_coord * max_coord;
            if(std::abs(D) < det_eps){
                return std::make_tuple(std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::quiet_NaN());
            }
            const double A_sq = A.x * A.x + A.y * A.y;
            const double B_sq = B.x * B.x + B.y * B.y;
            const double C_sq = C.x * C.x + C.y * C.y;
            const double cx = (A_sq * (B.y - C.y) + B_sq * (C.y - A.y) + C_sq * (A.y - B.y)) / D;
            const double cy = (A_sq * (C.x - B.x) + B_sq * (A.x - C.x) + C_sq * (B.x - A.x)) / D;
            const double r_sq = (A.x - cx) * (A.x - cx) + (A.y - cy) * (A.y - cy);
            return std::make_tuple(cx, cy, r_sq);
        };

        for(const auto &f : mesh.faces){
            const auto &A = mesh.vertices[f[0]];
            const auto &B = mesh.vertices[f[1]];
            const auto &C = mesh.vertices[f[2]];
            const auto [cx, cy, r_sq] = compute_circumcircle(A, B, C);
            if(!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(r_sq)){
                continue;
            }

            // Check that no other vertex is strictly inside this circumcircle.
            // Using consistent tolerance with the implementation.
            const double r_tol = r_sq * machine_eps;
            for(size_t i = 0; i < mesh.vertices.size(); ++i){
                // Skip the triangle's own vertices.
                if(i == f[0] || i == f[1] || i == f[2]) continue;

                const auto &P = mesh.vertices[i];
                const double dist_sq = (P.x - cx) * (P.x - cx) + (P.y - cy) * (P.y - cy);
                // Points should be outside or on the circle (not strictly inside).
                // This mirrors the implementation's check: dist_sq < (r_sq - r_tol) means "inside".
                // So we require the opposite: dist_sq >= (r_sq - r_tol).
                REQUIRE( dist_sq >= (r_sq - r_tol) );
            }
        }
    }

    SUBCASE("random vertices produce valid triangulation"){
        std::mt19937 re(12345);
        std::uniform_real_distribution<double> rd(0.0, 10.0);

        std::vector<vec3<double>> verts;
        for(size_t i = 0; i < 20; ++i){
            verts.emplace_back(vec3<double>(rd(re), rd(re), 0.0));
        }

        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 20 );
        REQUIRE( mesh.faces.size() > 0 );

        // All faces should be triangles with valid indices.
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
            for(const auto &idx : f){
                REQUIRE( idx < mesh.vertices.size() );
            }
        }
    }

    SUBCASE("collinear vertices handled gracefully"){
        // All vertices on a line -- no valid triangulation possible.
        std::vector<vec3<double>> verts {{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(2.0, 0.0, 0.0),
            vec3<double>(3.0, 0.0, 0.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        // For collinear points, we expect no valid triangles (or degenerate triangles removed).
        REQUIRE( mesh.faces.size() == 0 );
    }

    SUBCASE("works with float type and uint64_t indices"){
        std::vector<vec3<float>> verts {{
            vec3<float>(0.0f, 0.0f, 0.0f),
            vec3<float>(1.0f, 0.0f, 0.0f),
            vec3<float>(0.5f, 1.0f, 0.0f) }};
        const auto mesh = Delaunay_Triangulation_2<float, uint64_t>(verts);
        REQUIRE( mesh.vertices.size() == 3 );
        REQUIRE( mesh.faces.size() == 1 );
    }
}


