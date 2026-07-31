#include "YgorMeshesBooleanBounded/TransverseCarrierArrangements.h"

#include "YgorMeshesBooleanBounded/FloatingBits.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
void require(bool value) {
  if (!value)
    std::abort();
}

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t id) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = id;
  return key;
}

intersection_occurrence_key occurrence(std::uint32_t slot) {
  intersection_occurrence_key key;
  key.event.semantic_namespace.bytes[0] = 0x5a;
  key.event.semantic_namespace.bytes[1] = 0x08;
  key.event.event_class = intersection_event_class::edge_facet_point;
  key.event.first_operand = operand_id::a;
  key.event.second_operand = operand_id::b;
  key.event.first_owner =
      feature(operand_id::a, relation_feature_kind::source_edge, 9);
  key.event.second_owner =
      feature(operand_id::b, relation_feature_kind::source_facet, 3);
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
  key.event.construction_source_provenance = 100 + slot;
  key.event.construction_geometric_lineage = 200 + slot;
  key.event.carrier_role = intersection_carrier_role::transverse_endpoint;
  key.event.contact_status = feature_relation_status::proper_crossing;
  key.event.contact_dimension = relation_contact_dimension::point;
  key.discriminator.role = occurrence_role::single_occurrence;
  key.discriminator.component07_occurrence = slot + 1;
  return key;
}

transverse_carrier_key carrier_key(
    std::uint64_t construction, std::uint64_t lineage,
    carrier_orientation_role orientation =
        carrier_orientation_role::canonical_forward) {
  transverse_carrier_key key;
  key.first_facet =
      feature(operand_id::a, relation_feature_kind::source_facet, 3);
  key.second_facet =
      feature(operand_id::b, relation_feature_kind::source_facet, 7);
  key.construction = relation_construction_id{construction};
  key.construction_lineage = lineage;
  key.orientation = orientation;
  return key;
}

transverse_carrier_proposal carrier_proposal(
    transverse_carrier_key key, std::uint64_t relation,
    std::uint64_t candidate, std::uint64_t lineage, bool authority) {
  transverse_carrier_proposal proposal;
  proposal.key = key;
  proposal.construction = key.construction;
  proposal.relation = feature_relation_id{relation};
  proposal.candidate = candidate_id{candidate};
  proposal.relation_lineage = lineage;
  proposal.designated_authority = authority;
  proposal.support_consistent = true;
  proposal.orientation_consistent = true;
  proposal.residuals_accepted = true;
  proposal.precision_evidence_complete = true;
  return proposal;
}

carrier_membership_proposal membership(
    transverse_carrier_key key, std::uint32_t slot, double lower, double upper,
    std::uint64_t relation, std::uint64_t relation_lineage) {
  carrier_membership_proposal proposal;
  proposal.carrier = key;
  proposal.occurrence_key = occurrence(slot);
  proposal.occurrence = event_occurrence_id{slot};
  proposal.event = event_id{slot};
  proposal.parameter = relation_interval_evidence_id{1000 + slot};
  proposal.nominal_bits = to_bits((lower + upper) * 0.5);
  proposal.lower_bits = to_bits(lower);
  proposal.upper_bits = to_bits(upper);
  proposal.parameter_lineage = 2000 + slot;
  proposal.relation_lineage = relation_lineage;
  proposal.relation = feature_relation_id{relation};
  proposal.exact_equal_eligible = lower == upper;
  proposal.cluster_eligible = lower == upper;
  proposal.first_region_evidence =
      relation_source_facet_region_id{3000 + slot * 2};
  proposal.second_region_evidence =
      relation_source_facet_region_id{3001 + slot * 2};
  proposal.first_region_contains = true;
  proposal.second_region_contains = true;
  return proposal;
}

transverse_relation_interval_proposal relation_interval(
    const carrier_membership_proposal &start,
    const carrier_membership_proposal &end, std::uint64_t lineage,
    intersection_span_activation activation =
        intersection_span_activation::active_transverse_intersection) {
  transverse_relation_interval_proposal proposal;
  proposal.carrier = start.carrier;
  proposal.relation = start.relation;
  proposal.interval_lineage = lineage;
  proposal.start_parameter = start.parameter;
  proposal.end_parameter = end.parameter;
  proposal.start_lower_bits = start.lower_bits;
  proposal.start_upper_bits = start.upper_bits;
  proposal.end_lower_bits = end.lower_bits;
  proposal.end_upper_bits = end.upper_bits;
  proposal.start_occurrence = start.occurrence;
  proposal.end_occurrence = end.occurrence;
  proposal.activation = activation;
  proposal.first_region_evidence =
      relation_source_facet_region_id{4000 + lineage * 2};
  proposal.second_region_evidence =
      relation_source_facet_region_id{4001 + lineage * 2};
  proposal.first_region_contains = true;
  proposal.second_region_contains = true;
  proposal.ownership_verified = true;
  return proposal;
}

void require_subcode(const bounded_boolean_error &error,
                     intersection_subcode subcode) {
  require(error.subcode == static_cast<std::uint32_t>(subcode));
}
} // namespace

int main() {
  const auto key = carrier_key(31, 7001);
  auto authority = carrier_proposal(key, 10, 20, 8001, true);
  auto witness = carrier_proposal(key, 10, 21, 8001, false);
  auto m0 = membership(key, 0, 0.0, 0.0, 10, 8001);
  auto m1 = membership(key, 1, 1.0, 1.0, 10, 8001);
  auto m2 = membership(key, 2, 3.0, 3.0, 10, 8001);
  auto m3 = membership(key, 3, 4.0, 4.0, 10, 8001);
  auto i0 = relation_interval(m0, m1, 11);
  auto i1 = relation_interval(m2, m3, 12);

  std::vector<transverse_carrier_proposal> carriers{witness, authority};
  std::vector<carrier_membership_proposal> memberships{m2, m0, m3, m1};
  std::vector<transverse_relation_interval_proposal> intervals{i1, i0};
  transverse_carrier_arrangement_tables tables;
  bounded_boolean_error error;
  require(build_transverse_carrier_arrangements<double>(
      carriers, memberships, intervals, tables, error));
  require(tables.carriers.size() == 1);
  require(tables.carriers[0].candidate_provenance.count == 2);
  require(tables.memberships.size() == 4);
  require(tables.clusters.size() == 4);
  require(tables.spans.size() == 3);
  require(tables.spans[0].activation ==
          intersection_span_activation::active_transverse_intersection);
  require(tables.spans[1].activation ==
          intersection_span_activation::inactive);
  require(tables.spans[2].activation ==
          intersection_span_activation::active_transverse_intersection);
  require(tables.spans[0].classification_cut);
  require(tables.spans[0].output_edge_allowed);
  require(tables.spans[0].provenance.count == 1);
  require(tables.spans[0].region_incidence.count == 2);
  require(tables.spans[1].provenance.count == 0);
  require(tables.spans[1].region_incidence.count == 0);
  require(!tables.spans[1].classification_cut);
  require(!tables.spans[1].contact_delimiter);
  require(!tables.spans[1].output_edge_allowed);
  require(tables.carriers[0].region_incidence.count == 4);
  require(verify_transverse_carrier_arrangements<double>(
      carriers, memberships, intervals, tables, error));

  auto permuted_carriers = carriers;
  auto permuted_memberships = memberships;
  auto permuted_intervals = intervals;
  std::reverse(permuted_carriers.begin(), permuted_carriers.end());
  std::reverse(permuted_memberships.begin(), permuted_memberships.end());
  std::reverse(permuted_intervals.begin(), permuted_intervals.end());
  transverse_carrier_arrangement_tables permuted;
  require(build_transverse_carrier_arrangements<double>(
      permuted_carriers, permuted_memberships, permuted_intervals, permuted,
      error));
  require(verify_transverse_carrier_arrangements<double>(
      carriers, memberships, intervals, permuted, error));

  auto mutated = tables;
  mutated.spans[1].activation =
      intersection_span_activation::active_transverse_intersection;
  require(!verify_transverse_carrier_arrangements<double>(
      carriers, memberships, intervals, mutated, error));
  require_subcode(error, intersection_subcode::verifier_rejection);

  auto equal_member = membership(key, 4, 1.0, 1.0, 10, 8001);
  auto equal_memberships = memberships;
  equal_memberships.push_back(equal_member);
  transverse_carrier_arrangement_tables equal_tables;
  require(build_transverse_carrier_arrangements<double>(
      carriers, equal_memberships, intervals, equal_tables, error));
  require(equal_tables.memberships.size() == 5);
  require(equal_tables.clusters.size() == 4);
  require(equal_tables.clusters[1].occurrence_members.count == 2);
  require(equal_tables.clusters[1].separate_output_occurrences);

  auto unsupported_equal = equal_memberships;
  unsupported_equal[3].exact_equal_eligible = false;
  unsupported_equal[3].cluster_eligible = false;
  unsupported_equal[4].exact_equal_eligible = false;
  unsupported_equal[4].cluster_eligible = false;
  require(!build_transverse_carrier_arrangements<double>(
      carriers, unsupported_equal, intervals, permuted, error));
  require_subcode(error, intersection_subcode::exact_equal_without_evidence);

  auto n0 = membership(key, 10, 0.0, 1.0, 10, 8001);
  auto n1 = membership(key, 11, 0.5, 1.5, 10, 8001);
  auto n2 = membership(key, 12, 1.1, 2.0, 10, 8001);
  n0.parameter_lineage = 9000;
  n1.parameter_lineage = 9000;
  n2.parameter_lineage = 9000;
  n0.cluster_eligible = true;
  n1.cluster_eligible = true;
  n2.cluster_eligible = true;
  require(!build_transverse_carrier_arrangements<double>(
      carriers, {n0, n1, n2}, {}, permuted, error));
  require_subcode(error, intersection_subcode::cluster_invalid);

  auto missing_endpoint = intervals;
  missing_endpoint[0].end_occurrence = event_occurrence_id{999};
  require(!build_transverse_carrier_arrangements<double>(
      carriers, memberships, missing_endpoint, permuted, error));
  require_subcode(error, intersection_subcode::transverse_span_invalid);

  auto bad_membership = memberships;
  bad_membership.front().first_region_contains = false;
  require(!build_transverse_carrier_arrangements<double>(
      carriers, bad_membership, intervals, permuted, error));
  require_subcode(error, intersection_subcode::parameter_invalid);

  auto bad_interval = intervals;
  bad_interval.front().second_region_evidence =
      relation_source_facet_region_id{intersection_invalid_ordinal};
  require(!build_transverse_carrier_arrangements<double>(
      carriers, memberships, bad_interval, permuted, error));
  require_subcode(error, intersection_subcode::transverse_span_invalid);

  auto unsupported_carrier = carriers;
  unsupported_carrier.front().residuals_accepted = false;
  require(!build_transverse_carrier_arrangements<double>(
      unsupported_carrier, memberships, intervals, permuted, error));
  require_subcode(error, intersection_subcode::transverse_carrier_invalid);

  auto coplanar_carrier = carriers;
  coplanar_carrier.front().coplanar = true;
  require(!build_transverse_carrier_arrangements<double>(
      coplanar_carrier, memberships, intervals, permuted, error));
  require_subcode(error, intersection_subcode::coplanar_routed_transverse);

  auto duplicate_intervals = intervals;
  duplicate_intervals.push_back(intervals.front());
  require(!build_transverse_carrier_arrangements<double>(
      carriers, memberships, duplicate_intervals, permuted, error));
  require_subcode(error, intersection_subcode::transverse_span_invalid);

  auto unrelated_membership = memberships;
  unrelated_membership.front().relation = feature_relation_id{999};
  require(!build_transverse_carrier_arrangements<double>(
      carriers, unrelated_membership, intervals, permuted, error));
  require_subcode(error, intersection_subcode::membership_incomplete);

  auto outside_nominal = memberships;
  outside_nominal.front().nominal_bits = to_bits(9.0);
  require(!build_transverse_carrier_arrangements<double>(
      carriers, outside_nominal, intervals, permuted, error));
  require_subcode(error, intersection_subcode::parameter_invalid);

  const auto second_key = carrier_key(32, 7002);
  auto second_authority = carrier_proposal(second_key, 11, 30, 8002, true);
  auto s0 = membership(second_key, 20, -1.0, -1.0, 11, 8002);
  auto s1 = membership(second_key, 21, 2.0, 2.0, 11, 8002);
  auto second_interval = relation_interval(s0, s1, 13);
  transverse_carrier_arrangement_tables distinct;
  require(build_transverse_carrier_arrangements<double>(
      {authority, witness, second_authority},
      {m0, m1, m2, m3, s0, s1}, {i0, i1, second_interval}, distinct,
      error));
  require(distinct.carriers.size() == 2);

  auto reversed_key = carrier_key(
      33, 7003, carrier_orientation_role::canonical_reverse);
  auto reversed_authority =
      carrier_proposal(reversed_key, 12, 40, 8003, true);
  auto r0 = membership(reversed_key, 30, -2.0, -2.0, 12, 8003);
  auto r1 = membership(reversed_key, 31, -1.0, -1.0, 12, 8003);
  auto reversed_interval = relation_interval(r0, r1, 14);
  require(build_transverse_carrier_arrangements<double>(
      {reversed_authority}, {r0, r1}, {reversed_interval}, distinct, error));
  require(distinct.carriers.size() == 1);
  require(distinct.carriers[0].key.orientation ==
          carrier_orientation_role::canonical_reverse);

  auto remapped_key = remap_transverse_carrier_key(second_key);
  auto remapped_authority =
      carrier_proposal(remapped_key, 13, 50, 8004, true);
  auto q0 = membership(remapped_key, 40, 0.0, 0.0, 13, 8004);
  auto q1 = membership(remapped_key, 41, 1.0, 1.0, 13, 8004);
  auto remapped_interval = relation_interval(q0, q1, 15);
  require(build_transverse_carrier_arrangements<double>(
      {remapped_authority}, {q0, q1}, {remapped_interval}, distinct, error));
  require(valid_transverse_carrier_key(distinct.carriers[0].key));

  return 0;
}
