#include "YgorMeshesBooleanBounded/CanonicalIntersectionComplex.h"
#include "YgorMeshesBooleanBounded/IntersectionKeys.h"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
void require(bool condition) {
  if (!condition)
    std::abort();
}
} // namespace

namespace {

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary,
                             std::uint64_t secondary = 0) {
  relation_feature_key out;
  out.operand = operand;
  out.kind = kind;
  out.primary = primary;
  out.secondary = secondary;
  return out;
}

intersection_event_key event_key(std::uint64_t lineage = 11) {
  intersection_event_key key;
  key.semantic_namespace.bytes[0] = 0x42;
  key.event_class = intersection_event_class::edge_facet_point;
  key.first_operand = operand_id::a;
  key.second_operand = operand_id::b;
  key.first_owner = feature(operand_id::a, relation_feature_kind::source_edge, 3);
  key.second_owner = feature(operand_id::b, relation_feature_kind::source_facet, 7);
  key.public_relation.semantic_namespace = key.semantic_namespace;
  key.public_relation.family = feature_relation_family::source_edge_source_facet;
  key.public_relation.first = key.first_owner;
  key.public_relation.second = key.second_owner;
  key.public_relation.occurrence = 2;
  key.construction_kind = relation_construction_kind::bounded_point;
  key.construction_precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  key.authoritative_source_feature = key.first_owner;
  key.construction_source_provenance = 9;
  key.construction_geometric_lineage = lineage;
  key.carrier_role = intersection_carrier_role::transverse_endpoint;
  key.contact_status = feature_relation_status::proper_crossing;
  key.contact_dimension = relation_contact_dimension::point;
  return key;
}

intersection_occurrence_key occurrence_key(std::uint64_t lineage = 11) {
  intersection_occurrence_key key;
  key.event = event_key(lineage);
  key.discriminator.role = occurrence_role::single_occurrence;
  key.discriminator.component07_occurrence = 2;
  return key;
}

void test_domains_and_closed_values() {
  static_assert(!std::is_same_v<event_id, event_occurrence_id>);
  static_assert(!std::is_same_v<source_edge_membership_id,
                                carrier_membership_id>);
  static_assert(static_cast<std::uint32_t>(
                    intersection_checkpoint::transaction_commit) == 24);
  static_assert(static_cast<std::uint32_t>(
                    intersection_subcode::unsupported_version) == 80001);
  require(contract_versions::intersection_provider != 0);
  require(contract_versions::intersection_section_digest_layout != 0);
}

void test_event_and_occurrence_identity() {
  auto event = event_key();
  require(valid_intersection_event_key(event));
  auto occurrence = occurrence_key();
  require(valid_intersection_occurrence_key(occurrence));

  auto separate = occurrence;
  separate.discriminator.role = occurrence_role::topology_separated_contact;
  separate.discriminator.occurrence_lineage = 91;
  require(valid_intersection_occurrence_key(separate));
  require(separate.event == occurrence.event);
  require(!(separate == occurrence));

  auto coordinate_coincident_but_distinct = event_key(12);
  require(valid_intersection_event_key(coordinate_coincident_but_distinct));
  require(!(coordinate_coincident_but_distinct == event));
}

void test_forbidden_and_malformed_keys() {
  auto key = event_key();
  key.first_owner.kind = relation_feature_kind::source_triangle;
  require(!valid_intersection_event_key(key));

  key = event_key();
  key.construction_geometric_lineage = 0;
  require(!valid_intersection_event_key(key));

  auto occurrence = occurrence_key();
  occurrence.discriminator.role = occurrence_role::multiplicity_occurrence;
  occurrence.discriminator.multiplicity_slot = 0;
  occurrence.discriminator.occurrence_lineage = 77;
  require(!valid_intersection_occurrence_key(occurrence));

  source_edge_cluster_key cluster;
  cluster.source_edge =
      feature(operand_id::a, relation_feature_kind::source_edge, 3);
  cluster.members = {occurrence_key(), occurrence_key()};
  require(!valid_source_edge_cluster_key(cluster));
}

void test_canonical_encoding_and_order() {
  auto first = event_key(11);
  auto second = event_key(12);
  const auto first_bytes = encode_intersection_event_key(first);
  const auto second_bytes = encode_intersection_event_key(second);
  require(first < second);
  require(first_bytes < second_bytes);
  require(first_bytes == encode_intersection_event_key(first));
}

void test_operand_exchange_involution() {
  auto event = event_key();
  require(remap_intersection_event_key(remap_intersection_event_key(event)) ==
         event);
  auto occurrence = occurrence_key();
  require(remap_intersection_occurrence_key(
             remap_intersection_occurrence_key(occurrence)) == occurrence);

  transverse_carrier_key carrier;
  carrier.first_facet =
      feature(operand_id::a, relation_feature_kind::source_facet, 1);
  carrier.second_facet =
      feature(operand_id::b, relation_feature_kind::source_facet, 2);
  carrier.construction = relation_construction_id(4);
  carrier.construction_lineage = 99;
  require(valid_transverse_carrier_key(carrier));
  require(remap_transverse_carrier_key(remap_transverse_carrier_key(carrier)) ==
         carrier);
}

void test_source_direction_involution() {
  source_edge_membership_key membership;
  membership.source_edge =
      feature(operand_id::a, relation_feature_kind::source_edge, 3);
  membership.occurrence = occurrence_key();
  membership.role = intersection_membership_role::overlap_start;
  membership.parameter_evidence = relation_interval_evidence_id(8);
  membership.parameter_lineage = 10;
  membership.relation_lineage = 12;
  membership.overlap_lineage = 14;
  membership.facet_use_role = source_facet_use_role::left_incident;
  require(valid_source_edge_membership_key(membership));
  require(reverse_source_edge_membership_key(
             reverse_source_edge_membership_key(membership)) == membership);

  source_edge_cluster_key cluster;
  cluster.source_edge = membership.source_edge;
  cluster.members = {occurrence_key()};
  require(valid_source_edge_cluster_key(cluster));

  source_edge_interval_key interval;
  interval.source_edge = membership.source_edge;
  interval.left.kind = boundary_reference_kind::start_sentinel;
  interval.right.kind = boundary_reference_kind::cluster;
  interval.right.cluster = cluster;
  require(valid_source_edge_interval_key(interval));
  require(reverse_source_edge_interval_key(
             reverse_source_edge_interval_key(interval)) == interval);
}

void test_interval_validity_uses_semantic_sequence_order() {
  const auto edge =
      feature(operand_id::a, relation_feature_kind::source_edge, 3);

  source_edge_cluster_key lexical_later;
  lexical_later.source_edge = edge;
  lexical_later.members = {occurrence_key(91)};
  require(valid_source_edge_cluster_key(lexical_later));

  source_edge_cluster_key lexical_earlier;
  lexical_earlier.source_edge = edge;
  lexical_earlier.members = {occurrence_key(17)};
  require(valid_source_edge_cluster_key(lexical_earlier));
  require(lexical_earlier < lexical_later);

  source_edge_interval_key interval;
  interval.source_edge = edge;
  interval.left.kind = boundary_reference_kind::cluster;
  interval.left.cluster = lexical_later;
  interval.right.kind = boundary_reference_kind::cluster;
  interval.right.cluster = lexical_earlier;
  interval.canonical_ordinal = 4;

  // The interval is valid because bounded parameter evidence, not lineage-key
  // lexicography, establishes sequence order.
  require(valid_source_edge_interval_key(interval));
}

void test_owner_checked_artifact_view() {
  static_assert(std::is_final_v<canonical_intersection_complex<float,
                                                               std::uint32_t>>);
}

} // namespace

int main() {
  test_domains_and_closed_values();
  test_event_and_occurrence_identity();
  test_forbidden_and_malformed_keys();
  test_canonical_encoding_and_order();
  test_operand_exchange_involution();
  test_source_direction_involution();
  test_interval_validity_uses_semantic_sequence_order();
  test_owner_checked_artifact_view();
  return 0;
}
