#include "StrictFloatingBuild.h"
#include "RelationVerifier.h"
#include "CoplanarRelationOverlay.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
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
      std::int32_t crossing = 0;
      for (const auto &event : source.events)
        crossing += event.numeric_crossing;
      if (record.numeric_crossing_multiplicity != crossing ||
          record.family != feature_relation_family::source_edge_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge/facet crossing summary mismatch");
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
    bounded_boolean_error symbolic_error;
    if (!verify_symbolic_relation_decision(symbolic, eligibility, decision,
                                           symbolic_error)) {
      error = symbolic_error;
      return false;
    }
    const auto *eligibility_request =
        find_request(artifact.request_graph_, eligibility.request);
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

  std::vector<std::int64_t> crossing_sums(artifact.relations_.size(), 0);
  for (const auto &record : artifact.crossings_) {
    if (record.relation.ordinal() >= artifact.relations_.size() ||
        record.numeric_crossing < -1 || record.numeric_crossing > 1 ||
        record.symbolic_crossing < -1 || record.symbolic_crossing > 1 ||
        !record.source_fan_resolved || !record.locally_conservative ||
        record.reserved16 != 0 || record.reserved32 != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 crossing/source-fan record is malformed");
    crossing_sums[record.relation.ordinal()] += record.numeric_crossing;
  }
  for (std::size_t i = 0; i < artifact.relations_.size(); ++i)
    if (crossing_sums[i] !=
        artifact.relations_[i].numeric_crossing_multiplicity)
      return fail(relation_subcode::verifier_rejection,
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
