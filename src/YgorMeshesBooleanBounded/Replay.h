#pragma once

#include "InvocationSources.h"
#include "Policies.h"
#include "SymbolicPolicy.h"

#include <algorithm>
#include <utility>

namespace ygor::mesh_boolean::bounded {
struct decoded_replay_input { std::uint8_t scalar_bytes=0,index_bytes=0;boolean_operation operation=boolean_operation::set_union;std::vector<std::uint8_t> truth_bytes,symbolic_bytes,source_a_bytes,source_b_bytes;bounded_boolean_digest digest{}; };
template<class T,class I>
std::vector<std::uint8_t> encode_replay_input(boolean_operation operation,const bounded_boolean_options<T>&options,const immutable_invocation_sources<T,I>&sources,const truth_table&truth,const symbolic_policy_table&symbolic) {
    canonical_writer w;w.u32(0x52424759);w.u16(1);w.u16(1);w.u8(sizeof(T));w.u8(sizeof(I));w.u8(static_cast<std::uint8_t>(operation));
    w.floating(options.tolerance);w.floating(options.input_precision_a);w.floating(options.input_precision_b);
    w.u8(static_cast<std::uint8_t>(options.solids.kind));w.u8(static_cast<std::uint8_t>(options.contacts.kind));w.u8(static_cast<std::uint8_t>(options.output.kind));
    w.u8(static_cast<std::uint8_t>(options.verification.level));w.u8(static_cast<std::uint8_t>(options.determinism.mode));w.u8(static_cast<std::uint8_t>(options.execution.mode));w.u32(options.execution.requested_workers);w.u8(static_cast<std::uint8_t>(options.diagnostics.replay));
    w.sized_bytes(truth.bytes);for(auto byte:truth.digest.bytes)w.u8(byte);w.sized_bytes(symbolic.bytes);for(auto byte:symbolic.digest.bytes)w.u8(byte);
    w.sized_bytes(sources.a.canonical_bytes());w.sized_bytes(sources.b.canonical_bytes());return w.take();
}
inline boolean_outcome<decoded_replay_input> decode_replay_input(const std::vector<std::uint8_t>&bytes,std::uint64_t maximum_section_bytes=1ULL<<34) {
    canonical_reader r(bytes);std::uint32_t magic=0;std::uint16_t replay_version=0,public_version=0;decoded_replay_input out;std::uint8_t operation=0;
    if(!r.u32(magic)||magic!=0x52424759||!r.u16(replay_version)||replay_version!=1||!r.u16(public_version)||public_version!=1||!r.u8(out.scalar_bytes)||!r.u8(out.index_bytes)||!r.u8(operation)||operation<1||operation>5||(out.scalar_bytes!=4&&out.scalar_bytes!=8)||(out.index_bytes!=4&&out.index_bytes!=8))return boolean_outcome<decoded_replay_input>::failure(policy_error(6001));
    out.operation=static_cast<boolean_operation>(operation);std::uint64_t scalar=0;for(unsigned i=0;i<3;++i){if(out.scalar_bytes==4){std::uint32_t value=0;if(!r.u32(value))return boolean_outcome<decoded_replay_input>::failure(policy_error(6002));}else if(!r.u64(scalar))return boolean_outcome<decoded_replay_input>::failure(policy_error(6002));}
    std::uint8_t enum_value=0;for(unsigned i=0;i<6;++i)if(!r.u8(enum_value)||enum_value==0)return boolean_outcome<decoded_replay_input>::failure(policy_error(6003));std::uint32_t workers=0;if(!r.u32(workers)||!r.u8(enum_value)||enum_value<1||enum_value>3)return boolean_outcome<decoded_replay_input>::failure(policy_error(6003));
    std::vector<std::uint8_t> truth_digest,symbolic_digest;
    if(!r.sized_bytes(out.truth_bytes,maximum_section_bytes)||!r.fixed_bytes(32,truth_digest)||!r.sized_bytes(out.symbolic_bytes,maximum_section_bytes)||!r.fixed_bytes(32,symbolic_digest)||!r.sized_bytes(out.source_a_bytes,maximum_section_bytes)||!r.sized_bytes(out.source_b_bytes,maximum_section_bytes)||!r.complete())return boolean_outcome<decoded_replay_input>::failure(policy_error(6004));
    const auto truth=sha256::digest(out.truth_bytes),symbolic=sha256::digest(out.symbolic_bytes);if(!std::equal(truth.bytes.begin(),truth.bytes.end(),truth_digest.begin())||!std::equal(symbolic.bytes.begin(),symbolic.bytes.end(),symbolic_digest.begin()))return boolean_outcome<decoded_replay_input>::failure(policy_error(6005));out.digest=sha256::digest(bytes);return boolean_outcome<decoded_replay_input>::success(std::move(out));
}
}
