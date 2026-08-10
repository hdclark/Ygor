#include "YgorMeshesBooleanBounded/EventCoordinates.h"

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

relation_construction_record make_construction() {
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
  record.nominal_bits[0] = 10;
  record.lower_bits[0] = 9;
  record.upper_bits[0] = 11;
  record.source_provenance = 100;
  record.geometric_lineage = 200;
  record.finite = true;
  record.tolerance_compatible = true;
  record.precision_evidence_complete = true;
  record.tolerance_boundary_bits = 400;
  record.ledger_begin = 0;
  record.ledger_count = 2;
  return record;
}

relation_construction_ledger_record make_ledger(std::uint64_t ordinal,
                                                std::uint32_t occurrence) {
  relation_construction_ledger_record record;
  record.id = relation_construction_ledger_id{ordinal};
  record.construction = relation_construction_id{0};
  record.source_relation = feature_relation_id{0};
  record.precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  record.coordinate_space = relation_construction_coordinate_space::world_3d;
  record.component_count = 3;
  record.occurrence = occurrence;
  record.nominal_bits[0] = 10;
  record.lower_bits[0] = 9;
  record.upper_bits[0] = 11;
  record.source_provenance = 100;
  record.geometric_lineage = 200;
  record.finite = true;
  record.tolerance_compatible = true;
  record.synthetic_authority = ordinal == 0;
  record.lineage_compatible = true;
  record.enclosure_compatible = true;
  record.parameter_compatible = true;
  record.residual_compatible = true;
  record.precision_evidence_complete = true;
  record.tolerance_boundary_bits = 400;
  return record;
}

relation_event_seed_record make_seed(std::uint64_t ordinal,
                                     std::uint32_t occurrence,
                                     bool distinct) {
  relation_event_seed_record record;
  record.id = relation_event_seed_id{ordinal};
  record.key.semantic_namespace.bytes[0] = 0x71;
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
  record.construction_ledger_begin = 0;
  record.construction_ledger_count = 2;
  record.precision_evidence_complete = true;
  record.distinct_occurrence_required = distinct;
  return record;
}

} // namespace

int main() {
  std::vector<relation_construction_record> constructions{make_construction()};
  std::vector<relation_construction_ledger_record> ledger{
      make_ledger(0, 0), make_ledger(1, 1)};
  std::vector<relation_event_seed_record> seeds{
      make_seed(0, 0, false), make_seed(1, 1, true)};

  bounded_boolean_error error;
  std::vector<normalized_event_seed_proposal> proposals;
  require(normalize_event_seed_records(seeds, constructions, proposals, error));
  event_interning_tables interning;
  require(intern_normalized_event_seeds(proposals, interning, error));
  require(interning.events.size() == 1);

  event_coordinate_tables coordinates;
  require(attach_event_coordinates(proposals, constructions, ledger, interning,
                                  coordinates, error));
  require(coordinates.construction_witness_index.size() == 2);
  require(interning.events[0].point.kind ==
         bounded_point_reference_kind::constructed_point);
  require(interning.events[0].point.construction ==
         relation_construction_id{0});
  require(interning.events[0].point.precision_ledger ==
         relation_construction_ledger_id{0});
  require(interning.events[0].construction_witnesses.begin == 0);
  require(interning.events[0].construction_witnesses.count == 2);

  auto bad_ledger = ledger;
  bad_ledger[1].enclosure_compatible = false;
  event_interning_tables bad_interning;
  require(intern_normalized_event_seeds(proposals, bad_interning, error));
  require(!attach_event_coordinates(proposals, constructions, bad_ledger,
                                   bad_interning, coordinates, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(
             intersection_subcode::secondary_witness_incompatible));

  auto missing_use = ledger;
  missing_use[1].occurrence = 9;
  require(intern_normalized_event_seeds(proposals, bad_interning, error));
  require(!attach_event_coordinates(proposals, constructions, missing_use,
                                   bad_interning, coordinates, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(
             intersection_subcode::secondary_witness_incompatible));
  return 0;
}
