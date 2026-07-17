#include "YgorMeshesBooleanBroadPhase.h"
#include <algorithm>
#include <new>
#include <numeric>
#include <tuple>

#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "The Boolean broad phase requires strict floating-point compilation"
#endif

namespace ygor { namespace mesh_boolean {
namespace {
bool interval_valid(const exact_interval&i)noexcept{return i.lower_closed&&i.upper_closed&&!(i.upper<i.lower);}
bool interval_overlap(const exact_interval&a,const exact_interval&b)noexcept{return !(a.upper<b.lower||b.upper<a.lower);}
exact_interval interval_union(const exact_interval&a,const exact_interval&b){return {b.lower<a.lower?b.lower:a.lower,a.upper<b.upper?b.upper:a.upper,true,true};}
exact_feature_bound3 from_box(const exact_box3&b){return {{b.minimum.x,b.maximum.x,true,true},{b.minimum.y,b.maximum.y,true,true},{b.minimum.z,b.maximum.z,true,true}};}
bool key_less(const facet_candidate_key&a,const facet_candidate_key&b)noexcept{return std::tie(a.operand_a_facet,a.operand_b_facet)<std::tie(b.operand_a_facet,b.operand_b_facet);}
bool bounded_key_less(const bounded_feature_key&a,const bounded_feature_key&b)noexcept{return std::tie(a.caller_domain,a.canonical_rank)<std::tie(b.caller_domain,b.canonical_rank);}
void enc_digest(canonical_encoder&e,const digest&d){e.raw(d.bytes.data(),d.bytes.size());}

template<class T,class I>std::vector<std::uint8_t> semantic_bytes(const candidate_stream<T,I>&a){canonical_encoder e;const char tag[]="YGBCAN04";e.raw(reinterpret_cast<const std::uint8_t*>(tag),8);e.u16(candidate_stream_schema);e.u16(broad_phase_bound_semantics_version);enc_digest(e,a.validated->payload->operands[0].semantic_digest);enc_digest(e,a.validated->payload->operands[1].semantic_digest);e.u64(a.candidates.size());for(const auto&c:a.candidates){e.id(c.id);e.id(c.key.operand_a_facet);e.id(c.key.operand_b_facet);}return e.bytes();}
template<class T,class I>std::vector<std::uint8_t> invocation_bytes(const candidate_stream<T,I>&a){canonical_encoder e;const char tag[]="YGBBPA04";e.raw(reinterpret_cast<const std::uint8_t*>(tag),8);e.u16(candidate_stream_schema);enc_digest(e,a.setup_digest);enc_digest(e,a.upstream_digest);enc_digest(e,a.kernel_policy_digest);e.u16(broad_phase_bound_semantics_version);e.u16(broad_phase_build_policy_version);const auto&s=a.statistics;e.u64(s.operand_a_facets);e.u64(s.operand_b_facets);e.u64(s.operand_a_safe);e.u64(s.operand_b_safe);e.u64(s.operand_a_fallback);e.u64(s.operand_b_fallback);e.boolean(s.cartesian_pairs.representable_in_u64);if(s.cartesian_pairs.representable_in_u64)e.u64(s.cartesian_pairs.value);e.u64(s.exact_box_overlap_candidates);e.u64(s.fallback_added_pairs);e.u64(s.final_candidates);e.byte_string(a.canonical_candidate_bytes);return e.bytes();}
template<class T,class I>digest artifact_digest_for(const candidate_stream<T,I>&a){canonical_encoder e;const char tag[]="YGBART01";e.raw(reinterpret_cast<const std::uint8_t*>(tag),8);e.byte(static_cast<std::uint8_t>(artifact_slot::candidate_stream));e.u64(candidate_stream_type_tag+(static_cast<std::uint64_t>(std::is_same<T,double>::value)<<8)+(static_cast<std::uint64_t>(std::is_same<I,std::uint64_t>::value)));e.u16(candidate_stream_schema);e.byte_string(a.artifact_bytes);return domain_digest({{'Y','G','B','A','R','T','0','1'}},e.bytes());}

struct feature{facet_id id;exact_feature_bound3 bound;};
struct node{exact_feature_bound3 bound;std::size_t begin=0,count=0,left=0,right=0;bool leaf=true;};
struct tree{std::vector<feature> features;std::vector<node> nodes;std::size_t max_depth=0;};
std::size_t build_node(tree&t,std::size_t begin,std::size_t count,std::size_t depth){const auto index=t.nodes.size();t.nodes.push_back({t.features[begin].bound,begin,count,0,0,true});t.max_depth=std::max(t.max_depth,depth);for(std::size_t i=begin+1;i<begin+count;++i)t.nodes[index].bound=exact_bound_union(t.nodes[index].bound,t.features[i].bound);if(count<=8)return index;auto spread=[&](unsigned axis){const auto&b=t.nodes[index].bound;const exact_interval&q=axis==0?b.x:axis==1?b.y:b.z;return q.upper-q.lower;};unsigned axis=0;if(spread(0)<spread(1))axis=1;if(spread(axis)<spread(2))axis=2;auto center=[axis](const feature&f)->exact_scalar{const auto&q=axis==0?f.bound.x:axis==1?f.bound.y:f.bound.z;return q.lower+q.upper;};std::sort(t.features.begin()+begin,t.features.begin()+begin+count,[&](const feature&a,const feature&b){auto x=center(a),y=center(b);return x==y?a.id<b.id:x<y;});const auto left_count=count/2;t.nodes[index].leaf=false;t.nodes[index].left=build_node(t,begin,left_count,depth+1);t.nodes[index].right=build_node(t,begin+left_count,count-left_count,depth+1);return index;}
tree make_tree(std::vector<feature>f){tree t;t.features=std::move(f);std::sort(t.features.begin(),t.features.end(),[](const auto&a,const auto&b){return a.id<b.id;});if(!t.features.empty()){t.nodes.reserve(t.features.size()*2);build_node(t,0,t.features.size(),0);}return t;}
std::vector<facet_candidate_key> traverse(const tree&a,const tree&b,broad_phase_implementation_statistics&stats,const std::function<bool()>&cancelled){std::vector<facet_candidate_key>out;if(a.nodes.empty()||b.nodes.empty())return out;std::vector<std::pair<std::size_t,std::size_t>>stack{{0,0}};while(!stack.empty()){if((stats.node_pair_tests&1023U)==0&&cancelled&&cancelled())throw boolean_error(make_error(boolean_error_code::resource_limit,boolean_stage::broad_phase,"cancelled"));auto p=stack.back();stack.pop_back();++stats.node_pair_tests;const auto&x=a.nodes[p.first];const auto&y=b.nodes[p.second];if(!exact_bounds_overlap(x.bound,y.bound))continue;if(x.leaf&&y.leaf){for(std::size_t i=0;i<x.count;++i)for(std::size_t j=0;j<y.count;++j){++stats.facet_pair_tests;const auto&af=a.features[x.begin+i];const auto&bf=b.features[y.begin+j];if(exact_bounds_overlap(af.bound,bf.bound))out.push_back({af.id,bf.id});}continue;}if(y.leaf||(!x.leaf&&x.count>=y.count)){stack.push_back({x.right,p.second});stack.push_back({x.left,p.second});}else{stack.push_back({p.first,y.right});stack.push_back({p.first,y.left});}}return out;}

template<class T,class I>bool validate_bounds(const validated_operands<T,I>&v){for(const auto&f:v.facets){if(!valid_exact_feature_bound(from_box(f.bounds))||!f.id.valid()||f.id.value_for_debug()>=v.facets.size()||v.facets[f.id.value_for_debug()].id!=f.id)return false;exact_box3 b;bool first=true;for(auto id:f.ring){if(!id.valid()||id.value_for_debug()>=v.vertices.size())return false;const auto&p=v.vertices[id.value_for_debug()].exact_coordinate;if(first){b={p,p};first=false;}else{if(p.x<b.minimum.x)b.minimum.x=p.x;if(b.maximum.x<p.x)b.maximum.x=p.x;if(p.y<b.minimum.y)b.minimum.y=p.y;if(b.maximum.y<p.y)b.maximum.y=p.y;if(p.z<b.minimum.z)b.minimum.z=p.z;if(b.maximum.z<p.z)b.maximum.z=p.z;}}if(first||!(b.minimum==f.bounds.minimum)||!(b.maximum==f.bounds.maximum))return false;}return true;}
template<class T,class I>std::vector<facet_candidate_key> exhaustive(const validated_operands<T,I>&v){std::vector<const validated_facet*>a,b;for(const auto&f:v.facets)(f.operand==operand_a()?a:b).push_back(&f);std::vector<facet_candidate_key>out;for(auto x:a)for(auto y:b)if(exact_bounds_overlap(from_box(x->bounds),from_box(y->bounds)))out.push_back({x->id,y->id});std::sort(out.begin(),out.end(),key_less);return out;}
template<class T,class I>std::vector<facet_candidate_key> sweep(const validated_operands<T,I>&v){struct event{exact_scalar x;bool start;operand_id role;const validated_facet*facet;};std::vector<event>events;for(const auto&f:v.facets){events.push_back({f.bounds.minimum.x,true,f.operand,&f});events.push_back({f.bounds.maximum.x,false,f.operand,&f});}std::sort(events.begin(),events.end(),[](const event&a,const event&b){if(a.x!=b.x)return a.x<b.x;if(a.start!=b.start)return a.start>b.start;if(a.role!=b.role)return a.role<b.role;return a.facet->id<b.facet->id;});std::vector<const validated_facet*>active[2];std::vector<facet_candidate_key>out;for(const auto&e:events){const unsigned role=e.role==operand_a()?0:1;if(e.start){for(const auto*other:active[1-role]){auto a=from_box(e.facet->bounds),b=from_box(other->bounds);if(interval_overlap(a.y,b.y)&&interval_overlap(a.z,b.z))out.push_back(role==0?facet_candidate_key{e.facet->id,other->id}:facet_candidate_key{other->id,e.facet->id});}active[role].push_back(e.facet);}else{auto it=std::find(active[role].begin(),active[role].end(),e.facet);if(it==active[role].end())throw std::logic_error("invalid sweep transition");active[role].erase(it);}}if(!active[0].empty()||!active[1].empty())throw std::logic_error("incomplete sweep");std::sort(out.begin(),out.end(),key_less);out.erase(std::unique(out.begin(),out.end()),out.end());return out;}

template<class T,class I>status_or<verification_report>verify_typed(const artifact_view&view,const verification_spec&spec,const verification_environment_view&env)noexcept{try{const auto&a=*static_cast<const candidate_stream<T,I>*>(view.payload);verification_report r;r.checker_version=spec.checker_version;r.owner=view.owner;r.stage=boolean_stage::broad_phase;r.slot=view.slot;r.artifact_type_tag=view.artifact_type_tag;r.artifact_schema=view.artifact_schema;r.setup_digest=env.setup_digest;r.artifact_digest=view.artifact_digest;r.invariant_set_digest=spec.invariant_set_digest;r.outcome=verification_outcome::pass;bool failed=false;for(auto code:spec.required_invariants){invariant_result q;q.code=code;q.status=failed?check_status::not_run_due_to_prior_failure:check_status::passed;bool ok=true;if(!failed)switch(code){case invariant_code::broad_phase_binding:ok=a.owner==view.owner&&a.setup_digest==env.setup_digest&&a.validated&&a.validated->owner==view.owner&&a.upstream_digest==a.validated->artifact_digest;break;case invariant_code::broad_phase_bounds:ok=validate_bounds(*a.validated->payload);break;case invariant_code::broad_phase_candidates:{auto expected=sweep(*a.validated->payload);if(spec.level==verification_level::exhaustive)ok=expected==exhaustive(*a.validated->payload);ok=ok&&expected.size()==a.candidates.size();for(std::size_t i=0;i<expected.size()&&ok;++i)ok=a.candidates[i].id.value_for_debug()==i&&a.candidates[i].key==expected[i];ok=ok&&a.statistics.final_candidates==expected.size()&&a.statistics.exact_box_overlap_candidates==expected.size();break;}case invariant_code::broad_phase_canonical_encoding:{auto sem=semantic_bytes(a);auto inv=invocation_bytes(a);ok=sem==a.canonical_candidate_bytes&&inv==a.artifact_bytes&&artifact_digest_for(a)==view.artifact_digest;break;}default:ok=false;}if(!ok){q.status=check_status::failed;q.subcode=1;failed=true;r.outcome=verification_outcome::invariant_failure;}r.results.push_back(q);}evidence_record evidence;evidence.kind=evidence_kind::coverage;evidence.invariant=invariant_code::broad_phase_candidates;canonical_encoder p;p.u64(a.candidates.size());evidence.exact_payload=p.bytes();evidence.dependencies={a.upstream_digest,a.artifact_digest};evidence.evidence_digest=evidence_digest(evidence);r.evidence.push_back(evidence);auto bytes=encode_verification_report(r);if(!bytes.has_value())return bytes.error();r.report_digest=domain_digest({{'Y','G','B','V','E','R','0','1'}},bytes.value());return r;}catch(const std::bad_alloc&){return make_error(boolean_error_code::resource_limit,boolean_stage::broad_phase,"broad_phase_verifier_allocation");}catch(...){return make_error(boolean_error_code::internal_invariant_error,boolean_stage::broad_phase,"broad_phase_verifier_exception");}}
template<class T,class I>status_or<verification_report>callback(const artifact_view&v,const verification_spec&s,const verification_environment_view&e)noexcept{return verify_typed<T,I>(v,s,e);}
}

bool valid_exact_feature_bound(const exact_feature_bound3&b)noexcept{return interval_valid(b.x)&&interval_valid(b.y)&&interval_valid(b.z);}
bool exact_bounds_overlap(const exact_feature_bound3&a,const exact_feature_bound3&b)noexcept{return interval_overlap(a.x,b.x)&&interval_overlap(a.y,b.y)&&interval_overlap(a.z,b.z);}
exact_feature_bound3 exact_bound_union(const exact_feature_bound3&a,const exact_feature_bound3&b){return {interval_union(a.x,b.x),interval_union(a.y,b.y),interval_union(a.z,b.z)};}

status_or<std::vector<bounded_feature_pair>> enumerate_bounded_feature_self(
    const std::vector<bounded_feature_view> &in,
    resource_accountant *accountant,
    const std::function<bool()> &cancelled) {
  try {
    std::vector<bounded_feature_view> features = in;
    if (features.empty()) return std::vector<bounded_feature_pair>{};
    const auto owner = features.front().key.owner;
    const auto domain = features.front().key.caller_domain;
    std::sort(features.begin(), features.end(), [](const auto &a, const auto &b) {
      return bounded_key_less(a.key, b.key);
    });
    for (std::size_t i = 0; i < features.size(); ++i)
      if (features[i].key.owner != owner ||
          features[i].key.caller_domain != domain ||
          features[i].key.canonical_rank != i ||
          (features[i].source == bound_source_kind::exact_box &&
           !valid_exact_feature_bound(features[i].bound)))
        return make_error(boolean_error_code::input_contract_error,
                          boolean_stage::input_validation,
                          "bounded_feature_contract");
    auto pairs = checked_multiply(features.size(), features.size() - 1,
                                  boolean_stage::input_validation);
    if (!pairs.has_value()) return pairs.error();
    if (accountant) {
      auto reserved = accountant->reserve(resource_kind::work_units,
                                           pairs.value() / 2,
                                           boolean_stage::input_validation);
      if (!reserved.has_value()) return reserved.error();
    }

    struct event {
      exact_scalar coordinate;
      bool start = false;
      std::size_t feature = 0;
    };
    std::vector<event> events;
    std::vector<std::size_t> fallback;
    events.reserve(features.size() * 2);
    for (std::size_t i = 0; i < features.size(); ++i) {
      if (features[i].source == bound_source_kind::exhaustive_fallback) {
        fallback.push_back(i);
      } else {
        events.push_back({features[i].bound.x.lower, true, i});
        events.push_back({features[i].bound.x.upper, false, i});
      }
    }
    std::sort(events.begin(), events.end(), [&](const event &a, const event &b) {
      if (a.coordinate != b.coordinate) return a.coordinate < b.coordinate;
      if (a.start != b.start) return a.start > b.start;
      return a.feature < b.feature;
    });

    std::vector<bounded_feature_pair> result;
    std::vector<std::size_t> active;
    std::uint64_t work = 0;
    auto add_pair = [&](std::size_t a, std::size_t b) {
      if (b < a) std::swap(a, b);
      result.push_back({features[a].key, features[b].key});
    };
    for (const auto &current : events) {
      if (cancelled && ((work++) & 1023U) == 0 && cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::input_validation, "cancelled");
      if (current.start) {
        for (auto other : active) {
          ++work;
          const auto &a = features[current.feature].bound;
          const auto &b = features[other].bound;
          if (interval_overlap(a.y, b.y) && interval_overlap(a.z, b.z))
            add_pair(current.feature, other);
        }
        active.push_back(current.feature);
      } else {
        auto position = std::find(active.begin(), active.end(), current.feature);
        if (position == active.end())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::input_validation,
                            "bounded_feature_sweep_transition");
        active.erase(position);
      }
    }
    if (!active.empty())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::input_validation,
                        "bounded_feature_sweep_incomplete");

    // A feature without a certified box remains a candidate against everything.
    for (auto i : fallback)
      for (std::size_t j = 0; j < features.size(); ++j) {
        if (j == i) continue;
        if (cancelled && ((work++) & 1023U) == 0 && cancelled())
          return make_error(boolean_error_code::resource_limit,
                            boolean_stage::input_validation, "cancelled");
        add_pair(i, j);
      }
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
      return std::tie(a.first.canonical_rank, a.second.canonical_rank) <
             std::tie(b.first.canonical_rank, b.second.canonical_rank);
    });
    result.erase(std::unique(result.begin(), result.end(),
                             [](const auto &a, const auto &b) {
                               return a.first.canonical_rank ==
                                          b.first.canonical_rank &&
                                      a.second.canonical_rank ==
                                          b.second.canonical_rank;
                             }),
                 result.end());
    return result;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation,
                      "bounded_feature_allocation");
  }
}

status_or<bool>register_broad_phase_verifier(verifier_registry&r,coordinate_tag c,index_tag i){verifier_registration x;x.slot=artifact_slot::candidate_stream;x.artifact_type_tag=candidate_stream_type_tag+(static_cast<std::uint64_t>(c)<<8)+static_cast<std::uint64_t>(i);x.artifact_schema=candidate_stream_schema;x.mandatory={invariant_code::broad_phase_binding,invariant_code::broad_phase_bounds,invariant_code::broad_phase_candidates,invariant_code::broad_phase_canonical_encoding};x.exhaustive=x.mandatory;if(c==coordinate_tag::binary32&&i==index_tag::uint32)x.callback=&callback<float,std::uint32_t>;else if(c==coordinate_tag::binary32)x.callback=&callback<float,std::uint64_t>;else if(i==index_tag::uint32)x.callback=&callback<double,std::uint32_t>;else x.callback=&callback<double,std::uint64_t>;return r.register_verifier(std::move(x));}

template <class T, class I>
status_or<std::shared_ptr<const published_artifact<candidate_stream<T, I>>>>
enumerate_broad_phase_candidates(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::broad_phase, "cancelled");
    auto upstream = validate_operands(ctx);
    if (!upstream.has_value())
      return upstream.error();
    performance_scope producer(ctx.performance_collector_for_internal_use(),
                               boolean_stage::broad_phase,
                               performance_role::producer);
    if (upstream.value()->owner != ctx.owner() ||
        upstream.value()->payload->setup_digest != ctx.replay().setup)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::broad_phase, "upstream_binding");
    if (!validate_bounds(*upstream.value()->payload))
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::broad_phase,
                        "malformed_authoritative_bound");
    std::vector<feature> af, bf;
    for (const auto &f : upstream.value()->payload->facets)
      (f.operand == operand_a() ? af : bf)
          .push_back({f.id, from_box(f.bounds)});
    auto product =
        checked_multiply(af.size(), bf.size(), boolean_stage::broad_phase);
    stage_transaction<candidate_stream<T, I>> tx(
        ctx.owner(), boolean_stage::broad_phase,
        artifact_slot::candidate_stream,
        std::make_unique<candidate_stream<T, I>>(),
        ctx.performance_collector_for_internal_use());
    auto &draft = tx.draft();
    draft.owner = ctx.owner();
    draft.setup_digest = ctx.replay().setup;
    draft.upstream_digest = upstream.value()->artifact_digest;
    draft.kernel_policy_digest =
        upstream.value()->payload->kernel_policy_digest;
    draft.validated = upstream.value();
    draft.statistics.operand_a_facets = draft.statistics.operand_a_safe =
        af.size();
    draft.statistics.operand_b_facets = draft.statistics.operand_b_safe =
        bf.size();
    draft.statistics.cartesian_pairs.representable_in_u64 = product.has_value();
    if (product.has_value())
      draft.statistics.cartesian_pairs.value = product.value();
    auto ta = make_tree(std::move(af)), tb = make_tree(std::move(bf));
    draft.implementation_statistics.node_count =
        ta.nodes.size() + tb.nodes.size();
    draft.implementation_statistics.max_depth =
        std::max(ta.max_depth, tb.max_depth);
    auto keys = traverse(ta, tb, draft.implementation_statistics,
                         [&] { return ctx.cancelled(); });
    std::sort(keys.begin(), keys.end(), key_less);
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    auto candidate_charge = ctx.accountant().reserve_scoped(
        resource_kind::candidates, keys.size(), boolean_stage::broad_phase);
    if (!candidate_charge.has_value())
      return candidate_charge.error();
    for (std::size_t i = 0; i < keys.size(); ++i)
      draft.candidates.push_back(
          {candidate_id::from_canonical_value(i), keys[i]});
    draft.statistics.exact_box_overlap_candidates = keys.size();
    draft.statistics.final_candidates = keys.size();
    draft.canonical_candidate_bytes = semantic_bytes(draft);
    draft.artifact_bytes = invocation_bytes(draft);
    draft.artifact_digest = artifact_digest_for(draft);
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::broad_phase,
                        "broad_phase_verifier_registry_required");
    const auto type =
        candidate_stream_type_tag +
        (static_cast<std::uint64_t>(ctx.platform().coordinate) << 8) +
        static_cast<std::uint64_t>(ctx.platform().index);
    auto spec = registry->specification(artifact_slot::candidate_stream, type,
                                        candidate_stream_schema,
                                        ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    verification_environment_view env;
    env.owner = ctx.owner();
    env.setup_digest = ctx.replay().setup;
    env.op = ctx.contract().selected_operation();
    env.options = &ctx.options();
    env.coordinate = ctx.platform().coordinate;
    env.index = ctx.platform().index;
    env.exact_kernel = &ctx.kernel();
    env.raw_operands = {env.coordinate, env.index, &ctx.operand_a_mesh(),
                        &ctx.operand_b_mesh()};
    env.accountant = &ctx.accountant();
    env.cancelled = [&] { return ctx.cancelled(); };
    auto authoritative = ctx.accountant().reserve_scoped(
        resource_kind::authoritative_bytes,
        draft.artifact_bytes.size() +
            draft.candidates.size() * sizeof(facet_candidate),
        boolean_stage::broad_phase);
    if (!authoritative.has_value())
      return authoritative.error();
    tx.stage_reservation(std::move(candidate_charge.value()));
    tx.stage_reservation(std::move(authoritative.value()));
    performance_count(performance_counter::broad_phase_node_pairs,
                      draft.implementation_statistics.node_pair_tests);
    performance_count(performance_counter::broad_phase_leaf_facet_pairs,
                      draft.implementation_statistics.facet_pair_tests);
    performance_count(performance_counter::broad_phase_final_candidates,
                      draft.statistics.final_candidates);
    producer.finish();
    auto verified = tx.freeze_and_verify(type, candidate_stream_schema, 1,
                                         draft.artifact_digest, spec.value(),
                                         env, *registry);
    if (!verified.has_value())
      return verified.error();
    return tx.publish();
  } catch (const boolean_error &e) {
    return e;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::broad_phase, "broad_phase_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::broad_phase, "broad_phase_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T,I) template status_or<std::shared_ptr<const published_artifact<candidate_stream<T,I>>>>enumerate_broad_phase_candidates(boolean_context<T,I>&)
INST(float,std::uint32_t);INST(float,std::uint64_t);INST(double,std::uint32_t);INST(double,std::uint64_t);
#undef INST
} }
