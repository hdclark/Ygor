#include "YgorMeshesBooleanBounded/CoplanarCarrierArrangements.h"

#include "YgorMeshesBooleanBounded/FloatingBits.h"
#include "YgorMeshesBooleanBounded/Sha256.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
void require_impl(bool value, int line) {
  if (!value) {
    std::fprintf(stderr, "require failed at line %d\n", line);
    std::abort();
  }
}
#define require(value) require_impl((value), __LINE__)

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary,
                             std::uint64_t secondary = 0) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = primary;
  key.secondary = secondary;
  return key;
}

bounded_boolean_digest digest(std::uint8_t seed) {
  bounded_boolean_digest value;
  value.bytes[0] = seed;
  value.bytes[31] = static_cast<std::uint8_t>(seed ^ 0xa5U);
  return value;
}

intersection_occurrence_key occurrence(std::uint32_t slot,
                                       occurrence_role role =
                                           occurrence_role::overlap_boundary_occurrence) {
  intersection_occurrence_key key;
  key.event.semantic_namespace.bytes[0] = 0xc0;
  key.event.semantic_namespace.bytes[1] = 0x08;
  key.event.event_class = intersection_event_class::overlap_endpoint;
  key.event.first_operand = operand_id::a;
  key.event.second_operand = operand_id::b;
  key.event.first_owner =
      feature(operand_id::a, relation_feature_kind::source_edge, 10);
  key.event.second_owner =
      feature(operand_id::b, relation_feature_kind::source_edge, 20);
  key.event.public_relation.semantic_namespace = key.event.semantic_namespace;
  key.event.public_relation.family =
      feature_relation_family::source_edge_source_edge;
  key.event.public_relation.first = key.event.first_owner;
  key.event.public_relation.second = key.event.second_owner;
  key.event.public_relation.occurrence = slot + 1;
  key.event.construction_kind = relation_construction_kind::bounded_point;
  key.event.construction_precedence =
      relation_construction_precedence::source_edge_source_edge_point;
  key.event.authoritative_source_feature = key.event.first_owner;
  key.event.construction_source_provenance = 1000 + slot;
  key.event.construction_geometric_lineage = 2000 + slot;
  key.event.carrier_role = slot == 0 ? intersection_carrier_role::overlap_start
                                     : intersection_carrier_role::overlap_end;
  key.event.contact_status = feature_relation_status::overlap;
  key.event.contact_dimension = relation_contact_dimension::curve;
  key.discriminator.role = role;
  key.discriminator.component07_occurrence = slot + 1;
  key.discriminator.occurrence_lineage = 3000 + slot;
  return key;
}

coplanar_support_key support_key(std::uint64_t lineage,
                                 bool opposite = false) {
  coplanar_support_key key;
  key.first_facet =
      feature(operand_id::a, relation_feature_kind::source_facet, 3);
  key.second_facet =
      feature(operand_id::b, relation_feature_kind::source_facet, 7);
  key.support_lineage = lineage;
  key.opposite_orientation = opposite;
  key.symbolic_owner = operand_id::a;
  return key;
}

coplanar_support_proposal support_proposal(coplanar_support_key key,
                                           std::uint64_t relation,
                                           std::uint64_t candidate,
                                           bool authority) {
  coplanar_support_proposal proposal;
  proposal.key = key;
  proposal.relation = feature_relation_id{relation};
  proposal.candidate = candidate_id{candidate};
  proposal.first_source_facet_semantic_digest = digest(0x31);
  proposal.second_source_facet_semantic_digest = digest(0x72);
  proposal.original_boundary_edges = {
      feature(operand_id::a, relation_feature_kind::source_edge, 10),
      feature(operand_id::a, relation_feature_kind::source_edge, 11),
      feature(operand_id::b, relation_feature_kind::source_edge, 20),
      feature(operand_id::b, relation_feature_kind::source_edge, 21)};
  proposal.designated_authority = authority;
  proposal.support_consistent = true;
  proposal.orientation_consistent = true;
  proposal.symbolic_policy_consistent = true;
  proposal.precision_evidence_complete = true;
  return proposal;
}

bounded_boolean_digest combined_digest(const coplanar_support_proposal &proposal) {
  canonical_writer writer;
  encode_coplanar_support_key(writer, proposal.key);
  for (auto byte : proposal.first_source_facet_semantic_digest.bytes)
    writer.u8(byte);
  for (auto byte : proposal.second_source_facet_semantic_digest.bytes)
    writer.u8(byte);
  return sha256::digest(writer.bytes());
}

collinear_overlap_carrier_key carrier_key(coplanar_support_key support,
                                           std::uint64_t lineage,
                                           bool opposite = false) {
  collinear_overlap_carrier_key key;
  key.first_edge =
      feature(operand_id::a, relation_feature_kind::source_edge, 10);
  key.second_edge =
      feature(operand_id::b, relation_feature_kind::source_edge, 20);
  key.coplanar_support_lineage = support.support_lineage;
  key.overlap_lineage = lineage;
  key.opposite_direction = opposite;
  key.symbolic_owner = support.symbolic_owner;
  return key;
}

collinear_overlap_carrier_proposal carrier_proposal(
    collinear_overlap_carrier_key key, std::uint64_t slot,
    event_occurrence_id start, event_occurrence_id end,
    bool zero_length = false) {
  collinear_overlap_carrier_proposal proposal;
  proposal.key = key;
  proposal.first_parameter_interval = relation_construction_id{100 + slot};
  proposal.second_parameter_interval = relation_construction_id{200 + slot};
  proposal.first_parameter_evidence = relation_interval_evidence_id{300 + slot};
  proposal.second_parameter_evidence = relation_interval_evidence_id{400 + slot};
  const double lower = zero_length ? 0.5 : 0.25;
  const double upper = zero_length ? 0.5 : 0.75;
  proposal.first_lower_bits = to_bits(lower);
  proposal.first_upper_bits = to_bits(upper);
  proposal.second_lower_bits = to_bits(lower);
  proposal.second_upper_bits = to_bits(upper);
  proposal.first_domain = zero_length ? parameter_domain_status::stable_interior
                                      : parameter_domain_status::stable_interior;
  proposal.second_domain = proposal.first_domain;
  proposal.start_occurrence = start;
  proposal.end_occurrence = end;
  proposal.start_occurrence_key = occurrence(start.ordinal());
  proposal.end_occurrence_key = occurrence(end.ordinal());
  proposal.start_source_vertex =
      feature(operand_id::a, relation_feature_kind::source_vertex, 1 + slot);
  proposal.end_source_vertex = zero_length
                                   ? proposal.start_source_vertex
                                   : feature(operand_id::b,
                                             relation_feature_kind::source_vertex,
                                             2 + slot);
  proposal.relation = feature_relation_id{500 + slot};
  proposal.candidate = candidate_id{600 + slot};
  proposal.source_provenance = {
      feature(operand_id::a, relation_feature_kind::source_facet, 3),
      feature(operand_id::a, relation_feature_kind::sheet_occurrence, 11),
      feature(operand_id::b, relation_feature_kind::source_facet, 7),
      feature(operand_id::b, relation_feature_kind::sheet_occurrence, 21)};
  proposal.first_original_source_edge = true;
  proposal.second_original_source_edge = true;
  proposal.first_direction_valid = true;
  proposal.second_direction_valid = true;
  proposal.parameter_correspondence_verified = true;
  proposal.endpoint_ownership_verified = true;
  proposal.half_open_first = !zero_length;
  proposal.half_open_second = false;
  proposal.half_open_policy_consistent = true;
  proposal.exact_zero_length = zero_length;
  proposal.definitely_positive_length = !zero_length;
  proposal.separate_sheet_required = true;
  return proposal;
}

coplanar_overlap_key overlap_key(coplanar_support_key support,
                                 std::uint64_t lineage,
                                 relation_coplanar_component_kind kind,
                                 std::uint8_t sheet_mask = 3) {
  coplanar_overlap_key key;
  key.support = support;
  key.component_lineage = lineage;
  key.component_kind = kind;
  key.sheet_mask = sheet_mask;
  key.symbolic_owner = support.symbolic_owner;
  return key;
}

coplanar_overlap_component_proposal overlap_proposal(
    coplanar_overlap_key key, std::uint64_t component_id,
    std::vector<event_occurrence_id> events,
    std::vector<collinear_overlap_carrier_key> carriers,
    bool zero_measure = false) {
  coplanar_overlap_component_proposal proposal;
  proposal.key = key;
  proposal.component07_component =
      relation_coplanar_overlap_component_id{component_id};
  proposal.relation = feature_relation_id{700 + component_id};
  proposal.boundary_events = std::move(events);
  proposal.boundary_carriers = std::move(carriers);
  proposal.closed = key.component_kind ==
                        relation_coplanar_component_kind::area_boundary ||
                    key.component_kind == relation_coplanar_component_kind::coincident_sheet_boundary;
  proposal.distinct_sheet_occurrences = key.sheet_mask == 3;
  proposal.zero_measure = zero_measure;
  proposal.component_assembly_complete = true;
  return proposal;
}

coplanar_region_incidence_proposal region_proposal(
    const coplanar_support_proposal &support,
    const coplanar_overlap_component_proposal &component,
    std::vector<event_occurrence_id> events,
    std::vector<collinear_overlap_carrier_key> carriers,
    coplanar_region_classification classification,
    feature_relation_status status) {
  coplanar_region_incidence_proposal proposal;
  proposal.support = support.key;
  proposal.component = component.key;
  proposal.first_facet = support.key.first_facet;
  proposal.second_facet = support.key.second_facet;
  proposal.first_triangle =
      feature(operand_id::a, relation_feature_kind::source_triangle, 31, 3);
  proposal.second_triangle =
      feature(operand_id::b, relation_feature_kind::source_triangle, 71, 7);
  proposal.classification = classification;
  proposal.relation_status = status;
  proposal.symbolic_owner = support.key.symbolic_owner;
  proposal.sheet_mask = component.key.sheet_mask;
  proposal.boundary_events = std::move(events);
  proposal.boundary_carriers = std::move(carriers);
  proposal.coverage_witnesses = {
      proposal.first_triangle,
      feature(operand_id::a, relation_feature_kind::facet_internal_diagonal,
              91, 3),
      proposal.second_triangle};
  proposal.source_facet_semantic_digest = combined_digest(support);
  proposal.coverage_complete = true;
  proposal.internal_diagonals_coverage_only = true;
  proposal.source_facet_semantics_verified = true;
  return proposal;
}

void require_failure(
    const std::vector<coplanar_support_proposal> &supports,
    const std::vector<collinear_overlap_carrier_proposal> &carriers,
    const std::vector<coplanar_overlap_component_proposal> &overlaps,
    const std::vector<coplanar_region_incidence_proposal> &regions) {
  coplanar_carrier_arrangement_tables tables;
  bounded_boolean_error error;
  require(!build_coplanar_carrier_arrangements<double>(
      supports, carriers, overlaps, regions, tables, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::overlap_carrier_invalid));
}
} // namespace

int main() {
  const auto support_key0 = support_key(9001, false);
  auto authority = support_proposal(support_key0, 10, 20, true);
  auto witness = support_proposal(support_key0, 10, 21, false);
  const auto carrier0_key = carrier_key(support_key0, 9101, false);
  auto carrier0 = carrier_proposal(carrier0_key, 1, event_occurrence_id{0},
                                   event_occurrence_id{1});
  const auto component0_key = overlap_key(
      support_key0, 9201, relation_coplanar_component_kind::boundary_segment);
  auto component0 = overlap_proposal(component0_key, 1,
                                     {event_occurrence_id{0},
                                      event_occurrence_id{1}},
                                     {carrier0_key});
  auto region0 = region_proposal(
      authority, component0,
      {event_occurrence_id{0}, event_occurrence_id{1}}, {carrier0_key},
      coplanar_region_classification::segment_contact,
      feature_relation_status::segment_contact);

  const auto zero_key = carrier_key(support_key0, 9102, true);
  auto zero_carrier = carrier_proposal(zero_key, 2, event_occurrence_id{2},
                                       event_occurrence_id{2}, true);
  const auto zero_component_key = overlap_key(
      support_key0, 9202, relation_coplanar_component_kind::boundary_segment);
  auto zero_component = overlap_proposal(
      zero_component_key, 2, {event_occurrence_id{2}}, {zero_key}, true);
  auto zero_region = region_proposal(
      authority, zero_component, {event_occurrence_id{2}}, {zero_key},
      coplanar_region_classification::point_contact,
      feature_relation_status::point_contact);

  std::vector<coplanar_support_proposal> supports{witness, authority};
  std::vector<collinear_overlap_carrier_proposal> carriers{zero_carrier,
                                                            carrier0};
  std::vector<coplanar_overlap_component_proposal> overlaps{zero_component,
                                                             component0};
  std::vector<coplanar_region_incidence_proposal> regions{zero_region, region0};
  coplanar_carrier_arrangement_tables tables;
  bounded_boolean_error error;
  require(build_coplanar_carrier_arrangements<double>(
      supports, carriers, overlaps, regions, tables, error));
  require(tables.supports.size() == 1);
  require(tables.carriers.size() == 2);
  require(tables.overlaps.size() == 2);
  require(tables.regions.size() == 2);
  require(tables.supports[0].original_boundary_edges.count == 4);
  require(tables.supports[0].boundary_events.count == 3);
  require(tables.supports[0].boundary_carriers.count == 2);
  require(tables.supports[0].overlap_components.count == 2);
  require(tables.supports[0].region_incidence.count == 2);
  require(!tables.carriers[0].zero_length);
  require(tables.carriers[1].zero_length);
  require(tables.overlaps[1].zero_measure);
  require(tables.regions[0].internal_diagonal_coverage_only);
  require(tables.regions[0].coverage_complete);
  require(verify_coplanar_carrier_arrangements<double>(
      supports, carriers, overlaps, regions, tables, error));

  auto permuted_supports = supports;
  auto permuted_carriers = carriers;
  auto permuted_overlaps = overlaps;
  auto permuted_regions = regions;
  std::reverse(permuted_supports.begin(), permuted_supports.end());
  std::reverse(permuted_carriers.begin(), permuted_carriers.end());
  std::reverse(permuted_overlaps.begin(), permuted_overlaps.end());
  std::reverse(permuted_regions.begin(), permuted_regions.end());
  coplanar_carrier_arrangement_tables permuted;
  require(build_coplanar_carrier_arrangements<double>(
      permuted_supports, permuted_carriers, permuted_overlaps,
      permuted_regions, permuted, error));
  require(verify_coplanar_carrier_arrangements<double>(
      supports, carriers, overlaps, regions, permuted, error));

  auto mutated = tables;
  mutated.carriers[0].parameter_correspondence_verified = false;
  require(!verify_coplanar_carrier_arrangements<double>(
      supports, carriers, overlaps, regions, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  auto bad = carrier0;
  bad.parameter_correspondence_verified = false;
  require_failure(supports, {bad, zero_carrier}, overlaps, regions);
  bad = carrier0;
  bad.first_original_source_edge = false;
  require_failure(supports, {bad, zero_carrier}, overlaps, regions);
  bad = carrier0;
  bad.source_provenance.push_back(feature(
      operand_id::a, relation_feature_kind::facet_internal_diagonal, 99, 3));
  require_failure(supports, {bad, zero_carrier}, overlaps, regions);
  bad = carrier0;
  bad.half_open_second = true;
  require_failure(supports, {bad, zero_carrier}, overlaps, regions);

  auto bad_zero = zero_carrier;
  bad_zero.end_occurrence = event_occurrence_id{3};
  require_failure(supports, {carrier0, bad_zero}, overlaps, regions);

  auto bad_region = region0;
  bad_region.internal_diagonals_coverage_only = false;
  require_failure(supports, carriers, overlaps, {bad_region, zero_region});
  bad_region = region0;
  bad_region.source_facet_semantic_digest.bytes[0] ^= 1U;
  require_failure(supports, carriers, overlaps, {bad_region, zero_region});

  auto duplicate = component0;
  require_failure(supports, carriers, {component0, duplicate, zero_component},
                  regions);

  const auto remapped_support = remap_coplanar_support_key(support_key0);
  require(remap_coplanar_support_key(remapped_support) == support_key0);
  const auto remapped_component = remap_coplanar_overlap_key(component0_key);
  require(remap_coplanar_overlap_key(remapped_component) == component0_key);
  const auto remapped_carrier = remap_collinear_overlap_carrier_key(carrier0_key);
  require(remap_collinear_overlap_carrier_key(remapped_carrier) == carrier0_key);

  return 0;
}
