#pragma once

#include "SignedFeatureRelations.h"

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool verify_relation_event_candidate_evidence(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
