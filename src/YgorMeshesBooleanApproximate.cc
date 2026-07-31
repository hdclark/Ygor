#include "YgorMeshesBooleanApproximate.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

bool checked_add(std::uint64_t &value, std::uint64_t increment,
                 std::uint64_t limit) {
  if (increment > limit || value > limit - increment)
    return false;
  value += increment;
  return true;
}

bool checked_pairs(std::uint64_t count, std::uint64_t &pairs) {
  if (count < 2) {
    pairs = 0;
    return true;
  }
  const auto a = count % 2 == 0 ? count / 2 : count;
  const auto b = count % 2 == 0 ? count - 1 : (count - 1) / 2;
  if (b != 0 && a > std::numeric_limits<std::uint64_t>::max() / b)
    return false;
  pairs = a * b;
  return true;
}

template <class T> exact_scalar exact_value(T value) {
  auto decoded = decode_coordinate(value, boolean_stage::geometry_realization);
  if (!decoded.has_value())
    throw std::invalid_argument("nonfinite approximate policy value");
  return decoded.value().value;
}

exact_scalar coefficient(const big_int &value) {
  return exact_scalar(value, big_uint(1));
}

exact_scalar plane_value(const exact_plane3 &plane, const exact_point3 &point) {
  return coefficient(plane.a) * point.x + coefficient(plane.b) * point.y +
         coefficient(plane.c) * point.z + coefficient(plane.d);
}

exact_scalar plane_normal_squared(const exact_plane3 &plane) {
  const auto a = coefficient(plane.a), b = coefficient(plane.b),
             c = coefficient(plane.c);
  return a.pow(2) + b.pow(2) + c.pow(2);
}

exact_scalar relation_residual(const exact_defining_relation_record &relation,
                               const exact_point3 &point) {
  return relation.coefficients[0] * point.x +
         relation.coefficients[1] * point.y +
         relation.coefficients[2] * point.z + relation.coefficients[3];
}

exact_scalar relation_normal_squared(
    const exact_defining_relation_record &relation) {
  return relation.coefficients[0].pow(2) +
         relation.coefficients[1].pow(2) +
         relation.coefficients[2].pow(2);
}

template <class T>
exact_point3 decode_point(const std::array<coordinate_bits<T>, 3> &bits) {
  std::array<exact_scalar, 3> values;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto decoded = decode_coordinate(bits[axis],
                                     boolean_stage::geometry_realization);
    if (!decoded.has_value())
      throw std::invalid_argument("nonfinite approximate candidate");
    values[axis] = decoded.value().value;
  }
  return {values[0], values[1], values[2]};
}

template <class T> struct axis_value {
  coordinate_bits<T> bits;
  exact_scalar value;
  exact_scalar error;
  std::uint32_t distance = 0;
  std::uint32_t rank = 0;
};

template <class T> struct point_value {
  std::array<coordinate_bits<T>, 3> bits;
  std::array<std::uint32_t, 3> axis_ranks{{0, 0, 0}};
  exact_point3 point;
  exact_scalar squared_error;
  std::uint32_t max_distance = 0;
  std::uint64_t sum_distance = 0;
  std::uint64_t rank = 0;
};

bool displacement_allowed(const exact_point3 &, const exact_point3 &,
                          const approximation_policy_contract &);

template <class T>
std::vector<axis_value<T>> symmetric_axis_candidates(
    const exact_scalar &target, std::uint32_t radius,
    const std::function<bool()> &cancelled,
    const std::function<bool()> &reserve_candidate, bool &limited) {
  if (cancelled && cancelled()) {
    limited = true;
    return {};
  }
  const auto nearest = round_binary_nearest_even<T>(target);
  if (!nearest)
    return {};
  std::vector<std::pair<coordinate_bits<T>, std::uint32_t>> raw;
  if (reserve_candidate && !reserve_candidate()) {
    limited = true;
    return {};
  }
  raw.push_back({*nearest, 0});
  auto lower = *nearest, upper = *nearest;
  for (std::uint32_t distance = 1; distance != 0; ++distance) {
    if (cancelled && cancelled()) {
      limited = true;
      return {};
    }
    if (auto next = predecessor_bits<T>(lower)) {
      lower = *next;
      if (reserve_candidate && !reserve_candidate()) {
        limited = true;
        return {};
      }
      raw.push_back({lower, distance});
    }
    if (auto next = successor_bits<T>(upper)) {
      upper = *next;
      if (reserve_candidate && !reserve_candidate()) {
        limited = true;
        return {};
      }
      raw.push_back({upper, distance});
    }
    if (distance == radius)
      break;
  }
  std::vector<axis_value<T>> out;
  for (const auto &entry : raw) {
    auto decoded = decode_coordinate(entry.first,
                                     boolean_stage::geometry_realization);
    if (decoded.has_value()) {
      if (reserve_candidate && !reserve_candidate()) {
        limited = true;
        return {};
      }
      out.push_back({entry.first, decoded.value().value,
                     (decoded.value().value - target).abs(), entry.second, 0});
    }
  }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.distance != b.distance)
      return a.distance < b.distance;
    if (a.error != b.error)
      return a.error < b.error;
    if (a.value != b.value)
      return a.value < b.value;
    return a.bits.bits < b.bits.bits;
  });
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i].rank = static_cast<std::uint32_t>(i);
  return out;
}

template <class T>
std::vector<point_value<T>> point_candidates(
    const exact_point3 &target, bool fixed_original,
    const approximation_policy_contract &policy, std::uint64_t cap,
    std::uint64_t &axis_count, bool &truncated,
    std::uint64_t &candidate_evaluations,
    std::uint64_t candidate_evaluation_limit,
    const std::function<bool()> &cancelled,
    const std::function<bool()> &reserve_candidate, bool &limited) {
  const std::array<exact_scalar, 3> targets{{target.x, target.y, target.z}};
  std::array<std::vector<axis_value<T>>, 3> axes;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    axes[axis] = symmetric_axis_candidates<T>(
        targets[axis], fixed_original ? 0 : policy.candidate_ulp_radius,
        cancelled, reserve_candidate, limited);
    if (limited)
      return {};
    if (axes[axis].size() >
        std::numeric_limits<std::uint64_t>::max() - axis_count) {
      limited = true;
      return {};
    }
    axis_count += axes[axis].size();
    if (axes[axis].empty())
      return {};
  }
  std::vector<point_value<T>> out;
  for (const auto &x : axes[0])
    for (const auto &y : axes[1])
      for (const auto &z : axes[2]) {
        if ((cancelled && cancelled()) ||
            candidate_evaluations == candidate_evaluation_limit ||
            candidate_evaluations ==
                std::numeric_limits<std::uint64_t>::max()) {
          limited = true;
          return out;
        }
        ++candidate_evaluations;
        point_value<T> candidate;
        candidate.bits = {{x.bits, y.bits, z.bits}};
        candidate.axis_ranks = {{x.rank, y.rank, z.rank}};
        candidate.point = {x.value, y.value, z.value};
        candidate.squared_error = x.error.pow(2) + y.error.pow(2) +
                                  z.error.pow(2);
        candidate.max_distance =
            std::max({x.distance, y.distance, z.distance});
        candidate.sum_distance = std::uint64_t(x.distance) + y.distance +
                                 z.distance;
        if (displacement_allowed(target, candidate.point, policy) &&
            (!fixed_original || candidate.point == target)) {
          if (reserve_candidate && !reserve_candidate()) {
            limited = true;
            return out;
          }
          out.push_back(std::move(candidate));
          if (out.size() > cap) {
            const auto worst = std::max_element(
                out.begin(), out.end(), [](const auto &a, const auto &b) {
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
            out.erase(worst);
            truncated = true;
          }
        }
      }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
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
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i].rank = i;
  return out;
}

bool displacement_allowed(const exact_point3 &target,
                          const exact_point3 &point,
                          const approximation_policy_contract &policy) {
  const exact_point3 d{point.x - target.x, point.y - target.y,
                       point.z - target.z};
  const auto global = exact_value(policy.max_vertex_displacement);
  if (global.pow(2) < d.x.pow(2) + d.y.pow(2) + d.z.pow(2))
    return false;
  const std::array<bool, 3> present{{policy.has_max_axis_displacement_x,
                                     policy.has_max_axis_displacement_y,
                                     policy.has_max_axis_displacement_z}};
  const std::array<double, 3> bounds{{policy.max_axis_displacement_x,
                                      policy.max_axis_displacement_y,
                                      policy.max_axis_displacement_z}};
  const std::array<exact_scalar, 3> delta{{d.x.abs(), d.y.abs(), d.z.abs()}};
  for (std::size_t axis = 0; axis < 3; ++axis)
    if (present[axis] && exact_value(bounds[axis]) < delta[axis])
      return false;
  return true;
}

template <class T, class I>
std::vector<std::uint8_t> output_bytes(const boolean_success<T, I> &output) {
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

template <class I>
std::uint64_t connected_face_components(
    const std::vector<std::vector<I>> &faces) {
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::vector<std::size_t>> uses;
  for (std::size_t face = 0; face < faces.size(); ++face)
    for (std::size_t edge = 0; edge < faces[face].size(); ++edge) {
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

void encode_point(canonical_encoder &encoder, const exact_point3 &point) {
  encode(encoder, point.x);
  encode(encoder, point.y);
  encode(encoder, point.z);
}

template <class T, class I>
std::vector<std::uint8_t> certificate_bytes(
    const certified_approximate_certificate<T, I> &certificate) {
  canonical_encoder e;
  e.u16(certificate.schema);
  e.byte(static_cast<std::uint8_t>(certificate.coordinate));
  e.byte(static_cast<std::uint8_t>(certificate.index));
  for (const auto *digest : {&certificate.exact_result_digest,
                             &certificate.selected_boundary_digest,
                             &certificate.policy_digest})
    e.raw(digest->bytes.data(), digest->bytes.size());
  e.u64(certificate.exact_vertex_occurrences);
  e.u64(certificate.exact_edges);
  e.u64(certificate.exact_halfedges);
  e.u64(certificate.exact_cycles);
  e.u64(certificate.exact_patches);
  e.u64(certificate.vertices.size());
  for (const auto &vertex : certificate.vertices) {
    e.u64(vertex.selected.value_for_debug());
    encode_point(e, vertex.exact_target);
    for (const auto bits : vertex.output_bits)
      e.u64(bits.bits);
    encode_point(e, vertex.displacement);
    encode(e, vertex.squared_displacement);
    for (const auto rank : vertex.accepted_axis_ranks)
      e.u32(rank);
    e.u64(vertex.accepted_candidate_rank);
    e.boolean(vertex.original_vertex);
  }
  e.u64(certificate.triangles.size());
  for (const auto &triangle : certificate.triangles) {
    e.u64(triangle.triangle.value_for_debug());
    e.u64(triangle.patch.value_for_debug());
    for (const auto vertex : triangle.vertices)
      e.u64(vertex.value_for_debug());
    e.byte(static_cast<std::uint8_t>(triangle.projection));
    e.byte(static_cast<std::uint8_t>(triangle.exact_orientation));
    for (const auto &deviation : triangle.support_plane_deviations)
      encode(e, deviation);
    for (const auto &deviation :
         triangle.normalized_squared_support_plane_deviations)
      encode(e, deviation);
  }
  e.u64(certificate.occurrence_maps.size());
  for (const auto &occurrence : certificate.occurrence_maps) {
    e.u64(occurrence.occurrence.value_for_debug());
    e.u64(occurrence.output_vertex.value_for_debug());
    e.u64(occurrence.exact_cyclic_halfedges.size());
    for (const auto halfedge : occurrence.exact_cyclic_halfedges)
      e.u64(halfedge.value_for_debug());
    e.u64(occurrence.output_cyclic_triangles.size());
    for (const auto triangle : occurrence.output_cyclic_triangles)
      e.u64(triangle.value_for_debug());
  }
  e.u64(certificate.patch_adjacency.size());
  for (const auto &adjacency : certificate.patch_adjacency) {
    e.u64(adjacency.edge.value_for_debug());
    e.u64(adjacency.exact_radial_patches.size());
    for (const auto patch : adjacency.exact_radial_patches)
      e.u64(patch.value_for_debug());
    e.u64(adjacency.output_incident_triangles.size());
    for (const auto triangle : adjacency.output_incident_triangles)
      e.u64(triangle.value_for_debug());
  }
  e.u64(certificate.obligations.size());
  for (const auto &obligation : certificate.obligations) {
    e.byte(static_cast<std::uint8_t>(obligation.kind));
    for (const auto *values : {&obligation.vertices, &obligation.triangles,
                               &obligation.exact_entities}) {
      e.u64(values->size());
      for (const auto value : *values)
        e.u64(value);
    }
    e.boolean(obligation.passed);
    encode(e, obligation.measured_value);
    encode(e, obligation.allowed_value);
  }
  e.u64(certificate.relaxed_relations.size());
  for (const auto &relation : certificate.relaxed_relations) {
    e.byte(static_cast<std::uint8_t>(relation.kind));
    e.u64(relation.exact_entity);
    e.byte(relation.axis);
    encode(e, relation.exact_value);
    encode(e, relation.emitted_value);
    encode(e, relation.absolute_deviation);
    encode(e, relation.allowed_deviation);
    e.boolean(relation.defining_relation.has_value());
    if (relation.defining_relation)
      e.u64(relation.defining_relation->value_for_debug());
    e.byte(static_cast<std::uint8_t>(relation.relation_kind));
    e.u16(relation.relation_formula_version);
    e.byte(static_cast<std::uint8_t>(relation.relation_expected));
    encode(e, relation.exact_residual);
    encode(e, relation.emitted_residual);
  }
  e.u64(certificate.defining_relations.size());
  for (const auto &relation : certificate.defining_relations) {
    e.u64(relation.vertex.value_for_debug());
    e.u64(relation.relation.value_for_debug());
    e.byte(static_cast<std::uint8_t>(relation.kind));
    e.u16(relation.formula_version);
    e.byte(static_cast<std::uint8_t>(relation.expected));
    encode(e, relation.exact_residual);
    encode(e, relation.emitted_residual);
    encode(e, relation.normalized_squared_residual_change);
    encode(e, relation.allowed_squared_residual_change);
    e.boolean(relation.exact);
    e.boolean(relation.passed);
  }
  e.u64(certificate.components.size());
  for (const auto &component : certificate.components) {
    e.u64(component.id);
    for (const auto *values : {&component.variables, &component.variable_order,
                                &component.obligations,
                                &component.accepted_ranks,
                               &component.rejected_prefix_witnesses}) {
      e.u64(values->size());
      for (const auto value : *values)
        e.u64(value);
    }
    e.u64(component.visited_nodes);
    e.u64(component.complete_assignments);
    e.raw(component.graph_digest.bytes.data(), 16);
    e.raw(component.transcript_digest.bytes.data(), 16);
  }
  const auto &search = certificate.search;
  e.u16(search.candidate_generation_version);
  e.u32(search.candidate_ulp_radius);
  e.u64(search.candidate_cap);
  e.u64(search.candidate_evaluation_limit);
  e.u64(search.candidate_evaluations);
  e.u64(search.search_node_limit);
  e.u64(search.obligation_limit);
  e.u64(search.triangle_pair_limit);
  e.u64(search.predicate_check_limit);
  e.u64(search.predicate_checks);
  e.u64(search.verifier_work_limit);
  e.u64(search.verifier_record_limit);
  e.u64(search.verifier_byte_limit);
  e.u64(search.generated_axis_candidates);
  e.u64(search.generated_point_candidates);
  e.u64(search.visited_nodes);
  e.u64(search.complete_assignments);
  e.u64(search.accepted_candidate_ranks.size());
  for (const auto rank : search.accepted_candidate_ranks)
    e.u64(rank);
  e.raw(search.candidate_domain_digest.bytes.data(), 16);
  e.raw(search.transcript_digest.bytes.data(), 16);
  encode(e, certificate.maximum_squared_vertex_displacement);
  for (const auto &value : certificate.maximum_axis_displacements)
    encode(e, value);
  encode(e, certificate.maximum_support_plane_deviation);
  encode(e, certificate.maximum_normalized_squared_support_plane_deviation);
  return e.bytes();
}

product_error approximation_error(product_error_code code, const char *key,
                                  std::uint32_t subcode = 0) {
  return make_product_error(code, key, subcode);
}

product_error convert_error(const boolean_error &source) {
  auto result = make_product_error(promote_error_code(source.code),
                                   source.message_key);
  result.subcode = source.subcode;
  result.detail = render_error(source);
  canonical_encoder replay;
  replay.raw(source.replay.setup_digest.bytes.data(),
             source.replay.setup_digest.bytes.size());
  replay.byte_string(source.replay_payload);
  result.replay_binding_digest = domain_digest(
      {{'Y', 'G', 'B', 'A', 'F', 'A', 'I', 'L'}}, replay.bytes());
  return result;
}

} // namespace

template <class T, class I>
void canonicalize_certified_approximate_certificate(
    certified_approximate_certificate<T, I> &certificate) {
  certificate.canonical_bytes = certificate_bytes(certificate);
  certificate.certificate_digest = domain_digest(
      {{'Y', 'G', 'B', 'A', 'C', 'E', 'R', '1'}},
      certificate.canonical_bytes);
}

template <class T, class I>
product_status_or<certified_mesh_payload<T, I>>
realize_certified_approximate_embedding(
    boolean_context<T, I> &context, const exact_result_handle &exact,
    const backend_identity &backend, const product_realization_policy &policy) {
  try {
    boolean_product_options validation_options;
    validation_options.result.representation =
        result_representation::certified_approximate_mesh;
    validation_options.realization = policy;
    auto valid_policy = validate_product_options(validation_options);
    if (!valid_policy.has_value())
      return valid_policy.error();
    if (!backend.capabilities.has(
            backend_capability::certified_approximate_output))
      return approximation_error(product_error_code::backend_capability_mismatch,
                                 "approximate.backend_capability");
    if (context.cancelled())
      return approximation_error(product_error_code::resource_limit,
                                 "approximate.cancelled");
    auto exact_boundary = read_exact_result(exact);
    if (!exact_boundary.has_value())
      return exact_boundary.error();
    if (exact_boundary.value()->topology ==
        selected_boundary_topology::closed_stratified_nonmanifold)
      return approximation_error(product_error_code::result_topology_not_supported,
                                 "approximate.nonmanifold_mesh");
    std::vector<resource_reservation> resource_charges;
    std::optional<product_error> resource_failure;
    auto reserve_resource = [&](resource_kind kind, std::uint64_t amount) {
      auto charge = context.accountant().reserve_scoped(
          kind, amount, boolean_stage::geometry_realization);
      if (!charge.has_value()) {
        resource_failure = convert_error(charge.error());
        return false;
      }
      resource_charges.push_back(std::move(charge.value()));
      return true;
    };
    auto selected_artifact = select_boolean_boundary(context);
    if (!selected_artifact.has_value())
      return convert_error(selected_artifact.error());
    const auto &selected = *selected_artifact.value()->payload;
    if (selected.artifact_digest !=
        exact_boundary.value()->selected_artifact_digest)
      return approximation_error(product_error_code::stale_binding,
                                 "approximate.exact_selection_binding");
    auto triangulated = triangulate_selected_boundary_for_realization(selected);
    if (!triangulated.has_value())
      return convert_error(triangulated.error());

    using candidate_list = std::vector<point_value<T>>;
    std::vector<candidate_list> domains;
    domains.reserve(exact_boundary.value()->vertices.size());
    std::uint64_t axis_candidates = 0, point_candidate_count = 0,
                  candidate_evaluations = 0;
    bool candidate_cap_truncated = false;
    bool candidate_generation_limited = false;
    canonical_encoder domain_encoding;
    const char domain_tag[] = "YGBADOM1";
    domain_encoding.raw(reinterpret_cast<const std::uint8_t *>(domain_tag), 8);
    domain_encoding.u16(1);
    domain_encoding.u64(exact_boundary.value()->vertices.size());
    for (const auto &vertex : exact_boundary.value()->vertices) {
      const bool fixed_original = !vertex.original_vertices.empty() &&
                                  !policy.approximation
                                       .allow_original_vertex_movement;
      std::vector<point_value<T>> candidates;
      if (fixed_original) {
        if (context.cancelled() ||
            candidate_evaluations ==
                policy.search.max_candidate_evaluations)
          return approximation_error(product_error_code::resource_limit,
                                     context.cancelled()
                                         ? "approximate.cancelled"
                                         : "approximate.candidate_evaluation_limit");
        ++candidate_evaluations;
        axis_candidates += 3;
        if (vertex.original_raw_bits.empty() ||
            vertex.original_raw_bits.front().coordinate !=
                exact_result_coordinate_type<T>())
          return approximation_error(product_error_code::stale_binding,
                                      "approximate.original_raw_bits_missing");
        if (std::any_of(
                vertex.original_raw_bits.begin() + 1,
                vertex.original_raw_bits.end(), [&](const auto &raw) {
                  return raw.coordinate !=
                             vertex.original_raw_bits.front().coordinate ||
                         raw.bits != vertex.original_raw_bits.front().bits;
                }))
          return approximation_error(
              product_error_code::output_not_representable,
              "approximate.conflicting_original_raw_bits");
        point_value<T> fixed;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          if constexpr (std::is_same<T, float>::value)
            fixed.bits[axis].bits = static_cast<std::uint32_t>(
                vertex.original_raw_bits.front().bits[axis]);
          else
            fixed.bits[axis].bits =
                vertex.original_raw_bits.front().bits[axis];
        }
        fixed.point = decode_point(fixed.bits);
        if (!(fixed.point == vertex.coordinate))
          return approximation_error(product_error_code::stale_binding,
                                      "approximate.original_raw_bits_value");
        if (!reserve_resource(resource_kind::candidates, 1))
          return *resource_failure;
        candidates = {std::move(fixed)};
      } else {
        candidates = point_candidates<T>(
            vertex.coordinate, false, policy.approximation,
            policy.search.max_candidates, axis_candidates,
            candidate_cap_truncated, candidate_evaluations,
            policy.search.max_candidate_evaluations,
            [&] { return context.cancelled(); },
            [&] { return reserve_resource(resource_kind::candidates, 1); },
            candidate_generation_limited);
        if (candidate_generation_limited)
          return resource_failure
                     ? *resource_failure
                     : approximation_error(
                           product_error_code::resource_limit,
                           context.cancelled()
                               ? "approximate.cancelled"
                               : "approximate.candidate_evaluation_limit");
      }
      if (candidates.empty())
        return approximation_error(product_error_code::output_not_representable,
                                   "approximate.completed_search_exhaustion",
                                   approximate_search_exhausted_subcode);
      if (!checked_add(point_candidate_count, candidates.size(),
                       std::numeric_limits<std::uint64_t>::max()))
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.candidate_count_overflow");
      domain_encoding.u64(candidates.size());
      for (const auto &candidate : candidates)
        for (const auto bits : candidate.bits)
          domain_encoding.u64(bits.bits);
      domains.push_back(std::move(candidates));
    }

    const auto &triangles = triangulated.value();
    std::uint64_t triangle_pairs = 0;
    if (!checked_pairs(triangles.size(), triangle_pairs) ||
        triangle_pairs > policy.search.max_triangle_pairs)
      return approximation_error(product_error_code::resource_limit,
                                 "approximate.triangle_pair_limit");
    if (!reserve_resource(resource_kind::realization_pair_checks,
                          triangle_pairs))
      return *resource_failure;
    const auto vertex_count = exact_boundary.value()->vertices.size();
    std::vector<std::vector<defining_relation_id>> vertex_relations(vertex_count);
    for (std::size_t vertex = 0; vertex < vertex_count; ++vertex) {
      std::set<construction_node_id> visited;
      std::vector<construction_node_id> pending =
          exact_boundary.value()->vertices[vertex].constructions;
      std::set<defining_relation_id> relations;
      while (!pending.empty()) {
        if (context.cancelled())
          return approximation_error(product_error_code::resource_limit,
                                     "approximate.cancelled");
        const auto construction_id = pending.back();
        pending.pop_back();
        if (!visited.insert(construction_id).second)
          continue;
        if (construction_id.value_for_debug() >=
            exact_boundary.value()->constructions.size())
          return approximation_error(product_error_code::stale_binding,
                                     "approximate.construction_range");
        const auto &construction = exact_boundary.value()->constructions[
            construction_id.value_for_debug()];
        pending.insert(pending.end(), construction.children.begin(),
                       construction.children.end());
        relations.insert(construction.defining_relations.begin(),
                         construction.defining_relations.end());
      }
      vertex_relations[vertex] = {relations.begin(), relations.end()};
    }
    std::uint64_t relation_count = 0;
    for (const auto &relations : vertex_relations)
      if (!checked_add(relation_count, relations.size(),
                       std::numeric_limits<std::uint64_t>::max()))
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.relation_count_overflow");
    std::uint64_t vertex_pairs = 0, obligation_count = 2;
    if (!checked_pairs(vertex_count, vertex_pairs) ||
        !checked_add(obligation_count, vertex_pairs,
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, relation_count,
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, exact_boundary.value()->edges.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, exact_boundary.value()->edges.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, exact_boundary.value()->edges.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count,
                     exact_boundary.value()->vertex_occurrences.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, triangles.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, triangles.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, triangles.size(),
                     policy.search.max_obligations) ||
        !checked_add(obligation_count, triangle_pairs,
                     policy.search.max_obligations))
      return approximation_error(product_error_code::resource_limit,
                                 "approximate.obligation_limit");
    std::uint64_t record_bound = 0;
    auto add_record_bound = [&](std::uint64_t count) {
      return checked_add(record_bound, count,
                         policy.search.max_verifier_records);
    };
    if (!add_record_bound(vertex_count) ||
        !add_record_bound(triangles.size()) ||
        !add_record_bound(exact_boundary.value()->vertex_occurrences.size()) ||
        !add_record_bound(exact_boundary.value()->edges.size()) ||
        !add_record_bound(obligation_count) ||
        !add_record_bound(relation_count) ||
        !add_record_bound(point_candidate_count))
      return approximation_error(product_error_code::resource_limit,
                                 "approximate.record_limit");
    for (const auto &occurrence : exact_boundary.value()->vertex_occurrences)
      if (!add_record_bound(occurrence.incident_halfedges.size()))
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.record_limit");
    for (const auto &edge : exact_boundary.value()->edges)
      if (!add_record_bound(edge.uses.size()))
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.record_limit");
    std::vector<approximate_obligation_evidence> obligations;
    obligations.reserve(static_cast<std::size_t>(obligation_count));
    if (!reserve_resource(resource_kind::realization_graph_nodes, vertex_count) ||
        !reserve_resource(resource_kind::realization_graph_edges,
                          obligation_count) ||
        !reserve_resource(resource_kind::evidence_records, obligation_count))
      return *resource_failure;
    auto add_obligation = [&](approximate_obligation_kind kind,
                              std::vector<std::uint64_t> vertices,
                              std::vector<std::uint64_t> triangle_ids = {},
                              std::vector<std::uint64_t> entities = {}) {
      approximate_obligation_evidence obligation;
      obligation.kind = kind;
      obligation.vertices = std::move(vertices);
      obligation.triangles = std::move(triangle_ids);
      obligation.exact_entities = std::move(entities);
      obligations.push_back(std::move(obligation));
    };
    std::vector<std::uint64_t> all_vertices(vertex_count);
    for (std::size_t i = 0; i < vertex_count; ++i)
      all_vertices[i] = i;
    add_obligation(approximate_obligation_kind::occurrence_isomorphic_topology,
                   all_vertices);
    for (std::size_t i = 0; i < vertex_count; ++i)
      for (std::size_t j = i + 1; j < vertex_count; ++j) {
        if (context.cancelled())
          return approximation_error(product_error_code::resource_limit,
                                     "approximate.cancelled");
        add_obligation(approximate_obligation_kind::distinct_required_vertices,
                       {i, j});
      }
    for (std::size_t vertex = 0; vertex < vertex_relations.size(); ++vertex)
      for (const auto relation : vertex_relations[vertex])
        add_obligation(approximate_obligation_kind::defining_relation,
                       {vertex}, {}, {relation.value_for_debug()});
    for (const auto &edge : exact_boundary.value()->edges) {
      add_obligation(approximate_obligation_kind::selected_edge_order,
                     {edge.lower.value_for_debug(),
                      edge.upper.value_for_debug()},
                     {}, {edge.id.value_for_debug()});
      add_obligation(approximate_obligation_kind::required_incidence,
                     {edge.lower.value_for_debug(),
                      edge.upper.value_for_debug()},
                     {}, {edge.id.value_for_debug()});
      add_obligation(approximate_obligation_kind::edge_radial_order,
                     {edge.lower.value_for_debug(),
                      edge.upper.value_for_debug()},
                     {}, {edge.id.value_for_debug()});
    }
    for (const auto &occurrence : exact_boundary.value()->vertex_occurrences)
      add_obligation(approximate_obligation_kind::vertex_link_order,
                     {occurrence.vertex.value_for_debug()}, {},
                     {occurrence.id.value_for_debug()});
    for (const auto &triangle : triangles) {
      std::vector<std::uint64_t> variables;
      for (const auto vertex : triangle.vertices)
        variables.push_back(vertex.value_for_debug());
      add_obligation(approximate_obligation_kind::triangle_noncollapse,
                     variables, {triangle.id.value_for_debug()},
                     {triangle.patch.value_for_debug()});
      add_obligation(approximate_obligation_kind::triangle_orientation,
                     variables, {triangle.id.value_for_debug()},
                     {triangle.patch.value_for_debug()});
      add_obligation(approximate_obligation_kind::support_plane_deviation,
                     variables, {triangle.id.value_for_debug()},
                     {triangle.patch.value_for_debug()});
    }
    for (std::size_t i = 0; i < triangles.size(); ++i)
      for (std::size_t j = i + 1; j < triangles.size(); ++j) {
        if (context.cancelled())
          return approximation_error(product_error_code::resource_limit,
                                     "approximate.cancelled");
        std::set<std::uint64_t> variables;
        for (const auto vertex : triangles[i].vertices)
          variables.insert(vertex.value_for_debug());
        for (const auto vertex : triangles[j].vertices)
          variables.insert(vertex.value_for_debug());
        add_obligation(
            approximate_obligation_kind::prohibited_nonadjacent_intersection,
            {variables.begin(), variables.end()}, {i, j});
      }
    add_obligation(approximate_obligation_kind::global_embedding, all_vertices);

    std::map<std::uint64_t, exact_plane3> patch_planes;
    for (const auto &geometry : exact_boundary.value()->patch_geometry)
      patch_planes.emplace(geometry.patch.value_for_debug(), geometry.plane);
    std::uint64_t predicate_checks = 0;
    bool predicate_limited = false;
    auto evaluate = [&](std::uint64_t obligation_id,
                        const std::vector<std::pair<std::uint64_t,
                                                    std::uint64_t>> &assignment) {
      if (context.cancelled() ||
          !checked_add(predicate_checks, 1,
                       policy.search.max_predicate_checks)) {
        predicate_limited = true;
        return false;
      }
      const auto &obligation = obligations.at(obligation_id);
      std::map<std::uint64_t, exact_point3> points;
      for (const auto &entry : assignment) {
        if (entry.first >= domains.size() ||
            entry.second >= domains[entry.first].size())
          return false;
        points.emplace(entry.first, domains[entry.first][entry.second].point);
      }
      auto point = [&](std::uint64_t id) -> const exact_point3 & {
        return points.at(id);
      };
      switch (obligation.kind) {
      case approximate_obligation_kind::distinct_required_vertices:
      case approximate_obligation_kind::selected_edge_order:
      case approximate_obligation_kind::required_incidence:
      case approximate_obligation_kind::edge_radial_order:
        return !(point(obligation.vertices[0]) ==
                 point(obligation.vertices[1]));
      case approximate_obligation_kind::triangle_noncollapse:
      case approximate_obligation_kind::triangle_orientation: {
        const auto &triangle = triangles.at(obligation.triangles.front());
        return orient2d(project(point(triangle.vertices[0].value_for_debug()),
                                triangle.projection),
                        project(point(triangle.vertices[1].value_for_debug()),
                                triangle.projection),
                        project(point(triangle.vertices[2].value_for_debug()),
                                triangle.projection)) ==
               triangle.exact_orientation;
      }
      case approximate_obligation_kind::support_plane_deviation: {
        const auto &triangle = triangles.at(obligation.triangles.front());
        const auto found = patch_planes.find(triangle.patch.value_for_debug());
        if (found == patch_planes.end())
          return false;
        const auto bound =
            exact_value(policy.approximation.max_support_plane_deviation);
        const auto allowed = bound.pow(2) * plane_normal_squared(found->second);
        for (const auto vertex : triangle.vertices)
          if (allowed < plane_value(found->second,
                                    point(vertex.value_for_debug())).pow(2))
            return false;
        return true;
      }
      case approximate_obligation_kind::defining_relation: {
        const auto relation_id = obligation.exact_entities.front();
        if (relation_id >= exact_boundary.value()->defining_relations.size())
          return false;
        const auto &relation =
            exact_boundary.value()->defining_relations[relation_id];
        const auto vertex = obligation.vertices.front();
        const auto exact_residual = relation_residual(
            relation, exact_boundary.value()->vertices[vertex].coordinate);
        const auto emitted_residual = relation_residual(relation, point(vertex));
        const auto change = emitted_residual - exact_residual;
        const auto normal = relation_normal_squared(relation);
        if (normal.is_zero())
          return change.is_zero();
        const auto global =
            exact_value(policy.approximation.max_vertex_displacement);
        if (global.pow(2) * normal < change.pow(2))
          return false;
        return relation.expected == exact_sign::zero ||
               emitted_residual.sign() == relation.expected;
      }
      case approximate_obligation_kind::prohibited_nonadjacent_intersection: {
        const auto &a = triangles.at(obligation.triangles[0]);
        const auto &b = triangles.at(obligation.triangles[1]);
        std::size_t shared = 0;
        for (const auto av : a.vertices)
          for (const auto bv : b.vertices)
            shared += av == bv;
        const auto relation = relate_triangles(
            {point(a.vertices[0].value_for_debug()),
             point(a.vertices[1].value_for_debug()),
             point(a.vertices[2].value_for_debug())},
            {point(b.vertices[0].value_for_debug()),
             point(b.vertices[1].value_for_debug()),
             point(b.vertices[2].value_for_debug())});
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

    std::vector<detail::realization_solver_variable> variables;
    for (std::size_t i = 0; i < domains.size(); ++i)
      variables.push_back({i, domains[i].size()});
    std::vector<detail::realization_solver_constraint> constraints;
    for (std::size_t i = 0; i < obligations.size(); ++i)
      constraints.push_back({i, obligations[i].vertices});
    auto solved = detail::solve_realization_constraint_components(
        std::move(variables), std::move(constraints), evaluate,
        policy.search.max_search_nodes, std::numeric_limits<std::uint64_t>::max(),
        std::numeric_limits<std::uint64_t>::max(),
        [&] { return context.cancelled(); }, &context.executor());
    if (context.cancelled() || solved.limited || predicate_limited)
      return approximation_error(product_error_code::resource_limit,
                                 "approximate.solver_limit");
    if (!solved.accepted)
      return approximation_error(product_error_code::output_not_representable,
                                 "approximate.completed_search_exhaustion",
                                 approximate_search_exhausted_subcode);
    std::vector<std::uint64_t> accepted(domains.size());
    for (const auto &component : solved.components)
      for (std::size_t i = 0; i < component.variables.size(); ++i)
        accepted[component.variables[i]] = component.accepted_ranks[i];

    auto certificate =
        std::make_shared<certified_approximate_certificate<T, I>>();
    certificate->exact_result_digest = exact->canonical_digest;
    certificate->selected_boundary_digest = selected.artifact_digest;
    certificate->policy_digest = approximate_realization_policy_digest(policy);
    certificate->policy = policy;
    certificate->exact_vertex_occurrences =
        exact_boundary.value()->vertex_occurrences.size();
    certificate->exact_edges = exact_boundary.value()->edges.size();
    certificate->exact_halfedges = exact_boundary.value()->halfedges.size();
    certificate->exact_cycles = exact_boundary.value()->cycles.size();
    certificate->exact_patches = exact_boundary.value()->patches.size();
    certificate->search.candidate_generation_version =
        policy.approximation.candidate_generation_version;
    certificate->search.candidate_ulp_radius =
        policy.approximation.candidate_ulp_radius;
    certificate->search.candidate_cap = policy.search.max_candidates;
    certificate->search.candidate_evaluation_limit =
        policy.search.max_candidate_evaluations;
    certificate->search.candidate_evaluations = candidate_evaluations;
    certificate->search.search_node_limit = policy.search.max_search_nodes;
    certificate->search.obligation_limit = policy.search.max_obligations;
    certificate->search.triangle_pair_limit = policy.search.max_triangle_pairs;
    certificate->search.predicate_check_limit =
        policy.search.max_predicate_checks;
    certificate->search.verifier_work_limit = policy.search.max_verifier_work;
    certificate->search.verifier_record_limit =
        policy.search.max_verifier_records;
    certificate->search.verifier_byte_limit = policy.search.max_verifier_bytes;
    certificate->search.generated_axis_candidates = axis_candidates;
    certificate->search.generated_point_candidates = point_candidate_count;
    certificate->search.visited_nodes = solved.visited_nodes;
    certificate->search.complete_assignments = solved.complete_assignments;
    certificate->search.accepted_candidate_ranks = accepted;
    certificate->search.candidate_domain_digest = domain_digest(
        {{'Y', 'G', 'B', 'A', 'D', 'O', 'M', '1'}}, domain_encoding.bytes());

    auto success = std::make_shared<boolean_success<T, I>>();
    success->selected_operation = exact_boundary.value()->selected_operation;
    success->selected_boundary_digest = selected.artifact_digest;
    success->mesh.vertices.reserve(domains.size());
    exact_scalar maximum_squared(0);
    std::array<exact_scalar, 3> maximum_axis;
    for (std::size_t i = 0; i < domains.size(); ++i) {
      const auto &candidate = domains[i].at(accepted[i]);
      const auto &target = exact_boundary.value()->vertices[i].coordinate;
      approximate_vertex_evidence<T> evidence;
      evidence.selected = selected_vertex_id::from_canonical_value(i);
      evidence.exact_target = target;
      evidence.output_bits = candidate.bits;
      evidence.displacement = {candidate.point.x - target.x,
                               candidate.point.y - target.y,
                               candidate.point.z - target.z};
      evidence.squared_displacement = candidate.squared_error;
      evidence.accepted_axis_ranks = candidate.axis_ranks;
      evidence.accepted_candidate_rank = accepted[i];
      evidence.original_vertex =
          !exact_boundary.value()->vertices[i].original_vertices.empty();
      maximum_squared = std::max(maximum_squared,
                                 evidence.squared_displacement);
      const std::array<exact_scalar, 3> displacement{
          {evidence.displacement.x.abs(), evidence.displacement.y.abs(),
           evidence.displacement.z.abs()}};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        maximum_axis[axis] = std::max(maximum_axis[axis], displacement[axis]);
        if (!displacement[axis].is_zero()) {
          approximate_relaxed_relation relation;
          relation.kind = evidence.original_vertex
                              ? approximate_relaxed_relation_kind::
                                    original_vertex_coordinate
                              : approximate_relaxed_relation_kind::
                                    target_coordinate;
          relation.exact_entity = i;
          relation.axis = static_cast<std::uint8_t>(axis);
          relation.exact_value =
              axis == 0 ? target.x : axis == 1 ? target.y : target.z;
          relation.emitted_value = axis == 0   ? candidate.point.x
                                   : axis == 1 ? candidate.point.y
                                               : candidate.point.z;
          relation.absolute_deviation = displacement[axis];
          relation.allowed_deviation = exact_value(
              axis == 0 && policy.approximation.has_max_axis_displacement_x
                  ? policy.approximation.max_axis_displacement_x
                  : axis == 1 && policy.approximation.has_max_axis_displacement_y
                        ? policy.approximation.max_axis_displacement_y
                        : axis == 2 &&
                                  policy.approximation.has_max_axis_displacement_z
                              ? policy.approximation.max_axis_displacement_z
                              : policy.approximation.max_vertex_displacement);
          certificate->relaxed_relations.push_back(std::move(relation));
        }
      }
      success->mesh.vertices.push_back(
          {value_of_bits(candidate.bits[0]), value_of_bits(candidate.bits[1]),
           value_of_bits(candidate.bits[2])});
      certificate->vertices.push_back(std::move(evidence));
    }
    certificate->maximum_squared_vertex_displacement = maximum_squared;
    certificate->maximum_axis_displacements = maximum_axis;

    exact_scalar maximum_plane(0);
    exact_scalar maximum_normalized_plane(0);
    for (const auto &triangle : triangles) {
      if (triangle.vertices[0].value_for_debug() >
              std::numeric_limits<I>::max() ||
          triangle.vertices[1].value_for_debug() >
              std::numeric_limits<I>::max() ||
          triangle.vertices[2].value_for_debug() >
              std::numeric_limits<I>::max())
        return approximation_error(product_error_code::index_overflow,
                                   "approximate.index_overflow");
      success->mesh.faces.push_back(
          {static_cast<I>(triangle.vertices[0].value_for_debug()),
           static_cast<I>(triangle.vertices[1].value_for_debug()),
           static_cast<I>(triangle.vertices[2].value_for_debug())});
      approximate_triangle_evidence evidence;
      evidence.triangle = triangle.id;
      evidence.patch = triangle.patch;
      evidence.vertices =
          {{selected_vertex_id::from_canonical_value(
                triangle.vertices[0].value_for_debug()),
            selected_vertex_id::from_canonical_value(
                triangle.vertices[1].value_for_debug()),
            selected_vertex_id::from_canonical_value(
                triangle.vertices[2].value_for_debug())}};
      evidence.projection = triangle.projection;
      evidence.exact_orientation = triangle.exact_orientation;
      const auto &plane = patch_planes.at(triangle.patch.value_for_debug());
      const auto normal = plane_normal_squared(plane);
      for (std::size_t n = 0; n < 3; ++n) {
        const auto point = decode_point(certificate
                                            ->vertices[triangle.vertices[n]
                                                           .value_for_debug()]
                                            .output_bits);
        evidence.support_plane_deviations[n] = plane_value(plane, point).abs();
        evidence.normalized_squared_support_plane_deviations[n] =
            evidence.support_plane_deviations[n].pow(2) / normal;
        maximum_plane = std::max(maximum_plane,
                                 evidence.support_plane_deviations[n]);
        maximum_normalized_plane = std::max(
            maximum_normalized_plane,
            evidence.normalized_squared_support_plane_deviations[n]);
        if (!evidence.support_plane_deviations[n].is_zero()) {
          approximate_relaxed_relation relation;
          relation.kind = approximate_relaxed_relation_kind::support_plane;
          relation.exact_entity = triangle.id.value_for_debug();
          relation.axis = static_cast<std::uint8_t>(n);
          relation.absolute_deviation =
              evidence.support_plane_deviations[n].pow(2);
          const auto bound =
              exact_value(policy.approximation.max_support_plane_deviation);
          relation.allowed_deviation =
              bound.pow(2) * plane_normal_squared(plane);
          certificate->relaxed_relations.push_back(std::move(relation));
        }
      }
      certificate->triangles.push_back(std::move(evidence));
    }
    certificate->maximum_support_plane_deviation = maximum_plane;
    certificate->maximum_normalized_squared_support_plane_deviation =
        maximum_normalized_plane;
    const auto global_relation_bound =
        exact_value(policy.approximation.max_vertex_displacement).pow(2);
    for (std::size_t vertex = 0; vertex < vertex_relations.size(); ++vertex) {
      const auto emitted = decode_point(certificate->vertices[vertex].output_bits);
      for (const auto relation_id : vertex_relations[vertex]) {
        if (relation_id.value_for_debug() >=
            exact_boundary.value()->defining_relations.size())
          return approximation_error(product_error_code::stale_binding,
                                     "approximate.relation_range");
        const auto &relation = exact_boundary.value()->defining_relations[
            relation_id.value_for_debug()];
        approximate_defining_relation_evidence evidence;
        evidence.vertex = selected_vertex_id::from_canonical_value(vertex);
        evidence.relation = relation.id;
        evidence.kind = relation.kind;
        evidence.formula_version = relation.formula_version;
        evidence.expected = relation.expected;
        evidence.exact_residual = relation_residual(
            relation, exact_boundary.value()->vertices[vertex].coordinate);
        evidence.emitted_residual = relation_residual(relation, emitted);
        const auto change = evidence.emitted_residual - evidence.exact_residual;
        const auto normal = relation_normal_squared(relation);
        evidence.normalized_squared_residual_change =
            normal.is_zero() ? change.pow(2) : change.pow(2) / normal;
        evidence.allowed_squared_residual_change = global_relation_bound;
        evidence.exact = change.is_zero();
        evidence.passed =
            !(evidence.allowed_squared_residual_change <
              evidence.normalized_squared_residual_change) &&
            (relation.expected == exact_sign::zero ||
             evidence.emitted_residual.sign() == relation.expected);
        if (!evidence.passed)
          return approximation_error(product_error_code::internal_invariant_error,
                                     "approximate.relation_assignment");
        if (!evidence.exact) {
          approximate_relaxed_relation relaxed;
          relaxed.kind = approximate_relaxed_relation_kind::defining_relation;
          relaxed.exact_entity = vertex;
          relaxed.absolute_deviation =
              evidence.normalized_squared_residual_change;
          relaxed.allowed_deviation = evidence.allowed_squared_residual_change;
          relaxed.defining_relation = relation.id;
          relaxed.relation_kind = relation.kind;
          relaxed.relation_formula_version = relation.formula_version;
          relaxed.relation_expected = relation.expected;
          relaxed.exact_residual = evidence.exact_residual;
          relaxed.emitted_residual = evidence.emitted_residual;
          certificate->relaxed_relations.push_back(std::move(relaxed));
        }
        certificate->defining_relations.push_back(std::move(evidence));
      }
    }
    auto triangle_for_halfedge = [&](const selected_halfedge &halfedge)
        -> std::optional<realization_triangle_id> {
      for (const auto &triangle : certificate->triangles) {
        if (triangle.patch != halfedge.patch)
          continue;
        for (std::size_t edge = 0; edge < 3; ++edge)
          if (triangle.vertices[edge] == halfedge.origin &&
              triangle.vertices[(edge + 1) % 3] == halfedge.destination)
            return triangle.triangle;
      }
      return {};
    };
    for (const auto &occurrence : exact_boundary.value()->vertex_occurrences) {
      if (context.cancelled())
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.cancelled");
      approximate_occurrence_map mapping;
      mapping.occurrence = occurrence.id;
      mapping.output_vertex = occurrence.vertex;
      mapping.exact_cyclic_halfedges = occurrence.incident_halfedges;
      if (occurrence.incident_halfedges.empty())
        return approximation_error(product_error_code::internal_invariant_error,
                                   "approximate.empty_occurrence_link");
      const auto first_halfedge = occurrence.incident_halfedges.front();
      if (first_halfedge.value_for_debug() >=
          exact_boundary.value()->halfedges.size())
        return approximation_error(product_error_code::internal_invariant_error,
                                   "approximate.occurrence_halfedge_range");
      const auto &anchor_halfedge = exact_boundary.value()
                                        ->halfedges[first_halfedge.value_for_debug()];
      const auto start = anchor_halfedge.origin == occurrence.vertex
                             ? anchor_halfedge.destination
                             : anchor_halfedge.origin;
      std::map<selected_vertex_id,
               std::pair<selected_vertex_id, realization_triangle_id>>
          link_successor;
      for (const auto &triangle : certificate->triangles)
        for (std::size_t vertex = 0; vertex < 3; ++vertex)
          if (triangle.vertices[vertex] == occurrence.vertex) {
            const auto next = triangle.vertices[(vertex + 1) % 3];
            const auto previous = triangle.vertices[(vertex + 2) % 3];
            if (!link_successor.emplace(next,
                                        std::make_pair(previous,
                                                       triangle.triangle))
                     .second)
              return approximation_error(
                  product_error_code::internal_invariant_error,
                  "approximate.nonmanifold_output_link");
          }
      auto current = start;
      std::set<selected_vertex_id> visited_link;
      while (visited_link.insert(current).second) {
        if (context.cancelled())
          return approximation_error(product_error_code::resource_limit,
                                     "approximate.cancelled");
        const auto successor = link_successor.find(current);
        if (successor == link_successor.end())
          return approximation_error(product_error_code::internal_invariant_error,
                                     "approximate.open_output_link");
        mapping.output_cyclic_triangles.push_back(successor->second.second);
        current = successor->second.first;
      }
      if (current != start || visited_link.size() != link_successor.size())
        return approximation_error(product_error_code::internal_invariant_error,
                                   "approximate.disconnected_output_link");
      certificate->occurrence_maps.push_back(std::move(mapping));
    }
    for (const auto &edge : exact_boundary.value()->edges) {
      if (context.cancelled())
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.cancelled");
      approximate_patch_adjacency adjacency;
      adjacency.edge = edge.id;
      for (const auto halfedge_id : edge.uses) {
        if (halfedge_id.value_for_debug() >=
            exact_boundary.value()->halfedges.size())
          return approximation_error(product_error_code::internal_invariant_error,
                                     "approximate.radial_halfedge_range");
        const auto &halfedge =
            exact_boundary.value()->halfedges[halfedge_id.value_for_debug()];
        adjacency.exact_radial_patches.push_back(halfedge.patch);
        auto triangle = triangle_for_halfedge(halfedge);
        if (!triangle)
          return approximation_error(product_error_code::internal_invariant_error,
                                     "approximate.radial_triangle");
        adjacency.output_incident_triangles.push_back(*triangle);
      }
      certificate->patch_adjacency.push_back(std::move(adjacency));
    }
    for (std::size_t i = 0; i < obligations.size(); ++i) {
      std::vector<std::pair<std::uint64_t, std::uint64_t>> assignment;
      for (const auto variable : obligations[i].vertices)
        assignment.push_back({variable, accepted[variable]});
      obligations[i].passed = evaluate(i, assignment);
      if (predicate_limited)
        return approximation_error(product_error_code::resource_limit,
                                   "approximate.predicate_check_limit");
      if (obligations[i].kind ==
          approximate_obligation_kind::support_plane_deviation) {
        const auto triangle = obligations[i].triangles.front();
        for (const auto &value : certificate->triangles[triangle]
                                     .normalized_squared_support_plane_deviations)
          obligations[i].measured_value =
              std::max(obligations[i].measured_value, value);
        obligations[i].allowed_value =
            exact_value(policy.approximation.max_support_plane_deviation).pow(2);
      } else if (obligations[i].kind ==
                 approximate_obligation_kind::defining_relation) {
        const auto relation = obligations[i].exact_entities.front();
        const auto vertex = obligations[i].vertices.front();
        const auto found = std::find_if(
            certificate->defining_relations.begin(),
            certificate->defining_relations.end(), [&](const auto &value) {
              return value.vertex.value_for_debug() == vertex &&
                     value.relation.value_for_debug() == relation;
            });
        if (found == certificate->defining_relations.end())
          return approximation_error(product_error_code::internal_invariant_error,
                                     "approximate.relation_obligation");
        obligations[i].measured_value =
            found->normalized_squared_residual_change;
        obligations[i].allowed_value = found->allowed_squared_residual_change;
      }
      if (!obligations[i].passed)
        return approximation_error(product_error_code::internal_invariant_error,
                                   "approximate.accepted_obligation_failed");
    }
    certificate->obligations = std::move(obligations);
    certificate->search.predicate_checks = predicate_checks;
    for (std::size_t i = 0; i < solved.components.size(); ++i) {
      const auto &source = solved.components[i];
      approximate_constraint_component component;
      component.id = i;
      component.variables = source.variables;
      component.variable_order = source.variable_order;
      component.obligations = source.constraints;
      component.accepted_ranks = source.accepted_ranks;
      component.rejected_prefix_witnesses = source.rejected_prefix_witnesses;
      component.visited_nodes = source.visited_nodes;
      component.complete_assignments = source.complete_assignments;
      canonical_encoder graph;
      for (const auto value : component.variables)
        graph.u64(value);
      for (const auto value : component.variable_order)
        graph.u64(value);
      for (const auto value : component.obligations)
        graph.u64(value);
      component.graph_digest = domain_digest(
          {{'Y', 'G', 'B', 'A', 'G', 'R', 'P', '1'}}, graph.bytes());
      canonical_encoder transcript;
      for (const auto value : component.variable_order)
        transcript.u64(value);
      for (const auto value : component.obligations)
        transcript.u64(value);
      for (const auto value : component.accepted_ranks)
        transcript.u64(value);
      for (const auto value : component.rejected_prefix_witnesses)
        transcript.u64(value);
      transcript.u64(component.visited_nodes);
      transcript.u64(component.complete_assignments);
      transcript.u64(certificate->search.candidate_cap);
      transcript.u64(certificate->search.candidate_evaluation_limit);
      transcript.u64(certificate->search.candidate_evaluations);
      transcript.u64(certificate->search.search_node_limit);
      transcript.u64(certificate->search.obligation_limit);
      transcript.u64(certificate->search.triangle_pair_limit);
      transcript.u64(certificate->search.predicate_check_limit);
      transcript.u64(certificate->search.predicate_checks);
      transcript.u64(certificate->search.verifier_work_limit);
      transcript.u64(certificate->search.verifier_record_limit);
      transcript.u64(certificate->search.verifier_byte_limit);
      component.transcript_digest = domain_digest(
          {{'Y', 'G', 'B', 'A', 'T', 'R', 'N', '1'}}, transcript.bytes());
      certificate->components.push_back(std::move(component));
    }
    canonical_encoder transcript;
    transcript.raw(certificate->search.candidate_domain_digest.bytes.data(), 16);
    transcript.u64(certificate->search.candidate_cap);
    transcript.u64(certificate->search.candidate_evaluation_limit);
    transcript.u64(certificate->search.candidate_evaluations);
    transcript.u64(certificate->search.search_node_limit);
    transcript.u64(certificate->search.obligation_limit);
    transcript.u64(certificate->search.triangle_pair_limit);
    transcript.u64(certificate->search.predicate_check_limit);
    transcript.u64(certificate->search.predicate_checks);
    transcript.u64(certificate->search.verifier_work_limit);
    transcript.u64(certificate->search.verifier_record_limit);
    transcript.u64(certificate->search.verifier_byte_limit);
    transcript.u64(certificate->search.visited_nodes);
    transcript.u64(certificate->search.complete_assignments);
    for (const auto rank : accepted)
      transcript.u64(rank);
    certificate->search.transcript_digest = domain_digest(
        {{'Y', 'G', 'B', 'A', 'S', 'R', 'C', '1'}}, transcript.bytes());

    const auto bytes = output_bytes(*success);
    success->canonical_output_digest = domain_digest(
        {{'Y', 'G', 'B', 'A', 'O', 'U', 'T', '1'}}, bytes);
    success->summary.vertices = success->mesh.vertices.size();
    success->summary.faces = success->mesh.faces.size();
    success->summary.face_indices = 3 * success->mesh.faces.size();
    success->summary.components = connected_face_components(success->mesh.faces);
    success->summary.semantic_digest = success->canonical_output_digest;

    certified_mesh_payload<T, I> payload;
    payload.success = std::move(success);
    payload.semantics =
        product_realization_semantics::certified_approximate_embedding_v1;
    payload.exact_result_digest = exact->canonical_digest;
    payload.policy = policy;
    payload.output_canonical_bytes = bytes;
    payload.output_semantic_digest = payload.success->canonical_output_digest;
    payload.realization_canonical_bytes = domain_encoding.bytes();
    payload.realization_semantic_digest = certificate->search.transcript_digest;
    payload.obligation_count = certificate->obligations.size();
    payload.constraint_component_count = certificate->components.size();
    payload.certificate.semantics = payload.semantics;
    payload.certificate.backend = backend;
    payload.certificate.exact_result_digest = exact->canonical_digest;
    payload.approximate_certificate = certificate;
    canonicalize_certified_approximate_certificate(*certificate);
    if (!reserve_resource(resource_kind::evidence_bytes,
                          certificate->canonical_bytes.size()))
      return *resource_failure;
    auto verified = verify_certified_approximate_embedding(
        exact, policy, *payload.success, *certificate);
    if (!verified.has_value())
      return verified.error();
    for (auto &charge : resource_charges)
      charge.commit();
    payload.certificate.certificate_digest = certificate->certificate_digest;
    return payload;
  } catch (const std::bad_alloc &) {
    return approximation_error(product_error_code::resource_limit,
                               "approximate.allocation");
  } catch (const std::exception &error) {
    auto result = approximation_error(product_error_code::internal_invariant_error,
                                      "approximate.exception");
    result.detail = error.what();
    return result;
  }
}

#define YGOR_APPROXIMATE_DEFINE(T, I)                                         \
  template void canonicalize_certified_approximate_certificate(               \
      certified_approximate_certificate<T, I> &);                             \
  template product_status_or<certified_mesh_payload<T, I>>                    \
  realize_certified_approximate_embedding(                                    \
      boolean_context<T, I> &, const exact_result_handle &,                   \
      const backend_identity &, const product_realization_policy &)
YGOR_APPROXIMATE_DEFINE(float, std::uint32_t);
YGOR_APPROXIMATE_DEFINE(float, std::uint64_t);
YGOR_APPROXIMATE_DEFINE(double, std::uint32_t);
YGOR_APPROXIMATE_DEFINE(double, std::uint64_t);
#undef YGOR_APPROXIMATE_DEFINE

} // namespace mesh_boolean
} // namespace ygor
