#include "YgorMeshesBooleanInputTopology.h"
#include <algorithm>
#include <map>
#include <new>
#include <set>

#if defined(__FAST_MATH__)
#error "Boolean input topology must not be compiled with fast-math"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Boolean input topology must not assume finite-only arithmetic"
#endif

namespace ygor { namespace mesh_boolean {
namespace{
template<class T,class I>using artifact_t=validated_operands<T,I>;
boolean_error input_error(input_validation_subcode c,const char*k){return make_error(boolean_error_code::input_contract_error,boolean_stage::input_validation,k,static_cast<std::uint32_t>(c));}
template<class T>exact_point3 point(const decoded_coordinate<T>&x,const decoded_coordinate<T>&y,const decoded_coordinate<T>&z){return{x.value,y.value,z.value};}
void enc_point(canonical_encoder&e,const exact_point3&p){p.x.encode(e);p.y.encode(e);p.z.encode(e);}
void enc_plane(canonical_encoder&e,const exact_plane3&p){p.a.encode(e);p.b.encode(e);p.c.encode(e);p.d.encode(e);e.byte(static_cast<std::uint8_t>(p.oriented));}
void enc_box(canonical_encoder&,const exact_box3&);
template <class T, class I>
std::vector<std::uint8_t> encode_artifact(const artifact_t<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBVAT02";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(validated_operands_schema);
  e.u64(validated_operands_type_tag);
  for (const auto &o : a.operands) {
    e.id(o.operand);
    e.u64(o.raw_vertex_count);
    e.u64(o.raw_face_count);
    e.u64(o.vertices.size());
    e.u64(o.facets.size());
    e.u64(o.shells.size());
    e.boolean(o.bounds.has_value());
    if (o.bounds)
      enc_box(e, *o.bounds);
  }
  e.u64(a.vertices.size());
  for (const auto &v : a.vertices) {
    e.id(v.id);
    e.id(v.operand);
    enc_point(e, v.exact_coordinate);
    e.id(v.shell);
    e.u64(v.ordered_outgoing_link.size());
    for (auto h : v.ordered_outgoing_link)
      e.id(h);
  }
  e.u64(a.facets.size());
  for (const auto &f : a.facets) {
    e.id(f.id);
    e.id(f.operand);
    e.u64(f.ring.size());
    for (auto v : f.ring)
      e.id(v);
    enc_plane(e, f.plane);
    e.byte(static_cast<std::uint8_t>(f.projection));
    f.projected_double_area.encode(e);
    enc_box(e, f.bounds);
    e.u64(f.triangles.size());
    for (const auto &t : f.triangles) {
      e.id(t[0]);
      e.id(t[1]);
      e.id(t[2]);
    }
    e.u64(f.neighbors.size());
    for (auto neighbor : f.neighbors)
      e.id(neighbor);
    e.id(f.shell);
  }
  e.u64(a.edge_uses.size());
  for (const auto &h : a.edge_uses) {
    e.id(h.id);
    e.id(h.origin);
    e.id(h.destination);
    e.id(h.facet);
    e.id(h.twin);
    e.id(h.edge);
    e.id(h.shell);
  }
  e.u64(a.edges.size());
  for (const auto &x : a.edges) {
    e.id(x.id);
    e.id(x.first);
    e.id(x.second);
    e.id(x.uses[0]);
    e.id(x.uses[1]);
    e.id(x.shell);
  }
  e.u64(a.shells.size());
  for (const auto &s : a.shells) {
    e.id(s.id);
    e.id(s.operand);
    e.u64(s.facets.size());
    for (auto f : s.facets)
      e.id(f);
    s.oriented_six_volume.encode(e);
    enc_box(e, s.bounds);
    e.boolean(s.parent.has_value());
    if (s.parent)
      e.id(*s.parent);
    e.u64(s.children.size());
    for (auto child : s.children)
      e.id(child);
    e.u32(s.depth);
    e.byte(static_cast<std::uint8_t>(s.orientation));
    e.byte(static_cast<std::uint8_t>(s.contribution));
  }
  e.u64(a.evidence.size());
  for (const auto &v : a.evidence) {
    e.u16(v.schema);
    e.byte(static_cast<std::uint8_t>(v.kind));
    e.id(v.operand);
    e.u64(v.checked);
    e.raw(v.evidence_digest.bytes.data(), v.evidence_digest.bytes.size());
  }
  return e.bytes();
}
template<class T,class I>digest artifact_digest_for(const artifact_t<T,I>&a){canonical_encoder e;e.raw(a.setup_digest.bytes.data(),16);e.byte(static_cast<std::uint8_t>(artifact_slot::validated_operands));e.byte_string(encode_artifact(a));return domain_digest({{'Y','G','B','A','R','T','0','1'}},e.bytes());}
exact_scalar area2(const std::vector<exact_point2>&p){exact_scalar a(0);for(std::size_t i=0;i<p.size();++i){const auto&x=p[i];const auto&y=p[(i+1)%p.size()];a=a+x.x*y.y-y.x*x.y;}return a;}
bool adjacent(std::size_t a,std::size_t b,std::size_t n){return a==b||(a+1)%n==b||(b+1)%n==a;}
status_or<bool>simple_ring(const std::vector<exact_point2>&p){for(std::size_t i=0;i<p.size();++i)for(std::size_t j=i+1;j<p.size();++j){if(adjacent(i,j,p.size()))continue;auto r=relate_segments({p[i],p[(i+1)%p.size()]},{p[j],p[(j+1)%p.size()]});if(r.dimension!=intersection_dimension::empty)return input_error(input_validation_subcode::self_intersection,"self_intersecting_facet");}return true;}
bool in_or_on_triangle(const exact_point2&p,const exact_point2&a,const exact_point2&b,const exact_point2&c,exact_sign winding){auto x=orient2d(a,b,p),y=orient2d(b,c,p),z=orient2d(c,a,p);if(winding==exact_sign::positive)return x!=exact_sign::negative&&y!=exact_sign::negative&&z!=exact_sign::negative;return x!=exact_sign::positive&&y!=exact_sign::positive&&z!=exact_sign::positive;}
status_or<std::vector<std::array<std::size_t,3>>>triangulate(const std::vector<exact_point2>&p){std::vector<std::array<std::size_t,3>>out;std::vector<std::size_t>cycle(p.size());for(std::size_t i=0;i<p.size();++i)cycle[i]=i;auto winding=area2(p).sign();while(cycle.size()>3){std::optional<std::size_t>best;std::array<std::size_t,3>best_key{{0,0,0}};for(std::size_t k=0;k<cycle.size();++k){auto a=cycle[(k+cycle.size()-1)%cycle.size()],b=cycle[k],c=cycle[(k+1)%cycle.size()];if(orient2d(p[a],p[b],p[c])!=winding)continue;bool clear=true;for(auto q:cycle)if(q!=a&&q!=b&&q!=c&&in_or_on_triangle(p[q],p[a],p[b],p[c],winding)){clear=false;break;}if(!clear)continue;exact_point2 mid{(p[a].x+p[c].x)/exact_scalar(2),(p[a].y+p[c].y)/exact_scalar(2)};auto loc=classify_point_polygon(mid,p);if(!loc.has_value()||loc.value().kind!=point_region_kind::open_interior)continue;std::array<std::size_t,3>key{{a,b,c}};if(!best||key<best_key){best=k;best_key=key;}}if(!best)return make_error(boolean_error_code::internal_invariant_error,boolean_stage::input_validation,"exact_triangulation_failed");auto k=*best,a=cycle[(k+cycle.size()-1)%cycle.size()],b=cycle[k],c=cycle[(k+1)%cycle.size()];out.push_back({{a,b,c}});cycle.erase(cycle.begin()+k);}if(orient2d(p[cycle[0]],p[cycle[1]],p[cycle[2]])!=winding)return make_error(boolean_error_code::internal_invariant_error,boolean_stage::input_validation,"degenerate_triangulation_tail");out.push_back({{cycle[0],cycle[1],cycle[2]}});exact_scalar sum(0);for(const auto&t:out)sum=sum+(p[t[0]].x*p[t[1]].y-p[t[0]].y*p[t[1]].x+p[t[1]].x*p[t[2]].y-p[t[1]].y*p[t[2]].x+p[t[2]].x*p[t[0]].y-p[t[2]].y*p[t[0]].x);if(out.size()!=p.size()-2||sum!=area2(p))return make_error(boolean_error_code::internal_invariant_error,boolean_stage::input_validation,"triangulation_partition");return out;}
template<class T,class I>std::vector<exact_triangle3>facet_triangles(const artifact_t<T,I>&a,const validated_facet&f){std::vector<exact_triangle3>out;for(const auto&t:f.triangles)out.push_back({a.vertices[t[0].value_for_debug()].exact_coordinate,a.vertices[t[1].value_for_debug()].exact_coordinate,a.vertices[t[2].value_for_debug()].exact_coordinate});return out;}
template<class T,class I>std::vector<exact_triangle3>shell_triangles(const artifact_t<T,I>&a,const validated_shell&s){std::vector<exact_triangle3>out;for(auto id:s.facets){auto q=facet_triangles(a,a.facets[id.value_for_debug()]);out.insert(out.end(),q.begin(),q.end());}return out;}
exact_box3 point_bounds(const std::vector<exact_point3>&points){if(points.empty())throw std::invalid_argument("empty bounds");exact_box3 b{points.front(),points.front()};for(const auto&p:points){if(p.x<b.minimum.x)b.minimum.x=p.x;if(b.maximum.x<p.x)b.maximum.x=p.x;if(p.y<b.minimum.y)b.minimum.y=p.y;if(b.maximum.y<p.y)b.maximum.y=p.y;if(p.z<b.minimum.z)b.minimum.z=p.z;if(b.maximum.z<p.z)b.maximum.z=p.z;}return b;}
bool equal_box(const exact_box3&a,const exact_box3&b){return a.minimum==b.minimum&&a.maximum==b.maximum;}
void enc_box(canonical_encoder&e,const exact_box3&b){enc_point(e,b.minimum);enc_point(e,b.maximum);}
template <class T, class I>
status_or<std::vector<std::uint8_t>>
semantic_operand_bytes(const artifact_t<T, I> &a, operand_id role,
                       resource_accountant *accountant = nullptr,
                       const std::function<bool()> &cancelled = {}) {
  std::vector<std::vector<std::uint8_t>> vertices, facets, shells;
  for (const auto &v : a.vertices)
    if (v.operand == role) {
      canonical_encoder e;
      enc_point(e, v.exact_coordinate);
      vertices.push_back(e.bytes());
    }
  for (const auto &f : a.facets)
    if (f.operand == role) {
      std::vector<std::vector<std::uint8_t>> cycle;
      for (auto id : f.ring) {
        canonical_encoder p;
        enc_point(p, a.vertices[id.value_for_debug()].exact_coordinate);
        cycle.push_back(p.bytes());
      }
      std::vector<std::uint8_t> best;
      for (std::size_t shift = 0; shift < cycle.size(); ++shift) {
        canonical_encoder q;
        q.u64(cycle.size());
        for (std::size_t k = 0; k < cycle.size(); ++k)
          q.byte_string(cycle[(shift + k) % cycle.size()]);
        if (best.empty() || q.bytes() < best)
          best = q.bytes();
      }
      facets.push_back(std::move(best));
    }
  for (const auto &s : a.shells)
    if (s.operand == role) {
      canonical_encoder e;
      s.oriented_six_volume.encode(e);
      e.u32(s.depth);
      e.byte(static_cast<std::uint8_t>(s.orientation));
      e.byte(static_cast<std::uint8_t>(s.contribution));
      shells.push_back(e.bytes());
    }
  canonical_graph graph;
  std::map<std::uint64_t, std::uint64_t> vertex_nodes, facet_nodes,
      incidence_nodes, shell_nodes;
  auto add_node = [&](std::vector<std::uint8_t> color) {
    auto id = static_cast<std::uint64_t>(graph.nodes.size());
    graph.nodes.push_back({std::move(color)});
    return id;
  };
  for (const auto &v : a.vertices)
    if (v.operand == role) {
      canonical_encoder color;
      color.byte(1);
      enc_point(color, v.exact_coordinate);
      color.u64(v.ordered_outgoing_link.size());
      vertex_nodes.emplace(v.id.value_for_debug(), add_node(color.bytes()));
    }
  for (const auto &f : a.facets)
    if (f.operand == role) {
      canonical_encoder color;
      color.byte(2);
      enc_plane(color, f.plane);
      color.u64(f.ring.size());
      facet_nodes.emplace(f.id.value_for_debug(), add_node(color.bytes()));
    }
  for (const auto &h : a.edge_uses)
    if (h.operand == role) {
      canonical_encoder color;
      color.byte(3);
      enc_point(color,
                a.vertices[h.origin.value_for_debug()].exact_coordinate);
      enc_point(color,
                a.vertices[h.destination.value_for_debug()].exact_coordinate);
      incidence_nodes.emplace(h.id.value_for_debug(), add_node(color.bytes()));
    }
  for (const auto &s : a.shells)
    if (s.operand == role) {
      canonical_encoder color;
      color.byte(4);
      enc_box(color, s.bounds);
      s.oriented_six_volume.encode(color);
      color.u64(s.facets.size());
      shell_nodes.emplace(s.id.value_for_debug(), add_node(color.bytes()));
    }
  auto arc = [&](std::uint64_t from, std::uint64_t to,
                 std::uint16_t type) {
    graph.arcs.push_back({from, to, type});
  };
  for (const auto &h : a.edge_uses)
    if (h.operand == role) {
      auto node = incidence_nodes.at(h.id.value_for_debug());
      arc(node, vertex_nodes.at(h.origin.value_for_debug()), 1);
      arc(node, vertex_nodes.at(h.destination.value_for_debug()), 2);
      arc(node, facet_nodes.at(h.facet.value_for_debug()), 3);
      arc(node, incidence_nodes.at(h.next.value_for_debug()), 4);
      arc(node, incidence_nodes.at(h.previous.value_for_debug()), 5);
      arc(node, incidence_nodes.at(h.twin.value_for_debug()), 6);
      arc(node, shell_nodes.at(h.shell.value_for_debug()), 7);
    }
  for (const auto &s : a.shells)
    if (s.operand == role)
      for (auto facet : s.facets)
        arc(shell_nodes.at(s.id.value_for_debug()),
            facet_nodes.at(facet.value_for_debug()), 8);
  auto canonical_graph =
      canonicalize_graph_exhaustive(graph, accountant, cancelled);
  if (!canonical_graph.has_value())
    return canonical_graph.error();
  std::sort(vertices.begin(), vertices.end());
  std::sort(facets.begin(), facets.end());
  std::sort(shells.begin(), shells.end());
  canonical_encoder e;
  const char tag[] = "YGBOPD02";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.id(role);
  e.u64(vertices.size());
  for (const auto &x : vertices)
    e.byte_string(x);
  e.u64(facets.size());
  for (const auto &x : facets)
    e.byte_string(x);
  e.u64(shells.size());
  for (const auto &x : shells)
    e.byte_string(x);
  e.byte_string(canonical_graph.value().canonical_bytes);
  return e.bytes();
}

template<class T,class I>status_or<std::shared_ptr<artifact_t<T,I>>>build(boolean_context<T,I>&ctx){
    auto out=std::make_shared<artifact_t<T,I>>();out->owner=ctx.owner();out->setup_digest=ctx.replay().setup;auto kb=ctx.kernel().arithmetic_policy_bytes();out->kernel_policy_digest=domain_digest({{'Y','G','B','K','E','R','0','3'}},kb);
    const fv_surface_mesh<T,I>*raw[2]={&ctx.operand_a_mesh(),&ctx.operand_b_mesh()};std::uint64_t vertex_base=0,facet_base=0,use_base=0,edge_base=0,shell_base=0;
    for(unsigned role=0;role<2;++role){if(ctx.cancelled())return make_error(boolean_error_code::resource_limit,boolean_stage::input_validation,"cancelled");const auto&m=*raw[role];std::uint64_t ring_entries=0;for(const auto&face:m.faces){auto sum=checked_add(ring_entries,face.size(),boolean_stage::input_validation);if(!sum.has_value())return sum.error();ring_entries=sum.value();}auto entities=checked_add(m.vertices.size(),m.faces.size(),boolean_stage::input_validation);if(!entities.has_value())return entities.error();entities=checked_add(entities.value(),ring_entries,boolean_stage::input_validation);if(!entities.has_value())return entities.error();auto charged=ctx.accountant().reserve(resource_kind::entities,entities.value(),boolean_stage::input_validation);if(!charged.has_value())return charged.error();charged=ctx.accountant().reserve(resource_kind::work_units,entities.value(),boolean_stage::input_validation);if(!charged.has_value())return charged.error();auto oid=role?operand_b():operand_a();auto&op=out->operands[role];op.operand=oid;op.raw_vertex_count=m.vertices.size();op.raw_face_count=m.faces.size();std::vector<bool>used(m.vertices.size(),false);std::vector<exact_point3>points(m.vertices.size());
      for (std::size_t i = 0; i < m.vertices.size(); ++i) {
        const auto &v = m.vertices[i];
        auto x = decode_coordinate(v.x), y = decode_coordinate(v.y),
             z = decode_coordinate(v.z);
        if (!x.has_value() || !y.has_value() || !z.has_value())
          return input_error(input_validation_subcode::nonfinite_coordinate,
                             "nonfinite_coordinate");
        auto scalar_bits = [](const exact_scalar &q) {
          return static_cast<std::uint64_t>(
              q.numerator().magnitude().bit_length() +
              q.denominator().bit_length());
        };
        auto exact_bits = checked_add(scalar_bits(x.value().value),
                                      scalar_bits(y.value().value),
                                      boolean_stage::input_validation);
        if (!exact_bits.has_value())
          return exact_bits.error();
        exact_bits = checked_add(exact_bits.value(),
                                 scalar_bits(z.value().value),
                                 boolean_stage::input_validation);
        if (!exact_bits.has_value())
          return exact_bits.error();
        auto exact_charge = ctx.accountant().reserve(
            resource_kind::exact_number_bits, exact_bits.value(),
            boolean_stage::input_validation);
        if (!exact_charge.has_value())
          return exact_charge.error();
        points[i] = point(x.value(), y.value(), z.value());
      }
        for(std::size_t fi=0;fi<m.faces.size();++fi){const auto&f=m.faces[fi];if(f.size()<3)return input_error(input_validation_subcode::short_ring,"short_ring");std::set<I>seen;for(std::size_t j=0;j<f.size();++j){if(static_cast<std::uint64_t>(f[j])>=m.vertices.size())return input_error(input_validation_subcode::index_out_of_range,"index_out_of_range");if(!seen.insert(f[j]).second)return input_error(input_validation_subcode::repeated_vertex,"repeated_ring_vertex");used[f[j]]=true;if(points[f[j]]==points[f[(j+1)%f.size()]])return input_error(input_validation_subcode::zero_length_edge,"zero_length_edge");}}
        std::vector<original_vertex_id>vmap(m.vertices.size());std::vector<std::size_t>vertex_order;for(std::size_t i=0;i<m.vertices.size();++i)if(used[i])vertex_order.push_back(i);std::sort(vertex_order.begin(),vertex_order.end(),[&](std::size_t x,std::size_t y){int c=lexicographic_compare(points[x],points[y]);return c?c<0:x<y;});for(auto i:vertex_order){auto id=original_vertex_id::from_canonical_value(vertex_base++);vmap[i]=id;validated_vertex<T>v;v.id=id;v.operand=oid;v.raw_coordinate={m.vertices[i].x,m.vertices[i].y,m.vertices[i].z};v.raw_bits={bits_of(m.vertices[i].x),bits_of(m.vertices[i].y),bits_of(m.vertices[i].z)};v.exact_coordinate=points[i];out->vertices.push_back(std::move(v));op.vertices.push_back(id);}for(std::size_t i=0;i<m.vertices.size();++i){source_vertex_provenance<T>p;p.operand=oid;p.raw_vertex_ordinal=i;p.raw_bits={bits_of(m.vertices[i].x),bits_of(m.vertices[i].y),bits_of(m.vertices[i].z)};p.exact_coordinate=points[i];if(used[i]){p.disposition=raw_vertex_disposition::retained;p.canonical_vertex=vmap[i];}out->provenance.push_back(std::move(p));}
        auto face_key = [&](std::size_t fi) {
          std::vector<std::uint64_t> x;
          for (I v : m.faces[fi])
            x.push_back(vmap[v].value_for_debug());
          std::vector<std::uint64_t> best;
          for (std::size_t shift = 0; shift < x.size(); ++shift) {
            std::vector<std::uint64_t> q;
            for (std::size_t k = 0; k < x.size(); ++k)
              q.push_back(x[(shift + k) % x.size()]);
            if (best.empty() || q < best)
              best = q;
          }
          return best;
        };
        std::vector<std::size_t> face_order(m.faces.size());
        for (std::size_t i = 0; i < m.faces.size(); ++i)
          face_order[i] = i;
        std::sort(face_order.begin(), face_order.end(),
                  [&](std::size_t x, std::size_t y) {
                    auto a = face_key(x), b = face_key(y);
                    return a != b ? a < b : x < y;
                  });
        std::set<std::vector<std::uint64_t>> facet_keys;
        std::vector<std::vector<edge_use_id>> face_uses(m.faces.size());
        for (std::size_t fi : face_order) {
          const auto &rf = m.faces[fi];
          std::vector<std::uint64_t> fk;
          for (I x : rf)
            fk.push_back(static_cast<std::uint64_t>(x));
          std::vector<std::uint64_t> canonical;
          for (unsigned reverse = 0; reverse < 2; ++reverse)
            for (std::size_t shift = 0; shift < fk.size(); ++shift) {
              std::vector<std::uint64_t> q;
              for (std::size_t k = 0; k < fk.size(); ++k)
                q.push_back(fk[reverse ? (shift + fk.size() - k) % fk.size()
                                       : (shift + k) % fk.size()]);
              if (canonical.empty() || q < canonical)
                canonical = q;
            }
          if (!facet_keys.insert(canonical).second)
            return input_error(input_validation_subcode::duplicate_facet,
                               "duplicate_facet");
          validated_facet f;
          f.id = facet_id::from_canonical_value(facet_base++);
          f.operand = oid;
          f.raw_face_ordinal = fi;
          std::size_t ring_start = 0;
          for (std::size_t i = 1; i < rf.size(); ++i)
            if (vmap[rf[i]] < vmap[rf[ring_start]])
              ring_start = i;
          for (std::size_t i = 0; i < rf.size(); ++i)
            f.ring.push_back(vmap[rf[(ring_start + i) % rf.size()]]);
          status_or<exact_plane3> pl = input_error(
              input_validation_subcode::degenerate_facet, "degenerate_facet");
          for (std::size_t i = 0; i < rf.size() && !pl.has_value(); ++i)
            for (std::size_t j = i + 1; j < rf.size() && !pl.has_value(); ++j)
              for (std::size_t k = j + 1; k < rf.size() && !pl.has_value(); ++k)
                pl = support_plane(points[rf[i]], points[rf[j]], points[rf[k]]);
          if (!pl.has_value())
            return input_error(input_validation_subcode::degenerate_facet,
                               "degenerate_facet");
          f.plane = pl.value();
          for (I v : rf)
            if (plane_side(f.plane, points[v]) != exact_sign::zero)
              return input_error(input_validation_subcode::nonplanar_facet,
                                 "nonplanar_facet");
          f.projection = dominant_projection(f.plane);
          std::vector<exact_point2> pp;
          for (std::size_t i = 0; i < rf.size(); ++i)
            pp.push_back(project(points[rf[(ring_start + i) % rf.size()]],
                                 f.projection));
          f.projected_double_area = area2(pp);
          if (f.projected_double_area.is_zero())
            return input_error(input_validation_subcode::degenerate_facet,
                               "zero_area_facet");
          auto simple = simple_ring(pp);
          if (!simple.has_value())
            return simple.error();
          auto tris = triangulate(pp);
          if (!tris.has_value())
            return tris.error();
          for (const auto &t : tris.value())
            f.triangles.push_back({f.ring[t[0]], f.ring[t[1]], f.ring[t[2]]});
          std::vector<exact_point3> facet_points;
          for (auto id : f.ring)
            facet_points.push_back(
                out->vertices[id.value_for_debug()].exact_coordinate);
          f.bounds = point_bounds(facet_points);
          for (std::size_t j = 0; j < rf.size(); ++j) {
            validated_edge_use h;
            h.id = edge_use_id::from_canonical_value(use_base++);
            h.operand = oid;
            h.facet = f.id;
            h.ring_offset = j;
            h.origin = vmap[rf[(ring_start + j) % rf.size()]];
            h.destination = vmap[rf[(ring_start + j + 1) % rf.size()]];
            face_uses[fi].push_back(h.id);
            f.edge_uses.push_back(h.id);
            out->edge_uses.push_back(h);
          }
          op.facets.push_back(f.id);
          out->facets.push_back(std::move(f));
        }
        for(std::size_t fi=0;fi<face_uses.size();++fi)for(std::size_t j=0;j<face_uses[fi].size();++j){auto&h=out->edge_uses[face_uses[fi][j].value_for_debug()];h.previous=face_uses[fi][(j+face_uses[fi].size()-1)%face_uses[fi].size()];h.next=face_uses[fi][(j+1)%face_uses[fi].size()];}
        using key=std::pair<std::uint64_t,std::uint64_t>;std::map<key,std::vector<edge_use_id>>groups;for(auto fid:op.facets)for(auto hid:out->facets[fid.value_for_debug()].edge_uses){const auto&h=out->edge_uses[hid.value_for_debug()];auto a=h.origin.value_for_debug(),b=h.destination.value_for_debug();groups[{std::min(a,b),std::max(a,b)}].push_back(hid);}for(auto&g:groups){if(g.second.size()==1)return input_error(input_validation_subcode::boundary_edge,"boundary_edge");if(g.second.size()!=2)return input_error(input_validation_subcode::nonmanifold_edge,"nonmanifold_edge");auto&h=out->edge_uses[g.second[0].value_for_debug()];auto&q=out->edge_uses[g.second[1].value_for_debug()];if(h.origin!=q.destination||h.destination!=q.origin)return input_error(input_validation_subcode::same_direction_uses,"same_direction_edge_uses");validated_edge e;e.id=undirected_edge_id::from_canonical_value(edge_base++);e.operand=oid;e.first=original_vertex_id::from_canonical_value(g.first.first);e.second=original_vertex_id::from_canonical_value(g.first.second);e.uses={h.id,q.id};h.twin=q.id;q.twin=h.id;h.edge=e.id;q.edge=e.id;out->edges.push_back(e);}
        std::set<facet_id> remaining(op.facets.begin(), op.facets.end());
        while (!remaining.empty()) {
          validated_shell s;
          s.id = shell_id::from_canonical_value(shell_base++);
          s.operand = oid;
          std::vector<facet_id> todo{*remaining.begin()};
          remaining.erase(todo.front());
          for (std::size_t n = 0; n < todo.size(); ++n) {
            auto fid = todo[n];
            s.facets.push_back(fid);
            auto &f = out->facets[fid.value_for_debug()];
            f.shell = s.id;
            for (auto hid : f.edge_uses) {
              auto &h = out->edge_uses[hid.value_for_debug()];
              h.shell = s.id;
              auto &ed = out->edges[h.edge.value_for_debug()];
              ed.shell = s.id;
              s.edges.push_back(ed.id);
              auto nf = out->edge_uses[h.twin.value_for_debug()].facet;
              f.neighbors.push_back(nf);
              if (remaining.erase(nf))
                todo.push_back(nf);
            }
          }
          std::sort(s.edges.begin(), s.edges.end());
          s.edges.erase(std::unique(s.edges.begin(), s.edges.end()),
                        s.edges.end());
          std::set<original_vertex_id> sv;
          exact_scalar volume(0);
          for (auto fid : s.facets) {
            const auto &f = out->facets[fid.value_for_debug()];
            for (auto v : f.ring)
              sv.insert(v);
            for (const auto &t : f.triangles) {
              const auto &a =
                  out->vertices[t[0].value_for_debug()].exact_coordinate;
              const auto &b =
                  out->vertices[t[1].value_for_debug()].exact_coordinate;
              const auto &c =
                  out->vertices[t[2].value_for_debug()].exact_coordinate;
              volume = volume + dot(exact_vector3{a.x, a.y, a.z},
                                    cross(exact_vector3{b.x, b.y, b.z},
                                          exact_vector3{c.x, c.y, c.z}));
            }
          }
          s.vertices.assign(sv.begin(), sv.end());
          std::vector<exact_point3> shell_points;
          for (auto id : s.vertices)
            shell_points.push_back(
                out->vertices[id.value_for_debug()].exact_coordinate);
          s.bounds = point_bounds(shell_points);
          s.oriented_six_volume = volume;
          if (volume.is_zero())
            return input_error(input_validation_subcode::degenerate_facet,
                               "zero_volume_shell");
          s.orientation = volume.sign() == exact_sign::positive
                              ? shell_orientation::outward
                              : shell_orientation::inward;
          for (auto v : s.vertices)
            out->vertices[v.value_for_debug()].shell = s.id;
          op.shells.push_back(s.id);
          out->shells.push_back(std::move(s));
        }
        for(auto vid:op.vertices){auto&v=out->vertices[vid.value_for_debug()];std::vector<edge_use_id>outgoing;for(const auto&h:out->edge_uses)if(h.operand==oid&&h.origin==vid)outgoing.push_back(h.id);if(!outgoing.empty()){auto start=*std::min_element(outgoing.begin(),outgoing.end()),cur=start;std::set<edge_use_id>seen;do{if(!seen.insert(cur).second)break;v.ordered_outgoing_link.push_back(cur);const auto&h=out->edge_uses[cur.value_for_debug()];cur=out->edge_uses[h.previous.value_for_debug()].twin;}while(cur!=start);if(seen.size()!=outgoing.size()||cur!=start)return input_error(input_validation_subcode::disconnected_vertex_link,"disconnected_vertex_link");}}
        auto pair_work=checked_multiply(op.facets.size(),op.facets.empty()?0:op.facets.size()-1,boolean_stage::input_validation);if(!pair_work.has_value())return pair_work.error();pair_work=pair_work.value()/2;auto pair_charge=ctx.accountant().reserve(resource_kind::work_units,pair_work.value(),boolean_stage::input_validation);if(!pair_charge.has_value())return pair_charge.error();for(std::size_t i=0;i<op.facets.size();++i)for(std::size_t j=i+1;j<op.facets.size();++j){if(ctx.cancelled())return make_error(boolean_error_code::resource_limit,boolean_stage::input_validation,"cancelled");const auto&f=out->facets[op.facets[i].value_for_debug()];const auto&g=out->facets[op.facets[j].value_for_debug()];auto relation=relate_polygons(facet_triangles(*out,f),facet_triangles(*out,g));std::size_t shared_vertices=0;for(auto x:f.ring)if(std::find(g.ring.begin(),g.ring.end(),x)!=g.ring.end())++shared_vertices;bool shared_edge=false;for(auto h:f.edge_uses)for(auto q:g.edge_uses)if(out->edge_uses[h.value_for_debug()].edge==out->edge_uses[q.value_for_debug()].edge)shared_edge=true;if(f.shell!=g.shell){if(relation!=polygon_intersection_kind::disjoint)return input_error(input_validation_subcode::shell_contact,"shell_boundary_contact");}else if(shared_edge){if(relation!=polygon_intersection_kind::segment)return input_error(input_validation_subcode::self_intersection,"adjacent_facet_intersection");}else if(shared_vertices){if(relation!=polygon_intersection_kind::point)return input_error(input_validation_subcode::self_intersection,"vertex_adjacent_facet_intersection");}else if(relation!=polygon_intersection_kind::disjoint)return input_error(input_validation_subcode::self_intersection,"nonadjacent_facet_intersection");}
        std::vector<shell_id>role_shells=op.shells;std::vector<std::vector<bool>>inside(role_shells.size(),std::vector<bool>(role_shells.size(),false));for(std::size_t i=0;i<role_shells.size();++i)for(std::size_t j=0;j<role_shells.size();++j)if(i!=j){const auto&s=out->shells[role_shells[i].value_for_debug()];const auto&t=out->shells[role_shells[j].value_for_debug()];auto witness=*std::min_element(s.vertices.begin(),s.vertices.end());const auto&p=out->vertices[witness.value_for_debug()].exact_coordinate;auto loc=classify_point_closed_triangle_shell(p,shell_triangles(*out,t));if(!loc.has_value())return loc.error();if(loc.value()==solid_point_kind::boundary)return input_error(input_validation_subcode::shell_contact,"shell_boundary_contact");inside[i][j]=loc.value()==solid_point_kind::inside;}
        for(std::size_t i=0;i<role_shells.size();++i){std::optional<std::size_t>parent;for(std::size_t j=0;j<role_shells.size();++j)if(inside[i][j]){bool nearest=true;for(std::size_t k=0;k<role_shells.size();++k)if(k!=j&&inside[i][k]&&!inside[j][k])nearest=false;if(nearest){if(parent)return input_error(input_validation_subcode::ambiguous_nesting,"ambiguous_shell_parent");parent=j;}}if(parent){auto&child=out->shells[role_shells[i].value_for_debug()];child.parent=role_shells[*parent];out->shells[role_shells[*parent].value_for_debug()].children.push_back(child.id);}}
        for (std::size_t pass = 0; pass < role_shells.size(); ++pass)
          for (auto id : role_shells) {
            auto &s = out->shells[id.value_for_debug()];
            if (s.parent)
              s.depth = out->shells[s.parent->value_for_debug()].depth + 1;
          }
        for (auto id : role_shells) {
          auto &s = out->shells[id.value_for_debug()];
          s.contribution = s.depth % 2 ? shell_contribution::cavity_boundary
                                       : shell_contribution::material_boundary;
          auto expected = s.depth % 2 ? shell_orientation::inward
                                      : shell_orientation::outward;
          if (s.orientation != expected)
            return input_error(input_validation_subcode::orientation_mismatch,
                               s.depth % 2 ? "cavity_shell_not_inward"
                                           : "material_shell_not_outward");
          std::sort(s.children.begin(), s.children.end());
        }
        if (!op.vertices.empty()) {
          std::vector<exact_point3> operand_points;
          for (auto id : op.vertices)
            operand_points.push_back(
                out->vertices[id.value_for_debug()].exact_coordinate);
          op.bounds = point_bounds(operand_points);
        }
        const std::array<std::pair<validation_evidence_kind, std::uint64_t>, 7>
            summaries{{{validation_evidence_kind::coordinate_scan,
                        op.raw_vertex_count},
                       {validation_evidence_kind::facet_audit, op.facets.size()},
                       {validation_evidence_kind::triangulation,
                        op.facets.size()},
                       {validation_evidence_kind::edge_pairing,
                        std::count_if(out->edges.begin(), out->edges.end(),
                                      [&](const auto &e) {
                                        return e.operand == oid;
                                      })},
                       {validation_evidence_kind::vertex_links,
                        op.vertices.size()},
                       {validation_evidence_kind::embeddedness,
                        op.facets.size() * (op.facets.size() - 1) / 2},
                       {validation_evidence_kind::nesting,
                        op.shells.size() *
                            (op.shells.empty() ? 0 : op.shells.size() - 1)}}};
        for (const auto &summary : summaries) {
          validation_evidence evidence;
          evidence.kind = summary.first;
          evidence.operand = oid;
          evidence.checked = summary.second;
          canonical_encoder encoded;
          encoded.u16(evidence.schema);
          encoded.byte(static_cast<std::uint8_t>(evidence.kind));
          encoded.id(evidence.operand);
          encoded.u64(evidence.checked);
          evidence.evidence_digest =
              domain_digest({{'Y', 'G', 'B', 'E', 'V', 'D', '0', '2'}},
                            encoded.bytes());
          out->evidence.push_back(std::move(evidence));
        }
    }
    for(unsigned role=0;role<2;++role){auto bytes=semantic_operand_bytes(*out,role?operand_b():operand_a(),&ctx.accountant(),[&ctx]{return ctx.cancelled();});if(!bytes.has_value())return bytes.error();out->operands[role].semantic_digest=domain_digest({{'Y','G','B','O','P','D','0','2'}},bytes.value());}out->artifact_digest=artifact_digest_for(*out);return out;
}

template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &env) noexcept {
  try {
    const auto &a = *static_cast<const artifact_t<T, I> *>(v.payload);
    const auto *raw_a = static_cast<const fv_surface_mesh<T, I> *>(
        env.raw_operands.operand_a);
    const auto *raw_b = static_cast<const fv_surface_mesh<T, I> *>(
        env.raw_operands.operand_b);
    const fv_surface_mesh<T, I> *raw[2] = {raw_a, raw_b};
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::input_validation;
    r.slot = v.slot;
    r.artifact_type_tag = v.artifact_type_tag;
    r.artifact_schema = v.artifact_schema;
    r.setup_digest = env.setup_digest;
    r.artifact_digest = v.artifact_digest;
    r.invariant_set_digest = s.invariant_set_digest;
    r.outcome = verification_outcome::pass;
    if (!env.accountant)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::input_validation,
                        "missing_verifier_accountant");
    auto verifier_charge = env.accountant->reserve_scoped(
        resource_kind::verifier_work, s.required_invariants.size(),
        boolean_stage::input_validation);
    if (!verifier_charge.has_value())
      return verifier_charge.error();
    auto scratch_charge = env.accountant->reserve_scoped(
        resource_kind::verifier_scratch_bytes,
        a.vertices.size() * sizeof(std::uint64_t) +
            a.edge_uses.size() * sizeof(std::uint64_t),
        boolean_stage::input_validation);
    if (!scratch_charge.has_value())
      return scratch_charge.error();
    bool failed = false;
    for (auto code : s.required_invariants) {
      if (env.cancelled && env.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::input_validation, "cancelled");
      invariant_result x;
      x.code = code;
      x.status = failed ? check_status::not_run_due_to_prior_failure
                        : check_status::passed;
      if (!failed) {
        bool ok = true;
        switch (code) {
        case invariant_code::input_binding:
          ok = a.owner == v.owner && a.setup_digest == env.setup_digest &&
               raw_a && raw_b && env.coordinate == env.raw_operands.coordinate &&
               env.index == env.raw_operands.index;
          break;
        case invariant_code::input_coordinates: {
          std::size_t provenance_index = 0;
          for (unsigned role = 0; role < 2 && ok; ++role) {
            ok = a.operands[role].raw_vertex_count == raw[role]->vertices.size() &&
                 a.operands[role].raw_face_count == raw[role]->faces.size();
            std::vector<bool> used(raw[role]->vertices.size(), false);
            for (const auto &face : raw[role]->faces)
              for (I index : face) {
                if (static_cast<std::uint64_t>(index) >= used.size()) {
                  ok = false;
                  break;
                }
                used[index] = true;
              }
            for (std::size_t i = 0;
                 i < raw[role]->vertices.size() && ok;
                 ++i, ++provenance_index) {
              if (provenance_index >= a.provenance.size()) {
                ok = false;
                break;
              }
              const auto &source = a.provenance[provenance_index];
              const auto &vertex = raw[role]->vertices[i];
              auto dx = decode_coordinate(vertex.x), dy = decode_coordinate(vertex.y),
                   dz = decode_coordinate(vertex.z);
              ok = dx.has_value() && dy.has_value() && dz.has_value() &&
                   source.operand == (role ? operand_b() : operand_a()) &&
                   source.raw_vertex_ordinal == i &&
                   source.raw_bits[0].bits == bits_of(vertex.x).bits &&
                   source.raw_bits[1].bits == bits_of(vertex.y).bits &&
                   source.raw_bits[2].bits == bits_of(vertex.z).bits &&
                   source.exact_coordinate ==
                       point(dx.value(), dy.value(), dz.value()) &&
                   (source.disposition == raw_vertex_disposition::retained) ==
                       used[i] &&
                   source.canonical_vertex.has_value() == used[i];
            }
          }
          ok = ok && provenance_index == a.provenance.size();
          for (unsigned role = 0; role < 2 && ok; ++role) {
            std::vector<exact_point3> retained;
            for (auto id : a.operands[role].vertices)
              retained.push_back(
                  a.vertices[id.value_for_debug()].exact_coordinate);
            ok = a.operands[role].bounds.has_value() == !retained.empty();
            if (!retained.empty())
              ok = ok && equal_box(*a.operands[role].bounds,
                                   point_bounds(retained));
          }
          break;
        }
        case invariant_code::input_rings:
          for (unsigned role = 0; role < 2 && ok; ++role)
            for (std::size_t fi = 0; fi < raw[role]->faces.size() && ok; ++fi) {
              const validated_facet *stored = nullptr;
              for (const auto &facet : a.facets)
                if (facet.operand == (role ? operand_b() : operand_a()) &&
                    facet.raw_face_ordinal == fi) {
                  stored = &facet;
                  break;
                }
              ok = stored && stored->ring.size() == raw[role]->faces[fi].size() &&
                   stored->edge_uses.size() == stored->ring.size();
              if (!stored)
                continue;
              std::size_t raw_start = 0;
              for (; raw_start < stored->ring.size(); ++raw_start) {
                const auto ordinal = static_cast<std::uint64_t>(
                    raw[role]->faces[fi][raw_start]);
                for (const auto &candidate : a.provenance)
                  if (candidate.operand == stored->operand &&
                      candidate.raw_vertex_ordinal == ordinal &&
                      candidate.canonical_vertex &&
                      *candidate.canonical_vertex == stored->ring[0])
                    goto ring_start_found;
              }
            ring_start_found:
              ok = raw_start < stored->ring.size();
              for (std::size_t k = 0; k < stored->ring.size() && ok; ++k) {
                const auto ordinal = static_cast<std::uint64_t>(
                    raw[role]->faces[fi][(raw_start + k) % stored->ring.size()]);
                const source_vertex_provenance<T> *source = nullptr;
                for (const auto &candidate : a.provenance)
                  if (candidate.operand == stored->operand &&
                      candidate.raw_vertex_ordinal == ordinal) {
                    source = &candidate;
                    break;
                  }
                ok = source && source->canonical_vertex &&
                     *source->canonical_vertex == stored->ring[k];
              }
            }
          break;
        case invariant_code::input_facets:
          for (const auto &f : a.facets) {
            std::vector<exact_point2> projected;
            for (auto q : f.ring)
              ok = ok && q.valid() && q.value_for_debug() < a.vertices.size() &&
                   plane_side(
                       f.plane,
                       a.vertices[q.value_for_debug()].exact_coordinate) ==
                       exact_sign::zero;
            for (auto q : f.ring)
              projected.push_back(project(
                  a.vertices[q.value_for_debug()].exact_coordinate,
                  f.projection));
            std::vector<exact_point3> facet_points;
            for (auto q : f.ring)
              facet_points.push_back(
                  a.vertices[q.value_for_debug()].exact_coordinate);
            ok = ok && equal_box(f.bounds, point_bounds(facet_points));
            exact_scalar polygon_area(0), triangle_area(0);
            for (std::size_t i = 0; i < projected.size(); ++i)
              polygon_area = polygon_area +
                             projected[i].x *
                                 projected[(i + 1) % projected.size()].y -
                             projected[i].y *
                                 projected[(i + 1) % projected.size()].x;
            ok = ok && !polygon_area.is_zero() &&
                 polygon_area == f.projected_double_area &&
                 f.triangles.size() == f.ring.size() - 2;
            for (const auto &t : f.triangles) {
              ok = ok && t[0].value_for_debug() < a.vertices.size() &&
                   t[1].value_for_debug() < a.vertices.size() &&
                   t[2].value_for_debug() < a.vertices.size();
              if (!ok)
                break;
              auto p = project(a.vertices[t[0].value_for_debug()].exact_coordinate,
                               f.projection);
              auto q = project(a.vertices[t[1].value_for_debug()].exact_coordinate,
                               f.projection);
              auto r3 = project(a.vertices[t[2].value_for_debug()].exact_coordinate,
                                f.projection);
              auto signed_area = p.x * q.y - p.y * q.x + q.x * r3.y -
                                 q.y * r3.x + r3.x * p.y - r3.y * p.x;
              ok = ok && !signed_area.is_zero() &&
                   signed_area.sign() == polygon_area.sign();
              triangle_area = triangle_area + signed_area;
            }
            ok = ok && triangle_area == polygon_area;
          }
          break;
        case invariant_code::input_edges:
          {
          std::map<std::pair<std::uint64_t, std::uint64_t>, std::size_t>
              reconstructed;
          for (const auto &f : a.facets)
            for (std::size_t i = 0; i < f.ring.size(); ++i) {
              auto x = f.ring[i].value_for_debug();
              auto y = f.ring[(i + 1) % f.ring.size()].value_for_debug();
              ++reconstructed[{std::min(x, y), std::max(x, y)}];
            }
          ok = reconstructed.size() == a.edges.size();
          for (const auto &entry : reconstructed)
            ok = ok && entry.second == 2;
          for (const auto &e : a.edges) {
            ok = ok && e.uses[0].value_for_debug() < a.edge_uses.size() &&
                 e.uses[1].value_for_debug() < a.edge_uses.size();
            if (ok) {
              const auto &h = a.edge_uses[e.uses[0].value_for_debug()];
              const auto &q = a.edge_uses[e.uses[1].value_for_debug()];
              ok = ok && h.twin == q.id && q.twin == h.id &&
                   h.origin == q.destination && h.destination == q.origin;
            }
          }
          break;
          }
        case invariant_code::input_vertex_links:
          for (const auto &q : a.vertices)
            ok = ok && (!q.shell.valid() || !q.ordered_outgoing_link.empty());
          break;
        case invariant_code::input_shells:
          for (const auto &q : a.shells) {
            ok = ok && !q.oriented_six_volume.is_zero() && !q.facets.empty() &&
                 ((q.depth % 2 == 0) ==
                  (q.oriented_six_volume.sign() == exact_sign::positive));
            std::vector<exact_point3> shell_points;
            for (auto id : q.vertices)
              shell_points.push_back(
                  a.vertices[id.value_for_debug()].exact_coordinate);
            ok = ok && !shell_points.empty() &&
                 equal_box(q.bounds, point_bounds(shell_points));
          }
          for (std::size_t i = 0; i < a.facets.size() && ok; ++i)
            for (std::size_t j = i + 1; j < a.facets.size() && ok; ++j) {
              const auto &f = a.facets[i], &g = a.facets[j];
              if (f.operand != g.operand)
                continue;
              auto make_triangles = [&](const validated_facet &facet) {
                std::vector<exact_triangle3> triangles;
                for (const auto &t : facet.triangles)
                  triangles.push_back(
                      {a.vertices[t[0].value_for_debug()].exact_coordinate,
                       a.vertices[t[1].value_for_debug()].exact_coordinate,
                       a.vertices[t[2].value_for_debug()].exact_coordinate});
                return triangles;
              };
              auto relation =
                  relate_polygons(make_triangles(f), make_triangles(g));
              bool shared_edge = false;
              std::size_t shared_vertices = 0;
              for (auto x : f.ring)
                if (std::find(g.ring.begin(), g.ring.end(), x) != g.ring.end())
                  ++shared_vertices;
              for (auto x : f.edge_uses)
                for (auto y : g.edge_uses)
                  if (a.edge_uses[x.value_for_debug()].edge ==
                      a.edge_uses[y.value_for_debug()].edge)
                    shared_edge = true;
              ok = f.shell != g.shell
                       ? relation == polygon_intersection_kind::disjoint
                       : shared_edge
                             ? relation == polygon_intersection_kind::segment
                             : shared_vertices
                                   ? relation == polygon_intersection_kind::point
                                   : relation == polygon_intersection_kind::disjoint;
            }
          for (unsigned role = 0; role < 2 && ok; ++role) {
            std::vector<const validated_shell *> shells;
            for (const auto &shell : a.shells)
              if (shell.operand == (role ? operand_b() : operand_a()))
                shells.push_back(&shell);
            std::vector<std::vector<bool>> contained(
                shells.size(), std::vector<bool>(shells.size(), false));
            auto triangles_for_shell = [&](const validated_shell &shell) {
              std::vector<exact_triangle3> triangles;
              for (auto facet_id : shell.facets)
                for (const auto &t :
                     a.facets[facet_id.value_for_debug()].triangles)
                  triangles.push_back(
                      {a.vertices[t[0].value_for_debug()].exact_coordinate,
                       a.vertices[t[1].value_for_debug()].exact_coordinate,
                       a.vertices[t[2].value_for_debug()].exact_coordinate});
              return triangles;
            };
            for (std::size_t i = 0; i < shells.size() && ok; ++i)
              for (std::size_t j = 0; j < shells.size() && ok; ++j)
                if (i != j) {
                  auto witness = *std::min_element(shells[i]->vertices.begin(),
                                                   shells[i]->vertices.end());
                  auto location = classify_point_closed_triangle_shell(
                      a.vertices[witness.value_for_debug()].exact_coordinate,
                      triangles_for_shell(*shells[j]));
                  ok = location.has_value() &&
                       location.value() != solid_point_kind::boundary;
                  if (ok)
                    contained[i][j] =
                        location.value() == solid_point_kind::inside;
                }
            for (std::size_t i = 0; i < shells.size() && ok; ++i) {
              const validated_shell *parent = nullptr;
              for (std::size_t j = 0; j < shells.size(); ++j)
                if (contained[i][j]) {
                  bool nearest = true;
                  for (std::size_t k = 0; k < shells.size(); ++k)
                    if (k != j && contained[i][k] && !contained[j][k])
                      nearest = false;
                  if (nearest) {
                    ok = ok && parent == nullptr;
                    parent = shells[j];
                  }
                }
              ok = ok && shells[i]->parent.has_value() == (parent != nullptr);
              if (parent)
                ok = ok && *shells[i]->parent == parent->id &&
                     shells[i]->depth == parent->depth + 1;
              else
                ok = ok && shells[i]->depth == 0;
            }
          }
          break;
        case invariant_code::input_canonical_encoding:
          ok = artifact_digest_for(a) == v.artifact_digest;
          for (unsigned role = 0; role < 2 && ok; ++role) {
            auto bytes = semantic_operand_bytes(
                a, role ? operand_b() : operand_a(), env.accountant,
                env.cancelled);
            if (!bytes.has_value())
              return bytes.error();
            ok = a.operands[role].semantic_digest ==
                  domain_digest({{'Y', 'G', 'B', 'O', 'P', 'D', '0', '2'}},
                                bytes.value());
            original_vertex_id previous;
            bool have_previous = false;
            for (auto id : a.operands[role].vertices) {
              if (have_previous)
                ok = ok && lexicographic_compare(
                               a.vertices[previous.value_for_debug()]
                                   .exact_coordinate,
                               a.vertices[id.value_for_debug()].exact_coordinate) <
                               0;
              previous = id;
              have_previous = true;
            }
          }
          break;
        default:
          ok = false;
        }
        if (!ok) {
          x.status = check_status::failed;
          x.subcode = 1;
          failed = true;
          r.outcome = verification_outcome::invariant_failure;
        }
      }
      r.results.push_back(std::move(x));
    }
    evidence_record evidence;
    evidence.kind = evidence_kind::raw_scan;
    evidence.invariant = invariant_code::input_coordinates;
    canonical_encoder evidence_payload;
    evidence_payload.u64(a.provenance.size());
    evidence_payload.u64(a.facets.size());
    evidence_payload.u64(a.shells.size());
    evidence.exact_payload = evidence_payload.bytes();
    evidence.dependencies = {a.setup_digest, a.artifact_digest};
    evidence.evidence_digest = evidence_digest(evidence);
    r.evidence.push_back(std::move(evidence));
    auto evidence_charge = env.accountant->reserve_scoped(
        resource_kind::evidence_records, r.evidence.size(),
        boolean_stage::input_validation);
    if (!evidence_charge.has_value())
      return evidence_charge.error();
    auto enc = encode_verification_report(r);
    if (!enc.has_value())
      return enc.error();
    auto report_charge = env.accountant->reserve_scoped(
        resource_kind::report_bytes, enc.value().size(),
        boolean_stage::input_validation);
    if (!report_charge.has_value())
      return report_charge.error();
    r.report_digest =
        domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}}, enc.value());
    verifier_charge.value().commit();
    scratch_charge.value().commit();
    evidence_charge.value().commit();
    report_charge.value().commit();
    return r;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation,
                      "input_verifier_allocation");
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::input_validation,
                      "input_verifier_exception");
  }
}
template<class T,class I>status_or<verification_report>callback(const artifact_view&v,const verification_spec&s,const verification_environment_view&e)noexcept{return verify_typed<T,I>(v,s,e);}
}

status_or<bool>register_input_topology_verifier(verifier_registry&r,coordinate_tag c,index_tag i){verifier_registration x;x.slot=artifact_slot::validated_operands;x.artifact_type_tag=validated_operands_type_tag+(static_cast<std::uint64_t>(c)<<8)+static_cast<std::uint64_t>(i);x.artifact_schema=validated_operands_schema;x.mandatory={invariant_code::input_binding,invariant_code::input_coordinates,invariant_code::input_rings,invariant_code::input_facets,invariant_code::input_edges,invariant_code::input_vertex_links,invariant_code::input_shells,invariant_code::input_canonical_encoding};x.exhaustive=x.mandatory;if(c==coordinate_tag::binary32&&i==index_tag::uint32)x.callback=&callback<float,std::uint32_t>;else if(c==coordinate_tag::binary32)x.callback=&callback<float,std::uint64_t>;else if(i==index_tag::uint32)x.callback=&callback<double,std::uint32_t>;else x.callback=&callback<double,std::uint64_t>;return r.register_verifier(std::move(x));}
template <class T, class I>
status_or<std::shared_ptr<const published_artifact<validated_operands<T, I>>>>
validate_operands(boolean_context<T, I> &ctx) {
  try {
    auto made = build(ctx);
    if (!made.has_value())
      return made.error();
    auto candidate = std::shared_ptr<const artifact_t<T, I>>(made.value());
    auto type = validated_operands_type_tag +
                (static_cast<std::uint64_t>(ctx.platform().coordinate) << 8) +
                static_cast<std::uint64_t>(ctx.platform().index);
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::input_validation,
                        "input_verifier_registry_required");
    auto spec = registry->specification(artifact_slot::validated_operands, type,
                                        validated_operands_schema,
                                        ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    artifact_view view{ctx.owner(), artifact_slot::validated_operands,
                       type,        validated_operands_schema,
                       1,           candidate->artifact_digest,
                       candidate,   candidate.get()};
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
    env.cancelled = [&ctx] { return ctx.cancelled(); };
    auto artifact_bytes = encode_artifact(*candidate);
    auto authoritative = ctx.accountant().reserve_scoped(
        resource_kind::authoritative_bytes, artifact_bytes.size(),
        boolean_stage::input_validation);
    if (!authoritative.has_value())
      return authoritative.error();
    stage_transaction<artifact_t<T, I>, artifact_t<T, I>> tx(
        ctx.owner(), boolean_stage::input_validation,
        artifact_slot::validated_operands,
        std::unique_ptr<artifact_t<T, I>>(new artifact_t<T, I>(*candidate)));
    tx.stage_reservation(std::move(authoritative.value()));
    auto ok = tx.verify(candidate, view, spec.value(), env, *registry);
    if (!ok.has_value())
      return ok.error();
    return tx.publish();
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation, "allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::input_validation,
                        "input_validation_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T,I) template status_or<std::shared_ptr<const published_artifact<validated_operands<T,I>>>>validate_operands(boolean_context<T,I>&)
INST(float,std::uint32_t);INST(float,std::uint64_t);INST(double,std::uint32_t);INST(double,std::uint64_t);
#undef INST
} }
