
#include <limits>
#include <utility>
#include <iostream>
#include <random>

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
}

TEST_CASE( "Convex_Hull" ){
    SUBCASE("a valid seed tetrahedron can reliably be found, regardless of input orientation"){
        long int random_seed = 123456;
        std::mt19937 re( random_seed );
        std::uniform_real_distribution<> rd(0.0, 1.0); //Random distribution.

        std::vector<vec3<double>> all_verts {{
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)),
            vec3<double>(rd(re), rd(re), rd(re)) }};

        std::vector< std::reference_wrapper<vec3<double>> > all_vert_refs;
        for(auto &v : all_verts){
            all_vert_refs.emplace_back( std::ref(v) );
        }

        // The following monstrosity iterates over the allowed permutations of 4 vertices selected.
        // It iterates allowed combinations from 'N-pick-M' where N = 6 and M = 4.
        //
        // This test is done to ensure that whatever order the points are in, a positive-volume seed tetrahedron
        // can be found.
        const long int N_all_verts = all_vert_refs.size();
        const long int N_to_test = 4;

        std::vector<bool> v(N_all_verts);
        std::fill(v.begin(), v.begin() + N_to_test, true);

        do{
            std::vector<long int> indices;
            for(long int i = 0; i < N_all_verts; ++i){
                if(v[i] == true) indices.push_back(i);
            }
            if(indices.size() != N_to_test) throw std::runtime_error("Insufficient data (1)");

            std::sort( std::begin(indices), std::end(indices) );
            do{
                std::vector< std::reference_wrapper<vec3<double>> > vert_refs;
                for(const auto &j : indices){
                    vert_refs.emplace_back( all_vert_refs.at(j) );
                }

                if(vert_refs.size() != N_to_test) throw std::runtime_error("Insufficient data (2)");

                const auto faces = Convex_Hull<double,uint32_t>(vert_refs);

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

        std::vector< std::reference_wrapper<vec3<double>> > all_vert_refs;
        for(auto &v : all_verts){
            all_vert_refs.emplace_back( std::ref(v) );
        }

        auto faces = Convex_Hull<double,uint32_t>(all_vert_refs);
        REQUIRE( faces.size() == 6 );
    }

    SUBCASE("a seed tetrahedron can reliably be found"){
        long int random_seed = 123456;
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

        std::vector< std::reference_wrapper<vec3<double>> > all_vert_refs;
        for(auto &v : all_verts){
            all_vert_refs.emplace_back( std::ref(v) );
        }

        auto faces = Convex_Hull<double,uint32_t>(all_vert_refs);
        REQUIRE( faces.size() == 12 );
    }
}


