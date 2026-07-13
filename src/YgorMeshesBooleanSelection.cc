#include "YgorMeshesBooleanSelection.h"
#include <algorithm>
#include <map>
#include <set>

#if defined(__FAST_MATH__)
#error "Component 10 requires strict floating-point compilation"
#endif

namespace ygor {
namespace mesh_boolean {
namespace {
template <class T, class I> std::uint64_t type_tag() {
  return selected_exact_boundary_type_tag +
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
template <class Id> void ids(canonical_encoder &e, const std::vector<Id> &v) {
  e.u64(v.size());
  for (auto x : v)
    e.id(x);
}
template <class T, class I>
std::vector<std::uint8_t> semantic(const selected_exact_boundary<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN10";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(selected_exact_boundary_schema);
  e.byte(static_cast<std::uint8_t>(a.selected_operation));
  e.raw(a.labeled->payload->certificate.semantic_digest.bytes.data(), 16);
  e.u64(a.decisions.size());
  for (const auto &d : a.decisions) {
    e.id(d.id);
    e.id(d.patch);
    e.id(d.negative_label);
    e.id(d.positive_label);
    occupancy(e, d.negative_occupancy);
    occupancy(e, d.positive_occupancy);
    e.boolean(d.result_negative);
    e.boolean(d.result_positive);
    e.byte(static_cast<std::uint8_t>(d.kind));
    e.byte(static_cast<std::uint8_t>(d.orientation));
    e.boolean(bool(d.selected));
    if (d.selected)
      e.id(*d.selected);
    e.boolean(bool(d.representative));
    if (d.representative)
      e.id(*d.representative);
    ids(e, d.provenance);
  }
  e.u64(a.vertices.size());
  for (const auto &v : a.vertices) {
    e.id(v.id);
    e.id(v.source);
    e.id(v.symbolic);
  }
  e.u64(a.edges.size());
  for (const auto &x : a.edges) {
    e.id(x.id);
    e.id(x.source);
    e.id(x.lower);
    e.id(x.upper);
    ids(e, x.uses);
  }
  e.u64(a.halfedges.size());
  for (const auto &h : a.halfedges) {
    e.id(h.id);
    e.id(h.patch);
    e.id(h.cycle);
    e.id(h.edge);
    e.id(h.origin);
    e.id(h.destination);
    e.id(h.next);
    e.id(h.previous);
  }
  e.u64(a.cycles.size());
  for (const auto &c : a.cycles) {
    e.id(c.id);
    e.id(c.patch);
    e.boolean(c.hole);
    ids(e, c.halfedges);
  }
  e.u64(a.patches.size());
  for (const auto &p : a.patches) {
    e.id(p.id);
    e.id(p.source);
    e.byte(static_cast<std::uint8_t>(p.orientation));
    ids(e, p.cycles);
    e.id(p.representative);
    ids(e, p.provenance);
  }
  e.id(a.certificate.id);
  e.u64(a.certificate.decisions);
  e.u64(a.certificate.discard_exterior);
  e.u64(a.certificate.discard_internal);
  e.u64(a.certificate.select_preserved);
  e.u64(a.certificate.select_reversed);
  e.u64(a.certificate.selected_patches);
  e.u64(a.certificate.selected_cycles);
  e.u64(a.certificate.selected_halfedges);
  e.u64(a.certificate.selected_edges);
  e.u64(a.certificate.selected_vertices);
  e.u64(a.certificate.provenance_uses);
  e.u64(a.certificate.connected_components);
  return e.bytes();
}
template <class T, class I>
std::vector<std::uint8_t> invocation(const selected_exact_boundary<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBSEL10";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(selected_exact_boundary_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.raw(a.labeled_digest.bytes.data(), 16);
  e.raw(a.arrangement_digest.bytes.data(), 16);
  e.byte_string(a.canonical_bytes);
  return e.bytes();
}
template <class T, class I>
digest artifact_digest_for(const selected_exact_boundary<T, I> &a) {
  canonical_encoder e;
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(artifact_slot::selected_exact_boundary));
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}
template <class T, class I>
status_or<bool> orient_to_negative_side(const arrangement_complex<T, I> &a,
                                        const global_patch &p,
                                        std::vector<global_vertex_id> &ring) {
  if (ring.size() < 3 || p.uses.empty())
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::boolean_selection,
                      "unoriented_patch_ring");
  auto uses = p.uses;
  std::sort(uses.begin(), uses.end());
  for (auto use_id : uses) {
    if (use_id.value_for_debug() >= a.sheet_uses.size())
      continue;
    const auto &use = a.sheet_uses[use_id.value_for_debug()];
    for (auto halfedge_id : use.boundary) {
      if (halfedge_id.value_for_debug() >= a.halfedges.size())
        continue;
      const auto &h = a.halfedges[halfedge_id.value_for_debug()];
      for (std::size_t i = 0; i < ring.size(); ++i) {
        const auto x = ring[i], y = ring[(i + 1) % ring.size()];
        if (!((h.origin == x && h.destination == y) ||
              (h.origin == y && h.destination == x)))
          continue;
        const bool ring_agrees = h.origin == x;
        const bool source_has_negative_inside =
            use.occupied_side == patch_plane_side::negative;
        if (ring_agrees != source_has_negative_inside)
          std::reverse(ring.begin(), ring.end());
        return true;
      }
    }
  }
  return make_error(boolean_error_code::internal_invariant_error,
                    boolean_stage::boolean_selection, "unoriented_patch_ring");
}
template <class T, class I>
bool independently_reconstruct_topology(const selected_exact_boundary<T, I> &a) {
  const auto &g = *a.arrangement->payload;
  const auto &l = *a.labeled->payload;
  const operation_contract op(a.selected_operation);
  std::map<global_patch_id, const selected_patch *> selected_by_source;
  for (const auto &patch : a.patches)
    if (!selected_by_source.emplace(patch.source, &patch).second)
      return false;

  std::map<global_atomic_edge_id,
           std::vector<std::pair<global_vertex_id, global_vertex_id>>>
      expected_edge_uses;
  std::uint64_t expected_selected = 0;
  for (const auto &source_patch : g.patches) {
    const auto &labels = l.labels(source_patch.id);
    const bool negative = op.occupied(labels.negative_occupancy.in_a,
                                      labels.negative_occupancy.in_b);
    const bool positive = op.occupied(labels.positive_occupancy.in_a,
                                      labels.positive_occupancy.in_b);
    const auto found = selected_by_source.find(source_patch.id);
    if (negative == positive) {
      if (found != selected_by_source.end())
        return false;
      continue;
    }
    ++expected_selected;
    if (found == selected_by_source.end())
      return false;
    const auto &selected = *found->second;
    const auto expected_orientation = negative ? selected_orientation::preserved
                                               : selected_orientation::reversed;
    if (selected.orientation != expected_orientation ||
        selected.cycles.size() != 1 + source_patch.holes.size())
      return false;

    std::vector<std::pair<bool, std::vector<global_vertex_id>>> expected_rings{
        {false, source_patch.outer}};
    for (const auto &hole : source_patch.holes)
      expected_rings.push_back({true, hole});
    for (std::size_t ring_index = 0; ring_index < expected_rings.size();
         ++ring_index) {
      auto &expected = expected_rings[ring_index].second;
      bool orientation_found = false;
      for (const auto use_id : source_patch.uses) {
        if (use_id.value_for_debug() >= g.sheet_uses.size())
          return false;
        const auto &use = g.sheet_uses[use_id.value_for_debug()];
        if (use.patch != source_patch.id)
          return false;
        for (const auto halfedge_id : use.boundary) {
          if (halfedge_id.value_for_debug() >= g.halfedges.size())
            return false;
          const auto &halfedge = g.halfedges[halfedge_id.value_for_debug()];
          for (std::size_t i = 0; i < expected.size(); ++i) {
            const auto x = expected[i], y = expected[(i + 1) % expected.size()];
            if (!((halfedge.origin == x && halfedge.destination == y) ||
                  (halfedge.origin == y && halfedge.destination == x)))
              continue;
            const bool agrees = halfedge.origin == x;
            const bool negative_inside =
                use.occupied_side == patch_plane_side::negative;
            if (agrees != negative_inside)
              std::reverse(expected.begin(), expected.end());
            orientation_found = true;
            break;
          }
          if (orientation_found)
            break;
        }
        if (orientation_found)
          break;
      }
      if (!orientation_found)
        return false;
      if (expected_orientation == selected_orientation::reversed)
        std::reverse(expected.begin(), expected.end());
      std::rotate(expected.begin(),
                  std::min_element(expected.begin(), expected.end()),
                  expected.end());

      const auto cycle_id = selected.cycles[ring_index];
      if (cycle_id.value_for_debug() >= a.cycles.size())
        return false;
      const auto &cycle = a.cycles[cycle_id.value_for_debug()];
      if (cycle.patch != selected.id || cycle.hole != expected_rings[ring_index].first ||
          cycle.halfedges.size() != expected.size())
        return false;
      std::vector<global_vertex_id> actual;
      for (const auto selected_halfedge_id : cycle.halfedges) {
        if (selected_halfedge_id.value_for_debug() >= a.halfedges.size())
          return false;
        const auto &halfedge = a.halfedges[selected_halfedge_id.value_for_debug()];
        if (halfedge.origin.value_for_debug() >= a.vertices.size() ||
            halfedge.destination.value_for_debug() >= a.vertices.size())
          return false;
        const auto origin = a.vertices[halfedge.origin.value_for_debug()].source;
        const auto destination =
            a.vertices[halfedge.destination.value_for_debug()].source;
        actual.push_back(origin);
        if (destination != expected[(actual.size()) % expected.size()])
          return false;
        if (halfedge.edge.value_for_debug() >= a.edges.size())
          return false;
        expected_edge_uses[a.edges[halfedge.edge.value_for_debug()].source]
            .push_back({origin, destination});
      }
      if (actual != expected)
        return false;
    }
  }
  if (expected_selected != a.patches.size() ||
      expected_edge_uses.size() != a.edges.size())
    return false;
  for (const auto &entry : expected_edge_uses)
    if (entry.second.size() != 2 ||
        entry.second[0].first != entry.second[1].second ||
        entry.second[0].second != entry.second[1].first)
      return false;
  return true;
}
template <class T, class I> bool valid(const selected_exact_boundary<T, I> &a) {
  if (!a.labeled || !a.arrangement || a.owner != a.labeled->owner ||
      a.labeled->payload->arrangement.get() != a.arrangement.get() ||
      a.labeled_digest != a.labeled->artifact_digest ||
      a.arrangement_digest != a.arrangement->artifact_digest ||
      a.selected_operation > operation::symmetric_difference)
    return false;
  const auto &g = *a.arrangement->payload;
  const auto &l = *a.labeled->payload;
  if (a.decisions.size() != g.patches.size())
    return false;
  operation_contract op(a.selected_operation);
  std::vector<unsigned> edge_uses(a.edges.size());
  std::vector<unsigned> vertex_uses(a.vertices.size());
  std::vector<unsigned> selected_patch_uses(a.patches.size());
  std::set<global_vertex_id> selected_sources;
  std::set<global_atomic_edge_id> selected_edge_sources;
  std::set<global_patch_id> selected_patch_sources;
  for (std::size_t i = 0; i < a.decisions.size(); ++i) {
    const auto &d = a.decisions[i];
    if (d.id.value_for_debug() != i || d.patch.value_for_debug() != i ||
        d.negative_label.value_for_debug() >= l.side_labels.size() ||
        d.positive_label.value_for_debug() >= l.side_labels.size())
      return false;
    if (d.kind > patch_decision_kind::select_reversed ||
        d.orientation > selected_orientation::reversed)
      return false;
    const auto &nl = l.side_labels[d.negative_label.value_for_debug()];
    const auto &pl = l.side_labels[d.positive_label.value_for_debug()];
    if (nl.patch != d.patch || pl.patch != d.patch ||
        nl.side != patch_plane_side::negative ||
        pl.side != patch_plane_side::positive ||
        nl.occupancy.in_a != d.negative_occupancy.in_a ||
        nl.occupancy.in_b != d.negative_occupancy.in_b ||
        pl.occupancy.in_a != d.positive_occupancy.in_a ||
        pl.occupancy.in_b != d.positive_occupancy.in_b)
      return false;
    bool n = op.occupied(d.negative_occupancy.in_a, d.negative_occupancy.in_b),
         p = op.occupied(d.positive_occupancy.in_a, d.positive_occupancy.in_b);
    if (n != d.result_negative || p != d.result_positive)
      return false;
    if ((n != p) != bool(d.selected))
      return false;
    const auto expected_kind = n == p
        ? (n ? patch_decision_kind::discard_internal
             : patch_decision_kind::discard_exterior)
        : (n ? patch_decision_kind::select_preserved
             : patch_decision_kind::select_reversed);
    const auto expected_orientation = n == p
        ? selected_orientation::none
        : (n ? selected_orientation::preserved : selected_orientation::reversed);
    auto provenance = g.patches[i].uses;
    std::sort(provenance.begin(), provenance.end());
    if (d.kind != expected_kind || d.orientation != expected_orientation ||
        d.provenance != provenance || bool(d.representative) != bool(d.selected))
      return false;
    if (d.selected) {
      if (d.selected->value_for_debug() >= a.patches.size() || provenance.empty() ||
          *d.representative != provenance.front())
        return false;
      const auto &sp = a.patches[d.selected->value_for_debug()];
      if (sp.id != *d.selected || sp.source != d.patch ||
          sp.orientation != d.orientation || sp.representative != *d.representative ||
          sp.provenance != provenance)
        return false;
      ++selected_patch_uses[d.selected->value_for_debug()];
    } else if (d.representative)
      return false;
  }
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    if (a.vertices[i].id.value_for_debug() != i ||
        a.vertices[i].source.value_for_debug() >= g.vertices.size() ||
        !selected_sources.insert(a.vertices[i].source).second ||
        a.vertices[i].symbolic != g.vertices[a.vertices[i].source.value_for_debug()].symbolic)
      return false;
  for (auto n : selected_patch_uses)
    if (n != 1)
      return false;
  for (std::size_t i = 0; i < a.edges.size(); ++i) {
    const auto &x = a.edges[i];
    if (x.id.value_for_debug() != i ||
        x.source.value_for_debug() >= g.edges.size() || x.uses.size() != 2 ||
        !selected_edge_sources.insert(x.source).second || x.uses[0] == x.uses[1])
      return false;
    if (x.uses[0].value_for_debug() >= a.halfedges.size() ||
        x.uses[1].value_for_debug() >= a.halfedges.size())
      return false;
    const auto &h0 = a.halfedges[x.uses[0].value_for_debug()];
    const auto &h1 = a.halfedges[x.uses[1].value_for_debug()];
    if (h0.origin != h1.destination || h0.destination != h1.origin)
      return false;
    if (x.lower.value_for_debug() >= a.vertices.size() ||
        x.upper.value_for_debug() >= a.vertices.size() || x.lower == x.upper)
      return false;
    const auto &source = g.edges[x.source.value_for_debug()];
    const auto lower_source = a.vertices[x.lower.value_for_debug()].source;
    const auto upper_source = a.vertices[x.upper.value_for_debug()].source;
    if (lower_source != source.lower || upper_source != source.upper ||
        h0.edge != x.id || h1.edge != x.id)
      return false;
  }
  for (std::size_t i = 0; i < a.halfedges.size(); ++i) {
    const auto &h = a.halfedges[i];
    if (h.id.value_for_debug() != i ||
        h.edge.value_for_debug() >= a.edges.size() ||
        h.next.value_for_debug() >= a.halfedges.size() ||
        h.previous.value_for_debug() >= a.halfedges.size() ||
        h.patch.value_for_debug() >= a.patches.size() ||
        h.cycle.value_for_debug() >= a.cycles.size() ||
        h.origin.value_for_debug() >= a.vertices.size() ||
        h.destination.value_for_debug() >= a.vertices.size() ||
        a.halfedges[h.next.value_for_debug()].previous != h.id ||
        a.halfedges[h.previous.value_for_debug()].next != h.id)
      return false;
    ++edge_uses[h.edge.value_for_debug()];
    ++vertex_uses[h.origin.value_for_debug()];
  }
  for (auto n : edge_uses)
    if (n != 2)
      return false;
  for (auto n : vertex_uses)
    if (n == 0)
      return false;
  for (std::size_t i = 0; i < a.cycles.size(); ++i) {
    const auto &c = a.cycles[i];
    if (c.id.value_for_debug() != i || c.patch.value_for_debug() >= a.patches.size() ||
        c.halfedges.size() < 3)
      return false;
    for (std::size_t j = 0; j < c.halfedges.size(); ++j) {
      const auto hid = c.halfedges[j];
      if (hid.value_for_debug() >= a.halfedges.size())
        return false;
      const auto &h = a.halfedges[hid.value_for_debug()];
      if (h.cycle != c.id || h.patch != c.patch ||
          h.next != c.halfedges[(j + 1) % c.halfedges.size()] ||
          h.previous != c.halfedges[(j + c.halfedges.size() - 1) % c.halfedges.size()])
        return false;
    }
  }
  for (std::size_t i = 0; i < a.patches.size(); ++i) {
    const auto &p = a.patches[i];
    if (p.id.value_for_debug() != i || p.source.value_for_debug() >= g.patches.size() ||
        p.cycles.empty() || p.orientation == selected_orientation::none ||
        !selected_patch_sources.insert(p.source).second)
      return false;
    for (auto cid : p.cycles)
      if (cid.value_for_debug() >= a.cycles.size() ||
          a.cycles[cid.value_for_debug()].patch != p.id)
        return false;
  }
  if (a.certificate.id.value_for_debug() != 0 ||
      a.certificate.decisions != a.decisions.size() ||
      a.certificate.selected_patches != a.patches.size() ||
      a.certificate.selected_cycles != a.cycles.size() ||
      a.certificate.selected_halfedges != a.halfedges.size() ||
      a.certificate.selected_edges != a.edges.size() ||
      a.certificate.selected_vertices != a.vertices.size())
    return false;
  std::uint64_t discard_exterior = 0, discard_internal = 0,
                select_preserved = 0, select_reversed = 0,
                provenance_uses = 0;
  for (const auto &decision : a.decisions) {
    provenance_uses += decision.provenance.size();
    switch (decision.kind) {
    case patch_decision_kind::discard_exterior:
      ++discard_exterior;
      break;
    case patch_decision_kind::discard_internal:
      ++discard_internal;
      break;
    case patch_decision_kind::select_preserved:
      ++select_preserved;
      break;
    case patch_decision_kind::select_reversed:
      ++select_reversed;
      break;
    }
  }
  if (a.certificate.discard_exterior != discard_exterior ||
      a.certificate.discard_internal != discard_internal ||
      a.certificate.select_preserved != select_preserved ||
      a.certificate.select_reversed != select_reversed ||
      a.certificate.provenance_uses != provenance_uses ||
      discard_exterior + discard_internal + select_preserved +
              select_reversed !=
          a.decisions.size())
    return false;
  std::uint64_t components = 0;
  std::vector<std::vector<selected_patch_id>> adjacency(a.patches.size());
  for (const auto &edge : a.edges) {
    const auto p0 = a.halfedges[edge.uses[0].value_for_debug()].patch;
    const auto p1 = a.halfedges[edge.uses[1].value_for_debug()].patch;
    adjacency[p0.value_for_debug()].push_back(p1);
    adjacency[p1.value_for_debug()].push_back(p0);
  }
  std::vector<bool> reached(a.patches.size());
  for (const auto &patch : a.patches) {
    if (reached[patch.id.value_for_debug()])
      continue;
    ++components;
    std::vector<selected_patch_id> pending{patch.id};
    reached[patch.id.value_for_debug()] = true;
    while (!pending.empty()) {
      const auto current = pending.back();
      pending.pop_back();
      for (const auto next : adjacency[current.value_for_debug()])
        if (!reached[next.value_for_debug()]) {
          reached[next.value_for_debug()] = true;
          pending.push_back(next);
        }
    }
  }
  if (components != a.certificate.connected_components)
    return false;
  return semantic(a) == a.canonical_bytes &&
         invocation(a) == a.artifact_bytes &&
         artifact_digest_for(a) == a.artifact_digest &&
         a.certificate.semantic_digest ==
             domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '1', '0'}},
                           a.canonical_bytes);
}
template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    const auto &a =
        *static_cast<const selected_exact_boundary<T, I> *>(v.payload);
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::boolean_selection;
    r.slot = v.slot;
    r.artifact_type_tag = v.artifact_type_tag;
    r.artifact_schema = v.artifact_schema;
    r.setup_digest = e.setup_digest;
    r.artifact_digest = v.artifact_digest;
    r.invariant_set_digest = s.invariant_set_digest;
    bool ok = valid(a) && independently_reconstruct_topology(a) &&
              a.selected_operation == e.op;
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
    r.dependency_digests = {a.labeled_digest, a.arrangement_digest};
    auto b = encode_verification_report(r);
    if (!b.has_value())
      return b.error();
    r.report_digest =
        domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}}, b.value());
    return r;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::boolean_selection,
                      "selection_verifier_exception");
  }
}
template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}
} // namespace

status_or<bool> register_boolean_selection_verifier(verifier_registry &r,
                                                    coordinate_tag c,
                                                    index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::selected_exact_boundary;
  x.artifact_type_tag = selected_exact_boundary_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = selected_exact_boundary_schema;
  x.mandatory = {
      invariant_code::selection_binding, invariant_code::selection_decisions,
      invariant_code::selection_topology, invariant_code::selection_orientation,
      invariant_code::selection_canonical_encoding};
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
    std::shared_ptr<const published_artifact<selected_exact_boundary<T, I>>>>
select_boolean_boundary(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::boolean_selection, "cancelled");
    if (auto old =
            ctx.artifacts().latest(artifact_slot::selected_exact_boundary))
      return std::static_pointer_cast<
          const published_artifact<selected_exact_boundary<T, I>>>(old);
    auto lr = classify_arrangement_cells(ctx);
    if (!lr.has_value())
      return lr.error();
    auto labeled = lr.value();
    if (ctx.artifacts().latest_generation(artifact_slot::labeled_arrangement) !=
        labeled->generation)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::boolean_selection, "stale_labels");
    const auto &l = *labeled->payload;
    const auto &g = *l.arrangement->payload;
    selected_exact_boundary<T, I> a;
    a.owner = ctx.owner();
    a.selected_operation = ctx.contract().selected_operation();
    a.setup_digest = ctx.replay().setup;
    a.labeled_digest = labeled->artifact_digest;
    a.arrangement_digest = l.arrangement->artifact_digest;
    a.labeled = labeled;
    a.arrangement = l.arrangement;
    a.constructions = l.constructions;
    for (const auto &p : g.patches) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::boolean_selection, "cancelled");
      const auto &labels = l.labels(p.id);
      patch_selection_decision d;
      d.id =
          patch_selection_decision_id::from_canonical_value(a.decisions.size());
      d.patch = p.id;
      d.negative_label = labels.negative;
      d.positive_label = labels.positive;
      d.negative_occupancy = labels.negative_occupancy;
      d.positive_occupancy = labels.positive_occupancy;
      d.result_negative = ctx.contract().occupied(d.negative_occupancy.in_a,
                                                  d.negative_occupancy.in_b);
      d.result_positive = ctx.contract().occupied(d.positive_occupancy.in_a,
                                                  d.positive_occupancy.in_b);
      d.provenance = p.uses;
      std::sort(d.provenance.begin(), d.provenance.end());
      if (d.result_negative == d.result_positive) {
        d.kind = d.result_negative ? patch_decision_kind::discard_internal
                                   : patch_decision_kind::discard_exterior;
      } else {
        d.orientation = d.result_negative ? selected_orientation::preserved
                                          : selected_orientation::reversed;
        d.kind = d.result_negative ? patch_decision_kind::select_preserved
                                   : patch_decision_kind::select_reversed;
        if (d.provenance.empty())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::boolean_selection,
                            "uncovered_patch");
        d.representative = d.provenance.front();
        d.selected = selected_patch_id::from_canonical_value(a.patches.size());
        selected_patch sp;
        sp.id = *d.selected;
        sp.source = p.id;
        sp.orientation = d.orientation;
        sp.representative = *d.representative;
        sp.provenance = d.provenance;
        a.patches.push_back(std::move(sp));
      }
      a.decisions.push_back(std::move(d));
    }
    std::map<global_vertex_id, selected_vertex_id> vertex_map;
    std::map<global_atomic_edge_id, selected_edge_id> edge_map;
    std::map<std::pair<global_vertex_id, global_vertex_id>,
             global_atomic_edge_id>
        source_edges;
    for (const auto &e : g.edges)
      if (!source_edges.emplace(std::make_pair(e.lower, e.upper), e.id).second)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::boolean_selection,
                          "ambiguous_cycle_edge");
    auto vertex = [&](global_vertex_id source) {
      auto it = vertex_map.find(source);
      if (it != vertex_map.end())
        return it->second;
      auto id = selected_vertex_id::from_canonical_value(a.vertices.size());
      vertex_map[source] = id;
      if (source.value_for_debug() >= g.vertices.size())
        throw std::logic_error("selected vertex source out of range");
      a.vertices.push_back({id, source, g.vertices[source.value_for_debug()].symbolic});
      return id;
    };
    for (auto &sp : a.patches) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::boolean_selection, "cancelled");
      const auto &gp = g.patches[sp.source.value_for_debug()];
      std::vector<std::pair<bool, std::vector<global_vertex_id>>> rings{
          {false, gp.outer}};
      for (const auto &h : gp.holes)
        rings.push_back({true, h});
      for (auto ring : rings) {
        auto oriented = orient_to_negative_side(g, gp, ring.second);
        if (!oriented.has_value())
          return oriented.error();
        if (sp.orientation == selected_orientation::reversed)
          std::reverse(ring.second.begin(), ring.second.end());
        auto least = std::min_element(ring.second.begin(), ring.second.end());
        std::rotate(ring.second.begin(), least, ring.second.end());
        selected_cycle cycle;
        cycle.id = selected_cycle_id::from_canonical_value(a.cycles.size());
        cycle.patch = sp.id;
        cycle.hole = ring.first;
        for (std::size_t i = 0; i < ring.second.size(); ++i) {
          auto gv0 = ring.second[i],
               gv1 = ring.second[(i + 1) % ring.second.size()];
          auto key = std::minmax(gv0, gv1);
          auto ge = source_edges.find(key);
          if (ge == source_edges.end())
            return make_error(boolean_error_code::internal_invariant_error,
                              boolean_stage::boolean_selection,
                              "missing_cycle_edge");
          auto ei = edge_map.find(ge->second);
          selected_edge_id eid;
          if (ei == edge_map.end()) {
            eid = selected_edge_id::from_canonical_value(a.edges.size());
            edge_map[ge->second] = eid;
            const auto &source = g.edges[ge->second.value_for_debug()];
            a.edges.push_back({eid,
                               ge->second,
                               vertex(source.lower),
                               vertex(source.upper),
                               {}});
          } else
            eid = ei->second;
          auto hid =
              selected_halfedge_id::from_canonical_value(a.halfedges.size());
          a.halfedges.push_back(
              {hid, sp.id, cycle.id, eid, vertex(gv0), vertex(gv1), {}, {}});
          a.edges[eid.value_for_debug()].uses.push_back(hid);
          cycle.halfedges.push_back(hid);
        }
        for (std::size_t i = 0; i < cycle.halfedges.size(); ++i) {
          auto &h = a.halfedges[cycle.halfedges[i].value_for_debug()];
          h.next = cycle.halfedges[(i + 1) % cycle.halfedges.size()];
          h.previous = cycle.halfedges[(i + cycle.halfedges.size() - 1) %
                                       cycle.halfedges.size()];
        }
        sp.cycles.push_back(cycle.id);
        a.cycles.push_back(std::move(cycle));
      }
    }
    for (const auto &e : a.edges) {
      if (ctx.cancelled())
        return make_error(boolean_error_code::resource_limit,
                          boolean_stage::boolean_selection, "cancelled");
      if (e.uses.size() != 2)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::boolean_selection,
                          "nonmanifold_selected_edge");
      const auto &h0 = a.halfedges[e.uses[0].value_for_debug()];
      const auto &h1 = a.halfedges[e.uses[1].value_for_debug()];
      if (h0.origin != h1.destination || h0.destination != h1.origin)
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::boolean_selection,
                          "same_direction_selected_edge");
    }
    std::vector<std::set<selected_patch_id>> adj(a.patches.size());
    for (const auto &e : a.edges) {
      auto x = a.halfedges[e.uses[0].value_for_debug()].patch,
           y = a.halfedges[e.uses[1].value_for_debug()].patch;
      adj[x.value_for_debug()].insert(y);
      adj[y.value_for_debug()].insert(x);
    }
    std::vector<bool> seen(a.patches.size());
    for (const auto &p : a.patches)
      if (!seen[p.id.value_for_debug()]) {
        ++a.certificate.connected_components;
        std::vector<selected_patch_id> q{p.id};
        seen[p.id.value_for_debug()] = true;
        while (!q.empty()) {
          if (ctx.cancelled())
            return make_error(boolean_error_code::resource_limit,
                              boolean_stage::boolean_selection, "cancelled");
          auto x = q.back();
          q.pop_back();
          for (auto y : adj[x.value_for_debug()])
            if (!seen[y.value_for_debug()]) {
              seen[y.value_for_debug()] = true;
              q.push_back(y);
            }
        }
      }
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::boolean_selection, "cancelled");
    a.certificate.id = selection_certificate_id::from_canonical_value(0);
    a.certificate.decisions = a.decisions.size();
    a.certificate.selected_patches = a.patches.size();
    a.certificate.selected_cycles = a.cycles.size();
    a.certificate.selected_halfedges = a.halfedges.size();
    a.certificate.selected_edges = a.edges.size();
    a.certificate.selected_vertices = a.vertices.size();
    for (const auto &decision : a.decisions) {
      a.certificate.provenance_uses += decision.provenance.size();
      switch (decision.kind) {
      case patch_decision_kind::discard_exterior:
        ++a.certificate.discard_exterior;
        break;
      case patch_decision_kind::discard_internal:
        ++a.certificate.discard_internal;
        break;
      case patch_decision_kind::select_preserved:
        ++a.certificate.select_preserved;
        break;
      case patch_decision_kind::select_reversed:
        ++a.certificate.select_reversed;
        break;
      }
    }
    a.canonical_bytes = semantic(a);
    a.certificate.semantic_digest = domain_digest(
        {{'Y', 'G', 'B', 'C', 'A', 'N', '1', '0'}}, a.canonical_bytes);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    std::vector<resource_reservation> charges;
    auto reserve = [&](resource_kind k, std::uint64_t n) -> status_or<bool> {
      auto q = ctx.accountant().reserve_scoped(
          k, n, boolean_stage::boolean_selection);
      if (!q.has_value())
        return q.error();
      charges.push_back(std::move(q.value()));
      return true;
    };
    for (auto q : {std::make_pair(resource_kind::selection_decisions,
                                  std::uint64_t(a.decisions.size())),
                   std::make_pair(resource_kind::selected_patches,
                                  std::uint64_t(a.patches.size())),
                   std::make_pair(resource_kind::selected_cycles,
                                  std::uint64_t(a.cycles.size())),
                   std::make_pair(resource_kind::selected_halfedges,
                                  std::uint64_t(a.halfedges.size())),
                   std::make_pair(resource_kind::selected_edges,
                                  std::uint64_t(a.edges.size())),
                   std::make_pair(resource_kind::selected_vertices,
                                  std::uint64_t(a.vertices.size())),
                   std::make_pair(resource_kind::selection_provenance,
                                  a.certificate.provenance_uses)}) {
      auto ok = reserve(q.first, q.second);
      if (!ok.has_value())
        return ok.error();
    }
    auto ptr =
        std::make_shared<const selected_exact_boundary<T, I>>(std::move(a));
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::boolean_selection,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::selected_exact_boundary, type_tag<T, I>(),
        selected_exact_boundary_schema, ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    artifact_view view{ctx.owner(),
                       artifact_slot::selected_exact_boundary,
                       type_tag<T, I>(),
                       selected_exact_boundary_schema,
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
    stage_transaction<selected_exact_boundary<T, I>,
                      selected_exact_boundary<T, I>>
        tx(ctx.owner(), boolean_stage::boolean_selection,
           artifact_slot::selected_exact_boundary,
           std::make_unique<selected_exact_boundary<T, I>>());
    for (auto &charge : charges)
      tx.stage_reservation(std::move(charge));
    auto ok = tx.verify(ptr, view, spec.value(), env, ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::boolean_selection, "cancelled");
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::boolean_selection, "selection_allocation");
  } catch (const std::exception &e) {
    auto x =
        make_error(boolean_error_code::internal_invariant_error,
                   boolean_stage::boolean_selection, "selection_exception");
    x.detail = e.what();
    return x;
  }
}
#define INST(T, I)                                                             \
  template status_or<std::shared_ptr<                                          \
      const published_artifact<selected_exact_boundary<T, I>>>>                \
  select_boolean_boundary(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST
} // namespace mesh_boolean
} // namespace ygor
