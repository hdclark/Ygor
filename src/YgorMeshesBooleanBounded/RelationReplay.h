#pragma once

#include "SignedFeatureRelations.h"

namespace ygor::mesh_boolean::bounded {

std::vector<std::uint8_t> encode_relation_diagnostic_semantics(
    const std::vector<relation_diagnostic_record> &records);

std::vector<std::uint8_t> encode_relation_replay_checkpoint_semantics(
    const std::vector<relation_replay_checkpoint_record> &records);

std::vector<std::uint8_t> encode_relation_replay_evidence_semantics(
    const relation_replay_evidence &evidence);

bounded_boolean_digest relation_failure_replay_digest(
    const bounded_boolean_digest &invocation_replay_digest,
    const bounded_boolean_error &error);

template <class T, class I> struct relation_replay_bundle_builder final {
  static bool build(signed_feature_relations<T, I> &artifact,
                    const relation_capabilities &capabilities,
                    bounded_boolean_error &error);
};

template <class T, class I>
bool build_relation_replay_bundle(signed_feature_relations<T, I> &artifact,
                                  const relation_capabilities &capabilities,
                                  bounded_boolean_error &error) {
  return relation_replay_bundle_builder<T, I>::build(artifact, capabilities,
                                                     error);
}

template <class T, class I>
bool verify_relation_replay_bundle(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
