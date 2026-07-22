#pragma once

#include "ExactFloatExpansion.h"
#include "SourceTriangulationTypes.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace source_polygon_kernel_detail {
inline std::int8_t exact_record_sign(const exact_relation_record &record) noexcept {
  return record.status == exact_relation_status::exact_negative
             ? -1
             : record.status == exact_relation_status::exact_positive
                   ? 1
                   : record.status == exact_relation_status::exact_zero ? 0 : 2;
}

template <class T>
std::optional<finite_interval<T>> difference(const finite_interval<T> &a,
                                             const finite_interval<T> &b) {
  auto result = interval_subtract(a, b);
  return result ? result.value : std::nullopt;
}

template <class T>
std::optional<finite_interval<T>> determinant(
    const projected_source_point<T> &a, const projected_source_point<T> &b,
    const projected_source_point<T> &c) {
  auto bax = difference(b.enclosure[0], a.enclosure[0]);
  auto bay = difference(b.enclosure[1], a.enclosure[1]);
  auto cax = difference(c.enclosure[0], a.enclosure[0]);
  auto cay = difference(c.enclosure[1], a.enclosure[1]);
  if (!bax || !bay || !cax || !cay)
    return std::nullopt;
  auto positive = interval_multiply(*bax, *cay);
  auto negative = interval_multiply(*bay, *cax);
  if (!positive || !negative)
    return std::nullopt;
  auto result = interval_subtract(*positive.value, *negative.value);
  return result ? result.value : std::nullopt;
}

template <class T>
bool intervals_separated(const finite_interval<T> &a,
                         const finite_interval<T> &b) noexcept {
  return finite_numeric_less(a.upper(), b.lower()) ||
         finite_numeric_less(b.upper(), a.lower());
}

template <class T>
bool point_box_disjoint(const projected_source_point<T> &p,
                        const projected_source_point<T> &a,
                        const projected_source_point<T> &b) noexcept {
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const T low = finite_numeric_less(a.enclosure[axis].lower(),
                                     b.enclosure[axis].lower())
                      ? a.enclosure[axis].lower()
                      : b.enclosure[axis].lower();
    const T high = finite_numeric_less(a.enclosure[axis].upper(),
                                      b.enclosure[axis].upper())
                       ? b.enclosure[axis].upper()
                       : a.enclosure[axis].upper();
    if (finite_numeric_less(p.enclosure[axis].upper(), low) ||
        finite_numeric_less(high, p.enclosure[axis].lower()))
      return true;
  }
  return false;
}

template <class T>
bool segment_boxes_disjoint(const projected_source_point<T> &a,
                            const projected_source_point<T> &b,
                            const projected_source_point<T> &c,
                            const projected_source_point<T> &d) noexcept {
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const T ab_low = finite_numeric_less(a.enclosure[axis].lower(),
                                        b.enclosure[axis].lower())
                         ? a.enclosure[axis].lower()
                         : b.enclosure[axis].lower();
    const T ab_high = finite_numeric_less(a.enclosure[axis].upper(),
                                         b.enclosure[axis].upper())
                          ? b.enclosure[axis].upper()
                          : a.enclosure[axis].upper();
    const T cd_low = finite_numeric_less(c.enclosure[axis].lower(),
                                        d.enclosure[axis].lower())
                         ? c.enclosure[axis].lower()
                         : d.enclosure[axis].lower();
    const T cd_high = finite_numeric_less(c.enclosure[axis].upper(),
                                         d.enclosure[axis].upper())
                          ? d.enclosure[axis].upper()
                          : c.enclosure[axis].upper();
    if (finite_numeric_less(ab_high, cd_low) ||
        finite_numeric_less(cd_high, ab_low))
      return true;
  }
  return false;
}
} // namespace source_polygon_kernel_detail

template <class T> class bounded_source_polygon_kernel final {
public:
  static source_orientation_evidence<T>
  orientation(const projected_source_point<T> &a,
              const projected_source_point<T> &b,
              const projected_source_point<T> &c) {
    source_orientation_evidence<T> evidence;
    auto determinant = source_polygon_kernel_detail::determinant(a, b, c);
    if (determinant)
      evidence.determinant = *determinant;
    evidence.exact_sign = source_polygon_kernel_detail::exact_record_sign(
        exact_orient_2d(a.nominal, b.nominal, c.nominal));
    if (!determinant) {
      evidence.bounded_sign = bounded_planar_sign::uncertain;
    } else if (finite_numeric_less(T(0), determinant->lower())) {
      evidence.bounded_sign = bounded_planar_sign::positive;
    } else if (finite_numeric_less(determinant->upper(), T(0))) {
      evidence.bounded_sign = bounded_planar_sign::negative;
    } else {
      evidence.bounded_sign = bounded_planar_sign::uncertain;
    }
    return evidence;
  }

  static std::optional<source_orientation_evidence<T>>
  polygon_orientation(const std::vector<projected_source_point<T>> &polygon) {
    if (polygon.size() < 3)
      return std::nullopt;
    auto total = finite_interval<T>::checked_singleton(T(0));
    if (!total)
      return std::nullopt;
    std::vector<std::array<T, 2>> nominal;
    nominal.reserve(polygon.size());
    for (const auto &point : polygon)
      nominal.push_back(point.nominal);
    for (std::size_t i = 0; i < polygon.size(); ++i) {
      const auto &a = polygon[i];
      const auto &b = polygon[(i + 1) % polygon.size()];
      auto positive = interval_multiply(a.enclosure[0], b.enclosure[1]);
      auto negative = interval_multiply(b.enclosure[0], a.enclosure[1]);
      if (!positive || !negative)
        return std::nullopt;
      auto edge = interval_subtract(*positive.value, *negative.value);
      if (!edge)
        return std::nullopt;
      auto next = interval_add(*total, *edge.value);
      if (!next)
        return std::nullopt;
      total = next.value;
    }
    source_orientation_evidence<T> result;
    result.determinant = *total;
    result.exact_sign = source_polygon_kernel_detail::exact_record_sign(
        exact_polygon_area_2d(nominal));
    if (finite_numeric_less(T(0), total->lower()))
      result.bounded_sign = bounded_planar_sign::positive;
    else if (finite_numeric_less(total->upper(), T(0)))
      result.bounded_sign = bounded_planar_sign::negative;
    else
      result.bounded_sign = bounded_planar_sign::uncertain;
    return result;
  }

  static bounded_segment_relation
  segment_relation(const projected_source_point<T> &a,
                   const projected_source_point<T> &b,
                   const projected_source_point<T> &c,
                   const projected_source_point<T> &d) {
    if (source_polygon_kernel_detail::segment_boxes_disjoint(a, b, c, d))
      return bounded_segment_relation::definitely_disjoint;
    const auto ab_c = orientation(a, b, c);
    const auto ab_d = orientation(a, b, d);
    const auto cd_a = orientation(c, d, a);
    const auto cd_b = orientation(c, d, b);
    const auto value = [](const source_orientation_evidence<T> &evidence,
                           bool identity_zero) {
      if (identity_zero)
        return 0;
      if (evidence.bounded_sign == bounded_planar_sign::negative)
        return -1;
      if (evidence.bounded_sign == bounded_planar_sign::positive)
        return 1;
      // A singleton zero enclosure is a definite collinearity statement.  It is
      // still ineligible as an ear orientation, but it is sufficient to prove a
      // segment contact or a disjoint collinear ordering.
      if (evidence.exact_sign == 0 &&
          evidence.determinant.lower() == T(0) &&
          evidence.determinant.upper() == T(0))
        return 0;
      return 2;
    };
    const int x = value(ab_c, c.source_vertex == a.source_vertex ||
                                   c.source_vertex == b.source_vertex);
    const int y = value(ab_d, d.source_vertex == a.source_vertex ||
                                   d.source_vertex == b.source_vertex);
    const int z = value(cd_a, a.source_vertex == c.source_vertex ||
                                   a.source_vertex == d.source_vertex);
    const int w = value(cd_b, b.source_vertex == c.source_vertex ||
                                   b.source_vertex == d.source_vertex);
    if (x == 2 || y == 2 || z == 2 || w == 2)
      return bounded_segment_relation::uncertain;
    if (x * y < 0 && z * w < 0)
      return bounded_segment_relation::proper_crossing;
    if (x == 0 && y == 0 && z == 0 && w == 0) {
      const bool shared_endpoint =
          a.source_vertex == c.source_vertex || a.source_vertex == d.source_vertex ||
          b.source_vertex == c.source_vertex || b.source_vertex == d.source_vertex;
      return shared_endpoint ? bounded_segment_relation::endpoint_contact
                             : bounded_segment_relation::collinear_overlap;
    }
    if ((x == 0 && !source_polygon_kernel_detail::point_box_disjoint(c, a, b)) ||
        (y == 0 && !source_polygon_kernel_detail::point_box_disjoint(d, a, b)) ||
        (z == 0 && !source_polygon_kernel_detail::point_box_disjoint(a, c, d)) ||
        (w == 0 && !source_polygon_kernel_detail::point_box_disjoint(b, c, d)))
      return bounded_segment_relation::endpoint_contact;
    return bounded_segment_relation::definitely_disjoint;
  }

  static source_candidate_disposition
  point_in_or_on_triangle(const projected_source_point<T> &p,
                          const projected_source_point<T> &a,
                          const projected_source_point<T> &b,
                          const projected_source_point<T> &c,
                          bounded_planar_sign polygon_sign) {
    if (source_polygon_kernel_detail::point_box_disjoint(p, a, b) &&
        source_polygon_kernel_detail::point_box_disjoint(p, b, c) &&
        source_polygon_kernel_detail::point_box_disjoint(p, c, a))
      return source_candidate_disposition::definitely_rejected;
    const int expected = static_cast<int>(polygon_sign);
    bool uncertain = false;
    for (const auto &evidence :
         {orientation(a, b, p), orientation(b, c, p), orientation(c, a, p)}) {
      if (evidence.bounded_sign == bounded_planar_sign::uncertain) {
        uncertain = true;
        continue;
      }
      if (static_cast<int>(evidence.bounded_sign) != expected)
        return source_candidate_disposition::definitely_rejected;
    }
    return uncertain ? source_candidate_disposition::uncertainty_rejected
                     : source_candidate_disposition::eligible;
  }

  static bool definitely_same_point(const projected_source_point<T> &a,
                                    const projected_source_point<T> &b) noexcept {
    return a.source_vertex == b.source_vertex;
  }
};

} // namespace ygor::mesh_boolean::bounded
