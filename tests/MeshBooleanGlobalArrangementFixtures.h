#pragma once
#include "MeshBooleanLocalRefinementFixtures.h"
#include <YgorMeshesBooleanGlobalArrangement.h>
#include <set>
namespace arrangement_test {
using namespace local_test;
inline std::shared_ptr<verifier_registry> registry(){
  auto r=std::make_shared<verifier_registry>();
  for(auto c:{coordinate_tag::binary32,coordinate_tag::binary64})for(auto i:{index_tag::uint32,index_tag::uint64}){
    require(register_input_topology_verifier(*r,c,i).has_value(),"input verifier");
    require(register_broad_phase_verifier(*r,c,i).has_value(),"broad verifier");
    require(register_intersection_events_verifier(*r,c,i).has_value(),"event verifier");
    require(register_symbolic_registry_verifier(*r,c,i).has_value(),"symbolic verifier");
    require(register_local_refinement_verifier(*r,c,i).has_value(),"local verifier");
    require(register_global_arrangement_verifier(*r,c,i).has_value(),"arrangement verifier");
  }
  require(r->freeze().has_value(),"freeze");return r;
}
template<class T,class I>void structural_oracle(const arrangement_complex<T,I>&a){
  require(a.patch_sides.size()==2*a.patches.size(),"two sides per patch");
  require(a.transitions.size()>=a.patches.size(),"crossings and preserving transitions");
  require(a.vertex_sectors.size()==a.vertex_occurrences.size(),"complete occurrence-owned vertex links");
  require(a.probes.size()==a.patch_sides.size(),"one formal probe per side component");
  for(const auto&h:a.halfedges){const auto&m=a.halfedges[h.sheet_mate.value_for_debug()];require(m.sheet_mate==h.id&&m.origin_occurrence==h.destination_occurrence&&m.destination_occurrence==h.origin_occurrence,"mate involution");}
  for(const auto&p:a.patches)require(!p.uses.empty(),"patch coverage");
  require(a.certificate.local_patches==a.sheet_uses.size(),"total patch map");
  std::vector<std::vector<sheet_use_id>> uses_by_edge(a.edges.size());
  for(const auto&halfedge:a.halfedges)uses_by_edge[halfedge.edge.value_for_debug()].push_back(halfedge.use);
  for(auto&uses:uses_by_edge){std::sort(uses.begin(),uses.end());uses.erase(std::unique(uses.begin(),uses.end()),uses.end());}
  for(const auto&seam:a.seams)require(seam.incident_uses==uses_by_edge[seam.edge.value_for_debug()],"indexed seam incidence equals exhaustive scan");
  for(const auto&occurrence:a.vertex_occurrences){
    std::set<global_atomic_edge_id> incident;
    for(auto halfedge:occurrence.incident_halfedges)incident.insert(a.halfedges[halfedge.value_for_debug()].edge);
    std::vector<exact_vector3> directions;
    const auto&p=a.symbolic->payload->vertices[a.vertices[occurrence.vertex.value_for_debug()].symbolic.value_for_debug()].point;
    for(auto edge:incident){const auto&e=a.edges[edge.value_for_debug()];const auto other=e.lower==occurrence.vertex?e.upper:e.lower;const auto&q=a.symbolic->payload->vertices[a.vertices[other.value_for_debug()].symbolic.value_for_debug()].point;const auto d=q-p;bool duplicate=false;for(const auto&x:directions){const auto c=cross(d,x);if(c.x.sign()==exact_sign::zero&&c.y.sign()==exact_sign::zero&&c.z.sign()==exact_sign::zero&&dot_sign_exact(d,x)==exact_sign::positive){duplicate=true;break;}}if(!duplicate)directions.push_back(d);}
    const auto&sector=a.vertex_sectors[occurrence.id.value_for_debug()];
    require(sector.boundary_rays.size()==directions.size()*2,"local exact direction classes");
  }
}
}
