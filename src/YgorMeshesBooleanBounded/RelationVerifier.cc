#include "StrictFloatingBuild.h"
#include "RelationVerifier.h"
#include "CoplanarRelationOverlay.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error verifier_error(relation_subcode subcode,
                                     const char *summary) {
  return relation_error(subcode,
                        bounded_boolean_error_category::internal_invariant_error,
                        summary,
                        relation_checkpoint::independent_verification);
}

const canonical_relation_request *find_request(
    const relation_request_graph &graph,
    const relation_request_key &key) noexcept {
  const auto it = std::lower_bound(
      graph.requests.begin(), graph.requests.end(), key,
      [](const canonical_relation_request &record,
         const relation_request_key &candidate) {
        return record.key < candidate;
      });
  return it == graph.requests.end() || it->key != key ? nullptr : &*it;
}

bool request_has_dependency(const relation_request_graph &graph,
                            const canonical_relation_request &consumer,
                            relation_request_id producer) noexcept {
  if (consumer.dependency_begin > graph.dependencies.size() ||
      consumer.dependency_count >
          graph.dependencies.size() - consumer.dependency_begin)
    return false;
  for (std::uint64_t offset = 0; offset < consumer.dependency_count; ++offset)
    if (graph.dependencies[consumer.dependency_begin + offset].producer ==
        producer)
      return true;
  return false;
}

bool request_has_witness(const relation_request_graph &graph,
                         const canonical_relation_request &request,
                         candidate_id candidate) noexcept {
  if (request.witness_begin > graph.candidate_witnesses.size() ||
      request.witness_count >
          graph.candidate_witnesses.size() - request.witness_begin)
    return false;
  for (std::uint64_t offset = 0; offset < request.witness_count; ++offset)
    if (graph.candidate_witnesses[request.witness_begin + offset] == candidate)
      return true;
  return false;
}

feature_relation_status edge_status(
    source_edge_contact_class contact,
    source_edge_orientation_relation orientation) noexcept {
  switch (contact) {
  case source_edge_contact_class::none:
    return feature_relation_status::definitely_separated;
  case source_edge_contact_class::proper_crossing:
    return feature_relation_status::proper_crossing;
  case source_edge_contact_class::endpoint_contact:
    return feature_relation_status::endpoint_crossing;
  case source_edge_contact_class::point_contact:
    return feature_relation_status::point_contact;
  case source_edge_contact_class::partial_overlap:
    return feature_relation_status::overlap;
  case source_edge_contact_class::first_contains_second:
  case source_edge_contact_class::second_contains_first:
    return feature_relation_status::containment;
  case source_edge_contact_class::equal:
    return orientation == source_edge_orientation_relation::opposite
               ? feature_relation_status::coincidence_opposite_orientation
               : feature_relation_status::coincidence_same_orientation;
  }
  return feature_relation_status::not_evaluated;
}

feature_relation_status edge_facet_status(
    source_edge_facet_contact_class contact) noexcept {
  switch (contact) {
  case source_edge_facet_contact_class::none:
    return feature_relation_status::definitely_separated;
  case source_edge_facet_contact_class::proper_face_crossing:
    return feature_relation_status::proper_crossing;
  case source_edge_facet_contact_class::boundary_crossing:
    return feature_relation_status::endpoint_crossing;
  case source_edge_facet_contact_class::endpoint_contact:
  case source_edge_facet_contact_class::coplanar_point_contact:
    return feature_relation_status::point_contact;
  case source_edge_facet_contact_class::tangent_contact:
    return feature_relation_status::tangency;
  case source_edge_facet_contact_class::coplanar_boundary_overlap:
    return feature_relation_status::overlap;
  case source_edge_facet_contact_class::coplanar_containment:
    return feature_relation_status::containment;
  }
  return feature_relation_status::not_evaluated;
}

feature_relation_status facet_status(
    source_facet_support_relation_class classification) noexcept {
  switch (classification) {
  case source_facet_support_relation_class::transverse:
    return feature_relation_status::proper_crossing;
  case source_facet_support_relation_class::parallel_separated:
    return feature_relation_status::definitely_separated;
  case source_facet_support_relation_class::coplanar_same_orientation:
    return feature_relation_status::coincidence_same_orientation;
  case source_facet_support_relation_class::coplanar_opposite_orientation:
    return feature_relation_status::coincidence_opposite_orientation;
  }
  return feature_relation_status::not_evaluated;
}

feature_relation_status overlay_status(
    coplanar_facet_overlay_class classification) noexcept {
  switch (classification) {
  case coplanar_facet_overlay_class::disjoint:
    return feature_relation_status::definitely_separated;
  case coplanar_facet_overlay_class::point_contact:
    return feature_relation_status::point_contact;
  case coplanar_facet_overlay_class::segment_contact:
    return feature_relation_status::segment_contact;
  case coplanar_facet_overlay_class::area_overlap:
    return feature_relation_status::overlap;
  case coplanar_facet_overlay_class::first_contains_second:
  case coplanar_facet_overlay_class::second_contains_first:
    return feature_relation_status::containment;
  case coplanar_facet_overlay_class::equal_same_orientation:
    return feature_relation_status::coincidence_same_orientation;
  case coplanar_facet_overlay_class::equal_opposite_orientation:
    return feature_relation_status::coincidence_opposite_orientation;
  }
  return feature_relation_status::not_evaluated;
}

relation_coplanar_arc_kind verifier_coplanar_arc_kind(
    coplanar_overlap_arc_kind kind) noexcept {
  return kind == coplanar_overlap_arc_kind::shared_boundary
             ? relation_coplanar_arc_kind::shared_boundary
             : relation_coplanar_arc_kind::interior_boundary;
}

relation_coplanar_component_kind verifier_coplanar_component_kind(
    coplanar_overlap_component_kind kind) noexcept {
  switch (kind) {
  case coplanar_overlap_component_kind::isolated_point:
    return relation_coplanar_component_kind::isolated_point;
  case coplanar_overlap_component_kind::boundary_segment:
    return relation_coplanar_component_kind::boundary_segment;
  case coplanar_overlap_component_kind::area_boundary:
    return relation_coplanar_component_kind::area_boundary;
  case coplanar_overlap_component_kind::coincident_sheet_boundary:
    return relation_coplanar_component_kind::coincident_sheet_boundary;
  }
  return relation_coplanar_component_kind::isolated_point;
}

relation_request_key verifier_imported_geometry_key(
    const bounded_boolean_digest &semantic_namespace,
    const relation_feature_key &feature, relation_record_scope scope) noexcept {
  relation_request_key out;
  out.semantic_namespace = semantic_namespace;
  out.family = relation_request_family::imported_source_geometry;
  out.scope = scope;
  out.first = feature;
  out.second = relation_feature_key{};
  out.second.operand = feature.operand;
  out.formula_version = contract_versions::exact_relation_formulas;
  out.policy_version = contract_versions::relation_request_key_schema;
  return out;
}

relation_request_key verifier_derived_key(const relation_request_key &base,
                                          relation_request_family family,
                                          std::uint64_t directed_use,
                                          std::uint32_t occurrence) noexcept {
  relation_request_key out = base;
  out.family = family;
  out.directed_use = directed_use;
  out.occurrence_discriminator = occurrence;
  out.formula_version = contract_versions::exact_relation_formulas;
  out.policy_version = contract_versions::relation_request_key_schema;
  out.reserved = 0;
  return out;
}

std::uint64_t verifier_tagged_use(std::uint8_t domain,
                                  std::uint8_t category = 0) noexcept {
  return (static_cast<std::uint64_t>(domain) << 56U) |
         (static_cast<std::uint64_t>(category) << 48U);
}

template <class T>
bool finite_construction_component(const relation_construction_record &record,
                                   std::size_t component) noexcept {
  using bits_type = floating_uint_t<T>;
  const auto nominal = from_bits<T>(static_cast<bits_type>(record.nominal_bits[component]));
  const auto lower = from_bits<T>(static_cast<bits_type>(record.lower_bits[component]));
  const auto upper = from_bits<T>(static_cast<bits_type>(record.upper_bits[component]));
  return finite_bits(nominal) && finite_bits(lower) && finite_bits(upper) &&
         !finite_numeric_less(upper, lower) &&
         !finite_numeric_less(nominal, lower) &&
         !finite_numeric_less(upper, nominal);
}

bool valid_operation(boolean_operation operation) noexcept {
  const auto raw = static_cast<std::uint8_t>(operation);
  return raw >= 1 && raw <= 5;
}

bool public_contact(feature_relation_status value) noexcept {
  return value != feature_relation_status::not_evaluated &&
         value != feature_relation_status::definitely_separated;
}

bool verifier_symbolic_source_family(
    relation_request_family family) noexcept {
  return family == relation_request_family::source_edge_source_edge ||
         family == relation_request_family::source_edge_source_facet ||
         family == relation_request_family::source_facet_source_facet ||
         family == relation_request_family::coplanar_source_facet_overlay;
}

template <class T>
bool verifier_set_truth_symbolic_evidence(
    const relation_truth_record &truth, symbolic_eligibility_reason reason,
    symbolic_eligibility_record &eligibility) noexcept {
  if (truth.exact_relation != exact_relation_status::exact_zero ||
      truth.exact_formula == 0 ||
      truth.bounded_sign == bounded_sign_status::invalid ||
      truth.disposition == predicate_disposition::fail_invalid)
    return false;
  eligibility.exact_relation = exact_relation_status::exact_zero;
  eligibility.reason = reason;
  eligibility.evidence_formula_version = truth.exact_formula;
  eligibility.exact_lineage_tie = true;
  eligibility.rounded_nominal_zero =
      from_bits<T>(static_cast<floating_uint_t<T>>(
          truth.rounded_nominal_bits)) == T(0);
  eligibility.inherited_uncertainty =
      truth.bounded_sign == bounded_sign_status::overlaps_boundary;
  return true;
}

template <class T>
bool verifier_set_region_symbolic_evidence(
    const source_facet_point_region_record<T> &region,
    symbolic_eligibility_record &eligibility) noexcept {
  if (!region.boundary_ownership_resolved ||
      (region.classification !=
           source_facet_point_region_class::original_edge &&
       region.classification !=
           source_facet_point_region_class::original_vertex) ||
      region.source_edge_owners.empty())
    return false;
  std::uint16_t formula = 0;
  bool inherited_uncertainty = false;
  for (const auto &owner : region.source_edge_owners) {
    if (owner.edge_ordinal >= region.orientation_evidence.size())
      return false;
    const auto &evidence = region.orientation_evidence[owner.edge_ordinal];
    if (evidence.exact_sign != 0 || evidence.formula_version == 0 ||
        (formula != 0 && formula != evidence.formula_version))
      return false;
    formula = evidence.formula_version;
    inherited_uncertainty =
        inherited_uncertainty ||
        evidence.bounded_sign == bounded_planar_sign::uncertain;
  }
  eligibility.exact_relation = exact_relation_status::exact_zero;
  eligibility.reason =
      region.classification == source_facet_point_region_class::original_vertex
          ? symbolic_eligibility_reason::shared_source_endpoint
          : symbolic_eligibility_reason::collinear_source_edge_lineage;
  eligibility.evidence_formula_version = formula;
  eligibility.exact_lineage_tie = true;
  eligibility.rounded_nominal_zero = false;
  eligibility.inherited_uncertainty = inherited_uncertainty;
  return true;
}

bool verifier_symbolic_eligibility_equal(
    const symbolic_eligibility_record &a,
    const symbolic_eligibility_record &b) noexcept {
  return a.request == b.request && a.exact_relation == b.exact_relation &&
         a.reason == b.reason &&
         a.evidence_formula_version == b.evidence_formula_version &&
         a.exact_lineage_tie == b.exact_lineage_tie &&
         a.representational_tie_evidence ==
             b.representational_tie_evidence &&
         a.structural_category_eligible == b.structural_category_eligible &&
         a.tolerance_compatible == b.tolerance_compatible &&
         a.rounded_nominal_zero == b.rounded_nominal_zero &&
         a.inherited_uncertainty == b.inherited_uncertainty &&
         a.separated_realizations_possible ==
             b.separated_realizations_possible &&
         a.owner_is_original_source_feature ==
             b.owner_is_original_source_feature &&
         a.reserved8 == b.reserved8 && a.reserved == b.reserved;
}

struct verifier_source_fan_group_key final {
  bool boundary_group = false;
  relation_feature_key query_edge{};
  operand_id opposite_operand = operand_id::a;
  std::uint8_t boundary_kind = 0;
  std::uint64_t owner_primary = 0;
  std::uint64_t owner_secondary = 0;
  relation_request_key singleton_relation{};
  std::uint32_t singleton_occurrence = 0;

  friend bool operator<(const verifier_source_fan_group_key &a,
                        const verifier_source_fan_group_key &b) noexcept {
    return std::tie(a.boundary_group, a.query_edge, a.opposite_operand,
                    a.boundary_kind, a.owner_primary, a.owner_secondary,
                    a.singleton_relation, a.singleton_occurrence) <
           std::tie(b.boundary_group, b.query_edge, b.opposite_operand,
                    b.boundary_kind, b.owner_primary, b.owner_secondary,
                    b.singleton_relation, b.singleton_occurrence);
  }
  friend bool operator==(const verifier_source_fan_group_key &a,
                         const verifier_source_fan_group_key &b) noexcept {
    return std::tie(a.boundary_group, a.query_edge, a.opposite_operand,
                    a.boundary_kind, a.owner_primary, a.owner_secondary,
                    a.singleton_relation, a.singleton_occurrence) ==
           std::tie(b.boundary_group, b.query_edge, b.opposite_operand,
                    b.boundary_kind, b.owner_primary, b.owner_secondary,
                    b.singleton_relation, b.singleton_occurrence);
  }
};

template <class T> struct verifier_crossing_descriptor final {
  verifier_source_fan_group_key group{};
  relation_request_key source_relation{};
  feature_relation_id relation{0};
  const source_edge_facet_event_record<T> *event = nullptr;
  std::uint32_t occurrence = 0;
  std::int8_t local_transition = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
};

template <class T>
std::int8_t verifier_local_transition(
    const source_edge_facet_event_record<T> &event) noexcept {
  const auto occupancy = [](source_edge_facet_occupancy_state state) {
    return state == source_edge_facet_occupancy_state::occupied
               ? std::int8_t{1}
           : state == source_edge_facet_occupancy_state::unoccupied
               ? std::int8_t{0}
               : std::int8_t{2};
  };
  const auto before = occupancy(event.before);
  const auto after = occupancy(event.after);
  return before <= 1 && after <= 1
             ? static_cast<std::int8_t>(after - before)
             : std::int8_t{0};
}

template <class T>
bool verifier_source_fan_key(const relation_request_key &relation,
                             const source_edge_facet_event_record<T> &event,
                             std::uint32_t occurrence,
                             verifier_source_fan_group_key &key) {
  key = verifier_source_fan_group_key{};
  key.query_edge = relation.first;
  key.opposite_operand = relation.second.operand;
  const bool source_boundary =
      event.region.classification ==
          source_facet_point_region_class::original_edge ||
      event.region.classification ==
          source_facet_point_region_class::original_vertex;
  if (!source_boundary) {
    key.singleton_relation = relation;
    key.singleton_occurrence = occurrence;
    return true;
  }
  if (event.kind != source_edge_facet_event_kind::boundary_crossing &&
      event.kind != source_edge_facet_event_kind::tangent_contact)
    return false;
  key.boundary_group = true;
  if (event.region.classification ==
      source_facet_point_region_class::original_vertex) {
    if (event.region.source_vertex_owners.size() != 1)
      return false;
    key.boundary_kind = 2;
    key.owner_primary = event.region.source_vertex_owners.front();
    return true;
  }
  if (event.region.classification !=
          source_facet_point_region_class::original_edge ||
      event.region.source_edge_owners.empty())
    return false;
  key.boundary_kind = 1;
  const auto canonical_endpoints = [](const auto &owner) {
    return std::minmax(owner.origin_source_vertex,
                       owner.destination_source_vertex);
  };
  const auto first = canonical_endpoints(event.region.source_edge_owners.front());
  key.owner_primary = first.first;
  key.owner_secondary = first.second;
  for (const auto &owner : event.region.source_edge_owners) {
    const auto endpoints = canonical_endpoints(owner);
    if (endpoints.first != key.owner_primary ||
        endpoints.second != key.owner_secondary)
      return false;
  }
  return true;
}

template <class T, class I>
bool verifier_source_facet_feature(
    const canonical_halfedge_operand<T, I> &topology,
    std::uint64_t source_facet,
    relation_feature_key &feature) noexcept {
  if (source_facet >= topology.source_facet_to_group().size())
    return false;
  const auto group = topology.source_facet_to_group()[source_facet];
  if (group >= topology.facet_groups().size())
    return false;
  const auto &record = topology.facet_groups()[group];
  if (record.canonical_id != group || record.source_facet != source_facet)
    return false;
  feature = relation_feature_key{};
  feature.operand = topology.operand();
  feature.kind = relation_feature_kind::source_facet;
  feature.primary = source_facet;
  feature.secondary = record.ring;
  return valid_relation_feature_key(feature);
}

template <class T, class I>
bool verifier_expected_source_fan_facets(
    const canonical_candidate_stream<T, I> &candidates,
    const verifier_source_fan_group_key &key,
    std::vector<relation_feature_key> &facets) {
  facets.clear();
  if (!key.boundary_group || !candidates.manifolds())
    return false;
  const auto topology =
      key.opposite_operand == operand_id::a ? candidates.manifolds()->a()
                                            : candidates.manifolds()->b();
  if (!topology || topology->operand() != key.opposite_operand)
    return false;
  if (key.boundary_kind == 1) {
    const auto wanted = std::minmax(key.owner_primary, key.owner_secondary);
    const canonical_manifold_edge_record<T> *match = nullptr;
    for (const auto &edge : topology->edges()) {
      if (edge.edge_class != canonical_edge_class::source_edge ||
          !edge.source_feature_owner ||
          edge.halfedges[0] >= topology->halfedges().size())
        continue;
      const auto &halfedge = topology->halfedges()[edge.halfedges[0]];
      const auto endpoints =
          std::minmax(halfedge.source_origin, halfedge.source_destination);
      if (endpoints.first == wanted.first && endpoints.second == wanted.second) {
        if (match)
          return false;
        match = &edge;
      }
    }
    if (!match)
      return false;
    for (const auto source_facet : match->facets) {
      relation_feature_key feature;
      if (!verifier_source_facet_feature(*topology, source_facet, feature))
        return false;
      facets.push_back(feature);
    }
  } else if (key.boundary_kind == 2) {
    if (key.owner_primary >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex = topology->source_vertex_to_vertex()[key.owner_primary];
    if (vertex >= topology->vertices().size())
      return false;
    const auto fan = topology->vertices()[vertex].fan;
    if (fan >= topology->fans().size())
      return false;
    for (const auto halfedge_id : topology->fans()[fan].outgoing_halfedges) {
      if (halfedge_id >= topology->halfedges().size())
        return false;
      relation_feature_key feature;
      if (!verifier_source_facet_feature(
              *topology, topology->halfedges()[halfedge_id].source_facet,
              feature))
        return false;
      facets.push_back(feature);
    }
  } else {
    return false;
  }
  std::sort(facets.begin(), facets.end());
  facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
  return facets.size() >= 2;
}

} // namespace

template <class T, class I>
bool verify_signed_feature_relations(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = verifier_error(subcode, summary);
    return false;
  };
  if (artifact.schema_version_ != contract_versions::relation_artifact_schema ||
      artifact.provider_version_ != contract_versions::relation_provider ||
      artifact.graph_policy_version_ != contract_versions::relation_graph_policy ||
      artifact.truth_policy_version_ != contract_versions::relation_truth_policy ||
      artifact.codec_version_ != contract_versions::relation_codec ||
      artifact.verifier_version_ != contract_versions::relation_verifier ||
      artifact.provider_ !=
          relation_provider_kind::canonical_source_feature_relation_graph_v1 ||
      artifact.verification_ !=
          relation_verification_disposition::independently_verified ||
      !artifact.owner_.anchor ||
      !artifact.request_graph_.owner.same_owner(artifact.owner_) ||
      !valid_operation(artifact.operation_) ||
      !finite_bits(artifact.residual_boundary_) ||
      artifact.residual_boundary_ < T(0) ||
      artifact.symbolic_policy_digest_ != materialize_symbolic_policy().digest)
    return fail(relation_subcode::unsupported_version,
                "Component 07 version, provider, policy, or owner mismatch");

  bounded_boolean_error graph_error;
  if (!verify_relation_request_graph(artifact.request_graph_, graph_error)) {
    error = graph_error;
    return false;
  }
  if (artifact.graph_digest_ != artifact.request_graph_.semantic_digest)
    return fail(relation_subcode::digest_mismatch,
                "Component 07 graph digest mismatch");
  if (!artifact.candidates_ || !artifact.source_edge_stage_ ||
      !artifact.source_edge_facet_stage_ || !artifact.source_facet_stage_ ||
      !artifact.coplanar_overlay_stage_)
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 detailed predecessor stages are missing");
  if (!artifact.candidates_->owner().same_owner(artifact.owner_) ||
      artifact.candidates_->candidate_digest() != artifact.candidate_digest_ ||
      artifact.candidates_->verification() !=
          broad_phase_verification_disposition::independently_verified)
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 candidate predecessor handshake failed");

  relation_capabilities capabilities;
  capabilities.owner = artifact.owner_;
  capabilities.maximum_requests =
      std::max<std::uint64_t>(capabilities.maximum_requests,
                              artifact.request_graph_.requests.size());
  capabilities.maximum_dependencies =
      std::max<std::uint64_t>(capabilities.maximum_dependencies,
                              artifact.request_graph_.dependencies.size());
  capabilities.maximum_consumers =
      std::max<std::uint64_t>(capabilities.maximum_consumers,
                              artifact.request_graph_.reverse_consumers.size() +
                                  artifact.request_graph_.candidate_witnesses.size());
  if (!verify_candidate_source_edge_relation_stage(
          *artifact.candidates_, artifact.context_digest_,
          artifact.residual_boundary_, capabilities,
          *artifact.source_edge_stage_, error) ||
      !verify_candidate_source_edge_facet_relation_stage(
          *artifact.candidates_, *artifact.source_edge_stage_,
          artifact.context_digest_, artifact.residual_boundary_, capabilities,
          *artifact.source_edge_facet_stage_, error) ||
      !verify_candidate_source_facet_relation_stage(
          *artifact.candidates_, *artifact.source_edge_facet_stage_,
          artifact.context_digest_, artifact.residual_boundary_, capabilities,
          *artifact.source_facet_stage_, error) ||
      !verify_candidate_coplanar_overlay_stage(
          *artifact.candidates_, *artifact.source_edge_stage_,
          *artifact.source_facet_stage_, *artifact.coplanar_overlay_stage_,
          error))
    return false;

  std::map<std::pair<relation_request_id, std::uint32_t>, std::uint32_t>
      canonical_edge_facet_occurrences;
  for (const auto &entry : artifact.source_edge_facet_stage_->ordered_events)
    if (!canonical_edge_facet_occurrences
             .emplace(std::make_pair(entry.relation, entry.local_event),
                      entry.canonical_occurrence)
             .second)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 canonical event occurrence is duplicated");
  const auto canonical_event_occurrence =
      [&](relation_request_id relation, std::uint32_t local_event,
          std::uint32_t &occurrence) {
        const auto found = canonical_edge_facet_occurrences.find(
            std::make_pair(relation, local_event));
        if (found == canonical_edge_facet_occurrences.end())
          return false;
        occurrence = found->second;
        return true;
      };

  struct verifier_overlay_descriptor final {
    relation_request_key key{};
    std::size_t ordinal = 0;
  };
  std::vector<verifier_overlay_descriptor> overlay_descriptors;
  overlay_descriptors.reserve(artifact.coplanar_overlay_stage_->overlays.size());
  if (artifact.coplanar_overlay_stage_->links.size() !=
      artifact.coplanar_overlay_stage_->overlays.size())
    return fail(relation_subcode::coplanar_overlay_invariant,
                "Component 07 coplanar overlay links are incomplete");
  for (std::size_t i = 0;
       i < artifact.coplanar_overlay_stage_->overlays.size(); ++i) {
    const auto &link = artifact.coplanar_overlay_stage_->links[i];
    if (link.overlay_ordinal != i || link.reserved != 0 ||
        link.support_relation.ordinal() >=
            artifact.source_facet_stage_->request_graph.requests.size())
      return fail(relation_subcode::coplanar_overlay_dependency_missing,
                  "Component 07 coplanar overlay support link is malformed");
    auto key = artifact.source_facet_stage_->request_graph
                   .requests[link.support_relation.ordinal()]
                   .key;
    key.family = relation_request_family::coplanar_source_facet_overlay;
    key.directed_use = 0;
    key.occurrence_discriminator = 0;
    overlay_descriptors.push_back({key, i});
  }
  std::sort(overlay_descriptors.begin(), overlay_descriptors.end(),
            [](const verifier_overlay_descriptor &a,
               const verifier_overlay_descriptor &b) { return a.key < b.key; });
  for (std::size_t i = 1; i < overlay_descriptors.size(); ++i)
    if (overlay_descriptors[i - 1].key == overlay_descriptors[i].key)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 coplanar overlay key is duplicated");

  const std::size_t expected_relations =
      artifact.source_edge_stage_->relations.size() +
      artifact.source_edge_facet_stage_->relations.size() +
      artifact.source_facet_stage_->relations.size() +
      artifact.coplanar_overlay_stage_->overlays.size();
  if (artifact.relations_.size() != expected_relations)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 final relation table is incomplete");

  std::map<relation_request_key, feature_relation_id> relation_by_key;
  for (std::size_t i = 0; i < artifact.relations_.size(); ++i) {
    const auto &record = artifact.relations_[i];
    if (record.id.ordinal() != i ||
        record.producer.ordinal() >= artifact.request_graph_.requests.size() ||
        record.status == feature_relation_status::not_evaluated ||
        record.truth_begin > artifact.truth_records_.size() ||
        record.truth_count > artifact.truth_records_.size() - record.truth_begin ||
        record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 feature relation is malformed");
    const auto &producer =
        artifact.request_graph_.requests[record.producer.ordinal()];
    if (producer.key.scope != record.scope ||
        (record.scope == relation_record_scope::public_source_feature &&
         producer.key.scope != relation_record_scope::public_source_feature) ||
        !relation_by_key.emplace(producer.key, record.id).second)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 relation producer mapping is inconsistent");

    feature_relation_status expected = feature_relation_status::not_evaluated;
    switch (producer.key.family) {
    case relation_request_family::source_edge_source_edge: {
      const auto *request = find_request(artifact.source_edge_stage_->request_graph,
                                         producer.key);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 edge relation does not map to its detailed producer");
      const auto &source =
          artifact.source_edge_stage_->relations[request->id.ordinal()];
      expected = edge_status(source.contact, source.orientation);
      if (record.family != feature_relation_family::source_edge_source_edge)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge relation family mismatch");
      break;
    }
    case relation_request_family::source_edge_source_facet: {
      const auto *request = find_request(
          artifact.source_edge_facet_stage_->request_graph, producer.key);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 edge/facet relation does not map to its detailed producer");
      const auto &source = artifact.source_edge_facet_stage_->relations[
          request->id.ordinal()];
      expected = edge_facet_status(source.contact);
      if (record.family != feature_relation_family::source_edge_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge/facet relation family mismatch");
      break;
    }
    case relation_request_family::source_facet_source_facet: {
      const auto *request = find_request(artifact.source_facet_stage_->request_graph,
                                         producer.key);
      if (!request || request->id.ordinal() >=
                          artifact.source_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 facet relation does not map to its detailed producer");
      expected = facet_status(artifact.source_facet_stage_->relations[
                                  request->id.ordinal()]
                                  .classification);
      if (record.family != feature_relation_family::source_facet_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 facet relation family mismatch");
      break;
    }
    case relation_request_family::coplanar_source_facet_overlay: {
      const auto descriptor = std::lower_bound(
          overlay_descriptors.begin(), overlay_descriptors.end(), producer.key,
          [](const verifier_overlay_descriptor &candidate,
             const relation_request_key &key) { return candidate.key < key; });
      if (descriptor == overlay_descriptors.end() ||
          !(descriptor->key == producer.key))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 overlay relation does not map to its exact support lineage");
      const auto &source =
          artifact.coplanar_overlay_stage_->overlays[descriptor->ordinal];
      expected = overlay_status(source.classification);
      if (record.family != feature_relation_family::source_facet_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 overlay relation family mismatch");
      break;
    }
    default:
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 generic relation has a non-authoritative producer family");
    }
    if (record.status != expected)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 generic relation status disagrees with detailed evidence");
  }

  std::vector<relation_request_key> expected_imports;
  expected_imports.reserve(artifact.relations_.size() * 2U);
  for (const auto &relation : artifact.relations_) {
    const auto &key = artifact.request_graph_.requests[
                          relation.producer.ordinal()]
                          .key;
    expected_imports.push_back(verifier_imported_geometry_key(
        key.semantic_namespace, key.first, relation.scope));
    if (key.second.kind != relation_feature_kind::none)
      expected_imports.push_back(verifier_imported_geometry_key(
          key.semantic_namespace, key.second, relation.scope));
  }
  std::sort(expected_imports.begin(), expected_imports.end());
  expected_imports.erase(
      std::unique(expected_imports.begin(), expected_imports.end()),
      expected_imports.end());
  if (artifact.imported_geometry_.size() != expected_imports.size() ||
      artifact.bounded_primitives_.size() != artifact.truth_records_.size() ||
      artifact.truth_lineage_.size() != artifact.truth_records_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 primitive support tables are incomplete");
  for (std::size_t i = 0; i < expected_imports.size(); ++i) {
    const auto &record = artifact.imported_geometry_[i];
    const auto *producer = find_request(artifact.request_graph_,
                                        expected_imports[i]);
    if (!producer || record.id.ordinal() != i ||
        record.producer != producer->id ||
        record.feature != expected_imports[i].first ||
        record.scope != expected_imports[i].scope || record.reserved8 != 0 ||
        record.reserved16 != 0 || record.reserved32 != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 imported geometry record does not reconstruct");
  }

  std::size_t exact_ordinal = 0;
  for (const auto &relation : artifact.relations_) {
    const auto &relation_request = artifact.request_graph_.requests[
        relation.producer.ordinal()];
    for (std::uint64_t local = 0; local < relation.truth_count; ++local) {
      const auto truth_index = relation.truth_begin + local;
      if (truth_index >= artifact.truth_records_.size() ||
          local > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 primitive truth range is malformed");
      const auto &truth = artifact.truth_records_[truth_index];
      const auto &bounded = artifact.bounded_primitives_[truth_index];
      const auto &lineage = artifact.truth_lineage_[truth_index];
      const auto bounded_key = verifier_derived_key(
          relation_request.key,
          relation_request_family::rounded_bounded_primitive,
          verifier_tagged_use(
              0x10U, static_cast<std::uint8_t>(relation_request.key.family)),
          static_cast<std::uint32_t>(local));
      const auto *bounded_producer =
          find_request(artifact.request_graph_, bounded_key);
      if (!bounded_producer || bounded.id.ordinal() != truth_index ||
          bounded.producer != bounded_producer->id ||
          bounded.source_relation != relation.id ||
          bounded.truth_ordinal != local ||
          bounded.rounded_nominal_bits != truth.rounded_nominal_bits ||
          bounded.bounded_sign != truth.bounded_sign ||
          bounded.disposition != truth.disposition ||
          bounded.rounded_formula != truth.rounded_formula ||
          bounded.reserved16 != 0 || bounded.reserved32 != 0 ||
          lineage.id.ordinal() != truth_index ||
          lineage.source_relation != relation.id ||
          lineage.truth_ordinal != local ||
          lineage.bounded_primitive != bounded.id || lineage.reserved8 != 0 ||
          lineage.reserved16 != 0 || lineage.reserved32 != 0 ||
          !request_has_dependency(artifact.request_graph_, relation_request,
                                  bounded.producer))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 bounded primitive lineage does not reconstruct");

      const auto check_import_dependency =
          [&](const relation_feature_key &feature) {
            const auto key = verifier_imported_geometry_key(
                relation_request.key.semantic_namespace, feature,
                relation.scope);
            const auto *import = find_request(artifact.request_graph_, key);
            return import && request_has_dependency(
                                 artifact.request_graph_, *bounded_producer,
                                 import->id);
          };
      if (!check_import_dependency(relation_request.key.first) ||
          (relation_request.key.second.kind != relation_feature_kind::none &&
           !check_import_dependency(relation_request.key.second)))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 bounded primitive omits imported geometry lineage");

      if (truth.exact_formula != 0) {
        if (exact_ordinal >= artifact.exact_relations_.size())
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 exact relation table is truncated");
        const auto &exact = artifact.exact_relations_[exact_ordinal];
        const auto exact_key = verifier_derived_key(
            relation_request.key,
            relation_request_family::exact_stored_coordinate_relation,
            verifier_tagged_use(
                0x11U, static_cast<std::uint8_t>(relation_request.key.family)),
            static_cast<std::uint32_t>(local));
        const auto *exact_producer =
            find_request(artifact.request_graph_, exact_key);
        if (!exact_producer || exact.id.ordinal() != exact_ordinal ||
            exact.producer != exact_producer->id ||
            exact.source_relation != relation.id ||
            exact.truth_ordinal != local || exact.status != truth.exact_relation ||
            exact.exact_formula != truth.exact_formula ||
            exact.reserved16 != 0 || exact.reserved32 != 0 ||
            !lineage.has_exact_relation ||
            lineage.exact_relation != exact.id ||
            !request_has_dependency(artifact.request_graph_, relation_request,
                                    exact.producer))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 exact relation lineage does not reconstruct");
        const auto check_exact_import =
            [&](const relation_feature_key &feature) {
              const auto key = verifier_imported_geometry_key(
                  relation_request.key.semantic_namespace, feature,
                  relation.scope);
              const auto *import = find_request(artifact.request_graph_, key);
              return import && request_has_dependency(
                                   artifact.request_graph_, *exact_producer,
                                   import->id);
            };
        if (!check_exact_import(relation_request.key.first) ||
            (relation_request.key.second.kind != relation_feature_kind::none &&
             !check_exact_import(relation_request.key.second)))
          return fail(relation_subcode::missing_dependency,
                      "Component 07 exact relation omits imported geometry lineage");
        ++exact_ordinal;
      } else if (lineage.has_exact_relation) {
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 unavailable exact truth has an exact producer");
      }
    }
  }
  if (exact_ordinal != artifact.exact_relations_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 exact relation table has trailing records");

  for (const auto &truth : artifact.truth_records_)
    if (truth.reserved != 0 ||
        truth.bounded_sign == bounded_sign_status::invalid ||
        truth.exact_relation == exact_relation_status::invalid ||
        truth.disposition == predicate_disposition::fail_invalid)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 truth record is invalid");

  struct expected_interval_evidence final {
    relation_request_key key{};
    feature_relation_id source_relation{0};
    relation_interval_evidence_record value{};
  };
  struct expected_region_evidence final {
    relation_request_key key{};
    feature_relation_id source_relation{0};
    relation_source_facet_region_record<T> value{};
  };
  std::vector<expected_interval_evidence> expected_intervals;
  std::vector<expected_region_evidence> expected_regions;

  const auto accepted_unit_interval = [](const finite_interval<T> &interval) {
    return finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
           !finite_numeric_less(interval.upper(), interval.lower()) &&
           interval.lower() >= T(0) && interval.upper() <= T(1);
  };
  const auto accepted_residual = [](const finite_interval<T> &interval,
                                    T boundary) {
    return finite_bits(boundary) && boundary >= T(0) &&
           finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
           !finite_numeric_less(interval.upper(), interval.lower()) &&
           interval.lower() >= -boundary && interval.upper() <= boundary;
  };
  const auto set_contributors = [](relation_interval_evidence_record &out,
                                   const uncertainty_contributors &value) {
    const double contributors[]{value.inherited_a, value.inherited_b,
                                value.machine_floor, value.construction,
                                value.conditioning, value.conversion,
                                value.prior_cleanup, value.current_cleanup};
    for (std::size_t i = 0; i < out.contributor_bits.size(); ++i)
      out.contributor_bits[i] =
          static_cast<std::uint64_t>(to_bits(contributors[i]));
  };
  const auto primitive_dependencies = [&](const feature_relation_record &relation,
                                          const relation_request_key &base,
                                          std::vector<relation_request_id> &out) {
    out.clear();
    const auto add = [&](const relation_request_key &key) {
      const auto *request = find_request(artifact.request_graph_, key);
      if (!request) return false;
      out.push_back(request->id);
      return true;
    };
    if (!add(verifier_imported_geometry_key(base.semantic_namespace, base.first,
                                            relation.scope)) ||
        (base.second.kind != relation_feature_kind::none &&
         !add(verifier_imported_geometry_key(base.semantic_namespace,
                                             base.second, relation.scope))))
      return false;
    for (std::uint64_t local = 0; local < relation.truth_count; ++local) {
      if (local > std::numeric_limits<std::uint32_t>::max()) return false;
      const auto truth_index = relation.truth_begin + local;
      if (truth_index >= artifact.truth_records_.size()) return false;
      if (!add(verifier_derived_key(
              base, relation_request_family::rounded_bounded_primitive,
              verifier_tagged_use(0x10U,
                                  static_cast<std::uint8_t>(base.family)),
              static_cast<std::uint32_t>(local))))
        return false;
      if (artifact.truth_records_[truth_index].exact_formula != 0 &&
          !add(verifier_derived_key(
              base,
              relation_request_family::exact_stored_coordinate_relation,
              verifier_tagged_use(0x11U,
                                  static_cast<std::uint8_t>(base.family)),
              static_cast<std::uint32_t>(local))))
        return false;
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return true;
  };
  const auto exact_dependencies = [&](const canonical_relation_request &request,
                                      std::vector<relation_request_id> expected) {
    if (request.dependency_begin > artifact.request_graph_.dependencies.size() ||
        request.dependency_count >
            artifact.request_graph_.dependencies.size() - request.dependency_begin)
      return false;
    std::vector<relation_request_id> actual;
    actual.reserve(request.dependency_count);
    for (std::uint64_t offset = 0; offset < request.dependency_count; ++offset)
      actual.push_back(artifact.request_graph_
                           .dependencies[request.dependency_begin + offset]
                           .producer);
    std::sort(actual.begin(), actual.end());
    actual.erase(std::unique(actual.begin(), actual.end()), actual.end());
    std::sort(expected.begin(), expected.end());
    expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
    return actual == expected;
  };
  const auto next_interval = [](relation_interval_evidence_kind kind,
                                std::array<std::uint64_t, 16> &counters,
                                std::uint32_t &out) {
    const auto index = static_cast<std::size_t>(kind);
    if (index == 0 || index >= counters.size() ||
        counters[index] > std::numeric_limits<std::uint32_t>::max())
      return false;
    out = static_cast<std::uint32_t>(counters[index]++);
    return true;
  };
  const auto next_region = [](relation_source_facet_region_kind kind,
                              std::array<std::uint64_t, 7> &counters,
                              std::uint32_t &out) {
    const auto index = static_cast<std::size_t>(kind);
    if (index == 0 || index >= counters.size() ||
        counters[index] > std::numeric_limits<std::uint32_t>::max())
      return false;
    out = static_cast<std::uint32_t>(counters[index]++);
    return true;
  };
  const auto append_interval = [&](const relation_request_key &base,
                                   feature_relation_id source_relation,
                                   relation_interval_evidence_kind kind,
                                   std::uint32_t occurrence,
                                   std::uint8_t component,
                                   const finite_interval<T> &interval,
                                   bool has_rounded_nominal, T rounded,
                                   bool has_parameter_metadata,
                                   parameter_domain_status domain,
                                   T domain_margin,
                                   exact_relation_status exact_zero,
                                   exact_relation_status exact_one,
                                   const uncertainty_contributors &contributors,
                                   std::uint64_t trace_root,
                                   T comparison_boundary,
                                   bool within_authorized_boundary) {
    if (!finite_bits(interval.lower()) || !finite_bits(interval.upper()) ||
        finite_numeric_less(interval.upper(), interval.lower()) ||
        (has_rounded_nominal &&
         (!finite_bits(rounded) || !interval.contains(rounded))) ||
        !finite_bits(domain_margin) || !finite_bits(comparison_boundary))
      return false;
    expected_interval_evidence expected;
    expected.key = verifier_derived_key(
        base, relation_request_family::source_point_source_facet_region,
        verifier_tagged_use(0x20U, static_cast<std::uint8_t>(kind)) |
            static_cast<std::uint64_t>(component),
        occurrence);
    expected.source_relation = source_relation;
    expected.value.kind = kind;
    expected.value.occurrence = occurrence;
    expected.value.component = component;
    expected.value.has_rounded_nominal = has_rounded_nominal;
    expected.value.has_parameter_metadata = has_parameter_metadata;
    expected.value.within_authorized_boundary = within_authorized_boundary;
    expected.value.rounded_nominal_bits =
        has_rounded_nominal ? static_cast<std::uint64_t>(to_bits(rounded)) : 0;
    expected.value.lower_bits =
        static_cast<std::uint64_t>(to_bits(interval.lower()));
    expected.value.upper_bits =
        static_cast<std::uint64_t>(to_bits(interval.upper()));
    expected.value.domain = has_parameter_metadata
                                ? domain
                                : parameter_domain_status::invalid;
    expected.value.domain_margin_bits =
        has_parameter_metadata
            ? static_cast<std::uint64_t>(to_bits(domain_margin))
            : 0;
    expected.value.exact_zero = has_parameter_metadata
                                    ? exact_zero
                                    : exact_relation_status::unavailable;
    expected.value.exact_one = has_parameter_metadata
                                   ? exact_one
                                   : exact_relation_status::unavailable;
    if (has_parameter_metadata)
      set_contributors(expected.value, contributors);
    expected.value.trace_root = has_parameter_metadata ? trace_root : 0;
    expected.value.comparison_boundary_bits =
        static_cast<std::uint64_t>(to_bits(comparison_boundary));
    expected_intervals.push_back(std::move(expected));
    return true;
  };
  const auto append_parameter = [&](const relation_request_key &base,
                                    feature_relation_id source_relation,
                                    relation_interval_evidence_kind kind,
                                    std::uint32_t occurrence,
                                    const source_edge_parameter_evidence<T> &p) {
    return append_interval(base, source_relation, kind, occurrence, 0,
                           p.enclosure, true, p.rounded_nominal, true, p.domain,
                           p.domain_margin, p.exact_zero, p.exact_one,
                           p.contributors, p.trace_root, T(0),
                           p.domain != parameter_domain_status::invalid);
  };
  const auto append_simple_parameter =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_interval_evidence_kind kind, std::uint32_t occurrence,
          T rounded, const finite_interval<T> &interval) {
        return append_interval(
            base, source_relation, kind, occurrence, 0, interval, true, rounded,
            false, parameter_domain_status::invalid, T(0),
            exact_relation_status::unavailable,
            exact_relation_status::unavailable, uncertainty_contributors{}, 0,
            T(0), accepted_unit_interval(interval));
      };
  const auto append_plain =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_interval_evidence_kind kind, std::uint32_t occurrence,
          std::uint8_t component, const finite_interval<T> &interval,
          T comparison_boundary, bool accepted) {
        return append_interval(
            base, source_relation, kind, occurrence, component, interval,
            false, T(0), false, parameter_domain_status::invalid, T(0),
            exact_relation_status::unavailable,
            exact_relation_status::unavailable, uncertainty_contributors{}, 0,
            comparison_boundary, accepted);
      };
  const auto append_region_snapshot =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_source_facet_region_kind kind, std::uint32_t occurrence,
          const source_edge_geometry_snapshot<T> &point,
          const source_facet_point_region_record<T> &region) {
        if (!valid_source_facet_point_region_record(region)) return false;
        expected_region_evidence expected;
        expected.key = verifier_derived_key(
            base, relation_request_family::source_point_source_facet_region,
            verifier_tagged_use(0x21U, static_cast<std::uint8_t>(kind)),
            occurrence);
        expected.source_relation = source_relation;
        expected.value.kind = kind;
        expected.value.occurrence = occurrence;
        expected.value.query_component_count = 3;
        expected.value.query_source_identity_valid =
            region.query_source_identity_valid;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          expected.value.query_nominal_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.rounded_nominal[axis]));
          expected.value.query_lower_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].lower()));
          expected.value.query_upper_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].upper()));
        }
        expected.value.region = region;
        expected_regions.push_back(std::move(expected));
        return true;
      };
  const auto append_region_projected =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_source_facet_region_kind kind, std::uint32_t occurrence,
          const projected_source_point<T> &point,
          const source_facet_point_region_record<T> &region) {
        if (!valid_source_facet_point_region_record(region)) return false;
        expected_region_evidence expected;
        expected.key = verifier_derived_key(
            base, relation_request_family::source_point_source_facet_region,
            verifier_tagged_use(0x21U, static_cast<std::uint8_t>(kind)),
            occurrence);
        expected.source_relation = source_relation;
        expected.value.kind = kind;
        expected.value.occurrence = occurrence;
        expected.value.query_component_count = 2;
        expected.value.query_source_identity_valid =
            region.query_source_identity_valid;
        for (std::size_t axis = 0; axis < 2; ++axis) {
          expected.value.query_nominal_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.nominal[axis]));
          expected.value.query_lower_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].lower()));
          expected.value.query_upper_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].upper()));
        }
        expected.value.region = region;
        expected_regions.push_back(std::move(expected));
        return true;
      };
  const auto append_partition =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          const source_facet_segment_partition_record<T> &partition,
          relation_source_facet_region_kind breakpoint_kind,
          relation_source_facet_region_kind interval_kind,
          std::array<std::uint64_t, 16> &interval_counters,
          std::array<std::uint64_t, 7> &region_counters) {
        if (!valid_source_facet_segment_partition_record(partition)) return false;
        for (const auto &contact : partition.contacts) {
          std::uint32_t occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_contact_first_parameter,
                  interval_counters, occurrence) ||
              !append_simple_parameter(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_contact_first_parameter,
                  occurrence, contact.first_rounded_parameter,
                  contact.first_parameter))
            return false;
          if (contact.kind ==
              source_facet_segment_contact_kind::boundary_overlap) {
            if (!next_interval(
                    relation_interval_evidence_kind::segment_contact_second_parameter,
                    interval_counters, occurrence) ||
                !append_simple_parameter(
                    base, source_relation,
                    relation_interval_evidence_kind::segment_contact_second_parameter,
                    occurrence, contact.second_rounded_parameter,
                    contact.second_parameter))
              return false;
          }
        }
        for (const auto &breakpoint : partition.breakpoints) {
          std::uint32_t interval_occurrence = 0, region_occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_breakpoint_parameter,
                  interval_counters, interval_occurrence) ||
              !append_simple_parameter(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_breakpoint_parameter,
                  interval_occurrence, breakpoint.rounded_parameter,
                  breakpoint.parameter) ||
              !next_region(breakpoint_kind, region_counters,
                           region_occurrence) ||
              !append_region_projected(base, source_relation, breakpoint_kind,
                                       region_occurrence, breakpoint.point,
                                       breakpoint.region))
            return false;
        }
        for (const auto &interval : partition.intervals) {
          std::uint32_t interval_occurrence = 0, region_occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_interval_witness_parameter,
                  interval_counters, interval_occurrence) ||
              !append_simple_parameter(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_interval_witness_parameter,
                  interval_occurrence, interval.rounded_witness_parameter,
                  interval.witness_parameter) ||
              !next_region(interval_kind, region_counters,
                           region_occurrence) ||
              !append_region_projected(base, source_relation, interval_kind,
                                       region_occurrence,
                                       interval.witness_point,
                                       interval.witness_region))
            return false;
        }
        for (const auto &witness : partition.triangle_witnesses) {
          std::uint32_t occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_triangle_witness_parameter,
                  interval_counters, occurrence) ||
              !append_plain(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_triangle_witness_parameter,
                  occurrence, 0, witness.parameter, T(0),
                  accepted_unit_interval(witness.parameter)))
            return false;
        }
        return true;
      };

  for (const auto &relation : artifact.relations_) {
    const auto &base = artifact.request_graph_.requests[relation.producer.ordinal()].key;
    std::array<std::uint64_t, 16> interval_counters{};
    std::array<std::uint64_t, 7> region_counters{};
    switch (base.family) {
    case relation_request_family::source_edge_source_edge: {
      const auto *request = find_request(artifact.source_edge_stage_->request_graph,
                                         base);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 edge source is absent");
      const auto &source =
          artifact.source_edge_stage_->relations[request->id.ordinal()];
      for (std::uint32_t i = 0; i < source.parameter_count; ++i) {
        std::uint32_t occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::source_edge_first_parameter,
                interval_counters, occurrence) ||
            !append_parameter(
                base, relation.id,
                relation_interval_evidence_kind::source_edge_first_parameter,
                occurrence, source.first_parameters[i]) ||
            !next_interval(
                relation_interval_evidence_kind::source_edge_second_parameter,
                interval_counters, occurrence) ||
            !append_parameter(
                base, relation.id,
                relation_interval_evidence_kind::source_edge_second_parameter,
                occurrence, source.second_parameters[i]))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 edge parameters do not reconstruct");
      }
      for (std::uint32_t point = 0; point < source.point_count; ++point) {
        std::uint32_t first_occurrence = 0, second_occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::source_edge_first_carrier_residual,
                interval_counters, first_occurrence) ||
            !next_interval(
                relation_interval_evidence_kind::source_edge_second_carrier_residual,
                interval_counters, second_occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 edge residual occurrence overflow");
        for (std::uint8_t axis = 0; axis < 3; ++axis) {
          const auto &first = source.points[point].first_carrier_residual[axis];
          const auto &second = source.points[point].second_carrier_residual[axis];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::source_edge_first_carrier_residual,
                  first_occurrence, axis, first, source.residual_boundary,
                  accepted_residual(first, source.residual_boundary)) ||
              !append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::source_edge_second_carrier_residual,
                  second_occurrence, axis, second, source.residual_boundary,
                  accepted_residual(second, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 edge residuals do not reconstruct");
        }
      }
      break;
    }
    case relation_request_family::source_edge_source_facet: {
      const auto *request = find_request(
          artifact.source_edge_facet_stage_->request_graph, base);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 edge/facet source is absent");
      const auto &source = artifact.source_edge_facet_stage_->relations[
          request->id.ordinal()];
      for (const auto &event : source.events) {
        std::uint32_t parameter_occurrence = 0, residual_occurrence = 0;
        std::uint32_t support_occurrence = 0, region_occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::edge_facet_event_parameter,
                interval_counters, parameter_occurrence) ||
            !append_parameter(
                base, relation.id,
                relation_interval_evidence_kind::edge_facet_event_parameter,
                parameter_occurrence, event.parameter) ||
            !next_interval(
                relation_interval_evidence_kind::edge_facet_edge_carrier_residual,
                interval_counters, residual_occurrence) ||
            !next_interval(
                relation_interval_evidence_kind::edge_facet_support_residual,
                interval_counters, support_occurrence) ||
            !next_region(relation_source_facet_region_kind::edge_facet_event,
                         region_counters, region_occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 edge/facet occurrence overflow");
        for (std::uint8_t axis = 0; axis < 3; ++axis) {
          const auto &residual = event.construction.edge_carrier_residual[axis];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::edge_facet_edge_carrier_residual,
                  residual_occurrence, axis, residual,
                  source.residual_boundary,
                  accepted_residual(residual, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 edge/facet carrier residual does not reconstruct");
        }
        if (!append_plain(
                base, relation.id,
                relation_interval_evidence_kind::edge_facet_support_residual,
                support_occurrence, 0, event.construction.support_residual,
                source.residual_boundary,
                accepted_residual(event.construction.support_residual,
                                  source.residual_boundary)) ||
            !append_region_snapshot(
                base, relation.id,
                relation_source_facet_region_kind::edge_facet_event,
                region_occurrence, event.construction.point, event.region))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 edge/facet event does not reconstruct");
      }
      if (source.has_coplanar_partition &&
          !append_partition(
              base, relation.id, source.coplanar_partition,
              relation_source_facet_region_kind::edge_facet_partition_breakpoint,
              relation_source_facet_region_kind::edge_facet_partition_interval,
              interval_counters, region_counters))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 family-04 edge/facet partition does not reconstruct");
      break;
    }
    case relation_request_family::source_facet_source_facet: {
      const auto *request = find_request(
          artifact.source_facet_stage_->request_graph, base);
      if (!request || request->id.ordinal() >=
                          artifact.source_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 facet source is absent");
      const auto &source =
          artifact.source_facet_stage_->relations[request->id.ordinal()];
      if (source.has_transverse_carrier) {
        std::uint32_t occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::facet_facet_direction_squared,
                interval_counters, occurrence) ||
            !append_plain(
                base, relation.id,
                relation_interval_evidence_kind::facet_facet_direction_squared,
                occurrence, 0, source.transverse_carrier.direction_squared,
                T(0), source.transverse_carrier.direction_squared.lower() > T(0)))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 carrier conditioning does not reconstruct");
        if (!next_interval(
                relation_interval_evidence_kind::facet_facet_point_plane_residual,
                interval_counters, occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 point-plane occurrence overflow");
        for (std::uint8_t component = 0; component < 2; ++component) {
          const auto &residual =
              source.transverse_carrier.point_plane_residuals[component];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::facet_facet_point_plane_residual,
                  occurrence, component, residual, source.residual_boundary,
                  accepted_residual(residual, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 point-plane residual does not reconstruct");
        }
        if (!next_interval(
                relation_interval_evidence_kind::facet_facet_direction_plane_residual,
                interval_counters, occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 direction-plane occurrence overflow");
        for (std::uint8_t component = 0; component < 2; ++component) {
          const auto &residual =
              source.transverse_carrier.direction_plane_residuals[component];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::facet_facet_direction_plane_residual,
                  occurrence, component, residual, source.residual_boundary,
                  accepted_residual(residual, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 direction-plane residual does not reconstruct");
        }
      }
      break;
    }
    case relation_request_family::coplanar_source_facet_overlay: {
      const auto descriptor = std::lower_bound(
          overlay_descriptors.begin(), overlay_descriptors.end(), base,
          [](const verifier_overlay_descriptor &candidate,
             const relation_request_key &key) { return candidate.key < key; });
      if (descriptor == overlay_descriptors.end() || !(descriptor->key == base))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 overlay source is absent");
      const auto &source =
          artifact.coplanar_overlay_stage_->overlays[descriptor->ordinal];
      for (const auto &witness : source.vertex_regions) {
        if (witness.polygon >= source.facets.size() ||
            witness.vertex_ordinal >=
                source.facets[witness.polygon].polygon.size())
          return fail(relation_subcode::coplanar_overlay_region_unresolved,
                      "Component 07 family-04 overlay witness is out of range");
        std::uint32_t occurrence = 0;
        if (!next_region(
                relation_source_facet_region_kind::overlay_vertex_witness,
                region_counters, occurrence) ||
            !append_region_projected(
                base, relation.id,
                relation_source_facet_region_kind::overlay_vertex_witness,
                occurrence,
                source.facets[witness.polygon].polygon[witness.vertex_ordinal],
                witness.region))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 overlay vertex region does not reconstruct");
      }
      for (const auto &partition : source.boundary_partitions)
        if (!append_partition(
                base, relation.id, partition.partition,
                relation_source_facet_region_kind::overlay_partition_breakpoint,
                relation_source_facet_region_kind::overlay_partition_interval,
                interval_counters, region_counters))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 overlay partition does not reconstruct");
      break;
    }
    default:
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 family-04 source family is invalid");
    }
  }

  std::sort(expected_intervals.begin(), expected_intervals.end(),
            [](const expected_interval_evidence &a,
               const expected_interval_evidence &b) { return a.key < b.key; });
  std::sort(expected_regions.begin(), expected_regions.end(),
            [](const expected_region_evidence &a,
               const expected_region_evidence &b) { return a.key < b.key; });
  if (artifact.interval_evidence_.size() != expected_intervals.size() ||
      artifact.source_facet_regions_.size() != expected_regions.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 family-04 evidence tables are incomplete");

  const auto same_interval = [](const relation_interval_evidence_record &a,
                                const relation_interval_evidence_record &b) {
    return a.kind == b.kind && a.occurrence == b.occurrence &&
           a.component == b.component &&
           a.has_rounded_nominal == b.has_rounded_nominal &&
           a.has_parameter_metadata == b.has_parameter_metadata &&
           a.within_authorized_boundary == b.within_authorized_boundary &&
           a.rounded_nominal_bits == b.rounded_nominal_bits &&
           a.lower_bits == b.lower_bits && a.upper_bits == b.upper_bits &&
           a.domain == b.domain &&
           a.domain_margin_bits == b.domain_margin_bits &&
           a.exact_zero == b.exact_zero && a.exact_one == b.exact_one &&
           a.contributor_bits == b.contributor_bits &&
           a.trace_root == b.trace_root &&
           a.comparison_boundary_bits == b.comparison_boundary_bits &&
           a.reserved8 == 0 && a.reserved16 == 0 && a.reserved32 == 0;
  };
  for (std::size_t i = 0; i < expected_intervals.size(); ++i) {
    const auto &expected = expected_intervals[i];
    const auto &record = artifact.interval_evidence_[i];
    const auto *producer = find_request(artifact.request_graph_, expected.key);
    if (!producer || record.id.ordinal() != i ||
        record.producer != producer->id ||
        record.source_relation != expected.source_relation ||
        !same_interval(record, expected.value))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 interval evidence does not reconstruct");
    const auto &base_request = artifact.request_graph_.requests[
        artifact.relations_[record.source_relation.ordinal()].producer.ordinal()];
    std::vector<relation_request_id> dependencies;
    if (!primitive_dependencies(
            artifact.relations_[record.source_relation.ordinal()],
            base_request.key, dependencies) ||
        !exact_dependencies(*producer, dependencies) ||
        !request_has_dependency(artifact.request_graph_, base_request,
                                producer->id))
      return fail(relation_subcode::missing_dependency,
                  "Component 07 interval evidence dependency closure is incomplete");
  }

  const auto region_bytes = [](const source_facet_point_region_record<T> &region) {
    canonical_writer writer;
    source_edge_facet_detail::encode_region(writer, region);
    return writer.take();
  };
  for (std::size_t i = 0; i < expected_regions.size(); ++i) {
    const auto &expected = expected_regions[i];
    const auto &record = artifact.source_facet_regions_[i];
    const auto *producer = find_request(artifact.request_graph_, expected.key);
    if (!producer || record.id.ordinal() != i ||
        record.producer != producer->id ||
        record.source_relation != expected.source_relation ||
        record.kind != expected.value.kind ||
        record.occurrence != expected.value.occurrence ||
        record.query_component_count != expected.value.query_component_count ||
        record.query_source_identity_valid !=
            expected.value.query_source_identity_valid ||
        record.query_nominal_bits != expected.value.query_nominal_bits ||
        record.query_lower_bits != expected.value.query_lower_bits ||
        record.query_upper_bits != expected.value.query_upper_bits ||
        !valid_source_facet_point_region_record(record.region) ||
        region_bytes(record.region) != region_bytes(expected.value.region) ||
        record.reserved8 != 0 || record.reserved16 != 0 ||
        record.reserved32 != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 source-facet region evidence does not reconstruct");
    const auto &base_request = artifact.request_graph_.requests[
        artifact.relations_[record.source_relation.ordinal()].producer.ordinal()];
    std::vector<relation_request_id> dependencies;
    if (!primitive_dependencies(
            artifact.relations_[record.source_relation.ordinal()],
            base_request.key, dependencies) ||
        !exact_dependencies(*producer, dependencies) ||
        !request_has_dependency(artifact.request_graph_, base_request,
                                producer->id))
      return fail(relation_subcode::missing_dependency,
                  "Component 07 source-facet region dependency closure is incomplete");
  }

  std::size_t expected_constructions = 0;
  for (const auto &record : artifact.source_edge_stage_->relations)
    expected_constructions += record.point_count;
  for (const auto &record : artifact.source_edge_facet_stage_->relations)
    expected_constructions += record.events.size();
  for (const auto &record : artifact.source_facet_stage_->relations)
    expected_constructions += record.has_transverse_carrier ? 1U : 0U;
  for (const auto &record : artifact.coplanar_overlay_stage_->overlays)
    expected_constructions += record.event_nodes.size();
  if (artifact.constructions_.size() != expected_constructions)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 authoritative construction table is incomplete");
  std::map<relation_request_key, relation_construction_id>
      construction_by_key;
  for (std::size_t i = 0; i < artifact.constructions_.size(); ++i) {
    const auto &record = artifact.constructions_[i];
    if (record.id.ordinal() != i ||
        record.producer.ordinal() >= artifact.request_graph_.requests.size() ||
        artifact.request_graph_.requests[record.producer.ordinal()].key.family !=
            relation_request_family::authoritative_construction ||
        record.component_count == 0 || record.component_count > 6 ||
        record.residual_truth_begin > artifact.truth_records_.size() ||
        record.residual_truth_count >
            artifact.truth_records_.size() - record.residual_truth_begin ||
        !record.finite || !record.tolerance_compatible || record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 construction record is malformed");
    for (std::size_t component = 0; component < record.component_count; ++component)
      if (!finite_construction_component<T>(record, component))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 construction enclosure is not finite and ordered");
    const auto &producer =
        artifact.request_graph_.requests[record.producer.ordinal()];
    if (!construction_by_key.emplace(producer.key, record.id).second)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 construction producer is duplicated");
  }

  std::size_t expected_node = 0;
  std::size_t expected_arc = 0;
  std::size_t expected_component = 0;
  for (const auto &descriptor : overlay_descriptors) {
    const auto relation = relation_by_key.find(descriptor.key);
    if (relation == relation_by_key.end())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 coplanar final relation is absent");
    const auto &source =
        artifact.coplanar_overlay_stage_->overlays[descriptor.ordinal];
    if (!source.complete_event_lineage ||
        !source.complete_authorized_arc_coverage ||
        !source.complete_overlap_component_assembly)
      return fail(relation_subcode::coplanar_overlay_invariant,
                  "Component 07 coplanar predecessor topology is incomplete");
    const auto node_begin = expected_node;
    const auto arc_begin = expected_arc;

    for (std::size_t local = 0; local < source.event_nodes.size(); ++local) {
      if (expected_node >= artifact.coplanar_event_nodes_.size() ||
          local > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar event-node table is incomplete");
      const auto &expected = source.event_nodes[local];
      const auto &record = artifact.coplanar_event_nodes_[expected_node];
      const auto construction_key = verifier_derived_key(
          descriptor.key, relation_request_family::authoritative_construction,
          verifier_tagged_use(10, 4), static_cast<std::uint32_t>(local));
      const auto construction = construction_by_key.find(construction_key);
      if (expected.id != local || construction == construction_by_key.end() ||
          record.id.ordinal() != expected_node ||
          record.overlay_relation != relation->second ||
          record.representative != construction->second ||
          record.occurrences.size() != expected.occurrences.size() ||
          record.distinct_sheet_occurrences !=
              source.distinct_sheet_occurrences ||
          record.reserved16 != 0 || record.reserved32 != 0)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar event node does not reconstruct");
      std::uint8_t sheet_mask = 0;
      for (std::size_t occurrence = 0;
           occurrence < expected.occurrences.size(); ++occurrence) {
        const auto &a = expected.occurrences[occurrence];
        const auto &b = record.occurrences[occurrence];
        if (a.polygon > 1 || b.polygon != a.polygon ||
            b.edge_ordinal != a.edge_ordinal ||
            b.breakpoint_ordinal != a.breakpoint_ordinal ||
            b.query_source_vertex_valid != a.query_source_vertex_valid ||
            b.query_source_vertex != a.query_source_vertex ||
            b.event_lineages.size() != a.event_lineages.size() ||
            b.reserved8 != 0 || b.reserved16 != 0 || b.reserved32 != 0)
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar node occurrence does not reconstruct");
        sheet_mask = static_cast<std::uint8_t>(
            sheet_mask | (std::uint8_t{1} << a.polygon));
        for (std::size_t lineage = 0; lineage < a.event_lineages.size();
             ++lineage) {
          const auto &x = a.event_lineages[lineage];
          const auto &y = b.event_lineages[lineage];
          if (y.contact_lineage != x.contact_lineage ||
              y.endpoint_role != x.endpoint_role || y.reserved8 != 0 ||
              y.reserved16 != 0)
            return fail(relation_subcode::coplanar_overlay_invariant,
                        "Component 07 final coplanar event lineage does not reconstruct");
        }
      }
      if (record.sheet_mask != sheet_mask)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar node sheet mask is inconsistent");
      ++expected_node;
    }

    for (std::size_t local = 0; local < source.oriented_arcs.size(); ++local) {
      if (expected_arc >= artifact.coplanar_oriented_arcs_.size())
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar arc table is incomplete");
      const auto &expected = source.oriented_arcs[local];
      const auto &record = artifact.coplanar_oriented_arcs_[expected_arc];
      if (expected.id != local || expected.start_node >= source.event_nodes.size() ||
          expected.end_node >= source.event_nodes.size() ||
          record.id.ordinal() != expected_arc ||
          record.overlay_relation != relation->second ||
          record.kind != verifier_coplanar_arc_kind(expected.kind) ||
          record.start_node.ordinal() != node_begin + expected.start_node ||
          record.end_node.ordinal() != node_begin + expected.end_node ||
          record.occurrences.size() != expected.occurrences.size() ||
          record.overlap_lineages.size() != expected.overlap_lineages.size() ||
          record.reserved8 != 0 || record.reserved16 != 0 ||
          record.reserved32 != 0)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar oriented arc does not reconstruct");
      std::uint8_t sheet_mask = 0;
      for (std::size_t occurrence = 0;
           occurrence < expected.occurrences.size(); ++occurrence) {
        const auto &a = expected.occurrences[occurrence];
        const auto &b = record.occurrences[occurrence];
        if (a.polygon > 1 || a.start_node >= source.event_nodes.size() ||
            a.end_node >= source.event_nodes.size() ||
            b.polygon != a.polygon || b.edge_ordinal != a.edge_ordinal ||
            b.interval_ordinal != a.interval_ordinal ||
            b.start_node.ordinal() != node_begin + a.start_node ||
            b.end_node.ordinal() != node_begin + a.end_node ||
            b.forward_along_source_edge != a.forward_along_source_edge ||
            b.reserved8 != 0 || b.reserved16 != 0 || b.reserved32 != 0)
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar arc occurrence does not reconstruct");
        sheet_mask = static_cast<std::uint8_t>(
            sheet_mask | (std::uint8_t{1} << a.polygon));
      }
      if (record.sheet_mask != sheet_mask)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar arc sheet mask is inconsistent");
      for (std::size_t lineage = 0;
           lineage < expected.overlap_lineages.size(); ++lineage) {
        const auto source_id = expected.overlap_lineages[lineage];
        if (source_id.ordinal() >=
            artifact.source_edge_stage_->request_graph.requests.size())
          return fail(relation_subcode::coplanar_overlay_dependency_missing,
                      "Component 07 coplanar source lineage is out of range");
        const auto &key = artifact.source_edge_stage_->request_graph
                              .requests[source_id.ordinal()]
                              .key;
        const auto *published = find_request(artifact.request_graph_, key);
        if (!published || record.overlap_lineages[lineage] != published->id)
          return fail(relation_subcode::coplanar_overlay_dependency_missing,
                      "Component 07 final coplanar arc lineage does not reconstruct");
      }
      ++expected_arc;
    }

    for (std::size_t local = 0; local < source.overlap_components.size(); ++local) {
      if (expected_component >=
          artifact.coplanar_overlap_components_.size())
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar component table is incomplete");
      const auto &expected = source.overlap_components[local];
      const auto &record =
          artifact.coplanar_overlap_components_[expected_component];
      if (expected.id != local || record.id.ordinal() != expected_component ||
          record.overlay_relation != relation->second ||
          record.kind != verifier_coplanar_component_kind(expected.kind) ||
          record.node_ids.size() != expected.node_ids.size() ||
          record.arc_ids.size() != expected.arc_ids.size() ||
          record.sheet_mask != expected.sheet_mask ||
          record.closed != expected.closed ||
          record.distinct_sheet_occurrences !=
              source.distinct_sheet_occurrences ||
          record.reserved8 != 0 || record.reserved16 != 0 ||
          record.reserved32 != 0)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar component does not reconstruct");
      for (std::size_t node = 0; node < expected.node_ids.size(); ++node)
        if (expected.node_ids[node] >= source.event_nodes.size() ||
            record.node_ids[node].ordinal() !=
                node_begin + expected.node_ids[node])
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar component node does not reconstruct");
      for (std::size_t arc = 0; arc < expected.arc_ids.size(); ++arc)
        if (expected.arc_ids[arc] >= source.oriented_arcs.size() ||
            record.arc_ids[arc].ordinal() != arc_begin + expected.arc_ids[arc])
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar component arc does not reconstruct");
      ++expected_component;
    }
  }
  if (expected_node != artifact.coplanar_event_nodes_.size() ||
      expected_arc != artifact.coplanar_oriented_arcs_.size() ||
      expected_component != artifact.coplanar_overlap_components_.size())
    return fail(relation_subcode::coplanar_overlay_invariant,
                "Component 07 final coplanar topology has trailing records");

  if (artifact.symbolic_eligibility_.size() !=
      artifact.symbolic_decisions_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 symbolic eligibility/decision counts disagree");
  const auto symbolic = materialize_symbolic_policy();
  for (std::size_t i = 0; i < artifact.symbolic_decisions_.size(); ++i) {
    const auto &eligibility = artifact.symbolic_eligibility_[i];
    const auto &decision = artifact.symbolic_decisions_[i];
    if (eligibility.request.family !=
            relation_request_family::symbolic_eligibility ||
        !find_request(artifact.request_graph_, eligibility.request) ||
        decision.id.ordinal() != i ||
        decision.operation != artifact.operation_ ||
        !decision.nominal_geometry_unchanged || decision.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic record violates its publication boundary");
    const auto *eligibility_request =
        find_request(artifact.request_graph_, eligibility.request);
    const relation_request_key *source_key = nullptr;
    const canonical_relation_request *construction_request = nullptr;
    if (!eligibility_request)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic eligibility request is absent");
    for (std::uint64_t offset = 0;
         offset < eligibility_request->dependency_count; ++offset) {
      const auto dependency_index =
          eligibility_request->dependency_begin + offset;
      if (dependency_index >= artifact.request_graph_.dependencies.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic dependency range is malformed");
      const auto producer_id =
          artifact.request_graph_.dependencies[dependency_index].producer;
      if (producer_id.ordinal() >= artifact.request_graph_.requests.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic dependency producer is absent");
      const auto &producer =
          artifact.request_graph_.requests[producer_id.ordinal()];
      if (verifier_symbolic_source_family(producer.key.family)) {
        if (source_key)
          return fail(relation_subcode::duplicate_authoritative_producer,
                      "Component 07 symbolic eligibility has multiple source relations");
        source_key = &producer.key;
      } else if (producer.key.family ==
                 relation_request_family::authoritative_construction) {
        if (construction_request)
          return fail(relation_subcode::duplicate_authoritative_producer,
                      "Component 07 symbolic eligibility has multiple constructions");
        construction_request = &producer;
      }
    }
    if (!source_key)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic eligibility source relation is absent");

    bool construction_tolerance_compatible = true;
    if (construction_request) {
      const relation_construction_record *construction = nullptr;
      for (const auto &candidate : artifact.constructions_)
        if (candidate.producer == construction_request->id) {
          if (construction)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic construction producer is duplicated");
          construction = &candidate;
        }
      if (!construction)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic construction record is absent");
      construction_tolerance_compatible = construction->tolerance_compatible;
    }

    symbolic_eligibility_record expected_eligibility;
    expected_eligibility.request = eligibility.request;
    bool reconstructed_evidence = false;
    const auto occurrence = eligibility.request.occurrence_discriminator;
    switch (source_key->family) {
    case relation_request_family::source_edge_source_edge: {
      const auto *source_request =
          find_request(artifact.source_edge_stage_->request_graph, *source_key);
      if (!source_request ||
          source_request->id.ordinal() >=
              artifact.source_edge_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge source is absent");
      const auto &source = artifact.source_edge_stage_->relations[
          source_request->id.ordinal()];
      const source_edge_point_construction<T> *point = nullptr;
      if (construction_request) {
        if (occurrence >= source.point_count)
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 symbolic edge occurrence is out of range");
        point = &source.points[occurrence];
      }
      if (source.contact == source_edge_contact_class::partial_overlap ||
          source.contact == source_edge_contact_class::first_contains_second ||
          source.contact == source_edge_contact_class::second_contains_first ||
          source.contact == source_edge_contact_class::equal) {
        reconstructed_evidence =
            source.has_collinearity_truth &&
            verifier_set_truth_symbolic_evidence<T>(
                source.collinearity_truth,
                source.contact == source_edge_contact_class::equal
                    ? symbolic_eligibility_reason::equal_source_feature_lineage
                    : symbolic_eligibility_reason::collinear_source_edge_lineage,
                expected_eligibility);
      } else if (source.contact ==
                     source_edge_contact_class::endpoint_contact ||
                 source.contact == source_edge_contact_class::point_contact) {
        const auto reason =
            point && point->first_endpoint_owner_mask != 0 &&
                    point->second_endpoint_owner_mask != 0
                ? symbolic_eligibility_reason::shared_source_endpoint
                : symbolic_eligibility_reason::exact_formula_zero;
        if (source.has_collinearity_truth &&
            source.collinearity_truth.exact_relation ==
                exact_relation_status::exact_zero)
          reconstructed_evidence = verifier_set_truth_symbolic_evidence<T>(
              source.collinearity_truth, reason, expected_eligibility);
        else if (source.has_coplanarity_truth)
          reconstructed_evidence = verifier_set_truth_symbolic_evidence<T>(
              source.coplanarity_truth, reason, expected_eligibility);
      }
      break;
    }
    case relation_request_family::source_edge_source_facet: {
      const auto *source_request = find_request(
          artifact.source_edge_facet_stage_->request_graph, *source_key);
      if (!source_request ||
          source_request->id.ordinal() >=
              artifact.source_edge_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge/facet source is absent");
      const auto &source = artifact.source_edge_facet_stage_->relations[
          source_request->id.ordinal()];
      const source_edge_facet_event_record<T> *event = nullptr;
      for (std::size_t local_event = 0; local_event < source.events.size();
           ++local_event) {
        if (local_event > std::numeric_limits<std::uint32_t>::max())
          return fail(relation_subcode::count_overflow,
                      "Component 07 symbolic edge/facet local event overflowed");
        std::uint32_t candidate_occurrence = 0;
        if (!canonical_event_occurrence(
                source_request->id, static_cast<std::uint32_t>(local_event),
                candidate_occurrence))
          return fail(relation_subcode::missing_dependency,
                      "Component 07 symbolic edge/facet event order is absent");
        if (candidate_occurrence == occurrence) {
          if (event)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic edge/facet occurrence is duplicated");
          event = &source.events[local_event];
        }
      }
      if (!event)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge/facet occurrence is absent");
      if (event->region.classification ==
              source_facet_point_region_class::original_edge ||
          event->region.classification ==
              source_facet_point_region_class::original_vertex)
        reconstructed_evidence = verifier_set_region_symbolic_evidence(
            event->region, expected_eligibility);
      else
        for (const auto &truth : source.endpoint_support_truth)
          if (!reconstructed_evidence &&
              truth.exact_relation == exact_relation_status::exact_zero)
            reconstructed_evidence = verifier_set_truth_symbolic_evidence<T>(
                truth, symbolic_eligibility_reason::exact_formula_zero,
                expected_eligibility);
      break;
    }
    case relation_request_family::source_facet_source_facet: {
      const auto *source_request =
          find_request(artifact.source_facet_stage_->request_graph, *source_key);
      if (!source_request ||
          source_request->id.ordinal() >=
              artifact.source_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic facet source is absent");
      const auto &source = artifact.source_facet_stage_->relations[
          source_request->id.ordinal()];
      reconstructed_evidence =
          source.has_coplanarity_truth &&
          verifier_set_truth_symbolic_evidence<T>(
              source.coplanarity_truth,
              symbolic_eligibility_reason::coplanar_source_facet_lineage,
              expected_eligibility);
      break;
    }
    case relation_request_family::coplanar_source_facet_overlay: {
      const auto descriptor = std::lower_bound(
          overlay_descriptors.begin(), overlay_descriptors.end(), *source_key,
          [](const verifier_overlay_descriptor &candidate,
             const relation_request_key &key) { return candidate.key < key; });
      if (descriptor == overlay_descriptors.end() ||
          !(descriptor->key == *source_key))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic overlay source lineage is absent");
      const auto &source =
          artifact.coplanar_overlay_stage_->overlays[descriptor->ordinal];
      reconstructed_evidence =
          source.support_relation.has_coplanarity_truth &&
          verifier_set_truth_symbolic_evidence<T>(
              source.support_relation.coplanarity_truth,
              symbolic_eligibility_reason::coincident_source_contract,
              expected_eligibility);
      break;
    }
    default:
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic source family is invalid");
    }
    expected_eligibility.representational_tie_evidence = false;
    expected_eligibility.structural_category_eligible =
        reconstructed_evidence;
    expected_eligibility.tolerance_compatible =
        reconstructed_evidence && construction_tolerance_compatible;
    expected_eligibility.separated_realizations_possible = false;
    expected_eligibility.owner_is_original_source_feature =
        source_key->first.kind !=
            relation_feature_kind::facet_internal_diagonal &&
        source_key->second.kind !=
            relation_feature_kind::facet_internal_diagonal;
    if (!reconstructed_evidence ||
        !verifier_symbolic_eligibility_equal(eligibility,
                                             expected_eligibility))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic eligibility does not reconstruct from predecessor truth");

    bounded_boolean_error symbolic_error;
    if (!verify_symbolic_relation_decision(symbolic, eligibility, decision,
                                           symbolic_error)) {
      error = symbolic_error;
      return false;
    }
    bool has_decision_consumer = false;
    if (eligibility_request)
      for (const auto &candidate : artifact.request_graph_.requests)
        if (candidate.key.family ==
                relation_request_family::symbolic_relation_decision &&
            request_has_dependency(artifact.request_graph_, candidate,
                                   eligibility_request->id)) {
          has_decision_consumer = true;
          break;
        }
    if (!has_decision_consumer)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic decision request is absent");
  }

  std::map<std::pair<relation_request_key, std::uint32_t>,
           const symbolic_relation_decision_record *>
      symbolic_by_source_occurrence;
  for (std::size_t i = 0; i < artifact.symbolic_eligibility_.size(); ++i) {
    const auto &eligibility = artifact.symbolic_eligibility_[i];
    const auto *request = find_request(artifact.request_graph_, eligibility.request);
    if (!request)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic eligibility producer is absent");
    const relation_request_key *source = nullptr;
    for (std::uint64_t offset = 0; offset < request->dependency_count; ++offset) {
      const auto index = request->dependency_begin + offset;
      if (index >= artifact.request_graph_.dependencies.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic dependency range is malformed");
      const auto producer =
          artifact.request_graph_.dependencies[index].producer.ordinal();
      if (producer >= artifact.request_graph_.requests.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic dependency producer is absent");
      const auto &key = artifact.request_graph_.requests[producer].key;
      if (key.family == relation_request_family::source_edge_source_facet) {
        if (source)
          return fail(relation_subcode::duplicate_authoritative_producer,
                      "Component 07 symbolic eligibility has multiple source relations");
        source = &key;
      }
    }
    if (source &&
        !symbolic_by_source_occurrence
             .emplace(std::make_pair(*source,
                                     eligibility.request.occurrence_discriminator),
                      &artifact.symbolic_decisions_[i])
             .second)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 symbolic source occurrence is duplicated");
  }

  std::vector<verifier_crossing_descriptor<T>> expected_crossings;
  for (std::size_t i = 0;
       i < artifact.source_edge_facet_stage_->request_graph.requests.size(); ++i) {
    const auto &request =
        artifact.source_edge_facet_stage_->request_graph.requests[i];
    const auto relation = relation_by_key.find(request.key);
    if (relation == relation_by_key.end() ||
        i >= artifact.source_edge_facet_stage_->relations.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 crossing detailed producer is absent");
    const auto &source = artifact.source_edge_facet_stage_->relations[i];
    for (std::size_t local_event = 0; local_event < source.events.size();
         ++local_event) {
      if (local_event > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::count_overflow,
                    "Component 07 crossing local event overflowed");
      const auto &event = source.events[local_event];
      verifier_crossing_descriptor<T> descriptor;
      if (!canonical_event_occurrence(
              request.id, static_cast<std::uint32_t>(local_event),
              descriptor.occurrence) ||
          !verifier_source_fan_key(request.key, event, descriptor.occurrence,
                                   descriptor.group))
        return fail(relation_subcode::crossing_fan_incomplete,
                    "Component 07 verifier could not reconstruct source-fan lineage");
      descriptor.source_relation = request.key;
      descriptor.relation = relation->second;
      descriptor.event = &event;
      descriptor.local_transition = verifier_local_transition(event);
      descriptor.half_open_owner = request.key.first.operand;
      const auto symbolic = symbolic_by_source_occurrence.find(
          std::make_pair(request.key, descriptor.occurrence));
      if (symbolic != symbolic_by_source_occurrence.end()) {
        descriptor.symbolic_crossing =
            symbolic->second->symbolic_crossing_contribution;
        descriptor.half_open_owner = symbolic->second->half_open_owner;
      }
      expected_crossings.push_back(descriptor);
    }
  }
  std::sort(expected_crossings.begin(), expected_crossings.end(),
            [](const verifier_crossing_descriptor<T> &a,
               const verifier_crossing_descriptor<T> &b) {
              return std::tie(a.group, a.source_relation, a.occurrence) <
                     std::tie(b.group, b.source_relation, b.occurrence);
            });
  if (artifact.crossings_.size() != expected_crossings.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 crossing contribution table is incomplete");

  std::vector<std::int64_t> crossing_sums(artifact.relations_.size(), 0);
  std::size_t published = 0;
  for (std::size_t begin = 0, group = 0; begin < expected_crossings.size(); ++group) {
    std::size_t end = begin + 1;
    while (end < expected_crossings.size() &&
           expected_crossings[end].group == expected_crossings[begin].group)
      ++end;
    const auto count = end - begin;
    bool has_positive = false;
    bool has_negative = false;
    bool complete = true;
    bool boundary_crossing_group = false;
    bool tangent_group = false;
    for (std::size_t i = begin; i < end; ++i) {
      const auto &event = *expected_crossings[i].event;
      if (expected_crossings[i].group.boundary_group) {
        boundary_crossing_group =
            boundary_crossing_group ||
            event.kind == source_edge_facet_event_kind::boundary_crossing;
        tangent_group = tangent_group ||
                        event.kind ==
                            source_edge_facet_event_kind::tangent_contact;
        if (event.kind == source_edge_facet_event_kind::boundary_crossing) {
          complete = complete &&
                     (expected_crossings[i].local_transition == -1 ||
                      expected_crossings[i].local_transition == 1);
          has_positive = has_positive ||
                         expected_crossings[i].local_transition > 0;
          has_negative = has_negative ||
                         expected_crossings[i].local_transition < 0;
        } else {
          complete = complete &&
                     event.kind ==
                         source_edge_facet_event_kind::tangent_contact &&
                     expected_crossings[i].local_transition == 0 &&
                     event.numeric_crossing == 0;
        }
      } else {
        complete = complete &&
                   (event.kind !=
                        source_edge_facet_event_kind::proper_face_crossing ||
                    (expected_crossings[i].local_transition ==
                         event.numeric_crossing &&
                     event.numeric_crossing != 0));
      }
      complete = complete &&
                 expected_crossings[i].half_open_owner ==
                     expected_crossings[begin].half_open_owner;
    }
    if (expected_crossings[begin].group.boundary_group) {
      std::vector<relation_feature_key> expected_facets;
      std::vector<relation_feature_key> actual_facets;
      if (!artifact.candidates_ ||
          !verifier_expected_source_fan_facets(
              *artifact.candidates_, expected_crossings[begin].group,
              expected_facets))
        complete = false;
      for (std::size_t i = begin; i < end; ++i)
        actual_facets.push_back(expected_crossings[i].source_relation.second);
      std::sort(actual_facets.begin(), actual_facets.end());
      actual_facets.erase(
          std::unique(actual_facets.begin(), actual_facets.end()),
          actual_facets.end());
      complete = complete && count >= 2 &&
                 actual_facets == expected_facets &&
                 boundary_crossing_group != tangent_group;
    }
    if (!complete)
      return fail(relation_subcode::crossing_fan_incomplete,
                  "Component 07 independently reconstructed source fan is incomplete");

    std::int32_t group_total = 0;
    if (expected_crossings[begin].group.boundary_group) {
      if (boundary_crossing_group && has_positive != has_negative)
        group_total = has_positive ? 1 : -1;
    } else {
      group_total = expected_crossings[begin].event->numeric_crossing;
    }
    const bool has_numeric_owner = group_total != 0;
    for (std::size_t i = begin; i < end; ++i, ++published) {
      const auto &expected = expected_crossings[i];
      const auto &record = artifact.crossings_[published];
      const auto ordinal = static_cast<std::uint32_t>(i - begin);
      const auto expected_numeric =
          has_numeric_owner && i == begin ? group_total : 0;
      const auto expected_symbolic =
          i == begin ? expected.symbolic_crossing : 0;
      if (record.relation != expected.relation ||
          record.numeric_crossing != expected_numeric ||
          record.symbolic_crossing != expected_symbolic ||
          record.half_open_owner !=
              expected_crossings[begin].half_open_owner ||
          record.occurrence != expected.occurrence ||
          record.source_fan_group != group ||
          record.source_fan_group_size != count ||
          record.source_fan_group_ordinal != ordinal ||
          record.local_transition != expected.local_transition ||
          record.numeric_owner != (has_numeric_owner && i == begin) ||
          !record.source_fan_resolved || !record.locally_conservative ||
          record.reserved16 != 0 || record.reserved32 != 0)
        return fail(relation_subcode::crossing_conservation_failed,
                    "Component 07 crossing/source-fan record does not reconstruct");
      crossing_sums[record.relation.ordinal()] += record.numeric_crossing;
    }
    begin = end;
  }
  for (std::size_t i = 0; i < artifact.relations_.size(); ++i)
    if (crossing_sums[i] !=
        artifact.relations_[i].numeric_crossing_multiplicity)
      return fail(relation_subcode::crossing_conservation_failed,
                  "Component 07 local numeric crossing conservation failed");

  if (!std::is_sorted(
          artifact.event_seeds_.begin(), artifact.event_seeds_.end(),
          [](const relation_event_seed_record &a,
             const relation_event_seed_record &b) { return a.key < b.key; }))
    return fail(relation_subcode::canonical_order_mismatch,
                "Component 07 event seeds are not ordered");
  for (std::size_t i = 0; i < artifact.event_seeds_.size(); ++i) {
    const auto &record = artifact.event_seeds_[i];
    if (record.id.ordinal() != i || !valid_relation_event_seed_key(record.key) ||
        record.source_relation.ordinal() >= artifact.relations_.size() ||
        record.construction.ordinal() >= artifact.constructions_.size() ||
        record.incidence_begin > artifact.event_seed_incidence_.size() ||
        record.incidence_count < 2 ||
        record.incidence_count >
            artifact.event_seed_incidence_.size() - record.incidence_begin ||
        record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event seed is malformed");
    if (i != 0 && !(artifact.event_seeds_[i - 1].key < record.key))
      return fail(relation_subcode::canonical_order_mismatch,
                  "Component 07 event seeds are not strictly ordered");
    bool has_first = false;
    bool has_second = false;
    for (std::uint64_t offset = 0; offset < record.incidence_count; ++offset) {
      const auto index = record.incidence_begin + offset;
      const auto &feature = artifact.event_seed_incidence_[index];
      if (!valid_relation_feature_key(feature) ||
          (offset != 0 &&
           !(artifact.event_seed_incidence_[index - 1] < feature)))
        return fail(relation_subcode::canonical_order_mismatch,
                    "Component 07 event-seed incidence is malformed");
      has_first = has_first || feature == record.key.first;
      has_second = has_second || feature == record.key.second;
    }
    if (!has_first || !has_second)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event seed omits authoritative source incidence");
    const auto construction_request = artifact.request_graph_.requests[
        artifact.constructions_[record.construction.ordinal()].producer.ordinal()];
    bool found_seed_request = false;
    for (const auto &request : artifact.request_graph_.requests)
      if (request.key.family == relation_request_family::event_seed &&
          request.key.first == record.key.first &&
          request.key.second == record.key.second &&
          request.key.occurrence_discriminator == record.key.occurrence &&
          request_has_dependency(artifact.request_graph_, request,
                                 construction_request.id)) {
        found_seed_request = true;
        break;
      }
    if (!found_seed_request)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 event seed request is absent from the graph");
  }

  if (!std::is_sorted(
          artifact.candidate_dispositions_.begin(),
          artifact.candidate_dispositions_.end(),
          [](const relation_candidate_disposition_record &a,
             const relation_candidate_disposition_record &b) {
            return a.candidate < b.candidate;
          }))
    return fail(relation_subcode::canonical_order_mismatch,
                "Component 07 candidate dispositions are not ordered");
  const auto expected_candidates = artifact.candidates_->candidates().size();
  if (artifact.candidate_dispositions_.size() != expected_candidates)
    return fail(relation_subcode::candidate_disposition_missing,
                "Component 07 does not contain exactly one disposition per candidate");
  for (std::size_t i = 0; i < artifact.candidate_dispositions_.size(); ++i) {
    const auto &record = artifact.candidate_dispositions_[i];
    if (record.id.ordinal() != i || record.candidate.ordinal() != i ||
        record.bookkeeping_request.ordinal() >=
            artifact.request_graph_.requests.size() ||
        artifact.request_graph_.requests[record.bookkeeping_request.ordinal()]
                .key.family != relation_request_family::candidate_disposition ||
        !request_has_witness(
            artifact.request_graph_,
            artifact.request_graph_.requests[record.bookkeeping_request.ordinal()],
            record.candidate) ||
        record.reserved != 0)
      return fail(relation_subcode::candidate_disposition_contradiction,
                  "Component 07 candidate disposition is malformed");
    if (record.disposition ==
        candidate_relation_disposition_kind::mapped_to_public_relation) {
      if (record.public_relation.ordinal() >= artifact.relations_.size() ||
          !public_contact(
              artifact.relations_[record.public_relation.ordinal()].status))
        return fail(relation_subcode::candidate_disposition_contradiction,
                    "Component 07 mapped candidate lacks a public contact relation");
    }
  }

  if (artifact.statistics_.candidate_count != expected_candidates ||
      artifact.statistics_.unique_request_count !=
          artifact.request_graph_.requests.size() ||
      artifact.statistics_.dependency_count !=
          artifact.request_graph_.dependencies.size() ||
      artifact.statistics_.reverse_consumer_count !=
          artifact.request_graph_.reverse_consumers.size() ||
      artifact.statistics_.candidate_witness_count !=
          artifact.request_graph_.candidate_witnesses.size() ||
      artifact.statistics_.imported_geometry_count !=
          artifact.imported_geometry_.size() ||
      artifact.statistics_.bounded_primitive_count !=
          artifact.bounded_primitives_.size() ||
      artifact.statistics_.exact_relation_count !=
          artifact.exact_relations_.size() ||
      artifact.statistics_.truth_lineage_count !=
          artifact.truth_lineage_.size() ||
      artifact.statistics_.interval_evidence_count !=
          artifact.interval_evidence_.size() ||
      artifact.statistics_.source_facet_region_count !=
          artifact.source_facet_regions_.size() ||
      artifact.statistics_.public_relation_count +
              artifact.statistics_.bookkeeping_relation_count !=
          artifact.relations_.size() ||
      artifact.statistics_.construction_count !=
          artifact.constructions_.size() ||
      artifact.statistics_.coplanar_event_node_count !=
          artifact.coplanar_event_nodes_.size() ||
      artifact.statistics_.coplanar_oriented_arc_count !=
          artifact.coplanar_oriented_arcs_.size() ||
      artifact.statistics_.coplanar_overlap_component_count !=
          artifact.coplanar_overlap_components_.size() ||
      artifact.statistics_.symbolic_eligibility_count !=
          artifact.symbolic_eligibility_.size() ||
      artifact.statistics_.symbolic_decision_count !=
          artifact.symbolic_decisions_.size() ||
      artifact.statistics_.crossing_record_count != artifact.crossings_.size() ||
      artifact.statistics_.event_seed_count != artifact.event_seeds_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 statistics do not reconstruct from records");

  const auto &evidence = artifact.verification_evidence_;
  if (evidence.id.ordinal() != 0 ||
      evidence.verifier_version != contract_versions::relation_verifier ||
      !evidence.graph_reconstructed || !evidence.owner_exclusion_checked ||
      !evidence.selection_boundary_checked ||
      !evidence.candidate_dispositions_complete || evidence.reserved != 0 ||
      evidence.semantic_digest != artifact.graph_digest_)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 verification evidence is incomplete");

  auto owner_changed = artifact;
  owner_changed.owner_ = context_owner_token::create();
  owner_changed.request_graph_.owner = owner_changed.owner_;
  if (encode_signed_feature_relations(owner_changed) != artifact.canonical_bytes_)
    return fail(relation_subcode::owner_in_semantics,
                "runtime owner token contaminated Component 07 semantic bytes");
  return verify_relation_codec(artifact, error);
}

template bool verify_signed_feature_relations<float, std::uint32_t>(
    const signed_feature_relations<float, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_signed_feature_relations<float, std::uint64_t>(
    const signed_feature_relations<float, std::uint64_t> &,
    bounded_boolean_error &);
template bool verify_signed_feature_relations<double, std::uint32_t>(
    const signed_feature_relations<double, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_signed_feature_relations<double, std::uint64_t>(
    const signed_feature_relations<double, std::uint64_t> &,
    bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
