#include "Selection.h"

namespace ygor::mesh_boolean::bounded {

#define YGOR_INSTANTIATE_SELECTION_BUILD(T, I)                               \
  template boolean_outcome<                                                  \
      std::shared_ptr<const retained_surface_complex<T, I>>>                 \
  build_retained_surface_complex<T, I>(                                      \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const classification_complex<T, I>>,                   \
      selection_capabilities, selection_codec_limits)

YGOR_INSTANTIATE_SELECTION_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_SELECTION_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_SELECTION_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_SELECTION_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_SELECTION_BUILD

} // namespace ygor::mesh_boolean::bounded
