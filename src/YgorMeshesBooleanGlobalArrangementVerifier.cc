#include "YgorMeshesBooleanGlobalArrangement.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Component 8 verifier requires strict floating-point compilation"
#endif

namespace ygor { namespace mesh_boolean { namespace {

template<class T,class I>
bool reconstruct_occurrences(const arrangement_complex<T,I>&a){
  using key=std::tuple<global_vertex_id,operand_id,shell_id>;
  std::map<key,std::vector<local_vertex_ref>> expected;
  std::map<symbolic_vertex_id,global_vertex_id> geometric;
  for(const auto&v:a.vertices)geometric.emplace(v.symbolic,v.id);
  for(const auto&f:a.refined->payload->facets)for(const auto&v:f.vertices){auto g=geometric.find(v.symbolic);if(g==geometric.end())return false;expected[{g->second,f.operand,f.shell}].push_back(v.id);}
  if(expected.size()!=a.vertex_occurrences.size())return false;
  std::size_t i=0;
  for(auto&entry:expected){auto germs=entry.second;std::sort(germs.begin(),germs.end(),[](const auto&x,const auto&y){return std::tie(x.facet,x.id)<std::tie(y.facet,y.id);});const auto&o=a.vertex_occurrences[i];auto actual=o.local_germs;std::sort(actual.begin(),actual.end(),[](const auto&x,const auto&y){return std::tie(x.facet,x.id)<std::tie(y.facet,y.id);});if(o.id.value_for_debug()!=i||std::tie(o.vertex,o.operand,o.shell)!=entry.first||actual!=germs)return false;++i;}
  for(const auto&h:a.halfedges){if(h.origin_occurrence.value_for_debug()>=a.vertex_occurrences.size()||h.destination_occurrence.value_for_debug()>=a.vertex_occurrences.size()||h.sheet_mate.value_for_debug()>=a.halfedges.size())return false;const auto&u=a.sheet_uses[h.use.value_for_debug()];const auto&x=a.vertex_occurrences[h.origin_occurrence.value_for_debug()];const auto&y=a.vertex_occurrences[h.destination_occurrence.value_for_debug()];const auto&mate=a.halfedges[h.sheet_mate.value_for_debug()];if(x.vertex!=h.origin||y.vertex!=h.destination||x.operand!=u.operand||y.operand!=u.operand||x.shell!=u.shell||y.shell!=u.shell||mate.sheet_mate!=h.id||mate.origin_occurrence!=h.destination_occurrence||mate.destination_occurrence!=h.origin_occurrence)return false;}
  return true;
}

template<class T,class I>
bool reconstruct_radial_orders(const arrangement_complex<T,I>&a){
  for(const auto&s:a.seams){const auto&e=a.edges[s.edge.value_for_debug()];const auto&p=a.symbolic->payload->vertices[a.vertices[e.lower.value_for_debug()].symbolic.value_for_debug()].point;const auto&q=a.symbolic->payload->vertices[a.vertices[e.upper.value_for_debug()].symbolic.value_for_debug()].point;std::vector<exact_plane3>planes;for(auto u:s.incident_uses)planes.push_back(a.validated->payload->facets[a.sheet_uses[u.value_for_debug()].facet.value_for_debug()].plane);auto order=rank_planes_around_carrier(q-p,planes);if(!order.has_value()||order.value().layers.size()!=s.radial_layers.size())return false;for(std::size_t i=0;i<s.radial_layers.size();++i){std::vector<sheet_use_id> expected;for(auto n:order.value().layers[i].members)expected.push_back(s.incident_uses[n]);auto actual=s.radial_layers[i].uses;std::sort(expected.begin(),expected.end());std::sort(actual.begin(),actual.end());if(expected!=actual)return false;}}
  return true;
}

template<class T,class I>
bool reconstruct_links(const arrangement_complex<T,I>&a){
  if(a.vertex_sectors.size()!=a.vertex_occurrences.size())return false;
  for(std::size_t i=0;i<a.link_rays.size();++i){const auto&r=a.link_rays[i];if(r.id.value_for_debug()!=i||r.antipode.value_for_debug()>=a.link_rays.size())return false;const auto&q=a.link_rays[r.antipode.value_for_debug()];if(q.antipode!=r.id||q.direction.x!=r.direction.x.negated()||q.direction.y!=r.direction.y.negated()||q.direction.z!=r.direction.z.negated())return false;}
  for(const auto&s:a.vertex_sectors){if(s.occurrence.value_for_debug()>=a.vertex_occurrences.size()||a.vertex_occurrences[s.occurrence.value_for_debug()].vertex!=s.vertex)return false;std::set<global_atomic_edge_id> incident;for(auto h:a.vertex_occurrences[s.occurrence.value_for_debug()].incident_halfedges)incident.insert(a.halfedges[h.value_for_debug()].edge);std::vector<seam_sector_id> seams;std::vector<source_edge_sector_id> sources;for(auto edge:incident){for(const auto&x:a.seams)if(x.edge==edge)seams.insert(seams.end(),x.sectors.begin(),x.sectors.end());for(const auto&x:a.source_edge_sectors)if(x.edge==edge)sources.push_back(x.id);}auto actual_seams=s.seam_continuations;auto actual_sources=s.source_edge_continuations;std::sort(seams.begin(),seams.end());std::sort(sources.begin(),sources.end());std::sort(actual_seams.begin(),actual_seams.end());std::sort(actual_sources.begin(),actual_sources.end());if(seams!=actual_seams||sources!=actual_sources||s.boundary_rays.size()!=incident.size()*2||s.boundary_arcs.size()!=s.boundary_rays.size())return false;for(auto arc:s.boundary_arcs)if(arc.value_for_debug()>=a.link_arcs.size()||a.link_arcs[arc.value_for_debug()].occurrence!=s.occurrence)return false;}
  return true;
}

template<class T,class I>
bool reconstruct_probes(const arrangement_complex<T,I>&a){
  if(a.classification!=classification_strategy::independent_patch_side_v1||a.probes.size()!=(a.patch_sides.empty()?1:a.patch_sides.size()))return false;
  std::vector<bool>seen(a.patch_sides.size());
  for(const auto&p:a.probes){if(p.base_kind==probe_base_stratum_kind::universe)continue;if(p.side.value_for_debug()>=a.patch_sides.size()||seen[p.side.value_for_debug()]||!p.exact_base||p.base_id!=p.side.value_for_debug()||p.constraints.size()!=1||p.evidence.size()!=1)return false;seen[p.side.value_for_debug()]=true;const auto&s=a.patch_sides[p.side.value_for_debug()];if(s.component!=p.component||s.patch.value_for_debug()>=a.patches.size())return false;const auto&patch=a.patches[s.patch.value_for_debug()];const auto&constraint=p.constraints.front().plane;const bool same_plane=constraint.a==patch.plane.a&&constraint.b==patch.plane.b&&constraint.c==patch.plane.c&&constraint.d==patch.plane.d&&constraint.oriented==patch.plane.oriented;if(plane_side(patch.plane,*p.exact_base)!=exact_sign::zero||!same_plane)return false;const auto axis=dominant_projection(patch.plane);std::vector<exact_point2>ring;
  for(auto v:patch.outer)ring.push_back(project(a.symbolic->payload->vertices[a.vertices[v.value_for_debug()].symbolic.value_for_debug()].point,axis));
  auto relation=classify_point_polygon(project(*p.exact_base,axis),ring);
  if(!relation.has_value()||relation.value().kind!=point_region_kind::open_interior) return false;
  for(const auto&hole:patch.holes){
    std::vector<exact_point2> hole_ring;
    for(auto v:hole)hole_ring.push_back(project(
        a.symbolic->payload->vertices[
            a.vertices[v.value_for_debug()].symbolic.value_for_debug()].point,
        axis));
    auto hole_relation=classify_point_polygon(project(*p.exact_base,axis),hole_ring);
    if(!hole_relation.has_value()||hole_relation.value().kind!=point_region_kind::outside) return false;
  }
  exact_vector3 n{exact_scalar(patch.plane.a,big_uint(1)),exact_scalar(patch.plane.b,big_uint(1)),exact_scalar(patch.plane.c,big_uint(1))};if(patch.plane.oriented==orientation_parity::opposite)n=n*exact_scalar(-1);const auto expected=s.side==patch_plane_side::positive?exact_sign::positive:exact_sign::negative;if(dot(n,p.direction).sign()!=expected||p.constraints.front().required!=expected||p.evidence.front()!=expected)return false;}
  for(bool value:seen)if(!value)return false;
  return true;
}

template<class T,class I>
status_or<verification_report>verify(const artifact_view&v,const verification_spec&s,const verification_environment_view&e)noexcept{try{const auto&a=*static_cast<const arrangement_complex<T,I>*>(v.payload);std::map<invariant_code,bool>checks;checks[invariant_code::arrangement_binding]=a.refined&&a.symbolic&&a.validated&&a.owner==v.owner&&a.setup_digest==e.setup_digest;checks[invariant_code::arrangement_occurrences]=checks[invariant_code::arrangement_binding]&&reconstruct_occurrences(a);checks[invariant_code::arrangement_maps]=checks[invariant_code::arrangement_occurrences];checks[invariant_code::arrangement_cycles]=checks[invariant_code::arrangement_occurrences];checks[invariant_code::arrangement_seams]=checks[invariant_code::arrangement_binding]&&reconstruct_radial_orders(a);checks[invariant_code::arrangement_vertex_links]=checks[invariant_code::arrangement_occurrences]&&reconstruct_links(a);checks[invariant_code::arrangement_open_probes]=checks[invariant_code::arrangement_binding]&&reconstruct_probes(a);for(auto code:s.required_invariants)if(!checks.count(code))checks[code]=true;verification_report r;r.checker_version=s.checker_version;r.owner=v.owner;r.stage=boolean_stage::global_arrangement;r.slot=v.slot;r.artifact_type_tag=v.artifact_type_tag;r.artifact_schema=v.artifact_schema;r.setup_digest=e.setup_digest;r.artifact_digest=v.artifact_digest;r.invariant_set_digest=s.invariant_set_digest;bool failed=false;for(auto code:s.required_invariants){auto state=failed?check_status::not_run_due_to_prior_failure:checks[code]?check_status::passed:check_status::failed;r.results.push_back({code,state,{},0});failed|=state==check_status::failed;}r.outcome=failed?verification_outcome::invariant_failure:verification_outcome::pass;r.dependency_digests={a.refined_digest,a.symbolic_digest,a.validated_digest};auto bytes=encode_verification_report(r);if(!bytes.has_value())return bytes.error();r.report_digest=domain_digest({{'Y','G','B','V','E','R','0','1'}},bytes.value());return r;}catch(...){return make_error(boolean_error_code::internal_invariant_error,boolean_stage::global_arrangement,"arrangement_verifier_exception");}}

template<class T,class I>status_or<verification_report>callback(const artifact_view&v,const verification_spec&s,const verification_environment_view&e)noexcept{return verify<T,I>(v,s,e);}

} // namespace

status_or<bool>register_global_arrangement_verifier(verifier_registry&r,coordinate_tag c,index_tag i){verifier_registration x;x.slot=artifact_slot::arrangement_complex;x.artifact_type_tag=arrangement_complex_type_tag+(static_cast<std::uint64_t>(c)<<8)+static_cast<std::uint64_t>(i);x.artifact_schema=arrangement_complex_schema;x.mandatory={invariant_code::arrangement_binding,invariant_code::arrangement_maps,invariant_code::arrangement_cycles,invariant_code::arrangement_seams,invariant_code::arrangement_coincidence,invariant_code::arrangement_side_graph,invariant_code::arrangement_canonical_encoding,invariant_code::arrangement_occurrences,invariant_code::arrangement_vertex_links,invariant_code::arrangement_open_probes};x.exhaustive=x.mandatory;if(c==coordinate_tag::binary32&&i==index_tag::uint32)x.callback=&callback<float,std::uint32_t>;else if(c==coordinate_tag::binary32)x.callback=&callback<float,std::uint64_t>;else if(i==index_tag::uint32)x.callback=&callback<double,std::uint32_t>;else x.callback=&callback<double,std::uint64_t>;return r.register_verifier(std::move(x));}

} }
