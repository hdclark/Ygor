#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/IntersectionDescriptors.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {

void require(bool value) {
  if (!value)
    std::abort();
}

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

relation_feature_key edge_feature(
    const canonical_manifold_edge_record<double> &edge,
    operand_id operand) {
  return feature(operand, relation_feature_kind::source_edge,
                 edge.key.primary, edge.key.secondary);
}

relation_feature_key facet_feature(
    const canonical_halfedge_operand<double, std::uint32_t> &topology,
    std::uint64_t source_facet) {
  require(source_facet < topology.source_facet_to_group().size());
  const auto group = topology.source_facet_to_group()[source_facet];
  require(group < topology.facet_groups().size());
  const auto &record = topology.facet_groups()[group];
  require(record.source_facet == source_facet);
  return feature(topology.operand(), relation_feature_kind::source_facet,
                 source_facet, record.ring);
}

std::vector<relation_feature_key> semantic_fan_facets(
    const canonical_halfedge_operand<double, std::uint32_t> &topology,
    std::uint64_t source_vertex) {
  require(source_vertex < topology.source_vertex_to_vertex().size());
  const auto vertex_id = topology.source_vertex_to_vertex()[source_vertex];
  require(vertex_id < topology.vertices().size());
  const auto &vertex = topology.vertices()[vertex_id];
  require(vertex.fan < topology.fans().size());
  std::vector<relation_feature_key> facets;
  for (const auto halfedge_id : topology.fans()[vertex.fan].outgoing_halfedges) {
    require(halfedge_id < topology.halfedges().size());
    const auto next = facet_feature(topology,
                                    topology.halfedges()[halfedge_id].source_facet);
    if (facets.empty() || facets.back() != next)
      facets.push_back(next);
  }
  if (facets.size() > 1 && facets.front() == facets.back())
    facets.pop_back();
  return facets;
}

intersection_event_key event_key(
    const relation_event_seed_key &public_relation,
    const relation_feature_key &query_edge,
    const relation_feature_key &source_vertex) {
  intersection_event_key key;
  key.semantic_namespace.bytes[0] = 0x91;
  key.event_class = intersection_event_class::source_vertex_contact;
  key.first_operand = query_edge.operand;
  key.second_operand = source_vertex.operand;
  key.first_owner = query_edge;
  key.second_owner = source_vertex;
  key.public_relation = public_relation;
  key.construction_kind = relation_construction_kind::bounded_point;
  key.construction_precedence =
      relation_construction_precedence::accepted_source_vertex;
  key.authoritative_source_feature = source_vertex;
  key.reused_source_vertex = source_vertex;
  key.carrier_role = intersection_carrier_role::none;
  key.contact_status = feature_relation_status::endpoint_crossing;
  key.contact_dimension = relation_contact_dimension::point;
  return key;
}

void append_incidence(event_incidence_tables &tables,
                      const event_seed_binding_record &binding,
                      const intersection_event_key &event,
                      const intersection_occurrence_key &occurrence,
                      event_incidence_kind kind,
                      relation_feature_key source_feature,
                      std::uint64_t proof_primary,
                      std::uint64_t proof_secondary,
                      std::int32_t numeric,
                      std::int8_t symbolic,
                      bool source_owner) {
  event_incidence_record record;
  record.id = event_incidence_id{tables.records.size()};
  record.event = binding.event;
  record.occurrence = binding.occurrence;
  record.seed_binding = binding.id;
  record.kind = kind;
  record.feature = source_feature;
  record.relation = binding.relation;
  record.payload_primary = proof_primary;
  record.payload_secondary = proof_secondary;
  record.payload_occurrence = binding.seed_key.occurrence;
  record.numeric_crossing = numeric;
  record.symbolic_crossing = symbolic;
  record.source_feature_owner = source_owner;
  record.key.event = event;
  record.key.occurrence = occurrence;
  record.key.seed = binding.seed_key;
  record.key.kind = kind;
  record.key.feature = source_feature;
  record.key.predecessor_relation = binding.relation.ordinal();
  record.key.proof_primary = proof_primary;
  record.key.proof_secondary = proof_secondary;
  record.key.proof_occurrence = binding.seed_key.occurrence;
  record.key.numeric_crossing = numeric;
  record.key.symbolic_crossing = symbolic;
  record.key.source_feature_owner = source_owner;
  require(valid_event_incidence_key(record.key));
  tables.by_seed.push_back(record.id);
  tables.records.push_back(std::move(record));
}

enum class fan_case { positive, tangent, zero_net };

struct fan_fixture final {
  std::vector<relation_crossing_record> crossings;
  std::vector<relation_event_seed_record> seeds;
  event_interning_tables interning;
  event_incidence_tables incidence;
  relation_feature_key source_vertex;
  std::vector<relation_feature_key> facets;
};

fan_fixture make_fan_fixture(
    const canonical_source_manifolds<double, std::uint32_t> &manifolds,
    fan_case which, bool separated = false, bool swapped = false) {
  fan_fixture out;
  const operand_id query_operand = swapped ? operand_id::b : operand_id::a;
  const operand_id fan_operand = swapped ? operand_id::a : operand_id::b;
  const auto &query_topology =
      *(query_operand == operand_id::a ? manifolds.a() : manifolds.b());
  const auto &fan_topology =
      *(fan_operand == operand_id::a ? manifolds.a() : manifolds.b());
  const auto &query_edge_record = query_topology.edges().front();
  require(query_edge_record.edge_class == canonical_edge_class::source_edge);
  const auto query_edge = edge_feature(query_edge_record, query_operand);
  out.source_vertex =
      feature(fan_operand, relation_feature_kind::source_vertex, 0);
  out.facets = semantic_fan_facets(fan_topology, 0);
  require(out.facets.size() >= 3);

  out.seeds.resize(out.facets.size());
  for (std::size_t i = 0; i < out.seeds.size(); ++i) {
    auto &seed = out.seeds[i];
    seed.id = relation_event_seed_id{i};
    seed.key.semantic_namespace.bytes[0] = 0x31;
    seed.key.semantic_namespace.bytes[1] = static_cast<std::uint8_t>(i + 1);
    seed.key.family = feature_relation_family::source_edge_source_facet;
    seed.key.first = query_edge;
    seed.key.second = out.facets[i];
    seed.key.occurrence = static_cast<std::uint32_t>(i + 1);
    seed.source_relation = feature_relation_id{i};
    seed.contact_status =
        which == fan_case::tangent ? feature_relation_status::tangency
                                   : feature_relation_status::endpoint_crossing;
    seed.contact_dimension = relation_contact_dimension::point;
    seed.accepted_source_vertex = out.source_vertex;
    seed.accepted_source_vertex_reused = true;
    seed.precision_evidence_complete = true;
    seed.distinct_occurrence_required = separated;
    seed.half_open_owner = query_operand;
    seed.numeric_crossing =
        which == fan_case::positive ? 1 : 0;
  }

  out.interning.events.resize(1);
  out.interning.events[0].id = event_id{0};
  out.interning.events[0].key =
      event_key(out.seeds.front().key, query_edge, out.source_vertex);
  require(valid_intersection_event_key(out.interning.events[0].key));

  out.interning.occurrences.resize(1);
  auto &occurrence = out.interning.occurrences[0];
  occurrence.id = event_occurrence_id{0};
  occurrence.event = event_id{0};
  occurrence.key.event = out.interning.events[0].key;
  if (separated) {
    occurrence.key.discriminator.role =
        occurrence_role::topology_separated_contact;
    occurrence.key.discriminator.occurrence_lineage = 77;
    occurrence.topology_separate = true;
    occurrence.requires_contact_separation = true;
  }
  require(valid_intersection_occurrence_key(occurrence.key));

  out.interning.seed_bindings.resize(out.seeds.size());
  out.incidence.seed_ranges.resize(out.seeds.size());
  out.crossings.resize(out.seeds.size());
  for (std::size_t i = 0; i < out.seeds.size(); ++i) {
    auto &binding = out.interning.seed_bindings[i];
    binding.id = event_seed_binding_id{i};
    binding.seed = out.seeds[i].id;
    binding.seed_key = out.seeds[i].key;
    binding.canonical_seed_ordinal = i;
    binding.event = event_id{0};
    binding.occurrence = event_occurrence_id{0};
    binding.relation = out.seeds[i].source_relation;
    binding.accepted_source_vertex = out.source_vertex;
    binding.compatibility_verified = true;

    const auto begin = out.incidence.by_seed.size();
    append_incidence(out.incidence, binding, out.interning.events[0].key,
                     occurrence.key, event_incidence_kind::source_vertex,
                     out.source_vertex, i, 0, 0, 0, false);
    append_incidence(out.incidence, binding, out.interning.events[0].key,
                     occurrence.key, event_incidence_kind::source_facet,
                     out.facets[i], i, 0, 0, 0, true);
    append_incidence(
        out.incidence, binding, out.interning.events[0].key, occurrence.key,
        event_incidence_kind::descriptor_precursor, relation_feature_key{},
        static_cast<std::uint64_t>(out.seeds[i].contact_status),
        static_cast<std::uint64_t>(out.seeds[i].contact_dimension),
        out.seeds[i].numeric_crossing, 0, false);
    if (out.seeds[i].numeric_crossing != 0)
      append_incidence(out.incidence, binding, out.interning.events[0].key,
                       occurrence.key,
                       event_incidence_kind::crossing_contribution,
                       relation_feature_key{}, i, 0,
                       out.seeds[i].numeric_crossing, 0, false);
    out.incidence.seed_ranges[i] =
        {begin, out.incidence.by_seed.size() - begin};

    auto &crossing = out.crossings[i];
    crossing.relation = out.seeds[i].source_relation;
    crossing.half_open_owner = query_operand;
    crossing.occurrence = out.seeds[i].key.occurrence;
    crossing.source_fan_group = 41;
    crossing.source_fan_group_size =
        static_cast<std::uint32_t>(out.seeds.size());
    crossing.source_fan_group_ordinal = static_cast<std::uint32_t>(i);
    if (which == fan_case::positive) {
      crossing.local_transition = 1;
      crossing.numeric_crossing = i == 0 ? 1 : 0;
      crossing.numeric_owner = i == 0;
    } else if (which == fan_case::zero_net) {
      crossing.local_transition = i == 1 ? -1 : 1;
    }
    crossing.source_fan_resolved = true;
    crossing.locally_conservative = true;
  }
  return out;
}

intersection_descriptor_tables make_base(
    const canonical_source_manifolds<double, std::uint32_t> &manifolds) {
  intersection_descriptor_tables base;
  const auto &edge = manifolds.b()->edges().front();
  require(edge.edge_class == canonical_edge_class::source_edge);
  intersection_descriptor_record record;
  record.id = intersection_descriptor_id{0};
  record.key.locus =
      intersection_descriptor_locus::source_edge_cluster_boundary;
  record.key.category = intersection_descriptor_category::endpoint_crossing;
  record.key.source_feature = edge_feature(edge, operand_id::b);
  record.key.parent_lineage = 100;
  record.key.boundary_ordinal = 1;
  record.key.orientation = 1;
  record.signed_crossing_delta = 1;
  record.symbolic_owner = operand_id::a;
  record.classification_consumable = true;
  record.selection_consumable = true;
  record.topology_consumable = true;
  require(valid_intersection_descriptor_key(record.key));
  base.records.push_back(record);
  return base;
}

std::size_t count_locus(const intersection_descriptor_tables &tables,
                        intersection_descriptor_locus locus) {
  return static_cast<std::size_t>(std::count_if(
      tables.records.begin(), tables.records.end(),
      [=](const auto &record) { return record.key.locus == locus; }));
}

std::vector<intersection_descriptor_record> semantic_topology_records(
    const intersection_descriptor_tables &tables) {
  std::vector<intersection_descriptor_record> result;
  for (const auto &record : tables.records) {
    if (record.key.locus !=
            intersection_descriptor_locus::source_vertex_sector &&
        record.key.locus !=
            intersection_descriptor_locus::source_facet_original_edge_adjacency)
      continue;
    auto copy = record;
    copy.id = intersection_descriptor_id{0};
    copy.provenance = {};
    result.push_back(std::move(copy));
  }
  return result;
}

bool same_descriptor_semantics(const intersection_descriptor_record &a,
                               const intersection_descriptor_record &b) {
  return a.key == b.key &&
         a.signed_crossing_delta == b.signed_crossing_delta &&
         a.symbolic_owner == b.symbolic_owner &&
         a.symbolic_rule_ordinal == b.symbolic_rule_ordinal &&
         a.continuation_allowed == b.continuation_allowed &&
         a.occurrence_separation_required ==
             b.occurrence_separation_required &&
         a.classification_consumable == b.classification_consumable &&
         a.selection_consumable == b.selection_consumable &&
         a.topology_consumable == b.topology_consumable &&
         a.schema_version == b.schema_version && a.reserved8 == b.reserved8;
}

bool same_semantic_projection(const intersection_descriptor_tables &a,
                              const intersection_descriptor_tables &b) {
  const auto left = semantic_topology_records(a);
  const auto right = semantic_topology_records(b);
  if (left.size() != right.size())
    return false;
  for (std::size_t i = 0; i < left.size(); ++i)
    if (!same_descriptor_semantics(left[i], right[i]))
      return false;
  return true;
}

const intersection_descriptor_record *find_sector(
    const intersection_descriptor_tables &tables,
    std::uint64_t sector) {
  for (const auto &record : tables.records)
    if (record.key.locus ==
            intersection_descriptor_locus::source_vertex_sector &&
        record.key.boundary_ordinal == sector)
      return &record;
  return nullptr;
}


} // namespace

int main() {
  auto predecessor = broad_phase_tests::build_predecessors(
      broad_phase_tests::box(),
      broad_phase_tests::box(2, 2, 2, 3, 3, 3),
      source_triangulation_provider_kind::indexed_dependency_v1, false);
  const auto &manifolds = *predecessor.manifolds;
  const auto base = make_base(manifolds);
  bounded_boolean_error error;

  auto fixture = make_fan_fixture(manifolds, fan_case::positive);
  intersection_descriptor_tables tables;
  require(extend_intersection_descriptors_with_source_topology(
      manifolds, fixture.crossings, fixture.seeds, fixture.interning,
      fixture.incidence, base, tables, error));
  require(verify_intersection_source_topology_descriptors(
      manifolds, fixture.crossings, fixture.seeds, fixture.interning,
      fixture.incidence, base, tables, error));
  require(count_locus(tables,
                      intersection_descriptor_locus::source_vertex_sector) ==
          fixture.facets.size());
  require(count_locus(
              tables,
              intersection_descriptor_locus::
                  transparent_internal_diagonal_adjacency) == 12);
  require(count_locus(
              tables,
              intersection_descriptor_locus::
                  source_facet_original_edge_adjacency) == 24);
  for (std::size_t i = 0; i < fixture.facets.size(); ++i) {
    const auto *sector = find_sector(tables, i);
    require(sector != nullptr);
    require(sector->key.source_feature == fixture.source_vertex);
    require(sector->key.category ==
            intersection_descriptor_category::endpoint_crossing);
    require(sector->key.orientation == 1);
    require(sector->signed_crossing_delta == (i == 0 ? 1 : 0));
    require(!sector->continuation_allowed);
    require(sector->classification_consumable);
    require(sector->selection_consumable);
    require(sector->topology_consumable);
    require(sector->provenance.count >= 3);
  }

  bool saw_affected_adjacency = false;
  bool saw_transparent = false;
  for (const auto &record : tables.records) {
    if (record.key.locus ==
            intersection_descriptor_locus::
                source_facet_original_edge_adjacency &&
        record.key.category ==
            intersection_descriptor_category::endpoint_crossing) {
      saw_affected_adjacency = true;
      require(record.signed_crossing_delta == 1);
      require(record.classification_consumable);
      require(record.selection_consumable);
    }
    if (record.key.locus ==
        intersection_descriptor_locus::
            transparent_internal_diagonal_adjacency) {
      saw_transparent = true;
      require(record.key.category ==
              intersection_descriptor_category::bookkeeping_only);
      require(record.continuation_allowed);
      require(record.topology_consumable);
      require(!record.classification_consumable);
      require(!record.selection_consumable);
    }
  }
  require(saw_affected_adjacency && saw_transparent);

  auto reversed_crossings = fixture.crossings;
  std::reverse(reversed_crossings.begin(), reversed_crossings.end());
  intersection_descriptor_tables reversed_tables;
  require(extend_intersection_descriptors_with_source_topology(
      manifolds, reversed_crossings, fixture.seeds, fixture.interning,
      fixture.incidence, base, reversed_tables, error));
  require(same_semantic_projection(tables, reversed_tables));

  auto swapped = make_fan_fixture(manifolds, fan_case::positive, false, true);
  intersection_descriptor_tables swapped_tables;
  require(extend_intersection_descriptors_with_source_topology(
      manifolds, swapped.crossings, swapped.seeds, swapped.interning,
      swapped.incidence, base, swapped_tables, error));
  require(verify_intersection_source_topology_descriptors(
      manifolds, swapped.crossings, swapped.seeds, swapped.interning,
      swapped.incidence, base, swapped_tables, error));
  require(count_locus(swapped_tables,
                      intersection_descriptor_locus::source_vertex_sector) ==
          swapped.facets.size());
  for (std::size_t i = 0; i < swapped.facets.size(); ++i) {
    const auto *sector = find_sector(swapped_tables, i);
    require(sector != nullptr);
    require(sector->key.source_feature.operand == operand_id::a);
    require(sector->symbolic_owner == operand_id::b);
  }

  auto alternative = broad_phase_tests::build_predecessors(
      broad_phase_tests::box(),
      broad_phase_tests::box(2, 2, 2, 3, 3, 3),
      source_triangulation_provider_kind::full_rescan_reference_v1, false);
  const auto &alternative_manifolds = *alternative.manifolds;
  const auto alternative_base = make_base(alternative_manifolds);
  auto alternative_fixture =
      make_fan_fixture(alternative_manifolds, fan_case::positive);
  intersection_descriptor_tables alternative_tables;
  require(extend_intersection_descriptors_with_source_topology(
      alternative_manifolds, alternative_fixture.crossings,
      alternative_fixture.seeds, alternative_fixture.interning,
      alternative_fixture.incidence, alternative_base, alternative_tables,
      error));
  require(verify_intersection_source_topology_descriptors(
      alternative_manifolds, alternative_fixture.crossings,
      alternative_fixture.seeds, alternative_fixture.interning,
      alternative_fixture.incidence, alternative_base, alternative_tables,
      error));
  require(same_semantic_projection(tables, alternative_tables));

  auto tangent = make_fan_fixture(manifolds, fan_case::tangent);
  intersection_descriptor_tables tangent_tables;
  require(extend_intersection_descriptors_with_source_topology(
      manifolds, tangent.crossings, tangent.seeds, tangent.interning,
      tangent.incidence, base, tangent_tables, error));
  for (std::size_t i = 0; i < tangent.facets.size(); ++i) {
    const auto *sector = find_sector(tangent_tables, i);
    require(sector != nullptr);
    require(sector->key.category == intersection_descriptor_category::tangent);
    require(sector->key.orientation == 0);
    require(sector->signed_crossing_delta == 0);
    require(sector->continuation_allowed);
  }

  auto zero_net = make_fan_fixture(manifolds, fan_case::zero_net);
  intersection_descriptor_tables zero_net_tables;
  require(extend_intersection_descriptors_with_source_topology(
      manifolds, zero_net.crossings, zero_net.seeds, zero_net.interning,
      zero_net.incidence, base, zero_net_tables, error));
  require(find_sector(zero_net_tables, 0)->key.orientation == 1);
  require(find_sector(zero_net_tables, 1)->key.orientation == -1);
  for (std::size_t i = 0; i < zero_net.facets.size(); ++i)
    require(find_sector(zero_net_tables, i)->signed_crossing_delta == 0);

  auto separated = make_fan_fixture(manifolds, fan_case::tangent, true);
  intersection_descriptor_tables separated_tables;
  require(extend_intersection_descriptors_with_source_topology(
      manifolds, separated.crossings, separated.seeds, separated.interning,
      separated.incidence, base, separated_tables, error));
  for (std::size_t i = 0; i < separated.facets.size(); ++i) {
    const auto *sector = find_sector(separated_tables, i);
    require(sector != nullptr);
    require(sector->key.category ==
            intersection_descriptor_category::topology_separated_contact);
    require(sector->occurrence_separation_required);
    require(!sector->continuation_allowed);
  }

  auto mutated = tables;
  auto mutated_sector = std::find_if(
      mutated.records.begin(), mutated.records.end(), [](const auto &record) {
        return record.key.locus ==
                   intersection_descriptor_locus::source_vertex_sector &&
               record.key.boundary_ordinal == 0;
      });
  require(mutated_sector != mutated.records.end());
  mutated_sector->signed_crossing_delta = 0;
  require(!verify_intersection_source_topology_descriptors(
      manifolds, fixture.crossings, fixture.seeds, fixture.interning,
      fixture.incidence, base, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  auto missing = fixture.crossings;
  missing.pop_back();
  require(!extend_intersection_descriptors_with_source_topology(
      manifolds, missing, fixture.seeds, fixture.interning, fixture.incidence,
      base, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));

  auto duplicate_ordinal = fixture.crossings;
  duplicate_ordinal.back().source_fan_group_ordinal = 0;
  require(!extend_intersection_descriptors_with_source_topology(
      manifolds, duplicate_ordinal, fixture.seeds, fixture.interning,
      fixture.incidence, base, mutated, error));

  auto mixed_owner = fixture.crossings;
  mixed_owner.back().half_open_owner = operand_id::b;
  require(!extend_intersection_descriptors_with_source_topology(
      manifolds, mixed_owner, fixture.seeds, fixture.interning,
      fixture.incidence, base, mutated, error));

  auto bad_facet = fixture.seeds;
  bad_facet.back().key.second = bad_facet.front().key.second;
  auto bad_interning = fixture.interning;
  bad_interning.seed_bindings.back().seed_key = bad_facet.back().key;
  auto bad_incidence = fixture.incidence;
  const auto range = bad_incidence.seed_ranges.back();
  for (std::uint64_t i = 0; i < range.count; ++i) {
    auto &record = bad_incidence.records[
        bad_incidence.by_seed[range.begin + i].ordinal()];
    record.key.seed = bad_facet.back().key;
    if (record.kind == event_incidence_kind::source_facet) {
      record.feature = bad_facet.back().key.second;
      record.key.feature = record.feature;
    }
  }
  require(!extend_intersection_descriptors_with_source_topology(
      manifolds, fixture.crossings, bad_facet, bad_interning, bad_incidence,
      base, mutated, error));

  auto malformed_incidence = fixture.incidence;
  malformed_incidence.records.front().reserved16 = 1;
  require(!extend_intersection_descriptors_with_source_topology(
      manifolds, fixture.crossings, fixture.seeds, fixture.interning,
      malformed_incidence, base, mutated, error));

  return 0;
}
