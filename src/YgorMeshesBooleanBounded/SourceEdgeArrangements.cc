#include "SourceEdgeArrangements.h"

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

template <class T>
bool decode_parameter(const source_edge_membership_proposal &proposal,
                      finite_interval<T> &value) noexcept {
  if (proposal.parameter.ordinal() == intersection_invalid_ordinal ||
      proposal.domain == parameter_domain_status::outside ||
      proposal.domain == parameter_domain_status::invalid)
    return false;
  const auto lower = from_bits<T>(
      static_cast<floating_uint_t<T>>(proposal.lower_bits));
  const auto upper = from_bits<T>(
      static_cast<floating_uint_t<T>>(proposal.upper_bits));
  const auto interval = finite_interval<T>::create(lower, upper);
  if (!interval || finite_numeric_less(lower, T(0)) ||
      finite_numeric_less(T(1), upper))
    return false;
  value = *interval;
  return true;
}

template <class T>
intersection_order_disposition compare_parameters(
    const source_edge_membership_proposal &a,
    const source_edge_membership_proposal &b) noexcept {
  finite_interval<T> left;
  finite_interval<T> right;
  if (!decode_parameter(a, left) || !decode_parameter(b, right))
    return intersection_order_disposition::invalid;
  if (finite_numeric_less(left.upper(), right.lower()))
    return intersection_order_disposition::definitely_before;
  if (finite_numeric_less(right.upper(), left.lower()))
    return intersection_order_disposition::definitely_after;
  if (to_bits(left.lower()) == to_bits(right.lower()) &&
      to_bits(left.upper()) == to_bits(right.upper()) &&
      a.exact_equal_eligible && b.exact_equal_eligible)
    return intersection_order_disposition::exact_equal;
  if (a.cluster_eligible && b.cluster_eligible &&
      a.key.parameter_lineage == b.key.parameter_lineage &&
      a.key.parameter_lineage != 0)
    return intersection_order_disposition::unresolved_overlap;
  return intersection_order_disposition::invalid;
}

struct disjoint_set final {
  explicit disjoint_set(std::size_t size) : parent(size), rank(size, 0) {
    std::iota(parent.begin(), parent.end(), std::size_t{0});
  }
  std::size_t find(std::size_t value) {
    if (parent[value] != value)
      parent[value] = find(parent[value]);
    return parent[value];
  }
  void unite(std::size_t a, std::size_t b) {
    a = find(a);
    b = find(b);
    if (a == b)
      return;
    if (rank[a] < rank[b])
      std::swap(a, b);
    parent[b] = a;
    if (rank[a] == rank[b])
      ++rank[a];
  }
  std::vector<std::size_t> parent;
  std::vector<std::uint8_t> rank;
};

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
        !checked_range(incidence.seed_ranges[i].begin,
                       incidence.seed_ranges[i].count,
                       incidence.by_seed.size())) {
      error = arrangement_error(intersection_subcode::membership_incomplete,
                                "Component 08 source-edge incidence range is malformed",
                                intersection_checkpoint::source_edge_membership_proposals);
      return false;
    }
    const auto range = incidence.seed_ranges[i];
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
      const relation_interval_evidence_record *parameter = nullptr;
      for (const auto &candidate : interval_evidence) {
        if (candidate.source_relation == seed.source_relation &&
            candidate.kind == kind &&
            candidate.occurrence == seed.key.occurrence) {
          if (parameter != nullptr) {
            error = arrangement_error(intersection_subcode::parameter_invalid,
                                      "Component 08 source-edge parameter is ambiguous",
                                      intersection_checkpoint::source_edge_membership_proposals);
            return false;
          }
          parameter = &candidate;
        }
      }
      if (parameter == nullptr || !parameter->has_parameter_metadata ||
          !parameter->within_authorized_boundary ||
          parameter->trace_root == 0) {
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
      proposal.key.parameter_lineage = parameter->trace_root;
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
      proposal.contributions = binding.incidence;
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

    disjoint_set sets(local.size());
    const ordering_certificate_id invalid_certificate{
        intersection_invalid_ordinal};
    std::vector<std::vector<intersection_order_disposition>> relation(
        local.size(), std::vector<intersection_order_disposition>(
                          local.size(), intersection_order_disposition::invalid));
    std::vector<std::vector<ordering_certificate_id>> certificate_ids(
        local.size(), std::vector<ordering_certificate_id>(
                          local.size(), invalid_certificate));
    std::vector<finite_interval<T>> parameter_intervals(local.size());
    for (std::size_t i = 0; i < local.size(); ++i) {
      if (!decode_parameter(proposals[local[i]], parameter_intervals[i])) {
        error = arrangement_error(intersection_subcode::parameter_invalid,
                                  "Component 08 source-edge parameter decode failed",
                                  intersection_checkpoint::source_edge_ordering);
        return false;
      }
      relation[i][i] = intersection_order_disposition::exact_equal;
    }

    auto record_comparison = [&](std::size_t first, std::size_t second) {
      if (first == second ||
          relation[first][second] != intersection_order_disposition::invalid)
        return true;
      const auto disposition = compare_parameters<T>(
          proposals[local[first]], proposals[local[second]]);
      ++sequence.comparison_count;
      if (disposition == intersection_order_disposition::invalid) {
        const bool identical_bounds =
            proposals[local[first]].lower_bits ==
                proposals[local[second]].lower_bits &&
            proposals[local[first]].upper_bits ==
                proposals[local[second]].upper_bits;
        error = arrangement_error(
            identical_bounds
                ? intersection_subcode::exact_equal_without_evidence
                : intersection_subcode::unresolved_topology_order,
            identical_bounds
                ? "Component 08 exact-equal parameter lacks evidence"
                : "Component 08 source-edge order is unresolved",
            intersection_checkpoint::source_edge_ordering);
        return false;
      }
      relation[first][second] = disposition;
      relation[second][first] =
          disposition == intersection_order_disposition::definitely_before
              ? intersection_order_disposition::definitely_after
              : disposition == intersection_order_disposition::definitely_after
                    ? intersection_order_disposition::definitely_before
                    : disposition;
      ordering_certificate_record certificate;
      certificate.id = ordering_certificate_id{
          tables.ordering_certificates.size()};
      certificate.disposition = disposition;
      certificate.first_parameter = proposals[local[first]].parameter;
      certificate.second_parameter = proposals[local[second]].parameter;
      const auto first_lineage =
          proposals[local[first]].key.parameter_lineage;
      const auto second_lineage =
          proposals[local[second]].key.parameter_lineage;
      certificate.exact_evidence_lineage =
          disposition == intersection_order_disposition::exact_equal
              ? std::max(first_lineage, second_lineage)
              : 0;
      certificate.comparison_evidence_lineage =
          std::min(first_lineage, second_lineage);
      certificate.topology_safe = true;
      tables.ordering_certificates.push_back(certificate);
      certificate_ids[first][second] = certificate.id;
      certificate_ids[second][first] = certificate.id;
      if (disposition == intersection_order_disposition::exact_equal ||
          disposition == intersection_order_disposition::unresolved_overlap)
        sets.unite(first, second);
      return true;
    };

    std::vector<std::size_t> sweep(local.size());
    std::iota(sweep.begin(), sweep.end(), std::size_t{0});
    std::sort(sweep.begin(), sweep.end(), [&](std::size_t a, std::size_t b) {
      const auto &left = parameter_intervals[a];
      const auto &right = parameter_intervals[b];
      if (finite_total_less(left.lower(), right.lower()))
        return true;
      if (finite_total_less(right.lower(), left.lower()))
        return false;
      if (finite_total_less(left.upper(), right.upper()))
        return true;
      if (finite_total_less(right.upper(), left.upper()))
        return false;
      return proposals[local[a]].key < proposals[local[b]].key;
    });

    std::vector<std::size_t> active;
    for (std::size_t position = 0; position < sweep.size(); ++position) {
      const auto current = sweep[position];
      std::vector<std::size_t> retained;
      retained.reserve(active.size() + 1);
      for (const auto candidate : active) {
        if (finite_numeric_less(parameter_intervals[candidate].upper(),
                                parameter_intervals[current].lower()))
          continue;
        if (!record_comparison(candidate, current))
          return false;
        retained.push_back(candidate);
      }
      if (position != 0) {
        const auto previous = sweep[position - 1];
        if (!record_comparison(previous, current))
          return false;
      }
      retained.push_back(current);
      active = std::move(retained);
    }

    std::vector<std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < local.size(); ++i) {
      const auto root = sets.find(i);
      auto found = std::find_if(groups.begin(), groups.end(), [&](const auto &g) {
        return sets.find(g.front()) == root;
      });
      if (found == groups.end())
        groups.push_back({i});
      else
        found->push_back(i);
    }
    for (auto &group : groups)
      std::sort(group.begin(), group.end(), [&](std::size_t a, std::size_t b) {
        return proposals[local[a]].key < proposals[local[b]].key;
      });

    for (const auto &group : groups) {
      for (std::size_t i = 0; i < group.size(); ++i) {
        for (std::size_t j = i + 1; j < group.size(); ++j) {
          const auto disposition = relation[group[i]][group[j]];
          if ((disposition != intersection_order_disposition::exact_equal &&
               disposition !=
                   intersection_order_disposition::unresolved_overlap) ||
              certificate_ids[group[i]][group[j]].ordinal() ==
                  intersection_invalid_ordinal) {
            error = arrangement_error(
                intersection_subcode::cluster_invalid,
                "Component 08 source-edge cluster is not an all-pairs clique",
                intersection_checkpoint::source_edge_ordering);
            return false;
          }
        }
      }
    }

    std::vector<std::vector<bool>> before(
        groups.size(), std::vector<bool>(groups.size(), false));
    for (std::size_t a = 0; a < groups.size(); ++a) {
      for (std::size_t b = a + 1; b < groups.size(); ++b) {
        int direction = 0;
        for (const auto ai : groups[a]) {
          for (const auto bi : groups[b]) {
            const auto disposition = relation[ai][bi];
            if (disposition == intersection_order_disposition::invalid)
              continue;
            const int current =
                disposition == intersection_order_disposition::definitely_before
                    ? -1
                    : disposition == intersection_order_disposition::definitely_after
                          ? 1
                          : 0;
            if (current == 0 || (direction != 0 && direction != current)) {
              error = arrangement_error(
                  intersection_subcode::bounded_order_contradiction,
                  "Component 08 source-edge cluster order contradicts",
                  intersection_checkpoint::source_edge_ordering);
              return false;
            }
            direction = current;
          }
        }
        before[a][b] = direction < 0;
        before[b][a] = direction > 0;
      }
    }

    std::vector<std::size_t> order;
    std::vector<bool> emitted(groups.size(), false);
    while (order.size() != groups.size()) {
      std::size_t candidate = groups.size();
      for (std::size_t i = 0; i < groups.size(); ++i) {
        if (emitted[i])
          continue;
        bool has_predecessor = false;
        for (std::size_t j = 0; j < groups.size(); ++j)
          if (!emitted[j] && before[j][i])
            has_predecessor = true;
        if (!has_predecessor) {
          if (candidate != groups.size()) {
            error = arrangement_error(
                intersection_subcode::unresolved_topology_order,
                "Component 08 source-edge cluster order is not total",
                intersection_checkpoint::source_edge_ordering);
            return false;
          }
          candidate = i;
        }
      }
      if (candidate == groups.size()) {
        error = arrangement_error(intersection_subcode::bounded_order_contradiction,
                                  "Component 08 source-edge order is cyclic",
                                  intersection_checkpoint::source_edge_ordering);
        return false;
      }
      emitted[candidate] = true;
      order.push_back(candidate);
    }

    sequence.clusters.begin = tables.sequence_cluster_index.size();
    sequence.memberships.begin = tables.membership_sequence_index.size();
    std::vector<source_edge_cluster_id> ordered_cluster_ids;
    std::vector<std::size_t> ordered_group_indices;
    for (const auto group_index : order) {
      const auto &group = groups[group_index];
      source_edge_cluster_record cluster;
      cluster.id = source_edge_cluster_id{tables.clusters.size()};
      cluster.sequence = sequence_id;
      cluster.key.source_edge = domain.source_edge;
      cluster.key.equivalence =
          group.size() == 1
              ? intersection_cluster_equivalence::exact_parameter_coincidence
              : intersection_cluster_equivalence::exact_parameter_coincidence;
      for (const auto member : group) {
        const auto &proposal = proposals[local[member]];
        cluster.key.members.push_back(proposal.key.occurrence);
      }
      std::sort(cluster.key.members.begin(), cluster.key.members.end());
      cluster.key.members.erase(
          std::unique(cluster.key.members.begin(), cluster.key.members.end()),
          cluster.key.members.end());
      for (std::size_t i = 0; i < group.size(); ++i)
        for (std::size_t j = i + 1; j < group.size(); ++j)
          if (relation[group[i]][group[j]] ==
              intersection_order_disposition::unresolved_overlap)
            cluster.key.equivalence =
                intersection_cluster_equivalence::lineage_authorized_unresolved;
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
      const auto equivalence_certificate =
          group.size() > 1 ? certificate_ids[group[0]][group[1]]
                           : invalid_certificate;
      cluster.ordering_certificate = equivalence_certificate;
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
        record.ordering_certificate = equivalence_certificate;
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
    std::vector<ordering_certificate_id> adjacency_certificates;
    if (ordered_cluster_ids.size() > 1)
      adjacency_certificates.reserve(ordered_cluster_ids.size() - 1);
    for (std::size_t i = 1; i < ordered_cluster_ids.size(); ++i) {
      const auto &left_group = groups[ordered_group_indices[i - 1]];
      const auto &right_group = groups[ordered_group_indices[i]];
      ordering_certificate_id certificate = invalid_certificate;
      for (const auto left : left_group) {
        for (const auto right : right_group) {
          if (relation[left][right] ==
                  intersection_order_disposition::definitely_before &&
              certificate_ids[left][right].ordinal() !=
                  intersection_invalid_ordinal) {
            if (certificate.ordinal() == intersection_invalid_ordinal ||
                certificate_ids[left][right] < certificate)
              certificate = certificate_ids[left][right];
          }
        }
      }
      if (certificate.ordinal() == intersection_invalid_ordinal) {
        error = arrangement_error(
            intersection_subcode::unresolved_topology_order,
            "Component 08 adjacent source-edge clusters lack precedence evidence",
            intersection_checkpoint::source_edge_ordering);
        return false;
      }
      adjacency_certificates.push_back(certificate);
    }
    for (std::size_t i = 0; i < ordered_cluster_ids.size(); ++i) {
      auto &cluster = tables.clusters[ordered_cluster_ids[i].ordinal()];
      if (i != 0)
        cluster.predecessor = ordered_cluster_ids[i - 1];
      if (i + 1 != ordered_cluster_ids.size())
        cluster.successor = ordered_cluster_ids[i + 1];
      if (cluster.ordering_certificate.ordinal() ==
          intersection_invalid_ordinal) {
        if (i != 0)
          cluster.ordering_certificate = adjacency_certificates[i - 1];
        else if (!adjacency_certificates.empty())
          cluster.ordering_certificate = adjacency_certificates.front();
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
