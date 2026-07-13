#include "MeshBooleanCellClassificationFixtures.h"
#include <iostream>
using namespace classification_test;
template<class T,class I>
static void qualification(){
  auto a=input_test::box<T,I>(T(0),T(1)),b=input_test::box<T,I>(T(3),T(4));
  boolean_options one;one.execution.max_threads=1;auto c1=context(a,b,classification_test::registry(),operation::regularized_union,one);auto x=classify_arrangement_cells(*c1);require(x.has_value(),"classification single-thread schedule");
  boolean_options many;many.execution.max_threads=4;auto c4=context(a,b,classification_test::registry(),operation::regularized_union,many);auto y=classify_arrangement_cells(*c4);require(y.has_value(),"classification multi-thread schedule");require(x.value()->payload->canonical_bytes==y.value()->payload->canonical_bytes,"classification schedule determinism");

  auto translated_a=a,translated_b=b;translate(translated_a,T(8),T(-4),T(2));translate(translated_b,T(8),T(-4),T(2));auto translated_context=context(translated_a,translated_b,classification_test::registry());auto translated=classify_arrangement_cells(*translated_context);require(translated.has_value(),"translated classification");require(label_histogram(*translated.value()->payload)==label_histogram(*x.value()->payload),"translation-equivariant labels");
  auto scaled_a=input_test::box<T,I>(T(0),T(2)),scaled_b=input_test::box<T,I>(T(6),T(8));auto scaled_context=context(scaled_a,scaled_b,classification_test::registry());auto scaled=classify_arrangement_cells(*scaled_context);require(scaled.has_value(),"scaled classification");require(label_histogram(*scaled.value()->payload)==label_histogram(*x.value()->payload),"power-of-two scale-equivariant labels");
  auto swapped_context=context(b,a,classification_test::registry());auto swapped=classify_arrangement_cells(*swapped_context);require(swapped.has_value(),"operand-swapped classification");auto expected=label_histogram(*x.value()->payload);std::swap(expected[1],expected[2]);require(label_histogram(*swapped.value()->payload)==expected,"operand-swap occupancy mapping");

  cancellation_source stop;auto cancelled_context=context(a,b,classification_test::registry(),operation::regularized_union,boolean_options{},&stop);stop.cancel();auto cancelled=classify_arrangement_cells(*cancelled_context);require(!cancelled.has_value()&&cancelled.error().code==boolean_error_code::resource_limit,"classification cancellation category");require(cancelled_context->artifacts().latest_generation(artifact_slot::labeled_arrangement)==0,"classification cancellation rollback");

  const auto&base=*x.value()->payload;
  using member=resource_limit resource_policy::*;
  const std::array<std::tuple<member,resource_kind,std::uint64_t>,5> limits{{
    {&resource_policy::cells,resource_kind::cells,base.regions.size()},
    {&resource_policy::seed_certificates,resource_kind::seed_certificates,base.seeds.size()},
    {&resource_policy::classification_transitions,resource_kind::classification_transitions,base.transitions.size()},
    {&resource_policy::patch_side_labels,resource_kind::patch_side_labels,base.side_labels.size()},
    {&resource_policy::propagation_records,resource_kind::propagation_records,base.propagation.size()}}};
  for(const auto&entry:limits){const auto field=std::get<0>(entry);const auto kind=std::get<1>(entry);const auto required=std::get<2>(entry);require(required>0,"classification resource fixture");boolean_options exact;exact.resources.*field={false,required};auto exact_context=context(a,b,classification_test::registry(),operation::regularized_union,exact);auto accepted=classify_arrangement_cells(*exact_context);require(accepted.has_value(),"classification exact resource limit");require(exact_context->accountant().used(kind)==required,"classification exact resource charge");boolean_options short_limit;short_limit.resources.*field={false,required-1};auto short_context=context(a,b,classification_test::registry(),operation::regularized_union,short_limit);auto rejected=classify_arrangement_cells(*short_context);require(!rejected.has_value()&&rejected.error().code==boolean_error_code::resource_limit,"classification one-under resource rejection");require(short_context->artifacts().latest_generation(artifact_slot::labeled_arrangement)==0&&short_context->accountant().used(kind)==0,"classification resource rollback");}

  auto equal_a=input_test::box<T,I>(T(0),T(1)),equal_b=equal_a;auto equal_context=context(equal_a,equal_b,classification_test::registry());auto equal=classify_arrangement_cells(*equal_context);require(equal.has_value(),"equal classification");std::uint64_t coincident=0;for(const auto&t:equal.value()->payload->transitions)if(t.crossing==classification_crossing_kind::coincident){++coincident;require(t.transfer==classification_transfer_kind::both&&t.coincidence&&t.uses.size()==2,"complete coincident transfer provenance");}require(coincident>0,"equal solids have coincident transfers");
  auto partial_b=equal_b;translate(partial_b,T(0.5),T(0),T(0));auto partial_context=context(equal_a,partial_b,classification_test::registry());auto partial=classify_arrangement_cells(*partial_context);require(partial.has_value(),"partial coincidence classification");require(!partial.value()->payload->arrangement->payload->coincident_groups.empty(),"partial coincidence retained");
}
int main(){try{
  const auto operations={operation::regularized_union,operation::regularized_intersection,operation::a_minus_b,operation::b_minus_a,operation::symmetric_difference};
  auto run=[](auto scalar,auto index){using T=decltype(scalar);using I=decltype(index);
    run_analytic_classification_case<T,I>(classification_test::registry(),T(0),T(1),T(3),T(4));
    run_analytic_classification_case<T,I>(classification_test::registry(),T(0),T(3),T(1),T(2));
    run_analytic_classification_case<T,I>(classification_test::registry(),T(0),T(1),T(0),T(1));
    run_analytic_classification_case<T,I>(classification_test::registry(),T(0),T(2),T(1),T(3));
    run_analytic_classification_case<T,I>(classification_test::registry(),T(0),T(1),T(1),T(2));
  };
  run(float{},std::uint32_t{});run(float{},std::uint64_t{});run(double{},std::uint32_t{});run(double{},std::uint64_t{});

  std::vector<std::uint8_t> operation_independent;
  for(auto op:operations){auto bytes=run_analytic_classification_case<double,std::uint32_t>(classification_test::registry(),0.0,2.0,1.0,3.0,op);if(operation_independent.empty())operation_independent=std::move(bytes);else require(bytes==operation_independent,"classification canonical bytes independent of operation");}

  auto r=classification_test::registry();auto cavity=input_test::box<double,std::uint32_t>(0.0,6.0);input_test::append(cavity,input_test::box<double,std::uint32_t>(1.0,5.0),true);input_test::append(cavity,input_test::box<double,std::uint32_t>(2.0,4.0),false);fv_surface_mesh<double,std::uint32_t> empty;auto cavity_context=context(cavity,empty,r);auto cavity_result=classify_arrangement_cells(*cavity_context);if(!cavity_result.has_value())throw std::runtime_error("nested classification: "+render_error(cavity_result.error()));classification_oracle(*cavity_result.value()->payload);require(cavity_result.value()->payload->validated->payload->shells.size()==3,"classification retains nested shells");unsigned material=0,cavity_boundary=0;for(const auto&s:cavity_result.value()->payload->validated->payload->shells){material+=s.contribution==shell_contribution::material_boundary;cavity_boundary+=s.contribution==shell_contribution::cavity_boundary;}require(material==2&&cavity_boundary==1,"classification alternating shell polarity evidence");

  auto empty_context=context(empty,empty,classification_test::registry());auto empty_result=classify_arrangement_cells(*empty_context);if(!empty_result.has_value())throw std::runtime_error("empty classification: "+render_error(empty_result.error()));classification_oracle(*empty_result.value()->payload);require(empty_result.value()->payload->regions.size()==1,"empty universe region");require(empty_result.value()->payload->patch_labels.empty(),"empty universe has no patch labels");
  qualification<double,std::uint32_t>();
  std::cout<<"ok\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
