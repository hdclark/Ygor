#include "YgorMeshesBooleanBounded/EventInterning.h"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
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
                             std::uint64_t primary) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = primary;
  return key;
}

relation_event_seed_key seed_key(std::uint64_t edge, std::uint32_t occurrence) {
  relation_event_seed_key key;
  key.semantic_namespace.bytes[0] = 0x5a;
  key.family = feature_relation_family::source_edge_source_facet;
  key.first = feature(operand_id::a, relation_feature_kind::source_edge, edge);
  key.second = feature(operand_id::b, relation_feature_kind::source_facet, 7);
  key.occurrence = occurrence;
  return key;
}

relation_construction_record construction(std::uint64_t ordinal,
                                          std::uint64_t relation,
                                          std::uint64_t edge,
                                          std::uint64_t lineage) {
  relation_construction_record record;
  record.id = relation_construction_id{ordinal};
  record.source_relation = feature_relation_id{relation};
  record.kind = relation_construction_kind::bounded_point;
  record.precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  record.coordinate_space = relation_construction_coordinate_space::world_3d;
  record.component_count = 3;
  record.authoritative_source_feature =
      feature(operand_id::a, relation_feature_kind::source_edge, edge);
  record.nominal_bits[0] = 1;
  record.lower_bits[0] = 1;
  record.upper_bits[0] = 1;
  record.source_provenance = 100 + edge;
  record.geometric_lineage = lineage;
  record.finite = true;
  record.tolerance_compatible = true;
  record.precision_evidence_complete = true;
  return record;
}

relation_event_seed_record seed(std::uint64_t ordinal, std::uint64_t edge,
                                std::uint32_t occurrence,
                                std::uint64_t construction_id,
                                std::uint64_t relation,
                                bool distinct) {
  relation_event_seed_record record;
  record.id = relation_event_seed_id{ordinal};
  record.key = seed_key(edge, occurrence);
  record.source_relation = feature_relation_id{relation};
  record.construction = relation_construction_id{construction_id};
  record.contact_status = feature_relation_status::proper_crossing;
  record.contact_dimension = relation_contact_dimension::point;
  record.construction_kind = relation_construction_kind::bounded_point;
  record.numeric_crossing = 1;
  record.half_open_owner = operand_id::a;
  record.precision_evidence_complete = true;
  record.distinct_occurrence_required = distinct;
  return record;
}

} // namespace

int main() {
  std::vector<relation_construction_record> constructions{
      construction(0, 0, 3, 1000), construction(1, 1, 9, 2000)};
  std::vector<relation_event_seed_record> seeds{
      seed(0, 3, 0, 0, 0, false),
      seed(1, 3, 1, 0, 0, true),
      seed(2, 9, 0, 1, 1, false)};

  bounded_boolean_error error;
  std::vector<normalized_event_seed_proposal> proposals;
  require(normalize_event_seed_records(seeds, constructions, proposals, error));
  require(proposals.size() == 3);
  require(proposals[0].event_key == proposals[1].event_key);
  require(!(proposals[0].occurrence_key == proposals[1].occurrence_key));
  require(proposals[0].event_key != proposals[2].event_key);

  event_interning_tables tables;
  require(intern_normalized_event_seeds(proposals, tables, error));
  require(tables.events.size() == 2);
  require(tables.occurrences.size() == 3);
  require(tables.seed_bindings.size() == 3);
  require(tables.seed_to_event[0] == tables.seed_to_event[1]);
  require(tables.seed_to_occurrence[0] != tables.seed_to_occurrence[1]);
  require(tables.seed_to_event[0] != tables.seed_to_event[2]);
  require(tables.events[0].occurrences.count == 2);
  require(tables.events[0].seed_bindings.count == 2);
  require(tables.seed_bindings[0].canonical_seed_ordinal == 0);
  require(tables.seed_bindings[1].canonical_seed_ordinal == 1);
  require(tables.seed_bindings[2].canonical_seed_ordinal == 2);

  auto permuted = proposals;
  std::reverse(permuted.begin(), permuted.end());
  event_interning_tables permuted_tables;
  require(intern_normalized_event_seeds(permuted, permuted_tables, error));
  require(permuted_tables.events.size() == tables.events.size());
  require(permuted_tables.occurrences.size() == tables.occurrences.size());
  for (std::size_t i = 0; i < tables.events.size(); ++i)
    require(permuted_tables.events[i].key == tables.events[i].key);
  for (std::size_t i = 0; i < tables.occurrences.size(); ++i)
    require(permuted_tables.occurrences[i].key == tables.occurrences[i].key);
  require(permuted_tables.seed_to_event == tables.seed_to_event);
  require(permuted_tables.seed_to_occurrence == tables.seed_to_occurrence);

  auto duplicate = proposals;
  duplicate.push_back(proposals.front());
  duplicate.back().seed = relation_event_seed_id{3};
  require(!intern_normalized_event_seeds(duplicate, permuted_tables, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(
             intersection_subcode::duplicate_event_incompatible));

  auto conflict = proposals;
  conflict[1].construction = relation_construction_id{1};
  require(!intern_normalized_event_seeds(conflict, permuted_tables, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(
             intersection_subcode::authoritative_construction_conflict));

  auto malformed = seeds;
  malformed[0].key.first.kind = relation_feature_kind::facet_internal_diagonal;
  require(!normalize_event_seed_records(malformed, constructions, proposals, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(
             intersection_subcode::internal_diagonal_public_ownership));
  return 0;
}
