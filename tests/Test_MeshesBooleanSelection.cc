#include "MeshBooleanSelectionFixtures.h"
#include <iostream>
using namespace selection_test;

int main(){try{
  auto r=selection_test::registry();auto a=cube<double,std::uint32_t>(),b=cube<double,std::uint32_t>();translate(b,3,0,0);
  auto c=context(a,b,r);auto result=select_boolean_boundary(*c);
  if(!result.has_value())throw std::runtime_error(render_error(result.error()));
  const auto&x=*result.value()->payload;selection_oracle(x);require(result.value()->report.passed(),"verified");
  require(x.patches.size()==12,"disjoint union shells");require(c->artifacts().latest_generation(artifact_slot::selected_exact_boundary)==1,"published");
  auto repeated=select_boolean_boundary(*c);require(repeated.has_value()&&repeated.value().get()==result.value().get(),"idempotent");

  mutation_rejected(*r,*c,x,[](auto&v){v.selected_operation=operation::regularized_intersection;},"operation mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.decisions.front().result_negative=!v.decisions.front().result_negative;},"truth mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.decisions.front().kind=patch_decision_kind::discard_internal;},"decision mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.decisions.front().orientation=selected_orientation::none;},"orientation mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.decisions.front().representative=sheet_use_id::from_canonical_value(v.arrangement->payload->sheet_uses.size());},"representative mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.vertices.front().source=global_vertex_id::from_canonical_value(v.arrangement->payload->vertices.size());},"vertex mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.vertex_occurrences.front().incident_halfedges.pop_back();},"vertex occurrence mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.edges.front().uses.pop_back();},"edge incidence mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.halfedges.front().destination=v.halfedges.front().origin;},"halfedge mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.cycles.front().halfedges.pop_back();},"cycle mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.patches.front().provenance.clear();},"patch provenance mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){++v.certificate.connected_components;},"certificate mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.canonical_bytes.push_back(0);},"canonical encoding mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.artifact_bytes.push_back(0);},"artifact encoding mutation rejected");

  auto archive=replay_archive(*result.value(),*c);auto encoded=encode_replay_archive(archive);require(encoded.has_value(),"selection replay encode");
  auto decoded=decode_replay_archive(encoded.value());require(decoded.has_value()&&decoded.value().artifact==x.artifact_bytes&&decoded.value().report_digest==result.value()->report.report_digest,"selection replay round trip");
  auto malformed=encoded.value();malformed.front()^=1;require(!decode_replay_archive(malformed).has_value(),"selection malformed replay rejected");
  std::cout<<"ok\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
