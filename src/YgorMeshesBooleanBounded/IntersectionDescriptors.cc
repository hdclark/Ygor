#include "IntersectionDescriptors.h"

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

} // namespace ygor::mesh_boolean::bounded
