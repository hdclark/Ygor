#pragma once
#ifndef YGOR_MESHES_EXACT_KERNEL_H_
#define YGOR_MESHES_EXACT_KERNEL_H_
#include "YgorMeshesExactArithmetic.h"
#include <optional>
#include <type_traits>

namespace ygor { namespace mesh_boolean {
struct exact_point2{exact_scalar x,y;}; struct exact_vector2{exact_scalar x,y;};
struct exact_point3{exact_scalar x,y,z;}; struct exact_vector3{exact_scalar x,y,z;};
inline bool operator==(const exact_point2&a,const exact_point2&b){return a.x==b.x&&a.y==b.y;}inline bool operator==(const exact_point3&a,const exact_point3&b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
enum class projection_axis:std::uint8_t{drop_x,drop_y,drop_z}; enum class orientation_parity:std::int8_t{opposite=-1,agree=1};
struct exact_segment2{exact_point2 origin,destination;};struct exact_segment3{exact_point3 origin,destination;};
struct exact_triangle3{exact_point3 a,b,c;};
struct exact_line2{exact_point2 anchor;exact_vector2 direction;};struct exact_line3{exact_point3 anchor;exact_vector3 direction;};struct exact_ray3{exact_point3 anchor;exact_vector3 direction;};
struct exact_interval{exact_scalar lower,upper;bool lower_closed=true,upper_closed=true;};
struct exact_box2{exact_point2 minimum,maximum;};struct exact_box3{exact_point3 minimum,maximum;};
struct exact_plane3{big_int a,b,c,d;orientation_parity oriented=orientation_parity::agree;};

enum class construction_kind:std::uint8_t{exact_relation};
enum class defining_relation_kind:std::uint8_t{
    coordinate_equality,point_on_plane,point_on_line_or_carrier,
    affine_parameter,equal_point,ordered_on_carrier
};
struct defining_relation {
    defining_relation_id id;
    defining_relation_kind kind=defining_relation_kind::coordinate_equality;
    std::uint16_t formula_version=1;
    construction_node_id construction;
    std::vector<construction_node_id> operand_nodes;
    std::vector<feature_ref> defining_sources;
    // Schema-v1 relations use a canonical affine polynomial ax+by+cz+d.
    std::array<exact_scalar,4> coefficients{{exact_scalar(0),exact_scalar(0),exact_scalar(0),exact_scalar(0)}};
    exact_sign expected=exact_sign::zero;
};
struct construction_node {
    construction_node_id id;
    construction_kind kind=construction_kind::exact_relation;
    std::vector<construction_node_id> children;
    std::vector<feature_ref> defining_sources;
    std::vector<defining_relation_id> defining_relations;
    std::vector<std::uint8_t> exact_result;
};
struct construction_storage {
    context_owner_token owner;
    std::vector<construction_node> nodes;
    std::vector<defining_relation> relations;
};
exact_sign substitute_defining_relation(const defining_relation&,const exact_point3&);
bool defining_relation_satisfied(const defining_relation&,const exact_point3&);

exact_vector2 operator-(const exact_point2&,const exact_point2&);exact_vector3 operator-(const exact_point3&,const exact_point3&);
exact_point2 operator+(const exact_point2&,const exact_vector2&);exact_point3 operator+(const exact_point3&,const exact_vector3&);
exact_vector2 operator*(const exact_vector2&,const exact_scalar&);exact_vector3 operator*(const exact_vector3&,const exact_scalar&);
exact_scalar dot(const exact_vector2&,const exact_vector2&);exact_scalar dot(const exact_vector3&,const exact_vector3&);exact_vector3 cross(const exact_vector3&,const exact_vector3&);
exact_sign orient2d(const exact_point2&,const exact_point2&,const exact_point2&);exact_sign orient3d(const exact_point3&,const exact_point3&,const exact_point3&,const exact_point3&);
int lexicographic_compare(const exact_point2&,const exact_point2&);int lexicographic_compare(const exact_point3&,const exact_point3&);
exact_point2 project(const exact_point3&,projection_axis);
status_or<exact_plane3>support_plane(const exact_point3&,const exact_point3&,const exact_point3&);
exact_sign plane_side(const exact_plane3&,const exact_point3&);projection_axis dominant_projection(const exact_plane3&);

enum class point_line_relation:std::uint8_t{off_carrier,on_carrier};
enum class point_segment_relation:std::uint8_t{off_carrier,before_origin,at_origin,open_interior,at_destination,after_destination};
enum class point_region_kind:std::uint8_t{outside,open_interior,boundary_vertex,boundary_edge_interior};
struct point_region_relation{point_region_kind kind=point_region_kind::outside;std::size_t feature=0;};
point_line_relation classify_point_line(const exact_point2&,const exact_line2&);point_line_relation classify_point_line(const exact_point3&,const exact_line3&);
point_segment_relation classify_point_segment(const exact_point2&,const exact_segment2&);point_segment_relation classify_point_segment(const exact_point3&,const exact_segment3&);
status_or<point_region_relation>classify_point_triangle(const exact_point3&,const exact_point3&,const exact_point3&,const exact_point3&);
status_or<point_region_relation>classify_point_polygon(const exact_point2&,const std::vector<exact_point2>&);
enum class polygon_intersection_kind:std::uint8_t{disjoint,point,segment,area};
polygon_intersection_kind relate_triangles(const exact_triangle3&,const exact_triangle3&);
polygon_intersection_kind relate_polygons(const std::vector<exact_triangle3>&,const std::vector<exact_triangle3>&);
enum class perturbation_domain:std::uint8_t{generic_ray,ray_owner,open_side,equal_candidate,radial_order};struct perturbation_key{perturbation_domain domain;std::vector<feature_ref>stable_features;std::uint32_t local_rank=0;};
template<class R,class D>struct symbolic_result{R unperturbed;std::optional<D>decision;perturbation_key key;};
enum class solid_point_kind:std::uint8_t{outside,inside,boundary};
status_or<solid_point_kind>classify_point_closed_triangle_shell(const exact_point3&,const std::vector<exact_triangle3>&);

// A query-local open point. Coordinates remain exact; epsilon is a formal,
// positive infinitesimal and is never assigned a floating-point value.
struct formal_open_point_view {
    exact_point3 base;
    exact_vector3 infinitesimal_direction;
    perturbation_key key{perturbation_domain::generic_ray,{},0};
};
enum class formal_operand_location_kind:std::uint8_t{outside,inside};
enum class formal_ray_ownership_kind:std::uint8_t{facet_interior,source_edge,source_vertex,tangent};
struct formal_ray_hit {
    std::size_t triangle=0;
    std::optional<facet_id> source_facet;
    std::uint32_t source_primitive=0;
    exact_scalar parameter_constant;
    exact_scalar parameter_epsilon;
    std::int8_t signed_contribution=0;
    std::uint32_t parameter_group=0;
    formal_ray_ownership_kind ownership=formal_ray_ownership_kind::facet_interior;
    std::optional<original_vertex_id> source_vertex;
    // Complete cyclic incident-facet order at source_vertex and the member
    // selected by ray_boundary_owner_v1. This makes vertex ownership replay
    // independent of facet and triangulation insertion order.
    std::vector<facet_id> source_vertex_fan;
    std::optional<std::uint32_t> source_vertex_fan_index;
    std::optional<std::array<original_vertex_id,2>> source_edge;
    // Directed as it occurs in source_facet's ring. source_edge remains the
    // canonical unordered topology key used to join the two incident facets.
    std::optional<std::array<original_vertex_id,2>> source_edge_direction;
    bool owns_boundary_crossing=false;
};
struct sourced_exact_triangle3 {
    exact_triangle3 triangle;
    facet_id source_facet;
    std::uint32_t source_primitive=0;
    std::optional<std::array<original_vertex_id,3>> source_vertices;
    std::vector<exact_point3> source_ring;
    std::vector<original_vertex_id> source_ring_vertices;
    // Aligned with source_ring_vertices; each entry is the complete cyclic
    // outgoing-link facet order at that source vertex.
    std::vector<std::vector<facet_id>> source_vertex_fans;
};
struct formal_operand_location {
    formal_operand_location_kind location=formal_operand_location_kind::outside;
    std::int32_t signed_degree=0;
    // 0..9 identify generic_ray_v1 directions; later indices identify exact
    // stable-key fallbacks whose coefficients are retained in ray_direction.
    std::uint8_t ray_direction_index=0;
    exact_vector3 ray_direction;
    std::vector<formal_ray_hit> hits;
};
bool validate_formal_ray_ownership_evidence(const formal_operand_location&) noexcept;
status_or<formal_operand_location> locate_formal_open_point(const formal_open_point_view&,const std::vector<exact_triangle3>&,std::uint8_t first_direction=0);
status_or<formal_operand_location> locate_formal_open_point(const formal_open_point_view&,const std::vector<sourced_exact_triangle3>&,std::uint8_t first_direction=0);

enum class carrier_relation:std::uint8_t{skew,nonparallel_coplanar,parallel_distinct,collinear};enum class intersection_dimension:std::uint8_t{empty,point,segment};
enum class segment_point_kind:std::uint8_t{none,proper_crossing,endpoint_endpoint,endpoint_interior};enum class overlap_kind:std::uint8_t{none,partial,first_contains_second,second_contains_first,equal_same_direction,equal_opposite_direction};
struct segment_relation2{carrier_relation carrier=carrier_relation::parallel_distinct;intersection_dimension dimension=intersection_dimension::empty;segment_point_kind point_kind=segment_point_kind::none;overlap_kind overlap=overlap_kind::none;std::optional<exact_point2>point;std::optional<exact_segment2>overlap_segment;std::optional<exact_scalar>first_parameter,second_parameter;};
struct segment_relation3{carrier_relation carrier=carrier_relation::skew;intersection_dimension dimension=intersection_dimension::empty;segment_point_kind point_kind=segment_point_kind::none;overlap_kind overlap=overlap_kind::none;std::optional<exact_point3>point;std::optional<exact_segment3>overlap_segment;std::optional<exact_scalar>first_parameter,second_parameter;};
segment_relation2 relate_segments(const exact_segment2&,const exact_segment2&);segment_relation3 relate_segments(const exact_segment3&,const exact_segment3&);

enum class segment_plane_kind:std::uint8_t{strictly_positive,strictly_negative,proper_crossing,origin_on_plane,destination_on_plane,contained};
struct segment_plane_relation{segment_plane_kind kind;std::optional<exact_scalar>parameter;std::optional<exact_point3>point;};
segment_plane_relation relate_segment_plane(const exact_segment3&,const exact_plane3&);
enum class line_plane_kind:std::uint8_t{unique,parallel_disjoint,contained};struct line_plane_result{line_plane_kind kind;std::optional<exact_scalar>parameter;std::optional<exact_point3>point;};
line_plane_result intersect_line_plane(const exact_line3&,const exact_plane3&);
enum class line_line_kind:std::uint8_t{unique,parallel_disjoint,coincident};struct line_line_result2{line_line_kind kind;std::optional<exact_scalar>first_parameter,second_parameter;std::optional<exact_point2>point;};
line_line_result2 intersect_lines(const exact_line2&,const exact_line2&);
enum class plane_plane_kind:std::uint8_t{nonparallel,parallel_disjoint,coincident_same,coincident_opposite};struct plane_plane_result{plane_plane_kind kind;std::optional<exact_line3>line;};
plane_plane_result intersect_planes(const exact_plane3&,const exact_plane3&);
exact_point2 affine_interpolate(const exact_point2&,const exact_point2&,const exact_scalar&);exact_point3 affine_interpolate(const exact_point3&,const exact_point3&,const exact_scalar&);

enum class open_side_decision:std::int8_t{negative=-1,positive=1};
symbolic_result<exact_sign,open_side_decision>open_side(const exact_plane3&,const exact_point3&,bool positive,const perturbation_key&);
struct carrier_radial_layer { std::vector<std::size_t> members; exact_vector3 ray; };
struct exact_carrier_radial_order { exact_vector3 carrier; std::vector<carrier_radial_layer> layers; };
status_or<exact_carrier_radial_order> rank_planes_around_carrier(const exact_vector3&,const std::vector<exact_plane3>&);
struct strict_cone_constraint { exact_vector3 normal; exact_sign required=exact_sign::positive; };
struct exact_cone_witness { exact_vector3 direction; std::vector<exact_sign> evaluations; };
status_or<exact_cone_witness> construct_strict_cone_witness(const std::vector<strict_cone_constraint>&);

enum class predicate_execution_policy:std::uint8_t{automatic,exact_only,force_filter_attempt,force_exact_fallback};
template<class T>class exact_kernel final:public exact_kernel_services<T>{static_assert(std::is_same<T,float>::value||std::is_same<T,double>::value,"binary float only");public:coordinate_tag coordinate_type()const noexcept override{return std::is_same<T,float>::value?coordinate_tag::binary32:coordinate_tag::binary64;}std::vector<std::uint8_t>arithmetic_policy_bytes()const override;std::uint64_t implementation_type_tag()const noexcept override{return 0x5947424b45523033ULL;}status_or<decoded_coordinate<T>>decode(T v,boolean_stage s=boolean_stage::input_validation)const{return decode_coordinate(v,s);}exact_sign orientation(const exact_point2&a,const exact_point2&b,const exact_point2&c,predicate_execution_policy={})const{return orient2d(a,b,c);}exact_sign orientation(const exact_point3&a,const exact_point3&b,const exact_point3&c,const exact_point3&d,predicate_execution_policy={})const{return orient3d(a,b,c,d);}};
extern template class exact_kernel<float>;extern template class exact_kernel<double>;
} }
#endif
