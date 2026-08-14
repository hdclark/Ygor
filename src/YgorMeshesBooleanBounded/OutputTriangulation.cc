#include "OutputTriangulation.h"

namespace ygor::mesh_boolean::bounded {

#define YGOR_INSTANTIATE_OUTPUT_TRIANGULATION_BUILD(T, I)                    \
  template boolean_outcome<                                                  \
      std::shared_ptr<const triangulated_output_complex<T, I>>>              \
  build_triangulated_output_complex<T, I>(                                   \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const retained_surface_complex<T, I>>,                 \
      std::shared_ptr<const polygonal_output_complex<T, I>>,                 \
      output_triangulation_capabilities, output_triangulation_codec_limits)

YGOR_INSTANTIATE_OUTPUT_TRIANGULATION_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_OUTPUT_TRIANGULATION_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_OUTPUT_TRIANGULATION_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_OUTPUT_TRIANGULATION_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_OUTPUT_TRIANGULATION_BUILD

} // namespace ygor::mesh_boolean::bounded
