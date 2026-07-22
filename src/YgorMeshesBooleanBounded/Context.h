#pragma once

#include "Identity.h"
#include "InvocationSources.h"
#include "Policies.h"
#include "SymbolicPolicy.h"
#include "Replay.h"

#include <memory>

namespace ygor::mesh_boolean::bounded {
template<class T>struct precision_bootstrap_record { std::uint16_t version=1;T machine_floor=T(0);T input_precision_a=T(0);T input_precision_b=T(0);bounded_boolean_digest source_digest{}; };
template<class T,class I>struct pending_invocation { immutable_invocation_sources<T,I> sources;bounded_boolean_options<T> options;boolean_operation operation;truth_table truth;symbolic_policy_table symbolic;std::vector<std::uint8_t> replay_bytes;bounded_boolean_digest replay_digest{}; };
template<class T,class I>struct boolean_context { std::shared_ptr<const immutable_invocation_sources<T,I>> sources;bounded_boolean_options<T> options;boolean_operation operation;truth_table truth;symbolic_policy_table symbolic;precision_bootstrap_record<T> precision;context_owner_token owner;bounded_boolean_digest input_digest{},context_digest{},replay_digest{}; };
template<class T,class I>
boolean_outcome<pending_invocation<T,I>> build_pending_invocation(const fv_surface_mesh<T,I>&a,const fv_surface_mesh<T,I>&b,boolean_operation operation,bounded_boolean_options<T> options) {
    const auto op=static_cast<std::uint8_t>(operation);if(op<1||op>5)return boolean_outcome<pending_invocation<T,I>>::failure(policy_error(2010));
    auto normalized=normalize_options(options);if(!normalized.has_value())return boolean_outcome<pending_invocation<T,I>>::failure(*normalized.error());
    auto sources=capture_invocation_sources(a,b);if(!sources.has_value())return boolean_outcome<pending_invocation<T,I>>::failure(*sources.error());
    const auto &r=normalized.value()->resources;
    std::uint64_t vertices=0,faces=0,indices=0;
    if(!checked_add<std::uint64_t>(sources.value()->a.vertex_count(),sources.value()->b.vertex_count(),vertices)||!checked_add<std::uint64_t>(sources.value()->a.face_count(),sources.value()->b.face_count(),faces)||!checked_add<std::uint64_t>(sources.value()->a.indices().size(),sources.value()->b.indices().size(),indices)||vertices>r.source_vertices.hard||faces>r.source_faces.hard||indices>r.source_indices.hard)return boolean_outcome<pending_invocation<T,I>>::failure(source_error(1203));
    pending_invocation<T,I> out{std::move(*sources.value()),std::move(*normalized.value()),operation,materialize_truth_table(),materialize_symbolic_policy(),{}, {}};
    out.replay_bytes=encode_replay_input(operation,out.options,out.sources,out.truth,out.symbolic);out.replay_digest=sha256::digest(out.replay_bytes);
    return boolean_outcome<pending_invocation<T,I>>::success(std::move(out));
}
template<class T,class I>bool verify_pending(const pending_invocation<T,I>&p){return verify_source(p.sources.a)&&verify_source(p.sources.b)&&sha256::digest(p.replay_bytes)==p.replay_digest&&materialize_truth_table().digest==p.truth.digest&&verify_symbolic_policy(p.symbolic);}
template<class T,class I>
boolean_outcome<boolean_context<T,I>> finalize_context(pending_invocation<T,I> pending,precision_bootstrap_record<T> precision) {
    if(!verify_pending(pending))return boolean_outcome<boolean_context<T,I>>::failure(policy_error(2101,bounded_boolean_error_category::internal_invariant_error));
    if(precision.version!=1||precision.source_digest!=pending.sources.digest||!std::isfinite(precision.machine_floor)||precision.machine_floor<T(0)||precision.machine_floor>pending.options.tolerance||precision.input_precision_a!=pending.options.input_precision_a||precision.input_precision_b!=pending.options.input_precision_b)
        return boolean_outcome<boolean_context<T,I>>::failure(policy_error(2102,bounded_boolean_error_category::invalid_tolerance));
    auto sources=std::make_shared<const immutable_invocation_sources<T,I>>(std::move(pending.sources));
    boolean_context<T,I> context{sources,std::move(pending.options),pending.operation,std::move(pending.truth),std::move(pending.symbolic),precision,context_owner_token::create(),sources->digest,{},pending.replay_digest};
    canonical_writer writer;writer.u16(contract_versions::context);writer.u8(static_cast<std::uint8_t>(context.operation));writer.floating(context.options.tolerance);writer.floating(context.precision.machine_floor);for(auto byte:context.input_digest.bytes)writer.u8(byte);for(auto byte:context.truth.digest.bytes)writer.u8(byte);for(auto byte:context.symbolic.digest.bytes)writer.u8(byte);for(auto byte:context.replay_digest.bytes)writer.u8(byte);context.context_digest=sha256::digest(writer.bytes());
    return boolean_outcome<boolean_context<T,I>>::success(std::move(context));
}
}
