#include "Classification.h"

namespace ygor::mesh_boolean::bounded {

#define YGOR_INSTANTIATE_CLASSIFICATION_BUILD(T, I)                          \
  template boolean_outcome<                                                  \
      std::shared_ptr<const classification_complex<T, I>>>                   \
  build_classification_complex<T, I>(                                        \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      classification_capabilities, classification_codec_limits,              \
      classification_verifier_limits)

YGOR_INSTANTIATE_CLASSIFICATION_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_CLASSIFICATION_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_CLASSIFICATION_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_CLASSIFICATION_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_CLASSIFICATION_BUILD

} // namespace ygor::mesh_boolean::bounded
