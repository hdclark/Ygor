#pragma once
#ifndef YGOR_MESHES_BOOLEAN_TRANSACTION_H_
#define YGOR_MESHES_BOOLEAN_TRANSACTION_H_
#include "YgorMeshesBooleanContract.h"
#include "YgorMeshesBooleanPerformance.h"
#include <algorithm>

namespace ygor { namespace mesh_boolean {
template<class Id,class Record>class published_store{
    context_owner_token owner_;boolean_stage stage_;digest digest_;std::vector<Record>records_;
public:published_store(context_owner_token o,boolean_stage s,digest d,std::vector<Record>r):owner_(o),stage_(s),digest_(d),records_(std::move(r)){}
    context_owner_token owner()const{return owner_;}boolean_stage producing_stage()const{return stage_;}const digest&artifact_digest()const{return digest_;}std::size_t size()const{return records_.size();}
    const Record&at(Id id)const{if(!id.valid()||id.value_for_debug()>=records_.size())throw std::out_of_range("store ID");return records_[id.value_for_debug()];}
    auto begin()const{return records_.cbegin();}auto end()const{return records_.cend();}
};

template<class Id,class Key,class Record>class canonical_id_factory{
public:
    struct provisional{Key key;Record record;};
    template<class Less,class Merge>static status_or<std::vector<std::pair<Id,Record>>> assign(std::vector<provisional>in,Less less,Merge merge){
        std::sort(in.begin(),in.end(),[&](const auto&a,const auto&b)noexcept{return less(a.key,b.key);});
        std::vector<std::pair<Id,Record>>out;
        for(std::size_t i=0;i<in.size();){std::size_t j=i+1;while(j<in.size()&&!less(in[i].key,in[j].key)&&!less(in[j].key,in[i].key))++j;
            auto merged=merge(in.begin()+i,in.begin()+j);if(!merged.has_value())return merged.error();
            if(out.size()>=Id::invalid_value)return make_error(boolean_error_code::resource_limit,boolean_stage::context_setup,"id_exhaustion");
            out.emplace_back(Id::from_canonical_value(static_cast<std::uint64_t>(out.size())),std::move(merged.value()));i=j;}
        return out;
    }
};

enum class transaction_state:std::uint8_t{open,verified,published,failed};
class publication_certificate{
    context_owner_token owner_;boolean_stage stage_;artifact_slot slot_;std::uint64_t type_=0;std::uint16_t schema_=0;digest artifact_,report_,invariants_;bool consumed_=false;
    publication_certificate(context_owner_token o,boolean_stage s,artifact_slot sl,std::uint64_t t,std::uint16_t sc,digest a,digest r,digest i):owner_(o),stage_(s),slot_(sl),type_(t),schema_(sc),artifact_(a),report_(r),invariants_(i){}
    template<class D,class A>friend class stage_transaction;
public:publication_certificate(const publication_certificate&)=delete;publication_certificate&operator=(const publication_certificate&)=delete;publication_certificate(publication_certificate&&)=default;
};
template<class Artifact>struct published_artifact{context_owner_token owner;boolean_stage stage;artifact_slot slot;digest artifact_digest;std::shared_ptr<const Artifact>payload;verification_report report;std::uint64_t generation=1;std::shared_ptr<const void>prior_generation;};
template<class Draft,class Artifact>class stage_transaction{
    context_owner_token owner_;boolean_stage stage_;artifact_slot slot_;performance_collector*performance_=nullptr;boolean_stage performance_verifier_stage_;transaction_state state_=transaction_state::open;std::unique_ptr<Draft>draft_;std::shared_ptr<const Artifact>candidate_;verification_report report_;std::unique_ptr<publication_certificate>certificate_;std::vector<resource_reservation>reservations_;
public:
    stage_transaction(context_owner_token o,boolean_stage s,artifact_slot slot,std::unique_ptr<Draft>d,performance_collector*p=nullptr):owner_(o),stage_(s),slot_(slot),performance_(p),performance_verifier_stage_(s),draft_(std::move(d)){}
    stage_transaction(context_owner_token o,boolean_stage s,artifact_slot slot,std::unique_ptr<Draft>d,performance_collector*p,boolean_stage verifier_stage):owner_(o),stage_(s),slot_(slot),performance_(p),performance_verifier_stage_(verifier_stage),draft_(std::move(d)){}
    stage_transaction(stage_transaction&&)=default;stage_transaction(const stage_transaction&)=delete;transaction_state state()const{return state_;}Draft&draft(){if(state_!=transaction_state::open)throw std::logic_error("transaction not open");return*draft_;}
    void stage_reservation(resource_reservation r){if(state_!=transaction_state::open)throw std::logic_error("transaction not open");reservations_.push_back(std::move(r));}
    status_or<bool>verify(std::shared_ptr<const Artifact>a,const artifact_view&v,const verification_spec&spec,const verification_environment_view&env,const verifier_service&service){if(state_!=transaction_state::open)return make_error(boolean_error_code::internal_invariant_error,stage_,"transaction_state");performance_scope timing(performance_,performance_verifier_stage_,performance_role::verifier);auto r=service.verify(v,spec,env);timing.finish();if(!r.has_value()||!r.value().passed()){state_=transaction_state::failed;return r.has_value()?make_error(boolean_error_code::internal_invariant_error,stage_,"verification_failed"):r.error();}if(r.value().owner!=owner_||r.value().stage!=stage_||r.value().slot!=slot_||r.value().artifact_digest!=v.artifact_digest||r.value().artifact_type_tag!=v.artifact_type_tag||r.value().artifact_schema!=v.artifact_schema||r.value().invariant_set_digest!=spec.invariant_set_digest){state_=transaction_state::failed;return make_error(boolean_error_code::internal_invariant_error,stage_,"verification_binding");}candidate_=std::move(a);report_=std::move(r.value());certificate_.reset(new publication_certificate(owner_,stage_,slot_,v.artifact_type_tag,v.artifact_schema,v.artifact_digest,report_.report_digest,spec.invariant_set_digest));state_=transaction_state::verified;return true;}
    status_or<std::shared_ptr<const published_artifact<Artifact>>>publish(){if(state_!=transaction_state::verified||!certificate_||certificate_->consumed_||certificate_->owner_!=owner_||certificate_->stage_!=stage_||certificate_->slot_!=slot_||certificate_->artifact_!=report_.artifact_digest||certificate_->report_!=report_.report_digest){state_=transaction_state::failed;return make_error(boolean_error_code::internal_invariant_error,stage_,"publish_unverified");}certificate_->consumed_=true;auto p=std::make_shared<published_artifact<Artifact>>(published_artifact<Artifact>{owner_,stage_,slot_,report_.artifact_digest,candidate_,report_,1,{}});for(auto&r:reservations_)r.commit();state_=transaction_state::published;draft_.reset();return std::shared_ptr<const published_artifact<Artifact>>(std::move(p));}
    status_or<std::shared_ptr<const published_artifact<Artifact>>>compare_and_publish(artifact_generation_catalog&catalog,std::uint64_t expected,std::shared_ptr<const void>prior={}){if(state_!=transaction_state::verified||!certificate_||certificate_->consumed_||certificate_->owner_!=owner_||certificate_->stage_!=stage_||certificate_->slot_!=slot_||certificate_->artifact_!=report_.artifact_digest||certificate_->report_!=report_.report_digest){state_=transaction_state::failed;return make_error(boolean_error_code::internal_invariant_error,stage_,"publish_unverified");}auto p=std::make_shared<published_artifact<Artifact>>(published_artifact<Artifact>{owner_,stage_,slot_,report_.artifact_digest,candidate_,report_,expected+1,std::move(prior)});auto generation=catalog.compare_and_publish(slot_,expected,owner_,p);if(!generation.has_value()){state_=transaction_state::failed;return generation.error();}if(generation.value()!=p->generation){state_=transaction_state::failed;return make_error(boolean_error_code::internal_invariant_error,stage_,"generation_mismatch");}certificate_->consumed_=true;for(auto&r:reservations_)r.commit();state_=transaction_state::published;draft_.reset();return std::shared_ptr<const published_artifact<Artifact>>(std::move(p));}
};
} }
#endif
