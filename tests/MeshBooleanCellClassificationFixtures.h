#pragma once
#include "MeshBooleanGlobalArrangementFixtures.h"
#include <YgorMeshesBooleanCellClassification.h>
#include <iomanip>
#include <sstream>
namespace classification_test {
using namespace arrangement_test;
template<class T>constexpr coordinate_tag coordinate_type(){return std::is_same<T,float>::value?coordinate_tag::binary32:coordinate_tag::binary64;}
template<class I>constexpr index_tag index_type(){return std::is_same<I,std::uint32_t>::value?index_tag::uint32:index_tag::uint64;}
inline std::shared_ptr<verifier_registry> registry(){
  auto r=std::make_shared<verifier_registry>();
  for(auto c:{coordinate_tag::binary32,coordinate_tag::binary64})for(auto i:{index_tag::uint32,index_tag::uint64}){
    require(register_input_topology_verifier(*r,c,i).has_value(),"input verifier");
    require(register_broad_phase_verifier(*r,c,i).has_value(),"broad verifier");
    require(register_intersection_events_verifier(*r,c,i).has_value(),"event verifier");
    require(register_symbolic_registry_verifier(*r,c,i).has_value(),"symbolic verifier");
    require(register_local_refinement_verifier(*r,c,i).has_value(),"local verifier");
    require(register_global_arrangement_verifier(*r,c,i).has_value(),"arrangement verifier");
    require(register_cell_classification_verifier(*r,c,i).has_value(),"classification verifier");
  }
  require(r->freeze().has_value(),"freeze");return r;
}
template<class T,class I>
auto context(fv_surface_mesh<T,I>&a,fv_surface_mesh<T,I>&b,
             std::shared_ptr<verifier_registry> r,operation op,
             const boolean_options&options=boolean_options{},cancellation_source*cancel=nullptr){
  std::shared_ptr<const exact_kernel_services<T>> k=std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> v=r;
  auto c=make_boolean_context(a,b,op,options,k,v,cancel);
  require(c.has_value(),"classification context");return std::move(c.value());
}
template<class T,class I,class Mutate>
void mutation_rejected(verifier_registry&r,boolean_context<T,I>&c,
                       const labeled_arrangement<T,I>&source,Mutate mutate,
                       const char*message){
  auto changed=std::make_shared<labeled_arrangement<T,I>>(source);mutate(*changed);
  const auto type=labeled_arrangement_type_tag+(static_cast<std::uint64_t>(coordinate_type<T>())<<8)+static_cast<std::uint64_t>(index_type<I>());
  auto spec=r.specification(artifact_slot::labeled_arrangement,type,labeled_arrangement_schema,verification_level::mandatory);
  require(spec.has_value(),"classification mutation specification");
  artifact_view view{c.owner(),artifact_slot::labeled_arrangement,type,labeled_arrangement_schema,1,changed->artifact_digest,changed,changed.get()};
  verification_environment_view env{c.owner(),c.replay().setup,c.contract().selected_operation(),&c.options(),coordinate_type<T>(),index_type<I>(),&c.kernel(),{},&c.accountant(),[&]{return c.cancelled();}};
  auto checked=r.verify(view,spec.value(),env);
  if(!checked.has_value())throw std::runtime_error(std::string(message)+": "+render_error(checked.error()));
  require(!checked.value().passed(),message);
}
template<class T,class I>
std::array<std::uint64_t,4> label_histogram(const labeled_arrangement<T,I>&a){
  std::array<std::uint64_t,4> result{{}};for(const auto&r:a.regions)++result[(r.label.in_a?2U:0U)+(r.label.in_b?1U:0U)];return result;
}
template<class T>std::string coordinate_diagnostic(T value){
  const auto bits=bits_of(value).bits;std::ostringstream out;
  out<<"0x"<<std::hex<<std::setfill('0')<<std::setw(sizeof(T)*2)<<bits;return out.str();
}
template<class T,class I>
verification_replay_archive replay_archive(const published_artifact<labeled_arrangement<T,I>>&a,
                                           const boolean_context<T,I>&c){
  verification_replay_archive archive;archive.coordinate=coordinate_type<T>();archive.index=index_type<I>();archive.setup_digest=c.replay().setup;archive.artifact_digest=a.artifact_digest;archive.report_digest=a.report.report_digest;
  archive.operand_a.assign(c.replay().input_a.bytes.begin(),c.replay().input_a.bytes.end());archive.operand_b.assign(c.replay().input_b.bytes.begin(),c.replay().input_b.bytes.end());archive.artifact=a.payload->artifact_bytes;
  auto report=encode_verification_report(a.report);require(report.has_value(),"classification replay report");archive.report=std::move(report.value());archive.dependencies=a.report.dependency_digests;return archive;
}
template<class T,class I>void classification_oracle(const labeled_arrangement<T,I>&a){
  require(a.regions.size()==a.arrangement->payload->probes.size(),"region coverage");
  require(a.side_labels.size()==a.arrangement->payload->patch_sides.size(),"side coverage");
  require(a.patch_labels.size()==a.arrangement->payload->patches.size(),"patch coverage");
  require(a.propagation.size()==a.regions.size(),"propagation coverage");
  require(a.arc_checks.size()==a.transitions.size(),"directed arc coverage");
  require(a.certificate.exterior_bound_disjoint,"exterior bound certificate");
  require(a.certificate.exterior_region.value_for_debug()<a.regions.size(),"exterior region");
  require(a.regions[a.certificate.exterior_region.value_for_debug()].label==occupancy_pair{},"exterior occupancy");
  if(a.arrangement->payload->patches.empty())require(!a.certificate.exterior_attachment_side,"universe exterior attachment");
  else{
    require(bool(a.certificate.exterior_attachment_side),"exterior attachment side");
    require(bool(a.certificate.exterior_target_patch),"exterior target patch");
    require(bool(a.certificate.exterior_target_witness),"exterior target witness");
    require(!a.certificate.exterior_first_hits.empty(),"exterior first-hit group");
    require(a.certificate.exterior_target_patch->value_for_debug()==0,"least canonical exterior target");
    const auto side=a.certificate.exterior_attachment_side->value_for_debug();
    require(side<a.side_labels.size(),"exterior attachment range");
    require(a.side_labels[side].region==a.certificate.exterior_region,"exterior attachment region");
    const auto&label=a.side_labels[side];
    const auto&patch=a.arrangement->payload->patches[label.patch.value_for_debug()];
    const auto sign=plane_side(patch.plane,a.certificate.exterior_witness);
    require(sign!=exact_sign::zero,"exterior target incidence");
    require(label.side==(sign==exact_sign::negative?patch_plane_side::negative:patch_plane_side::positive),"exterior witness-facing side");
    const auto parameter=a.certificate.exterior_first_hits.front().parameter;
    for(const auto&hit:a.certificate.exterior_first_hits){
      require(hit.parameter==parameter,"grouped first-hit parameter");
      require(hit.relation!=point_region_kind::outside,"first hit on patch domain");
      require(hit.witness_facing_side.value_for_debug()<a.side_labels.size(),"first-hit side range");
      require(a.side_labels[hit.witness_facing_side.value_for_debug()].region==a.certificate.exterior_region,"first-hit region ownership");
    }
  }
  require(a.certificate.exterior_operand_a.location==operand_location_kind::outside,"exterior A");
  require(a.certificate.exterior_operand_b.location==operand_location_kind::outside,"exterior B");
  for(const auto&s:a.seeds){
    require(s.operand_a_primary.location==s.operand_a_alternate.location,"alternate A location");
    require(s.operand_a_primary.signed_degree==s.operand_a_alternate.signed_degree,"alternate A degree");
    require(s.operand_b_primary.location==s.operand_b_alternate.location,"alternate B location");
    require(s.operand_b_primary.signed_degree==s.operand_b_alternate.signed_degree,"alternate B degree");
    for(const auto&h:s.operand_a_primary.hits){require(bool(h.source_facet),"A source facet");require(h.source_facet->value_for_debug()<a.validated->payload->facets.size(),"A source facet range");require(a.validated->payload->facets[h.source_facet->value_for_debug()].operand==operand_a(),"A source operand");require(!h.owns_boundary_crossing||h.signed_contribution!=0,"A replay owner contribution");}
    for(const auto&h:s.operand_b_primary.hits){require(bool(h.source_facet),"B source facet");require(h.source_facet->value_for_debug()<a.validated->payload->facets.size(),"B source facet range");require(a.validated->payload->facets[h.source_facet->value_for_debug()].operand==operand_b(),"B source operand");require(!h.owns_boundary_crossing||h.signed_contribution!=0,"B replay owner contribution");}
  }
  for(const auto&p:a.patch_labels)require(p.negative!=p.positive,"distinct side labels");
  for(const auto&t:a.transitions){
    require(t.before==a.regions[t.from.value_for_debug()].label,"transfer source label");
    require(t.after==a.regions[t.to.value_for_debug()].label,"transfer destination label");
  }
  for(const auto&x:a.arc_checks){
    const auto&t=a.transitions[x.transition.value_for_debug()];
    require(a.propagation[x.from_path.value_for_debug()].region==t.from,"arc source path");
    require(a.propagation[x.to_path.value_for_debug()].region==t.to,"arc destination path");
    require(x.transferred==t.after,"arc transferred label");
    require(a.transitions[t.reverse.value_for_debug()].from_side==t.to_side,"reverse endpoint side");
    require(a.transitions[t.reverse.value_for_debug()].coincidence==t.coincidence,"reverse coincidence provenance");
  }
  for(const auto&x:a.cycle_checks){
    const auto&t=a.transitions[x.closing_transition.value_for_debug()];
    require(x.from_label==t.before&&x.to_label==t.after,"cycle closing transfer");
    require(a.propagation[x.from_path.value_for_debug()].root==x.root,"cycle source root");
    require(a.propagation[x.to_path.value_for_debug()].root==x.root,"cycle destination root");
  }
}

template<class T,class I>std::vector<std::uint8_t> run_analytic_classification_case(
    std::shared_ptr<verifier_registry> r,T a_lo,T a_hi,T b_lo,T b_hi,
    operation op=operation::regularized_union){
  auto a=input_test::box<T,I>(a_lo,a_hi),b=input_test::box<T,I>(b_lo,b_hi);
  auto c=context(a,b,std::move(r),op);auto result=classify_arrangement_cells(*c);
  if(!result.has_value())throw std::runtime_error("analytic classification case op="+std::to_string(static_cast<unsigned>(op))+" bound_bits="+coordinate_diagnostic(a_lo)+","+coordinate_diagnostic(a_hi)+","+coordinate_diagnostic(b_lo)+","+coordinate_diagnostic(b_hi)+": "+render_error(result.error()));
  classification_oracle(*result.value()->payload);require(result.value()->report.passed(),"classification verified");
  return result.value()->payload->canonical_bytes;
}
}
