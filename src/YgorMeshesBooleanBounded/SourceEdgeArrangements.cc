#include "SourceEdgeArrangements.h"

#include "BoundedCarrierOrdering.h"
#include "FloatingBits.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <tuple>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error arrangement_error(intersection_subcode subcode,
                                         const char *summary,
                                         intersection_checkpoint checkpoint) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::input_contract_error,
                            summary, checkpoint);
}

bool checked_range(std::uint64_t begin, std::uint64_t count,
                   std::size_t size) noexcept {
  return begin <= size && count <= size - static_cast<std::size_t>(begin);
}

relation_interval_evidence_kind parameter_kind_for(
    const relation_event_seed_record &seed,
    const relation_feature_key &edge) noexcept {
  if (seed.key.family == feature_relation_family::source_edge_source_edge) {
    if (edge == seed.key.first)
      return relation_interval_evidence_kind::source_edge_first_parameter;
    if (edge == seed.key.second)
      return relation_interval_evidence_kind::source_edge_second_parameter;
  }
  if (seed.key.family == feature_relation_family::source_edge_source_facet &&
      edge == seed.key.first)
    return relation_interval_evidence_kind::edge_facet_event_parameter;
  return relation_interval_evidence_kind::facet_facet_direction_squared;
}

source_facet_use_role facet_role_for(
    const relation_event_seed_record &seed) noexcept {
  const bool first = seed.key.first.kind == relation_feature_kind::source_facet;
  const bool second =
      seed.key.second.kind == relation_feature_kind::source_facet;
  if (first && second)
    return source_facet_use_role::both_incident;
  if (first)
    return source_facet_use_role::left_incident;
  if (second)
    return source_facet_use_role::right_incident;
  return source_facet_use_role::none;
}

std::uint64_t parameter_lineage_for(
    const relation_interval_evidence_record &parameter) noexcept {
  if (parameter.trace_root != 0)
    return parameter.trace_root;
  if (parameter.id.ordinal() == std::numeric_limits<std::uint64_t>::max())
    return 0;
  // Component 07 permits a zero arithmetic trace root when no separately
  // allocated trace node was required.  The canonical interval-evidence ID is
  // still immutable proof lineage, so use its one-based identity for ordering
  // certificates instead of rejecting otherwise complete evidence.
  return parameter.id.ordinal() + 1;
}

bool direct_seed_incidence_range(
    const event_incidence_tables &incidence, const intersection_range &indexed,
    event_seed_binding_id seed, intersection_range &direct) noexcept {
  direct = {};
  if (!checked_range(indexed.begin, indexed.count, incidence.by_seed.size()))
    return false;
  if (indexed.count == 0)
    return true;
  const auto first = incidence.by_seed[indexed.begin].ordinal();
  if (!checked_range(first, indexed.count, incidence.records.size()))
    return false;
  for (std::uint64_t offset = 0; offset < indexed.count; ++offset) {
    const auto id = incidence.by_seed[indexed.begin + offset];
    if (id.ordinal() != first + offset ||
        incidence.records[id.ordinal()].seed_binding != seed)
      return false;
  }
  direct = intersection_range{first, indexed.count};
  return true;
}

bool direct_incident_facet_range(
    const event_incidence_tables &incidence, const intersection_range &indexed,
    intersection_range &direct) noexcept {
  direct = {};
  bool found = false;
  std::uint64_t first = 0;
  std::uint64_t count = 0;
  for (std::uint64_t offset = 0; offset < indexed.count; ++offset) {
    const auto id = incidence.by_seed[indexed.begin + offset];
    if (id.ordinal() >= incidence.records.size())
      return false;
    const auto &record = incidence.records[id.ordinal()];
    const bool incident_facet =
        record.kind == event_incidence_kind::source_facet &&
        record.source_feature_owner && !record.bookkeeping_only &&
        record.candidate.ordinal() == intersection_invalid_ordinal;
    if (!incident_facet)
      continue;
    if (!found) {
      found = true;
      first = id.ordinal();
    } else if (id.ordinal() != first + count) {
      return false;
    }
    ++count;
  }
  if (found)
    direct = intersection_range{first, count};
  return true;
}

template <class T>
bool decode_parameter(const source_edge_membership_proposal &proposal,
                      finite_interval<T> &value) noexcept {
  if (proposal.parameter.ordinal() == intersection_invalid_ordinal ||
      proposal.domain == parameter_domain_status::outside ||
      proposal.domain == parameter_domain_status::invalid)
    return false;
  auto lower = from_bits<T>(
      static_cast<floating_uint_t<T>>(proposal.lower_bits));
  auto upper = from_bits<T>(
      static_cast<floating_uint_t<T>>(proposal.upper_bits));
  // Exact endpoint evidence certifies the nominal parameter as exactly zero or
  // one; the bounded enclosure may overshoot the unit domain by a rounding
  // margin from the division that produced it. Clamp those certified endpoints
  // back into the source-edge domain rather than rejecting otherwise
  // authoritative evidence.
  if (proposal.exact_zero == exact_relation_status::exact_zero &&
      finite_numeric_less(lower, T(0)))
    lower = T(0);
  if (proposal.exact_one == exact_relation_status::exact_zero &&
      finite_numeric_less(T(1), upper))
    upper = T(1);
  const auto interval = finite_interval<T>::create(lower, upper);
  if (!interval || finite_numeric_less(lower, T(0)) ||
      finite_numeric_less(T(1), upper))
    return false;
  value = *interval;
  return true;
}

bool equal_range(intersection_range a, intersection_range b) noexcept {
  return a.begin == b.begin && a.count == b.count;
}

bool equal_tables(const source_edge_arrangement_tables &a,
                  const source_edge_arrangement_tables &b) noexcept {
  if (a.memberships.size() != b.memberships.size() ||
      a.membership_sequence_index != b.membership_sequence_index ||
      a.sequences.size() != b.sequences.size() ||
      a.clusters.size() != b.clusters.size() ||
      a.cluster_occurrence_index != b.cluster_occurrence_index ||
      a.cluster_membership_index != b.cluster_membership_index ||
      a.sequence_cluster_index != b.sequence_cluster_index ||
      a.intervals.size() != b.intervals.size() ||
      a.sequence_interval_index != b.sequence_interval_index ||
      a.ordering_certificates.size() != b.ordering_certificates.size())
    return false;
  for (std::size_t i = 0; i < a.memberships.size(); ++i) {
    const auto &x = a.memberships[i];
    const auto &y = b.memberships[i];
    if (x.id != y.id || !(x.key == y.key) || x.occurrence != y.occurrence ||
        x.event != y.event || x.parameter != y.parameter ||
        !equal_range(x.contributions, y.contributions) ||
        !equal_range(x.incident_facet_uses, y.incident_facet_uses) ||
        x.ordering_certificate != y.ordering_certificate ||
        x.exact_equal_eligible != y.exact_equal_eligible ||
        x.cluster_eligible != y.cluster_eligible ||
        x.internal_diagonal_discovery != y.internal_diagonal_discovery ||
        x.bookkeeping_only != y.bookkeeping_only)
      return false;
  }
  for (std::size_t i = 0; i < a.sequences.size(); ++i) {
    const auto &x = a.sequences[i];
    const auto &y = b.sequences[i];
    if (x.id != y.id || x.source_edge != y.source_edge ||
        x.start.source_vertex != y.start.source_vertex ||
        x.end.source_vertex != y.end.source_vertex ||
        !equal_range(x.clusters, y.clusters) ||
        !equal_range(x.memberships, y.memberships) ||
        !equal_range(x.intervals, y.intervals) ||
        x.canonical_forward != y.canonical_forward ||
        x.comparison_count != y.comparison_count)
      return false;
  }
  for (std::size_t i = 0; i < a.clusters.size(); ++i) {
    const auto &x = a.clusters[i];
    const auto &y = b.clusters[i];
    if (x.id != y.id || x.sequence != y.sequence || !(x.key == y.key) ||
        !equal_range(x.member_occurrences, y.member_occurrences) ||
        !equal_range(x.membership_ids, y.membership_ids) ||
        x.predecessor != y.predecessor || x.successor != y.successor ||
        x.ordering_certificate != y.ordering_certificate ||
        x.shared_output_coordinate != y.shared_output_coordinate ||
        x.separate_output_occurrences != y.separate_output_occurrences)
      return false;
  }
  for (std::size_t i = 0; i < a.intervals.size(); ++i) {
    const auto &x = a.intervals[i];
    const auto &y = b.intervals[i];
    if (x.id != y.id || x.sequence != y.sequence || !(x.key == y.key) ||
        x.left_parameter != y.left_parameter ||
        x.right_parameter != y.right_parameter ||
        x.length_disposition != y.length_disposition ||
        x.propagation_allowed != y.propagation_allowed ||
        x.retention_allowed != y.retention_allowed ||
        x.split_required != y.split_required ||
        x.duplicate_required != y.duplicate_required)
      return false;
  }
  for (std::size_t i = 0; i < a.ordering_certificates.size(); ++i) {
    const auto &x = a.ordering_certificates[i];
    const auto &y = b.ordering_certificates[i];
    if (x.id != y.id || x.disposition != y.disposition ||
        x.first_parameter != y.first_parameter ||
        x.second_parameter != y.second_parameter ||
        x.exact_evidence_lineage != y.exact_evidence_lineage ||
        x.comparison_evidence_lineage != y.comparison_evidence_lineage ||
        x.topology_safe != y.topology_safe)
      return false;
  }
  return true;
}

} // namespace

bool collect_source_edge_membership_proposals(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_construction_record> &constructions,
    const std::vector<relation_construction_ledger_record> &construction_ledger,
    const std::vector<relation_interval_evidence_record> &interval_evidence,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    std::vector<source_edge_membership_proposal> &proposals,
    bounded_boolean_error &error) {
  proposals.clear();
  if (seeds.size() != interning.seed_bindings.size() ||
      incidence.seed_ranges.size() != seeds.size()) {
    error = arrangement_error(intersection_subcode::membership_incomplete,
                              "Component 08 source-edge seed map is incomplete",
                              intersection_checkpoint::source_edge_membership_proposals);
    return false;
  }
  for (std::size_t i = 0; i < seeds.size(); ++i) {
    const auto &seed = seeds[i];
    const auto &binding = interning.seed_bindings[i];
    if (seed.id.ordinal() != i || binding.seed != seed.id ||
        seed.construction.ordinal() >= constructions.size() ||
        !checked_range(incidence.seed_ranges[i].begin,
                       incidence.seed_ranges[i].count,
                       incidence.by_seed.size())) {
      error = arrangement_error(intersection_subcode::membership_incomplete,
                                "Component 08 source-edge incidence range is malformed",
                                intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
    const auto range = incidence.seed_ranges[i];
    const auto &construction = constructions[seed.construction.ordinal()];
    if (construction.id != seed.construction ||
        !checked_range(construction.interval_evidence_begin,
                       construction.interval_evidence_count,
                       interval_evidence.size()) ||
        !checked_range(seed.construction_ledger_begin,
                       seed.construction_ledger_count,
                       construction_ledger.size())) {
      error = arrangement_error(
          intersection_subcode::membership_incomplete,
          "Component 08 source-edge construction authority is malformed",
          intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
    intersection_range contribution_range;
    intersection_range incident_facet_range;
    if (!direct_seed_incidence_range(incidence, range, binding.id,
                                     contribution_range) ||
        !direct_incident_facet_range(incidence, range,
                                     incident_facet_range)) {
      error = arrangement_error(
          intersection_subcode::membership_incomplete,
          "Component 08 source-edge incidence is not canonically contiguous",
          intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
    for (std::uint64_t offset = 0; offset < range.count; ++offset) {
      const auto id = incidence.by_seed[range.begin + offset];
      if (id.ordinal() >= incidence.records.size()) {
        error = arrangement_error(intersection_subcode::membership_incomplete,
                                  "Component 08 source-edge incidence ID is invalid",
                                  intersection_checkpoint::source_edge_membership_proposals);
        return false;
      }
      const auto &record = incidence.records[id.ordinal()];
      if (record.kind != event_incidence_kind::source_edge ||
          record.bookkeeping_only || !record.source_feature_owner ||
          record.candidate.ordinal() != intersection_invalid_ordinal)
        continue;
      const auto kind = parameter_kind_for(seed, record.feature);
      if (kind == relation_interval_evidence_kind::facet_facet_direction_squared) {
        error = arrangement_error(intersection_subcode::membership_incomplete,
                                  "Component 08 source-edge has no parameter family",
                                  intersection_checkpoint::source_edge_membership_proposals);
        return false;
      }
      // The authoritative bounded parameter is owned by the seed's own
      // Component 07 relation and is scoped to the construction's authority.
      // A collinear-overlap endpoint or an accepted source-vertex construction
      // is shared by several relations; the construction record's own
      // interval-evidence range names only the canonical producer relation,
      // while consumer relations publish their endpoint evidence in separate
      // construction-ledger entries. The canonical seed therefore reads the
      // construction's own range and a consumer seed reads its ledger entry,
      // so each seed selects its own relation's parameter without admitting
      // unrelated global evidence or a second copy of the authority.
      const relation_interval_evidence_record *parameter = nullptr;
      const auto scan_range = [&](std::uint64_t begin,
                                  std::uint64_t count) -> bool {
        if (!checked_range(begin, count, interval_evidence.size())) {
          error = arrangement_error(
              intersection_subcode::parameter_invalid,
              "Component 08 source-edge evidence range is malformed",
              intersection_checkpoint::source_edge_membership_proposals);
          return false;
        }
        for (std::uint64_t evidence_ordinal = begin;
             evidence_ordinal < begin + count; ++evidence_ordinal) {
          const auto &candidate = interval_evidence[evidence_ordinal];
          if (candidate.id.ordinal() != evidence_ordinal) {
            error = arrangement_error(
                intersection_subcode::parameter_invalid,
                "Component 08 source-edge parameter ID is not canonical",
                intersection_checkpoint::source_edge_membership_proposals);
            return false;
          }
          if (candidate.source_relation == seed.source_relation &&
              candidate.kind == kind &&
              candidate.occurrence == seed.key.occurrence) {
            if (parameter != nullptr && parameter != &candidate) {
              error = arrangement_error(
                  intersection_subcode::parameter_invalid,
                  "Component 08 source-edge parameter is ambiguous",
                  intersection_checkpoint::source_edge_membership_proposals);
              return false;
            }
            parameter = &candidate;
          }
        }
        return true;
      };
      if (construction.source_relation == seed.source_relation) {
        if (!scan_range(construction.interval_evidence_begin,
                        construction.interval_evidence_count))
          return false;
      } else {
        for (std::uint64_t ledger_offset = 0;
             ledger_offset < seed.construction_ledger_count; ++ledger_offset) {
          const auto &entry = construction_ledger[seed.construction_ledger_begin +
                                                  ledger_offset];
          if (entry.source_relation != seed.source_relation)
            continue;
          if (!scan_range(entry.interval_evidence_begin,
                          entry.interval_evidence_count))
            return false;
        }
      }
      const auto parameter_lineage =
          parameter != nullptr ? parameter_lineage_for(*parameter) : 0;
      if (parameter == nullptr || !parameter->has_rounded_nominal ||
          !parameter->has_parameter_metadata ||
          !parameter->within_authorized_boundary ||
          parameter->domain == parameter_domain_status::invalid ||
          parameter->domain == parameter_domain_status::outside ||
          parameter_lineage == 0) {
        error = arrangement_error(intersection_subcode::parameter_invalid,
                                  "Component 08 source-edge parameter is missing",
                                  intersection_checkpoint::source_edge_membership_proposals);
        return false;
      }
      source_edge_membership_proposal proposal;
      proposal.key.source_edge = record.feature;
      if (binding.occurrence.ordinal() >= interning.occurrences.size()) {
        error = arrangement_error(intersection_subcode::membership_incomplete,
                                  "Component 08 source-edge occurrence is invalid",
                                  intersection_checkpoint::source_edge_membership_proposals);
        return false;
      }
      proposal.key.occurrence =
          interning.occurrences[binding.occurrence.ordinal()].key;
      proposal.key.role =
          parameter->exact_zero == exact_relation_status::exact_zero ||
                  parameter->exact_one == exact_relation_status::exact_zero
              ? intersection_membership_role::endpoint
              : intersection_membership_role::interior;
      proposal.key.parameter_evidence = parameter->id;
      proposal.key.parameter_lineage = parameter_lineage;
      proposal.key.relation_lineage = seed.source_relation.ordinal() + 1;
      proposal.key.facet_use_role = facet_role_for(seed);
      proposal.occurrence = binding.occurrence;
      proposal.event = binding.event;
      proposal.parameter = parameter->id;
      proposal.nominal_bits = parameter->rounded_nominal_bits;
      proposal.lower_bits = parameter->lower_bits;
      proposal.upper_bits = parameter->upper_bits;
      proposal.domain = parameter->domain;
      proposal.exact_zero = parameter->exact_zero;
      proposal.exact_one = parameter->exact_one;
      proposal.contributions = contribution_range;
      proposal.incident_facet_uses = incident_facet_range;
      proposal.exact_equal_eligible =
          parameter->exact_zero == exact_relation_status::exact_zero ||
          parameter->exact_one == exact_relation_status::exact_zero;
      proposal.cluster_eligible = proposal.exact_equal_eligible;
      proposals.push_back(std::move(proposal));
    }
  }
  std::sort(proposals.begin(), proposals.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  for (std::size_t i = 0; i < proposals.size(); ++i) {
    if (!valid_source_edge_membership_key(proposals[i].key) ||
        (i != 0 && proposals[i - 1].key == proposals[i].key)) {
      error = arrangement_error(intersection_subcode::membership_incomplete,
                                "Component 08 source-edge membership key is invalid",
                                intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
  }
  return true;
}

template <class T>
bool build_source_edge_arrangements(
    const std::vector<source_edge_domain_record> &domains,
    const std::vector<source_edge_membership_proposal> &input,
    source_edge_arrangement_tables &tables,
    bounded_boolean_error &error) {
  tables = source_edge_arrangement_tables{};
  auto sorted_domains = domains;
  std::sort(sorted_domains.begin(), sorted_domains.end(),
            [](const auto &a, const auto &b) {
              return a.source_edge < b.source_edge;
            });
  for (std::size_t d = 0; d < sorted_domains.size(); ++d) {
    const auto &domain = sorted_domains[d];
    if (!valid_relation_feature_key(domain.source_edge) ||
        domain.source_edge.kind != relation_feature_kind::source_edge ||
        !valid_relation_feature_key(domain.start_vertex) ||
        !valid_relation_feature_key(domain.end_vertex) ||
        domain.start_vertex.kind != relation_feature_kind::source_vertex ||
        domain.end_vertex.kind != relation_feature_kind::source_vertex ||
        domain.start_vertex.operand != domain.source_edge.operand ||
        domain.end_vertex.operand != domain.source_edge.operand ||
        (d != 0 && sorted_domains[d - 1].source_edge == domain.source_edge)) {
      error = arrangement_error(intersection_subcode::source_edge_sequence_invalid,
                                "Component 08 source-edge domain is invalid",
                                intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
  }

  std::vector<source_edge_membership_proposal> proposals = input;
  std::sort(proposals.begin(), proposals.end(), [](const auto &a, const auto &b) {
    return a.key < b.key;
  });
  for (std::size_t i = 0; i < proposals.size(); ++i) {
    finite_interval<T> decoded;
    if (!valid_source_edge_membership_key(proposals[i].key) ||
        proposals[i].parameter != proposals[i].key.parameter_evidence ||
        !decode_parameter(proposals[i], decoded) ||
        (i != 0 && proposals[i - 1].key == proposals[i].key)) {
      error = arrangement_error(intersection_subcode::parameter_invalid,
                                "Component 08 source-edge proposal is invalid",
                                intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
  }

  for (const auto &domain : sorted_domains) {
    const source_edge_sequence_id sequence_id{tables.sequences.size()};
    source_edge_sequence_record sequence;
    sequence.id = sequence_id;
    sequence.source_edge = domain.source_edge;
    sequence.start.side = source_edge_sentinel_side::start;
    sequence.start.source_vertex = domain.start_vertex;
    sequence.start.source_edge = domain.source_edge;
    sequence.end.side = source_edge_sentinel_side::end;
    sequence.end.source_vertex = domain.end_vertex;
    sequence.end.source_edge = domain.source_edge;
    sequence.canonical_forward = true;

    std::vector<std::size_t> local;
    for (std::size_t i = 0; i < proposals.size(); ++i)
      if (proposals[i].key.source_edge == domain.source_edge)
        local.push_back(i);

    std::vector<bounded_ordering_member> ordering_members;
    ordering_members.reserve(local.size());
    for (std::size_t i = 0; i < local.size(); ++i) {
      const auto &proposal = proposals[local[i]];
      bounded_ordering_member member;
      member.input_ordinal = i;
      member.parameter = proposal.parameter;
      member.occurrence = proposal.key.occurrence;
      member.nominal_bits = proposal.nominal_bits;
      member.lower_bits = proposal.lower_bits;
      member.upper_bits = proposal.upper_bits;
      member.exact_evidence_lineage =
          proposal.exact_equal_eligible ? proposal.key.parameter_lineage : 0;
      member.comparison_evidence_lineage = proposal.key.parameter_lineage;
      member.cluster_lineage = proposal.key.parameter_lineage;
      member.exact_equal_eligible = proposal.exact_equal_eligible;
      member.unresolved_cluster_eligible = proposal.cluster_eligible;
      member.topology_interchangeable = proposal.cluster_eligible;
      member.exact_endpoint =
          proposal.exact_zero == exact_relation_status::exact_zero
              ? static_cast<std::uint8_t>(1)
              : proposal.exact_one == exact_relation_status::exact_zero
                    ? static_cast<std::uint8_t>(2)
                    : static_cast<std::uint8_t>(0);
      ordering_members.push_back(member);
    }

    bounded_ordering_result ordering;
    if (!build_bounded_carrier_order<T>(
            ordering_members, intersection_checkpoint::source_edge_ordering,
            ordering, error) ||
        !verify_bounded_carrier_order<T>(
            ordering_members, intersection_checkpoint::source_edge_ordering,
            ordering, error))
      return false;
    sequence.comparison_count = ordering.comparison_count;

    // Resolve pair certificates by the ordering's local certificate ordinal.
    // Only certificates referenced by a published cluster or membership are
    // retained; the remaining all-pairs clique certificates are ordering
    // verification evidence that must not become orphaned artifact records.
    const auto pair_certificate_local =
        [&](std::size_t first, std::size_t second,
            intersection_order_disposition required) {
          std::uint64_t best = intersection_invalid_ordinal;
          for (const auto &pair : ordering.pair_certificates) {
            intersection_order_disposition disposition =
                intersection_order_disposition::invalid;
            if (pair.first_input_ordinal == first &&
                pair.second_input_ordinal == second) {
              disposition = ordering.certificates[pair.certificate.ordinal()]
                                .disposition;
            } else if (pair.first_input_ordinal == second &&
                       pair.second_input_ordinal == first) {
              const auto stored =
                  ordering.certificates[pair.certificate.ordinal()].disposition;
              disposition =
                  stored == intersection_order_disposition::definitely_before
                      ? intersection_order_disposition::definitely_after
                      : stored ==
                                intersection_order_disposition::definitely_after
                            ? intersection_order_disposition::definitely_before
                            : stored;
            } else {
              continue;
            }
            if (disposition == required) {
              const auto candidate = pair.certificate.ordinal();
              if (best == intersection_invalid_ordinal || candidate < best)
                best = candidate;
            }
          }
          return best;
        };

    std::vector<std::vector<std::size_t>> groups;
    groups.reserve(ordering.clusters.size());
    for (const auto &ordered_cluster : ordering.clusters) {
      std::vector<std::size_t> group;
      group.reserve(ordered_cluster.members.count);
      for (std::uint64_t offset = 0; offset < ordered_cluster.members.count;
           ++offset)
        group.push_back(static_cast<std::size_t>(
            ordering.ordered_member_ordinals[ordered_cluster.members.begin +
                                             offset]));
      groups.push_back(std::move(group));
    }

    sequence.clusters.begin = tables.sequence_cluster_index.size();
    sequence.memberships.begin = tables.membership_sequence_index.size();
    std::vector<source_edge_cluster_id> ordered_cluster_ids;
    std::vector<std::size_t> ordered_group_indices;
    std::vector<std::uint64_t> referenced_local;
    for (std::size_t group_index = 0; group_index < groups.size();
         ++group_index) {
      const auto &group = groups[group_index];
      source_edge_cluster_record cluster;
      cluster.id = source_edge_cluster_id{tables.clusters.size()};
      cluster.sequence = sequence_id;
      cluster.key.source_edge = domain.source_edge;
      cluster.key.equivalence = ordering.clusters[group_index].equivalence;
      for (const auto member : group) {
        const auto &proposal = proposals[local[member]];
        cluster.key.members.push_back(proposal.key.occurrence);
      }
      std::sort(cluster.key.members.begin(), cluster.key.members.end());
      cluster.key.members.erase(
          std::unique(cluster.key.members.begin(), cluster.key.members.end()),
          cluster.key.members.end());
      if (!valid_source_edge_cluster_key(cluster.key)) {
        error = arrangement_error(intersection_subcode::cluster_invalid,
                                  "Component 08 source-edge cluster key is invalid",
                                  intersection_checkpoint::source_edge_ordering);
        return false;
      }
      cluster.member_occurrences.begin = tables.cluster_occurrence_index.size();
      for (const auto &occurrence_key : cluster.key.members) {
        auto found = std::find_if(group.begin(), group.end(), [&](std::size_t m) {
          return proposals[local[m]].key.occurrence == occurrence_key;
        });
        if (found == group.end()) {
          error = arrangement_error(intersection_subcode::cluster_invalid,
                                    "Component 08 source-edge occurrence is missing",
                                    intersection_checkpoint::source_edge_ordering);
          return false;
        }
        tables.cluster_occurrence_index.push_back(
            proposals[local[*found]].occurrence);
      }
      cluster.member_occurrences.count =
          tables.cluster_occurrence_index.size() -
          cluster.member_occurrences.begin;
      const auto equivalence_local =
          group.size() > 1
              ? pair_certificate_local(
                    group[0], group[1],
                    ordering.clusters[group_index].equivalence ==
                            intersection_cluster_equivalence::
                                lineage_authorized_unresolved
                        ? intersection_order_disposition::unresolved_overlap
                        : intersection_order_disposition::exact_equal)
              : intersection_invalid_ordinal;
      cluster.ordering_certificate = ordering_certificate_id{equivalence_local};
      if (equivalence_local != intersection_invalid_ordinal)
        referenced_local.push_back(equivalence_local);
      cluster.membership_ids.begin = tables.cluster_membership_index.size();
      for (const auto member : group) {
        const auto &proposal = proposals[local[member]];
        source_edge_membership_record record;
        record.id = source_edge_membership_id{tables.memberships.size()};
        record.key = proposal.key;
        record.occurrence = proposal.occurrence;
        record.event = proposal.event;
        record.parameter = proposal.parameter;
        record.contributions = proposal.contributions;
        record.incident_facet_uses = proposal.incident_facet_uses;
        record.exact_equal_eligible = proposal.exact_equal_eligible;
        record.cluster_eligible = proposal.cluster_eligible;
        record.internal_diagonal_discovery =
            proposal.internal_diagonal_discovery;
        record.bookkeeping_only = proposal.bookkeeping_only;
        record.ordering_certificate = ordering_certificate_id{equivalence_local};
        tables.memberships.push_back(record);
        tables.membership_sequence_index.push_back(record.id);
        tables.cluster_membership_index.push_back(record.id);
      }
      cluster.membership_ids.count =
          tables.cluster_membership_index.size() - cluster.membership_ids.begin;
      cluster.shared_output_coordinate = true;
      cluster.separate_output_occurrences = cluster.key.members.size() > 1;
      tables.clusters.push_back(cluster);
      tables.sequence_cluster_index.push_back(cluster.id);
      ordered_cluster_ids.push_back(cluster.id);
      ordered_group_indices.push_back(group_index);
    }
    sequence.clusters.count =
        tables.sequence_cluster_index.size() - sequence.clusters.begin;
    sequence.memberships.count =
        tables.membership_sequence_index.size() - sequence.memberships.begin;
    std::vector<std::uint64_t> adjacency_local;
    if (ordered_cluster_ids.size() > 1)
      adjacency_local.reserve(ordered_cluster_ids.size() - 1);
    for (std::size_t i = 1; i < ordered_cluster_ids.size(); ++i) {
      const auto &left_group = groups[ordered_group_indices[i - 1]];
      const auto &right_group = groups[ordered_group_indices[i]];
      std::uint64_t certificate = intersection_invalid_ordinal;
      for (const auto left : left_group)
        for (const auto right : right_group) {
          const auto candidate = pair_certificate_local(
              left, right, intersection_order_disposition::definitely_before);
          if (candidate != intersection_invalid_ordinal &&
              (certificate == intersection_invalid_ordinal ||
               candidate < certificate))
            certificate = candidate;
        }
      if (certificate == intersection_invalid_ordinal) {
        error = arrangement_error(
            intersection_subcode::unresolved_topology_order,
            "Component 08 adjacent source-edge clusters lack precedence evidence",
            intersection_checkpoint::source_edge_ordering);
        return false;
      }
      adjacency_local.push_back(certificate);
    }
    for (std::size_t i = 0; i < ordered_cluster_ids.size(); ++i) {
      auto &cluster = tables.clusters[ordered_cluster_ids[i].ordinal()];
      if (i != 0)
        cluster.predecessor = ordered_cluster_ids[i - 1];
      if (i + 1 != ordered_cluster_ids.size())
        cluster.successor = ordered_cluster_ids[i + 1];
      if (cluster.ordering_certificate.ordinal() ==
          intersection_invalid_ordinal) {
        const auto adjacent =
            i != 0
                ? adjacency_local[i - 1]
                : (!adjacency_local.empty() ? adjacency_local.front()
                                            : intersection_invalid_ordinal);
        cluster.ordering_certificate = ordering_certificate_id{adjacent};
        if (adjacent != intersection_invalid_ordinal)
          referenced_local.push_back(adjacent);
      }
      const auto membership_range = cluster.membership_ids;
      for (std::uint64_t offset = 0; offset < membership_range.count; ++offset) {
        const auto membership_id = tables.cluster_membership_index[
            membership_range.begin + offset];
        auto &membership = tables.memberships[membership_id.ordinal()];
        if (membership.ordering_certificate.ordinal() ==
            intersection_invalid_ordinal)
          membership.ordering_certificate = cluster.ordering_certificate;
      }
    }

    // Publish only the referenced certificates, in ascending local order, with
    // contiguous global IDs.
    std::sort(referenced_local.begin(), referenced_local.end());
    referenced_local.erase(
        std::unique(referenced_local.begin(), referenced_local.end()),
        referenced_local.end());
    const std::uint64_t certificate_base =
        tables.ordering_certificates.size();
    for (std::uint64_t index = 0; index < referenced_local.size(); ++index) {
      auto certificate = ordering.certificates[referenced_local[index]];
      certificate.id = ordering_certificate_id{certificate_base + index};
      tables.ordering_certificates.push_back(certificate);
    }
    const auto remap_certificate = [&](ordering_certificate_id id) {
      if (id.ordinal() == intersection_invalid_ordinal)
        return id;
      const auto found = std::lower_bound(
          referenced_local.begin(), referenced_local.end(), id.ordinal());
      return ordering_certificate_id{
          certificate_base +
          static_cast<std::uint64_t>(found - referenced_local.begin())};
    };
    for (const auto &cluster_id : ordered_cluster_ids) {
      auto &cluster = tables.clusters[cluster_id.ordinal()];
      cluster.ordering_certificate =
          remap_certificate(cluster.ordering_certificate);
      const auto membership_range = cluster.membership_ids;
      for (std::uint64_t offset = 0; offset < membership_range.count; ++offset) {
        auto &membership = tables.memberships[tables.cluster_membership_index[
            membership_range.begin + offset].ordinal()];
        membership.ordering_certificate =
            remap_certificate(membership.ordering_certificate);
      }
    }

    sequence.intervals.begin = tables.sequence_interval_index.size();
    const std::size_t interval_count = ordered_cluster_ids.size() + 1;
    for (std::size_t i = 0; i < interval_count; ++i) {
      source_edge_interval_record interval;
      interval.id = source_edge_interval_id{tables.intervals.size()};
      interval.sequence = sequence_id;
      interval.key.source_edge = domain.source_edge;
      interval.key.canonical_ordinal = i;
      if (i == 0)
        interval.key.left.kind = boundary_reference_kind::start_sentinel;
      else {
        interval.key.left.kind = boundary_reference_kind::cluster;
        interval.key.left.cluster =
            tables.clusters[ordered_cluster_ids[i - 1].ordinal()].key;
        const auto membership_id = tables.cluster_membership_index[
            tables.clusters[ordered_cluster_ids[i - 1].ordinal()]
                .membership_ids.begin];
        interval.left_parameter =
            tables.memberships[membership_id.ordinal()].parameter;
      }
      if (i == ordered_cluster_ids.size())
        interval.key.right.kind = boundary_reference_kind::end_sentinel;
      else {
        interval.key.right.kind = boundary_reference_kind::cluster;
        interval.key.right.cluster =
            tables.clusters[ordered_cluster_ids[i].ordinal()].key;
        const auto membership_id = tables.cluster_membership_index[
            tables.clusters[ordered_cluster_ids[i].ordinal()]
                .membership_ids.begin];
        interval.right_parameter =
            tables.memberships[membership_id.ordinal()].parameter;
      }
      interval.length_disposition =
          intersection_interval_length::definitely_positive;
      if (!ordered_cluster_ids.empty() && i == 0) {
        const auto membership_id = tables.cluster_membership_index[
            tables.clusters[ordered_cluster_ids.front().ordinal()]
                .membership_ids.begin];
        const auto &proposal = *std::find_if(
            proposals.begin(), proposals.end(), [&](const auto &p) {
              return p.parameter == tables.memberships[membership_id.ordinal()].parameter;
            });
        if (proposal.exact_zero == exact_relation_status::exact_zero)
          interval.length_disposition = intersection_interval_length::exact_zero;
      } else if (!ordered_cluster_ids.empty() &&
                 i == ordered_cluster_ids.size()) {
        const auto membership_id = tables.cluster_membership_index[
            tables.clusters[ordered_cluster_ids.back().ordinal()]
                .membership_ids.begin];
        const auto &proposal = *std::find_if(
            proposals.begin(), proposals.end(), [&](const auto &p) {
              return p.parameter == tables.memberships[membership_id.ordinal()].parameter;
            });
        if (proposal.exact_one == exact_relation_status::exact_zero)
          interval.length_disposition = intersection_interval_length::exact_zero;
      }
      interval.key.interval_class = interval.length_disposition;
      interval.propagation_allowed =
          interval.length_disposition ==
          intersection_interval_length::definitely_positive;
      interval.retention_allowed = true;
      interval.split_required = !ordered_cluster_ids.empty();
      if (!valid_source_edge_interval_key(interval.key)) {
        error = arrangement_error(intersection_subcode::source_edge_partition_invalid,
                                  "Component 08 source-edge interval key is invalid",
                                  intersection_checkpoint::source_edge_partition);
        return false;
      }
      tables.intervals.push_back(interval);
      tables.sequence_interval_index.push_back(interval.id);
    }
    sequence.intervals.count =
        tables.sequence_interval_index.size() - sequence.intervals.begin;
    tables.sequences.push_back(sequence);
  }

  for (const auto &proposal : proposals) {
    if (std::none_of(sorted_domains.begin(), sorted_domains.end(),
                     [&](const auto &domain) {
                       return domain.source_edge == proposal.key.source_edge;
                     })) {
      error = arrangement_error(intersection_subcode::membership_incomplete,
                                "Component 08 source-edge domain is missing",
                                intersection_checkpoint::source_edge_partition);
      return false;
    }
  }

  // Publish memberships in canonical complete-key order. The cluster/sequence
  // indexes reference memberships by ID, so reassign the dense IDs after the
  // sort.
  std::vector<std::uint64_t> membership_remap(tables.memberships.size());
  std::vector<source_edge_membership_record> sorted_memberships =
      tables.memberships;
  std::sort(sorted_memberships.begin(), sorted_memberships.end(),
            [](const auto &a, const auto &b) { return a.key < b.key; });
  for (std::size_t i = 0; i < sorted_memberships.size(); ++i) {
    membership_remap[sorted_memberships[i].id.ordinal()] = i;
    sorted_memberships[i].id = source_edge_membership_id{i};
  }
  tables.memberships = std::move(sorted_memberships);
  for (auto &id : tables.membership_sequence_index)
    id = source_edge_membership_id{membership_remap[id.ordinal()]};
  for (auto &id : tables.cluster_membership_index)
    id = source_edge_membership_id{membership_remap[id.ordinal()]};
  return true;
}

template <class T>
bool verify_source_edge_arrangements(
    const std::vector<source_edge_domain_record> &domains,
    const std::vector<source_edge_membership_proposal> &proposals,
    const source_edge_arrangement_tables &tables,
    bounded_boolean_error &error) {
  source_edge_arrangement_tables rebuilt;
  if (!build_source_edge_arrangements<T>(domains, proposals, rebuilt, error))
    return false;
  if (!equal_tables(rebuilt, tables)) {
    error = arrangement_error(intersection_subcode::source_edge_sequence_invalid,
                              "Component 08 source-edge reconstruction disagrees",
                              intersection_checkpoint::source_edge_partition);
    return false;
  }
  return true;
}

template bool build_source_edge_arrangements<float>(
    const std::vector<source_edge_domain_record> &,
    const std::vector<source_edge_membership_proposal> &,
    source_edge_arrangement_tables &, bounded_boolean_error &);
template bool build_source_edge_arrangements<double>(
    const std::vector<source_edge_domain_record> &,
    const std::vector<source_edge_membership_proposal> &,
    source_edge_arrangement_tables &, bounded_boolean_error &);
template bool verify_source_edge_arrangements<float>(
    const std::vector<source_edge_domain_record> &,
    const std::vector<source_edge_membership_proposal> &,
    const source_edge_arrangement_tables &, bounded_boolean_error &);
template bool verify_source_edge_arrangements<double>(
    const std::vector<source_edge_domain_record> &,
    const std::vector<source_edge_membership_proposal> &,
    const source_edge_arrangement_tables &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
