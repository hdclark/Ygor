#include "MeshBooleanExactKernelFixtures.h"
#include <iostream>
using namespace exact_test;
static void predicates(){require(orient2d(p2(0,0),p2(1,0),p2(0,1))==exact_sign::positive,"orient2d");require(orient3d(p3(0,0,0),p3(1,0,0),p3(0,1,0),p3(0,0,1))==exact_sign::positive,"orient3d");auto pl=support_plane(p3(0,0,0),p3(1,0,0),p3(0,1,0)).value();require(plane_side(pl,p3(0,0,1))==exact_sign::positive,"plane side");require(dominant_projection(pl)==projection_axis::drop_z,"projection");}
static void classifications(){exact_segment2 s{p2(0,0),p2(4,0)};require(classify_point_segment(p2(2,0),s)==point_segment_relation::open_interior,"point segment");auto poly=std::vector<exact_point2>{p2(0,0),p2(4,0),p2(4,4),p2(0,4)};require(classify_point_polygon(p2(2,2),poly).value().kind==point_region_kind::open_interior,"point polygon");require(classify_point_polygon(p2(4,2),poly).value().kind==point_region_kind::boundary_edge_interior,"polygon boundary");auto x=relate_segments(s,{p2(2,-1),p2(2,1)});require(x.dimension==intersection_dimension::point&&x.point_kind==segment_point_kind::proper_crossing&&*x.point==p2(2,0),"segment crossing");}
static void constructions(){auto pl=support_plane(p3(0,0,0),p3(2,0,0),p3(0,2,0)).value();auto x=intersect_line_plane({p3(1,1,-1),exact_vector3{q(0),q(0),q(2)}},pl);require(x.kind==line_plane_kind::unique&&plane_side(pl,*x.point)==exact_sign::zero,"line plane");auto pp=intersect_planes(pl,support_plane(p3(0,0,0),p3(0,1,0),p3(0,0,1)).value());require(pp.kind==plane_plane_kind::nonparallel,"plane plane");require(plane_side(pl,pp.line->anchor)==exact_sign::zero,"carrier substitution");}
static void polygon_relations(){exact_triangle3 a{p3(0,0,0),p3(4,0,0),p3(0,4,0)},b{p3(1,1,-1),p3(1,1,1),p3(3,1,0)},c{p3(1,1,0),p3(2,1,0),p3(1,2,0)};require(relate_triangles(a,b)==polygon_intersection_kind::segment,"crossing triangles");require(relate_triangles(a,c)==polygon_intersection_kind::area,"coplanar overlap");std::vector<exact_triangle3>tet{{p3(0,0,0),p3(0,4,0),p3(4,0,0)},{p3(0,0,0),p3(4,0,0),p3(0,0,4)},{p3(4,0,0),p3(0,4,0),p3(0,0,4)},{p3(0,4,0),p3(0,0,0),p3(0,0,4)}};require(classify_point_closed_triangle_shell(p3(1,1,1),tet).value()==solid_point_kind::inside,"point in shell");require(classify_point_closed_triangle_shell(p3(5,5,5),tet).value()==solid_point_kind::outside,"point outside shell");}
static void formal_point_location(){std::vector<exact_triangle3>tet{{p3(0,0,0),p3(0,4,0),p3(4,0,0)},{p3(0,0,0),p3(4,0,0),p3(0,0,4)},{p3(4,0,0),p3(0,4,0),p3(0,0,4)},{p3(0,4,0),p3(0,0,0),p3(0,0,4)}};formal_open_point_view inside{p3(1,1,1),exact_vector3{q(0),q(0),q(0)},{}},outside{p3(5,5,5),exact_vector3{q(0),q(0),q(0)},{}};auto a=locate_formal_open_point(inside,tet),b=locate_formal_open_point(outside,tet);require(a.has_value()&&a.value().location==formal_operand_location_kind::inside&&a.value().signed_degree==1,"formal inside degree");auto a2=locate_formal_open_point(inside,tet,static_cast<std::uint8_t>(a.value().ray_direction_index+1));require(a2.has_value()&&a2.value().signed_degree==a.value().signed_degree&&a2.value().ray_direction_index!=a.value().ray_direction_index,"alternate formal ray");require(b.has_value()&&b.value().location==formal_operand_location_kind::outside&&b.value().signed_degree==0,"formal outside degree");formal_open_point_view face_side{p3(1,1,0),exact_vector3{q(0),q(0),q(1)},{}};auto c=locate_formal_open_point(face_side,tet);require(c.has_value()&&c.value().signed_degree==1,"formal origin moves to occupied side");face_side.infinitesimal_direction=exact_vector3{q(0),q(0),q(-1)};auto d=locate_formal_open_point(face_side,tet);require(d.has_value()&&d.value().signed_degree==0,"formal origin moves to exterior side");}
static void grouped_ray_ownership(){
  const auto facet=facet_id::from_canonical_value(0);
  const auto v0=original_vertex_id::from_canonical_value(0),v1=original_vertex_id::from_canonical_value(1),v2=original_vertex_id::from_canonical_value(2),v3=original_vertex_id::from_canonical_value(3);
  const std::vector<exact_point3> square{p3(1,-1,-1),p3(1,1,-1),p3(1,1,1),p3(1,-1,1)};
  const std::vector<original_vertex_id> square_ids{v0,v1,v2,v3};
  formal_open_point_view center{p3(0,0,0),exact_vector3{q(0),q(1),q(0)}, {}};
  std::vector<sourced_exact_triangle3> diagonal_a{{{square[0],square[1],square[2]},facet,0,std::array<original_vertex_id,3>{v0,v1,v2},square,square_ids},{{square[0],square[2],square[3]},facet,1,std::array<original_vertex_id,3>{v0,v2,v3},square,square_ids}};
  std::vector<sourced_exact_triangle3> diagonal_b{{{square[0],square[1],square[3]},facet,0,std::array<original_vertex_id,3>{v0,v1,v3},square,square_ids},{{square[1],square[2],square[3]},facet,1,std::array<original_vertex_id,3>{v1,v2,v3},square,square_ids}};
  auto a=locate_formal_open_point(center,diagonal_a),b=locate_formal_open_point(center,diagonal_b);
  require(a.has_value()&&b.has_value()&&a.value().signed_degree==1&&b.value().signed_degree==1,"source polygon degree independent of diagonal");
  require(a.value().hits.size()==1&&b.value().hits.size()==1&&a.value().hits[0].source_facet==facet&&b.value().hits[0].source_facet==facet,"one source polygon owner");
  require(a.value().hits[0].ownership==formal_ray_ownership_kind::facet_interior&&b.value().hits[0].ownership==formal_ray_ownership_kind::facet_interior&&!a.value().hits[0].source_edge&&!b.value().hits[0].source_edge,"internal diagonals are not source edges");
  formal_open_point_view edge{p3(0,0,0),exact_vector3{q(0),q(1),q(0)}, {}};
  const std::vector<exact_point3> triangle_ring{p3(1,0,-1),p3(1,1,0),p3(1,0,1)};
  const std::vector<original_vertex_id> triangle_ids{v0,v1,v2};
  std::vector<sourced_exact_triangle3> edge_triangle{{{triangle_ring[0],triangle_ring[1],triangle_ring[2]},facet,0,std::array<original_vertex_id,3>{v0,v1,v2},triangle_ring,triangle_ids}};
  auto e=locate_formal_open_point(edge,edge_triangle);
  require(e.has_value()&&e.value().hits.size()==1&&e.value().hits[0].ownership==formal_ray_ownership_kind::source_edge&&e.value().hits[0].source_edge==std::optional<std::array<original_vertex_id,2>>({v0,v2})&&e.value().hits[0].source_edge_direction==std::optional<std::array<original_vertex_id,2>>({v2,v0})&&e.value().hits[0].owns_boundary_crossing,"ordered source edge ownership evidence");
  std::vector<sourced_exact_triangle3> shared_edge{edge_triangle[0],{{triangle_ring[0],triangle_ring[1],triangle_ring[2]},facet_id::from_canonical_value(2),0,std::array<original_vertex_id,3>{v0,v3,v2},triangle_ring,std::vector<original_vertex_id>{v0,v3,v2}}};
  auto se=locate_formal_open_point(edge,shared_edge);
  require(se.has_value()&&se.value().signed_degree==1&&se.value().hits.size()==2,"shared source edge contributes once");
  require(se.value().hits[0].source_edge==se.value().hits[1].source_edge&&se.value().hits[0].source_edge_direction==std::optional<std::array<original_vertex_id,2>>({v2,v0})&&se.value().hits[0].signed_contribution+se.value().hits[1].signed_contribution==1&&se.value().hits[0].owns_boundary_crossing!=se.value().hits[1].owns_boundary_crossing,"shared source edge has one ordered owner");
  require(validate_formal_ray_ownership_evidence(se.value()),"shared source edge replay evidence");
  auto damaged=se.value();damaged.hits[0].source_edge_direction=std::array<original_vertex_id,2>{v0,v1};
  require(!validate_formal_ray_ownership_evidence(damaged),"reject mismatched ordered edge replay evidence");
  damaged=se.value();for(auto &hit:damaged.hits)hit.owns_boundary_crossing=false;
  require(!validate_formal_ray_ownership_evidence(damaged),"reject missing crossing owner replay evidence");
  formal_open_point_view vertex{p3(0,0,0),exact_vector3{q(0),q(1),q(1)}, {}};
  const std::vector<exact_point3> vertex_ring{p3(1,0,0),p3(1,2,0),p3(1,0,2)};
  const std::vector<facet_id> vertex_fan{facet,facet_id::from_canonical_value(2),facet_id::from_canonical_value(3)};
  std::vector<sourced_exact_triangle3> vertex_triangle{{{vertex_ring[0],vertex_ring[1],vertex_ring[2]},facet,0,std::array<original_vertex_id,3>{v0,v1,v2},vertex_ring,triangle_ids,{vertex_fan,{facet},{facet}}}};
  auto v=locate_formal_open_point(vertex,vertex_triangle);
  require(v.has_value()&&v.value().hits.size()==1&&v.value().hits[0].ownership==formal_ray_ownership_kind::source_vertex&&v.value().hits[0].source_vertex==v0&&v.value().hits[0].source_vertex_fan==vertex_fan&&v.value().hits[0].source_vertex_fan_index==0,"ordered source vertex fan ownership evidence");
  require(validate_formal_ray_ownership_evidence(v.value()),"source vertex fan replay evidence");
  auto damaged_vertex=v.value();damaged_vertex.hits[0].source_vertex_fan_index=1;
  require(!validate_formal_ray_ownership_evidence(damaged_vertex),"reject wrong source vertex fan owner");
  damaged_vertex=v.value();damaged_vertex.hits[0].source_vertex_fan.push_back(facet);
  require(!validate_formal_ray_ownership_evidence(damaged_vertex),"reject duplicate source vertex fan member");
  const auto opposite=facet_id::from_canonical_value(1);
  const std::vector<exact_point3> tangent_ring{p3(1,-2,-2),p3(1,2,-2),p3(1,0,2)};
  std::vector<sourced_exact_triangle3> tangent{{{tangent_ring[0],tangent_ring[1],tangent_ring[2]},facet,0,std::array<original_vertex_id,3>{v0,v1,v2},tangent_ring,triangle_ids},{{tangent_ring[2],tangent_ring[1],tangent_ring[0]},opposite,0,std::array<original_vertex_id,3>{v2,v1,v0},tangent_ring,triangle_ids}};
  auto t=locate_formal_open_point(center,tangent);
  require(t.has_value()&&t.value().signed_degree==0&&t.value().hits.size()==2&&t.value().hits[0].parameter_group==t.value().hits[1].parameter_group&&t.value().hits[0].ownership==formal_ray_ownership_kind::tangent&&t.value().hits[1].ownership==formal_ray_ownership_kind::tangent&&!t.value().hits[0].owns_boundary_crossing&&!t.value().hits[1].owns_boundary_crossing,"equal parameter tangent has no owner");
  require(validate_formal_ray_ownership_evidence(t.value()),"tangent replay evidence");
  auto false_crossing=t.value();false_crossing.hits[0].owns_boundary_crossing=true;
  require(!validate_formal_ray_ownership_evidence(false_crossing),"reject tangent crossing owner evidence");
}
static void stable_formal_ray_fallback(){
  const std::array<exact_vector3,10> directions{{
      {q(1),q(0),q(0)},{q(0),q(1),q(0)},{q(0),q(0),q(1)},
      {q(1),q(1),q(1)},{q(1),q(2),q(4)},{q(1),q(4),q(2)},
      {q(2),q(1),q(4)},{q(4),q(1),q(2)},{q(2),q(4),q(1)},
      {q(4),q(2),q(1)}}};
  std::vector<exact_triangle3> coplanar;
  for(const auto &d:directions){
    const exact_vector3 auxiliary=d.x.is_zero()?exact_vector3{q(1),q(0),q(0)}:exact_vector3{q(0),q(1),q(0)};
    coplanar.push_back({p3(0,0,0),exact_point3{d.x,d.y,d.z},exact_point3{auxiliary.x,auxiliary.y,auxiliary.z}});
  }
  formal_open_point_view query{p3(0,0,0),exact_vector3{q(0),q(0),q(0)}, {perturbation_domain::generic_ray,{},7}};
  auto result=locate_formal_open_point(query,coplanar);
  require(result.has_value()&&result.value().ray_direction_index==10&&result.value().signed_degree==0,"stable exact ray fallback after fixed-direction exhaustion");
  auto alternate=locate_formal_open_point(query,coplanar,static_cast<std::uint8_t>(result.value().ray_direction_index+1));
  require(alternate.has_value()&&alternate.value().ray_direction_index!=result.value().ray_direction_index&&alternate.value().signed_degree==result.value().signed_degree,"stable fallback has a distinct wrapping alternate");
}
static void radial_and_cones(){auto xy=support_plane(p3(0,0,0),p3(1,0,0),p3(0,1,0)).value(),xz=support_plane(p3(0,0,0),p3(1,0,0),p3(0,0,1)).value();auto order=rank_planes_around_carrier(exact_vector3{q(1),q(0),q(0)},{xy,xz});require(order.has_value()&&order.value().layers.size()==2,"exact carrier radial layers");auto witness=construct_strict_cone_witness({{exact_vector3{q(1),q(0),q(0)},exact_sign::positive},{exact_vector3{q(0),q(1),q(0)},exact_sign::positive}});require(witness.has_value()&&witness.value().evaluations==std::vector<exact_sign>({exact_sign::positive,exact_sign::positive}),"strict cone witness");}
int main(){try{predicates();classifications();constructions();polygon_relations();formal_point_location();grouped_ray_ownership();stable_formal_ray_fallback();radial_and_cones();exact_kernel<double>k;require(k.coordinate_type()==coordinate_tag::binary64&&!k.arithmetic_policy_bytes().empty(),"service");std::cout<<"PASS exact kernel\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
