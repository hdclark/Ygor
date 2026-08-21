#include "YgorMeshesBooleanAttributes.h"
#include "YgorMeshesBooleanExactResult.h"
#include "YgorMeshesBooleanInputTopology.h"
#include "YgorMeshesBooleanNormalization.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> source_input_tag{
    {'Y', 'G', 'B', 'A', 'S', 'R', '0', '1'}};
constexpr std::array<char, 8> report_tag{
    {'Y', 'G', 'B', 'A', 'T', 'R', '0', '1'}};
constexpr std::array<char, 8> report_digest_tag{
    {'Y', 'G', 'B', 'A', 'T', 'D', '0', '1'}};
constexpr std::array<char, 8> catalog_digest_tag{
    {'Y', 'G', 'B', 'A', 'C', 'A', 'T', '1'}};
constexpr std::array<char, 8> value_digest_tag{
    {'Y', 'G', 'B', 'A', 'V', 'A', 'L', '1'}};

product_error attr_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

bool digest_zero(const digest &d) noexcept { return d == digest{}; }

bool known(attribute_source_entity_kind v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_source_entity_kind::edge);
}
bool known(attribute_channel_kind v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_channel_kind::construction_provenance);
}
bool known(attribute_target_kind v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_target_kind::output_face);
}
bool known(attribute_resolution_kind v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_resolution_kind::omitted);
}
bool known(attribute_issue_kind v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_issue_kind::conflict);
}
bool known(attribute_issue_reason v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_issue_reason::unavailable_source_mapping);
}

bool valid_policy(const attribute_transfer_policy_contract &p) noexcept {
  boolean_product_options options;
  options.attributes = p;
  return validate_product_options(options).has_value();
}

void encode_digest(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}
void encode_raw_ref(canonical_encoder &e, const attribute_raw_entity_ref &r) {
  e.id(r.operand);
  e.byte(static_cast<std::uint8_t>(r.kind));
  e.u64(r.primary);
  e.u64(r.secondary);
}
void encode_policy(canonical_encoder &e,
                   const attribute_transfer_policy_contract &p) {
  e.u16(p.schema);
  e.byte(static_cast<std::uint8_t>(p.mode));
  e.byte(static_cast<std::uint8_t>(p.conflicts));
  e.byte(static_cast<std::uint8_t>(p.body_ids));
  e.byte(static_cast<std::uint8_t>(p.shell_ids));
  e.byte(static_cast<std::uint8_t>(p.facet_ids));
  e.byte(static_cast<std::uint8_t>(p.materials));
  e.byte(static_cast<std::uint8_t>(p.face_metadata));
  e.byte(static_cast<std::uint8_t>(p.vertex_normals));
  e.byte(static_cast<std::uint8_t>(p.vertex_colours));
  e.byte(static_cast<std::uint8_t>(p.interpolation));
  e.byte(static_cast<std::uint8_t>(p.sharp_edges));
  e.byte(static_cast<std::uint8_t>(p.texture_seams));
  e.byte(static_cast<std::uint8_t>(p.opaque_channels));
  e.byte(static_cast<std::uint8_t>(p.construction_provenance));
  e.boolean(p.report_absent_supported_channels);
}
std::vector<std::uint8_t> record_bytes(const std::array<char, 8> &tag,
                                       std::uint16_t schema,
                                       const std::vector<std::uint8_t> &payload) {
  canonical_encoder e;
  e.raw(reinterpret_cast<const std::uint8_t *>(tag.data()), tag.size());
  e.u16(schema);
  e.u64(payload.size());
  e.raw(payload.data(), payload.size());
  return e.bytes();
}

class reader {
  const std::vector<std::uint8_t> &bytes_;
  const attribute_decode_limits &limits_;
  std::size_t at_ = 0;
  std::uint64_t references_ = 0;
  void require(std::size_t n) {
    if (n > bytes_.size() - at_)
      throw std::runtime_error("truncated");
  }
public:
  reader(const std::vector<std::uint8_t> &b,
         const attribute_decode_limits &l) : bytes_(b), limits_(l) {}
  std::size_t remaining() const noexcept { return bytes_.size() - at_; }
  std::uint8_t byte() { require(1); return bytes_[at_++]; }
  bool boolean() { const auto v=byte(); if(v>1) throw std::runtime_error("bool"); return v!=0; }
  std::uint16_t u16() { require(2); auto v=(std::uint16_t(bytes_[at_])<<8)|bytes_[at_+1]; at_+=2; return v; }
  std::uint32_t u32() { require(4); std::uint32_t v=0; for(int i=0;i<4;++i)v=(v<<8)|byte(); return v; }
  std::uint64_t u64() { require(8); std::uint64_t v=0; for(int i=0;i<8;++i)v=(v<<8)|byte(); return v; }
  std::uint64_t count(std::uint64_t limit, bool reference=false) {
    const auto n=u64(); if(n>limit) throw std::runtime_error("count_limit");
    if(reference){ if(n>limits_.max_references-references_) throw std::runtime_error("reference_limit"); references_+=n; }
    return n;
  }
  digest digest_value() { require(16); digest d; std::copy(bytes_.begin()+static_cast<std::ptrdiff_t>(at_), bytes_.begin()+static_cast<std::ptrdiff_t>(at_+16), d.bytes.begin()); at_+=16; return d; }
  std::string string() { const auto n=count(limits_.max_string_bytes); if(n>remaining()) throw std::runtime_error("string"); std::string s(reinterpret_cast<const char*>(bytes_.data()+at_),static_cast<std::size_t>(n)); at_+=static_cast<std::size_t>(n); return s; }
  std::vector<std::uint8_t> byte_string() { const auto n=count(limits_.max_value_bytes); if(n>remaining()) throw std::runtime_error("bytes"); std::vector<std::uint8_t> v(bytes_.begin()+static_cast<std::ptrdiff_t>(at_),bytes_.begin()+static_cast<std::ptrdiff_t>(at_+n)); at_+=static_cast<std::size_t>(n); return v; }
};

template<class E> E read_enum(reader &r, bool(*f)(E) noexcept) {
  const auto v=static_cast<E>(r.byte()); if(!f(v)) throw std::runtime_error("enum"); return v;
}
attribute_raw_entity_ref read_raw_ref(reader &r) {
  attribute_raw_entity_ref out;
  const auto op=r.u64(); if(op>1) throw std::runtime_error("operand");
  out.operand=operand_id::from_canonical_value(op);
  out.kind=read_enum<attribute_source_entity_kind>(r,known);
  out.primary=r.u64(); out.secondary=r.u64(); return out;
}

void canonicalize_source_input(source_attribute_input &input) {
  std::sort(input.values.begin(),input.values.end(),[](const auto&a,const auto&b){
    return std::tie(a.entity,a.channel,a.name,a.value)<std::tie(b.entity,b.channel,b.name,b.value);
  });
  input.values.erase(std::unique(input.values.begin(),input.values.end(),[](const auto&a,const auto&b){return a.entity==b.entity&&a.channel==b.channel&&a.name==b.name&&a.value==b.value;}),input.values.end());
}
void encode_source_input_payload(canonical_encoder&e,const source_attribute_input&i){
  e.u16(i.schema);e.id(i.operand);e.string(i.body_id);e.u64(i.values.size());
  for(const auto&v:i.values){encode_raw_ref(e,v.entity);e.byte(static_cast<std::uint8_t>(v.channel));e.string(v.name);e.byte_string(v.value);}
}

std::vector<std::uint8_t> source_value_bytes(attribute_channel_kind channel,
                                             const std::string &name,
                                             const std::vector<std::uint8_t> &value) {
  canonical_encoder e; e.byte(static_cast<std::uint8_t>(channel));e.string(name);e.byte_string(value);return e.bytes();
}

template<class T> std::vector<std::uint8_t> point_bytes(const vec3<T>&v){
  canonical_encoder e;e.floating(v.x);e.floating(v.y);e.floating(v.z);return e.bytes();
}
std::vector<std::uint8_t> u32_bytes(std::uint32_t v){canonical_encoder e;e.u32(v);return e.bytes();}

template <class Mesh>
std::vector<std::array<std::uint64_t,2>> raw_edges_from_faces(const Mesh &mesh) {
  std::set<std::array<std::uint64_t,2>> edges;
  for(const auto&face:mesh.faces) for(std::size_t i=0;i<face.size();++i){
    auto a=static_cast<std::uint64_t>(face[i]);auto b=static_cast<std::uint64_t>(face[(i+1)%face.size()]);
    edges.insert({std::min(a,b),std::max(a,b)});
  }
  return {edges.begin(),edges.end()};
}

template<class Range,class Pred> std::uint64_t nth_id(const Range&r,std::uint64_t n,Pred pred){
  std::uint64_t seen=0;for(const auto&x:r)if(pred(x)){if(seen++==n)return x.id.value_for_debug();}return attribute_unmapped_id;
}

std::uint64_t mapped_ordinal(const normalization_mapping *mapping,
                             std::uint64_t source) {
  if(!mapping || mapping->status!=normalization_map_status::total) return source;
  if(source>=mapping->source_to_prepared.size()) return normalization_removed_ordinal;
  return mapping->source_to_prepared[static_cast<std::size_t>(source)];
}

template<class T,class I>
product_status_or<attribute_source_catalog> build_catalog(
    boolean_context<T,I>&context,const validated_operands<T,I>&validated,
    operand_id operand,const source_attribute_input*explicit_input){
  try{
    const auto role=static_cast<std::size_t>(operand.value_for_debug());
    const auto&source=context.attribute_source_mesh(operand);
    const auto*normalization=context.normalization_report_for(operand);
    attribute_source_catalog out;out.operand=operand;
    out.body_id=explicit_input&&!explicit_input->body_id.empty()?explicit_input->body_id:(role==0?"operand-A":"operand-B");
    auto add=[&](attribute_source_entity_kind kind,std::uint64_t primary,std::uint64_t secondary,std::uint64_t prepared){
      attribute_source_entity_record e;e.source={operand,kind,primary,secondary};e.prepared_id=prepared;e.retained=prepared!=attribute_unmapped_id&&prepared!=normalization_removed_ordinal;out.entities.push_back(e);
    };
    add(attribute_source_entity_kind::body,0,0,0);
    const normalization_mapping*vm=normalization?&normalization->vertices:nullptr;
    for(std::uint64_t raw=0;raw<source.vertices.size();++raw){
      const auto prepared_raw=mapped_ordinal(vm,raw);std::uint64_t prepared=attribute_unmapped_id;
      if(prepared_raw!=normalization_removed_ordinal&&prepared_raw<validated.raw_vertex_provenance[role].size())prepared=validated.raw_vertex_provenance[role][static_cast<std::size_t>(prepared_raw)];
      add(attribute_source_entity_kind::vertex,raw,0,prepared);
    }
    const normalization_mapping*fm=normalization?&normalization->facets:nullptr;
    for(std::uint64_t raw=0;raw<source.faces.size();++raw){
      const auto prepared_raw=mapped_ordinal(fm,raw);std::uint64_t prepared=attribute_unmapped_id;
      if(prepared_raw!=normalization_removed_ordinal&&prepared_raw<validated.raw_facets[role].size())prepared=validated.raw_facets[role][static_cast<std::size_t>(prepared_raw)].value_for_debug();
      add(attribute_source_entity_kind::facet,raw,0,prepared);
    }
    auto raw_edges=normalization&&!normalization->source_edges.empty()?normalization->source_edges:raw_edges_from_faces(source);
    const normalization_mapping*em=normalization?&normalization->edges:nullptr;
    for(std::uint64_t raw=0;raw<raw_edges.size();++raw){
      const auto prepared_raw=mapped_ordinal(em,raw);const auto prepared=prepared_raw==normalization_removed_ordinal?attribute_unmapped_id:nth_id(validated.edges,prepared_raw,[&](const auto&e){return e.operand==operand;});
      add(attribute_source_entity_kind::edge,raw_edges[raw][0],raw_edges[raw][1],prepared);
    }
    const normalization_mapping*sm=normalization?&normalization->shells:nullptr;
    const auto shell_count=sm&&sm->status==normalization_map_status::total?sm->source_to_prepared.size():validated.operands[role].shells.size();
    for(std::uint64_t raw=0;raw<shell_count;++raw){
      const auto prepared_raw=mapped_ordinal(sm,raw);const auto prepared=prepared_raw==normalization_removed_ordinal?attribute_unmapped_id:nth_id(validated.shells,prepared_raw,[&](const auto&s){return s.operand==operand;});
      add(attribute_source_entity_kind::shell,raw,0,prepared);
    }
    std::sort(out.entities.begin(),out.entities.end(),[](const auto&a,const auto&b){return a.source<b.source;});
    auto find_entity=[&](const attribute_raw_entity_ref&r)->std::optional<std::uint64_t>{auto it=std::lower_bound(out.entities.begin(),out.entities.end(),r,[](const auto&e,const auto&key){return e.source<key;});if(it==out.entities.end()||!(it->source==r))return{};return static_cast<std::uint64_t>(it-out.entities.begin());};
    auto add_value=[&](attribute_raw_entity_ref ref,attribute_channel_kind channel,std::string name,std::vector<std::uint8_t> value)->product_status_or<bool>{
      auto entity=find_entity(ref);if(!entity)return attr_error(product_error_code::input_contract_error,"attribute_source_input.entity");
      attribute_source_value_record record;record.source_entity=*entity;record.channel=channel;record.name=std::move(name);record.value=std::move(value);record.value_digest=domain_digest(value_digest_tag,source_value_bytes(record.channel,record.name,record.value));out.values.push_back(std::move(record));return true;};
    if(explicit_input){
      if(explicit_input->operand!=operand)return attr_error(product_error_code::input_contract_error,"attribute_source_input.operand");
      for(const auto&v:explicit_input->values){auto ok=add_value(v.entity,v.channel,v.name,v.value);if(!ok.has_value())return ok.error();}
    }else{
      for(std::size_t i=0;i<source.vertex_normals.size()&&i<source.vertices.size();++i){auto ok=add_value({operand,attribute_source_entity_kind::vertex,i,0},attribute_channel_kind::vertex_normal,"normal",point_bytes(source.vertex_normals[i]));if(!ok.has_value())return ok.error();}
      for(std::size_t i=0;i<source.vertex_colours.size()&&i<source.vertices.size();++i){auto ok=add_value({operand,attribute_source_entity_kind::vertex,i,0},attribute_channel_kind::vertex_colour,"colour",u32_bytes(source.vertex_colours[i]));if(!ok.has_value())return ok.error();}
      for(const auto&entry:source.metadata){auto ok=add_value({operand,attribute_source_entity_kind::body,0,0},attribute_channel_kind::opaque,entry.first,{entry.second.begin(),entry.second.end()});if(!ok.has_value())return ok.error();}
    }
    std::sort(out.values.begin(),out.values.end(),[](const auto&a,const auto&b){return std::tie(a.source_entity,a.channel,a.name,a.value)<std::tie(b.source_entity,b.channel,b.name,b.value);});
    out.values.erase(std::unique(out.values.begin(),out.values.end(),[](const auto&a,const auto&b){return a.source_entity==b.source_entity&&a.channel==b.channel&&a.name==b.name&&a.value==b.value;}),out.values.end());
    canonical_encoder c;c.u16(out.schema);c.id(out.operand);c.string(out.body_id);c.u64(out.entities.size());for(const auto&e:out.entities){encode_raw_ref(c,e.source);c.u64(e.prepared_id);c.boolean(e.retained);}c.u64(out.values.size());for(const auto&v:out.values){c.u64(v.source_entity);c.byte(static_cast<std::uint8_t>(v.channel));c.string(v.name);c.byte_string(v.value);encode_digest(c,v.value_digest);}out.catalog_digest=domain_digest(catalog_digest_tag,c.bytes());return out;
  }catch(const std::bad_alloc&){return attr_error(product_error_code::resource_limit,"attribute_catalog.allocation");}catch(...){return attr_error(product_error_code::internal_invariant_error,"attribute_catalog.exception");}
}

std::uint64_t global_entity_index(const std::array<attribute_source_catalog,2>&s,std::size_t role,std::uint64_t local){return role==0?local:static_cast<std::uint64_t>(s[0].entities.size())+local;}
std::uint64_t global_value_index(const std::array<attribute_source_catalog,2>&s,std::size_t role,std::uint64_t local){return role==0?local:static_cast<std::uint64_t>(s[0].values.size())+local;}
std::pair<std::size_t,std::uint64_t> local_entity_index(const std::array<attribute_source_catalog,2>&s,std::uint64_t global){if(global<s[0].entities.size())return{0,global};return{1,global-s[0].entities.size()};}
std::pair<std::size_t,std::uint64_t> local_value_index(const std::array<attribute_source_catalog,2>&s,std::uint64_t global){if(global<s[0].values.size())return{0,global};return{1,global-s[0].values.size()};}
const attribute_source_value_record& value_at(const std::array<attribute_source_catalog,2>&s,std::uint64_t global){auto p=local_value_index(s,global);return s[p.first].values.at(static_cast<std::size_t>(p.second));}

std::vector<std::uint64_t> entities_for_prepared(const std::array<attribute_source_catalog,2>&s,operand_id op,attribute_source_entity_kind kind,std::uint64_t prepared){
  const auto role=static_cast<std::size_t>(op.value_for_debug());std::vector<std::uint64_t> out;if(role>1)return out;for(std::size_t i=0;i<s[role].entities.size();++i){const auto&e=s[role].entities[i];if(e.source.kind==kind&&e.retained&&e.prepared_id==prepared)out.push_back(global_entity_index(s,role,i));}return out;
}
std::vector<std::uint64_t> values_for_entities(const std::array<attribute_source_catalog,2>&s,const std::vector<std::uint64_t>&entities,attribute_channel_kind channel,const std::string&name){
  std::set<std::uint64_t>wanted(entities.begin(),entities.end());std::vector<std::uint64_t>out;for(std::size_t r=0;r<2;++r)for(std::size_t i=0;i<s[r].values.size();++i){const auto&v=s[r].values[i];if(v.channel==channel&&v.name==name&&wanted.count(global_entity_index(s,r,v.source_entity)))out.push_back(global_value_index(s,r,i));}return out;
}
std::set<std::string> names_for_channel(const std::array<attribute_source_catalog,2>&s,const std::vector<std::uint64_t>&entities,attribute_channel_kind channel){std::set<std::uint64_t>wanted(entities.begin(),entities.end());std::set<std::string>out;for(std::size_t r=0;r<2;++r)for(const auto&v:s[r].values)if(v.channel==channel&&wanted.count(global_entity_index(s,r,v.source_entity)))out.insert(v.name);return out;}

std::vector<std::uint8_t> source_set_value(std::vector<std::uint64_t> entities,const std::array<attribute_source_catalog,2>&sources){
  std::sort(entities.begin(),entities.end());entities.erase(std::unique(entities.begin(),entities.end()),entities.end());canonical_encoder e;e.u64(entities.size());for(auto global:entities){auto p=local_entity_index(sources,global);const auto&r=sources[p.first].entities.at(static_cast<std::size_t>(p.second)).source;encode_raw_ref(e,r);}return e.bytes();
}

struct report_builder{
  attribute_transfer_report report;
  void issue(attribute_issue_kind kind,attribute_issue_reason reason,attribute_target_kind target,std::uint64_t target_id,attribute_channel_kind channel,std::string name,std::vector<std::uint64_t>entities,std::vector<std::uint64_t>values){attribute_issue_record x;x.kind=kind;x.reason=reason;x.target=target;x.target_id=target_id;x.channel=channel;x.name=std::move(name);std::sort(entities.begin(),entities.end());entities.erase(std::unique(entities.begin(),entities.end()),entities.end());std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());x.source_entities=std::move(entities);x.source_values=std::move(values);report.issues.push_back(std::move(x));if(kind==attribute_issue_kind::omission)++report.omissions;else++report.conflicts;}
  std::optional<std::uint64_t> transfer(attribute_target_kind target,std::uint64_t target_id,attribute_channel_kind channel,const std::string&name,const std::vector<std::uint64_t>&entities,std::vector<std::uint64_t>values,attribute_merge_policy policy,bool constructed){
    std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());if(report.policy.mode==attribute_transfer_mode::omit_all_with_report||policy==attribute_merge_policy::omit_with_report){issue(attribute_issue_kind::omission,attribute_issue_reason::policy_omits_channel,target,target_id,channel,name,entities,values);return{};}if(values.empty()){issue(attribute_issue_kind::omission,constructed?attribute_issue_reason::constructed_entity_has_no_source_value:attribute_issue_reason::absent_source_value,target,target_id,channel,name,entities,values);return{};}
    std::vector<std::uint64_t>unique=values;std::sort(unique.begin(),unique.end(),[&](auto a,auto b){return value_at(report.sources,a).value<value_at(report.sources,b).value;});unique.erase(std::unique(unique.begin(),unique.end(),[&](auto a,auto b){return value_at(report.sources,a).value==value_at(report.sources,b).value;}),unique.end());attribute_transfer_record record;record.target=target;record.target_id=target_id;record.channel=channel;record.name=name;record.source_values=values;
    if(unique.size()==1){record.value=value_at(report.sources,unique.front()).value;record.resolution=constructed&&values.size()==1?attribute_resolution_kind::split_copy:values.size()==1?attribute_resolution_kind::copied:attribute_resolution_kind::merged_equal;}
    else if(policy==attribute_merge_policy::choose_representative){auto chosen=*std::min_element(values.begin(),values.end());record.value=value_at(report.sources,chosen).value;record.resolution=attribute_resolution_kind::representative_copy;}
    else if(policy==attribute_merge_policy::deterministic_set_union){canonical_encoder e;e.u64(unique.size());for(auto v:unique)e.byte_string(value_at(report.sources,v).value);record.value=e.bytes();record.resolution=attribute_resolution_kind::set_union;}
    else{issue(attribute_issue_kind::conflict,channel==attribute_channel_kind::texture_seam?attribute_issue_reason::texture_seam_mismatch:attribute_issue_reason::unequal_source_values,target,target_id,channel,name,entities,values);issue(attribute_issue_kind::omission,attribute_issue_reason::unequal_source_values,target,target_id,channel,name,entities,values);return{};}
    record.value_digest=domain_digest(value_digest_tag,source_value_bytes(record.channel,record.name,record.value));auto index=report.transfers.size();report.transfers.push_back(std::move(record));return index;
  }
};

void add_transfer(attribute_transfer_report&r,attribute_target_kind target,std::uint64_t id,std::optional<std::uint64_t>x){if(!x)return;auto it=std::find_if(r.exact_mappings.begin(),r.exact_mappings.end(),[&](const auto&m){return m.target==target&&m.target_id==id;});if(it!=r.exact_mappings.end())it->transfers.push_back(*x);}

void add_identifier(report_builder&b,attribute_target_kind target,std::uint64_t id,attribute_channel_kind channel,attribute_identifier_policy policy,std::vector<std::uint64_t>entities){
  std::sort(entities.begin(),entities.end());entities.erase(std::unique(entities.begin(),entities.end()),entities.end());if(b.report.policy.mode==attribute_transfer_mode::omit_all_with_report||policy==attribute_identifier_policy::omit_with_report){b.issue(attribute_issue_kind::omission,attribute_issue_reason::policy_omits_channel,target,id,channel,{},entities,{});return;}attribute_transfer_record r;r.target=target;r.target_id=id;r.channel=channel;r.resolution=entities.size()<=1?attribute_resolution_kind::copied:attribute_resolution_kind::set_union;r.value=source_set_value(entities,b.report.sources);r.value_digest=domain_digest(value_digest_tag,source_value_bytes(r.channel,r.name,r.value));const auto i=b.report.transfers.size();b.report.transfers.push_back(std::move(r));add_transfer(b.report,target,id,i);
}

void encode_catalog(canonical_encoder&e,const attribute_source_catalog&c){e.u16(c.schema);e.id(c.operand);e.string(c.body_id);e.u64(c.entities.size());for(const auto&x:c.entities){encode_raw_ref(e,x.source);e.u64(x.prepared_id);e.boolean(x.retained);}e.u64(c.values.size());for(const auto&v:c.values){e.u64(v.source_entity);e.byte(static_cast<std::uint8_t>(v.channel));e.string(v.name);e.byte_string(v.value);encode_digest(e,v.value_digest);}encode_digest(e,c.catalog_digest);}
void encode_report_payload(canonical_encoder&e,const attribute_transfer_report&r){e.u16(r.schema);e.u16(r.checker_version);encode_policy(e,r.policy);encode_digest(e,r.policy_digest);encode_digest(e,r.exact_result_digest);e.boolean(r.output_digest.has_value());if(r.output_digest)encode_digest(e,*r.output_digest);for(const auto&s:r.sources)encode_catalog(e,s);e.u64(r.exact_mappings.size());for(const auto&m:r.exact_mappings){e.byte(static_cast<std::uint8_t>(m.target));e.u64(m.target_id);e.u64(m.source_entities.size());for(auto x:m.source_entities)e.u64(x);e.u64(m.transfers.size());for(auto x:m.transfers)e.u64(x);encode_digest(e,m.provenance_digest);}e.u64(r.output_mappings.size());for(const auto&m:r.output_mappings){e.byte(static_cast<std::uint8_t>(m.target));e.u64(m.output_id);e.byte(static_cast<std::uint8_t>(m.exact_target));e.u64(m.exact_id);e.u64(m.transfers.size());for(auto x:m.transfers)e.u64(x);}e.u64(r.transfers.size());for(const auto&t:r.transfers){e.byte(static_cast<std::uint8_t>(t.target));e.u64(t.target_id);e.byte(static_cast<std::uint8_t>(t.channel));e.string(t.name);e.byte(static_cast<std::uint8_t>(t.resolution));e.u64(t.source_values.size());for(auto x:t.source_values)e.u64(x);e.byte_string(t.value);encode_digest(e,t.value_digest);}e.u64(r.issues.size());for(const auto&i:r.issues){e.byte(static_cast<std::uint8_t>(i.kind));e.byte(static_cast<std::uint8_t>(i.reason));e.byte(static_cast<std::uint8_t>(i.target));e.u64(i.target_id);e.byte(static_cast<std::uint8_t>(i.channel));e.string(i.name);e.u64(i.source_entities.size());for(auto x:i.source_entities)e.u64(x);e.u64(i.source_values.size());for(auto x:i.source_values)e.u64(x);}e.u64(r.omissions);e.u64(r.conflicts);}

void finalize_report(attribute_transfer_report&r){
  for(auto&m:r.exact_mappings){std::sort(m.source_entities.begin(),m.source_entities.end());m.source_entities.erase(std::unique(m.source_entities.begin(),m.source_entities.end()),m.source_entities.end());m.transfers.clear();}
  std::sort(r.exact_mappings.begin(),r.exact_mappings.end(),[](const auto&a,const auto&b){return std::tie(a.target,a.target_id)<std::tie(b.target,b.target_id);});
  std::map<std::tuple<std::uint64_t,attribute_channel_kind,std::string>,std::set<std::pair<attribute_target_kind,std::uint64_t>>>uses;for(const auto&t:r.transfers)if(t.resolution==attribute_resolution_kind::copied&&t.source_values.size()==1)uses[{t.source_values.front(),t.channel,t.name}].insert({t.target,t.target_id});for(auto&t:r.transfers)if(t.resolution==attribute_resolution_kind::copied&&t.source_values.size()==1&&uses[{t.source_values.front(),t.channel,t.name}].size()>1)t.resolution=attribute_resolution_kind::split_copy;
  std::sort(r.transfers.begin(),r.transfers.end(),[](const auto&a,const auto&b){return std::tie(a.target,a.target_id,a.channel,a.name,a.value,a.source_values)<std::tie(b.target,b.target_id,b.channel,b.name,b.value,b.source_values);});for(std::size_t i=0;i<r.transfers.size();++i)add_transfer(r,r.transfers[i].target,r.transfers[i].target_id,i);
  std::sort(r.output_mappings.begin(),r.output_mappings.end(),[](const auto&a,const auto&b){return std::tie(a.target,a.output_id,a.exact_target,a.exact_id)<std::tie(b.target,b.output_id,b.exact_target,b.exact_id);});for(auto&m:r.output_mappings){auto it=std::find_if(r.exact_mappings.begin(),r.exact_mappings.end(),[&](const auto&x){return x.target==m.exact_target&&x.target_id==m.exact_id;});m.transfers=it==r.exact_mappings.end()?std::vector<std::uint64_t>{}:it->transfers;}
  std::sort(r.issues.begin(),r.issues.end(),[](const auto&a,const auto&b){return std::tie(a.kind,a.reason,a.target,a.target_id,a.channel,a.name,a.source_entities,a.source_values)<std::tie(b.kind,b.reason,b.target,b.target_id,b.channel,b.name,b.source_entities,b.source_values);});canonical_encoder payload;encode_report_payload(payload,r);r.canonical_bytes=record_bytes(report_tag,attribute_transfer_report_schema,payload.bytes());r.report_digest=domain_digest(report_digest_tag,r.canonical_bytes);
}

} // namespace

template<class T,class I>
product_status_or<std::array<attribute_source_catalog,2>> make_attribute_source_catalogs(boolean_context<T,I>&context,const std::array<source_attribute_input,2>*inputs){auto validated=validate_operands(context);if(!validated.has_value())return make_product_error(promote_error_code(validated.error().code),validated.error().message_key);std::array<attribute_source_catalog,2>out;for(std::size_t r=0;r<2;++r){auto c=build_catalog(context,*validated.value()->payload,operand_id::from_canonical_value(r),inputs?&(*inputs)[r]:nullptr);if(!c.has_value())return c.error();out[r]=std::move(c.value());}return out;}

product_status_or<std::vector<std::uint8_t>> encode_source_attribute_input(const source_attribute_input&input){try{if(input.schema!=attribute_source_input_schema||input.operand.value_for_debug()>1)return attr_error(product_error_code::input_contract_error,"attribute_source_input.schema");for(const auto&v:input.values)if(v.entity.operand!=input.operand||!known(v.entity.kind)||!known(v.channel))return attr_error(product_error_code::input_contract_error,"attribute_source_input.value");auto c=input;canonicalize_source_input(c);canonical_encoder p;encode_source_input_payload(p,c);return record_bytes(source_input_tag,attribute_source_input_schema,p.bytes());}catch(const std::bad_alloc&){return attr_error(product_error_code::resource_limit,"attribute_source_input.allocation");}catch(...){return attr_error(product_error_code::internal_invariant_error,"attribute_source_input.exception");}}

product_status_or<source_attribute_input> decode_source_attribute_input(const std::vector<std::uint8_t>&bytes,const attribute_decode_limits&limits){try{if(bytes.size()>limits.max_record_bytes)return attr_error(product_error_code::resource_limit,"attribute_source_input.record_limit");reader o(bytes,limits);for(char c:source_input_tag)if(o.byte()!=static_cast<std::uint8_t>(c))throw std::runtime_error("tag");if(o.u16()!=attribute_source_input_schema)return attr_error(product_error_code::stale_binding,"attribute_source_input.schema");auto n=o.u64();if(n!=o.remaining())throw std::runtime_error("payload");std::vector<std::uint8_t>payload(bytes.end()-static_cast<std::ptrdiff_t>(n),bytes.end());reader r(payload,limits);source_attribute_input out;out.schema=r.u16();auto op=r.u64();if(out.schema!=attribute_source_input_schema||op>1)throw std::runtime_error("schema");out.operand=operand_id::from_canonical_value(op);out.body_id=r.string();auto count=r.count(limits.max_values);out.values.reserve(static_cast<std::size_t>(count));for(std::uint64_t i=0;i<count;++i){source_attribute_value_input v;v.entity=read_raw_ref(r);v.channel=read_enum<attribute_channel_kind>(r,known);v.name=r.string();v.value=r.byte_string();if(v.entity.operand!=out.operand)throw std::runtime_error("operand");out.values.push_back(std::move(v));}if(r.remaining())throw std::runtime_error("trailing");auto encoded=encode_source_attribute_input(out);if(!encoded.has_value()||encoded.value()!=bytes)throw std::runtime_error("noncanonical");return out;}catch(const std::bad_alloc&){return attr_error(product_error_code::resource_limit,"attribute_source_input.allocation");}catch(const std::exception&x){auto e=attr_error(product_error_code::input_contract_error,"attribute_source_input.decode");e.detail=x.what();return e;}}

product_status_or<attribute_transfer_report> make_attribute_transfer_report(const exact_result_handle&exact,const attribute_transfer_policy_contract&policy,std::array<attribute_source_catalog,2>sources,const attribute_output_binding*output){
  try{if(!exact.valid()||!valid_policy(policy))return attr_error(product_error_code::input_contract_error,"attribute_report.policy");auto boundary=read_exact_result(exact);if(!boundary.has_value())return boundary.error();report_builder b;b.report.policy=policy;b.report.policy_digest=attribute_transfer_policy_digest(policy);b.report.exact_result_digest=exact->canonical_digest;b.report.sources=std::move(sources);
    for(const auto&v:boundary.value()->vertices){attribute_exact_entity_mapping m;m.target=attribute_target_kind::exact_vertex;m.target_id=v.id.value_for_debug();canonical_encoder p;p.u64(m.target_id);for(const auto&s:v.original_vertices){auto e=entities_for_prepared(b.report.sources,s.operand,attribute_source_entity_kind::vertex,s.vertex.value_for_debug());m.source_entities.insert(m.source_entities.end(),e.begin(),e.end());p.id(s.operand);p.id(s.vertex);}for(auto c:v.constructions)p.id(c);m.provenance_digest=domain_digest({{'Y','G','B','A','V','P','R','1'}},p.bytes());b.report.exact_mappings.push_back(std::move(m));}
    for(std::size_t i=0;i<boundary.value()->edges.size();++i){const auto&e=boundary.value()->edges[i];attribute_exact_entity_mapping m;m.target=attribute_target_kind::exact_edge;m.target_id=e.id.value_for_debug();canonical_encoder p;p.u64(m.target_id);if(i<boundary.value()->edge_geometry.size())for(const auto&s:boundary.value()->edge_geometry[i].contributors){auto x=entities_for_prepared(b.report.sources,s.operand,attribute_source_entity_kind::edge,s.edge.value_for_debug());m.source_entities.insert(m.source_entities.end(),x.begin(),x.end());p.id(s.operand);p.id(s.shell);p.id(s.edge);for(auto f:s.facets)p.id(f);}m.provenance_digest=domain_digest({{'Y','G','B','A','E','P','R','1'}},p.bytes());b.report.exact_mappings.push_back(std::move(m));}
    for(const auto&pmap:boundary.value()->provenance){attribute_exact_entity_mapping m;m.target=attribute_target_kind::exact_patch;m.target_id=pmap.patch.value_for_debug();canonical_encoder p;p.u64(m.target_id);for(const auto&s:pmap.contributors){for(auto pair:{std::make_pair(attribute_source_entity_kind::body,std::uint64_t(0)),std::make_pair(attribute_source_entity_kind::shell,s.shell.value_for_debug()),std::make_pair(attribute_source_entity_kind::facet,s.facet.value_for_debug())}){auto x=entities_for_prepared(b.report.sources,s.operand,pair.first,pair.second);m.source_entities.insert(m.source_entities.end(),x.begin(),x.end());}p.id(s.operand);p.id(s.shell);p.id(s.facet);p.boolean(s.representative);}m.provenance_digest=domain_digest({{'Y','G','B','A','P','P','R','1'}},p.bytes());b.report.exact_mappings.push_back(std::move(m));}
    for(auto&mapping:b.report.exact_mappings){std::sort(mapping.source_entities.begin(),mapping.source_entities.end());mapping.source_entities.erase(std::unique(mapping.source_entities.begin(),mapping.source_entities.end()),mapping.source_entities.end());const bool constructed=mapping.source_entities.empty();
      if(mapping.target==attribute_target_kind::exact_patch){std::vector<std::uint64_t>body,shell,facet;for(auto x:mapping.source_entities){auto q=local_entity_index(b.report.sources,x);const auto k=b.report.sources[q.first].entities[q.second].source.kind;(k==attribute_source_entity_kind::body?body:k==attribute_source_entity_kind::shell?shell:facet).push_back(x);}add_identifier(b,mapping.target,mapping.target_id,attribute_channel_kind::source_body_id,policy.body_ids,body);add_identifier(b,mapping.target,mapping.target_id,attribute_channel_kind::source_shell_id,policy.shell_ids,shell);add_identifier(b,mapping.target,mapping.target_id,attribute_channel_kind::source_facet_id,policy.facet_ids,facet);for(auto channel:{attribute_channel_kind::material,attribute_channel_kind::face_metadata}){auto names=names_for_channel(b.report.sources,facet,channel);if(names.empty()&&policy.report_absent_supported_channels)b.issue(attribute_issue_kind::omission,facet.empty()?attribute_issue_reason::constructed_entity_has_no_source_value:attribute_issue_reason::absent_source_value,mapping.target,mapping.target_id,channel,{},facet,{});for(const auto&name:names){auto vals=values_for_entities(b.report.sources,facet,channel,name);auto t=b.transfer(mapping.target,mapping.target_id,channel,name,facet,vals,channel==attribute_channel_kind::material?policy.materials:policy.face_metadata,false);add_transfer(b.report,mapping.target,mapping.target_id,t);}}}
      if(mapping.target==attribute_target_kind::exact_vertex){for(auto channel:{attribute_channel_kind::vertex_normal,attribute_channel_kind::vertex_colour}){auto names=names_for_channel(b.report.sources,mapping.source_entities,channel);if(names.empty()&&policy.report_absent_supported_channels)b.issue(attribute_issue_kind::omission,constructed?attribute_issue_reason::constructed_entity_has_no_source_value:attribute_issue_reason::absent_source_value,mapping.target,mapping.target_id,channel,{},mapping.source_entities,{});for(const auto&name:names){auto vals=values_for_entities(b.report.sources,mapping.source_entities,channel,name);auto vp=channel==attribute_channel_kind::vertex_normal?policy.vertex_normals:policy.vertex_colours;attribute_merge_policy mp=vp==attribute_vertex_copy_policy::choose_representative?attribute_merge_policy::choose_representative:vp==attribute_vertex_copy_policy::omit_with_report?attribute_merge_policy::omit_with_report:attribute_merge_policy::require_equal;if(constructed&&vals.empty()&&policy.interpolation==attribute_interpolation_policy::prohibit_and_report){b.issue(attribute_issue_kind::omission,attribute_issue_reason::interpolation_prohibited,mapping.target,mapping.target_id,channel,name,mapping.source_entities,vals);continue;}auto t=b.transfer(mapping.target,mapping.target_id,channel,name,mapping.source_entities,vals,mp,constructed);add_transfer(b.report,mapping.target,mapping.target_id,t);}}auto seams=names_for_channel(b.report.sources,mapping.source_entities,attribute_channel_kind::texture_seam);if(seams.empty()&&policy.report_absent_supported_channels)b.issue(attribute_issue_kind::omission,constructed?attribute_issue_reason::constructed_entity_has_no_source_value:attribute_issue_reason::absent_source_value,mapping.target,mapping.target_id,attribute_channel_kind::texture_seam,{},mapping.source_entities,{});for(const auto&name:seams){auto vals=values_for_entities(b.report.sources,mapping.source_entities,attribute_channel_kind::texture_seam,name);auto mp=policy.texture_seams==attribute_texture_seam_policy::preserve_source_sets?attribute_merge_policy::deterministic_set_union:policy.texture_seams==attribute_texture_seam_policy::omit_with_report?attribute_merge_policy::omit_with_report:attribute_merge_policy::require_equal;auto t=b.transfer(mapping.target,mapping.target_id,attribute_channel_kind::texture_seam,name,mapping.source_entities,vals,mp,constructed);add_transfer(b.report,mapping.target,mapping.target_id,t);}}
      if(mapping.target==attribute_target_kind::exact_edge){auto names=names_for_channel(b.report.sources,mapping.source_entities,attribute_channel_kind::sharp_edge);if(names.empty()&&policy.report_absent_supported_channels)b.issue(attribute_issue_kind::omission,constructed?attribute_issue_reason::constructed_entity_has_no_source_value:attribute_issue_reason::absent_source_value,mapping.target,mapping.target_id,attribute_channel_kind::sharp_edge,{},mapping.source_entities,{});for(const auto&name:names){auto vals=values_for_entities(b.report.sources,mapping.source_entities,attribute_channel_kind::sharp_edge,name);if(policy.sharp_edges==attribute_sharp_edge_policy::any_source&&policy.mode!=attribute_transfer_mode::omit_all_with_report){bool any=false,valid=true;for(auto x:vals){const auto&v=value_at(b.report.sources,x).value;if(v.size()!=1||v[0]>1)valid=false;else any=any||v[0];}if(valid){attribute_transfer_record r;r.target=mapping.target;r.target_id=mapping.target_id;r.channel=attribute_channel_kind::sharp_edge;r.name=name;r.resolution=attribute_resolution_kind::any_source;r.source_values=vals;r.value={static_cast<std::uint8_t>(any)};r.value_digest=domain_digest(value_digest_tag,source_value_bytes(r.channel,r.name,r.value));auto idx=b.report.transfers.size();b.report.transfers.push_back(std::move(r));add_transfer(b.report,mapping.target,mapping.target_id,idx);}else b.issue(attribute_issue_kind::conflict,attribute_issue_reason::unequal_source_values,mapping.target,mapping.target_id,attribute_channel_kind::sharp_edge,name,mapping.source_entities,vals);}else{auto mp=policy.sharp_edges==attribute_sharp_edge_policy::omit_with_report?attribute_merge_policy::omit_with_report:attribute_merge_policy::require_equal;auto t=b.transfer(mapping.target,mapping.target_id,attribute_channel_kind::sharp_edge,name,mapping.source_entities,vals,mp,constructed);add_transfer(b.report,mapping.target,mapping.target_id,t);}}}
      auto opaque=names_for_channel(b.report.sources,mapping.source_entities,attribute_channel_kind::opaque);for(const auto&name:opaque){auto vals=values_for_entities(b.report.sources,mapping.source_entities,attribute_channel_kind::opaque,name);auto t=b.transfer(mapping.target,mapping.target_id,attribute_channel_kind::opaque,name,mapping.source_entities,vals,policy.opaque_channels,constructed);add_transfer(b.report,mapping.target,mapping.target_id,t);}
      if(policy.mode!=attribute_transfer_mode::omit_all_with_report&&policy.construction_provenance==attribute_construction_provenance_policy::compact_digest){attribute_transfer_record r;r.target=mapping.target;r.target_id=mapping.target_id;r.channel=attribute_channel_kind::construction_provenance;r.resolution=attribute_resolution_kind::compact_construction;r.value.assign(mapping.provenance_digest.bytes.begin(),mapping.provenance_digest.bytes.end());r.value_digest=domain_digest(value_digest_tag,source_value_bytes(r.channel,r.name,r.value));auto idx=b.report.transfers.size();b.report.transfers.push_back(std::move(r));add_transfer(b.report,mapping.target,mapping.target_id,idx);}else b.issue(attribute_issue_kind::omission,attribute_issue_reason::policy_omits_channel,mapping.target,mapping.target_id,attribute_channel_kind::construction_provenance,{},mapping.source_entities,{});
    }
    if(output){auto valid=validate_attribute_output_binding(*output,exact);if(!valid.has_value())return valid.error();b.report.output_digest=output->output_digest;for(std::size_t i=0;i<output->output_vertex_exact_vertices.size();++i)b.report.output_mappings.push_back({attribute_target_kind::output_vertex,i,attribute_target_kind::exact_vertex,output->output_vertex_exact_vertices[i],{}});for(std::size_t i=0;i<output->output_face_exact_patches.size();++i)b.report.output_mappings.push_back({attribute_target_kind::output_face,i,attribute_target_kind::exact_patch,output->output_face_exact_patches[i],{}});}
    std::set<std::uint64_t>used;for(const auto&m:b.report.exact_mappings)used.insert(m.source_entities.begin(),m.source_entities.end());for(std::size_t role=0;role<2;++role)for(std::size_t vi=0;vi<b.report.sources[role].values.size();++vi){auto globalv=global_value_index(b.report.sources,role,vi);const auto&v=b.report.sources[role].values[vi];auto ge=global_entity_index(b.report.sources,role,v.source_entity);if(!used.count(ge))b.issue(attribute_issue_kind::omission,attribute_issue_reason::removed_internal_entity,attribute_target_kind::exact_patch,attribute_unmapped_id,v.channel,v.name,{ge},{globalv});}
    if(policy.mode==attribute_transfer_mode::require_lossless&&(b.report.omissions||b.report.conflicts))return attr_error(product_error_code::attribute_transfer_conflict,"attribute_report.lossless");if(policy.conflicts==attribute_conflict_policy::reject&&b.report.conflicts)return attr_error(product_error_code::attribute_transfer_conflict,"attribute_report.conflict");finalize_report(b.report);auto verified=verify_attribute_transfer_report(b.report,exact,output);if(!verified.has_value())return verified.error();return b.report;
  }catch(const std::bad_alloc&){return attr_error(product_error_code::resource_limit,"attribute_report.allocation");}catch(...){return attr_error(product_error_code::internal_invariant_error,"attribute_report.exception");}
}

product_status_or<std::vector<std::uint8_t>> encode_attribute_transfer_report(const attribute_transfer_report&r){try{if(r.schema!=attribute_transfer_report_schema||r.checker_version!=attribute_transfer_checker_version||!valid_policy(r.policy))return attr_error(product_error_code::stale_binding,"attribute_report.schema");auto c=r;finalize_report(c);if((!r.canonical_bytes.empty()&&r.canonical_bytes!=c.canonical_bytes)||(!digest_zero(r.report_digest)&&r.report_digest!=c.report_digest))return attr_error(product_error_code::stale_binding,"attribute_report.canonical");return c.canonical_bytes;}catch(const std::bad_alloc&){return attr_error(product_error_code::resource_limit,"attribute_report.encode_allocation");}catch(...){return attr_error(product_error_code::internal_invariant_error,"attribute_report.encode_exception");}}

product_status_or<attribute_transfer_report> decode_attribute_transfer_report(const std::vector<std::uint8_t>&bytes,const attribute_decode_limits&limits){try{if(bytes.size()>limits.max_record_bytes)return attr_error(product_error_code::resource_limit,"attribute_report.record_limit");reader o(bytes,limits);for(char c:report_tag)if(o.byte()!=static_cast<std::uint8_t>(c))throw std::runtime_error("tag");if(o.u16()!=attribute_transfer_report_schema)return attr_error(product_error_code::stale_binding,"attribute_report.schema");auto n=o.u64();if(n!=o.remaining())throw std::runtime_error("payload");std::vector<std::uint8_t>payload(bytes.end()-static_cast<std::ptrdiff_t>(n),bytes.end());reader r(payload,limits);attribute_transfer_report out;out.schema=r.u16();out.checker_version=r.u16();if(out.schema!=attribute_transfer_report_schema||out.checker_version!=attribute_transfer_checker_version)throw std::runtime_error("schema");out.policy.schema=r.u16();out.policy.mode=static_cast<attribute_transfer_mode>(r.byte());out.policy.conflicts=static_cast<attribute_conflict_policy>(r.byte());out.policy.body_ids=static_cast<attribute_identifier_policy>(r.byte());out.policy.shell_ids=static_cast<attribute_identifier_policy>(r.byte());out.policy.facet_ids=static_cast<attribute_identifier_policy>(r.byte());out.policy.materials=static_cast<attribute_merge_policy>(r.byte());out.policy.face_metadata=static_cast<attribute_merge_policy>(r.byte());out.policy.vertex_normals=static_cast<attribute_vertex_copy_policy>(r.byte());out.policy.vertex_colours=static_cast<attribute_vertex_copy_policy>(r.byte());out.policy.interpolation=static_cast<attribute_interpolation_policy>(r.byte());out.policy.sharp_edges=static_cast<attribute_sharp_edge_policy>(r.byte());out.policy.texture_seams=static_cast<attribute_texture_seam_policy>(r.byte());out.policy.opaque_channels=static_cast<attribute_merge_policy>(r.byte());out.policy.construction_provenance=static_cast<attribute_construction_provenance_policy>(r.byte());out.policy.report_absent_supported_channels=r.boolean();if(!valid_policy(out.policy))throw std::runtime_error("policy");out.policy_digest=r.digest_value();out.exact_result_digest=r.digest_value();if(r.boolean())out.output_digest=r.digest_value();for(auto&c:out.sources){c.schema=r.u16();auto op=r.u64();if(op>1)throw std::runtime_error("operand");c.operand=operand_id::from_canonical_value(op);c.body_id=r.string();auto ne=r.count(limits.max_entities);c.entities.reserve(ne);for(std::uint64_t i=0;i<ne;++i){attribute_source_entity_record e;e.source=read_raw_ref(r);e.prepared_id=r.u64();e.retained=r.boolean();c.entities.push_back(std::move(e));}auto nv=r.count(limits.max_values);c.values.reserve(nv);for(std::uint64_t i=0;i<nv;++i){attribute_source_value_record v;v.source_entity=r.u64();v.channel=read_enum<attribute_channel_kind>(r,known);v.name=r.string();v.value=r.byte_string();v.value_digest=r.digest_value();c.values.push_back(std::move(v));}c.catalog_digest=r.digest_value();}
    auto nm=r.count(limits.max_mappings);out.exact_mappings.reserve(nm);for(std::uint64_t i=0;i<nm;++i){attribute_exact_entity_mapping m;m.target=read_enum<attribute_target_kind>(r,known);m.target_id=r.u64();auto ns=r.count(limits.max_references,true);m.source_entities.reserve(ns);for(std::uint64_t j=0;j<ns;++j)m.source_entities.push_back(r.u64());auto nt=r.count(limits.max_references,true);m.transfers.reserve(nt);for(std::uint64_t j=0;j<nt;++j)m.transfers.push_back(r.u64());m.provenance_digest=r.digest_value();out.exact_mappings.push_back(std::move(m));}
    nm=r.count(limits.max_mappings);out.output_mappings.reserve(nm);for(std::uint64_t i=0;i<nm;++i){attribute_output_entity_mapping m;m.target=read_enum<attribute_target_kind>(r,known);m.output_id=r.u64();m.exact_target=read_enum<attribute_target_kind>(r,known);m.exact_id=r.u64();auto nt=r.count(limits.max_references,true);m.transfers.reserve(nt);for(std::uint64_t j=0;j<nt;++j)m.transfers.push_back(r.u64());out.output_mappings.push_back(std::move(m));}
    auto nt=r.count(limits.max_transfers);out.transfers.reserve(nt);for(std::uint64_t i=0;i<nt;++i){attribute_transfer_record t;t.target=read_enum<attribute_target_kind>(r,known);t.target_id=r.u64();t.channel=read_enum<attribute_channel_kind>(r,known);t.name=r.string();t.resolution=read_enum<attribute_resolution_kind>(r,known);auto ns=r.count(limits.max_references,true);t.source_values.reserve(ns);for(std::uint64_t j=0;j<ns;++j)t.source_values.push_back(r.u64());t.value=r.byte_string();t.value_digest=r.digest_value();out.transfers.push_back(std::move(t));}
    auto ni=r.count(limits.max_issues);out.issues.reserve(ni);for(std::uint64_t i=0;i<ni;++i){attribute_issue_record x;x.kind=read_enum<attribute_issue_kind>(r,known);x.reason=read_enum<attribute_issue_reason>(r,known);x.target=read_enum<attribute_target_kind>(r,known);x.target_id=r.u64();x.channel=read_enum<attribute_channel_kind>(r,known);x.name=r.string();auto ne=r.count(limits.max_references,true);x.source_entities.reserve(ne);for(std::uint64_t j=0;j<ne;++j)x.source_entities.push_back(r.u64());auto nv=r.count(limits.max_references,true);x.source_values.reserve(nv);for(std::uint64_t j=0;j<nv;++j)x.source_values.push_back(r.u64());out.issues.push_back(std::move(x));}out.omissions=r.u64();out.conflicts=r.u64();if(r.remaining())throw std::runtime_error("trailing");out.canonical_bytes=bytes;out.report_digest=domain_digest(report_digest_tag,bytes);auto encoded=encode_attribute_transfer_report(out);if(!encoded.has_value()||encoded.value()!=bytes)throw std::runtime_error("noncanonical");return out;}catch(const std::bad_alloc&){return attr_error(product_error_code::resource_limit,"attribute_report.decode_allocation");}catch(const std::exception&x){auto e=attr_error(product_error_code::input_contract_error,"attribute_report.decode");e.detail=x.what();return e;}}

product_status_or<bool> validate_attribute_output_binding(const attribute_output_binding&b,const exact_result_handle&exact) noexcept {try{if(!exact.valid()||b.schema!=attribute_output_binding_schema||b.exact_result_digest!=exact->canonical_digest||digest_zero(b.output_digest))return attr_error(product_error_code::stale_binding,"attribute_output.binding");auto boundary=read_exact_result(exact);if(!boundary.has_value())return boundary.error();for(auto x:b.output_vertex_exact_vertices)if(x>=boundary.value()->vertices.size())return attr_error(product_error_code::stale_binding,"attribute_output.vertex");for(auto x:b.output_face_exact_patches)if(x>=boundary.value()->patches.size())return attr_error(product_error_code::stale_binding,"attribute_output.face");return true;}catch(...){return attr_error(product_error_code::internal_invariant_error,"attribute_output.exception");}}

product_status_or<bool> verify_serialized_attribute_transfer_report(const std::vector<std::uint8_t>&bytes,const exact_result_handle&exact,const attribute_output_binding*output,const attribute_decode_limits&limits) noexcept {auto decoded=decode_attribute_transfer_report(bytes,limits);if(!decoded.has_value())return decoded.error();return verify_attribute_transfer_report(decoded.value(),exact,output);}

#define YGOR_ATTRIBUTE_DEFINE(T,I) template product_status_or<std::array<attribute_source_catalog,2>> make_attribute_source_catalogs(boolean_context<T,I>&,const std::array<source_attribute_input,2>*);
YGOR_ATTRIBUTE_DEFINE(float,std::uint32_t)
YGOR_ATTRIBUTE_DEFINE(float,std::uint64_t)
YGOR_ATTRIBUTE_DEFINE(double,std::uint32_t)
YGOR_ATTRIBUTE_DEFINE(double,std::uint64_t)
#undef YGOR_ATTRIBUTE_DEFINE

} // namespace mesh_boolean
} // namespace ygor
