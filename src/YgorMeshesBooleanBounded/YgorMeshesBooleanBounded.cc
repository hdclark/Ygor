#include "StrictFloatingBuild.h"
#include "Context.h"
#include "PlatformQualification.h"
#include "PrecisionContext.h"
#include "PrecisionVerifier.h"
#include "InputValidation.h"

#include <iomanip>
#include <new>
#include <sstream>
#include <stdexcept>

std::string bounded_boolean_digest::hex()const { static constexpr char digits[]="0123456789abcdef";std::string out;out.resize(64);for(std::size_t i=0;i<bytes.size();++i){out[2*i]=digits[bytes[i]>>4];out[2*i+1]=digits[bytes[i]&15];}return out; }

namespace {
bounded_boolean_error entry_error(bounded_boolean_error_category category,std::uint32_t subcode,const char*summary){bounded_boolean_error e;e.category=category;e.subcode=subcode;e.stage=1;e.summary=summary;return e;}
template<class T,class I>
bounded_boolean_result<T,I> invoke(const fv_surface_mesh<T,I>&a,const fv_surface_mesh<T,I>&b,boolean_operation operation,const bounded_boolean_options<T>&options,const bounded_boolean_cancellation_token&token){
    using namespace ygor::mesh_boolean::bounded;
    static_assert(supported_type_profile<T,I>(),"Unsupported bounded Boolean V1 type profile");
    floating_environment_guard guard;if(!guard.qualified())return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::unsupported_platform,3001,"strict floating environment unavailable"));
    if(token.cancellation_requested()){auto e=entry_error(bounded_boolean_error_category::cancelled,4001,"bounded Boolean cancelled before source capture");e.witnesses[0]=token.reason();e.witness_count=1;return bounded_boolean_result<T,I>(e);}
    try {
        auto pending=build_pending_invocation(a,b,operation,options);if(!pending.has_value())return bounded_boolean_result<T,I>(*pending.error());
        if(!verify_pending(*pending.value()))return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::internal_invariant_error,5001,"pending invocation verification failed"));
        if(token.cancellation_requested()){auto e=entry_error(bounded_boolean_error_category::cancelled,4002,"bounded Boolean cancelled after source capture");e.replay_digest=pending.value()->replay_digest;e.witnesses[0]=token.reason();e.witness_count=1;return bounded_boolean_result<T,I>(e);}
        auto preflight=preflight_precision(*pending.value());if(!preflight.has_value())return bounded_boolean_result<T,I>(*preflight.error());
        if(!verify_precision_preflight(*preflight.value(),*pending.value()))return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::internal_invariant_error,30024,"precision preflight verification failed"));
        auto frozen=finalize_context(std::move(*pending.value()),make_precision_bootstrap_record(*preflight.value()));if(!frozen.has_value())return bounded_boolean_result<T,I>(*frozen.error());
        precision_runtime_capabilities capabilities;capabilities.expected_owner=&frozen.value()->owner;
        auto precision=build_precision_context(*preflight.value(),*frozen.value(),capabilities);if(!precision.has_value())return bounded_boolean_result<T,I>(*precision.error());
        if(!verify_precision_context(**precision.value(),*preflight.value(),*frozen.value(),capabilities))return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::internal_invariant_error,30025,"precision context verification failed"));
        if(token.cancellation_requested()){auto e=entry_error(bounded_boolean_error_category::cancelled,4003,"bounded Boolean cancelled after precision bootstrap");e.context_digest=(*precision.value())->digest();e.replay_digest=frozen.value()->replay_digest;e.witnesses[0]=token.reason();e.witness_count=1;return bounded_boolean_result<T,I>(e);}
        resource_manager input_resources(frozen.value()->options.resources);input_validation_capabilities input_caps;input_caps.owner=frozen.value()->owner;input_caps.cancellation=&token;input_caps.resources=&input_resources;
        auto validated_a=validate_operand(operand_id::a,frozen.value()->sources->a,*frozen.value(),**precision.value(),input_caps);if(!validated_a.has_value())return bounded_boolean_result<T,I>(*validated_a.error());
        auto validated_b=validate_operand(operand_id::b,frozen.value()->sources->b,*frozen.value(),**precision.value(),input_caps);if(!validated_b.has_value())return bounded_boolean_result<T,I>(*validated_b.error());
        auto e=entry_error(bounded_boolean_error_category::result_geometry_not_validated,2004,"Component 04 source triangulation is not installed");e.component=4;e.stage=static_cast<std::uint16_t>(stage_id::publication);e.context_digest=(*precision.value())->digest();e.replay_digest=frozen.value()->replay_digest;e.witnesses[0]=(*validated_a.value())->facets().size();e.witnesses[1]=(*validated_b.value())->facets().size();e.witness_count=2;return bounded_boolean_result<T,I>(e);
    } catch(const std::bad_alloc &) {
        return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::resource_limit,1201,"host allocation failed"));
    } catch(const std::length_error &) {
        return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::resource_limit,1202,"container capacity failed"));
    } catch(...) {
        return bounded_boolean_result<T,I>(entry_error(bounded_boolean_error_category::internal_invariant_error,5002,"unexpected Component 01 exception"));
    }
}
}
template<class T,class I>bounded_boolean_result<T,I> bounded_boolean(const fv_surface_mesh<T,I>&a,const fv_surface_mesh<T,I>&b,boolean_operation operation,const bounded_boolean_options<T>&options){return invoke(a,b,operation,options,bounded_boolean_cancellation_token{});}
template<class T,class I>bounded_boolean_result<T,I> bounded_boolean(const fv_surface_mesh<T,I>&a,const fv_surface_mesh<T,I>&b,boolean_operation operation,const bounded_boolean_options<T>&options,const bounded_boolean_cancellation_token&token){return invoke(a,b,operation,options,token);}
#define YGOR_INSTANTIATE(T,I) template bounded_boolean_result<T,I> bounded_boolean(const fv_surface_mesh<T,I>&,const fv_surface_mesh<T,I>&,boolean_operation,const bounded_boolean_options<T>&);template bounded_boolean_result<T,I> bounded_boolean(const fv_surface_mesh<T,I>&,const fv_surface_mesh<T,I>&,boolean_operation,const bounded_boolean_options<T>&,const bounded_boolean_cancellation_token&)
YGOR_INSTANTIATE(float,std::uint32_t);YGOR_INSTANTIATE(float,std::uint64_t);YGOR_INSTANTIATE(double,std::uint32_t);YGOR_INSTANTIATE(double,std::uint64_t);
#undef YGOR_INSTANTIATE
