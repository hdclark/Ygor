#pragma once

#include "MeshBooleanSymbolicRegistryFixtures.h"

namespace event_test {
using namespace symbolic_test;

template <class T, class I>
fv_surface_mesh<T, I>
extruded_ring(const std::vector<std::array<T, 2>> &ring, T lower = T(0),
              T upper = T(1)) {
  fv_surface_mesh<T, I> mesh;
  for (T z : {lower, upper})
    for (const auto &point : ring)
      mesh.vertices.push_back({point[0], point[1], z});

  std::vector<I> bottom, top;
  for (std::size_t i = 0; i < ring.size(); ++i) {
    bottom.push_back(static_cast<I>(ring.size() - 1 - i));
    top.push_back(static_cast<I>(ring.size() + i));
  }
  mesh.faces.push_back(std::move(bottom));
  mesh.faces.push_back(std::move(top));
  for (std::size_t i = 0; i < ring.size(); ++i) {
    const auto next = (i + 1) % ring.size();
    mesh.faces.push_back({static_cast<I>(i), static_cast<I>(next),
                          static_cast<I>(ring.size() + next),
                          static_cast<I>(ring.size() + i)});
  }
  return mesh;
}

struct vertical_crossing {
  exact_scalar y;
  std::size_t edge = 0;
};

inline exact_scalar edge_y_at(const std::vector<exact_point2> &ring,
                              std::size_t edge, const exact_scalar &x) {
  const auto &a = ring[edge];
  const auto &b = ring[(edge + 1) % ring.size()];
  return a.y + (b.y - a.y) * ((x - a.x) / (b.x - a.x));
}

inline std::vector<vertical_crossing>
vertical_crossings(const std::vector<exact_point2> &ring,
                   const exact_scalar &x) {
  std::vector<vertical_crossing> out;
  for (std::size_t i = 0; i < ring.size(); ++i) {
    const auto &a = ring[i];
    const auto &b = ring[(i + 1) % ring.size()];
    if (a.x == b.x) continue;
    const auto low = a.x < b.x ? a.x : b.x;
    const auto high = a.x < b.x ? b.x : a.x;
    if (low < x && x < high) out.push_back({edge_y_at(ring, i, x), i});
  }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    return a.y == b.y ? a.edge < b.edge : a.y < b.y;
  });
  if ((out.size() & 1U) != 0) throw std::logic_error("odd vertical crossings");
  return out;
}

// Independent bounded oracle: all boundary intersections define vertical
// slabs. Within a slab the crossing order is fixed, so overlap length is
// linear and its exact trapezoidal integral gives the intersection area.
inline exact_scalar
concave_intersection_area_oracle(const std::vector<exact_point2> &a,
                                 const std::vector<exact_point2> &b) {
  std::vector<exact_scalar> cuts;
  for (const auto &p : a) cuts.push_back(p.x);
  for (const auto &p : b) cuts.push_back(p.x);
  for (std::size_t i = 0; i < a.size(); ++i) {
    exact_segment2 sa{a[i], a[(i + 1) % a.size()]};
    for (std::size_t j = 0; j < b.size(); ++j) {
      auto relation = relate_segments(
          sa, exact_segment2{b[j], b[(j + 1) % b.size()]});
      if (relation.point) cuts.push_back(relation.point->x);
      if (relation.overlap_segment) {
        cuts.push_back(relation.overlap_segment->origin.x);
        cuts.push_back(relation.overlap_segment->destination.x);
      }
    }
  }
  std::sort(cuts.begin(), cuts.end());
  cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

  exact_scalar area(0);
  for (std::size_t slab = 0; slab + 1 < cuts.size(); ++slab) {
    if (cuts[slab] == cuts[slab + 1]) continue;
    const auto mid = (cuts[slab] + cuts[slab + 1]) / exact_scalar(2);
    const auto ca = vertical_crossings(a, mid);
    const auto cb = vertical_crossings(b, mid);
    std::size_t ia = 0, ib = 0;
    while (ia + 1 < ca.size() && ib + 1 < cb.size()) {
      const auto lower = ca[ia].y < cb[ib].y ? cb[ib] : ca[ia];
      const auto upper = ca[ia + 1].y < cb[ib + 1].y ? ca[ia + 1] : cb[ib + 1];
      if (lower.y < upper.y) {
        const bool lower_a = !(ca[ia].y < cb[ib].y);
        const bool upper_a = ca[ia + 1].y < cb[ib + 1].y;
        auto y = [&](bool from_a, std::size_t edge, const exact_scalar &x) {
          return edge_y_at(from_a ? a : b, edge, x);
        };
        const auto left_length =
            y(upper_a, upper.edge, cuts[slab]) -
            y(lower_a, lower.edge, cuts[slab]);
        const auto right_length =
            y(upper_a, upper.edge, cuts[slab + 1]) -
            y(lower_a, lower.edge, cuts[slab + 1]);
        area = area + (left_length + right_length) *
                          (cuts[slab + 1] - cuts[slab]) / exact_scalar(2);
      }
      if (ca[ia + 1].y < cb[ib + 1].y)
        ia += 2;
      else if (cb[ib + 1].y < ca[ia + 1].y)
        ib += 2;
      else {
        ia += 2;
        ib += 2;
      }
    }
  }
  return area;
}

inline exact_point2 point2(std::int64_t x, std::int64_t y) {
  return {exact_scalar(x), exact_scalar(y)};
}
} // namespace event_test
