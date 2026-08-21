#pragma once

#include "ContextVerifier.h"
#include "RelationSemanticProjection.h"
#include "SignedFeatureRelations.h"

#include <algorithm>

namespace ygor::mesh_boolean::bounded {

inline bool same_relation_predecessor_commitment(
    const relation_predecessor_commitment_record &a,
    const relation_predecessor_commitment_record &b) noexcept {
  return a.component == b.component && a.schema_version == b.schema_version &&
         a.provider_version == b.provider_version &&
         a.policy_version == b.policy_version &&
         a.codec_version == b.codec_version &&
         a.verifier_version == b.verifier_version &&
         a.independently_verified == b.independently_verified &&
         a.reserved8 == b.reserved8 &&
         a.artifact_digest_a == b.artifact_digest_a &&
         a.artifact_digest_b == b.artifact_digest_b &&
         a.semantic_digest_a == b.semantic_digest_a &&
         a.semantic_digest_b == b.semantic_digest_b &&
         a.exact_digest_a == b.exact_digest_a &&
         a.exact_digest_b == b.exact_digest_b &&
         a.policy_digest == b.policy_digest &&
         a.commitment_schema == b.commitment_schema &&
         a.reserved16 == b.reserved16 && a.reserved32 == b.reserved32;
}

template <class T, class I>
bool build_relation_predecessor_commitments(
    const bounded_boolean_digest &context_digest,
    const bounded_boolean_digest &precision_semantic_digest,
    const bounded_boolean_digest &symbolic_policy_digest,
    const canonical_candidate_stream<T, I> &candidates,
    std::array<relation_predecessor_commitment_record, 6> &commitments) {
  const auto &manifolds = candidates.manifolds();
  if (!manifolds || !manifolds->a() || !manifolds->b() ||
      !manifolds->a()->validated() || !manifolds->b()->validated() ||
      !manifolds->a()->source_triangles() ||
      !manifolds->b()->source_triangles())
    return false;
  commitments = {};
  for (std::size_t i = 0; i < commitments.size(); ++i)
    commitments[i].component =
        static_cast<relation_predecessor_component>(i + 1);

  const auto &validated_a = manifolds->a()->validated();
  const auto &validated_b = manifolds->b()->validated();
  commitments[0].schema_version = contract_versions::context;
  commitments[0].provider_version = contract_versions::context;
  commitments[0].policy_version = contract_versions::truth_table;
  commitments[0].codec_version = contract_versions::canonical_bytes;
  commitments[0].verifier_version = contract_versions::verifier;
  commitments[0].independently_verified = true;
  commitments[0].artifact_digest_a = context_digest;
  commitments[0].semantic_digest_a = validated_a->source_digest();
  commitments[0].semantic_digest_b = validated_b->source_digest();
  commitments[0].exact_digest_a = materialize_truth_table().digest;
  commitments[0].policy_digest = symbolic_policy_digest;

  const auto validated_commitment = [](const auto &validated) {
    canonical_writer writer;
    writer.u32(0x56375259U);
    writer.u16(validated->schema_version());
    for (const auto byte : validated->source_digest().bytes) writer.u8(byte);
    for (const auto byte : validated->context_digest().bytes) writer.u8(byte);
    writer.u8(static_cast<std::uint8_t>(validated->certificate()));
    return sha256::digest(writer.bytes());
  };
  commitments[1].schema_version = contract_versions::validated_operand;
  commitments[1].provider_version = contract_versions::input_validation_provider;
  commitments[1].codec_version = contract_versions::validated_operand_codec;
  commitments[1].verifier_version = contract_versions::validated_operand_verifier;
  commitments[1].independently_verified = true;
  commitments[1].artifact_digest_a = validated_commitment(validated_a);
  commitments[1].artifact_digest_b = validated_commitment(validated_b);
  commitments[1].semantic_digest_a = validated_a->source_digest();
  commitments[1].semantic_digest_b = validated_b->source_digest();

  commitments[2].schema_version = precision_context_schema_v1;
  commitments[2].provider_version = precision_context_provider_v1;
  commitments[2].policy_version = precision_arithmetic_profile_v1;
  commitments[2].codec_version = contract_versions::precision_codec;
  commitments[2].verifier_version = contract_versions::precision_verifier;
  commitments[2].independently_verified = true;
  commitments[2].artifact_digest_a = precision_semantic_digest;
  commitments[2].semantic_digest_a = precision_semantic_digest;
  commitments[2].exact_digest_a = precision_semantic_digest;
  canonical_writer profile;
  profile.u32(0x46375259U);
  profile.u16(precision_scalar_profile_v1);
  profile.u16(precision_arithmetic_profile_v1);
  profile.u8(sizeof(T));
  profile.u8(sizeof(I));
  profile.boolean(runtime_floating_profile_qualified<T>());
  commitments[2].policy_digest = sha256::digest(profile.bytes());

  const auto &triangles_a = manifolds->a()->source_triangles();
  const auto &triangles_b = manifolds->b()->source_triangles();
  const auto triangle_commitment = [](const auto &triangles) {
    canonical_writer writer;
    writer.u32(0x54375259U);
    writer.u16(triangles->schema_version());
    writer.u16(triangles->provider_version());
    writer.u16(triangles->policy_version());
    for (const auto byte : triangles->source_semantic_digest().bytes)
      writer.u8(byte);
    for (const auto byte : triangles->exact_triangulation_digest().bytes)
      writer.u8(byte);
    return sha256::digest(writer.bytes());
  };
  commitments[3].schema_version = triangles_a->schema_version();
  commitments[3].provider_version = triangles_a->provider_version();
  commitments[3].policy_version = triangles_a->policy_version();
  commitments[3].codec_version = triangles_a->codec_version();
  commitments[3].verifier_version = triangles_a->verifier_version();
  commitments[3].independently_verified = true;
  commitments[3].artifact_digest_a = triangle_commitment(triangles_a);
  commitments[3].artifact_digest_b = triangle_commitment(triangles_b);
  commitments[3].semantic_digest_a = triangles_a->source_semantic_digest();
  commitments[3].semantic_digest_b = triangles_b->source_semantic_digest();
  commitments[3].exact_digest_a = triangles_a->exact_triangulation_digest();
  commitments[3].exact_digest_b = triangles_b->exact_triangulation_digest();

  const auto manifold_commitment = [](const auto &manifold) {
    canonical_writer writer;
    writer.u32(0x4d375259U);
    writer.u16(manifold->schema_version());
    writer.u16(manifold->provider_version());
    writer.u16(manifold->policy_version());
    for (const auto byte : manifold->source_semantic_digest().bytes)
      writer.u8(byte);
    for (const auto byte : manifold->exact_topology_digest().bytes)
      writer.u8(byte);
    for (const auto byte : manifold->geometry_attachment_digest().bytes)
      writer.u8(byte);
    for (const auto byte : manifold->precision_attachment_digest().bytes)
      writer.u8(byte);
    return sha256::digest(writer.bytes());
  };
  commitments[4].schema_version = manifolds->a()->schema_version();
  commitments[4].provider_version = manifolds->a()->provider_version();
  commitments[4].policy_version = manifolds->a()->policy_version();
  commitments[4].codec_version = manifolds->a()->codec_version();
  commitments[4].verifier_version = manifolds->a()->verifier_version();
  commitments[4].independently_verified =
      manifolds->a()->verification() ==
          canonical_halfedge_verification_disposition::independently_verified &&
      manifolds->b()->verification() ==
          canonical_halfedge_verification_disposition::independently_verified;
  commitments[4].artifact_digest_a = manifold_commitment(manifolds->a());
  commitments[4].artifact_digest_b = manifold_commitment(manifolds->b());
  commitments[4].semantic_digest_a = manifolds->a()->source_semantic_digest();
  commitments[4].semantic_digest_b = manifolds->b()->source_semantic_digest();
  commitments[4].exact_digest_a = manifolds->a()->exact_topology_digest();
  commitments[4].exact_digest_b = manifolds->b()->exact_topology_digest();

  commitments[5].schema_version = candidates.schema_version();
  commitments[5].provider_version = candidates.provider_version();
  commitments[5].policy_version = candidates.domain_policy_version();
  commitments[5].codec_version = candidates.codec_version();
  commitments[5].verifier_version = candidates.verifier_version();
  commitments[5].independently_verified =
      candidates.verification() ==
      broad_phase_verification_disposition::independently_verified;
  canonical_writer candidate_commitment;
  candidate_commitment.u32(0x43375259U);
  candidate_commitment.u16(candidates.schema_version());
  candidate_commitment.u16(candidates.provider_version());
  candidate_commitment.u16(candidates.domain_policy_version());
  for (const auto byte : candidates.candidate_digest().bytes)
    candidate_commitment.u8(byte);
  commitments[5].artifact_digest_a =
      sha256::digest(candidate_commitment.bytes());
  commitments[5].semantic_digest_a = candidates.candidate_digest();
  canonical_writer primitive_commitment;
  primitive_commitment.u32(0x42375259U);
  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto &table = candidates.primitive_table(operand);
    for (const auto byte : table.source_semantic_digest.bytes)
      primitive_commitment.u8(byte);
    for (const auto byte : table.exact_topology_digest.bytes)
      primitive_commitment.u8(byte);
    for (const auto byte : table.geometry_attachment_digest.bytes)
      primitive_commitment.u8(byte);
  }
  commitments[5].exact_digest_a = sha256::digest(primitive_commitment.bytes());
  canonical_writer policy;
  policy.u32(0x50375259U);
  policy.u16(candidates.provider_version());
  policy.u16(candidates.domain_policy_version());
  commitments[5].policy_digest = sha256::digest(policy.bytes());
  return true;
}

template <class T, class I>
bool validate_relation_predecessors_before_work(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    const canonical_candidate_stream<T, I> &candidates,
    std::array<relation_predecessor_commitment_record, 6> &commitments,
    bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = relation_error(subcode,
                           bounded_boolean_error_category::internal_invariant_error,
                           summary, relation_checkpoint::predecessor_validation);
    return false;
  };
  if (!verify_context(context) || !context.owner.same_owner(precision.owner()) ||
      !context.owner.same_owner(candidates.owner()))
    return fail(relation_subcode::wrong_owner,
                "Component 07 predecessor owner or context verification failed");
  if (context.truth.digest != materialize_truth_table().digest ||
      context.symbolic.digest != materialize_symbolic_policy().digest ||
      precision.schema_version() != precision_context_schema_v1 ||
      precision.provider_version() != precision_context_provider_v1 ||
      precision.scalar_profile_version() != precision_scalar_profile_v1 ||
      precision.arithmetic_profile_version() != precision_arithmetic_profile_v1 ||
      !precision.ordinary_success_eligible() ||
      precision.boolean_context_digest() != context.context_digest ||
      candidates.precision_digest() != precision.digest())
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 context, profile, truth, or precision commitment failed");
  if (candidates.schema_version() != contract_versions::broad_phase_artifact_schema ||
      candidates.provider_version() != contract_versions::broad_phase_provider ||
      candidates.domain_policy_version() !=
          contract_versions::broad_phase_candidate_domain_policy ||
      candidates.codec_version() != contract_versions::broad_phase_codec ||
      candidates.verifier_version() != contract_versions::broad_phase_verifier ||
      candidates.verification() !=
          broad_phase_verification_disposition::independently_verified)
    return fail(relation_subcode::predecessor_not_verified,
                "Component 07 candidate predecessor version or verification failed");
  const auto &manifolds = candidates.manifolds();
  if (!manifolds || manifolds->schema_version() !=
                        contract_versions::canonical_source_manifolds_schema ||
      !manifolds->owner().same_owner(context.owner) ||
      candidates.predecessor_digest() != manifolds->digest())
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 candidate-to-manifold commitment failed");
  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto &manifold = operand == operand_id::a ? manifolds->a() : manifolds->b();
    if (!manifold || !manifold->owner().same_owner(context.owner) ||
        manifold->schema_version() != contract_versions::canonical_halfedge_operand_schema ||
        manifold->provider_version() != contract_versions::canonical_halfedge_provider ||
        manifold->policy_version() != contract_versions::canonical_halfedge_policy ||
        manifold->codec_version() != contract_versions::canonical_halfedge_codec ||
        manifold->verifier_version() != contract_versions::canonical_halfedge_verifier ||
        manifold->verification() !=
            canonical_halfedge_verification_disposition::independently_verified ||
        !manifold->validated() || !manifold->source_triangles())
      return fail(relation_subcode::predecessor_not_verified,
                  "Component 07 canonical-manifold commitment failed");
    const auto &validated = manifold->validated();
    const auto &triangles = manifold->source_triangles();
    const auto &table = candidates.primitive_table(operand);
    if (!validated->owner().same_owner(context.owner) ||
        validated->schema_version() != contract_versions::validated_operand ||
        validated->context_digest() != context.context_digest ||
        validated->precision_digest() != precision.digest() ||
        !triangles->owner().same_owner(context.owner) ||
        triangles->schema_version() != contract_versions::source_triangle_complex ||
        triangles->provider_version() != contract_versions::source_triangulation_provider ||
        triangles->policy_version() != contract_versions::source_triangulation_policy ||
        triangles->codec_version() != contract_versions::source_triangle_complex_codec ||
        triangles->verifier_version() != contract_versions::source_triangle_complex_verifier ||
        triangles->predecessor_digest() != validated->digest() ||
        triangles->precision_digest() != precision.digest())
      return fail(relation_subcode::predecessor_mismatch,
                  "Component 07 Component 02-04 digest handshake failed");
    if (manifold->validated_operand_digest() != validated->digest() ||
        manifold->source_triangle_complex_digest() != triangles->digest() ||
        manifold->precision_digest() != precision.digest())
      return fail(relation_subcode::predecessor_mismatch,
                  "Component 07 Component 05 predecessor digest handshake failed");
    for (const auto &group : manifold->facet_groups()) {
      const auto found = std::find_if(
          triangles->facets().begin(), triangles->facets().end(),
          [&](const auto &facet) { return facet.facet == group.source_facet; });
      if (found == triangles->facets().end() ||
          group.source_semantic_digest != found->semantic_digest ||
          group.exact_triangulation_digest != found->exact_digest)
        return fail(relation_subcode::predecessor_mismatch,
                    "Component 07 Component 04-05 facet digests disagree");
    }
    if (
        table.predecessor_digest != manifold->digest() ||
        table.source_semantic_digest != manifold->source_semantic_digest() ||
        table.exact_topology_digest != manifold->exact_topology_digest() ||
        table.geometry_attachment_digest != manifold->geometry_attachment_digest() ||
        table.precision_attachment_digest != manifold->precision_attachment_digest())
      return fail(relation_subcode::predecessor_mismatch,
                  "Component 07 Component 05-06 digest handshake failed");
  }
  if (!build_relation_predecessor_commitments(
          context.context_digest, relation_precision_semantic_digest(precision),
          context.symbolic.digest, candidates, commitments))
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 predecessor commitment construction failed");
  return true;
}

} // namespace ygor::mesh_boolean::bounded
