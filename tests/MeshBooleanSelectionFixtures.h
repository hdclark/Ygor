#pragma once
#include "MeshBooleanCellClassificationFixtures.h"
#include <YgorMeshesBooleanSelection.h>
namespace selection_test {
using namespace classification_test;
inline bool oracle_occupied(operation op, occupancy_pair value) {
  switch (op) {
  case operation::regularized_union: return value.in_a || value.in_b;
  case operation::regularized_intersection: return value.in_a && value.in_b;
  case operation::a_minus_b: return value.in_a && !value.in_b;
  case operation::b_minus_a: return value.in_b && !value.in_a;
  case operation::symmetric_difference: return value.in_a != value.in_b;
  }
  throw std::runtime_error("unknown operation");
}
inline patch_decision_kind oracle_decision(operation op, occupancy_pair negative,
                                           occupancy_pair positive) {
  const bool n = oracle_occupied(op, negative);
  const bool p = oracle_occupied(op, positive);
  if (n == p)
    return n ? patch_decision_kind::discard_internal
             : patch_decision_kind::discard_exterior;
  return n ? patch_decision_kind::select_preserved
           : patch_decision_kind::select_reversed;
}
inline std::shared_ptr<verifier_registry> registry(){
  auto r=std::make_shared<verifier_registry>();
  for(auto c:{coordinate_tag::binary32,coordinate_tag::binary64})for(auto i:{index_tag::uint32,index_tag::uint64}){
    require(register_input_topology_verifier(*r,c,i).has_value(),"input verifier");require(register_broad_phase_verifier(*r,c,i).has_value(),"broad verifier");require(register_intersection_events_verifier(*r,c,i).has_value(),"event verifier");require(register_symbolic_registry_verifier(*r,c,i).has_value(),"symbolic verifier");require(register_local_refinement_verifier(*r,c,i).has_value(),"local verifier");require(register_global_arrangement_verifier(*r,c,i).has_value(),"arrangement verifier");require(register_cell_classification_verifier(*r,c,i).has_value(),"classification verifier");require(register_boolean_selection_verifier(*r,c,i).has_value(),"selection verifier");
  }
  require(r->freeze().has_value(),"freeze");return r;
}
template<class T,class I,class Mutate>
void mutation_rejected(verifier_registry&r,boolean_context<T,I>&c,
                       const selected_exact_boundary<T,I>&source,Mutate mutate,
                       const char*message){
  auto changed=std::make_shared<selected_exact_boundary<T,I>>(source);mutate(*changed);
  const auto type=selected_exact_boundary_type_tag+(static_cast<std::uint64_t>(coordinate_type<T>())<<8)+static_cast<std::uint64_t>(index_type<I>());
  auto spec=r.specification(artifact_slot::selected_exact_boundary,type,selected_exact_boundary_schema,verification_level::mandatory);
  require(spec.has_value(),"selection mutation specification");
  artifact_view view{c.owner(),artifact_slot::selected_exact_boundary,type,selected_exact_boundary_schema,1,changed->artifact_digest,changed,changed.get()};
  verification_environment_view env{c.owner(),c.replay().setup,c.contract().selected_operation(),&c.options(),coordinate_type<T>(),index_type<I>(),&c.kernel(),{},&c.accountant(),[&]{return c.cancelled();}};
  auto checked=r.verify(view,spec.value(),env);
  if(!checked.has_value())throw std::runtime_error(std::string(message)+": "+render_error(checked.error()));
  require(!checked.value().passed(),message);
}
template<class T,class I>
std::array<std::uint64_t,4> decision_histogram(const selected_exact_boundary<T,I>&a){
  std::array<std::uint64_t,4> result{{}};for(const auto&d:a.decisions)++result[static_cast<unsigned>(d.kind)];return result;
}
template<class T,class I>
verification_replay_archive replay_archive(const published_artifact<selected_exact_boundary<T,I>>&a,
                                           const boolean_context<T,I>&c){
  verification_replay_archive archive;archive.coordinate=coordinate_type<T>();archive.index=index_type<I>();archive.setup_digest=c.replay().setup;archive.artifact_digest=a.artifact_digest;archive.report_digest=a.report.report_digest;
  archive.operand_a.assign(c.replay().input_a.bytes.begin(),c.replay().input_a.bytes.end());archive.operand_b.assign(c.replay().input_b.bytes.begin(),c.replay().input_b.bytes.end());archive.artifact=a.payload->artifact_bytes;
  auto report=encode_verification_report(a.report);require(report.has_value(),"selection replay report");archive.report=std::move(report.value());archive.dependencies=a.report.dependency_digests;return archive;
}
template<class T,class I>void selection_oracle(const selected_exact_boundary<T,I>&a){
  require(a.decisions.size()==a.arrangement->payload->patches.size(),"total decisions");
  for(const auto&d:a.decisions){
    const bool n=oracle_occupied(a.selected_operation,d.negative_occupancy);
    const bool p=oracle_occupied(a.selected_operation,d.positive_occupancy);
    require(d.result_negative==n&&d.result_positive==p,"independent truth bits");
    require(d.kind==oracle_decision(a.selected_operation,d.negative_occupancy,d.positive_occupancy),"independent decision");
    require(bool(d.selected)==(n!=p),"truth projection");
  }
  std::uint64_t provenance=0;
  for(const auto&d:a.decisions)provenance+=d.provenance.size();
  require(a.certificate.decisions==a.decisions.size(),"decision certificate count");
  require(a.certificate.selected_patches==a.patches.size(),"patch certificate count");
  require(a.certificate.selected_cycles==a.cycles.size(),"cycle certificate count");
  require(a.certificate.selected_halfedges==a.halfedges.size(),"halfedge certificate count");
  require(a.certificate.selected_edges==a.edges.size(),"edge certificate count");
  require(a.certificate.selected_vertices==a.vertices.size(),"vertex certificate count");
  require(a.certificate.provenance_uses==provenance,"provenance certificate count");
  for(const auto&p:a.patches){
    require(p.source.value_for_debug()<a.arrangement->payload->patches.size(),"selected source patch");
    require(!p.provenance.empty(),"selected provenance");
    require(std::find(p.provenance.begin(),p.provenance.end(),p.representative)!=p.provenance.end(),"representative retained");
    const auto&d=a.decisions[p.source.value_for_debug()];
    require(d.selected&&d.selected->value_for_debug()==p.id.value_for_debug(),"decision selected mapping");
    require(d.orientation==p.orientation,"selected orientation mapping");
  }
  for(const auto&cycle:a.cycles){
    require(!cycle.halfedges.empty(),"nonempty selected cycle");
    for(std::size_t j=0;j<cycle.halfedges.size();++j){
      const auto id=cycle.halfedges[j].value_for_debug();require(id<a.halfedges.size(),"cycle halfedge range");
      const auto&h=a.halfedges[id];const auto&next=a.halfedges[h.next.value_for_debug()];const auto&previous=a.halfedges[h.previous.value_for_debug()];
      require(h.cycle==cycle.id&&h.patch==cycle.patch,"halfedge ownership");
      require(next.previous==h.id&&previous.next==h.id,"cycle link involution");
      require(h.destination==next.origin,"cycle endpoint continuity");
    }
  }
  for(const auto&e:a.edges){
    require(e.uses.size()==2,"closed selected edge");
    const auto&x=a.halfedges[e.uses[0].value_for_debug()];const auto&y=a.halfedges[e.uses[1].value_for_debug()];
    require(x.edge==e.id&&y.edge==e.id,"selected edge ownership");
    require(x.origin==y.destination&&x.destination==y.origin,"opposite selected edge uses");
  }
}

template<class T,class I>void run_analytic_selection_case(std::shared_ptr<verifier_registry> r,
    T a_lo,T a_hi,T b_lo,T b_hi,operation op){
  auto a=input_test::box<T,I>(a_lo,a_hi),b=input_test::box<T,I>(b_lo,b_hi);
  auto c=classification_test::context(a,b,std::move(r),op,boolean_options{});auto result=select_boolean_boundary(*c);
  if(!result.has_value())throw std::runtime_error("analytic selection case op="+std::to_string(static_cast<unsigned>(op))+" bound_bits="+coordinate_diagnostic(a_lo)+","+coordinate_diagnostic(a_hi)+","+coordinate_diagnostic(b_lo)+","+coordinate_diagnostic(b_hi)+": "+render_error(result.error()));
  selection_oracle(*result.value()->payload);require(result.value()->report.passed(),"selection verified");
}
template<class T,class I>void run_nonmanifold_selection_case(std::shared_ptr<verifier_registry> r,
    T a_lo,T a_hi,T b_lo,T b_hi,operation op){
  auto a=input_test::box<T,I>(a_lo,a_hi),b=input_test::box<T,I>(b_lo,b_hi);
  auto c=classification_test::context(a,b,std::move(r),op,boolean_options{});auto result=select_boolean_boundary(*c);
  require(!result.has_value(),"nonmanifold selection must fail closed");
  require(result.error().code==boolean_error_code::internal_invariant_error,"nonmanifold selection error category");
  require(result.error().message_key=="nonmanifold_selected_edge","nonmanifold selection invariant");
  require(c->artifacts().latest_generation(artifact_slot::selected_exact_boundary)==0,"failed selection not published");
}
}
