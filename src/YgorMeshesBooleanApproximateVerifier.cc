#include "YgorMeshesBooleanApproximate.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

product_status_or<bool> reject(const char *key) {
  return make_product_error(product_error_code::verifier_disagreement, key);
}

product_status_or<bool> resource(const char *key) {
  return make_product_error(product_error_code::resource_limit, key);
}

template <class T> exact_scalar dyadic(T value) {
  auto decoded = decode_coordinate(value, boolean_stage::final_verification);
  if (!decoded.has_value())
    throw std::invalid_argument("nonfinite verifier value");
  return decoded.value().value;
}

exact_scalar as_scalar(const big_int &value) {
  return exact_scalar(value, big_uint(1));
}

exact_scalar evaluate_plane(const exact_plane3 &plane,
                            const exact_point3 &point) {
  return as_scalar(plane.a) * point.x + as_scalar(plane.b) * point.y +
         as_scalar(plane.c) * point.z + as_scalar(plane.d);
}

exact_scalar normal_squared(const exact_plane3 &plane) {
  return as_scalar(plane.a).pow(2) + as_scalar(plane.b).pow(2) +
         as_scalar(plane.c).pow(2);
}

exact_scalar replay_relation_residual(
    const exact_defining_relation_record &relation, const exact_point3 &point) {
  return relation.coefficients[0] * point.x +
         relation.coefficients[1] * point.y +
         relation.coefficients[2] * point.z + relation.coefficients[3];
}

exact_scalar replay_relation_normal_squared(
    const exact_defining_relation_record &relation) {
  return relation.coefficients[0].pow(2) +
         relation.coefficients[1].pow(2) +
         relation.coefficients[2].pow(2);
}

template <class T>
exact_point3 replay_point(const std::array<coordinate_bits<T>, 3> &bits) {
  return {dyadic(value_of_bits(bits[0])), dyadic(value_of_bits(bits[1])),
          dyadic(value_of_bits(bits[2]))};
}

template <class T, class I>
std::vector<std::uint8_t> replay_output_bytes(
    const boolean_success<T, I> &output) {
  canonical_encoder encoder;
  encoder.u64(output.mesh.vertices.size());
  for (const auto &point : output.mesh.vertices) {
    encoder.floating(point.x);
    encoder.floating(point.y);
    encoder.floating(point.z);
  }
  encoder.u64(output.mesh.faces.size());
  for (const auto &face : output.mesh.faces) {
    encoder.u64(face.size());
    for (const auto index : face)
      encoder.u64(static_cast<std::uint64_t>(index));
  }
  return encoder.bytes();
}

void replay_encode_point(canonical_encoder &encoder,
                         const exact_point3 &point) {
  encode(encoder, point.x);
  encode(encoder, point.y);
  encode(encoder, point.z);
}

template <class T, class I>
std::vector<std::uint8_t> replay_certificate_bytes(
    const certified_approximate_certificate<T, I> &certificate) {
  canonical_encoder e;
  e.u16(certificate.schema);
  e.byte(static_cast<std::uint8_t>(certificate.coordinate));
  e.byte(static_cast<std::uint8_t>(certificate.index));
  for (const auto *d : {&certificate.exact_result_digest,
                        &certificate.selected_boundary_digest,
                        &certificate.policy_digest})
    e.raw(d->bytes.data(), 16);
  e.u64(certificate.exact_vertex_occurrences);
  e.u64(certificate.exact_edges);
  e.u64(certificate.exact_halfedges);
  e.u64(certificate.exact_cycles);
  e.u64(certificate.exact_patches);
  e.u64(certificate.vertices.size());
  for (const auto &v : certificate.vertices) {
    e.u64(v.selected.value_for_debug());
    replay_encode_point(e, v.exact_target);
    for (const auto bits : v.output_bits)
      e.u64(bits.bits);
    replay_encode_point(e, v.displacement);
    encode(e, v.squared_displacement);
    for (const auto rank : v.accepted_axis_ranks)
      e.u32(rank);
    e.u64(v.accepted_candidate_rank);
    e.boolean(v.original_vertex);
  }
  e.u64(certificate.triangles.size());
  for (const auto &t : certificate.triangles) {
    e.u64(t.triangle.value_for_debug());
    e.u64(t.patch.value_for_debug());
    for (const auto vertex : t.vertices)
      e.u64(vertex.value_for_debug());
    e.byte(static_cast<std::uint8_t>(t.projection));
    e.byte(static_cast<std::uint8_t>(t.exact_orientation));
    for (const auto &deviation : t.support_plane_deviations)
      encode(e, deviation);
    for (const auto &deviation :
         t.normalized_squared_support_plane_deviations)
      encode(e, deviation);
  }
  e.u64(certificate.occurrence_maps.size());
  for (const auto &o : certificate.occurrence_maps) {
    e.u64(o.occurrence.value_for_debug());
    e.u64(o.output_vertex.value_for_debug());
    e.u64(o.exact_cyclic_halfedges.size());
    for (const auto halfedge : o.exact_cyclic_halfedges)
      e.u64(halfedge.value_for_debug());
    e.u64(o.output_cyclic_triangles.size());
    for (const auto triangle : o.output_cyclic_triangles)
      e.u64(triangle.value_for_debug());
  }
  e.u64(certificate.patch_adjacency.size());
  for (const auto &a : certificate.patch_adjacency) {
    e.u64(a.edge.value_for_debug());
    e.u64(a.exact_radial_patches.size());
    for (const auto patch : a.exact_radial_patches)
      e.u64(patch.value_for_debug());
    e.u64(a.output_incident_triangles.size());
    for (const auto triangle : a.output_incident_triangles)
      e.u64(triangle.value_for_debug());
  }
  e.u64(certificate.obligations.size());
  for (const auto &o : certificate.obligations) {
    e.byte(static_cast<std::uint8_t>(o.kind));
    for (const auto *values : {&o.vertices, &o.triangles, &o.exact_entities}) {
      e.u64(values->size());
      for (const auto value : *values)
        e.u64(value);
    }
    e.boolean(o.passed);
    encode(e, o.measured_value);
    encode(e, o.allowed_value);
  }
  e.u64(certificate.relaxed_relations.size());
  for (const auto &r : certificate.relaxed_relations) {
    e.byte(static_cast<std::uint8_t>(r.kind));
    e.u64(r.exact_entity);
    e.byte(r.axis);
    encode(e, r.exact_value);
    encode(e, r.emitted_value);
    encode(e, r.absolute_deviation);
    encode(e, r.allowed_deviation);
    e.boolean(r.defining_relation.has_value());
    if (r.defining_relation)
      e.u64(r.defining_relation->value_for_debug());
    e.byte(static_cast<std::uint8_t>(r.relation_kind));
    e.u16(r.relation_formula_version);
    e.byte(static_cast<std::uint8_t>(r.relation_expected));
    encode(e, r.exact_residual);
    encode(e, r.emitted_residual);
  }
  e.u64(certificate.defining_relations.size());
  for (const auto &r : certificate.defining_relations) {
    e.u64(r.vertex.value_for_debug());
    e.u64(r.relation.value_for_debug());
    e.byte(static_cast<std::uint8_t>(r.kind));
    e.u16(r.formula_version);
    e.byte(static_cast<std::uint8_t>(r.expected));
    encode(e, r.exact_residual);
    encode(e, r.emitted_residual);
    encode(e, r.normalized_squared_residual_change);
    encode(e, r.allowed_squared_residual_change);
    e.boolean(r.exact);
    e.boolean(r.passed);
  }
  e.u64(certificate.components.size());
  for (const auto &c : certificate.components) {
    e.u64(c.id);
    for (const auto *values : {&c.variables, &c.variable_order,
                                &c.obligations,
                                &c.accepted_ranks,
                               &c.rejected_prefix_witnesses}) {
      e.u64(values->size());
      for (const auto value : *values)
        e.u64(value);
    }
    e.u64(c.visited_nodes);
    e.u64(c.complete_assignments);
    e.raw(c.graph_digest.bytes.data(), 16);
    e.raw(c.transcript_digest.bytes.data(), 16);
  }
  const auto &s = certificate.search;
  e.u16(s.candidate_generation_version);
  e.u32(s.candidate_ulp_radius);
  e.u64(s.candidate_cap);
  e.u64(s.candidate_evaluation_limit);
  e.u64(s.candidate_evaluations);
  e.u64(s.search_node_limit);
  e.u64(s.obligation_limit);
  e.u64(s.triangle_pair_limit);
  e.u64(s.predicate_check_limit);
  e.u64(s.predicate_checks);
  e.u64(s.verifier_work_limit);
  e.u64(s.verifier_record_limit);
  e.u64(s.verifier_byte_limit);
  e.u64(s.generated_axis_candidates);
  e.u64(s.generated_point_candidates);
  e.u64(s.visited_nodes);
  e.u64(s.complete_assignments);
  e.u64(s.accepted_candidate_ranks.size());
  for (const auto rank : s.accepted_candidate_ranks)
    e.u64(rank);
  e.raw(s.candidate_domain_digest.bytes.data(), 16);
  e.raw(s.transcript_digest.bytes.data(), 16);
  encode(e, certificate.maximum_squared_vertex_displacement);
  for (const auto &value : certificate.maximum_axis_displacements)
    encode(e, value);
  encode(e, certificate.maximum_support_plane_deviation);
  encode(e, certificate.maximum_normalized_squared_support_plane_deviation);
  return e.bytes();
}

template <class T> struct replay_axis_candidate {
  coordinate_bits<T> bits;
  exact_scalar value;
  exact_scalar error;
  std::uint32_t distance = 0;
  std::uint32_t rank = 0;
};

template <class T> struct replay_point_candidate {
  std::array<coordinate_bits<T>, 3> bits;
  std::array<std::uint32_t, 3> axis_ranks{{0, 0, 0}};
  exact_point3 point;
  exact_scalar squared_error;
  std::uint32_t max_distance = 0;
  std::uint64_t sum_distance = 0;
};

template <class T>
std::vector<replay_axis_candidate<T>> replay_axis(
    const exact_scalar &target, std::uint32_t radius,
    approximate_verifier_budget &budget) {
  auto nearest = round_binary_nearest_even<T>(target);
  if (!nearest)
    return {};
  std::vector<replay_axis_candidate<T>> result;
  auto append = [&](coordinate_bits<T> bits, std::uint32_t distance) {
    if (!budget.work(1, "approximate_verifier.work_limit") ||
        !budget.records(1, "approximate_verifier.record_limit") ||
        !budget.bytes(sizeof(replay_axis_candidate<T>),
                      "approximate_verifier.byte_limit"))
      return;
    auto decoded = decode_coordinate(bits, boolean_stage::final_verification);
    if (decoded.has_value()) {
      replay_axis_candidate<T> candidate;
      candidate.bits = bits;
      candidate.value = decoded.value().value;
      candidate.error = (decoded.value().value - target).abs();
      candidate.distance = distance;
      result.push_back(std::move(candidate));
    }
  };
  append(*nearest, 0);
  auto predecessor = *nearest, successor = *nearest;
  for (std::uint32_t step = 1; step != 0; ++step) {
    if (budget.failed())
      return {};
    if (auto next = predecessor_bits<T>(predecessor)) {
      predecessor = *next;
      append(predecessor, step);
    }
    if (auto next = successor_bits<T>(successor)) {
      successor = *next;
      append(successor, step);
    }
    if (step == radius)
      break;
  }
  std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
    return std::tie(a.distance, a.error, a.value, a.bits.bits) <
           std::tie(b.distance, b.error, b.value, b.bits.bits);
  });
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i].rank = static_cast<std::uint32_t>(i);
  return result;
}

template <class T>
std::vector<replay_point_candidate<T>> replay_domain(
    const exact_point3 &target, bool fixed, std::uint32_t radius,
    std::uint64_t cap, const approximation_policy_contract &policy,
    std::uint64_t &axis_total, std::uint64_t &candidate_evaluations,
    std::uint64_t candidate_evaluation_limit, bool &limited,
    approximate_verifier_budget &budget) {
  const std::array<exact_scalar, 3> target_axis{{target.x, target.y, target.z}};
  std::array<std::vector<replay_axis_candidate<T>>, 3> axes;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    axes[axis] = replay_axis<T>(target_axis[axis], fixed ? 0 : radius, budget);
    if (budget.failed()) {
      limited = true;
      return {};
    }
    if (axes[axis].size() >
        std::numeric_limits<std::uint64_t>::max() - axis_total) {
      limited = true;
      return {};
    }
    axis_total += axes[axis].size();
  }
  std::vector<replay_point_candidate<T>> result;
  const auto global = dyadic(policy.max_vertex_displacement);
  const std::array<bool, 3> present{{policy.has_max_axis_displacement_x,
                                     policy.has_max_axis_displacement_y,
                                     policy.has_max_axis_displacement_z}};
  const std::array<double, 3> bounds{{policy.max_axis_displacement_x,
                                      policy.max_axis_displacement_y,
                                      policy.max_axis_displacement_z}};
  for (const auto &x : axes[0])
    for (const auto &y : axes[1])
      for (const auto &z : axes[2]) {
        if (!budget.work(1, "approximate_verifier.work_limit")) {
          limited = true;
          return result;
        }
        if (candidate_evaluations == candidate_evaluation_limit ||
            candidate_evaluations ==
                std::numeric_limits<std::uint64_t>::max()) {
          limited = true;
          return result;
        }
        ++candidate_evaluations;
        replay_point_candidate<T> candidate;
        candidate.bits = {{x.bits, y.bits, z.bits}};
        candidate.axis_ranks = {{x.rank, y.rank, z.rank}};
        candidate.point = {x.value, y.value, z.value};
        const std::array<exact_scalar, 3> delta{
            {(x.value - target.x).abs(), (y.value - target.y).abs(),
             (z.value - target.z).abs()}};
        candidate.squared_error = delta[0].pow(2) + delta[1].pow(2) +
                                  delta[2].pow(2);
        candidate.max_distance =
            std::max({x.distance, y.distance, z.distance});
        candidate.sum_distance = std::uint64_t(x.distance) + y.distance +
                                 z.distance;
        if (!budget.predicate("approximate_verifier.predicate_limit")) {
          limited = true;
          return result;
        }
        if (global.pow(2) < candidate.squared_error)
          continue;
        bool accepted = true;
        if (fixed) {
          if (!budget.predicate("approximate_verifier.predicate_limit")) {
            limited = true;
            return result;
          }
          accepted = candidate.point == target;
        }
        for (std::size_t axis = 0; axis < 3; ++axis)
          if (present[axis] &&
              (!budget.predicate("approximate_verifier.predicate_limit") ||
               dyadic(bounds[axis]) < delta[axis]))
            accepted = false;
        if (accepted) {
          if (!budget.records(1, "approximate_verifier.record_limit")) {
            limited = true;
            return result;
          }
          if (!budget.bytes(sizeof(replay_point_candidate<T>),
                            "approximate_verifier.byte_limit")) {
            limited = true;
            return result;
          }
          result.push_back(std::move(candidate));
          if (result.size() > cap) {
            const auto worst = std::max_element(
                result.begin(), result.end(), [](const auto &a, const auto &b) {
                  if (a.max_distance != b.max_distance)
                    return a.max_distance < b.max_distance;
                  if (a.sum_distance != b.sum_distance)
                    return a.sum_distance < b.sum_distance;
                  if (a.squared_error != b.squared_error)
                    return a.squared_error < b.squared_error;
                  if (a.axis_ranks != b.axis_ranks)
                    return a.axis_ranks < b.axis_ranks;
                  return std::make_tuple(a.bits[0].bits, a.bits[1].bits,
                                         a.bits[2].bits) <
                         std::make_tuple(b.bits[0].bits, b.bits[1].bits,
                                         b.bits[2].bits);
                });
            result.erase(worst);
          }
        }
      }
  std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
    if (a.max_distance != b.max_distance)
      return a.max_distance < b.max_distance;
    if (a.sum_distance != b.sum_distance)
      return a.sum_distance < b.sum_distance;
    if (a.squared_error != b.squared_error)
      return a.squared_error < b.squared_error;
    if (a.axis_ranks != b.axis_ranks)
      return a.axis_ranks < b.axis_ranks;
    return std::make_tuple(a.bits[0].bits, a.bits[1].bits, a.bits[2].bits) <
           std::make_tuple(b.bits[0].bits, b.bits[1].bits, b.bits[2].bits);
  });
  return result;
}

template <class T, class I>
bool verify_vertex_links(const boolean_success<T, I> &output,
                         approximate_verifier_budget &budget) {
  std::vector<std::vector<std::pair<std::uint64_t, std::uint64_t>>> links(
      output.mesh.vertices.size());
  for (const auto &face : output.mesh.faces) {
    if (!budget.work(1, "approximate_verifier.work_limit"))
      return false;
    if (face.size() != 3)
      return false;
    for (std::size_t i = 0; i < 3; ++i) {
      const auto center = static_cast<std::uint64_t>(face[i]);
      if (center >= links.size())
        return false;
      links[center].push_back(
          {static_cast<std::uint64_t>(face[(i + 1) % 3]),
           static_cast<std::uint64_t>(face[(i + 2) % 3])});
    }
  }
  for (const auto &link : links) {
    if (!budget.work(1, "approximate_verifier.work_limit"))
      return false;
    if (link.empty())
      return false;
    std::map<std::uint64_t, std::vector<std::uint64_t>> adjacency;
    for (const auto &edge : link) {
      adjacency[edge.first].push_back(edge.second);
      adjacency[edge.second].push_back(edge.first);
    }
    for (const auto &entry : adjacency)
      if (entry.second.size() != 2)
        return false;
    std::set<std::uint64_t> visited;
    std::vector<std::uint64_t> pending{adjacency.begin()->first};
    while (!pending.empty()) {
      const auto current = pending.back();
      pending.pop_back();
      if (!visited.insert(current).second)
        continue;
      for (const auto next : adjacency[current])
        pending.push_back(next);
    }
    if (visited.size() != adjacency.size())
      return false;
  }
  return true;
}

template <class I>
std::uint64_t replay_connected_components(
    const std::vector<std::vector<I>> &faces,
    approximate_verifier_budget &budget) {
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::vector<std::size_t>> uses;
  for (std::size_t face = 0; face < faces.size(); ++face)
    for (std::size_t edge = 0; edge < faces[face].size(); ++edge) {
      if (!budget.work(1, "approximate_verifier.work_limit"))
        return 0;
      const auto a = static_cast<std::uint64_t>(faces[face][edge]);
      const auto b = static_cast<std::uint64_t>(
          faces[face][(edge + 1) % faces[face].size()]);
      uses[std::minmax(a, b)].push_back(face);
    }
  std::vector<std::vector<std::size_t>> adjacency(faces.size());
  for (const auto &entry : uses)
    for (const auto a : entry.second)
      for (const auto b : entry.second)
        if (a != b)
          adjacency[a].push_back(b);
  std::vector<bool> visited(faces.size());
  std::uint64_t components = 0;
  for (std::size_t root = 0; root < faces.size(); ++root) {
    if (!budget.work(1, "approximate_verifier.work_limit"))
      return 0;
    if (visited[root])
      continue;
    ++components;
    std::vector<std::size_t> pending{root};
    while (!pending.empty()) {
      const auto current = pending.back();
      pending.pop_back();
      if (visited[current])
        continue;
      visited[current] = true;
      pending.insert(pending.end(), adjacency[current].begin(),
                     adjacency[current].end());
    }
  }
  return components;
}

} // namespace

template <class T, class I>
product_status_or<bool> detail::verify_certified_approximate_embedding_with_budget(
    const exact_result_handle &exact, const product_realization_policy &policy,
    const boolean_success<T, I> &output,
    const certified_approximate_certificate<T, I> &certificate,
    approximate_verifier_budget &budget) noexcept {
  try {
    boolean_product_options options;
    options.result.representation =
        result_representation::certified_approximate_mesh;
    options.realization = policy;
    if (!validate_product_options(options).has_value())
      return reject("approximate_verifier.policy");
    auto boundary_result = read_exact_result(exact);
    if (!boundary_result.has_value())
      return boundary_result.error();
    const auto &boundary = *boundary_result.value();
    if (!budget.bytes(exact->canonical_bytes.size(),
                      "approximate_verifier.byte_limit") ||
        !budget.bytes(certificate.canonical_bytes.size(),
                      "approximate_verifier.byte_limit"))
      return resource("approximate_verifier.byte_limit");
    const auto records_before_binding = budget.verifier_records;
    auto add_records = [&](std::uint64_t count) {
      return budget.records(count, "approximate_verifier.record_limit");
    };
    if (!add_records(boundary.vertices.size()) ||
        !add_records(boundary.decisions.size()) ||
        !add_records(boundary.side_labels.size()) ||
        !add_records(boundary.edges.size()) ||
        !add_records(boundary.edge_geometry.size()) ||
        !add_records(boundary.halfedges.size()) ||
        !add_records(boundary.cycles.size()) ||
        !add_records(boundary.patches.size()) ||
        !add_records(boundary.patch_geometry.size()) ||
        !add_records(boundary.topology_obstructions.size()) ||
        !add_records(boundary.curves.size()) ||
        !add_records(boundary.vertex_occurrences.size()) ||
        !add_records(boundary.constructions.size()) ||
        !add_records(boundary.defining_relations.size()) ||
        !add_records(boundary.provenance.size()) ||
        !add_records(boundary.source_vertex_occurrences.size()) ||
        !add_records(boundary.link_rays.size()) ||
        !add_records(boundary.link_arcs.size()) ||
        !add_records(boundary.vertex_sectors.size()) ||
        !add_records(output.mesh.vertices.size()) ||
        !add_records(output.mesh.faces.size()) ||
        !add_records(certificate.vertices.size()) ||
        !add_records(certificate.triangles.size()) ||
        !add_records(certificate.occurrence_maps.size()) ||
        !add_records(certificate.patch_adjacency.size()) ||
        !add_records(certificate.obligations.size()) ||
        !add_records(certificate.relaxed_relations.size()) ||
        !add_records(certificate.defining_relations.size()) ||
        !add_records(certificate.components.size()))
      return resource("approximate_verifier.record_limit");
    for (const auto &vertex : boundary.vertices)
      if (!add_records(vertex.original_vertices.size()) ||
          !add_records(vertex.original_raw_bits.size()) ||
          !add_records(vertex.constructions.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &occurrence : boundary.vertex_occurrences)
      if (!add_records(occurrence.incident_halfedges.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &edge : boundary.edges)
      if (!add_records(edge.uses.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &geometry : boundary.edge_geometry)
      if (!add_records(geometry.curves.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &cycle : boundary.cycles)
      if (!add_records(cycle.halfedges.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &patch : boundary.patches)
      if (!add_records(patch.cycles.size()) ||
          !add_records(patch.provenance.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &curve : boundary.curves)
      if (!add_records(curve.facets.size()) ||
          !add_records(curve.constructions.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &construction : boundary.constructions)
      if (!add_records(construction.children.size()) ||
          !add_records(construction.defining_sources.size()) ||
          !add_records(construction.defining_relations.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &relation : boundary.defining_relations)
      if (!add_records(relation.operand_nodes.size()) ||
          !add_records(relation.defining_sources.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &provenance : boundary.provenance)
      if (!add_records(provenance.contributors.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &occurrence : boundary.source_vertex_occurrences)
      if (!add_records(occurrence.incident_halfedges.size()) ||
          !add_records(occurrence.link_regions.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &arc : boundary.link_arcs)
      if (!add_records(arc.layers.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &sector : boundary.vertex_sectors)
      if (!add_records(sector.boundary_rays.size()) ||
          !add_records(sector.boundary_arcs.size()) ||
          !add_records(sector.witness_evidence.size()) ||
          !add_records(sector.seam_continuations.size()) ||
          !add_records(sector.source_edge_continuations.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &face : output.mesh.faces) {
      if (!add_records(face.size()))
        return resource("approximate_verifier.record_limit");
    }
    for (const auto &occurrence : certificate.occurrence_maps)
      if (!add_records(occurrence.exact_cyclic_halfedges.size()) ||
          !add_records(occurrence.output_cyclic_triangles.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &adjacency : certificate.patch_adjacency)
      if (!add_records(adjacency.exact_radial_patches.size()) ||
          !add_records(adjacency.output_incident_triangles.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &obligation : certificate.obligations)
      if (!add_records(obligation.vertices.size()) ||
          !add_records(obligation.triangles.size()) ||
          !add_records(obligation.exact_entities.size()))
        return resource("approximate_verifier.record_limit");
    for (const auto &component : certificate.components)
      if (!add_records(component.variables.size()) ||
          !add_records(component.variable_order.size()) ||
          !add_records(component.obligations.size()) ||
          !add_records(component.accepted_ranks.size()) ||
          !add_records(component.rejected_prefix_witnesses.size()))
        return resource("approximate_verifier.record_limit");
    if (!add_records(certificate.search.accepted_candidate_ranks.size()))
      return resource("approximate_verifier.record_limit");
    if (!budget.work(budget.verifier_records - records_before_binding,
                     "approximate_verifier.work_limit"))
      return resource(budget.failure_key);
    if (budget.failed())
      return resource(budget.failure_key);
    if (certificate.obligations.size() > policy.search.max_obligations)
      return resource("approximate_verifier.obligation_limit");
    auto spend_work = [&](std::uint64_t amount = 1) {
      return budget.work(amount, "approximate_verifier.work_limit");
    };
    auto spend_predicate = [&] {
      return budget.predicate("approximate_verifier.predicate_limit");
    };
    std::vector<std::vector<defining_relation_id>> vertex_relations(
        boundary.vertices.size());
    for (std::size_t vertex = 0; vertex < boundary.vertices.size(); ++vertex) {
      std::set<construction_node_id> visited;
      std::vector<construction_node_id> pending =
          boundary.vertices[vertex].constructions;
      std::set<defining_relation_id> relations;
      while (!pending.empty()) {
        if (!spend_work())
          return resource(budget.failure_key);
        const auto construction_id = pending.back();
        pending.pop_back();
        if (!visited.insert(construction_id).second)
          continue;
        if (construction_id.value_for_debug() >= boundary.constructions.size())
          return reject("approximate_verifier.construction_range");
        const auto &construction =
            boundary.constructions[construction_id.value_for_debug()];
        pending.insert(pending.end(), construction.children.begin(),
                       construction.children.end());
        relations.insert(construction.defining_relations.begin(),
                         construction.defining_relations.end());
      }
      vertex_relations[vertex] = {relations.begin(), relations.end()};
    }
    if (certificate.schema != certified_approximate_embedding_schema ||
        certificate.coordinate != exact_result_coordinate_type<T>() ||
        certificate.index != exact_result_index_type<I>() ||
        certificate.exact_result_digest != exact->canonical_digest ||
        certificate.selected_boundary_digest !=
            boundary.selected_artifact_digest ||
        certificate.policy_digest !=
            approximate_realization_policy_digest(policy) ||
        approximate_realization_policy_digest(certificate.policy) !=
            certificate.policy_digest ||
        output.selected_boundary_digest != boundary.selected_artifact_digest ||
        certificate.exact_vertex_occurrences !=
            boundary.vertex_occurrences.size() ||
        certificate.exact_edges != boundary.edges.size() ||
        certificate.exact_halfedges != boundary.halfedges.size() ||
        certificate.exact_cycles != boundary.cycles.size() ||
        certificate.exact_patches != boundary.patches.size() ||
        boundary.topology ==
            selected_boundary_topology::closed_stratified_nonmanifold)
      return reject("approximate_verifier.binding");
    if (certificate.vertices.size() != boundary.vertices.size() ||
        output.mesh.vertices.size() != boundary.vertices.size() ||
        certificate.search.accepted_candidate_ranks.size() !=
            boundary.vertices.size())
      return reject("approximate_verifier.vertex_count");
    std::vector<std::uint64_t> occurrence_counts(boundary.vertices.size());
    for (const auto &occurrence : boundary.vertex_occurrences) {
      if (!spend_work())
        return resource(budget.failure_key);
      if (occurrence.vertex.value_for_debug() >= occurrence_counts.size())
        return reject("approximate_verifier.occurrence_range");
      ++occurrence_counts[occurrence.vertex.value_for_debug()];
    }
    if (std::any_of(occurrence_counts.begin(), occurrence_counts.end(),
                    [](std::uint64_t count) { return count != 1; }))
      return reject("approximate_verifier.occurrence_isomorphism");

    canonical_encoder domain_encoding;
    const char domain_tag[] = "YGBADOM1";
    domain_encoding.raw(reinterpret_cast<const std::uint8_t *>(domain_tag), 8);
    domain_encoding.u16(1);
    domain_encoding.u64(boundary.vertices.size());
    std::uint64_t axis_total = 0, point_total = 0,
                  candidate_evaluations = 0;
    bool candidate_generation_limited = false;
    exact_scalar maximum_squared(0);
    std::array<exact_scalar, 3> maximum_axis;
    std::vector<approximate_relaxed_relation> expected_relaxed;
    std::vector<exact_point3> points;
    std::vector<std::vector<replay_point_candidate<T>>> domains;
    domains.reserve(boundary.vertices.size());
    points.reserve(boundary.vertices.size());
    for (std::size_t i = 0; i < boundary.vertices.size(); ++i) {
      if (!spend_work())
        return resource(budget.failure_key);
      const auto &source = boundary.vertices[i];
      const auto &evidence = certificate.vertices[i];
      const bool original = !source.original_vertices.empty();
      const bool fixed = original &&
                         !policy.approximation.allow_original_vertex_movement;
      std::vector<replay_point_candidate<T>> domain;
      if (fixed) {
        if (!budget.work(1, "approximate_verifier.work_limit") ||
            !budget.records(1, "approximate_verifier.record_limit") ||
            !budget.bytes(sizeof(replay_point_candidate<T>),
                          "approximate_verifier.byte_limit"))
          return resource(budget.failure_key);
        if (candidate_evaluations == policy.search.max_candidate_evaluations)
          return reject("approximate_verifier.candidate_evaluation_limit");
        ++candidate_evaluations;
        axis_total += 3;
        if (source.original_raw_bits.empty() ||
            source.original_raw_bits.front().coordinate !=
                exact_result_coordinate_type<T>())
          return reject("approximate_verifier.original_raw_bits_missing");
        if (std::any_of(
                source.original_raw_bits.begin() + 1,
                source.original_raw_bits.end(), [&](const auto &raw) {
                  return raw.coordinate !=
                             source.original_raw_bits.front().coordinate ||
                         raw.bits != source.original_raw_bits.front().bits;
                }))
          return reject("approximate_verifier.conflicting_original_raw_bits");
        replay_point_candidate<T> fixed_candidate;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          if constexpr (std::is_same<T, float>::value)
            fixed_candidate.bits[axis].bits = static_cast<std::uint32_t>(
                source.original_raw_bits.front().bits[axis]);
          else
            fixed_candidate.bits[axis].bits =
                source.original_raw_bits.front().bits[axis];
        }
        fixed_candidate.point = replay_point(fixed_candidate.bits);
        if (!spend_predicate())
          return resource(budget.failure_key);
        if (!(fixed_candidate.point == source.coordinate))
          return reject("approximate_verifier.original_raw_bits_value");
        domain = {std::move(fixed_candidate)};
      } else {
        domain = replay_domain<T>(
            source.coordinate, false, policy.approximation.candidate_ulp_radius,
            policy.search.max_candidates, policy.approximation, axis_total,
            candidate_evaluations, policy.search.max_candidate_evaluations,
            candidate_generation_limited, budget);
        if (candidate_generation_limited)
          return budget.failed()
                     ? resource(budget.failure_key)
                     : reject("approximate_verifier.candidate_evaluation_limit");
      }
      point_total += domain.size();
      domain_encoding.u64(domain.size());
      for (const auto &candidate : domain)
        for (const auto bits : candidate.bits)
          domain_encoding.u64(bits.bits);
      const auto rank = certificate.search.accepted_candidate_ranks[i];
      if (rank >= domain.size() || evidence.selected.value_for_debug() != i ||
          !(evidence.exact_target == source.coordinate) ||
          evidence.output_bits[0].bits != domain[rank].bits[0].bits ||
          evidence.output_bits[1].bits != domain[rank].bits[1].bits ||
          evidence.output_bits[2].bits != domain[rank].bits[2].bits ||
          evidence.accepted_axis_ranks != domain[rank].axis_ranks ||
          evidence.accepted_candidate_rank != rank ||
          evidence.original_vertex != original)
        return reject("approximate_verifier.vertex_domain");
      const auto point = replay_point(evidence.output_bits);
      const exact_point3 displacement{point.x - source.coordinate.x,
                                      point.y - source.coordinate.y,
                                      point.z - source.coordinate.z};
      const auto squared = displacement.x.pow(2) + displacement.y.pow(2) +
                           displacement.z.pow(2);
      if (!(evidence.displacement.x == displacement.x &&
            evidence.displacement.y == displacement.y &&
            evidence.displacement.z == displacement.z) ||
          evidence.squared_displacement != squared ||
          bits_of(output.mesh.vertices[i].x).bits != evidence.output_bits[0].bits ||
          bits_of(output.mesh.vertices[i].y).bits != evidence.output_bits[1].bits ||
          bits_of(output.mesh.vertices[i].z).bits != evidence.output_bits[2].bits)
        return reject("approximate_verifier.vertex_evidence");
      maximum_squared = std::max(maximum_squared, squared);
      const std::array<exact_scalar, 3> delta{
          {displacement.x.abs(), displacement.y.abs(), displacement.z.abs()}};
      for (std::size_t axis = 0; axis < 3; ++axis)
        maximum_axis[axis] = std::max(maximum_axis[axis], delta[axis]);
      for (std::size_t axis = 0; axis < 3; ++axis)
        if (!delta[axis].is_zero()) {
          approximate_relaxed_relation relation;
          relation.kind = original
                              ? approximate_relaxed_relation_kind::
                                    original_vertex_coordinate
                              : approximate_relaxed_relation_kind::
                                    target_coordinate;
          relation.exact_entity = i;
          relation.axis = static_cast<std::uint8_t>(axis);
          relation.exact_value = axis == 0   ? source.coordinate.x
                                 : axis == 1 ? source.coordinate.y
                                             : source.coordinate.z;
          relation.emitted_value = axis == 0   ? point.x
                                   : axis == 1 ? point.y
                                               : point.z;
          relation.absolute_deviation = delta[axis];
          relation.allowed_deviation = dyadic(
              axis == 0 && policy.approximation.has_max_axis_displacement_x
                  ? policy.approximation.max_axis_displacement_x
                  : axis == 1 && policy.approximation.has_max_axis_displacement_y
                        ? policy.approximation.max_axis_displacement_y
                        : axis == 2 &&
                                  policy.approximation.has_max_axis_displacement_z
                              ? policy.approximation.max_axis_displacement_z
                              : policy.approximation.max_vertex_displacement);
          expected_relaxed.push_back(std::move(relation));
        }
      points.push_back(point);
      domains.push_back(std::move(domain));
    }
    const auto replay_domain_digest = ygor::mesh_boolean::domain_digest(
        {{'Y', 'G', 'B', 'A', 'D', 'O', 'M', '1'}}, domain_encoding.bytes());
    if (certificate.search.candidate_generation_version != 1 ||
        certificate.search.candidate_generation_version !=
            policy.approximation.candidate_generation_version ||
        certificate.search.candidate_ulp_radius !=
            policy.approximation.candidate_ulp_radius ||
        certificate.search.candidate_cap != policy.search.max_candidates ||
        certificate.search.candidate_evaluation_limit !=
            policy.search.max_candidate_evaluations ||
        certificate.search.candidate_evaluations != candidate_evaluations ||
        certificate.search.search_node_limit != policy.search.max_search_nodes ||
        certificate.search.obligation_limit != policy.search.max_obligations ||
        certificate.search.triangle_pair_limit !=
            policy.search.max_triangle_pairs ||
        certificate.search.predicate_check_limit !=
            policy.search.max_predicate_checks ||
        certificate.search.verifier_work_limit !=
            policy.search.max_verifier_work ||
        certificate.search.verifier_record_limit !=
            policy.search.max_verifier_records ||
        certificate.search.verifier_byte_limit !=
            policy.search.max_verifier_bytes ||
        certificate.search.generated_axis_candidates != axis_total ||
        certificate.search.generated_point_candidates != point_total ||
        certificate.search.candidate_domain_digest != replay_domain_digest ||
        certificate.maximum_squared_vertex_displacement != maximum_squared ||
        certificate.maximum_axis_displacements != maximum_axis)
      return reject("approximate_verifier.search_domain");

    std::map<std::uint64_t, exact_plane3> planes;
    for (const auto &geometry : boundary.patch_geometry)
      planes.emplace(geometry.patch.value_for_debug(), geometry.plane);
    auto authorized_result =
        reconstruct_authorized_approximate_triangulation(
            boundary, [&](std::uint64_t work, std::uint64_t predicates,
                          std::uint64_t records, std::uint64_t bytes) {
              if (predicates > work)
                return false;
              if (!budget.records(records,
                                  "approximate_verifier.record_limit") ||
                  !budget.bytes(bytes, "approximate_verifier.byte_limit"))
                return false;
              if (!budget.work(work - predicates,
                               "approximate_verifier.work_limit"))
                return false;
              for (std::uint64_t i = 0; i < predicates; ++i)
                if (!budget.predicate(
                        "approximate_verifier.predicate_limit"))
                  return false;
              return true;
            });
    if (!authorized_result.has_value())
      return budget.failed() ? resource(budget.failure_key)
                             : product_status_or<bool>(authorized_result.error());
    const auto &authorized = authorized_result.value();
    if (certificate.triangles.size() != output.mesh.faces.size() ||
        certificate.triangles.size() != authorized.size())
      return reject("approximate_verifier.triangle_count");
    std::map<std::pair<std::uint64_t, std::uint64_t>,
             std::vector<std::pair<std::uint64_t, std::uint64_t>>>
        edge_uses;
    std::map<std::uint64_t, exact_scalar> patch_areas;
    exact_scalar maximum_plane(0);
    exact_scalar maximum_normalized_plane(0);
    for (std::size_t i = 0; i < certificate.triangles.size(); ++i) {
      const auto &triangle = certificate.triangles[i];
      const auto &authority = authorized[i];
      const auto &face = output.mesh.faces[i];
      if (triangle.triangle.value_for_debug() != i || face.size() != 3 ||
          triangle.patch.value_for_debug() >= boundary.patches.size() ||
          triangle.triangle != authority.triangle ||
          triangle.patch != authority.patch ||
          triangle.vertices != authority.vertices ||
          triangle.projection != authority.projection ||
          triangle.exact_orientation != authority.exact_orientation)
        return reject("approximate_verifier.triangle_binding");
      std::array<std::uint64_t, 3> ids;
      for (std::size_t n = 0; n < 3; ++n) {
        ids[n] = triangle.vertices[n].value_for_debug();
        if (ids[n] >= points.size() || static_cast<std::uint64_t>(face[n]) != ids[n])
          return reject("approximate_verifier.triangle_vertex");
      }
      for (std::size_t n = 0; n < 3; ++n) {
        edge_uses[std::minmax(ids[n], ids[(n + 1) % 3])].push_back(
            {ids[n], ids[(n + 1) % 3]});
      }
      if (!spend_predicate())
        return resource(budget.failure_key);
      const auto orientation = orient2d(project(points[ids[0]], triangle.projection),
                                        project(points[ids[1]], triangle.projection),
                                        project(points[ids[2]], triangle.projection));
      if (orientation == exact_sign::zero ||
          orientation != triangle.exact_orientation)
        return reject("approximate_verifier.orientation_noncollapse");
      const auto plane = planes.find(triangle.patch.value_for_debug());
      if (plane == planes.end())
        return reject("approximate_verifier.support_plane_missing");
      const auto bound = dyadic(policy.approximation.max_support_plane_deviation);
      const auto allowed = bound.pow(2) * normal_squared(plane->second);
      for (std::size_t n = 0; n < 3; ++n) {
        if (!spend_predicate())
          return resource(budget.failure_key);
        const auto deviation = evaluate_plane(plane->second, points[ids[n]]).abs();
        const auto normalized =
            deviation.pow(2) / normal_squared(plane->second);
        if (triangle.support_plane_deviations[n] != deviation ||
            triangle.normalized_squared_support_plane_deviations[n] !=
                normalized ||
            allowed < deviation.pow(2))
          return reject("approximate_verifier.support_plane");
        maximum_plane = std::max(maximum_plane, deviation);
        maximum_normalized_plane = std::max(maximum_normalized_plane, normalized);
        if (!deviation.is_zero()) {
          approximate_relaxed_relation relation;
          relation.kind = approximate_relaxed_relation_kind::support_plane;
          relation.exact_entity = i;
          relation.axis = static_cast<std::uint8_t>(n);
          relation.absolute_deviation = deviation.pow(2);
          relation.allowed_deviation = allowed;
          expected_relaxed.push_back(std::move(relation));
        }
      }
      const auto a = project(boundary.vertices[ids[0]].coordinate,
                             triangle.projection);
      const auto b = project(boundary.vertices[ids[1]].coordinate,
                             triangle.projection);
      const auto c = project(boundary.vertices[ids[2]].coordinate,
                             triangle.projection);
      patch_areas[triangle.patch.value_for_debug()] =
          patch_areas[triangle.patch.value_for_debug()] +
          (a.x * b.y - a.y * b.x + b.x * c.y - b.y * c.x +
           c.x * a.y - c.y * a.x);
    }
    if (certificate.maximum_support_plane_deviation != maximum_plane)
      return reject("approximate_verifier.support_maximum");
    if (certificate.maximum_normalized_squared_support_plane_deviation !=
        maximum_normalized_plane)
      return reject("approximate_verifier.normalized_support_maximum");
    std::vector<approximate_defining_relation_evidence> expected_relations;
    const auto relation_bound =
        dyadic(policy.approximation.max_vertex_displacement).pow(2);
    for (std::size_t vertex = 0; vertex < vertex_relations.size(); ++vertex)
      for (const auto relation_id : vertex_relations[vertex]) {
        if (!spend_predicate() || !spend_predicate())
          return resource(budget.failure_key);
        if (relation_id.value_for_debug() >= boundary.defining_relations.size())
          return reject("approximate_verifier.relation_range");
        const auto &relation =
            boundary.defining_relations[relation_id.value_for_debug()];
        approximate_defining_relation_evidence evidence;
        evidence.vertex = selected_vertex_id::from_canonical_value(vertex);
        evidence.relation = relation.id;
        evidence.kind = relation.kind;
        evidence.formula_version = relation.formula_version;
        evidence.expected = relation.expected;
        evidence.exact_residual = replay_relation_residual(
            relation, boundary.vertices[vertex].coordinate);
        evidence.emitted_residual =
            replay_relation_residual(relation, points[vertex]);
        const auto change = evidence.emitted_residual - evidence.exact_residual;
        const auto normal = replay_relation_normal_squared(relation);
        evidence.normalized_squared_residual_change =
            normal.is_zero() ? change.pow(2) : change.pow(2) / normal;
        evidence.allowed_squared_residual_change = relation_bound;
        evidence.exact = change.is_zero();
        evidence.passed =
            !(relation_bound < evidence.normalized_squared_residual_change) &&
            (relation.expected == exact_sign::zero ||
             evidence.emitted_residual.sign() == relation.expected);
        if (!evidence.passed)
          return reject("approximate_verifier.relation_failed");
        if (!evidence.exact) {
          approximate_relaxed_relation relaxed;
          relaxed.kind = approximate_relaxed_relation_kind::defining_relation;
          relaxed.exact_entity = vertex;
          relaxed.absolute_deviation =
              evidence.normalized_squared_residual_change;
          relaxed.allowed_deviation = relation_bound;
          relaxed.defining_relation = relation.id;
          relaxed.relation_kind = relation.kind;
          relaxed.relation_formula_version = relation.formula_version;
          relaxed.relation_expected = relation.expected;
          relaxed.exact_residual = evidence.exact_residual;
          relaxed.emitted_residual = evidence.emitted_residual;
          expected_relaxed.push_back(std::move(relaxed));
        }
        expected_relations.push_back(std::move(evidence));
      }
    if (certificate.defining_relations.size() != expected_relations.size())
      return reject("approximate_verifier.relation_inventory");
    for (std::size_t i = 0; i < expected_relations.size(); ++i) {
      const auto &actual = certificate.defining_relations[i];
      const auto &expected = expected_relations[i];
      if (actual.vertex != expected.vertex ||
          actual.relation != expected.relation || actual.kind != expected.kind ||
          actual.formula_version != expected.formula_version ||
          actual.expected != expected.expected ||
          actual.exact_residual != expected.exact_residual ||
          actual.emitted_residual != expected.emitted_residual ||
          actual.normalized_squared_residual_change !=
              expected.normalized_squared_residual_change ||
          actual.allowed_squared_residual_change !=
              expected.allowed_squared_residual_change ||
          actual.exact != expected.exact || actual.passed != expected.passed)
        return reject("approximate_verifier.relation_evidence");
    }
    if (certificate.relaxed_relations.size() != expected_relaxed.size())
      return reject("approximate_verifier.relaxed_inventory");
    for (std::size_t i = 0; i < expected_relaxed.size(); ++i) {
      const auto &actual = certificate.relaxed_relations[i];
      const auto &expected = expected_relaxed[i];
      if (actual.kind != expected.kind ||
          actual.exact_entity != expected.exact_entity ||
          actual.axis != expected.axis ||
          actual.exact_value != expected.exact_value ||
          actual.emitted_value != expected.emitted_value ||
          actual.absolute_deviation != expected.absolute_deviation ||
          actual.allowed_deviation != expected.allowed_deviation ||
          actual.defining_relation != expected.defining_relation ||
          actual.relation_kind != expected.relation_kind ||
          actual.relation_formula_version !=
              expected.relation_formula_version ||
          actual.relation_expected != expected.relation_expected ||
          actual.exact_residual != expected.exact_residual ||
          actual.emitted_residual != expected.emitted_residual)
        return reject("approximate_verifier.relaxed_relation");
    }
    for (const auto &use : edge_uses)
      if (use.second.size() != 2 ||
          use.second[0].first != use.second[1].second ||
          use.second[0].second != use.second[1].first)
        return reject("approximate_verifier.edge_incidence");
    for (const auto &edge : boundary.edges)
      if (edge_uses.count(std::minmax(edge.lower.value_for_debug(),
                                     edge.upper.value_for_debug())) == 0)
        return reject("approximate_verifier.selected_edge_missing");
    if (!verify_vertex_links(output, budget)) {
      if (budget.failed())
        return resource(budget.failure_key);
      else
        return reject("approximate_verifier.vertex_link");
    }
    auto triangle_for_halfedge = [&](const selected_halfedge &halfedge)
        -> std::optional<realization_triangle_id> {
      for (const auto &triangle : authorized) {
        if (!spend_work())
          return {};
        if (triangle.patch != halfedge.patch)
          continue;
        for (std::size_t edge = 0; edge < 3; ++edge)
          if (triangle.vertices[edge] == halfedge.origin &&
              triangle.vertices[(edge + 1) % 3] == halfedge.destination)
            return triangle.triangle;
      }
      return {};
    };
    if (certificate.occurrence_maps.size() !=
        boundary.vertex_occurrences.size())
      return reject("approximate_verifier.occurrence_map_count");
    for (std::size_t i = 0; i < boundary.vertex_occurrences.size(); ++i) {
      if (!spend_work())
        return resource(budget.failure_key);
      const auto &exact_occurrence = boundary.vertex_occurrences[i];
      const auto &mapping = certificate.occurrence_maps[i];
      if (mapping.occurrence != exact_occurrence.id ||
          mapping.output_vertex != exact_occurrence.vertex ||
          mapping.exact_cyclic_halfedges != exact_occurrence.incident_halfedges ||
          exact_occurrence.incident_halfedges.empty())
        return reject("approximate_verifier.occurrence_map_binding");
      const auto first_halfedge = exact_occurrence.incident_halfedges.front();
      if (first_halfedge.value_for_debug() >= boundary.halfedges.size())
        return reject("approximate_verifier.occurrence_halfedge_range");
      const auto &anchor_halfedge =
          boundary.halfedges[first_halfedge.value_for_debug()];
      const auto start = anchor_halfedge.origin == exact_occurrence.vertex
                             ? anchor_halfedge.destination
                             : anchor_halfedge.origin;
      std::map<selected_vertex_id,
               std::pair<selected_vertex_id, realization_triangle_id>>
          link_successor;
      for (const auto &triangle : authorized)
        for (std::size_t vertex = 0; vertex < 3; ++vertex)
          if (!spend_work())
            return resource(budget.failure_key);
          else
          if (triangle.vertices[vertex] == exact_occurrence.vertex) {
            const auto next = triangle.vertices[(vertex + 1) % 3];
            const auto previous = triangle.vertices[(vertex + 2) % 3];
            if (!link_successor.emplace(next,
                                        std::make_pair(previous,
                                                       triangle.triangle))
                     .second)
              return reject("approximate_verifier.nonmanifold_output_link");
          }
      std::vector<realization_triangle_id> expected_triangles;
      auto current = start;
      std::set<selected_vertex_id> visited_link;
      while (visited_link.insert(current).second) {
        if (!spend_work())
          return resource(budget.failure_key);
        const auto successor = link_successor.find(current);
        if (successor == link_successor.end())
          return reject("approximate_verifier.open_output_link");
        expected_triangles.push_back(successor->second.second);
        current = successor->second.first;
      }
      if (current != start || visited_link.size() != link_successor.size() ||
          mapping.output_cyclic_triangles != expected_triangles)
        return reject("approximate_verifier.vertex_link_cyclic_sequence");
      std::vector<selected_patch_id> output_patch_cycle;
      for (const auto triangle_id : expected_triangles) {
        const auto patch = authorized[triangle_id.value_for_debug()].patch;
        if (output_patch_cycle.empty() || output_patch_cycle.back() != patch)
          output_patch_cycle.push_back(patch);
      }
      if (output_patch_cycle.size() > 1 &&
          output_patch_cycle.front() == output_patch_cycle.back())
        output_patch_cycle.pop_back();
      std::map<selected_patch_id, std::vector<selected_patch_id>> patch_graph;
      for (const auto &edge : boundary.edges) {
        if (!spend_work())
          return resource(budget.failure_key);
        if (edge.lower != exact_occurrence.vertex &&
            edge.upper != exact_occurrence.vertex)
          continue;
        std::vector<selected_patch_id> patches;
        for (const auto use : edge.uses) {
          if (use.value_for_debug() >= boundary.halfedges.size())
            return reject("approximate_verifier.link_edge_use_range");
          patches.push_back(boundary.halfedges[use.value_for_debug()].patch);
        }
        std::sort(patches.begin(), patches.end());
        patches.erase(std::unique(patches.begin(), patches.end()), patches.end());
        if (patches.size() != 2)
          return reject("approximate_verifier.link_edge_patches");
        patch_graph[patches[0]].push_back(patches[1]);
        patch_graph[patches[1]].push_back(patches[0]);
      }
      for (auto &entry : patch_graph) {
        std::sort(entry.second.begin(), entry.second.end());
        entry.second.erase(std::unique(entry.second.begin(), entry.second.end()),
                           entry.second.end());
        if (entry.second.size() != 2)
          return reject("approximate_verifier.exact_patch_link");
      }
      auto canonical_cycle = [](std::vector<selected_patch_id> cycle) {
        if (cycle.empty()) return cycle;
        auto minimum = std::min_element(cycle.begin(), cycle.end());
        std::rotate(cycle.begin(), minimum, cycle.end());
        auto reversed = cycle;
        std::reverse(reversed.begin() + 1, reversed.end());
        return reversed < cycle ? reversed : cycle;
      };
      std::vector<selected_patch_id> exact_patch_cycle;
      if (!patch_graph.empty()) {
        const auto first = patch_graph.begin()->first;
        auto previous = first;
        auto current_patch = patch_graph.begin()->second.front();
        exact_patch_cycle.push_back(first);
        while (current_patch != first) {
          exact_patch_cycle.push_back(current_patch);
          const auto &neighbors = patch_graph.at(current_patch);
          const auto next = neighbors[0] == previous ? neighbors[1] : neighbors[0];
          previous = current_patch;
          current_patch = next;
          if (exact_patch_cycle.size() > patch_graph.size())
            return reject("approximate_verifier.exact_patch_link_cycle");
        }
      }
      if (canonical_cycle(output_patch_cycle) !=
          canonical_cycle(exact_patch_cycle))
        return reject("approximate_verifier.vertex_link_patch_order");
    }
    if (certificate.patch_adjacency.size() != boundary.edges.size())
      return reject("approximate_verifier.patch_adjacency_count");
    for (std::size_t i = 0; i < boundary.edges.size(); ++i) {
      if (!spend_work())
        return resource(budget.failure_key);
      const auto &edge = boundary.edges[i];
      const auto &adjacency = certificate.patch_adjacency[i];
      if (adjacency.edge != edge.id ||
          adjacency.exact_radial_patches.size() != edge.uses.size() ||
          adjacency.output_incident_triangles.size() != edge.uses.size())
        return reject("approximate_verifier.patch_adjacency_binding");
      for (std::size_t use = 0; use < edge.uses.size(); ++use) {
        const auto halfedge_id = edge.uses[use];
        if (halfedge_id.value_for_debug() >= boundary.halfedges.size())
          return reject("approximate_verifier.radial_halfedge_range");
        const auto &halfedge = boundary.halfedges[halfedge_id.value_for_debug()];
        const auto expected = triangle_for_halfedge(halfedge);
        if (budget.failed())
          return resource(budget.failure_key);
        if (adjacency.exact_radial_patches[use] != halfedge.patch || !expected ||
            adjacency.output_incident_triangles[use] != *expected)
          return reject("approximate_verifier.edge_radial_sequence");
      }
    }
    for (const auto &patch : boundary.patches) {
      if (!spend_work())
        return resource(budget.failure_key);
      if (patch.cycles.empty())
        return reject("approximate_verifier.patch_cycles");
      const auto triangle = std::find_if(
          certificate.triangles.begin(), certificate.triangles.end(),
          [&](const auto &value) { return value.patch == patch.id; });
      if (triangle == certificate.triangles.end())
        return reject("approximate_verifier.patch_triangulation");
      exact_scalar cycle_area(0);
      for (const auto cycle_id : patch.cycles) {
        if (cycle_id.value_for_debug() >= boundary.cycles.size())
          return reject("approximate_verifier.cycle_range");
        const auto &cycle = boundary.cycles[cycle_id.value_for_debug()];
        for (const auto halfedge_id : cycle.halfedges) {
          if (!spend_work())
            return resource(budget.failure_key);
          if (halfedge_id.value_for_debug() >= boundary.halfedges.size())
            return reject("approximate_verifier.halfedge_range");
          const auto &halfedge = boundary.halfedges[halfedge_id.value_for_debug()];
          const auto &a = boundary.vertices[halfedge.origin.value_for_debug()];
          const auto &b = boundary.vertices[halfedge.destination.value_for_debug()];
          const auto pa = project(a.coordinate, triangle->projection);
          const auto pb = project(b.coordinate, triangle->projection);
          cycle_area = cycle_area + pa.x * pb.y - pa.y * pb.x;
        }
      }
      if (!spend_predicate())
        return resource(budget.failure_key);
      if (patch_areas[patch.id.value_for_debug()] != cycle_area)
        return reject("approximate_verifier.patch_partition");
    }
    std::uint64_t observed_triangle_pairs = 0;
    for (std::size_t i = 0; i < certificate.triangles.size(); ++i)
      for (std::size_t j = i + 1; j < certificate.triangles.size(); ++j) {
        if (observed_triangle_pairs == policy.search.max_triangle_pairs)
          return resource("approximate_verifier.triangle_pair_limit");
        ++observed_triangle_pairs;
        if (!spend_predicate())
          return resource(budget.failure_key);
        const auto &a = certificate.triangles[i];
        const auto &b = certificate.triangles[j];
        std::size_t shared = 0;
        for (const auto av : a.vertices)
          for (const auto bv : b.vertices)
            shared += av == bv;
        const auto relation = relate_triangles(
            {points[a.vertices[0].value_for_debug()],
             points[a.vertices[1].value_for_debug()],
             points[a.vertices[2].value_for_debug()]},
            {points[b.vertices[0].value_for_debug()],
             points[b.vertices[1].value_for_debug()],
             points[b.vertices[2].value_for_debug()]});
        if (!((shared == 0 && relation == polygon_intersection_kind::disjoint) ||
              (shared == 1 && relation == polygon_intersection_kind::point) ||
              (shared == 2 && relation == polygon_intersection_kind::segment)))
          return reject("approximate_verifier.prohibited_intersection");
      }

    std::vector<approximate_obligation_evidence> expected_obligations;
    auto expect = [&](approximate_obligation_kind kind,
                      std::vector<std::uint64_t> vertices,
                      std::vector<std::uint64_t> triangles = {},
                      std::vector<std::uint64_t> entities = {}) {
      approximate_obligation_evidence value;
      value.kind = kind;
      value.vertices = std::move(vertices);
      value.triangles = std::move(triangles);
      value.exact_entities = std::move(entities);
      expected_obligations.push_back(std::move(value));
    };
    std::vector<std::uint64_t> all_vertices(boundary.vertices.size());
    for (std::size_t i = 0; i < all_vertices.size(); ++i)
      all_vertices[i] = i;
    expect(approximate_obligation_kind::occurrence_isomorphic_topology,
           all_vertices);
    for (std::size_t i = 0; i < boundary.vertices.size(); ++i)
      for (std::size_t j = i + 1; j < boundary.vertices.size(); ++j)
        expect(approximate_obligation_kind::distinct_required_vertices, {i, j});
    for (std::size_t vertex = 0; vertex < vertex_relations.size(); ++vertex)
      for (const auto relation : vertex_relations[vertex])
        expect(approximate_obligation_kind::defining_relation, {vertex}, {},
               {relation.value_for_debug()});
    for (const auto &edge : boundary.edges) {
      const std::vector<std::uint64_t> vertices{
          edge.lower.value_for_debug(), edge.upper.value_for_debug()};
      const std::vector<std::uint64_t> entity{edge.id.value_for_debug()};
      expect(approximate_obligation_kind::selected_edge_order, vertices, {}, entity);
      expect(approximate_obligation_kind::required_incidence, vertices, {}, entity);
      expect(approximate_obligation_kind::edge_radial_order, vertices, {}, entity);
    }
    for (const auto &occurrence : boundary.vertex_occurrences)
      expect(approximate_obligation_kind::vertex_link_order,
             {occurrence.vertex.value_for_debug()}, {},
             {occurrence.id.value_for_debug()});
    for (const auto &triangle : certificate.triangles) {
      const std::vector<std::uint64_t> vertices{
          triangle.vertices[0].value_for_debug(),
          triangle.vertices[1].value_for_debug(),
          triangle.vertices[2].value_for_debug()};
      const std::vector<std::uint64_t> triangle_id{
          triangle.triangle.value_for_debug()};
      const std::vector<std::uint64_t> patch{triangle.patch.value_for_debug()};
      expect(approximate_obligation_kind::triangle_noncollapse, vertices,
             triangle_id, patch);
      expect(approximate_obligation_kind::triangle_orientation, vertices,
             triangle_id, patch);
      expect(approximate_obligation_kind::support_plane_deviation, vertices,
             triangle_id, patch);
    }
    for (std::size_t i = 0; i < certificate.triangles.size(); ++i)
      for (std::size_t j = i + 1; j < certificate.triangles.size(); ++j) {
        std::set<std::uint64_t> participants;
        for (const auto vertex : certificate.triangles[i].vertices)
          participants.insert(vertex.value_for_debug());
        for (const auto vertex : certificate.triangles[j].vertices)
          participants.insert(vertex.value_for_debug());
        expect(approximate_obligation_kind::prohibited_nonadjacent_intersection,
               {participants.begin(), participants.end()}, {i, j});
      }
    expect(approximate_obligation_kind::global_embedding, all_vertices);
    if (certificate.obligations.size() != expected_obligations.size())
      return reject("approximate_verifier.obligation_universe");
    for (std::size_t i = 0; i < expected_obligations.size(); ++i)
      if (certificate.obligations[i].kind != expected_obligations[i].kind ||
          certificate.obligations[i].vertices != expected_obligations[i].vertices ||
          certificate.obligations[i].triangles != expected_obligations[i].triangles ||
          certificate.obligations[i].exact_entities !=
              expected_obligations[i].exact_entities)
        return reject("approximate_verifier.obligation_binding");
    for (const auto &obligation : certificate.obligations) {
      if (obligation.kind ==
          approximate_obligation_kind::support_plane_deviation) {
        exact_scalar measured(0);
        for (const auto &value :
             certificate.triangles[obligation.triangles.front()]
                 .normalized_squared_support_plane_deviations)
          measured = std::max(measured, value);
        if (obligation.measured_value != measured ||
            obligation.allowed_value !=
                dyadic(policy.approximation.max_support_plane_deviation).pow(2))
          return reject("approximate_verifier.support_obligation_values");
      } else if (obligation.kind ==
                 approximate_obligation_kind::defining_relation) {
        const auto found = std::find_if(
            expected_relations.begin(), expected_relations.end(),
            [&](const auto &value) {
              return value.vertex.value_for_debug() ==
                         obligation.vertices.front() &&
                     value.relation.value_for_debug() ==
                         obligation.exact_entities.front();
            });
        if (found == expected_relations.end() ||
            obligation.measured_value !=
                found->normalized_squared_residual_change ||
            obligation.allowed_value !=
                found->allowed_squared_residual_change)
          return reject("approximate_verifier.relation_obligation_values");
      }
    }
    if (certificate.obligations.empty() ||
        std::any_of(certificate.obligations.begin(),
                    certificate.obligations.end(),
                    [](const auto &o) { return !o.passed; }))
      return reject("approximate_verifier.obligations");
    std::set<approximate_obligation_kind> mandatory;
    for (const auto &obligation : certificate.obligations)
      mandatory.insert(obligation.kind);
    for (const auto kind : {
             approximate_obligation_kind::occurrence_isomorphic_topology,
             approximate_obligation_kind::distinct_required_vertices,
             approximate_obligation_kind::selected_edge_order,
             approximate_obligation_kind::required_incidence,
             approximate_obligation_kind::triangle_noncollapse,
             approximate_obligation_kind::triangle_orientation,
             approximate_obligation_kind::support_plane_deviation,
             approximate_obligation_kind::prohibited_nonadjacent_intersection,
             approximate_obligation_kind::vertex_link_order,
             approximate_obligation_kind::edge_radial_order,
             approximate_obligation_kind::global_embedding})
      if (!mandatory.count(kind) && !boundary.vertices.empty())
        return reject("approximate_verifier.mandatory_obligation_missing");
    if (!expected_relations.empty() &&
        !mandatory.count(approximate_obligation_kind::defining_relation))
      return reject("approximate_verifier.defining_obligation_missing");
    struct replay_component {
      std::vector<std::uint64_t> variables;
      std::vector<std::uint64_t> variable_order;
      std::vector<std::uint64_t> obligations;
      std::vector<std::uint64_t> accepted_ranks;
      std::vector<std::uint64_t> rejected;
      std::uint64_t visited = 0, complete = 0;
      bool accepted = false, limited = false;
    } replay;
    std::uint64_t predicate_checks = 0;
    bool predicate_limited = false;
    replay.variables.resize(boundary.vertices.size());
    for (std::size_t i = 0; i < replay.variables.size(); ++i)
      replay.variables[i] = i;
    replay.obligations.resize(certificate.obligations.size());
    for (std::size_t i = 0; i < replay.obligations.size(); ++i)
      replay.obligations[i] = i;
    auto evaluate_assignment = [&](std::uint64_t obligation_id,
                                   const std::map<std::uint64_t,
                                                  std::uint64_t> &assignment) {
      if (predicate_checks == std::numeric_limits<std::uint64_t>::max()) {
        predicate_limited = true;
        return false;
      }
      ++predicate_checks;
      const auto &obligation = certificate.obligations[obligation_id];
      auto point_for = [&](std::uint64_t vertex) -> const exact_point3 & {
        return domains[vertex][assignment.at(vertex)].point;
      };
      switch (obligation.kind) {
      case approximate_obligation_kind::distinct_required_vertices:
      case approximate_obligation_kind::selected_edge_order:
      case approximate_obligation_kind::required_incidence:
      case approximate_obligation_kind::edge_radial_order:
        if (!spend_predicate()) {
          predicate_limited = true;
          return false;
        }
        return !(point_for(obligation.vertices[0]) ==
                 point_for(obligation.vertices[1]));
      case approximate_obligation_kind::triangle_noncollapse:
      case approximate_obligation_kind::triangle_orientation: {
        if (!spend_predicate()) {
          predicate_limited = true;
          return false;
        }
        const auto &triangle = authorized[obligation.triangles.front()];
        return orient2d(
                   project(point_for(triangle.vertices[0].value_for_debug()),
                           triangle.projection),
                   project(point_for(triangle.vertices[1].value_for_debug()),
                           triangle.projection),
                   project(point_for(triangle.vertices[2].value_for_debug()),
                           triangle.projection)) == triangle.exact_orientation;
      }
      case approximate_obligation_kind::support_plane_deviation: {
        const auto &triangle = authorized[obligation.triangles.front()];
        const auto &plane = planes.at(triangle.patch.value_for_debug());
        const auto allowed =
            dyadic(policy.approximation.max_support_plane_deviation).pow(2) *
            normal_squared(plane);
        for (const auto vertex : triangle.vertices)
          if (!spend_predicate()) {
            predicate_limited = true;
            return false;
          } else
          if (allowed < evaluate_plane(
                            plane, point_for(vertex.value_for_debug())).pow(2))
            return false;
        return true;
      }
      case approximate_obligation_kind::defining_relation: {
        if (!spend_predicate() || !spend_predicate()) {
          predicate_limited = true;
          return false;
        }
        const auto &relation = boundary.defining_relations[
            obligation.exact_entities.front()];
        const auto vertex = obligation.vertices.front();
        const auto exact_residual = replay_relation_residual(
            relation, boundary.vertices[vertex].coordinate);
        const auto emitted =
            replay_relation_residual(relation, point_for(vertex));
        const auto change = emitted - exact_residual;
        const auto normal = replay_relation_normal_squared(relation);
        return (normal.is_zero()
                    ? change.is_zero()
                    : !(dyadic(policy.approximation.max_vertex_displacement)
                                .pow(2) * normal < change.pow(2))) &&
               (relation.expected == exact_sign::zero ||
                emitted.sign() == relation.expected);
      }
      case approximate_obligation_kind::prohibited_nonadjacent_intersection: {
        if (!spend_predicate()) {
          predicate_limited = true;
          return false;
        }
        const auto &a = authorized[obligation.triangles[0]];
        const auto &b = authorized[obligation.triangles[1]];
        std::size_t shared = 0;
        for (const auto av : a.vertices)
          for (const auto bv : b.vertices)
            shared += av == bv;
        const auto relation = relate_triangles(
            {point_for(a.vertices[0].value_for_debug()),
             point_for(a.vertices[1].value_for_debug()),
             point_for(a.vertices[2].value_for_debug())},
            {point_for(b.vertices[0].value_for_debug()),
             point_for(b.vertices[1].value_for_debug()),
             point_for(b.vertices[2].value_for_debug())});
        return (shared == 0 && relation == polygon_intersection_kind::disjoint) ||
               (shared == 1 && relation == polygon_intersection_kind::point) ||
               (shared == 2 && relation == polygon_intersection_kind::segment);
      }
      case approximate_obligation_kind::occurrence_isomorphic_topology:
      case approximate_obligation_kind::vertex_link_order:
      case approximate_obligation_kind::global_embedding:
        return true;
      }
      return false;
    };
    std::vector<std::uint64_t> degree(boundary.vertices.size());
    for (const auto &obligation : certificate.obligations)
      for (const auto vertex : obligation.vertices)
        ++degree[vertex];
    auto order = replay.variables;
    std::sort(order.begin(), order.end(), [&](auto a, auto b) {
      if (domains[a].size() != domains[b].size())
        return domains[a].size() < domains[b].size();
      if (degree[a] != degree[b])
        return degree[a] > degree[b];
      return a < b;
    });
    replay.variable_order = order;
    if (order.empty()) {
      replay.accepted = std::all_of(
          replay.obligations.begin(), replay.obligations.end(),
          [&](auto id) { return evaluate_assignment(id, {}); });
    } else if (std::all_of(order.begin(), order.end(),
                           [&](auto id) { return domains[id].size() == 1; })) {
      replay.visited = order.size() + 1;
      if (replay.visited > policy.search.max_search_nodes ||
          !spend_work(replay.visited))
        replay.limited = true;
      else {
        std::map<std::uint64_t, std::uint64_t> assignment;
        for (const auto variable : order)
          assignment[variable] = 0;
        replay.accepted = std::all_of(
            replay.obligations.begin(), replay.obligations.end(),
            [&](auto id) { return evaluate_assignment(id, assignment); });
        replay.complete = replay.accepted ? 1 : 0;
        if (replay.accepted)
          replay.accepted_ranks.assign(replay.variables.size(), 0);
      }
    } else {
      std::map<std::uint64_t, std::uint64_t> assignment, accepted;
      std::function<void(std::size_t)> dfs = [&](std::size_t depth) {
        if (replay.accepted || replay.limited)
          return;
        if (replay.visited == policy.search.max_search_nodes) {
          replay.limited = true;
          return;
        }
        if (!spend_work()) {
          replay.limited = true;
          return;
        }
        ++replay.visited;
        if (depth == order.size()) {
          ++replay.complete;
          replay.accepted = true;
          accepted = assignment;
          return;
        }
        const auto variable = order[depth];
        for (std::uint64_t rank = 0; rank < domains[variable].size(); ++rank) {
          assignment[variable] = rank;
          std::optional<std::uint64_t> conflict;
          for (const auto obligation_id : replay.obligations) {
            if (!spend_work()) {
              replay.limited = true;
              break;
            }
            const auto &obligation = certificate.obligations[obligation_id];
            if (std::all_of(obligation.vertices.begin(),
                            obligation.vertices.end(), [&](auto vertex) {
                  return assignment.count(vertex) != 0;
                }) && !evaluate_assignment(obligation_id, assignment)) {
              conflict = obligation_id;
              break;
            }
          }
          if (conflict)
            replay.rejected.push_back(*conflict);
          else
            dfs(depth + 1);
          assignment.erase(variable);
          if (replay.accepted || replay.limited)
            break;
        }
      };
      dfs(0);
      if (replay.accepted)
        for (const auto variable : replay.variables)
          replay.accepted_ranks.push_back(accepted[variable]);
    }
    if (predicate_limited)
      return resource(budget.failed() ? budget.failure_key
                                      : "approximate_verifier.predicate_limit");
    if (budget.failed())
      return resource(budget.failure_key);
    if (replay.limited || !replay.accepted)
      return reject("approximate_verifier.search_replay_outcome");
    std::map<std::uint64_t, std::uint64_t> final_assignment;
    for (std::size_t i = 0; i < replay.variables.size(); ++i)
      final_assignment.emplace(replay.variables[i], replay.accepted_ranks[i]);
    for (const auto obligation : replay.obligations)
      if (!evaluate_assignment(obligation, final_assignment)) {
        if (predicate_limited)
          return resource(budget.failed() ? budget.failure_key
                                          : "approximate_verifier.predicate_limit");
        return reject("approximate_verifier.accepted_obligation_replay");
      }
    const std::size_t expected_component_count = replay.variables.empty() ? 0 : 1;
    if (certificate.components.size() != expected_component_count)
      return reject("approximate_verifier.component_partition");
    if (!certificate.components.empty()) {
      const auto &component = certificate.components.front();
      if (component.variables != replay.variables ||
          component.variable_order != replay.variable_order ||
          component.obligations != replay.obligations ||
          component.accepted_ranks != replay.accepted_ranks ||
          component.rejected_prefix_witnesses != replay.rejected ||
          component.visited_nodes != replay.visited ||
          component.complete_assignments != replay.complete)
        return reject("approximate_verifier.component_transcript_replay");
      canonical_encoder graph;
      for (const auto value : component.variables)
        graph.u64(value);
      for (const auto value : component.variable_order)
        graph.u64(value);
      for (const auto value : component.obligations)
        graph.u64(value);
      if (component.graph_digest != ygor::mesh_boolean::domain_digest(
              {{'Y', 'G', 'B', 'A', 'G', 'R', 'P', '1'}}, graph.bytes()))
        return reject("approximate_verifier.component_digest");
      canonical_encoder component_transcript;
      for (const auto value : component.variable_order)
        component_transcript.u64(value);
      for (const auto value : component.obligations)
        component_transcript.u64(value);
      for (const auto value : component.accepted_ranks)
        component_transcript.u64(value);
      for (const auto value : component.rejected_prefix_witnesses)
        component_transcript.u64(value);
      component_transcript.u64(component.visited_nodes);
      component_transcript.u64(component.complete_assignments);
      component_transcript.u64(certificate.search.candidate_cap);
      component_transcript.u64(certificate.search.candidate_evaluation_limit);
      component_transcript.u64(certificate.search.candidate_evaluations);
      component_transcript.u64(certificate.search.search_node_limit);
      component_transcript.u64(certificate.search.obligation_limit);
      component_transcript.u64(certificate.search.triangle_pair_limit);
      component_transcript.u64(certificate.search.predicate_check_limit);
      component_transcript.u64(predicate_checks);
      component_transcript.u64(certificate.search.verifier_work_limit);
      component_transcript.u64(certificate.search.verifier_record_limit);
      component_transcript.u64(certificate.search.verifier_byte_limit);
      if (component.transcript_digest != ygor::mesh_boolean::domain_digest(
              {{'Y', 'G', 'B', 'A', 'T', 'R', 'N', '1'}},
              component_transcript.bytes()))
        return reject("approximate_verifier.component_transcript_digest");
    }
    if (certificate.search.visited_nodes != replay.visited ||
        certificate.search.complete_assignments != replay.complete ||
        certificate.search.accepted_candidate_ranks != replay.accepted_ranks ||
        certificate.search.predicate_checks != predicate_checks)
      return reject("approximate_verifier.search_counters");
    canonical_encoder search_transcript;
    search_transcript.raw(replay_domain_digest.bytes.data(), 16);
    search_transcript.u64(certificate.search.candidate_cap);
    search_transcript.u64(certificate.search.candidate_evaluation_limit);
    search_transcript.u64(certificate.search.candidate_evaluations);
    search_transcript.u64(certificate.search.search_node_limit);
    search_transcript.u64(certificate.search.obligation_limit);
    search_transcript.u64(certificate.search.triangle_pair_limit);
    search_transcript.u64(certificate.search.predicate_check_limit);
    search_transcript.u64(predicate_checks);
    search_transcript.u64(certificate.search.verifier_work_limit);
    search_transcript.u64(certificate.search.verifier_record_limit);
    search_transcript.u64(certificate.search.verifier_byte_limit);
    search_transcript.u64(certificate.search.visited_nodes);
    search_transcript.u64(certificate.search.complete_assignments);
    for (const auto rank : certificate.search.accepted_candidate_ranks)
      search_transcript.u64(rank);
    if (certificate.search.transcript_digest != ygor::mesh_boolean::domain_digest(
            {{'Y', 'G', 'B', 'A', 'S', 'R', 'C', '1'}},
            search_transcript.bytes()))
      return reject("approximate_verifier.search_transcript");

    const auto bytes = replay_output_bytes(output);
    if (!budget.bytes(bytes.size(), "approximate_verifier.byte_limit"))
      return resource(budget.failure_key);
    const auto output_digest = ygor::mesh_boolean::domain_digest(
        {{'Y', 'G', 'B', 'A', 'O', 'U', 'T', '1'}}, bytes);
    if (output.canonical_output_digest != output_digest ||
        output.summary.semantic_digest != output_digest ||
        output.summary.vertices != output.mesh.vertices.size() ||
        output.summary.faces != output.mesh.faces.size() ||
        output.summary.face_indices != 3 * output.mesh.faces.size() ||
        output.summary.components != boundary.certificate.connected_components)
      return reject("approximate_verifier.output_digest");
    const auto replay_components =
        replay_connected_components(output.mesh.faces, budget);
    if (budget.failed())
      return resource(budget.failure_key);
    if (output.summary.components != replay_components)
      return reject("approximate_verifier.output_digest");
    const auto certificate_bytes = replay_certificate_bytes(certificate);
    if (certificate.canonical_bytes != certificate_bytes ||
        certificate.certificate_digest != ygor::mesh_boolean::domain_digest(
            {{'Y', 'G', 'B', 'A', 'C', 'E', 'R', '1'}}, certificate_bytes))
      return reject("approximate_verifier.certificate_digest");
    return true;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "approximate_verifier.allocation");
  } catch (...) {
    return reject("approximate_verifier.exception");
  }
}

template <class T, class I>
product_status_or<bool> verify_certified_approximate_embedding(
    const exact_result_handle &exact, const product_realization_policy &policy,
    const boolean_success<T, I> &output,
    const certified_approximate_certificate<T, I> &certificate) noexcept {
  approximate_verifier_budget budget(policy.search);
  return detail::verify_certified_approximate_embedding_with_budget(
      exact, policy, output, certificate, budget);
}

#define YGOR_APPROXIMATE_VERIFY_DEFINE(T, I)                                  \
  template product_status_or<bool>                                            \
  detail::verify_certified_approximate_embedding_with_budget(                 \
      const exact_result_handle &, const product_realization_policy &,        \
      const boolean_success<T, I> &,                                          \
      const certified_approximate_certificate<T, I> &,                        \
      approximate_verifier_budget &) noexcept;                                \
  template product_status_or<bool> verify_certified_approximate_embedding(   \
      const exact_result_handle &, const product_realization_policy &,        \
      const boolean_success<T, I> &,                                          \
      const certified_approximate_certificate<T, I> &) noexcept
YGOR_APPROXIMATE_VERIFY_DEFINE(float, std::uint32_t);
YGOR_APPROXIMATE_VERIFY_DEFINE(float, std::uint64_t);
YGOR_APPROXIMATE_VERIFY_DEFINE(double, std::uint32_t);
YGOR_APPROXIMATE_VERIFY_DEFINE(double, std::uint64_t);
#undef YGOR_APPROXIMATE_VERIFY_DEFINE

} // namespace mesh_boolean
} // namespace ygor
