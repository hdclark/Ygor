#pragma once

#include "SignedFeatureRelations.h"

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
std::vector<std::uint8_t>
encode_signed_feature_relations(const signed_feature_relations<T, I> &artifact);

template <class T, class I>
bool verify_relation_codec(const signed_feature_relations<T, I> &artifact,
                           bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
