#pragma once
#ifndef YGOR_MESHES_BOOLEAN_BROAD_PHASE_H_
#define YGOR_MESHES_BOOLEAN_BROAD_PHASE_H_
#include "YgorMeshesBooleanInputTopology.h"

namespace ygor { namespace mesh_boolean {

constexpr std::uint64_t candidate_stream_type_tag=0x59474243414e3034ULL; // YGBCAN04
constexpr std::uint16_t candidate_stream_schema=1;
constexpr std::uint16_t broad_phase_bound_semantics_version=1;
constexpr std::uint16_t broad_phase_build_policy_version=1;

struct exact_feature_bound3{exact_interval x,y,z;};
bool valid_exact_feature_bound(const exact_feature_bound3&) noexcept;
bool exact_bounds_overlap(const exact_feature_bound3&,const exact_feature_bound3&) noexcept;
exact_feature_bound3 exact_bound_union(const exact_feature_bound3&,const exact_feature_bound3&);

enum class bound_fallback_reason:std::uint8_t{missing_exact_bound=1,uncertified_enclosure=2,caller_requested=3};
enum class bound_source_kind:std::uint8_t{exact_box,exhaustive_fallback};
struct bounded_feature_key{context_owner_token owner;std::uint32_t caller_domain=0;std::uint64_t canonical_rank=0;};
struct bounded_feature_view{bounded_feature_key key;bound_source_kind source=bound_source_kind::exact_box;exact_feature_bound3 bound;bound_fallback_reason fallback_reason=bound_fallback_reason::caller_requested;};
struct bounded_feature_pair{bounded_feature_key first,second;};

status_or<std::vector<bounded_feature_pair>> enumerate_bounded_feature_self(
    const std::vector<bounded_feature_view>&,resource_accountant* = nullptr,
    const std::function<bool()>& = {});

struct facet_candidate_key{facet_id operand_a_facet,operand_b_facet;};
inline bool operator==(const facet_candidate_key&a,const facet_candidate_key&b){return a.operand_a_facet==b.operand_a_facet&&a.operand_b_facet==b.operand_b_facet;}
struct facet_candidate{candidate_id id;facet_candidate_key key;};
struct checked_cartesian_count{bool representable_in_u64=true;std::uint64_t value=0;};
struct broad_phase_statistics{
    std::uint64_t operand_a_facets=0,operand_b_facets=0;
    std::uint64_t operand_a_safe=0,operand_b_safe=0,operand_a_fallback=0,operand_b_fallback=0;
    checked_cartesian_count cartesian_pairs;
    std::uint64_t exact_box_overlap_candidates=0,fallback_added_pairs=0,final_candidates=0;
};
struct broad_phase_implementation_statistics{std::uint64_t node_count=0,max_depth=0,node_pair_tests=0,facet_pair_tests=0;};

template<class T,class I>struct candidate_stream{
    context_owner_token owner;digest setup_digest,upstream_digest,kernel_policy_digest,artifact_digest;
    std::shared_ptr<const published_artifact<validated_operands<T,I>>> validated;
    broad_phase_statistics statistics;
    broad_phase_implementation_statistics implementation_statistics;
    std::vector<facet_candidate> candidates;
    std::vector<std::uint8_t> canonical_candidate_bytes,artifact_bytes;
};

status_or<bool> register_broad_phase_verifier(verifier_registry&,coordinate_tag,index_tag);
template<class T,class I>status_or<std::shared_ptr<const published_artifact<candidate_stream<T,I>>>>enumerate_broad_phase_candidates(boolean_context<T,I>&);

#define YGOR_BROAD_EXTERN(T,I) extern template status_or<std::shared_ptr<const published_artifact<candidate_stream<T,I>>>>enumerate_broad_phase_candidates(boolean_context<T,I>&)
YGOR_BROAD_EXTERN(float,std::uint32_t);YGOR_BROAD_EXTERN(float,std::uint64_t);YGOR_BROAD_EXTERN(double,std::uint32_t);YGOR_BROAD_EXTERN(double,std::uint64_t);
#undef YGOR_BROAD_EXTERN
} }
#endif
