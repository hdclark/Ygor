#include "FinalVerification.h"

namespace ygor::mesh_boolean::bounded {

#define YGOR_INSTANTIATE_FINAL_VERIFICATION_BUILD(T, I)                      \
  template boolean_outcome<                                                  \
      std::shared_ptr<const verified_boolean_result<T, I>>>                  \
  verify_and_publish<T, I>(                                                  \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const cleaned_triangle_manifold<T>>,                   \
      std::shared_ptr<const assembled_output_candidate<T, I>>,               \
      final_verification_capabilities)

YGOR_INSTANTIATE_FINAL_VERIFICATION_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_FINAL_VERIFICATION_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_FINAL_VERIFICATION_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_FINAL_VERIFICATION_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_FINAL_VERIFICATION_BUILD

} // namespace ygor::mesh_boolean::bounded
