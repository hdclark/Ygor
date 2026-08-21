#include "ExactFloatExpansion.h"

namespace ygor::mesh_boolean::bounded {
template exact_relation_record exact_determinant_2x2(const std::array<float, 4> &) noexcept;
template exact_relation_record exact_determinant_2x2(const std::array<double, 4> &) noexcept;
template exact_relation_record exact_determinant_3x3(const std::array<float, 9> &) noexcept;
template exact_relation_record exact_determinant_3x3(const std::array<double, 9> &) noexcept;
template exact_relation_record exact_orient_2d(const std::array<float, 2> &, const std::array<float, 2> &, const std::array<float, 2> &) noexcept;
template exact_relation_record exact_orient_2d(const std::array<double, 2> &, const std::array<double, 2> &, const std::array<double, 2> &) noexcept;
template exact_relation_record exact_orient_3d(const std::array<float, 3> &, const std::array<float, 3> &, const std::array<float, 3> &, const std::array<float, 3> &) noexcept;
template exact_relation_record exact_orient_3d(const std::array<double, 3> &, const std::array<double, 3> &, const std::array<double, 3> &, const std::array<double, 3> &) noexcept;
} // namespace ygor::mesh_boolean::bounded
