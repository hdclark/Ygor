#include "YgorMeshesBooleanIntersectionEvents.h"
#include "YgorMeshesBooleanExecutor.h"
#include <algorithm>
#include <new>
#include <tuple>

#if defined(__FAST_MATH__) ||                                                  \
    (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Boolean intersection events require strict floating-point compilation"
#endif

namespace ygor {
namespace mesh_boolean {
int canonical_incidence_compare(const raw_source_incidence &a,
                                const raw_source_incidence &b) noexcept {
  int c = canonical_feature_compare(a.source, b.source);
  if (c) return c;
  if (a.location != b.location)
    return static_cast<std::uint8_t>(a.location) <
                   static_cast<std::uint8_t>(b.location)
               ? -1
               : 1;
  if (a.directed_edge_parameter.has_value() !=
      b.directed_edge_parameter.has_value())
    return a.directed_edge_parameter ? 1 : -1;
  if (a.directed_edge_parameter) {
    c = canonical_encoding_compare(*a.directed_edge_parameter,
                                   *b.directed_edge_parameter);
    if (c) return c;
  }
  if (a.local_side.has_value() != b.local_side.has_value())
    return a.local_side ? 1 : -1;
  if (a.local_side != b.local_side)
    return static_cast<std::uint8_t>(*a.local_side) <
                   static_cast<std::uint8_t>(*b.local_side)
               ? -1
               : 1;
  if (a.orientation != b.orientation)
    return static_cast<std::uint8_t>(a.orientation) <
                   static_cast<std::uint8_t>(b.orientation)
               ? -1
               : 1;
  return 0;
}

int canonical_ownership_compare(const raw_curve_ownership &a,
                                const raw_curve_ownership &b) noexcept {
  if (a.operand != b.operand) return a.operand < b.operand ? -1 : 1;
  const int source = canonical_feature_compare(a.source, b.source);
  if (source) return source;
  if (a.direction != b.direction)
    return static_cast<std::uint8_t>(a.direction) <
                   static_cast<std::uint8_t>(b.direction)
               ? -1
               : 1;
  if (a.multiplicity != b.multiplicity)
    return a.multiplicity < b.multiplicity ? -1 : 1;
  return 0;
}

std::optional<std::size_t> detail::construction_key_index::find(
    const digest &key, const construction_storage &storage,
    const std::vector<feature_ref> &sources,
    const std::vector<std::uint8_t> &payload) const {
  const auto bucket = buckets_.find(key);
  if (bucket == buckets_.end()) return std::nullopt;
  performance_count(performance_counter::hash_bucket_probes,
                    bucket->second.size());
  for (const auto index : bucket->second) {
    performance_count(performance_counter::exact_equality_checks);
    const auto &node = storage.nodes.at(index);
    if (node.kind == construction_kind::exact_relation &&
        node.defining_sources == sources && node.exact_result == payload)
      return index;
  }
  return std::nullopt;
}

void detail::construction_key_index::insert(const digest &key,
                                            std::size_t index) {
  buckets_[key].push_back(index);
}

namespace {
template <class T, class I> std::uint64_t type_tag() {
  return raw_event_set_type_tag +
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
void enc_incidence(canonical_encoder &e, const raw_source_incidence &x) {
  enc_feature(e, x.source);
  e.byte(static_cast<std::uint8_t>(x.location));
  e.boolean(x.directed_edge_parameter.has_value());
  if (x.directed_edge_parameter) x.directed_edge_parameter->encode(e);
  e.boolean(x.local_side.has_value());
  if (x.local_side) e.byte(static_cast<std::uint8_t>(*x.local_side));
  e.byte(static_cast<std::uint8_t>(x.orientation));
}
bool incidence_less(const raw_source_incidence &a,
                    const raw_source_incidence &b) {
  return canonical_incidence_less(a, b);
}
void normalize_incidences(std::vector<raw_source_incidence> &xs) {
  std::sort(xs.begin(), xs.end(), incidence_less);
  xs.erase(std::unique(xs.begin(), xs.end(), canonical_incidence_equal),
           xs.end());
}
void enc_ownership(canonical_encoder &e, const raw_curve_ownership &x) {
  e.id(x.operand);
  enc_feature(e, x.source);
  e.byte(static_cast<std::uint8_t>(x.direction));
  e.u32(x.multiplicity);
}
template <class T, class I>
std::vector<raw_curve_ownership>
curve_ownership(const std::vector<raw_source_incidence> &incidences,
                 const validated_operands<T, I> &validated) {
  std::vector<raw_curve_ownership> out;
  for (const auto &incidence : incidences) {
    std::optional<operand_id> operand;
    if (const auto *f = std::get_if<facet_ref>(&incidence.source))
      operand = f->operand;
    else if (const auto *use = std::get_if<edge_use_id>(&incidence.source)) {
      if (use->valid() && use->value_for_debug() < validated.edge_uses.size())
        operand = validated.edge_uses[use->value_for_debug()].operand;
    }
    if (!operand) continue;
    raw_curve_ownership owner{*operand, incidence.source,
                              incidence.orientation, 1};
    auto prior = std::find_if(out.begin(), out.end(), [&](const auto &x) {
      return canonical_ownership_equal(x, owner);
    });
    if (prior == out.end()) out.push_back(std::move(owner));
  }
  std::sort(out.begin(), out.end(), canonical_ownership_less);
  return out;
}
void apply_source_directions(
    std::vector<raw_curve_ownership> &ownership,
    const std::vector<source_edge_parameter_interval> &source_intervals) {
  for (auto &owner : ownership) {
    const auto *edge_use = std::get_if<edge_use_id>(&owner.source);
    if (!edge_use) continue;
    const auto mapping = std::find_if(
        source_intervals.begin(), source_intervals.end(),
        [&](const auto &x) { return x.edge_use == *edge_use; });
    if (mapping != source_intervals.end()) owner.direction = mapping->orientation;
  }
  std::sort(ownership.begin(), ownership.end(), canonical_ownership_less);
}
raw_derivation exact_derivation(const std::vector<raw_source_incidence> &xs,
                                std::vector<std::uint8_t> payload) {
  raw_derivation d;
  d.incidences = xs;
  for (const auto &x : xs) d.defining_sources.push_back(x.source);
  std::sort(d.defining_sources.begin(), d.defining_sources.end(),
            canonical_feature_less);
  d.defining_sources.erase(std::unique(d.defining_sources.begin(),
      d.defining_sources.end()), d.defining_sources.end());
  d.evidence.kind = evidence_kind::exact_relation;
  d.evidence.invariant = invariant_code::event_geometry;
  d.evidence.entities = d.defining_sources;
  d.evidence.exact_payload = std::move(payload);
  d.evidence.evidence_digest = evidence_digest(d.evidence);
  return d;
}
template <class T, class I>
void freeze_constructions(raw_event_set<T, I> &a) {
  auto storage = std::make_shared<construction_storage>();
  storage->owner = a.owner;
  detail::construction_key_index keys;
  auto freeze = [&](raw_derivation &d, const exact_point3 *result_point) {
    canonical_encoder e;
    e.byte(static_cast<std::uint8_t>(construction_kind::exact_relation));
    e.u64(d.defining_sources.size());
    for (const auto &source : d.defining_sources) enc_feature(e, source);
    e.byte_string(d.evidence.exact_payload);
    performance_count(performance_counter::canonical_key_encodings);
    const auto key = domain_digest({{'Y', 'G', 'B', 'C', 'K', 'E', 'Y', '1'}},
                                   e.bytes());
    auto prior = keys.find(key, *storage, d.defining_sources,
                           d.evidence.exact_payload);
    std::size_t index = prior.value_or(storage->nodes.size());
    if (!prior) {
      construction_node node;
      node.id = construction_node_id::from_canonical_value(index);
      node.defining_sources = d.defining_sources;
      node.exact_result = d.evidence.exact_payload;
      storage->nodes.push_back(std::move(node));
      keys.insert(key, index);
    } else {
      performance_count(performance_counter::duplicate_derivations);
    }
    d.construction = construction_node_id::from_canonical_value(index);
    auto &node = storage->nodes[index];
    if (result_point && node.defining_relations.empty()) {
      const std::array<exact_scalar, 3> target{{result_point->x, result_point->y,
                                                result_point->z}};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        defining_relation relation;
        relation.id = defining_relation_id::from_canonical_value(storage->relations.size());
        relation.construction = node.id;
        relation.defining_sources = node.defining_sources;
        relation.coefficients[axis] = exact_scalar(1);
        relation.coefficients[3] = target[axis].negated();
        node.defining_relations.push_back(relation.id);
        storage->relations.push_back(std::move(relation));
      }
      const auto &validated = *a.candidates->payload->validated->payload;
      for (const auto &source : node.defining_sources) {
        const auto *facet_source = std::get_if<facet_id>(&source);
        if (!facet_source ||
            facet_source->value_for_debug() >= validated.facets.size())
          continue;
        const auto &plane = validated.facets[facet_source->value_for_debug()].plane;
        defining_relation relation;
        relation.id = defining_relation_id::from_canonical_value(storage->relations.size());
        relation.kind = defining_relation_kind::point_on_plane;
        relation.construction = node.id;
        relation.defining_sources = {*facet_source};
        relation.coefficients = {{exact_scalar(plane.a, big_uint(1)),
                                  exact_scalar(plane.b, big_uint(1)),
                                  exact_scalar(plane.c, big_uint(1)),
                                  exact_scalar(plane.d, big_uint(1))}};
        node.defining_relations.push_back(relation.id);
        storage->relations.push_back(std::move(relation));
      }
    }
  };
  for (auto &point : a.points) for (auto &d : point.derivations) freeze(d, &point.point);
  for (auto &interval : a.intervals) for (auto &d : interval.derivations) freeze(d, nullptr);
  for (auto &region : a.regions) for (auto &d : region.derivations) freeze(d, nullptr);
  for (auto &carrier : a.carriers) for (auto &d : carrier.derivations) freeze(d, nullptr);
  a.constructions = std::move(storage);
}
std::vector<std::uint8_t> point_payload(const exact_point3 &p) {
  canonical_encoder e; enc_point(e, p); return e.bytes();
}
exact_scalar parameter(const exact_line3 &l, const exact_point3 &p) {
  if (!l.direction.x.is_zero())
    return (p.x - l.anchor.x) / l.direction.x;
  if (!l.direction.y.is_zero())
    return (p.y - l.anchor.y) / l.direction.y;
  return (p.z - l.anchor.z) / l.direction.z;
}
exact_point3 at(const exact_line3 &l, const exact_scalar &t) {
  return l.anchor + l.direction * t;
}
template <class T, class I>
const validated_facet &facet(const validated_operands<T, I> &v, facet_id id) {
  if (!id.valid() || id.value_for_debug() >= v.facets.size() ||
      v.facets[id.value_for_debug()].id != id)
    throw std::logic_error("facet ID");
  return v.facets[id.value_for_debug()];
}
template <class T, class I>
std::vector<exact_point3> ring3(const validated_operands<T, I> &v,
                                const validated_facet &f) {
  std::vector<exact_point3> r;
  for (auto id : f.ring) {
    if (!id.valid() || id.value_for_debug() >= v.vertices.size())
      throw std::logic_error("vertex ID");
    r.push_back(v.vertices[id.value_for_debug()].exact_coordinate);
  }
  return r;
}
template <class T, class I>
std::vector<exact_point2> ring2(const validated_operands<T, I> &v,
                                const validated_facet &f) {
  std::vector<exact_point2> r;
  for (const auto &p : ring3(v, f))
    r.push_back(project(p, f.projection));
  return r;
}
bool inside(const exact_point3 &p, const validated_facet &f,
            const std::vector<exact_point2> &r) {
  auto q = classify_point_polygon(project(p, f.projection), r);
  return q.has_value() && q.value().kind != point_region_kind::outside;
}
template <class T, class I>
std::vector<exact_interval> line_polygon(const exact_line3 &l,
                                         const validated_operands<T, I> &v,
                                         const validated_facet &f) {
  auto r = ring3(v, f);
  auto p2 = ring2(v, f);
  std::vector<exact_scalar> cuts;
  // Edge/line intersection is most simply and exactly evaluated in the facet
  // chart.
  exact_line2 ql{project(l.anchor, f.projection),
                 {project(l.anchor + l.direction, f.projection) -
                  project(l.anchor, f.projection)}};
  for (std::size_t i = 0; i < p2.size(); ++i) {
    exact_line2 el{p2[i], p2[(i + 1) % p2.size()] - p2[i]};
    auto x = intersect_lines(ql, el);
    if (x.kind == line_line_kind::unique &&
        classify_point_segment(*x.point, {p2[i], p2[(i + 1) % p2.size()]}) !=
            point_segment_relation::off_carrier) {
      auto s =
          classify_point_segment(*x.point, {p2[i], p2[(i + 1) % p2.size()]});
      if (s == point_segment_relation::at_origin ||
          s == point_segment_relation::at_destination ||
          s == point_segment_relation::open_interior)
        cuts.push_back(*x.first_parameter);
    } else if (x.kind == line_line_kind::coincident) {
      cuts.push_back(parameter(l, r[i]));
      cuts.push_back(parameter(l, r[(i + 1) % r.size()]));
    }
  }
  std::sort(cuts.begin(), cuts.end());
  cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
  std::vector<exact_interval> out;
  for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
    auto m = (cuts[i] + cuts[i + 1]) / exact_scalar(2);
    if (inside(at(l, m), f, p2))
      out.push_back({cuts[i], cuts[i + 1], true, true});
  }
  for (auto t : cuts)
    if (inside(at(l, t), f, p2)) {
      bool covered = false;
      for (const auto &q : out)
        covered |= !(t < q.lower) && !(q.upper < t);
      if (!covered)
        out.push_back({t, t, true, true});
    }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    return a.lower == b.lower ? a.upper < b.upper : a.lower < b.lower;
  });
  return out;
}
std::vector<exact_interval>
intersect_sets(const std::vector<exact_interval> &a,
               const std::vector<exact_interval> &b) {
  std::vector<exact_interval> o;
  for (const auto &x : a)
    for (const auto &y : b) {
      auto lo = x.lower < y.lower ? y.lower : x.lower,
           hi = x.upper < y.upper ? x.upper : y.upper;
      if (!(hi < lo))
        o.push_back({lo, hi, true, true});
    }
  std::sort(o.begin(), o.end(), [](const auto &x, const auto &y) {
    return x.lower == y.lower ? x.upper < y.upper : x.lower < y.lower;
  });
  o.erase(std::unique(o.begin(), o.end(),
                      [](const auto &x, const auto &y) {
                        return x.lower == y.lower && x.upper == y.upper;
                      }),
          o.end());
  return o;
}
raw_source_incidence finc(operand_id op, facet_id f,
                          local_incidence_location l) {
  return {facet_ref{op, f}, l, std::nullopt, std::nullopt,
          orientation_parity::agree};
}
template <class T, class I>
std::vector<raw_source_incidence>
point_incidences(const exact_point3 &p, const validated_operands<T, I> &v,
                 const validated_facet &f) {
  std::vector<raw_source_incidence> out;
  auto location = classify_point_polygon(project(p, f.projection), ring2(v, f));
  out.push_back(finc(
      f.operand, f.id,
      location.has_value() &&
              location.value().kind == point_region_kind::open_interior
          ? local_incidence_location::facet_open_interior
          : local_incidence_location::facet_boundary));
  for (std::size_t i = 0; i < f.ring.size(); ++i) {
    const auto &a = v.vertices[f.ring[i].value_for_debug()].exact_coordinate;
    const auto &b =
        v.vertices[f.ring[(i + 1) % f.ring.size()].value_for_debug()]
            .exact_coordinate;
    auto rel = classify_point_segment(p, {a, b});
    if (rel == point_segment_relation::off_carrier ||
        rel == point_segment_relation::before_origin ||
        rel == point_segment_relation::after_destination)
      continue;
    local_incidence_location loc =
        rel == point_segment_relation::at_origin
            ? local_incidence_location::directed_edge_origin
            : rel == point_segment_relation::at_destination
                  ? local_incidence_location::directed_edge_destination
                  : local_incidence_location::directed_edge_open_interior;
    auto t = parameter(exact_line3{a, b - a}, p);
    out.push_back({f.edge_uses[i], loc, t, std::nullopt,
                   orientation_parity::agree});
    out.push_back({v.edge_uses[f.edge_uses[i].value_for_debug()].edge, loc, t,
                   std::nullopt, orientation_parity::agree});
    if (rel != point_segment_relation::open_interior)
      out.push_back({f.ring[(rel == point_segment_relation::at_origin)
                                ? i
                                : (i + 1) % f.ring.size()],
                     local_incidence_location::source_vertex, std::nullopt,
                     std::nullopt, orientation_parity::agree});
  }
  normalize_incidences(out);
  return out;
}

template <class T, class I>
std::vector<source_edge_parameter_interval> source_intervals(
    const exact_point3 &lower, const exact_point3 &upper,
    const std::vector<raw_source_incidence> &incidences,
    const validated_operands<T, I> &v) {
  std::vector<source_edge_parameter_interval> out;
  for (const auto &incidence : incidences) {
    const auto *id = std::get_if<edge_use_id>(&incidence.source);
    if (!id || !id->valid() || id->value_for_debug() >= v.edge_uses.size())
      continue;
    const auto &use = v.edge_uses[id->value_for_debug()];
    const auto &a = v.vertices[use.origin.value_for_debug()].exact_coordinate;
    const auto &b = v.vertices[use.destination.value_for_debug()].exact_coordinate;
    auto lo_relation = classify_point_segment(lower, {a, b});
    auto hi_relation = classify_point_segment(upper, {a, b});
    auto on_segment = [](point_segment_relation relation) {
      return relation == point_segment_relation::at_origin ||
             relation == point_segment_relation::at_destination ||
             relation == point_segment_relation::open_interior;
    };
    if (!on_segment(lo_relation) || !on_segment(hi_relation)) continue;
    auto lo = parameter(exact_line3{a, b - a}, lower);
    auto hi = parameter(exact_line3{a, b - a}, upper);
    source_edge_parameter_interval mapping;
    mapping.edge_use = *id;
    mapping.orientation = hi < lo ? orientation_parity::opposite
                                  : orientation_parity::agree;
    mapping.parameters = hi < lo ? exact_interval{hi, lo, true, true}
                                 : exact_interval{lo, hi, true, true};
    out.push_back(std::move(mapping));
  }
  std::sort(out.begin(), out.end(), [](const auto &x, const auto &y) {
    return x.edge_use.value_for_debug() < y.edge_use.value_for_debug();
  });
  out.erase(std::unique(out.begin(), out.end(), [](const auto &x, const auto &y) {
              return x.edge_use == y.edge_use;
            }), out.end());
  return out;
}

exact_point3 lift(const exact_point2 &p, const exact_plane3 &pl,
                  projection_axis axis) {
  exact_scalar a(pl.a, big_uint(1)), b(pl.b, big_uint(1)),
      c(pl.c, big_uint(1)), d(pl.d, big_uint(1));
  if (axis == projection_axis::drop_x) {
    exact_scalar y = p.x, z = p.y;
    return {(exact_scalar(0) - (b * y + c * z + d)) / a, y, z};
  }
  if (axis == projection_axis::drop_y) {
    exact_scalar z = p.x, x = p.y;
    return {x, (exact_scalar(0) - (a * x + c * z + d)) / b, z};
  }
  exact_scalar x = p.x, y = p.y;
  return {x, y, (exact_scalar(0) - (a * x + b * y + d)) / c};
}

exact_scalar polygon_area2(const std::vector<exact_point2> &p) {
  exact_scalar a(0);
  for (std::size_t i = 0; i < p.size(); ++i)
    a = a + p[i].x * p[(i + 1) % p.size()].y -
        p[i].y * p[(i + 1) % p.size()].x;
  return a;
}

struct planar_overlay {
  std::vector<std::vector<exact_point2>> cycles;
  std::vector<std::pair<exact_point2, exact_point2>> contact_segments;
  std::vector<exact_point2> contact_points;
};

bool angular_less(const exact_vector2 &incoming, const exact_vector2 &a,
                  const exact_vector2 &b) {
  auto ax = dot(incoming, a), ay = incoming.x * a.y - incoming.y * a.x;
  auto bx = dot(incoming, b), by = incoming.x * b.y - incoming.y * b.x;
  auto upper = [](const exact_scalar &x, const exact_scalar &y) {
    return y.sign() == exact_sign::positive ||
           (y.is_zero() && x.sign() != exact_sign::negative);
  };
  const bool au = upper(ax, ay), bu = upper(bx, by);
  if (au != bu) return au;
  auto cross_angle = ax * by - ay * bx;
  if (!cross_angle.is_zero()) return cross_angle.sign() == exact_sign::positive;
  return a.x == b.x ? a.y < b.y : a.x < b.x;
}

exact_scalar segment_parameter(const exact_point2 &a, const exact_point2 &b,
                               const exact_point2 &p) {
  auto d = b - a;
  return !d.x.is_zero() ? (p.x - a.x) / d.x : (p.y - a.y) / d.y;
}

bool polygon_contains(const exact_point2 &p,
                      const std::vector<exact_point2> &ring) {
  auto r = classify_point_polygon(p, ring);
  return r.has_value() && r.value().kind != point_region_kind::outside;
}

// The boundary of A intersect B consists solely of source-boundary atoms.
// Splitting source edges exhaustively therefore avoids semantic triangulation
// diagonals and works for concave and disconnected intersections.
planar_overlay overlay_polygons(const std::vector<exact_point2> &a,
                                const std::vector<exact_point2> &b) {
  struct atom { exact_point2 from, to; };
  std::vector<atom> atoms;
  std::vector<std::pair<exact_point2, exact_point2>> contacts;
  std::vector<exact_point2> points;
  auto add_boundary = [&](const std::vector<exact_point2> &source,
                          const std::vector<exact_point2> &other) {
    const bool reverse = polygon_area2(source).sign() == exact_sign::negative;
    for (std::size_t i = 0; i < source.size(); ++i) {
      auto p = source[i], q = source[(i + 1) % source.size()];
      std::vector<exact_scalar> cuts{exact_scalar(0), exact_scalar(1)};
      for (std::size_t j = 0; j < other.size(); ++j) {
        auto r = relate_segments(exact_segment2{p, q},
                                 exact_segment2{other[j], other[(j + 1) % other.size()]});
        if (r.dimension == intersection_dimension::point && r.point) {
          cuts.push_back(segment_parameter(p, q, *r.point));
          points.push_back(*r.point);
        } else if (r.dimension == intersection_dimension::segment && r.overlap_segment) {
          cuts.push_back(segment_parameter(p, q, r.overlap_segment->origin));
          cuts.push_back(segment_parameter(p, q, r.overlap_segment->destination));
          contacts.push_back({r.overlap_segment->origin,
                              r.overlap_segment->destination});
        }
      }
      std::sort(cuts.begin(), cuts.end());
      cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
      for (std::size_t j = 0; j + 1 < cuts.size(); ++j) {
        auto x = p + (q - p) * cuts[j];
        auto y = p + (q - p) * cuts[j + 1];
        auto m = x + (y - x) * (exact_scalar(1) / exact_scalar(2));
        if (!polygon_contains(m, other))
          continue;
        atoms.push_back(reverse ? atom{y, x} : atom{x, y});
      }
    }
  };
  add_boundary(a, b);
  add_boundary(b, a);
  std::sort(atoms.begin(), atoms.end(), [](const atom &x, const atom &y) {
    int c = lexicographic_compare(x.from, y.from);
    return c ? c < 0 : lexicographic_compare(x.to, y.to) < 0;
  });
  atoms.erase(std::unique(atoms.begin(), atoms.end(), [](const atom &x, const atom &y) {
                return x.from == y.from && x.to == y.to;
              }), atoms.end());
  std::vector<bool> used(atoms.size());
  planar_overlay out;
  for (std::size_t seed = 0; seed < atoms.size(); ++seed) {
    if (used[seed]) continue;
    std::vector<exact_point2> cycle;
    std::size_t current = seed;
    while (!used[current]) {
      used[current] = true;
      cycle.push_back(atoms[current].from);
      auto endpoint = atoms[current].to;
      if (endpoint == cycle.front()) break;
      auto next = atoms.size();
      for (std::size_t k = 0; k < atoms.size(); ++k)
        if (!used[k] && atoms[k].from == endpoint &&
            (next == atoms.size() ||
             angular_less(atoms[current].to - atoms[current].from,
                          atoms[k].to - atoms[k].from,
                          atoms[next].to - atoms[next].from)))
          next = k;
      if (next == atoms.size()) { cycle.clear(); break; }
      current = next;
    }
    if (cycle.size() >= 3 && !polygon_area2(cycle).is_zero())
      out.cycles.push_back(std::move(cycle));
  }
  out.contact_segments = std::move(contacts);
  std::sort(points.begin(), points.end(), [](const auto &x, const auto &y) {
    return lexicographic_compare(x, y) < 0;
  });
  points.erase(std::unique(points.begin(), points.end()), points.end());
  out.contact_points = std::move(points);
  return out;
}

struct candidate_shard {
  std::optional<plane_plane_result> planes;
  std::vector<exact_interval> nonparallel_components;
  planar_overlay coplanar_overlay;
};

exact_point2 overlap_witness(const std::vector<exact_point2> &cycle,
                             const std::vector<exact_point2> &a,
                             const std::vector<exact_point2> &b) {
  for (std::size_t i = 0; i < cycle.size(); ++i) {
    const auto &p = cycle[(i + cycle.size() - 1) % cycle.size()];
    const auto &q = cycle[i];
    const auto &r = cycle[(i + 1) % cycle.size()];
    if (orient2d(p, q, r) != exact_sign::positive) continue;
    exact_point2 w{(p.x + q.x + r.x) / exact_scalar(3),
                   (p.y + q.y + r.y) / exact_scalar(3)};
    auto ca = classify_point_polygon(w, a), cb = classify_point_polygon(w, b);
    if (ca.has_value() && cb.has_value() &&
        ca.value().kind == point_region_kind::open_interior &&
        cb.value().kind == point_region_kind::open_interior)
      return w;
  }
  throw std::logic_error("coplanar overlap witness");
}

struct verifier_vertical_crossing {
  exact_scalar y;
  std::size_t edge = 0;
};

exact_scalar verifier_edge_y(const std::vector<exact_point2> &ring,
                             std::size_t edge, const exact_scalar &x) {
  const auto &p = ring[edge];
  const auto &q = ring[(edge + 1) % ring.size()];
  return p.y + (q.y - p.y) * ((x - p.x) / (q.x - p.x));
}

std::vector<verifier_vertical_crossing>
verifier_crossings(const std::vector<exact_point2> &ring,
                   const exact_scalar &x) {
  std::vector<verifier_vertical_crossing> out;
  for (std::size_t i = 0; i < ring.size(); ++i) {
    const auto &p = ring[i];
    const auto &q = ring[(i + 1) % ring.size()];
    if (p.x == q.x) continue;
    const auto &low = p.x < q.x ? p.x : q.x;
    const auto &high = p.x < q.x ? q.x : p.x;
    if (low < x && x < high)
      out.push_back({verifier_edge_y(ring, i, x), i});
  }
  std::sort(out.begin(), out.end(), [](const auto &x, const auto &y) {
    return x.y == y.y ? x.edge < y.edge : x.y < y.y;
  });
  if ((out.size() & 1U) != 0)
    throw std::logic_error("coplanar verifier odd crossing count");
  return out;
}

// Independent of the producer's boundary walk: decompose both source polygons
// into exact vertical slabs and integrate their overlapping y-intervals.
exact_scalar verifier_overlap_area(const std::vector<exact_point2> &a,
                                    const std::vector<exact_point2> &b) {
  std::vector<exact_scalar> cuts;
  for (const auto &p : a) cuts.push_back(p.x);
  for (const auto &p : b) cuts.push_back(p.x);
  for (std::size_t i = 0; i < a.size(); ++i) {
    const exact_segment2 sa{a[i], a[(i + 1) % a.size()]};
    for (std::size_t j = 0; j < b.size(); ++j) {
      const auto relation = relate_segments(
          sa, exact_segment2{b[j], b[(j + 1) % b.size()]});
      if (relation.point) cuts.push_back(relation.point->x);
      if (relation.overlap_segment) {
        cuts.push_back(relation.overlap_segment->origin.x);
        cuts.push_back(relation.overlap_segment->destination.x);
      }
    }
  }
  std::sort(cuts.begin(), cuts.end());
  cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

  exact_scalar area(0);
  for (std::size_t slab = 0; slab + 1 < cuts.size(); ++slab) {
    const auto &left = cuts[slab];
    const auto &right = cuts[slab + 1];
    if (left == right) continue;
    const auto middle = (left + right) / exact_scalar(2);
    const auto ca = verifier_crossings(a, middle);
    const auto cb = verifier_crossings(b, middle);
    std::size_t ia = 0, ib = 0;
    while (ia + 1 < ca.size() && ib + 1 < cb.size()) {
      const auto lower = ca[ia].y < cb[ib].y ? cb[ib] : ca[ia];
      const auto upper = ca[ia + 1].y < cb[ib + 1].y ? ca[ia + 1] : cb[ib + 1];
      if (lower.y < upper.y) {
        const bool lower_a = !(ca[ia].y < cb[ib].y);
        const bool upper_a = ca[ia + 1].y < cb[ib + 1].y;
        auto ordinate = [&](bool from_a, std::size_t edge,
                            const exact_scalar &x) {
          return verifier_edge_y(from_a ? a : b, edge, x);
        };
        const auto left_length = ordinate(upper_a, upper.edge, left) -
                                 ordinate(lower_a, lower.edge, left);
        const auto right_length = ordinate(upper_a, upper.edge, right) -
                                  ordinate(lower_a, lower.edge, right);
        area = area + (left_length + right_length) * (right - left) /
                          exact_scalar(2);
      }
      if (ca[ia + 1].y < cb[ib + 1].y)
        ia += 2;
      else if (cb[ib + 1].y < ca[ia + 1].y)
        ib += 2;
      else {
        ia += 2;
        ib += 2;
      }
    }
  }
  return area;
}

struct verifier_boundary_atom {
  exact_point2 from, to;
  std::vector<raw_curve_ownership> ownership;
};

void verifier_normalize_ownership(std::vector<raw_curve_ownership> &owners) {
  std::sort(owners.begin(), owners.end(), canonical_ownership_less);
  owners.erase(
      std::unique(owners.begin(), owners.end(), canonical_ownership_equal),
      owners.end());
}

// Reconstruct overlap halfedges directly from source edges. This deliberately
// does not call overlay_polygons or consume its cycles, incidences, or mappings.
template <class T, class I>
std::vector<verifier_boundary_atom> verifier_overlap_boundary(
    const validated_operands<T, I> &v, const validated_facet &fa,
    const validated_facet &fb) {
  const auto a = ring2(v, fa), b = ring2(v, fb);
  std::vector<verifier_boundary_atom> atoms;
  auto split = [&](const validated_facet &source,
                   const std::vector<exact_point2> &source_ring,
                   const std::vector<exact_point2> &other_ring) {
    const bool reverse = polygon_area2(source_ring).sign() == exact_sign::negative;
    for (std::size_t i = 0; i < source_ring.size(); ++i) {
      const auto &p = source_ring[i];
      const auto &q = source_ring[(i + 1) % source_ring.size()];
      std::vector<exact_scalar> cuts{exact_scalar(0), exact_scalar(1)};
      for (std::size_t j = 0; j < other_ring.size(); ++j) {
        const auto relation = relate_segments(
            exact_segment2{p, q}, exact_segment2{other_ring[j],
                                                 other_ring[(j + 1) % other_ring.size()]});
        if (relation.point)
          cuts.push_back(segment_parameter(p, q, *relation.point));
        if (relation.overlap_segment) {
          cuts.push_back(segment_parameter(p, q, relation.overlap_segment->origin));
          cuts.push_back(segment_parameter(p, q, relation.overlap_segment->destination));
        }
      }
      std::sort(cuts.begin(), cuts.end());
      cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
      for (std::size_t j = 0; j + 1 < cuts.size(); ++j) {
        const auto x = p + (q - p) * cuts[j];
        const auto y = p + (q - p) * cuts[j + 1];
        const auto midpoint = x + (y - x) * (exact_scalar(1) / exact_scalar(2));
        const auto location = classify_point_polygon(midpoint, other_ring);
        if (!location.has_value() || location.value().kind == point_region_kind::outside)
          continue;
        verifier_boundary_atom atom{reverse ? y : x, reverse ? x : y, {}};
        atom.ownership.push_back(
            {source.operand, source.edge_uses[i],
             reverse ? orientation_parity::opposite : orientation_parity::agree, 1});
        atoms.push_back(std::move(atom));
      }
    }
  };
  split(fa, a, b);
  split(fb, b, a);
  std::sort(atoms.begin(), atoms.end(), [](const auto &x, const auto &y) {
    const int from = lexicographic_compare(x.from, y.from);
    return from ? from < 0 : lexicographic_compare(x.to, y.to) < 0;
  });
  std::vector<verifier_boundary_atom> merged;
  for (auto &atom : atoms) {
    if (merged.empty() || !(merged.back().from == atom.from) ||
        !(merged.back().to == atom.to))
      merged.push_back(std::move(atom));
    else
      merged.back().ownership.insert(merged.back().ownership.end(),
                                     atom.ownership.begin(), atom.ownership.end());
  }
  for (auto &atom : merged) {
    atom.ownership.push_back(
        {fa.operand, facet_ref{fa.operand, fa.id}, orientation_parity::agree, 1});
    atom.ownership.push_back(
        {fb.operand, facet_ref{fb.operand, fb.id}, orientation_parity::agree, 1});
    verifier_normalize_ownership(atom.ownership);
  }
  return merged;
}
template <class T, class I>
std::vector<std::uint8_t> semantic(const raw_event_set<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN05";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(raw_event_set_schema);
  e.raw(a.candidates->payload->canonical_candidate_bytes.data(),
        a.candidates->payload->canonical_candidate_bytes.size());
  e.u64(a.classifications.size());
  for (const auto &c : a.classifications) {
    e.id(c.candidate);
    e.id(c.facets.operand_a_facet); e.id(c.facets.operand_b_facet);
    e.byte(static_cast<std::uint8_t>(c.plane_relation));
    e.byte(static_cast<std::uint8_t>(c.aggregate));
    e.u64(c.event_begin);
    e.u64(c.event_end);
    e.u64(c.carrier_begin);
    e.u64(c.carrier_end);
    e.u64(c.point_count); e.u64(c.interval_count); e.u64(c.region_count);
  }
  e.u64(a.carriers.size());
  for (const auto &c : a.carriers) {
    e.id(c.candidate); enc_line(e, c.carrier);
    e.byte(static_cast<std::uint8_t>(c.direction));
    e.u64(c.facets.size()); for (auto f : c.facets) e.id(f);
    e.u64(c.derivations.size());
    for (const auto &d : c.derivations) e.id(d.construction);
  }
  e.u64(a.points.size());
  for (const auto &p : a.points) {
    e.id(p.id);
    e.id(p.candidate);
    e.byte(static_cast<std::uint8_t>(p.kind));
    enc_point(e, p.point);
    e.u64(p.incidences.size());
    for (const auto &x : p.incidences) enc_incidence(e, x);
    e.u64(p.derivations.size());
    for (const auto &d : p.derivations) {
      e.id(d.construction);
      e.u64(d.defining_sources.size());
      for (const auto &x : d.defining_sources) enc_feature(e, x);
      e.raw(d.evidence.evidence_digest.bytes.data(), 16);
    }
  }
  e.u64(a.intervals.size());
  for (const auto &q : a.intervals) {
    e.id(q.id);
    e.id(q.candidate);
    e.byte(static_cast<std::uint8_t>(q.kind));
    enc_line(e, q.carrier);
    e.id(q.lower_point);
    e.id(q.upper_point);
    q.carrier_parameters.lower.encode(e);
    q.carrier_parameters.upper.encode(e);
    e.byte(static_cast<std::uint8_t>(q.direction));
    e.u64(q.incidences.size());
    for (const auto &x : q.incidences) enc_incidence(e, x);
    e.u64(q.source_intervals.size());
    for (const auto &x : q.source_intervals) {
      e.id(x.edge_use); x.parameters.lower.encode(e); x.parameters.upper.encode(e);
      e.byte(static_cast<std::uint8_t>(x.orientation));
    }
    e.u64(q.ownership.size());
    for (const auto &x : q.ownership) enc_ownership(e, x);
    e.u64(q.derivations.size());
    for (const auto &d : q.derivations)
      { e.id(d.construction); e.raw(d.evidence.evidence_digest.bytes.data(), 16); }
  }
  e.u64(a.regions.size());
  for (const auto &r : a.regions) {
    e.id(r.id);
    e.id(r.candidate);
    e.byte(static_cast<std::uint8_t>(r.plane_orientation));
    e.u64(r.boundary_cycles.size());
    for (const auto &cycle : r.boundary_cycles) {
      e.byte(static_cast<std::uint8_t>(cycle.role));
      e.u64(cycle.vertices.size());
      for (auto id : cycle.vertices) e.id(id);
      e.u64(cycle.intervals.size());
      for (auto id : cycle.intervals) e.id(id);
      e.u64(cycle.ownership.size());
      for (const auto &edge : cycle.ownership) {
        e.u64(edge.size());
        for (const auto &x : edge) enc_ownership(e, x);
      }
    }
    r.area.encode(e);
    enc_point(e, r.interior_witness);
    e.u64(r.incidences.size());
    for (const auto &x : r.incidences) enc_incidence(e, x);
    e.u64(r.derivations.size());
    for (const auto &d : r.derivations)
      { e.id(d.construction); e.raw(d.evidence.evidence_digest.bytes.data(), 16); }
  }
  e.u64(a.constructions->nodes.size());
  for (const auto &node : a.constructions->nodes) {
    e.id(node.id); e.byte(static_cast<std::uint8_t>(node.kind));
    e.u64(node.children.size()); for (auto child : node.children) e.id(child);
    e.u64(node.defining_sources.size());
    for (const auto &source : node.defining_sources) enc_feature(e, source);
    e.u64(node.defining_relations.size());
    for (auto relation : node.defining_relations) e.id(relation);
    e.byte_string(node.exact_result);
  }
  e.u64(a.constructions->relations.size());
  for (const auto &relation : a.constructions->relations) {
    e.id(relation.id); e.byte(static_cast<std::uint8_t>(relation.kind));
    e.u16(relation.formula_version); e.id(relation.construction);
    e.u64(relation.operand_nodes.size());
    for (auto operand : relation.operand_nodes) e.id(operand);
    e.u64(relation.defining_sources.size());
    for (const auto &source : relation.defining_sources) enc_feature(e, source);
    for (const auto &coefficient : relation.coefficients) encode(e, coefficient);
    e.byte(static_cast<std::uint8_t>(relation.expected));
  }
  return e.bytes();
}
template <class T, class I>
std::vector<std::uint8_t> invocation(const raw_event_set<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBRAW05";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(raw_event_set_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.raw(a.upstream_digest.bytes.data(), 16);
  e.raw(a.kernel_policy_digest.bytes.data(), 16);
  e.byte_string(a.canonical_event_bytes);
  return e.bytes();
}
template <class T, class I>
digest artifact_digest_for(const raw_event_set<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBART01";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.byte(static_cast<std::uint8_t>(artifact_slot::raw_event_set));
  e.u64(type_tag<T, I>());
  e.u16(raw_event_set_schema);
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}
template <class T, class I>
bool structurally_valid(const raw_event_set<T, I> &a) {
  if (!a.candidates || a.owner != a.candidates->owner ||
      !a.constructions || a.constructions->owner != a.owner ||
      a.upstream_digest != a.candidates->artifact_digest ||
      a.classifications.size() != a.candidates->payload->candidates.size())
    return false;
  for (std::size_t i = 0; i < a.constructions->nodes.size(); ++i) {
    const auto &node = a.constructions->nodes[i];
    if (node.id.value_for_debug() != i ||
        node.kind != construction_kind::exact_relation) return false;
    for (auto child : node.children)
      if (!child.valid() || child.value_for_debug() >= i) return false;
    for (auto relation : node.defining_relations)
      if (!relation.valid() || relation.value_for_debug() >= a.constructions->relations.size() ||
          a.constructions->relations[relation.value_for_debug()].construction != node.id)
        return false;
  }
  for (std::size_t i = 0; i < a.constructions->relations.size(); ++i) {
    const auto &relation = a.constructions->relations[i];
    if (relation.id.value_for_debug() != i || relation.formula_version != 1 ||
        relation.construction.value_for_debug() >= a.constructions->nodes.size() ||
        static_cast<unsigned>(relation.kind) >
            static_cast<unsigned>(defining_relation_kind::ordered_on_carrier) ||
        (relation.expected != exact_sign::negative &&
         relation.expected != exact_sign::zero &&
         relation.expected != exact_sign::positive))
      return false;
    if (relation.kind == defining_relation_kind::coordinate_equality &&
        relation.defining_sources !=
            a.constructions->nodes[relation.construction.value_for_debug()]
                .defining_sources)
      return false;
  }
  auto valid_derivations = [&](const auto &records) {
    for (const auto &record : records) for (const auto &d : record.derivations) {
      if (!d.construction.valid() ||
          d.construction.value_for_debug() >= a.constructions->nodes.size())
        return false;
      const auto &node = a.constructions->nodes[d.construction.value_for_debug()];
      if (node.defining_sources != d.defining_sources ||
          node.exact_result != d.evidence.exact_payload) return false;
    }
    return true;
  };
  if (!valid_derivations(a.points) || !valid_derivations(a.intervals) ||
      !valid_derivations(a.regions) || !valid_derivations(a.carriers)) return false;
  for (const auto &point : a.points)
    for (const auto &derivation : point.derivations) {
      const auto &node = a.constructions->nodes[
          derivation.construction.value_for_debug()];
      for (auto relation : node.defining_relations)
        if (!defining_relation_satisfied(
                a.constructions->relations[relation.value_for_debug()],
                point.point))
          return false;
    }
  std::uint64_t next = 0;
  std::vector<bool> seen;
  for (std::size_t i = 0; i < a.classifications.size(); ++i) {
    const auto &c = a.classifications[i];
    if (c.candidate.value_for_debug() != i || c.event_begin != next ||
        c.event_end < c.event_begin)
      return false;
    next = c.event_end;
  }
  seen.assign(next, false);
  auto claim = [&](raw_event_id id) {
    if (!id.valid() || id.value_for_debug() >= seen.size() ||
        seen[id.value_for_debug()])
      return false;
    seen[id.value_for_debug()] = true;
    return true;
  };
  if (a.event_index.size() != next) return false;
  for (std::size_t i = 0; i < a.points.size(); ++i) {
    const auto &p = a.points[i];
    if (!claim(p.id) || p.candidate.value_for_debug() >= a.classifications.size() ||
        p.facets.operand_a_facet != a.classifications[p.candidate.value_for_debug()].facets.operand_a_facet ||
        p.facets.operand_b_facet != a.classifications[p.candidate.value_for_debug()].facets.operand_b_facet ||
        a.event_index[p.id.value_for_debug()].dimension != event_dimension::point ||
        a.event_index[p.id.value_for_debug()].ordinal != i)
      return false;
  }
  auto point = [&](raw_event_id id) { return find_raw_point(a, id); };
  for (std::size_t i = 0; i < a.intervals.size(); ++i) {
    const auto &q = a.intervals[i];
    auto lo = point(q.lower_point), hi = point(q.upper_point);
    if (!claim(q.id) || !lo || !hi ||
        a.event_index[q.id.value_for_debug()].dimension != event_dimension::interval ||
        a.event_index[q.id.value_for_debug()].ordinal != i ||
        !(q.carrier_parameters.lower < q.carrier_parameters.upper) ||
        !(at(q.carrier, q.carrier_parameters.lower) == lo->point) ||
        !(at(q.carrier, q.carrier_parameters.upper) == hi->point) ||
        lo->candidate != q.candidate || hi->candidate != q.candidate ||
        q.ownership.empty())
      return false;
    auto expected_mappings = source_intervals(lo->point, hi->point,
                                              q.incidences,
                                              *a.candidates->payload->validated->payload);
    if (q.source_intervals.size() != expected_mappings.size()) return false;
    for (std::size_t i = 0; i < q.source_intervals.size(); ++i) {
      const auto &stored = q.source_intervals[i];
      const auto &expected = expected_mappings[i];
      if (stored.edge_use != expected.edge_use ||
          stored.parameters.lower != expected.parameters.lower ||
          stored.parameters.upper != expected.parameters.upper ||
          stored.orientation != expected.orientation ||
          stored.parameters.lower < exact_scalar(0) ||
          exact_scalar(1) < stored.parameters.upper)
        return false;
    }
    for (const auto &owner : q.ownership)
      if (!owner.operand.valid() || owner.multiplicity == 0 ||
          (owner.direction != orientation_parity::agree &&
           owner.direction != orientation_parity::opposite)) return false;
  }
  for (std::size_t region_ordinal = 0; region_ordinal < a.regions.size();
       ++region_ordinal) {
    const auto &r = a.regions[region_ordinal];
    if (!claim(r.id) || !(exact_scalar(0) < r.area) ||
        a.event_index[r.id.value_for_debug()].dimension != event_dimension::region ||
        a.event_index[r.id.value_for_debug()].ordinal != region_ordinal ||
        r.boundary_cycles.empty())
      return false;
    exact_scalar signed_area(0);
    std::size_t outer_count = 0;
    std::vector<exact_point2> outer_points;
    const auto &f = a.candidates->payload->validated->payload->facets[
        r.facets.operand_a_facet.value_for_debug()];
    for (const auto &cycle : r.boundary_cycles) {
      if (cycle.vertices.size() < 3 ||
          cycle.intervals.size() != cycle.vertices.size() ||
          cycle.ownership.size() != cycle.intervals.size())
        return false;
      exact_scalar twice(0);
      for (std::size_t i = 0; i < cycle.vertices.size(); ++i) {
        auto id = cycle.vertices[i];
        if (!point(id))
          return false;
        const auto *p = point(id);
        const auto *q = point(cycle.vertices[(i + 1) % cycle.vertices.size()]);
        if (!q) return false;
        const auto *interval = find_raw_interval(a, cycle.intervals[i]);
        if (!interval || interval->candidate != r.candidate ||
            interval->lower_point != id ||
            interval->upper_point != cycle.vertices[(i + 1) % cycle.vertices.size()] ||
            interval->kind != raw_interval_kind::coplanar_overlap_boundary_segment)
          return false;
        auto pp = project(p->point, f.projection), qq = project(q->point, f.projection);
        twice = twice + pp.x * qq.y - pp.y * qq.x;
      }
      if (twice.is_zero() ||
          (cycle.role == region_cycle_role::outer) !=
              (twice.sign() == exact_sign::positive))
        return false;
      if (cycle.role == region_cycle_role::outer) {
        ++outer_count;
        outer_points.clear();
        for (auto id : cycle.vertices)
          outer_points.push_back(project(point(id)->point, f.projection));
      }
      signed_area = signed_area + twice;
    }
    if (outer_count != 1 ||
        r.boundary_cycles.front().role != region_cycle_role::outer ||
        signed_area / exact_scalar(2) != r.area)
      return false;
    for (const auto &cycle : r.boundary_cycles) {
      if (cycle.role != region_cycle_role::hole) continue;
      auto location = classify_point_polygon(
          project(point(cycle.vertices.front())->point, f.projection),
          outer_points);
      if (!location.has_value() ||
          location.value().kind != point_region_kind::open_interior)
        return false;
    }
  }
  if (std::find(seen.begin(), seen.end(), false) != seen.end())
    return false;
  std::vector<std::array<std::uint64_t, 3>> candidate_counts(
      a.classifications.size());
  for (const auto &p : a.points) ++candidate_counts[p.candidate.value_for_debug()][0];
  for (const auto &q : a.intervals) ++candidate_counts[q.candidate.value_for_debug()][1];
  for (const auto &r : a.regions) ++candidate_counts[r.candidate.value_for_debug()][2];
  for (const auto &c : a.classifications) {
    const auto counts = candidate_counts[c.candidate.value_for_debug()];
    const auto pc = counts[0], ic = counts[1], rc = counts[2];
    if (c.facets.operand_a_facet != a.candidates->payload->candidates[c.candidate.value_for_debug()].key.operand_a_facet ||
        c.facets.operand_b_facet != a.candidates->payload->candidates[c.candidate.value_for_debug()].key.operand_b_facet ||
        c.carrier_end < c.carrier_begin || c.carrier_end > a.carriers.size() ||
        pc != c.point_count || ic != c.interval_count || rc != c.region_count ||
        pc + ic + rc != c.event_end - c.event_begin)
      return false;
  }
  const auto &validated = *a.candidates->payload->validated->payload;
  for (const auto &p : a.points) {
    const auto &c = a.classifications[p.candidate.value_for_debug()];
    std::vector<raw_source_incidence> expected = point_incidences(
        p.point, validated, facet(validated, c.facets.operand_a_facet));
    auto other = point_incidences(
        p.point, validated, facet(validated, c.facets.operand_b_facet));
    expected.insert(expected.end(), other.begin(), other.end());
    normalize_incidences(expected);
    auto actual = p.incidences;
    normalize_incidences(actual);
    if (expected.size() != actual.size() ||
        !std::equal(expected.begin(), expected.end(), actual.begin(),
                    canonical_incidence_equal))
      return false;
  }
  return semantic(a) == a.canonical_event_bytes &&
         invocation(a) == a.artifact_bytes &&
         artifact_digest_for(a) == a.artifact_digest;
}
template <class T, class I>
status_or<bool> independently_verify(const raw_event_set<T, I> &a,
                                     const verification_environment_view &env) {
  auto work = checked_add(a.classifications.size(), a.points.size(),
                          boolean_stage::intersection_events);
  if (!work.has_value()) return work.error();
  auto dimensions = checked_add(a.intervals.size(), a.regions.size(),
                                boolean_stage::intersection_events);
  if (!dimensions.has_value()) return dimensions.error();
  work = checked_add(work.value(), dimensions.value(),
                     boolean_stage::intersection_events);
  if (!work.has_value()) return work.error();
  std::optional<resource_reservation> work_charge, scratch_charge;
  if (env.accountant) {
    auto charged = env.accountant->reserve_scoped(
        resource_kind::verifier_work, work.value(),
        boolean_stage::intersection_events);
    if (!charged.has_value()) return charged.error();
    work_charge.emplace(std::move(charged.value()));
    auto bytes = checked_multiply(a.points.size(), sizeof(exact_point3),
                                  boolean_stage::intersection_events);
    if (!bytes.has_value()) return bytes.error();
    charged = env.accountant->reserve_scoped(
        resource_kind::verifier_scratch_bytes, bytes.value(),
        boolean_stage::intersection_events);
    if (!charged.has_value()) return charged.error();
    scratch_charge.emplace(std::move(charged.value()));
  }
  const auto &validated = *a.candidates->payload->validated->payload;
  for (std::size_t i = 0; i < a.classifications.size(); ++i) {
    if ((i & 63U) == 0 && env.cancelled && env.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::intersection_events, "cancelled");
    const auto &stored = a.classifications[i];
    const auto &candidate = a.candidates->payload->candidates[i];
    const auto &fa = facet(validated, candidate.key.operand_a_facet);
    const auto &fb = facet(validated, candidate.key.operand_b_facet);
    const auto relation = intersect_planes(fa.plane, fb.plane);
    const auto expected = relation.kind == plane_plane_kind::parallel_disjoint
        ? event_plane_relation::parallel_disjoint
        : relation.kind == plane_plane_kind::nonparallel
              ? event_plane_relation::nonparallel
              : relation.kind == plane_plane_kind::coincident_same
                    ? event_plane_relation::coincident_same_orientation
                    : event_plane_relation::coincident_opposite_orientation;
    if (stored.facets.operand_a_facet != fa.id ||
        stored.facets.operand_b_facet != fb.id || stored.plane_relation != expected)
      return false;
    if (relation.kind == plane_plane_kind::parallel_disjoint &&
        (stored.aggregate != pair_aggregate_relation::disjoint ||
          stored.event_begin != stored.event_end ||
          stored.carrier_begin != stored.carrier_end)) return false;
    if (relation.kind == plane_plane_kind::nonparallel) {
      const auto &line = *relation.line;
      auto expected_components = intersect_sets(line_polygon(line, validated, fa),
                                                line_polygon(line, validated, fb));
      std::vector<exact_interval> actual_intervals;
      std::vector<exact_scalar> actual_points;
      for (std::uint64_t id = stored.event_begin; id < stored.event_end; ++id) {
        const auto event = raw_event_id::from_canonical_value(id);
        if (const auto *interval = find_raw_interval(a, event))
          actual_intervals.push_back(interval->carrier_parameters);
        else if (const auto *point_event = find_raw_point(a, event))
          actual_points.push_back(parameter(line, point_event->point));
      }
      std::sort(actual_intervals.begin(), actual_intervals.end(),
                [](const auto &x, const auto &y) {
                  return x.lower == y.lower ? x.upper < y.upper
                                            : x.lower < y.lower;
                });
      std::sort(actual_points.begin(), actual_points.end());
      actual_points.erase(std::unique(actual_points.begin(), actual_points.end()),
                          actual_points.end());
      std::vector<exact_interval> expected_intervals;
      std::vector<exact_scalar> expected_points;
      for (const auto &component : expected_components) {
        expected_points.push_back(component.lower);
        if (component.lower != component.upper) {
          expected_points.push_back(component.upper);
          expected_intervals.push_back(component);
        }
      }
      std::sort(expected_points.begin(), expected_points.end());
      expected_points.erase(std::unique(expected_points.begin(), expected_points.end()),
                            expected_points.end());
      if (actual_intervals.size() != expected_intervals.size() ||
          actual_points != expected_points) return false;
      for (std::size_t j = 0; j < actual_intervals.size(); ++j)
        if (actual_intervals[j].lower != expected_intervals[j].lower ||
            actual_intervals[j].upper != expected_intervals[j].upper)
          return false;
    } else if (relation.kind != plane_plane_kind::parallel_disjoint) {
      auto a2 = ring2(validated, fa), b2 = ring2(validated, fb);
      const auto expected_area = verifier_overlap_area(a2, b2);
      exact_scalar stored_area(0);
      for (std::uint64_t id = stored.event_begin; id < stored.event_end; ++id)
        if (const auto *region = find_raw_region(
                a, raw_event_id::from_canonical_value(id)))
          stored_area = stored_area + region->area;
      if (stored_area != expected_area) return false;
      if (exact_scalar(0) < expected_area) {
        const auto expected_boundary = verifier_overlap_boundary(validated, fa, fb);
        std::vector<bool> matched(expected_boundary.size(), false);
        std::size_t actual_count = 0;
        for (std::uint64_t id = stored.event_begin; id < stored.event_end; ++id) {
          const auto *region = find_raw_region(
              a, raw_event_id::from_canonical_value(id));
          if (!region) continue;
          for (const auto &cycle : region->boundary_cycles) {
            for (std::size_t edge = 0; edge < cycle.intervals.size(); ++edge) {
              ++actual_count;
              const auto *interval = find_raw_interval(a, cycle.intervals[edge]);
              if (!interval) return false;
              const auto *lower = find_raw_point(a, interval->lower_point);
              const auto *upper = find_raw_point(a, interval->upper_point);
              if (!lower || !upper) return false;
              const auto from = project(lower->point, fa.projection);
              const auto to = project(upper->point, fa.projection);
              const auto expected = std::find_if(
                  expected_boundary.begin(), expected_boundary.end(),
                  [&](const auto &x) { return x.from == from && x.to == to; });
              if (expected == expected_boundary.end()) return false;
              const auto index = static_cast<std::size_t>(
                  std::distance(expected_boundary.begin(), expected));
              if (matched[index]) return false;
              matched[index] = true;
              auto interval_owners = interval->ownership;
              auto cycle_owners = cycle.ownership[edge];
              verifier_normalize_ownership(interval_owners);
              verifier_normalize_ownership(cycle_owners);
              if (interval_owners.size() != expected->ownership.size() ||
                  cycle_owners.size() != expected->ownership.size()) return false;
              if (!std::equal(expected->ownership.begin(),
                              expected->ownership.end(),
                              interval_owners.begin(),
                              canonical_ownership_equal) ||
                  !std::equal(expected->ownership.begin(),
                              expected->ownership.end(), cycle_owners.begin(),
                              canonical_ownership_equal))
                return false;
            }
          }
        }
        if (actual_count != expected_boundary.size() ||
            std::find(matched.begin(), matched.end(), false) != matched.end())
          return false;
      }
    }
  }
  for (std::size_t i = 0; i < a.points.size(); ++i) {
    if ((i & 63U) == 0 && env.cancelled && env.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::intersection_events, "cancelled");
    const auto &p = a.points[i];
    const auto &c = a.classifications[p.candidate.value_for_debug()];
    const auto &fa = facet(validated, c.facets.operand_a_facet);
    const auto &fb = facet(validated, c.facets.operand_b_facet);
    if (!inside(p.point, fa, ring2(validated, fa)) ||
        !inside(p.point, fb, ring2(validated, fb))) return false;
  }
  if (env.cancelled && env.cancelled())
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::intersection_events, "cancelled");
  return structurally_valid(a);
}
template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    const auto &a = *static_cast<const raw_event_set<T, I> *>(v.payload);
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::intersection_events;
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
    r.dependency_digests = {a.upstream_digest};
    auto bytes = encode_verification_report(r);
    if (!bytes.has_value())
      return bytes.error();
    r.report_digest = domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}},
                                    bytes.value());
    return r;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::intersection_events,
                      "event_verifier_exception");
  }
}
template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}
} // namespace

status_or<bool> register_intersection_events_verifier(verifier_registry &r,
                                                      coordinate_tag c,
                                                      index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::raw_event_set;
  x.artifact_type_tag = raw_event_set_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = raw_event_set_schema;
  x.mandatory = {invariant_code::event_binding, invariant_code::event_ledger,
                 invariant_code::event_geometry,
                 invariant_code::event_canonical_encoding};
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
status_or<std::shared_ptr<const published_artifact<raw_event_set<T, I>>>>
discover_intersection_events(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::intersection_events, "cancelled");
    auto upstream = enumerate_broad_phase_candidates(ctx);
    if (!upstream.has_value())
      return upstream.error();
    performance_scope producer(ctx.performance_collector_for_internal_use(),
                               boolean_stage::intersection_events,
                               performance_role::producer);
    stage_transaction<raw_event_set<T, I>> tx(
        ctx.owner(), boolean_stage::intersection_events,
        artifact_slot::raw_event_set, std::make_unique<raw_event_set<T, I>>(),
        ctx.performance_collector_for_internal_use());
    auto &a = tx.draft();
    a.owner = ctx.owner();
    a.setup_digest = ctx.replay().setup;
    a.upstream_digest = upstream.value()->artifact_digest;
    a.kernel_policy_digest = upstream.value()->payload->kernel_policy_digest;
    a.candidates = upstream.value();
    const auto &v = *upstream.value()->payload->validated->payload;
    auto work = checked_add(upstream.value()->payload->candidates.size(),
                            v.edge_uses.size(),
                            boolean_stage::intersection_events);
    if (!work.has_value()) return work.error();
    auto work_charge = ctx.accountant().reserve_scoped(
        resource_kind::work_units, work.value(),
        boolean_stage::intersection_events);
    if (!work_charge.has_value()) return work_charge.error();
    auto private_bytes = checked_multiply(work.value(),
                                          sizeof(raw_source_incidence),
                                          boolean_stage::intersection_events);
    if (!private_bytes.has_value()) return private_bytes.error();
    auto private_charge = ctx.accountant().reserve_scoped(
        resource_kind::stage_private_bytes, private_bytes.value(),
        boolean_stage::intersection_events);
    if (!private_charge.has_value()) return private_charge.error();
    const auto &candidates = upstream.value()->payload->candidates;
    std::vector<candidate_shard> shards(candidates.size());
    std::vector<deterministic_task> tasks;
    tasks.reserve(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      tasks.push_back({candidates[i].id.value_for_debug(), [&, i]() -> status_or<bool> {
        if (ctx.cancelled())
          return make_error(boolean_error_code::resource_limit,
                            boolean_stage::intersection_events, "cancelled");
        const auto &candidate = candidates[i];
        const auto &fa = facet(v, candidate.key.operand_a_facet);
        const auto &fb = facet(v, candidate.key.operand_b_facet);
        candidate_shard shard;
        shard.planes = intersect_planes(fa.plane, fb.plane);
        if (shard.planes->kind == plane_plane_kind::nonparallel) {
          const auto &line = *shard.planes->line;
          shard.nonparallel_components = intersect_sets(
              line_polygon(line, v, fa), line_polygon(line, v, fb));
        } else if (shard.planes->kind != plane_plane_kind::parallel_disjoint) {
          shard.coplanar_overlay = overlay_polygons(ring2(v, fa), ring2(v, fb));
        }
        shards[i] = std::move(shard);
        return true;
      }});
    }
    cancellation_source dispatch_cancellation;
    auto dispatched = ctx.executor().run(std::move(tasks),
                                         dispatch_cancellation.token());
    if (!dispatched.has_value()) {
      auto error = dispatched.error();
      error.stage = boolean_stage::intersection_events;
      return error;
    }
    std::uint64_t next = 0;
    for (std::size_t shard_index = 0; shard_index < candidates.size();
         ++shard_index) {
      const auto &cand = candidates[shard_index];
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::intersection_events, "cancelled");
      candidate_classification c;
      c.candidate = cand.id;
      c.facets = cand.key;
      c.event_begin = next;
      c.carrier_begin = a.carriers.size();
      const auto &fa = facet(v, cand.key.operand_a_facet);
      const auto &fb = facet(v, cand.key.operand_b_facet);
      if (!shards[shard_index].planes)
        throw std::logic_error("missing candidate shard");
      const auto &pp = *shards[shard_index].planes;
      if (pp.kind == plane_plane_kind::parallel_disjoint) {
        c.plane_relation = event_plane_relation::parallel_disjoint;
      } else if (pp.kind == plane_plane_kind::nonparallel) {
        c.plane_relation = event_plane_relation::nonparallel;
        auto line = *pp.line;
        const auto &common = shards[shard_index].nonparallel_components;
        if (!common.empty())
          a.carriers.push_back(
              {cand.id, line, orientation_parity::agree, {fa.id, fb.id}, {}});
        for (const auto &q : common) {
          auto addp = [&](const exact_scalar &t, raw_point_kind k) {
            auto value = at(line, t);
            auto prior = std::find_if(a.points.begin(), a.points.end(),
              [&](const auto &x) { return x.candidate == cand.id && x.point == value; });
            if (prior != a.points.end()) return std::make_pair(prior->id, false);
            raw_point_event p;
            p.id = raw_event_id::from_canonical_value(next++);
            p.candidate = cand.id;
            p.facets = cand.key;
            p.point = value;
            p.kind = k;
            p.incidences = point_incidences(p.point, v, fa);
            auto bi = point_incidences(p.point, v, fb);
            p.incidences.insert(p.incidences.end(), bi.begin(), bi.end());
            normalize_incidences(p.incidences);
            p.derivations.push_back(exact_derivation(p.incidences,
                                                     point_payload(p.point)));
            a.points.push_back(std::move(p));
            return std::make_pair(a.points.back().id, true);
          };
          if (q.lower == q.upper) {
            if (addp(q.lower, raw_point_kind::tangency).second) ++c.point_count;
          } else {
            auto lop = addp(q.lower, raw_point_kind::proper_transverse_endpoint),
                 hip = addp(q.upper, raw_point_kind::proper_transverse_endpoint);
            auto lo = lop.first, hi = hip.first;
            c.point_count += lop.second + hip.second;
            raw_interval_event z;
            z.id = raw_event_id::from_canonical_value(next++);
            z.candidate = cand.id;
            z.facets = cand.key;
            z.carrier = line;
            z.lower_point = lo;
            z.upper_point = hi;
            z.carrier_parameters = q;
            z.incidences = {
                finc(operand_a(), fa.id,
                     local_incidence_location::facet_open_interior),
                finc(operand_b(), fb.id,
                     local_incidence_location::facet_open_interior)};
            z.source_intervals = source_intervals(at(line, q.lower),
                                                  at(line, q.upper),
                                                  z.incidences, v);
            z.ownership = curve_ownership(z.incidences, v);
            apply_source_directions(z.ownership, z.source_intervals);
            canonical_encoder interval_evidence;
            enc_line(interval_evidence, z.carrier);
            z.carrier_parameters.lower.encode(interval_evidence);
            z.carrier_parameters.upper.encode(interval_evidence);
            z.derivations.push_back(exact_derivation(
                z.incidences, interval_evidence.bytes()));
            a.intervals.push_back(std::move(z));
            ++c.interval_count;
          }
        }
        c.aggregate = c.interval_count
                          ? pair_aggregate_relation::curve_contact
                          : c.point_count
                                ? pair_aggregate_relation::point_contact
                                : pair_aggregate_relation::disjoint;
      } else {
        c.plane_relation =
            pp.kind == plane_plane_kind::coincident_same
                ? event_plane_relation::coincident_same_orientation
                : event_plane_relation::coincident_opposite_orientation;
        auto pa = ring2(v, fa), pb = ring2(v, fb);
        const auto &overlay = shards[shard_index].coplanar_overlay;
        auto add_point = [&](const exact_point2 &q) {
          auto p = lift(q, fa.plane, fa.projection);
          auto prior = std::find_if(a.points.begin(), a.points.end(),
            [&](const auto &x) { return x.candidate == cand.id && x.point == p; });
          if (prior != a.points.end()) return prior->id;
          raw_point_event z;
          z.id = raw_event_id::from_canonical_value(next++);
          z.candidate = cand.id;
          z.facets = cand.key;
          z.point = p;
          z.kind = raw_point_kind::overlap_boundary_vertex;
          z.incidences = point_incidences(p, v, fa);
          auto bi = point_incidences(p, v, fb);
          z.incidences.insert(z.incidences.end(), bi.begin(), bi.end());
          normalize_incidences(z.incidences);
          z.derivations.push_back(exact_derivation(z.incidences,
                                                   point_payload(z.point)));
          a.points.push_back(std::move(z));
          ++c.point_count;
          return a.points.back().id;
        };
        struct overlay_component {
          std::size_t outer;
          std::vector<std::size_t> holes;
        };
        std::vector<overlay_component> components;
        for (std::size_t i = 0; i < overlay.cycles.size(); ++i)
          if (polygon_area2(overlay.cycles[i]).sign() == exact_sign::positive)
            components.push_back({i, {}});
        for (std::size_t i = 0; i < overlay.cycles.size(); ++i) {
          if (polygon_area2(overlay.cycles[i]).sign() != exact_sign::negative)
            continue;
          std::optional<std::size_t> owner;
          exact_scalar owner_area;
          for (std::size_t j = 0; j < components.size(); ++j) {
            const auto &outer = overlay.cycles[components[j].outer];
            auto location = classify_point_polygon(overlay.cycles[i].front(), outer);
            if (!location.has_value() ||
                location.value().kind != point_region_kind::open_interior)
              continue;
            auto area = polygon_area2(outer);
            if (!owner || area < owner_area) { owner = j; owner_area = area; }
          }
          if (!owner) throw std::logic_error("uncontained overlap hole");
          components[*owner].holes.push_back(i);
        }
        for (const auto &component : components) {
          std::vector<std::size_t> cycle_indices{component.outer};
          cycle_indices.insert(cycle_indices.end(), component.holes.begin(),
                               component.holes.end());
          raw_region_event region;
          region.candidate = cand.id;
          region.facets = cand.key;
          region.plane_orientation = pp.kind == plane_plane_kind::coincident_same
                                         ? orientation_parity::agree
                                         : orientation_parity::opposite;
          exact_scalar twice_area(0);
          for (auto cycle_index : cycle_indices) {
            const auto &overlap = overlay.cycles[cycle_index];
            std::vector<raw_event_id> cycle;
            for (const auto &q : overlap)
              cycle.push_back(add_point(q));
            raw_region_boundary_cycle boundary;
            boundary.role = polygon_area2(overlap).sign() == exact_sign::positive
                                ? region_cycle_role::outer
                                : region_cycle_role::hole;
            boundary.vertices = cycle;
            for (std::size_t i = 0; i < overlap.size(); ++i) {
              auto p = lift(overlap[i], fa.plane, fa.projection);
              auto q = lift(overlap[(i + 1) % overlap.size()], fa.plane,
                            fa.projection);
              raw_interval_event z;
              z.id = raw_event_id::from_canonical_value(next++);
              z.candidate = cand.id;
              z.facets = cand.key;
              z.kind = raw_interval_kind::coplanar_overlap_boundary_segment;
              z.carrier = {p, q - p};
              z.lower_point = cycle[i];
              z.upper_point = cycle[(i + 1) % cycle.size()];
              z.carrier_parameters = {exact_scalar(0), exact_scalar(1), true,
                                      true};
              auto midpoint = at(z.carrier, exact_scalar(1) / exact_scalar(2));
              z.incidences = point_incidences(midpoint, v, fa);
              auto midpoint_b = point_incidences(midpoint, v, fb);
              z.incidences.insert(z.incidences.end(), midpoint_b.begin(),
                                  midpoint_b.end());
              normalize_incidences(z.incidences);
              z.source_intervals = source_intervals(p, q, z.incidences, v);
              z.ownership = curve_ownership(z.incidences, v);
              apply_source_directions(z.ownership, z.source_intervals);
              canonical_encoder interval_evidence;
              enc_line(interval_evidence, z.carrier);
              z.carrier_parameters.lower.encode(interval_evidence);
              z.carrier_parameters.upper.encode(interval_evidence);
              z.derivations.push_back(exact_derivation(
                  z.incidences, interval_evidence.bytes()));
              a.carriers.push_back({cand.id, z.carrier,
                                    orientation_parity::agree,
                                    {fa.id, fb.id}, {}});
              a.intervals.push_back(std::move(z));
              boundary.intervals.push_back(a.intervals.back().id);
              boundary.ownership.push_back(a.intervals.back().ownership);
              ++c.interval_count;
            }
            twice_area = twice_area + polygon_area2(overlap);
            region.boundary_cycles.push_back(std::move(boundary));
          }
          region.area = twice_area / exact_scalar(2);
          auto witness = overlap_witness(overlay.cycles[component.outer], pa, pb);
          region.interior_witness = lift(witness, fa.plane, fa.projection);
          region.incidences = {finc(operand_a(), fa.id, local_incidence_location::facet_open_interior),
                               finc(operand_b(), fb.id, local_incidence_location::facet_open_interior)};
          normalize_incidences(region.incidences);
          canonical_encoder region_evidence;
          region.area.encode(region_evidence);
          enc_point(region_evidence, region.interior_witness);
          region.derivations.push_back(exact_derivation(
              region.incidences, region_evidence.bytes()));
          region.id = raw_event_id::from_canonical_value(next++);
          a.regions.push_back(std::move(region));
          ++c.region_count;
        }
        const bool same_geometry = overlay.cycles.size() == 1 &&
            overlay.cycles.front().size() == pa.size() && pa.size() == pb.size() &&
            std::all_of(pa.begin(), pa.end(), [&](const auto &p) {
              return std::find(pb.begin(), pb.end(), p) != pb.end();
            });
        if (c.region_count)
          c.aggregate = same_geometry
                            ? (pp.kind == plane_plane_kind::coincident_same
                                   ? pair_aggregate_relation::equal_same_orientation
                                   : pair_aggregate_relation::equal_opposite_orientation)
                            : pair_aggregate_relation::coplanar_positive_area_overlap;
        else if (!overlay.contact_segments.empty()) {
          for (const auto &segment : overlay.contact_segments) {
            auto lo = add_point(segment.first), hi = add_point(segment.second);
            if (lo == hi) continue;
            auto p = lift(segment.first, fa.plane, fa.projection);
            auto q = lift(segment.second, fa.plane, fa.projection);
            raw_interval_event z;
            z.id = raw_event_id::from_canonical_value(next++);
            z.candidate = cand.id; z.facets = cand.key;
            z.kind = raw_interval_kind::coincident_source_edge_subsegment;
            z.carrier = {p, q - p}; z.lower_point = lo; z.upper_point = hi;
            z.carrier_parameters = {exact_scalar(0), exact_scalar(1), true, true};
            auto midpoint = at(z.carrier, exact_scalar(1) / exact_scalar(2));
            z.incidences = point_incidences(midpoint, v, fa);
            auto midpoint_b = point_incidences(midpoint, v, fb);
            z.incidences.insert(z.incidences.end(), midpoint_b.begin(),
                                midpoint_b.end());
            normalize_incidences(z.incidences);
            z.source_intervals = source_intervals(p, q, z.incidences, v);
            z.ownership = curve_ownership(z.incidences, v);
            apply_source_directions(z.ownership, z.source_intervals);
            canonical_encoder interval_evidence;
            enc_line(interval_evidence, z.carrier);
            z.carrier_parameters.lower.encode(interval_evidence);
            z.carrier_parameters.upper.encode(interval_evidence);
            z.derivations.push_back(exact_derivation(
                z.incidences, interval_evidence.bytes()));
            a.carriers.push_back({cand.id, z.carrier, orientation_parity::agree,
                                  {fa.id, fb.id}, {}});
            a.intervals.push_back(std::move(z)); ++c.interval_count;
          }
          c.aggregate = pair_aggregate_relation::coplanar_boundary_contact;
        } else if (!overlay.contact_points.empty()) {
          for (const auto &p : overlay.contact_points) add_point(p);
          c.aggregate = pair_aggregate_relation::point_contact;
        } else c.aggregate = pair_aggregate_relation::disjoint;
      }
      c.carrier_end = a.carriers.size();
      c.event_end = next;
      a.classifications.push_back(c);
    }
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::intersection_events, "cancelled");
    auto event_charge = ctx.accountant().reserve_scoped(
        resource_kind::raw_events, next, boolean_stage::intersection_events);
    if (!event_charge.has_value()) return event_charge.error();
    a.event_index.resize(next);
    for (std::size_t i = 0; i < a.points.size(); ++i)
      a.event_index[a.points[i].id.value_for_debug()] =
          {event_dimension::point, i};
    for (std::size_t i = 0; i < a.intervals.size(); ++i)
      a.event_index[a.intervals[i].id.value_for_debug()] =
          {event_dimension::interval, i};
    for (std::size_t i = 0; i < a.regions.size(); ++i)
      a.event_index[a.regions[i].id.value_for_debug()] =
          {event_dimension::region, i};
    freeze_constructions(a);
    a.canonical_event_bytes = semantic(a);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::intersection_events,
                        "verifier_registry_required");
    auto spec = registry->specification(artifact_slot::raw_event_set,
                                        type_tag<T, I>(), raw_event_set_schema,
                                        ctx.options().verification);
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
    tx.stage_reservation(std::move(event_charge.value()));
    performance_count(performance_counter::event_candidate_facet_pairs,
                      a.classifications.size());
    performance_count(performance_counter::plane_relation_classes,
                      a.classifications.size());
    performance_count(performance_counter::raw_events,
                      a.points.size() + a.intervals.size() + a.regions.size());
    producer.finish();
    auto ok = tx.freeze_and_verify(type_tag<T, I>(), raw_event_set_schema, 1,
                                   a.artifact_digest, spec.value(), env,
                                   ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::intersection_events, "cancelled");
    return tx.publish();
  } catch (const boolean_error &e) {
    return e;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::intersection_events, "event_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::intersection_events, "event_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T, I)                                                             \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<raw_event_set<T, I>>>>          \
  discover_intersection_events(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST
} // namespace mesh_boolean
} // namespace ygor
