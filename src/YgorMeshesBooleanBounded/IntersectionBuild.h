#pragma once

#include "IntersectionVerifier.h"
#include "Outcome.h"

#include <cstdint>
#include <memory>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_intersection_complex<T, I>>>
build_canonical_intersection_complex(
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    intersection_capabilities capabilities,
    intersection_codec_limits codec_limits = {},
    intersection_verifier_limits verifier_limits = {});

#define YGOR_DECLARE_INTERSECTION_BUILD(T, I)                               \
  extern template boolean_outcome<                                          \
      std::shared_ptr<const canonical_intersection_complex<T, I>>>          \
  build_canonical_intersection_complex<T, I>(                               \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      intersection_capabilities, intersection_codec_limits,                 \
      intersection_verifier_limits)

YGOR_DECLARE_INTERSECTION_BUILD(float, std::uint32_t);
YGOR_DECLARE_INTERSECTION_BUILD(float, std::uint64_t);
YGOR_DECLARE_INTERSECTION_BUILD(double, std::uint32_t);
YGOR_DECLARE_INTERSECTION_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_INTERSECTION_BUILD

} // namespace ygor::mesh_boolean::bounded
