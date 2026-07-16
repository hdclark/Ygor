#include "YgorMeshesBooleanCellClassification.h"
#include <algorithm>
#include <deque>
#include <map>
#include <set>

#if defined(__FAST_MATH__)
#error "Component 9 requires strict floating-point compilation"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Component 9 must not assume finite-only arithmetic"
#endif

namespace ygor {
namespace mesh_boolean {
namespace {
template <class T, class I> std::uint64_t type_tag() {
  return labeled_arrangement_type_tag +
         (static_cast<std::uint64_t>(std::is_same<T, double>::value
                                         ? coordinate_tag::binary64
                                         : coordinate_tag::binary32)
          << 8) +
         static_cast<std::uint64_t>(std::is_same<I, std::uint64_t>::value
                                        ? index_tag::uint64
                                        : index_tag::uint32);
}
void occupancy(canonical_encoder &e, occupancy_pair x) {
  e.boolean(x.in_a);
  e.boolean(x.in_b);
}
std::vector<std::uint8_t> conflict_bytes(
    const classification_conflict_certificate &c) {
  canonical_encoder e;
  const char tag[] = "YGBCON09";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(c.schema);
  e.byte(static_cast<std::uint8_t>(c.kind));
  for (const auto *path : {&c.established, &c.competing}) {
    e.id(path->root);
    e.id(path->terminal);
    e.u64(path->steps.size());
    for (const auto &step : path->steps) {
      e.byte(static_cast<std::uint8_t>(step.kind));
      e.id(step.from);
      e.id(step.to);
      e.boolean(bool(step.transition));
      if (step.transition)
        e.id(*step.transition);
      e.id(step.seed);
      occupancy(e, step.before);
      occupancy(e, step.after);
    }
    occupancy(e, path->terminal_label);
  }
  return e.bytes();
}
void ray_evidence(canonical_encoder &e, const operand_ray_evidence &r) {
  e.byte(r.direction_index);
  encode(e, r.direction.x);
  encode(e, r.direction.y);
  encode(e, r.direction.z);
  e.u32(static_cast<std::uint32_t>(r.signed_degree));
  e.byte(static_cast<std::uint8_t>(r.location));
  e.u64(r.hits.size());
  for (const auto &h : r.hits) {
    e.u64(h.triangle);
    e.boolean(bool(h.source_facet));
    if (h.source_facet)
      e.id(*h.source_facet);
    e.u32(h.source_primitive);
    encode(e, h.parameter_constant);
    encode(e, h.parameter_epsilon);
    e.byte(static_cast<std::uint8_t>(h.signed_contribution));
    e.u32(h.parameter_group);
    e.byte(static_cast<std::uint8_t>(h.ownership));
    e.boolean(bool(h.source_vertex));
    if (h.source_vertex)
      e.id(*h.source_vertex);
    e.u64(h.source_vertex_fan.size());
    for (const auto id : h.source_vertex_fan)
      e.id(id);
    e.boolean(bool(h.source_vertex_fan_index));
    if (h.source_vertex_fan_index)
      e.u32(*h.source_vertex_fan_index);
    e.boolean(bool(h.source_edge));
    if (h.source_edge) {
      e.id((*h.source_edge)[0]);
      e.id((*h.source_edge)[1]);
    }
    e.boolean(bool(h.source_edge_direction));
    if (h.source_edge_direction) {
      e.id((*h.source_edge_direction)[0]);
      e.id((*h.source_edge_direction)[1]);
    }
    e.boolean(h.owns_boundary_crossing);
  }
  e.raw(r.evidence_digest.bytes.data(), 16);
}
template <class Id> void ids(canonical_encoder &e, const std::vector<Id> &v) {
  e.u64(v.size());
  for (auto x : v)
    e.id(x);
}
template <class T, class I>
std::vector<std::uint8_t> semantic(const labeled_arrangement<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN09";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(labeled_arrangement_schema);
  e.raw(a.arrangement_semantic_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(a.classification));
  e.u64(a.regions.size());
  for (const auto &r : a.regions) {
    e.id(r.id);
    e.id(r.source_component);
    occupancy(e, r.label);
    e.id(r.seed);
    ids(e, r.patch_sides);
  }
  e.u64(a.seeds.size());
  for (const auto &s : a.seeds) {
    e.id(s.id);
    e.id(s.source_side);
    e.id(s.source_component);
    e.byte(static_cast<std::uint8_t>(s.base_kind));
    e.u64(s.base_id);
    for (const auto *l : {&s.operand_a, &s.operand_b}) {
      e.byte(static_cast<std::uint8_t>(l->location));
      e.u32(static_cast<std::uint32_t>(l->signed_degree));
      ids(e, l->boundary_sources);
      e.raw(l->evidence_digest.bytes.data(), 16);
    }
    ray_evidence(e, s.operand_a_primary);
    ray_evidence(e, s.operand_a_alternate);
    ray_evidence(e, s.operand_b_primary);
    ray_evidence(e, s.operand_b_alternate);
  }
  e.u64(a.transitions.size());
  for (const auto &t : a.transitions) {
    e.id(t.id);
    e.id(t.reverse);
    e.id(t.source);
    e.id(t.from);
    e.id(t.to);
    e.id(t.from_side);
    e.id(t.to_side);
    e.byte(static_cast<std::uint8_t>(t.crossing));
    e.byte(static_cast<std::uint8_t>(t.transfer));
    occupancy(e, t.before);
    occupancy(e, t.after);
    ids(e, t.uses);
    e.boolean(bool(t.coincidence));
    if (t.coincidence)
      e.id(*t.coincidence);
  }
  e.u64(a.arc_checks.size());
  for (const auto &x : a.arc_checks) {
    e.id(x.transition);
    e.id(x.from_path);
    e.id(x.to_path);
    e.boolean(x.tree_edge);
    occupancy(e, x.transferred);
  }
  e.u64(a.cycle_checks.size());
  for (const auto &x : a.cycle_checks) {
    e.id(x.closing_transition);
    e.id(x.from_path);
    e.id(x.to_path);
    e.id(x.root);
    occupancy(e, x.from_label);
    occupancy(e, x.to_label);
  }
  e.u64(a.propagation.size());
  for (const auto &p : a.propagation) {
    e.id(p.id);
    e.id(p.region);
    e.id(p.root);
    e.boolean(bool(p.predecessor));
    if (p.predecessor)
      e.id(*p.predecessor);
    e.boolean(bool(p.transition));
    if (p.transition)
      e.id(*p.transition);
    occupancy(e, p.propagated);
  }
  e.u64(a.side_labels.size());
  for (const auto &s : a.side_labels) {
    e.id(s.id);
    e.id(s.source_side);
    e.id(s.patch);
    e.byte(static_cast<std::uint8_t>(s.side));
    e.id(s.region);
    occupancy(e, s.occupancy);
  }
  e.u64(a.patch_labels.size());
  for (const auto &p : a.patch_labels) {
    e.id(p.patch);
    e.id(p.negative);
    e.id(p.positive);
    occupancy(e, p.negative_occupancy);
    occupancy(e, p.positive_occupancy);
  }
  e.id(a.certificate.exterior_region);
  e.boolean(bool(a.certificate.exterior_attachment_side));
  if (a.certificate.exterior_attachment_side)
    e.id(*a.certificate.exterior_attachment_side);
  e.boolean(bool(a.certificate.exterior_target_patch));
  if (a.certificate.exterior_target_patch)
    e.id(*a.certificate.exterior_target_patch);
  e.boolean(bool(a.certificate.exterior_target_witness));
  if (a.certificate.exterior_target_witness) {
    encode(e, a.certificate.exterior_target_witness->x);
    encode(e, a.certificate.exterior_target_witness->y);
    encode(e, a.certificate.exterior_target_witness->z);
  }
  e.u64(a.certificate.exterior_first_hits.size());
  for (const auto &h : a.certificate.exterior_first_hits) {
    e.id(h.patch);
    encode(e, h.parameter);
    e.byte(static_cast<std::uint8_t>(h.relation));
    e.id(h.witness_facing_side);
  }
  encode(e, a.certificate.exterior_witness.x);
  encode(e, a.certificate.exterior_witness.y);
  encode(e, a.certificate.exterior_witness.z);
  ray_evidence(e, a.certificate.exterior_operand_a);
  ray_evidence(e, a.certificate.exterior_operand_b);
  e.boolean(a.certificate.exterior_bound_disjoint);
  e.u64(a.certificate.regions);
  e.u64(a.certificate.seeds);
  e.u64(a.certificate.directed_transitions);
  e.u64(a.certificate.side_labels);
  return e.bytes();
}
template <class T, class I>
std::vector<std::uint8_t> invocation(const labeled_arrangement<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBLAB09";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(labeled_arrangement_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.raw(a.arrangement_digest.bytes.data(), 16);
  e.raw(a.validated_digest.bytes.data(), 16);
  e.byte_string(a.canonical_bytes);
  return e.bytes();
}
template <class T, class I>
digest artifact_digest_for(const labeled_arrangement<T, I> &a) {
  canonical_encoder e;
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(artifact_slot::labeled_arrangement));
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}
template <class T, class I>
std::vector<sourced_exact_triangle3>
operand_triangles(const validated_operands<T, I> &v, operand_id operand) {
  std::vector<sourced_exact_triangle3> out;
  for (const auto &f : v.facets)
    if (f.operand == operand) {
      std::uint32_t primitive = 0;
      std::vector<exact_point3> ring;
      std::vector<std::vector<facet_id>> vertex_fans;
      ring.reserve(f.ring.size());
      vertex_fans.reserve(f.ring.size());
      for (const auto vertex : f.ring) {
        ring.push_back(v.vertices[vertex.value_for_debug()].exact_coordinate);
        std::vector<facet_id> fan;
        for (const auto use : v.vertices[vertex.value_for_debug()].ordered_outgoing_link)
          fan.push_back(v.edge_uses[use.value_for_debug()].facet);
        vertex_fans.push_back(std::move(fan));
      }
      for (const auto &t : f.triangles) {
        out.push_back({{v.vertices[t[0].value_for_debug()].exact_coordinate,
                        v.vertices[t[1].value_for_debug()].exact_coordinate,
                        v.vertices[t[2].value_for_debug()].exact_coordinate},
                       f.id, primitive, std::array<original_vertex_id, 3>{t[0], t[1], t[2]},
                        ring, f.ring, vertex_fans});
        ++primitive;
      }
    }
  return out;
}
exact_sign formal_plane_sign(const exact_plane3 &plane,
                             const formal_open_point_view &point) {
  auto s = plane_side(plane, point.base);
  if (s != exact_sign::zero)
    return s;
  exact_vector3 n{exact_scalar(plane.a, big_uint(1)),
                  exact_scalar(plane.b, big_uint(1)),
                  exact_scalar(plane.c, big_uint(1))};
  if (plane.oriented == orientation_parity::opposite)
    n = n * exact_scalar(-1);
  return dot(n, point.infinitesimal_direction).sign();
}
template <class T, class I>
status_or<formal_open_point_view>
formal_probe(const arrangement_complex<T, I> &a,
             const open_probe_descriptor &p) {
  formal_open_point_view q;
  q.infinitesimal_direction = p.direction;
  q.key.domain = p.base_kind == probe_base_stratum_kind::patch_side
                     ? perturbation_domain::open_side
                     : perturbation_domain::generic_ray;
  q.key.local_rank = p.formula_version;
  if (p.exact_base) {
    q.base = *p.exact_base;
    q.key.stable_features.push_back(p.side);
  } else if (p.base_vertex) {
    const auto i = p.base_vertex->value_for_debug();
    if (i >= a.symbolic->payload->vertices.size())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "probe_base_vertex");
    q.base = a.symbolic->payload->vertices[i].point;
    q.key.stable_features.push_back(*p.base_vertex);
  } else if (p.base_kind != probe_base_stratum_kind::universe)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::cell_classification, "probe_missing_base");
  for (const auto &c : p.constraints)
    if (formal_plane_sign(c.plane, q) != c.required)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification, "probe_constraint");
  return q;
}
digest location_digest(operand_id op, open_region_component_id component,
                        const formal_operand_location &location) {
  canonical_encoder e;
  e.id(op);
  e.id(component);
  e.byte(static_cast<std::uint8_t>(location.location));
  e.u32(static_cast<std::uint32_t>(location.signed_degree));
  e.byte(location.ray_direction_index);
  encode(e, location.ray_direction.x);
  encode(e, location.ray_direction.y);
  encode(e, location.ray_direction.z);
  e.u64(location.hits.size());
  for (const auto &h : location.hits) {
    e.u64(h.triangle);
    if (!h.source_facet)
      throw std::logic_error("classification hit lacks source facet");
    e.id(*h.source_facet);
    e.u32(h.source_primitive);
    encode(e, h.parameter_constant);
    encode(e, h.parameter_epsilon);
    e.byte(static_cast<std::uint8_t>(h.signed_contribution));
    e.u32(h.parameter_group);
    e.byte(static_cast<std::uint8_t>(h.ownership));
    e.boolean(bool(h.source_vertex));
    if (h.source_vertex)
      e.id(*h.source_vertex);
    ids(e, h.source_vertex_fan);
    e.boolean(bool(h.source_vertex_fan_index));
    if (h.source_vertex_fan_index)
      e.u32(*h.source_vertex_fan_index);
    e.boolean(bool(h.source_edge));
    if (h.source_edge) {
      e.id((*h.source_edge)[0]);
      e.id((*h.source_edge)[1]);
    }
    e.boolean(bool(h.source_edge_direction));
    if (h.source_edge_direction) {
      e.id((*h.source_edge_direction)[0]);
      e.id((*h.source_edge_direction)[1]);
    }
    e.boolean(h.owns_boundary_crossing);
  }
  return domain_digest({{'Y', 'G', 'B', 'L', 'O', 'C', '0', '9'}}, e.bytes());
}

exact_sign formal_sign(const exact_scalar &constant,
                       const exact_scalar &epsilon) {
  const auto s = constant.sign();
  return s == exact_sign::zero ? epsilon.sign() : s;
}

struct formal_projected_point {
  exact_point2 constant;
  exact_vector2 epsilon;
};

formal_projected_point project_formal(const exact_point3 &constant,
                                      const exact_vector3 &epsilon,
                                      projection_axis axis) {
  if (axis == projection_axis::drop_x)
    return {{constant.y, constant.z}, {epsilon.y, epsilon.z}};
  if (axis == projection_axis::drop_y)
    return {{constant.z, constant.x}, {epsilon.z, epsilon.x}};
  return {{constant.x, constant.y}, {epsilon.x, epsilon.y}};
}

// Verification-only formal point-in-polygon over the original source ring.
// This deliberately does not use the producer's certified triangulation.
bool formal_point_in_facet(const formal_projected_point &p,
                           const std::vector<exact_point2> &ring) {
  bool inside = false;
  for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
    const auto jy = formal_sign(ring[j].y - p.constant.y,
                                p.epsilon.y.negated());
    const auto iy = formal_sign(ring[i].y - p.constant.y,
                                p.epsilon.y.negated());
    const auto edge = ring[i] - ring[j];
    const auto q = p.constant - ring[j];
    const auto orient = formal_sign(
        edge.x * q.y - edge.y * q.x,
        edge.x * p.epsilon.y - edge.y * p.epsilon.x);
    const bool upward = jy != exact_sign::positive && iy == exact_sign::positive;
    const bool downward = iy != exact_sign::positive && jy == exact_sign::positive;
    if ((upward && orient == exact_sign::positive) ||
        (downward && orient == exact_sign::negative))
      inside = !inside;
  }
  return inside;
}

template <class T, class I>
bool independently_reconstruct_ray(const validated_operands<T, I> &v,
                                   operand_id operand,
                                   const formal_open_point_view &point,
                                   const operand_ray_evidence &stored) {
  const auto &direction = stored.direction;
  std::int64_t degree = 0;
  std::vector<facet_id> sources;
  for (const auto &facet : v.facets) {
    if (facet.operand != operand)
      continue;
    exact_vector3 normal{exact_scalar(facet.plane.a, big_uint(1)),
                         exact_scalar(facet.plane.b, big_uint(1)),
                         exact_scalar(facet.plane.c, big_uint(1))};
    if (facet.plane.oriented == orientation_parity::opposite)
      normal = normal * exact_scalar(-1);
    const auto denominator = dot(normal, direction);
    if (denominator.is_zero())
      continue;
    exact_scalar plane_value(facet.plane.d, big_uint(1));
    plane_value = plane_value + exact_scalar(facet.plane.a, big_uint(1)) * point.base.x +
                  exact_scalar(facet.plane.b, big_uint(1)) * point.base.y +
                  exact_scalar(facet.plane.c, big_uint(1)) * point.base.z;
    if (facet.plane.oriented == orientation_parity::opposite)
      plane_value = plane_value.negated();
    const auto tc = plane_value.negated() / denominator;
    const auto te = dot(normal, point.infinitesimal_direction).negated() /
                    denominator;
    if (formal_sign(tc, te) != exact_sign::positive)
      continue;
    const auto hit = point.base + direction * tc;
    const auto direction_epsilon = direction * te;
    const exact_vector3 hit_epsilon{
        point.infinitesimal_direction.x + direction_epsilon.x,
        point.infinitesimal_direction.y + direction_epsilon.y,
        point.infinitesimal_direction.z + direction_epsilon.z};
    std::vector<exact_point2> ring;
    ring.reserve(facet.ring.size());
    for (const auto vertex : facet.ring)
      ring.push_back(project(v.vertices[vertex.value_for_debug()].exact_coordinate,
                             facet.projection));
    if (!formal_point_in_facet(project_formal(hit, hit_epsilon, facet.projection),
                               ring))
      continue;
    degree += denominator.sign() == exact_sign::positive ? 1 : -1;
    sources.push_back(facet.id);
  }
  std::sort(sources.begin(), sources.end());
  std::vector<facet_id> stored_sources;
  for (const auto &h : stored.hits)
    if (h.source_facet)
      stored_sources.push_back(*h.source_facet);
  std::sort(stored_sources.begin(), stored_sources.end());
  return degree == stored.signed_degree &&
         stored.location == (degree == 1 ? operand_location_kind::inside
                                         : operand_location_kind::outside) &&
         sources == stored_sources;
}
template <class T, class I>
bool independently_check_shell_polarity(const validated_operands<T, I> &v,
                                        operand_id operand,
                                        const formal_open_point_view &point,
                                        const operand_ray_evidence &stored) {
  const auto &direction = stored.direction;
  std::map<shell_id, std::int64_t> degrees;
  for (const auto &facet : v.facets) {
    if (facet.operand != operand)
      continue;
    exact_vector3 normal{exact_scalar(facet.plane.a, big_uint(1)),
                         exact_scalar(facet.plane.b, big_uint(1)),
                         exact_scalar(facet.plane.c, big_uint(1))};
    if (facet.plane.oriented == orientation_parity::opposite)
      normal = normal * exact_scalar(-1);
    const auto denominator = dot(normal, direction);
    if (denominator.is_zero())
      continue;
    exact_scalar plane_value(facet.plane.d, big_uint(1));
    plane_value = plane_value + exact_scalar(facet.plane.a, big_uint(1)) * point.base.x +
                  exact_scalar(facet.plane.b, big_uint(1)) * point.base.y +
                  exact_scalar(facet.plane.c, big_uint(1)) * point.base.z;
    if (facet.plane.oriented == orientation_parity::opposite)
      plane_value = plane_value.negated();
    const auto tc = plane_value.negated() / denominator;
    const auto te = dot(normal, point.infinitesimal_direction).negated() /
                    denominator;
    if (formal_sign(tc, te) != exact_sign::positive)
      continue;
    const auto hit = point.base + direction * tc;
    const auto delta = direction * te;
    const exact_vector3 epsilon{point.infinitesimal_direction.x + delta.x,
                                point.infinitesimal_direction.y + delta.y,
                                point.infinitesimal_direction.z + delta.z};
    std::vector<exact_point2> ring;
    for (const auto vertex : facet.ring)
      ring.push_back(project(v.vertices[vertex.value_for_debug()].exact_coordinate,
                             facet.projection));
    if (formal_point_in_facet(project_formal(hit, epsilon, facet.projection), ring))
      degrees[facet.shell] += denominator.sign() == exact_sign::positive ? 1 : -1;
  }
  std::int64_t operand_degree = 0;
  std::set<shell_id> containing;
  for (const auto shell_id : v.operands[operand.value_for_debug()].shells) {
    const auto &shell = v.shells[shell_id.value_for_debug()];
    const auto degree = degrees[shell_id];
    const auto expected = shell.orientation == shell_orientation::outward ? 1 : -1;
    if (degree != 0 && degree != expected)
      return false;
    if (degree != 0)
      containing.insert(shell_id);
    operand_degree += degree;
  }
  for (const auto shell_id : containing) {
    auto parent = v.shells[shell_id.value_for_debug()].parent;
    while (parent) {
      if (!containing.count(*parent))
        return false;
      parent = v.shells[parent->value_for_debug()].parent;
    }
  }
  const bool occupied = operand_degree == 1;
  return (operand_degree == 0 || occupied) &&
         operand_degree == stored.signed_degree &&
         stored.location == (occupied ? operand_location_kind::inside
                                      : operand_location_kind::outside);
}
std::vector<facet_id> boundary_sources(const formal_operand_location &location) {
  std::vector<facet_id> out;
  for (const auto &h : location.hits) {
    if (!h.source_facet)
      throw std::logic_error("classification hit lacks source facet");
    out.push_back(*h.source_facet);
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}
operand_ray_evidence ray_record(operand_id op,
                                open_region_component_id component,
                                const formal_operand_location &location) {
  return {location.ray_direction_index,
          location.ray_direction,
          location.signed_degree,
          location.location == formal_operand_location_kind::inside
              ? operand_location_kind::inside
              : operand_location_kind::outside,
          location.hits,
          location_digest(op, component, location)};
}
bool same_ray(const operand_ray_evidence &stored,
              const formal_operand_location &actual) {
  if (stored.direction_index != actual.ray_direction_index ||
      stored.direction.x != actual.ray_direction.x ||
      stored.direction.y != actual.ray_direction.y ||
      stored.direction.z != actual.ray_direction.z ||
      stored.signed_degree != actual.signed_degree ||
      stored.location != (actual.location == formal_operand_location_kind::inside
                              ? operand_location_kind::inside
                              : operand_location_kind::outside) ||
      stored.hits.size() != actual.hits.size())
    return false;
  for (std::size_t i = 0; i < stored.hits.size(); ++i) {
    const auto &x = stored.hits[i];
    const auto &y = actual.hits[i];
    if (x.triangle != y.triangle || x.source_facet != y.source_facet ||
        x.source_primitive != y.source_primitive ||
        x.parameter_constant != y.parameter_constant ||
        x.parameter_epsilon != y.parameter_epsilon ||
        x.signed_contribution != y.signed_contribution ||
        x.parameter_group != y.parameter_group || x.ownership != y.ownership ||
        x.source_vertex != y.source_vertex || x.source_edge != y.source_edge ||
        x.source_vertex_fan != y.source_vertex_fan ||
        x.source_vertex_fan_index != y.source_vertex_fan_index ||
        x.source_edge_direction != y.source_edge_direction ||
        x.owns_boundary_crossing != y.owns_boundary_crossing)
      return false;
  }
  return true;
}
template <class T, class I>
bool valid_source_attribution(const validated_operands<T, I> &v,
                               operand_id operand,
                               const operand_location_evidence &location,
                               const operand_ray_evidence &ray,
                               bool compare_location_sources = true) {
  formal_operand_location replay;
  replay.location = ray.location == operand_location_kind::inside
                        ? formal_operand_location_kind::inside
                        : formal_operand_location_kind::outside;
  replay.signed_degree = ray.signed_degree;
  replay.hits = ray.hits;
  if (!validate_formal_ray_ownership_evidence(replay))
    return false;
  std::vector<facet_id> sources;
  for (const auto &h : ray.hits) {
    if (!h.source_facet || h.source_facet->value_for_debug() >= v.facets.size())
      return false;
    const auto &facet = v.facets[h.source_facet->value_for_debug()];
    if (facet.id != *h.source_facet || facet.operand != operand ||
        h.source_primitive >= facet.triangles.size())
      return false;
    if (h.ownership == formal_ray_ownership_kind::source_vertex) {
      if (!h.source_vertex ||
          std::find(facet.ring.begin(), facet.ring.end(), *h.source_vertex) ==
              facet.ring.end())
        return false;
      const auto vi = h.source_vertex->value_for_debug();
      if (vi >= v.vertices.size() || v.vertices[vi].operand != operand ||
          v.vertices[vi].ordered_outgoing_link.empty())
        return false;
      std::vector<facet_id> fan;
      for (const auto use_id : v.vertices[vi].ordered_outgoing_link)
        fan.push_back(v.edge_uses[use_id.value_for_debug()].facet);
      if (h.source_vertex_fan != fan || !h.source_vertex_fan_index ||
          *h.source_vertex_fan_index >= fan.size() ||
          fan[*h.source_vertex_fan_index] != facet.id)
        return false;
      for (const auto use_id : v.vertices[vi].ordered_outgoing_link) {
        if (use_id.value_for_debug() >= v.edge_uses.size())
          return false;
        const auto &use = v.edge_uses[use_id.value_for_debug()];
        if (use.origin != *h.source_vertex || use.operand != operand ||
            use.twin.value_for_debug() >= v.edge_uses.size() ||
            v.edge_uses[use.twin.value_for_debug()].twin != use.id)
          return false;
      }
    } else if (h.ownership == formal_ray_ownership_kind::source_edge) {
      if (!h.source_edge || !h.source_edge_direction)
        return false;
      bool found = false;
      for (std::size_t i = 0; i < facet.ring.size(); ++i) {
        const std::array<original_vertex_id, 2> directed{
            facet.ring[i], facet.ring[(i + 1) % facet.ring.size()]};
        auto edge = directed;
        if (edge[1] < edge[0])
          std::swap(edge[0], edge[1]);
        found = found || (directed == *h.source_edge_direction &&
                          edge == *h.source_edge);
      }
      if (!found)
        return false;
      auto edge = std::find_if(v.edges.begin(), v.edges.end(), [&](const auto &x) {
        return x.operand == operand && x.first == (*h.source_edge)[0] &&
               x.second == (*h.source_edge)[1];
      });
      if (edge == v.edges.end())
        return false;
      const auto &first = v.edge_uses[edge->uses[0].value_for_debug()];
      const auto &second = v.edge_uses[edge->uses[1].value_for_debug()];
      if (first.twin != second.id || second.twin != first.id ||
          first.facet == second.facet || first.shell != second.shell ||
          first.shell != edge->shell)
        return false;
    } else if (h.ownership == formal_ray_ownership_kind::facet_interior &&
               (h.source_vertex || h.source_edge || h.source_edge_direction))
      return false;
    sources.push_back(*h.source_facet);
  }
  std::sort(sources.begin(), sources.end());
  sources.erase(std::unique(sources.begin(), sources.end()), sources.end());
  return !compare_location_sources || sources == location.boundary_sources;
}
template <class T, class I>
exact_point3 exterior_witness(const validated_operands<T, I> &v,
                              const arrangement_complex<T, I> &g) {
  exact_scalar m(0);
  for (const auto &x : v.vertices)
    for (const auto *c : {&x.exact_coordinate.x, &x.exact_coordinate.y,
                          &x.exact_coordinate.z})
      if (m < c->abs())
        m = c->abs();
  m = m + exact_scalar(1);
  if (g.patches.empty())
    return {m, m, m};
  // A nonzero plane polynomial has at most three roots on this curve.
  // Trying 3*n+1 integer parameters therefore finds a point off every plane.
  for (std::size_t i = 1; i <= 3 * g.patches.size() + 1; ++i) {
    const exact_scalar t(static_cast<std::int64_t>(i));
    const exact_point3 q{m + t, m + t * t, m + t * t * t};
    bool incident = false;
    for (const auto &p : g.patches)
      incident = incident || plane_side(p.plane, q) == exact_sign::zero;
    if (!incident)
      return q;
  }
  throw std::logic_error("failed to construct exterior witness");
}
struct exterior_attachment_result {
  global_patch_id target_patch;
  exact_point3 target_witness;
  patch_side_id side;
  std::vector<exterior_attachment_hit> first_hits;
};

template <class T, class I>
point_region_relation patch_relation(const arrangement_complex<T, I> &g,
                                     const global_patch &patch,
                                     const exact_point3 &point) {
  std::vector<exact_point2> outer;
  for (auto id : patch.outer)
    outer.push_back(project(g.symbolic->payload->vertices[
                                g.vertices[id.value_for_debug()].symbolic.value_for_debug()]
                                .point,
                            dominant_projection(patch.plane)));
  auto relation = classify_point_polygon(project(point, dominant_projection(patch.plane)), outer);
  if (!relation.has_value())
    throw std::logic_error("invalid attachment patch polygon");
  if (relation.value().kind == point_region_kind::outside)
    return relation.value();
  for (const auto &hole_ids : patch.holes) {
    std::vector<exact_point2> hole;
    for (auto id : hole_ids)
      hole.push_back(project(g.symbolic->payload->vertices[
                                 g.vertices[id.value_for_debug()].symbolic.value_for_debug()]
                                 .point,
                             dominant_projection(patch.plane)));
    auto h = classify_point_polygon(project(point, dominant_projection(patch.plane)), hole);
    if (!h.has_value())
      throw std::logic_error("invalid attachment hole polygon");
    if (h.value().kind == point_region_kind::open_interior)
      return {point_region_kind::outside, 0};
    if (h.value().kind != point_region_kind::outside)
      return h.value();
  }
  return relation.value();
}

template <class T, class I>
exact_point3 patch_interior_witness(const arrangement_complex<T, I> &g,
                                    const global_patch &patch) {
  const exact_scalar third = exact_scalar(1) / exact_scalar(3);
  for (std::size_t i = 0; i < patch.outer.size(); ++i)
    for (std::size_t j = i + 1; j < patch.outer.size(); ++j)
      for (std::size_t k = j + 1; k < patch.outer.size(); ++k) {
        const auto point = [&](std::size_t n) -> const exact_point3 & {
          const auto gv = patch.outer[n].value_for_debug();
          return g.symbolic->payload->vertices[g.vertices[gv].symbolic.value_for_debug()].point;
        };
        const exact_point3 q{(point(i).x + point(j).x + point(k).x) * third,
                             (point(i).y + point(j).y + point(k).y) * third,
                             (point(i).z + point(j).z + point(k).z) * third};
        if (patch_relation(g, patch, q).kind == point_region_kind::open_interior)
          return q;
      }
  throw std::logic_error("patch has no certified interior witness");
}

template <class T, class I>
exterior_attachment_result
exterior_attachment(const arrangement_complex<T, I> &g,
                    const exact_point3 &witness) {
  if (g.patches.empty())
    throw std::logic_error("empty arrangement has no attachment");
  const auto &target = g.patches.front();
  const auto target_witness = patch_interior_witness(g, target);
  const exact_vector3 direction = target_witness - witness;
  std::vector<exterior_attachment_hit> hits;
  for (const auto &patch : g.patches) {
    exact_vector3 normal{exact_scalar(patch.plane.a, big_uint(1)),
                         exact_scalar(patch.plane.b, big_uint(1)),
                         exact_scalar(patch.plane.c, big_uint(1))};
    if (patch.plane.oriented == orientation_parity::opposite)
      normal = normal * exact_scalar(-1);
    const auto denominator = dot(normal, direction);
    if (denominator.is_zero())
      continue;
    const exact_scalar value = exact_scalar(patch.plane.a, big_uint(1)) * witness.x +
                               exact_scalar(patch.plane.b, big_uint(1)) * witness.y +
                               exact_scalar(patch.plane.c, big_uint(1)) * witness.z +
                               exact_scalar(patch.plane.d, big_uint(1));
    auto parameter = value.negated() / dot(exact_vector3{
        exact_scalar(patch.plane.a, big_uint(1)),
        exact_scalar(patch.plane.b, big_uint(1)),
        exact_scalar(patch.plane.c, big_uint(1))}, direction);
    if (!(exact_scalar(0) < parameter) || exact_scalar(1) < parameter)
      continue;
    const auto point = witness + direction * parameter;
    const auto relation = patch_relation(g, patch, point);
    if (relation.kind == point_region_kind::outside)
      continue;
    const auto sign = plane_side(patch.plane, witness);
    if (sign == exact_sign::zero)
      throw std::logic_error("exterior witness lies on hit plane");
    const auto side = sign == exact_sign::negative ? patch_plane_side::negative
                                                    : patch_plane_side::positive;
    auto found = std::find_if(g.patch_sides.begin(), g.patch_sides.end(),
                             [&](const patch_side &s) {
                               return s.patch == patch.id && s.side == side;
                             });
    if (found == g.patch_sides.end())
      throw std::logic_error("attachment hit lacks patch side");
    hits.push_back({patch.id, parameter, relation.kind, found->id});
  }
  if (hits.empty())
    throw std::logic_error("attachment ray misses arrangement");
  std::sort(hits.begin(), hits.end(), [](const auto &a, const auto &b) {
    if (a.parameter != b.parameter) return a.parameter < b.parameter;
    return a.patch < b.patch;
  });
  const auto first_parameter = hits.front().parameter;
  hits.erase(std::remove_if(hits.begin(), hits.end(), [&](const auto &h) {
               return h.parameter != first_parameter;
             }), hits.end());
  const auto component = g.patch_sides[hits.front().witness_facing_side.value_for_debug()].component;
  for (const auto &h : hits)
    if (g.patch_sides[h.witness_facing_side.value_for_debug()].component != component)
      throw std::logic_error("ambiguous exterior attachment group");
  return {target.id, target_witness, hits.front().witness_facing_side, std::move(hits)};
}
template <class T, class I>
bool verify_geometric_evidence(const labeled_arrangement<T, I> &a) {
  const auto &g = *a.arrangement->payload;
  if (a.classification != classification_strategy::independent_patch_side_v1 ||
      g.classification != a.classification)
    return false;
  const auto ta = operand_triangles(*a.validated->payload, operand_a());
  const auto tb = operand_triangles(*a.validated->payload, operand_b());
  if (a.seeds.size() != g.probes.size())
    return false;
  for (std::size_t i = 0; i < g.probes.size(); ++i) {
    auto p = formal_probe(g, g.probes[i]);
    if (!p.has_value())
      return false;
    const auto &s = a.seeds[i];
    auto ap = locate_formal_open_point(p.value(), ta, s.operand_a_primary.direction_index);
    auto aa = locate_formal_open_point(p.value(), ta, s.operand_a_alternate.direction_index);
    auto bp = locate_formal_open_point(p.value(), tb, s.operand_b_primary.direction_index);
    auto ba = locate_formal_open_point(p.value(), tb, s.operand_b_alternate.direction_index);
    if (!ap.has_value() || !aa.has_value() || !bp.has_value() || !ba.has_value() ||
        !same_ray(s.operand_a_primary, ap.value()) ||
        !same_ray(s.operand_a_alternate, aa.value()) ||
        !same_ray(s.operand_b_primary, bp.value()) ||
        !same_ray(s.operand_b_alternate, ba.value()) ||
         !valid_source_attribution(*a.validated->payload, operand_a(), s.operand_a,
                                   s.operand_a_primary) ||
         !valid_source_attribution(*a.validated->payload, operand_a(), s.operand_a,
                                   s.operand_a_alternate, false) ||
         !valid_source_attribution(*a.validated->payload, operand_b(), s.operand_b,
                                   s.operand_b_primary) ||
         !valid_source_attribution(*a.validated->payload, operand_b(), s.operand_b,
                                   s.operand_b_alternate, false) ||
         !independently_reconstruct_ray(*a.validated->payload, operand_a(), p.value(),
                                        s.operand_a_primary) ||
         !independently_reconstruct_ray(*a.validated->payload, operand_a(), p.value(),
                                        s.operand_a_alternate) ||
          !independently_reconstruct_ray(*a.validated->payload, operand_b(), p.value(),
                                         s.operand_b_primary) ||
          !independently_reconstruct_ray(*a.validated->payload, operand_b(), p.value(),
                                         s.operand_b_alternate) ||
          !independently_check_shell_polarity(*a.validated->payload, operand_a(),
                                               p.value(), s.operand_a_primary) ||
          !independently_check_shell_polarity(*a.validated->payload, operand_a(),
                                               p.value(), s.operand_a_alternate) ||
          !independently_check_shell_polarity(*a.validated->payload, operand_b(),
                                               p.value(), s.operand_b_primary) ||
          !independently_check_shell_polarity(*a.validated->payload, operand_b(),
                                               p.value(), s.operand_b_alternate))
      return false;
  }
  const auto witness = exterior_witness(*a.validated->payload, g);
  std::optional<exterior_attachment_result> attachment;
  if (!g.patches.empty()) attachment = exterior_attachment(g, witness);
  if (!(witness == a.certificate.exterior_witness) ||
      bool(attachment) != bool(a.certificate.exterior_attachment_side) ||
      !a.certificate.exterior_bound_disjoint)
    return false;
  if (attachment &&
      (attachment->side != a.certificate.exterior_attachment_side ||
       attachment->target_patch != a.certificate.exterior_target_patch ||
       !a.certificate.exterior_target_witness ||
       !(attachment->target_witness == *a.certificate.exterior_target_witness) ||
       attachment->first_hits.size() != a.certificate.exterior_first_hits.size()))
    return false;
  if (attachment)
    for (std::size_t i = 0; i < attachment->first_hits.size(); ++i) {
      const auto &x = attachment->first_hits[i];
      const auto &y = a.certificate.exterior_first_hits[i];
      if (x.patch != y.patch || x.parameter != y.parameter ||
          x.relation != y.relation || x.witness_facing_side != y.witness_facing_side)
        return false;
    }
  formal_open_point_view q{witness, {exact_scalar(0), exact_scalar(0), exact_scalar(0)},
                           {perturbation_domain::generic_ray, {}, 0}};
  auto la = locate_formal_open_point(q, ta, a.certificate.exterior_operand_a.direction_index);
  auto lb = locate_formal_open_point(q, tb, a.certificate.exterior_operand_b.direction_index);
  return la.has_value() && lb.has_value() &&
         same_ray(a.certificate.exterior_operand_a, la.value()) &&
         same_ray(a.certificate.exterior_operand_b, lb.value()) &&
         la.value().location == formal_operand_location_kind::outside &&
         lb.value().location == formal_operand_location_kind::outside &&
          a.certificate.exterior_region.value_for_debug() < a.regions.size() &&
           (!attachment ||
            (attachment->side.value_for_debug() < a.side_labels.size() &&
             a.side_labels[attachment->side.value_for_debug()].region ==
                a.certificate.exterior_region)) &&
          a.regions[a.certificate.exterior_region.value_for_debug()].label == occupancy_pair{};
}
template <class T, class I> bool valid(const labeled_arrangement<T, I> &a) {
  if (!a.arrangement || !a.validated || a.owner != a.arrangement->owner ||
      a.arrangement->payload->validated.get() != a.validated.get() ||
      a.arrangement_digest != a.arrangement->artifact_digest ||
      a.arrangement_semantic_digest !=
          a.arrangement->payload->certificate.semantic_digest)
    return false;
  const auto &g = *a.arrangement->payload;
  if (a.regions.size() != g.probes.size() ||
      a.seeds.size() != a.regions.size() ||
      a.propagation.size() != a.regions.size() ||
      a.side_labels.size() != g.patch_sides.size() ||
      a.patch_labels.size() != g.patches.size())
    return false;
  for (std::size_t i = 0; i < a.regions.size(); ++i) {
    const auto &r = a.regions[i];
    if (r.id.value_for_debug() != i || r.seed.value_for_debug() != i ||
        a.seeds[i].source_component != r.source_component ||
        (!g.patch_sides.empty() &&
         (a.seeds[i].source_side.value_for_debug() >= g.patch_sides.size() ||
          g.probes[i].side != a.seeds[i].source_side)))
      return false;
  }
  for (std::size_t i = 0; i < a.propagation.size(); ++i) {
    const auto &p = a.propagation[i];
    if (p.id.value_for_debug() != i ||
        p.region.value_for_debug() >= a.regions.size() ||
        p.root.value_for_debug() >= a.regions.size() ||
        p.propagated != a.regions[p.region.value_for_debug()].label ||
        bool(p.predecessor) != bool(p.transition))
      return false;
    if (p.predecessor && p.predecessor->value_for_debug() >= a.regions.size())
      return false;
    if (p.transition && p.transition->value_for_debug() >= a.transitions.size())
      return false;
  }
  for (std::size_t i = 0; i < a.side_labels.size(); ++i) {
    const auto &s = a.side_labels[i];
    if (s.id.value_for_debug() != i ||
        s.source_side.value_for_debug() >= g.patch_sides.size() ||
        s.region.value_for_debug() >= a.regions.size())
      return false;
    const auto &r = a.regions[s.region.value_for_debug()];
    const auto &direct = a.seeds[r.seed.value_for_debug()];
    const occupancy_pair direct_label{
        direct.operand_a.location == operand_location_kind::inside,
        direct.operand_b.location == operand_location_kind::inside};
    if (direct.source_side != s.source_side || s.occupancy != direct_label ||
        s.occupancy != r.label)
      return false;
  }
  for (std::size_t i = 0; i < a.patch_labels.size(); ++i) {
    const auto &p = a.patch_labels[i];
    if (p.patch.value_for_debug() != i ||
        p.negative.value_for_debug() >= a.side_labels.size() ||
        p.positive.value_for_debug() >= a.side_labels.size())
      return false;
    const auto &n = a.side_labels[p.negative.value_for_debug()];
    const auto &q = a.side_labels[p.positive.value_for_debug()];
    if (n.side != patch_plane_side::negative ||
        q.side != patch_plane_side::positive || n.patch != p.patch ||
        q.patch != p.patch)
      return false;
  }
  for (std::size_t i = 0; i < a.transitions.size(); ++i) {
    const auto &t = a.transitions[i];
    if (t.id.value_for_debug() != i ||
        t.reverse.value_for_debug() >= a.transitions.size() ||
        a.transitions[t.reverse.value_for_debug()].reverse != t.id ||
        t.from.value_for_debug() >= a.regions.size() ||
        t.to.value_for_debug() >= a.regions.size() ||
        t.from_side.value_for_debug() >= g.patch_sides.size() ||
        t.to_side.value_for_debug() >= g.patch_sides.size() ||
        g.patch_sides[t.from_side.value_for_debug()].component !=
            a.regions[t.from.value_for_debug()].source_component ||
        g.patch_sides[t.to_side.value_for_debug()].component !=
            a.regions[t.to.value_for_debug()].source_component ||
        t.before != a.regions[t.from.value_for_debug()].label ||
        t.after != a.regions[t.to.value_for_debug()].label ||
        a.transitions[t.reverse.value_for_debug()].from_side != t.to_side ||
        a.transitions[t.reverse.value_for_debug()].to_side != t.from_side ||
        a.transitions[t.reverse.value_for_debug()].coincidence != t.coincidence)
      return false;
  }
  if (a.arc_checks.size() != a.transitions.size())
    return false;
  std::vector<bool> checked(a.transitions.size(), false);
  for (const auto &x : a.arc_checks) {
    const auto ti = x.transition.value_for_debug();
    if (ti >= a.transitions.size() || checked[ti] ||
        x.from_path.value_for_debug() >= a.propagation.size() ||
        x.to_path.value_for_debug() >= a.propagation.size())
      return false;
    checked[ti] = true;
    const auto &t = a.transitions[ti];
    const auto &from_path = a.propagation[x.from_path.value_for_debug()];
    const auto &to_path = a.propagation[x.to_path.value_for_debug()];
    if (from_path.region != t.from || to_path.region != t.to ||
        from_path.propagated != t.before || x.transferred != t.after ||
        to_path.propagated != x.transferred ||
        x.tree_edge != (to_path.predecessor == std::optional<classification_region_id>(t.from) &&
                        to_path.transition == std::optional<classification_transition_id>(t.id)))
      return false;
  }
  std::size_t expected_cycles = 0;
  for (std::size_t i = 0; i < a.transitions.size(); ++i)
    if (i < a.transitions[i].reverse.value_for_debug() &&
        !a.arc_checks[i].tree_edge &&
        !a.arc_checks[a.transitions[i].reverse.value_for_debug()].tree_edge)
      ++expected_cycles;
  if (a.cycle_checks.size() != expected_cycles)
    return false;
  for (const auto &x : a.cycle_checks) {
    const auto ti = x.closing_transition.value_for_debug();
    if (ti >= a.transitions.size() ||
        x.from_path.value_for_debug() >= a.propagation.size() ||
        x.to_path.value_for_debug() >= a.propagation.size())
      return false;
    const auto &t = a.transitions[ti];
    const auto &fp = a.propagation[x.from_path.value_for_debug()];
    const auto &tp = a.propagation[x.to_path.value_for_debug()];
    if (fp.region != t.from || tp.region != t.to || fp.root != tp.root ||
        x.root != fp.root || x.from_label != fp.propagated ||
        x.to_label != tp.propagated || t.before != x.from_label ||
        t.after != x.to_label)
      return false;
  }
  if (a.certificate.regions != a.regions.size() ||
      a.certificate.seeds != a.seeds.size() ||
      a.certificate.directed_transitions != a.transitions.size() ||
      a.certificate.side_labels != a.side_labels.size())
    return false;
  return verify_geometric_evidence(a) && semantic(a) == a.canonical_bytes &&
         invocation(a) == a.artifact_bytes &&
         artifact_digest_for(a) == a.artifact_digest &&
         a.certificate.semantic_digest ==
             domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '0', '9'}},
                           a.canonical_bytes);
}
template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    const auto &a = *static_cast<const labeled_arrangement<T, I> *>(v.payload);
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::cell_classification;
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
      auto st = ok ? check_status::passed
                   : failed ? check_status::not_run_due_to_prior_failure
                            : check_status::failed;
      r.results.push_back({c, st, {}, 0});
      failed |= st == check_status::failed;
    }
    r.dependency_digests = {a.arrangement_digest, a.validated_digest};
    auto b = encode_verification_report(r);
    if (!b.has_value())
      return b.error();
    r.report_digest =
        domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}}, b.value());
    return r;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::cell_classification,
                      "classification_verifier_exception");
  }
}
template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}
} // namespace

status_or<bool> register_cell_classification_verifier(verifier_registry &r,
                                                      coordinate_tag c,
                                                      index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::labeled_arrangement;
  x.artifact_type_tag = labeled_arrangement_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = labeled_arrangement_schema;
  x.mandatory = {invariant_code::classification_binding,
                 invariant_code::classification_regions,
                 invariant_code::classification_transfers,
                 invariant_code::classification_side_labels,
                 invariant_code::classification_canonical_encoding};
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
status_or<std::shared_ptr<const published_artifact<labeled_arrangement<T, I>>>>
classify_arrangement_cells(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::cell_classification, "cancelled");
    if (auto old = ctx.artifacts().latest(artifact_slot::labeled_arrangement))
      return std::static_pointer_cast<
          const published_artifact<labeled_arrangement<T, I>>>(old);
    auto ar = build_global_arrangement(ctx);
    if (!ar.has_value())
      return ar.error();
    performance_scope producer(ctx.performance_collector_for_internal_use(),
                               boolean_stage::cell_classification,
                               performance_role::producer);
    auto arrangement = ar.value();
    if (ctx.artifacts().latest_generation(artifact_slot::arrangement_complex) !=
        arrangement->generation)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "stale_arrangement");
    const auto &g = *arrangement->payload;
    labeled_arrangement<T, I> a;
    a.owner = ctx.owner();
    a.setup_digest = ctx.replay().setup;
    a.arrangement_digest = arrangement->artifact_digest;
    a.arrangement_semantic_digest = g.certificate.semantic_digest;
    a.validated_digest = g.validated->artifact_digest;
    a.arrangement = arrangement;
    a.validated = g.validated;
    a.constructions = g.constructions;
    a.classification = ctx.options().classification.strategy;
    if (a.classification != classification_strategy::independent_patch_side_v1 ||
        g.classification != a.classification)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "classification_strategy_mismatch");
    const auto triangles_a =
                   operand_triangles(*g.validated->payload, operand_a()),
               triangles_b =
                   operand_triangles(*g.validated->payload, operand_b());
    std::map<open_region_component_id, classification_region_id> region_ids;
    for (std::size_t i = 0; i < g.probes.size(); ++i) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::cell_classification, "cancelled");
      const auto &p = g.probes[i];
      if (region_ids.count(p.component))
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "duplicate_region_probe");
      auto probe = formal_probe(g, p);
      if (!probe.has_value())
        return probe.error();
      auto la = locate_formal_open_point(probe.value(), triangles_a),
           lb = locate_formal_open_point(probe.value(), triangles_b);
      if (!la.has_value())
        return la.error();
      if (!lb.has_value())
        return lb.error();
      auto la2 = locate_formal_open_point(
               probe.value(), triangles_a,
               static_cast<std::uint8_t>(la.value().ray_direction_index + 1)),
           lb2 = locate_formal_open_point(
               probe.value(), triangles_b,
               static_cast<std::uint8_t>(lb.value().ray_direction_index + 1));
      if (!la2.has_value())
        return la2.error();
      if (!lb2.has_value())
        return lb2.error();
      if (la.value().location != la2.value().location ||
          la.value().signed_degree != la2.value().signed_degree ||
          lb.value().location != lb2.value().location ||
          lb.value().signed_degree != lb2.value().signed_degree)
        return make_error(boolean_error_code::internal_invariant_error,
                           boolean_stage::cell_classification,
                           "alternate_formal_ray_disagreement");
      const auto shell_a = ray_record(operand_a(), p.component, la.value());
      const auto shell_b = ray_record(operand_b(), p.component, lb.value());
      if (!independently_check_shell_polarity(*g.validated->payload, operand_a(),
                                               probe.value(), shell_a) ||
          !independently_check_shell_polarity(*g.validated->payload, operand_b(),
                                               probe.value(), shell_b))
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "shell_polarity_point_location_disagreement");
      occupancy_pair label{
          la.value().location == formal_operand_location_kind::inside,
          lb.value().location == formal_operand_location_kind::inside};
      seed_classification_certificate seed;
      seed.id = seed_certificate_id::from_canonical_value(i);
      seed.source_side = p.side;
      seed.source_component = p.component;
      seed.base_kind = p.base_kind;
      seed.base_id = p.base_id;
      seed.operand_a = {label.in_a ? operand_location_kind::inside
                                   : operand_location_kind::outside,
                        la.value().signed_degree,
                         boundary_sources(la.value()),
                        location_digest(operand_a(), p.component, la.value())};
      seed.operand_b = {label.in_b ? operand_location_kind::inside
                                   : operand_location_kind::outside,
                        lb.value().signed_degree,
                         boundary_sources(lb.value()),
                         location_digest(operand_b(), p.component, lb.value())};
      seed.operand_a_primary = ray_record(operand_a(), p.component, la.value());
      seed.operand_a_alternate = ray_record(operand_a(), p.component, la2.value());
      seed.operand_b_primary = ray_record(operand_b(), p.component, lb.value());
      seed.operand_b_alternate = ray_record(operand_b(), p.component, lb2.value());
      a.seeds.push_back(seed);
      classification_region r;
      r.id = classification_region_id::from_canonical_value(i);
      r.source_component = p.component;
      r.label = label;
      r.seed = seed.id;
      region_ids.emplace(p.component, r.id);
      a.regions.push_back(std::move(r));
    }
    for (const auto &s : g.patch_sides) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::cell_classification, "cancelled");
      auto found = region_ids.find(s.component);
      if (found == region_ids.end())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification, "missing_region");
      auto rid = found->second;
      a.regions[rid.value_for_debug()].patch_sides.push_back(s.id);
      patch_side_label x;
      x.id = patch_side_label_id::from_canonical_value(a.side_labels.size());
      x.source_side = s.id;
      x.patch = s.patch;
      x.side = s.side;
      x.region = rid;
      x.occupancy = a.regions[rid.value_for_debug()].label;
      a.side_labels.push_back(x);
    }
    for (const auto &p : g.patches) {
      const patch_side_label *n = nullptr, *q = nullptr;
      for (const auto &s : a.side_labels)
        if (s.patch == p.id)
          (s.side == patch_plane_side::negative ? n : q) = &s;
      if (!n || !q)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "missing_patch_sides");
      a.patch_labels.push_back(
          {p.id, n->id, q->id, n->occupancy, q->occupancy});
    }
    for (const auto &t : g.transitions) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::cell_classification, "cancelled");
      if (t.from.value_for_debug() >= a.side_labels.size() ||
          t.to.value_for_debug() >= a.side_labels.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "transition_endpoint");
      const auto &from = a.side_labels[t.from.value_for_debug()];
      const auto &to = a.side_labels[t.to.value_for_debug()];
      if (!t.region_crossing) {
        if (from.region != to.region || from.occupancy != to.occupancy)
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::cell_classification,
                            "preserving_transition_crosses_region");
        continue;
      }
      if (from.region == to.region || t.uses.empty())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "invalid_crossing");
      std::optional<patch_plane_side> side_a, side_b;
      for (auto uid : t.uses) {
        if (uid.value_for_debug() >= g.sheet_uses.size())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::cell_classification, "crossing_use");
        const auto &u = g.sheet_uses[uid.value_for_debug()];
        if (u.patch != from.patch || u.patch != to.patch)
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::cell_classification,
                            "crossing_use_patch");
        auto &side = u.operand == operand_a() ? side_a : side_b;
        if (side && *side != u.occupied_side)
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::cell_classification,
                            "contradictory_crossing_members");
        side = u.occupied_side;
      }
      if (!side_a && !side_b)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "crossing_without_operand");
      auto expected = [&](patch_plane_side endpoint,
                          const patch_side_label &actual) {
        occupancy_pair x = actual.occupancy;
        if (side_a)
          x.in_a = *side_a == endpoint;
        if (side_b)
          x.in_b = *side_b == endpoint;
        return x;
      };
      const auto before = expected(from.side, from),
                 after = expected(to.side, to);
      if ((!side_a && before.in_a != after.in_a) ||
          (!side_b && before.in_b != after.in_b) || before != from.occupancy ||
          after != to.occupancy)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "crossing_transfer_conflict");
      classification_transfer_kind kind =
          side_a && side_b ? classification_transfer_kind::both
                           : side_a ? classification_transfer_kind::operand_a
                                    : classification_transfer_kind::operand_b;
      auto first = classification_transition_id::from_canonical_value(
               a.transitions.size()),
           second = classification_transition_id::from_canonical_value(
               a.transitions.size() + 1);
      auto crossing = t.kind == side_transition_kind::coincidence_crossing
                          ? classification_crossing_kind::coincident
                          : classification_crossing_kind::individual;
      a.transitions.push_back({first, second, t.id, from.region, to.region,
                               t.from, t.to, crossing, kind, before, after,
                               t.uses, t.coincidence});
      a.transitions.push_back({second, first, t.id, to.region, from.region,
                               t.to, t.from, crossing, kind, after, before,
                               t.uses, t.coincidence});
    }
    std::vector<std::vector<classification_transition_id>> adjacency(a.regions.size());
    for (const auto &t : a.transitions)
      adjacency[t.from.value_for_debug()].push_back(t.id);
    for (auto &out : adjacency)
      std::sort(out.begin(), out.end(), [&](auto x, auto y) {
        const auto &lhs = a.transitions[x.value_for_debug()];
        const auto &rhs = a.transitions[y.value_for_debug()];
        if (lhs.to != rhs.to)
          return lhs.to < rhs.to;
        if (lhs.source != rhs.source)
          return lhs.source < rhs.source;
        return lhs.id < rhs.id;
      });
    std::vector<bool> visited(a.regions.size(), false);
    std::vector<std::optional<classification_propagation_record>> records(
        a.regions.size());
    for (std::size_t root = 0; root < a.regions.size(); ++root) {
      if (visited[root])
        continue;
      std::deque<classification_region_id> queue;
      auto root_id = classification_region_id::from_canonical_value(root);
      visited[root] = true;
      records[root] = classification_propagation_record{
          {},           root_id,      root_id,
          std::nullopt, std::nullopt, a.regions[root].label};
      queue.push_back(root_id);
      while (!queue.empty()) {
        if (ctx.cancelled())
          return make_error(boolean_error_code::resource_limit,
                            boolean_stage::cell_classification, "cancelled");
        auto current = queue.front();
        queue.pop_front();
        for (auto tid : adjacency[current.value_for_debug()]) {
          const auto &t = a.transitions[tid.value_for_debug()];
          if (t.before != a.regions[current.value_for_debug()].label ||
              t.after != a.regions[t.to.value_for_debug()].label) {
            auto error = make_error(boolean_error_code::internal_invariant_error,
                                    boolean_stage::cell_classification,
                                    "propagation_conflict");
            std::vector<classification_region_id> established_regions;
            auto cursor = current;
            while (true) {
              established_regions.push_back(cursor);
              const auto &record = records[cursor.value_for_debug()];
              if (!record || !record->predecessor)
                break;
              cursor = *record->predecessor;
            }
            std::reverse(established_regions.begin(), established_regions.end());
            classification_conflict_certificate conflict;
            conflict.kind = classification_conflict_kind::seed_transfer;
            conflict.established.root = established_regions.front();
            conflict.established.terminal = t.to;
            const auto root_seed = a.regions[established_regions.front().value_for_debug()].seed;
            const auto root_label = a.regions[established_regions.front().value_for_debug()].label;
            conflict.established.steps.push_back({
                classification_path_step_kind::root_seed,
                established_regions.front(), established_regions.front(),
                std::nullopt, root_seed, root_label, root_label});
            for (std::size_t i = 1; i < established_regions.size(); ++i) {
              const auto destination = established_regions[i];
              const auto &record = *records[destination.value_for_debug()];
              const auto &edge = a.transitions[record.transition->value_for_debug()];
              conflict.established.steps.push_back({
                  classification_path_step_kind::directed_transfer, edge.from,
                  edge.to, edge.id, a.regions[edge.to.value_for_debug()].seed,
                  edge.before, edge.after});
            }
            conflict.established.steps.push_back({
                classification_path_step_kind::directed_transfer, t.from, t.to,
                t.id, a.regions[t.to.value_for_debug()].seed, t.before, t.after});
            conflict.established.terminal_label = t.after;
            const auto direct_label = a.regions[t.to.value_for_debug()].label;
            conflict.competing.root = t.to;
            conflict.competing.terminal = t.to;
            conflict.competing.steps.push_back({
                classification_path_step_kind::direct_seed, t.to, t.to,
                std::nullopt, a.regions[t.to.value_for_debug()].seed,
                direct_label, direct_label});
            conflict.competing.terminal_label = direct_label;
            error.replay_payload = conflict_bytes(conflict);
            conflict.replay_digest = domain_digest(
                {{'Y', 'G', 'B', 'C', 'O', 'N', '0', '9'}},
                error.replay_payload);
            error.detail = "conflict_digest=" +
                           conflict.replay_digest.hex() +
                           ";established_steps=" +
                           std::to_string(conflict.established.steps.size()) +
                           ";competing_steps=" +
                           std::to_string(conflict.competing.steps.size());
            return error;
          }
          const auto next = t.to.value_for_debug();
          if (!visited[next]) {
            visited[next] = true;
            records[next] = classification_propagation_record{
                {}, t.to, root_id, current, t.id, t.after};
            queue.push_back(t.to);
          }
        }
      }
    }
    for (std::size_t i = 0; i < records.size(); ++i) {
      if (!records[i])
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "missing_propagation");
      records[i]->id =
          propagation_path_id::from_canonical_value(a.propagation.size());
      a.propagation.push_back(*records[i]);
    }
    for (const auto &t : a.transitions) {
      const auto &from_path = a.propagation[t.from.value_for_debug()];
      const auto &to_path = a.propagation[t.to.value_for_debug()];
      if (from_path.propagated != t.before || to_path.propagated != t.after)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "propagation_transfer_application");
      const bool tree = to_path.predecessor ==
                            std::optional<classification_region_id>(t.from) &&
                        to_path.transition ==
                            std::optional<classification_transition_id>(t.id);
      a.arc_checks.push_back({t.id, from_path.id, to_path.id, tree, t.after});
    }
    for (const auto &t : a.transitions) {
      if (t.reverse < t.id || a.arc_checks[t.id.value_for_debug()].tree_edge ||
          a.arc_checks[t.reverse.value_for_debug()].tree_edge)
        continue;
      const auto &from_path = a.propagation[t.from.value_for_debug()];
      const auto &to_path = a.propagation[t.to.value_for_debug()];
      if (from_path.root != to_path.root)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "cycle_crosses_propagation_component");
      a.cycle_checks.push_back({t.id, from_path.id, to_path.id, from_path.root,
                                from_path.propagated, to_path.propagated});
    }
    a.certificate.regions = a.regions.size();
    a.certificate.seeds = a.seeds.size();
    a.certificate.directed_transitions = a.transitions.size();
    a.certificate.side_labels = a.side_labels.size();
    if (a.regions.empty())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "missing_universe_region");
    a.certificate.exterior_witness =
        exterior_witness(*a.validated->payload, g);
    if (!g.patches.empty()) {
      const auto attachment = exterior_attachment(g, a.certificate.exterior_witness);
      a.certificate.exterior_attachment_side = attachment.side;
      a.certificate.exterior_target_patch = attachment.target_patch;
      a.certificate.exterior_target_witness = attachment.target_witness;
      a.certificate.exterior_first_hits = attachment.first_hits;
    }
    if (a.certificate.exterior_attachment_side) {
      const auto side = a.certificate.exterior_attachment_side->value_for_debug();
      if (side >= a.side_labels.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "exterior_attachment_range");
      a.certificate.exterior_region = a.side_labels[side].region;
    } else {
      if (a.regions.size() != 1 ||
          g.probes.front().base_kind != probe_base_stratum_kind::universe)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::cell_classification,
                          "missing_exterior_attachment");
      a.certificate.exterior_region = a.regions.front().id;
    }
    if (a.regions[a.certificate.exterior_region.value_for_debug()].label !=
        occupancy_pair{})
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "attached_exterior_region_not_outside");
    formal_open_point_view exterior_probe{
        a.certificate.exterior_witness,
        {exact_scalar(0), exact_scalar(0), exact_scalar(0)},
        {perturbation_domain::generic_ray, {}, 0}};
    auto exterior_a = locate_formal_open_point(exterior_probe, triangles_a);
    auto exterior_b = locate_formal_open_point(exterior_probe, triangles_b);
    if (!exterior_a.has_value()) return exterior_a.error();
    if (!exterior_b.has_value()) return exterior_b.error();
    if (exterior_a.value().location != formal_operand_location_kind::outside ||
        exterior_b.value().location != formal_operand_location_kind::outside)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "exterior_witness_not_outside");
    const auto exterior_component =
        a.regions[a.certificate.exterior_region.value_for_debug()].source_component;
    a.certificate.exterior_operand_a =
        ray_record(operand_a(), exterior_component, exterior_a.value());
    a.certificate.exterior_operand_b =
        ray_record(operand_b(), exterior_component, exterior_b.value());
    a.certificate.exterior_bound_disjoint = true;
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::cell_classification, "cancelled");
    a.canonical_bytes = semantic(a);
    a.certificate.semantic_digest = domain_digest(
        {{'Y', 'G', 'B', 'C', 'A', 'N', '0', '9'}}, a.canonical_bytes);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    std::vector<resource_reservation> charges;
    auto reserve = [&](resource_kind k, std::uint64_t n) -> status_or<bool> {
      auto q = ctx.accountant().reserve_scoped(
          k, n, boolean_stage::cell_classification);
      if (!q.has_value())
        return q.error();
      charges.push_back(std::move(q.value()));
      return true;
    };
    for (auto q :
         {std::make_pair(resource_kind::cells, std::uint64_t(a.regions.size())),
          std::make_pair(resource_kind::seed_certificates,
                         std::uint64_t(a.seeds.size())),
          std::make_pair(resource_kind::classification_transitions,
                         std::uint64_t(a.transitions.size())),
          std::make_pair(resource_kind::patch_side_labels,
                         std::uint64_t(a.side_labels.size())),
          std::make_pair(resource_kind::propagation_records,
                         std::uint64_t(a.propagation.size()))}) {
      auto ok = reserve(q.first, q.second);
      if (!ok.has_value())
        return ok.error();
    }
    auto ptr = std::make_shared<const labeled_arrangement<T, I>>(std::move(a));
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::labeled_arrangement, type_tag<T, I>(),
        labeled_arrangement_schema, ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    artifact_view view{ctx.owner(),
                       artifact_slot::labeled_arrangement,
                       type_tag<T, I>(),
                       labeled_arrangement_schema,
                       1,
                       ptr->artifact_digest,
                       ptr,
                       ptr.get()};
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
    stage_transaction<labeled_arrangement<T, I>, labeled_arrangement<T, I>> tx(
        ctx.owner(), boolean_stage::cell_classification,
        artifact_slot::labeled_arrangement,
        std::make_unique<labeled_arrangement<T, I>>(),
        ctx.performance_collector_for_internal_use());
    for (auto &charge : charges)
      tx.stage_reservation(std::move(charge));
    performance_count(performance_counter::classification_source_facets,
                      ptr->validated->payload->facets.size());
    performance_count(performance_counter::classification_probes,
                      ptr->seeds.size());
    performance_count(performance_counter::alternate_rays,
                      ptr->seeds.size() * 2);
    producer.finish();
    auto ok = tx.verify(ptr, view, spec.value(), env, ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::cell_classification, "cancelled");
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::cell_classification,
                      "classification_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::cell_classification,
                        "classification_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T, I)                                                             \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<labeled_arrangement<T, I>>>>    \
  classify_arrangement_cells(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST
} // namespace mesh_boolean
} // namespace ygor
