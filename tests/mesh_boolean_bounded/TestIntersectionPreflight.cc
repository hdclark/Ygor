#include "YgorMeshesBooleanBounded/IntersectionPreflight.h"

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

relation_construction_record construction() {
  relation_construction_record value;
  value.id = relation_construction_id{0};
  value.source_relation = feature_relation_id{0};
  value.kind = relation_construction_kind::bounded_point;
  value.precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  value.component_count = 3;
  value.authoritative_source_feature =
      feature(operand_id::a, relation_feature_kind::source_edge, 1);
  value.geometric_lineage = 4;
  value.finite = true;
  value.tolerance_compatible = true;
  value.precision_evidence_complete = true;
  value.ledger_begin = 0;
  value.ledger_count = 1;
  return value;
}

relation_event_seed_record seed() {
  relation_event_seed_record value;
  value.id = relation_event_seed_id{0};
  value.key.semantic_namespace.bytes[0] = 1;
  value.key.family = feature_relation_family::source_edge_source_facet;
  value.key.first =
      feature(operand_id::a, relation_feature_kind::source_edge, 1);
  value.key.second =
      feature(operand_id::b, relation_feature_kind::source_facet, 2);
  value.source_relation = feature_relation_id{0};
  value.construction = relation_construction_id{0};
  value.contact_status = feature_relation_status::proper_crossing;
  value.contact_dimension = relation_contact_dimension::point;
  value.construction_kind = relation_construction_kind::bounded_point;
  value.construction_ledger_begin = 0;
  value.construction_ledger_count = 1;
  value.incidence_begin = 0;
  value.incidence_count = 2;
  value.candidate_incidence_begin = 0;
  value.candidate_incidence_count = 1;
  value.precision_evidence_complete = true;
  return value;
}

} // namespace

int main() {
  std::vector<relation_construction_record> constructions{construction()};
  std::vector<relation_event_seed_record> seeds{seed()};
  intersection_capabilities capabilities;
  intersection_preflight_plan plan;
  bounded_boolean_error error;

  require(preflight_intersection_event_records(seeds, constructions, 1, 2, 1,
                                              capabilities, plan, error));
  require(plan.estimate.event_count == 1);
  require(plan.estimate.occurrence_count == 1);
  require(plan.estimate.seed_binding_count == 1);
  require(plan.estimate.incidence_count == 5);
  require(plan.estimate.persistent_bytes != 0);
  require(plan.estimate.temporary_bytes != 0);

  auto limited = capabilities;
  limited.maximum_events = 0;
  require(!preflight_intersection_event_records(seeds, constructions, 1, 2, 1,
                                               limited, plan, error));
  require(error.category == bounded_boolean_error_category::resource_limit);
  require(error.subcode ==
         static_cast<std::uint32_t>(intersection_subcode::capacity_exceeded));

  auto malformed_seeds = seeds;
  malformed_seeds[0].incidence_count = 3;
  require(!preflight_intersection_event_records(
      malformed_seeds, constructions, 1, 2, 1, capabilities, plan, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(intersection_subcode::malformed_reference));

  auto malformed_constructions = constructions;
  malformed_constructions[0].ledger_count = 2;
  require(!preflight_intersection_event_records(
      seeds, malformed_constructions, 1, 2, 1, capabilities, plan, error));
  require(error.subcode ==
         static_cast<std::uint32_t>(intersection_subcode::malformed_reference));
  return 0;
}
