#include "YgorMeshesBooleanOutput.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <queue>
#include <set>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Component 12 requires strict floating-point compilation"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Component 12 must not assume finite-only arithmetic"
#endif

namespace ygor {
namespace mesh_boolean {
namespace {

template <class T, class I> std::uint64_t type_tag() {
  return assembled_output_type_tag +
         (static_cast<std::uint64_t>(std::is_same<T, double>::value
                                         ? coordinate_tag::binary64
                                         : coordinate_tag::binary32)
          << 8) +
         static_cast<std::uint64_t>(std::is_same<I, std::uint64_t>::value
                                        ? index_tag::uint64
                                        : index_tag::uint32);
}

digest policy_digest(const output_policy &p) {
  canonical_encoder e;
  e.u16(p.schema);
  e.u16(p.ordering_version);
  e.u16(p.encoding_version);
  e.byte(static_cast<std::uint8_t>(p.topology));
  return domain_digest({{'Y', 'G', 'B', 'O', 'P', 'L', '1', '2'}}, e.bytes());
}

template <class T> auto raw_bits(T v) {
  typename std::conditional<sizeof(T) == 4, std::uint32_t, std::uint64_t>::type
      b = 0;
  std::memcpy(&b, &v, sizeof(v));
  return b;
}

template <class T>
void encode_bits(canonical_encoder &e, coordinate_bits<T> b) {
  if constexpr (sizeof(T) == 4)
    e.u32(b.bits);
  else
    e.u64(b.bits);
}

template <class T>
std::vector<std::uint8_t> vertex_token(const realization_vertex<T> &v) {
  canonical_encoder e;
  const char tag[] = "YGBVTX12";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(1);
  e.byte_string(v.owner_free_semantic_key);
  for (auto b : v.accepted_bits)
    encode_bits(e, b);
  return e.bytes();
}

void append_bytes(canonical_encoder &e, const std::vector<std::uint8_t> &b) {
  e.byte_string(b);
}

template <class T, class I> struct prepared_face {
  const realization_triangle *triangle = nullptr;
  std::array<realization_vertex_id, 3> ring;
  std::array<realization_edge_role, 3> roles;
  std::uint8_t rotation = 0;
  std::vector<std::uint8_t> key;
  std::size_t component = 0;
};

template <class T, class I>
status_or<std::vector<prepared_face<T, I>>>
prepare_faces(const realized_boundary<T, I> &r,
              const std::vector<std::vector<std::uint8_t>> &tokens) {
  std::vector<prepared_face<T, I>> out;
  out.reserve(r.triangles.size());
  std::set<std::vector<std::uint8_t>> unique;
  for (const auto &t : r.triangles) {
    if (t.id.value_for_debug() >= r.triangles.size() ||
        t.patch.value_for_debug() >= r.selected->payload->patches.size())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly, "triangle_binding");
    for (auto v : t.vertices)
      if (v.value_for_debug() >= tokens.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "triangle_vertex_range");
    prepared_face<T, I> f;
    f.triangle = &t;
    std::array<std::vector<std::uint8_t>, 3> rotations;
    for (std::size_t q = 0; q < 3; ++q) {
      canonical_encoder e;
      for (std::size_t n = 0; n < 3; ++n)
        append_bytes(e, tokens[t.vertices[(n + q) % 3].value_for_debug()]);
      rotations[q] = e.bytes();
    }
    f.rotation = static_cast<std::uint8_t>(
        std::min_element(rotations.begin(), rotations.end()) -
        rotations.begin());
    for (std::size_t n = 0; n < 3; ++n) {
      const auto source = (n + f.rotation) % 3;
      f.ring[n] = t.vertices[source];
      const auto h = t.halfedges[source];
      if (h.value_for_debug() >= r.halfedges.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "triangle_halfedge_range");
      const auto &he = r.halfedges[h.value_for_debug()];
      if (he.triangle != t.id || he.origin != t.vertices[source] ||
          he.destination != t.vertices[(source + 1) % 3])
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "triangle_halfedge_binding");
      f.roles[n] = he.role;
    }
    canonical_encoder key;
    append_bytes(key, rotations[f.rotation]);
    append_bytes(key, t.owner_free_semantic_key);
    key.byte(static_cast<std::uint8_t>(t.projection));
    for (auto role : f.roles)
      key.byte(static_cast<std::uint8_t>(role));
    f.key = key.bytes();
    if (!unique.insert(f.key).second)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "duplicate_triangle_key");
    out.push_back(std::move(f));
  }
  return out;
}

struct edge_use {
  std::size_t face = 0;
  bool forward = false;
};

template <class T, class I>
status_or<std::vector<output_component_record>>
order_components(std::vector<prepared_face<T, I>> &faces,
                 const std::vector<std::vector<std::uint8_t>> &tokens) {
  using edge_key =
      std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>;
  std::map<edge_key, std::vector<edge_use>> edges;
  for (std::size_t fi = 0; fi < faces.size(); ++fi)
    for (std::size_t n = 0; n < 3; ++n) {
      const auto &a = tokens[faces[fi].ring[n].value_for_debug()];
      const auto &b = tokens[faces[fi].ring[(n + 1) % 3].value_for_debug()];
      if (a == b)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "collapsed_semantic_edge");
      const bool forward = a < b;
      edges[forward ? edge_key{a, b} : edge_key{b, a}].push_back({fi, forward});
    }
  std::vector<std::vector<std::size_t>> adjacency(faces.size());
  for (const auto &entry : edges) {
    if (entry.second.size() != 2 ||
        entry.second[0].forward == entry.second[1].forward)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "nonmanifold_realized_edge");
    const auto a = entry.second[0].face, b = entry.second[1].face;
    adjacency[a].push_back(b);
    adjacency[b].push_back(a);
  }
  struct component {
    std::vector<std::size_t> faces;
    std::vector<std::uint8_t> key;
  };
  std::vector<component> found;
  std::vector<bool> visited(faces.size());
  std::vector<std::size_t> seeds(faces.size());
  for (std::size_t i = 0; i < seeds.size(); ++i)
    seeds[i] = i;
  std::sort(seeds.begin(), seeds.end(),
            [&](auto a, auto b) { return faces[a].key < faces[b].key; });
  for (auto seed : seeds)
    if (!visited[seed]) {
      component c;
      std::queue<std::size_t> q;
      q.push(seed);
      visited[seed] = true;
      while (!q.empty()) {
        auto x = q.front();
        q.pop();
        c.faces.push_back(x);
        std::sort(adjacency[x].begin(), adjacency[x].end(),
                  [&](auto a, auto b) { return faces[a].key < faces[b].key; });
        for (auto n : adjacency[x])
          if (!visited[n]) {
            visited[n] = true;
            q.push(n);
          }
      }
      std::sort(c.faces.begin(), c.faces.end(),
                [&](auto a, auto b) { return faces[a].key < faces[b].key; });
      std::set<std::vector<std::uint8_t>> vertices;
      std::set<edge_key> component_edges;
      canonical_encoder key;
      key.u16(1);
      key.u64(c.faces.size());
      for (auto f : c.faces) {
        append_bytes(key, faces[f].key);
        for (auto v : faces[f].ring)
          vertices.insert(tokens[v.value_for_debug()]);
        for (std::size_t n = 0; n < 3; ++n) {
          auto a = tokens[faces[f].ring[n].value_for_debug()],
               b = tokens[faces[f].ring[(n + 1) % 3].value_for_debug()];
          component_edges.insert(a < b ? edge_key{a, b} : edge_key{b, a});
        }
      }
      key.u64(vertices.size());
      for (const auto &v : vertices)
        append_bytes(key, v);
      key.u64(component_edges.size());
      for (const auto &e : component_edges) {
        append_bytes(key, e.first);
        append_bytes(key, e.second);
      }
      c.key = key.bytes();
      found.push_back(std::move(c));
    }
  std::sort(found.begin(), found.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  for (std::size_t i = 1; i < found.size(); ++i)
    if (found[i - 1].key == found[i].key)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "duplicate_component_key");
  std::vector<prepared_face<T, I>> ordered;
  ordered.reserve(faces.size());
  std::vector<output_component_record> records;
  records.reserve(found.size());
  for (std::size_t ci = 0; ci < found.size(); ++ci) {
    output_component_record record;
    record.id = output_component_id::from_canonical_value(ci);
    record.first_face = ordered.size();
    record.face_count = found[ci].faces.size();
    record.semantic_key = found[ci].key;
    for (auto f : found[ci].faces) {
      faces[f].component = ci;
      ordered.push_back(std::move(faces[f]));
    }
    records.push_back(std::move(record));
  }
  faces = std::move(ordered);
  return records;
}

template <class T, class I>
std::vector<std::uint8_t> semantic(const assembled_output<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN12";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(assembled_output_schema);
  e.byte(std::is_same<T, double>::value ? 1 : 0);
  e.byte(std::is_same<I, std::uint64_t>::value ? 1 : 0);
  e.u16(a.policy.ordering_version);
  e.u16(a.policy.encoding_version);
  e.byte(static_cast<std::uint8_t>(a.policy.topology));
  e.raw(a.realized->payload->certificate.semantic_digest.bytes.data(), 16);
  e.raw(a.realized->payload->policy_digest.bytes.data(), 16);
  e.u64(a.components.size());
  for (const auto &c : a.components) {
    e.id(c.id);
    e.u64(c.first_face);
    e.u64(c.face_count);
    append_bytes(e, c.semantic_key);
  }
  e.u64(a.mesh.vertices.size());
  for (const auto &v : a.mesh.vertices) {
    e.floating(v.x);
    e.floating(v.y);
    e.floating(v.z);
  }
  e.u64(a.mesh.faces.size());
  for (const auto &f : a.mesh.faces) {
    e.u64(3);
    for (auto i : f)
      e.u64(i);
  }
  e.u64(a.vertices.size());
  for (const auto &v : a.vertices) {
    e.id(v.id);
    e.id(v.realization);
    e.u64(v.public_index);
    append_bytes(e, v.semantic_token);
  }
  e.u64(a.faces.size());
  for (const auto &f : a.faces) {
    e.id(f.id);
    e.id(f.realization);
    e.id(f.component);
    e.byte(f.cyclic_rotation);
    for (auto i : f.public_vertices)
      e.u64(i);
    append_bytes(e, f.semantic_key);
  }
  e.u64(a.mesh.involved_faces.size());
  for (const auto &x : a.mesh.involved_faces) {
    e.u64(x.size());
    for (auto i : x)
      e.u64(i);
  }
  e.u64(0);
  e.u64(0);
  e.u64(0);
  const auto &c = a.certificate;
  e.id(c.id);
  e.u16(c.schema);
  e.u16(c.ordering_version);
  e.u16(c.encoding_version);
  e.u64(c.vertices);
  e.u64(c.faces);
  e.u64(c.components);
  e.u64(c.face_indices);
  e.u64(c.involved_face_entries);
  e.u64(c.edge_uses);
  for (const auto *d :
       {&c.selected_digest, &c.realized_digest, &c.policy_digest,
        &c.topology_digest, &c.mapping_digest})
    e.raw(d->bytes.data(), 16);
  return e.bytes();
}

template <class T, class I>
std::vector<std::uint8_t> invocation(const assembled_output<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBOUT12";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(assembled_output_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(a.selected_operation));
  e.raw(a.input_a_digest.bytes.data(), 16);
  e.raw(a.input_b_digest.bytes.data(), 16);
  e.raw(a.selected_digest.bytes.data(), 16);
  e.raw(a.realized_digest.bytes.data(), 16);
  e.raw(a.policy_digest.bytes.data(), 16);
  e.u64(a.realized->payload->selected->generation);
  e.u64(a.realized->generation);
  e.raw(a.realized->payload->selected->report.report_digest.bytes.data(), 16);
  e.raw(a.realized->report.report_digest.bytes.data(), 16);
  e.raw(a.certificate.semantic_digest.bytes.data(), 16);
  e.byte_string(a.canonical_bytes);
  return e.bytes();
}

template <class T, class I>
digest artifact_digest_for(const assembled_output<T, I> &a) {
  canonical_encoder e;
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(artifact_slot::assembled_output));
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}

template <class T>
bool raw_coordinate_matches(const vec3<T> &v, const realization_vertex<T> &r) {
  return raw_bits(v.x) == r.accepted_bits[0].bits &&
         raw_bits(v.y) == r.accepted_bits[1].bits &&
         raw_bits(v.z) == r.accepted_bits[2].bits;
}

template <class I>
bool vertex_links_are_cycles(const std::vector<std::vector<I>> &faces,
                             std::size_t vertex_count,
                             const std::function<bool()> &cancelled) {
  std::vector<std::vector<std::size_t>> incident(vertex_count);
  for (std::size_t f = 0; f < faces.size(); ++f)
    for (auto v : faces[f])
      incident[static_cast<std::size_t>(v)].push_back(f);
  for (std::size_t v = 0; v < vertex_count; ++v) {
    if (cancelled && cancelled())
      return false;
    if (incident[v].empty())
      return false;
    std::map<std::size_t, std::vector<std::size_t>> neighbors;
    for (auto f : incident[v])
      for (auto g : incident[v])
        if (f < g) {
          std::size_t shared = 0;
          for (auto x : faces[f])
            for (auto y : faces[g])
              shared += x == y;
          if (shared == 2) {
            neighbors[f].push_back(g);
            neighbors[g].push_back(f);
          }
        }
    for (auto f : incident[v])
      if (neighbors[f].size() != 2)
        return false;
    std::set<std::size_t> reached;
    std::queue<std::size_t> q;
    q.push(incident[v].front());
    reached.insert(incident[v].front());
    while (!q.empty()) {
      auto f = q.front();
      q.pop();
      for (auto n : neighbors[f])
        if (reached.insert(n).second)
          q.push(n);
    }
    if (reached.size() != incident[v].size())
      return false;
  }
  return true;
}

template <class T, class I>
bool independently_valid(const assembled_output<T, I> &a,
                         const verification_environment_view &env) {
  if (!a.realized || !a.realized->payload || a.owner != env.owner ||
      a.setup_digest != env.setup_digest ||
      a.selected_digest != a.realized->payload->selected_digest ||
      a.realized_digest != a.realized->artifact_digest ||
      a.policy_digest != policy_digest(a.policy) ||
      !a.realized->report.passed() ||
      a.policy.topology !=
          output_topology_policy::triangulated_v1_no_simplification ||
      !a.mesh.vertex_normals.empty() || !a.mesh.vertex_colours.empty() ||
      !a.mesh.metadata.empty())
    return false;
  if (!a.public_result || a.public_result->mesh.faces != a.mesh.faces ||
      a.public_result->mesh.involved_faces != a.mesh.involved_faces ||
      !a.public_result->mesh.vertex_normals.empty() ||
      !a.public_result->mesh.vertex_colours.empty() ||
      !a.public_result->mesh.metadata.empty() ||
      a.public_result->selected_operation != a.selected_operation ||
      a.public_result->policy.schema != a.policy.schema ||
      a.public_result->policy.ordering_version != a.policy.ordering_version ||
      a.public_result->policy.encoding_version != a.policy.encoding_version ||
      a.public_result->policy.topology != a.policy.topology ||
      a.public_result->policy.include_compact_provenance !=
          a.policy.include_compact_provenance ||
      a.public_result->input_a_digest != a.input_a_digest ||
      a.public_result->input_b_digest != a.input_b_digest ||
      a.public_result->selected_boundary_digest != a.selected_digest ||
      a.public_result->realized_boundary_digest != a.realized_digest ||
      a.public_result->canonical_output_digest !=
          a.certificate.semantic_digest ||
      a.public_result->summary.vertices != a.certificate.vertices ||
      a.public_result->summary.faces != a.certificate.faces ||
      a.public_result->summary.components != a.certificate.components ||
      a.public_result->summary.face_indices != a.certificate.face_indices ||
      a.public_result->summary.involved_face_entries !=
          a.certificate.involved_face_entries ||
      a.public_result->summary.semantic_digest !=
          a.certificate.semantic_digest ||
      !a.public_result->certificate ||
      a.public_result->certificate->id != a.certificate.id ||
      a.public_result->certificate->schema != a.certificate.schema ||
      a.public_result->certificate->ordering_version !=
          a.certificate.ordering_version ||
      a.public_result->certificate->encoding_version !=
          a.certificate.encoding_version ||
      a.public_result->certificate->vertices != a.certificate.vertices ||
      a.public_result->certificate->faces != a.certificate.faces ||
      a.public_result->certificate->components != a.certificate.components ||
      a.public_result->certificate->face_indices !=
          a.certificate.face_indices ||
      a.public_result->certificate->involved_face_entries !=
          a.certificate.involved_face_entries ||
      a.public_result->certificate->edge_uses != a.certificate.edge_uses ||
      a.public_result->certificate->selected_digest !=
          a.certificate.selected_digest ||
      a.public_result->certificate->realized_digest !=
          a.certificate.realized_digest ||
      a.public_result->certificate->policy_digest !=
          a.certificate.policy_digest ||
      a.public_result->certificate->topology_digest !=
          a.certificate.topology_digest ||
      a.public_result->certificate->mapping_digest !=
          a.certificate.mapping_digest ||
      a.public_result->certificate->semantic_digest !=
          a.certificate.semantic_digest ||
      !a.public_result->compact_provenance.empty() ||
      a.public_result->mesh.vertices.size() != a.mesh.vertices.size())
    return false;
  for (std::size_t i = 0; i < a.mesh.vertices.size(); ++i)
    if (raw_bits(a.public_result->mesh.vertices[i].x) !=
            raw_bits(a.mesh.vertices[i].x) ||
        raw_bits(a.public_result->mesh.vertices[i].y) !=
            raw_bits(a.mesh.vertices[i].y) ||
        raw_bits(a.public_result->mesh.vertices[i].z) !=
            raw_bits(a.mesh.vertices[i].z))
      return false;
  const auto &r = *a.realized->payload;
  if (!r.selected || !r.selected->payload)
    return false;
  const auto selected_type = selected_exact_boundary_type_tag +
                             (static_cast<std::uint64_t>(env.coordinate) << 8) +
                             static_cast<std::uint64_t>(env.index);
  if (!validate_verification_report(a.realized->report).has_value() ||
      !validate_verification_report(r.selected->report).has_value())
    return false;
  if (r.owner != a.owner || r.setup_digest != a.setup_digest ||
      r.artifact_digest != a.realized->artifact_digest ||
      r.selected_digest != r.selected->artifact_digest ||
      r.selected->owner != a.owner ||
      r.selected->stage != boolean_stage::boolean_selection ||
      r.selected->slot != artifact_slot::selected_exact_boundary ||
      !r.selected->report.passed() || a.realized->report.owner != a.owner ||
      a.realized->report.stage != boolean_stage::geometry_realization ||
      a.realized->report.slot != artifact_slot::realized_boundary ||
      a.realized->report.artifact_type_tag !=
          realized_boundary_type_tag +
              (static_cast<std::uint64_t>(env.coordinate) << 8) +
              static_cast<std::uint64_t>(env.index) ||
      a.realized->report.artifact_schema != realized_boundary_schema ||
      a.realized->report.setup_digest != a.setup_digest ||
      a.realized->report.artifact_digest != a.realized->artifact_digest ||
      r.selected->report.owner != a.owner ||
      r.selected->report.stage != boolean_stage::boolean_selection ||
      r.selected->report.slot != artifact_slot::selected_exact_boundary ||
      r.selected->report.artifact_type_tag != selected_type ||
      r.selected->report.artifact_schema != selected_exact_boundary_schema ||
      r.selected->report.setup_digest != a.setup_digest ||
      r.selected->report.artifact_digest != r.selected->artifact_digest ||
      r.certificate.selected_digest != r.selected_digest ||
      r.certificate.policy_digest != r.policy_digest)
    return false;
  for (std::size_t i = 0; i < r.vertices.size(); ++i)
    if (r.vertices[i].id.value_for_debug() != i ||
        r.vertices[i].owner_free_semantic_key.empty())
      return false;
  for (std::size_t i = 0; i < r.triangles.size(); ++i) {
    const auto &triangle = r.triangles[i];
    if (triangle.id.value_for_debug() != i ||
        triangle.patch.value_for_debug() >=
            r.selected->payload->patches.size() ||
        triangle.owner_free_semantic_key.empty())
      return false;
    for (auto vertex : triangle.vertices)
      if (vertex.value_for_debug() >= r.vertices.size())
        return false;
    for (auto halfedge : triangle.halfedges)
      if (halfedge.value_for_debug() >= r.halfedges.size())
        return false;
  }
  if (a.mesh.faces.size() != r.triangles.size() ||
      a.faces.size() != r.triangles.size() ||
      a.mesh.vertices.size() != a.vertices.size() ||
      a.mesh.involved_faces.size() != a.mesh.vertices.size())
    return false;
  std::vector<std::vector<std::uint8_t>> tokens(r.vertices.size());
  std::set<std::vector<std::uint8_t>> unique_tokens;
  for (std::size_t i = 0; i < r.vertices.size(); ++i) {
    canonical_encoder e;
    const char tag[] = "YGBVTX12";
    e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
    e.u16(1);
    e.byte_string(r.vertices[i].owner_free_semantic_key);
    for (auto b : r.vertices[i].accepted_bits)
      encode_bits(e, b);
    tokens[i] = e.bytes();
    if (!unique_tokens.insert(tokens[i]).second)
      return false;
  }
  std::vector<std::vector<I>> rebuilt_involved(a.mesh.vertices.size());
  std::vector<bool> seen(a.mesh.vertices.size());
  std::size_t next = 0;
  std::map<std::pair<I, I>, std::vector<bool>> edge_directions;
  std::set<realization_triangle_id> triangle_map;
  std::set<realization_vertex_id> vertex_map;
  for (std::size_t fi = 0; fi < a.mesh.faces.size(); ++fi) {
    const auto &face = a.mesh.faces[fi];
    if (face.size() != 3 || a.faces[fi].id.value_for_debug() != fi ||
        a.faces[fi].public_vertices !=
            std::array<I, 3>{{face[0], face[1], face[2]}})
      return false;
    if (!triangle_map.insert(a.faces[fi].realization).second ||
        a.faces[fi].realization.value_for_debug() >= r.triangles.size() ||
        a.faces[fi].cyclic_rotation > 2)
      return false;
    const auto &source = r.triangles[a.faces[fi].realization.value_for_debug()];
    for (std::size_t n = 0; n < 3; ++n) {
      if (static_cast<std::size_t>(face[n]) >= a.mesh.vertices.size() ||
          face[n] == face[(n + 1) % 3] ||
          a.faces[fi].public_vertices[n] != face[n])
        return false;
      const auto symbol =
          source.vertices[(n + a.faces[fi].cyclic_rotation) % 3];
      if (static_cast<std::size_t>(face[n]) >= a.vertices.size() ||
          a.vertices[face[n]].realization != symbol)
        return false;
      rebuilt_involved[face[n]].push_back(static_cast<I>(fi));
      if (!seen[face[n]]) {
        if (static_cast<std::size_t>(face[n]) != next++)
          return false;
        seen[face[n]] = true;
      }
      auto x = face[n], y = face[(n + 1) % 3];
      const bool forward = x < y;
      edge_directions[forward ? std::make_pair(x, y) : std::make_pair(y, x)]
          .push_back(forward);
    }
  }
  if (triangle_map.size() != r.triangles.size() ||
      next != a.mesh.vertices.size() ||
      rebuilt_involved != a.mesh.involved_faces ||
      !vertex_links_are_cycles(a.mesh.faces, a.mesh.vertices.size(),
                               env.cancelled))
    return false;
  for (const auto &edge : edge_directions)
    if (edge.second.size() != 2 || edge.second[0] == edge.second[1])
      return false;
  for (std::size_t i = 0; i < a.vertices.size(); ++i) {
    const auto &m = a.vertices[i];
    if (m.id.value_for_debug() != i ||
        static_cast<std::size_t>(m.public_index) != i ||
        m.realization.value_for_debug() >= r.vertices.size() ||
        !vertex_map.insert(m.realization).second ||
        m.semantic_token != tokens[m.realization.value_for_debug()] ||
        !raw_coordinate_matches(a.mesh.vertices[i],
                                r.vertices[m.realization.value_for_debug()]))
      return false;
  }
  struct verifier_face {
    realization_triangle_id id;
    std::array<realization_vertex_id, 3> ring;
    std::uint8_t rotation = 0;
    std::vector<std::uint8_t> key;
    std::size_t component = 0;
  };
  std::vector<verifier_face> verifier_faces;
  std::set<std::vector<std::uint8_t>> unique_face_keys;
  for (const auto &triangle : r.triangles) {
    if (triangle.id.value_for_debug() >= r.triangles.size())
      return false;
    std::array<std::vector<std::uint8_t>, 3> rotations;
    for (std::size_t q = 0; q < 3; ++q) {
      canonical_encoder encoded;
      for (std::size_t n = 0; n < 3; ++n)
        encoded.byte_string(
            tokens[triangle.vertices[(n + q) % 3].value_for_debug()]);
      rotations[q] = encoded.bytes();
    }
    verifier_face face;
    face.id = triangle.id;
    face.rotation = static_cast<std::uint8_t>(
        std::min_element(rotations.begin(), rotations.end()) -
        rotations.begin());
    canonical_encoder key;
    key.byte_string(rotations[face.rotation]);
    key.byte_string(triangle.owner_free_semantic_key);
    key.byte(static_cast<std::uint8_t>(triangle.projection));
    for (std::size_t n = 0; n < 3; ++n) {
      const auto source = (n + face.rotation) % 3;
      face.ring[n] = triangle.vertices[source];
      const auto halfedge_id = triangle.halfedges[source];
      if (halfedge_id.value_for_debug() >= r.halfedges.size())
        return false;
      const auto &halfedge = r.halfedges[halfedge_id.value_for_debug()];
      if (halfedge.triangle != triangle.id ||
          halfedge.origin != triangle.vertices[source] ||
          halfedge.destination != triangle.vertices[(source + 1) % 3])
        return false;
      key.byte(static_cast<std::uint8_t>(halfedge.role));
    }
    face.key = key.bytes();
    if (!unique_face_keys.insert(face.key).second)
      return false;
    verifier_faces.push_back(std::move(face));
  }
  using verifier_edge =
      std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>;
  struct verifier_use {
    std::size_t face = 0;
    bool forward = false;
  };
  std::map<verifier_edge, std::vector<verifier_use>> verifier_edges;
  for (std::size_t fi = 0; fi < verifier_faces.size(); ++fi)
    for (std::size_t n = 0; n < 3; ++n) {
      const auto &x = tokens[verifier_faces[fi].ring[n].value_for_debug()];
      const auto &y =
          tokens[verifier_faces[fi].ring[(n + 1) % 3].value_for_debug()];
      const bool forward = x < y;
      verifier_edges[forward ? verifier_edge{x, y} : verifier_edge{y, x}]
          .push_back({fi, forward});
    }
  std::vector<std::vector<std::size_t>> adjacency(verifier_faces.size());
  for (const auto &edge : verifier_edges) {
    if (edge.second.size() != 2 ||
        edge.second[0].forward == edge.second[1].forward)
      return false;
    adjacency[edge.second[0].face].push_back(edge.second[1].face);
    adjacency[edge.second[1].face].push_back(edge.second[0].face);
  }
  struct verifier_component {
    std::vector<std::size_t> faces;
    std::vector<std::uint8_t> key;
  };
  std::vector<verifier_component> verifier_components;
  std::vector<std::size_t> seeds(verifier_faces.size());
  std::vector<bool> visited(verifier_faces.size());
  for (std::size_t i = 0; i < seeds.size(); ++i)
    seeds[i] = i;
  std::sort(seeds.begin(), seeds.end(), [&](auto x, auto y) {
    return verifier_faces[x].key < verifier_faces[y].key;
  });
  for (auto seed : seeds) {
    if (visited[seed])
      continue;
    verifier_component component;
    std::queue<std::size_t> pending;
    pending.push(seed);
    visited[seed] = true;
    while (!pending.empty()) {
      const auto face = pending.front();
      pending.pop();
      component.faces.push_back(face);
      for (auto neighbor : adjacency[face])
        if (!visited[neighbor]) {
          visited[neighbor] = true;
          pending.push(neighbor);
        }
    }
    std::sort(component.faces.begin(), component.faces.end(),
              [&](auto x, auto y) {
                return verifier_faces[x].key < verifier_faces[y].key;
              });
    std::set<std::vector<std::uint8_t>> component_vertices;
    std::set<verifier_edge> component_edges;
    canonical_encoder descriptor;
    descriptor.u16(1);
    descriptor.u64(component.faces.size());
    for (auto face : component.faces) {
      descriptor.byte_string(verifier_faces[face].key);
      for (auto vertex : verifier_faces[face].ring)
        component_vertices.insert(tokens[vertex.value_for_debug()]);
      for (std::size_t n = 0; n < 3; ++n) {
        auto x = tokens[verifier_faces[face].ring[n].value_for_debug()];
        auto y =
            tokens[verifier_faces[face].ring[(n + 1) % 3].value_for_debug()];
        component_edges.insert(x < y ? verifier_edge{x, y}
                                     : verifier_edge{y, x});
      }
    }
    descriptor.u64(component_vertices.size());
    for (const auto &vertex : component_vertices)
      descriptor.byte_string(vertex);
    descriptor.u64(component_edges.size());
    for (const auto &edge : component_edges) {
      descriptor.byte_string(edge.first);
      descriptor.byte_string(edge.second);
    }
    component.key = descriptor.bytes();
    verifier_components.push_back(std::move(component));
  }
  std::sort(verifier_components.begin(), verifier_components.end(),
            [](const auto &x, const auto &y) { return x.key < y.key; });
  if (verifier_components.size() != a.components.size())
    return false;
  std::size_t expected_face = 0;
  for (std::size_t ci = 0; ci < verifier_components.size(); ++ci) {
    const auto &component = verifier_components[ci];
    if (a.components[ci].id.value_for_debug() != ci ||
        a.components[ci].first_face != expected_face ||
        a.components[ci].face_count != component.faces.size() ||
        a.components[ci].semantic_key != component.key)
      return false;
    for (auto source : component.faces) {
      verifier_faces[source].component = ci;
      if (a.faces[expected_face].realization != verifier_faces[source].id ||
          a.faces[expected_face].cyclic_rotation !=
              verifier_faces[source].rotation ||
          a.faces[expected_face].semantic_key != verifier_faces[source].key ||
          a.faces[expected_face].component.value_for_debug() != ci)
        return false;
      ++expected_face;
    }
  }
  const auto &c = a.certificate;
  if (c.id.value_for_debug() != 0 || c.schema != 1 ||
      c.ordering_version != a.policy.ordering_version ||
      c.encoding_version != a.policy.encoding_version ||
      c.vertices != a.mesh.vertices.size() || c.faces != a.mesh.faces.size() ||
      c.components != a.components.size() ||
      c.face_indices != 3 * a.mesh.faces.size() ||
      c.involved_face_entries != 3 * a.mesh.faces.size() ||
      c.edge_uses != 3 * a.mesh.faces.size() ||
      c.selected_digest != r.selected->payload->certificate.semantic_digest ||
      c.realized_digest != r.certificate.semantic_digest ||
      c.policy_digest != a.policy_digest)
    return false;
  canonical_encoder topology;
  for (const auto &face : a.mesh.faces)
    for (auto index : face)
      topology.u64(index);
  if (c.topology_digest !=
      domain_digest({{'Y', 'G', 'B', 'T', 'O', 'P', '1', '2'}},
                    topology.bytes()))
    return false;
  canonical_encoder mappings;
  for (const auto &vertex : a.vertices) {
    mappings.id(vertex.realization);
    mappings.u64(vertex.public_index);
  }
  for (const auto &face : a.faces) {
    mappings.id(face.realization);
    mappings.id(face.id);
    mappings.byte(face.cyclic_rotation);
  }
  if (c.mapping_digest !=
      domain_digest({{'Y', 'G', 'B', 'M', 'A', 'P', '1', '2'}},
                    mappings.bytes()))
    return false;
  for (const auto &obligation : r.obligations)
    if (obligation.expected != obligation.actual)
      return false;
  canonical_encoder semantic_check;
  const char semantic_tag[] = "YGBCAN12";
  semantic_check.raw(reinterpret_cast<const std::uint8_t *>(semantic_tag), 8);
  semantic_check.u16(assembled_output_schema);
  semantic_check.byte(std::is_same<T, double>::value ? 1 : 0);
  semantic_check.byte(std::is_same<I, std::uint64_t>::value ? 1 : 0);
  semantic_check.u16(a.policy.ordering_version);
  semantic_check.u16(a.policy.encoding_version);
  semantic_check.byte(static_cast<std::uint8_t>(a.policy.topology));
  semantic_check.raw(r.certificate.semantic_digest.bytes.data(), 16);
  semantic_check.raw(r.policy_digest.bytes.data(), 16);
  semantic_check.u64(a.components.size());
  for (const auto &component : a.components) {
    semantic_check.id(component.id);
    semantic_check.u64(component.first_face);
    semantic_check.u64(component.face_count);
    semantic_check.byte_string(component.semantic_key);
  }
  semantic_check.u64(a.mesh.vertices.size());
  for (const auto &vertex : a.mesh.vertices) {
    semantic_check.floating(vertex.x);
    semantic_check.floating(vertex.y);
    semantic_check.floating(vertex.z);
  }
  semantic_check.u64(a.mesh.faces.size());
  for (const auto &face : a.mesh.faces) {
    semantic_check.u64(3);
    for (auto index : face)
      semantic_check.u64(index);
  }
  semantic_check.u64(a.vertices.size());
  for (const auto &vertex : a.vertices) {
    semantic_check.id(vertex.id);
    semantic_check.id(vertex.realization);
    semantic_check.u64(vertex.public_index);
    semantic_check.byte_string(vertex.semantic_token);
  }
  semantic_check.u64(a.faces.size());
  for (const auto &face : a.faces) {
    semantic_check.id(face.id);
    semantic_check.id(face.realization);
    semantic_check.id(face.component);
    semantic_check.byte(face.cyclic_rotation);
    for (auto index : face.public_vertices)
      semantic_check.u64(index);
    semantic_check.byte_string(face.semantic_key);
  }
  semantic_check.u64(a.mesh.involved_faces.size());
  for (const auto &incidence : a.mesh.involved_faces) {
    semantic_check.u64(incidence.size());
    for (auto face : incidence)
      semantic_check.u64(face);
  }
  semantic_check.u64(0);
  semantic_check.u64(0);
  semantic_check.u64(0);
  semantic_check.id(c.id);
  semantic_check.u16(c.schema);
  semantic_check.u16(c.ordering_version);
  semantic_check.u16(c.encoding_version);
  semantic_check.u64(c.vertices);
  semantic_check.u64(c.faces);
  semantic_check.u64(c.components);
  semantic_check.u64(c.face_indices);
  semantic_check.u64(c.involved_face_entries);
  semantic_check.u64(c.edge_uses);
  for (const auto *binding :
       {&c.selected_digest, &c.realized_digest, &c.policy_digest,
        &c.topology_digest, &c.mapping_digest})
    semantic_check.raw(binding->bytes.data(), 16);
  if (semantic_check.bytes() != a.canonical_bytes)
    return false;
  canonical_encoder invocation_check;
  const char invocation_tag[] = "YGBOUT12";
  invocation_check.raw(reinterpret_cast<const std::uint8_t *>(invocation_tag),
                       8);
  invocation_check.u16(assembled_output_schema);
  invocation_check.raw(a.setup_digest.bytes.data(), 16);
  invocation_check.byte(static_cast<std::uint8_t>(a.selected_operation));
  invocation_check.raw(a.input_a_digest.bytes.data(), 16);
  invocation_check.raw(a.input_b_digest.bytes.data(), 16);
  invocation_check.raw(a.selected_digest.bytes.data(), 16);
  invocation_check.raw(a.realized_digest.bytes.data(), 16);
  invocation_check.raw(a.policy_digest.bytes.data(), 16);
  invocation_check.u64(r.selected->generation);
  invocation_check.u64(a.realized->generation);
  invocation_check.raw(r.selected->report.report_digest.bytes.data(), 16);
  invocation_check.raw(a.realized->report.report_digest.bytes.data(), 16);
  invocation_check.raw(c.semantic_digest.bytes.data(), 16);
  invocation_check.byte_string(a.canonical_bytes);
  if (invocation_check.bytes() != a.artifact_bytes)
    return false;
  canonical_encoder artifact_check;
  artifact_check.raw(a.setup_digest.bytes.data(), 16);
  artifact_check.byte(
      static_cast<std::uint8_t>(artifact_slot::assembled_output));
  artifact_check.byte_string(a.artifact_bytes);
  return a.artifact_digest ==
             domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}},
                           artifact_check.bytes()) &&
         c.semantic_digest ==
             domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '1', '2'}},
                           a.canonical_bytes);
}

template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::output_assembly;
    r.slot = v.slot;
    r.artifact_type_tag = v.artifact_type_tag;
    r.artifact_schema = v.artifact_schema;
    r.setup_digest = e.setup_digest;
    r.artifact_digest = v.artifact_digest;
    r.invariant_set_digest = s.invariant_set_digest;
    const auto *a = static_cast<const assembled_output<T, I> *>(v.payload);
    status_or<resource_reservation> scratch = resource_reservation{};
    status_or<resource_reservation> work = resource_reservation{};
    if (a && e.accountant) {
      auto scratch_entities =
          checked_add(a->mesh.faces.size(), a->mesh.vertices.size(),
                      boolean_stage::output_assembly);
      if (!scratch_entities.has_value())
        return scratch_entities.error();
      auto scratch_bytes = checked_multiply(scratch_entities.value(), 1024,
                                            boolean_stage::output_assembly);
      if (!scratch_bytes.has_value())
        return scratch_bytes.error();
      auto encoded_scratch =
          checked_add(a->canonical_bytes.size(), a->artifact_bytes.size(),
                      boolean_stage::output_assembly);
      if (!encoded_scratch.has_value())
        return encoded_scratch.error();
      auto encoded_copies = checked_multiply(encoded_scratch.value(), 4,
                                             boolean_stage::output_assembly);
      if (!encoded_copies.has_value())
        return encoded_copies.error();
      auto total_scratch =
          checked_add(scratch_bytes.value(), encoded_copies.value(),
                      boolean_stage::output_assembly);
      if (!total_scratch.has_value())
        return total_scratch.error();
      scratch = e.accountant->reserve_scoped(
          resource_kind::verifier_scratch_bytes, total_scratch.value(),
          boolean_stage::output_assembly);
      if (!scratch.has_value())
        return scratch.error();
      auto quadratic_work =
          checked_multiply(a->mesh.faces.size(), a->mesh.faces.size(),
                           boolean_stage::output_assembly);
      if (!quadratic_work.has_value())
        return quadratic_work.error();
      auto total_work =
          checked_add(quadratic_work.value(), 1 + a->mesh.vertices.size(),
                      boolean_stage::output_assembly);
      if (!total_work.has_value())
        return total_work.error();
      work = e.accountant->reserve_scoped(resource_kind::verifier_work,
                                          total_work.value(),
                                          boolean_stage::output_assembly);
      if (!work.has_value())
        return work.error();
    }
    const bool binding_ok = e.options && a && v.owner == e.owner &&
                            v.slot == artifact_slot::assembled_output &&
                            v.artifact_type_tag == type_tag<T, I>() &&
                            v.artifact_schema == assembled_output_schema &&
                            v.artifact_digest == a->artifact_digest &&
                            e.coordinate == (std::is_same<T, double>::value
                                                 ? coordinate_tag::binary64
                                                 : coordinate_tag::binary32) &&
                            e.index == (std::is_same<I, std::uint64_t>::value
                                            ? index_tag::uint64
                                            : index_tag::uint32) &&
                            a->policy.schema == e.options->output.schema;
    const bool ok = binding_ok && independently_valid(*a, e);
    if (e.cancelled && e.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::output_assembly, "cancelled");
    r.outcome = ok ? verification_outcome::pass
                   : verification_outcome::invariant_failure;
    bool failed = false;
    for (auto code : s.required_invariants) {
      auto st = ok ? check_status::passed
                   : failed ? check_status::not_run_due_to_prior_failure
                            : check_status::failed;
      r.results.push_back({code, st, {}, 0});
      failed |= st == check_status::failed;
    }
    if (a)
      r.dependency_digests = {a->selected_digest, a->realized_digest,
                              a->policy_digest};
    auto bytes = encode_verification_report(r);
    if (!bytes.has_value())
      return bytes.error();
    r.report_digest = domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}},
                                    bytes.value());
    return r;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::output_assembly,
                      "output_verifier_allocation");
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::output_assembly,
                      "output_verifier_exception");
  }
}

template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}

} // namespace

status_or<bool> register_boolean_output_verifier(verifier_registry &r,
                                                 coordinate_tag c,
                                                 index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::assembled_output;
  x.artifact_type_tag = assembled_output_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = assembled_output_schema;
  x.mandatory = {invariant_code::output_binding,
                 invariant_code::output_structure,
                 invariant_code::output_coordinates,
                 invariant_code::output_topology,
                 invariant_code::output_mappings,
                 invariant_code::output_certificate,
                 invariant_code::output_canonical_encoding};
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
status_or<std::shared_ptr<const published_artifact<assembled_output<T, I>>>>
assemble_boolean_output_artifact(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::output_assembly, "cancelled");
    if (auto old = ctx.artifacts().latest(artifact_slot::assembled_output)) {
      auto typed = std::static_pointer_cast<
          const published_artifact<assembled_output<T, I>>>(old);
      if (!typed || typed->owner != ctx.owner() ||
          typed->stage != boolean_stage::output_assembly ||
          typed->slot != artifact_slot::assembled_output || !typed->payload ||
          !typed->report.passed() || typed->generation != 1 ||
          typed->artifact_digest != typed->payload->artifact_digest ||
          typed->payload->owner != ctx.owner() ||
          typed->payload->setup_digest != ctx.replay().setup ||
          typed->report.artifact_type_tag != type_tag<T, I>() ||
          typed->report.artifact_schema != assembled_output_schema)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "cached_output_binding");
      return typed;
    }
    auto realized_result = realize_selected_boundary(ctx);
    if (!realized_result.has_value())
      return realized_result.error();
    auto realized = realized_result.value();
    if (!realized || !realized->payload || realized->owner != ctx.owner() ||
        realized->stage != boolean_stage::geometry_realization ||
        realized->slot != artifact_slot::realized_boundary ||
        !realized->report.passed() ||
        ctx.artifacts().latest_generation(artifact_slot::realized_boundary) !=
            realized->generation)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "realized_dependency_binding");
    const auto &r = *realized->payload;
    if (r.owner != ctx.owner() || r.setup_digest != ctx.replay().setup ||
        !r.selected || !r.selected->payload ||
        r.selected->owner != ctx.owner() ||
        r.selected->stage != boolean_stage::boolean_selection ||
        r.selected->slot != artifact_slot::selected_exact_boundary ||
        !r.selected->report.passed() ||
        r.selected_digest != r.selected->artifact_digest ||
        r.certificate.selected_digest != r.selected_digest ||
        r.certificate.policy_digest != r.policy_digest ||
        r.certificate.triangulation_version != 1 ||
        r.certificate.obligation_version != 1)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "realized_payload_binding");
    const auto expected_realization_type =
        realized_boundary_type_tag +
        (static_cast<std::uint64_t>(ctx.platform().coordinate) << 8) +
        static_cast<std::uint64_t>(ctx.platform().index);
    const auto expected_selection_type =
        selected_exact_boundary_type_tag +
        (static_cast<std::uint64_t>(ctx.platform().coordinate) << 8) +
        static_cast<std::uint64_t>(ctx.platform().index);
    auto realized_report_valid = validate_verification_report(realized->report);
    if (!realized_report_valid.has_value())
      return realized_report_valid.error();
    auto selected_report_valid =
        validate_verification_report(r.selected->report);
    if (!selected_report_valid.has_value())
      return selected_report_valid.error();
    if (realized->report.owner != ctx.owner() ||
        realized->report.stage != boolean_stage::geometry_realization ||
        realized->report.slot != artifact_slot::realized_boundary ||
        realized->report.artifact_type_tag != expected_realization_type ||
        realized->report.artifact_schema != realized_boundary_schema ||
        realized->report.setup_digest != ctx.replay().setup ||
        realized->report.artifact_digest != realized->artifact_digest ||
        r.artifact_digest != realized->artifact_digest ||
        r.selected->report.owner != ctx.owner() ||
        r.selected->report.stage != boolean_stage::boolean_selection ||
        r.selected->report.slot != artifact_slot::selected_exact_boundary ||
        r.selected->report.artifact_type_tag != expected_selection_type ||
        r.selected->report.artifact_schema != selected_exact_boundary_schema ||
        r.selected->report.setup_digest != ctx.replay().setup ||
        r.selected->report.artifact_digest != r.selected->artifact_digest)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "realized_report_binding");
    for (std::size_t i = 0; i < r.vertices.size(); ++i)
      if (r.vertices[i].id.value_for_debug() != i ||
          r.vertices[i].owner_free_semantic_key.empty())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "realization_vertex_binding");
    for (std::size_t i = 0; i < r.triangles.size(); ++i) {
      const auto &triangle = r.triangles[i];
      if (triangle.id.value_for_debug() != i ||
          triangle.patch.value_for_debug() >=
              r.selected->payload->patches.size() ||
          triangle.owner_free_semantic_key.empty())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "realization_triangle_binding");
      for (auto vertex : triangle.vertices)
        if (vertex.value_for_debug() >= r.vertices.size())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::output_assembly,
                            "realization_triangle_vertex_range");
      for (auto halfedge : triangle.halfedges)
        if (halfedge.value_for_debug() >= r.halfedges.size())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::output_assembly,
                            "realization_triangle_halfedge_range");
    }
    std::set<realization_vertex_id> used;
    for (const auto &t : r.triangles)
      for (auto v : t.vertices)
        used.insert(v);
    const checked_cardinality vertex_count{
        0, static_cast<std::uint64_t>(used.size())},
        face_count{0, static_cast<std::uint64_t>(r.triangles.size())};
    if (!index_capacity_accepts<I>(vertex_count) ||
        !index_capacity_accepts<I>(face_count))
      return make_error(boolean_error_code::index_overflow,
                        boolean_stage::output_assembly,
                        "public_index_capacity");
    std::vector<resource_reservation> charges;
    auto reserve = [&](resource_kind kind, std::uint64_t n) -> status_or<bool> {
      auto x = ctx.accountant().reserve_scoped(kind, n,
                                               boolean_stage::output_assembly);
      if (!x.has_value())
        return x.error();
      charges.push_back(std::move(x.value()));
      return true;
    };
    auto face_indices =
        checked_multiply(face_count.low, 3, boolean_stage::output_assembly);
    if (!face_indices.has_value())
      return face_indices.error();
    auto mappings_count = checked_add(vertex_count.low, face_count.low,
                                      boolean_stage::output_assembly);
    if (!mappings_count.has_value())
      return mappings_count.error();
    auto private_entities = checked_add(vertex_count.low, face_count.low,
                                        boolean_stage::output_assembly);
    if (!private_entities.has_value())
      return private_entities.error();
    auto private_bytes = checked_multiply(private_entities.value(), 256,
                                          boolean_stage::output_assembly);
    if (!private_bytes.has_value())
      return private_bytes.error();
    for (const auto pair :
         {std::pair<resource_kind, std::uint64_t>{
              resource_kind::output_vertices, vertex_count.low},
          {resource_kind::output_faces, face_count.low},
          {resource_kind::output_face_indices, face_indices.value()},
          {resource_kind::output_involved_entries, face_indices.value()},
          {resource_kind::output_mappings, mappings_count.value()},
          {resource_kind::output_certificate_entries, 7},
          {resource_kind::stage_private_bytes, private_bytes.value()},
          {resource_kind::work_units, 1 + private_entities.value()}}) {
      auto ok = reserve(pair.first, pair.second);
      if (!ok.has_value())
        return ok.error();
    }
    assembled_output<T, I> a;
    a.owner = ctx.owner();
    a.selected_operation = ctx.contract().selected_operation();
    a.setup_digest = ctx.replay().setup;
    a.input_a_digest = ctx.replay().input_a;
    a.input_b_digest = ctx.replay().input_b;
    a.selected_digest = r.selected_digest;
    a.realized_digest = realized->artifact_digest;
    a.policy = ctx.options().output;
    a.policy_digest = policy_digest(a.policy);
    a.realized = realized;
    std::vector<std::vector<std::uint8_t>> tokens(r.vertices.size());
    std::set<std::vector<std::uint8_t>> unique;
    for (std::size_t i = 0; i < r.vertices.size(); ++i) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::output_assembly, "cancelled");
      tokens[i] = vertex_token(r.vertices[i]);
      if (!unique.insert(tokens[i]).second)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::output_assembly,
                          "duplicate_vertex_token");
    }
    auto faces = prepare_faces(r, tokens);
    if (!faces.has_value())
      return faces.error();
    auto components = order_components(faces.value(), tokens);
    if (!components.has_value())
      return components.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::output_assembly, "cancelled");
    a.components = std::move(components.value());
    auto component_charge =
        reserve(resource_kind::output_components, a.components.size());
    if (!component_charge.has_value())
      return component_charge.error();
    std::map<realization_vertex_id, I> indices;
    for (std::size_t fi = 0; fi < faces.value().size(); ++fi) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::output_assembly, "cancelled");
      const auto &source = faces.value()[fi];
      output_face_record<I> record;
      record.id = output_face_id::from_canonical_value(fi);
      record.realization = source.triangle->id;
      record.component =
          output_component_id::from_canonical_value(source.component);
      record.cyclic_rotation = source.rotation;
      record.semantic_key = source.key;
      std::vector<I> public_face;
      public_face.reserve(3);
      for (std::size_t n = 0; n < 3; ++n) {
        auto found = indices.find(source.ring[n]);
        I index{};
        if (found == indices.end()) {
          auto checked = checked_output_index<I>(indices.size());
          if (!checked.has_value())
            return checked.error();
          index = checked.value();
          indices.emplace(source.ring[n], index);
          const auto &rv = r.vertices[source.ring[n].value_for_debug()];
          a.mesh.vertices.push_back(rv.coordinate);
          if (!raw_coordinate_matches(a.mesh.vertices.back(), rv))
            return make_error(boolean_error_code::internal_invariant_error,
                              boolean_stage::output_assembly,
                              "coordinate_bit_copy");
          output_vertex_record<I> vr;
          vr.id = output_vertex_id::from_canonical_value(a.vertices.size());
          vr.realization = source.ring[n];
          vr.public_index = index;
          vr.semantic_token = tokens[source.ring[n].value_for_debug()];
          a.vertices.push_back(std::move(vr));
        } else
          index = found->second;
        record.public_vertices[n] = index;
        public_face.push_back(index);
      }
      a.faces.push_back(std::move(record));
      a.mesh.faces.push_back(std::move(public_face));
    }
    a.mesh.involved_faces.resize(a.mesh.vertices.size());
    for (std::size_t fi = 0; fi < a.mesh.faces.size(); ++fi) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::output_assembly, "cancelled");
      auto checked = checked_output_index<I>(fi);
      if (!checked.has_value())
        return checked.error();
      for (auto v : a.mesh.faces[fi])
        a.mesh.involved_faces[v].push_back(checked.value());
    }
    a.certificate.id = output_assembly_certificate_id::from_canonical_value(0);
    a.certificate.ordering_version = a.policy.ordering_version;
    a.certificate.encoding_version = a.policy.encoding_version;
    a.certificate.vertices = a.mesh.vertices.size();
    a.certificate.faces = a.mesh.faces.size();
    a.certificate.components = a.components.size();
    a.certificate.face_indices = face_indices.value();
    a.certificate.involved_face_entries = face_indices.value();
    a.certificate.edge_uses = face_indices.value();
    a.certificate.selected_digest =
        r.selected->payload->certificate.semantic_digest;
    a.certificate.realized_digest = r.certificate.semantic_digest;
    a.certificate.policy_digest = a.policy_digest;
    canonical_encoder topology;
    for (const auto &f : a.mesh.faces)
      for (auto i : f)
        topology.u64(i);
    a.certificate.topology_digest = domain_digest(
        {{'Y', 'G', 'B', 'T', 'O', 'P', '1', '2'}}, topology.bytes());
    canonical_encoder mappings;
    for (const auto &v : a.vertices) {
      mappings.id(v.realization);
      mappings.u64(v.public_index);
    }
    for (const auto &f : a.faces) {
      mappings.id(f.realization);
      mappings.id(f.id);
      mappings.byte(f.cyclic_rotation);
    }
    a.certificate.mapping_digest = domain_digest(
        {{'Y', 'G', 'B', 'M', 'A', 'P', '1', '2'}}, mappings.bytes());
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::output_assembly, "cancelled");
    a.canonical_bytes = semantic(a);
    a.certificate.semantic_digest = domain_digest(
        {{'Y', 'G', 'B', 'C', 'A', 'N', '1', '2'}}, a.canonical_bytes);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    auto encoded_bytes =
        checked_add(a.canonical_bytes.size(), a.artifact_bytes.size(),
                    boolean_stage::output_assembly);
    if (!encoded_bytes.has_value())
      return encoded_bytes.error();
    auto bytes_charge =
        reserve(resource_kind::output_canonical_bytes, encoded_bytes.value());
    if (!bytes_charge.has_value())
      return bytes_charge.error();
    auto auth_charge =
        reserve(resource_kind::authoritative_bytes, a.artifact_bytes.size());
    if (!auth_charge.has_value())
      return auth_charge.error();
    boolean_success<T, I> public_result;
    public_result.mesh = a.mesh;
    public_result.selected_operation = a.selected_operation;
    public_result.policy = a.policy;
    public_result.input_a_digest = a.input_a_digest;
    public_result.input_b_digest = a.input_b_digest;
    public_result.selected_boundary_digest = a.selected_digest;
    public_result.realized_boundary_digest = a.realized_digest;
    public_result.canonical_output_digest = a.certificate.semantic_digest;
    public_result.summary = {a.certificate.vertices,
                             a.certificate.faces,
                             a.certificate.components,
                             a.certificate.face_indices,
                             a.certificate.involved_face_entries,
                             a.certificate.semantic_digest};
    public_result.certificate =
        std::make_shared<const output_assembly_certificate>(a.certificate);
    a.public_result =
        std::make_shared<const boolean_success<T, I>>(std::move(public_result));
    auto ptr = std::make_shared<const assembled_output<T, I>>(std::move(a));
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::assembled_output, type_tag<T, I>(),
        assembled_output_schema, ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    artifact_view view{ctx.owner(),
                       artifact_slot::assembled_output,
                       type_tag<T, I>(),
                       assembled_output_schema,
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
    stage_transaction<assembled_output<T, I>, assembled_output<T, I>> tx(
        ctx.owner(), boolean_stage::output_assembly,
        artifact_slot::assembled_output,
        std::make_unique<assembled_output<T, I>>());
    for (auto &charge : charges)
      tx.stage_reservation(std::move(charge));
    auto verified = tx.verify(ptr, view, spec.value(), env, ctx.verifiers());
    if (!verified.has_value())
      return verified.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::output_assembly, "cancelled");
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::output_assembly, "output_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::output_assembly, "output_exception");
    x.detail = e.what();
    return x;
  }
}

template <class T, class I>
boolean_result<T, I> assemble_boolean_output(boolean_context<T, I> &ctx) {
  auto artifact = assemble_boolean_output_artifact(ctx);
  if (!artifact.has_value())
    return artifact.error();
  if (!artifact.value()->payload->public_result)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::output_assembly,
                      "published_public_result_missing");
  return artifact.value()->payload->public_result;
}

#define INST(T, I)                                                             \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<assembled_output<T, I>>>>       \
  assemble_boolean_output_artifact(boolean_context<T, I> &);                   \
  template boolean_result<T, I> assemble_boolean_output(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST

} // namespace mesh_boolean
} // namespace ygor
