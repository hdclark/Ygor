#include "BoundedCarrierOrdering.h"

#include "FloatingBits.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error ordering_error(intersection_subcode subcode,
                                     const char *summary,
                                     intersection_checkpoint checkpoint) {
  const bool geometric =
      subcode == intersection_subcode::exact_equal_without_evidence ||
      subcode == intersection_subcode::unresolved_topology_order ||
      subcode == intersection_subcode::bounded_order_contradiction ||
      subcode == intersection_subcode::cluster_invalid;
  return intersection_error(
      subcode,
      geometric
          ? bounded_boolean_error_category::geometric_condition_exceeds_tolerance
          : bounded_boolean_error_category::input_contract_error,
      summary, checkpoint);
}

struct disjoint_set final {
  explicit disjoint_set(std::size_t count) : parent(count), rank(count, 0) {
    std::iota(parent.begin(), parent.end(), std::size_t{0});
  }

  std::size_t find(std::size_t value) {
    if (parent[value] != value)
      parent[value] = find(parent[value]);
    return parent[value];
  }

  void unite(std::size_t first, std::size_t second) {
    first = find(first);
    second = find(second);
    if (first == second)
      return;
    if (rank[first] < rank[second])
      std::swap(first, second);
    parent[second] = first;
    if (rank[first] == rank[second])
      ++rank[first];
  }

  std::vector<std::size_t> parent{};
  std::vector<std::uint8_t> rank{};
};

template <class T>
bool decode_interval(const bounded_ordering_member &member,
                     finite_interval<T> &interval) noexcept {
  if (member.parameter.ordinal() == intersection_invalid_ordinal ||
      member.reserved8 != 0)
    return false;
  const auto lower =
      from_bits<T>(static_cast<floating_uint_t<T>>(member.lower_bits));
  const auto upper =
      from_bits<T>(static_cast<floating_uint_t<T>>(member.upper_bits));
  const auto decoded = finite_interval<T>::create(lower, upper);
  if (!decoded)
    return false;
  interval = *decoded;
  return true;
}

intersection_order_disposition reverse_disposition(
    intersection_order_disposition disposition) noexcept {
  if (disposition == intersection_order_disposition::definitely_before)
    return intersection_order_disposition::definitely_after;
  if (disposition == intersection_order_disposition::definitely_after)
    return intersection_order_disposition::definitely_before;
  return disposition;
}

template <class T>
intersection_order_disposition compare_members(
    const bounded_ordering_member &first,
    const finite_interval<T> &first_interval,
    const bounded_ordering_member &second,
    const finite_interval<T> &second_interval) noexcept {
  if (finite_numeric_less(first_interval.upper(), second_interval.lower()))
    return intersection_order_disposition::definitely_before;
  if (finite_numeric_less(second_interval.upper(), first_interval.lower()))
    return intersection_order_disposition::definitely_after;

  const bool identical_bounds =
      to_bits(first_interval.lower()) == to_bits(second_interval.lower()) &&
      to_bits(first_interval.upper()) == to_bits(second_interval.upper());
  if (identical_bounds && first.exact_equal_eligible &&
      second.exact_equal_eligible && first.exact_evidence_lineage != 0 &&
      second.exact_evidence_lineage != 0)
    return intersection_order_disposition::exact_equal;

  if (first.unresolved_cluster_eligible &&
      second.unresolved_cluster_eligible && first.topology_interchangeable &&
      second.topology_interchangeable && first.cluster_lineage != 0 &&
      first.cluster_lineage == second.cluster_lineage)
    return intersection_order_disposition::unresolved_overlap;

  return intersection_order_disposition::invalid;
}


template <class T>
intersection_order_disposition verify_compare_members(
    const bounded_ordering_member &first,
    const finite_interval<T> &first_interval,
    const bounded_ordering_member &second,
    const finite_interval<T> &second_interval) noexcept {
  // Deliberately separate control flow from the producer comparison helper.
  if (finite_numeric_less(second_interval.upper(), first_interval.lower()))
    return intersection_order_disposition::definitely_after;
  if (finite_numeric_less(first_interval.upper(), second_interval.lower()))
    return intersection_order_disposition::definitely_before;

  const bool exact_bounds =
      to_bits(first_interval.lower()) == to_bits(second_interval.lower()) &&
      to_bits(first_interval.upper()) == to_bits(second_interval.upper());
  if (exact_bounds && first.exact_equal_eligible &&
      second.exact_equal_eligible && first.exact_evidence_lineage != 0 &&
      second.exact_evidence_lineage != 0)
    return intersection_order_disposition::exact_equal;

  const bool authorized_cluster =
      first.unresolved_cluster_eligible &&
      second.unresolved_cluster_eligible && first.topology_interchangeable &&
      second.topology_interchangeable && first.cluster_lineage != 0 &&
      first.cluster_lineage == second.cluster_lineage;
  return authorized_cluster
             ? intersection_order_disposition::unresolved_overlap
             : intersection_order_disposition::invalid;
}

} // namespace

template <class T>
bool build_bounded_carrier_order(
    const std::vector<bounded_ordering_member> &members,
    intersection_checkpoint checkpoint, bounded_ordering_result &result,
    bounded_boolean_error &error) {
  result = bounded_ordering_result{};
  if (members.empty())
    return true;

  std::vector<finite_interval<T>> intervals(members.size());
  std::vector<std::size_t> input_order(members.size());
  std::iota(input_order.begin(), input_order.end(), std::size_t{0});
  std::sort(input_order.begin(), input_order.end(), [&](std::size_t first,
                                                        std::size_t second) {
    return members[first].input_ordinal < members[second].input_ordinal;
  });
  for (std::size_t i = 0; i < input_order.size(); ++i) {
    const auto index = input_order[i];
    if (!valid_intersection_occurrence_key(members[index].occurrence) ||
        !decode_interval<T>(members[index], intervals[index]) ||
        (i != 0 && members[input_order[i - 1]].input_ordinal ==
                       members[index].input_ordinal)) {
      error = ordering_error(intersection_subcode::parameter_invalid,
                             "Component 08 bounded-order member is invalid",
                             checkpoint);
      return false;
    }
  }

  const auto invalid = ordering_certificate_id{intersection_invalid_ordinal};
  std::vector<std::vector<intersection_order_disposition>> dispositions(
      members.size(), std::vector<intersection_order_disposition>(
                          members.size(), intersection_order_disposition::invalid));
  std::vector<std::vector<ordering_certificate_id>> certificate_ids(
      members.size(),
      std::vector<ordering_certificate_id>(members.size(), invalid));
  for (std::size_t i = 0; i < members.size(); ++i)
    dispositions[i][i] = intersection_order_disposition::exact_equal;

  disjoint_set clusters(members.size());
  auto record_comparison = [&](std::size_t first, std::size_t second,
                               bool active_pair) {
    if (first == second ||
        dispositions[first][second] !=
            intersection_order_disposition::invalid)
      return true;
    const auto disposition = compare_members<T>(members[first], intervals[first],
                                                members[second], intervals[second]);
    ++result.comparison_count;
    if (active_pair)
      ++result.active_pair_count;
    if (disposition == intersection_order_disposition::invalid) {
      const bool identical_bounds =
          members[first].lower_bits == members[second].lower_bits &&
          members[first].upper_bits == members[second].upper_bits;
      error = ordering_error(
          identical_bounds
              ? intersection_subcode::exact_equal_without_evidence
              : intersection_subcode::unresolved_topology_order,
          identical_bounds
              ? "Component 08 exact-equal parameter lacks exact evidence"
              : "Component 08 topology-relevant bounded order is unresolved",
          checkpoint);
      return false;
    }

    dispositions[first][second] = disposition;
    dispositions[second][first] = reverse_disposition(disposition);
    ordering_certificate_record certificate;
    certificate.id = ordering_certificate_id{result.certificates.size()};
    certificate.disposition = disposition;
    certificate.first_parameter = members[first].parameter;
    certificate.second_parameter = members[second].parameter;
    certificate.exact_evidence_lineage =
        disposition == intersection_order_disposition::exact_equal
            ? std::max(members[first].exact_evidence_lineage,
                       members[second].exact_evidence_lineage)
            : 0;
    const auto first_comparison = members[first].comparison_evidence_lineage;
    const auto second_comparison = members[second].comparison_evidence_lineage;
    certificate.comparison_evidence_lineage =
        first_comparison == 0
            ? second_comparison
            : second_comparison == 0
                  ? first_comparison
                  : std::min(first_comparison, second_comparison);
    certificate.topology_safe = true;
    result.certificates.push_back(certificate);

    const auto first_ordinal = members[first].input_ordinal;
    const auto second_ordinal = members[second].input_ordinal;
    bounded_ordering_pair_certificate pair;
    pair.first_input_ordinal = first_ordinal;
    pair.second_input_ordinal = second_ordinal;
    pair.certificate = certificate.id;
    result.pair_certificates.push_back(pair);
    certificate_ids[first][second] = certificate.id;
    certificate_ids[second][first] = certificate.id;

    if (disposition == intersection_order_disposition::exact_equal ||
        disposition == intersection_order_disposition::unresolved_overlap)
      clusters.unite(first, second);
    return true;
  };

  std::vector<std::size_t> sweep(members.size());
  std::iota(sweep.begin(), sweep.end(), std::size_t{0});
  std::sort(sweep.begin(), sweep.end(), [&](std::size_t first,
                                            std::size_t second) {
    if (finite_total_less(intervals[first].lower(), intervals[second].lower()))
      return true;
    if (finite_total_less(intervals[second].lower(), intervals[first].lower()))
      return false;
    if (finite_total_less(intervals[first].upper(), intervals[second].upper()))
      return true;
    if (finite_total_less(intervals[second].upper(), intervals[first].upper()))
      return false;
    if (members[first].occurrence < members[second].occurrence)
      return true;
    if (members[second].occurrence < members[first].occurrence)
      return false;
    return members[first].input_ordinal < members[second].input_ordinal;
  });

  std::vector<std::size_t> active;
  for (std::size_t position = 0; position < sweep.size(); ++position) {
    const auto current = sweep[position];
    std::vector<std::size_t> retained;
    retained.reserve(active.size() + 1);
    for (const auto candidate : active) {
      if (finite_numeric_less(intervals[candidate].upper(),
                              intervals[current].lower()))
        continue;
      if (!record_comparison(candidate, current, true))
        return false;
      retained.push_back(candidate);
    }
    if (position != 0 &&
        !record_comparison(sweep[position - 1], current, false))
      return false;
    retained.push_back(current);
    active = std::move(retained);
  }

  std::sort(result.pair_certificates.begin(), result.pair_certificates.end(),
            [](const auto &first, const auto &second) {
    if (first.first_input_ordinal != second.first_input_ordinal)
      return first.first_input_ordinal < second.first_input_ordinal;
    return first.second_input_ordinal < second.second_input_ordinal;
  });

  std::vector<std::vector<std::size_t>> groups;
  for (std::size_t i = 0; i < members.size(); ++i) {
    const auto root = clusters.find(i);
    auto found = std::find_if(groups.begin(), groups.end(), [&](const auto &group) {
      return clusters.find(group.front()) == root;
    });
    if (found == groups.end())
      groups.push_back({i});
    else
      found->push_back(i);
  }
  for (auto &group : groups)
    std::sort(group.begin(), group.end(), [&](std::size_t first,
                                             std::size_t second) {
      if (members[first].occurrence < members[second].occurrence)
        return true;
      if (members[second].occurrence < members[first].occurrence)
        return false;
      return members[first].input_ordinal < members[second].input_ordinal;
    });

  for (const auto &group : groups) {
    for (std::size_t i = 0; i < group.size(); ++i) {
      for (std::size_t j = i + 1; j < group.size(); ++j) {
        const auto disposition = dispositions[group[i]][group[j]];
        if ((disposition != intersection_order_disposition::exact_equal &&
             disposition !=
                 intersection_order_disposition::unresolved_overlap) ||
            certificate_ids[group[i]][group[j]].ordinal() ==
                intersection_invalid_ordinal) {
          error = ordering_error(
              intersection_subcode::cluster_invalid,
              "Component 08 bounded-order cluster is not an all-pairs clique",
              checkpoint);
          return false;
        }
      }
    }
  }

  std::vector<std::vector<bool>> before(
      groups.size(), std::vector<bool>(groups.size(), false));
  for (std::size_t first_group = 0; first_group < groups.size(); ++first_group) {
    for (std::size_t second_group = first_group + 1;
         second_group < groups.size(); ++second_group) {
      int direction = 0;
      for (const auto first : groups[first_group]) {
        for (const auto second : groups[second_group]) {
          const auto disposition = dispositions[first][second];
          if (disposition == intersection_order_disposition::invalid)
            continue;
          const int current =
              disposition == intersection_order_disposition::definitely_before
                  ? -1
                  : disposition ==
                            intersection_order_disposition::definitely_after
                        ? 1
                        : 0;
          if (current == 0 || (direction != 0 && direction != current)) {
            error = ordering_error(
                intersection_subcode::bounded_order_contradiction,
                "Component 08 bounded-order cluster precedence contradicts",
                checkpoint);
            return false;
          }
          direction = current;
        }
      }
      before[first_group][second_group] = direction < 0;
      before[second_group][first_group] = direction > 0;
    }
  }

  std::vector<std::size_t> group_order;
  std::vector<bool> emitted(groups.size(), false);
  while (group_order.size() != groups.size()) {
    std::size_t candidate = groups.size();
    for (std::size_t group = 0; group < groups.size(); ++group) {
      if (emitted[group])
        continue;
      bool has_predecessor = false;
      for (std::size_t other = 0; other < groups.size(); ++other)
        if (!emitted[other] && before[other][group])
          has_predecessor = true;
      if (!has_predecessor) {
        if (candidate != groups.size()) {
          error = ordering_error(
              intersection_subcode::unresolved_topology_order,
              "Component 08 bounded-order cluster order is not total",
              checkpoint);
          return false;
        }
        candidate = group;
      }
    }
    if (candidate == groups.size()) {
      error = ordering_error(intersection_subcode::bounded_order_contradiction,
                             "Component 08 bounded-order precedence is cyclic",
                             checkpoint);
      return false;
    }
    emitted[candidate] = true;
    group_order.push_back(candidate);
  }

  for (const auto group_index : group_order) {
    const auto &group = groups[group_index];
    bounded_ordering_cluster cluster;
    cluster.members.begin = result.ordered_member_ordinals.size();
    cluster.members.count = group.size();
    cluster.canonical_key_ordinal = members[group.front()].input_ordinal;
    cluster.topology_interchangeable = true;
    cluster.equivalence =
        intersection_cluster_equivalence::exact_parameter_coincidence;
    for (std::size_t i = 0; i < group.size(); ++i) {
      const auto member = group[i];
      result.ordered_member_ordinals.push_back(members[member].input_ordinal);
      cluster.canonical_key_ordinal =
          std::min(cluster.canonical_key_ordinal, members[member].input_ordinal);
      cluster.topology_interchangeable =
          cluster.topology_interchangeable &&
          (group.size() == 1 || members[member].topology_interchangeable);
      for (std::size_t j = i + 1; j < group.size(); ++j)
        if (dispositions[group[i]][group[j]] ==
            intersection_order_disposition::unresolved_overlap)
          cluster.equivalence =
              intersection_cluster_equivalence::lineage_authorized_unresolved;
    }
    result.clusters.push_back(cluster);
  }

  for (std::size_t i = 1; i < group_order.size(); ++i) {
    bool certified = false;
    for (const auto previous : groups[group_order[i - 1]]) {
      for (const auto current : groups[group_order[i]]) {
        if (dispositions[previous][current] ==
                intersection_order_disposition::definitely_before &&
            certificate_ids[previous][current].ordinal() !=
                intersection_invalid_ordinal)
          certified = true;
      }
    }
    if (!certified) {
      error = ordering_error(
          intersection_subcode::unresolved_topology_order,
          "Component 08 adjacent bounded-order clusters lack precedence evidence",
          checkpoint);
      return false;
    }
  }

  return true;
}

template <class T>
bool verify_bounded_carrier_order(
    const std::vector<bounded_ordering_member> &members,
    intersection_checkpoint checkpoint, const bounded_ordering_result &result,
    bounded_boolean_error &error) {
  const auto reject = [&](const char *summary) {
    error = intersection_error(
        intersection_subcode::verifier_rejection,
        bounded_boolean_error_category::internal_invariant_error, summary,
        checkpoint);
    return false;
  };

  if (members.empty()) {
    if (!result.ordered_member_ordinals.empty() || !result.clusters.empty() ||
        !result.certificates.empty() || !result.pair_certificates.empty() ||
        result.comparison_count != 0 || result.active_pair_count != 0)
      return reject("Component 08 empty bounded-order result is noncanonical");
    return true;
  }

  std::vector<finite_interval<T>> intervals(members.size());
  std::vector<std::size_t> by_ordinal(members.size());
  std::iota(by_ordinal.begin(), by_ordinal.end(), std::size_t{0});
  std::sort(by_ordinal.begin(), by_ordinal.end(), [&](std::size_t first,
                                                      std::size_t second) {
    return members[first].input_ordinal < members[second].input_ordinal;
  });
  for (std::size_t i = 0; i < by_ordinal.size(); ++i) {
    const auto member = by_ordinal[i];
    if (!valid_intersection_occurrence_key(members[member].occurrence) ||
        !decode_interval<T>(members[member], intervals[member]) ||
        (i != 0 && members[by_ordinal[i - 1]].input_ordinal ==
                       members[member].input_ordinal))
      return reject("Component 08 bounded-order verifier input is invalid");
  }

  std::vector<std::vector<intersection_order_disposition>> dispositions(
      members.size(), std::vector<intersection_order_disposition>(
                          members.size(), intersection_order_disposition::invalid));
  disjoint_set components(members.size());
  for (std::size_t i = 0; i < members.size(); ++i) {
    dispositions[i][i] = intersection_order_disposition::exact_equal;
    for (std::size_t j = i + 1; j < members.size(); ++j) {
      const auto disposition = verify_compare_members<T>(
          members[i], intervals[i], members[j], intervals[j]);
      if (disposition == intersection_order_disposition::invalid)
        return reject("Component 08 verifier found unresolved bounded order");
      dispositions[i][j] = disposition;
      dispositions[j][i] = reverse_disposition(disposition);
      if (disposition == intersection_order_disposition::exact_equal ||
          disposition == intersection_order_disposition::unresolved_overlap)
        components.unite(i, j);
    }
  }

  std::vector<std::vector<std::size_t>> groups;
  for (std::size_t i = 0; i < members.size(); ++i) {
    const auto root = components.find(i);
    auto found = std::find_if(groups.begin(), groups.end(), [&](const auto &group) {
      return components.find(group.front()) == root;
    });
    if (found == groups.end())
      groups.push_back({i});
    else
      found->push_back(i);
  }
  for (auto &group : groups) {
    std::sort(group.begin(), group.end(), [&](std::size_t first,
                                             std::size_t second) {
      if (members[first].occurrence < members[second].occurrence)
        return true;
      if (members[second].occurrence < members[first].occurrence)
        return false;
      return members[first].input_ordinal < members[second].input_ordinal;
    });
    for (std::size_t i = 0; i < group.size(); ++i)
      for (std::size_t j = i + 1; j < group.size(); ++j) {
        const auto disposition = dispositions[group[i]][group[j]];
        if (disposition != intersection_order_disposition::exact_equal &&
            disposition != intersection_order_disposition::unresolved_overlap)
          return reject("Component 08 verifier cluster is not pairwise valid");
      }
  }

  std::vector<std::vector<bool>> before(
      groups.size(), std::vector<bool>(groups.size(), false));
  for (std::size_t first_group = 0; first_group < groups.size(); ++first_group) {
    for (std::size_t second_group = first_group + 1;
         second_group < groups.size(); ++second_group) {
      int direction = 0;
      for (const auto first : groups[first_group])
        for (const auto second : groups[second_group]) {
          const auto disposition = dispositions[first][second];
          const int current =
              disposition == intersection_order_disposition::definitely_before
                  ? -1
                  : disposition ==
                            intersection_order_disposition::definitely_after
                        ? 1
                        : 0;
          if (current == 0 || (direction != 0 && direction != current))
            return reject(
                "Component 08 verifier found contradictory cluster order");
          direction = current;
        }
      before[first_group][second_group] = direction < 0;
      before[second_group][first_group] = direction > 0;
    }
  }

  std::vector<std::size_t> group_order;
  std::vector<bool> emitted(groups.size(), false);
  while (group_order.size() != groups.size()) {
    std::size_t candidate = groups.size();
    for (std::size_t group = 0; group < groups.size(); ++group) {
      if (emitted[group])
        continue;
      bool has_predecessor = false;
      for (std::size_t other = 0; other < groups.size(); ++other)
        if (!emitted[other] && before[other][group])
          has_predecessor = true;
      if (!has_predecessor) {
        if (candidate != groups.size())
          return reject("Component 08 verifier cluster order is not total");
        candidate = group;
      }
    }
    if (candidate == groups.size())
      return reject("Component 08 verifier cluster order is cyclic");
    emitted[candidate] = true;
    group_order.push_back(candidate);
  }

  std::vector<std::uint64_t> expected_ordinals;
  std::vector<bounded_ordering_cluster> expected_clusters;
  for (const auto group_index : group_order) {
    const auto &group = groups[group_index];
    bounded_ordering_cluster cluster;
    cluster.members.begin = expected_ordinals.size();
    cluster.members.count = group.size();
    cluster.canonical_key_ordinal = members[group.front()].input_ordinal;
    cluster.topology_interchangeable = true;
    cluster.equivalence =
        intersection_cluster_equivalence::exact_parameter_coincidence;
    for (std::size_t i = 0; i < group.size(); ++i) {
      const auto member = group[i];
      expected_ordinals.push_back(members[member].input_ordinal);
      cluster.canonical_key_ordinal =
          std::min(cluster.canonical_key_ordinal, members[member].input_ordinal);
      cluster.topology_interchangeable =
          cluster.topology_interchangeable &&
          (group.size() == 1 || members[member].topology_interchangeable);
      for (std::size_t j = i + 1; j < group.size(); ++j)
        if (dispositions[group[i]][group[j]] ==
            intersection_order_disposition::unresolved_overlap)
          cluster.equivalence =
              intersection_cluster_equivalence::lineage_authorized_unresolved;
    }
    expected_clusters.push_back(cluster);
  }

  if (result.ordered_member_ordinals != expected_ordinals ||
      result.clusters.size() != expected_clusters.size())
    return reject("Component 08 verifier reconstructed a different order");
  for (std::size_t i = 0; i < result.clusters.size(); ++i) {
    const auto &actual = result.clusters[i];
    const auto &expected = expected_clusters[i];
    if (actual.equivalence != expected.equivalence ||
        actual.members.begin != expected.members.begin ||
        actual.members.count != expected.members.count ||
        actual.canonical_key_ordinal != expected.canonical_key_ordinal ||
        actual.topology_interchangeable != expected.topology_interchangeable ||
        actual.reserved8 != 0 || actual.reserved16 != 0 ||
        actual.reserved32 != 0)
      return reject("Component 08 verifier reconstructed a different cluster");
  }

  if (result.certificates.size() != result.pair_certificates.size() ||
      result.comparison_count != result.certificates.size())
    return reject("Component 08 bounded-order certificate count is invalid");
  for (std::size_t i = 0; i < result.certificates.size(); ++i)
    if (result.certificates[i].id.ordinal() != i)
      return reject("Component 08 ordering certificate IDs are noncanonical");

  std::vector<std::vector<bool>> covered(
      members.size(), std::vector<bool>(members.size(), false));
  std::uint64_t active_count = 0;
  std::uint64_t previous_first = 0;
  std::uint64_t previous_second = 0;
  bool have_previous = false;
  for (std::size_t i = 0; i < result.pair_certificates.size(); ++i) {
    const auto &pair = result.pair_certificates[i];
    if (pair.certificate.ordinal() >= result.certificates.size() ||
        (have_previous &&
         (pair.first_input_ordinal < previous_first ||
          (pair.first_input_ordinal == previous_first &&
           pair.second_input_ordinal <= previous_second))))
      return reject("Component 08 pair certificates are noncanonical");
    have_previous = true;
    previous_first = pair.first_input_ordinal;
    previous_second = pair.second_input_ordinal;

    const auto first_it = std::lower_bound(
        by_ordinal.begin(), by_ordinal.end(), pair.first_input_ordinal,
        [&](std::size_t index, std::uint64_t ordinal) {
          return members[index].input_ordinal < ordinal;
        });
    const auto second_it = std::lower_bound(
        by_ordinal.begin(), by_ordinal.end(), pair.second_input_ordinal,
        [&](std::size_t index, std::uint64_t ordinal) {
          return members[index].input_ordinal < ordinal;
        });
    if (first_it == by_ordinal.end() || second_it == by_ordinal.end() ||
        members[*first_it].input_ordinal != pair.first_input_ordinal ||
        members[*second_it].input_ordinal != pair.second_input_ordinal ||
        *first_it == *second_it || covered[*first_it][*second_it])
      return reject("Component 08 pair certificate references are invalid");

    const auto first = *first_it;
    const auto second = *second_it;
    covered[first][second] = true;
    covered[second][first] = true;
    const auto expected_disposition = dispositions[first][second];
    const auto &certificate = result.certificates[pair.certificate.ordinal()];
    const auto first_comparison = members[first].comparison_evidence_lineage;
    const auto second_comparison = members[second].comparison_evidence_lineage;
    const auto expected_comparison =
        first_comparison == 0
            ? second_comparison
            : second_comparison == 0
                  ? first_comparison
                  : std::min(first_comparison, second_comparison);
    const auto expected_exact =
        expected_disposition == intersection_order_disposition::exact_equal
            ? std::max(members[first].exact_evidence_lineage,
                       members[second].exact_evidence_lineage)
            : 0;
    if (certificate.id != pair.certificate ||
        certificate.disposition != expected_disposition ||
        certificate.first_parameter != members[first].parameter ||
        certificate.second_parameter != members[second].parameter ||
        certificate.exact_evidence_lineage != expected_exact ||
        certificate.comparison_evidence_lineage != expected_comparison ||
        !certificate.topology_safe ||
        certificate.policy_version !=
            contract_versions::intersection_bounded_ordering_policy ||
        certificate.reserved8 != 0)
      return reject("Component 08 ordering certificate reconstruction failed");
    if (expected_disposition == intersection_order_disposition::exact_equal ||
        expected_disposition ==
            intersection_order_disposition::unresolved_overlap)
      ++active_count;
  }
  if (result.active_pair_count != active_count)
    return reject("Component 08 active comparison count is invalid");

  for (std::size_t i = 0; i < members.size(); ++i)
    for (std::size_t j = i + 1; j < members.size(); ++j) {
      const auto disposition = dispositions[i][j];
      if ((disposition == intersection_order_disposition::exact_equal ||
           disposition == intersection_order_disposition::unresolved_overlap) &&
          !covered[i][j])
        return reject("Component 08 cluster pair lacks a certificate");
    }

  for (std::size_t i = 1; i < group_order.size(); ++i) {
    bool certified = false;
    for (const auto first : groups[group_order[i - 1]])
      for (const auto second : groups[group_order[i]])
        if (dispositions[first][second] ==
                intersection_order_disposition::definitely_before &&
            covered[first][second])
          certified = true;
    if (!certified)
      return reject("Component 08 adjacent clusters lack verifier evidence");
  }
  return true;
}

template bool build_bounded_carrier_order<float>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    bounded_ordering_result &, bounded_boolean_error &);
template bool build_bounded_carrier_order<double>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    bounded_ordering_result &, bounded_boolean_error &);
template bool verify_bounded_carrier_order<float>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    const bounded_ordering_result &, bounded_boolean_error &);
template bool verify_bounded_carrier_order<double>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    const bounded_ordering_result &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
