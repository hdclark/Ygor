#include "IntersectionAggregation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error aggregate_error(intersection_subcode subcode,
                                      const char *summary) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::input_contract_error,
                            summary,
                            intersection_checkpoint::aggregate_reconstruction);
}

bounded_boolean_error verifier_error(const char *summary) {
  return intersection_error(intersection_subcode::verifier_rejection,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary,
                            intersection_checkpoint::aggregate_reconstruction);
}

bool checked_range(intersection_range range, std::size_t size) noexcept {
  return range.begin <= size &&
         range.count <= size - static_cast<std::size_t>(range.begin);
}

template <class T, class U>
bool checked_add(T &target, U value) noexcept {
  static_assert(std::numeric_limits<T>::is_integer, "integer required");
  const auto promoted = static_cast<T>(value);
  if ((promoted > 0 && target > std::numeric_limits<T>::max() - promoted) ||
      (promoted < 0 && target < std::numeric_limits<T>::min() - promoted))
    return false;
  target = static_cast<T>(target + promoted);
  return true;
}

std::uint8_t owner_bit(operand_id owner) noexcept {
  return static_cast<std::uint8_t>(1U << static_cast<std::uint8_t>(owner));
}

struct locus_key final {
  intersection_aggregate_locus locus =
      intersection_aggregate_locus::event_occurrence;
  relation_feature_key source_feature{};
  std::uint64_t ordinal = 0;

  friend bool operator<(const locus_key &a, const locus_key &b) noexcept {
    return std::tie(a.locus, a.source_feature, a.ordinal) <
           std::tie(b.locus, b.source_feature, b.ordinal);
  }
  friend bool operator==(const locus_key &a, const locus_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct contact_key final {
  locus_key locus{};
  feature_relation_status status = feature_relation_status::not_evaluated;
  relation_contact_dimension dimension = relation_contact_dimension::none;

  friend bool operator<(const contact_key &a, const contact_key &b) noexcept {
    return std::tie(a.locus, a.status, a.dimension) <
           std::tie(b.locus, b.status, b.dimension);
  }
};

bool valid_incidence_kind(event_incidence_kind kind) noexcept {
  const auto value = static_cast<std::uint8_t>(kind);
  return value >= static_cast<std::uint8_t>(event_incidence_kind::source_vertex) &&
         value <=
             static_cast<std::uint8_t>(event_incidence_kind::descriptor_precursor);
}

bool valid_contact_class(feature_relation_status status,
                         relation_contact_dimension dimension) noexcept {
  const bool valid_status =
      status == feature_relation_status::proper_crossing ||
      status == feature_relation_status::endpoint_crossing ||
      status == feature_relation_status::point_contact ||
      status == feature_relation_status::segment_contact ||
      status == feature_relation_status::tangency ||
      status == feature_relation_status::overlap ||
      status == feature_relation_status::containment ||
      status == feature_relation_status::coincidence_same_orientation ||
      status == feature_relation_status::coincidence_opposite_orientation;
  const bool valid_dimension = dimension == relation_contact_dimension::point ||
      dimension == relation_contact_dimension::curve ||
      dimension == relation_contact_dimension::area;
  return valid_status && valid_dimension;
}

struct contribution_catalog final {
  std::vector<std::vector<event_incidence_id>> crossing_by_event{};
  std::vector<std::vector<event_incidence_id>> crossing_by_occurrence{};
  std::vector<std::vector<event_incidence_id>> contact_by_event{};
  std::vector<std::vector<event_incidence_id>> contact_by_occurrence{};
  std::vector<std::vector<relation_feature_key>> facets_by_binding{};
  std::vector<std::vector<relation_feature_key>> shells_by_binding{};
};

bool validate_incidence_and_build_catalog(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence, contribution_catalog &catalog,
    bounded_boolean_error &error) {
  if (interning.seed_bindings.size() != seeds.size() ||
      incidence.event_ranges.size() != interning.events.size() ||
      incidence.occurrence_ranges.size() != interning.occurrences.size() ||
      incidence.seed_ranges.size() != interning.seed_bindings.size()) {
    error = aggregate_error(intersection_subcode::aggregate_mismatch,
                            "Component 08 aggregate input ranges are incomplete");
    return false;
  }
  catalog.crossing_by_event.resize(interning.events.size());
  catalog.crossing_by_occurrence.resize(interning.occurrences.size());
  catalog.contact_by_event.resize(interning.events.size());
  catalog.contact_by_occurrence.resize(interning.occurrences.size());
  catalog.facets_by_binding.resize(interning.seed_bindings.size());
  catalog.shells_by_binding.resize(interning.seed_bindings.size());

  for (std::size_t i = 0; i < interning.events.size(); ++i) {
    if (interning.events[i].id.ordinal() != i) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate event ID is malformed");
      return false;
    }
  }
  for (std::size_t i = 0; i < interning.occurrences.size(); ++i) {
    const auto &occurrence = interning.occurrences[i];
    if (occurrence.id.ordinal() != i ||
        occurrence.event.ordinal() >= interning.events.size()) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate occurrence is malformed");
      return false;
    }
  }
  for (std::size_t i = 0; i < interning.seed_bindings.size(); ++i) {
    const auto &binding = interning.seed_bindings[i];
    if (binding.id.ordinal() != i || binding.seed.ordinal() >= seeds.size() ||
        seeds[binding.seed.ordinal()].id != binding.seed ||
        binding.event.ordinal() >= interning.events.size() ||
        binding.occurrence.ordinal() >= interning.occurrences.size() ||
        interning.occurrences[binding.occurrence.ordinal()].event !=
            binding.event) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate seed binding is malformed");
      return false;
    }
  }

  for (std::size_t i = 0; i < incidence.records.size(); ++i) {
    const auto &record = incidence.records[i];
    if (record.id.ordinal() != i || !valid_incidence_kind(record.kind) ||
        record.event.ordinal() >= interning.events.size() ||
        record.occurrence.ordinal() >= interning.occurrences.size() ||
        record.seed_binding.ordinal() >= interning.seed_bindings.size() ||
        interning.seed_bindings[record.seed_binding.ordinal()].event !=
            record.event ||
        interning.seed_bindings[record.seed_binding.ordinal()].occurrence !=
            record.occurrence ||
        record.schema_version != contract_versions::intersection_incidence_schema ||
        record.reserved16 != 0) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate incidence is malformed");
      return false;
    }
    if (record.kind == event_incidence_kind::crossing_contribution) {
      const auto &binding =
          interning.seed_bindings[record.seed_binding.ordinal()];
      const auto &seed = seeds[binding.seed.ordinal()];
      if (record.numeric_crossing != seed.numeric_crossing ||
          record.symbolic_crossing != seed.symbolic_crossing ||
          record.numeric_crossing < -1 || record.numeric_crossing > 1 ||
          record.symbolic_crossing < -1 || record.symbolic_crossing > 1 ||
          (record.numeric_crossing == 0 && record.symbolic_crossing == 0)) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 crossing contribution disagrees with its seed");
        return false;
      }
      catalog.crossing_by_event[record.event.ordinal()].push_back(record.id);
      catalog.crossing_by_occurrence[record.occurrence.ordinal()].push_back(
          record.id);
    } else if (record.kind == event_incidence_kind::descriptor_precursor) {
      const auto status =
          static_cast<feature_relation_status>(record.payload_primary);
      const auto dimension =
          static_cast<relation_contact_dimension>(record.payload_secondary);
      const auto &binding =
          interning.seed_bindings[record.seed_binding.ordinal()];
      const auto &seed = seeds[binding.seed.ordinal()];
      if (status != seed.contact_status || dimension != seed.contact_dimension ||
          !valid_contact_class(status, dimension)) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 contact precursor disagrees with its seed");
        return false;
      }
      catalog.contact_by_event[record.event.ordinal()].push_back(record.id);
      catalog.contact_by_occurrence[record.occurrence.ordinal()].push_back(
          record.id);
    }
    if (record.bookkeeping_only || !record.source_feature_owner)
      continue;
    if (record.kind == event_incidence_kind::source_facet) {
      if (record.feature.kind != relation_feature_kind::source_facet) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 source-facet subtotal ownership is malformed");
        return false;
      }
      catalog.facets_by_binding[record.seed_binding.ordinal()].push_back(
          record.feature);
    } else if (record.kind == event_incidence_kind::source_shell) {
      if (record.feature.kind != relation_feature_kind::sheet_occurrence) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 source-shell subtotal ownership is malformed");
        return false;
      }
      catalog.shells_by_binding[record.seed_binding.ordinal()].push_back(
          record.feature);
    }
  }

  auto canonicalize_ids = [](auto &ranges) {
    for (auto &range : ranges) {
      std::sort(range.begin(), range.end());
      range.erase(std::unique(range.begin(), range.end()), range.end());
    }
  };
  auto canonicalize_features = [](auto &ranges) {
    for (auto &range : ranges) {
      std::sort(range.begin(), range.end());
      range.erase(std::unique(range.begin(), range.end()), range.end());
    }
  };
  canonicalize_ids(catalog.crossing_by_event);
  canonicalize_ids(catalog.crossing_by_occurrence);
  canonicalize_ids(catalog.contact_by_event);
  canonicalize_ids(catalog.contact_by_occurrence);
  canonicalize_features(catalog.facets_by_binding);
  canonicalize_features(catalog.shells_by_binding);
  return true;
}

template <class Id>
void append_unique(std::vector<Id> &target, const std::vector<Id> &source) {
  target.insert(target.end(), source.begin(), source.end());
  std::sort(target.begin(), target.end());
  target.erase(std::unique(target.begin(), target.end()), target.end());
}

void add_occurrence_members(
    const locus_key &key, const std::vector<event_occurrence_id> &occurrences,
    const contribution_catalog &catalog,
    std::map<locus_key, std::vector<event_incidence_id>> &crossing,
    std::map<locus_key, std::vector<event_incidence_id>> &contact) {
  auto &crossing_members = crossing[key];
  auto &contact_members = contact[key];
  for (const auto occurrence : occurrences) {
    if (occurrence.ordinal() >= catalog.crossing_by_occurrence.size())
      continue;
    append_unique(crossing_members,
                  catalog.crossing_by_occurrence[occurrence.ordinal()]);
    append_unique(contact_members,
                  catalog.contact_by_occurrence[occurrence.ordinal()]);
  }
}

bool collect_producer_loci(
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const contribution_catalog &catalog,
    std::map<locus_key, std::vector<event_incidence_id>> &crossing,
    std::map<locus_key, std::vector<event_incidence_id>> &contact,
    bounded_boolean_error &error) {
  for (std::size_t i = 0; i < interning.events.size(); ++i) {
    locus_key key;
    key.locus = intersection_aggregate_locus::conceptual_event;
    key.ordinal = i;
    if (!catalog.crossing_by_event[i].empty())
      crossing[key] = catalog.crossing_by_event[i];
    if (!catalog.contact_by_event[i].empty())
      contact[key] = catalog.contact_by_event[i];
  }
  for (std::size_t i = 0; i < interning.occurrences.size(); ++i) {
    locus_key key;
    key.locus = intersection_aggregate_locus::event_occurrence;
    key.ordinal = i;
    if (!catalog.crossing_by_occurrence[i].empty())
      crossing[key] = catalog.crossing_by_occurrence[i];
    if (!catalog.contact_by_occurrence[i].empty())
      contact[key] = catalog.contact_by_occurrence[i];
  }

  for (std::size_t i = 0; i < source_edges.clusters.size(); ++i) {
    const auto &cluster = source_edges.clusters[i];
    if (cluster.id.ordinal() != i ||
        cluster.sequence.ordinal() >= source_edges.sequences.size() ||
        !checked_range(cluster.member_occurrences,
                       source_edges.cluster_occurrence_index.size())) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate source-edge cluster is malformed");
      return false;
    }
    std::vector<event_occurrence_id> occurrences;
    for (std::uint64_t j = 0; j < cluster.member_occurrences.count; ++j) {
      const auto occurrence = source_edges.cluster_occurrence_index[
          cluster.member_occurrences.begin + j];
      if (occurrence.ordinal() >= interning.occurrences.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate source-edge occurrence is invalid");
        return false;
      }
      occurrences.push_back(occurrence);
    }
    locus_key cluster_key;
    cluster_key.locus = intersection_aggregate_locus::cluster;
    cluster_key.ordinal = i;
    cluster_key.source_feature =
        source_edges.sequences[cluster.sequence.ordinal()].source_edge;
    add_occurrence_members(cluster_key, occurrences, catalog, crossing, contact);
    locus_key boundary_key = cluster_key;
    boundary_key.locus = intersection_aggregate_locus::source_edge_boundary;
    add_occurrence_members(boundary_key, occurrences, catalog, crossing, contact);
  }

  for (std::size_t i = 0; i < source_edges.sequences.size(); ++i) {
    const auto &sequence = source_edges.sequences[i];
    if (sequence.id.ordinal() != i ||
        !checked_range(sequence.memberships,
                       source_edges.membership_sequence_index.size())) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate source-edge sequence is malformed");
      return false;
    }
    std::vector<event_occurrence_id> occurrences;
    for (std::uint64_t j = 0; j < sequence.memberships.count; ++j) {
      const auto membership_id = source_edges.membership_sequence_index[
          sequence.memberships.begin + j];
      if (membership_id.ordinal() >= source_edges.memberships.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate source-edge membership is invalid");
        return false;
      }
      const auto occurrence =
          source_edges.memberships[membership_id.ordinal()].occurrence;
      if (occurrence.ordinal() >= interning.occurrences.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate source-edge occurrence is invalid");
        return false;
      }
      occurrences.push_back(occurrence);
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::source_edge;
    key.source_feature = sequence.source_edge;
    key.ordinal = i;
    add_occurrence_members(key, occurrences, catalog, crossing, contact);
  }

  std::map<source_edge_cluster_key, source_edge_cluster_id> source_cluster_ids;
  for (const auto &cluster : source_edges.clusters) {
    if (!source_cluster_ids.emplace(cluster.key, cluster.id).second) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate source-edge cluster key is duplicated");
      return false;
    }
  }

  for (std::size_t i = 0; i < source_edges.intervals.size(); ++i) {
    const auto &interval = source_edges.intervals[i];
    if (interval.id.ordinal() != i ||
        interval.sequence.ordinal() >= source_edges.sequences.size()) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate source-edge interval is malformed");
      return false;
    }
    std::vector<event_occurrence_id> occurrences;
    const source_edge_boundary_key boundaries[] = {interval.key.left,
                                                   interval.key.right};
    for (const auto &boundary : boundaries) {
      if (boundary.kind != boundary_reference_kind::cluster)
        continue;
      const auto found = source_cluster_ids.find(boundary.cluster);
      if (found == source_cluster_ids.end() ||
          found->second.ordinal() >= source_edges.clusters.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate interval boundary is invalid");
        return false;
      }
      const auto &cluster = source_edges.clusters[found->second.ordinal()];
      if (!checked_range(cluster.member_occurrences,
                         source_edges.cluster_occurrence_index.size())) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate interval cluster range is invalid");
        return false;
      }
      for (std::uint64_t j = 0; j < cluster.member_occurrences.count; ++j) {
        const auto occurrence = source_edges.cluster_occurrence_index[
            cluster.member_occurrences.begin + j];
        if (occurrence.ordinal() >= interning.occurrences.size()) {
          error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                  "Component 08 aggregate interval occurrence is invalid");
          return false;
        }
        occurrences.push_back(occurrence);
      }
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::source_edge_interval;
    key.source_feature = interval.key.source_edge;
    key.ordinal = i;
    add_occurrence_members(key, occurrences, catalog, crossing, contact);
  }

  for (std::size_t i = 0; i < transverse.clusters.size(); ++i) {
    const auto &cluster = transverse.clusters[i];
    if (cluster.id.ordinal() != i ||
        !checked_range(cluster.occurrence_members,
                       transverse.cluster_occurrence_index.size())) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate carrier cluster is malformed");
      return false;
    }
    std::vector<event_occurrence_id> occurrences;
    for (std::uint64_t j = 0; j < cluster.occurrence_members.count; ++j) {
      const auto occurrence = transverse.cluster_occurrence_index[
          cluster.occurrence_members.begin + j];
      if (occurrence.ordinal() >= interning.occurrences.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate carrier occurrence is invalid");
        return false;
      }
      occurrences.push_back(occurrence);
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::carrier_cluster;
    key.ordinal = i;
    add_occurrence_members(key, occurrences, catalog, crossing, contact);
  }

  for (std::size_t i = 0; i < transverse.spans.size(); ++i) {
    const auto &span = transverse.spans[i];
    if (span.id.ordinal() != i ||
        span.left.ordinal() >= transverse.clusters.size() ||
        span.right.ordinal() >= transverse.clusters.size()) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate carrier span is malformed");
      return false;
    }
    std::vector<event_occurrence_id> occurrences;
    for (const auto cluster_id : {span.left, span.right}) {
      const auto &cluster = transverse.clusters[cluster_id.ordinal()];
      if (!checked_range(cluster.occurrence_members,
                         transverse.cluster_occurrence_index.size())) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate carrier span cluster range is invalid");
        return false;
      }
      for (std::uint64_t j = 0; j < cluster.occurrence_members.count; ++j) {
        const auto occurrence = transverse.cluster_occurrence_index[
            cluster.occurrence_members.begin + j];
        if (occurrence.ordinal() >= interning.occurrences.size()) {
          error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                  "Component 08 aggregate span occurrence is invalid");
          return false;
        }
        occurrences.push_back(occurrence);
      }
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::carrier_span;
    key.ordinal = i;
    add_occurrence_members(key, occurrences, catalog, crossing, contact);
  }

  for (std::size_t i = 0; i < coplanar.carriers.size(); ++i) {
    const auto &carrier = coplanar.carriers[i];
    if (carrier.id.ordinal() != i ||
        carrier.start_occurrence.ordinal() >= interning.occurrences.size() ||
        carrier.end_occurrence.ordinal() >= interning.occurrences.size()) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate overlap carrier is malformed");
      return false;
    }
    for (std::uint64_t side = 0; side != 2; ++side) {
      locus_key key;
      key.locus = intersection_aggregate_locus::overlap_boundary;
      key.ordinal = i * 2 + side;
      add_occurrence_members(
          key,
          {side == 0 ? carrier.start_occurrence : carrier.end_occurrence},
          catalog, crossing, contact);
    }
  }

  for (std::size_t i = 0; i < coplanar.overlaps.size(); ++i) {
    const auto &overlap = coplanar.overlaps[i];
    if (overlap.id.ordinal() != i ||
        !checked_range(overlap.boundary_events,
                       coplanar.overlap_boundary_event_index.size())) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate coplanar component is malformed");
      return false;
    }
    std::vector<event_occurrence_id> occurrences;
    for (std::uint64_t j = 0; j < overlap.boundary_events.count; ++j) {
      const auto occurrence = coplanar.overlap_boundary_event_index[
          overlap.boundary_events.begin + j];
      if (occurrence.ordinal() >= interning.occurrences.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate coplanar occurrence is invalid");
        return false;
      }
      occurrences.push_back(occurrence);
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::coplanar_component;
    key.ordinal = i;
    add_occurrence_members(key, occurrences, catalog, crossing, contact);
  }

  for (const auto &record : incidence.records) {
    if (record.kind != event_incidence_kind::crossing_contribution)
      continue;
    const auto binding = record.seed_binding.ordinal();
    for (const auto &facet : catalog.facets_by_binding[binding]) {
      locus_key key;
      key.locus = intersection_aggregate_locus::source_facet;
      key.source_feature = facet;
      key.ordinal = facet.primary;
      append_unique(crossing[key], std::vector<event_incidence_id>{record.id});
    }
    for (const auto &shell : catalog.shells_by_binding[binding]) {
      locus_key key;
      key.locus = intersection_aggregate_locus::shell;
      key.source_feature = shell;
      key.ordinal = shell.primary;
      append_unique(crossing[key], std::vector<event_incidence_id>{record.id});
    }
  }
  for (const auto &record : incidence.records) {
    if (record.kind != event_incidence_kind::descriptor_precursor)
      continue;
    const auto binding = record.seed_binding.ordinal();
    for (const auto &facet : catalog.facets_by_binding[binding]) {
      locus_key key;
      key.locus = intersection_aggregate_locus::source_facet;
      key.source_feature = facet;
      key.ordinal = facet.primary;
      append_unique(contact[key], std::vector<event_incidence_id>{record.id});
    }
    for (const auto &shell : catalog.shells_by_binding[binding]) {
      locus_key key;
      key.locus = intersection_aggregate_locus::shell;
      key.source_feature = shell;
      key.ordinal = shell.primary;
      append_unique(contact[key], std::vector<event_incidence_id>{record.id});
    }
  }

  for (auto iterator = crossing.begin(); iterator != crossing.end();) {
    if (iterator->second.empty())
      iterator = crossing.erase(iterator);
    else
      ++iterator;
  }
  for (auto iterator = contact.begin(); iterator != contact.end();) {
    if (iterator->second.empty())
      iterator = contact.erase(iterator);
    else
      ++iterator;
  }
  return true;
}

bool seed_for_member(const std::vector<relation_event_seed_record> &seeds,
                     const event_interning_tables &interning,
                     const event_incidence_record &member,
                     const relation_event_seed_record *&seed) noexcept {
  if (member.seed_binding.ordinal() >= interning.seed_bindings.size())
    return false;
  const auto seed_id =
      interning.seed_bindings[member.seed_binding.ordinal()].seed;
  if (seed_id.ordinal() >= seeds.size() || seeds[seed_id.ordinal()].id != seed_id)
    return false;
  seed = &seeds[seed_id.ordinal()];
  return true;
}

bool accumulate_crossing(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const std::vector<event_incidence_id> &members,
    std::int64_t &numeric, std::int32_t &symbolic,
    std::int32_t &entering, std::int32_t &leaving,
    std::uint8_t &owner_mask, bounded_boolean_error &error) {
  numeric = 0;
  symbolic = 0;
  entering = 0;
  leaving = 0;
  owner_mask = 0;
  event_incidence_id previous{intersection_invalid_ordinal};
  for (const auto id : members) {
    if (id.ordinal() >= incidence.records.size() ||
        (previous.ordinal() != intersection_invalid_ordinal &&
         !(previous < id))) {
      error = aggregate_error(intersection_subcode::member_erased,
                              "Component 08 aggregate members are not unique canonical IDs");
      return false;
    }
    previous = id;
    const auto &member = incidence.records[id.ordinal()];
    if (member.id != id ||
        member.kind != event_incidence_kind::crossing_contribution) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate member is not a crossing contribution");
      return false;
    }
    const relation_event_seed_record *seed = nullptr;
    if (!seed_for_member(seeds, interning, member, seed) ||
        seed->numeric_crossing != member.numeric_crossing ||
        seed->symbolic_crossing != member.symbolic_crossing) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate member seed proof is invalid");
      return false;
    }
    if (!checked_add(numeric, member.numeric_crossing) ||
        !checked_add(symbolic, member.symbolic_crossing)) {
      error = aggregate_error(intersection_subcode::aggregate_mismatch,
                              "Component 08 aggregate signed sum overflowed");
      return false;
    }
    if (member.numeric_crossing > 0) {
      if (!checked_add(entering, 1)) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate entering count overflowed");
        return false;
      }
    } else if (member.numeric_crossing < 0) {
      if (!checked_add(leaving, 1)) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 aggregate leaving count overflowed");
        return false;
      }
    }
    if (member.symbolic_crossing != 0)
      owner_mask = static_cast<std::uint8_t>(owner_mask |
                                             owner_bit(seed->half_open_owner));
  }
  return true;
}

operand_id canonical_owner(std::uint8_t owner_mask) noexcept {
  return owner_mask == owner_bit(operand_id::b) ? operand_id::b
                                                : operand_id::a;
}

bool build_subtotals(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const contribution_catalog &catalog,
    const std::vector<event_incidence_id> &aggregate_members,
    bool facets, std::vector<crossing_subtotal_record> &records,
    std::vector<event_incidence_id> &member_index,
    intersection_range &range, bounded_boolean_error &error) {
  std::map<relation_feature_key, std::vector<event_incidence_id>> groups;
  for (const auto id : aggregate_members) {
    const auto &member = incidence.records[id.ordinal()];
    const auto binding = member.seed_binding.ordinal();
    const auto &features = facets ? catalog.facets_by_binding[binding]
                                  : catalog.shells_by_binding[binding];
    for (const auto &feature : features)
      append_unique(groups[feature], std::vector<event_incidence_id>{id});
  }
  range.begin = records.size();
  for (const auto &entry : groups) {
    crossing_subtotal_record record;
    record.source_feature = entry.first;
    record.members.begin = member_index.size();
    record.members.count = entry.second.size();
    member_index.insert(member_index.end(), entry.second.begin(),
                        entry.second.end());
    std::int32_t entering = 0;
    std::int32_t leaving = 0;
    if (!accumulate_crossing(seeds, interning, incidence, entry.second,
                             record.numeric_signed_sum,
                             record.symbolic_signed_sum, entering, leaving,
                             record.symbolic_owner_mask, error))
      return false;
    record.mixed_symbolic_ownership =
        record.symbolic_owner_mask ==
        static_cast<std::uint8_t>(owner_bit(operand_id::a) |
                                  owner_bit(operand_id::b));
    record.symbolic_owner = canonical_owner(record.symbolic_owner_mask);
    records.push_back(std::move(record));
  }
  range.count = records.size() - range.begin;
  return true;
}

bool publish_crossing(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const contribution_catalog &catalog,
    const std::map<locus_key, std::vector<event_incidence_id>> &groups,
    intersection_aggregate_tables &tables, bounded_boolean_error &error) {
  for (const auto &entry : groups) {
    crossing_aggregate_record record;
    record.id = crossing_aggregate_id{tables.crossing.size()};
    record.locus = entry.first.locus;
    record.source_feature = entry.first.source_feature;
    record.locus_ordinal = entry.first.ordinal;
    record.members.begin = tables.crossing_members.size();
    record.members.count = entry.second.size();
    tables.crossing_members.insert(tables.crossing_members.end(),
                                   entry.second.begin(), entry.second.end());
    if (!accumulate_crossing(
            seeds, interning, incidence, entry.second,
            record.numeric_signed_sum, record.symbolic_signed_sum,
            record.entering_count, record.leaving_count,
            record.symbolic_owner_mask, error))
      return false;
    record.mixed_symbolic_ownership =
        record.symbolic_owner_mask ==
        static_cast<std::uint8_t>(owner_bit(operand_id::a) |
                                  owner_bit(operand_id::b));
    record.symbolic_owner = canonical_owner(record.symbolic_owner_mask);
    record.zero_net_contact_retained =
        record.numeric_signed_sum == 0 && !entry.second.empty();
    if (!build_subtotals(seeds, interning, incidence, catalog, entry.second,
                         true, tables.facet_subtotals,
                         tables.facet_subtotal_members,
                         record.facet_subtotals, error) ||
        !build_subtotals(seeds, interning, incidence, catalog, entry.second,
                         false, tables.shell_subtotals,
                         tables.shell_subtotal_members,
                         record.shell_subtotals, error))
      return false;
    record.member_order_verified = true;
    record.conserved = true;
    tables.crossing.push_back(std::move(record));
  }
  return true;
}

bool publish_contact(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const std::map<locus_key, std::vector<event_incidence_id>> &groups,
    intersection_aggregate_tables &tables, bounded_boolean_error &error) {
  std::map<contact_key, std::vector<event_incidence_id>> split;
  for (const auto &entry : groups) {
    for (const auto id : entry.second) {
      if (id.ordinal() >= incidence.records.size()) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 contact aggregate member is invalid");
        return false;
      }
      const auto &member = incidence.records[id.ordinal()];
      if (member.kind != event_incidence_kind::descriptor_precursor) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 contact aggregate member has wrong kind");
        return false;
      }
      contact_key key;
      key.locus = entry.first;
      key.status = static_cast<feature_relation_status>(member.payload_primary);
      key.dimension =
          static_cast<relation_contact_dimension>(member.payload_secondary);
      if (!valid_contact_class(key.status, key.dimension)) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 contact aggregate class is invalid");
        return false;
      }
      split[key].push_back(id);
    }
  }
  for (auto &entry : split) {
    auto &members = entry.second;
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    contact_aggregate_record record;
    record.id = contact_aggregate_id{tables.contact.size()};
    record.locus = entry.first.locus.locus;
    record.source_feature = entry.first.locus.source_feature;
    record.locus_ordinal = entry.first.locus.ordinal;
    record.contact_status = entry.first.status;
    record.contact_dimension = entry.first.dimension;
    record.members.begin = tables.contact_members.size();
    record.members.count = members.size();
    tables.contact_members.insert(tables.contact_members.end(), members.begin(),
                                  members.end());
    std::int64_t numeric = 0;
    for (const auto id : members) {
      const auto &member = incidence.records[id.ordinal()];
      const relation_event_seed_record *seed = nullptr;
      if (!seed_for_member(seeds, interning, member, seed) ||
          seed->contact_status != record.contact_status ||
          seed->contact_dimension != record.contact_dimension) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 contact aggregate seed proof is invalid");
        return false;
      }
      if (!checked_add(numeric, seed->numeric_crossing)) {
        error = aggregate_error(intersection_subcode::aggregate_mismatch,
                                "Component 08 contact aggregate numeric sum overflowed");
        return false;
      }
      if (seed->symbolic_crossing != 0)
        record.symbolic_owner_mask = static_cast<std::uint8_t>(
            record.symbolic_owner_mask | owner_bit(seed->half_open_owner));
    }
    record.mixed_symbolic_ownership =
        record.symbolic_owner_mask ==
        static_cast<std::uint8_t>(owner_bit(operand_id::a) |
                                  owner_bit(operand_id::b));
    record.symbolic_owner = canonical_owner(record.symbolic_owner_mask);
    record.zero_net_retained = numeric == 0;
    record.tangent_retained =
        record.contact_status == feature_relation_status::tangency;
    record.coincidence_retained =
        record.contact_status ==
            feature_relation_status::coincidence_same_orientation ||
        record.contact_status ==
            feature_relation_status::coincidence_opposite_orientation ||
        record.contact_status == feature_relation_status::overlap;
    record.reconstructed = true;
    tables.contact.push_back(std::move(record));
  }
  return true;
}

bool collect_verifier_loci(
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const contribution_catalog &catalog,
    std::map<locus_key, std::vector<event_incidence_id>> &crossing,
    std::map<locus_key, std::vector<event_incidence_id>> &contact,
    bounded_boolean_error &error) {
  std::vector<std::set<locus_key>> occurrence_loci(interning.occurrences.size());
  for (std::size_t i = 0; i < occurrence_loci.size(); ++i) {
    locus_key key;
    key.locus = intersection_aggregate_locus::event_occurrence;
    key.ordinal = i;
    occurrence_loci[i].insert(key);
  }

  auto add_locus = [&](event_occurrence_id occurrence, const locus_key &key) {
    if (occurrence.ordinal() >= occurrence_loci.size())
      return false;
    occurrence_loci[occurrence.ordinal()].insert(key);
    return true;
  };

  std::map<source_edge_cluster_key, std::vector<event_occurrence_id>>
      source_cluster_occurrences;
  for (std::size_t i = 0; i < source_edges.clusters.size(); ++i) {
    const auto &cluster = source_edges.clusters[i];
    if (cluster.id.ordinal() != i ||
        !checked_range(cluster.member_occurrences,
                       source_edges.cluster_occurrence_index.size()) ||
        cluster.sequence.ordinal() >= source_edges.sequences.size()) {
      error = verifier_error(
          "Component 08 aggregate verifier source-edge cluster is malformed");
      return false;
    }
    locus_key cluster_key;
    cluster_key.locus = intersection_aggregate_locus::cluster;
    cluster_key.source_feature =
        source_edges.sequences[cluster.sequence.ordinal()].source_edge;
    cluster_key.ordinal = i;
    locus_key boundary_key = cluster_key;
    boundary_key.locus = intersection_aggregate_locus::source_edge_boundary;
    auto &occurrences = source_cluster_occurrences[cluster.key];
    for (std::uint64_t j = 0; j < cluster.member_occurrences.count; ++j) {
      const auto occurrence = source_edges.cluster_occurrence_index[
          cluster.member_occurrences.begin + j];
      occurrences.push_back(occurrence);
      if (!add_locus(occurrence, cluster_key) ||
          !add_locus(occurrence, boundary_key)) {
        error = verifier_error(
            "Component 08 aggregate verifier source-edge occurrence is invalid");
        return false;
      }
    }
    std::sort(occurrences.begin(), occurrences.end());
    occurrences.erase(std::unique(occurrences.begin(), occurrences.end()),
                      occurrences.end());
  }

  for (std::size_t i = 0; i < source_edges.sequences.size(); ++i) {
    const auto &sequence = source_edges.sequences[i];
    if (sequence.id.ordinal() != i ||
        !checked_range(sequence.memberships,
                       source_edges.membership_sequence_index.size())) {
      error = verifier_error(
          "Component 08 aggregate verifier source-edge sequence is malformed");
      return false;
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::source_edge;
    key.source_feature = sequence.source_edge;
    key.ordinal = i;
    for (std::uint64_t j = 0; j < sequence.memberships.count; ++j) {
      const auto id = source_edges.membership_sequence_index[
          sequence.memberships.begin + j];
      if (id.ordinal() >= source_edges.memberships.size() ||
          !add_locus(source_edges.memberships[id.ordinal()].occurrence, key)) {
        error = verifier_error(
            "Component 08 aggregate verifier source-edge membership is invalid");
        return false;
      }
    }
  }

  for (std::size_t i = 0; i < source_edges.intervals.size(); ++i) {
    const auto &interval = source_edges.intervals[i];
    if (interval.id.ordinal() != i ||
        interval.sequence.ordinal() >= source_edges.sequences.size()) {
      error = verifier_error(
          "Component 08 aggregate verifier source-edge interval is malformed");
      return false;
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::source_edge_interval;
    key.source_feature = interval.key.source_edge;
    key.ordinal = i;
    const source_edge_boundary_key boundaries[] = {interval.key.left,
                                                   interval.key.right};
    for (const auto &boundary : boundaries) {
      if (boundary.kind != boundary_reference_kind::cluster)
        continue;
      const auto found = source_cluster_occurrences.find(boundary.cluster);
      if (found == source_cluster_occurrences.end()) {
        error = verifier_error(
            "Component 08 aggregate verifier interval boundary is absent");
        return false;
      }
      for (const auto occurrence : found->second)
        if (!add_locus(occurrence, key)) {
          error = verifier_error(
              "Component 08 aggregate verifier interval occurrence is invalid");
          return false;
        }
    }
  }

  std::vector<std::vector<event_occurrence_id>> carrier_cluster_occurrences(
      transverse.clusters.size());
  for (std::size_t i = 0; i < transverse.clusters.size(); ++i) {
    const auto &cluster = transverse.clusters[i];
    if (cluster.id.ordinal() != i ||
        !checked_range(cluster.occurrence_members,
                       transverse.cluster_occurrence_index.size())) {
      error = verifier_error(
          "Component 08 aggregate verifier carrier cluster is malformed");
      return false;
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::carrier_cluster;
    key.ordinal = i;
    for (std::uint64_t j = 0; j < cluster.occurrence_members.count; ++j) {
      const auto occurrence = transverse.cluster_occurrence_index[
          cluster.occurrence_members.begin + j];
      carrier_cluster_occurrences[i].push_back(occurrence);
      if (!add_locus(occurrence, key)) {
        error = verifier_error(
            "Component 08 aggregate verifier carrier occurrence is invalid");
        return false;
      }
    }
  }

  for (std::size_t i = 0; i < transverse.spans.size(); ++i) {
    const auto &span = transverse.spans[i];
    if (span.id.ordinal() != i ||
        span.left.ordinal() >= carrier_cluster_occurrences.size() ||
        span.right.ordinal() >= carrier_cluster_occurrences.size()) {
      error = verifier_error(
          "Component 08 aggregate verifier carrier span is malformed");
      return false;
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::carrier_span;
    key.ordinal = i;
    for (const auto cluster : {span.left, span.right})
      for (const auto occurrence :
           carrier_cluster_occurrences[cluster.ordinal()])
        if (!add_locus(occurrence, key)) {
          error = verifier_error(
              "Component 08 aggregate verifier span occurrence is invalid");
          return false;
        }
  }

  for (std::size_t i = 0; i < coplanar.carriers.size(); ++i) {
    const auto &carrier = coplanar.carriers[i];
    if (carrier.id.ordinal() != i) {
      error = verifier_error(
          "Component 08 aggregate verifier overlap carrier is malformed");
      return false;
    }
    locus_key start;
    start.locus = intersection_aggregate_locus::overlap_boundary;
    start.ordinal = i * 2;
    locus_key end = start;
    end.ordinal += 1;
    if (!add_locus(carrier.start_occurrence, start) ||
        !add_locus(carrier.end_occurrence, end)) {
      error = verifier_error(
          "Component 08 aggregate verifier overlap endpoint is invalid");
      return false;
    }
  }

  for (std::size_t i = 0; i < coplanar.overlaps.size(); ++i) {
    const auto &overlap = coplanar.overlaps[i];
    if (overlap.id.ordinal() != i ||
        !checked_range(overlap.boundary_events,
                       coplanar.overlap_boundary_event_index.size())) {
      error = verifier_error(
          "Component 08 aggregate verifier coplanar component is malformed");
      return false;
    }
    locus_key key;
    key.locus = intersection_aggregate_locus::coplanar_component;
    key.ordinal = i;
    for (std::uint64_t j = 0; j < overlap.boundary_events.count; ++j)
      if (!add_locus(coplanar.overlap_boundary_event_index[
                         overlap.boundary_events.begin + j],
                     key)) {
        error = verifier_error(
            "Component 08 aggregate verifier coplanar occurrence is invalid");
        return false;
      }
  }

  for (const auto &record : incidence.records) {
    const bool is_crossing =
        record.kind == event_incidence_kind::crossing_contribution;
    const bool is_contact =
        record.kind == event_incidence_kind::descriptor_precursor;
    if (!is_crossing && !is_contact)
      continue;
    if (record.event.ordinal() >= interning.events.size() ||
        record.occurrence.ordinal() >= occurrence_loci.size() ||
        record.seed_binding.ordinal() >= catalog.facets_by_binding.size()) {
      error = verifier_error(
          "Component 08 aggregate verifier member reference is invalid");
      return false;
    }
    auto &target = is_crossing ? crossing : contact;
    locus_key event_key;
    event_key.locus = intersection_aggregate_locus::conceptual_event;
    event_key.ordinal = record.event.ordinal();
    target[event_key].push_back(record.id);
    for (const auto &key : occurrence_loci[record.occurrence.ordinal()])
      target[key].push_back(record.id);
    for (const auto &facet :
         catalog.facets_by_binding[record.seed_binding.ordinal()]) {
      locus_key key;
      key.locus = intersection_aggregate_locus::source_facet;
      key.source_feature = facet;
      key.ordinal = facet.primary;
      target[key].push_back(record.id);
    }
    for (const auto &shell :
         catalog.shells_by_binding[record.seed_binding.ordinal()]) {
      locus_key key;
      key.locus = intersection_aggregate_locus::shell;
      key.source_feature = shell;
      key.ordinal = shell.primary;
      target[key].push_back(record.id);
    }
  }

  auto canonicalize = [](auto &groups) {
    for (auto iterator = groups.begin(); iterator != groups.end();) {
      auto &members = iterator->second;
      std::sort(members.begin(), members.end());
      members.erase(std::unique(members.begin(), members.end()), members.end());
      if (members.empty())
        iterator = groups.erase(iterator);
      else
        ++iterator;
    }
  };
  canonicalize(crossing);
  canonicalize(contact);
  return true;
}

bool verify_subtotal_range(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const contribution_catalog &catalog,
    const std::vector<event_incidence_id> &aggregate_members, bool facets,
    const std::vector<crossing_subtotal_record> &records,
    const std::vector<event_incidence_id> &member_index,
    intersection_range range, std::uint64_t &next_record,
    std::uint64_t &next_member) {
  if (range.begin != next_record || !checked_range(range, records.size()))
    return false;
  std::map<relation_feature_key, std::vector<event_incidence_id>> expected;
  for (const auto id : aggregate_members) {
    if (id.ordinal() >= incidence.records.size())
      return false;
    const auto binding = incidence.records[id.ordinal()].seed_binding.ordinal();
    if (binding >= catalog.facets_by_binding.size())
      return false;
    const auto &features = facets ? catalog.facets_by_binding[binding]
                                  : catalog.shells_by_binding[binding];
    for (const auto &feature : features)
      expected[feature].push_back(id);
  }
  if (expected.size() != range.count)
    return false;
  auto iterator = expected.begin();
  for (std::uint64_t i = 0; i < range.count; ++i, ++iterator) {
    const auto &record = records[range.begin + i];
    auto members = iterator->second;
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    if (record.source_feature != iterator->first ||
        record.members.begin != next_member ||
        record.members.count != members.size() ||
        !checked_range(record.members, member_index.size()) ||
        !std::equal(members.begin(), members.end(),
                    member_index.begin() + record.members.begin))
      return false;
    std::int64_t numeric = 0;
    std::int32_t symbolic = 0;
    std::int32_t entering = 0;
    std::int32_t leaving = 0;
    std::uint8_t owner_mask = 0;
    bounded_boolean_error ignored;
    if (!accumulate_crossing(seeds, interning, incidence, members, numeric,
                             symbolic, entering, leaving, owner_mask,
                             ignored) ||
        record.numeric_signed_sum != numeric ||
        record.symbolic_signed_sum != symbolic ||
        record.symbolic_owner_mask != owner_mask ||
        record.symbolic_owner != canonical_owner(owner_mask) ||
        record.mixed_symbolic_ownership !=
            (owner_mask == static_cast<std::uint8_t>(
                               owner_bit(operand_id::a) |
                               owner_bit(operand_id::b))) ||
        record.schema_version != contract_versions::intersection_aggregate_schema ||
        record.reserved16 != 0)
      return false;
    next_member += record.members.count;
  }
  next_record += range.count;
  return true;
}

bool verify_crossing_records(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const contribution_catalog &catalog,
    const std::map<locus_key, std::vector<event_incidence_id>> &expected,
    const intersection_aggregate_tables &tables) {
  if (tables.crossing.size() != expected.size())
    return false;
  std::uint64_t next_member = 0;
  std::uint64_t next_facet_record = 0;
  std::uint64_t next_facet_member = 0;
  std::uint64_t next_shell_record = 0;
  std::uint64_t next_shell_member = 0;
  std::size_t ordinal = 0;
  for (const auto &entry : expected) {
    const auto &record = tables.crossing[ordinal];
    if (record.id.ordinal() != ordinal || record.locus != entry.first.locus ||
        record.source_feature != entry.first.source_feature ||
        record.locus_ordinal != entry.first.ordinal ||
        record.members.begin != next_member ||
        record.members.count != entry.second.size() ||
        !checked_range(record.members, tables.crossing_members.size()) ||
        !std::equal(entry.second.begin(), entry.second.end(),
                    tables.crossing_members.begin() + record.members.begin))
      return false;
    std::int64_t numeric = 0;
    std::int32_t symbolic = 0;
    std::int32_t entering = 0;
    std::int32_t leaving = 0;
    std::uint8_t owner_mask = 0;
    bounded_boolean_error ignored;
    if (!accumulate_crossing(seeds, interning, incidence, entry.second,
                             numeric, symbolic, entering, leaving, owner_mask,
                             ignored) ||
        record.numeric_signed_sum != numeric ||
        record.symbolic_signed_sum != symbolic ||
        record.entering_count != entering ||
        record.leaving_count != leaving ||
        record.symbolic_owner_mask != owner_mask ||
        record.symbolic_owner != canonical_owner(owner_mask) ||
        record.mixed_symbolic_ownership !=
            (owner_mask == static_cast<std::uint8_t>(
                               owner_bit(operand_id::a) |
                               owner_bit(operand_id::b))) ||
        record.zero_net_contact_retained !=
            (numeric == 0 && !entry.second.empty()) ||
        !record.member_order_verified || !record.conserved ||
        record.schema_version != contract_versions::intersection_aggregate_schema ||
        record.reserved8 != 0)
      return false;
    if (!verify_subtotal_range(
            seeds, interning, incidence, catalog, entry.second, true,
            tables.facet_subtotals, tables.facet_subtotal_members,
            record.facet_subtotals, next_facet_record, next_facet_member) ||
        !verify_subtotal_range(
            seeds, interning, incidence, catalog, entry.second, false,
            tables.shell_subtotals, tables.shell_subtotal_members,
            record.shell_subtotals, next_shell_record, next_shell_member))
      return false;
    next_member += record.members.count;
    ++ordinal;
  }
  return next_member == tables.crossing_members.size() &&
         next_facet_record == tables.facet_subtotals.size() &&
         next_facet_member == tables.facet_subtotal_members.size() &&
         next_shell_record == tables.shell_subtotals.size() &&
         next_shell_member == tables.shell_subtotal_members.size();
}

bool verify_contact_records(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const std::map<locus_key, std::vector<event_incidence_id>> &groups,
    const intersection_aggregate_tables &tables) {
  std::map<contact_key, std::vector<event_incidence_id>> expected;
  for (const auto &entry : groups)
    for (const auto id : entry.second) {
      if (id.ordinal() >= incidence.records.size())
        return false;
      const auto &member = incidence.records[id.ordinal()];
      contact_key key;
      key.locus = entry.first;
      key.status = static_cast<feature_relation_status>(member.payload_primary);
      key.dimension =
          static_cast<relation_contact_dimension>(member.payload_secondary);
      expected[key].push_back(id);
    }
  for (auto &entry : expected) {
    auto &members = entry.second;
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
  }
  if (tables.contact.size() != expected.size())
    return false;
  std::uint64_t next_member = 0;
  std::size_t ordinal = 0;
  for (const auto &entry : expected) {
    const auto &record = tables.contact[ordinal];
    if (record.id.ordinal() != ordinal ||
        record.locus != entry.first.locus.locus ||
        record.source_feature != entry.first.locus.source_feature ||
        record.locus_ordinal != entry.first.locus.ordinal ||
        record.contact_status != entry.first.status ||
        record.contact_dimension != entry.first.dimension ||
        record.members.begin != next_member ||
        record.members.count != entry.second.size() ||
        !checked_range(record.members, tables.contact_members.size()) ||
        !std::equal(entry.second.begin(), entry.second.end(),
                    tables.contact_members.begin() + record.members.begin))
      return false;
    std::int64_t numeric = 0;
    std::uint8_t owner_mask = 0;
    for (const auto id : entry.second) {
      const auto &member = incidence.records[id.ordinal()];
      const relation_event_seed_record *seed = nullptr;
      if (!seed_for_member(seeds, interning, member, seed) ||
          seed->contact_status != record.contact_status ||
          seed->contact_dimension != record.contact_dimension ||
          !checked_add(numeric, seed->numeric_crossing))
        return false;
      if (seed->symbolic_crossing != 0)
        owner_mask = static_cast<std::uint8_t>(
            owner_mask | owner_bit(seed->half_open_owner));
    }
    const bool coincidence =
        record.contact_status ==
            feature_relation_status::coincidence_same_orientation ||
        record.contact_status ==
            feature_relation_status::coincidence_opposite_orientation ||
        record.contact_status == feature_relation_status::overlap;
    if (record.symbolic_owner_mask != owner_mask ||
        record.symbolic_owner != canonical_owner(owner_mask) ||
        record.mixed_symbolic_ownership !=
            (owner_mask == static_cast<std::uint8_t>(
                               owner_bit(operand_id::a) |
                               owner_bit(operand_id::b))) ||
        record.zero_net_retained != (numeric == 0) ||
        record.tangent_retained !=
            (record.contact_status == feature_relation_status::tangency) ||
        record.coincidence_retained != coincidence || !record.reconstructed ||
        record.schema_version != contract_versions::intersection_aggregate_schema ||
        record.reserved16 != 0)
      return false;
    next_member += record.members.count;
    ++ordinal;
  }
  return next_member == tables.contact_members.size();
}


} // namespace

bool build_intersection_aggregates(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    intersection_aggregate_tables &tables, bounded_boolean_error &error) {
  tables = intersection_aggregate_tables{};
  contribution_catalog catalog;
  if (!validate_incidence_and_build_catalog(seeds, interning, incidence,
                                             catalog, error))
    return false;
  std::map<locus_key, std::vector<event_incidence_id>> crossing;
  std::map<locus_key, std::vector<event_incidence_id>> contact;
  if (!collect_producer_loci(interning, incidence, source_edges, transverse,
                             coplanar, catalog, crossing, contact, error) ||
      !publish_crossing(seeds, interning, incidence, catalog, crossing, tables,
                        error) ||
      !publish_contact(seeds, interning, incidence, contact, tables, error))
    return false;
  return verify_intersection_aggregates(
      seeds, interning, incidence, source_edges, transverse, coplanar, tables,
      error);
}

bool verify_intersection_aggregates(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &tables, bounded_boolean_error &error) {
  contribution_catalog catalog;
  bounded_boolean_error local_error;
  if (!validate_incidence_and_build_catalog(seeds, interning, incidence,
                                             catalog, local_error)) {
    error = verifier_error(
        "Component 08 aggregate verifier rejected malformed inputs");
    return false;
  }
  std::map<locus_key, std::vector<event_incidence_id>> crossing;
  std::map<locus_key, std::vector<event_incidence_id>> contact;
  if (!collect_verifier_loci(interning, incidence, source_edges, transverse,
                             coplanar, catalog, crossing, contact, error) ||
      !verify_crossing_records(seeds, interning, incidence, catalog, crossing,
                               tables) ||
      !verify_contact_records(seeds, interning, incidence, contact, tables)) {
    if (error.subcode !=
        static_cast<std::uint32_t>(intersection_subcode::verifier_rejection))
      error = verifier_error(
          "Component 08 aggregate verifier reconstruction mismatch");
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
