#pragma once
#ifndef YGOR_MESHES_BOOLEAN_INTERSECTION_EVENTS_H_
#define YGOR_MESHES_BOOLEAN_INTERSECTION_EVENTS_H_
#include "YgorMeshesBooleanBroadPhase.h"
#include <map>

namespace ygor {
namespace mesh_boolean {
constexpr std::uint64_t raw_event_set_type_tag = 0x5947425241573035ULL;
constexpr std::uint16_t raw_event_set_schema = 4;
enum class event_plane_relation : std::uint8_t {
  nonparallel,
  parallel_disjoint,
  coincident_same_orientation,
  coincident_opposite_orientation
};
enum class pair_aggregate_relation : std::uint8_t {
  disjoint,
  point_contact,
  curve_contact,
  coplanar_boundary_contact,
  coplanar_positive_area_overlap,
  equal_same_orientation,
  equal_opposite_orientation
};
enum class event_dimension : std::uint8_t { point, interval, region };
struct raw_event_locator {
  event_dimension dimension = event_dimension::point;
  std::uint64_t ordinal = 0;
};
enum class raw_point_kind : std::uint8_t {
  proper_transverse_endpoint,
  proper_boundary_crossing,
  vertex_vertex,
  vertex_edge_interior,
  vertex_facet_interior,
  edge_edge,
  edge_facet,
  tangency,
  overlap_boundary_vertex
};
enum class raw_interval_kind : std::uint8_t {
  transverse_facet_intersection,
  coincident_source_edge_subsegment,
  coplanar_overlap_boundary_segment,
  coplanar_interior_carrier_segment
};
enum class local_incidence_location : std::uint8_t {
  source_vertex,
  directed_edge_origin,
  directed_edge_destination,
  directed_edge_open_interior,
  facet_boundary,
  facet_open_interior
};
struct raw_source_incidence {
  feature_ref source;
  local_incidence_location location =
      local_incidence_location::facet_open_interior;
  std::optional<exact_scalar> directed_edge_parameter;
  std::optional<exact_sign> local_side;
  orientation_parity orientation = orientation_parity::agree;
};
struct raw_derivation {
  construction_node_id construction;
  std::vector<feature_ref> defining_sources;
  std::vector<raw_source_incidence> incidences;
  evidence_record evidence;
};
struct raw_point_event {
  raw_event_id id;
  candidate_id candidate;
  facet_candidate_key facets;
  exact_point3 point;
  raw_point_kind kind = raw_point_kind::tangency;
  std::vector<raw_source_incidence> incidences;
  std::vector<raw_derivation> derivations;
};
struct source_edge_parameter_interval {
  edge_use_id edge_use;
  exact_interval parameters;
  orientation_parity orientation = orientation_parity::agree;
};
struct raw_curve_ownership {
  operand_id operand;
  feature_ref source;
  orientation_parity direction = orientation_parity::agree;
  std::uint32_t multiplicity = 1;
};
int canonical_incidence_compare(const raw_source_incidence &,
                                const raw_source_incidence &) noexcept;
int canonical_ownership_compare(const raw_curve_ownership &,
                                const raw_curve_ownership &) noexcept;
inline bool canonical_incidence_less(const raw_source_incidence &a,
                                     const raw_source_incidence &b) noexcept {
  return canonical_incidence_compare(a, b) < 0;
}
inline bool canonical_incidence_equal(const raw_source_incidence &a,
                                      const raw_source_incidence &b) noexcept {
  return canonical_incidence_compare(a, b) == 0;
}
inline bool canonical_ownership_less(const raw_curve_ownership &a,
                                     const raw_curve_ownership &b) noexcept {
  return canonical_ownership_compare(a, b) < 0;
}
inline bool canonical_ownership_equal(const raw_curve_ownership &a,
                                      const raw_curve_ownership &b) noexcept {
  return canonical_ownership_compare(a, b) == 0;
}

namespace detail {
class construction_key_index {
  std::map<digest, std::vector<std::size_t>> buckets_;

public:
  std::optional<std::size_t>
  find(const digest &, const construction_storage &,
       const std::vector<feature_ref> &,
       const std::vector<std::uint8_t> &) const;
  void insert(const digest &, std::size_t);
};
} // namespace detail
struct raw_interval_event {
  raw_event_id id;
  candidate_id candidate;
  facet_candidate_key facets;
  raw_interval_kind kind = raw_interval_kind::transverse_facet_intersection;
  exact_line3 carrier;
  orientation_parity direction = orientation_parity::agree;
  raw_event_id lower_point, upper_point;
  exact_interval carrier_parameters;
  std::vector<source_edge_parameter_interval> source_intervals;
  std::vector<raw_curve_ownership> ownership;
  std::vector<raw_source_incidence> incidences;
  std::vector<raw_derivation> derivations;
};
enum class region_cycle_role : std::uint8_t { outer, hole };
struct raw_region_boundary_cycle {
  region_cycle_role role = region_cycle_role::outer;
  std::vector<raw_event_id> vertices;
  std::vector<raw_event_id> intervals;
  std::vector<std::vector<raw_curve_ownership>> ownership;
};
struct raw_region_event {
  raw_event_id id;
  candidate_id candidate;
  facet_candidate_key facets;
  orientation_parity plane_orientation = orientation_parity::agree;
  std::vector<raw_region_boundary_cycle> boundary_cycles;
  std::vector<raw_source_incidence> incidences;
  exact_scalar area;
  exact_point3 interior_witness;
  std::vector<raw_derivation> derivations;
};
struct raw_carrier_derivation {
  candidate_id candidate;
  exact_line3 carrier;
  orientation_parity direction = orientation_parity::agree;
  std::vector<facet_id> facets;
  std::vector<raw_derivation> derivations;
};
struct candidate_classification {
  candidate_id candidate;
  facet_candidate_key facets;
  event_plane_relation plane_relation = event_plane_relation::parallel_disjoint;
  pair_aggregate_relation aggregate = pair_aggregate_relation::disjoint;
  std::uint64_t event_begin = 0, event_end = 0, carrier_begin = 0,
                carrier_end = 0;
  std::uint64_t point_count = 0, interval_count = 0, region_count = 0;
};
template <class T, class I> struct raw_event_set {
  context_owner_token owner;
  digest setup_digest, upstream_digest, kernel_policy_digest, artifact_digest;
  std::shared_ptr<const published_artifact<candidate_stream<T, I>>> candidates;
  std::shared_ptr<const construction_storage> constructions;
  std::vector<candidate_classification> classifications;
  std::vector<raw_carrier_derivation> carriers;
  std::vector<raw_point_event> points;
  std::vector<raw_interval_event> intervals;
  std::vector<raw_region_event> regions;
  // Dense raw_event_id -> dimension/ordinal lookup. It is derived metadata and
  // is not part of the canonical artifact encoding.
  std::vector<raw_event_locator> event_index;
  std::vector<std::uint8_t> canonical_event_bytes, artifact_bytes;
};
template <class T, class I>
inline const raw_point_event *find_raw_point(const raw_event_set<T, I> &a,
                                             raw_event_id id) noexcept {
  if (!id.valid() || id.value_for_debug() >= a.event_index.size()) return nullptr;
  const auto &location = a.event_index[id.value_for_debug()];
  if (location.dimension != event_dimension::point ||
      location.ordinal >= a.points.size()) return nullptr;
  const auto &record = a.points[location.ordinal];
  return record.id == id ? &record : nullptr;
}
template <class T, class I>
inline const raw_interval_event *find_raw_interval(const raw_event_set<T, I> &a,
                                                   raw_event_id id) noexcept {
  if (!id.valid() || id.value_for_debug() >= a.event_index.size()) return nullptr;
  const auto &location = a.event_index[id.value_for_debug()];
  if (location.dimension != event_dimension::interval ||
      location.ordinal >= a.intervals.size()) return nullptr;
  const auto &record = a.intervals[location.ordinal];
  return record.id == id ? &record : nullptr;
}
template <class T, class I>
inline const raw_region_event *find_raw_region(const raw_event_set<T, I> &a,
                                               raw_event_id id) noexcept {
  if (!id.valid() || id.value_for_debug() >= a.event_index.size()) return nullptr;
  const auto &location = a.event_index[id.value_for_debug()];
  if (location.dimension != event_dimension::region ||
      location.ordinal >= a.regions.size()) return nullptr;
  const auto &record = a.regions[location.ordinal];
  return record.id == id ? &record : nullptr;
}
status_or<bool> register_intersection_events_verifier(verifier_registry &,
                                                      coordinate_tag,
                                                      index_tag);
template <class T, class I>
status_or<std::shared_ptr<const published_artifact<raw_event_set<T, I>>>>
discover_intersection_events(boolean_context<T, I> &);
#define YGOR_EVENT_EXTERN(T, I)                                                \
  extern template status_or<                                                   \
      std::shared_ptr<const published_artifact<raw_event_set<T, I>>>>          \
  discover_intersection_events(boolean_context<T, I> &)
YGOR_EVENT_EXTERN(float, std::uint32_t);
YGOR_EVENT_EXTERN(float, std::uint64_t);
YGOR_EVENT_EXTERN(double, std::uint32_t);
YGOR_EVENT_EXTERN(double, std::uint64_t);
#undef YGOR_EVENT_EXTERN
} // namespace mesh_boolean
} // namespace ygor
#endif
