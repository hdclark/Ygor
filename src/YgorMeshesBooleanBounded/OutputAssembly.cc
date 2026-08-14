#include "OutputAssembly.h"

namespace ygor::mesh_boolean::bounded {

#define YGOR_INSTANTIATE_OUTPUT_ASSEMBLY_BUILD(T, I)                         \
  template boolean_outcome<                                                  \
      std::shared_ptr<const assembled_output_candidate<T, I>>>               \
  assemble_output_candidate<T, I>(                                           \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const cleaned_triangle_manifold<T>>,                   \
      output_assembly_capabilities, output_assembly_codec_limits)

YGOR_INSTANTIATE_OUTPUT_ASSEMBLY_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_OUTPUT_ASSEMBLY_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_OUTPUT_ASSEMBLY_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_OUTPUT_ASSEMBLY_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_OUTPUT_ASSEMBLY_BUILD

} // namespace ygor::mesh_boolean::bounded
