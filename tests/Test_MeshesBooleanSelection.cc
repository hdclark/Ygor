#include "MeshBooleanSelectionFixtures.h"
#include <YgorMeshesBooleanExactResult.h>
#include <iostream>
using namespace selection_test;

int main(){try{
  auto r=selection_test::registry();auto a=cube<double,std::uint32_t>(),b=cube<double,std::uint32_t>();translate(b,3,0,0);
  auto c=context(a,b,r);auto result=select_boolean_boundary(*c);
  if(!result.has_value())throw std::runtime_error(render_error(result.error()));
  const auto&x=*result.value()->payload;selection_oracle(x);require(result.value()->report.passed(),"verified");
  require(x.patches.size()==12,"disjoint union shells");require(c->artifacts().latest_generation(artifact_slot::selected_exact_boundary)==1,"published");
  backend_capabilities capabilities;capabilities.set(backend_capability::exact_set_semantics);capabilities.set(backend_capability::exact_coordinates);capabilities.set(backend_capability::stratified_output);capabilities.set(backend_capability::deterministic_canonical_output);capabilities.set(backend_capability::certified_failure_categories);capabilities.set(backend_capability::provenance_mapping);capabilities.set(backend_capability::strict_prepared_operands);
  auto identity=make_backend_identity(backend_id::experimental_exact_v1,{1,0,0},"selection-p1-test",capabilities,backend_maturity::experimental);require(identity.has_value(),"exact result backend");
  exact_result_backend_binding backend;backend.producer=identity.value();backend.attempted_backends={backend_id::experimental_exact_v1};
  exact_result_preparation_binding preparation;preparation.input_digest=x.setup_digest;preparation.prepared_digest=x.arrangement_digest;preparation.policy_digest=x.labeled_digest;preparation.report_digest=x.artifact_digest;
  auto detached=detach_exact_stratified_boundary(x,backend,preparation);if(!detached.has_value())throw std::runtime_error(detached.error().message_key+": "+detached.error().detail);
  auto frozen_exact=freeze_exact_stratified_boundary(std::move(detached.value()));require(frozen_exact.has_value(),"freeze detached exact result");
  auto exact_bytes=encode_exact_stratified_boundary(*frozen_exact.value());require(exact_bytes.has_value(),"encode detached exact result");
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
  c.reset();require(verify_serialized_exact_stratified_boundary(exact_bytes.value()).has_value(),"exact result survives context destruction");
  std::cout<<"ok\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
