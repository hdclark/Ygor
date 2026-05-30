#include <algorithm>
#include <array>
#include <vector>

#include <YgorMath.h>

#include "doctest/doctest.h"

namespace {

template <class T, class I>
fv_surface_mesh<T, I>
make_mesh(const std::initializer_list<vec3<T>> &vertices,
          const std::initializer_list<std::vector<I>> &faces){
    fv_surface_mesh<T, I> mesh;
    mesh.vertices.assign(vertices.begin(), vertices.end());
    mesh.faces.assign(faces.begin(), faces.end());
    mesh.recreate_involved_face_index();
    return mesh;
}

template <class T>
bool
contains_point(const std::vector<vec3<T>> &points,
               const vec3<T> &target,
               T eps = static_cast<T>(1.0e-6)){
    return std::any_of(points.begin(), points.end(), [&](const vec3<T> &point){
        return point.distance(target) <= eps;
    });
}

bool
contains_primitive(const std::vector<fv_surface_mesh_intersection_primitive> &primitives,
                   fv_surface_mesh_intersection_primitive_type type,
                   std::vector<size_t> indices){
    std::sort(indices.begin(), indices.end());
    return std::any_of(primitives.begin(), primitives.end(), [&](const auto &primitive){
        auto actual = primitive.indices;
        std::sort(actual.begin(), actual.end());
        return (primitive.type == type) && (actual == indices);
    });
}

} // namespace

TEST_CASE("fv_surface_mesh::intersections_with"){
    SUBCASE("extracts a point intersection at matching vertices"){
        const auto lhs = make_mesh<double, uint64_t>({
                vec3<double>(0.0, 0.0, 0.0),
                vec3<double>(2.0, 0.0, 0.0),
                vec3<double>(0.0, 2.0, 0.0)
            }, {
                { 0, 1, 2 }
            });
        const auto rhs = make_mesh<double, uint64_t>({
                vec3<double>(0.0, 0.0, 0.0),
                vec3<double>(0.0, -1.0, 1.0),
                vec3<double>(-1.0, 0.0, 1.0)
            }, {
                { 0, 1, 2 }
            });

        const auto intersections = lhs.intersections_with(rhs);
        REQUIRE(intersections.size() == 1UL);
        const auto &isect = intersections.front();
        CHECK(isect.lhs_geometry.type == fv_surface_mesh_intersection_geometry_type::Point);
        REQUIRE(isect.lhs_geometry.vertices.size() == 1UL);
        CHECK(isect.lhs_geometry.vertices.front().distance(vec3<double>(0.0, 0.0, 0.0)) < 1.0e-8);
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Vertex, { 0 }));
        CHECK(contains_primitive(isect.rhs_primitives, fv_surface_mesh_intersection_primitive_type::Vertex, { 0 }));
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Face, { 0 }));
        CHECK(contains_primitive(isect.rhs_primitives, fv_surface_mesh_intersection_primitive_type::Face, { 0 }));
    }

    SUBCASE("extracts a noncoplanar segment and linked edge references"){
        const auto lhs = make_mesh<double, uint64_t>({
                vec3<double>(0.0, 0.0, 0.0),
                vec3<double>(2.0, 0.0, 0.0),
                vec3<double>(0.0, 2.0, 0.0)
            }, {
                { 0, 1, 2 }
            });
        const auto rhs = make_mesh<double, uint64_t>({
                vec3<double>(0.5, 0.5, -1.0),
                vec3<double>(0.5, 0.5, 1.0),
                vec3<double>(1.5, 0.5, 0.0)
            }, {
                { 0, 1, 2 }
            });

        const auto intersections = lhs.intersections_with(rhs);
        REQUIRE(intersections.size() == 1UL);
        const auto &isect = intersections.front();
        CHECK(isect.lhs_geometry.type == fv_surface_mesh_intersection_geometry_type::Segment);
        REQUIRE(isect.lhs_geometry.vertices.size() == 2UL);
        CHECK(isect.lhs_geometry.vertices.at(0).distance(vec3<double>(0.5, 0.5, 0.0)) < 1.0e-8);
        CHECK(isect.lhs_geometry.vertices.at(1).distance(vec3<double>(1.5, 0.5, 0.0)) < 1.0e-8);
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 1, 2 }));
        CHECK(contains_primitive(isect.rhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 0, 2 }));
    }

    SUBCASE("extracts a coplanar shared edge"){
        const auto lhs = make_mesh<double, uint64_t>({
                vec3<double>(0.0, 0.0, 0.0),
                vec3<double>(2.0, 0.0, 0.0),
                vec3<double>(0.0, 2.0, 0.0)
            }, {
                { 0, 1, 2 }
            });
        const auto rhs = make_mesh<double, uint64_t>({
                vec3<double>(0.0, 0.0, 0.0),
                vec3<double>(2.0, 0.0, 0.0),
                vec3<double>(1.0, -1.0, 0.0)
            }, {
                { 0, 1, 2 }
            });

        const auto intersections = lhs.intersections_with(rhs);
        REQUIRE(intersections.size() == 1UL);
        const auto &isect = intersections.front();
        CHECK(isect.lhs_geometry.type == fv_surface_mesh_intersection_geometry_type::Segment);
        REQUIRE(isect.lhs_geometry.vertices.size() == 2UL);
        CHECK(isect.lhs_geometry.vertices.at(0).distance(vec3<double>(0.0, 0.0, 0.0)) < 1.0e-8);
        CHECK(isect.lhs_geometry.vertices.at(1).distance(vec3<double>(2.0, 0.0, 0.0)) < 1.0e-8);
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 0, 1 }));
        CHECK(contains_primitive(isect.rhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 0, 1 }));
    }

    SUBCASE("extracts a coplanar face overlap polygon"){
        const auto lhs = make_mesh<double, uint64_t>({
                vec3<double>(0.0, 0.0, 0.0),
                vec3<double>(1.0, 0.0, 0.0),
                vec3<double>(0.0, 1.0, 0.0)
            }, {
                { 0, 1, 2 }
            });
        const auto rhs = lhs;

        const auto intersections = lhs.intersections_with(rhs);
        REQUIRE(intersections.size() == 1UL);
        const auto &isect = intersections.front();
        CHECK(isect.lhs_geometry.type == fv_surface_mesh_intersection_geometry_type::Polygon);
        REQUIRE(isect.lhs_geometry.vertices.size() == 3UL);
        CHECK(contains_point(isect.lhs_geometry.vertices, vec3<double>(0.0, 0.0, 0.0)));
        CHECK(contains_point(isect.lhs_geometry.vertices, vec3<double>(1.0, 0.0, 0.0)));
        CHECK(contains_point(isect.lhs_geometry.vertices, vec3<double>(0.0, 1.0, 0.0)));
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Face, { 0 }));
        CHECK(contains_primitive(isect.rhs_primitives, fv_surface_mesh_intersection_primitive_type::Face, { 0 }));
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 0, 1 }));
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 0, 2 }));
        CHECK(contains_primitive(isect.lhs_primitives, fv_surface_mesh_intersection_primitive_type::Edge, { 1, 2 }));
    }
}
