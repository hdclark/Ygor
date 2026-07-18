#include "MeshBooleanCellClassificationFixtures.h"
#include <iostream>
using namespace classification_test;

int main(){try{
  auto r=classification_test::registry();
  auto a=cube<double,std::uint32_t>(),b=cube<double,std::uint32_t>();translate(b,3,0,0);
  auto c=context(a,b,r);auto result=classify_arrangement_cells(*c);
  if(!result.has_value())throw std::runtime_error(render_error(result.error()));
  const auto&x=*result.value()->payload;classification_oracle(x);
  require(result.value()->report.passed(),"verified");
  require(c->artifacts().latest_generation(artifact_slot::labeled_arrangement)==1,"published");
  auto repeated=classify_arrangement_cells(*c);require(repeated.has_value()&&repeated.value().get()==result.value().get(),"idempotent");

  mutation_rejected(*r,*c,x,[](auto&v){v.owner=context_owner_token{};},"owner mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.validated.reset();},"null validated dependency rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.regions.front().label.in_a=!v.regions.front().label.in_a;},"region label mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.seeds.front().source_component=open_region_component_id::from_canonical_value(v.regions.size());},"seed binding mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){++v.seeds.front().operand_a_primary.signed_degree;},"ray degree mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.propagation.front().root=classification_region_id::from_canonical_value(v.regions.size());},"propagation mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.transitions.front().reverse=v.transitions.front().id;},"reverse transition mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.arc_checks.front().transferred.in_b=!v.arc_checks.front().transferred.in_b;},"arc evidence mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.side_labels.front().occupancy.in_a=!v.side_labels.front().occupancy.in_a;},"side label mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){++v.certificate.regions;},"certificate mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.canonical_bytes.push_back(0);},"canonical encoding mutation rejected");
  mutation_rejected(*r,*c,x,[](auto&v){v.artifact_bytes.push_back(0);},"artifact encoding mutation rejected");

  auto archive=replay_archive(*result.value(),*c);auto encoded=encode_replay_archive(archive);
  require(encoded.has_value(),"classification replay encode");auto decoded=decode_replay_archive(encoded.value());
  require(decoded.has_value()&&decoded.value().artifact==x.artifact_bytes&&decoded.value().dependencies==result.value()->report.dependency_digests,"classification replay round trip");
  auto malformed=encoded.value();malformed.pop_back();require(!decode_replay_archive(malformed).has_value(),"classification truncated replay rejected");
  std::cout<<"ok\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
