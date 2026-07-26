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
    key.singleton_occurrence = event.occurrence;
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
      const source_facet_coplanar_overlay_record<T> *source = nullptr;
      for (const auto &overlay : artifact.coplanar_overlay_stage_->overlays)
        if (overlay.facets[0].feature == producer.key.first &&
            overlay.facets[1].feature == producer.key.second) {
          if (source)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 overlay producer is ambiguous");
          source = &overlay;
        }
      if (!source)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 overlay relation does not map to its detailed producer");
      expected = overlay_status(source->classification);
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

  for (const auto &truth : artifact.truth_records_)
    if (truth.reserved != 0 ||
        truth.bounded_sign == bounded_sign_status::invalid ||
        truth.exact_relation == exact_relation_status::invalid ||
        truth.disposition == predicate_disposition::fail_invalid)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 truth record is invalid");

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
  }

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
      for (const auto &candidate : source.events)
        if (candidate.occurrence == occurrence) {
          if (event)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic edge/facet occurrence is duplicated");
          event = &candidate;
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
      const source_facet_coplanar_overlay_record<T> *source = nullptr;
      for (const auto &candidate : artifact.coplanar_overlay_stage_->overlays)
        if (candidate.facets[0].feature == source_key->first &&
            candidate.facets[1].feature == source_key->second) {
          if (source)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic overlay source is duplicated");
          source = &candidate;
        }
      if (!source)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic overlay source is absent");
      reconstructed_evidence =
          source->support_relation.has_coplanarity_truth &&
          verifier_set_truth_symbolic_evidence<T>(
              source->support_relation.coplanarity_truth,
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
    for (const auto &event : source.events) {
      verifier_crossing_descriptor<T> descriptor;
      if (!verifier_source_fan_key(request.key, event, descriptor.group))
        return fail(relation_subcode::crossing_fan_incomplete,
                    "Component 07 verifier could not reconstruct source-fan lineage");
      descriptor.source_relation = request.key;
      descriptor.relation = relation->second;
      descriptor.event = &event;
      descriptor.local_transition = verifier_local_transition(event);
      descriptor.half_open_owner = request.key.first.operand;
      const auto symbolic = symbolic_by_source_occurrence.find(
          std::make_pair(request.key, event.occurrence));
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
              return std::tie(a.group, a.source_relation,
                              a.event->occurrence) <
                     std::tie(b.group, b.source_relation,
                              b.event->occurrence);
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
          record.occurrence != expected.event->occurrence ||
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
      artifact.statistics_.public_relation_count +
              artifact.statistics_.bookkeeping_relation_count !=
          artifact.relations_.size() ||
      artifact.statistics_.construction_count !=
          artifact.constructions_.size() ||
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
