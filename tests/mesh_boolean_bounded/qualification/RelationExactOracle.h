#pragma once

#include "ExactFloatImport.h"
#include "ExactGeometryOracle.h"
#include "YgorMeshesBooleanBounded/SourceEdgeRelationTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace ygor::mesh_boolean::qualification {

struct ExactSourceEdgeClassification final {
  bounded::source_edge_support_class support =
      bounded::source_edge_support_class::skew_separated;
  bounded::source_edge_contact_class contact =
      bounded::source_edge_contact_class::none;
  bounded::source_edge_orientation_relation orientation =
      bounded::source_edge_orientation_relation::not_applicable;
  std::vector<ExactRational> first_parameters;
  std::vector<ExactRational> second_parameters;
  std::vector<ExactPoint3> points;
};

namespace relation_exact_oracle_detail {

template <class T> ExactRational import_scalar(T value) {
  if constexpr (std::is_same<T, float>::value)
    return import_exact(value).value;
  else {
    static_assert(std::is_same<T, double>::value,
                  "relation exact oracle supports binary32/binary64 only");
    return import_exact(value).value;
  }
}

inline ExactVector3 subtract(const ExactPoint3 &a, const ExactPoint3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline ExactPoint3 add_scaled(const ExactPoint3 &point,
                              const ExactVector3 &direction,
                              const ExactRational &parameter) {
  return {point.x + direction.x * parameter,
          point.y + direction.y * parameter,
          point.z + direction.z * parameter};
}

inline ExactRational component(const ExactPoint3 &point, std::size_t axis) {
  switch (axis) {
  case 0:
    return point.x;
  case 1:
    return point.y;
  default:
    return point.z;
  }
}

inline bool zero_vector(const ExactVector3 &value) noexcept {
  return value.x.is_zero() && value.y.is_zero() && value.z.is_zero();
}

inline bool in_closed_unit(const ExactRational &value) {
  return value >= ExactRational(0) && value <= ExactRational(1);
}

inline bool in_open_unit(const ExactRational &value) {
  return value > ExactRational(0) && value < ExactRational(1);
}

inline std::size_t nonzero_axis(const ExactVector3 &value) {
  if (!value.x.is_zero())
    return 0;
  if (!value.y.is_zero())
    return 1;
  return 2;
}

inline std::array<std::size_t, 2>
solve_axes_from_cross(const ExactVector3 &cross_value) {
  if (!cross_value.x.is_zero())
    return {{1, 2}};
  if (!cross_value.y.is_zero())
    return {{2, 0}};
  return {{0, 1}};
}

template <class T>
ExactPoint3 exact_point(const bounded::source_edge_geometry_snapshot<T> &point) {
  return {import_scalar(point.rounded_nominal[0]),
          import_scalar(point.rounded_nominal[1]),
          import_scalar(point.rounded_nominal[2])};
}

template <class T>
bool contains(const bounded::finite_interval<T> &interval,
              const ExactRational &value) {
  return import_scalar(interval.lower()) <= value &&
         value <= import_scalar(interval.upper());
}

template <class T>
bool point_encloses(const bounded::source_edge_geometry_snapshot<T> &point,
                    const ExactPoint3 &value) {
  return contains(point.enclosure[0], value.x) &&
         contains(point.enclosure[1], value.y) &&
         contains(point.enclosure[2], value.z);
}

} // namespace relation_exact_oracle_detail

template <class T>
ExactSourceEdgeClassification classify_source_edges_exact(
    const bounded::source_edge_relation_record<T> &record) {
  using namespace relation_exact_oracle_detail;
  const auto p0 = exact_point(record.first_start);
  const auto p1 = exact_point(record.first_end);
  const auto q0 = exact_point(record.second_start);
  const auto q1 = exact_point(record.second_end);
  const auto u = subtract(p1, p0);
  const auto v = subtract(q1, q0);
  const auto w = subtract(q0, p0);
  const auto uv = cross(u, v);

  ExactSourceEdgeClassification result;
  if (!zero_vector(uv)) {
    const auto coplanarity = dot(w, uv);
    if (!coplanarity.is_zero()) {
      result.support = bounded::source_edge_support_class::skew_separated;
      return result;
    }

    result.support =
        bounded::source_edge_support_class::nonparallel_coplanar;
    const auto axes = solve_axes_from_cross(uv);
    const auto ui = component(u, axes[0]);
    const auto uj = component(u, axes[1]);
    const auto vi = component(v, axes[0]);
    const auto vj = component(v, axes[1]);
    const auto wi = component(w, axes[0]);
    const auto wj = component(w, axes[1]);
    const auto denominator = determinant2(ui, -vi, uj, -vj);
    const auto first_parameter = determinant2(wi, -vi, wj, -vj) /
                                 denominator;
    const auto second_parameter = determinant2(ui, wi, uj, wj) /
                                  denominator;
    // The production contract retains the unique support-line parameters even
    // when the finite segments do not meet.  Point construction is published
    // only when both parameters lie in the closed segment domains.
    result.first_parameters.push_back(first_parameter);
    result.second_parameters.push_back(second_parameter);
    if (!in_closed_unit(first_parameter) ||
        !in_closed_unit(second_parameter))
      return result;

    result.contact = in_open_unit(first_parameter) &&
                             in_open_unit(second_parameter)
                         ? bounded::source_edge_contact_class::proper_crossing
                         : bounded::source_edge_contact_class::endpoint_contact;
    result.points.push_back(add_scaled(p0, u, first_parameter));
    return result;
  }

  const auto wu = cross(w, u);
  if (!zero_vector(wu)) {
    result.support = bounded::source_edge_support_class::parallel_separated;
    return result;
  }

  result.support = bounded::source_edge_support_class::collinear;
  result.orientation = dot(u, v).sign() >= 0
                           ? bounded::source_edge_orientation_relation::same
                           : bounded::source_edge_orientation_relation::opposite;
  const auto axis = nonzero_axis(u);
  const auto first_q0 = component(w, axis) / component(u, axis);
  const auto first_q1 =
      (component(q1, axis) - component(p0, axis)) / component(u, axis);
  const auto q_min = first_q0 < first_q1 ? first_q0 : first_q1;
  const auto q_max = first_q0 < first_q1 ? first_q1 : first_q0;
  const auto overlap_first =
      q_min > ExactRational(0) ? q_min : ExactRational(0);
  const auto overlap_last =
      q_max < ExactRational(1) ? q_max : ExactRational(1);
  if (overlap_last < overlap_first) {
    // A disjoint collinear relation retains the second edge's two endpoints in
    // the first carrier parameterization and their native 0/1 parameters.
    result.first_parameters.push_back(first_q0);
    result.first_parameters.push_back(first_q1);
    result.second_parameters.push_back(ExactRational(0));
    result.second_parameters.push_back(ExactRational(1));
    return result;
  }

  const auto second_parameter = [&](const ExactRational &first_parameter) {
    const auto point = add_scaled(p0, u, first_parameter);
    return (component(point, axis) - component(q0, axis)) /
           component(v, axis);
  };
  result.first_parameters.push_back(overlap_first);
  result.second_parameters.push_back(second_parameter(overlap_first));
  result.points.push_back(add_scaled(p0, u, overlap_first));
  if (overlap_first == overlap_last) {
    result.contact = bounded::source_edge_contact_class::point_contact;
    return result;
  }

  result.first_parameters.push_back(overlap_last);
  result.second_parameters.push_back(second_parameter(overlap_last));
  result.points.push_back(add_scaled(p0, u, overlap_last));
  if (q_min == ExactRational(0) && q_max == ExactRational(1))
    result.contact = bounded::source_edge_contact_class::equal;
  else if (q_min >= ExactRational(0) && q_max <= ExactRational(1))
    result.contact =
        bounded::source_edge_contact_class::first_contains_second;
  else if (q_min <= ExactRational(0) && q_max >= ExactRational(1))
    result.contact =
        bounded::source_edge_contact_class::second_contains_first;
  else
    result.contact = bounded::source_edge_contact_class::partial_overlap;
  return result;
}

template <class T>
bool exact_source_edge_record_agrees(
    const bounded::source_edge_relation_record<T> &record,
    std::string &failure) {
  using namespace relation_exact_oracle_detail;
  const auto exact = classify_source_edges_exact(record);
  if (exact.support != record.support || exact.contact != record.contact ||
      exact.orientation != record.orientation) {
    failure = "exact source-edge category disagrees with Component 07";
    return false;
  }
  if (exact.first_parameters.size() != record.parameter_count ||
      exact.second_parameters.size() != record.parameter_count ||
      exact.points.size() != record.point_count) {
    failure = "exact source-edge cardinality disagrees with Component 07";
    return false;
  }
  for (std::size_t i = 0; i < exact.first_parameters.size(); ++i) {
    if (!contains(record.first_parameters[i].enclosure,
                  exact.first_parameters[i]) ||
        !contains(record.second_parameters[i].enclosure,
                  exact.second_parameters[i])) {
      failure = "exact source-edge parameter escapes the published enclosure";
      return false;
    }
  }
  for (std::size_t i = 0; i < exact.points.size(); ++i) {
    if (!point_encloses(record.points[i].point, exact.points[i])) {
      failure = "exact source-edge point escapes the published enclosure";
      return false;
    }
  }
  return true;
}

} // namespace ygor::mesh_boolean::qualification
