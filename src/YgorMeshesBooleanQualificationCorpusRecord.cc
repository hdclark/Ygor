#include "YgorMeshesBooleanQualificationCorpusInternal.h"

#include <algorithm>
#include <array>
#include <limits>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace qualification_corpus_detail {
namespace {
constexpr std::array<char,8> record_tag{{'Y','G','B','Q','C','R','0','1'}};
template<class E> unsigned ordinal(E v) noexcept {
  return static_cast<unsigned>(v);
}
bool known(qualification_corpus_record_kind v) noexcept {
  return ordinal(v)<=ordinal(qualification_corpus_record_kind::minimized_regression);
}
bool known(qualification_operand_order v) noexcept {
  return ordinal(v)<=ordinal(qualification_operand_order::b_then_a);
}
bool known(qualification_geometry_category v) noexcept {
  return ordinal(v)<ordinal(qualification_geometry_category::count);
}
bool known(operation v) noexcept {
  return ordinal(v)<=ordinal(operation::symmetric_difference);
}
bool known(coordinate_tag v) noexcept {
  return ordinal(v)<=ordinal(coordinate_tag::binary64);
}
bool known(index_tag v) noexcept { return ordinal(v)<=ordinal(index_tag::uint64); }
bool known(result_representation v) noexcept {
  return ordinal(v)<=ordinal(result_representation::certified_approximate_mesh);
}
bool known(product_realization_semantics v) noexcept {
  return ordinal(v)<=ordinal(
      product_realization_semantics::certified_approximate_embedding_v1);
}
bool known(preparation_mode v) noexcept {
  return ordinal(v)<=ordinal(preparation_mode::normalized);
}
bool known(model_unit v) noexcept { return ordinal(v)<=ordinal(model_unit::foot); }
bool known(qualification_outcome v) noexcept {
  return ordinal(v)<=ordinal(qualification_outcome::infrastructure_failure);
}
bool known(product_error_code v) noexcept {
  return ordinal(v)<=ordinal(product_error_code::verifier_disagreement);
}
bool known(qualification_corpus_source v) noexcept {
  return ordinal(v)<=ordinal(qualification_corpus_source::minimized_regression);
}
bool known(qualification_redistribution v) noexcept {
  return ordinal(v)<=ordinal(qualification_redistribution::private_digest_only);
}
bool text(const std::string &s) noexcept {
  return !s.empty() && s.size()<=1024U*1024U &&
         std::find(s.begin(),s.end(),'\0')==s.end();
}
template<class V,class Less,class Equal>
bool canonical_set(std::vector<V> &v,Less less,Equal equal){
  std::sort(v.begin(),v.end(),less);
  return std::adjacent_find(v.begin(),v.end(),equal)==v.end();
}
template<class V> bool canonical_enum_set(std::vector<V> &v){
  return canonical_set(v,[](V a,V b){return ordinal(a)<ordinal(b);},
                        [](V a,V b){return a==b;});
}
bool source_matches(const qualification_corpus_record &r) noexcept {
  if(r.kind==qualification_corpus_record_kind::generated_pair_family ||
     r.kind==qualification_corpus_record_kind::operation_chain_family)
    return r.source==qualification_corpus_source::generated_construction_known;
  if(r.kind==qualification_corpus_record_kind::cad_like_pair_family)
    return r.source==qualification_corpus_source::internally_generated_cad_like ||
           r.source==qualification_corpus_source::licensed_external ||
           r.source==qualification_corpus_source::private_external;
  return r.source==qualification_corpus_source::minimized_regression;
}
bool result_matches(const qualification_result_mode_binding &v) noexcept {
  if(v.representation==result_representation::exact_stratified)
    return v.semantics==product_realization_semantics::not_requested;
  if(v.representation==result_representation::exact_in_T_mesh)
    return v.semantics==product_realization_semantics::exact_in_T;
  return v.semantics==
      product_realization_semantics::certified_approximate_embedding_v1;
}
bool allowed_expected(qualification_outcome v) noexcept {
  return v==qualification_outcome::verified_exact_success ||
         v==qualification_outcome::verified_certified_approximate_success ||
         v==qualification_outcome::expected_typed_failure ||
         v==qualification_outcome::timeout_or_resource_limit;
}
void encode(canonical_encoder &e,const qualification_type_binding &v){
  e.byte(static_cast<std::uint8_t>(v.coordinate));
  e.byte(static_cast<std::uint8_t>(v.index));
}
void encode(canonical_encoder &e,const qualification_result_mode_binding &v){
  e.byte(static_cast<std::uint8_t>(v.representation));
  e.byte(static_cast<std::uint8_t>(v.semantics)); encode_digest(e,v.policy_digest);
}
void encode(canonical_encoder &e,const qualification_preparation_binding &v){
  e.byte(static_cast<std::uint8_t>(v.mode));
  e.byte(static_cast<std::uint8_t>(v.tolerance_unit));
  e.u64(v.model_tolerance_binary64_bits); encode_digest(e,v.policy_digest);
}
void encode(canonical_encoder &e,const qualification_corpus_record &r){
  e.u16(r.schema); e.string(r.identifier);
  e.byte(static_cast<std::uint8_t>(r.kind));
  e.byte(static_cast<std::uint8_t>(r.source));
  e.byte(static_cast<std::uint8_t>(r.redistribution));
  e.string(r.license_or_provenance); e.string(r.recipe_identifier);
  e.string(r.recipe_version); encode_digest(e,r.recipe_digest);
  e.u64(r.first_case_ordinal); e.u64(r.case_count);
  e.u32(r.minimum_chain_steps); e.u32(r.maximum_chain_steps);
  e.u64(r.geometry_categories.size());
  for(auto v:r.geometry_categories)e.byte(static_cast<std::uint8_t>(v));
  e.u64(r.operations.size());
  for(const auto &v:r.operations){
    e.byte(static_cast<std::uint8_t>(v.selected_operation));
    e.byte(static_cast<std::uint8_t>(v.operand_order));
  }
  e.u64(r.type_specializations.size());
  for(const auto &v:r.type_specializations)encode(e,v);
  e.u64(r.result_modes.size()); for(const auto &v:r.result_modes)encode(e,v);
  e.u64(r.preparation_policies.size());
  for(const auto &v:r.preparation_policies)encode(e,v);
  e.u64(r.expected_outcomes.size());
  for(auto v:r.expected_outcomes)e.byte(static_cast<std::uint8_t>(v));
  e.u64(r.expected_failure_codes.size());
  for(auto v:r.expected_failure_codes)e.u16(static_cast<std::uint16_t>(v));
  e.string(r.expectation_identifier); encode_digest(e,r.expectation_digest);
  e.string(r.permanent_test_id);
}
} // namespace

product_error error(const char *key){
  return make_product_error(product_error_code::qualification_policy_violation,key);
}
bool digest_zero(const digest &d) noexcept { return d==digest{}; }
void encode_digest(canonical_encoder &e,const digest &d){
  e.raw(d.bytes.data(),d.bytes.size());
}

product_status_or<qualification_corpus_record>
canonicalize_record(qualification_corpus_record r){
  if(r.schema!=qualification_corpus_inventory_schema_version || !text(r.identifier) ||
     !known(r.kind) || !known(r.source) || !known(r.redistribution) ||
     !text(r.license_or_provenance) || !text(r.recipe_identifier) ||
     !text(r.recipe_version) || digest_zero(r.recipe_digest) || !r.case_count ||
     !text(r.expectation_identifier) || digest_zero(r.expectation_digest) ||
     !text(r.permanent_test_id) || !source_matches(r))
    return error("qualification_corpus_record_contract");
  if(r.first_case_ordinal>std::numeric_limits<std::uint64_t>::max()-(r.case_count-1))
    return error("qualification_corpus_ordinal_overflow");
  const bool chain=r.kind==qualification_corpus_record_kind::operation_chain_family;
  if(chain){
    if(r.minimum_chain_steps<qualification_operation_chain_step_floor ||
       r.maximum_chain_steps<r.minimum_chain_steps)
      return error("qualification_corpus_chain_step_floor");
  }else if(r.minimum_chain_steps || r.maximum_chain_steps){
    return error("qualification_corpus_nonchain_steps");
  }
  if(r.kind==qualification_corpus_record_kind::minimized_regression && r.case_count!=1)
    return error("qualification_corpus_regression_singleton");
  if(r.geometry_categories.empty() || r.operations.empty() ||
     r.type_specializations.empty() || r.result_modes.empty() ||
     r.preparation_policies.empty() || r.expected_outcomes.empty())
    return error("qualification_corpus_record_missing_coverage");

  for(auto v:r.geometry_categories)if(!known(v))
    return error("qualification_corpus_unknown_geometry_category");
  if(!canonical_enum_set(r.geometry_categories))
    return error("qualification_corpus_duplicate_geometry_category");
  for(const auto &v:r.operations)if(!known(v.selected_operation)||!known(v.operand_order))
    return error("qualification_corpus_unknown_operation_coverage");
  if(!canonical_set(r.operations,[](const auto&a,const auto&b){return a<b;},
                                  [](const auto&a,const auto&b){return a==b;}))
    return error("qualification_corpus_duplicate_operation_coverage");
  for(const auto &v:r.type_specializations)if(!known(v.coordinate)||!known(v.index))
    return error("qualification_corpus_unknown_type_specialization");
  if(!canonical_set(r.type_specializations,
      [](const auto&a,const auto&b){return std::tie(a.coordinate,a.index)<
                                         std::tie(b.coordinate,b.index);},
      [](const auto&a,const auto&b){return a.coordinate==b.coordinate&&a.index==b.index;}))
    return error("qualification_corpus_duplicate_type_specialization");
  for(const auto &v:r.result_modes)
    if(!known(v.representation)||!known(v.semantics)||digest_zero(v.policy_digest)||
       !result_matches(v))return error("qualification_corpus_invalid_result_mode");
  if(!canonical_set(r.result_modes,
      [](const auto&a,const auto&b){return std::tie(a.representation,a.semantics,a.policy_digest)<
                                         std::tie(b.representation,b.semantics,b.policy_digest);},
      [](const auto&a,const auto&b){return a.representation==b.representation&&
          a.semantics==b.semantics&&a.policy_digest==b.policy_digest;}))
    return error("qualification_corpus_duplicate_result_mode");
  for(const auto &v:r.preparation_policies){
    if(!known(v.mode)||!known(v.tolerance_unit)||digest_zero(v.policy_digest))
      return error("qualification_corpus_invalid_preparation_policy");
    if(v.mode==preparation_mode::strict_validation &&
       (v.tolerance_unit!=model_unit::unspecified||v.model_tolerance_binary64_bits))
      return error("qualification_corpus_hidden_strict_tolerance");
  }
  if(!canonical_set(r.preparation_policies,
      [](const auto&a,const auto&b){return std::tie(a.mode,a.tolerance_unit,
          a.model_tolerance_binary64_bits,a.policy_digest)<std::tie(b.mode,
          b.tolerance_unit,b.model_tolerance_binary64_bits,b.policy_digest);},
      [](const auto&a,const auto&b){return a.mode==b.mode&&
          a.tolerance_unit==b.tolerance_unit&&
          a.model_tolerance_binary64_bits==b.model_tolerance_binary64_bits&&
          a.policy_digest==b.policy_digest;}))
    return error("qualification_corpus_duplicate_preparation_policy");
  for(auto v:r.expected_outcomes)if(!known(v)||!allowed_expected(v))
    return error("qualification_corpus_invalid_expected_outcome");
  if(!canonical_enum_set(r.expected_outcomes))
    return error("qualification_corpus_duplicate_expected_outcome");
  for(auto v:r.expected_failure_codes)if(!known(v))
    return error("qualification_corpus_unknown_expected_failure");
  if(!canonical_enum_set(r.expected_failure_codes))
    return error("qualification_corpus_duplicate_expected_failure");
  const bool typed=std::find(r.expected_outcomes.begin(),r.expected_outcomes.end(),
      qualification_outcome::expected_typed_failure)!=r.expected_outcomes.end();
  if(typed!=!r.expected_failure_codes.empty())
    return error("qualification_corpus_typed_failure_detail_mismatch");
  canonical_encoder e; encode(e,r);
  const digest computed=domain_digest(record_tag,e.bytes());
  if(!digest_zero(r.record_digest)&&r.record_digest!=computed)
    return error("qualification_corpus_stale_record_digest");
  r.record_digest=computed;
  return r;
}

} // namespace qualification_corpus_detail

const char *qualification_corpus_record_kind_token(
    qualification_corpus_record_kind v) noexcept {
  switch(v){
  case qualification_corpus_record_kind::generated_pair_family:return "generated_pair_family";
  case qualification_corpus_record_kind::cad_like_pair_family:return "cad_like_pair_family";
  case qualification_corpus_record_kind::operation_chain_family:return "operation_chain_family";
  case qualification_corpus_record_kind::minimized_regression:return "minimized_regression";
  }
  return "invalid";
}
const char *qualification_operand_order_token(qualification_operand_order v) noexcept {
  return v==qualification_operand_order::a_then_b?"a_then_b":
         v==qualification_operand_order::b_then_a?"b_then_a":"invalid";
}
const char *qualification_geometry_category_token(
    qualification_geometry_category v) noexcept {
  static const char *const names[]={"non_box_intersection","rotated_or_skewed_convex",
    "concave_or_reentrant","disconnected_components","nested_shells_or_cavities",
    "coplanar_overlay","high_valence_contact","thin_sliver_or_dense",
    "alternate_subdivision","scale_extremes","floating_point_edge_cases",
    "non_dyadic_intersection","stratified_non_manifold","index_or_resource_boundary",
    "attribute_or_provenance_conflict","normalization_defect","serialization_or_replay"};
  const auto i=static_cast<unsigned>(v);
  return i<static_cast<unsigned>(qualification_geometry_category::count)?names[i]:"invalid";
}
std::vector<qualification_geometry_category> required_qualification_geometry_categories(){
  std::vector<qualification_geometry_category> result;
  for(unsigned i=0;i<static_cast<unsigned>(qualification_geometry_category::count);++i)
    result.push_back(static_cast<qualification_geometry_category>(i));
  return result;
}
std::vector<qualification_operation_coverage> required_qualification_operation_coverage(){
  std::vector<qualification_operation_coverage> result;
  for(unsigned i=0;i<=static_cast<unsigned>(operation::symmetric_difference);++i){
    result.push_back({static_cast<operation>(i),qualification_operand_order::a_then_b});
    result.push_back({static_cast<operation>(i),qualification_operand_order::b_then_a});
  }
  return result;
}

} // namespace mesh_boolean
} // namespace ygor
