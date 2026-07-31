#include "MeshBooleanExactKernelFixtures.h"
#include <YgorMeshesBooleanPerformance.h>
#include <algorithm>
#include <cfenv>
#include <iostream>
using namespace exact_test;
static void predicates(){require(orient2d(p2(0,0),p2(1,0),p2(0,1))==exact_sign::positive,"orient2d");require(orient3d(p3(0,0,0),p3(1,0,0),p3(0,1,0),p3(0,0,1))==exact_sign::positive,"orient3d");auto pl=support_plane(p3(0,0,0),p3(1,0,0),p3(0,1,0)).value();require(plane_side(pl,p3(0,0,1))==exact_sign::positive,"plane side");require(dominant_projection(pl)==projection_axis::drop_z,"projection");}
static void certified_filters(){
  exact_kernel<float> k32;exact_kernel<double> k64;
  const auto a=p2(0,0),b=p2(4,0),c=p2(0,3);
  require(exact_filter_policy::orient2d(a,b,c,coordinate_tag::binary32)==exact_sign::positive,"orient2d binary32 accepts");
  require(exact_filter_policy::orient2d(a,b,c,coordinate_tag::binary64)==exact_sign::positive,"orient2d binary64 accepts");
  require(k32.orientation(a,b,c,predicate_execution_policy::force_filter_attempt)==orient2d_exact(a,b,c),"orient2d forced attempt");
  require(k64.orientation(a,b,c,predicate_execution_policy::force_exact_fallback)==orient2d_exact(a,b,c),"orient2d forced fallback");
  const auto p0=p3(0,0,0),p1=p3(3,0,0),p2v=p3(0,4,0),p3v=p3(0,0,5);
  require(exact_filter_policy::orient3d(p0,p1,p2v,p3v,coordinate_tag::binary64)==exact_sign::positive,"orient3d accepts");
  require(k64.orientation(p0,p1,p2v,p3v,predicate_execution_policy::force_filter_attempt)==orient3d_exact(p0,p1,p2v,p3v),"orient3d forced attempt");
  auto plane=support_plane(p0,p1,p2v).value();
  require(exact_filter_policy::plane_side(plane,p3v,coordinate_tag::binary64)==exact_sign::positive,"plane filter accepts");
  require(k64.side(plane,p3v,predicate_execution_policy::force_exact_fallback)==plane_side_exact(plane,p3v),"plane forced fallback");
  exact_vector3 u{q(1),q(2),q(3)},v{q(4),q(5),q(6)};
  require(exact_filter_policy::dot_sign(u,v,coordinate_tag::binary64)==exact_sign::positive,"dot filter accepts");
  require(k64.sign_of_dot(u,v,predicate_execution_policy::force_exact_fallback)==dot_sign_exact(u,v),"dot forced fallback");
  require(!exact_filter_policy::orient2d(a,b,p2(8,0),coordinate_tag::binary64),"exact zero falls back");
  const exact_point2 thirds{q(1,3),q(1,3)};
  require(!exact_filter_policy::orient2d(thirds,p2(1,1),p2(2,3),coordinate_tag::binary64),"non-dyadic falls back");
  const auto denorm=decode_coordinate(from_bits<float>(1U)).value().value;
  require(!exact_filter_policy::orient2d({q(0),q(0)},{denorm,q(0)},{q(0),denorm},coordinate_tag::binary32),"subnormal filter bypass");
  const auto negative_zero=decode_coordinate(from_bits<float>(0x80000000U)).value().value;
  require(!exact_filter_policy::orient2d({negative_zero,q(0)},b,p2(8,0),coordinate_tag::binary32),"signed zero degeneracy falls back");
  const auto original_round=std::fegetround();
  if(std::fesetround(FE_DOWNWARD)==0){require(!exact_filter_policy::orient2d(a,b,c,coordinate_tag::binary64),"non-nearest filter bypass");require(orient2d(a,b,c)==orient2d_exact(a,b,c),"ambient rounding exact fallback");require(std::fegetround()==FE_DOWNWARD,"filter preserves ambient rounding");require(std::fesetround(original_round)==0,"restore rounding");}
  require(exact_filter_policy::orient2d_proof_version>0&&exact_filter_policy::orient3d_proof_version>0&&exact_filter_policy::plane_side_proof_version>0&&exact_filter_policy::dot_sign_proof_version>0,"proof versions enabled");
  require(k32.arithmetic_policy_bytes()!=k64.arithmetic_policy_bytes(),"coordinate policy identity");
  performance_collector collector;{performance_scope scope(&collector,boolean_stage::input_validation,performance_role::producer);(void)orient2d(a,b,c);(void)orient2d(a,b,p2(8,0));(void)k64.orientation(a,b,c,predicate_execution_policy::force_exact_fallback);}
  const auto counters=collector.snapshot()->stage(boolean_stage::input_validation).producer;
  require(counters.value(performance_counter::orient2d_filter_accepts)>0&&counters.value(performance_counter::orient2d_filter_fallbacks)>0&&counters.value(performance_counter::exact_fallbacks)>0,"filter counters");
}
static void classifications(){exact_segment2 s{p2(0,0),p2(4,0)};require(classify_point_segment(p2(2,0),s)==point_segment_relation::open_interior,"point segment");auto poly=std::vector<exact_point2>{p2(0,0),p2(4,0),p2(4,4),p2(0,4)};require(classify_point_polygon(p2(2,2),poly).value().kind==point_region_kind::open_interior,"point polygon");require(classify_point_polygon(p2(4,2),poly).value().kind==point_region_kind::boundary_edge_interior,"polygon boundary");auto x=relate_segments(s,{p2(2,-1),p2(2,1)});require(x.dimension==intersection_dimension::point&&x.point_kind==segment_point_kind::proper_crossing&&*x.point==p2(2,0),"segment crossing");}
static bool same_plane(const exact_plane3&a,const exact_plane3&b){return a.a==b.a&&a.b==b.b&&a.c==b.c&&a.d==b.d&&a.oriented==b.oriented;}
static void division_free_incidence(){
  const exact_segment2 s{p2(4,2),p2(-2,-1)};
  require(classify_on_segment(p2(6,3),s)==point_segment_relation::before_origin,"negative segment before");
  require(classify_on_segment(p2(4,2),s)==point_segment_relation::at_origin,"negative segment origin");
  require(classify_on_segment(p2(0,0),s)==point_segment_relation::open_interior,"negative segment interior");
  require(classify_on_segment(p2(-2,-1),s)==point_segment_relation::at_destination,"negative segment destination");
  require(classify_on_segment(p2(-4,-2),s)==point_segment_relation::after_destination,"negative segment after");
  require(!is_on_closed_segment(p2(0,1),s)&&is_on_closed_segment(p2(0,0),s),"closed segment incidence");
  const exact_segment3 s3{p3(1,2,3),p3(5,6,7)};
  require(classify_on_segment(p3(3,4,5),s3)==point_segment_relation::open_interior,"3D segment interior");
  require(classify_on_segment(p3(3,4,6),s3)==point_segment_relation::off_carrier,"3D segment carrier");
  performance_collector incidence;{
    performance_scope scope(&incidence,boolean_stage::input_validation,performance_role::producer);
    (void)classify_on_segment(p2(0,0),s);(void)is_on_closed_segment(p3(3,4,5),s3);
    const std::vector<exact_point2> polygon{p2(0,0),p2(4,0),p2(4,4),p2(0,4)};
    (void)classify_point_polygon(p2(2,2),polygon);(void)classify_point_polygon(p2(4,2),polygon);(void)classify_point_polygon(p2(5,4),polygon);
  }
  require(incidence.snapshot()->stage(boolean_stage::input_validation).producer.value(performance_counter::geometric_exact_divisions)==0,"incidence performs no division");
  performance_collector construction;{
    performance_scope scope(&construction,boolean_stage::input_validation,performance_role::producer);
    require(segment_parameter(p2(0,0),s)==q(2,3),"requested segment parameter");
  }
  require(construction.snapshot()->stage(boolean_stage::input_validation).producer.value(performance_counter::geometric_exact_divisions)==1,"parameter performs one division");
}
static void dyadic_planes(){
  const exact_point3 a{q(1,8),q(-3,4),q(5,2)},b{q(9,8),q(1,4),q(5,2)},c{q(1,8),q(-3,4),q(7,2)};
  auto fast=support_plane_dyadic(a,b,c),general=support_plane(a,b,c);
  require(fast.has_value()&&general.has_value()&&same_plane(fast.value(),general.value()),"dyadic plane differential");
  std::array<exact_point3,3> points{{a,b,c}};std::array<unsigned,3> order{{0,1,2}};
  do{auto x=support_plane_dyadic(points[order[0]],points[order[1]],points[order[2]]),y=support_plane(points[order[0]],points[order[1]],points[order[2]]);require(x.has_value()&&y.has_value()&&same_plane(x.value(),y.value()),"dyadic plane permutation");}while(std::next_permutation(order.begin(),order.end()));
  const exact_point3 third{q(1,3),q(0),q(0)};
  require(!support_plane_dyadic(third,p3(0,1,0),p3(0,0,1)).has_value(),"non-dyadic plane rejected");
  require(!support_plane_dyadic(p3(0,0,0),p3(1,1,1),p3(2,2,2)).has_value(),"dyadic collinear plane rejected");
  const auto negative_zero=decode_coordinate(from_bits<float>(0x80000000U)).value().value;
  const auto subnormal=decode_coordinate(from_bits<float>(1U)).value().value;
  const exact_point3 e0{negative_zero,q(0),q(0)},e1{q(1),q(0),q(0)},e2{q(0),subnormal,q(0)};
  fast=support_plane_dyadic(e0,e1,e2);general=support_plane(e0,e1,e2);
  require(fast.has_value()&&general.has_value()&&same_plane(fast.value(),general.value()),"signed-zero subnormal plane differential");
}
static void constructions(){auto pl=support_plane(p3(0,0,0),p3(2,0,0),p3(0,2,0)).value();auto x=intersect_line_plane({p3(1,1,-1),exact_vector3{q(0),q(0),q(2)}},pl);require(x.kind==line_plane_kind::unique&&plane_side(pl,*x.point)==exact_sign::zero,"line plane");auto pp=intersect_planes(pl,support_plane(p3(0,0,0),p3(0,1,0),p3(0,0,1)).value());require(pp.kind==plane_plane_kind::nonparallel,"plane plane");require(plane_side(pl,pp.line->anchor)==exact_sign::zero,"carrier substitution");}
static void polygon_relations(){exact_triangle3 a{p3(0,0,0),p3(4,0,0),p3(0,4,0)},b{p3(1,1,-1),p3(1,1,1),p3(3,1,0)},c{p3(1,1,0),p3(2,1,0),p3(1,2,0)};require(relate_triangles(a,b)==polygon_intersection_kind::segment,"crossing triangles");require(relate_triangles(a,c)==polygon_intersection_kind::area,"coplanar overlap");std::vector<exact_triangle3>tet{{p3(0,0,0),p3(0,4,0),p3(4,0,0)},{p3(0,0,0),p3(4,0,0),p3(0,0,4)},{p3(4,0,0),p3(0,4,0),p3(0,0,4)},{p3(0,4,0),p3(0,0,0),p3(0,0,4)}};require(classify_point_closed_triangle_shell(p3(1,1,1),tet).value()==solid_point_kind::inside,"point in shell");require(classify_point_closed_triangle_shell(p3(5,5,5),tet).value()==solid_point_kind::outside,"point outside shell");}
static void formal_point_location(){std::vector<exact_triangle3>tet{{p3(0,0,0),p3(0,4,0),p3(4,0,0)},{p3(0,0,0),p3(4,0,0),p3(0,0,4)},{p3(4,0,0),p3(0,4,0),p3(0,0,4)},{p3(0,4,0),p3(0,0,0),p3(0,0,4)}};formal_open_point_view inside{p3(1,1,1),exact_vector3{q(0),q(0),q(0)},{}},outside{p3(5,5,5),exact_vector3{q(0),q(0),q(0)},{}};auto a=locate_formal_open_point(inside,tet),b=locate_formal_open_point(outside,tet);require(a.has_value()&&a.value().location==formal_operand_location_kind::inside&&a.value().signed_degree==1,"formal inside degree");auto a2=locate_formal_open_point(inside,tet,static_cast<std::uint8_t>(a.value().ray_direction_index+1));require(a2.has_value()&&a2.value().signed_degree==a.value().signed_degree&&a2.value().ray_direction_index!=a.value().ray_direction_index,"alternate formal ray");require(b.has_value()&&b.value().location==formal_operand_location_kind::outside&&b.value().signed_degree==0,"formal outside degree");formal_open_point_view face_side{p3(1,1,0),exact_vector3{q(0),q(0),q(1)},{}};auto c=locate_formal_open_point(face_side,tet);require(c.has_value()&&c.value().signed_degree==1,"formal origin moves to occupied side");face_side.infinitesimal_direction=exact_vector3{q(0),q(0),q(-1)};auto d=locate_formal_open_point(face_side,tet);require(d.has_value()&&d.value().signed_degree==0,"formal origin moves to exterior side");}
static bool same_formal_location(const formal_operand_location&a,const formal_operand_location&b){
  if(a.location!=b.location||a.signed_degree!=b.signed_degree||a.ray_direction_index!=b.ray_direction_index||a.ray_direction.x!=b.ray_direction.x||a.ray_direction.y!=b.ray_direction.y||a.ray_direction.z!=b.ray_direction.z||a.hits.size()!=b.hits.size())return false;
  for(std::size_t i=0;i<a.hits.size();++i){const auto&x=a.hits[i];const auto&y=b.hits[i];if(x.triangle!=y.triangle||x.source_facet!=y.source_facet||x.source_primitive!=y.source_primitive||x.parameter_constant!=y.parameter_constant||x.parameter_epsilon!=y.parameter_epsilon||x.signed_contribution!=y.signed_contribution||x.parameter_group!=y.parameter_group||x.ownership!=y.ownership||x.source_vertex!=y.source_vertex||x.source_vertex_fan!=y.source_vertex_fan||x.source_vertex_fan_index!=y.source_vertex_fan_index||x.source_edge!=y.source_edge||x.source_edge_direction!=y.source_edge_direction||x.owns_boundary_crossing!=y.owns_boundary_crossing)return false;}return true;
}
static void require_index_differential(const formal_open_point_view&qv,const sourced_exact_operand3&operand,std::uint8_t direction=0){const auto index=build_sourced_exact_ray_index(operand);const auto exhaustive=locate_formal_open_point(qv,operand,direction),accelerated=locate_formal_open_point(qv,operand,index,direction);require(exhaustive.has_value()==accelerated.has_value(),"ray index outcome differential");if(exhaustive.has_value())require(same_formal_location(exhaustive.value(),accelerated.value()),"complete ray index evidence differential");else require(exhaustive.error().code==accelerated.error().code&&exhaustive.error().subcode==accelerated.error().subcode,"ray index error differential");}
static void grouped_ray_ownership(){
  const auto facet=facet_id::from_canonical_value(0);
  const auto v0=original_vertex_id::from_canonical_value(0),v1=original_vertex_id::from_canonical_value(1),v2=original_vertex_id::from_canonical_value(2),v3=original_vertex_id::from_canonical_value(3);
  auto add_facet=[](sourced_exact_operand3&operand,facet_id id,
                    const std::vector<exact_point3>&ring,
                    const std::vector<original_vertex_id>&ids,
                    std::vector<std::vector<facet_id>>fans,
                    const std::vector<std::pair<exact_triangle3,std::array<original_vertex_id,3>>>&triangles){
    auto plane=support_plane(triangles[0].first.a,triangles[0].first.b,
                             triangles[0].first.c).value();
    exact_vector3 normal{exact_scalar(plane.a,big_uint(1)),exact_scalar(plane.b,big_uint(1)),exact_scalar(plane.c,big_uint(1))};
    if(plane.oriented==orientation_parity::opposite)normal=normal*exact_scalar(-1);
    if(fans.empty())fans.resize(ring.size());
    sourced_exact_facet3 source;source.source_facet=id;source.source_shell=shell_id::from_canonical_value(0);source.projection=dominant_projection(plane);source.source_plane=plane;source.source_normal=normal;source.source_ring=ring;source.source_ring_vertices=ids;source.source_vertex_fans=std::move(fans);source.triangle_begin=operand.triangles.size();
    for(std::size_t i=0;i<triangles.size();++i)operand.triangles.push_back({triangles[i].first,id,static_cast<std::uint32_t>(i),triangles[i].second,operand.facets.size()});
    source.triangle_end=operand.triangles.size();for(const auto&p:ring)source.projected_ring.push_back(project(p,source.projection));operand.facets.push_back(std::move(source));
  };
  const std::vector<exact_point3> square{p3(1,-1,-1),p3(1,1,-1),p3(1,1,1),p3(1,-1,1)};
  const std::vector<original_vertex_id> square_ids{v0,v1,v2,v3};
  formal_open_point_view center{p3(0,0,0),exact_vector3{q(0),q(1),q(0)}, {}};
  sourced_exact_operand3 diagonal_a,diagonal_b;
  add_facet(diagonal_a,facet,square,square_ids,{},{{{square[0],square[1],square[2]},{v0,v1,v2}},{{square[0],square[2],square[3]},{v0,v2,v3}}});
  add_facet(diagonal_b,facet,square,square_ids,{},{{{square[0],square[1],square[3]},{v0,v1,v3}},{{square[1],square[2],square[3]},{v1,v2,v3}}});
  require(diagonal_a.facets.size()==1&&diagonal_a.facets[0].source_ring.size()==4&&diagonal_a.triangles.size()==2&&diagonal_a.triangles[0].source_facet_index==diagonal_a.triangles[1].source_facet_index,"source facet geometry stored once");
  auto a=locate_formal_open_point(center,diagonal_a),b=locate_formal_open_point(center,diagonal_b);
  require_index_differential(center,diagonal_a);require_index_differential(center,diagonal_b);
  require(a.has_value()&&b.has_value()&&a.value().signed_degree==1&&b.value().signed_degree==1,"source polygon degree independent of diagonal");
  require(a.value().hits.size()==1&&b.value().hits.size()==1&&a.value().hits[0].source_facet==facet&&b.value().hits[0].source_facet==facet,"one source polygon owner");
  require(a.value().hits[0].ownership==formal_ray_ownership_kind::facet_interior&&b.value().hits[0].ownership==formal_ray_ownership_kind::facet_interior&&!a.value().hits[0].source_edge&&!b.value().hits[0].source_edge,"internal diagonals are not source edges");
  auto malformed=diagonal_a;malformed.triangles[1].source_facet_index=1;require(!locate_formal_open_point(center,malformed).has_value(),"reject malformed source facet reference");
  formal_open_point_view edge{p3(0,0,0),exact_vector3{q(0),q(1),q(0)}, {}};
  const std::vector<exact_point3> triangle_ring{p3(1,0,-1),p3(1,1,0),p3(1,0,1)};
  const std::vector<original_vertex_id> triangle_ids{v0,v1,v2};
  sourced_exact_operand3 edge_triangle;add_facet(edge_triangle,facet,triangle_ring,triangle_ids,{},{{{triangle_ring[0],triangle_ring[1],triangle_ring[2]},{v0,v1,v2}}});
  auto e=locate_formal_open_point(edge,edge_triangle);
  require_index_differential(edge,edge_triangle);
  require(e.has_value()&&e.value().hits.size()==1&&e.value().hits[0].ownership==formal_ray_ownership_kind::source_edge&&e.value().hits[0].source_edge==std::optional<std::array<original_vertex_id,2>>({v0,v2})&&e.value().hits[0].source_edge_direction==std::optional<std::array<original_vertex_id,2>>({v2,v0})&&e.value().hits[0].owns_boundary_crossing,"ordered source edge ownership evidence");
  sourced_exact_operand3 shared_edge=edge_triangle;add_facet(shared_edge,facet_id::from_canonical_value(2),triangle_ring,{v0,v3,v2},{},{{{triangle_ring[0],triangle_ring[1],triangle_ring[2]},{v0,v3,v2}}});
  auto se=locate_formal_open_point(edge,shared_edge);
  require_index_differential(edge,shared_edge);
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
  sourced_exact_operand3 vertex_triangle;add_facet(vertex_triangle,facet,vertex_ring,triangle_ids,{vertex_fan,{facet},{facet}},{{{vertex_ring[0],vertex_ring[1],vertex_ring[2]},{v0,v1,v2}}});
  auto v=locate_formal_open_point(vertex,vertex_triangle);
  require_index_differential(vertex,vertex_triangle);
  require(v.has_value()&&v.value().hits.size()==1&&v.value().hits[0].ownership==formal_ray_ownership_kind::source_vertex&&v.value().hits[0].source_vertex==v0&&v.value().hits[0].source_vertex_fan==vertex_fan&&v.value().hits[0].source_vertex_fan_index==0,"ordered source vertex fan ownership evidence");
  require(validate_formal_ray_ownership_evidence(v.value()),"source vertex fan replay evidence");
  auto damaged_vertex=v.value();damaged_vertex.hits[0].source_vertex_fan_index=1;
  require(!validate_formal_ray_ownership_evidence(damaged_vertex),"reject wrong source vertex fan owner");
  damaged_vertex=v.value();damaged_vertex.hits[0].source_vertex_fan.push_back(facet);
  require(!validate_formal_ray_ownership_evidence(damaged_vertex),"reject duplicate source vertex fan member");
  const auto opposite=facet_id::from_canonical_value(1);
  const std::vector<exact_point3> tangent_ring{p3(1,-2,-2),p3(1,2,-2),p3(1,0,2)};
  sourced_exact_operand3 tangent;add_facet(tangent,facet,tangent_ring,triangle_ids,{},{{{tangent_ring[0],tangent_ring[1],tangent_ring[2]},{v0,v1,v2}}});add_facet(tangent,opposite,tangent_ring,triangle_ids,{},{{{tangent_ring[2],tangent_ring[1],tangent_ring[0]},{v2,v1,v0}}});
  auto t=locate_formal_open_point(center,tangent);
  require_index_differential(center,tangent);
  require(t.has_value()&&t.value().signed_degree==0&&t.value().hits.size()==2&&t.value().hits[0].parameter_group==t.value().hits[1].parameter_group&&t.value().hits[0].ownership==formal_ray_ownership_kind::tangent&&t.value().hits[1].ownership==formal_ray_ownership_kind::tangent&&!t.value().hits[0].owns_boundary_crossing&&!t.value().hits[1].owns_boundary_crossing,"equal parameter tangent has no owner");
  require(validate_formal_ray_ownership_evidence(t.value()),"tangent replay evidence");
  auto false_crossing=t.value();false_crossing.hits[0].owns_boundary_crossing=true;
  require(!validate_formal_ray_ownership_evidence(false_crossing),"reject tangent crossing owner evidence");
  sourced_exact_operand3 sparse=diagonal_a;for(std::size_t i=1;i<=20;++i){const auto y=static_cast<std::int64_t>(100*i);const std::vector<exact_point3>ring{p3(1,y-1,-1),p3(1,y+1,-1),p3(1,y,1)};add_facet(sparse,facet_id::from_canonical_value(i),ring,{v0,v1,v2},{},{{{ring[0],ring[1],ring[2]},{v0,v1,v2}}});}require_index_differential(center,sparse);require(build_sourced_exact_ray_index(sparse).nodes.size()>1,"ray index subdivides sparse operand");
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
int main(){try{predicates();certified_filters();classifications();division_free_incidence();dyadic_planes();constructions();polygon_relations();formal_point_location();grouped_ray_ownership();stable_formal_ray_fallback();radial_and_cones();exact_kernel<double>k;require(k.coordinate_type()==coordinate_tag::binary64&&!k.arithmetic_policy_bytes().empty(),"service");std::cout<<"PASS exact kernel\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
