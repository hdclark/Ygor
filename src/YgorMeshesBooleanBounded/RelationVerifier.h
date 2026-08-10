#pragma once

#include "RelationCodec.h"

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool verify_signed_feature_relations(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
