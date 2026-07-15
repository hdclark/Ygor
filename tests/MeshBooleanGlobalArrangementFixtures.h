#pragma once
#include "MeshBooleanLocalRefinementFixtures.h"
#include <YgorMeshesBooleanGlobalArrangement.h>
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
}
}
