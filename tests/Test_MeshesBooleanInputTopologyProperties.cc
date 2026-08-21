#include "MeshBooleanInputTopologyFixtures.h"
#include <algorithm>
#include <iostream>
using namespace input_test;
namespace {
template<class T>std::size_t exhaustive_rotation(const std::vector<T>&v){std::size_t best=0;for(std::size_t candidate=1;candidate<v.size();++candidate){std::vector<T>a,b;for(std::size_t i=0;i<v.size();++i){a.push_back(v[(candidate+i)%v.size()]);b.push_back(v[(best+i)%v.size()]);}if(a<b)best=candidate;}return best;}
template<class T>std::vector<T>rotation_key(const std::vector<T>&v,std::size_t shift){std::vector<T>out;for(std::size_t i=0;i<v.size();++i)out.push_back(v[(shift+i)%v.size()]);return out;}
exact_scalar polygon_area2(const std::vector<exact_point2>&p){exact_scalar area(0);for(std::size_t i=0;i<p.size();++i)area=area+p[i].x*p[(i+1)%p.size()].y-p[i].y*p[(i+1)%p.size()].x;return area;}
bool triangle_contains(const exact_point2&p,const exact_point2&a,const exact_point2&b,const exact_point2&c,exact_sign winding){const auto x=orient2d(a,b,p),y=orient2d(b,c,p),z=orient2d(c,a,p);return winding==exact_sign::positive?x!=exact_sign::negative&&y!=exact_sign::negative&&z!=exact_sign::negative:x!=exact_sign::positive&&y!=exact_sign::positive&&z!=exact_sign::positive;}
std::vector<std::array<std::size_t,3>> exhaustive_triangulation(const std::vector<exact_point2>&p){std::vector<std::array<std::size_t,3>>out;std::vector<std::size_t>cycle(p.size());for(std::size_t i=0;i<p.size();++i)cycle[i]=i;const auto winding=polygon_area2(p).sign();while(cycle.size()>3){std::optional<std::size_t>best;std::array<std::size_t,3>best_key{{0,0,0}};for(std::size_t k=0;k<cycle.size();++k){const auto a=cycle[(k+cycle.size()-1)%cycle.size()],b=cycle[k],c=cycle[(k+1)%cycle.size()];if(orient2d(p[a],p[b],p[c])!=winding)continue;bool clear=true;for(auto q:cycle)if(q!=a&&q!=b&&q!=c&&triangle_contains(p[q],p[a],p[b],p[c],winding)){clear=false;break;}if(!clear)continue;const exact_point2 midpoint{(p[a].x+p[c].x)/exact_scalar(2),(p[a].y+p[c].y)/exact_scalar(2)};const auto location=classify_point_polygon(midpoint,p);if(!location.has_value()||location.value().kind!=point_region_kind::open_interior)continue;const std::array<std::size_t,3>key{{a,b,c}};if(!best||key<best_key){best=k;best_key=key;}}require(best.has_value(),"exhaustive triangulation finds ear");const auto k=*best,a=cycle[(k+cycle.size()-1)%cycle.size()],b=cycle[k],c=cycle[(k+1)%cycle.size()];out.push_back({{a,b,c}});cycle.erase(cycle.begin()+k);}out.push_back({{cycle[0],cycle[1],cycle[2]}});return out;}
void rotation_oracle(){std::uint32_t state=0x9e3779b9U;for(std::size_t n=1;n<=12;++n)for(std::size_t sample=0;sample<256;++sample){std::vector<std::uint32_t>values(n);for(auto&v:values){state=state*1664525U+1013904223U;v=(state>>28)+(sample%3);}const auto linear=input_topology_detail::minimal_cyclic_rotation(values),exhaustive=exhaustive_rotation(values);require(rotation_key(values,linear)==rotation_key(values,exhaustive),"linear rotation matches exhaustive oracle");auto reversed=values;std::reverse(reversed.begin(),reversed.end());const auto forward_key=rotation_key(values,linear),reverse_key=rotation_key(reversed,input_topology_detail::minimal_cyclic_rotation(reversed));const auto exhaustive_unoriented=std::min(rotation_key(values,exhaustive),rotation_key(reversed,exhaustive_rotation(reversed)));require(std::min(forward_key,reverse_key)==exhaustive_unoriented,"unoriented rotation matches exhaustive oracle");}}
bool same_plane(const exact_plane3&a,const exact_plane3&b){return a.a==b.a&&a.b==b.b&&a.c==b.c&&a.d==b.d&&a.oriented==b.oriented;}
template<class T,class I>void artifact_oracles(){auto a=prism<T,I>(true),b=fv_surface_mesh<T,I>{};auto r=registry();auto c=context(a,b,r);auto x=validate_operands(*c);require(x.has_value(),"oracle prism validates");const auto&artifact=*x.value()->payload;for(std::size_t raw_face=0;raw_face<a.faces.size();++raw_face){const auto&facet=artifact.facets[artifact.raw_facets[0][raw_face].value_for_debug()];const auto&geometry=artifact.facet_geometry[facet.id.value_for_debug()];std::vector<original_vertex_id>raw_ids;for(auto raw_vertex:a.faces[raw_face]){const auto provenance=artifact.raw_vertex_provenance[0][raw_vertex];raw_ids.push_back(*artifact.provenance[provenance].canonical_vertex);}const auto expected=rotation_key(raw_ids,exhaustive_rotation(raw_ids));require(facet.ring==expected,"stored ring matches exhaustive rotation");status_or<exact_plane3>plane=make_error(boolean_error_code::input_contract_error,boolean_stage::input_validation,"no_support");for(std::size_t i=0;i<facet.ring.size()&&!plane.has_value();++i)for(std::size_t j=i+1;j<facet.ring.size()&&!plane.has_value();++j)for(std::size_t k=j+1;k<facet.ring.size()&&!plane.has_value();++k)plane=support_plane_dyadic(artifact.vertices[facet.ring[i].value_for_debug()].exact_coordinate,artifact.vertices[facet.ring[j].value_for_debug()].exact_coordinate,artifact.vertices[facet.ring[k].value_for_debug()].exact_coordinate);require(plane.has_value()&&same_plane(plane.value(),facet.plane),"linear support plane matches exhaustive triple oracle");std::vector<exact_point2>projected;for(std::size_t i=0;i<facet.ring.size();++i){const auto point=artifact.vertices[facet.ring[i].value_for_debug()].exact_coordinate;projected.push_back(project(point,facet.projection));require(geometry.ring3[i]==point&&geometry.ring2[i]==projected.back(),"cached ring matches independent reconstruction");const auto&edge=geometry.edges[i];require(edge.segment.origin==point&&edge.segment.destination==artifact.vertices[facet.ring[(i+1)%facet.ring.size()].value_for_debug()].exact_coordinate,"cached edge matches independent reconstruction");std::vector<facet_id>fan;for(auto use:artifact.vertices[facet.ring[i].value_for_debug()].ordered_outgoing_link)fan.push_back(artifact.edge_uses[use.value_for_debug()].facet);require(geometry.vertex_fans[i]==fan,"cached fan matches independent reconstruction");}const auto oracle=exhaustive_triangulation(projected);require(oracle.size()==facet.triangles.size(),"incremental triangulation oracle size");for(std::size_t i=0;i<oracle.size();++i){require(facet.triangles[i]==std::array<original_vertex_id,3>{{facet.ring[oracle[i][0]],facet.ring[oracle[i][1]],facet.ring[oracle[i][2]]}},"incremental triangulation matches ordered exhaustive oracle");const auto&t=facet.triangles[i];require(geometry.triangles[i].triangle.a==artifact.vertices[t[0].value_for_debug()].exact_coordinate&&geometry.triangles[i].triangle.b==artifact.vertices[t[1].value_for_debug()].exact_coordinate&&geometry.triangles[i].triangle.c==artifact.vertices[t[2].value_for_debug()].exact_coordinate,"cached triangle matches independent reconstruction");}}}
}
template<class T,class I>void rotated(){auto a=tetra<T,I>(),b=fv_surface_mesh<T,I>{};for(auto&f:a.faces)std::rotate(f.begin(),f.begin()+1,f.end());std::reverse(a.faces.begin(),a.faces.end());auto r=registry();auto c=context(a,b,r);auto x=validate_operands(*c);require(x.has_value(),"rotated rings and facets validate");require(x.value()->payload->edges.size()==6,"edge reconstruction");}
template<class T,class I>void canonical_digest(){auto a=prism<T,I>(true),permuted=a,b=fv_surface_mesh<T,I>{};std::reverse(permuted.vertices.begin(),permuted.vertices.end());for(auto&f:permuted.faces)for(auto&v:f)v=static_cast<I>(permuted.vertices.size()-1-v);for(auto&f:permuted.faces)std::rotate(f.begin(),f.begin()+2%f.size(),f.end());std::reverse(permuted.faces.begin(),permuted.faces.end());auto r=registry();auto c1=context(a,b,r),c2=context(permuted,b,r);auto x=validate_operands(*c1),y=validate_operands(*c2);require(x.has_value()&&y.has_value(),"permutations validate");require(x.value()->payload->operands[0].semantic_digest==y.value()->payload->operands[0].semantic_digest,"canonical semantic digest");require(x.value()->payload->artifact_digest!=y.value()->payload->artifact_digest,"provenance-bound artifact digest");}
template<class T,class I>void repeated_determinism(){auto a=prism<T,I>(true),b=fv_surface_mesh<T,I>{};auto r=registry();std::optional<digest>artifact,report;for(int i=0;i<8;++i){auto c=context(a,b,r);auto x=validate_operands(*c);require(x.has_value(),"repeated validation");if(artifact){require(*artifact==x.value()->payload->artifact_digest,"repeated artifact digest");require(*report==x.value()->report.report_digest,"repeated report digest");}else{artifact=x.value()->payload->artifact_digest;report=x.value()->report.report_digest;}}}
template<class T,class I>void component_permutation(){auto a=box<T,I>(T(0),T(1)),permuted=box<T,I>(T(3),T(4)),b=fv_surface_mesh<T,I>{};append(a,box<T,I>(T(3),T(4)));append(permuted,box<T,I>(T(0),T(1)));auto r=registry();auto c1=context(a,b,r),c2=context(permuted,b,r);auto x=validate_operands(*c1),y=validate_operands(*c2);require(x.has_value()&&y.has_value(),"component permutations validate");require(x.value()->payload->operands[0].semantic_digest==y.value()->payload->operands[0].semantic_digest,"component permutation semantic digest");}
std::uint64_t verifier_resources(std::size_t components) {
  using T = double;
  using I = std::uint32_t;
  fv_surface_mesh<T, I> a, b;
  for (std::size_t i = 0; i < components; ++i)
    append(a, box<T, I>(T(3 * i), T(3 * i + 1)));
  auto verifiers = registry();
  boolean_options options;
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> service = verifiers;
  auto made = make_boolean_context(a, b, operation::regularized_union, options,
                                   kernel, service);
  require(made.has_value(), "resource context");
  auto result = validate_operands(*made.value());
  require(result.has_value(), "resource fixture validates");
  const auto type = validated_operands_type_tag +
                    (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
  auto spec = verifiers->specification(
      artifact_slot::validated_operands, type, validated_operands_schema,
      verification_level::mandatory);
  require(spec.has_value(), "input resource specification");
  artifact_view view{made.value()->owner(), artifact_slot::validated_operands,
                     type, validated_operands_schema, 1,
                     result.value()->payload->artifact_digest,
                     result.value()->payload, result.value()->payload.get()};
  verification_environment_view env;
  env.owner = made.value()->owner();
  env.setup_digest = made.value()->replay().setup;
  env.coordinate = coordinate_tag::binary64;
  env.index = index_tag::uint32;
  env.raw_operands = {env.coordinate, env.index, &a, &b};
  env.exact_kernel = &made.value()->kernel();
  auto run=[&](resource_kind kind,std::uint64_t limit){resource_policy policy;if(kind==resource_kind::verifier_work)policy.verifier_work={false,limit};else policy.verifier_scratch_bytes={false,limit};resource_accountant accountant(policy);env.accountant=&accountant;auto checked=verifiers->verify(view,spec.value(),env);require(accountant.used(resource_kind::verifier_work)==0&&accountant.used(resource_kind::verifier_scratch_bytes)==0,"input verifier temporary resources roll back");return checked;};
  auto threshold=[&](resource_kind kind){std::uint64_t high=1;while(!run(kind,high).has_value())high*=2;std::uint64_t low=0;while(low+1<high){const auto middle=low+(high-low)/2;if(run(kind,middle).has_value())high=middle;else low=middle;}auto exact=run(kind,high);require(exact.has_value()&&exact.value().passed(),"input verifier exact resource limit");auto short_result=run(kind,high-1);require(!short_result.has_value()&&short_result.error().code==boolean_error_code::resource_limit,"input verifier one-under resource limit");return high;};
  const auto work=threshold(resource_kind::verifier_work);
  const auto scratch=threshold(resource_kind::verifier_scratch_bytes);
  require(work>0&&scratch>0,"input verifier resources admitted");
  return work;
}
void verifier_resource_scaling() {
  const auto small = verifier_resources(2);
  const auto large = verifier_resources(4);
  require(large < 3 * small,
          "disjoint input verifier work scales below quadratic doubling");
}
input_topology_detail::facet_sweep_entry sweep_entry(
    int min_x, int max_x, int min_y, int max_y, int min_z, int max_z,
    operand_id operand = operand_a()) {
  return {{{exact_scalar(min_x), exact_scalar(min_y), exact_scalar(min_z)},
           {exact_scalar(max_x), exact_scalar(max_y), exact_scalar(max_z)}},
          operand};
}
bool overlaps(const exact_box3 &a, const exact_box3 &b) {
  return !(a.maximum.x < b.minimum.x || b.maximum.x < a.minimum.x ||
           a.maximum.y < b.minimum.y || b.maximum.y < a.minimum.y ||
           a.maximum.z < b.minimum.z || b.maximum.z < a.minimum.z);
}
void facet_sweep_index_properties() {
  using input_topology_detail::facet_sweep_entry;
  using input_topology_detail::verifier_facet_sweep_candidates;
  std::vector<facet_sweep_entry> separated;
  constexpr std::size_t count = 128;
  for (std::size_t i = 0; i < count; ++i)
    separated.push_back(sweep_entry(0, 10, static_cast<int>(2 * i),
                                    static_cast<int>(2 * i + 1), 0, 1));
  const auto pruned = verifier_facet_sweep_candidates(separated);
  require(pruned.candidates.empty(), "equal-x disjoint-y candidates prune");
  require(pruned.counters.index_visits < 32 * count,
          "equal-x disjoint-y traversal is index bounded");
  require(pruned.counters.leaf_tests == 0,
          "equal-x disjoint-y avoids candidate leaves");

  std::uint32_t state = 0x65d31a4bU;
  for (std::size_t sample = 0; sample < 80; ++sample) {
    std::vector<facet_sweep_entry> entries;
    const auto size = 1U + sample % 18U;
    for (std::size_t i = 0; i < size; ++i) {
      auto next = [&] { state = state * 1664525U + 1013904223U; return state; };
      const int x = static_cast<int>(next() % 7U) - 3;
      const int y = static_cast<int>(next() % 11U) - 5;
      const int z = static_cast<int>(next() % 9U) - 4;
      const int dx = static_cast<int>(next() % 4U);
      const int dy = static_cast<int>(next() % 4U);
      const int dz = static_cast<int>(next() % 4U);
      entries.push_back(sweep_entry(x, x + dx, y, y + dy, z, z + dz,
                                    (next() & 1U) ? operand_a() : operand_b()));
    }
    std::vector<std::pair<std::size_t, std::size_t>> exhaustive;
    for (std::size_t i = 0; i < entries.size(); ++i)
      for (std::size_t j = i + 1; j < entries.size(); ++j)
        if (entries[i].operand == entries[j].operand &&
            overlaps(entries[i].bounds, entries[j].bounds))
          exhaustive.emplace_back(i, j);
    auto indexed = verifier_facet_sweep_candidates(entries);
    std::sort(indexed.candidates.begin(), indexed.candidates.end());
    require(indexed.candidates == exhaustive,
            "bounded facet sweep candidates match exhaustive pairs");
    require(indexed.counters.conservative_candidates == indexed.candidates.size(),
            "facet sweep candidate accounting is exact");
    const auto repeated = verifier_facet_sweep_candidates(entries);
    auto repeated_candidates = repeated.candidates;
    std::sort(repeated_candidates.begin(), repeated_candidates.end());
    require(repeated_candidates == indexed.candidates &&
                repeated.counters.sort_work == indexed.counters.sort_work &&
                repeated.counters.build_nodes == indexed.counters.build_nodes &&
                repeated.counters.index_visits == indexed.counters.index_visits &&
                repeated.counters.leaf_tests == indexed.counters.leaf_tests,
            "facet sweep candidates and counters are deterministic");
  }
}
void containment_differential(){using T=double;using I=std::uint32_t;std::uint32_t state=0x71c3a5d9U;for(std::size_t sample=0;sample<20;++sample){state=state*1664525U+1013904223U;fv_surface_mesh<T,I>a,b;const auto separated=1U+(state%4U);for(std::uint32_t i=0;i<separated;++i){const T base=T(12*i);append(a,box<T,I>(base,base+T(4)));}if((state>>8)&1U){append(a,box<T,I>(T(1),T(3)),true);if((state>>9)&1U)append(a,box<T,I>(T(1.5),T(2.5)));}auto verifiers=registry();auto made=context(a,b,verifiers);auto artifact=validate_operands(*made);require(artifact.has_value(),"bounded containment fixture validates");const auto type=validated_operands_type_tag+(static_cast<std::uint64_t>(coordinate_tag::binary64)<<8);artifact_view view{made->owner(),artifact_slot::validated_operands,type,validated_operands_schema,1,artifact.value()->payload->artifact_digest,artifact.value()->payload,artifact.value()->payload.get()};verification_environment_view env;env.owner=made->owner();env.setup_digest=made->replay().setup;env.coordinate=coordinate_tag::binary64;env.index=index_tag::uint32;env.raw_operands={env.coordinate,env.index,&a,&b};env.exact_kernel=&made->kernel();resource_accountant mandatory_accountant(resource_policy{}),exhaustive_accountant(resource_policy{});auto mandatory=verifiers->specification(artifact_slot::validated_operands,type,validated_operands_schema,verification_level::mandatory);auto exhaustive=verifiers->specification(artifact_slot::validated_operands,type,validated_operands_schema,verification_level::exhaustive);require(mandatory.has_value()&&exhaustive.has_value(),"containment differential specifications");env.accountant=&mandatory_accountant;auto fast=verifiers->verify(view,mandatory.value(),env);env.accountant=&exhaustive_accountant;auto oracle=verifiers->verify(view,exhaustive.value(),env);require(fast.has_value()&&oracle.has_value()&&fast.value().passed()&&oracle.value().passed(),"mandatory containment matches exhaustive oracle");require(mandatory_accountant.used(resource_kind::verifier_work)==0&&mandatory_accountant.used(resource_kind::verifier_scratch_bytes)==0&&exhaustive_accountant.used(resource_kind::verifier_work)==0&&exhaustive_accountant.used(resource_kind::verifier_scratch_bytes)==0,"differential verifier resources roll back");}}
int main(){try{rotation_oracle();rotated<float,std::uint32_t>();rotated<float,std::uint64_t>();rotated<double,std::uint32_t>();rotated<double,std::uint64_t>();canonical_digest<double,std::uint32_t>();artifact_oracles<double,std::uint32_t>();component_permutation<double,std::uint32_t>();repeated_determinism<double,std::uint32_t>();facet_sweep_index_properties();containment_differential();verifier_resource_scaling();std::cout<<"PASS input properties\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
