#include "MeshCleanup.h"

namespace ygor::mesh_boolean::bounded {

#define YGOR_INSTANTIATE_CLEANUP_BUILD(T, I)                                 \
  template boolean_outcome<                                                  \
      std::shared_ptr<const cleaned_triangle_manifold<T>>>                   \
  build_cleaned_triangle_manifold<T, I>(                                     \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const retained_surface_complex<T, I>>,                 \
      std::shared_ptr<const polygonal_output_complex<T, I>>,                 \
      std::shared_ptr<const triangulated_output_complex<T, I>>,              \
      cleanup_capabilities, cleanup_codec_limits)

YGOR_INSTANTIATE_CLEANUP_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_CLEANUP_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_CLEANUP_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_CLEANUP_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_CLEANUP_BUILD

} // namespace ygor::mesh_boolean::bounded
