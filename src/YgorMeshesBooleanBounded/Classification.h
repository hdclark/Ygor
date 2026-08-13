#pragma once

#include "BoundedShellQuery.h"
#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "ClassificationComplex.h"
#include "ClassificationTypes.h"
#include "PrecisionContext.h"
#include "SignedFeatureRelations.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>
build_classification_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    classification_capabilities capabilities,
    classification_codec_limits codec_limits = {},
    classification_verifier_limits verifier_limits = {});

#define YGOR_DECLARE_CLASSIFICATION_BUILD(T, I)                              \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const classification_complex<T, I>>>                   \
  build_classification_complex<T, I>(                                        \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      classification_capabilities, classification_codec_limits,              \
      classification_verifier_limits)

YGOR_DECLARE_CLASSIFICATION_BUILD(float, std::uint32_t);
YGOR_DECLARE_CLASSIFICATION_BUILD(float, std::uint64_t);
YGOR_DECLARE_CLASSIFICATION_BUILD(double, std::uint32_t);
YGOR_DECLARE_CLASSIFICATION_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_CLASSIFICATION_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
#include "ClassificationCodec.h"
#include "ClassificationVerifier.h"

namespace ygor::mesh_boolean::bounded {

namespace classification_detail {

using shell_winding_map =
    std::vector<std::pair<std::uint64_t, std::int64_t>>;

inline void accumulate_shell(shell_winding_map &map, std::uint64_t shell,
                             std::int64_t delta) {
  auto it = map.begin();
  while (it != map.end() && it->first < shell)
    ++it;
  if (it != map.end() && it->first == shell) {
    it->second += delta;
    if (it->second == 0)
      map.erase(it);
  } else {
    map.insert(it, {shell, delta});
  }
}

inline std::int64_t total_winding(const shell_winding_map &map) {
  std::int64_t sum = 0;
  for (const auto &entry : map)
    sum += entry.second;
  return sum;
}

inline shell_winding_map negate_winding(const shell_winding_map &map) {
  shell_winding_map out = map;
  for (auto &entry : out)
    entry.second = -entry.second;
  return out;
}

inline bool inverse_windings(const shell_winding_map &a,
                             const shell_winding_map &b) {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].first != b[i].first || a[i].second != -b[i].second)
      return false;
  return true;
}

template <class T>
std::array<T, 2> project_2d(const std::array<T, 3> &point,
                            std::uint8_t dropped_axis) {
  std::array<T, 2> out{};
  std::size_t slot = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (axis == dropped_axis)
      continue;
    out[slot++] = point[axis];
  }
  return out;
}

// Nominal coordinate + enclosure of a bounded point reference (source vertex or
// constructed intersection point).
template <class T>
struct point_geometry {
  std::array<T, 3> nominal{};
  std::array<finite_interval<T>, 3> enclosure{};
};

template <class T, class I>
std::optional<point_geometry<T>> source_vertex_geometry(
    const canonical_halfedge_operand<T, I> &manifold,
    const relation_feature_key &source_vertex) {
  if (source_vertex.kind != relation_feature_kind::source_vertex)
    return std::nullopt;
  if (source_vertex.primary >= manifold.source_vertex_to_vertex().size())
    return std::nullopt;
  const auto vertex = manifold.source_vertex_to_vertex()[source_vertex.primary];
  if (vertex >= manifold.vertices().size())
    return std::nullopt;
  const auto &record = manifold.vertices()[vertex];
  point_geometry<T> out;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal[axis] = record.committed_point[axis];
    const auto enclosure = finite_interval<T>::create(record.lower[axis],
                                                      record.upper[axis]);
    if (!enclosure)
      return std::nullopt;
    out.enclosure[axis] = *enclosure;
  }
  return out;
}

template <class T, class I>
std::optional<point_geometry<T>> occurrence_geometry(
    const canonical_intersection_complex<T, I> &intersections,
    const signed_feature_relations<T, I> &relations,
    const canonical_halfedge_operand<T, I> &manifold, event_occurrence_id id) {
  const auto *occurrence = intersections.occurrence(id, intersections.owner());
  if (!occurrence)
    return std::nullopt;
  const auto *event = intersections.event(occurrence->event, intersections.owner());
  if (!event)
    return std::nullopt;
  const auto &point = event->point;
  if (point.kind == bounded_point_reference_kind::source_point) {
    return source_vertex_geometry(manifold, point.source_vertex);
  }
  if (point.construction.ordinal() >= relations.constructions().size())
    return std::nullopt;
  const auto &construction = relations.constructions()[point.construction.ordinal()];
  point_geometry<T> out;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    using bits_type = floating_uint_t<T>;
    out.nominal[axis] =
        from_bits<T>(static_cast<bits_type>(construction.nominal_bits[axis]));
    const auto enclosure = finite_interval<T>::create(
        from_bits<T>(static_cast<bits_type>(construction.lower_bits[axis])),
        from_bits<T>(static_cast<bits_type>(construction.upper_bits[axis])));
    if (!enclosure)
      return std::nullopt;
    out.enclosure[axis] = *enclosure;
  }
  return out;
}

// A classification cell (positive-area atom region) in a facet's 2D frame.
template <class T> struct cell_polygon {
  std::array<T, 3> nominal_origin{};
  std::vector<std::array<T, 2>> ring2d;
  std::vector<std::array<T, 3>> ring3d;
  std::vector<finite_interval<T>> // per-axis radial enclosure of the witness
      witness_enclosure;
  std::uint64_t shell = 0;
  std::uint64_t source_facet = 0;
  std::uint8_t dropped_axis = 0;
  // Per-edge tags parallel to ring2d (edge i connects i -> i+1).
  enum class edge_kind : std::uint8_t {
    source_edge = 1,
    carrier = 2,
    diagonal = 3,
  };
  struct edge_tag {
    edge_kind kind = edge_kind::source_edge;
    std::uint64_t lineage = 0; // source-edge ordinal / carrier span ordinal
    std::int32_t delta = 0;    // crossing delta for carrier edges
    bool contact_delimiter = false;
  };
  std::vector<edge_tag> edges;
};

template <class T>
bool point_strictly_left(const std::array<T, 2> &a, const std::array<T, 2> &b,
                         const std::array<T, 2> &c, std::int8_t &sign) {
  const auto orient = exact_orient_2d(a, b, c);
  if (orient.status == exact_relation_status::exact_negative)
    sign = -1;
  else if (orient.status == exact_relation_status::exact_positive)
    sign = 1;
  else if (orient.status == exact_relation_status::exact_zero)
    sign = 0;
  else
    return false;
  return true;
}

template <class T>
std::optional<std::array<T, 2>>
line_edge_intersection(const std::array<T, 2> &p, const std::array<T, 2> &q,
                       const std::array<T, 2> &a, const std::array<T, 2> &b) {
  // Intersect the infinite line through (p,q) with the closed edge segment
  // (a,b). Exact orientation decides the crossing; the intersection coordinate
  // is a direct convex combination on nominal values.
  std::int8_t s_pq_a = 0, s_pq_b = 0;
  if (!point_strictly_left(p, q, a, s_pq_a) ||
      !point_strictly_left(p, q, b, s_pq_b))
    return std::nullopt;
  if (s_pq_a == 0)
    return a;
  if (s_pq_b == 0)
    return b;
  if (s_pq_a == s_pq_b)
    return std::nullopt; // same side of the line, no crossing
  const T abx = b[0] - a[0], aby = b[1] - a[1];
  const T pqx = q[0] - p[0], pqy = q[1] - p[1];
  const T denom = abx * pqy - aby * pqx;
  if (denom == T(0))
    return std::nullopt;
  const T ax = a[0], ay = a[1], px = p[0], py = p[1];
  const T t = ((px - ax) * pqy - (py - ay) * pqx) / denom;
  std::array<T, 2> out{{ax + t * abx, ay + t * aby}};
  return out;
}

// Split a convex cell polygon by a polyline path whose first and last points lie
// on the polygon boundary. Preserves per-edge provenance and assigns the path
// segments their authoritative carrier delta.
template <class T>
using cell_edge_kind = typename cell_polygon<T>::edge_kind;
template <class T>
using cell_edge_tag = typename cell_polygon<T>::edge_tag;

// Split a convex cell polygon by the full line through two reference points,
// preserving per-edge provenance. The chord edge receives the authoritative
// carrier delta; all other edges retain their source-edge tags.
template <class T>
bool split_cell_by_line(
    const std::vector<std::array<T, 2>> &ring2d,
    const std::vector<std::array<T, 3>> &ring3d,
    const std::vector<cell_edge_tag<T>> &edges,
    const std::array<T, 2> &p, const std::array<T, 2> &q, std::int32_t delta,
    std::uint64_t lineage, std::vector<std::array<T, 2>> &ring1,
    std::vector<std::array<T, 3>> &ring1_3d, std::vector<cell_edge_tag<T>> &edges1,
    std::vector<std::array<T, 2>> &ring2,
    std::vector<std::array<T, 3>> &ring2_3d,
    std::vector<cell_edge_tag<T>> &edges2) {
  const std::size_t n = ring2d.size();
  if (n < 3)
    return false;
  std::vector<std::int8_t> signs(n);
  for (std::size_t i = 0; i < n; ++i)
    if (!point_strictly_left(p, q, ring2d[i], signs[i]))
      return false;
  // Crossing edges: opposite nonzero signs. A vertex exactly on the line is
  // treated as belonging to neither crossing pair (degenerate).
  std::vector<std::size_t> crossings;
  for (std::size_t i = 0; i < n; ++i) {
    const std::int8_t sa = signs[i];
    const std::int8_t sb = signs[(i + 1) % n];
    if (sa != 0 && sb != 0 && sa != sb)
      crossings.push_back(i);
  }
  if (crossings.size() != 2)
    return false; // line misses, grazes, or is degenerate for this polygon
  const std::size_t e1 = crossings[0];
  const std::size_t e2 = crossings[1];

  const auto hit1 = line_edge_intersection(p, q, ring2d[e1], ring2d[(e1 + 1) % n]);
  const auto hit2 = line_edge_intersection(p, q, ring2d[e2], ring2d[(e2 + 1) % n]);
  if (!hit1 || !hit2)
    return false;

  auto interpolate_3d = [&](const std::array<T, 2> &hit, std::size_t edge) {
    const auto &a2 = ring2d[edge];
    const auto &b2 = ring2d[(edge + 1) % n];
    const auto &a3 = ring3d[edge];
    const auto &b3 = ring3d[(edge + 1) % n];
    std::array<T, 3> out{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      T t = T(0);
      const T d2 = b2[0] - a2[0];
      if (d2 != T(0))
        t = (hit[0] - a2[0]) / d2;
      else {
        const T d2y = b2[1] - a2[1];
        t = d2y != T(0) ? (hit[1] - a2[1]) / d2y : T(0);
      }
      out[axis] = a3[axis] + t * (b3[axis] - a3[axis]);
    }
    return out;
  };

  const auto hit1_3d = interpolate_3d(*hit1, e1);
  const auto hit2_3d = interpolate_3d(*hit2, e2);

  // Sub-polygon 1: hit1 -> walk ring (e1+1 .. e2) -> hit2 -> chord back.
  // Edges: edges[e1] (remnant), edges[e1+1..e2-1], edges[e2] (remnant), chord.
  ring1.push_back(*hit1);
  ring1_3d.push_back(hit1_3d);
  edges1.push_back(edges[e1]);
  for (std::size_t cursor = (e1 + 1) % n;; cursor = (cursor + 1) % n) {
    ring1.push_back(ring2d[cursor]);
    ring1_3d.push_back(ring3d[cursor]);
    if (cursor == e2)
      break;
    edges1.push_back(edges[cursor]);
  }
  ring1.push_back(*hit2);
  ring1_3d.push_back(hit2_3d);
  edges1.push_back(edges[e2]);
  {
    cell_edge_tag<T> tag;
    tag.kind = cell_edge_kind<T>::carrier;
    tag.lineage = lineage;
    tag.delta = delta;
    edges1.push_back(tag);
  }

  // Sub-polygon 2: hit2 -> walk ring (e2+1 .. e1) -> hit1 -> chord back.
  ring2.push_back(*hit2);
  ring2_3d.push_back(hit2_3d);
  edges2.push_back(edges[e2]);
  for (std::size_t cursor = (e2 + 1) % n;; cursor = (cursor + 1) % n) {
    ring2.push_back(ring2d[cursor]);
    ring2_3d.push_back(ring3d[cursor]);
    if (cursor == e1)
      break;
    edges2.push_back(edges[cursor]);
  }
  ring2.push_back(*hit1);
  ring2_3d.push_back(hit1_3d);
  edges2.push_back(edges[e1]);
  {
    cell_edge_tag<T> tag;
    tag.kind = cell_edge_kind<T>::carrier;
    tag.lineage = lineage;
    tag.delta = delta;
    edges2.push_back(tag);
  }
  return ring1.size() >= 3 && ring2.size() >= 3;
}

} // namespace classification_detail

// A bounded shell query is the single authoritative inclusion oracle. We wrap
// it behind the direction policy and a witness point.
template <class T, class I>
shell_query_result<T> run_seed_query(const canonical_halfedge_operand<T, I> &opposite,
                                     const std::array<T, 3> &nominal,
                                     const primitive_direction &direction) {
  shell_query_point<T> origin;
  origin.nominal = nominal;
  for (std::size_t axis = 0; axis < 3; ++axis)
    origin.enclosure[axis] = finite_interval<T>::singleton(nominal[axis]);
  return bounded_shell_query(opposite, origin, direction);
}

// ---------------------------------------------------------------------------
// The per-direction classifier. It consumes the immutable Component 05-08
// artifacts and appends atoms, adjacency, groups, quotient edges, propagation
// components, seed queries, and side labels for one operand direction.
// ---------------------------------------------------------------------------
template <class T, class I> class classification_builder {
public:
  classification_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
      std::shared_ptr<const signed_feature_relations<T, I>> relations,
      std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
      classification_capabilities capabilities,
      classification_codec_limits codec_limits = {},
      classification_verifier_limits verifier_limits = {})
      : context_(context), precision_(precision), manifolds_(std::move(manifolds)),
        relations_(std::move(relations)),
        intersections_(std::move(intersections)),
        capabilities_(std::move(capabilities)), codec_limits_(codec_limits),
        verifier_limits_(verifier_limits) {}

  boolean_outcome<std::shared_ptr<const classification_complex<T, I>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::failure(error);
    if (classification_cancelled(capabilities_,
                                 classification_checkpoint::predecessor_validation))
      return fail_cancelled<std::shared_ptr<const classification_complex<T, I>>>(
          classification_checkpoint::predecessor_validation);

    auto artifact = std::make_shared<classification_complex<T, I>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->precision_digest_ = precision_.digest();
    artifact->relation_digest_ = relations_->digest();
    artifact->intersection_digest_ = intersections_->digest();
    artifact->manifold_digests_[0] = manifolds_->a()->digest();
    artifact->manifold_digests_[1] = manifolds_->b()->digest();

    for (operand_id operand : {operand_id::a, operand_id::b}) {
      if (classification_cancelled(capabilities_,
                                   classification_checkpoint::triangle_incidence_proposals))
        return fail_cancelled<std::shared_ptr<const classification_complex<T, I>>>(
            classification_checkpoint::triangle_incidence_proposals);
      auto outcome = classify_direction(operand, *artifact);
      if (!outcome.has_value())
        return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::failure(*outcome.error());
    }

    // Verify, encode, digest.
    if (!verify_classification_complex(*artifact, *relations_, *intersections_,
                                       *manifolds_, error)) {
      return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::failure(error);
    }
    artifact->verification_ = classification_verification_disposition::independently_verified;
    std::vector<std::uint8_t> bytes;
    if (!encode_classification_complex(*artifact, bytes, codec_limits_, error)) {
      return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::failure(error);
    }
    artifact->canonical_bytes_ = std::move(bytes);
    artifact->digest_ = sha256::digest(artifact->canonical_bytes_);
    artifact->statistics_.persistent_bytes = artifact->canonical_bytes_.size();
    artifact->statistics_.canonical_bytes = artifact->canonical_bytes_.size();
    if (classification_cancelled(capabilities_,
                                 classification_checkpoint::commit))
      return fail_cancelled<std::shared_ptr<const classification_complex<T, I>>>(
          classification_checkpoint::commit);
    return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::success(std::move(artifact));
  }

private:
  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  std::shared_ptr<const signed_feature_relations<T, I>> relations_;
  std::shared_ptr<const canonical_intersection_complex<T, I>> intersections_;
  classification_capabilities capabilities_;
  classification_codec_limits codec_limits_{};
  classification_verifier_limits verifier_limits_{};

  template <class R>
  boolean_outcome<R> fail_cancelled(classification_checkpoint checkpoint) {
    bounded_boolean_error error = classification_error(
        classification_subcode::cancelled, bounded_boolean_error_category::cancelled,
        "classification cancelled", checkpoint);
    return boolean_outcome<R>::failure(error);
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    const auto &a = *manifolds_->a();
    const auto &b = *manifolds_->b();
    if (!a.owner().same_owner(context_.owner) || !b.owner().same_owner(context_.owner) ||
        !relations_->owner().same_owner(context_.owner) ||
        !intersections_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = classification_error(classification_subcode::wrong_owner,
                                   bounded_boolean_error_category::internal_invariant_error,
                                   "classification predecessor owner mismatch",
                                   classification_checkpoint::predecessor_validation);
      return false;
    }
    if (relations_->operation() != context_.operation ||
        intersections_->operation() != context_.operation) {
      error = classification_error(classification_subcode::wrong_operation,
                                   bounded_boolean_error_category::internal_invariant_error,
                                   "classification predecessor operation mismatch",
                                   classification_checkpoint::predecessor_validation);
      return false;
    }
    if (relations_->verification() != relation_verification_disposition::independently_verified ||
        intersections_->verification() != intersection_verification_disposition::independently_verified ||
        a.verification() != canonical_halfedge_verification_disposition::independently_verified ||
        b.verification() != canonical_halfedge_verification_disposition::independently_verified) {
      error = classification_error(classification_subcode::predecessor_not_verified,
                                   bounded_boolean_error_category::internal_invariant_error,
                                   "classification predecessor is not verified",
                                   classification_checkpoint::predecessor_validation);
      return false;
    }
    if (capabilities_.provider_version != contract_versions::classification_provider ||
        capabilities_.atom_domain_version != contract_versions::atom_domain_schema ||
        capabilities_.codec_version != contract_versions::classification_codec ||
        capabilities_.verifier_version != contract_versions::classification_verifier) {
      error = classification_error(classification_subcode::unsupported_version,
                                   bounded_boolean_error_category::input_contract_error,
                                   "classification capability version mismatch",
                                   classification_checkpoint::capability_validation);
      return false;
    }
    return true;
  }

  const canonical_halfedge_operand<T, I> &operand_manifold(operand_id id) const {
    return id == operand_id::a ? *manifolds_->a() : *manifolds_->b();
  }

  // Collect the transverse carrier cut paths crossing a source facet. Each path
  // is a maximal chain of active spans connected through shared occurrences,
  // carrying the authoritative signed crossing delta per segment.
  struct carrier_path {
    std::vector<std::array<T, 3>> points;
    std::vector<std::int32_t> deltas;
    std::vector<std::uint64_t> lineages;
  };

  boolean_outcome<std::vector<carrier_path>> collect_carrier_paths(
      operand_id operand, std::uint64_t source_facet, bounded_boolean_error &error) {
    (void)error;
    const auto &ic = *intersections_;
    const auto &manifold = operand_manifold(operand);
    const auto &spans = ic.carrier_active_spans();
    const auto &clusters = ic.carrier_clusters();
    const auto &cluster_occurrences = ic.carrier_cluster_occurrence_index();
    const auto &carriers = ic.transverse_carriers();
    const auto &span_index = ic.transverse_carrier_span_index();

    std::map<std::uint64_t, std::int32_t> span_delta;
    for (const auto &descriptor : ic.descriptors()) {
      if (descriptor.key.locus != intersection_descriptor_locus::transverse_active_span)
        continue;
      if (!descriptor.classification_consumable)
        continue;
      span_delta[descriptor.key.parent_lineage] = descriptor.signed_crossing_delta;
    }

    struct span_entry {
      std::uint64_t span_ordinal = 0;
      std::int32_t delta = 0;
      event_occurrence_id left_occ{0};
      event_occurrence_id right_occ{0};
      std::array<T, 3> left_point{};
      std::array<T, 3> right_point{};
    };
    std::vector<span_entry> entries;
    for (const auto &carrier : carriers) {
      const bool first_touches =
          carrier.key.first_facet.kind == relation_feature_kind::source_facet &&
          carrier.key.first_facet.operand == operand &&
          carrier.key.first_facet.primary == source_facet;
      const bool second_touches =
          carrier.key.second_facet.kind == relation_feature_kind::source_facet &&
          carrier.key.second_facet.operand == operand &&
          carrier.key.second_facet.primary == source_facet;
      const bool touches = first_touches || second_touches;
      if (!touches || !checked_range(carrier.active_spans, span_index.size()))
        continue;
      for (std::uint64_t offset = 0; offset < carrier.active_spans.count; ++offset) {
        const auto span_id = span_index[carrier.active_spans.begin + offset];
        if (span_id.ordinal() >= spans.size())
          continue;
        const auto &span = spans[span_id.ordinal()];
        if (!span.classification_cut && !span.contact_delimiter)
          continue;
        const auto delta_it = span_delta.find(span_id.ordinal());
        if (delta_it == span_delta.end())
          continue;
        if (span.left.ordinal() >= clusters.size() ||
            span.right.ordinal() >= clusters.size())
          continue;
        const auto &lc = clusters[span.left.ordinal()];
        const auto &rc = clusters[span.right.ordinal()];
        if (!checked_range(lc.occurrence_members, cluster_occurrences.size()) ||
            !checked_range(rc.occurrence_members, cluster_occurrences.size()) ||
            lc.occurrence_members.count == 0 || rc.occurrence_members.count == 0)
          continue;
        const auto left_occ = cluster_occurrences[lc.occurrence_members.begin];
        const auto right_occ = cluster_occurrences[rc.occurrence_members.begin];
        auto p = classification_detail::occurrence_geometry(ic, *relations_, manifold, left_occ);
        auto q = classification_detail::occurrence_geometry(ic, *relations_, manifold, right_occ);
        if (!p || !q)
          continue;
        span_entry entry;
        entry.span_ordinal = span_id.ordinal();
        entry.delta = delta_it->second;
        entry.left_occ = left_occ;
        entry.right_occ = right_occ;
        entry.left_point = p->nominal;
        entry.right_point = q->nominal;
        entries.push_back(std::move(entry));
      }
    }
    std::sort(entries.begin(), entries.end(),
              [](const span_entry &a, const span_entry &b) noexcept {
                return a.span_ordinal < b.span_ordinal;
              });

    // Emit one full-line cut per active span. Splitting by the full carrier line
    // (rather than only the active sub-segment) keeps every cell convex, which
    // is required for the certified vertex-average witness. Inactive line
    // portions later merge through zero-delta adjacency when their crossing is
    // zero; a nonzero span delta authoritatively separates the two sides.
    std::vector<bool> used(entries.size(), false);
    std::vector<carrier_path> paths;
    for (std::size_t i = 0; i < entries.size(); ++i) {
      const auto &entry = entries[i];
      carrier_path path;
      path.points.push_back(entry.left_point);
      path.points.push_back(entry.right_point);
      path.deltas.push_back(entry.delta);
      path.lineages.push_back(entry.span_ordinal);
      paths.push_back(std::move(path));
    }
    std::sort(paths.begin(), paths.end(),
              [](const carrier_path &a, const carrier_path &b) noexcept {
                return a.lineages < b.lineages;
              });
    return boolean_outcome<std::vector<carrier_path>>::success(std::move(paths));
  }

  boolean_outcome<void> classify_direction(operand_id operand,
                                           classification_complex<T, I> &artifact) {
    bounded_boolean_error error;
    const auto &manifold = operand_manifold(operand);
    const auto &opposite = operand_manifold(operand == operand_id::a ? operand_id::b
                                                                     : operand_id::a);
    // Map source facet -> its facet group for the ring.
    const auto &facet_groups = manifold.facet_groups();
    // Map source edge ordinal -> collinear-overlap cut delta (from source-edge
    // interval descriptors).
    std::map<std::uint64_t, std::int32_t> edge_cut_delta;
    for (const auto &descriptor : intersections_->descriptors()) {
      if (descriptor.key.locus != intersection_descriptor_locus::source_edge_open_interval)
        continue;
      if (!descriptor.classification_consumable)
        continue;
      if (descriptor.key.source_feature.kind != relation_feature_kind::source_edge)
        continue;
      const auto source_edge = descriptor.key.source_feature.primary;
      edge_cut_delta[source_edge] = descriptor.signed_crossing_delta;
    }

    // ---- Phase 1: build facet cells (atoms) split by carrier chords. ----
    std::vector<classification_detail::cell_polygon<T>> cells;
    std::vector<std::uint64_t> cell_facet_group;
    for (std::size_t group_index = 0; group_index < facet_groups.size(); ++group_index) {
      const auto &group = facet_groups[group_index];
      if (group.source_facet == classification_invalid_ordinal)
        continue;
      // Build the facet ring in the facet's 2D frame by traversing the boundary
      // halfedge cycle (group.vertices is canonical, not cyclic).
      const std::uint8_t dropped_axis = group.basis.dropped_axis;
      std::vector<std::array<T, 2>> ring2d;
      std::vector<std::array<T, 3>> ring3d;
      std::vector<typename classification_detail::cell_polygon<T>::edge_tag> edge_tags;
      if (!group.boundary_halfedges.empty()) {
        std::map<std::uint64_t, std::uint64_t> origin_to_halfedge;
        for (std::uint64_t he : group.boundary_halfedges) {
          if (he >= manifold.halfedges().size())
            continue;
          origin_to_halfedge[manifold.halfedges()[he].origin] = he;
        }
        std::uint64_t start_he = group.boundary_halfedges.front();
        std::uint64_t he = start_he;
        std::uint64_t guard = 0;
        do {
          if (he >= manifold.halfedges().size() || guard++ > group.boundary_halfedges.size()) {
            error = classification_error(classification_subcode::invalid_rotation_system,
                                         bounded_boolean_error_category::internal_invariant_error,
                                         "facet boundary halfedge cycle is broken",
                                         classification_checkpoint::local_normalization);
            return boolean_outcome<void>::failure(error);
          }
          const auto &halfedge = manifold.halfedges()[he];
          const auto vertex = halfedge.origin;
          if (vertex >= manifold.vertices().size()) {
            error = classification_error(classification_subcode::malformed_reference,
                                         bounded_boolean_error_category::internal_invariant_error,
                                         "facet ring vertex out of range",
                                         classification_checkpoint::local_normalization);
            return boolean_outcome<void>::failure(error);
          }
          const auto &record = manifold.vertices()[vertex];
          ring2d.push_back(classification_detail::project_2d(record.committed_point, dropped_axis));
          ring3d.push_back(record.committed_point);
          typename classification_detail::cell_polygon<T>::edge_tag tag;
          tag.kind = classification_detail::cell_polygon<T>::edge_kind::source_edge;
          tag.lineage = halfedge.source_undirected_edge == classification_invalid_ordinal
                            ? 0
                            : halfedge.source_undirected_edge;
          edge_tags.push_back(tag);
          const auto next_it = origin_to_halfedge.find(halfedge.destination);
          if (next_it == origin_to_halfedge.end())
            break;
          he = next_it->second;
        } while (he != start_he);
      }

      auto paths = collect_carrier_paths(operand, group.source_facet, error);
      if (!paths.has_value())
        return boolean_outcome<void>::failure(*paths.error());

      classification_detail::cell_polygon<T> base;
      base.ring2d = ring2d;
      base.ring3d = ring3d;
      base.shell = group.shell;
      base.source_facet = group.source_facet;
      base.dropped_axis = dropped_axis;
      base.edges = edge_tags;
      std::vector<classification_detail::cell_polygon<T>> current{std::move(base)};

      for (const auto &path : *paths.value()) {
        std::vector<std::array<T, 2>> path2d;
        path2d.reserve(path.points.size());
        for (const auto &point : path.points)
          path2d.push_back(classification_detail::project_2d(point, dropped_axis));
        std::vector<classification_detail::cell_polygon<T>> next;
        for (auto &poly : current) {
          std::vector<std::array<T, 2>> sub_ring1, sub_ring2;
          std::vector<std::array<T, 3>> sub_ring1_3d, sub_ring2_3d;
          std::vector<typename classification_detail::cell_polygon<T>::edge_tag>
              sub_edges1, sub_edges2;
          const std::int32_t delta = path.deltas.empty() ? 0 : path.deltas.front();
          const std::uint64_t lineage = path.lineages.empty() ? 0 : path.lineages.front();
          if (!classification_detail::split_cell_by_line(
                  poly.ring2d, poly.ring3d, poly.edges, path2d.front(),
                  path2d.back(), delta, lineage, sub_ring1, sub_ring1_3d, sub_edges1,
                  sub_ring2, sub_ring2_3d, sub_edges2)) {
            next.push_back(std::move(poly));
            continue;
          }
          classification_detail::cell_polygon<T> sub1;
          sub1.shell = poly.shell;
          sub1.source_facet = poly.source_facet;
          sub1.dropped_axis = poly.dropped_axis;
          sub1.ring2d = std::move(sub_ring1);
          sub1.ring3d = std::move(sub_ring1_3d);
          sub1.edges = std::move(sub_edges1);
          classification_detail::cell_polygon<T> sub2;
          sub2.shell = poly.shell;
          sub2.source_facet = poly.source_facet;
          sub2.dropped_axis = poly.dropped_axis;
          sub2.ring2d = std::move(sub_ring2);
          sub2.ring3d = std::move(sub_ring2_3d);
          sub2.edges = std::move(sub_edges2);
          next.push_back(std::move(sub1));
          next.push_back(std::move(sub2));
        }
        current = std::move(next);
      }

      for (auto &poly : current) {
        cells.push_back(std::move(poly));
        cell_facet_group.push_back(group_index);
      }
    }

    // ---- Phase 2: emit atoms. ----
    const std::uint64_t atom_begin = artifact.atoms_.size();
    for (auto &poly : cells) {
      classification_atom_record<T> atom;
      atom.canonical_id = artifact.atoms_.size();
      atom.operand = operand;
      atom.shell = poly.shell;
      atom.source_facet = poly.source_facet;
      atom.positive_area = poly.ring2d.size() >= 3;
      // Witness: average of 3D vertices (convex combination of a convex cell).
      std::array<T, 3> witness{};
      for (const auto &vertex : poly.ring3d)
        for (std::size_t axis = 0; axis < 3; ++axis)
          witness[axis] += vertex[axis];
      const T scale = static_cast<T>(poly.ring3d.size());
      for (std::size_t axis = 0; axis < 3; ++axis) {
        witness[axis] /= scale;
        atom.witness.nominal_bits[axis] = to_bits(witness[axis]);
      }
      // Build a canonical atom key from sorted boundary lineage.
      std::vector<std::uint64_t> boundary_lineage;
      for (const auto &tag : poly.edges)
        boundary_lineage.push_back(tag.lineage);
      std::sort(boundary_lineage.begin(), boundary_lineage.end());
      atom.key.operand = operand;
      atom.key.shell = poly.shell;
      atom.key.source_facet = poly.source_facet;
      atom.key.boundary_lineage = boundary_lineage;
      artifact.atoms_.push_back(std::move(atom));
    }
    const std::uint64_t atom_end = artifact.atoms_.size();

    // ---- Phase 3: adjacency. ----
    // Map from (undirected source-edge ordinal) to incident atoms; internal
    // diagonals are already transparent (merged) so only source edges and
    // carrier chords produce adjacency.
    std::map<std::uint64_t, std::vector<std::uint64_t>> edge_atoms;
    std::map<std::uint64_t, std::vector<std::uint64_t>> carrier_atoms;
    for (std::uint64_t index = atom_begin; index < atom_end; ++index) {
      const auto &poly = cells[index - atom_begin];
      for (const auto &tag : poly.edges) {
        if (tag.kind == classification_detail::cell_polygon<T>::edge_kind::source_edge) {
          edge_atoms[tag.lineage].push_back(index);
        } else if (tag.kind == classification_detail::cell_polygon<T>::edge_kind::carrier) {
          carrier_atoms[tag.lineage].push_back(index);
        }
      }
    }

    // Zero-delta adjacency across source edges (uncut continuation) and
    // crossing adjacency across cut source edges / carrier chords.
    std::vector<classification_adjacency_record> adjacency;
    for (const auto &entry : edge_atoms) {
      if (entry.second.size() != 2)
        continue;
      const auto cut = edge_cut_delta.find(entry.first);
      const std::int32_t delta = cut == edge_cut_delta.end() ? 0 : cut->second;
      classification_adjacency_record forward;
      forward.canonical_id = adjacency.size();
      forward.source_atom = entry.second[0];
      forward.destination_atom = entry.second[1];
      forward.key.source = artifact.atoms_[entry.second[0]].key;
      forward.key.destination = artifact.atoms_[entry.second[1]].key;
      forward.key.adjacency_class = delta == 0
          ? classification_adjacency_class::uncut_continuation
          : classification_adjacency_class::numeric_crossing;
      forward.key.eligibility = delta == 0 ? union_eligibility::required
                                           : union_eligibility::prohibited;
      forward.key.total_delta = delta;
      forward.key.semantic_locus_lineage = entry.first;
      forward.key.descriptor_lineage = 0;
      forward.descriptor_lineage = 0;
      classification_adjacency_record backward = forward;
      backward.canonical_id = adjacency.size() + 1;
      backward.source_atom = entry.second[1];
      backward.destination_atom = entry.second[0];
      backward.key.source = artifact.atoms_[entry.second[1]].key;
      backward.key.destination = artifact.atoms_[entry.second[0]].key;
      backward.key.total_delta = -delta;
      backward.key.eligibility = delta == 0 ? union_eligibility::required
                                            : union_eligibility::prohibited;
      forward.reverse = backward.canonical_id;
      backward.reverse = forward.canonical_id;
      adjacency.push_back(std::move(forward));
      adjacency.push_back(std::move(backward));
    }
    for (const auto &entry : carrier_atoms) {
      if (entry.second.size() != 2)
        continue;
      std::int32_t delta = 0;
      for (const auto &poly : cells) {
        for (const auto &tag : poly.edges) {
          if (tag.kind == classification_detail::cell_polygon<T>::edge_kind::carrier &&
              tag.lineage == entry.first) {
            delta = tag.delta;
            break;
          }
        }
        if (delta != 0)
          break;
      }
      classification_adjacency_record forward;
      forward.canonical_id = adjacency.size();
      forward.source_atom = entry.second[0];
      forward.destination_atom = entry.second[1];
      forward.key.source = artifact.atoms_[entry.second[0]].key;
      forward.key.destination = artifact.atoms_[entry.second[1]].key;
      forward.key.adjacency_class = delta == 0
          ? classification_adjacency_class::contact_delimiter
          : classification_adjacency_class::numeric_crossing;
      forward.key.eligibility = union_eligibility::prohibited;
      forward.key.total_delta = delta;
      forward.key.semantic_locus_lineage = entry.first;
      forward.key.descriptor_lineage = entry.first;
      forward.descriptor_lineage = entry.first;
      classification_adjacency_record backward = forward;
      backward.canonical_id = adjacency.size() + 1;
      backward.source_atom = entry.second[1];
      backward.destination_atom = entry.second[0];
      backward.key.source = artifact.atoms_[entry.second[1]].key;
      backward.key.destination = artifact.atoms_[entry.second[0]].key;
      backward.key.total_delta = -delta;
      forward.reverse = backward.canonical_id;
      backward.reverse = forward.canonical_id;
      adjacency.push_back(std::move(forward));
      adjacency.push_back(std::move(backward));
    }

    // ---- Phase 4: group atoms through zero-delta adjacency. ----
    std::vector<std::uint64_t> parent(atom_end - atom_begin);
    for (std::uint64_t i = 0; i < parent.size(); ++i)
      parent[i] = i;
    auto find = [&](std::uint64_t x) {
      while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
      }
      return x;
    };
    for (const auto &adj : adjacency) {
      if (adj.key.eligibility == union_eligibility::prohibited)
        continue;
      const auto ra = find(adj.source_atom - atom_begin);
      const auto rb = find(adj.destination_atom - atom_begin);
      if (ra != rb)
        parent[ra] = rb;
    }
    // Build groups.
    std::map<std::uint64_t, std::vector<std::uint64_t>> group_members;
    for (std::uint64_t i = atom_begin; i < atom_end; ++i)
      group_members[find(i - atom_begin)].push_back(i);
    const std::uint64_t group_begin = artifact.groups_.size();
    std::vector<std::uint64_t> atom_to_group(atom_end - atom_begin);
    for (const auto &entry : group_members) {
      classification_group_record group;
      group.canonical_id = artifact.groups_.size();
      group.members = entry.second;
      std::sort(group.members.begin(), group.members.end());
      artifact.groups_.push_back(std::move(group));
    }
    // Assign atom -> group ordinal.
    for (std::uint64_t g = group_begin; g < artifact.groups_.size(); ++g) {
      for (const auto atom : artifact.groups_[g].members) {
        atom_to_group[atom - atom_begin] = g;
        artifact.atoms_[atom].group = g;
      }
    }

    // ---- Phase 5: quotient graph. ----
    const std::uint64_t quotient_begin = artifact.quotient_edges_.size();
    for (const auto &adj : adjacency) {
      if (adj.key.eligibility != union_eligibility::prohibited)
        continue;
      if (adj.key.total_delta == 0 &&
          adj.key.adjacency_class != classification_adjacency_class::contact_delimiter)
        continue;
      quotient_edge_record edge;
      edge.canonical_id = artifact.quotient_edges_.size();
      edge.source_group = atom_to_group[adj.source_atom - atom_begin];
      edge.destination_group = atom_to_group[adj.destination_atom - atom_begin];
      edge.total_delta = adj.key.total_delta;
      edge.member_adjacency = {adj.canonical_id};
      edge.role = adj.key.total_delta == 0
                      ? propagation_edge_role::contact_relation
                      : propagation_edge_role::numeric_constraint;
      artifact.quotient_edges_.push_back(std::move(edge));
    }
    // Pair reverse quotient edges.
    for (std::uint64_t i = quotient_begin; i < artifact.quotient_edges_.size(); ++i) {
      auto &edge = artifact.quotient_edges_[i];
      for (std::uint64_t j = i + 1; j < artifact.quotient_edges_.size(); ++j) {
        auto &candidate = artifact.quotient_edges_[j];
        if (edge.source_group == candidate.destination_group &&
            edge.destination_group == candidate.source_group &&
            edge.total_delta == -candidate.total_delta) {
          edge.reverse = candidate.canonical_id;
          candidate.reverse = edge.canonical_id;
          break;
        }
      }
    }

    // ---- Phase 6: propagation components, anchors, and seeding. ----
    const std::uint64_t group_count = artifact.groups_.size() - group_begin;
    std::vector<std::uint64_t> group_parent(group_count);
    for (std::uint64_t i = 0; i < group_count; ++i)
      group_parent[i] = i;
    auto gfind = [&](std::uint64_t x) {
      while (group_parent[x] != x) {
        group_parent[x] = group_parent[group_parent[x]];
        x = group_parent[x];
      }
      return x;
    };
    for (const auto &edge : artifact.quotient_edges_) {
      if (edge.role != propagation_edge_role::numeric_constraint)
        continue;
      const auto ra = gfind(edge.source_group - group_begin);
      const auto rb = gfind(edge.destination_group - group_begin);
      if (ra != rb)
        group_parent[ra] = rb;
    }
    std::map<std::uint64_t, std::vector<std::uint64_t>> components;
    for (std::uint64_t i = 0; i < group_count; ++i)
      components[gfind(i)].push_back(group_begin + i);

    std::vector<std::int64_t> winding(group_count, 0);
    std::vector<classification_detail::shell_winding_map> shell_winding(group_count);
    std::vector<bool> anchored(group_count, false);

    const auto empty_opposite = opposite.triangles().empty();
    for (const auto &entry : components) {
      propagation_component_record component;
      component.canonical_id = artifact.propagation_components_.size();
      component.groups = entry.second;
      std::sort(component.groups.begin(), component.groups.end());

      bool has_anchor = false;
      std::int64_t anchor_winding = 0;
      classification_detail::shell_winding_map anchor_shell;
      seed_source_kind anchor_source = seed_source_kind::empty_opposite;

      if (empty_opposite) {
        has_anchor = true;
        anchor_source = seed_source_kind::empty_opposite;
      }

      if (!has_anchor) {
        // Try a bounded shell query at the canonical witness of the least atom.
        for (const auto group_ordinal : component.groups) {
          const auto &group = artifact.groups_[group_ordinal];
          if (group.members.empty())
            continue;
          const auto atom_ordinal = group.members.front();
          const auto &atom = artifact.atoms_[atom_ordinal];
          std::array<T, 3> nominal{};
          for (std::size_t axis = 0; axis < 3; ++axis)
            nominal[axis] = from_bits<T>(static_cast<floating_uint_t<T>>(
                atom.witness.nominal_bits[axis]));
          bool success = false;
          seed_query_record query;
          query.canonical_id = artifact.seed_queries_.size();
          query.component = component.canonical_id;
          query.group = group_ordinal;
          query.atom = atom_ordinal;
          query.source_kind = seed_source_kind::bounded_shell_query;
          for (std::uint32_t direction_index = 0;
               direction_index < canonical_primitive_directions().size();
               ++direction_index) {
            const auto result = run_seed_query(
                opposite, nominal, canonical_primitive_directions()[direction_index]);
            seed_attempt_record attempt;
            attempt.canonical_id = query.attempts.size();
            attempt.direction_index = direction_index;
            if (result.disposition == shell_query_disposition::definite) {
              attempt.disposition = seed_attempt_disposition::successful;
              query.attempts.push_back(attempt);
              query.total_winding = result.total_winding;
              for (const auto &entry : result.shell_winding)
                query.shell_winding.push_back(
                    {entry.first, static_cast<std::int64_t>(entry.second)});
              success = true;
              break;
            }
            attempt.disposition = seed_attempt_disposition::rejected_uncertain;
            query.attempts.push_back(attempt);
            if (classification_cancelled(capabilities_, classification_checkpoint::shell_query))
              return fail_cancelled<void>(classification_checkpoint::shell_query);
          }
          if (success) {
            has_anchor = true;
            anchor_source = seed_source_kind::bounded_shell_query;
            anchor_winding = query.total_winding;
            for (const auto &entry : query.shell_winding)
              classification_detail::accumulate_shell(anchor_shell, entry.first, entry.second);
            artifact.seed_queries_.push_back(std::move(query));
            break;
          }
        }
      }

      if (!has_anchor) {
        error = classification_error(classification_subcode::unanchored_propagation_component,
                                     bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
                                     "classification propagation component is unanchored",
                                     classification_checkpoint::anchor_discovery);
        return boolean_outcome<void>::failure(error);
      }

      component.anchor_group = component.groups.front();
      component.anchor_source = anchor_source;
      // Anchor value stored at the component's anchor group.
      const auto anchor_local = component.anchor_group - group_begin;
      winding[anchor_local] = anchor_winding;
      shell_winding[anchor_local] = anchor_shell;
      anchored[anchor_local] = true;
      artifact.propagation_components_.push_back(std::move(component));
    }

    // ---- Phase 7: propagate winding. ----
    bool progressed = true;
    while (progressed) {
      progressed = false;
      for (const auto &edge : artifact.quotient_edges_) {
        if (edge.role != propagation_edge_role::numeric_constraint)
          continue;
        const auto source_local = edge.source_group - group_begin;
        const auto destination_local = edge.destination_group - group_begin;
        if (anchored[source_local] && !anchored[destination_local]) {
          winding[destination_local] = winding[source_local] + edge.total_delta;
          shell_winding[destination_local] = shell_winding[source_local];
          for (const auto &entry : edge.shell_deltas)
            classification_detail::accumulate_shell(shell_winding[destination_local],
                                                    entry.first, entry.second);
          anchored[destination_local] = true;
          progressed = true;
        } else if (anchored[source_local] && anchored[destination_local]) {
          if (winding[destination_local] != winding[source_local] + edge.total_delta) {
            error = classification_error(classification_subcode::nonzero_cycle_residual,
                                         bounded_boolean_error_category::internal_invariant_error,
                                         "classification winding cycle residual is nonzero",
                                         classification_checkpoint::residual_verification);
            return boolean_outcome<void>::failure(error);
          }
        }
      }
      if (classification_cancelled(capabilities_, classification_checkpoint::winding_propagation))
        return fail_cancelled<void>(classification_checkpoint::winding_propagation);
    }
    for (std::uint64_t i = 0; i < group_count; ++i) {
      if (!anchored[i]) {
        error = classification_error(classification_subcode::unanchored_propagation_component,
                                     bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
                                     "classification group is unanchored",
                                     classification_checkpoint::winding_propagation);
        return boolean_outcome<void>::failure(error);
      }
      if (winding[i] < 0 || winding[i] > 1) {
        error = classification_error(classification_subcode::nonboundary_winding_outside_domain,
                                     bounded_boolean_error_category::input_geometry_not_epsilon_valid,
                                     "classification winding outside the accepted solid domain",
                                     classification_checkpoint::residual_verification);
        return boolean_outcome<void>::failure(error);
      }
      artifact.groups_[group_begin + i].total_winding = winding[i];
      for (const auto &entry : shell_winding[i])
        artifact.groups_[group_begin + i].shell_winding.push_back(
            {entry.first, entry.second});
    }

    // ---- Phase 8: side labels. ----
    for (std::uint64_t i = atom_begin; i < atom_end; ++i) {
      const auto &atom = artifact.atoms_[i];
      const auto group = atom.group;
      if (group == classification_invalid_ordinal)
        continue;
      const auto w = artifact.groups_[group].total_winding;
      atom_side_label_record label;
      label.canonical_id = artifact.side_labels_.size();
      label.atom = i;
      const occupancy_state state = w == 0 ? occupancy_state::strict_outside
                                           : occupancy_state::strict_inside;
      label.negative_side = state;
      label.positive_side = state;
      label.negative_origin = side_occupancy_origin::numeric;
      label.positive_origin = side_occupancy_origin::numeric;
      label.contact = boundary_contact_state::none;
      artifact.side_labels_.push_back(std::move(label));
      artifact.atoms_[i].side_label = artifact.side_labels_.size() - 1;
    }

    artifact.statistics_.atom_count = artifact.atoms_.size();
    artifact.statistics_.adjacency_count = artifact.adjacency_.size();
    artifact.statistics_.group_count = artifact.groups_.size();
    artifact.statistics_.quotient_edge_count = artifact.quotient_edges_.size();
    artifact.statistics_.propagation_component_count = artifact.propagation_components_.size();
    artifact.statistics_.seed_query_count = artifact.seed_queries_.size();
    artifact.statistics_.side_label_count = artifact.side_labels_.size();
    return boolean_outcome<void>::success();
  }

  bool checked_range(intersection_range range, std::uint64_t size) const noexcept {
    return range.begin <= size && range.count <= size - range.begin;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>
build_classification_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    classification_capabilities capabilities, classification_codec_limits codec_limits,
    classification_verifier_limits verifier_limits) {
  try {
    classification_builder<T, I> builder(context, precision, std::move(manifolds),
                                         std::move(relations), std::move(intersections),
                                         std::move(capabilities), codec_limits, verifier_limits);
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::failure(
        classification_error(classification_subcode::resource_preflight,
                             bounded_boolean_error_category::resource_limit,
                             "classification allocation failed",
                             classification_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const classification_complex<T, I>>>::failure(
        classification_error(classification_subcode::internal_invariant,
                             bounded_boolean_error_category::internal_invariant_error,
                             "classification unexpected exception",
                             classification_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
