#include "IntersectionDescriptors.h"

#include "CanonicalHalfedgeQueries.h"

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

bounded_boolean_error descriptor_error(const char *summary) {
  return intersection_error(intersection_subcode::descriptor_mismatch,
                            bounded_boolean_error_category::input_contract_error,
                            summary,
                            intersection_checkpoint::descriptor_derivation);
}

bounded_boolean_error descriptor_verifier_error(const char *summary) {
  return intersection_error(intersection_subcode::verifier_rejection,
                            bounded_boolean_error_category::internal_invariant_error,
                            summary,
                            intersection_checkpoint::descriptor_derivation);
}

bool checked_range(intersection_range range, std::size_t size) noexcept {
  return range.begin <= size &&
         range.count <= size - static_cast<std::size_t>(range.begin);
}

template <class T, class U>
bool checked_add(T &target, U value) noexcept {
  static_assert(std::numeric_limits<T>::is_integer, "integer required");
  const auto converted = static_cast<T>(value);
  if ((converted > 0 && target > std::numeric_limits<T>::max() - converted) ||
      (converted < 0 && target < std::numeric_limits<T>::min() - converted))
    return false;
  target = static_cast<T>(target + converted);
  return true;
}

struct aggregate_key final {
  intersection_aggregate_locus locus =
      intersection_aggregate_locus::event_occurrence;
  relation_feature_key source_feature{};
  std::uint64_t ordinal = 0;

  friend bool operator<(const aggregate_key &a,
                        const aggregate_key &b) noexcept {
    return std::tie(a.locus, a.source_feature, a.ordinal) <
           std::tie(b.locus, b.source_feature, b.ordinal);
  }
};

struct aggregate_index final {
  std::map<aggregate_key, const crossing_aggregate_record *> crossing{};
  std::map<aggregate_key, std::vector<const contact_aggregate_record *>>
      contact{};
  std::vector<std::vector<event_incidence_id>> crossing_by_binding{};
};

bool build_aggregate_index(
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_aggregate_tables &aggregates,
    aggregate_index &index, bounded_boolean_error &error) {
  index.crossing_by_binding.resize(interning.seed_bindings.size());
  for (const auto &record : incidence.records) {
    if (record.seed_binding.ordinal() >= index.crossing_by_binding.size()) {
      error = descriptor_error(
          "Component 08 descriptor incidence binding is invalid");
      return false;
    }
    if (record.kind == event_incidence_kind::crossing_contribution)
      index.crossing_by_binding[record.seed_binding.ordinal()].push_back(
          record.id);
  }
  for (auto &members : index.crossing_by_binding) {
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
  }

  for (std::size_t i = 0; i < aggregates.crossing.size(); ++i) {
    const auto &record = aggregates.crossing[i];
    if (record.id.ordinal() != i ||
        !checked_range(record.members, aggregates.crossing_members.size()) ||
        record.schema_version !=
            contract_versions::intersection_aggregate_schema ||
        record.reserved8 != 0) {
      error = descriptor_error(
          "Component 08 descriptor crossing aggregate is malformed");
      return false;
    }
    const aggregate_key key{record.locus, record.source_feature,
                            record.locus_ordinal};
    if (!index.crossing.emplace(key, &record).second) {
      error = descriptor_error(
          "Component 08 descriptor crossing aggregate key is duplicated");
      return false;
    }
  }
  for (std::size_t i = 0; i < aggregates.contact.size(); ++i) {
    const auto &record = aggregates.contact[i];
    if (record.id.ordinal() != i ||
        !checked_range(record.members, aggregates.contact_members.size()) ||
        record.schema_version !=
            contract_versions::intersection_aggregate_schema ||
        record.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 descriptor contact aggregate is malformed");
      return false;
    }
    const aggregate_key key{record.locus, record.source_feature,
                            record.locus_ordinal};
    index.contact[key].push_back(&record);
  }
  for (auto &entry : index.contact) {
    auto &records = entry.second;
    std::sort(records.begin(), records.end(), [](const auto *a, const auto *b) {
      return a->id < b->id;
    });
  }
  return true;
}

struct descriptor_proposal final {
  intersection_descriptor_record record{};
  std::vector<event_incidence_id> provenance{};
};

void canonicalize_provenance(std::vector<event_incidence_id> &provenance) {
  std::sort(provenance.begin(), provenance.end());
  provenance.erase(std::unique(provenance.begin(), provenance.end()),
                   provenance.end());
}

struct rule_group_key final {
  intersection_descriptor_category category =
      intersection_descriptor_category::no_influence;
  operand_id symbolic_owner = operand_id::a;
  std::uint64_t symbolic_rule_ordinal = 0;
  std::int8_t orientation = 0;
  bool occurrence_separation_required = false;

  friend bool operator<(const rule_group_key &a,
                        const rule_group_key &b) noexcept {
    return std::tie(a.category, a.symbolic_owner,
                    a.symbolic_rule_ordinal, a.orientation,
                    a.occurrence_separation_required) <
           std::tie(b.category, b.symbolic_owner,
                    b.symbolic_rule_ordinal, b.orientation,
                    b.occurrence_separation_required);
  }
};

bool category_from_contact(feature_relation_status status, bool interior,
                           bool separated,
                           intersection_descriptor_category &category) {
  if (separated &&
      (status == feature_relation_status::point_contact ||
       status == feature_relation_status::segment_contact ||
       status == feature_relation_status::tangency ||
       status == feature_relation_status::containment)) {
    category = intersection_descriptor_category::topology_separated_contact;
    return true;
  }
  switch (status) {
  case feature_relation_status::proper_crossing:
    category = intersection_descriptor_category::proper_crossing;
    return true;
  case feature_relation_status::endpoint_crossing:
    category = intersection_descriptor_category::endpoint_crossing;
    return true;
  case feature_relation_status::tangency:
    category = intersection_descriptor_category::tangent;
    return true;
  case feature_relation_status::point_contact:
  case feature_relation_status::segment_contact:
  case feature_relation_status::containment:
    category = intersection_descriptor_category::contact_delimiter;
    return true;
  case feature_relation_status::overlap:
    category = interior
                   ? intersection_descriptor_category::coincident_sheet_interior
                   : intersection_descriptor_category::coplanar_overlap_boundary;
    return true;
  case feature_relation_status::coincidence_same_orientation:
  case feature_relation_status::coincidence_opposite_orientation:
    category = interior
                   ? intersection_descriptor_category::coincident_sheet_interior
                   : intersection_descriptor_category::coincident_sheet_boundary;
    return true;
  case feature_relation_status::not_evaluated:
  case feature_relation_status::definitely_separated:
    return false;
  }
  return false;
}

operand_id descriptor_symbolic_owner(
    const relation_event_seed_record &seed) noexcept {
  const bool owner_is_semantic =
      seed.has_symbolic_decision || seed.symbolic_crossing != 0 ||
      seed.contact_status == feature_relation_status::endpoint_crossing ||
      seed.contact_status == feature_relation_status::overlap ||
      seed.contact_status ==
          feature_relation_status::coincidence_same_orientation ||
      seed.contact_status ==
          feature_relation_status::coincidence_opposite_orientation;
  return owner_is_semantic ? seed.half_open_owner : operand_id::a;
}

void append_locus_provenance(
    const aggregate_key &locus, const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<event_incidence_id> &provenance) {
  const auto crossing = index.crossing.find(locus);
  if (crossing != index.crossing.end()) {
    const auto range = crossing->second->members;
    provenance.insert(provenance.end(),
                      aggregates.crossing_members.begin() + range.begin,
                      aggregates.crossing_members.begin() + range.begin +
                          range.count);
  }
  const auto contacts = index.contact.find(locus);
  if (contacts != index.contact.end()) {
    for (const auto *contact : contacts->second) {
      provenance.insert(provenance.end(),
                        aggregates.contact_members.begin() +
                            contact->members.begin,
                        aggregates.contact_members.begin() +
                            contact->members.begin + contact->members.count);
    }
  }
}

void set_consumption_flags(intersection_descriptor_record &record) noexcept {
  record.continuation_allowed = false;
  record.occurrence_separation_required = false;
  record.classification_consumable = false;
  record.selection_consumable = false;
  record.topology_consumable = true;
  switch (record.key.category) {
  case intersection_descriptor_category::no_influence:
    record.continuation_allowed = true;
    break;
  case intersection_descriptor_category::proper_crossing:
  case intersection_descriptor_category::endpoint_crossing:
    record.classification_consumable = true;
    record.selection_consumable = true;
    break;
  case intersection_descriptor_category::tangent:
    record.continuation_allowed = true;
    record.classification_consumable = true;
    break;
  case intersection_descriptor_category::contact_delimiter:
    record.continuation_allowed = true;
    record.classification_consumable = true;
    break;
  case intersection_descriptor_category::coplanar_overlap_boundary:
  case intersection_descriptor_category::coincident_sheet_boundary:
    record.classification_consumable = true;
    record.selection_consumable = true;
    break;
  case intersection_descriptor_category::coincident_sheet_interior:
    record.continuation_allowed = true;
    record.classification_consumable = true;
    record.selection_consumable = true;
    break;
  case intersection_descriptor_category::bookkeeping_only:
    record.continuation_allowed = true;
    break;
  case intersection_descriptor_category::topology_separated_contact:
    record.occurrence_separation_required = true;
    record.classification_consumable = true;
    break;
  case intersection_descriptor_category::unresolved:
  case intersection_descriptor_category::invalid:
    record.topology_consumable = false;
    break;
  }
}

bool seed_from_precursor(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_record &member,
    const relation_event_seed_record *&seed,
    bounded_boolean_error &error) {
  if (member.kind != event_incidence_kind::descriptor_precursor ||
      member.seed_binding.ordinal() >= interning.seed_bindings.size()) {
    error = descriptor_error(
        "Component 08 descriptor precursor binding is malformed");
    return false;
  }
  const auto seed_id =
      interning.seed_bindings[member.seed_binding.ordinal()].seed;
  if (seed_id.ordinal() >= seeds.size() || seeds[seed_id.ordinal()].id != seed_id) {
    error = descriptor_error(
        "Component 08 descriptor precursor seed is malformed");
    return false;
  }
  seed = &seeds[seed_id.ordinal()];
  if (static_cast<std::uint64_t>(seed->contact_status) !=
          member.payload_primary ||
      static_cast<std::uint64_t>(seed->contact_dimension) !=
          member.payload_secondary ||
      member.orientation < -1 || member.orientation > 1) {
    error = descriptor_error(
        "Component 08 descriptor precursor disagrees with its seed");
    return false;
  }
  return true;
}

struct grouped_descriptor final {
  std::int32_t signed_delta = 0;
  std::vector<event_incidence_id> provenance{};
};

bool append_contact_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index, const aggregate_key &aggregate_locus,
    intersection_descriptor_locus descriptor_locus,
    relation_feature_key source_feature, bool interior,
    bool force_separation, const intersection_occurrence_key *occurrence,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  const auto found = index.contact.find(aggregate_locus);
  if (found == index.contact.end())
    return true;
  for (const auto *contact : found->second) {
    std::map<rule_group_key, grouped_descriptor> groups;
    for (std::uint64_t i = 0; i < contact->members.count; ++i) {
      const auto incidence_id =
          aggregates.contact_members[contact->members.begin + i];
      if (incidence_id.ordinal() >= incidence.records.size()) {
        error = descriptor_error(
            "Component 08 descriptor contact member is invalid");
        return false;
      }
      const auto &member = incidence.records[incidence_id.ordinal()];
      const relation_event_seed_record *seed = nullptr;
      if (!seed_from_precursor(seeds, interning, member, seed, error))
        return false;
      if (seed->contact_status != contact->contact_status ||
          seed->contact_dimension != contact->contact_dimension) {
        error = descriptor_error(
            "Component 08 descriptor contact member class mismatches its aggregate");
        return false;
      }
      const auto occurrence_id = member.occurrence;
      if (occurrence_id.ordinal() >= interning.occurrences.size()) {
        error = descriptor_error(
            "Component 08 descriptor occurrence reference is invalid");
        return false;
      }
      const auto &occurrence_record =
          interning.occurrences[occurrence_id.ordinal()];
      const bool separated =
          force_separation || occurrence_record.topology_separate ||
          occurrence_record.requires_contact_separation;
      intersection_descriptor_category category;
      if (!category_from_contact(seed->contact_status, interior, separated,
                                 category)) {
        error = descriptor_error(
            "Component 08 descriptor contact class has no derivation rule");
        return false;
      }
      rule_group_key group_key;
      group_key.category = category;
      group_key.symbolic_owner = descriptor_symbolic_owner(*seed);
      group_key.symbolic_rule_ordinal =
          seed->has_symbolic_decision ? seed->symbolic_rule_ordinal : 0;
      group_key.orientation = member.orientation;
      group_key.occurrence_separation_required = separated;
      auto &group = groups[group_key];
      if (!checked_add(group.signed_delta, seed->numeric_crossing)) {
        error = descriptor_error(
            "Component 08 descriptor crossing delta overflowed");
        return false;
      }
      group.provenance.push_back(incidence_id);
      const auto binding = member.seed_binding.ordinal();
      if (binding >= index.crossing_by_binding.size()) {
        error = descriptor_error(
            "Component 08 descriptor crossing binding is invalid");
        return false;
      }
      group.provenance.insert(group.provenance.end(),
                              index.crossing_by_binding[binding].begin(),
                              index.crossing_by_binding[binding].end());
    }

    std::uint64_t group_ordinal = 0;
    for (auto &entry : groups) {
      canonicalize_provenance(entry.second.provenance);
      descriptor_proposal proposal;
      proposal.record.key.locus = descriptor_locus;
      proposal.record.key.category = entry.first.category;
      proposal.record.key.source_feature = source_feature;
      if (occurrence != nullptr)
        proposal.record.key.occurrence = *occurrence;
      proposal.record.key.parent_lineage = contact->id.ordinal();
      proposal.record.key.boundary_ordinal = group_ordinal;
      proposal.record.key.orientation = entry.first.orientation;
      proposal.record.signed_crossing_delta = entry.second.signed_delta;
      proposal.record.symbolic_owner = entry.first.symbolic_owner;
      proposal.record.symbolic_rule_ordinal =
          entry.first.symbolic_rule_ordinal;
      set_consumption_flags(proposal.record);
      if (entry.first.occurrence_separation_required) {
        proposal.record.occurrence_separation_required = true;
        proposal.record.continuation_allowed = false;
      }
      proposal.provenance = std::move(entry.second.provenance);
      proposals.push_back(std::move(proposal));
      ++group_ordinal;
    }
  }
  return true;
}

void append_empty_descriptor(
    intersection_descriptor_locus locus,
    intersection_descriptor_category category,
    relation_feature_key source_feature, std::uint64_t parent_lineage,
    std::uint64_t boundary_ordinal, std::int8_t orientation,
    operand_id owner, std::vector<descriptor_proposal> &proposals) {
  descriptor_proposal proposal;
  proposal.record.key.locus = locus;
  proposal.record.key.category = category;
  proposal.record.key.source_feature = source_feature;
  proposal.record.key.parent_lineage = parent_lineage;
  proposal.record.key.boundary_ordinal = boundary_ordinal;
  proposal.record.key.orientation = orientation;
  proposal.record.symbolic_owner = owner;
  set_consumption_flags(proposal.record);
  proposals.push_back(std::move(proposal));
}

bool append_source_edge_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  for (std::size_t i = 0; i < source_edges.sequences.size(); ++i) {
    const auto &sequence = source_edges.sequences[i];
    if (sequence.id.ordinal() != i ||
        !valid_relation_feature_key(sequence.source_edge, false) ||
        sequence.source_edge.kind != relation_feature_kind::source_edge ||
        sequence.schema_version !=
            contract_versions::intersection_source_edge_sequence_schema ||
        sequence.reserved8 != 0 || sequence.reserved32 != 0) {
      error = descriptor_error(
          "Component 08 descriptor source-edge sequence is malformed");
      return false;
    }
    if (sequence.memberships.count == 0) {
      append_empty_descriptor(
          intersection_descriptor_locus::whole_source_edge,
          intersection_descriptor_category::no_influence,
          sequence.source_edge, sequence.id.ordinal(), 0,
          sequence.canonical_forward ? 1 : -1, operand_id::a, proposals);
    }
  }

  for (std::size_t i = 0; i < source_edges.clusters.size(); ++i) {
    const auto &cluster = source_edges.clusters[i];
    if (cluster.id.ordinal() != i ||
        cluster.sequence.ordinal() >= source_edges.sequences.size() ||
        cluster.schema_version != contract_versions::intersection_cluster_schema) {
      error = descriptor_error(
          "Component 08 descriptor source-edge cluster is malformed");
      return false;
    }
    const auto &sequence = source_edges.sequences[cluster.sequence.ordinal()];
    if (!(cluster.key.source_edge == sequence.source_edge)) {
      error = descriptor_error(
          "Component 08 descriptor source-edge cluster owner mismatches");
      return false;
    }
    const aggregate_key locus{intersection_aggregate_locus::source_edge_boundary,
                              sequence.source_edge, i};
    if (!append_contact_descriptors(
            seeds, interning, incidence, aggregates, index, locus,
            intersection_descriptor_locus::source_edge_cluster_boundary,
            sequence.source_edge, false, cluster.separate_output_occurrences,
            nullptr, proposals, error))
      return false;
    if (index.contact.find(locus) == index.contact.end()) {
      error = descriptor_error(
          "Component 08 source-edge cluster has no descriptor precursor");
      return false;
    }
  }

  for (std::size_t i = 0; i < source_edges.intervals.size(); ++i) {
    const auto &interval = source_edges.intervals[i];
    if (interval.id.ordinal() != i ||
        interval.sequence.ordinal() >= source_edges.sequences.size() ||
        interval.schema_version !=
            contract_versions::intersection_source_edge_interval_schema ||
        interval.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 descriptor source-edge interval is malformed");
      return false;
    }
    const auto &sequence = source_edges.sequences[interval.sequence.ordinal()];
    if (!(interval.key.source_edge == sequence.source_edge)) {
      error = descriptor_error(
          "Component 08 descriptor source-edge interval owner mismatches");
      return false;
    }
    intersection_descriptor_category category =
        intersection_descriptor_category::no_influence;
    switch (interval.length_disposition) {
    case intersection_interval_length::definitely_positive:
      category = interval.duplicate_required
                     ? intersection_descriptor_category::coincident_sheet_interior
                     : intersection_descriptor_category::no_influence;
      break;
    case intersection_interval_length::exact_zero:
      category = intersection_descriptor_category::bookkeeping_only;
      break;
    case intersection_interval_length::overlap:
      category = intersection_descriptor_category::coincident_sheet_interior;
      break;
    case intersection_interval_length::uncertain:
      error = descriptor_error(
          "Component 08 unresolved source-edge interval cannot produce a descriptor");
      return false;
    default:
      error = descriptor_error(
          "Component 08 source-edge interval length class is invalid");
      return false;
    }
    append_empty_descriptor(
        intersection_descriptor_locus::source_edge_open_interval, category,
        sequence.source_edge, interval.sequence.ordinal(), interval.id.ordinal(),
        sequence.canonical_forward ? 1 : -1, operand_id::a, proposals);
    auto &record = proposals.back().record;
    record.signed_crossing_delta = interval.accumulated_crossing;
    record.continuation_allowed = interval.propagation_allowed;
    record.occurrence_separation_required = interval.duplicate_required;
    record.classification_consumable = interval.retention_allowed ||
                                       interval.accumulated_crossing != 0;
    record.selection_consumable = interval.retention_allowed;
    record.topology_consumable = true;
    append_locus_provenance(
        aggregate_key{intersection_aggregate_locus::source_edge_interval,
                      interval.key.source_edge, interval.id.ordinal()},
        aggregates, index, proposals.back().provenance);
  }

  for (std::size_t i = 0; i < source_edges.memberships.size(); ++i) {
    const auto &membership = source_edges.memberships[i];
    if (membership.id.ordinal() != i ||
        !valid_relation_feature_key(membership.key.source_edge, false) ||
        membership.key.source_edge.kind != relation_feature_kind::source_edge ||
        membership.schema_version !=
            contract_versions::intersection_source_edge_membership_schema ||
        membership.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 descriptor source-edge membership is malformed");
      return false;
    }
    if (!membership.internal_diagonal_discovery)
      continue;
    if (!membership.bookkeeping_only ||
        membership.event.ordinal() >= interning.events.size() ||
        membership.occurrence.ordinal() >= interning.occurrences.size() ||
        interning.occurrences[membership.occurrence.ordinal()].event !=
            membership.event) {
      error = descriptor_error(
          "Component 08 descriptor internal-diagonal membership is invalid");
      return false;
    }
    const aggregate_key occurrence_locus{
        intersection_aggregate_locus::event_occurrence,
        relation_feature_key{}, membership.occurrence.ordinal()};
    const auto crossing = index.crossing.find(occurrence_locus);
    if (crossing != index.crossing.end() &&
        crossing->second->numeric_signed_sum != 0) {
      error = descriptor_error(
          "Component 08 internal diagonal owns an authoritative crossing");
      return false;
    }
    append_empty_descriptor(
        intersection_descriptor_locus::transparent_internal_diagonal_adjacency,
        intersection_descriptor_category::bookkeeping_only,
        membership.key.source_edge, membership.id.ordinal(), 0, 0,
        operand_id::a, proposals);
    proposals.back().record.continuation_allowed = true;
    append_locus_provenance(occurrence_locus, aggregates, index,
                            proposals.back().provenance);
  }
  return true;
}

bool append_transverse_descriptors(
    const transverse_carrier_arrangement_tables &transverse,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  for (std::size_t i = 0; i < transverse.spans.size(); ++i) {
    const auto &span = transverse.spans[i];
    if (span.id.ordinal() != i ||
        span.carrier.ordinal() >= transverse.carriers.size() ||
        span.left.ordinal() >= transverse.clusters.size() ||
        span.right.ordinal() >= transverse.clusters.size() ||
        transverse.clusters[span.left.ordinal()].id != span.left ||
        transverse.clusters[span.right.ordinal()].id != span.right ||
        transverse.clusters[span.left.ordinal()].carrier != span.carrier ||
        transverse.clusters[span.right.ordinal()].carrier != span.carrier ||
        transverse.carriers[span.carrier.ordinal()].id != span.carrier ||
        transverse.carriers[span.carrier.ordinal()].schema_version !=
            contract_versions::intersection_carrier_schema ||
        transverse.carriers[span.carrier.ordinal()].reserved16 != 0 ||
        transverse.carriers[span.carrier.ordinal()].reserved32 != 0 ||
        span.schema_version != contract_versions::intersection_carrier_schema ||
        span.reserved8 != 0) {
      error = descriptor_error(
          "Component 08 descriptor transverse span is malformed");
      return false;
    }
    intersection_descriptor_locus locus =
        intersection_descriptor_locus::transverse_active_span;
    intersection_descriptor_category category =
        intersection_descriptor_category::no_influence;
    switch (span.activation) {
    case intersection_span_activation::inactive:
      locus = intersection_descriptor_locus::transverse_inactive_gap;
      category = intersection_descriptor_category::no_influence;
      break;
    case intersection_span_activation::active_transverse_intersection:
      category = intersection_descriptor_category::proper_crossing;
      break;
    case intersection_span_activation::active_overlap_boundary:
      category = intersection_descriptor_category::coplanar_overlap_boundary;
      break;
    case intersection_span_activation::active_coincident_boundary:
      category = intersection_descriptor_category::coincident_sheet_boundary;
      break;
    case intersection_span_activation::contact_only:
      category = intersection_descriptor_category::contact_delimiter;
      break;
    case intersection_span_activation::unresolved:
    case intersection_span_activation::invalid:
      error = descriptor_error(
          "Component 08 unresolved transverse span cannot produce a descriptor");
      return false;
    default:
      error = descriptor_error(
          "Component 08 transverse span activation is invalid");
      return false;
    }
    append_empty_descriptor(locus, category, relation_feature_key{},
                            span.id.ordinal(),
                            static_cast<std::uint64_t>(span.activation), 1,
                            operand_id::a, proposals);
    auto &record = proposals.back().record;
    const aggregate_key aggregate_locus{
        intersection_aggregate_locus::carrier_span,
        relation_feature_key{}, span.id.ordinal()};
    const auto crossing = index.crossing.find(aggregate_locus);
    if (crossing != index.crossing.end()) {
      const auto *aggregate = crossing->second;
      if (aggregate->mixed_symbolic_ownership) {
        error = descriptor_error(
            "Component 08 transverse descriptor has mixed symbolic ownership");
        return false;
      }
      if (aggregate->numeric_signed_sum <
              std::numeric_limits<std::int32_t>::min() ||
          aggregate->numeric_signed_sum >
              std::numeric_limits<std::int32_t>::max()) {
        error = descriptor_error(
            "Component 08 transverse descriptor delta is out of range");
        return false;
      }
      record.signed_crossing_delta =
          static_cast<std::int32_t>(aggregate->numeric_signed_sum);
      record.symbolic_owner = aggregate->symbolic_owner;
      proposals.back().provenance.assign(
          aggregates.crossing_members.begin() + aggregate->members.begin,
          aggregates.crossing_members.begin() + aggregate->members.begin +
              aggregate->members.count);
    } else if (span.activation != intersection_span_activation::inactive) {
      error = descriptor_error(
          "Component 08 active transverse span has no aggregate evidence");
      return false;
    }
    append_locus_provenance(aggregate_locus, aggregates, index,
                            proposals.back().provenance);
    record.continuation_allowed = false;
    record.classification_consumable = span.classification_cut ||
                                       span.contact_delimiter;
    record.selection_consumable = span.output_edge_allowed;
    record.topology_consumable = true;
  }
  return true;
}

bool append_coplanar_descriptors(
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  for (std::size_t i = 0; i < coplanar.overlaps.size(); ++i) {
    const auto &overlap = coplanar.overlaps[i];
    if (overlap.id.ordinal() != i ||
        static_cast<std::uint8_t>(overlap.kind) < 1 ||
        static_cast<std::uint8_t>(overlap.kind) > 4 ||
        overlap.boundary_events.count == 0 ||
        !checked_range(overlap.boundary_events,
                       coplanar.overlap_boundary_event_index.size()) ||
        overlap.schema_version != contract_versions::intersection_overlap_schema ||
        overlap.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 descriptor coplanar overlap is malformed");
      return false;
    }
    const bool coincident =
        overlap.kind == relation_coplanar_component_kind::coincident_sheet_boundary ||
        overlap.distinct_sheet_occurrences;
    const auto locus = coincident
                           ? intersection_descriptor_locus::coincident_sheet
                           : intersection_descriptor_locus::coplanar_overlap;
    const bool coincident_boundary =
        overlap.kind ==
            relation_coplanar_component_kind::coincident_sheet_boundary ||
        overlap.zero_measure;
    const auto category =
        coincident
            ? coincident_boundary
                  ? intersection_descriptor_category::coincident_sheet_boundary
                  : intersection_descriptor_category::coincident_sheet_interior
            : intersection_descriptor_category::coplanar_overlap_boundary;
    append_empty_descriptor(locus, category, relation_feature_key{},
                            overlap.id.ordinal(),
                            static_cast<std::uint64_t>(overlap.kind), 0,
                            overlap.symbolic_owner, proposals);
    auto &proposal = proposals.back();
    proposal.record.occurrence_separation_required =
        overlap.distinct_sheet_occurrences;
    proposal.record.continuation_allowed =
        category == intersection_descriptor_category::coincident_sheet_interior;
    const aggregate_key aggregate_locus{
        intersection_aggregate_locus::coplanar_component,
        relation_feature_key{}, overlap.id.ordinal()};
    const auto crossing = index.crossing.find(aggregate_locus);
    if (crossing != index.crossing.end()) {
      const auto *aggregate = crossing->second;
      if (aggregate->mixed_symbolic_ownership) {
        error = descriptor_error(
            "Component 08 coplanar descriptor has mixed symbolic ownership");
        return false;
      }
      if (aggregate->numeric_signed_sum <
              std::numeric_limits<std::int32_t>::min() ||
          aggregate->numeric_signed_sum >
              std::numeric_limits<std::int32_t>::max()) {
        error = descriptor_error(
            "Component 08 coplanar descriptor delta is out of range");
        return false;
      }
      proposal.record.signed_crossing_delta =
          static_cast<std::int32_t>(aggregate->numeric_signed_sum);
      proposal.provenance.assign(
          aggregates.crossing_members.begin() + aggregate->members.begin,
          aggregates.crossing_members.begin() + aggregate->members.begin +
              aggregate->members.count);
    }
    append_locus_provenance(aggregate_locus, aggregates, index,
                            proposal.provenance);
  }
  return true;
}

bool append_separated_occurrence_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  for (std::size_t i = 0; i < interning.occurrences.size(); ++i) {
    const auto &occurrence = interning.occurrences[i];
    if (!occurrence.topology_separate &&
        !occurrence.requires_contact_separation)
      continue;
    const aggregate_key locus{intersection_aggregate_locus::event_occurrence,
                              relation_feature_key{}, i};
    const auto found = index.contact.find(locus);
    if (found == index.contact.end()) {
      error = descriptor_error(
          "Component 08 separated occurrence has no contact evidence");
      return false;
    }
    if (!append_contact_descriptors(
            seeds, interning, incidence, aggregates, index, locus,
            intersection_descriptor_locus::separated_contact_occurrence,
            relation_feature_key{}, false, true, &occurrence.key, proposals,
            error))
      return false;
  }
  return true;
}

bool collect_producer_proposals(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  return append_source_edge_descriptors(
             seeds, interning, incidence, source_edges, aggregates, index,
             proposals, error) &&
         append_transverse_descriptors(transverse, aggregates, index,
                                       proposals, error) &&
         append_coplanar_descriptors(coplanar, aggregates, index, proposals,
                                     error) &&
         append_separated_occurrence_descriptors(
             seeds, interning, incidence, aggregates, index, proposals, error);
}

bool append_verifier_contact_record(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index, const contact_aggregate_record &contact,
    intersection_descriptor_locus descriptor_locus,
    relation_feature_key source_feature, bool interior,
    bool force_separation, const intersection_occurrence_key *occurrence,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  std::map<rule_group_key, grouped_descriptor> groups;
  for (std::uint64_t i = 0; i < contact.members.count; ++i) {
    const auto incidence_id =
        aggregates.contact_members[contact.members.begin + i];
    if (incidence_id.ordinal() >= incidence.records.size()) {
      error = descriptor_error(
          "Component 08 verifier contact member is invalid");
      return false;
    }
    const auto &member = incidence.records[incidence_id.ordinal()];
    const relation_event_seed_record *seed = nullptr;
    if (!seed_from_precursor(seeds, interning, member, seed, error))
      return false;
    if (seed->contact_status != contact.contact_status ||
        seed->contact_dimension != contact.contact_dimension) {
      error = descriptor_error(
          "Component 08 verifier contact class mismatches its aggregate");
      return false;
    }
    if (member.occurrence.ordinal() >= interning.occurrences.size()) {
      error = descriptor_error(
          "Component 08 verifier contact occurrence is invalid");
      return false;
    }
    const auto &occurrence_record =
        interning.occurrences[member.occurrence.ordinal()];
    const bool separated =
        force_separation || occurrence_record.topology_separate ||
        occurrence_record.requires_contact_separation;
    intersection_descriptor_category category;
    if (!category_from_contact(seed->contact_status, interior, separated,
                               category)) {
      error = descriptor_error(
          "Component 08 verifier contact class has no derivation rule");
      return false;
    }
    rule_group_key group_key;
    group_key.category = category;
    group_key.symbolic_owner = descriptor_symbolic_owner(*seed);
    group_key.symbolic_rule_ordinal =
        seed->has_symbolic_decision ? seed->symbolic_rule_ordinal : 0;
    group_key.orientation = member.orientation;
    group_key.occurrence_separation_required = separated;
    auto &group = groups[group_key];
    if (!checked_add(group.signed_delta, seed->numeric_crossing)) {
      error = descriptor_error(
          "Component 08 verifier descriptor delta overflowed");
      return false;
    }
    group.provenance.push_back(incidence_id);
    const auto binding = member.seed_binding.ordinal();
    if (binding >= index.crossing_by_binding.size()) {
      error = descriptor_error(
          "Component 08 verifier crossing binding is invalid");
      return false;
    }
    group.provenance.insert(group.provenance.end(),
                            index.crossing_by_binding[binding].begin(),
                            index.crossing_by_binding[binding].end());
  }

  std::uint64_t group_ordinal = 0;
  for (auto &entry : groups) {
    canonicalize_provenance(entry.second.provenance);
    descriptor_proposal proposal;
    proposal.record.key.locus = descriptor_locus;
    proposal.record.key.category = entry.first.category;
    proposal.record.key.source_feature = source_feature;
    if (occurrence != nullptr)
      proposal.record.key.occurrence = *occurrence;
    proposal.record.key.parent_lineage = contact.id.ordinal();
    proposal.record.key.boundary_ordinal = group_ordinal++;
    proposal.record.key.orientation = entry.first.orientation;
    proposal.record.signed_crossing_delta = entry.second.signed_delta;
    proposal.record.symbolic_owner = entry.first.symbolic_owner;
    proposal.record.symbolic_rule_ordinal =
        entry.first.symbolic_rule_ordinal;
    set_consumption_flags(proposal.record);
    if (entry.first.occurrence_separation_required) {
      proposal.record.occurrence_separation_required = true;
      proposal.record.continuation_allowed = false;
    }
    proposal.provenance = std::move(entry.second.provenance);
    proposals.push_back(std::move(proposal));
  }
  return true;
}

bool collect_verifier_proposals(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const aggregate_index &index,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  // The verifier starts from immutable aggregate records and maps each relevant
  // aggregate back to its topology locus. The producer does the opposite. This
  // prevents the verifier from trusting producer range iteration or dispatcher
  // coverage while retaining the frozen category table as shared schema.
  std::vector<bool> cluster_contact_seen(source_edges.clusters.size(), false);
  std::vector<bool> separated_contact_seen(interning.occurrences.size(), false);
  for (const auto &contact : aggregates.contact) {
    switch (contact.locus) {
    case intersection_aggregate_locus::source_edge_boundary: {
      if (contact.locus_ordinal >= source_edges.clusters.size()) {
        error = descriptor_error(
            "Component 08 verifier source-edge boundary aggregate is invalid");
        return false;
      }
      const auto &cluster = source_edges.clusters[contact.locus_ordinal];
      if (cluster.sequence.ordinal() >= source_edges.sequences.size()) {
        error = descriptor_error(
            "Component 08 verifier source-edge cluster sequence is invalid");
        return false;
      }
      const auto &sequence = source_edges.sequences[cluster.sequence.ordinal()];
      if (!(cluster.key.source_edge == sequence.source_edge) ||
          !(contact.source_feature == sequence.source_edge)) {
        error = descriptor_error(
            "Component 08 verifier source-edge aggregate owner mismatches");
        return false;
      }
      cluster_contact_seen[contact.locus_ordinal] = true;
      if (!append_verifier_contact_record(
              seeds, interning, incidence, aggregates, index, contact,
              intersection_descriptor_locus::source_edge_cluster_boundary,
              sequence.source_edge, false,
              cluster.separate_output_occurrences, nullptr, proposals, error))
        return false;
      break;
    }
    case intersection_aggregate_locus::event_occurrence: {
      if (contact.locus_ordinal >= interning.occurrences.size()) {
        error = descriptor_error(
            "Component 08 verifier occurrence aggregate is invalid");
        return false;
      }
      const auto &occurrence =
          interning.occurrences[contact.locus_ordinal];
      if (!occurrence.topology_separate &&
          !occurrence.requires_contact_separation)
        break;
      if (contact.source_feature.kind != relation_feature_kind::none) {
        error = descriptor_error(
            "Component 08 verifier occurrence aggregate has a source owner");
        return false;
      }
      separated_contact_seen[contact.locus_ordinal] = true;
      if (!append_verifier_contact_record(
              seeds, interning, incidence, aggregates, index, contact,
              intersection_descriptor_locus::separated_contact_occurrence,
              relation_feature_key{}, false, true, &occurrence.key, proposals,
              error))
        return false;
      break;
    }
    case intersection_aggregate_locus::conceptual_event:
    case intersection_aggregate_locus::cluster:
    case intersection_aggregate_locus::source_edge:
    case intersection_aggregate_locus::source_edge_interval:
    case intersection_aggregate_locus::carrier_cluster:
    case intersection_aggregate_locus::carrier_span:
    case intersection_aggregate_locus::overlap_boundary:
    case intersection_aggregate_locus::coplanar_component:
    case intersection_aggregate_locus::source_facet:
    case intersection_aggregate_locus::shell:
      break;
    default:
      error = descriptor_error(
          "Component 08 verifier contact aggregate locus is invalid");
      return false;
    }
  }
  if (std::find(cluster_contact_seen.begin(), cluster_contact_seen.end(),
                false) != cluster_contact_seen.end()) {
    error = descriptor_error(
        "Component 08 verifier found a source-edge cluster without contact evidence");
    return false;
  }
  for (std::size_t i = 0; i < interning.occurrences.size(); ++i) {
    const auto &occurrence = interning.occurrences[i];
    if ((occurrence.topology_separate ||
         occurrence.requires_contact_separation) &&
        !separated_contact_seen[i]) {
      error = descriptor_error(
          "Component 08 verifier found a separated occurrence without contact evidence");
      return false;
    }
  }

  // Independently scan non-contact topology in reverse canonical domain order.
  for (std::size_t reverse = source_edges.memberships.size(); reverse != 0;
       --reverse) {
    const std::size_t i = reverse - 1;
    const auto &membership = source_edges.memberships[i];
    if (membership.id.ordinal() != i ||
        !valid_relation_feature_key(membership.key.source_edge, false) ||
        membership.key.source_edge.kind != relation_feature_kind::source_edge ||
        membership.schema_version !=
            contract_versions::intersection_source_edge_membership_schema ||
        membership.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 verifier source-edge membership is malformed");
      return false;
    }
    if (!membership.internal_diagonal_discovery)
      continue;
    if (!membership.bookkeeping_only ||
        membership.event.ordinal() >= interning.events.size() ||
        membership.occurrence.ordinal() >= interning.occurrences.size() ||
        interning.occurrences[membership.occurrence.ordinal()].event !=
            membership.event) {
      error = descriptor_error(
          "Component 08 verifier internal-diagonal membership is invalid");
      return false;
    }
    const aggregate_key occurrence_locus{
        intersection_aggregate_locus::event_occurrence,
        relation_feature_key{}, membership.occurrence.ordinal()};
    const auto crossing = index.crossing.find(occurrence_locus);
    if (crossing != index.crossing.end() &&
        crossing->second->numeric_signed_sum != 0) {
      error = descriptor_error(
          "Component 08 verifier internal diagonal owns a crossing");
      return false;
    }
    append_empty_descriptor(
        intersection_descriptor_locus::transparent_internal_diagonal_adjacency,
        intersection_descriptor_category::bookkeeping_only,
        membership.key.source_edge, membership.id.ordinal(), 0, 0,
        operand_id::a, proposals);
    proposals.back().record.continuation_allowed = true;
    append_locus_provenance(occurrence_locus, aggregates, index,
                            proposals.back().provenance);
  }

  for (std::size_t reverse = source_edges.intervals.size(); reverse != 0;
       --reverse) {
    const std::size_t i = reverse - 1;
    const auto &interval = source_edges.intervals[i];
    if (interval.id.ordinal() != i ||
        interval.sequence.ordinal() >= source_edges.sequences.size() ||
        interval.schema_version !=
            contract_versions::intersection_source_edge_interval_schema ||
        interval.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 verifier source-edge interval is malformed");
      return false;
    }
    const auto &sequence = source_edges.sequences[interval.sequence.ordinal()];
    if (!(interval.key.source_edge == sequence.source_edge)) {
      error = descriptor_error(
          "Component 08 verifier source-edge interval owner mismatches");
      return false;
    }
    intersection_descriptor_category category;
    switch (interval.length_disposition) {
    case intersection_interval_length::definitely_positive:
      category = interval.duplicate_required
                     ? intersection_descriptor_category::coincident_sheet_interior
                     : intersection_descriptor_category::no_influence;
      break;
    case intersection_interval_length::exact_zero:
      category = intersection_descriptor_category::bookkeeping_only;
      break;
    case intersection_interval_length::overlap:
      category = intersection_descriptor_category::coincident_sheet_interior;
      break;
    case intersection_interval_length::uncertain:
      error = descriptor_error(
          "Component 08 verifier rejected an unresolved source-edge interval");
      return false;
    default:
      error = descriptor_error(
          "Component 08 verifier interval length class is invalid");
      return false;
    }
    append_empty_descriptor(
        intersection_descriptor_locus::source_edge_open_interval, category,
        sequence.source_edge, interval.sequence.ordinal(), interval.id.ordinal(),
        sequence.canonical_forward ? 1 : -1, operand_id::a, proposals);
    auto &record = proposals.back().record;
    record.signed_crossing_delta = interval.accumulated_crossing;
    record.continuation_allowed = interval.propagation_allowed;
    record.occurrence_separation_required = interval.duplicate_required;
    record.classification_consumable = interval.retention_allowed ||
                                       interval.accumulated_crossing != 0;
    record.selection_consumable = interval.retention_allowed;
    record.topology_consumable = true;
    append_locus_provenance(
        aggregate_key{intersection_aggregate_locus::source_edge_interval,
                      interval.key.source_edge, interval.id.ordinal()},
        aggregates, index, proposals.back().provenance);
  }

  for (std::size_t reverse = source_edges.sequences.size(); reverse != 0;
       --reverse) {
    const std::size_t i = reverse - 1;
    const auto &sequence = source_edges.sequences[i];
    if (sequence.id.ordinal() != i ||
        !valid_relation_feature_key(sequence.source_edge, false) ||
        sequence.source_edge.kind != relation_feature_kind::source_edge ||
        sequence.schema_version !=
            contract_versions::intersection_source_edge_sequence_schema ||
        sequence.reserved8 != 0 || sequence.reserved32 != 0) {
      error = descriptor_error(
          "Component 08 verifier source-edge sequence is malformed");
      return false;
    }
    if (sequence.memberships.count == 0) {
      append_empty_descriptor(
          intersection_descriptor_locus::whole_source_edge,
          intersection_descriptor_category::no_influence,
          sequence.source_edge, sequence.id.ordinal(), 0,
          sequence.canonical_forward ? 1 : -1, operand_id::a, proposals);
    }
  }

  for (std::size_t reverse = transverse.spans.size(); reverse != 0;
       --reverse) {
    const std::size_t i = reverse - 1;
    const auto &span = transverse.spans[i];
    if (span.id.ordinal() != i ||
        span.carrier.ordinal() >= transverse.carriers.size() ||
        span.left.ordinal() >= transverse.clusters.size() ||
        span.right.ordinal() >= transverse.clusters.size() ||
        transverse.clusters[span.left.ordinal()].id != span.left ||
        transverse.clusters[span.right.ordinal()].id != span.right ||
        transverse.clusters[span.left.ordinal()].carrier != span.carrier ||
        transverse.clusters[span.right.ordinal()].carrier != span.carrier ||
        transverse.carriers[span.carrier.ordinal()].id != span.carrier ||
        transverse.carriers[span.carrier.ordinal()].schema_version !=
            contract_versions::intersection_carrier_schema ||
        transverse.carriers[span.carrier.ordinal()].reserved16 != 0 ||
        transverse.carriers[span.carrier.ordinal()].reserved32 != 0 ||
        span.schema_version != contract_versions::intersection_carrier_schema ||
        span.reserved8 != 0) {
      error = descriptor_error(
          "Component 08 verifier transverse span is malformed");
      return false;
    }
    intersection_descriptor_locus locus =
        intersection_descriptor_locus::transverse_active_span;
    intersection_descriptor_category category;
    switch (span.activation) {
    case intersection_span_activation::inactive:
      locus = intersection_descriptor_locus::transverse_inactive_gap;
      category = intersection_descriptor_category::no_influence;
      break;
    case intersection_span_activation::active_transverse_intersection:
      category = intersection_descriptor_category::proper_crossing;
      break;
    case intersection_span_activation::active_overlap_boundary:
      category = intersection_descriptor_category::coplanar_overlap_boundary;
      break;
    case intersection_span_activation::active_coincident_boundary:
      category = intersection_descriptor_category::coincident_sheet_boundary;
      break;
    case intersection_span_activation::contact_only:
      category = intersection_descriptor_category::contact_delimiter;
      break;
    case intersection_span_activation::unresolved:
    case intersection_span_activation::invalid:
      error = descriptor_error(
          "Component 08 verifier rejected an unresolved transverse span");
      return false;
    default:
      error = descriptor_error(
          "Component 08 verifier transverse activation is invalid");
      return false;
    }
    append_empty_descriptor(locus, category, relation_feature_key{},
                            span.id.ordinal(),
                            static_cast<std::uint64_t>(span.activation), 1,
                            operand_id::a, proposals);
    auto &proposal = proposals.back();
    const aggregate_key aggregate_locus{
        intersection_aggregate_locus::carrier_span,
        relation_feature_key{}, span.id.ordinal()};
    const auto crossing = index.crossing.find(aggregate_locus);
    if (crossing != index.crossing.end()) {
      const auto *aggregate = crossing->second;
      if (aggregate->mixed_symbolic_ownership ||
          aggregate->numeric_signed_sum <
              std::numeric_limits<std::int32_t>::min() ||
          aggregate->numeric_signed_sum >
              std::numeric_limits<std::int32_t>::max()) {
        error = descriptor_error(
            "Component 08 verifier transverse aggregate is invalid");
        return false;
      }
      proposal.record.signed_crossing_delta =
          static_cast<std::int32_t>(aggregate->numeric_signed_sum);
      proposal.record.symbolic_owner = aggregate->symbolic_owner;
      proposal.provenance.assign(
          aggregates.crossing_members.begin() + aggregate->members.begin,
          aggregates.crossing_members.begin() + aggregate->members.begin +
              aggregate->members.count);
    } else if (span.activation != intersection_span_activation::inactive) {
      error = descriptor_error(
          "Component 08 verifier active span lacks aggregate evidence");
      return false;
    }
    append_locus_provenance(aggregate_locus, aggregates, index,
                            proposal.provenance);
    proposal.record.continuation_allowed = false;
    proposal.record.classification_consumable =
        span.classification_cut || span.contact_delimiter;
    proposal.record.selection_consumable = span.output_edge_allowed;
    proposal.record.topology_consumable = true;
  }

  for (std::size_t reverse = coplanar.overlaps.size(); reverse != 0;
       --reverse) {
    const std::size_t i = reverse - 1;
    const auto &overlap = coplanar.overlaps[i];
    if (overlap.id.ordinal() != i ||
        static_cast<std::uint8_t>(overlap.kind) < 1 ||
        static_cast<std::uint8_t>(overlap.kind) > 4 ||
        overlap.boundary_events.count == 0 ||
        !checked_range(overlap.boundary_events,
                       coplanar.overlap_boundary_event_index.size()) ||
        overlap.schema_version != contract_versions::intersection_overlap_schema ||
        overlap.reserved16 != 0) {
      error = descriptor_error(
          "Component 08 verifier coplanar overlap is malformed");
      return false;
    }
    const bool coincident =
        overlap.kind ==
            relation_coplanar_component_kind::coincident_sheet_boundary ||
        overlap.distinct_sheet_occurrences;
    const bool coincident_boundary =
        overlap.kind ==
            relation_coplanar_component_kind::coincident_sheet_boundary ||
        overlap.zero_measure;
    const auto locus = coincident
                           ? intersection_descriptor_locus::coincident_sheet
                           : intersection_descriptor_locus::coplanar_overlap;
    const auto category =
        coincident
            ? coincident_boundary
                  ? intersection_descriptor_category::coincident_sheet_boundary
                  : intersection_descriptor_category::coincident_sheet_interior
            : intersection_descriptor_category::coplanar_overlap_boundary;
    append_empty_descriptor(locus, category, relation_feature_key{},
                            overlap.id.ordinal(),
                            static_cast<std::uint64_t>(overlap.kind), 0,
                            overlap.symbolic_owner, proposals);
    auto &proposal = proposals.back();
    proposal.record.occurrence_separation_required =
        overlap.distinct_sheet_occurrences;
    proposal.record.continuation_allowed =
        category == intersection_descriptor_category::coincident_sheet_interior;
    const aggregate_key aggregate_locus{
        intersection_aggregate_locus::coplanar_component,
        relation_feature_key{}, overlap.id.ordinal()};
    const auto crossing = index.crossing.find(aggregate_locus);
    if (crossing != index.crossing.end()) {
      const auto *aggregate = crossing->second;
      if (aggregate->mixed_symbolic_ownership ||
          aggregate->numeric_signed_sum <
              std::numeric_limits<std::int32_t>::min() ||
          aggregate->numeric_signed_sum >
              std::numeric_limits<std::int32_t>::max()) {
        error = descriptor_error(
            "Component 08 verifier coplanar aggregate is invalid");
        return false;
      }
      proposal.record.signed_crossing_delta =
          static_cast<std::int32_t>(aggregate->numeric_signed_sum);
      proposal.provenance.assign(
          aggregates.crossing_members.begin() + aggregate->members.begin,
          aggregates.crossing_members.begin() + aggregate->members.begin +
              aggregate->members.count);
    }
    append_locus_provenance(aggregate_locus, aggregates, index,
                            proposal.provenance);
  }
  return true;
}

bool canonicalize_proposals(std::vector<descriptor_proposal> &proposals,
                            bounded_boolean_error &error) {
  for (auto &proposal : proposals) {
    canonicalize_provenance(proposal.provenance);
    if (!valid_intersection_descriptor_key(proposal.record.key) ||
        proposal.record.key.category ==
            intersection_descriptor_category::unresolved ||
        proposal.record.key.category ==
            intersection_descriptor_category::invalid ||
        proposal.record.reserved8 != 0 ||
        proposal.record.schema_version !=
            contract_versions::intersection_descriptor_schema) {
      error = descriptor_error(
          "Component 08 descriptor proposal is not canonical");
      return false;
    }
  }
  std::sort(proposals.begin(), proposals.end(), [](const auto &a, const auto &b) {
    if (a.record.key < b.record.key)
      return true;
    if (b.record.key < a.record.key)
      return false;
    return std::tie(a.record.signed_crossing_delta, a.record.symbolic_owner,
                    a.record.symbolic_rule_ordinal,
                    a.record.continuation_allowed,
                    a.record.occurrence_separation_required,
                    a.record.classification_consumable,
                    a.record.selection_consumable,
                    a.record.topology_consumable, a.provenance) <
           std::tie(b.record.signed_crossing_delta, b.record.symbolic_owner,
                    b.record.symbolic_rule_ordinal,
                    b.record.continuation_allowed,
                    b.record.occurrence_separation_required,
                    b.record.classification_consumable,
                    b.record.selection_consumable,
                    b.record.topology_consumable, b.provenance);
  });
  for (std::size_t i = 1; i < proposals.size(); ++i) {
    if (proposals[i - 1].record.key == proposals[i].record.key) {
      error = descriptor_error(
          "Component 08 descriptor key is not unique");
      return false;
    }
  }
  return true;
}

bool publish_proposals(const std::vector<descriptor_proposal> &proposals,
                       intersection_descriptor_tables &tables,
                       bounded_boolean_error &error) {
  tables = {};
  for (std::size_t i = 0; i < proposals.size(); ++i) {
    auto record = proposals[i].record;
    record.id = intersection_descriptor_id{i};
    record.provenance.begin = tables.provenance.size();
    record.provenance.count = proposals[i].provenance.size();
    tables.provenance.insert(tables.provenance.end(),
                             proposals[i].provenance.begin(),
                             proposals[i].provenance.end());
    if (!checked_range(record.provenance, tables.provenance.size())) {
      error = descriptor_error(
          "Component 08 descriptor provenance range overflowed");
      return false;
    }
    tables.records.push_back(std::move(record));
  }
  return true;
}

bool equal_descriptor_without_id(const intersection_descriptor_record &a,
                                 const intersection_descriptor_record &b) {
  return a.key == b.key &&
         a.signed_crossing_delta == b.signed_crossing_delta &&
         a.symbolic_owner == b.symbolic_owner &&
         a.symbolic_rule_ordinal == b.symbolic_rule_ordinal &&
         a.provenance.begin == b.provenance.begin &&
         a.provenance.count == b.provenance.count &&
         a.continuation_allowed == b.continuation_allowed &&
         a.occurrence_separation_required ==
             b.occurrence_separation_required &&
         a.classification_consumable == b.classification_consumable &&
         a.selection_consumable == b.selection_consumable &&
         a.topology_consumable == b.topology_consumable &&
         a.reserved8 == b.reserved8 &&
         a.schema_version == b.schema_version;
}

} // namespace

bool build_intersection_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    intersection_descriptor_tables &tables, bounded_boolean_error &error) {
  if (!verify_intersection_aggregates(
          seeds, interning, incidence, source_edges, transverse, coplanar,
          aggregates, error))
    return false;
  aggregate_index index;
  if (!build_aggregate_index(interning, incidence, aggregates, index, error))
    return false;
  std::vector<descriptor_proposal> proposals;
  if (!collect_producer_proposals(
          seeds, interning, incidence, source_edges, transverse, coplanar,
          aggregates, index, proposals, error) ||
      !canonicalize_proposals(proposals, error) ||
      !publish_proposals(proposals, tables, error))
    return false;
  return verify_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, coplanar,
      aggregates, tables, error);
}

bool verify_intersection_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &tables,
    bounded_boolean_error &error) {
  bounded_boolean_error aggregate_verification;
  if (!verify_intersection_aggregates(
          seeds, interning, incidence, source_edges, transverse, coplanar,
          aggregates, aggregate_verification)) {
    error = descriptor_verifier_error(
        "Component 08 descriptor verifier rejected aggregate inputs");
    return false;
  }
  aggregate_index index;
  bounded_boolean_error ignored;
  if (!build_aggregate_index(interning, incidence, aggregates, index, ignored)) {
    error = descriptor_verifier_error(
        "Component 08 descriptor verifier rejected aggregate indexing");
    return false;
  }
  std::vector<descriptor_proposal> expected;
  if (!collect_verifier_proposals(
          seeds, interning, incidence, source_edges, transverse, coplanar,
          aggregates, index, expected, ignored) ||
      !canonicalize_proposals(expected, ignored) ||
      expected.size() != tables.records.size()) {
    error = descriptor_verifier_error(
        "Component 08 descriptor verifier reconstruction failed");
    return false;
  }
  std::uint64_t next_provenance = 0;
  for (std::size_t i = 0; i < expected.size(); ++i) {
    auto expected_record = expected[i].record;
    expected_record.provenance.begin = next_provenance;
    expected_record.provenance.count = expected[i].provenance.size();
    const auto &record = tables.records[i];
    if (record.id.ordinal() != i ||
        !equal_descriptor_without_id(record, expected_record) ||
        !checked_range(record.provenance, tables.provenance.size()) ||
        !std::equal(expected[i].provenance.begin(),
                    expected[i].provenance.end(),
                    tables.provenance.begin() + record.provenance.begin)) {
      error = descriptor_verifier_error(
          "Component 08 descriptor record or provenance mismatch");
      return false;
    }
    if (record.provenance.count >
        std::numeric_limits<std::uint64_t>::max() - next_provenance) {
      error = descriptor_verifier_error(
          "Component 08 descriptor provenance count overflowed");
      return false;
    }
    next_provenance += record.provenance.count;
  }
  if (next_provenance != tables.provenance.size()) {
    error = descriptor_verifier_error(
        "Component 08 descriptor provenance has trailing records");
    return false;
  }
  return true;
}

namespace {

template <class T, class I>
const canonical_halfedge_operand<T, I> *topology_operand(
    const canonical_source_manifolds<T, I> &manifolds,
    operand_id operand) noexcept {
  if (operand == operand_id::a)
    return manifolds.a().get();
  if (operand == operand_id::b)
    return manifolds.b().get();
  return nullptr;
}

template <class T, class I>
bool validate_source_topology_predecessors(
    const canonical_source_manifolds<T, I> &manifolds,
    bounded_boolean_error &error) {
  if (manifolds.schema_version() !=
          contract_versions::canonical_source_manifolds_schema ||
      !manifolds.a() || !manifolds.b() ||
      manifolds.a()->operand() != operand_id::a ||
      manifolds.b()->operand() != operand_id::b ||
      !manifolds.owner().same_owner(manifolds.a()->owner()) ||
      !manifolds.owner().same_owner(manifolds.b()->owner()) ||
      manifolds.a()->verification() !=
          canonical_halfedge_verification_disposition::independently_verified ||
      manifolds.b()->verification() !=
          canonical_halfedge_verification_disposition::independently_verified) {
    error = descriptor_error(
        "Component 08 source-topology predecessor is malformed");
    return false;
  }
  return true;
}

bool copy_descriptor_table_to_proposals(
    const intersection_descriptor_tables &table,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  if (!std::is_sorted(table.records.begin(), table.records.end(),
                      [](const auto &a, const auto &b) {
                        return a.key < b.key;
                      })) {
    error = descriptor_error(
        "Component 08 base descriptor table is not canonically ordered");
    return false;
  }
  for (std::size_t i = 0; i < table.records.size(); ++i) {
    const auto &record = table.records[i];
    if (record.id.ordinal() != i ||
        !valid_intersection_descriptor_key(record.key) ||
        record.key.category == intersection_descriptor_category::unresolved ||
        record.key.category == intersection_descriptor_category::invalid ||
        record.schema_version !=
            contract_versions::intersection_descriptor_schema ||
        record.reserved8 != 0 ||
        !checked_range(record.provenance, table.provenance.size()) ||
        (i != 0 && table.records[i - 1].key == record.key)) {
      error = descriptor_error(
          "Component 08 base descriptor record is malformed");
      return false;
    }
    descriptor_proposal proposal;
    proposal.record = record;
    proposal.record.id = intersection_descriptor_id{0};
    proposal.record.provenance = {};
    proposal.provenance.assign(
        table.provenance.begin() + record.provenance.begin,
        table.provenance.begin() + record.provenance.begin +
            record.provenance.count);
    proposals.push_back(std::move(proposal));
  }
  return true;
}

relation_feature_key source_facet_feature(
    operand_id operand, std::uint64_t source_facet,
    std::uint64_t ring) noexcept {
  relation_feature_key feature;
  feature.operand = operand;
  feature.kind = relation_feature_kind::source_facet;
  feature.primary = source_facet;
  feature.secondary = ring;
  return feature;
}

template <class T, class I>
bool topology_facet_feature(
    const canonical_halfedge_operand<T, I> &topology,
    std::uint64_t source_facet,
    relation_feature_key &feature) noexcept {
  if (source_facet >= topology.source_facet_to_group().size())
    return false;
  const auto group_id = topology.source_facet_to_group()[source_facet];
  if (group_id >= topology.facet_groups().size())
    return false;
  const auto &group = topology.facet_groups()[group_id];
  if (group.canonical_id != group_id || group.source_facet != source_facet)
    return false;
  feature = source_facet_feature(topology.operand(), source_facet, group.ring);
  return valid_relation_feature_key(feature, false);
}

template <class T>
bool topology_edge_feature(const canonical_manifold_edge_record<T> &edge,
                           operand_id operand,
                           relation_feature_key &feature) noexcept {
  feature = relation_feature_key{};
  feature.operand = operand;
  if (edge.edge_class == canonical_edge_class::source_edge) {
    feature.kind = relation_feature_kind::source_edge;
    feature.primary = edge.key.primary;
    feature.secondary = edge.key.secondary;
  } else if (edge.edge_class ==
             canonical_edge_class::facet_internal_diagonal) {
    feature.kind = relation_feature_kind::facet_internal_diagonal;
    feature.primary = edge.source_facet;
    feature.secondary = edge.source_diagonal;
  } else {
    return false;
  }
  return valid_relation_feature_key(feature, false);
}

bool valid_crossing_shape(const relation_crossing_record &crossing) noexcept {
  return crossing.relation.ordinal() != intersection_invalid_ordinal &&
         crossing.numeric_crossing >= -1 && crossing.numeric_crossing <= 1 &&
         crossing.symbolic_crossing >= -1 &&
         crossing.symbolic_crossing <= 1 &&
         crossing.local_transition >= -1 && crossing.local_transition <= 1 &&
         (crossing.half_open_owner == operand_id::a ||
          crossing.half_open_owner == operand_id::b) &&
         crossing.source_fan_group_size != 0 &&
         crossing.source_fan_group_ordinal < crossing.source_fan_group_size &&
         crossing.numeric_owner == (crossing.numeric_crossing != 0) &&
         crossing.source_fan_resolved && crossing.locally_conservative &&
         crossing.reserved16 == 0 && crossing.reserved32 == 0;
}

bool valid_source_fan_seed(const relation_event_seed_record &seed,
                           const relation_crossing_record &crossing) noexcept {
  if (seed.id.ordinal() == intersection_invalid_ordinal ||
      !valid_relation_event_seed_key(seed.key) ||
      seed.schema_version != contract_versions::relation_event_seed_schema ||
      seed.reserved != 0 || !seed.precision_evidence_complete ||
      seed.source_relation != crossing.relation ||
      seed.key.occurrence != crossing.occurrence ||
      seed.key.family != feature_relation_family::source_edge_source_facet ||
      seed.key.first.kind != relation_feature_kind::source_edge ||
      seed.key.second.kind != relation_feature_kind::source_facet ||
      seed.contact_dimension != relation_contact_dimension::point ||
      seed.half_open_owner != crossing.half_open_owner)
    return false;
  switch (seed.contact_status) {
  case feature_relation_status::proper_crossing:
  case feature_relation_status::endpoint_crossing:
  case feature_relation_status::point_contact:
  case feature_relation_status::tangency:
    return true;
  case feature_relation_status::not_evaluated:
  case feature_relation_status::definitely_separated:
  case feature_relation_status::segment_contact:
  case feature_relation_status::overlap:
  case feature_relation_status::containment:
  case feature_relation_status::coincidence_same_orientation:
  case feature_relation_status::coincidence_opposite_orientation:
    return false;
  }
  return false;
}

struct resolved_fan_member final {
  const relation_crossing_record *crossing = nullptr;
  const relation_event_seed_record *seed = nullptr;
  event_seed_binding_id binding{intersection_invalid_ordinal};
  event_occurrence_id occurrence_id{intersection_invalid_ordinal};
  relation_feature_key source_vertex{};
  relation_feature_key source_facet{};
  intersection_occurrence_key occurrence{};
  bool topology_separate = false;
  bool requires_contact_separation = false;
  std::vector<event_incidence_id> provenance{};
};

bool validate_binding_and_incidence(
    const relation_event_seed_record &seed,
    const relation_crossing_record &crossing,
    const event_seed_binding_record &binding,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    resolved_fan_member &member,
    bounded_boolean_error &error) {
  if (binding.id.ordinal() >= interning.seed_bindings.size() ||
      interning.seed_bindings[binding.id.ordinal()].id != binding.id ||
      binding.seed != seed.id || !(binding.seed_key == seed.key) ||
      binding.relation != crossing.relation || !binding.compatibility_verified ||
      binding.event.ordinal() >= interning.events.size() ||
      binding.occurrence.ordinal() >= interning.occurrences.size() ||
      binding.schema_version !=
          contract_versions::intersection_seed_binding_schema ||
      binding.reserved8 != 0 || binding.reserved32 != 0) {
    error = descriptor_error(
        "Component 08 source-fan seed binding is malformed");
    return false;
  }
  const auto &event = interning.events[binding.event.ordinal()];
  const auto &occurrence = interning.occurrences[binding.occurrence.ordinal()];
  if (event.id != binding.event || occurrence.id != binding.occurrence ||
      occurrence.event != binding.event ||
      !valid_intersection_event_key(event.key) ||
      !valid_intersection_occurrence_key(occurrence.key) ||
      !(occurrence.key.event == event.key) ||
      event.schema_version != contract_versions::intersection_event_schema ||
      occurrence.schema_version !=
          contract_versions::intersection_occurrence_schema ||
      event.reserved16 != 0 || event.reserved32 != 0 ||
      occurrence.reserved16 != 0) {
    error = descriptor_error(
        "Component 08 source-fan event or occurrence is malformed");
    return false;
  }
  if (binding.id.ordinal() >= incidence.seed_ranges.size()) {
    error = descriptor_error(
        "Component 08 source-fan seed incidence range is absent");
    return false;
  }
  const auto range = incidence.seed_ranges[binding.id.ordinal()];
  if (!checked_range(range, incidence.by_seed.size())) {
    error = descriptor_error(
        "Component 08 source-fan seed incidence range is malformed");
    return false;
  }

  bool have_vertex = false;
  bool have_facet = false;
  bool have_precursor = false;
  relation_feature_key vertex;
  std::set<event_incidence_id> seen;
  for (std::uint64_t offset = 0; offset < range.count; ++offset) {
    const auto incidence_id = incidence.by_seed[range.begin + offset];
    if (incidence_id.ordinal() >= incidence.records.size() ||
        !seen.insert(incidence_id).second) {
      error = descriptor_error(
          "Component 08 source-fan incidence reference is invalid");
      return false;
    }
    const auto &record = incidence.records[incidence_id.ordinal()];
    if (record.id != incidence_id || record.seed_binding != binding.id ||
        record.event != binding.event || record.occurrence != binding.occurrence ||
        record.schema_version !=
            contract_versions::intersection_incidence_schema ||
        record.reserved16 != 0 || !valid_event_incidence_key(record.key) ||
        !(record.key.event == event.key) ||
        !(record.key.occurrence == occurrence.key) ||
        !(record.key.seed == seed.key) || record.key.kind != record.kind ||
        !(record.key.feature == record.feature) ||
        record.key.predecessor_relation != binding.relation.ordinal() ||
        record.key.numeric_crossing != record.numeric_crossing ||
        record.key.symbolic_crossing != record.symbolic_crossing ||
        record.key.orientation != record.orientation ||
        record.key.source_feature_owner != record.source_feature_owner ||
        record.key.bookkeeping_only != record.bookkeeping_only ||
        record.relation != binding.relation ||
        record.candidate.ordinal() != record.key.predecessor_candidate ||
        record.payload_primary != record.key.proof_primary ||
        record.payload_secondary != record.key.proof_secondary ||
        record.payload_occurrence != record.key.proof_occurrence) {
      error = descriptor_error(
          "Component 08 source-fan incidence ownership is malformed");
      return false;
    }
    if (record.kind == event_incidence_kind::source_vertex &&
        record.feature.operand == seed.key.second.operand &&
        !record.bookkeeping_only) {
      if (record.feature.kind != relation_feature_kind::source_vertex ||
          (have_vertex && !(vertex == record.feature))) {
        error = descriptor_error(
            "Component 08 source-fan names incompatible source vertices");
        return false;
      }
      vertex = record.feature;
      have_vertex = true;
    }
    if (record.kind == event_incidence_kind::source_facet &&
        !record.bookkeeping_only) {
      if (!(record.feature == seed.key.second) || have_facet) {
        error = descriptor_error(
            "Component 08 source-fan source-facet incidence is inconsistent");
        return false;
      }
      have_facet = true;
    }
    if (record.kind == event_incidence_kind::descriptor_precursor) {
      if (have_precursor ||
          record.payload_primary !=
              static_cast<std::uint64_t>(seed.contact_status) ||
          record.payload_secondary !=
              static_cast<std::uint64_t>(seed.contact_dimension)) {
        error = descriptor_error(
            "Component 08 source-fan descriptor precursor is inconsistent");
        return false;
      }
      have_precursor = true;
    }
    if (record.kind == event_incidence_kind::crossing_contribution ||
        record.kind == event_incidence_kind::descriptor_precursor ||
        record.kind == event_incidence_kind::source_vertex ||
        record.kind == event_incidence_kind::source_facet ||
        record.kind == event_incidence_kind::source_shell)
      member.provenance.push_back(incidence_id);
  }
  if (!have_facet || !have_precursor) {
    error = descriptor_error(
        "Component 08 source-fan mandatory incidence is incomplete");
    return false;
  }

  member.crossing = &crossing;
  member.seed = &seed;
  member.binding = binding.id;
  member.occurrence_id = binding.occurrence;
  member.source_vertex = have_vertex ? vertex : relation_feature_key{};
  member.source_facet = seed.key.second;
  member.occurrence = occurrence.key;
  member.topology_separate = occurrence.topology_separate;
  member.requires_contact_separation = occurrence.requires_contact_separation;
  canonicalize_provenance(member.provenance);
  return true;
}

bool resolve_crossing_binding_producer(
    const relation_crossing_record &crossing,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    resolved_fan_member &member,
    bounded_boolean_error &error) {
  if (!valid_crossing_shape(crossing)) {
    error = descriptor_error(
        "Component 08 source-fan crossing record is malformed");
    return false;
  }
  const relation_event_seed_record *seed = nullptr;
  const event_seed_binding_record *binding = nullptr;
  for (std::size_t i = 0; i < seeds.size(); ++i) {
    if (seeds[i].source_relation != crossing.relation ||
        seeds[i].key.occurrence != crossing.occurrence)
      continue;
    if (seed != nullptr || i >= interning.seed_bindings.size() ||
        seeds[i].id.ordinal() != i ||
        !valid_source_fan_seed(seeds[i], crossing)) {
      error = descriptor_error(
          "Component 08 source-fan crossing seed mapping is ambiguous");
      return false;
    }
    seed = &seeds[i];
    binding = &interning.seed_bindings[i];
  }
  if (seed == nullptr || binding == nullptr) {
    error = descriptor_error(
        "Component 08 source-fan crossing seed is absent");
    return false;
  }
  return validate_binding_and_incidence(*seed, crossing, *binding, interning,
                                        incidence, member, error);
}

template <class T, class I>
bool ordered_vertex_fan_facets(
    const canonical_source_manifolds<T, I> &manifolds,
    const relation_feature_key &source_vertex,
    std::vector<relation_feature_key> &facets,
    bounded_boolean_error &error) {
  facets.clear();
  const auto *topology = topology_operand(manifolds, source_vertex.operand);
  if (topology == nullptr ||
      source_vertex.kind != relation_feature_kind::source_vertex ||
      source_vertex.primary >= topology->source_vertex_to_vertex().size()) {
    error = descriptor_error(
        "Component 08 source-fan topology owner is invalid");
    return false;
  }
  const auto vertex_id = topology->source_vertex_to_vertex()[source_vertex.primary];
  if (vertex_id >= topology->vertices().size()) {
    error = descriptor_error(
        "Component 08 source-fan canonical vertex is invalid");
    return false;
  }
  const auto &vertex = topology->vertices()[vertex_id];
  if (vertex.canonical_id != vertex_id ||
      vertex.source_vertex != source_vertex.primary ||
      vertex.fan >= topology->fans().size()) {
    error = descriptor_error(
        "Component 08 source-fan vertex record is malformed");
    return false;
  }
  const auto &fan = topology->fans()[vertex.fan];
  if (fan.canonical_id != vertex.fan || fan.vertex != vertex_id ||
      fan.outgoing_halfedges.size() < 2) {
    error = descriptor_error(
        "Component 08 source-fan record is malformed");
    return false;
  }
  std::set<std::uint64_t> seen_halfedges;
  for (const auto halfedge_id : fan.outgoing_halfedges) {
    if (halfedge_id >= topology->halfedges().size() ||
        !seen_halfedges.insert(halfedge_id).second) {
      error = descriptor_error(
          "Component 08 source-fan halfedge is invalid");
      return false;
    }
    const auto &halfedge = topology->halfedges()[halfedge_id];
    if (halfedge.canonical_id != halfedge_id ||
        halfedge.origin != vertex_id || halfedge.edge >= topology->edges().size()) {
      error = descriptor_error(
          "Component 08 source-fan halfedge ownership is malformed");
      return false;
    }
    relation_feature_key facet;
    if (!topology_facet_feature(*topology, halfedge.source_facet, facet)) {
      error = descriptor_error(
          "Component 08 source-fan source facet is malformed");
      return false;
    }
    if (facets.empty() || !(facets.back() == facet))
      facets.push_back(facet);
  }
  if (facets.size() > 1 && facets.front() == facets.back())
    facets.pop_back();
  const std::set<relation_feature_key> unique(facets.begin(), facets.end());
  if (facets.size() < 2 || unique.size() != facets.size()) {
    error = descriptor_error(
        "Component 08 source-fan semantic sector order is malformed");
    return false;
  }
  return true;
}

bool validate_fan_group_conservation(
    const std::vector<resolved_fan_member> &members,
    bounded_boolean_error &error) {
  if (members.empty()) {
    error = descriptor_error("Component 08 source-fan group is empty");
    return false;
  }
  const auto expected_size = members.front().crossing->source_fan_group_size;
  if (members.size() != expected_size) {
    error = descriptor_error(
        "Component 08 source-fan group size is incomplete");
    return false;
  }
  std::vector<bool> ordinals(expected_size, false);
  std::int32_t numeric_sum = 0;
  std::int32_t symbolic_sum = 0;
  std::size_t numeric_owner_count = 0;
  bool has_positive_transition = false;
  bool has_negative_transition = false;
  bool any_tangent = false;
  bool any_nontangent = false;
  std::set<std::pair<std::uint64_t, std::uint32_t>> relation_occurrences;
  for (const auto &member : members) {
    const auto &crossing = *member.crossing;
    if (crossing.source_fan_group_size != expected_size ||
        crossing.source_fan_group_ordinal >= expected_size ||
        ordinals[crossing.source_fan_group_ordinal] ||
        !relation_occurrences
             .insert({crossing.relation.ordinal(), crossing.occurrence})
             .second ||
        !checked_add(numeric_sum, crossing.numeric_crossing) ||
        !checked_add(symbolic_sum, crossing.symbolic_crossing)) {
      error = descriptor_error(
          "Component 08 source-fan group ordinals or sums are malformed");
      return false;
    }
    ordinals[crossing.source_fan_group_ordinal] = true;
    numeric_owner_count += crossing.numeric_owner ? 1U : 0U;
    has_positive_transition =
        has_positive_transition || crossing.local_transition > 0;
    has_negative_transition =
        has_negative_transition || crossing.local_transition < 0;
    any_tangent = any_tangent ||
                  member.seed->contact_status == feature_relation_status::tangency;
    any_nontangent = any_nontangent ||
                     member.seed->contact_status != feature_relation_status::tangency;
  }
  if (!std::all_of(ordinals.begin(), ordinals.end(), [](bool v) { return v; }) ||
      numeric_sum < -1 || numeric_sum > 1 || symbolic_sum < -1 ||
      symbolic_sum > 1 ||
      ((numeric_sum == 0) != (numeric_owner_count == 0)) ||
      ((numeric_sum != 0) != (numeric_owner_count == 1)) ||
      (any_tangent && any_nontangent)) {
    error = descriptor_error(
        "Component 08 source-fan crossing ownership is not conservative");
    return false;
  }
  if (any_tangent && (has_positive_transition || has_negative_transition ||
                      numeric_sum != 0)) {
    error = descriptor_error(
        "Component 08 source-fan tangent group changes occupancy");
    return false;
  }
  if (numeric_sum > 0 &&
      (!has_positive_transition || has_negative_transition)) {
    error = descriptor_error(
        "Component 08 source-fan positive crossing is inconsistent");
    return false;
  }
  if (numeric_sum < 0 &&
      (!has_negative_transition || has_positive_transition)) {
    error = descriptor_error(
        "Component 08 source-fan negative crossing is inconsistent");
    return false;
  }
  if (!any_tangent && numeric_sum == 0 &&
      has_positive_transition != has_negative_transition) {
    error = descriptor_error(
        "Component 08 source-fan zero crossing is not locally conservative");
    return false;
  }
  return true;
}

bool append_fan_member_descriptor_producer(
    const resolved_fan_member &member, std::uint64_t fan_group,
    std::uint64_t sector_ordinal,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  intersection_descriptor_category category;
  const bool separated = member.topology_separate ||
                         member.requires_contact_separation ||
                         member.seed->distinct_occurrence_required;
  if (!category_from_contact(member.seed->contact_status, false, separated,
                             category)) {
    error = descriptor_error(
        "Component 08 source-fan contact class has no descriptor rule");
    return false;
  }
  if ((category == intersection_descriptor_category::proper_crossing ||
       category == intersection_descriptor_category::endpoint_crossing) &&
      member.crossing->local_transition == 0) {
    error = descriptor_error(
        "Component 08 source-fan crossing has no local transition");
    return false;
  }
  if ((category == intersection_descriptor_category::tangent ||
       category == intersection_descriptor_category::contact_delimiter ||
       category ==
           intersection_descriptor_category::topology_separated_contact) &&
      member.crossing->local_transition != 0 &&
      member.seed->contact_status != feature_relation_status::proper_crossing &&
      member.seed->contact_status != feature_relation_status::endpoint_crossing) {
    error = descriptor_error(
        "Component 08 source-fan contact changes local occupancy");
    return false;
  }
  descriptor_proposal proposal;
  proposal.record.key.locus =
      intersection_descriptor_locus::source_vertex_sector;
  proposal.record.key.category = category;
  proposal.record.key.source_feature = member.source_vertex;
  proposal.record.key.occurrence = member.occurrence;
  proposal.record.key.parent_lineage = fan_group;
  proposal.record.key.boundary_ordinal = sector_ordinal;
  proposal.record.key.orientation = member.crossing->local_transition;
  proposal.record.signed_crossing_delta = member.crossing->numeric_crossing;
  proposal.record.symbolic_owner = member.crossing->half_open_owner;
  proposal.record.symbolic_rule_ordinal =
      member.seed->has_symbolic_decision ? member.seed->symbolic_rule_ordinal : 0;
  set_consumption_flags(proposal.record);
  if (separated) {
    proposal.record.occurrence_separation_required = true;
    proposal.record.continuation_allowed = false;
  }
  proposal.provenance = member.provenance;
  proposals.push_back(std::move(proposal));
  return true;
}

template <class T, class I>
bool collect_source_vertex_sector_proposals_producer(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  if (seeds.size() != interning.seed_bindings.size() ||
      incidence.seed_ranges.size() != interning.seed_bindings.size()) {
    error = descriptor_error(
        "Component 08 source-fan predecessor counts disagree");
    return false;
  }
  std::map<std::uint64_t, std::vector<resolved_fan_member>> groups;
  for (const auto &crossing : crossings) {
    resolved_fan_member member;
    if (!resolve_crossing_binding_producer(crossing, seeds, interning,
                                           incidence, member, error))
      return false;
    if (member.source_vertex.kind == relation_feature_kind::none)
      continue;
    groups[crossing.source_fan_group].push_back(std::move(member));
  }

  for (auto &entry : groups) {
    auto &members = entry.second;
    if (!validate_fan_group_conservation(members, error))
      return false;
    const auto vertex = members.front().source_vertex;
    const auto owner = members.front().crossing->half_open_owner;
    for (const auto &member : members) {
      if (!(member.source_vertex == vertex) ||
          member.crossing->half_open_owner != owner) {
        error = descriptor_error(
            "Component 08 source-fan group mixes vertices or owners");
        return false;
      }
    }
    std::vector<relation_feature_key> ordered_facets;
    if (!ordered_vertex_fan_facets(manifolds, vertex, ordered_facets, error))
      return false;
    if (members.size() != ordered_facets.size()) {
      error = descriptor_error(
          "Component 08 source-fan contribution count differs from topology");
      return false;
    }
    std::map<relation_feature_key, const resolved_fan_member *> by_facet;
    for (const auto &member : members)
      if (!by_facet.emplace(member.source_facet, &member).second) {
        error = descriptor_error(
            "Component 08 source-fan repeats one semantic facet");
        return false;
      }
    for (std::size_t sector = 0; sector < ordered_facets.size(); ++sector) {
      const auto found = by_facet.find(ordered_facets[sector]);
      if (found == by_facet.end() ||
          !append_fan_member_descriptor_producer(
              *found->second, entry.first, sector, proposals, error)) {
        if (found == by_facet.end())
          error = descriptor_error(
              "Component 08 source-fan facets do not cover the canonical fan");
        return false;
      }
    }
  }
  return true;
}

template <class T, class I>
bool append_source_facet_adjacency_proposals_producer(
    const canonical_source_manifolds<T, I> &manifolds,
    const intersection_descriptor_tables &base,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  for (const operand_id operand : {operand_id::a, operand_id::b}) {
    const auto *topology = topology_operand(manifolds, operand);
    if (topology == nullptr) {
      error = descriptor_error(
          "Component 08 source-facet topology is absent");
      return false;
    }
    for (std::size_t i = 0; i < topology->edges().size(); ++i) {
      const auto &edge = topology->edges()[i];
      relation_feature_key edge_feature;
      if (edge.canonical_id != i || edge.key.operand != operand ||
          !topology_edge_feature(edge, operand, edge_feature)) {
        error = descriptor_error(
            "Component 08 source-facet edge record is malformed");
        return false;
      }
      relation_feature_key first_facet;
      relation_feature_key second_facet;
      if (!topology_facet_feature(*topology, edge.facets[0], first_facet) ||
          !topology_facet_feature(*topology, edge.facets[1], second_facet)) {
        error = descriptor_error(
            "Component 08 source-facet edge names an invalid facet");
        return false;
      }
      if (edge.edge_class ==
          canonical_edge_class::facet_internal_diagonal) {
        if (!(first_facet == second_facet) ||
            !canonical_edge_is_bookkeeping_only(edge)) {
          error = descriptor_error(
              "Component 08 internal diagonal has semantic contamination");
          return false;
        }
        append_empty_descriptor(
            intersection_descriptor_locus::
                transparent_internal_diagonal_adjacency,
            intersection_descriptor_category::bookkeeping_only,
            edge_feature, edge.canonical_id, 0, 0, operand, proposals);
        proposals.back().record.continuation_allowed = true;
        proposals.back().record.topology_consumable = true;
        continue;
      }
      if (first_facet == second_facet ||
          !canonical_edge_is_source_feature(edge)) {
        error = descriptor_error(
            "Component 08 source edge lacks two semantic facet owners");
        return false;
      }
      bool emitted = false;
      for (const auto &base_record : base.records) {
        if (!(base_record.key.source_feature == edge_feature) ||
            (base_record.key.locus !=
                 intersection_descriptor_locus::source_edge_cluster_boundary &&
             base_record.key.locus !=
                 intersection_descriptor_locus::source_edge_open_interval &&
             base_record.key.locus !=
                 intersection_descriptor_locus::whole_source_edge))
          continue;
        if (base_record.key.category ==
                intersection_descriptor_category::no_influence ||
            base_record.key.category ==
                intersection_descriptor_category::bookkeeping_only)
          continue;
        descriptor_proposal proposal;
        proposal.record = base_record;
        proposal.record.id = intersection_descriptor_id{0};
        proposal.record.provenance = {};
        proposal.record.key.locus =
            intersection_descriptor_locus::
                source_facet_original_edge_adjacency;
        proposal.record.key.parent_lineage = edge.canonical_id;
        proposal.record.key.boundary_ordinal = base_record.id.ordinal() + 1;
        proposal.record.topology_consumable = true;
        if (!checked_range(base_record.provenance, base.provenance.size())) {
          error = descriptor_error(
              "Component 08 source-facet base provenance is malformed");
          return false;
        }
        proposal.provenance.assign(
            base.provenance.begin() + base_record.provenance.begin,
            base.provenance.begin() + base_record.provenance.begin +
                base_record.provenance.count);
        proposals.push_back(std::move(proposal));
        emitted = true;
      }
      if (!emitted) {
        append_empty_descriptor(
            intersection_descriptor_locus::
                source_facet_original_edge_adjacency,
            intersection_descriptor_category::no_influence,
            edge_feature, edge.canonical_id, 0, 1, operand, proposals);
        proposals.back().record.continuation_allowed = true;
        proposals.back().record.topology_consumable = true;
      }
    }
  }
  return true;
}

template <class T, class I>
bool collect_topology_proposals_producer(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_descriptor_tables &base,
    std::vector<descriptor_proposal> &proposals,
    bounded_boolean_error &error) {
  return validate_source_topology_predecessors(manifolds, error) &&
         collect_source_vertex_sector_proposals_producer(
             manifolds, crossings, seeds, interning, incidence, proposals,
             error) &&
         append_source_facet_adjacency_proposals_producer(
             manifolds, base, proposals, error);
}

bool equal_descriptor_tables(const intersection_descriptor_tables &a,
                             const intersection_descriptor_tables &b) {
  if (a.records.size() != b.records.size() || a.provenance != b.provenance)
    return false;
  for (std::size_t i = 0; i < a.records.size(); ++i)
    if (a.records[i].id != b.records[i].id ||
        !equal_descriptor_without_id(a.records[i], b.records[i]))
      return false;
  return true;
}

// Verifier path. It indexes crossings first, then walks immutable seed bindings
// in reverse canonical order. This intentionally differs from the producer's
// crossing-first seed scan and does not call producer grouping or descriptor
// append routines.
using crossing_lookup_key = std::pair<std::uint64_t, std::uint32_t>;

bool resolve_crossings_verifier(
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    std::vector<resolved_fan_member> &resolved,
    bounded_boolean_error &error) {
  if (seeds.size() != interning.seed_bindings.size() ||
      incidence.seed_ranges.size() != interning.seed_bindings.size())
    return false;
  std::map<crossing_lookup_key, const relation_crossing_record *> lookup;
  for (auto it = crossings.rbegin(); it != crossings.rend(); ++it) {
    if (!valid_crossing_shape(*it) ||
        !lookup.emplace(crossing_lookup_key{it->relation.ordinal(),
                                            it->occurrence},
                        &*it)
             .second)
      return false;
  }
  std::set<crossing_lookup_key> consumed;
  for (std::size_t reverse = interning.seed_bindings.size(); reverse != 0;
       --reverse) {
    const auto index = reverse - 1;
    const auto &binding = interning.seed_bindings[index];
    if (binding.id.ordinal() != index || binding.seed.ordinal() >= seeds.size())
      return false;
    const auto &seed = seeds[binding.seed.ordinal()];
    if (seed.id != binding.seed)
      return false;
    const crossing_lookup_key key{seed.source_relation.ordinal(),
                                  seed.key.occurrence};
    const auto found = lookup.find(key);
    if (found == lookup.end())
      continue;
    if (!consumed.insert(key).second ||
        !valid_source_fan_seed(seed, *found->second))
      return false;
    resolved_fan_member member;
    if (!validate_binding_and_incidence(seed, *found->second, binding,
                                        interning, incidence, member, error))
      return false;
    resolved.push_back(std::move(member));
  }
  return consumed.size() == lookup.size();
}

template <class T, class I>
bool verifier_vertex_fan_facets(
    const canonical_halfedge_operand<T, I> &topology,
    const canonical_manifold_vertex_record<T> &vertex,
    std::vector<relation_feature_key> &facets) {
  facets.clear();
  if (vertex.fan >= topology.fans().size())
    return false;
  const auto &fan = topology.fans()[vertex.fan];
  if (fan.canonical_id != vertex.fan || fan.vertex != vertex.canonical_id ||
      fan.outgoing_halfedges.size() < 2)
    return false;
  std::vector<relation_feature_key> raw_reverse;
  std::set<std::uint64_t> seen;
  for (auto it = fan.outgoing_halfedges.rbegin();
       it != fan.outgoing_halfedges.rend(); ++it) {
    if (*it >= topology.halfedges().size() || !seen.insert(*it).second)
      return false;
    const auto &halfedge = topology.halfedges()[*it];
    if (halfedge.canonical_id != *it || halfedge.origin != vertex.canonical_id)
      return false;
    relation_feature_key facet;
    if (!topology_facet_feature(topology, halfedge.source_facet, facet))
      return false;
    raw_reverse.push_back(facet);
  }
  std::reverse(raw_reverse.begin(), raw_reverse.end());
  for (const auto &facet : raw_reverse)
    if (facets.empty() || !(facets.back() == facet))
      facets.push_back(facet);
  if (facets.size() > 1 && facets.front() == facets.back())
    facets.pop_back();
  return facets.size() >= 2 &&
         std::set<relation_feature_key>(facets.begin(), facets.end()).size() ==
             facets.size();
}

bool verifier_fan_group_conservation(
    const std::vector<const resolved_fan_member *> &members) noexcept {
  if (members.empty())
    return false;
  const std::uint32_t expected =
      members.front()->crossing->source_fan_group_size;
  if (expected == 0 || members.size() != expected)
    return false;

  std::map<std::uint32_t, const resolved_fan_member *> by_ordinal;
  std::set<std::pair<std::uint64_t, std::uint32_t>> relation_occurrences;
  std::int64_t numeric_total = 0;
  std::int64_t symbolic_total = 0;
  std::uint32_t numeric_owners = 0;
  std::uint32_t positive_transitions = 0;
  std::uint32_t negative_transitions = 0;
  std::uint32_t tangencies = 0;

  for (const auto *member : members) {
    if (member == nullptr || member->crossing == nullptr ||
        member->seed == nullptr)
      return false;
    const auto &crossing = *member->crossing;
    if (crossing.source_fan_group_size != expected ||
        crossing.source_fan_group_ordinal >= expected ||
        !by_ordinal.emplace(crossing.source_fan_group_ordinal, member).second ||
        !relation_occurrences
             .insert({crossing.relation.ordinal(), crossing.occurrence})
             .second)
      return false;
    numeric_total += static_cast<std::int64_t>(crossing.numeric_crossing);
    symbolic_total += static_cast<std::int64_t>(crossing.symbolic_crossing);
    numeric_owners += crossing.numeric_owner ? 1U : 0U;
    positive_transitions += crossing.local_transition > 0 ? 1U : 0U;
    negative_transitions += crossing.local_transition < 0 ? 1U : 0U;
    tangencies +=
        member->seed->contact_status == feature_relation_status::tangency
            ? 1U
            : 0U;
  }

  if (by_ordinal.size() != expected || numeric_total < -1 ||
      numeric_total > 1 || symbolic_total < -1 || symbolic_total > 1 ||
      numeric_owners != (numeric_total == 0 ? 0U : 1U) ||
      (tangencies != 0 && tangencies != expected))
    return false;
  if (tangencies == expected)
    return numeric_total == 0 && positive_transitions == 0 &&
           negative_transitions == 0;
  if (numeric_total > 0)
    return positive_transitions != 0 && negative_transitions == 0;
  if (numeric_total < 0)
    return negative_transitions != 0 && positive_transitions == 0;
  return positive_transitions != 0 && negative_transitions != 0;
}

bool verifier_category(const resolved_fan_member &member,
                       intersection_descriptor_category &category) {
  const bool separated = member.topology_separate ||
                         member.requires_contact_separation ||
                         member.seed->distinct_occurrence_required;
  if (separated &&
      (member.seed->contact_status == feature_relation_status::point_contact ||
       member.seed->contact_status == feature_relation_status::tangency)) {
    category = intersection_descriptor_category::topology_separated_contact;
    return true;
  }
  switch (member.seed->contact_status) {
  case feature_relation_status::proper_crossing:
    category = intersection_descriptor_category::proper_crossing;
    return member.crossing->local_transition != 0;
  case feature_relation_status::endpoint_crossing:
    category = intersection_descriptor_category::endpoint_crossing;
    return member.crossing->local_transition != 0;
  case feature_relation_status::tangency:
    category = intersection_descriptor_category::tangent;
    return member.crossing->local_transition == 0;
  case feature_relation_status::point_contact:
    category = intersection_descriptor_category::contact_delimiter;
    return member.crossing->local_transition == 0;
  case feature_relation_status::not_evaluated:
  case feature_relation_status::definitely_separated:
  case feature_relation_status::segment_contact:
  case feature_relation_status::overlap:
  case feature_relation_status::containment:
  case feature_relation_status::coincidence_same_orientation:
  case feature_relation_status::coincidence_opposite_orientation:
    return false;
  }
  return false;
}

void verifier_set_flags(intersection_descriptor_record &record) noexcept {
  record.continuation_allowed = false;
  record.occurrence_separation_required = false;
  record.classification_consumable = false;
  record.selection_consumable = false;
  record.topology_consumable = true;
  switch (record.key.category) {
  case intersection_descriptor_category::proper_crossing:
  case intersection_descriptor_category::endpoint_crossing:
    record.classification_consumable = true;
    record.selection_consumable = true;
    break;
  case intersection_descriptor_category::tangent:
  case intersection_descriptor_category::contact_delimiter:
    record.continuation_allowed = true;
    record.classification_consumable = true;
    break;
  case intersection_descriptor_category::topology_separated_contact:
    record.occurrence_separation_required = true;
    record.classification_consumable = true;
    break;
  case intersection_descriptor_category::no_influence:
  case intersection_descriptor_category::bookkeeping_only:
    record.continuation_allowed = true;
    break;
  case intersection_descriptor_category::coplanar_overlap_boundary:
  case intersection_descriptor_category::coincident_sheet_boundary:
    record.classification_consumable = true;
    record.selection_consumable = true;
    break;
  case intersection_descriptor_category::coincident_sheet_interior:
    record.continuation_allowed = true;
    record.classification_consumable = true;
    record.selection_consumable = true;
    break;
  case intersection_descriptor_category::unresolved:
  case intersection_descriptor_category::invalid:
    record.topology_consumable = false;
    break;
  }
}

bool append_fan_descriptor_verifier(
    const resolved_fan_member &member, std::uint64_t group,
    std::uint64_t sector, std::vector<descriptor_proposal> &expected) {
  intersection_descriptor_category category;
  if (!verifier_category(member, category))
    return false;
  descriptor_proposal proposal;
  proposal.record.key.locus =
      intersection_descriptor_locus::source_vertex_sector;
  proposal.record.key.category = category;
  proposal.record.key.source_feature = member.source_vertex;
  proposal.record.key.occurrence = member.occurrence;
  proposal.record.key.parent_lineage = group;
  proposal.record.key.boundary_ordinal = sector;
  proposal.record.key.orientation = member.crossing->local_transition;
  proposal.record.signed_crossing_delta = member.crossing->numeric_crossing;
  proposal.record.symbolic_owner = member.crossing->half_open_owner;
  proposal.record.symbolic_rule_ordinal =
      member.seed->has_symbolic_decision ? member.seed->symbolic_rule_ordinal : 0;
  verifier_set_flags(proposal.record);
  if (member.topology_separate || member.requires_contact_separation ||
      member.seed->distinct_occurrence_required) {
    proposal.record.occurrence_separation_required = true;
    proposal.record.continuation_allowed = false;
  }
  proposal.provenance = member.provenance;
  expected.push_back(std::move(proposal));
  return true;
}

template <class T, class I>
bool collect_source_vertex_sectors_verifier(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    std::vector<descriptor_proposal> &expected,
    bounded_boolean_error &error) {
  std::vector<resolved_fan_member> resolved;
  if (!resolve_crossings_verifier(crossings, seeds, interning, incidence,
                                  resolved, error))
    return false;
  std::set<std::pair<operand_id, std::uint64_t>> consumed_groups;
  for (const operand_id operand : {operand_id::b, operand_id::a}) {
    const auto *topology = topology_operand(manifolds, operand);
    if (!topology)
      return false;
    for (std::size_t reverse = topology->vertices().size(); reverse != 0;
         --reverse) {
      const auto &vertex_record = topology->vertices()[reverse - 1];
      if (vertex_record.canonical_id != reverse - 1)
        return false;
      relation_feature_key vertex;
      vertex.operand = operand;
      vertex.kind = relation_feature_kind::source_vertex;
      vertex.primary = vertex_record.source_vertex;
      if (!valid_relation_feature_key(vertex, false))
        return false;
      std::map<std::uint64_t, std::vector<const resolved_fan_member *>> groups;
      for (auto it = resolved.rbegin(); it != resolved.rend(); ++it)
        if (it->source_vertex == vertex)
          groups[it->crossing->source_fan_group].push_back(&*it);
      if (groups.empty())
        continue;
      std::vector<relation_feature_key> ordered_facets;
      if (!verifier_vertex_fan_facets(*topology, vertex_record,
                                      ordered_facets))
        return false;
      for (auto group_it = groups.rbegin(); group_it != groups.rend();
           ++group_it) {
        auto &members = group_it->second;
        if (!consumed_groups.insert({operand, group_it->first}).second ||
            members.size() != ordered_facets.size())
          return false;
        if (!verifier_fan_group_conservation(members)) {
          error = descriptor_error(
              "Component 08 verifier source-fan conservation failed");
          return false;
        }
        std::map<relation_feature_key, const resolved_fan_member *> by_facet;
        for (const auto *member : members)
          if (member->crossing->half_open_owner !=
                  members.front()->crossing->half_open_owner ||
              !by_facet.emplace(member->source_facet, member).second)
            return false;
        for (std::size_t sector = 0; sector < ordered_facets.size(); ++sector) {
          const auto found = by_facet.find(ordered_facets[sector]);
          if (found == by_facet.end() ||
              !append_fan_descriptor_verifier(*found->second,
                                              group_it->first, sector,
                                              expected))
            return false;
        }
      }
    }
  }
  for (const auto &member : resolved)
    if (member.source_vertex.kind != relation_feature_kind::none &&
        consumed_groups.find({member.source_vertex.operand,
                              member.crossing->source_fan_group}) ==
            consumed_groups.end())
      return false;
  return true;
}

template <class T, class I>
bool append_source_adjacency_verifier(
    const canonical_source_manifolds<T, I> &manifolds,
    const intersection_descriptor_tables &base,
    std::vector<descriptor_proposal> &expected) {
  for (const operand_id operand : {operand_id::b, operand_id::a}) {
    const auto *topology = topology_operand(manifolds, operand);
    if (!topology)
      return false;
    for (std::size_t reverse = topology->edges().size(); reverse != 0;
         --reverse) {
      const auto &edge = topology->edges()[reverse - 1];
      relation_feature_key feature;
      relation_feature_key first_facet;
      relation_feature_key second_facet;
      if (edge.canonical_id != reverse - 1 || edge.key.operand != operand ||
          !topology_edge_feature(edge, operand, feature) ||
          !topology_facet_feature(*topology, edge.facets[0], first_facet) ||
          !topology_facet_feature(*topology, edge.facets[1], second_facet))
        return false;
      if (edge.edge_class ==
          canonical_edge_class::facet_internal_diagonal) {
        if (!(first_facet == second_facet) ||
            !canonical_edge_is_bookkeeping_only(edge))
          return false;
        descriptor_proposal proposal;
        proposal.record.key.locus =
            intersection_descriptor_locus::
                transparent_internal_diagonal_adjacency;
        proposal.record.key.category =
            intersection_descriptor_category::bookkeeping_only;
        proposal.record.key.source_feature = feature;
        proposal.record.key.parent_lineage = edge.canonical_id;
        proposal.record.symbolic_owner = operand;
        verifier_set_flags(proposal.record);
        expected.push_back(std::move(proposal));
        continue;
      }
      if (first_facet == second_facet ||
          !canonical_edge_is_source_feature(edge))
        return false;
      std::size_t emitted = 0;
      for (auto it = base.records.rbegin(); it != base.records.rend(); ++it) {
        const auto &record = *it;
        if (!(record.key.source_feature == feature) ||
            (record.key.locus !=
                 intersection_descriptor_locus::source_edge_cluster_boundary &&
             record.key.locus !=
                 intersection_descriptor_locus::source_edge_open_interval &&
             record.key.locus !=
                 intersection_descriptor_locus::whole_source_edge) ||
            record.key.category ==
                intersection_descriptor_category::no_influence ||
            record.key.category ==
                intersection_descriptor_category::bookkeeping_only)
          continue;
        if (!checked_range(record.provenance, base.provenance.size()))
          return false;
        descriptor_proposal proposal;
        proposal.record = record;
        proposal.record.id = intersection_descriptor_id{0};
        proposal.record.provenance = {};
        proposal.record.key.locus =
            intersection_descriptor_locus::
                source_facet_original_edge_adjacency;
        proposal.record.key.parent_lineage = edge.canonical_id;
        proposal.record.key.boundary_ordinal = record.id.ordinal() + 1;
        proposal.record.topology_consumable = true;
        proposal.provenance.assign(
            base.provenance.begin() + record.provenance.begin,
            base.provenance.begin() + record.provenance.begin +
                record.provenance.count);
        expected.push_back(std::move(proposal));
        ++emitted;
      }
      if (emitted == 0) {
        descriptor_proposal proposal;
        proposal.record.key.locus =
            intersection_descriptor_locus::
                source_facet_original_edge_adjacency;
        proposal.record.key.category =
            intersection_descriptor_category::no_influence;
        proposal.record.key.source_feature = feature;
        proposal.record.key.parent_lineage = edge.canonical_id;
        proposal.record.key.orientation = 1;
        proposal.record.symbolic_owner = operand;
        verifier_set_flags(proposal.record);
        expected.push_back(std::move(proposal));
      }
    }
  }
  return true;
}

} // namespace

template <class T, class I>
bool extend_intersection_descriptors_with_source_topology(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_descriptor_tables &base,
    intersection_descriptor_tables &tables,
    bounded_boolean_error &error) {
  std::vector<descriptor_proposal> proposals;
  if (!copy_descriptor_table_to_proposals(base, proposals, error) ||
      !collect_topology_proposals_producer(manifolds, crossings, seeds,
                                           interning, incidence, base,
                                           proposals, error) ||
      !canonicalize_proposals(proposals, error) ||
      !publish_proposals(proposals, tables, error))
    return false;
  return verify_intersection_source_topology_descriptors(
      manifolds, crossings, seeds, interning, incidence, base, tables, error);
}

template <class T, class I>
bool verify_intersection_source_topology_descriptors(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_descriptor_tables &base,
    const intersection_descriptor_tables &tables,
    bounded_boolean_error &error) {
  bounded_boolean_error ignored;
  if (!validate_source_topology_predecessors(manifolds, ignored)) {
    error = descriptor_verifier_error(
        "Component 08 source-topology verifier rejected predecessors");
    return false;
  }
  std::vector<descriptor_proposal> expected;
  if (!copy_descriptor_table_to_proposals(base, expected, ignored) ||
      !collect_source_vertex_sectors_verifier(
          manifolds, crossings, seeds, interning, incidence, expected,
          ignored) ||
      !append_source_adjacency_verifier(manifolds, base, expected) ||
      !canonicalize_proposals(expected, ignored)) {
    error = descriptor_verifier_error(
        "Component 08 source-topology descriptor reconstruction failed");
    return false;
  }
  intersection_descriptor_tables reconstructed;
  if (!publish_proposals(expected, reconstructed, ignored) ||
      !equal_descriptor_tables(reconstructed, tables)) {
    error = descriptor_verifier_error(
        "Component 08 source-topology descriptor table mismatch");
    return false;
  }
  return true;
}

#define YGOR_INSTANTIATE_SOURCE_TOPOLOGY(T, I)                                \
  template bool extend_intersection_descriptors_with_source_topology<T, I>(   \
      const canonical_source_manifolds<T, I> &,                               \
      const std::vector<relation_crossing_record> &,                          \
      const std::vector<relation_event_seed_record> &,                        \
      const event_interning_tables &, const event_incidence_tables &,          \
      const intersection_descriptor_tables &, intersection_descriptor_tables &,\
      bounded_boolean_error &);                                                \
  template bool verify_intersection_source_topology_descriptors<T, I>(        \
      const canonical_source_manifolds<T, I> &,                               \
      const std::vector<relation_crossing_record> &,                          \
      const std::vector<relation_event_seed_record> &,                        \
      const event_interning_tables &, const event_incidence_tables &,          \
      const intersection_descriptor_tables &,                                \
      const intersection_descriptor_tables &, bounded_boolean_error &)

YGOR_INSTANTIATE_SOURCE_TOPOLOGY(float, std::uint32_t);
YGOR_INSTANTIATE_SOURCE_TOPOLOGY(float, std::uint64_t);
YGOR_INSTANTIATE_SOURCE_TOPOLOGY(double, std::uint32_t);
YGOR_INSTANTIATE_SOURCE_TOPOLOGY(double, std::uint64_t);

#undef YGOR_INSTANTIATE_SOURCE_TOPOLOGY

} // namespace ygor::mesh_boolean::bounded
