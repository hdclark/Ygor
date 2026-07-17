#include "YgorMeshesBooleanSymbolicRegistry.h"
#include "YgorMeshesBooleanExecutor.h"
#include <algorithm>
#include <new>

#if defined(__FAST_MATH__) ||                                                  \
    (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Boolean symbolic registry requires strict floating-point compilation"
#endif
namespace ygor {
namespace mesh_boolean {
namespace {
template <class T, class I> std::uint64_t type_tag() {
  return symbolic_complex_type_tag +
         (static_cast<std::uint64_t>(std::is_same<T, double>::value) << 8) +
         static_cast<std::uint64_t>(std::is_same<I, std::uint64_t>::value);
}
void enc_point(canonical_encoder &e, const exact_point3 &p) {
  p.x.encode(e);
  p.y.encode(e);
  p.z.encode(e);
}
void enc_line(canonical_encoder &e, const exact_line3 &l) {
  enc_point(e, l.anchor);
  l.direction.x.encode(e);
  l.direction.y.encode(e);
  l.direction.z.encode(e);
}
void enc_feature(canonical_encoder &e, const feature_ref &f) {
  e.byte(static_cast<std::uint8_t>(f.index()));
  std::visit([&](const auto &x) {
    using X = typename std::decay<decltype(x)>::type;
    if constexpr (std::is_same<X, original_vertex_ref>::value) {
      e.id(x.operand); e.id(x.vertex);
    } else if constexpr (std::is_same<X, facet_ref>::value) {
      e.id(x.operand); e.id(x.facet);
    } else e.id(x);
  }, f);
}
void enc_ownership(canonical_encoder &e, const raw_curve_ownership &x) {
  e.id(x.operand); enc_feature(e, x.source);
  e.byte(static_cast<std::uint8_t>(x.direction)); e.u32(x.multiplicity);
}
exact_scalar param(const exact_segment3 &s, const exact_point3 &p) {
  return segment_parameter(p, s);
}
exact_scalar lparam(const exact_line3 &l, const exact_point3 &p) {
  if (!l.direction.x.is_zero())
    return (p.x - l.anchor.x) / l.direction.x;
  if (!l.direction.y.is_zero())
    return (p.y - l.anchor.y) / l.direction.y;
  return (p.z - l.anchor.z) / l.direction.z;
}
bool same_line(const exact_line3 &a, const exact_line3 &b) {
  auto c = cross(a.direction, b.direction);
  return classify_point_line(a.anchor, b) == point_line_relation::on_carrier &&
         dot(c, c).is_zero();
}
exact_line3 canonical_line(const exact_line3 &line) {
  exact_line3 out = line;
  exact_scalar pivot;
  if (!out.direction.x.is_zero()) pivot = out.direction.x;
  else if (!out.direction.y.is_zero()) pivot = out.direction.y;
  else if (!out.direction.z.is_zero()) pivot = out.direction.z;
  else throw std::logic_error("zero carrier direction");
  out.direction = out.direction * (exact_scalar(1) / pivot);
  exact_scalar shift;
  if (!out.direction.x.is_zero()) shift = exact_scalar(0) - out.anchor.x;
  else if (!out.direction.y.is_zero()) shift = exact_scalar(0) - out.anchor.y;
  else shift = exact_scalar(0) - out.anchor.z;
  out.anchor = out.anchor + out.direction * shift;
  return out;
}
bool same_canonical_line(const exact_line3 &a, const exact_line3 &b) {
  return a.anchor == b.anchor && a.direction.x == b.direction.x &&
         a.direction.y == b.direction.y && a.direction.z == b.direction.z;
}
bool upper_half(const exact_vector2 &v) {
  return v.y.sign() == exact_sign::positive ||
         (v.y.is_zero() && v.x.sign() != exact_sign::negative);
}
int angular_compare(const exact_vector2 &a, const exact_vector2 &b) {
  const bool ah = upper_half(a), bh = upper_half(b);
  if (ah != bh) return ah ? -1 : 1;
  const auto cross_value = a.x * b.y - a.y * b.x;
  if (!cross_value.is_zero())
    return cross_value.sign() == exact_sign::positive ? -1 : 1;
  const auto dot_value = a.x * b.x + a.y * b.y;
  if (dot_value.sign() == exact_sign::positive) return 0;
  return ah ? -1 : 1;
}
exact_vector2 project_vector(const exact_vector3 &v, projection_axis axis) {
  if (axis == projection_axis::drop_x) return {v.y, v.z};
  if (axis == projection_axis::drop_y) return {v.z, v.x};
  return {v.x, v.y};
}
exact_vector3 plane_normal(const exact_plane3 &p) {
  auto scalar = [](const big_int &x) { return exact_scalar(x, big_uint(1)); };
  exact_vector3 n{scalar(p.a), scalar(p.b), scalar(p.c)};
  if (p.oriented == orientation_parity::opposite)
    n = n * exact_scalar(-1);
  return n;
}
exact_vector2 radial_direction(const exact_line3 &line,
                                const exact_vector3 &normal) {
  exact_vector3 axis = line.direction.x.is_zero()
                           ? exact_vector3{exact_scalar(1), exact_scalar(0), exact_scalar(0)}
                           : exact_vector3{exact_scalar(0), exact_scalar(1), exact_scalar(0)};
  auto first = cross(line.direction, axis);
  auto second = cross(line.direction, first);
  return {dot(normal, first), dot(normal, second)};
}
template <class T, class I>
std::vector<std::uint8_t> semantic(const symbolic_complex<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN06";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(symbolic_complex_schema);
  e.u64(a.vertices.size());
  for (const auto &v : a.vertices) {
    e.id(v.id);
    enc_point(e, v.point);
    auto ids = [&](const auto &xs) { e.u64(xs.size()); for (auto id : xs) e.id(id); };
    ids(v.original_vertices); ids(v.edge_uses); ids(v.undirected_edges);
    ids(v.facets); ids(v.overlap_regions); ids(v.raw_points);
    ids(v.incident_curves);
    ids(v.constructions);
  }
  e.u64(a.curves.size());
  for (const auto &c : a.curves) {
    e.id(c.id);
    e.byte(static_cast<std::uint8_t>(c.kind));
    enc_line(e, c.carrier);
    e.boolean(c.parent_carrier.has_value()); if (c.parent_carrier) e.id(*c.parent_carrier);
    e.boolean(c.lower.has_value()); if (c.lower) e.id(*c.lower);
    e.boolean(c.upper.has_value()); if (c.upper) e.id(*c.upper);
    if (c.parameters) {
      c.parameters->lower.encode(e);
      c.parameters->upper.encode(e);
    }
    e.u64(c.raw_intervals.size()); for (auto id : c.raw_intervals) e.id(id);
    e.u64(c.raw_carriers.size()); for (auto id : c.raw_carriers) e.id(id);
    e.u64(c.facets.size()); for (auto id : c.facets) e.id(id);
    e.u64(c.overlap_regions.size()); for (auto id : c.overlap_regions) e.id(id);
    e.u64(c.ownership.size()); for (const auto &x : c.ownership) enc_ownership(e, x);
    e.u64(c.ordered_vertices.size()); for (auto id : c.ordered_vertices) e.id(id);
    e.u64(c.ordered_intervals.size()); for (auto id : c.ordered_intervals) e.id(id);
    e.u64(c.constructions.size()); for (auto id : c.constructions) e.id(id);
  }
  e.u64(a.directed_edge_views.size());
  for (const auto &v : a.directed_edge_views) {
    e.id(v.edge_use); e.id(v.sequence); e.boolean(v.forward);
  }
  e.u64(a.angular_orders.size());
  for (const auto &o : a.angular_orders) {
    e.byte(static_cast<std::uint8_t>(o.kind)); e.id(o.facet); e.id(o.vertex);
    e.u64(o.groups.size());
    for (const auto &g : o.groups) {
      g.direction.x.encode(e); g.direction.y.encode(e);
      e.u64(g.curves.size()); for (auto id : g.curves) e.id(id);
    }
  }
  e.u64(a.radial_orders.size());
  for (const auto &o : a.radial_orders) {
    e.byte(static_cast<std::uint8_t>(o.kind)); e.id(o.carrier);
    e.u64(o.groups.size());
    for (const auto &g : o.groups) {
      g.direction.x.encode(e); g.direction.y.encode(e);
      e.u64(g.facets.size()); for (auto id : g.facets) e.id(id);
    }
  }
  e.u64(a.original_vertices.size());
  for (const auto &m : a.original_vertices) { e.id(m.source); e.id(m.symbolic); }
  e.u64(a.raw_points.size());
  for (const auto &m : a.raw_points) { e.id(m.source); e.id(m.symbolic); }
  e.u64(a.raw_carriers.size());
  for (const auto &m : a.raw_carriers) {
    e.u64(m.source_ordinal); e.id(m.candidate); e.id(m.carrier);
  }
  e.u64(a.edge_sequences.size());
  for (const auto &s : a.edge_sequences) {
    e.id(s.edge);
    e.id(s.canonical_origin);
    e.id(s.canonical_destination);
    e.u64(s.vertices.size());
    for (std::size_t i = 0; i < s.vertices.size(); ++i) {
      e.id(s.vertices[i]);
      s.parameters[i].encode(e);
    }
  }
  e.u64(a.raw_intervals.size());
  for (const auto &m : a.raw_intervals) {
    e.id(m.source);
    e.byte(static_cast<std::uint8_t>(m.parity));
    e.u64(m.atomic_intervals.size());
    for (auto id : m.atomic_intervals) e.id(id);
  }
  e.u64(a.raw_regions.size());
  for (const auto &m : a.raw_regions) {
    e.id(m.source);
    e.u64(m.boundary_cycles.size());
    for (const auto &cycle : m.boundary_cycles) {
      e.byte(static_cast<std::uint8_t>(cycle.role));
      e.u64(cycle.vertices.size());
      for (auto id : cycle.vertices) e.id(id);
      e.u64(cycle.boundary_intervals.size());
      for (const auto &edge : cycle.boundary_intervals) {
        e.u64(edge.size());
        for (auto id : edge) e.id(id);
      }
    }
  }
  e.u64(a.reconciliation_history.size());
  for (const auto &request : a.reconciliation_history) {
    e.u16(request.schema);
    e.raw(request.prior_digest.bytes.data(), request.prior_digest.bytes.size());
    e.u64(request.prior_generation);
    e.id(request.facet);
    e.id(request.first_curve);
    e.id(request.second_curve);
    enc_point(e, request.point);
    e.u64(request.constructions.size());
    for (auto id : request.constructions)
      e.id(id);
    e.raw(request.canonical_key.bytes.data(), request.canonical_key.bytes.size());
  }
  return e.bytes();
}
template <class T, class I>
std::vector<std::uint8_t> invocation(const symbolic_complex<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBSYM06";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(symbolic_complex_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.raw(a.upstream_digest.bytes.data(), 16);
  e.raw(a.validated_digest.bytes.data(), 16);
  e.raw(a.kernel_policy_digest.bytes.data(), 16);
  e.u64(a.generation);
  e.boolean(bool(a.prior_generation));
  if (a.prior_generation)
    e.raw(a.prior_generation->artifact_digest.bytes.data(), 16);
  e.byte_string(a.canonical_symbolic_bytes);
  return e.bytes();
}
template <class T, class I>
digest artifact_digest_for(const symbolic_complex<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBART01";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.byte(static_cast<std::uint8_t>(artifact_slot::symbolic_complex));
  e.u64(type_tag<T, I>());
  e.u16(symbolic_complex_schema);
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}
template <class T, class I> bool valid(const symbolic_complex<T, I> &a) {
  if (!a.raw_events || !a.validated || !a.constructions ||
      a.upstream_digest != a.raw_events->artifact_digest ||
      a.kernel_policy_digest != a.raw_events->payload->kernel_policy_digest ||
      a.constructions.get() != a.raw_events->payload->constructions.get() ||
      a.constructions->owner != a.owner || a.validated.get() !=
          a.raw_events->payload->candidates->payload->validated.get())
    return false;
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    if (a.vertices[i].id.value_for_debug() != i ||
        (i && lexicographic_compare(a.vertices[i - 1].point,
                                    a.vertices[i].point) >= 0))
      return false;
  auto valid_constructions = [&](const auto &ids) {
    return std::all_of(ids.begin(), ids.end(), [&](auto id) {
      return id.valid() && id.value_for_debug() < a.constructions->nodes.size();
    });
  };
  for (const auto &v : a.vertices)
    if (!valid_constructions(v.constructions)) return false;
  for (const auto &request : a.reconciliation_history)
    if (request.schema != symbolic_reconciliation_schema_v1 ||
        !valid_constructions(request.constructions) ||
        request.facet.value_for_debug() >= a.validated->payload->facets.size())
      return false;
  for (const auto &c : a.curves)
    if (!valid_constructions(c.constructions)) return false;
  bool seen_interval = false;
  for (std::size_t i = 0; i < a.curves.size(); ++i) {
    const auto &c = a.curves[i];
    if (c.id.value_for_debug() != i ||
        (c.kind != symbolic_curve_kind::carrier &&
         c.kind != symbolic_curve_kind::atomic_interval) ||
        !same_canonical_line(c.carrier, canonical_line(c.carrier)))
      return false;
    seen_interval |= c.kind == symbolic_curve_kind::atomic_interval;
    if (seen_interval && c.kind == symbolic_curve_kind::carrier) return false;
    if (c.kind == symbolic_curve_kind::carrier) {
      if (c.parent_carrier || c.parameters || c.lower || c.upper) return false;
      exact_scalar previous;
      bool have_previous = false;
      for (auto id : c.ordered_vertices) {
        if (id.value_for_debug() >= a.vertices.size() ||
            classify_point_line(a.vertices[id.value_for_debug()].point, c.carrier) !=
                point_line_relation::on_carrier) return false;
        auto t = lparam(c.carrier, a.vertices[id.value_for_debug()].point);
        if (have_previous && !(previous < t)) return false;
        previous = t; have_previous = true;
      }
      previous = exact_scalar(); have_previous = false;
      for (auto id : c.ordered_intervals) {
        if (id.value_for_debug() >= a.curves.size()) return false;
        const auto &atom = a.curves[id.value_for_debug()];
        if (atom.kind != symbolic_curve_kind::atomic_interval ||
            atom.parent_carrier != c.id || !atom.parameters ||
            (have_previous && atom.parameters->lower < previous)) return false;
        previous = atom.parameters->upper; have_previous = true;
      }
    }
    if (c.kind == symbolic_curve_kind::atomic_interval) {
      if (!c.parent_carrier || !c.parameters || !c.lower || !c.upper ||
          c.raw_intervals.empty() ||
          !(c.parameters->lower < c.parameters->upper) ||
          c.lower == c.upper ||
          c.parent_carrier->value_for_debug() >= a.curves.size() ||
          a.curves[c.parent_carrier->value_for_debug()].kind !=
              symbolic_curve_kind::carrier ||
          c.lower->value_for_debug() >= a.vertices.size() ||
          c.upper->value_for_debug() >= a.vertices.size())
        return false;
      if (c.ownership.empty()) return false;
      for (const auto &owner : c.ownership)
        if (!owner.operand.valid() || owner.multiplicity == 0) return false;
      auto point_at = [&](const exact_scalar &t) {
        return c.carrier.anchor + c.carrier.direction * t;
      };
      if (!(point_at(c.parameters->lower) ==
            a.vertices[c.lower->value_for_debug()].point) ||
          !(point_at(c.parameters->upper) ==
             a.vertices[c.upper->value_for_debug()].point))
        return false;
      for (auto endpoint : {*c.lower, *c.upper}) {
        const auto &incident = a.vertices[endpoint.value_for_debug()].incident_curves;
        if (!std::binary_search(incident.begin(), incident.end(), c.id) ||
            !std::binary_search(incident.begin(), incident.end(), *c.parent_carrier))
          return false;
      }
    }
  }
  if (a.original_vertices.size() != a.validated->payload->vertices.size() ||
      a.raw_points.size() != a.raw_events->payload->points.size() ||
      a.raw_intervals.size() != a.raw_events->payload->intervals.size() ||
      a.raw_regions.size() != a.raw_events->payload->regions.size() ||
       a.raw_carriers.size() != a.raw_events->payload->carriers.size() ||
       a.edge_sequences.size() != a.validated->payload->edges.size() ||
       a.directed_edge_views.size() != a.validated->payload->edge_uses.size())
    return false;
  for (std::size_t i = 0; i < a.original_vertices.size(); ++i) {
    const auto &m = a.original_vertices[i];
    if (m.source.value_for_debug() != i ||
        m.symbolic.value_for_debug() >= a.vertices.size() ||
        !(a.vertices[m.symbolic.value_for_debug()].point ==
          a.validated->payload->vertices[i].exact_coordinate)) return false;
  }
  const auto raw_event_count = a.raw_events->payload->event_index.size();
  if (a.raw_point_index.size() != raw_event_count ||
      a.raw_interval_index.size() != raw_event_count ||
      a.raw_region_index.size() != raw_event_count)
    return false;
  for (std::size_t i = 0; i < a.raw_points.size(); ++i) {
    const auto &m = a.raw_points[i];
    const auto *raw = find_raw_point(*a.raw_events->payload, m.source);
    if (!raw || a.raw_point_index[m.source.value_for_debug()] != i ||
        m.symbolic.value_for_debug() >= a.vertices.size() ||
        !(raw->point == a.vertices[m.symbolic.value_for_debug()].point)) return false;
  }
  for (std::size_t i = 0; i < a.raw_carriers.size(); ++i) {
    const auto &m = a.raw_carriers[i];
    if (m.source_ordinal != i || m.carrier.value_for_debug() >= a.curves.size() ||
        m.candidate != a.raw_events->payload->carriers[i].candidate ||
        a.curves[m.carrier.value_for_debug()].kind != symbolic_curve_kind::carrier ||
        !same_line(a.curves[m.carrier.value_for_debug()].carrier,
                   a.raw_events->payload->carriers[i].carrier)) return false;
  }
  for (const auto &vertex : a.vertices) {
    for (auto use_id : vertex.edge_uses) {
      if (use_id.value_for_debug() >= a.validated->payload->edge_uses.size()) return false;
      const auto &use = a.validated->payload->edge_uses[use_id.value_for_debug()];
      if (!std::binary_search(vertex.edge_uses.begin(), vertex.edge_uses.end(), use.twin) ||
          !std::binary_search(vertex.undirected_edges.begin(),
                              vertex.undirected_edges.end(), use.edge) ||
          !std::binary_search(vertex.facets.begin(), vertex.facets.end(), use.facet) ||
          !std::binary_search(vertex.facets.begin(), vertex.facets.end(),
              a.validated->payload->edge_uses[use.twin.value_for_debug()].facet)) return false;
    }
  }
  for (std::size_t mapping_ordinal = 0;
       mapping_ordinal < a.raw_intervals.size(); ++mapping_ordinal) {
    const auto &m = a.raw_intervals[mapping_ordinal];
    if (m.parity != orientation_parity::agree &&
        m.parity != orientation_parity::opposite)
      return false;
    const auto *raw = find_raw_interval(*a.raw_events->payload, m.source);
    if (!raw || a.raw_interval_index[m.source.value_for_debug()] != mapping_ordinal ||
        m.atomic_intervals.empty())
      return false;
    std::optional<symbolic_vertex_id> previous;
    for (std::size_t k = 0; k < m.atomic_intervals.size(); ++k) {
      auto id = m.atomic_intervals[m.parity == orientation_parity::agree
                                      ? k : m.atomic_intervals.size() - 1 - k];
      if (id.value_for_debug() >= a.curves.size()) return false;
      const auto &atom = a.curves[id.value_for_debug()];
      if (atom.kind != symbolic_curve_kind::atomic_interval ||
          std::find(atom.raw_intervals.begin(), atom.raw_intervals.end(), m.source) ==
              atom.raw_intervals.end()) return false;
      auto from = m.parity == orientation_parity::agree ? atom.lower : atom.upper;
      auto to = m.parity == orientation_parity::agree ? atom.upper : atom.lower;
      if (previous && previous != from) return false;
      previous = to;
    }
    auto point_map = [&](raw_event_id id)
        -> std::optional<symbolic_vertex_id> {
      const auto *found = find_raw_point_mapping(a, id);
      if (!found) return std::nullopt;
      return found->symbolic;
    };
    const auto &first = a.curves[m.atomic_intervals[
        m.parity == orientation_parity::agree ? 0 : m.atomic_intervals.size() - 1]
                                     .value_for_debug()];
    if ((m.parity == orientation_parity::agree ? first.lower : first.upper) !=
             point_map(raw->lower_point) ||
        previous != point_map(raw->upper_point)) return false;
  }
  for (const auto &s : a.edge_sequences) {
    if (s.vertices.size() != s.parameters.size() || s.vertices.size() < 2 ||
        !s.parameters.front().is_zero() ||
        s.parameters.back() != exact_scalar(1))
      return false;
    for (std::size_t i = 1; i < s.parameters.size(); ++i)
      if (!(s.parameters[i - 1] < s.parameters[i]))
        return false;
    if (s.edge.value_for_debug() >= a.validated->payload->edges.size()) return false;
    const auto &edge = a.validated->payload->edges[s.edge.value_for_debug()];
    if (s.canonical_origin != edge.first || s.canonical_destination != edge.second ||
        s.vertices.front() != a.original_vertices[edge.first.value_for_debug()].symbolic ||
        s.vertices.back() != a.original_vertices[edge.second.value_for_debug()].symbolic)
      return false;
    for (std::size_t i = 0; i < s.vertices.size(); ++i) {
      if (s.vertices[i].value_for_debug() >= a.vertices.size()) return false;
      const auto &vertex = a.vertices[s.vertices[i].value_for_debug()];
      if (!std::binary_search(vertex.undirected_edges.begin(),
                              vertex.undirected_edges.end(), s.edge) ||
          param({a.validated->payload->vertices[edge.first.value_for_debug()].exact_coordinate,
                 a.validated->payload->vertices[edge.second.value_for_debug()].exact_coordinate},
                vertex.point) != s.parameters[i]) return false;
    }
  }
  for (std::size_t i = 0; i < a.directed_edge_views.size(); ++i) {
    const auto &view = a.directed_edge_views[i];
    if (view.edge_use.value_for_debug() != i) return false;
    const auto &use = a.validated->payload->edge_uses[i];
    if (view.sequence != use.edge ||
        view.forward != (use.origin == a.validated->payload->edges[
                            use.edge.value_for_debug()].first)) return false;
    const auto &twin = a.directed_edge_views[use.twin.value_for_debug()];
    if (twin.sequence != view.sequence || twin.forward == view.forward) return false;
  }
  std::pair<facet_id, symbolic_vertex_id> previous_angular;
  bool have_angular = false;
  for (const auto &order : a.angular_orders) {
    if (order.kind != symbolic_order_kind::planar_angular || order.groups.empty() ||
        order.facet.value_for_debug() >= a.validated->payload->facets.size() ||
        order.vertex.value_for_debug() >= a.vertices.size()) return false;
    const auto key = std::make_pair(order.facet, order.vertex);
    if (have_angular && !(previous_angular < key)) return false;
    previous_angular = key; have_angular = true;
    std::vector<symbolic_curve_id> expected;
    const auto &facet = a.validated->payload->facets[order.facet.value_for_debug()];
    const auto vertex = order.vertex;
    for (const auto &curve : a.curves)
      if (curve.kind == symbolic_curve_kind::atomic_interval &&
          (curve.lower == vertex || curve.upper == vertex) &&
          intersect_line_plane(curve.carrier, facet.plane).kind == line_plane_kind::contained)
        expected.push_back(curve.id);
    std::vector<symbolic_curve_id> actual;
    std::optional<exact_vector2> previous_direction;
    for (const auto &group : order.groups) {
      if (group.curves.empty() || (group.direction.x.is_zero() && group.direction.y.is_zero()) ||
          (previous_direction && angular_compare(*previous_direction, group.direction) >= 0))
        return false;
      previous_direction = group.direction;
      for (auto id : group.curves) {
        if (id.value_for_debug() >= a.curves.size()) return false;
        const auto &curve = a.curves[id.value_for_debug()];
        if (curve.kind != symbolic_curve_kind::atomic_interval ||
            (curve.lower != vertex && curve.upper != vertex)) return false;
        auto d = curve.carrier.direction *
                 exact_scalar(curve.lower == vertex ? 1 : -1);
        if (angular_compare(group.direction, project_vector(d, facet.projection)) != 0)
          return false;
        actual.push_back(id);
      }
    }
    std::sort(actual.begin(), actual.end()); std::sort(expected.begin(), expected.end());
    if (actual != expected) return false;
  }
  const auto expected_radial_orders = static_cast<std::size_t>(std::count_if(
      a.curves.begin(), a.curves.end(), [](const auto &c) {
        return c.kind == symbolic_curve_kind::carrier && !c.facets.empty();
      }));
  if (a.radial_orders.size() != expected_radial_orders)
    return false;
  symbolic_curve_id previous_carrier;
  bool have_carrier = false;
  for (const auto &order : a.radial_orders) {
    if (order.kind != symbolic_order_kind::carrier_radial || order.groups.empty() ||
        order.carrier.value_for_debug() >= a.curves.size() ||
        (have_carrier && !(previous_carrier < order.carrier))) return false;
    previous_carrier = order.carrier; have_carrier = true;
    const auto &carrier = a.curves[order.carrier.value_for_debug()];
    if (carrier.kind != symbolic_curve_kind::carrier) return false;
    std::vector<facet_id> actual;
    std::optional<exact_vector2> previous_direction;
    for (const auto &group : order.groups) {
      if (group.facets.empty() || (group.direction.x.is_zero() && group.direction.y.is_zero()) ||
          (previous_direction && angular_compare(*previous_direction, group.direction) >= 0))
        return false;
      previous_direction = group.direction;
      for (auto id : group.facets) {
        if (id.value_for_debug() >= a.validated->payload->facets.size() ||
             angular_compare(group.direction, radial_direction(
                 carrier.carrier, plane_normal(a.validated->payload->facets[id.value_for_debug()].plane))) != 0)
          return false;
        actual.push_back(id);
      }
    }
    std::sort(actual.begin(), actual.end());
    if (actual != carrier.facets) return false;
  }
  for (std::size_t mapping_ordinal = 0;
       mapping_ordinal < a.raw_regions.size(); ++mapping_ordinal) {
    const auto &m = a.raw_regions[mapping_ordinal];
    const auto *raw_region = find_raw_region(*a.raw_events->payload, m.source);
    if (!raw_region ||
        a.raw_region_index[m.source.value_for_debug()] != mapping_ordinal ||
        m.boundary_cycles.size() != raw_region->boundary_cycles.size())
      return false;
    for (std::size_t i = 0; i < m.boundary_cycles.size(); ++i) {
      const auto &cycle = m.boundary_cycles[i];
      const auto &raw_cycle = raw_region->boundary_cycles[i];
      if ((cycle.role != region_cycle_role::outer &&
           cycle.role != region_cycle_role::hole) ||
          cycle.role != raw_cycle.role ||
          cycle.vertices.size() != raw_cycle.vertices.size() ||
          cycle.boundary_intervals.size() != raw_cycle.intervals.size())
        return false;
      for (std::size_t j = 0; j < cycle.vertices.size(); ++j) {
        if (cycle.vertices[j].value_for_debug() >= a.vertices.size() ||
            cycle.boundary_intervals[j].empty()) return false;
         const auto *point_map = find_raw_point_mapping(a, raw_cycle.vertices[j]);
         const auto *interval_map = find_raw_interval_mapping(a, raw_cycle.intervals[j]);
         if (!point_map || point_map->symbolic != cycle.vertices[j] ||
             !interval_map ||
            interval_map->atomic_intervals != cycle.boundary_intervals[j])
          return false;
        const auto next = cycle.vertices[(j + 1) % cycle.vertices.size()];
        for (auto id : cycle.boundary_intervals[j]) {
          if (id.value_for_debug() >= a.curves.size()) return false;
          const auto &atom = a.curves[id.value_for_debug()];
          if (atom.kind != symbolic_curve_kind::atomic_interval ||
              std::find(atom.raw_intervals.begin(), atom.raw_intervals.end(),
                        raw_cycle.intervals[j]) == atom.raw_intervals.end())
            return false;
        }
        const auto &first = a.curves[cycle.boundary_intervals[j].front().value_for_debug()];
        const auto &last = a.curves[cycle.boundary_intervals[j].back().value_for_debug()];
        if (interval_map->parity == orientation_parity::agree) {
          if (first.lower != cycle.vertices[j] || last.upper != next) return false;
        } else if (first.lower != next || last.upper != cycle.vertices[j]) {
          return false;
        }
      }
    }
  }
  return semantic(a) == a.canonical_symbolic_bytes &&
         invocation(a) == a.artifact_bytes &&
         artifact_digest_for(a) == a.artifact_digest;
}
template <class T, class I>
status_or<bool> independently_verify(const symbolic_complex<T, I> &a,
                                     const verification_environment_view &env) {
  auto members = checked_add(a.validated->payload->vertices.size(),
                              a.raw_events->payload->points.size(),
                              boolean_stage::symbolic_registry);
  if (!members.has_value()) return members.error();
  members = checked_add(members.value(), a.reconciliation_history.size(),
                        boolean_stage::symbolic_registry);
  if (!members.has_value()) return members.error();
  std::optional<resource_reservation> work_charge, scratch_charge;
  if (env.accountant) {
    auto charged = env.accountant->reserve_scoped(
        resource_kind::verifier_work, members.value(),
        boolean_stage::symbolic_registry);
    if (!charged.has_value()) return charged.error();
    work_charge.emplace(std::move(charged.value()));
    auto bytes = checked_multiply(members.value(), sizeof(exact_point3),
                                  boolean_stage::symbolic_registry);
    if (!bytes.has_value()) return bytes.error();
    charged = env.accountant->reserve_scoped(
        resource_kind::verifier_scratch_bytes, bytes.value(),
        boolean_stage::symbolic_registry);
    if (!charged.has_value()) return charged.error();
    scratch_charge.emplace(std::move(charged.value()));
  }
  std::vector<exact_point3> reconstructed;
  reconstructed.reserve(members.value());
  for (const auto &v : a.validated->payload->vertices)
    reconstructed.push_back(v.exact_coordinate);
  for (const auto &p : a.raw_events->payload->points)
    reconstructed.push_back(p.point);
  for (const auto &request : a.reconciliation_history)
    reconstructed.push_back(request.point);
  // Exhaustive identity is deliberately independent of sorting/fingerprints.
  for (std::size_t i = 0; i < reconstructed.size(); ++i)
    for (std::size_t j = i + 1; j < reconstructed.size(); ++j) {
      if (((i * reconstructed.size() + j) & 255U) == 0 && env.cancelled &&
          env.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::symbolic_registry, "cancelled");
      const bool equal = reconstructed[i] == reconstructed[j];
      const auto left = std::find_if(a.vertices.begin(), a.vertices.end(),
          [&](const auto &v) { return v.point == reconstructed[i]; });
      const auto right = std::find_if(a.vertices.begin(), a.vertices.end(),
          [&](const auto &v) { return v.point == reconstructed[j]; });
      if ((left == a.vertices.end()) || (right == a.vertices.end()) ||
          (equal != (left->id == right->id)))
        return false;
    }
  std::sort(reconstructed.begin(), reconstructed.end(), [](const auto &x,
                                                            const auto &y) {
    return lexicographic_compare(x, y) < 0;
  });
  reconstructed.erase(std::unique(reconstructed.begin(), reconstructed.end()),
                      reconstructed.end());
  if (reconstructed.size() != a.vertices.size()) return false;
  for (std::size_t i = 0; i < reconstructed.size(); ++i) {
    if ((i & 63U) == 0 && env.cancelled && env.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::symbolic_registry, "cancelled");
    if (!(reconstructed[i] == a.vertices[i].point)) return false;
  }
  // Compare every raw carrier pair without using producer interning helpers.
  const auto &raw_carriers = a.raw_events->payload->carriers;
  for (std::size_t i = 0; i < raw_carriers.size(); ++i) {
    if (i >= a.raw_carriers.size()) return false;
    for (std::size_t j = i + 1; j < raw_carriers.size(); ++j) {
      const bool equal = same_line(raw_carriers[i].carrier,
                                   raw_carriers[j].carrier);
      if (equal != (a.raw_carriers[i].carrier == a.raw_carriers[j].carrier))
        return false;
    }
  }
  // Small rational interval-union oracle: derive every atom from all registered
  // points on each raw interval and compare its exact coverage set.
  for (const auto &mapping : a.raw_intervals) {
    const auto *raw = find_raw_interval(*a.raw_events->payload, mapping.source);
    if (!raw) return false;
    const auto line = canonical_line(raw->carrier);
    auto endpoint = [&](raw_event_id id) -> const exact_point3 * {
      const auto *p = find_raw_point(*a.raw_events->payload, id);
      return p ? &p->point : nullptr;
    };
    const auto *p0 = endpoint(raw->lower_point), *p1 = endpoint(raw->upper_point);
    if (!p0 || !p1) return false;
    auto lo = lparam(line, *p0), hi = lparam(line, *p1);
    if (hi < lo) std::swap(lo, hi);
    std::vector<exact_scalar> cuts{lo, hi};
    for (const auto &vertex : a.vertices)
      if (classify_point_line(vertex.point, line) == point_line_relation::on_carrier) {
        const auto t = lparam(line, vertex.point);
        if (!(t < lo) && !(hi < t)) cuts.push_back(t);
      }
    std::sort(cuts.begin(), cuts.end());
    cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
    std::vector<std::pair<exact_scalar, exact_scalar>> expected;
    for (std::size_t i = 1; i < cuts.size(); ++i)
      if (cuts[i - 1] < cuts[i]) expected.push_back({cuts[i - 1], cuts[i]});
    std::vector<std::pair<exact_scalar, exact_scalar>> actual;
    for (auto id : mapping.atomic_intervals) {
      if (id.value_for_debug() >= a.curves.size()) return false;
      const auto &atom = a.curves[id.value_for_debug()];
      if (!atom.parameters || !same_line(line, atom.carrier)) return false;
      actual.push_back({atom.parameters->lower, atom.parameters->upper});
    }
    std::sort(actual.begin(), actual.end());
    if (actual != expected) return false;
  }
  if (env.cancelled && env.cancelled())
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::symbolic_registry, "cancelled");
  return valid(a);
}
template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    const auto &a = *static_cast<const symbolic_complex<T, I> *>(v.payload);
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::symbolic_registry;
    r.slot = v.slot;
    r.artifact_type_tag = v.artifact_type_tag;
    r.artifact_schema = v.artifact_schema;
    r.setup_digest = e.setup_digest;
    r.artifact_digest = v.artifact_digest;
    r.invariant_set_digest = s.invariant_set_digest;
    auto verified = independently_verify(a, e);
    if (!verified.has_value()) return verified.error();
    r.outcome = verified.value() ? verification_outcome::pass
                                 : verification_outcome::invariant_failure;
    bool failed = false;
    for (auto c : s.required_invariants) {
      auto status =
          failed ? check_status::not_run_due_to_prior_failure
                 : r.passed() ? check_status::passed : check_status::failed;
      r.results.push_back({c, status, {}, 0});
      failed |= status == check_status::failed;
    }
    r.dependency_digests = {a.upstream_digest, a.validated_digest};
    auto b = encode_verification_report(r);
    if (!b.has_value())
      return b.error();
    r.report_digest =
        domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}}, b.value());
    return r;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::symbolic_registry,
                      "symbolic_verifier_exception");
  }
}
template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}
} // namespace
status_or<bool> register_symbolic_registry_verifier(verifier_registry &r,
                                                    coordinate_tag c,
                                                    index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::symbolic_complex;
  x.artifact_type_tag = symbolic_complex_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = symbolic_complex_schema;
  x.mandatory = {invariant_code::symbolic_binding,
                 invariant_code::symbolic_identity,
                 invariant_code::symbolic_order,
                 invariant_code::symbolic_canonical_encoding};
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
status_or<std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>
build_symbolic_complex_impl(
    boolean_context<T, I> &ctx,
    const std::vector<symbolic_reconciliation_request> &requests,
    std::shared_ptr<const published_artifact<symbolic_complex<T, I>>> prior) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::symbolic_registry, "cancelled");
    status_or<std::shared_ptr<const published_artifact<raw_event_set<T, I>>>> raw =
        prior ? prior->payload->raw_events : discover_intersection_events(ctx);
    if (!raw.has_value())
      return raw.error();
    performance_scope producer(ctx.performance_collector_for_internal_use(),
                               boolean_stage::symbolic_registry,
                               performance_role::producer);
    stage_transaction<symbolic_complex<T, I>> tx(
        ctx.owner(), boolean_stage::symbolic_registry,
        artifact_slot::symbolic_complex,
        std::make_unique<symbolic_complex<T, I>>(),
        ctx.performance_collector_for_internal_use());
    auto &a = tx.draft();
    a.owner = ctx.owner();
    a.setup_digest = ctx.replay().setup;
    a.upstream_digest = raw.value()->artifact_digest;
    a.raw_events = raw.value();
    a.constructions = raw.value()->payload->constructions;
    a.validated = raw.value()->payload->candidates->payload->validated;
    a.validated_digest = a.validated->artifact_digest;
    a.kernel_policy_digest = raw.value()->payload->kernel_policy_digest;
    const auto raw_event_count = raw.value()->payload->event_index.size();
    const auto missing_mapping = std::numeric_limits<std::uint64_t>::max();
    a.raw_point_index.assign(raw_event_count, missing_mapping);
    a.raw_interval_index.assign(raw_event_count, missing_mapping);
    a.raw_region_index.assign(raw_event_count, missing_mapping);
    a.generation = prior ? prior->generation + 1 : 1;
    a.prior_generation = prior;
    if (prior)
      a.reconciliation_history = prior->payload->reconciliation_history;
    a.reconciliation_history.insert(a.reconciliation_history.end(),
                                    requests.begin(), requests.end());
    auto work = checked_add(a.validated->payload->vertices.size(),
                            raw.value()->payload->points.size(),
                            boolean_stage::symbolic_registry);
    if (!work.has_value()) return work.error();
    work = checked_add(work.value(), raw.value()->payload->intervals.size(),
                       boolean_stage::symbolic_registry);
    if (!work.has_value()) return work.error();
    work = checked_add(work.value(), a.reconciliation_history.size(),
                       boolean_stage::symbolic_registry);
    if (!work.has_value()) return work.error();
    auto work_charge = ctx.accountant().reserve_scoped(
        resource_kind::work_units, work.value(),
        boolean_stage::symbolic_registry);
    if (!work_charge.has_value()) return work_charge.error();
    auto private_bytes = checked_multiply(work.value(), sizeof(exact_point3),
                                          boolean_stage::symbolic_registry);
    if (!private_bytes.has_value()) return private_bytes.error();
    auto private_charge = ctx.accountant().reserve_scoped(
        resource_kind::stage_private_bytes, private_bytes.value(),
        boolean_stage::symbolic_registry);
    if (!private_charge.has_value()) return private_charge.error();
    struct member {
      exact_point3 p;
      std::optional<original_vertex_id> ov;
      std::optional<raw_event_id> rp;
    };
    constexpr std::size_t frontier = 8;
    const auto member_count = a.validated->payload->vertices.size() +
                               raw.value()->payload->points.size() +
                               a.reconciliation_history.size();
    std::vector<std::vector<member>> member_shards(
        (member_count + frontier - 1) / frontier);
    std::vector<deterministic_task> member_tasks;
    for (std::size_t shard = 0; shard < member_shards.size(); ++shard) {
      member_tasks.push_back({0x0601000000000000ULL + shard,
          [&, shard]() -> status_or<bool> {
            if (ctx.cancelled())
              return make_error(boolean_error_code::resource_limit,
                                boolean_stage::symbolic_registry, "cancelled");
            const auto begin = shard * frontier;
            const auto end = std::min(member_count, begin + frontier);
            auto &out = member_shards[shard];
            out.reserve(end - begin);
            for (auto i = begin; i < end; ++i) {
              if (i < a.validated->payload->vertices.size()) {
                const auto &v = a.validated->payload->vertices[i];
                out.push_back({v.exact_coordinate, v.id, std::nullopt});
              } else if (i < a.validated->payload->vertices.size() +
                                 raw.value()->payload->points.size()) {
                const auto &p = raw.value()->payload->points[
                    i - a.validated->payload->vertices.size()];
                out.push_back({p.point, std::nullopt, p.id});
              } else {
                const auto &request = a.reconciliation_history[
                    i - a.validated->payload->vertices.size() -
                    raw.value()->payload->points.size()];
                out.push_back({request.point, std::nullopt, std::nullopt});
              }
            }
            return true;
          }});
    }
    cancellation_source member_cancel;
    auto member_result = ctx.executor().run(std::move(member_tasks),
                                             member_cancel.token());
    if (!member_result.has_value()) return member_result.error();
    std::vector<member> m;
    m.reserve(member_count);
    for (auto &shard : member_shards)
      m.insert(m.end(), std::make_move_iterator(shard.begin()),
               std::make_move_iterator(shard.end()));
    std::sort(m.begin(), m.end(), [](const auto &x, const auto &y) {
      return lexicographic_compare(x.p, y.p) < 0;
    });
    for (std::size_t i = 0; i < m.size();) {
      if ((i & 63U) == 0 && ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::symbolic_registry, "cancelled");
      std::size_t j = i + 1;
      while (j < m.size() && m[j].p == m[i].p)
        ++j;
      symbolic_vertex v;
      v.id = symbolic_vertex_id::from_canonical_value(a.vertices.size());
      v.point = m[i].p;
      for (std::size_t k = i; k < j; ++k) {
        if (m[k].ov) {
          v.original_vertices.push_back(*m[k].ov);
          a.original_vertices.push_back({*m[k].ov, v.id});
        }
        if (m[k].rp) {
          v.raw_points.push_back(*m[k].rp);
          a.raw_point_index[m[k].rp->value_for_debug()] = a.raw_points.size();
          a.raw_points.push_back({*m[k].rp, v.id});
        }
      }
      a.vertices.push_back(std::move(v));
      i = j;
    }
    for (const auto &request : requests) {
      auto vertex = std::find_if(a.vertices.begin(), a.vertices.end(),
                                 [&](const auto &v) {
                                   return v.point == request.point;
                                 });
      if (vertex == a.vertices.end())
        throw std::logic_error("reconciliation vertex missing");
      vertex->facets.push_back(request.facet);
      vertex->constructions.insert(vertex->constructions.end(),
                                   request.constructions.begin(),
                                   request.constructions.end());
    }
    // Seed incidence, then close edge-use/edge/facet topology to a fixed point.
    for (auto &sv : a.vertices) {
      for (auto oid : sv.original_vertices) {
        const auto &ov = a.validated->payload->vertices[oid.value_for_debug()];
        sv.edge_uses.insert(sv.edge_uses.end(), ov.ordered_outgoing_link.begin(),
                            ov.ordered_outgoing_link.end());
        for (auto use_id : ov.ordered_outgoing_link) {
          const auto &use =
              a.validated->payload->edge_uses[use_id.value_for_debug()];
          sv.undirected_edges.push_back(use.edge);
          sv.facets.push_back(use.facet);
        }
      }
      for (auto rid : sv.raw_points) {
        const auto *rp = find_raw_point(*raw.value()->payload, rid);
        if (!rp)
          throw std::logic_error("raw point incidence");
        for (const auto &inc : rp->incidences) {
          if (auto x = std::get_if<edge_use_id>(&inc.source))
            sv.edge_uses.push_back(*x);
          else if (auto x = std::get_if<undirected_edge_id>(&inc.source))
            sv.undirected_edges.push_back(*x);
          else if (auto x = std::get_if<facet_id>(&inc.source))
            sv.facets.push_back(*x);
          else if (auto x = std::get_if<facet_ref>(&inc.source))
            sv.facets.push_back(x->facet);
          else if (auto x = std::get_if<original_vertex_id>(&inc.source))
            sv.original_vertices.push_back(*x);
        }
        for (const auto &derivation : rp->derivations)
          sv.constructions.push_back(derivation.construction);
      }
      auto uniq = [](auto &x) {
        std::sort(x.begin(), x.end());
        x.erase(std::unique(x.begin(), x.end()), x.end());
      };
      uniq(sv.original_vertices);
      uniq(sv.edge_uses);
      uniq(sv.undirected_edges);
      uniq(sv.facets);
      uniq(sv.constructions);
      bool changed = true;
      while (changed) {
        const auto counts = std::make_tuple(sv.edge_uses.size(),
                                            sv.undirected_edges.size(),
                                            sv.facets.size());
        const auto edge_uses = sv.edge_uses;
        const auto undirected_edges = sv.undirected_edges;
        for (auto use_id : edge_uses) {
          const auto &use = a.validated->payload->edge_uses[use_id.value_for_debug()];
          sv.undirected_edges.push_back(use.edge);
          sv.facets.push_back(use.facet);
          sv.edge_uses.push_back(use.twin);
          sv.facets.push_back(a.validated->payload->edge_uses[
                                  use.twin.value_for_debug()].facet);
        }
        for (auto edge_id : undirected_edges) {
          const auto &edge = a.validated->payload->edges[edge_id.value_for_debug()];
          for (auto use_id : edge.uses) {
            sv.edge_uses.push_back(use_id);
            sv.facets.push_back(a.validated->payload->edge_uses[
                                    use_id.value_for_debug()].facet);
          }
        }
        uniq(sv.edge_uses); uniq(sv.undirected_edges); uniq(sv.facets);
        changed = counts != std::make_tuple(sv.edge_uses.size(),
                                            sv.undirected_edges.size(),
                                            sv.facets.size());
      }
    }
    auto vid = [&](const exact_point3 &p) {
      auto it = std::lower_bound(a.vertices.begin(), a.vertices.end(), p,
                                 [](const auto &v, const auto &q) {
                                   return lexicographic_compare(v.point, q) < 0;
                                 });
      if (it == a.vertices.end() || !(it->point == p))
        throw std::logic_error("missing symbolic point");
      return it->id;
    };
    const auto line_count = raw.value()->payload->carriers.size() +
                            raw.value()->payload->intervals.size();
    std::vector<std::vector<exact_line3>> line_shards(
        (line_count + frontier - 1) / frontier);
    std::vector<deterministic_task> line_tasks;
    for (std::size_t shard = 0; shard < line_shards.size(); ++shard) {
      line_tasks.push_back({0x0602000000000000ULL + shard,
          [&, shard]() -> status_or<bool> {
            if (ctx.cancelled())
              return make_error(boolean_error_code::resource_limit,
                                boolean_stage::symbolic_registry, "cancelled");
            const auto begin = shard * frontier;
            const auto end = std::min(line_count, begin + frontier);
            auto &out = line_shards[shard];
            out.reserve(end - begin);
            for (auto i = begin; i < end; ++i) {
              const auto &line = i < raw.value()->payload->carriers.size()
                  ? raw.value()->payload->carriers[i].carrier
                  : raw.value()->payload->intervals[
                        i - raw.value()->payload->carriers.size()].carrier;
              out.push_back(canonical_line(line));
            }
            return true;
          }});
    }
    cancellation_source line_cancel;
    auto line_result = ctx.executor().run(std::move(line_tasks),
                                           line_cancel.token());
    if (!line_result.has_value()) return line_result.error();
    std::vector<exact_line3> lines;
    lines.reserve(line_count);
    for (auto &shard : line_shards)
      lines.insert(lines.end(), std::make_move_iterator(shard.begin()),
                   std::make_move_iterator(shard.end()));
    std::sort(lines.begin(), lines.end(), [](const auto &x, const auto &y) {
      int c = lexicographic_compare(x.anchor, y.anchor);
      if (c)
        return c < 0;
      c = x.direction.x.compare(y.direction.x);
      if (c)
        return c < 0;
      c = x.direction.y.compare(y.direction.y);
      return c ? c < 0 : x.direction.z < y.direction.z;
    });
    lines.erase(std::unique(lines.begin(), lines.end(), same_canonical_line),
                lines.end());
    for (const auto &l : lines) {
      symbolic_curve c;
      c.id = symbolic_curve_id::from_canonical_value(a.curves.size());
      c.carrier = l;
      for (std::size_t ri = 0; ri < raw.value()->payload->carriers.size(); ++ri) {
        const auto &r = raw.value()->payload->carriers[ri];
        if (same_line(l, r.carrier)) {
          c.raw_carriers.push_back(r.candidate);
          c.facets.insert(c.facets.end(), r.facets.begin(), r.facets.end());
          a.raw_carriers.push_back({ri, r.candidate, c.id});
          for (const auto &derivation : r.derivations)
            c.constructions.push_back(derivation.construction);
        }
      }
      std::sort(c.raw_carriers.begin(), c.raw_carriers.end());
      c.raw_carriers.erase(std::unique(c.raw_carriers.begin(), c.raw_carriers.end()),
                           c.raw_carriers.end());
      std::sort(c.facets.begin(), c.facets.end());
      c.facets.erase(std::unique(c.facets.begin(), c.facets.end()), c.facets.end());
      std::sort(c.constructions.begin(), c.constructions.end());
      c.constructions.erase(std::unique(c.constructions.begin(), c.constructions.end()),
                            c.constructions.end());
      a.curves.push_back(std::move(c));
    }
    const auto carrier_count = a.curves.size();
    for (std::size_t ci = 0; ci < carrier_count; ++ci) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::symbolic_registry, "cancelled");
      const auto parent_line = a.curves[ci].carrier;
      const auto parent_id = a.curves[ci].id;
      struct coverage {
        const raw_interval_event *raw;
        exact_scalar lo, hi;
        symbolic_vertex_id lower, upper;
        orientation_parity parity;
      };
      std::vector<coverage> covers;
      std::vector<std::pair<exact_scalar, symbolic_vertex_id>> cuts;
      for (const auto &q : raw.value()->payload->intervals) {
        if (!same_line(parent_line, q.carrier))
          continue;
        const auto *lp = find_raw_point(*raw.value()->payload, q.lower_point);
        const auto *up = find_raw_point(*raw.value()->payload, q.upper_point);
        if (!lp || !up)
          throw std::logic_error("interval endpoint");
        auto lo = lparam(parent_line, lp->point);
        auto hi = lparam(parent_line, up->point);
        auto lower = vid(lp->point), upper = vid(up->point);
        auto parity = orientation_parity::agree;
        if (hi < lo) {
          std::swap(lo, hi);
          std::swap(lower, upper);
          parity = orientation_parity::opposite;
        }
        covers.push_back({&q, lo, hi, lower, upper, parity});
        cuts.push_back({lo, lower});
        cuts.push_back({hi, upper});
      }
      for (const auto &vtx : a.vertices) {
        if (classify_point_line(vtx.point, parent_line) !=
            point_line_relation::on_carrier)
          continue;
        auto t = lparam(parent_line, vtx.point);
        for (const auto &c : covers)
          if (!(t < c.lo) && !(c.hi < t)) {
            cuts.push_back({t, vtx.id});
            break;
          }
      }
      std::sort(cuts.begin(), cuts.end(), [](const auto &x, const auto &y) {
        return x.first == y.first ? x.second < y.second : x.first < y.first;
      });
       cuts.erase(std::unique(cuts.begin(), cuts.end(), [](const auto &x, const auto &y) {
                    if (x.first == y.first && x.second != y.second)
                      throw std::logic_error("equal carrier parameter");
                    return x.first == y.first;
                  }), cuts.end());
      for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
        auto mid = (cuts[i].first + cuts[i + 1].first) / exact_scalar(2);
        symbolic_curve atom;
        atom.kind = symbolic_curve_kind::atomic_interval;
        atom.carrier = parent_line;
        atom.parent_carrier = parent_id;
        atom.parameters = exact_interval{cuts[i].first, cuts[i + 1].first, true, true};
        atom.lower = cuts[i].second;
        atom.upper = cuts[i + 1].second;
        for (const auto &c : covers)
          if (!(mid < c.lo) && !(c.hi < mid)) {
            atom.raw_intervals.push_back(c.raw->id);
            atom.ownership.insert(atom.ownership.end(), c.raw->ownership.begin(),
                                  c.raw->ownership.end());
            for (const auto &derivation : c.raw->derivations)
              atom.constructions.push_back(derivation.construction);
          }
        if (atom.raw_intervals.empty())
          continue;
        std::sort(atom.ownership.begin(), atom.ownership.end(),
                  canonical_ownership_less);
        atom.ownership.erase(
            std::unique(atom.ownership.begin(), atom.ownership.end(),
                        canonical_ownership_equal),
            atom.ownership.end());
        std::sort(atom.constructions.begin(), atom.constructions.end());
        atom.constructions.erase(std::unique(atom.constructions.begin(), atom.constructions.end()),
                                 atom.constructions.end());
        atom.id = symbolic_curve_id::from_canonical_value(a.curves.size());
        for (auto rid : atom.raw_intervals) {
          auto *map = find_raw_interval_mapping(a, rid);
          if (!map) {
            auto cover = std::find_if(covers.begin(), covers.end(),
                                      [&](const auto &c) {
                                        return c.raw->id == rid;
                                      });
            if (cover == covers.end())
              throw std::logic_error("raw interval coverage");
            a.raw_interval_index[rid.value_for_debug()] = a.raw_intervals.size();
            a.raw_intervals.push_back({rid, {}, cover->parity});
            map = &a.raw_intervals.back();
          }
          map->atomic_intervals.push_back(atom.id);
        }
        a.vertices[atom.lower->value_for_debug()].incident_curves.push_back(atom.id);
        a.vertices[atom.upper->value_for_debug()].incident_curves.push_back(atom.id);
        a.vertices[atom.lower->value_for_debug()].incident_curves.push_back(parent_id);
        a.vertices[atom.upper->value_for_debug()].incident_curves.push_back(parent_id);
        a.curves[ci].ordered_intervals.push_back(atom.id);
        a.curves.push_back(std::move(atom));
      }
      for (const auto &cut : cuts) a.curves[ci].ordered_vertices.push_back(cut.second);
    }
    for (const auto &region : raw.value()->payload->regions) {
      raw_region_mapping map;
      map.source = region.id;
      for (const auto &cycle : region.boundary_cycles) {
        symbolic_region_boundary_cycle out;
        out.role = cycle.role;
        for (auto raw_id : cycle.vertices) {
          const auto *p = find_raw_point_mapping(a, raw_id);
          if (!p)
            throw std::logic_error("region boundary point");
          out.vertices.push_back(p->symbolic);
          a.vertices[p->symbolic.value_for_debug()].overlap_regions.push_back(region.id);
        }
        for (auto raw_id : cycle.intervals) {
          const auto *interval = find_raw_interval_mapping(a, raw_id);
          if (!interval)
            throw std::logic_error("region boundary interval");
          out.boundary_intervals.push_back(interval->atomic_intervals);
          for (auto atom_id : interval->atomic_intervals)
            a.curves[atom_id.value_for_debug()].overlap_regions.push_back(region.id);
        }
        map.boundary_cycles.push_back(std::move(out));
      }
      a.raw_region_index[region.id.value_for_debug()] = a.raw_regions.size();
      a.raw_regions.push_back(std::move(map));
    }
    const auto &vv = *a.validated->payload;
    for (const auto &e : vv.edges) {
      const auto &p = vv.vertices[e.first.value_for_debug()].exact_coordinate;
      const auto &q = vv.vertices[e.second.value_for_debug()].exact_coordinate;
      std::vector<std::pair<exact_scalar, symbolic_vertex_id>> s;
      for (const auto &v : a.vertices)
        if (std::binary_search(v.undirected_edges.begin(),
                               v.undirected_edges.end(), e.id)) {
          auto k = classify_point_segment(v.point, {p, q});
          if (k != point_segment_relation::at_origin &&
              k != point_segment_relation::open_interior &&
              k != point_segment_relation::at_destination)
            throw std::logic_error("edge incidence off segment");
          s.push_back({param({p, q}, v.point), v.id});
        }
      std::sort(s.begin(), s.end(),
                [](const auto &x, const auto &y) { return x.first < y.first; });
      source_edge_split_sequence z;
      z.edge = e.id;
      z.canonical_origin = e.first;
      z.canonical_destination = e.second;
      for (const auto &x : s) {
        z.parameters.push_back(x.first);
        z.vertices.push_back(x.second);
      }
      a.edge_sequences.push_back(std::move(z));
      for (auto use_id : e.uses) {
        const auto &use = vv.edge_uses[use_id.value_for_debug()];
        if (use.origin != e.first && use.origin != e.second)
          throw std::logic_error("edge use endpoint");
        a.directed_edge_views.push_back({use_id, e.id, use.origin == e.first});
      }
    }
    std::sort(a.directed_edge_views.begin(), a.directed_edge_views.end(),
               [](const auto &x, const auto &y) { return x.edge_use < y.edge_use; });
    for (const auto &vertex : a.vertices) {
      for (auto facet_id_value : vertex.facets) {
        const auto &facet = vv.facets[facet_id_value.value_for_debug()];
        std::vector<std::pair<exact_vector2, symbolic_curve_id>> rays;
        for (auto curve_id : vertex.incident_curves) {
          const auto &curve = a.curves[curve_id.value_for_debug()];
          if (curve.kind != symbolic_curve_kind::atomic_interval ||
              (curve.lower != vertex.id && curve.upper != vertex.id) ||
              intersect_line_plane(curve.carrier, facet.plane).kind != line_plane_kind::contained)
            continue;
          auto direction = curve.carrier.direction *
                           exact_scalar(curve.lower == vertex.id ? 1 : -1);
          rays.push_back({project_vector(direction, facet.projection), curve.id});
        }
        if (rays.empty()) continue;
        std::sort(rays.begin(), rays.end(), [](const auto &x, const auto &y) {
          const int c = angular_compare(x.first, y.first);
          return c ? c < 0 : x.second < y.second;
        });
        planar_angular_order order;
        order.facet = facet.id; order.vertex = vertex.id;
        for (const auto &ray : rays) {
          if (order.groups.empty() ||
              angular_compare(order.groups.back().direction, ray.first) != 0)
            order.groups.push_back({ray.first, {}});
          order.groups.back().curves.push_back(ray.second);
        }
        a.angular_orders.push_back(std::move(order));
      }
    }
    std::sort(a.angular_orders.begin(), a.angular_orders.end(), [](const auto &x, const auto &y) {
      return std::make_pair(x.facet, x.vertex) < std::make_pair(y.facet, y.vertex);
    });
    for (std::size_t ci = 0; ci < carrier_count; ++ci) {
      const auto &carrier = a.curves[ci];
      if (carrier.facets.empty()) continue;
      std::vector<std::pair<exact_vector2, facet_id>> sheets;
      for (auto facet_id_value : carrier.facets)
         sheets.push_back({radial_direction(carrier.carrier,
             vv.facet_geometry[facet_id_value.value_for_debug()].oriented_normal), facet_id_value});
      std::sort(sheets.begin(), sheets.end(), [](const auto &x, const auto &y) {
        const int c = angular_compare(x.first, y.first);
        return c ? c < 0 : x.second < y.second;
      });
      carrier_radial_order order; order.carrier = carrier.id;
      for (const auto &sheet : sheets) {
        if (order.groups.empty() ||
            angular_compare(order.groups.back().direction, sheet.first) != 0)
          order.groups.push_back({sheet.first, {}});
        order.groups.back().facets.push_back(sheet.second);
      }
      a.radial_orders.push_back(std::move(order));
    }
    for (auto &v : a.vertices) {
      std::sort(v.incident_curves.begin(), v.incident_curves.end());
      v.incident_curves.erase(std::unique(v.incident_curves.begin(),
                                         v.incident_curves.end()),
                              v.incident_curves.end());
      std::sort(v.overlap_regions.begin(), v.overlap_regions.end());
      v.overlap_regions.erase(std::unique(v.overlap_regions.begin(),
                                         v.overlap_regions.end()),
                              v.overlap_regions.end());
    }
    for (auto &curve : a.curves) {
      std::sort(curve.overlap_regions.begin(), curve.overlap_regions.end());
      curve.overlap_regions.erase(std::unique(curve.overlap_regions.begin(),
                                              curve.overlap_regions.end()),
                                   curve.overlap_regions.end());
    }
    std::sort(a.original_vertices.begin(), a.original_vertices.end(),
              [](const auto &x, const auto &y) { return x.source < y.source; });
    std::sort(a.raw_points.begin(), a.raw_points.end(),
              [](const auto &x, const auto &y) { return x.source < y.source; });
    std::sort(a.raw_intervals.begin(), a.raw_intervals.end(),
              [](const auto &x, const auto &y) { return x.source < y.source; });
    std::sort(a.raw_carriers.begin(), a.raw_carriers.end(),
              [](const auto &x, const auto &y) {
                return x.source_ordinal < y.source_ordinal;
              });
    std::fill(a.raw_point_index.begin(), a.raw_point_index.end(),
              missing_mapping);
    std::fill(a.raw_interval_index.begin(), a.raw_interval_index.end(),
              missing_mapping);
    std::fill(a.raw_region_index.begin(), a.raw_region_index.end(),
              missing_mapping);
    for (std::size_t i = 0; i < a.raw_points.size(); ++i)
      a.raw_point_index[a.raw_points[i].source.value_for_debug()] = i;
    for (std::size_t i = 0; i < a.raw_intervals.size(); ++i)
      a.raw_interval_index[a.raw_intervals[i].source.value_for_debug()] = i;
    for (std::size_t i = 0; i < a.raw_regions.size(); ++i)
      a.raw_region_index[a.raw_regions[i].source.value_for_debug()] = i;
    auto vertex_charge = ctx.accountant().reserve_scoped(
        resource_kind::symbolic_vertices, a.vertices.size(),
        boolean_stage::symbolic_registry);
    if (!vertex_charge.has_value()) return vertex_charge.error();
    auto curve_charge = ctx.accountant().reserve_scoped(
        resource_kind::symbolic_curves, a.curves.size(),
        boolean_stage::symbolic_registry);
    if (!curve_charge.has_value()) return curve_charge.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::symbolic_registry, "cancelled");
    a.canonical_symbolic_bytes = semantic(a);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::symbolic_registry,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::symbolic_complex, type_tag<T, I>(),
        symbolic_complex_schema, ctx.options().verification);
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
    tx.stage_reservation(std::move(work_charge.value()));
    tx.stage_reservation(std::move(private_charge.value()));
    tx.stage_reservation(std::move(vertex_charge.value()));
    tx.stage_reservation(std::move(curve_charge.value()));
    performance_count(performance_counter::symbolic_vertices,
                      a.vertices.size());
    performance_count(performance_counter::symbolic_curves,
                      a.curves.size());
    performance_count(performance_counter::reconciliation_passes,
                      requests.empty() ? 0 : 1);
    producer.finish();
    auto ok = tx.freeze_and_verify(type_tag<T, I>(), symbolic_complex_schema,
                                   a.generation, a.artifact_digest,
                                   spec.value(), env, ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::symbolic_registry, "cancelled");
    if (prior)
      return tx.compare_and_publish(ctx.artifacts(), prior->generation, prior);
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const boolean_error &e) {
    return e;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::symbolic_registry, "symbolic_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::symbolic_registry, "symbolic_exception");
    x.detail = e.what();
    return x;
  }
}
template <class T, class I>
status_or<std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>
build_symbolic_complex(boolean_context<T, I> &ctx) {
  auto latest = ctx.artifacts().latest(artifact_slot::symbolic_complex);
  if (latest)
    return std::static_pointer_cast<
        const published_artifact<symbolic_complex<T, I>>>(latest);
  return build_symbolic_complex_impl<T, I>(ctx, {}, {});
}

template <class T, class I>
status_or<std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>
reconcile_symbolic_complex(
    boolean_context<T, I> &ctx,
    std::shared_ptr<const published_artifact<symbolic_complex<T, I>>> prior,
    std::vector<symbolic_reconciliation_request> requests) {
  if (!prior || prior->owner != ctx.owner() ||
      prior->slot != artifact_slot::symbolic_complex || requests.empty())
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::symbolic_registry,
                      "reconciliation_binding");
  if (ctx.artifacts().latest_generation(artifact_slot::symbolic_complex) !=
      prior->generation)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::symbolic_registry,
                      "stale_symbolic_generation");
  std::sort(requests.begin(), requests.end(), reconciliation_request_less);
  requests.erase(std::unique(requests.begin(), requests.end(),
                             reconciliation_request_equal),
                 requests.end());
  for (const auto &request : requests) {
    if (request.schema != symbolic_reconciliation_schema_v1 ||
        request.prior_digest != prior->artifact_digest ||
        request.prior_generation != prior->generation ||
        request.facet.value_for_debug() >=
            prior->payload->validated->payload->facets.size() ||
        request.first_curve.value_for_debug() >= prior->payload->curves.size() ||
        request.second_curve.value_for_debug() >= prior->payload->curves.size())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::symbolic_registry,
                        "malformed_reconciliation_request");
    const auto &facet = prior->payload->validated->payload
                            ->facets[request.facet.value_for_debug()];
    if (plane_side(facet.plane, request.point) != exact_sign::zero ||
        classify_point_line(request.point,
                            prior->payload
                                ->curves[request.first_curve.value_for_debug()]
                                .carrier) != point_line_relation::on_carrier ||
        classify_point_line(request.point,
                            prior->payload
                                ->curves[request.second_curve.value_for_debug()]
                                .carrier) != point_line_relation::on_carrier)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::symbolic_registry,
                        "reconciliation_geometry");
  }
  auto charge = ctx.accountant().reserve_scoped(
      resource_kind::reconciliation_requests, requests.size(),
      boolean_stage::symbolic_registry);
  if (!charge.has_value())
    return charge.error();
  auto generation = ctx.accountant().reserve_scoped(
      resource_kind::successor_generations, 1,
      boolean_stage::symbolic_registry);
  if (!generation.has_value())
    return generation.error();
  auto result = build_symbolic_complex_impl<T, I>(ctx, requests, prior);
  if (result.has_value()) {
    charge.value().commit();
    generation.value().commit();
  }
  return result;
}
#define INST(T, I)                                                             \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>       \
  build_symbolic_complex(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST
#define RECONCILE_INST(T, I)                                                   \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>       \
  reconcile_symbolic_complex(                                                  \
      boolean_context<T, I> &,                                                 \
      std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>,       \
      std::vector<symbolic_reconciliation_request>)
RECONCILE_INST(float, std::uint32_t);
RECONCILE_INST(float, std::uint64_t);
RECONCILE_INST(double, std::uint32_t);
RECONCILE_INST(double, std::uint64_t);
#undef RECONCILE_INST
} // namespace mesh_boolean
} // namespace ygor
