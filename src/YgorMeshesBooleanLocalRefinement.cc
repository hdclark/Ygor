#include "YgorMeshesBooleanLocalRefinement.h"
#include <algorithm>
#include <map>
#include <set>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Component 7 requires strict floating-point compilation"
#endif

namespace ygor {
namespace mesh_boolean {
int canonical_label_compare(const local_constraint_label &a,
                            const local_constraint_label &b) noexcept {
  if (a.source_kind != b.source_kind)
    return static_cast<std::uint8_t>(a.source_kind) <
                   static_cast<std::uint8_t>(b.source_kind)
               ? -1
               : 1;
  int c = canonical_feature_compare(a.source, b.source);
  if (c) return c;
  if (a.curve.has_value() != b.curve.has_value()) return a.curve ? 1 : -1;
  if (a.curve != b.curve) return *a.curve < *b.curve ? -1 : 1;
  if (a.overlap_region.has_value() != b.overlap_region.has_value())
    return a.overlap_region ? 1 : -1;
  if (a.overlap_region != b.overlap_region)
    return *a.overlap_region < *b.overlap_region ? -1 : 1;
  if (a.direction != b.direction)
    return static_cast<std::uint8_t>(a.direction) <
                   static_cast<std::uint8_t>(b.direction)
               ? -1
               : 1;
  if (a.multiplicity != b.multiplicity)
    return a.multiplicity < b.multiplicity ? -1 : 1;
  if (a.derivations.size() != b.derivations.size())
    return a.derivations.size() < b.derivations.size() ? -1 : 1;
  if (std::lexicographical_compare(a.derivations.begin(), a.derivations.end(),
                                   b.derivations.begin(), b.derivations.end()))
    return -1;
  if (a.derivations != b.derivations) return 1;
  return 0;
}
namespace {
template <class T, class I> std::uint64_t type_tag() {
  return refined_facet_patches_type_tag +
         (static_cast<std::uint64_t>(std::is_same<T, double>::value
                                         ? coordinate_tag::binary64
                                         : coordinate_tag::binary32)
          << 8) +
         static_cast<std::uint64_t>(std::is_same<I, std::uint64_t>::value
                                        ? index_tag::uint64
                                        : index_tag::uint32);
}
void point(canonical_encoder &e, const exact_point2 &p) {
  encode(e, p.x);
  encode(e, p.y);
}
void label(canonical_encoder &e, const local_constraint_label &l) {
  e.byte(static_cast<std::uint8_t>(l.source_kind));
  e.u64(l.source.index());
  std::visit(
      [&](const auto &source) {
        using source_type = typename std::decay<decltype(source)>::type;
        if constexpr (std::is_same<source_type, original_vertex_ref>::value) {
          e.id(source.operand);
          e.id(source.vertex);
        } else if constexpr (std::is_same<source_type, facet_ref>::value) {
          e.id(source.operand);
          e.id(source.facet);
        } else {
          e.id(source);
        }
      },
      l.source);
  e.boolean(bool(l.curve));
  if (l.curve)
    e.id(*l.curve);
  e.boolean(bool(l.overlap_region));
  if (l.overlap_region)
    e.id(*l.overlap_region);
  e.byte(static_cast<std::uint8_t>(l.direction));
  e.u32(l.multiplicity);
  e.u64(l.derivations.size());
  for (auto x : l.derivations)
    e.id(x);
}
bool label_less(const local_constraint_label &a,
                const local_constraint_label &b) {
  return canonical_label_compare(a, b) < 0;
}
template <class T, class I>
std::vector<std::uint8_t> semantic(const refined_facet_patches<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN07";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(refined_facet_patches_schema);
  e.u64(a.shared_edges.size());
  for (const auto &s : a.shared_edges) {
    e.id(s.id);
    e.id(s.lower);
    e.id(s.upper);
    e.u64(s.occurrences.size());
    for (const auto &o : s.occurrences) {
      e.id(o.facet);
      e.id(o.id);
    }
  }
  e.u64(a.facets.size());
  for (const auto &f : a.facets) {
    e.id(f.facet);
    e.id(f.operand);
    e.id(f.shell);
    e.byte(static_cast<std::uint8_t>(f.projection));
    e.u64(f.vertices.size());
    for (const auto &v : f.vertices) {
      e.id(v.id.id);
      e.id(v.symbolic);
      point(e, v.projected);
      e.u64(v.outgoing.size());
      for (const auto &h : v.outgoing)
        e.id(h.id);
    }
    e.u64(f.point_incidences.size());
    for (const auto &p : f.point_incidences) {
      e.id(p.vertex.id);
      e.byte(static_cast<std::uint8_t>(p.location));
      e.boolean(p.used_by_edge);
      e.u64(p.sources.size());
      for (const auto &source : p.sources) {
        local_constraint_label wrapper;
        wrapper.source = source;
        label(e, wrapper);
      }
      e.u64(p.events.size());
      for (auto event : p.events)
        e.id(event);
    }
    e.u64(f.edges.size());
    for (const auto &x : f.edges) {
      e.id(x.id.id);
      e.id(x.lower.id);
      e.id(x.upper.id);
      e.boolean(bool(x.shared_semantic_edge));
      if (x.shared_semantic_edge)
        e.id(*x.shared_semantic_edge);
      e.boolean(bool(x.canonical_interval));
      if (x.canonical_interval)
        e.id(*x.canonical_interval);
      e.boolean(x.source_boundary);
      e.boolean(x.artificial);
      e.u64(x.labels.size());
      for (const auto &l : x.labels)
        label(e, l);
    }
    e.u64(f.halfedges.size());
    for (const auto &h : f.halfedges) {
      e.id(h.id.id);
      e.id(h.twin.id);
      e.id(h.next.id);
      e.id(h.previous.id);
      e.id(h.edge.id);
      e.id(h.origin.id);
      e.id(h.destination.id);
      e.id(h.walk.id);
      e.id(h.left_face.id);
      e.byte(static_cast<std::uint8_t>(h.direction));
    }
    e.u64(f.walks.size());
    for (const auto &w : f.walks) {
      e.id(w.id.id);
      e.byte(static_cast<std::uint8_t>(w.area_class));
      encode(e, w.signed_double_area);
      e.id(w.face.id);
      e.u64(w.halfedges.size());
      for (const auto &h : w.halfedges)
        e.id(h.id);
    }
    e.u64(f.faces.size());
    for (const auto &x : f.faces) {
      e.id(x.id.id);
      e.byte(static_cast<std::uint8_t>(x.extent));
      e.byte(static_cast<std::uint8_t>(x.role));
      e.u64(x.walks.size());
      for (const auto &w : x.walks)
        e.id(w.id);
    }
    e.u64(f.patches.size());
    for (const auto &p : f.patches) {
      e.id(p.id.id);
      e.id(p.parent_face.id);
      e.id(p.outer.id);
      e.byte(static_cast<std::uint8_t>(p.decomposition));
      encode(e, p.signed_double_area);
      e.u64(p.holes.size());
      for (const auto &h : p.holes)
        e.id(h.id);
    }
    e.u64(f.source_boundary.size());
    for (const auto &chain : f.source_boundary) {
      e.id(chain.source);
      e.boolean(chain.forward);
      e.u64(chain.edges.size());
      for (const auto &edge : chain.edges)
        e.id(edge.id);
    }
    e.u64(f.constraints.size());
    for (const auto &chain : f.constraints) {
      local_constraint_label wrapper;
      wrapper.source = chain.source;
      wrapper.direction = chain.direction;
      wrapper.multiplicity = chain.multiplicity;
      label(e, wrapper);
      e.u64(chain.edges.size());
      for (const auto &edge : chain.edges)
        e.id(edge.id);
    }
    encode(e, f.certificate.source_double_area);
    encode(e, f.certificate.patch_double_area);
    e.u64(f.certificate.vertices);
    e.u64(f.certificate.edges);
    e.u64(f.certificate.halfedges);
    e.u64(f.certificate.faces);
    e.u64(f.certificate.components);
    e.u64(f.certificate.bounded_faces);
    e.u64(f.certificate.walks_positive);
    e.u64(f.certificate.walks_negative);
    e.u64(f.certificate.walks_zero);
    e.u64(f.certificate.source_domain_faces);
    e.u64(f.certificate.holes);
    e.u64(f.certificate.artificial_edges);
  }
  return e.bytes();
}
template <class T, class I>
std::vector<std::uint8_t> invocation(const refined_facet_patches<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBREF07";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(refined_facet_patches_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.raw(a.symbolic_digest.bytes.data(), 16);
  e.raw(a.validated_digest.bytes.data(), 16);
  e.raw(a.kernel_policy_digest.bytes.data(), 16);
  e.byte_string(a.canonical_bytes);
  return e.bytes();
}
template <class T, class I>
digest artifact_digest_for(const refined_facet_patches<T, I> &a) {
  canonical_encoder e;
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(artifact_slot::refined_facet_patches));
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}

exact_scalar area(const std::vector<local_halfedge_ref> &hs,
                  const local_refinement &f) {
  exact_scalar z(0);
  for (auto r : hs) {
    const auto &h = f.halfedges[r.id.value_for_debug()];
    const auto &p = f.vertices[h.origin.id.value_for_debug()].projected;
    const auto &q = f.vertices[h.destination.id.value_for_debug()].projected;
    z = z + p.x * q.y - p.y * q.x;
  }
  return z;
}
bool same_edge_pair(symbolic_vertex_id a, symbolic_vertex_id b,
                    symbolic_vertex_id c, symbolic_vertex_id d) {
  return (a == c && b == d) || (a == d && b == c);
}
int ray_compare(const exact_vector2 &a, const exact_vector2 &b) {
  auto upper = [](const exact_vector2 &v) {
    return exact_scalar(0) < v.y || (v.y.is_zero() && !(v.x < exact_scalar(0)));
  };
  bool au = upper(a), bu = upper(b);
  if (au != bu)
    return au ? -1 : 1;
  auto z = a.x * b.y - a.y * b.x;
  if (!z.is_zero())
    return exact_scalar(0) < z ? -1 : 1;
  return 0;
}
template <class T, class I>
status_or<std::vector<symbolic_reconciliation_request>>
audit_crossings(const published_artifact<symbolic_complex<T, I>> &published) {
  const auto &symbolic = *published.payload;
  std::vector<symbolic_reconciliation_request> requests;
  for (const auto &facet : symbolic.validated->payload->facets) {
    std::vector<const symbolic_curve *> curves;
    for (const auto &curve : symbolic.curves) {
      if (curve.kind != symbolic_curve_kind::atomic_interval || !curve.lower ||
          !curve.upper)
        continue;
      const bool incident = std::any_of(
          curve.raw_intervals.begin(), curve.raw_intervals.end(),
          [&](auto id) {
            const auto *raw = find_raw_interval(
                *symbolic.raw_events->payload, id);
            return raw &&
                   (raw->facets.operand_a_facet == facet.id ||
                    raw->facets.operand_b_facet == facet.id);
          });
      if (incident &&
          intersect_line_plane(curve.carrier, facet.plane).kind ==
              line_plane_kind::contained)
        curves.push_back(&curve);
    }
    for (std::size_t i = 0; i < curves.size(); ++i)
      for (std::size_t j = i + 1; j < curves.size(); ++j) {
        const auto &first = *curves[i];
        const auto &second = *curves[j];
        exact_segment3 first_segment{
            symbolic.vertices[first.lower->value_for_debug()].point,
            symbolic.vertices[first.upper->value_for_debug()].point};
        exact_segment3 second_segment{
            symbolic.vertices[second.lower->value_for_debug()].point,
            symbolic.vertices[second.upper->value_for_debug()].point};
        const auto relation = relate_segments(first_segment, second_segment);
        if (relation.dimension != intersection_dimension::point ||
            relation.point_kind != segment_point_kind::proper_crossing ||
            !relation.point)
          continue;
        if (std::any_of(symbolic.vertices.begin(), symbolic.vertices.end(),
                        [&](const auto &vertex) {
                          return vertex.point == *relation.point;
                        }))
          continue;
        symbolic_reconciliation_request request;
        request.prior_digest = published.artifact_digest;
        request.prior_generation = published.generation;
        request.facet = facet.id;
        request.first_curve = std::min(first.id, second.id);
        request.second_curve = std::max(first.id, second.id);
        request.point = *relation.point;
        request.constructions = first.constructions;
        request.constructions.insert(request.constructions.end(),
                                     second.constructions.begin(),
                                     second.constructions.end());
        std::sort(request.constructions.begin(), request.constructions.end());
        request.constructions.erase(
            std::unique(request.constructions.begin(),
                        request.constructions.end()),
            request.constructions.end());
        canonical_encoder encoded;
        encoded.id(request.facet);
        encoded.id(request.first_curve);
        encoded.id(request.second_curve);
        encode(encoded, request.point.x);
        encode(encoded, request.point.y);
        encode(encoded, request.point.z);
        request.canonical_key = domain_digest(
            {{'Y', 'G', 'B', 'R', 'E', 'C', '0', '7'}}, encoded.bytes());
        requests.push_back(std::move(request));
      }
  }
  std::sort(requests.begin(), requests.end(), reconciliation_request_less);
  requests.erase(std::unique(requests.begin(), requests.end(),
                             reconciliation_request_equal),
                 requests.end());
  return requests;
}
template <class T, class I>
status_or<local_refinement> build_facet(const symbolic_complex<T, I> &s,
                                        const validated_facet &vf,
                                        context_owner_token owner) {
  local_refinement f;
  f.facet = vf.id;
  f.operand = vf.operand;
  f.shell = vf.shell;
  f.projection = vf.projection;
  auto refv = [&](local_vertex_id id) {
    return local_vertex_ref{owner, vf.id, id};
  };
  auto refe = [&](local_atomic_edge_id id) {
    return local_atomic_edge_ref{owner, vf.id, id};
  };
  auto refh = [&](local_halfedge_id id) {
    return local_halfedge_ref{owner, vf.id, id};
  };
  auto refw = [&](local_boundary_walk_id id) {
    return local_boundary_walk_ref{owner, vf.id, id};
  };
  auto reff = [&](local_face_id id) {
    return local_face_ref{owner, vf.id, id};
  };
  std::set<symbolic_vertex_id> used;
  struct draft {
    symbolic_vertex_id a, b;
    std::optional<symbolic_curve_id> curve;
    std::vector<local_constraint_label> labels;
    bool boundary = false;
    bool artificial = false;
  };
  std::vector<draft> ds;
  for (auto use_id : vf.edge_uses) {
    const auto &view = s.directed_edge_views[use_id.value_for_debug()];
    const auto &seq = s.edge_sequences[view.sequence.value_for_debug()];
    source_edge_chain chain;
    chain.source = use_id;
    chain.forward = view.forward;
    for (std::size_t k = 1; k < seq.vertices.size(); ++k) {
      auto a = seq.vertices[k - 1], b = seq.vertices[k];
      local_constraint_label l;
      l.source_kind = local_constraint_source::source_boundary;
      l.source = use_id;
      l.direction = view.forward ? orientation_parity::agree
                                 : orientation_parity::opposite;
      ds.push_back({a, b, std::nullopt, {l}, true, false});
      used.insert(a);
      used.insert(b);
    }
    f.source_boundary.push_back(std::move(chain));
  }
  const auto &raw = *s.raw_events->payload;
  for (const auto &c : s.curves)
    if (c.kind == symbolic_curve_kind::atomic_interval && c.lower && c.upper) {
      bool incident = false;
      for (auto rid : c.raw_intervals) {
        const auto *it = find_raw_interval(raw, rid);
        if (!it)
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::local_refinement,
                            "missing_raw_interval");
        if (it->facets.operand_a_facet == vf.id ||
            it->facets.operand_b_facet == vf.id)
          incident = true;
      }
      if (!incident)
        continue;
      std::vector<local_constraint_label> ls;
      for (auto rid : c.raw_intervals) {
        const auto *it = find_raw_interval(raw, rid);
        if (!it)
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::local_refinement,
                            "missing_raw_interval");
        const auto &r = *it;
        if (r.facets.operand_a_facet != vf.id &&
            r.facets.operand_b_facet != vf.id)
          continue;
        local_constraint_label l;
        l.source_kind = local_constraint_source::intersection;
        l.source = rid;
        l.curve = c.id;
        l.direction = r.direction;
        for (const auto &d : r.derivations)
          l.derivations.push_back(d.construction);
        ls.push_back(std::move(l));
      }
      if (!ls.empty()) {
        ds.push_back(
            {*c.lower, *c.upper, c.id, std::move(ls), false, false});
        used.insert(*c.lower);
        used.insert(*c.upper);
      }
    }
  for (const auto &v : s.vertices)
    if (std::binary_search(v.facets.begin(), v.facets.end(), vf.id))
      used.insert(v.id);
  // Atomize every occurrence at all registry points on its closed segment.
  // This makes T-junctions and collinear overlap endpoints explicit before
  // geometric edge aggregation.
  std::vector<draft> atomized;
  for (const auto &d : ds) {
    const exact_segment2 segment{
        project(s.vertices[d.a.value_for_debug()].point, vf.projection),
        project(s.vertices[d.b.value_for_debug()].point, vf.projection)};
    const auto direction = segment.destination - segment.origin;
    std::vector<std::pair<exact_scalar, symbolic_vertex_id>> breakpoints;
    for (auto vertex : used) {
      const auto p =
          project(s.vertices[vertex.value_for_debug()].point, vf.projection);
      const auto relation = classify_point_segment(p, segment);
      if (relation != point_segment_relation::at_origin &&
          relation != point_segment_relation::open_interior &&
          relation != point_segment_relation::at_destination)
        continue;
      const auto offset = p - segment.origin;
      const auto parameter = !direction.x.is_zero() ? offset.x / direction.x
                                                     : offset.y / direction.y;
      breakpoints.push_back({parameter, vertex});
    }
    std::sort(breakpoints.begin(), breakpoints.end());
    breakpoints.erase(
        std::unique(breakpoints.begin(), breakpoints.end(),
                    [](const auto &a, const auto &b) {
                      return a.first == b.first;
                    }),
        breakpoints.end());
    if (breakpoints.size() < 2)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::local_refinement,
                        "constraint_atomization_endpoints");
    for (std::size_t i = 1; i < breakpoints.size(); ++i) {
      draft atom = d;
      atom.a = breakpoints[i - 1].second;
      atom.b = breakpoints[i].second;
      atomized.push_back(std::move(atom));
    }
  }
  ds = std::move(atomized);
  std::vector<symbolic_vertex_id> order(used.begin(), used.end());
  std::sort(order.begin(), order.end(), [&](auto a, auto b) {
    const auto &pa = s.vertices[a.value_for_debug()].point;
    const auto &pb = s.vertices[b.value_for_debug()].point;
    int c = lexicographic_compare(project(pa, vf.projection),
                                  project(pb, vf.projection));
    return c ? c < 0 : a < b;
  });
  std::map<std::uint64_t, local_vertex_id> vm;
  for (auto sid : order) {
    auto id = local_vertex_id::from_canonical_value(f.vertices.size());
    vm[sid.value_for_debug()] = id;
    f.vertices.push_back(
        {refv(id),
         sid,
         project(s.vertices[sid.value_for_debug()].point, vf.projection),
         {}});
    local_point_incidence pi;
    pi.vertex = refv(id);
    const auto &sv = s.vertices[sid.value_for_debug()];
    if (!sv.original_vertices.empty())
      pi.location = local_point_location::source_vertex;
    else if (!sv.undirected_edges.empty())
      pi.location = local_point_location::source_edge_interior;
    pi.used_by_edge = false;
    for (auto x : sv.raw_points) {
      pi.events.push_back(x);
      pi.sources.push_back(x);
    }
    f.point_incidences.push_back(std::move(pi));
  }
  // Exact audit: the registry must already contain every non-endpoint crossing.
  for (std::size_t i = 0; i < ds.size(); ++i)
    for (std::size_t j = i + 1; j < ds.size(); ++j) {
      exact_segment2 a{
          project(s.vertices[ds[i].a.value_for_debug()].point, vf.projection),
          project(s.vertices[ds[i].b.value_for_debug()].point, vf.projection)},
          b{project(s.vertices[ds[j].a.value_for_debug()].point, vf.projection),
            project(s.vertices[ds[j].b.value_for_debug()].point,
                    vf.projection)};
      auto r = relate_segments(a, b);
      if (r.dimension == intersection_dimension::point &&
          r.point_kind == segment_point_kind::proper_crossing)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::local_refinement,
                          "symbolic_reconciliation_required");
    }
  // A semantic dangling edge produces a weak boundary walk. Add the least
  // exact visible registered-vertex cut, one at a time, until every non-source
  // vertex has degree other than one. This is topology-driven; no length or
  // quality score participates in the choice.
  const auto source_polygon = [&] {
    std::vector<exact_point2> polygon;
    for (auto id : vf.ring)
      polygon.push_back(project(
          s.validated->payload->vertices[id.value_for_debug()].exact_coordinate,
          vf.projection));
    return polygon;
  }();
  const auto visible = [&](symbolic_vertex_id from, symbolic_vertex_id to) {
    const exact_segment2 candidate{
        project(s.vertices[from.value_for_debug()].point, vf.projection),
        project(s.vertices[to.value_for_debug()].point, vf.projection)};
    const auto midpoint = affine_interpolate(candidate.origin,
                                             candidate.destination,
                                             exact_scalar(1) / exact_scalar(2));
    auto location = classify_point_polygon(midpoint, source_polygon);
    if (!location.has_value() ||
        location.value().kind == point_region_kind::outside)
      return false;
    for (const auto &edge : ds) {
      if (same_edge_pair(from, to, edge.a, edge.b))
        return false;
      exact_segment2 existing{
          project(s.vertices[edge.a.value_for_debug()].point, vf.projection),
          project(s.vertices[edge.b.value_for_debug()].point, vf.projection)};
      const auto relation = relate_segments(candidate, existing);
      if (relation.dimension == intersection_dimension::segment ||
          (relation.dimension == intersection_dimension::point &&
           relation.point_kind == segment_point_kind::proper_crossing) ||
          (relation.dimension == intersection_dimension::point &&
           relation.point_kind == segment_point_kind::endpoint_interior))
        return false;
    }
    return true;
  };
  for (std::size_t cut_count = 0; cut_count < used.size() * used.size();
       ++cut_count) {
    std::map<symbolic_vertex_id, std::size_t> degree;
    for (const auto &edge : ds) {
      ++degree[edge.a];
      ++degree[edge.b];
    }
    std::optional<std::pair<symbolic_vertex_id, symbolic_vertex_id>> cut;
    for (auto from : used) {
      if (degree[from] != 1)
        continue;
      for (auto to : used) {
        if (from == to || !visible(from, to))
          continue;
        std::pair<symbolic_vertex_id, symbolic_vertex_id> candidate =
            from < to ? std::make_pair(from, to) : std::make_pair(to, from);
        if (!cut || candidate < *cut)
          cut = candidate;
      }
    }
    if (!cut)
      break;
    std::vector<std::pair<exact_scalar, symbolic_vertex_id>> points;
    exact_segment2 segment{
        project(s.vertices[cut->first.value_for_debug()].point, vf.projection),
        project(s.vertices[cut->second.value_for_debug()].point, vf.projection)};
    for (auto vertex : used) {
      const auto projected =
          project(s.vertices[vertex.value_for_debug()].point, vf.projection);
      const auto relation = classify_point_segment(projected, segment);
      if (relation == point_segment_relation::at_origin ||
          relation == point_segment_relation::open_interior ||
          relation == point_segment_relation::at_destination) {
        const auto direction = segment.destination - segment.origin;
        const auto offset = projected - segment.origin;
        const auto parameter = !direction.x.is_zero()
                                   ? offset.x / direction.x
                                   : offset.y / direction.y;
        points.push_back({parameter, vertex});
      }
    }
    std::sort(points.begin(), points.end());
    for (std::size_t i = 1; i < points.size(); ++i)
      ds.push_back({points[i - 1].second, points[i].second, std::nullopt, {},
                    false, true});
  }
  std::sort(ds.begin(), ds.end(), [](const auto &a, const auto &b) {
    auto ap = std::minmax(a.a, a.b), bp = std::minmax(b.a, b.b);
    return std::tie(ap.first, ap.second, a.boundary, a.curve) <
           std::tie(bp.first, bp.second, b.boundary, b.curve);
  });
  for (std::size_t i = 0; i < ds.size();) {
    std::size_t j = i + 1;
    while (j < ds.size() && same_edge_pair(ds[i].a, ds[i].b, ds[j].a, ds[j].b))
      ++j;
    auto lo = ds[i].a, hi = ds[i].b;
    const auto &lp = f.vertices[vm[lo.value_for_debug()].value_for_debug()].projected;
    const auto &hp = f.vertices[vm[hi.value_for_debug()].value_for_debug()].projected;
    if (lexicographic_compare(hp, lp) < 0)
      std::swap(lo, hi);
    local_atomic_edge e;
    e.id = refe(local_atomic_edge_id::from_canonical_value(f.edges.size()));
    e.lower = refv(vm[lo.value_for_debug()]);
    e.upper = refv(vm[hi.value_for_debug()]);
    for (std::size_t k = i; k < j; ++k) {
      e.labels.insert(e.labels.end(), ds[k].labels.begin(), ds[k].labels.end());
      e.source_boundary |= ds[k].boundary;
      e.artificial |= ds[k].artificial;
      if (ds[k].curve)
        e.canonical_interval = ds[k].curve;
    }
    f.edges.push_back(std::move(e));
    i = j;
  }
  for (auto &edge : f.edges) {
    std::sort(edge.labels.begin(), edge.labels.end(), label_less);
  }
  for (auto &e : f.edges)
    for (const auto &l : e.labels) {
      if (l.source_kind == local_constraint_source::source_boundary) {
        auto use = std::get<edge_use_id>(l.source);
        auto it =
            std::find_if(f.source_boundary.begin(), f.source_boundary.end(),
                         [&](const auto &x) { return use == x.source; });
        if (it != f.source_boundary.end())
          it->edges.push_back(e.id);
      } else {
        auto it =
            std::find_if(f.constraints.begin(), f.constraints.end(),
                         [&](const auto &x) { return x.source == l.source; });
        if (it == f.constraints.end())
          f.constraints.push_back(
              {l.source, {e.id}, l.direction, l.multiplicity});
        else
          it->edges.push_back(e.id);
      }
    }
  for (auto &e : f.edges) {
    auto n = f.halfedges.size();
    local_halfedge_id a = local_halfedge_id::from_canonical_value(n),
                      b = local_halfedge_id::from_canonical_value(n + 1);
    f.halfedges.push_back({refh(a),
                           refh(b),
                           {},
                           {},
                           e.id,
                           e.lower,
                           e.upper,
                           {},
                           {},
                           halfedge_direction::canonical});
    f.halfedges.push_back({refh(b),
                           refh(a),
                           {},
                           {},
                           e.id,
                           e.upper,
                           e.lower,
                           {},
                           {},
                           halfedge_direction::reverse});
    f.vertices[e.lower.id.value_for_debug()].outgoing.push_back(refh(a));
    f.vertices[e.upper.id.value_for_debug()].outgoing.push_back(refh(b));
    f.point_incidences[e.lower.id.value_for_debug()].used_by_edge = true;
    f.point_incidences[e.upper.id.value_for_debug()].used_by_edge = true;
  }
  for (auto &v : f.vertices)
    std::sort(v.outgoing.begin(), v.outgoing.end(), [&](auto a, auto b) {
      const auto &ha = f.halfedges[a.id.value_for_debug()];
      const auto &hb = f.halfedges[b.id.value_for_debug()];
      auto da = f.vertices[ha.destination.id.value_for_debug()].projected -
                v.projected,
           db = f.vertices[hb.destination.id.value_for_debug()].projected -
                v.projected;
      int c = ray_compare(da, db);
      return c ? c < 0 : a.id < b.id;
    });
  for (auto &h : f.halfedges) {
    const auto &star = f.vertices[h.destination.id.value_for_debug()].outgoing;
    auto it = std::find(star.begin(), star.end(), h.twin);
    if (it == star.end())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::local_refinement, "star_missing_twin");
    h.next = it == star.begin() ? star.back() : *(it - 1);
  }
  for (auto &h : f.halfedges)
    f.halfedges[h.next.id.value_for_debug()].previous = h.id;
  std::vector<bool> seen(f.halfedges.size());
  for (std::size_t i = 0; i < f.halfedges.size(); ++i)
    if (!seen[i]) {
      local_boundary_walk w;
      w.id = refw(local_boundary_walk_id::from_canonical_value(f.walks.size()));
      auto h = refh(local_halfedge_id::from_canonical_value(i));
      do {
        if (seen[h.id.value_for_debug()])
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::local_refinement,
                            "walk_not_permutation");
        seen[h.id.value_for_debug()] = true;
        w.halfedges.push_back(h);
        f.halfedges[h.id.value_for_debug()].walk = w.id;
        h = f.halfedges[h.id.value_for_debug()].next;
      } while (h.id.value_for_debug() != i);
      w.signed_double_area = area(w.halfedges, f);
      w.area_class = w.signed_double_area.is_zero()
                         ? boundary_area_class::zero_area
                         : (w.signed_double_area < exact_scalar(0)
                                ? boundary_area_class::negative
                                : boundary_area_class::positive);
      f.walks.push_back(std::move(w));
    }
  local_face unbounded;
  unbounded.id = reff(local_face_id::from_canonical_value(0));
  unbounded.extent = face_extent::unbounded;
  f.faces.push_back(unbounded);
  std::vector<std::vector<exact_point2>> positive_polygons;
  std::vector<local_boundary_walk_id> positive_walks;
  for (const auto &walk : f.walks)
    if (walk.area_class == boundary_area_class::positive) {
      std::vector<exact_point2> polygon;
      for (auto halfedge : walk.halfedges)
        polygon.push_back(
            f.vertices[f.halfedges[halfedge.id.value_for_debug()]
                           .origin.id.value_for_debug()]
                .projected);
      positive_polygons.push_back(std::move(polygon));
      positive_walks.push_back(walk.id.id);
    }
  const auto left_witness = [&](const local_boundary_walk &walk)
      -> status_or<exact_point2> {
    for (auto halfedge : walk.halfedges) {
      const auto &directed = f.halfedges[halfedge.id.value_for_debug()];
      const auto &origin =
          f.vertices[directed.origin.id.value_for_debug()].projected;
      const auto &destination =
          f.vertices[directed.destination.id.value_for_debug()].projected;
      const auto direction = destination - origin;
      if (direction.x.is_zero() && direction.y.is_zero())
        continue;
      const auto midpoint = affine_interpolate(origin, destination,
                                               exact_scalar(1) /
                                                   exact_scalar(2));
      exact_scalar scale(1);
      for (std::size_t attempt = 0; attempt < 256; ++attempt) {
        scale = scale / exact_scalar(2);
        exact_point2 witness{
            midpoint.x - direction.y * scale,
            midpoint.y + direction.x * scale};
        bool boundary = false;
        for (const auto &edge : f.edges) {
          const exact_segment2 segment{
              f.vertices[edge.lower.id.value_for_debug()].projected,
              f.vertices[edge.upper.id.value_for_debug()].projected};
          const auto relation = classify_point_segment(witness, segment);
          boundary |= relation == point_segment_relation::at_origin ||
                      relation == point_segment_relation::open_interior ||
                      relation == point_segment_relation::at_destination;
          if (edge.id != directed.edge) {
            const auto crossing =
                relate_segments(exact_segment2{midpoint, witness}, segment);
            boundary |= crossing.dimension != intersection_dimension::empty;
          }
        }
        if (!boundary)
          return witness;
      }
    }
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::local_refinement,
                      "face_witness_unavailable");
  };
  const auto signature = [&](const exact_point2 &witness) {
    std::vector<bool> result;
    for (const auto &polygon : positive_polygons) {
      auto relation = classify_point_polygon(witness, polygon);
      result.push_back(relation.has_value() &&
                       relation.value().kind ==
                           point_region_kind::open_interior);
    }
    return result;
  };
  std::map<std::vector<bool>, local_face_id> face_by_signature;
  face_by_signature[std::vector<bool>(positive_polygons.size(), false)] =
      local_face_id::from_canonical_value(0);
  for (auto &w : f.walks)
    if (w.area_class == boundary_area_class::positive) {
      auto witness = left_witness(w);
      if (!witness.has_value())
        return witness.error();
      local_face x;
      x.id = reff(local_face_id::from_canonical_value(f.faces.size()));
      x.role = cell_role::source_domain;
      x.walks.push_back(w.id);
      w.face = x.id;
      auto inserted = face_by_signature.emplace(signature(witness.value()),
                                                x.id.id);
      if (!inserted.second)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::local_refinement,
                          "positive_walk_face_equivalence");
      f.faces.push_back(std::move(x));
    }
  for (auto &w : f.walks)
    if (w.area_class != boundary_area_class::positive) {
      auto witness = left_witness(w);
      if (!witness.has_value())
        return witness.error();
      auto owner = face_by_signature.find(signature(witness.value()));
      if (owner == face_by_signature.end())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::local_refinement,
                          "walk_face_equivalence");
      local_face_id ownerface = owner->second;
      w.face = reff(ownerface);
      f.faces[ownerface.value_for_debug()].walks.push_back(w.id);
    }
  for (auto &h : f.halfedges)
    h.left_face = f.walks[h.walk.id.value_for_debug()].face;
  for (const auto &walk : f.walks)
    if (walk.area_class == boundary_area_class::positive) {
      std::set<local_vertex_id> vertices;
      for (auto halfedge : walk.halfedges)
        if (!vertices
                 .insert(f.halfedges[halfedge.id.value_for_debug()].origin.id)
                 .second)
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::local_refinement,
                            "non_jordan_decomposition_failed");
    }
  for (auto &face : f.faces)
    if (face.extent == face_extent::bounded) {
      auto outer =
          *std::find_if(face.walks.begin(), face.walks.end(), [&](auto w) {
            return f.walks[w.id.value_for_debug()].area_class ==
                   boundary_area_class::positive;
          });
      local_patch p;
      p.facet = vf.id;
      p.shell = vf.shell;
      p.operand = vf.operand;
      p.parent_face = face.id;
      p.outer = outer;
      p.decomposition = decomposition_kind::polygon_with_holes;
      for (auto walk : face.walks)
        for (auto halfedge : f.walks[walk.id.value_for_debug()].halfedges)
          if (f.edges[f.halfedges[halfedge.id.value_for_debug()]
                          .edge.id.value_for_debug()]
                  .artificial)
            p.decomposition = decomposition_kind::canonical_cut;
      p.signed_double_area =
          f.walks[outer.id.value_for_debug()].signed_double_area;
      for (auto w : face.walks)
        if (f.walks[w.id.value_for_debug()].area_class ==
            boundary_area_class::negative) {
          p.holes.push_back(w);
          p.signed_double_area =
              p.signed_double_area +
              f.walks[w.id.value_for_debug()].signed_double_area;
        }
      f.patches.push_back(std::move(p));
    }
  f.certificate.source_double_area =
      vf.projected_double_area < exact_scalar(0)
          ? exact_scalar(0) - vf.projected_double_area
          : vf.projected_double_area;
  for (const auto &p : f.patches)
    f.certificate.patch_double_area =
        f.certificate.patch_double_area + p.signed_double_area;
  f.certificate.vertices = f.vertices.size();
  f.certificate.edges = f.edges.size();
  f.certificate.halfedges = f.halfedges.size();
  f.certificate.faces = f.faces.size();
  f.certificate.bounded_faces = f.faces.size() - 1;
  f.certificate.source_domain_faces = f.patches.size();
  for (const auto &edge : f.edges)
    f.certificate.artificial_edges += edge.artificial;
  for (const auto &patch : f.patches)
    f.certificate.holes += patch.holes.size();
  for (const auto &w : f.walks) {
    f.certificate.walks_positive +=
        w.area_class == boundary_area_class::positive;
    f.certificate.walks_negative +=
        w.area_class == boundary_area_class::negative;
    f.certificate.walks_zero += w.area_class == boundary_area_class::zero_area;
  }
  std::vector<bool> vis(f.vertices.size());
  for (std::size_t i = 0; i < f.vertices.size(); ++i)
    if (!vis[i]) {
      ++f.certificate.components;
      std::vector<std::size_t> q{i};
      vis[i] = true;
      while (!q.empty()) {
        auto v = q.back();
        q.pop_back();
        for (auto hr : f.vertices[v].outgoing) {
          auto d = f.halfedges[hr.id.value_for_debug()]
                       .destination.id.value_for_debug();
          if (!vis[d]) {
            vis[d] = true;
            q.push_back(d);
          }
        }
      }
    }
  if (f.certificate.halfedges != 2 * f.certificate.edges ||
      f.certificate.bounded_faces + f.certificate.vertices !=
          f.certificate.edges + f.certificate.components ||
      f.certificate.vertices - f.certificate.edges + f.certificate.faces !=
          1 + f.certificate.components ||
      f.certificate.patch_double_area != f.certificate.source_double_area)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::local_refinement, "local_coverage");
  return f;
}

template <class T, class I> bool valid(const refined_facet_patches<T, I> &a) {
  if (!a.symbolic || !a.validated || a.owner != a.symbolic->owner ||
      a.validated.get() != a.symbolic->payload->validated.get())
    return false;
  if (a.facets.size() != a.validated->payload->facets.size())
    return false;
  for (std::size_t fi = 0; fi < a.facets.size(); ++fi) {
    const auto &f = a.facets[fi];
    const auto &source = a.validated->payload->facets[fi];
    if (f.facet.value_for_debug() != fi ||
        f.operand != source.operand || f.shell != source.shell ||
        f.projection != source.projection ||
        f.halfedges.size() != 2 * f.edges.size() || f.faces.empty() ||
        f.certificate.vertices + f.certificate.faces !=
            f.certificate.edges + 1 + f.certificate.components ||
        f.certificate.patch_double_area != f.certificate.source_double_area)
      return false;
    for (std::size_t i = 0; i < f.vertices.size(); ++i) {
      const auto &vertex = f.vertices[i];
      if (vertex.id.owner != a.owner || vertex.id.facet != f.facet ||
           vertex.id.id.value_for_debug() != i ||
          vertex.symbolic.value_for_debug() >= a.symbolic->payload->vertices.size() ||
          !(vertex.projected == project(a.symbolic->payload
                                             ->vertices[vertex.symbolic.value_for_debug()]
                                             .point,
                                         f.projection)))
        return false;
      std::vector<local_halfedge_ref> expected = vertex.outgoing;
      std::sort(expected.begin(), expected.end(), [&](auto left, auto right) {
        const auto &lh = f.halfedges[left.id.value_for_debug()];
        const auto &rh = f.halfedges[right.id.value_for_debug()];
        auto ld = f.vertices[lh.destination.id.value_for_debug()].projected -
                  vertex.projected;
        auto rd = f.vertices[rh.destination.id.value_for_debug()].projected -
                  vertex.projected;
        const auto order = ray_compare(ld, rd);
        return order ? order < 0 : left.id < right.id;
      });
      if (expected != vertex.outgoing)
        return false;
    }
    if (f.point_incidences.size() != f.vertices.size())
      return false;
    for (std::size_t i = 0; i < f.point_incidences.size(); ++i) {
      const auto &point_incidence = f.point_incidences[i];
      if (point_incidence.vertex != f.vertices[i].id ||
          point_incidence.used_by_edge !=
              !f.vertices[i].outgoing.empty())
        return false;
    }
    for (std::size_t i = 0; i < f.edges.size(); ++i) {
      const auto &edge = f.edges[i];
      if (edge.id.owner != a.owner || edge.id.facet != f.facet ||
          edge.id.id.value_for_debug() != i ||
          edge.lower.id.value_for_debug() >= f.vertices.size() ||
          edge.upper.id.value_for_debug() >= f.vertices.size() ||
          edge.lower.id == edge.upper.id ||
          (edge.artificial &&
           (edge.shared_semantic_edge || !edge.labels.empty() ||
            edge.canonical_interval)) ||
          (!edge.artificial &&
           (!edge.shared_semantic_edge || edge.labels.empty())))
        return false;
    }
    for (std::size_t i = 0; i < f.halfedges.size(); ++i) {
      const auto &h = f.halfedges[i];
      if (h.id.id.value_for_debug() != i ||
          h.id.owner != a.owner || h.id.facet != f.facet ||
          h.twin.id.value_for_debug() >= f.halfedges.size() ||
          h.next.id.value_for_debug() >= f.halfedges.size() ||
          h.previous.id.value_for_debug() >= f.halfedges.size() ||
          h.walk.id.value_for_debug() >= f.walks.size() ||
          h.left_face.id.value_for_debug() >= f.faces.size() ||
          f.halfedges[h.twin.id.value_for_debug()].twin.id != h.id.id ||
          f.halfedges[h.next.id.value_for_debug()].previous.id != h.id.id)
        return false;
    }
    exact_scalar patch_area(0);
    for (std::size_t i = 0; i < f.walks.size(); ++i) {
      const auto &walk = f.walks[i];
      if (walk.id.id.value_for_debug() != i || walk.halfedges.empty() ||
          area(walk.halfedges, f) != walk.signed_double_area)
        return false;
      auto cursor = walk.halfedges.front();
      for (std::size_t count = 0; count < walk.halfedges.size(); ++count) {
        if (cursor != walk.halfedges[count])
          return false;
        cursor = f.halfedges[cursor.id.value_for_debug()].next;
      }
      if (cursor != walk.halfedges.front())
        return false;
      std::set<local_vertex_id> walk_vertices;
      for (auto halfedge : walk.halfedges) {
        const auto &directed = f.halfedges[halfedge.id.value_for_debug()];
        if (directed.walk != walk.id || directed.left_face != walk.face)
          return false;
        if (walk.area_class == boundary_area_class::positive &&
            !walk_vertices.insert(directed.origin.id).second)
          return false;
      }
    }
    std::size_t unbounded = 0;
    for (std::size_t i = 0; i < f.faces.size(); ++i) {
      const auto &face = f.faces[i];
      if (face.id.id.value_for_debug() != i)
        return false;
      unbounded += face.extent == face_extent::unbounded;
      for (auto walk : face.walks)
        if (walk.id.value_for_debug() >= f.walks.size() ||
            f.walks[walk.id.value_for_debug()].face != face.id)
          return false;
    }
    if (unbounded != 1)
      return false;
    for (const auto &patch : f.patches) {
       if (patch.id.owner != a.owner || patch.id.facet != f.facet ||
           patch.facet != f.facet || patch.operand != f.operand ||
          patch.shell != f.shell ||
          patch.outer.id.value_for_debug() >= f.walks.size() ||
          patch.parent_face.id.value_for_debug() >= f.faces.size() ||
          !(exact_scalar(0) < patch.signed_double_area))
        return false;
      patch_area = patch_area + patch.signed_double_area;
    }
    if (patch_area != f.certificate.patch_double_area)
      return false;
    if (f.certificate.source_domain_faces != f.patches.size() ||
        f.certificate.bounded_faces + f.certificate.vertices !=
            f.certificate.edges + f.certificate.components)
      return false;
    for (const auto &chain : f.source_boundary) {
      if (chain.source.value_for_debug() >=
              a.validated->payload->edge_uses.size() ||
          chain.edges.empty())
        return false;
      for (auto edge : chain.edges)
        if (edge.id.value_for_debug() >= f.edges.size() ||
            !f.edges[edge.id.value_for_debug()].source_boundary)
          return false;
    }
  }
  for (std::size_t i = 0; i < a.shared_edges.size(); ++i) {
    const auto &shared = a.shared_edges[i];
    if (shared.id.value_for_debug() != i || !(shared.lower < shared.upper) ||
        shared.occurrences.empty())
      return false;
    for (auto occurrence : shared.occurrences) {
      if (occurrence.facet.value_for_debug() >= a.facets.size())
        return false;
      const auto &facet = a.facets[occurrence.facet.value_for_debug()];
      if (occurrence.id.value_for_debug() >= facet.edges.size() ||
          facet.edges[occurrence.id.value_for_debug()].shared_semantic_edge !=
              shared.id)
        return false;
    }
  }
  return semantic(a) == a.canonical_bytes &&
         invocation(a) == a.artifact_bytes &&
         artifact_digest_for(a) == a.artifact_digest;
}
template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    const auto &a =
        *static_cast<const refined_facet_patches<T, I> *>(v.payload);
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::local_refinement;
    r.slot = v.slot;
    r.artifact_type_tag = v.artifact_type_tag;
    r.artifact_schema = v.artifact_schema;
    r.setup_digest = e.setup_digest;
    r.artifact_digest = v.artifact_digest;
    r.invariant_set_digest = s.invariant_set_digest;
    bool ok = valid(a);
    r.outcome = ok ? verification_outcome::pass
                   : verification_outcome::invariant_failure;
    bool failed = false;
    for (auto c : s.required_invariants) {
      const auto status = ok ? check_status::passed
                             : failed ? check_status::not_run_due_to_prior_failure
                                      : check_status::failed;
      r.results.push_back({c, status, {}, 0});
      failed |= status == check_status::failed;
    }
    r.dependency_digests = {a.symbolic_digest, a.validated_digest};
    auto b = encode_verification_report(r);
    if (!b.has_value())
      return b.error();
    r.report_digest =
        domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}}, b.value());
    return r;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::local_refinement,
                      "local_verifier_exception");
  }
}
template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}
} // namespace

status_or<bool> register_local_refinement_verifier(verifier_registry &r,
                                                   coordinate_tag c,
                                                   index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::refined_facet_patches;
  x.artifact_type_tag = refined_facet_patches_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = refined_facet_patches_schema;
  x.mandatory = {invariant_code::local_binding,
                 invariant_code::local_constraints, invariant_code::local_dcel,
                 invariant_code::local_coverage,
                 invariant_code::local_canonical_encoding};
  x.exhaustive = x.mandatory;
  if (c == coordinate_tag::binary32 && i == index_tag::uint32)
    x.callback = &callback<float, std::uint32_t>;
  else if (c == coordinate_tag::binary32)
    x.callback = &callback<float, std::uint64_t>;
  else if (i == index_tag::uint32)
    x.callback = &callback<double, std::uint32_t>;
  else
    x.callback = &callback<double, std::uint64_t>;
  return r.register_verifier(std::move(x));
}
template <class T, class I>
status_or<
    std::shared_ptr<const published_artifact<refined_facet_patches<T, I>>>>
refine_source_facets(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::local_refinement, "cancelled");
    auto existing = ctx.artifacts().latest(
        artifact_slot::refined_facet_patches);
    if (existing)
      return std::static_pointer_cast<const published_artifact<
          refined_facet_patches<T, I>>>(existing);
    auto sym = build_symbolic_complex(ctx);
    if (!sym.has_value())
      return sym.error();
    performance_scope producer(ctx.performance_collector_for_internal_use(),
                               boolean_stage::local_refinement,
                               performance_role::producer);
    auto symbolic = sym.value();
    for (std::uint64_t pass = 0;; ++pass) {
      auto requests = audit_crossings(*symbolic);
      if (!requests.has_value())
        return requests.error();
      if (requests.value().empty())
        break;
      if (pass != 0)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::local_refinement,
                          "nonprogressing_reconciliation");
      auto successor = reconcile_symbolic_complex(
          ctx, symbolic, std::move(requests.value()));
      if (!successor.has_value())
        return successor.error();
      symbolic = successor.value();
    }
    stage_transaction<refined_facet_patches<T, I>> tx(
        ctx.owner(), boolean_stage::local_refinement,
        artifact_slot::refined_facet_patches,
        std::make_unique<refined_facet_patches<T, I>>(),
        ctx.performance_collector_for_internal_use());
    auto &a = tx.draft();
    a.owner = ctx.owner();
    a.setup_digest = ctx.replay().setup;
    a.symbolic_digest = symbolic->artifact_digest;
    a.symbolic = symbolic;
    a.validated = symbolic->payload->validated;
    a.validated_digest = a.validated->artifact_digest;
    a.kernel_policy_digest = symbolic->payload->kernel_policy_digest;
    a.constructions = symbolic->payload->constructions;
    for (const auto &vf : a.validated->payload->facets) {
      auto f = build_facet(*symbolic->payload, vf, ctx.owner());
      if (!f.has_value())
        return f.error();
      a.facets.push_back(std::move(f.value()));
    }
    std::map<std::pair<symbolic_vertex_id, symbolic_vertex_id>,
             shared_atomic_edge_id>
        shared;
    for (auto &f : a.facets)
      for (auto &e : f.edges)
        if (!e.artificial) {
          auto x = f.vertices[e.lower.id.value_for_debug()].symbolic,
               y = f.vertices[e.upper.id.value_for_debug()].symbolic;
          if (y < x)
            std::swap(x, y);
          auto key = std::make_pair(x, y);
          auto it = shared.find(key);
          if (it == shared.end()) {
            auto id = shared_atomic_edge_id::from_canonical_value(
                a.shared_edges.size());
            shared[key] = id;
            a.shared_edges.push_back({id, x, y, {}});
            it = shared.find(key);
          }
          e.shared_semantic_edge = it->second;
          a.shared_edges[it->second.value_for_debug()].occurrences.push_back(
              e.id);
        }
    std::uint64_t patch = 0;
    std::uint64_t vertices = 0, edges = 0, halfedges = 0, walks = 0,
                  faces = 0;
    for (auto &f : a.facets)
      for (auto &p : f.patches)
        p.id = local_patch_ref{ctx.owner(), f.facet,
                              local_patch_id::from_canonical_value(patch++)};
    for (const auto &f : a.facets) {
      vertices += f.vertices.size();
      edges += f.edges.size();
      halfedges += f.halfedges.size();
      walks += f.walks.size();
      faces += f.faces.size();
    }
    std::vector<resource_reservation> charges;
    auto reserve = [&](resource_kind kind,
                       std::uint64_t count) -> status_or<bool> {
      auto charge = ctx.accountant().reserve_scoped(
          kind, count, boolean_stage::local_refinement);
      if (!charge.has_value())
        return charge.error();
      charges.push_back(std::move(charge.value()));
      return true;
    };
    for (const auto &request :
         {std::make_pair(resource_kind::local_vertices, vertices),
          std::make_pair(resource_kind::local_atomic_edges, edges),
          std::make_pair(resource_kind::local_halfedges, halfedges),
          std::make_pair(resource_kind::local_boundary_walks, walks),
          std::make_pair(resource_kind::local_faces, faces),
          std::make_pair(resource_kind::local_patches, patch),
          std::make_pair(resource_kind::local_certificate_entries,
                         static_cast<std::uint64_t>(a.facets.size()))}) {
      auto reserved = reserve(request.first, request.second);
      if (!reserved.has_value())
        return reserved.error();
    }
    a.canonical_bytes = semantic(a);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::local_refinement,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::refined_facet_patches, type_tag<T, I>(),
        refined_facet_patches_schema, ctx.options().verification);
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
    performance_count(performance_counter::dcel_entities,
                      vertices + edges + halfedges + walks + faces + patch);
    producer.finish();
    auto ok = tx.freeze_and_verify(type_tag<T, I>(),
                                   refined_facet_patches_schema, 1,
                                   a.artifact_digest, spec.value(), env,
                                   ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::local_refinement, "cancelled");
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::local_refinement, "local_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::local_refinement, "local_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T, I)                                                             \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<refined_facet_patches<T, I>>>>  \
  refine_source_facets(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST
} // namespace mesh_boolean
} // namespace ygor
