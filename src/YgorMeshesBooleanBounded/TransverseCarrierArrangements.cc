#include "TransverseCarrierArrangements.h"

#include "BoundedCarrierOrdering.h"
#include "FloatingBits.h"
#include "Sha256.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error carrier_error(intersection_subcode subcode,
                                     const char *summary) {
  return intersection_error(subcode,
                            bounded_boolean_error_category::input_contract_error,
                            summary,
                            intersection_checkpoint::transverse_carriers);
}

template <class T>
bool decode_interval(std::uint64_t lower_bits, std::uint64_t upper_bits,
                     finite_interval<T> &value) noexcept {
  const T lower = from_bits<T>(static_cast<floating_uint_t<T>>(lower_bits));
  const T upper = from_bits<T>(static_cast<floating_uint_t<T>>(upper_bits));
  const auto interval = finite_interval<T>::create(lower, upper);
  if (!interval)
    return false;
  value = *interval;
  return true;
}

template <class T>
bool decode_parameter(const carrier_membership_proposal &proposal,
                      finite_interval<T> &value) noexcept {
  if (proposal.parameter.ordinal() == intersection_invalid_ordinal ||
      !decode_interval<T>(proposal.lower_bits, proposal.upper_bits, value))
    return false;
  const T nominal =
      from_bits<T>(static_cast<floating_uint_t<T>>(proposal.nominal_bits));
  const auto singleton = finite_interval<T>::create(nominal, nominal);
  return singleton && !finite_numeric_less(nominal, value.lower()) &&
         !finite_numeric_less(value.upper(), nominal);
}

bool equal_range(intersection_range a, intersection_range b) noexcept {
  return a.begin == b.begin && a.count == b.count;
}

bool equal_tables(const transverse_carrier_arrangement_tables &a,
                  const transverse_carrier_arrangement_tables &b) noexcept {
  if (a.carriers.size() != b.carriers.size() ||
      a.carrier_relation_provenance != b.carrier_relation_provenance ||
      a.carrier_candidate_provenance != b.carrier_candidate_provenance ||
      a.memberships.size() != b.memberships.size() ||
      a.carrier_membership_index != b.carrier_membership_index ||
      a.clusters.size() != b.clusters.size() ||
      a.cluster_occurrence_index != b.cluster_occurrence_index ||
      a.cluster_membership_index != b.cluster_membership_index ||
      a.carrier_cluster_index != b.carrier_cluster_index ||
      a.spans.size() != b.spans.size() ||
      a.carrier_span_index != b.carrier_span_index ||
      a.span_relation_provenance != b.span_relation_provenance ||
      a.span_region_incidence != b.span_region_incidence ||
      a.ordering_certificates.size() != b.ordering_certificates.size())
    return false;
  for (std::size_t i = 0; i < a.carriers.size(); ++i) {
    const auto &x = a.carriers[i];
    const auto &y = b.carriers[i];
    if (x.id != y.id || !(x.key == y.key) ||
        x.construction != y.construction ||
        !equal_range(x.relation_provenance, y.relation_provenance) ||
        !equal_range(x.candidate_provenance, y.candidate_provenance) ||
        !equal_range(x.memberships, y.memberships) ||
        !equal_range(x.clusters, y.clusters) ||
        !equal_range(x.active_spans, y.active_spans) ||
        !equal_range(x.region_incidence, y.region_incidence) ||
        x.carrier_digest != y.carrier_digest)
      return false;
  }
  for (std::size_t i = 0; i < a.memberships.size(); ++i) {
    const auto &x = a.memberships[i];
    const auto &y = b.memberships[i];
    if (x.id != y.id || x.carrier != y.carrier ||
        x.occurrence != y.occurrence || x.event != y.event ||
        x.parameter != y.parameter ||
        x.relation_lineage != y.relation_lineage ||
        x.ordering_certificate != y.ordering_certificate)
      return false;
  }
  for (std::size_t i = 0; i < a.clusters.size(); ++i) {
    const auto &x = a.clusters[i];
    const auto &y = b.clusters[i];
    if (x.id != y.id || x.carrier != y.carrier ||
        x.equivalence != y.equivalence ||
        !equal_range(x.occurrence_members, y.occurrence_members) ||
        !equal_range(x.membership_members, y.membership_members) ||
        x.predecessor != y.predecessor || x.successor != y.successor ||
        x.ordering_certificate != y.ordering_certificate ||
        x.shared_output_coordinate != y.shared_output_coordinate ||
        x.separate_output_occurrences != y.separate_output_occurrences)
      return false;
  }
  for (std::size_t i = 0; i < a.spans.size(); ++i) {
    const auto &x = a.spans[i];
    const auto &y = b.spans[i];
    if (x.id != y.id || x.carrier != y.carrier || x.left != y.left ||
        x.right != y.right || x.activation != y.activation ||
        x.relation_interval_lineage != y.relation_interval_lineage ||
        !equal_range(x.region_incidence, y.region_incidence) ||
        !equal_range(x.provenance, y.provenance) ||
        x.classification_cut != y.classification_cut ||
        x.contact_delimiter != y.contact_delimiter ||
        x.output_edge_allowed != y.output_edge_allowed)
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

bool valid_activation(intersection_span_activation activation) noexcept {
  return activation == intersection_span_activation::active_transverse_intersection ||
         activation == intersection_span_activation::active_overlap_boundary ||
         activation == intersection_span_activation::contact_only;
}

} // namespace

template <class T>
bool build_transverse_carrier_arrangements(
    const std::vector<transverse_carrier_proposal> &carrier_input,
    const std::vector<carrier_membership_proposal> &membership_input,
    const std::vector<transverse_relation_interval_proposal> &interval_input,
    transverse_carrier_arrangement_tables &tables,
    bounded_boolean_error &error) {
  tables = transverse_carrier_arrangement_tables{};

  auto carrier_proposals = carrier_input;
  std::sort(carrier_proposals.begin(), carrier_proposals.end(),
            [](const auto &a, const auto &b) {
              return std::tie(a.key, a.relation, a.candidate,
                              a.relation_lineage) <
                     std::tie(b.key, b.relation, b.candidate,
                              b.relation_lineage);
            });
  for (const auto &proposal : carrier_proposals) {
    if (!valid_transverse_carrier_key(proposal.key) ||
        proposal.construction != proposal.key.construction ||
        proposal.relation.ordinal() == intersection_invalid_ordinal ||
        proposal.candidate.ordinal() == intersection_invalid_ordinal ||
        proposal.relation_lineage == 0 || proposal.coplanar ||
        !proposal.support_consistent || !proposal.orientation_consistent ||
        !proposal.residuals_accepted ||
        !proposal.precision_evidence_complete) {
      error = carrier_error(
          proposal.coplanar ? intersection_subcode::coplanar_routed_transverse
                            : intersection_subcode::transverse_carrier_invalid,
          proposal.coplanar
              ? "Component 08 coplanar relation was routed through a transverse carrier"
              : "Component 08 transverse carrier proposal is invalid");
      return false;
    }
  }
  for (std::size_t i = 1; i < carrier_proposals.size(); ++i) {
    if (carrier_proposals[i - 1].key == carrier_proposals[i].key &&
        carrier_proposals[i - 1].relation == carrier_proposals[i].relation &&
        carrier_proposals[i - 1].candidate == carrier_proposals[i].candidate &&
        carrier_proposals[i - 1].relation_lineage ==
            carrier_proposals[i].relation_lineage) {
      error = carrier_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse carrier provenance tuple is duplicated");
      return false;
    }
  }

  std::size_t carrier_begin = 0;
  while (carrier_begin < carrier_proposals.size()) {
    std::size_t carrier_end = carrier_begin + 1;
    while (carrier_end < carrier_proposals.size() &&
           carrier_proposals[carrier_end].key ==
               carrier_proposals[carrier_begin].key)
      ++carrier_end;
    const auto authority_count = static_cast<std::size_t>(std::count_if(
        carrier_proposals.begin() + static_cast<std::ptrdiff_t>(carrier_begin),
        carrier_proposals.begin() + static_cast<std::ptrdiff_t>(carrier_end),
        [](const auto &proposal) { return proposal.designated_authority; }));
    if (authority_count != 1) {
      error = carrier_error(
          intersection_subcode::transverse_carrier_invalid,
          "Component 08 transverse carrier authority is not unique");
      return false;
    }

    transverse_carrier_record carrier;
    carrier.id = transverse_carrier_id{tables.carriers.size()};
    carrier.key = carrier_proposals[carrier_begin].key;
    carrier.construction = carrier.key.construction;
    carrier.relation_provenance.begin =
        tables.carrier_relation_provenance.size();
    carrier.candidate_provenance.begin =
        tables.carrier_candidate_provenance.size();
    for (std::size_t i = carrier_begin; i < carrier_end; ++i) {
      tables.carrier_relation_provenance.push_back(
          carrier_proposals[i].relation);
      tables.carrier_candidate_provenance.push_back(
          carrier_proposals[i].candidate);
    }
    std::sort(tables.carrier_relation_provenance.begin() +
                  static_cast<std::ptrdiff_t>(carrier.relation_provenance.begin),
              tables.carrier_relation_provenance.end());
    tables.carrier_relation_provenance.erase(
        std::unique(tables.carrier_relation_provenance.begin() +
                        static_cast<std::ptrdiff_t>(
                            carrier.relation_provenance.begin),
                    tables.carrier_relation_provenance.end()),
        tables.carrier_relation_provenance.end());
    std::sort(tables.carrier_candidate_provenance.begin() +
                  static_cast<std::ptrdiff_t>(carrier.candidate_provenance.begin),
              tables.carrier_candidate_provenance.end());
    tables.carrier_candidate_provenance.erase(
        std::unique(tables.carrier_candidate_provenance.begin() +
                        static_cast<std::ptrdiff_t>(
                            carrier.candidate_provenance.begin),
                    tables.carrier_candidate_provenance.end()),
        tables.carrier_candidate_provenance.end());
    carrier.relation_provenance.count =
        tables.carrier_relation_provenance.size() -
        carrier.relation_provenance.begin;
    carrier.candidate_provenance.count =
        tables.carrier_candidate_provenance.size() -
        carrier.candidate_provenance.begin;
    tables.carriers.push_back(carrier);
    carrier_begin = carrier_end;
  }

  auto memberships = membership_input;
  std::sort(memberships.begin(), memberships.end(), [](const auto &a,
                                                       const auto &b) {
    return std::tie(a.carrier, a.occurrence_key, a.parameter,
                    a.parameter_lineage, a.relation_lineage) <
           std::tie(b.carrier, b.occurrence_key, b.parameter,
                    b.parameter_lineage, b.relation_lineage);
  });
  for (std::size_t i = 0; i < memberships.size(); ++i) {
    finite_interval<T> parameter;
    if (!valid_transverse_carrier_key(memberships[i].carrier) ||
        !valid_intersection_occurrence_key(memberships[i].occurrence_key) ||
        memberships[i].occurrence.ordinal() == intersection_invalid_ordinal ||
        memberships[i].event.ordinal() == intersection_invalid_ordinal ||
        memberships[i].parameter_lineage == 0 ||
        memberships[i].relation_lineage == 0 ||
        memberships[i].relation.ordinal() == intersection_invalid_ordinal ||
        memberships[i].first_region_evidence.ordinal() ==
            intersection_invalid_ordinal ||
        memberships[i].second_region_evidence.ordinal() ==
            intersection_invalid_ordinal ||
        !memberships[i].first_region_contains ||
        !memberships[i].second_region_contains ||
        !decode_parameter<T>(memberships[i], parameter) ||
        (i != 0 && memberships[i - 1].carrier == memberships[i].carrier &&
         (memberships[i - 1].occurrence_key == memberships[i].occurrence_key ||
          memberships[i - 1].occurrence == memberships[i].occurrence))) {
      error = carrier_error(intersection_subcode::parameter_invalid,
                            "Component 08 transverse membership is invalid");
      return false;
    }
  }

  auto intervals = interval_input;
  std::sort(intervals.begin(), intervals.end(), [](const auto &a,
                                                   const auto &b) {
    return std::tie(a.carrier, a.interval_lineage, a.relation,
                    a.start_occurrence, a.end_occurrence) <
           std::tie(b.carrier, b.interval_lineage, b.relation,
                    b.start_occurrence, b.end_occurrence);
  });
  for (std::size_t i = 1; i < intervals.size(); ++i) {
    if (intervals[i - 1].carrier == intervals[i].carrier &&
        intervals[i - 1].interval_lineage == intervals[i].interval_lineage &&
        intervals[i - 1].relation == intervals[i].relation &&
        intervals[i - 1].start_occurrence == intervals[i].start_occurrence &&
        intervals[i - 1].end_occurrence == intervals[i].end_occurrence) {
      error = carrier_error(
          intersection_subcode::transverse_span_invalid,
          "Component 08 transverse relation interval tuple is duplicated");
      return false;
    }
  }

  for (auto &carrier : tables.carriers) {
    std::vector<std::size_t> local;
    for (std::size_t i = 0; i < memberships.size(); ++i)
      if (memberships[i].carrier == carrier.key)
        local.push_back(i);

    if (local.empty()) {
      error = carrier_error(
          intersection_subcode::membership_incomplete,
          "Component 08 transverse carrier has no finite endpoint membership");
      return false;
    }

    const auto relation_is_provenance = [&](feature_relation_id relation) {
      for (std::uint64_t offset = 0; offset < carrier.relation_provenance.count;
           ++offset) {
        if (tables.carrier_relation_provenance[
                carrier.relation_provenance.begin + offset] == relation)
          return true;
      }
      return false;
    };
    for (const auto member : local) {
      const bool lineage_matches = std::any_of(
          carrier_proposals.begin(), carrier_proposals.end(),
          [&](const auto &proposal) {
            return proposal.key == carrier.key &&
                   proposal.relation == memberships[member].relation &&
                   proposal.relation_lineage ==
                       memberships[member].relation_lineage;
          });
      if (!relation_is_provenance(memberships[member].relation) ||
          !lineage_matches) {
        error = carrier_error(
            intersection_subcode::membership_incomplete,
            "Component 08 transverse membership is not carrier provenance");
        return false;
      }
    }

    carrier.memberships.begin = tables.carrier_membership_index.size();
    carrier.clusters.begin = tables.carrier_cluster_index.size();
    carrier.active_spans.begin = tables.carrier_span_index.size();
    carrier.region_incidence.begin = tables.span_region_incidence.size();

    const ordering_certificate_id invalid_certificate{
        intersection_invalid_ordinal};
    std::vector<bounded_ordering_member> ordering_members;
    ordering_members.reserve(local.size());
    for (std::size_t i = 0; i < local.size(); ++i) {
      const auto &proposal = memberships[local[i]];
      bounded_ordering_member member;
      member.input_ordinal = i;
      member.parameter = proposal.parameter;
      member.occurrence = proposal.occurrence_key;
      member.nominal_bits = proposal.nominal_bits;
      member.lower_bits = proposal.lower_bits;
      member.upper_bits = proposal.upper_bits;
      member.exact_evidence_lineage =
          proposal.exact_equal_eligible ? proposal.parameter_lineage : 0;
      member.comparison_evidence_lineage = proposal.parameter_lineage;
      member.cluster_lineage = proposal.parameter_lineage;
      member.exact_equal_eligible = proposal.exact_equal_eligible;
      member.unresolved_cluster_eligible = proposal.cluster_eligible;
      member.topology_interchangeable = proposal.cluster_eligible;
      ordering_members.push_back(member);
    }

    bounded_ordering_result ordering;
    if (!build_bounded_carrier_order<T>(
            ordering_members, intersection_checkpoint::transverse_carriers,
            ordering, error) ||
        !verify_bounded_carrier_order<T>(
            ordering_members, intersection_checkpoint::transverse_carriers,
            ordering, error))
      return false;

    const std::uint64_t certificate_base =
        tables.ordering_certificates.size();
    for (auto certificate : ordering.certificates) {
      certificate.id = ordering_certificate_id{
          certificate_base + certificate.id.ordinal()};
      tables.ordering_certificates.push_back(certificate);
    }
    const auto pair_certificate =
        [&](std::size_t first, std::size_t second,
            intersection_order_disposition required) {
          ordering_certificate_id best{intersection_invalid_ordinal};
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
              const ordering_certificate_id candidate{
                  certificate_base + pair.certificate.ordinal()};
              if (best.ordinal() == intersection_invalid_ordinal ||
                  candidate < best)
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

    std::vector<carrier_cluster_id> ordered_cluster_ids;
    std::vector<std::size_t> ordered_group_indices;
    for (std::size_t group_index = 0; group_index < groups.size();
         ++group_index) {
      const auto &group = groups[group_index];
      carrier_cluster_record cluster;
      cluster.id = carrier_cluster_id{tables.clusters.size()};
      cluster.carrier = carrier.id;
      cluster.equivalence = ordering.clusters[group_index].equivalence;
      cluster.occurrence_members.begin =
          tables.cluster_occurrence_index.size();
      cluster.membership_members.begin =
          tables.cluster_membership_index.size();
      const auto equivalence_certificate =
          group.size() > 1
              ? pair_certificate(
                    group[0], group[1],
                    cluster.equivalence ==
                            intersection_cluster_equivalence::
                                lineage_authorized_unresolved
                        ? intersection_order_disposition::unresolved_overlap
                        : intersection_order_disposition::exact_equal)
              : invalid_certificate;
      cluster.ordering_certificate = equivalence_certificate;
      for (const auto member : group) {
        const auto &proposal = memberships[local[member]];
        carrier_membership_record record;
        record.id = carrier_membership_id{tables.memberships.size()};
        record.carrier = carrier.id;
        record.occurrence = proposal.occurrence;
        record.event = proposal.event;
        record.parameter = proposal.parameter;
        record.relation_lineage = proposal.relation_lineage;
        record.ordering_certificate = equivalence_certificate;
        tables.memberships.push_back(record);
        tables.carrier_membership_index.push_back(record.id);
        tables.cluster_membership_index.push_back(record.id);
        tables.cluster_occurrence_index.push_back(proposal.occurrence);
      }
      cluster.occurrence_members.count =
          tables.cluster_occurrence_index.size() -
          cluster.occurrence_members.begin;
      cluster.membership_members.count =
          tables.cluster_membership_index.size() -
          cluster.membership_members.begin;
      cluster.shared_output_coordinate = true;
      cluster.separate_output_occurrences = group.size() > 1;
      tables.clusters.push_back(cluster);
      tables.carrier_cluster_index.push_back(cluster.id);
      ordered_cluster_ids.push_back(cluster.id);
      ordered_group_indices.push_back(group_index);
    }

    std::vector<ordering_certificate_id> adjacency_certificates;
    if (ordered_cluster_ids.size() > 1)
      adjacency_certificates.reserve(ordered_cluster_ids.size() - 1);
    for (std::size_t i = 1; i < ordered_cluster_ids.size(); ++i) {
      const auto &left_group = groups[ordered_group_indices[i - 1]];
      const auto &right_group = groups[ordered_group_indices[i]];
      ordering_certificate_id certificate = invalid_certificate;
      for (const auto left : left_group)
        for (const auto right : right_group) {
          const auto candidate = pair_certificate(
              left, right, intersection_order_disposition::definitely_before);
          if (candidate.ordinal() != intersection_invalid_ordinal &&
              (certificate.ordinal() == intersection_invalid_ordinal ||
               candidate < certificate))
            certificate = candidate;
        }
      if (certificate.ordinal() == intersection_invalid_ordinal) {
        error = carrier_error(
            intersection_subcode::unresolved_topology_order,
            "Component 08 adjacent transverse clusters lack precedence evidence");
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
      for (std::uint64_t offset = 0;
           offset < cluster.membership_members.count; ++offset) {
        const auto membership_id = tables.cluster_membership_index[
            cluster.membership_members.begin + offset];
        auto &membership = tables.memberships[membership_id.ordinal()];
        if (membership.ordering_certificate.ordinal() ==
            intersection_invalid_ordinal)
          membership.ordering_certificate = cluster.ordering_certificate;
      }
    }

    std::vector<std::size_t> cluster_position(tables.clusters.size(),
                                              intersection_invalid_ordinal);
    for (std::size_t i = 0; i < ordered_cluster_ids.size(); ++i)
      cluster_position[ordered_cluster_ids[i].ordinal()] = i;

    struct resolved_interval final {
      const transverse_relation_interval_proposal *proposal = nullptr;
      std::size_t start_position = 0;
      std::size_t end_position = 0;
      bool used = false;
    };
    std::vector<resolved_interval> resolved;
    for (const auto &proposal : intervals) {
      if (!(proposal.carrier == carrier.key))
        continue;
      finite_interval<T> start;
      finite_interval<T> end;
      if (proposal.relation.ordinal() == intersection_invalid_ordinal ||
          proposal.interval_lineage == 0 ||
          proposal.start_parameter.ordinal() == intersection_invalid_ordinal ||
          proposal.end_parameter.ordinal() == intersection_invalid_ordinal ||
          !decode_interval<T>(proposal.start_lower_bits,
                              proposal.start_upper_bits, start) ||
          !decode_interval<T>(proposal.end_lower_bits,
                              proposal.end_upper_bits, end) ||
          !finite_numeric_less(start.upper(), end.lower()) ||
          proposal.start_occurrence.ordinal() == intersection_invalid_ordinal ||
          proposal.end_occurrence.ordinal() == intersection_invalid_ordinal ||
          proposal.first_region_evidence.ordinal() ==
              intersection_invalid_ordinal ||
          proposal.second_region_evidence.ordinal() ==
              intersection_invalid_ordinal ||
          !proposal.first_region_contains ||
          !proposal.second_region_contains || !proposal.ownership_verified ||
          !proposal.start_closed || !proposal.end_closed ||
          !valid_activation(proposal.activation) ||
          !relation_is_provenance(proposal.relation)) {
        error = carrier_error(intersection_subcode::transverse_span_invalid,
                              "Component 08 transverse relation interval is invalid");
        return false;
      }
      auto find_cluster = [&](event_occurrence_id occurrence) {
        carrier_cluster_id found{intersection_invalid_ordinal};
        for (const auto cluster_id : ordered_cluster_ids) {
          const auto &cluster = tables.clusters[cluster_id.ordinal()];
          for (std::uint64_t offset = 0;
               offset < cluster.occurrence_members.count; ++offset) {
            if (tables.cluster_occurrence_index[
                    cluster.occurrence_members.begin + offset] == occurrence) {
              if (found.ordinal() != intersection_invalid_ordinal)
                return carrier_cluster_id{intersection_invalid_ordinal};
              found = cluster_id;
            }
          }
        }
        return found;
      };
      const auto find_membership = [&](event_occurrence_id occurrence) {
        const carrier_membership_proposal *found = nullptr;
        for (const auto member : local) {
          if (memberships[member].occurrence == occurrence) {
            if (found != nullptr)
              return static_cast<const carrier_membership_proposal *>(nullptr);
            found = &memberships[member];
          }
        }
        return found;
      };
      const auto *start_membership = find_membership(proposal.start_occurrence);
      const auto *end_membership = find_membership(proposal.end_occurrence);
      if (start_membership == nullptr || end_membership == nullptr ||
          start_membership->parameter != proposal.start_parameter ||
          end_membership->parameter != proposal.end_parameter ||
          start_membership->lower_bits != proposal.start_lower_bits ||
          start_membership->upper_bits != proposal.start_upper_bits ||
          end_membership->lower_bits != proposal.end_lower_bits ||
          end_membership->upper_bits != proposal.end_upper_bits) {
        error = carrier_error(
            intersection_subcode::transverse_span_invalid,
            "Component 08 transverse interval endpoints disagree with membership evidence");
        return false;
      }

      const auto start_cluster = find_cluster(proposal.start_occurrence);
      const auto end_cluster = find_cluster(proposal.end_occurrence);
      if (start_cluster.ordinal() == intersection_invalid_ordinal ||
          end_cluster.ordinal() == intersection_invalid_ordinal ||
          cluster_position[start_cluster.ordinal()] >=
              cluster_position[end_cluster.ordinal()]) {
        error = carrier_error(
            intersection_subcode::transverse_span_invalid,
            "Component 08 transverse interval endpoint occurrence is missing or reversed");
        return false;
      }
      resolved.push_back(resolved_interval{
          &proposal, cluster_position[start_cluster.ordinal()],
          cluster_position[end_cluster.ordinal()], false});
    }

    for (std::size_t i = 1; i < ordered_cluster_ids.size(); ++i) {
      carrier_active_span_record span;
      span.id = carrier_active_span_id{tables.spans.size()};
      span.carrier = carrier.id;
      span.left = ordered_cluster_ids[i - 1];
      span.right = ordered_cluster_ids[i];
      span.activation = intersection_span_activation::inactive;
      span.provenance.begin = tables.span_relation_provenance.size();
      span.region_incidence.begin = tables.span_region_incidence.size();
      std::uint64_t lineage = 0;
      for (auto &entry : resolved) {
        if (entry.start_position <= i - 1 && entry.end_position >= i) {
          if (span.activation == intersection_span_activation::inactive) {
            span.activation = entry.proposal->activation;
            lineage = entry.proposal->interval_lineage;
          } else if (span.activation != entry.proposal->activation) {
            error = carrier_error(
                intersection_subcode::transverse_span_invalid,
                "Component 08 transverse span activation contradicts");
            return false;
          }
          lineage = std::min(lineage, entry.proposal->interval_lineage);
          tables.span_relation_provenance.push_back(entry.proposal->relation);
          tables.span_region_incidence.push_back(
              entry.proposal->first_region_evidence);
          tables.span_region_incidence.push_back(
              entry.proposal->second_region_evidence);
          entry.used = true;
        }
      }
      std::sort(tables.span_relation_provenance.begin() +
                    static_cast<std::ptrdiff_t>(span.provenance.begin),
                tables.span_relation_provenance.end());
      tables.span_relation_provenance.erase(
          std::unique(tables.span_relation_provenance.begin() +
                          static_cast<std::ptrdiff_t>(span.provenance.begin),
                      tables.span_relation_provenance.end()),
          tables.span_relation_provenance.end());
      span.provenance.count = tables.span_relation_provenance.size() -
                              span.provenance.begin;
      std::sort(tables.span_region_incidence.begin() +
                    static_cast<std::ptrdiff_t>(span.region_incidence.begin),
                tables.span_region_incidence.end());
      tables.span_region_incidence.erase(
          std::unique(tables.span_region_incidence.begin() +
                          static_cast<std::ptrdiff_t>(span.region_incidence.begin),
                      tables.span_region_incidence.end()),
          tables.span_region_incidence.end());
      span.region_incidence.count = tables.span_region_incidence.size() -
                                    span.region_incidence.begin;
      span.relation_interval_lineage = lineage;
      span.classification_cut =
          span.activation ==
          intersection_span_activation::active_transverse_intersection;
      span.contact_delimiter =
          span.activation == intersection_span_activation::contact_only;
      span.output_edge_allowed =
          span.activation ==
              intersection_span_activation::active_transverse_intersection ||
          span.activation ==
              intersection_span_activation::active_overlap_boundary;
      tables.spans.push_back(span);
      tables.carrier_span_index.push_back(span.id);
    }
    for (const auto &entry : resolved) {
      if (!entry.used) {
        error = carrier_error(
            intersection_subcode::transverse_span_invalid,
            "Component 08 transverse relation interval supports no finite open span");
        return false;
      }
    }

    carrier.memberships.count = tables.carrier_membership_index.size() -
                                carrier.memberships.begin;
    carrier.clusters.count = tables.carrier_cluster_index.size() -
                             carrier.clusters.begin;
    carrier.active_spans.count = tables.carrier_span_index.size() -
                                 carrier.active_spans.begin;
    carrier.region_incidence.count = tables.span_region_incidence.size() -
                                     carrier.region_incidence.begin;
    canonical_writer writer;
    encode_transverse_carrier_key(writer, carrier.key);
    writer.u64(carrier.relation_provenance.count);
    for (std::uint64_t offset = 0; offset < carrier.relation_provenance.count;
         ++offset)
      writer.u64(tables.carrier_relation_provenance[
                     carrier.relation_provenance.begin + offset]
                     .ordinal());
    writer.u64(carrier.candidate_provenance.count);
    for (std::uint64_t offset = 0; offset < carrier.candidate_provenance.count;
         ++offset)
      writer.u64(tables.carrier_candidate_provenance[
                     carrier.candidate_provenance.begin + offset]
                     .ordinal());
    writer.u64(carrier.memberships.count);
    for (std::uint64_t offset = 0; offset < carrier.memberships.count;
         ++offset) {
      const auto id = tables.carrier_membership_index[
          carrier.memberships.begin + offset];
      const auto &membership = tables.memberships[id.ordinal()];
      writer.u64(membership.occurrence.ordinal());
      writer.u64(membership.event.ordinal());
      writer.u64(membership.parameter.ordinal());
      writer.u64(membership.relation_lineage);
    }
    writer.u64(carrier.clusters.count);
    for (std::uint64_t offset = 0; offset < carrier.clusters.count; ++offset) {
      const auto id = tables.carrier_cluster_index[carrier.clusters.begin + offset];
      const auto &cluster = tables.clusters[id.ordinal()];
      writer.u8(static_cast<std::uint8_t>(cluster.equivalence));
      writer.u64(cluster.occurrence_members.count);
      for (std::uint64_t member = 0; member < cluster.occurrence_members.count;
           ++member)
        writer.u64(tables.cluster_occurrence_index[
                       cluster.occurrence_members.begin + member]
                       .ordinal());
    }
    writer.u64(carrier.active_spans.count);
    for (std::uint64_t offset = 0; offset < carrier.active_spans.count;
         ++offset) {
      const auto id = tables.carrier_span_index[carrier.active_spans.begin + offset];
      const auto &span = tables.spans[id.ordinal()];
      writer.u64(span.left.ordinal());
      writer.u64(span.right.ordinal());
      writer.u8(static_cast<std::uint8_t>(span.activation));
      writer.u64(span.relation_interval_lineage);
      writer.u64(span.provenance.count);
      for (std::uint64_t provenance = 0; provenance < span.provenance.count;
           ++provenance)
        writer.u64(tables.span_relation_provenance[
                       span.provenance.begin + provenance]
                       .ordinal());
      writer.u64(span.region_incidence.count);
      for (std::uint64_t region = 0; region < span.region_incidence.count;
           ++region)
        writer.u64(tables.span_region_incidence[
                       span.region_incidence.begin + region]
                       .ordinal());
    }
    carrier.carrier_digest = sha256::digest(writer.bytes());
  }

  for (const auto &proposal : memberships) {
    if (std::none_of(tables.carriers.begin(), tables.carriers.end(),
                     [&](const auto &carrier) {
                       return carrier.key == proposal.carrier;
                     })) {
      error = carrier_error(intersection_subcode::membership_incomplete,
                            "Component 08 transverse membership has no carrier");
      return false;
    }
  }
  for (const auto &proposal : intervals) {
    if (std::none_of(tables.carriers.begin(), tables.carriers.end(),
                     [&](const auto &carrier) {
                       return carrier.key == proposal.carrier;
                     })) {
      error = carrier_error(intersection_subcode::transverse_span_invalid,
                            "Component 08 transverse interval has no carrier");
      return false;
    }
  }
  return true;
}

template <class T>
bool verify_transverse_carrier_arrangements(
    const std::vector<transverse_carrier_proposal> &carrier_proposals,
    const std::vector<carrier_membership_proposal> &membership_proposals,
    const std::vector<transverse_relation_interval_proposal> &interval_proposals,
    const transverse_carrier_arrangement_tables &tables,
    bounded_boolean_error &error) {
  transverse_carrier_arrangement_tables rebuilt;
  if (!build_transverse_carrier_arrangements<T>(
          carrier_proposals, membership_proposals, interval_proposals,
          rebuilt, error))
    return false;
  if (!equal_tables(rebuilt, tables)) {
    error = carrier_error(
        intersection_subcode::verifier_rejection,
        "Component 08 transverse carrier reconstruction disagrees");
    return false;
  }
  return true;
}

template bool build_transverse_carrier_arrangements<float>(
    const std::vector<transverse_carrier_proposal> &,
    const std::vector<carrier_membership_proposal> &,
    const std::vector<transverse_relation_interval_proposal> &,
    transverse_carrier_arrangement_tables &, bounded_boolean_error &);
template bool build_transverse_carrier_arrangements<double>(
    const std::vector<transverse_carrier_proposal> &,
    const std::vector<carrier_membership_proposal> &,
    const std::vector<transverse_relation_interval_proposal> &,
    transverse_carrier_arrangement_tables &, bounded_boolean_error &);
template bool verify_transverse_carrier_arrangements<float>(
    const std::vector<transverse_carrier_proposal> &,
    const std::vector<carrier_membership_proposal> &,
    const std::vector<transverse_relation_interval_proposal> &,
    const transverse_carrier_arrangement_tables &, bounded_boolean_error &);
template bool verify_transverse_carrier_arrangements<double>(
    const std::vector<transverse_carrier_proposal> &,
    const std::vector<carrier_membership_proposal> &,
    const std::vector<transverse_relation_interval_proposal> &,
    const transverse_carrier_arrangement_tables &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
