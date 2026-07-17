#include "MeshBooleanInputTopologyFixtures.h"
#include <iostream>
#include <stdexcept>
using namespace ygor::mesh_boolean;
using namespace input_test;
static void mutations() {
  using T = double;
  using I = std::uint32_t;
  auto a = tetra<T, I>(), b = fv_surface_mesh<T, I>{};
  auto registry = input_test::registry();
  auto c = context(a, b, registry);
  auto published = validate_operands(*c);
  require(published.has_value(), "mutation fixture");
  auto type = validated_operands_type_tag +
              (static_cast<std::uint64_t>(coordinate_tag::binary64) << 8);
  auto spec = registry
                  ->specification(artifact_slot::validated_operands, type,
                                  validated_operands_schema,
                                  verification_level::mandatory)
                  .value();
  verification_environment_view env;
  env.owner = c->owner();
  env.setup_digest = c->replay().setup;
  env.coordinate = coordinate_tag::binary64;
  env.index = index_tag::uint32;
  env.raw_operands = {env.coordinate, env.index, &a, &b};
  env.exact_kernel = &c->kernel();
  env.accountant = &c->accountant();
  auto payload = published.value()->payload;
  artifact_view view{c->owner(), artifact_slot::validated_operands,
                     type,       validated_operands_schema,
                     1,          payload->artifact_digest,
                     payload,    payload.get()};
  a.vertices[0].x = T(9);
  auto stale = registry->verify(view, spec, env);
  require(stale.has_value() && !stale.value().passed(),
          "raw coordinate mutation detected");
  a.vertices[0].x = T(0);
  auto corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->edge_uses[0].twin = corrupt->edge_uses[0].id;
  artifact_view corrupt_view{c->owner(), artifact_slot::validated_operands,
                             type,       validated_operands_schema,
                             1,          payload->artifact_digest,
                             corrupt,    corrupt.get()};
  auto changed = registry->verify(corrupt_view, spec, env);
  require(changed.has_value() && !changed.value().passed(),
          "candidate incidence mutation detected");
  auto rejected = [&](std::shared_ptr<validated_operands<T, I>> candidate,
                      const char *message) {
    artifact_view changed_view{c->owner(), artifact_slot::validated_operands,
                               type,       validated_operands_schema,
                               1,          payload->artifact_digest,
                               candidate,  candidate.get()};
    auto result = registry->verify(changed_view, spec, env);
    require(result.has_value() && !result.value().passed(), message);
  };
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->facets[0].triangles[0][0] = corrupt->facets[0].triangles[0][1];
  rejected(corrupt, "triangulation mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->facets[0].bounds.maximum.x = exact_scalar(99);
  rejected(corrupt, "bounds mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->facet_geometry[0].ring2[0].x = exact_scalar(99);
  rejected(corrupt, "cached projection mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->facet_geometry[0].edges[0].bounds.maximum.x = exact_scalar(99);
  rejected(corrupt, "cached edge bound mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->facet_geometry[0].triangles[0].bounds.maximum.x = exact_scalar(99);
  rejected(corrupt, "cached triangle bound mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->facet_geometry[0].oriented_normal.x = exact_scalar(99);
  rejected(corrupt, "cached normal mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->vertices[0].ordered_outgoing_link[0]=edge_use_id::from_canonical_value(corrupt->edge_uses.size());
  rejected(corrupt, "malformed cached fan source detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  std::reverse(corrupt->facet_geometry[0].vertex_fans[0].begin(),corrupt->facet_geometry[0].vertex_fans[0].end());
  rejected(corrupt, "cached vertex fan mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->shells[0].depth = 1;
  rejected(corrupt, "shell depth mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->operands[0].semantic_digest.bytes[0] ^= 1;
  rejected(corrupt, "semantic digest mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->evidence[0].checked += 1;
  rejected(corrupt, "artifact evidence mutation detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->raw_vertex_provenance[0][0] = corrupt->provenance.size();
  rejected(corrupt, "stale raw vertex index detected");
  corrupt = std::make_shared<validated_operands<T, I>>(*payload);
  corrupt->raw_facets[0][0] =
      facet_id::from_canonical_value(corrupt->facets.size());
  rejected(corrupt, "malformed raw facet index detected");
  auto saved_face = a.faces[0][0];
  a.faces[0][0] = a.faces[0][1];
  stale = registry->verify(view, spec, env);
  require(stale.has_value() && !stale.value().passed(),
          "raw ring mutation detected");
  a.faces[0][0] = saved_face;
}
static void dependency_and_replay(){verification_dependency_graph g;g.slot=artifact_slot::validated_operands;g.nodes={{0,dependency_node_kind::artifact,{0},false},{1,dependency_node_kind::facet,{1},false},{2,dependency_node_kind::source_vertex,{2},false},{3,dependency_node_kind::evidence,{3},true}};g.edges={{0,1,1,false},{1,2,2,false},{0,3,3,true}};auto sliced=slice_dependencies(g,{0});require(sliced.has_value()&&sliced.value().nodes.size()==3&&sliced.value().edges.size()==2,"dependency slice");auto bytes=encode_dependency_slice(sliced.value()).value();auto decoded=decode_dependency_slice(bytes);require(decoded.has_value()&&decoded.value().slice_digest==sliced.value().slice_digest,"dependency round trip");bytes.push_back(0);require(!decode_dependency_slice(bytes).has_value(),"dependency trailing bytes");replay_seed seed;seed.slot=artifact_slot::validated_operands;seed.payload={1,2,3};auto encoded=encode_replay_seed(seed).value();auto replay=decode_replay_seed(encoded);require(replay.has_value()&&replay.value().payload==seed.payload,"replay round trip");encoded[0]^=1;require(!decode_replay_seed(encoded).has_value(),"replay bad tag");verification_replay_archive archive;archive.coordinate=coordinate_tag::binary64;archive.operand_a={1};archive.operand_b={2};archive.artifact={3};archive.report={4};archive.dependencies={digest{}};auto archive_bytes=encode_replay_archive(archive).value();auto archive_copy=decode_replay_archive(archive_bytes);require(archive_copy.has_value()&&archive_copy.value().artifact==archive.artifact&&archive_copy.value().report==archive.report,"replay archive round trip");archive_bytes.push_back(0);require(!decode_replay_archive(archive_bytes).has_value(),"replay archive trailing bytes");}
static void resource_limit_test(){using T=double;using I=std::uint32_t;auto a=tetra<T,I>(),b=fv_surface_mesh<T,I>{};auto registry=input_test::registry();boolean_options options;options.resources.verifier_work={false,1};std::shared_ptr<const exact_kernel_services<T>>kernel=std::make_shared<exact_kernel<T>>();std::shared_ptr<const verifier_service>service=registry;auto made=make_boolean_context(a,b,operation::regularized_union,options,kernel,service);require(made.has_value(),"limited context");auto result=validate_operands(*made.value());require(!result.has_value()&&result.error().code==boolean_error_code::resource_limit,"verifier work limit");}
static void canonical_oracle(){canonical_graph a;a.nodes={{{1}},{{1}},{{2}}};a.arcs={{0,2,1},{2,0,2},{1,2,1},{2,1,2}};auto first=canonicalize_graph_exhaustive(a,nullptr);require(first.has_value()&&first.value().source_by_label.size()==3,"canonical oracle");std::swap(a.nodes[0],a.nodes[1]);auto second=canonicalize_graph_exhaustive(a,nullptr);require(second.has_value()&&first.value().canonical_bytes==second.value().canonical_bytes,"canonical permutation");resource_policy policy;policy.work_units={false,1};resource_accountant limited(policy);auto exhausted=canonicalize_graph_exhaustive(a,&limited);require(!exhausted.has_value()&&exhausted.error().code==boolean_error_code::resource_limit,"canonical oracle limit");}
static void reservation_rollback(){resource_accountant accountant(resource_policy{});{auto reservation=accountant.reserve_scoped(resource_kind::verifier_scratch_bytes,17,boolean_stage::input_validation);require(reservation.has_value()&&accountant.used(resource_kind::verifier_scratch_bytes)==17,"scoped reservation");}require(accountant.used(resource_kind::verifier_scratch_bytes)==0,"reservation rollback");{auto reservation=accountant.reserve_scoped(resource_kind::authoritative_bytes,9,boolean_stage::input_validation);reservation.value().commit();}require(accountant.used(resource_kind::authoritative_bytes)==9,"reservation commit");}
int main(){try{verifier_registry r;require(register_input_topology_verifier(r,coordinate_tag::binary32,index_tag::uint32).has_value(),"register");require(!register_input_topology_verifier(r,coordinate_tag::binary32,index_tag::uint32).has_value(),"duplicate");require(r.freeze().has_value(),"freeze");require(!register_input_topology_verifier(r,coordinate_tag::binary64,index_tag::uint64).has_value(),"late registration");auto s=r.specification(artifact_slot::validated_operands,validated_operands_type_tag+(static_cast<std::uint64_t>(coordinate_tag::binary32)<<8),validated_operands_schema,verification_level::mandatory);require(s.has_value(),"spec");require(s.value().required_invariants.size()==8,"mandatory set");auto bad=s.value();std::reverse(bad.required_invariants.begin(),bad.required_invariants.end());require(invariant_set_digest(bad)!=s.value().invariant_set_digest,"set binding");mutations();dependency_and_replay();resource_limit_test();canonical_oracle();reservation_rollback();std::cout<<"PASS verification registry\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
