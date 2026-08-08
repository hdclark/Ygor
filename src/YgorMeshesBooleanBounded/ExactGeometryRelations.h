#pragma once

#include "ExactFloatExpansion.h"

#include <array>
#include <cstddef>

namespace ygor::mesh_boolean::bounded {

namespace exact_geometry_relations_detail {

template <class T, std::size_t Capacity>
exact_float_expansion_core::status exact_difference(
    T left, T right,
    exact_float_expansion_core::expansion<T, Capacity> &result) noexcept {
  using namespace exact_float_expansion_core;
  if (!finite(left) || !finite(right))
    return status::non_finite_input;
  T rounded = T(0);
  T residual = T(0);
  two_diff(left, right, rounded, residual);
  result.size = residual == T(0) ? 1 : 2;
  result.limbs[0] = residual == T(0) ? rounded : residual;
  if (residual != T(0))
    result.limbs[1] = rounded;
  return status::success;
}

template <class T, std::size_t Capacity>
exact_float_expansion_core::status exact_product(
    const exact_float_expansion_core::expansion<T, Capacity> &left,
    const exact_float_expansion_core::expansion<T, Capacity> &right,
    exact_float_expansion_core::expansion<T, Capacity> &result) noexcept {
  using namespace exact_float_expansion_core;
  expansion<T, Capacity> total;
  total.size = 1;
  total.limbs[0] = T(0);
  for (std::size_t i = 0; i < right.size; ++i) {
    expansion<T, Capacity> term;
    auto operation = scale(left, right.limbs[i], term);
    if (operation != status::success)
      return operation;
    expansion<T, Capacity> next;
    operation = sum(total, term, next);
    if (operation != status::success)
      return operation;
    total = next;
  }
  return compress(total, result);
}

template <class T, std::size_t Capacity>
exact_float_expansion_core::status exact_difference_of_products(
    const exact_float_expansion_core::expansion<T, Capacity> &a,
    const exact_float_expansion_core::expansion<T, Capacity> &b,
    const exact_float_expansion_core::expansion<T, Capacity> &c,
    const exact_float_expansion_core::expansion<T, Capacity> &d,
    exact_float_expansion_core::expansion<T, Capacity> &result) noexcept {
  using namespace exact_float_expansion_core;
  expansion<T, Capacity> first;
  expansion<T, Capacity> second;
  auto operation = exact_product(a, b, first);
  if (operation != status::success)
    return operation;
  operation = exact_product(c, d, second);
  if (operation != status::success)
    return operation;
  expansion<T, Capacity> negated;
  operation = scale(second, T(-1), negated);
  if (operation != status::success)
    return operation;
  expansion<T, Capacity> difference;
  operation = sum(first, negated, difference);
  if (operation != status::success)
    return operation;
  return compress(difference, result);
}

template <class T>
exact_relation_record combined_zero_relation(
    const std::array<exact_relation_record, 3> &components,
    exact_relation_formula_code formula) noexcept {
  std::size_t used = 0;
  std::size_t limit = 0;
  for (const auto &component : components) {
    if (component.evaluation_status != numeric_status::success ||
        component.status == exact_relation_status::invalid ||
        component.status == exact_relation_status::unavailable)
      return detail::relation_failure(
          formula, component.evaluation_status, component.capacity_limit);
    used += component.capacity_used;
    limit += component.capacity_limit;
  }

  exact_relation_record result;
  result.formula = formula;
  result.status =
      components[0].status == exact_relation_status::exact_zero &&
              components[1].status == exact_relation_status::exact_zero &&
              components[2].status == exact_relation_status::exact_zero
          ? exact_relation_status::exact_zero
          : exact_relation_status::exact_positive;
  result.evaluation_status = numeric_status::success;
  result.capacity_used = used;
  result.capacity_limit = limit;
  return result;
}

} // namespace exact_geometry_relations_detail

// Exact zero test for cross((a1-a0), (b1-b0)). The returned relation is
// nonnegative: exact_zero means parallel directions and exact_positive means
// nonparallel directions. It operates on the stored endpoint bits directly;
// no rounded difference is used as exact evidence.
template <class T>
exact_relation_record exact_segment_directions_parallel_3d(
    const std::array<T, 3> &a0, const std::array<T, 3> &a1,
    const std::array<T, 3> &b0, const std::array<T, 3> &b1) noexcept {
  static_assert(supported_precision_scalar_v<T>);
  using namespace exact_float_expansion_core;
  constexpr std::size_t capacity = 128;

  std::array<expansion<T, capacity>, 3> u;
  std::array<expansion<T, capacity>, 3> v;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto operation =
        exact_geometry_relations_detail::exact_difference(a1[axis], a0[axis],
                                                          u[axis]);
    if (operation != status::success)
      return detail::relation_failure(
          exact_relation_formula_code::collinearity_3d,
          detail::numeric_status_from_core(operation), capacity);
    operation =
        exact_geometry_relations_detail::exact_difference(b1[axis], b0[axis],
                                                          v[axis]);
    if (operation != status::success)
      return detail::relation_failure(
          exact_relation_formula_code::collinearity_3d,
          detail::numeric_status_from_core(operation), capacity);
  }

  constexpr std::array<std::array<std::size_t, 4>, 3> indices{{
      {{1, 2, 2, 1}},
      {{2, 0, 0, 2}},
      {{0, 1, 1, 0}},
  }};
  std::array<exact_relation_record, 3> components;
  for (std::size_t component = 0; component < 3; ++component) {
    expansion<T, capacity> value;
    const auto operation =
        exact_geometry_relations_detail::exact_difference_of_products(
            u[indices[component][0]], v[indices[component][1]],
            u[indices[component][2]], v[indices[component][3]], value);
    if (operation != status::success)
      return detail::relation_failure(
          exact_relation_formula_code::collinearity_3d,
          detail::numeric_status_from_core(operation), capacity);
    components[component].formula =
        exact_relation_formula_code::collinearity_3d;
    components[component].status =
        detail::exact_status_from_core(expansion_sign(value));
    components[component].evaluation_status = numeric_status::success;
    components[component].capacity_used = value.size;
    components[component].capacity_limit = capacity;
  }
  return exact_geometry_relations_detail::combined_zero_relation<T>(
      components, exact_relation_formula_code::collinearity_3d);
}

// Exact zero test for a point and a line through two stored-coordinate points.
// The relation is nonnegative and uses all three coordinate-plane orientations.
template <class T>
exact_relation_record exact_collinearity_3d(
    const std::array<T, 3> &a, const std::array<T, 3> &b,
    const std::array<T, 3> &point) noexcept {
  static_assert(supported_precision_scalar_v<T>);
  std::array<exact_relation_record, 3> components{{
      exact_orient_2d<T>({{a[0], a[1]}}, {{b[0], b[1]}},
                         {{point[0], point[1]}}),
      exact_orient_2d<T>({{a[0], a[2]}}, {{b[0], b[2]}},
                         {{point[0], point[2]}}),
      exact_orient_2d<T>({{a[1], a[2]}}, {{b[1], b[2]}},
                         {{point[1], point[2]}}),
  }};
  for (auto &component : components)
    component.formula = exact_relation_formula_code::collinearity_3d;
  return exact_geometry_relations_detail::combined_zero_relation<T>(
      components, exact_relation_formula_code::collinearity_3d);
}

namespace exact_geometry_relations_detail {

template <class T, std::size_t Capacity>
exact_float_expansion_core::status exact_plane_normal(
    const std::array<T, 3> &a0, const std::array<T, 3> &a1,
    const std::array<T, 3> &a2,
    std::array<exact_float_expansion_core::expansion<T, Capacity>, 3> &normal) noexcept {
  using namespace exact_float_expansion_core;
  std::array<expansion<T, Capacity>, 3> u;
  std::array<expansion<T, Capacity>, 3> v;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto operation = exact_difference(a1[axis], a0[axis], u[axis]);
    if (operation != status::success)
      return operation;
    operation = exact_difference(a2[axis], a0[axis], v[axis]);
    if (operation != status::success)
      return operation;
  }
  constexpr std::array<std::array<std::size_t, 4>, 3> indices{{
      {{1, 2, 2, 1}},
      {{2, 0, 0, 2}},
      {{0, 1, 1, 0}},
  }};
  for (std::size_t component = 0; component < 3; ++component) {
    const auto operation = exact_difference_of_products(
        u[indices[component][0]], v[indices[component][1]],
        u[indices[component][2]], v[indices[component][3]],
        normal[component]);
    if (operation != status::success)
      return operation;
  }
  return status::success;
}

template <class T>
exact_relation_record exact_plane_normal_failure(
    exact_relation_formula_code formula,
    exact_float_expansion_core::status operation,
    std::size_t capacity) noexcept {
  return detail::relation_failure(formula,
                                  detail::numeric_status_from_core(operation),
                                  capacity);
}

} // namespace exact_geometry_relations_detail

// Exact zero test for cross(normal(a0,a1,a2), normal(b0,b1,b2)).
// The record is nonnegative: exact_zero means parallel support planes and
// exact_positive means nonparallel support planes. All evidence is evaluated
// from the stored support-point bits without rounded plane coefficients.
template <class T>
exact_relation_record exact_plane_normals_parallel_3d(
    const std::array<T, 3> &a0, const std::array<T, 3> &a1,
    const std::array<T, 3> &a2, const std::array<T, 3> &b0,
    const std::array<T, 3> &b1, const std::array<T, 3> &b2) noexcept {
  static_assert(supported_precision_scalar_v<T>);
  using namespace exact_float_expansion_core;
  constexpr std::size_t capacity = 512;
  std::array<expansion<T, capacity>, 3> a_normal;
  std::array<expansion<T, capacity>, 3> b_normal;
  auto operation = exact_geometry_relations_detail::exact_plane_normal(
      a0, a1, a2, a_normal);
  if (operation != status::success)
    return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
        exact_relation_formula_code::plane_normal_parallel_3d, operation,
        capacity);
  operation = exact_geometry_relations_detail::exact_plane_normal(
      b0, b1, b2, b_normal);
  if (operation != status::success)
    return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
        exact_relation_formula_code::plane_normal_parallel_3d, operation,
        capacity);

  constexpr std::array<std::array<std::size_t, 4>, 3> indices{{
      {{1, 2, 2, 1}},
      {{2, 0, 0, 2}},
      {{0, 1, 1, 0}},
  }};
  std::array<exact_relation_record, 3> components;
  for (std::size_t component = 0; component < 3; ++component) {
    expansion<T, capacity> value;
    operation = exact_geometry_relations_detail::exact_difference_of_products(
        a_normal[indices[component][0]],
        b_normal[indices[component][1]],
        a_normal[indices[component][2]],
        b_normal[indices[component][3]], value);
    if (operation != status::success)
      return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
          exact_relation_formula_code::plane_normal_parallel_3d, operation,
          capacity);
    components[component].formula =
        exact_relation_formula_code::plane_normal_parallel_3d;
    components[component].status =
        detail::exact_status_from_core(expansion_sign(value));
    components[component].evaluation_status = numeric_status::success;
    components[component].capacity_used = value.size;
    components[component].capacity_limit = capacity;
  }
  return exact_geometry_relations_detail::combined_zero_relation<T>(
      components, exact_relation_formula_code::plane_normal_parallel_3d);
}

// Exact signed dot product of the two unnormalized support-plane normals.
// It is used only after exact parallelism has been established, so its sign
// deterministically records same versus opposite support orientation.
template <class T>
exact_relation_record exact_plane_normal_dot_3d(
    const std::array<T, 3> &a0, const std::array<T, 3> &a1,
    const std::array<T, 3> &a2, const std::array<T, 3> &b0,
    const std::array<T, 3> &b1, const std::array<T, 3> &b2) noexcept {
  static_assert(supported_precision_scalar_v<T>);
  using namespace exact_float_expansion_core;
  constexpr std::size_t capacity = 512;
  std::array<expansion<T, capacity>, 3> a_normal;
  std::array<expansion<T, capacity>, 3> b_normal;
  auto operation = exact_geometry_relations_detail::exact_plane_normal(
      a0, a1, a2, a_normal);
  if (operation != status::success)
    return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
        exact_relation_formula_code::plane_normal_dot_3d, operation, capacity);
  operation = exact_geometry_relations_detail::exact_plane_normal(
      b0, b1, b2, b_normal);
  if (operation != status::success)
    return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
        exact_relation_formula_code::plane_normal_dot_3d, operation, capacity);

  expansion<T, capacity> total;
  total.size = 1;
  total.limbs[0] = T(0);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    expansion<T, capacity> product;
    operation = exact_geometry_relations_detail::exact_product(
        a_normal[axis], b_normal[axis], product);
    if (operation != status::success)
      return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
          exact_relation_formula_code::plane_normal_dot_3d, operation,
          capacity);
    expansion<T, capacity> next;
    operation = sum(total, product, next);
    if (operation != status::success)
      return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
          exact_relation_formula_code::plane_normal_dot_3d, operation,
          capacity);
    total = next;
  }
  expansion<T, capacity> compressed;
  operation = compress(total, compressed);
  if (operation != status::success)
    return exact_geometry_relations_detail::exact_plane_normal_failure<T>(
        exact_relation_formula_code::plane_normal_dot_3d, operation, capacity);
  exact_relation_record result;
  result.formula = exact_relation_formula_code::plane_normal_dot_3d;
  result.status = detail::exact_status_from_core(expansion_sign(compressed));
  result.evaluation_status = numeric_status::success;
  result.capacity_used = compressed.size;
  result.capacity_limit = capacity;
  return result;
}

// Exact signed coplanarity relation for the four stored endpoints.
template <class T>
exact_relation_record exact_coplanarity_3d(
    const std::array<T, 3> &a0, const std::array<T, 3> &a1,
    const std::array<T, 3> &b0, const std::array<T, 3> &b1) noexcept {
  auto result = exact_orient_3d(a0, a1, b0, b1);
  result.formula = exact_relation_formula_code::coplanarity_3d;
  return result;
}

// Exact signed residual for the oriented plane through (a0,a1,a2). The sign
// matches dot(cross(a1-a0,a2-a0), point-a0), whereas orient_3d uses the
// opposite row-determinant convention.
template <class T>
exact_relation_record exact_plane_point_residual_3d(
    const std::array<T, 3> &a0, const std::array<T, 3> &a1,
    const std::array<T, 3> &a2, const std::array<T, 3> &point) noexcept {
  auto result = exact_orient_3d(a0, a1, a2, point);
  result.formula = exact_relation_formula_code::plane_point_residual_3d;
  if (result.status == exact_relation_status::exact_negative)
    result.status = exact_relation_status::exact_positive;
  else if (result.status == exact_relation_status::exact_positive)
    result.status = exact_relation_status::exact_negative;
  return result;
}

} // namespace ygor::mesh_boolean::bounded
