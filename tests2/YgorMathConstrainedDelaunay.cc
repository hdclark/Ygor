
#include <array>
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMathConstrainedDelaunay.h>
#include <YgorMeshesAdaptivePredicates.h>

#include "doctest/doctest.h"


// Helper functions.
namespace {

constexpr double sketch_point_merge_tolerance = 1.0e-6;
constexpr size_t sketch_curve_min_segments = 16U;
constexpr size_t sketch_curve_max_segments = 1024U;

template <class I>
using edge_type = std::pair<I, I>;

template <class I>
edge_type<I> make_edge(I a, I b){
    if(b < a){
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

template <class T>
vec2<T> as_vec2(const vec3<T> &v){
    return vec2<T>(v.x, v.y);
}

template <class T, class I>
std::set<edge_type<I>> collect_triangle_edges(const fv_surface_mesh<T, I> &mesh){
    std::set<edge_type<I>> edges;
    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        edges.insert(make_edge(face.at(0), face.at(1)));
        edges.insert(make_edge(face.at(1), face.at(2)));
        edges.insert(make_edge(face.at(2), face.at(0)));
    }
    return edges;
}

template <class T, class I>
void require_all_faces_are_triangles(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
    }
}

template <class T, class I>
void require_constraints_are_triangle_edges(const fv_surface_mesh<T, I> &mesh,
                                            const std::vector<edge_type<I>> &constraints){
    const auto mesh_edges = collect_triangle_edges(mesh);
    for(const auto &edge : constraints){
        REQUIRE(mesh_edges.count(edge) == 1);
    }
}

template <class T, class I>
void require_edges_are_manifold(const fv_surface_mesh<T, I> &mesh){
    std::map<edge_type<I>, size_t> counts;
    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        ++counts[make_edge(face.at(0), face.at(1))];
        ++counts[make_edge(face.at(1), face.at(2))];
        ++counts[make_edge(face.at(2), face.at(0))];
    }
    for(const auto &[edge, count] : counts){
        (void)edge;
        REQUIRE(count <= 2);
    }
}

template <class T, class I>
void require_non_constraint_edges_are_locally_delaunay(const fv_surface_mesh<T, I> &mesh,
                                                       const std::vector<edge_type<I>> &constraints){
    std::set<edge_type<I>> constraint_edges(constraints.begin(), constraints.end());
    std::map<edge_type<I>, std::vector<std::array<I, 3>>> incident;

    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        const std::array<I, 3> tri{{ face.at(0), face.at(1), face.at(2) }};
        incident[make_edge(tri[0], tri[1])].push_back(tri);
        incident[make_edge(tri[1], tri[2])].push_back(tri);
        incident[make_edge(tri[2], tri[0])].push_back(tri);
    }

    for(const auto &[edge, tris] : incident){
        if((tris.size() != 2) || (constraint_edges.count(edge) != 0)){
            continue;
        }

        const auto &t0 = tris.at(0);
        const auto &t1 = tris.at(1);
        const auto find_opposite_vertex = [](const std::array<I, 3> &tri, edge_type<I> edge_key) -> I {
            for(const auto idx : tri){
                if((idx != edge_key.first) && (idx != edge_key.second)){
                    return idx;
                }
            }
            return edge_key.first;
        };

        const auto w = find_opposite_vertex(t0, edge);
        const auto x = find_opposite_vertex(t1, edge);
        const auto a = as_vec2(mesh.vertices.at(edge.first));
        const auto b = as_vec2(mesh.vertices.at(edge.second));
        const auto c = as_vec2(mesh.vertices.at(w));
        const auto d = as_vec2(mesh.vertices.at(x));
        REQUIRE(incircle_sign(a, b, c, d) <= static_cast<T>(0));
    }
}

template <class T, class I>
void require_triangle_centroids_within_polygon(const fv_surface_mesh<T, I> &mesh,
                                               const std::vector<vec2<T>> &polygon){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        const vec2<T> centroid((a.x + b.x + c.x) / static_cast<T>(3),
                               (a.y + b.y + c.y) / static_cast<T>(3));
        REQUIRE(::point_in_polygon_or_on_boundary(polygon, centroid));
    }
}

struct sketch_primitive_spec {
    std::string kind;
    std::vector<size_t> vertices;
};

inline bool coincident_2d(const vec2<double> &a, const vec2<double> &b){
    return a.distance(b) <= sketch_point_merge_tolerance;
}

inline double normalize_angle(double angle){
    const auto pi = std::acos(-1.0);
    while(angle < 0.0) angle += 2.0 * pi;
    while((2.0 * pi) <= angle) angle -= 2.0 * pi;
    return angle;
}

inline std::pair<std::vector<vec2<double>>, bool>
deduplicate_sampled_points(const std::vector<vec2<double>> &sampled){
    std::vector<vec2<double>> out;
    out.reserve(sampled.size());
    for(const auto &point : sampled){
        if(out.empty() || !coincident_2d(out.back(), point)){
            out.push_back(point);
        }
    }

    bool closed = false;
    if((out.size() >= 2U) && coincident_2d(out.front(), out.back())){
        out.pop_back();
        closed = true;
    }
    return std::make_pair(std::move(out), closed);
}

std::pair<std::vector<vec2<double>>, std::vector<std::vector<uint64_t>>>
discretize_sketch_constraints(const std::string &sketch_text,
                              double max_error){
    vec3<double> plane_origin;
    vec3<double> plane_row_unit(1.0, 0.0, 0.0);
    vec3<double> plane_col_unit(0.0, 1.0, 0.0);
    std::map<size_t, vec3<double>> vertices_3d;
    std::vector<sketch_primitive_spec> primitives;

    std::istringstream input(sketch_text);
    std::string line;
    while(std::getline(input, line)){
        if(line.empty()) continue;
        std::istringstream line_stream(line);
        std::string token;
        line_stream >> token;
        if(token == "plane_origin"){
            line_stream >> plane_origin.x >> plane_origin.y >> plane_origin.z;
        }else if(token == "plane_row_unit"){
            line_stream >> plane_row_unit.x >> plane_row_unit.y >> plane_row_unit.z;
        }else if(token == "plane_col_unit"){
            line_stream >> plane_col_unit.x >> plane_col_unit.y >> plane_col_unit.z;
        }else if(token == "vertex"){
            size_t index = 0;
            vec3<double> point;
            line_stream >> index >> point.x >> point.y >> point.z;
            vertices_3d[index] = point;
        }else if(token == "primitive"){
            sketch_primitive_spec primitive;
            size_t primitive_index = 0;
            std::string geometry_tag;
            line_stream >> primitive.kind >> primitive_index >> geometry_tag;
            (void)primitive_index;
            while(true){
                size_t vertex_index = 0;
                line_stream >> vertex_index;
                if(!line_stream) break;
                primitive.vertices.push_back(vertex_index);
            }
            primitives.push_back(std::move(primitive));
        }
    }

    const auto project = [&](const vec3<double> &point){
        const auto rel = point - plane_origin;
        return vec2<double>(plane_row_unit.Dot(rel), plane_col_unit.Dot(rel));
    };

    size_t curve_segments = sketch_curve_min_segments;
    for(const auto &primitive : primitives){
        if(primitive.kind == "line"){
            continue;
        }
        size_t primitive_segments = sketch_curve_min_segments;
        if(primitive.kind == "circle"){
            const auto &centre = vertices_3d.at(primitive.vertices.at(0));
            const auto &radius_point = vertices_3d.at(primitive.vertices.at(1));
            const auto radius = centre.distance(radius_point);
            primitive_segments = static_cast<size_t>(std::ceil(2.0 * std::acos(-1.0)
                                                               * std::sqrt(radius / (8.0 * max_error))));
        }else if(primitive.kind == "arc"){
            const auto centre = project(vertices_3d.at(primitive.vertices.at(0)));
            const auto start = project(vertices_3d.at(primitive.vertices.at(1)));
            const auto stop = project(vertices_3d.at(primitive.vertices.at(2)));
            const auto radius = centre.distance(start);
            auto start_angle = normalize_angle(std::atan2(start.y - centre.y, start.x - centre.x));
            auto stop_angle = normalize_angle(std::atan2(stop.y - centre.y, stop.x - centre.x));
            if(stop_angle < start_angle) stop_angle += 2.0 * std::acos(-1.0);
            primitive_segments = static_cast<size_t>(std::ceil((stop_angle - start_angle)
                                                               * std::sqrt(radius / (8.0 * max_error))));
        }
        curve_segments = std::max(curve_segments,
                                  std::clamp(primitive_segments,
                                             sketch_curve_min_segments,
                                             sketch_curve_max_segments));
    }

    struct path_spec {
        std::vector<vec2<double>> points;
        bool closed = false;
    };
    std::vector<path_spec> paths;

    for(const auto &primitive : primitives){
        std::vector<vec2<double>> sampled;
        if(primitive.kind == "line"){
            sampled.push_back(project(vertices_3d.at(primitive.vertices.at(0))));
            sampled.push_back(project(vertices_3d.at(primitive.vertices.at(1))));
        }else if(primitive.kind == "circle"){
            const auto centre = project(vertices_3d.at(primitive.vertices.at(0)));
            const auto radius = vertices_3d.at(primitive.vertices.at(0))
                              .distance(vertices_3d.at(primitive.vertices.at(1)));
            sampled.reserve(curve_segments + 1U);
            const auto pi = std::acos(-1.0);
            for(size_t i = 0; i <= curve_segments; ++i){
                const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(curve_segments);
                sampled.emplace_back(centre.x + std::cos(angle) * radius,
                                     centre.y + std::sin(angle) * radius);
            }
        }else if(primitive.kind == "arc"){
            const auto centre = project(vertices_3d.at(primitive.vertices.at(0)));
            const auto start = project(vertices_3d.at(primitive.vertices.at(1)));
            const auto stop = project(vertices_3d.at(primitive.vertices.at(2)));
            const auto radius = centre.distance(start);
            auto start_angle = normalize_angle(std::atan2(start.y - centre.y, start.x - centre.x));
            auto stop_angle = normalize_angle(std::atan2(stop.y - centre.y, stop.x - centre.x));
            const auto pi = std::acos(-1.0);
            if(stop_angle < start_angle) stop_angle += 2.0 * pi;
            sampled.reserve(curve_segments + 1U);
            for(size_t i = 0; i <= curve_segments; ++i){
                const auto angle = start_angle + (stop_angle - start_angle)
                                               * (static_cast<double>(i) / static_cast<double>(curve_segments));
                sampled.emplace_back(centre.x + std::cos(angle) * radius,
                                     centre.y + std::sin(angle) * radius);
            }
        }

        auto [points, closed] = deduplicate_sampled_points(sampled);
        if(points.size() < 2U) continue;
        if(closed && (points.size() < 3U)) continue;
        paths.push_back(path_spec{ std::move(points), closed });
    }

    bool merged_any = true;
    while(merged_any){
        merged_any = false;
        for(size_t i = 0; i < paths.size(); ++i){
            if(paths.at(i).closed || paths.at(i).points.empty()) continue;
            for(size_t j = i + 1U; j < paths.size(); ++j){
                if(paths.at(j).closed || paths.at(j).points.empty()) continue;

                auto lhs = paths.at(i).points;
                auto rhs = paths.at(j).points;
                bool merged = true;
                if(coincident_2d(lhs.back(), rhs.front())){
                    lhs.insert(lhs.end(), std::next(rhs.begin()), rhs.end());
                }else if(coincident_2d(lhs.back(), rhs.back())){
                    std::reverse(rhs.begin(), rhs.end());
                    lhs.insert(lhs.end(), std::next(rhs.begin()), rhs.end());
                }else if(coincident_2d(lhs.front(), rhs.back())){
                    lhs.insert(lhs.begin(), rhs.begin(), std::prev(rhs.end()));
                }else if(coincident_2d(lhs.front(), rhs.front())){
                    std::reverse(rhs.begin(), rhs.end());
                    lhs.insert(lhs.begin(), rhs.begin(), std::prev(rhs.end()));
                }else{
                    merged = false;
                }

                if(merged){
                    auto [points, closed] = deduplicate_sampled_points(lhs);
                    paths.at(i).points = std::move(points);
                    paths.at(i).closed = closed;
                    paths.at(j).points.clear();
                    paths.at(j).closed = false;
                    merged_any = true;
                    break;
                }
            }
            if(merged_any) break;
        }
    }

    std::vector<vec2<double>> cdt_vertices;
    std::vector<std::vector<uint64_t>> cdt_edges;
    for(const auto &path : paths){
        if(!path.closed || (path.points.size() < 3U)) continue;
        const auto base = static_cast<uint64_t>(cdt_vertices.size());
        cdt_vertices.insert(cdt_vertices.end(), path.points.begin(), path.points.end());
        for(size_t i = 0; i < path.points.size(); ++i){
            cdt_edges.push_back({ base + static_cast<uint64_t>(i),
                                  base + static_cast<uint64_t>((i + 1U) % path.points.size()) });
        }
    }

    return std::make_pair(std::move(cdt_vertices), std::move(cdt_edges));
}

} // anonymous namespace

TEST_CASE( "Constrained_Delaunay_Triangulation_2 function" ){
    SUBCASE("empty input returns empty mesh"){
        const std::vector<vec2<double>> verts;
        const std::vector<std::vector<uint32_t>> edges;
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("two vertices with one constrained edge yields no faces"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 2);
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("triangle boundary constraints produce only the triangle"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 1);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(0, 2) });
    }

    SUBCASE("constraint insertion keeps only triangles and preserves the constrained edge"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(0.0, 2.0),
            vec2<double>(1.1, 0.8)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 2} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() >= 3);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh, { make_edge<uint32_t>(0, 2) });
        require_non_constraint_edges_are_locally_delaunay(mesh, { make_edge<uint32_t>(0, 2) });
    }

    SUBCASE("polygon boundary constraints produce only triangle faces"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(3.0, 1.0),
            vec2<double>(1.5, 2.5),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == 3);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 0) });
        require_non_constraint_edges_are_locally_delaunay(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 0) });
    }

    SUBCASE("non-convex constrained polygon stays inside the constrained region"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(6.0, 1.0),
            vec2<double>(-3.0, 4.0),
            vec2<double>(-3.0, 1.0),
            vec2<double>(-2.0, -1.0),
            vec2<double>(-2.0, -5.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == verts.size() - 2);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 5), make_edge<uint32_t>(5, 0) });
        require_triangle_centroids_within_polygon(mesh, verts);
    }

    SUBCASE("closed constrained regions are still filtered when extra constraints touch the boundary"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(0.0, 2.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(3.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}
        }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == 4);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 0), make_edge<uint32_t>(0, 4) });
        require_triangle_centroids_within_polygon(mesh,
            { verts.at(0), verts.at(1), verts.at(2), verts.at(3) });
        for(const auto &face : mesh.faces){
            REQUIRE(std::find(face.begin(), face.end(), uint32_t{5}) == face.end());
        }
    }

    SUBCASE("regular polygon remains triangulable even when very small and offset from the origin"){
        std::vector<vec2<double>> verts;
        std::vector<std::vector<uint32_t>> edges;
        constexpr uint32_t N = 20;
        constexpr double radius = 1.0e-9;
        constexpr double x_offset = 123.456;
        constexpr double y_offset = -987.654;
        const auto pi = std::acos(-1.0);
        for(uint32_t i = 0; i < N; ++i){
            const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(N);
            verts.emplace_back(x_offset + radius * std::cos(angle),
                               y_offset + radius * std::sin(angle));
            edges.push_back({ i, static_cast<uint32_t>((i + 1) % N) });
        }

        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == verts.size() - 2);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 5), make_edge<uint32_t>(5, 6),
              make_edge<uint32_t>(6, 7), make_edge<uint32_t>(7, 8), make_edge<uint32_t>(8, 9),
              make_edge<uint32_t>(9, 10), make_edge<uint32_t>(10, 11), make_edge<uint32_t>(11, 12),
              make_edge<uint32_t>(12, 13), make_edge<uint32_t>(13, 14), make_edge<uint32_t>(14, 15),
              make_edge<uint32_t>(15, 16), make_edge<uint32_t>(16, 17), make_edge<uint32_t>(17, 18),
              make_edge<uint32_t>(18, 19), make_edge<uint32_t>(19, 0) });
        require_edges_are_manifold(mesh);
        require_triangle_centroids_within_polygon(mesh, verts);
    }

    SUBCASE("regular constrained polygons on a common circle stay manifold across many vertex counts"){
        const auto pi = std::acos(-1.0);
        for(uint32_t n = 4; n <= 32; ++n){
            std::vector<vec2<double>> verts;
            std::vector<std::vector<uint32_t>> edges;
            verts.reserve(n);
            edges.reserve(n);
            for(uint32_t i = 0; i < n; ++i){
                const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(n);
                verts.emplace_back(std::cos(angle), std::sin(angle));
                edges.push_back({ i, static_cast<uint32_t>((i + 1) % n) });
            }

            const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
            REQUIRE(mesh.vertices.size() == verts.size());
            REQUIRE(mesh.faces.size() == verts.size() - 2);
            require_all_faces_are_triangles(mesh);
            require_edges_are_manifold(mesh);
            require_triangle_centroids_within_polygon(mesh, verts);
        }
    }

    SUBCASE("crossing constraints are rejected"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 2}, {1, 3} }};
        REQUIRE_THROWS( Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges) );
    }

    SUBCASE("constraint passing through another vertex is rejected"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 2} }};
        REQUIRE_THROWS( Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges) );
    }

    SUBCASE("malformed constraints are rejected"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1, 2} }};
        REQUIRE_THROWS( Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges) );
    }

    SUBCASE("out-of-range constraints are rejected"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 3} }};
        REQUIRE_THROWS( Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges) );
    }

    SUBCASE("self-edge constraints are rejected"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {1, 1} }};
        REQUIRE_THROWS( Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges) );
    }

    SUBCASE("duplicate constraints are rejected even when reversed"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 0} }};
        REQUIRE_THROWS( Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges) );
    }

    SUBCASE("float coordinates and uint64_t indices are supported"){
        const std::vector<vec2<float>> verts{{
            vec2<float>(0.0f, 0.0f),
            vec2<float>(1.0f, 0.0f),
            vec2<float>(0.0f, 1.0f),
            vec2<float>(1.0f, 1.0f)
        }};
        const std::vector<std::vector<uint64_t>> edges{{ {0, 2} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<float, uint64_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 4);
        REQUIRE(mesh.faces.size() == 2);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh, { make_edge<uint64_t>(0, 2) });
    }

    SUBCASE("nested sketch-style loops with many circular constraints remain triangulable"){
        const auto sketch = std::string(R"SKETCH(sketch_format_version 1
plane_origin 0 0 0
plane_row_unit 0 1 0
plane_col_unit 1 0 0
vertex 0 53.0496864318847656 82.5260238647460938 0
vertex 1 120.885643005371094 30.8414821624755859 0
vertex 2 169.33990478515625 34.8793373107910156 0
vertex 3 185.087539672851562 93.4282302856445312 0
vertex 4 189.932968139648438 136.2294921875 0
vertex 5 176.608047485351562 176.204254150390625 0
vertex 6 118.059150695800781 194.374603271484375 0
vertex 7 69.2011032104492188 185.087539672851562 0
vertex 8 45.7815475463867188 133.806777954101562 0
vertex 9 81.7184524536132812 125.327285766601562 0
vertex 10 97.4660873413085938 87.775238037109375 0
vertex 11 132.191635131835938 61.5291786193847656 0
vertex 12 153.188491821289062 86.16009521484375 0
vertex 13 156.822555541992188 120.078079223632812 0
vertex 14 138.248428344726562 147.535491943359375 0
vertex 15 108.368293762207031 162.87933349609375 0
vertex 16 88.5828094482421875 149.958206176757812 0
vertex 17 17.5722515278878433 175.780022799832437 0
vertex 18 31.6490554809570312 233.541793823242188 0
vertex 19 17.1127777099609375 206.488174438476562 0
vertex 20 237.175872802734375 244.444000244140625 0
vertex 21 210.526031494140625 211.737380981445312 0
vertex 22 233.541793823242188 240.002365112304688 0
vertex 23 233.541793823242188 233.945587158203125 0
vertex 24 229.907730102539062 228.696365356445312 0
vertex 25 223.8509521484375 225.466094970703125 0
vertex 26 134.614349365234375 103.522872924804688 0
vertex 27 126.134857177734375 92.6206588745117188 0
vertex 28 108.772079467773438 118.866722106933594 0
vertex 29 100.696372985839844 114.828865051269531 0
vertex 30 110.519683837890625 140.351287841796875 0
vertex 31 102.161697387695312 134.4364013671875 0
vertex 32 142.687698364257812 130.5584716796875 0
vertex 33 133.400634765625 122.886543273925781 0
vertex 34 116.588264465332031 109.229957580566406 0
vertex 35 113.862876892089844 121.063606262207031 0
vertex 36 118.749092102050781 126.078399658203125 0
vertex 37 107.176490783691406 124.921142578125 0
vertex 38 128.392921447753906 145.751815795898438 0
vertex 39 121.063606262207031 140.737014770507812 0
vertex 40 109.572837829589844 103.833267211914062 0
vertex 41 105.072380065917969 99.4614028930664062 0
vertex 42 146.733718872070312 110.648246765136719 0
vertex 43 142.361846923828125 106.147796630859375 0
vertex 44 135.932647705078125 141.251327514648438 0
vertex 45 133.618118286132812 139.1939697265625 0
vertex 46 110.087173461914062 150.123641967773438 0
vertex 47 109.572837829589844 146.651870727539062 0
vertex 48 94.7856292724609375 125.949775695800781 0
vertex 49 91.6996002197265625 124.663932800292969 0
vertex 50 109.187080383300781 92.2606735229492188 0
vertex 51 107.644073486328125 88.5317306518554688 0
vertex 52 149.176849365234375 115.534454345703125 0
vertex 53 146.4765625 114.762947082519531 0
vertex 54 144.804962158203125 97.404052734375 0
vertex 55 142.490447998046875 96.7611312866210938 0
vertex 56 22.7657737731933594 111.598579406738281 0
vertex 57 124.51971435546875 8.22949504852294922 0
vertex 58 203.661666870117188 51.0307579040527344 0
vertex 59 242.425079345703125 76.4692459106445312 0
vertex 60 131.384063720703125 250.904571533203125 0
vertex 61 9.03706645965576172 130.576492309570312 0
primitive line 0 normal 0 1
primitive line 1 normal 1 2
primitive line 2 normal 2 3
primitive line 3 normal 3 4
primitive line 4 normal 4 5
primitive line 5 normal 5 6
primitive line 6 normal 6 7
primitive line 7 normal 7 8
primitive line 8 normal 8 0
primitive line 9 normal 9 10
primitive line 10 normal 10 11
primitive line 11 normal 11 12
primitive line 12 normal 12 13
primitive line 13 normal 13 14
primitive line 14 normal 14 15
primitive line 15 normal 15 16
primitive line 16 normal 16 9
primitive arc 17 normal 19 18 17
primitive line 18 normal 17 19
primitive line 19 normal 19 18
primitive circle 20 normal 21 20
primitive circle 21 normal 21 22
primitive circle 22 normal 21 23
primitive circle 23 normal 21 24
primitive circle 24 normal 21 25
primitive circle 25 normal 27 26
primitive circle 26 normal 29 28
primitive circle 27 normal 31 30
primitive circle 28 normal 33 32
primitive circle 29 normal 39 38
primitive circle 30 normal 41 40
primitive circle 31 normal 43 42
primitive circle 32 normal 45 44
primitive circle 33 normal 47 46
primitive circle 34 normal 49 48
primitive circle 35 normal 51 50
primitive circle 36 normal 53 52
primitive circle 37 normal 55 54
primitive line 38 normal 56 57
primitive line 39 normal 57 58
primitive line 40 normal 58 59
primitive line 41 normal 59 60
primitive line 42 normal 60 61
primitive line 43 normal 61 56
)SKETCH");

        const auto [verts, edges] = discretize_sketch_constraints(sketch, 0.1);
        REQUIRE(verts.size() == 899);
        REQUIRE(edges.size() == 899);

        std::vector<edge_type<uint64_t>> constraints;
        constraints.reserve(edges.size());
        for(const auto &edge : edges){
            REQUIRE(edge.size() == 2);
            constraints.push_back(make_edge(edge.at(0), edge.at(1)));
        }

        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint64_t>(verts, edges);
        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE_FALSE(mesh.faces.empty());
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh, constraints);
        require_edges_are_manifold(mesh);
    }
}

TEST_CASE("adaptive orient3d keeps near-degenerate signs under the normal build flags"){
    const std::array<double, 3> a{{ 0.27046243662747216, -0.82109361271069092,  0.11235779824475989 }};
    const std::array<double, 3> b{{ 0.57930393901296728, -0.55673265201320743, -0.16266294128208603 }};
    const std::array<double, 3> c{{-0.50044415316658108, -0.41627067894555503,  0.60647264433458070 }};
    const std::array<double, 3> d{{ 0.11644074082461947, -0.59803231455648442,  0.18538916709908485 }};

    REQUIRE(adaptive_predicate::orient3d(a.data(), b.data(), c.data(), d.data()) < 0.0);
}
