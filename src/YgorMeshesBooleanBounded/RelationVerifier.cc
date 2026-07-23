#include "StrictFloatingBuild.h"
#include "RelationVerifier.h"

#include <algorithm>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error verifier_error(relation_subcode subcode,
                                     const char *summary) {
  return relation_error(subcode,
                        bounded_boolean_error_category::internal_invariant_error,
                        summary,
                        relation_checkpoint::independent_verification);
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
  if (artifact.schema_version_ != contract_versions::relation_artifact_schema || artifact.provider_version_ != contract_versions::relation_provider ||
      artifact.graph_policy_version_ != contract_versions::relation_graph_policy ||
      artifact.truth_policy_version_ != contract_versions::relation_truth_policy || artifact.codec_version_ != contract_versions::relation_codec ||
      artifact.verifier_version_ != contract_versions::relation_verifier ||
      artifact.provider_ !=
          relation_provider_kind::canonical_source_feature_relation_graph_v1 ||
      artifact.verification_ !=
          relation_verification_disposition::independently_verified ||
      !artifact.owner_.anchor ||
      !artifact.request_graph_.owner.same_owner(artifact.owner_))
    return fail(relation_subcode::unsupported_version,
                "Component 07 version, provider, verification, or owner mismatch");

  bounded_boolean_error graph_error;
  if (!verify_relation_request_graph(artifact.request_graph_, graph_error)) {
    error = graph_error;
    return false;
  }
  if (artifact.graph_digest_ != artifact.request_graph_.semantic_digest)
    return fail(relation_subcode::digest_mismatch,
                "Component 07 graph digest mismatch");
  if (artifact.candidates_) {
    if (!artifact.candidates_->owner().same_owner(artifact.owner_) ||
        artifact.candidates_->candidate_digest() != artifact.candidate_digest_ ||
        artifact.candidates_->verification() !=
            broad_phase_verification_disposition::independently_verified)
      return fail(relation_subcode::predecessor_mismatch,
                  "Component 07 candidate predecessor handshake failed");
  }

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
    if (record.scope == relation_record_scope::public_source_feature &&
        producer.key.scope != relation_record_scope::public_source_feature)
      return fail(relation_subcode::verifier_rejection,
                  "bookkeeping relation escaped into the public relation domain");
  }
  for (const auto &truth : artifact.truth_records_) {
    if (truth.reserved != 0 ||
        truth.bounded_sign == bounded_sign_status::invalid ||
        truth.exact_relation == exact_relation_status::invalid ||
        truth.disposition == predicate_disposition::fail_invalid)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 truth record is invalid");
  }
  for (std::size_t i = 0; i < artifact.constructions_.size(); ++i) {
    const auto &record = artifact.constructions_[i];
    if (record.id.ordinal() != i ||
        record.producer.ordinal() >= artifact.request_graph_.requests.size() ||
        record.component_count == 0 || record.component_count > 4 ||
        record.residual_truth_begin > artifact.truth_records_.size() ||
        record.residual_truth_count >
            artifact.truth_records_.size() - record.residual_truth_begin ||
        !record.finite || !record.tolerance_compatible || record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 construction record is malformed");
  }
  for (std::size_t i = 0; i < artifact.symbolic_decisions_.size(); ++i) {
    const auto &record = artifact.symbolic_decisions_[i];
    if (record.id.ordinal() != i || !record.nominal_geometry_unchanged ||
        record.reserved != 0 || !valid_relation_request_key(record.request))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic record violates its downstream boundary");
  }
  for (std::size_t i = 0; i < artifact.event_seeds_.size(); ++i) {
    const auto &record = artifact.event_seeds_[i];
    if (record.id.ordinal() != i || !valid_relation_event_seed_key(record.key) ||
        record.source_relation.ordinal() >= artifact.relations_.size() ||
        record.construction.ordinal() >= artifact.constructions_.size() ||
        record.incidence_begin > artifact.event_seed_incidence_.size() ||
        record.incidence_count >
            artifact.event_seed_incidence_.size() - record.incidence_begin ||
        record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 event seed is malformed");
    if (i != 0 && !(artifact.event_seeds_[i - 1].key < record.key))
      return fail(relation_subcode::canonical_order_mismatch,
                  "Component 07 event seeds are not strictly ordered");
    for (std::uint64_t offset = 0; offset < record.incidence_count; ++offset) {
      const auto index = record.incidence_begin + offset;
      const auto &feature = artifact.event_seed_incidence_[index];
      if (!valid_relation_feature_key(feature) ||
          (offset != 0 &&
           !(artifact.event_seed_incidence_[index - 1] < feature)))
        return fail(relation_subcode::canonical_order_mismatch,
                    "Component 07 event-seed incidence is malformed");
    }
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
  const auto expected_candidates =
      artifact.candidates_ ? artifact.candidates_->candidates().size() : 0;
  if (artifact.candidate_dispositions_.size() != expected_candidates)
    return fail(relation_subcode::candidate_disposition_missing,
                "Component 07 does not contain exactly one disposition per candidate");
  for (std::size_t i = 0; i < artifact.candidate_dispositions_.size(); ++i) {
    const auto &record = artifact.candidate_dispositions_[i];
    if (record.id.ordinal() != i || record.candidate.ordinal() != i ||
        record.reserved != 0)
      return fail(relation_subcode::candidate_disposition_contradiction,
                  "Component 07 candidate disposition is malformed");
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
      artifact.statistics_.symbolic_decision_count !=
          artifact.symbolic_decisions_.size() ||
      artifact.statistics_.event_seed_count != artifact.event_seeds_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 statistics do not reconstruct from records");

  const auto &evidence = artifact.verification_evidence_;
  if (evidence.id.ordinal() != 0 || evidence.verifier_version != contract_versions::relation_verifier ||
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
