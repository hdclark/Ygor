#include "YgorMeshesBooleanBounded/BoundedCarrierOrdering.h"
#include "YgorMeshesBooleanBounded/FloatingBits.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {

void require(bool value) {
  if (!value)
    std::abort();
}

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = primary;
  return key;
}

intersection_occurrence_key occurrence(std::uint32_t slot) {
  intersection_occurrence_key key;
  key.event.semantic_namespace.bytes[0] = 0x48;
  key.event.event_class = intersection_event_class::edge_facet_point;
  key.event.first_operand = operand_id::a;
  key.event.second_operand = operand_id::b;
  key.event.first_owner =
      feature(operand_id::a, relation_feature_kind::source_edge, 7);
  key.event.second_owner =
      feature(operand_id::b, relation_feature_kind::source_facet, 11);
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

bounded_ordering_member member(std::uint32_t slot, double lower, double upper) {
  bounded_ordering_member value;
  value.input_ordinal = slot;
  value.parameter = relation_interval_evidence_id{slot};
  value.occurrence = occurrence(slot);
  value.nominal_bits = to_bits((lower + upper) * 0.5);
  value.lower_bits = to_bits(lower);
  value.upper_bits = to_bits(upper);
  value.comparison_evidence_lineage = 1000 + slot;
  return value;
}

void test_definite_order_and_permutation() {
  const double separated = std::nextafter(0.5, 1.0);
  std::vector<bounded_ordering_member> members{
      member(2, separated, separated), member(0, 0.0, 0.0),
      member(1, 0.5, 0.5)};

  bounded_ordering_result result;
  bounded_boolean_error error;
  require(build_bounded_carrier_order<double>(
      members, intersection_checkpoint::source_edge_ordering, result, error));
  require((result.ordered_member_ordinals ==
           std::vector<std::uint64_t>{0, 1, 2}));
  require(result.clusters.size() == 3);
  require(result.certificates.size() == 2);
  require(result.comparison_count == 2);
  require(verify_bounded_carrier_order<double>(
      members, intersection_checkpoint::source_edge_ordering, result, error));

  std::reverse(members.begin(), members.end());
  bounded_ordering_result permuted;
  require(build_bounded_carrier_order<double>(
      members, intersection_checkpoint::source_edge_ordering, permuted, error));
  require(permuted.ordered_member_ordinals == result.ordered_member_ordinals);
  require(permuted.clusters.size() == result.clusters.size());
  require(permuted.certificates.size() == result.certificates.size());
}

void test_exact_and_lineage_clusters() {
  auto first = member(0, 0.25, 0.25);
  auto second = member(1, 0.25, 0.25);
  first.exact_equal_eligible = true;
  second.exact_equal_eligible = true;
  first.exact_evidence_lineage = 90;
  second.exact_evidence_lineage = 90;
  first.topology_interchangeable = true;
  second.topology_interchangeable = true;

  bounded_ordering_result result;
  bounded_boolean_error error;
  require(build_bounded_carrier_order<double>(
      {second, first}, intersection_checkpoint::source_edge_ordering, result,
      error));
  require(result.clusters.size() == 1);
  require(result.clusters[0].members.count == 2);
  require(result.clusters[0].equivalence ==
          intersection_cluster_equivalence::exact_parameter_coincidence);
  require((result.ordered_member_ordinals ==
           std::vector<std::uint64_t>{0, 1}));

  first = member(0, 0.2, 0.4);
  second = member(1, 0.3, 0.5);
  first.unresolved_cluster_eligible = true;
  second.unresolved_cluster_eligible = true;
  first.topology_interchangeable = true;
  second.topology_interchangeable = true;
  first.cluster_lineage = 44;
  second.cluster_lineage = 44;
  require(build_bounded_carrier_order<double>(
      {first, second}, intersection_checkpoint::transverse_carriers, result,
      error));
  require(result.clusters.size() == 1);
  require(result.clusters[0].equivalence ==
          intersection_cluster_equivalence::lineage_authorized_unresolved);
}

void test_fail_closed_cases() {
  bounded_ordering_result result;
  bounded_boolean_error error;

  auto first = member(0, 0.4, 0.6);
  auto second = member(1, 0.5, 0.7);
  require(!build_bounded_carrier_order<double>(
      {first, second}, intersection_checkpoint::source_edge_ordering, result,
      error));
  require(error.subcode == static_cast<std::uint32_t>(
                               intersection_subcode::unresolved_topology_order));
  require(error.category ==
          bounded_boolean_error_category::geometric_condition_exceeds_tolerance);

  first = member(0, 0.5, 0.5);
  second = member(1, 0.5, 0.5);
  require(!build_bounded_carrier_order<double>(
      {first, second}, intersection_checkpoint::source_edge_ordering, result,
      error));
  require(error.subcode == static_cast<std::uint32_t>(
                               intersection_subcode::exact_equal_without_evidence));

  auto a = member(0, 0.0, 0.4);
  auto b = member(1, 0.3, 0.7);
  auto c = member(2, 0.6, 1.0);
  for (auto *value : {&a, &b, &c}) {
    value->unresolved_cluster_eligible = true;
    value->topology_interchangeable = true;
    value->cluster_lineage = 77;
  }
  require(!build_bounded_carrier_order<double>(
      {a, b, c}, intersection_checkpoint::transverse_carriers, result, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::cluster_invalid));
}

void test_verifier_rejects_mutation() {
  bounded_ordering_result result;
  bounded_boolean_error error;
  const std::vector<bounded_ordering_member> members{
      member(0, 0.0, 0.0), member(1, 1.0, 1.0)};
  require(build_bounded_carrier_order<double>(
      members, intersection_checkpoint::source_edge_ordering, result, error));
  require(!result.certificates.empty());
  result.certificates[0].topology_safe = false;
  require(!verify_bounded_carrier_order<double>(
      members, intersection_checkpoint::source_edge_ordering, result, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));
}

} // namespace

int main() {
  test_definite_order_and_permutation();
  test_exact_and_lineage_clusters();
  test_fail_closed_cases();
  test_verifier_rejects_mutation();
  return 0;
}
