#include "YgorMeshesBooleanQualificationCorpusInternal.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>

namespace ygor {
namespace mesh_boolean {
namespace {
using namespace qualification_corpus_detail;
constexpr std::array<char,8> records_tag{{'Y','G','B','Q','C','S','0','1'}};
constexpr std::array<char,8> coverage_tag{{'Y','G','B','Q','C','V','0','1'}};
constexpr std::array<char,8> outcomes_tag{{'Y','G','B','Q','C','O','0','1'}};
constexpr std::array<char,8> inventory_tag{{'Y','G','B','Q','C','I','0','1'}};
using op_set=std::set<qualification_operation_coverage>;
using type_set=std::set<std::pair<coordinate_tag,index_tag>>;
using result_set=std::set<std::pair<result_representation,product_realization_semantics>>;
struct coverage{op_set operations;type_set types;result_set results;
                std::set<preparation_mode> preparations;};
bool add(std::uint64_t &a,std::uint64_t b) noexcept {
  if(b>std::numeric_limits<std::uint64_t>::max()-a)return false;
  a+=b;return true;
}
}

product_status_or<qualification_corpus_inventory>
make_qualification_corpus_inventory(qualification_corpus_inventory inventory){
  using namespace qualification_corpus_detail;
  if(inventory.schema!=qualification_corpus_inventory_schema_version ||
     inventory.identifier.empty() || inventory.version.empty() || inventory.records.empty())
    return error("qualification_corpus_inventory_contract");
  for(auto &r:inventory.records){
    auto made=canonicalize_record(std::move(r));
    if(!made.has_value())return made.error();
    r=std::move(made.value());
  }
  std::sort(inventory.records.begin(),inventory.records.end(),
            [](const auto&a,const auto&b){return a.identifier<b.identifier;});
  std::set<std::string> ids,tests;
  std::map<std::pair<std::string,std::string>,std::uint64_t> recipe_end;
  std::map<qualification_geometry_category,coverage> covered;
  std::uint64_t generated=0,cad=0,chains=0,regressions=0;
  canonical_encoder records,outcomes;
  records.u64(inventory.records.size());outcomes.u64(inventory.records.size());
  for(const auto &r:inventory.records){
    if(!ids.insert(r.identifier).second||!tests.insert(r.permanent_test_id).second)
      return error("qualification_corpus_duplicate_identifier");
    const auto key=std::make_pair(r.recipe_identifier,r.recipe_version);
    const auto prior=recipe_end.find(key);
    if(prior!=recipe_end.end()&&r.first_case_ordinal<prior->second)
      return error("qualification_corpus_overlapping_recipe_ordinals");
    recipe_end[key]=r.first_case_ordinal+r.case_count;
    auto *total=r.kind==qualification_corpus_record_kind::generated_pair_family?&generated:
      r.kind==qualification_corpus_record_kind::cad_like_pair_family?&cad:
      r.kind==qualification_corpus_record_kind::operation_chain_family?&chains:&regressions;
    if(!add(*total,r.case_count))return error("qualification_corpus_count_overflow");
    records.string(r.identifier);encode_digest(records,r.record_digest);
    outcomes.string(r.identifier);encode_digest(outcomes,r.expectation_digest);
    outcomes.u64(r.expected_outcomes.size());
    for(auto v:r.expected_outcomes)outcomes.byte(static_cast<std::uint8_t>(v));
    outcomes.u64(r.expected_failure_codes.size());
    for(auto v:r.expected_failure_codes)outcomes.u16(static_cast<std::uint16_t>(v));
    for(auto category:r.geometry_categories){
      auto &c=covered[category];c.operations.insert(r.operations.begin(),r.operations.end());
      for(const auto &v:r.type_specializations)c.types.insert({v.coordinate,v.index});
      for(const auto &v:r.result_modes)c.results.insert({v.representation,v.semantics});
      for(const auto &v:r.preparation_policies)c.preparations.insert(v.mode);
    }
  }
  if(generated<qualification_generated_pair_floor)
    return error("qualification_corpus_generated_pair_floor");
  if(cad<qualification_cad_like_pair_floor)
    return error("qualification_corpus_cad_like_pair_floor");
  if(chains<qualification_operation_chain_floor)
    return error("qualification_corpus_operation_chain_floor");
  if(!regressions)return error("qualification_corpus_missing_regressions");

  const auto operation_vector=required_qualification_operation_coverage();
  const op_set required_operations(operation_vector.begin(),operation_vector.end());
  const type_set required_types{{coordinate_tag::binary32,index_tag::uint32},
    {coordinate_tag::binary32,index_tag::uint64},{coordinate_tag::binary64,index_tag::uint32},
    {coordinate_tag::binary64,index_tag::uint64}};
  const result_set required_results{{result_representation::exact_stratified,
      product_realization_semantics::not_requested},{result_representation::exact_in_T_mesh,
      product_realization_semantics::exact_in_T},{result_representation::certified_approximate_mesh,
      product_realization_semantics::certified_approximate_embedding_v1}};
  const std::set<preparation_mode> required_preparations{preparation_mode::strict_validation,
      preparation_mode::diagnosis_only,preparation_mode::normalized};
  canonical_encoder coverage_bytes;
  for(auto category:required_qualification_geometry_categories()){
    const auto found=covered.find(category);
    if(found==covered.end())return error("qualification_corpus_missing_geometry_category");
    if(found->second.operations!=required_operations)
      return error("qualification_corpus_incomplete_operation_coverage");
    if(found->second.types!=required_types)
      return error("qualification_corpus_incomplete_type_coverage");
    if(found->second.results!=required_results)
      return error("qualification_corpus_incomplete_result_coverage");
    if(found->second.preparations!=required_preparations)
      return error("qualification_corpus_incomplete_preparation_coverage");
    coverage_bytes.byte(static_cast<std::uint8_t>(category));
    for(const auto &v:found->second.operations){coverage_bytes.byte(static_cast<std::uint8_t>(v.selected_operation));
      coverage_bytes.byte(static_cast<std::uint8_t>(v.operand_order));}
    for(const auto &v:found->second.types){coverage_bytes.byte(static_cast<std::uint8_t>(v.first));
      coverage_bytes.byte(static_cast<std::uint8_t>(v.second));}
    for(const auto &v:found->second.results){coverage_bytes.byte(static_cast<std::uint8_t>(v.first));
      coverage_bytes.byte(static_cast<std::uint8_t>(v.second));}
    for(auto v:found->second.preparations)coverage_bytes.byte(static_cast<std::uint8_t>(v));
  }
  const digest record_digest=domain_digest(records_tag,records.bytes());
  const digest coverage_digest=domain_digest(coverage_tag,coverage_bytes.bytes());
  const digest outcome_digest=domain_digest(outcomes_tag,outcomes.bytes());
  canonical_encoder all;all.u16(inventory.schema);all.string(inventory.identifier);
  all.string(inventory.version);all.u64(generated);all.u64(cad);all.u64(chains);
  all.u64(regressions);encode_digest(all,record_digest);encode_digest(all,coverage_digest);
  encode_digest(all,outcome_digest);const digest complete=domain_digest(inventory_tag,all.bytes());
  if((inventory.generated_pair_count&&inventory.generated_pair_count!=generated)||
     (inventory.cad_like_pair_count&&inventory.cad_like_pair_count!=cad)||
     (inventory.operation_chain_count&&inventory.operation_chain_count!=chains)||
     (inventory.minimized_regression_count&&inventory.minimized_regression_count!=regressions)||
     (!digest_zero(inventory.record_set_digest)&&inventory.record_set_digest!=record_digest)||
     (!digest_zero(inventory.category_coverage_digest)&&inventory.category_coverage_digest!=coverage_digest)||
     (!digest_zero(inventory.expected_outcome_digest)&&inventory.expected_outcome_digest!=outcome_digest)||
     (!digest_zero(inventory.inventory_digest)&&inventory.inventory_digest!=complete))
    return error("qualification_corpus_stale_inventory_binding");
  inventory.generated_pair_count=generated;inventory.cad_like_pair_count=cad;
  inventory.operation_chain_count=chains;inventory.minimized_regression_count=regressions;
  inventory.record_set_digest=record_digest;inventory.category_coverage_digest=coverage_digest;
  inventory.expected_outcome_digest=outcome_digest;inventory.inventory_digest=complete;
  return inventory;
}

product_status_or<bool> validate_qualification_corpus_inventory(
    const qualification_corpus_inventory &inventory) noexcept {
  using namespace qualification_corpus_detail;
  try{
    auto made=make_qualification_corpus_inventory(inventory);
    if(!made.has_value())return made.error();
    const auto &v=made.value();
    if(v.records.size()!=inventory.records.size())
      return error("qualification_corpus_inventory_not_canonical");
    for(std::size_t i=0;i<v.records.size();++i)
      if(v.records[i].identifier!=inventory.records[i].identifier||
         v.records[i].record_digest!=inventory.records[i].record_digest)
        return error("qualification_corpus_inventory_not_canonical");
    if(v.inventory_digest!=inventory.inventory_digest||
       v.record_set_digest!=inventory.record_set_digest||
       v.category_coverage_digest!=inventory.category_coverage_digest||
       v.expected_outcome_digest!=inventory.expected_outcome_digest||
       v.generated_pair_count!=inventory.generated_pair_count||
       v.cad_like_pair_count!=inventory.cad_like_pair_count||
       v.operation_chain_count!=inventory.operation_chain_count||
       v.minimized_regression_count!=inventory.minimized_regression_count)
      return error("qualification_corpus_inventory_not_canonical");
    return true;
  }catch(...){return error("qualification_corpus_inventory_validation_exception");}
}

} // namespace mesh_boolean
} // namespace ygor
