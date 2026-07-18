#include "YgorMeshesBooleanGlobalArrangement.h"
#include "YgorMeshesBooleanExecutor.h"
#include <algorithm>
#include <map>
#include <set>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Component 8 requires strict floating-point compilation"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Component 8 must not assume finite-only arithmetic"
#endif

namespace ygor { namespace mesh_boolean { namespace {
template<class T,class I> std::uint64_t type_tag(){return arrangement_complex_type_tag+(static_cast<std::uint64_t>(std::is_same<T,double>::value?coordinate_tag::binary64:coordinate_tag::binary32)<<8)+static_cast<std::uint64_t>(std::is_same<I,std::uint64_t>::value?index_tag::uint64:index_tag::uint32);}
void plane(canonical_encoder&e,const exact_plane3&p){encode(e,p.a);encode(e,p.b);encode(e,p.c);encode(e,p.d);e.byte(static_cast<std::uint8_t>(p.oriented));}
void vector3(canonical_encoder&e,const exact_vector3&v){encode(e,v.x);encode(e,v.y);encode(e,v.z);}
void point3(canonical_encoder&e,const exact_point3&p){encode(e,p.x);encode(e,p.y);encode(e,p.z);}
struct canonical_plane_key {
  exact_plane3 plane;
  bool include_orientation = true;
  bool operator<(const canonical_plane_key &o) const {
    int c = canonical_encoding_compare(plane.a, o.plane.a);
    if (!c) c = canonical_encoding_compare(plane.b, o.plane.b);
    if (!c) c = canonical_encoding_compare(plane.c, o.plane.c);
    if (!c) c = canonical_encoding_compare(plane.d, o.plane.d);
    if (c) return c < 0;
    if (include_orientation != o.include_orientation)
      return include_orientation < o.include_orientation;
    return include_orientation && plane.oriented != o.plane.oriented &&
           static_cast<std::uint8_t>(plane.oriented) <
               static_cast<std::uint8_t>(o.plane.oriented);
  }
};
template<class Id> void ids(canonical_encoder&e,const std::vector<Id>&v){e.u64(v.size());for(auto x:v)e.id(x);}
std::vector<symbolic_vertex_id> normalized_cycle(std::vector<symbolic_vertex_id> v){
  if(v.empty())return v;
  auto rotate_min=[](std::vector<symbolic_vertex_id>x){auto it=std::min_element(x.begin(),x.end());std::rotate(x.begin(),it,x.end());return x;};
  auto a=rotate_min(v);std::reverse(v.begin(),v.end());auto b=rotate_min(v);return b<a?b:a;
}
template<class T,class I> std::vector<std::uint8_t> semantic(const arrangement_complex<T,I>&a,bool quotient){
  canonical_encoder e;const char*tag=quotient?"YGBCAN8Q":"YGBCAN08";e.raw(reinterpret_cast<const std::uint8_t*>(tag),8);e.u16(arrangement_complex_schema);
  e.byte(static_cast<std::uint8_t>(a.classification));
  e.u64(a.vertices.size());for(const auto&v:a.vertices){e.id(v.id);e.id(v.symbolic);ids(e,v.incident_edges);ids(e,v.occurrences);if(!quotient){e.u64(v.local_occurrences.size());for(auto r:v.local_occurrences){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.vertex_occurrences.size());for(const auto&o:a.vertex_occurrences){e.id(o.id);e.id(o.vertex);e.id(o.operand);e.id(o.shell);ids(e,o.incident_halfedges);ids(e,o.link_regions);if(!quotient){e.u64(o.local_germs.size());for(auto r:o.local_germs){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.edges.size());for(const auto&x:a.edges){e.id(x.id);e.id(x.lower);e.id(x.upper);e.byte(static_cast<std::uint8_t>(x.kind));ids(e,x.curves);if(!quotient){e.u64(x.local_occurrences.size());for(auto r:x.local_occurrences){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.sheet_members.size());for(const auto&m:a.sheet_members){e.id(m.id);e.id(m.operand);e.id(m.shell);ids(e,m.facets);if(!quotient){e.u64(m.local_patches.size());for(auto r:m.local_patches){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.patches.size());for(const auto&p:a.patches){e.id(p.id);plane(e,p.plane);encode(e,p.projected_double_area);ids(e,p.outer);e.u64(p.holes.size());for(const auto&h:p.holes)ids(e,h);ids(e,p.uses);}
  e.u64(a.sheet_uses.size());for(const auto&u:a.sheet_uses){e.id(u.id);e.id(u.member);e.id(u.patch);e.id(u.operand);e.id(u.shell);e.id(u.facet);e.boolean(u.source_plane_agrees);e.byte(static_cast<std::uint8_t>(u.occupied_side));ids(e,u.boundary);if(!quotient){e.id(u.local_patch.facet);e.id(u.local_patch.id);}}
  e.u64(a.halfedges.size());for(const auto&h:a.halfedges){e.id(h.id);e.id(h.next);e.id(h.previous);e.id(h.sheet_mate);e.id(h.use);e.id(h.edge);e.id(h.origin);e.id(h.destination);e.id(h.origin_occurrence);e.id(h.destination_occurrence);if(!quotient){e.id(h.local.facet);e.id(h.local.id);}}
  e.u64(a.seams.size());for(const auto&s:a.seams){e.id(s.id);e.id(s.edge);ids(e,s.incident_uses);e.u64(s.radial_layers.size());for(const auto&l:s.radial_layers)ids(e,l.uses);ids(e,s.sectors);}
  e.u64(a.seam_sectors.size());for(const auto&s:a.seam_sectors){e.id(s.id);e.id(s.seam);e.u64(s.lower_layer);e.u64(s.upper_layer);ids(e,s.incident_sides);}
  e.u64(a.source_edge_sectors.size());for(const auto&s:a.source_edge_sectors){e.id(s.id);e.id(s.edge);e.u64(s.lower_layer);e.u64(s.upper_layer);ids(e,s.incident_uses);}
  e.u64(a.link_rays.size());for(const auto&r:a.link_rays){e.id(r.id);vector3(e,r.direction);e.id(r.antipode);}
  e.u64(a.link_arcs.size());for(const auto&r:a.link_arcs){e.id(r.id);e.id(r.occurrence);e.id(r.origin);e.id(r.destination);ids(e,r.layers);}
  e.u64(a.vertex_sectors.size());for(const auto&s:a.vertex_sectors){e.id(s.id);e.id(s.vertex);e.id(s.occurrence);e.id(s.region);e.byte(static_cast<std::uint8_t>(s.germ));ids(e,s.boundary_rays);ids(e,s.boundary_arcs);vector3(e,s.witness_direction);e.u64(s.witness_evidence.size());for(auto x:s.witness_evidence)e.byte(static_cast<std::uint8_t>(x));ids(e,s.seam_continuations);ids(e,s.source_edge_continuations);}
  e.u64(a.coincident_groups.size());for(const auto&g:a.coincident_groups){e.id(g.id);e.id(g.patch);ids(e,g.members);}
  e.u64(a.patch_sides.size());for(const auto&s:a.patch_sides){e.id(s.id);e.id(s.patch);e.byte(static_cast<std::uint8_t>(s.side));e.id(s.component);}
  e.u64(a.transitions.size());for(const auto&t:a.transitions){e.id(t.id);e.byte(static_cast<std::uint8_t>(t.kind));e.id(t.from);e.id(t.to);e.boolean(t.region_crossing);ids(e,t.uses);e.boolean(bool(t.coincidence));if(t.coincidence)e.id(*t.coincidence);}
  e.u64(a.probes.size());for(const auto&p:a.probes){e.id(p.side);e.id(p.component);e.byte(static_cast<std::uint8_t>(p.base_kind));e.u64(p.base_id);e.boolean(bool(p.base_vertex));if(p.base_vertex)e.id(*p.base_vertex);e.boolean(bool(p.exact_base));if(p.exact_base)point3(e,*p.exact_base);vector3(e,p.direction);e.u64(p.constraints.size());for(const auto&c:p.constraints){plane(e,c.plane);e.byte(static_cast<std::uint8_t>(c.required));}e.u64(p.evidence.size());for(auto x:p.evidence)e.byte(static_cast<std::uint8_t>(x));e.u16(p.formula_version);}
  if(!quotient){e.u64(a.local_maps.size());for(const auto&m:a.local_maps){e.byte(static_cast<std::uint8_t>(m.kind));e.id(m.facet);e.u64(m.local_id);e.u64(m.global_fragments.size());for(auto x:m.global_fragments)e.u64(x);e.boolean(m.retained_incidence_only);}}
  return e.bytes();
}
template<class T,class I>std::vector<std::uint8_t> invocation(const arrangement_complex<T,I>&a){canonical_encoder e;const char tag[]="YGBARR08";e.raw(reinterpret_cast<const std::uint8_t*>(tag),8);e.u16(arrangement_complex_schema);e.raw(a.setup_digest.bytes.data(),16);e.raw(a.refined_digest.bytes.data(),16);e.raw(a.symbolic_digest.bytes.data(),16);e.raw(a.validated_digest.bytes.data(),16);e.raw(a.kernel_policy_digest.bytes.data(),16);e.byte_string(a.canonical_bytes);return e.bytes();}
template<class T,class I>digest artifact_digest_for(const arrangement_complex<T,I>&a){canonical_encoder e;e.raw(a.setup_digest.bytes.data(),16);e.byte(static_cast<std::uint8_t>(artifact_slot::arrangement_complex));e.byte_string(a.artifact_bytes);return domain_digest({{'Y','G','B','A','R','T','0','1'}},e.bytes());}

struct projected_patch_cache {
  struct edge {
    exact_point2 u, v;
    exact_scalar lower_x, upper_x;
  };
  projection_axis axis = projection_axis::drop_z;
  std::vector<std::vector<exact_point2>> rings;
  std::vector<exact_scalar> slab_x;
  std::vector<edge> edges;
  std::vector<std::vector<std::size_t>> entering, leaving;
};

template<class T,class I>
projected_patch_cache project_patch(const arrangement_complex<T,I>&a,
                                    const global_patch&p){
  projected_patch_cache out;
  out.axis=dominant_projection(p.plane);
  auto point=[&](global_vertex_id id)->const exact_point3&{return a.symbolic->payload->vertices[a.vertices[id.value_for_debug()].symbolic.value_for_debug()].point;};
  out.rings.resize(1);
  for(auto v:p.outer)out.rings.front().push_back(project(point(v),out.axis));
  for(const auto&h:p.holes){
    out.rings.push_back({});
    for(auto v:h)out.rings.back().push_back(project(point(v),out.axis));
  }
  for(const auto&ring:out.rings)for(const auto&q:ring)out.slab_x.push_back(q.x);
  std::sort(out.slab_x.begin(),out.slab_x.end());
  out.slab_x.erase(std::unique(out.slab_x.begin(),out.slab_x.end()),out.slab_x.end());
  out.entering.resize(out.slab_x.size());
  out.leaving.resize(out.slab_x.size());
  for(const auto&ring:out.rings)for(std::size_t i=0;i<ring.size();++i){
    const auto&u=ring[i];const auto&v=ring[(i+1)%ring.size()];
    if(u.x==v.x)continue;
    projected_patch_cache::edge e{u,v,std::min(u.x,v.x),std::max(u.x,v.x)};
    const auto id=out.edges.size();
    const auto begin=static_cast<std::size_t>(std::lower_bound(out.slab_x.begin(),out.slab_x.end(),e.lower_x)-out.slab_x.begin());
    const auto end=static_cast<std::size_t>(std::lower_bound(out.slab_x.begin(),out.slab_x.end(),e.upper_x)-out.slab_x.begin());
    out.edges.push_back(std::move(e));
    out.entering[begin].push_back(id);
    out.leaving[end].push_back(id);
  }
  return out;
}

exact_point3 patch_interior(const global_patch&p,const projected_patch_cache&cache){
  const exact_scalar half=exact_scalar(1)/exact_scalar(2);
  auto lift=[&](const exact_point2&q){
    const exact_scalar A(p.plane.a,big_uint(1)),B(p.plane.b,big_uint(1));
    const exact_scalar C(p.plane.c,big_uint(1)),D(p.plane.d,big_uint(1));
    if(cache.axis==projection_axis::drop_x){
      const exact_scalar y=q.x,z=q.y;
      return exact_point3{(B*y+C*z+D).negated()/A,y,z};
    }
    if(cache.axis==projection_axis::drop_y){
      const exact_scalar z=q.x,x=q.y;
      return exact_point3{x,(A*x+C*z+D).negated()/B,z};
    }
    const exact_scalar x=q.x,y=q.y;
    return exact_point3{x,y,(A*x+B*y+D).negated()/C};
  };
  std::set<std::size_t> active;
  for(std::size_t slab=1;slab<cache.slab_x.size();++slab){
    const auto boundary=slab-1;
    for(auto edge:cache.leaving[boundary])active.erase(edge);
    for(auto edge:cache.entering[boundary])active.insert(edge);
    performance_count(performance_counter::patch_witness_slabs);
    const exact_scalar x=(cache.slab_x[slab-1]+cache.slab_x[slab])*half;
    std::vector<exact_scalar> crossings;
    crossings.reserve(active.size());
    for(auto id:active){
      const auto&e=cache.edges[id];
      crossings.push_back(e.u.y+(e.v.y-e.u.y)*(x-e.u.x)/(e.v.x-e.u.x));
      performance_count(performance_counter::patch_witness_crossings);
    }
    std::sort(crossings.begin(),crossings.end());
    crossings.erase(std::unique(crossings.begin(),crossings.end()),crossings.end());
    for(std::size_t interval=1;interval<crossings.size();++interval){
      if(crossings[interval-1]==crossings[interval])continue;
      const exact_point2 q{x,(crossings[interval-1]+crossings[interval])*half};
      auto outer=classify_point_polygon(q,cache.rings.front());
      if(!outer.has_value()||outer.value().kind!=point_region_kind::open_interior)continue;
      bool in_hole=false;
      for(std::size_t h=1;h<cache.rings.size();++h){
        auto location=classify_point_polygon(q,cache.rings[h]);
        if(!location.has_value()||location.value().kind!=point_region_kind::outside){in_hole=true;break;}
      }
      if(!in_hole)return lift(q);
    }

  }
  throw std::logic_error("global patch has no exact interior witness");
}

template<class T,class I> bool valid(const arrangement_complex<T,I>&a){
  if(!a.refined||!a.symbolic||!a.validated||a.owner!=a.refined->owner||a.refined->payload->symbolic.get()!=a.symbolic.get()||a.refined->payload->validated.get()!=a.validated.get())return false;
  if(a.classification!=classification_strategy::independent_patch_side_v1)return false;
  for(std::size_t i=0;i<a.vertices.size();++i)if(a.vertices[i].id.value_for_debug()!=i||a.vertices[i].symbolic.value_for_debug()>=a.symbolic->payload->vertices.size())return false;
  for(std::size_t i=0;i<a.vertex_occurrences.size();++i){const auto&o=a.vertex_occurrences[i];if(o.id.value_for_debug()!=i||o.vertex.value_for_debug()>=a.vertices.size()||o.local_germs.empty())return false;for(auto h:o.incident_halfedges)if(h.value_for_debug()>=a.halfedges.size())return false;}
  for(std::size_t i=0;i<a.edges.size();++i){const auto&x=a.edges[i];if(x.id.value_for_debug()!=i||x.lower.value_for_debug()>=a.vertices.size()||x.upper.value_for_debug()>=a.vertices.size()||!(x.lower<x.upper)||static_cast<unsigned>(x.kind)>3)return false;}
  for(std::size_t i=0;i<a.patches.size();++i){const auto&p=a.patches[i];if(p.id.value_for_debug()!=i||p.outer.size()<3||!(exact_scalar(0)<p.projected_double_area))return false;for(auto v:p.outer)if(v.value_for_debug()>=a.vertices.size())return false;}
  for(std::size_t i=0;i<a.sheet_uses.size();++i){const auto&u=a.sheet_uses[i];if(u.id.value_for_debug()!=i||u.patch.value_for_debug()>=a.patches.size()||u.member.value_for_debug()>=a.sheet_members.size()||u.boundary.empty())return false;}
  for(std::size_t i=0;i<a.halfedges.size();++i){const auto&h=a.halfedges[i];if(h.id.value_for_debug()!=i||h.origin.value_for_debug()>=a.vertices.size()||h.destination.value_for_debug()>=a.vertices.size()||h.origin_occurrence.value_for_debug()>=a.vertex_occurrences.size()||h.destination_occurrence.value_for_debug()>=a.vertex_occurrences.size()||a.vertex_occurrences[h.origin_occurrence.value_for_debug()].vertex!=h.origin||a.vertex_occurrences[h.destination_occurrence.value_for_debug()].vertex!=h.destination||h.next.value_for_debug()>=a.halfedges.size()||h.previous.value_for_debug()>=a.halfedges.size()||h.sheet_mate.value_for_debug()>=a.halfedges.size()||a.halfedges[h.next.value_for_debug()].previous!=h.id||a.halfedges[h.sheet_mate.value_for_debug()].sheet_mate!=h.id||a.halfedges[h.sheet_mate.value_for_debug()].origin_occurrence!=h.destination_occurrence||a.halfedges[h.sheet_mate.value_for_debug()].destination_occurrence!=h.origin_occurrence)return false;}
  for(const auto&u:a.sheet_uses){std::set<global_halfedge_id>seen;for(auto start:u.boundary){if(!seen.insert(start).second)continue;auto h=start;std::size_t n=0;do{if(h.value_for_debug()>=a.halfedges.size()||a.halfedges[h.value_for_debug()].use!=u.id||++n>a.halfedges.size())return false;h=a.halfedges[h.value_for_debug()].next;}while(h!=start);}}
  std::size_t expected=0;for(const auto&f:a.refined->payload->facets)expected+=f.vertices.size()*2+f.edges.size()+f.halfedges.size()+f.walks.size()+f.faces.size()+f.patches.size()+f.source_boundary.size()+f.constraints.size();if(a.local_maps.size()!=expected)return false;
  for(std::size_t i=0;i<a.patch_sides.size();++i)if(a.patch_sides[i].id.value_for_debug()!=i||a.patch_sides[i].patch.value_for_debug()>=a.patches.size())return false;
  for(std::size_t i=0;i<a.source_edge_sectors.size();++i)if(a.source_edge_sectors[i].id.value_for_debug()!=i||a.source_edge_sectors[i].edge.value_for_debug()>=a.edges.size()||a.edges[a.source_edge_sectors[i].edge.value_for_debug()].kind!=global_edge_kind::source_edge)return false;
  for(std::size_t i=0;i<a.link_rays.size();++i){const auto&r=a.link_rays[i];if(r.id.value_for_debug()!=i||r.antipode.value_for_debug()>=a.link_rays.size())return false;const auto&q=a.link_rays[r.antipode.value_for_debug()];if(q.antipode!=r.id||!(q.direction.x==r.direction.x.negated()&&q.direction.y==r.direction.y.negated()&&q.direction.z==r.direction.z.negated()))return false;}
  for(std::size_t i=0;i<a.link_arcs.size();++i){const auto&r=a.link_arcs[i];if(r.id.value_for_debug()!=i||r.occurrence.value_for_debug()>=a.vertex_occurrences.size()||r.origin.value_for_debug()>=a.link_rays.size()||r.destination.value_for_debug()>=a.link_rays.size()||r.origin==r.destination)return false;}
  for(std::size_t i=0;i<a.vertex_sectors.size();++i){const auto&s=a.vertex_sectors[i];if(s.id.value_for_debug()!=i||s.vertex.value_for_debug()>=a.vertices.size()||s.occurrence.value_for_debug()>=a.vertex_occurrences.size()||a.vertex_occurrences[s.occurrence.value_for_debug()].vertex!=s.vertex||s.region.value_for_debug()!=i)return false;for(auto r:s.boundary_rays)if(r.value_for_debug()>=a.link_rays.size())return false;for(auto r:s.boundary_arcs)if(r.value_for_debug()>=a.link_arcs.size()||a.link_arcs[r.value_for_debug()].occurrence!=s.occurrence)return false;}
  if(a.probes.size()!=(a.patch_sides.empty()?1:a.patch_sides.size()))return false;std::vector<bool>probe_side(a.patch_sides.size());for(const auto&p:a.probes){if(p.base_kind!=probe_base_stratum_kind::universe){if(p.side.value_for_debug()>=a.patch_sides.size()||probe_side[p.side.value_for_debug()]||p.base_id!=p.side.value_for_debug()||p.component!=a.patch_sides[p.side.value_for_debug()].component||!p.exact_base)return false;probe_side[p.side.value_for_debug()]=true;}if(p.constraints.size()!=p.evidence.size())return false;formal_open_point_view q;q.base=p.exact_base.value_or(exact_point3{});q.infinitesimal_direction=p.direction;for(std::size_t i=0;i<p.constraints.size();++i){auto s=plane_side(p.constraints[i].plane,q.base);if(s==exact_sign::zero){auto n=exact_vector3{exact_scalar(p.constraints[i].plane.a,big_uint(1)),exact_scalar(p.constraints[i].plane.b,big_uint(1)),exact_scalar(p.constraints[i].plane.c,big_uint(1))};s=dot(n,p.direction).sign();if(p.constraints[i].plane.oriented==orientation_parity::opposite)s=s==exact_sign::positive?exact_sign::negative:s==exact_sign::negative?exact_sign::positive:s;}if(s!=p.constraints[i].required||s!=p.evidence[i])return false;}}
  return semantic(a,false)==a.canonical_bytes&&semantic(a,true)==a.quotient_bytes&&invocation(a)==a.artifact_bytes&&artifact_digest_for(a)==a.artifact_digest&&a.certificate.semantic_digest==domain_digest({{'Y','G','B','C','A','N','0','8'}},a.canonical_bytes);
}
} // namespace

template <class T, class I>
status_or<std::shared_ptr<const published_artifact<arrangement_complex<T, I>>>>
build_global_arrangement(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::global_arrangement, "cancelled");
    if (auto old = ctx.artifacts().latest(artifact_slot::arrangement_complex))
      return std::static_pointer_cast<
          const published_artifact<arrangement_complex<T, I>>>(old);
    auto rr = refine_source_facets(ctx);
    if (!rr.has_value())
      return rr.error();
    performance_scope producer(ctx.performance_collector_for_internal_use(),
                               boolean_stage::global_arrangement,
                               performance_role::producer);
    auto refined = rr.value();
    if (ctx.artifacts().latest_generation(
            artifact_slot::refined_facet_patches) != refined->generation)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::global_arrangement, "stale_refinement");
    stage_transaction<arrangement_complex<T, I>> tx(
        ctx.owner(), boolean_stage::global_arrangement,
        artifact_slot::arrangement_complex,
        std::make_unique<arrangement_complex<T, I>>(),
        ctx.performance_collector_for_internal_use());
    auto &a = tx.draft();
    a.owner = ctx.owner();
    a.setup_digest = ctx.replay().setup;
    a.refined_digest = refined->artifact_digest;
    a.refined = refined;
    a.symbolic = refined->payload->symbolic;
    a.validated = refined->payload->validated;
    a.symbolic_digest = a.symbolic->artifact_digest;
    a.validated_digest = a.validated->artifact_digest;
    a.kernel_policy_digest = refined->payload->kernel_policy_digest;
    a.constructions = refined->payload->constructions;
    a.classification = ctx.options().classification.strategy;
    std::map<symbolic_vertex_id, global_vertex_id> vertices;
    for (const auto &f : refined->payload->facets)
      for (const auto &v : f.vertices) {
        auto it = vertices.find(v.symbolic);
        if (it == vertices.end()) {
          auto id = global_vertex_id::from_canonical_value(vertices.size());
          vertices[v.symbolic] = id;
          a.vertices.push_back({id, v.symbolic, {}, {}, {}});
          it = vertices.find(v.symbolic);
        }
        a.vertices[it->second.value_for_debug()].local_occurrences.push_back(
            v.id);
      }
    using occurrence_key = std::tuple<global_vertex_id, operand_id, shell_id>;
    std::set<occurrence_key> occurrence_keys;
    for (const auto &f : refined->payload->facets)
      for (const auto &v : f.vertices)
        occurrence_keys.emplace(vertices[v.symbolic], f.operand, f.shell);
    std::map<occurrence_key, vertex_occurrence_id> occurrence_ids;
    for (const auto &key : occurrence_keys) {
      auto id = vertex_occurrence_id::from_canonical_value(
          a.vertex_occurrences.size());
      occurrence_ids.emplace(key, id);
      vertex_occurrence o;
      o.id = id;
      o.vertex = std::get<0>(key);
      o.operand = std::get<1>(key);
      o.shell = std::get<2>(key);
      a.vertex_occurrences.push_back(std::move(o));
      a.vertices[std::get<0>(key).value_for_debug()].occurrences.push_back(id);
    }
    std::map<std::pair<facet_id, local_vertex_id>, vertex_occurrence_id>
        local_occurrence;
    for (const auto &f : refined->payload->facets)
      for (const auto &v : f.vertices) {
        auto id = occurrence_ids.at({vertices[v.symbolic], f.operand, f.shell});
        a.vertex_occurrences[id.value_for_debug()].local_germs.push_back(v.id);
        local_occurrence[{f.facet, v.id.id}] = id;
      }
    std::map<std::pair<global_vertex_id, global_vertex_id>,
             global_atomic_edge_id>
        edges;
    std::map<std::pair<facet_id, local_atomic_edge_id>, global_atomic_edge_id>
        local_edge;
    for (const auto &f : refined->payload->facets)
      for (const auto &e : f.edges) {
        auto x = vertices[f.vertices[e.lower.id.value_for_debug()].symbolic],
             y = vertices[f.vertices[e.upper.id.value_for_debug()].symbolic];
        if (y < x)
          std::swap(x, y);
        auto key = std::make_pair(x, y);
        auto it = edges.find(key);
        if (it == edges.end()) {
          auto id = global_atomic_edge_id::from_canonical_value(edges.size());
          global_edge_kind kind =
              e.artificial
                  ? global_edge_kind::transparent_artificial
                  : e.canonical_interval ? global_edge_kind::intersection_seam
                                         : global_edge_kind::source_edge;
          edges[key] = id;
          a.edges.push_back({id, x, y, kind, {}, {}});
          it = edges.find(key);
        }
        auto &ge = a.edges[it->second.value_for_debug()];
        ge.local_occurrences.push_back(e.id);
        if (e.canonical_interval) {
          ge.kind = global_edge_kind::intersection_seam;
          ge.curves.push_back(*e.canonical_interval);
        }
        local_edge[{f.facet, e.id.id}] = ge.id;
      }
    for (auto &e : a.edges) {
      std::sort(e.curves.begin(), e.curves.end());
      e.curves.erase(std::unique(e.curves.begin(), e.curves.end()),
                     e.curves.end());
      a.vertices[e.lower.value_for_debug()].incident_edges.push_back(e.id);
      a.vertices[e.upper.value_for_debug()].incident_edges.push_back(e.id);
    }
    std::map<facet_id, facet_id> sheet_component;
    for (const auto &seed : a.validated->payload->facets)
      if (!sheet_component.count(seed.id)) {
        std::vector<facet_id> q{seed.id}, part;
        sheet_component[seed.id] = seed.id;
        while (!q.empty()) {
          auto id = q.back();
          q.pop_back();
          part.push_back(id);
          const auto &f = a.validated->payload->facets[id.value_for_debug()];
          for (auto n : f.neighbors) {
            const auto &g = a.validated->payload->facets[n.value_for_debug()];
            if (g.operand == f.operand && g.shell == f.shell &&
                intersect_planes(f.plane, g.plane).kind ==
                    plane_plane_kind::coincident_same &&
                !sheet_component.count(n)) {
              sheet_component[n] = seed.id;
              q.push_back(n);
            }
          }
        }
        auto least = *std::min_element(part.begin(), part.end());
        for (auto id : part)
          sheet_component[id] = least;
      }
    std::map<std::tuple<operand_id, shell_id, facet_id, canonical_plane_key>,
             source_sheet_member_id>
        members;
    std::map<std::pair<facet_id, local_patch_id>, sheet_use_id> local_use;
    std::map<std::pair<facet_id, local_face_id>,
             std::vector<sheet_use_id>>
        uses_by_parent_face;
    struct patch_key {
      canonical_plane_key plane;
      std::vector<symbolic_vertex_id> outer;
      std::vector<std::vector<symbolic_vertex_id>> holes;
      bool operator<(const patch_key &o) const {
        return std::tie(plane, outer, holes) <
               std::tie(o.plane, o.outer, o.holes);
      }
    };
    std::map<patch_key, global_patch_id> patches;
    for (const auto &f : refined->payload->facets) {
      const auto &vf = a.validated->payload->facets[f.facet.value_for_debug()];
      for (const auto &p : f.patches) {
        auto mk = std::make_tuple(f.operand, f.shell, sheet_component[f.facet],
                                  canonical_plane_key{vf.plane, true});
        auto mi = members.find(mk);
        if (mi == members.end()) {
          auto id =
              source_sheet_member_id::from_canonical_value(members.size());
          members[mk] = id;
          a.sheet_members.push_back({id, f.operand, f.shell, {}, {}});
          mi = members.find(mk);
        }
        auto &m = a.sheet_members[mi->second.value_for_debug()];
        m.facets.push_back(f.facet);
        m.local_patches.push_back(p.id);
        auto cycle = [&](local_boundary_walk_ref wr) {
          std::vector<symbolic_vertex_id> out;
          for (auto hr : f.walks[wr.id.value_for_debug()].halfedges)
            out.push_back(f.vertices[f.halfedges[hr.id.value_for_debug()]
                                         .origin.id.value_for_debug()]
                              .symbolic);
          return normalized_cycle(std::move(out));
        };
        patch_key pk{canonical_plane_key{vf.plane, false}, cycle(p.outer), {}};
        for (auto h : p.holes)
          pk.holes.push_back(cycle(h));
        std::sort(pk.holes.begin(), pk.holes.end());
        auto pi = patches.find(pk);
        if (pi == patches.end()) {
          auto id = global_patch_id::from_canonical_value(patches.size());
          global_patch gp;
          gp.id = id;
          gp.plane = vf.plane;
          gp.projected_double_area = p.signed_double_area;
          for (auto sv : pk.outer)
            gp.outer.push_back(vertices[sv]);
          for (const auto &hole : pk.holes) {
            gp.holes.push_back({});
            for (auto sv : hole)
              gp.holes.back().push_back(vertices[sv]);
          }
          patches.emplace(pk, id);
          a.patches.push_back(std::move(gp));
          pi = patches.find(pk);
        }
        auto uid = sheet_use_id::from_canonical_value(a.sheet_uses.size());
        sheet_patch_use u;
        u.id = uid;
        u.member = mi->second;
        u.patch = pi->second;
        u.operand = f.operand;
        u.shell = f.shell;
        u.facet = f.facet;
        u.local_patch = p.id;
        u.source_plane_agrees =
            vf.plane.oriented ==
            a.patches[pi->second.value_for_debug()].plane.oriented;
        u.occupied_side = u.source_plane_agrees ? patch_plane_side::negative
                                                : patch_plane_side::positive;
        a.sheet_uses.push_back(std::move(u));
        a.patches[pi->second.value_for_debug()].uses.push_back(uid);
        local_use[{f.facet, p.id.id}] = uid;
        uses_by_parent_face[{f.facet, p.parent_face.id}].push_back(uid);
      }
    }
    for (auto &m : a.sheet_members) {
      std::sort(m.facets.begin(), m.facets.end());
      m.facets.erase(std::unique(m.facets.begin(), m.facets.end()),
                     m.facets.end());
    }
    std::vector<std::vector<sheet_use_id>> uses_by_facet(
        a.validated->payload->facets.size());
    for (const auto &use : a.sheet_uses)
      uses_by_facet[use.facet.value_for_debug()].push_back(use.id);
    std::map<std::pair<facet_id, local_halfedge_id>, global_halfedge_id>
        local_halfedge;
    for (const auto &f : refined->payload->facets)
      for (const auto &p : f.patches) {
        auto uid = local_use[{f.facet, p.id.id}];
        const bool reverse_source =
            a.validated->payload->facets[f.facet.value_for_debug()]
                .projected_double_area < exact_scalar(0);
        std::vector<local_boundary_walk_ref> walks{p.outer};
        walks.insert(walks.end(), p.holes.begin(), p.holes.end());
        for (auto wr : walks) {
          auto local_cycle = f.walks[wr.id.value_for_debug()].halfedges;
          if (reverse_source)
            std::reverse(local_cycle.begin(), local_cycle.end());
          std::vector<global_halfedge_id> cycle;
          for (auto hr : local_cycle) {
            const auto &lh = f.halfedges[hr.id.value_for_debug()];
            auto id =
                global_halfedge_id::from_canonical_value(a.halfedges.size());
            auto ol = reverse_source ? lh.destination : lh.origin,
                 dl = reverse_source ? lh.origin : lh.destination;
            auto o = local_occurrence.at({f.facet, ol.id}),
                 d = local_occurrence.at({f.facet, dl.id});
            auto og = a.vertex_occurrences[o.value_for_debug()].vertex,
                 dg = a.vertex_occurrences[d.value_for_debug()].vertex;
            a.halfedges.push_back({id,
                                   {},
                                   {},
                                   {},
                                   uid,
                                   local_edge[{f.facet, lh.edge.id}],
                                   og,
                                   dg,
                                   o,
                                   d,
                                   lh.id});
            a.vertex_occurrences[o.value_for_debug()]
                .incident_halfedges.push_back(id);
            local_halfedge[{f.facet, lh.id.id}] = id;
            cycle.push_back(id);
          }
          for (std::size_t i = 0; i < cycle.size(); ++i) {
            auto &h = a.halfedges[cycle[i].value_for_debug()];
            h.next = cycle[(i + 1) % cycle.size()];
            h.previous = cycle[(i + cycle.size() - 1) % cycle.size()];
          }
          a.sheet_uses[uid.value_for_debug()].boundary.insert(
              a.sheet_uses[uid.value_for_debug()].boundary.end(), cycle.begin(),
              cycle.end());
          ++a.certificate.patch_cycles;
        }
      }
    std::vector<std::vector<sheet_use_id>> uses_by_edge(a.edges.size());
    using mate_group_key =
        std::tuple<global_atomic_edge_id, operand_id, shell_id,
                   vertex_occurrence_id, vertex_occurrence_id>;
    std::map<mate_group_key, std::vector<global_halfedge_id>> mate_groups;
    for (const auto &h : a.halfedges) {
      const auto &u = a.sheet_uses[h.use.value_for_debug()];
      uses_by_edge[h.edge.value_for_debug()].push_back(h.use);
      mate_groups[{h.edge, u.operand, u.shell, h.origin_occurrence,
                   h.destination_occurrence}]
          .push_back(h.id);
      performance_count(performance_counter::global_incidence_records);
    }
    for (auto &entry : mate_groups)
      std::sort(entry.second.begin(), entry.second.end());
    for (const auto &entry : mate_groups) {
      const auto &key = entry.first;
      if (!(std::get<3>(key) < std::get<4>(key)))
        continue;
      mate_group_key reverse{std::get<0>(key), std::get<1>(key),
                             std::get<2>(key), std::get<4>(key),
                             std::get<3>(key)};
      auto opposite = mate_groups.find(reverse);
      performance_count(performance_counter::global_index_lookups);
      if (opposite == mate_groups.end() ||
          opposite->second.size() != entry.second.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::global_arrangement,
                          "unmatched_sheet_halfedge");
      for (std::size_t i = 0; i < entry.second.size(); ++i) {
        const auto x = entry.second[i], y = opposite->second[i];
        a.halfedges[x.value_for_debug()].sheet_mate = y;
        a.halfedges[y.value_for_debug()].sheet_mate = x;
        ++a.certificate.mate_pairs;
      }
    }
    for (const auto &entry : mate_groups)
      if (!(std::get<3>(entry.first) < std::get<4>(entry.first)) &&
          !(std::get<4>(entry.first) < std::get<3>(entry.first)))
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::global_arrangement,
                          "degenerate_sheet_halfedge");
    for (auto &uses : uses_by_edge) {
      std::sort(uses.begin(), uses.end());
      uses.erase(std::unique(uses.begin(), uses.end()), uses.end());
    }
    auto radial = [&](const global_atomic_edge &e,
                      const std::vector<sheet_use_id> &uses)
        -> status_or<exact_carrier_radial_order> {
      const auto &p = a.symbolic->payload
                          ->vertices[a.vertices[e.lower.value_for_debug()]
                                         .symbolic.value_for_debug()]
                          .point;
      const auto &q = a.symbolic->payload
                          ->vertices[a.vertices[e.upper.value_for_debug()]
                                         .symbolic.value_for_debug()]
                          .point;
      std::vector<exact_plane3> planes;
      for (auto u : uses)
        planes.push_back(a.validated->payload
                             ->facets[a.sheet_uses[u.value_for_debug()]
                                          .facet.value_for_debug()]
                             .plane);
      return rank_planes_around_carrier(q - p, planes);
    };
    for (const auto &e : a.edges) {
      const auto &uses = uses_by_edge[e.id.value_for_debug()];
      performance_count(performance_counter::global_index_lookups);
      if (uses.size() < 2)
        continue;
      auto order = radial(e, uses);
      if (!order.has_value())
        return order.error();
      if (e.kind == global_edge_kind::intersection_seam) {
        seam_record s;
        s.id = seam_id::from_canonical_value(a.seams.size());
        s.edge = e.id;
        s.incident_uses = uses;
        for (const auto &layer : order.value().layers) {
          radial_layer out;
          for (auto i : layer.members)
            out.uses.push_back(uses[i]);
          s.radial_layers.push_back(std::move(out));
        }
        if (s.radial_layers.size() > 1)
          for (std::size_t i = 0; i < s.radial_layers.size(); ++i) {
            auto id =
                seam_sector_id::from_canonical_value(a.seam_sectors.size());
            a.seam_sectors.push_back(
                {id, s.id, i, (i + 1) % s.radial_layers.size(), {}});
            s.sectors.push_back(id);
          }
        a.certificate.seam_sectors += s.sectors.size();
        a.seams.push_back(std::move(s));
      } else if (e.kind == global_edge_kind::source_edge) {
        for (std::size_t i = 0; i < order.value().layers.size(); ++i) {
          source_edge_sector s;
          s.id = source_edge_sector_id::from_canonical_value(
              a.source_edge_sectors.size());
          s.edge = e.id;
          s.lower_layer = i;
          s.upper_layer = (i + 1) % order.value().layers.size();
          for (auto j : order.value().layers[i].members)
            s.incident_uses.push_back(uses[j]);
          for (auto j : order.value().layers[s.upper_layer].members)
            s.incident_uses.push_back(uses[j]);
          std::sort(s.incident_uses.begin(), s.incident_uses.end());
          s.incident_uses.erase(
              std::unique(s.incident_uses.begin(), s.incident_uses.end()),
              s.incident_uses.end());
          a.source_edge_sectors.push_back(std::move(s));
        }
      }
    }
    std::vector<std::optional<seam_id>> seam_by_edge(a.edges.size());
    for (const auto &seam : a.seams)
      seam_by_edge[seam.edge.value_for_debug()] = seam.id;
    std::vector<std::vector<source_edge_sector_id>> source_sectors_by_edge(
        a.edges.size());
    for (const auto &sector : a.source_edge_sectors)
      source_sectors_by_edge[sector.edge.value_for_debug()].push_back(sector.id);
    for (auto &p : a.patches)
      if (p.uses.size() > 1) {
        coincident_group g;
        g.id = coincident_group_id::from_canonical_value(
            a.coincident_groups.size());
        g.patch = p.id;
        g.members = p.uses;
        a.certificate.coincident_memberships += g.members.size();
        a.coincident_groups.push_back(std::move(g));
      }
    std::vector<std::optional<coincident_group_id>> coincidence_by_patch(
        a.patches.size());
    for (const auto &group : a.coincident_groups)
      coincidence_by_patch[group.patch.value_for_debug()] = group.id;
    for (const auto &p : a.patches)
      for (unsigned side = 0; side < 2; ++side) {
        auto id = patch_side_id::from_canonical_value(a.patch_sides.size());
        a.patch_sides.push_back({id, p.id, static_cast<patch_plane_side>(side),
                                 open_region_component_id::from_canonical_value(
                                     id.value_for_debug())});
      }
    for (const auto &p : a.patches) {
      side_transition t;
      t.id = side_transition_id::from_canonical_value(a.transitions.size());
      t.from = patch_side_id::from_canonical_value(2 * p.id.value_for_debug());
      t.to =
          patch_side_id::from_canonical_value(2 * p.id.value_for_debug() + 1);
      t.region_crossing = true;
      const auto group = coincidence_by_patch[p.id.value_for_debug()];
      performance_count(performance_counter::global_index_lookups);
      if (!group) {
        t.kind = side_transition_kind::sheet_crossing;
        t.uses = p.uses;
      } else {
        t.kind = side_transition_kind::coincidence_crossing;
        t.coincidence = *group;
        t.uses = a.coincident_groups[group->value_for_debug()].members;
      }
      a.transitions.push_back(std::move(t));
    }
    a.certificate.side_transitions = a.transitions.size();
    auto normal = [](const exact_plane3 &p) {
      exact_vector3 n{exact_scalar(p.a, big_uint(1)),
                      exact_scalar(p.b, big_uint(1)),
                      exact_scalar(p.c, big_uint(1))};
      return p.oriented == orientation_parity::opposite ? n * exact_scalar(-1)
                                                        : n;
    };
    struct link_preparation {
      vertex_germ_kind germ = vertex_germ_kind::terminal_contact;
      std::vector<exact_vector3> representative_directions;
      std::vector<seam_sector_id> seam_continuations;
      std::vector<source_edge_sector_id> source_edge_continuations;
      exact_vector3 witness_direction;
      std::vector<exact_sign> witness_evidence;
    };
    std::vector<std::optional<link_preparation>> link_preparations(
        a.vertex_occurrences.size());
    std::vector<deterministic_task> link_tasks;
    link_tasks.reserve(a.vertex_occurrences.size());
    for (std::size_t occurrence_index = 0;
         occurrence_index < a.vertex_occurrences.size(); ++occurrence_index) {
      link_tasks.push_back(
          {a.vertex_occurrences[occurrence_index].id.value_for_debug(),
            [&, occurrence_index](cancellation_token task_cancel) -> status_or<bool> {
              try {
                if (task_cancel.cancelled() || ctx.cancelled())
                 return make_error(boolean_error_code::resource_limit,
                                   boolean_stage::global_arrangement,
                                   "cancelled");
               const auto &o = a.vertex_occurrences[occurrence_index];
               link_preparation prepared;
               std::set<global_atomic_edge_id> incident;
               for (auto h : o.incident_halfedges)
                 incident.insert(a.halfedges[h.value_for_debug()].edge);
               prepared.germ = incident.empty()
                                   ? vertex_germ_kind::terminal_contact
                                   : incident.size() == 1
                                         ? vertex_germ_kind::semicircle
                                         : vertex_germ_kind::wedge;
               std::vector<
                   std::pair<global_atomic_edge_id, exact_vector3>> directions;
               for (auto edge : incident) {
                 const auto &e = a.edges[edge.value_for_debug()];
                 auto other = e.lower == o.vertex ? e.upper : e.lower;
                 const auto &p =
                     a.symbolic->payload
                         ->vertices[a.vertices[o.vertex.value_for_debug()]
                                        .symbolic.value_for_debug()]
                         .point;
                 const auto &q =
                     a.symbolic->payload
                         ->vertices[a.vertices[other.value_for_debug()]
                                        .symbolic.value_for_debug()]
                         .point;
                 directions.push_back({edge, q - p});
               }
               std::vector<std::size_t> representatives;
               for (std::size_t i = 0; i < directions.size(); ++i) {
                 bool duplicate = false;
                 for (auto j : representatives) {
                   performance_count(
                       performance_counter::link_direction_candidates);
                   performance_count(
                       performance_counter::exact_link_direction_tests);
                   const auto c =
                       cross(directions[i].second, directions[j].second);
                   if (c.x.sign() == exact_sign::zero &&
                       c.y.sign() == exact_sign::zero &&
                       c.z.sign() == exact_sign::zero &&
                       dot_sign_exact(directions[i].second,
                                      directions[j].second) ==
                           exact_sign::positive) {
                     duplicate = true;
                     break;
                   }
                 }
                 if (!duplicate)
                   representatives.push_back(i);
               }
               prepared.representative_directions.reserve(
                   representatives.size());
               for (auto i : representatives)
                 prepared.representative_directions.push_back(
                     directions[i].second);
               for (auto edge : incident) {
                 const auto seam = seam_by_edge[edge.value_for_debug()];
                 performance_count(performance_counter::global_index_lookups);
                 if (seam) {
                   const auto &sectors =
                       a.seams[seam->value_for_debug()].sectors;
                   prepared.seam_continuations.insert(
                       prepared.seam_continuations.end(), sectors.begin(),
                       sectors.end());
                 }
                 const auto &source_sectors =
                     source_sectors_by_edge[edge.value_for_debug()];
                 performance_count(performance_counter::global_index_lookups);
                 prepared.source_edge_continuations.insert(
                     prepared.source_edge_continuations.end(),
                     source_sectors.begin(), source_sectors.end());
               }
               const auto &pl =
                   a.validated->payload
                       ->facets[o.local_germs.front().facet.value_for_debug()]
                       .plane;
               auto witness = construct_strict_cone_witness(
                   {{normal(pl), exact_sign::positive}});
               if (!witness.has_value())
                 return witness.error();
               prepared.witness_direction = witness.value().direction;
               prepared.witness_evidence = witness.value().evaluations;
               link_preparations[occurrence_index] = std::move(prepared);
               return true;
             } catch (const std::bad_alloc &) {
               return make_error(boolean_error_code::resource_limit,
                                 boolean_stage::global_arrangement,
                                 "arrangement_allocation");
             } catch (const std::exception &e) {
               auto error = make_error(
                   boolean_error_code::internal_invariant_error,
                   boolean_stage::global_arrangement, "arrangement_exception");
               error.detail = e.what();
               return error;
             }
           }});
    }
    cancellation_source link_cancellation;
    auto links =
        ctx.executor().run(std::move(link_tasks), link_cancellation.token());
    if (!links.has_value()) {
      auto error = links.error();
      error.stage = boolean_stage::global_arrangement;
      return error;
    }
    for (std::size_t occurrence_index = 0;
         occurrence_index < a.vertex_occurrences.size(); ++occurrence_index) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::global_arrangement, "cancelled");
      auto &o = a.vertex_occurrences[occurrence_index];
      if (!link_preparations[occurrence_index])
        throw std::logic_error("missing link preparation");
      auto &prepared = *link_preparations[occurrence_index];
      vertex_sector s;
      s.id = vertex_sector_id::from_canonical_value(a.vertex_sectors.size());
      s.vertex = o.vertex;
      s.occurrence = o.id;
      s.region = link_region_id::from_canonical_value(s.id.value_for_debug());
      s.germ = prepared.germ;
      for (auto &d : prepared.representative_directions) {
        auto r = link_ray_id::from_canonical_value(a.link_rays.size()),
             anti = link_ray_id::from_canonical_value(a.link_rays.size() + 1);
        a.link_rays.push_back({r, std::move(d), anti});
        a.link_rays.push_back(
            {anti, a.link_rays.back().direction * exact_scalar(-1), r});
        s.boundary_rays.push_back(r);
        s.boundary_rays.push_back(anti);
      }
      s.seam_continuations = std::move(prepared.seam_continuations);
      s.source_edge_continuations =
          std::move(prepared.source_edge_continuations);
      for (std::size_t i = 0; i < s.boundary_rays.size(); ++i) {
        auto id = link_arc_id::from_canonical_value(a.link_arcs.size());
        a.link_arcs.push_back(
            {id,
             o.id,
             s.boundary_rays[i],
             s.boundary_rays[(i + 1) % s.boundary_rays.size()],
             {}});
        s.boundary_arcs.push_back(id);
      }
      s.witness_direction = std::move(prepared.witness_direction);
      s.witness_evidence = std::move(prepared.witness_evidence);
      o.link_regions.push_back(s.id);
      a.vertex_sectors.push_back(std::move(s));
    }
    std::vector<exact_point3> patch_witnesses;
    patch_witnesses.reserve(a.patches.size());
    for(const auto&p:a.patches){
      const auto projected=project_patch(a,p);
      patch_witnesses.push_back(patch_interior(p,projected));
    }
    for (const auto &side : a.patch_sides) {
      const auto &p = a.patches[side.patch.value_for_debug()];
      strict_cone_constraint c{normal(p.plane),
                               side.side == patch_plane_side::positive
                                   ? exact_sign::positive
                                   : exact_sign::negative};
      auto witness = construct_strict_cone_witness({c});
      if (!witness.has_value())
        return witness.error();
      open_probe_descriptor probe;
      probe.side = side.id;
      probe.component = side.component;
      probe.base_kind = probe_base_stratum_kind::patch_side;
      probe.base_id = side.id.value_for_debug();
      probe.exact_base = patch_witnesses[p.id.value_for_debug()];
      probe.direction = witness.value().direction;
      probe.constraints.push_back({p.plane, c.required});
      probe.evidence = witness.value().evaluations;
      probe.formula_version =
          ctx.options().classification.probe_formula_version;
      a.probes.push_back(std::move(probe));
    }
    if (a.patches.empty()) {
      open_probe_descriptor probe;
      probe.side = patch_side_id::from_canonical_value(0);
      probe.component = open_region_component_id::from_canonical_value(0);
      probe.base_kind = probe_base_stratum_kind::universe;
      probe.exact_base =
          exact_point3{exact_scalar(0), exact_scalar(0), exact_scalar(0)};
      probe.direction = {exact_scalar(1), exact_scalar(0), exact_scalar(0)};
      a.probes.push_back(std::move(probe));
    }
    auto preserving = [&](side_transition_kind kind, sheet_use_id use) {
      const auto &u = a.sheet_uses[use.value_for_debug()];
      side_transition t;
      t.id = side_transition_id::from_canonical_value(a.transitions.size());
      t.kind = kind;
      t.from = t.to =
          patch_side_id::from_canonical_value(2 * u.patch.value_for_debug());
      t.uses = {use};
      a.transitions.push_back(std::move(t));
    };
    for (const auto &s : a.seam_sectors) {
      const auto &seam = a.seams[s.seam.value_for_debug()];
      if (!seam.incident_uses.empty())
        preserving(side_transition_kind::seam_sector,
                   seam.incident_uses.front());
    }
    for (const auto &s : a.source_edge_sectors)
      if (!s.incident_uses.empty())
        preserving(side_transition_kind::source_edge_sector,
                   s.incident_uses.front());
    for (const auto &s : a.vertex_sectors)
      if (!a.vertices[s.vertex.value_for_debug()].local_occurrences.empty()) {
        auto facet = a.vertices[s.vertex.value_for_debug()]
                         .local_occurrences.front()
                         .facet;
        performance_count(performance_counter::global_index_lookups);
        const auto &uses = uses_by_facet[facet.value_for_debug()];
        if (!uses.empty())
          preserving(side_transition_kind::vertex_sector, uses.front());
      }
    a.certificate.side_transitions = a.transitions.size();
    auto addmap = [&](local_map_kind k, facet_id f, std::uint64_t id,
                      std::vector<std::uint64_t> g, bool incidence = false) {
      a.local_maps.push_back({k, f, id, std::move(g), incidence});
    };
    for (const auto &f : refined->payload->facets) {
      a.certificate.local_vertices += f.vertices.size();
      a.certificate.local_edges += f.edges.size();
      a.certificate.local_halfedges += f.halfedges.size();
      a.certificate.local_walks += f.walks.size();
      a.certificate.local_faces += f.faces.size();
      a.certificate.local_patches += f.patches.size();
      for (const auto &v : f.vertices) {
        auto g = vertices[v.symbolic].value_for_debug();
        addmap(local_map_kind::vertex, f.facet, v.id.id.value_for_debug(), {g});
        addmap(local_map_kind::point_incidence, f.facet,
               v.id.id.value_for_debug(), {g});
      }
      for (const auto &e : f.edges)
        addmap(e.artificial ? local_map_kind::artificial_cut
                            : local_map_kind::edge,
               f.facet, e.id.id.value_for_debug(),
               {local_edge[{f.facet, e.id.id}].value_for_debug()});
      for (const auto &h : f.halfedges) {
        auto it = local_halfedge.find({f.facet, h.id.id});
        if (it == local_halfedge.end())
          addmap(local_map_kind::halfedge, f.facet, h.id.id.value_for_debug(),
                 {local_edge[{f.facet, h.edge.id}].value_for_debug()}, true);
        else
          addmap(local_map_kind::halfedge, f.facet, h.id.id.value_for_debug(),
                 {it->second.value_for_debug()});
      }
      for (const auto &w : f.walks) {
        std::vector<std::uint64_t> g;
        for (auto h : w.halfedges) {
          auto it = local_halfedge.find({f.facet, h.id});
          if (it != local_halfedge.end())
            g.push_back(it->second.value_for_debug());
        }
        addmap(local_map_kind::boundary_walk, f.facet,
               w.id.id.value_for_debug(), std::move(g), g.empty());
      }
      for (const auto &face : f.faces) {
        std::vector<std::uint64_t> g;
        const auto &uses = uses_by_parent_face[{f.facet, face.id.id}];
        for (auto use : uses)
          g.push_back(use.value_for_debug());
        performance_count(performance_counter::global_index_lookups);
        addmap(local_map_kind::face, f.facet, face.id.id.value_for_debug(),
               std::move(g), g.empty());
      }
      for (const auto &p : f.patches)
        addmap(local_map_kind::patch, f.facet, p.id.id.value_for_debug(),
               {local_use[{f.facet, p.id.id}].value_for_debug()});
      for (std::size_t i = 0; i < f.source_boundary.size(); ++i) {
        std::vector<std::uint64_t> g;
        for (auto e : f.source_boundary[i].edges)
          g.push_back(local_edge[{f.facet, e.id}].value_for_debug());
        addmap(local_map_kind::source_chain, f.facet, i, std::move(g));
      }
      for (std::size_t i = 0; i < f.constraints.size(); ++i) {
        std::vector<std::uint64_t> g;
        for (auto e : f.constraints[i].edges)
          g.push_back(local_edge[{f.facet, e.id}].value_for_debug());
        addmap(local_map_kind::constraint_chain, f.facet, i, std::move(g));
      }
    }
    std::vector<resource_reservation> charges;
    auto reserve = [&](resource_kind k, std::uint64_t n) -> status_or<bool> {
      auto r = ctx.accountant().reserve_scoped(
          k, n, boolean_stage::global_arrangement);
      if (!r.has_value())
        return r.error();
      charges.push_back(std::move(r.value()));
      return true;
    };
    for (const auto &q :
         {std::make_pair(resource_kind::global_vertices,
                         std::uint64_t(a.vertices.size())),
          std::make_pair(resource_kind::vertex_occurrences,
                         std::uint64_t(a.vertex_occurrences.size())),
          std::make_pair(resource_kind::global_atomic_edges,
                         std::uint64_t(a.edges.size())),
          std::make_pair(resource_kind::global_halfedges,
                         std::uint64_t(a.halfedges.size())),
          std::make_pair(resource_kind::global_patches,
                         std::uint64_t(a.patches.size())),
          std::make_pair(resource_kind::source_sheet_members,
                         std::uint64_t(a.sheet_members.size())),
          std::make_pair(resource_kind::sheet_uses,
                         std::uint64_t(a.sheet_uses.size())),
          std::make_pair(resource_kind::seams, std::uint64_t(a.seams.size())),
          std::make_pair(resource_kind::seam_sectors,
                         std::uint64_t(a.seam_sectors.size())),
          std::make_pair(resource_kind::source_edge_sectors,
                         std::uint64_t(a.source_edge_sectors.size())),
          std::make_pair(resource_kind::coincident_memberships,
                         a.certificate.coincident_memberships),
          std::make_pair(resource_kind::side_nodes,
                         std::uint64_t(a.patch_sides.size())),
          std::make_pair(resource_kind::vertex_sectors,
                         std::uint64_t(a.vertex_sectors.size())),
          std::make_pair(resource_kind::link_rays,
                         std::uint64_t(a.link_rays.size())),
          std::make_pair(resource_kind::link_arcs,
                         std::uint64_t(a.link_arcs.size())),
          std::make_pair(resource_kind::link_regions,
                         std::uint64_t(a.vertex_sectors.size())),
          std::make_pair(resource_kind::side_transitions,
                         std::uint64_t(a.transitions.size())),
          std::make_pair(resource_kind::probe_descriptors,
                         std::uint64_t(a.probes.size())),
          std::make_pair(resource_kind::mapping_entries,
                         std::uint64_t(a.local_maps.size())),
          std::make_pair(resource_kind::arrangement_certificate_entries,
                         std::uint64_t(12))}) {
      auto r = reserve(q.first, q.second);
      if (!r.has_value())
        return r.error();
    }
    a.quotient_bytes = semantic(a, true);
    a.canonical_bytes = semantic(a, false);
    a.certificate.semantic_digest = domain_digest(
        {{'Y', 'G', 'B', 'C', 'A', 'N', '0', '8'}}, a.canonical_bytes);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::global_arrangement,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::arrangement_complex, type_tag<T, I>(),
        arrangement_complex_schema, ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    verification_environment_view env{ctx.owner(),
                                      ctx.replay().setup,
                                      ctx.contract().selected_operation(),
                                      &ctx.options(),
                                      ctx.platform().coordinate,
                                      ctx.platform().index,
                                      &ctx.kernel(),
                                      {},
                                      &ctx.accountant(),
                                      [&] { return ctx.cancelled(); }};
    for (auto &charge : charges)
      tx.stage_reservation(std::move(charge));
    performance_count(performance_counter::global_patches,
                      a.patches.size());
    performance_count(performance_counter::global_edges, a.edges.size());
    performance_count(performance_counter::global_uses,
                      a.sheet_uses.size());
    performance_count(performance_counter::link_entities,
                      a.link_rays.size() + a.link_arcs.size() +
                          a.vertex_sectors.size());
    for (const auto &probe : a.probes)
      performance_count(performance_counter::probe_constraints,
                        probe.constraints.size());
    producer.finish();
    auto ok = tx.freeze_and_verify(type_tag<T, I>(), arrangement_complex_schema,
                                   1, a.artifact_digest, spec.value(), env,
                                   ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::global_arrangement, "cancelled");
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::global_arrangement,
                      "arrangement_allocation");
  } catch (const std::exception &e) {
    auto x =
        make_error(boolean_error_code::internal_invariant_error,
                   boolean_stage::global_arrangement, "arrangement_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T,I) template status_or<std::shared_ptr<const published_artifact<arrangement_complex<T,I>>>> build_global_arrangement(boolean_context<T,I>&)
INST(float,std::uint32_t);INST(float,std::uint64_t);INST(double,std::uint32_t);INST(double,std::uint64_t);
#undef INST
} }
