#include "YgorMeshesBooleanBounded/SourceEdgeArrangements.h"

#include "YgorMeshesBooleanBounded/FloatingBits.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
void require(bool value) { if (!value) std::abort(); }

relation_feature_key feature(relation_feature_kind kind, std::uint64_t id) {
  relation_feature_key key;
  key.operand = operand_id::a;
  key.kind = kind;
  key.primary = id;
  return key;
}

intersection_occurrence_key occurrence(std::uint32_t slot) {
  intersection_occurrence_key key;
  key.event.semantic_namespace.bytes[0] = 0x77;
  key.event.event_class = intersection_event_class::edge_facet_point;
  key.event.first_operand = operand_id::a;
  key.event.second_operand = operand_id::b;
  key.event.first_owner = feature(relation_feature_kind::source_edge, 9);
  key.event.second_owner.operand = operand_id::b;
  key.event.second_owner.kind = relation_feature_kind::source_facet;
  key.event.second_owner.primary = 3;
  key.event.public_relation.semantic_namespace = key.event.semantic_namespace;
  key.event.public_relation.family =
      feature_relation_family::source_edge_source_facet;
  key.event.public_relation.first = key.event.first_owner;
  key.event.public_relation.second = key.event.second_owner;
  key.event.public_relation.occurrence = slot + 1;
  key.event.construction_kind = relation_construction_kind::bounded_point;
  key.event.construction_precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  key.event.authoritative_source_feature = key.event.first_owner;
  key.event.construction_source_provenance = 10 + slot;
  key.event.construction_geometric_lineage = 20 + slot;
  key.event.carrier_role = intersection_carrier_role::transverse_endpoint;
  key.event.contact_status = feature_relation_status::proper_crossing;
  key.event.contact_dimension = relation_contact_dimension::point;
  key.discriminator.role = occurrence_role::single_occurrence;
  key.discriminator.component07_occurrence = slot + 1;
  return key;
}

source_edge_membership_proposal proposal(std::uint32_t slot, double lower,
                                         double upper,
                                         bool exact_zero = false,
                                         bool exact_one = false) {
  source_edge_membership_proposal p;
  p.key.source_edge = feature(relation_feature_kind::source_edge, 9);
  p.key.occurrence = occurrence(slot);
  p.key.role = exact_zero || exact_one
                   ? intersection_membership_role::endpoint
                   : intersection_membership_role::interior;
  p.key.parameter_evidence = relation_interval_evidence_id{slot};
  p.key.parameter_lineage = 100 + slot;
  p.key.relation_lineage = 200 + slot;
  p.key.facet_use_role = source_facet_use_role::right_incident;
  p.occurrence = event_occurrence_id{slot};
  p.event = event_id{slot};
  p.parameter = p.key.parameter_evidence;
  p.nominal_bits = to_bits((lower + upper) * 0.5);
  p.lower_bits = to_bits(lower);
  p.upper_bits = to_bits(upper);
  p.domain = exact_zero || exact_one ? parameter_domain_status::stable_endpoint
                                     : parameter_domain_status::stable_interior;
  p.exact_zero = exact_zero ? exact_relation_status::exact_zero
                            : exact_relation_status::exact_positive;
  p.exact_one = exact_one ? exact_relation_status::exact_zero
                          : exact_relation_status::exact_negative;
  p.exact_equal_eligible = lower == upper;
  p.cluster_eligible = lower == upper;
  return p;
}
} // namespace

int main() {
  source_edge_domain_record domain;
  domain.source_edge = feature(relation_feature_kind::source_edge, 9);
  domain.start_vertex = feature(relation_feature_kind::source_vertex, 1);
  domain.end_vertex = feature(relation_feature_kind::source_vertex, 2);

  auto p0 = proposal(0, 0.0, 0.0, true, false);
  auto p1 = proposal(1, 0.5, 0.5);
  auto p2 = proposal(2, 0.5, 0.5);
  p2.key.parameter_lineage = p1.key.parameter_lineage;
  auto p3 = proposal(3, 1.0, 1.0, false, true);
  std::vector<source_edge_membership_proposal> proposals{p2, p0, p3, p1};

  source_edge_arrangement_tables tables;
  bounded_boolean_error error;
  require(build_source_edge_arrangements<double>({domain}, proposals, tables,
                                                  error));
  require(tables.sequences.size() == 1);
  require(tables.memberships.size() == 4);
  require(tables.clusters.size() == 3);
  require(tables.intervals.size() == 4);
  require(tables.intervals.front().length_disposition ==
          intersection_interval_length::exact_zero);
  require(tables.intervals.back().length_disposition ==
          intersection_interval_length::exact_zero);
  require(tables.clusters[1].member_occurrences.count == 2);
  require(tables.clusters[1].separate_output_occurrences);
  require(tables.sequences[0].comparison_count == 3);
  require(tables.ordering_certificates.size() == 3);
  for (const auto &membership : tables.memberships)
    require(membership.ordering_certificate.ordinal() <
            tables.ordering_certificates.size());
  for (const auto &cluster : tables.clusters)
    require(cluster.ordering_certificate.ordinal() <
            tables.ordering_certificates.size());
  require(verify_source_edge_arrangements<double>({domain}, proposals, tables,
                                                   error));

  auto permuted = proposals;
  std::reverse(permuted.begin(), permuted.end());
  source_edge_arrangement_tables second;
  require(build_source_edge_arrangements<double>({domain}, permuted, second,
                                                  error));
  require(verify_source_edge_arrangements<double>({domain}, proposals, second,
                                                   error));

  auto unresolved = proposals;
  unresolved[0].lower_bits = to_bits(0.4);
  unresolved[0].upper_bits = to_bits(0.6);
  unresolved[0].exact_equal_eligible = false;
  unresolved[0].cluster_eligible = false;
  require(!build_source_edge_arrangements<double>({domain}, unresolved, second,
                                                   error));
  require(error.subcode == static_cast<std::uint32_t>(
                               intersection_subcode::unresolved_topology_order));

  auto unsupported_equal = proposals;
  unsupported_equal[0].exact_equal_eligible = false;
  unsupported_equal[0].cluster_eligible = false;
  unsupported_equal[3].exact_equal_eligible = false;
  unsupported_equal[3].cluster_eligible = false;
  require(!build_source_edge_arrangements<double>(
      {domain}, unsupported_equal, second, error));
  require(error.subcode == static_cast<std::uint32_t>(
                               intersection_subcode::exact_equal_without_evidence));

  require(build_source_edge_arrangements<double>({domain}, {}, second, error));
  require(second.sequences.size() == 1);
  require(second.intervals.size() == 1);
  require(second.intervals[0].length_disposition ==
          intersection_interval_length::definitely_positive);
  require(second.ordering_certificates.empty());
  return 0;
}
