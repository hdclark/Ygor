#include "YgorMeshesBooleanBounded/EventIncidence.h"

#include <cstdlib>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
void require(bool value) { if (!value) std::abort(); }

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = primary;
  return key;
}

relation_construction_record construction() {
  relation_construction_record record;
  record.id = relation_construction_id{0};
  record.source_relation = feature_relation_id{0};
  record.kind = relation_construction_kind::bounded_point;
  record.precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  record.coordinate_space = relation_construction_coordinate_space::world_3d;
  record.component_count = 3;
  record.authoritative_source_feature =
      feature(operand_id::a, relation_feature_kind::source_edge, 3);
  record.source_provenance = 10;
  record.geometric_lineage = 20;
  record.finite = true;
  record.tolerance_compatible = true;
  record.precision_evidence_complete = true;
  return record;
}

relation_event_seed_record seed(std::uint64_t ordinal,
                                std::uint32_t occurrence) {
  relation_event_seed_record record;
  record.id = relation_event_seed_id{ordinal};
  record.key.semantic_namespace.bytes[0] = 0x44;
  record.key.family = feature_relation_family::source_edge_source_facet;
  record.key.first =
      feature(operand_id::a, relation_feature_kind::source_edge, 3);
  record.key.second =
      feature(operand_id::b, relation_feature_kind::source_facet, 7);
  record.key.occurrence = occurrence;
  record.source_relation = feature_relation_id{0};
  record.construction = relation_construction_id{0};
  record.contact_status = feature_relation_status::proper_crossing;
  record.contact_dimension = relation_contact_dimension::point;
  record.construction_kind = relation_construction_kind::bounded_point;
  record.numeric_crossing = 1;
  record.incidence_begin = ordinal * 3;
  record.incidence_count = 3;
  record.candidate_incidence_begin = ordinal;
  record.candidate_incidence_count = 1;
  record.precision_evidence_complete = true;
  record.distinct_occurrence_required = occurrence != 0;
  return record;
}

relation_event_seed_candidate_incidence_record candidate_record(
    std::uint64_t ordinal, relation_feature_kind edge_kind,
    bool owner) {
  relation_event_seed_candidate_incidence_record record;
  record.id = relation_event_seed_incidence_id{ordinal};
  record.seed = relation_event_seed_id{ordinal};
  record.candidate = candidate_id{ordinal};
  record.disposition = relation_candidate_disposition_id{ordinal};
  record.candidate_edge = feature(operand_id::a, edge_kind, 3 + ordinal);
  record.source_triangle =
      feature(operand_id::b, relation_feature_kind::source_triangle, 11 + ordinal);
  record.edge_halfedges = {100 + ordinal * 10, 101 + ordinal * 10};
  record.triangle_halfedges = {200 + ordinal * 10, 201 + ordinal * 10,
                               202 + ordinal * 10};
  record.internal_diagonal_witness =
      edge_kind == relation_feature_kind::facet_internal_diagonal;
  record.source_feature_owner = owner;
  return record;
}
} // namespace

int main() {
  std::vector<relation_construction_record> constructions{construction()};
  std::vector<relation_event_seed_record> seeds{seed(0, 0), seed(1, 1)};
  std::vector<relation_feature_key> source_incidence{
      feature(operand_id::a, relation_feature_kind::source_edge, 3),
      feature(operand_id::b, relation_feature_kind::source_facet, 7),
      feature(operand_id::b, relation_feature_kind::source_triangle, 11),
      feature(operand_id::a, relation_feature_kind::source_edge, 3),
      feature(operand_id::b, relation_feature_kind::source_facet, 7),
      feature(operand_id::b, relation_feature_kind::source_triangle, 12)};
  std::vector<relation_event_seed_candidate_incidence_record> candidates{
      candidate_record(0, relation_feature_kind::source_edge, true),
      candidate_record(1, relation_feature_kind::facet_internal_diagonal,
                       false)};

  bounded_boolean_error error;
  std::vector<normalized_event_seed_proposal> proposals;
  require(normalize_event_seed_records(seeds, constructions, proposals, error));
  event_interning_tables interning;
  require(intern_normalized_event_seeds(proposals, interning, error));

  event_incidence_tables tables;
  require(build_event_incidence_records(
      seeds, source_incidence, candidates, 1, 2, interning, tables, error));
  require(!tables.records.empty());
  require(tables.event_ranges.size() == interning.events.size());
  require(tables.occurrence_ranges.size() == interning.occurrences.size());
  require(tables.seed_ranges.size() == seeds.size());
  require(tables.relation_ranges.size() == 1);
  require(tables.candidate_ranges.size() == 2);
  require(tables.halfedge_ranges.size() == 10);
  require(tables.by_event.size() == tables.records.size());
  for (const auto &record : tables.records) {
    if (record.feature.kind == relation_feature_kind::source_triangle ||
        record.feature.kind == relation_feature_kind::facet_internal_diagonal ||
        record.kind == event_incidence_kind::oriented_halfedge) {
      require(record.bookkeeping_only);
      require(!record.source_feature_owner);
    }
  }

  auto mutated = tables;
  std::swap(mutated.by_event.front(), mutated.by_event.back());
  require(!verify_event_incidence_records(
      seeds, source_incidence, candidates, 1, 2, interning, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::incidence_incomplete));

  auto bad_candidates = candidates;
  bad_candidates[1].source_feature_owner = true;
  require(!build_event_incidence_records(
      seeds, source_incidence, bad_candidates, 1, 2, interning, tables, error));
  require(error.subcode == static_cast<std::uint32_t>(
                               intersection_subcode::internal_diagonal_public_ownership));

  auto missing = seeds;
  missing[0].incidence_count = 100;
  require(!build_event_incidence_records(
      missing, source_incidence, candidates, 1, 2, interning, tables, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::incidence_incomplete));
  return 0;
}
