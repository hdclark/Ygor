#pragma once

#include "MeshBooleanAnalyticFixtures.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::testing {

template <class T, class I> struct performance_fixture {
  fv_surface_mesh<T, I> a;
  fv_surface_mesh<T, I> b;
};

inline const std::array<const char *, 9> &performance_fixture_names() {
  static const std::array<const char *, 9> names{
      {"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8"}};
  return names;
}

inline std::uint32_t checked_performance_size(std::uint32_t size) {
  if (size < 1 || size > 3)
    throw std::invalid_argument("fixture size must be 1, 2, or 3");
  return size;
}

inline std::uint32_t fixture_subdivisions(std::uint32_t size) {
  return checked_performance_size(size);
}

template <class T, class I>
fv_surface_mesh<T, I> subdivided_box(T x0, T x1, T y0, T y1, T z0, T z1,
                                    std::uint32_t subdivisions) {
  if (subdivisions == 0)
    throw std::invalid_argument("box subdivision count must be positive");

  fv_surface_mesh<T, I> mesh;
  using key = std::array<std::uint32_t, 3>;
  std::map<key, I> indices;
  const auto coordinate = [subdivisions](T lo, T hi, std::uint32_t i) {
    return lo + (hi - lo) * static_cast<T>(i) / static_cast<T>(subdivisions);
  };
  auto vertex = [&](std::uint32_t x, std::uint32_t y,
                    std::uint32_t z) {
    const key k{{x, y, z}};
    const auto found = indices.find(k);
    if (found != indices.end())
      return found->second;
    if (mesh.vertices.size() > static_cast<std::size_t>(std::numeric_limits<I>::max()))
      throw std::overflow_error("fixture vertex index overflow");
    const auto id = static_cast<I>(mesh.vertices.size());
    mesh.vertices.push_back({coordinate(x0, x1, x), coordinate(y0, y1, y),
                             coordinate(z0, z1, z)});
    indices.emplace(k, id);
    return id;
  };
  auto quad = [&](key p0, key p1, key p2, key p3) {
    mesh.faces.push_back({vertex(p0[0], p0[1], p0[2]),
                          vertex(p1[0], p1[1], p1[2]),
                          vertex(p2[0], p2[1], p2[2]),
                          vertex(p3[0], p3[1], p3[2])});
  };

  const auto n = subdivisions;
  for (std::uint32_t u = 0; u < n; ++u)
    for (std::uint32_t v = 0; v < n; ++v) {
      quad({u, v, 0}, {u, v + 1, 0}, {u + 1, v + 1, 0}, {u + 1, v, 0});
      quad({u, v, n}, {u + 1, v, n}, {u + 1, v + 1, n}, {u, v + 1, n});
      quad({u, 0, v}, {u + 1, 0, v}, {u + 1, 0, v + 1}, {u, 0, v + 1});
      quad({n, u, v}, {n, u + 1, v}, {n, u + 1, v + 1}, {n, u, v + 1});
      quad({u + 1, n, v}, {u, n, v}, {u, n, v + 1}, {u + 1, n, v + 1});
      quad({0, u + 1, v}, {0, u, v}, {0, u, v + 1}, {0, u + 1, v + 1});
    }
  return mesh;
}

template <class T, class I>
void append_box(fv_surface_mesh<T, I> &mesh, T x0, T x1, T y0, T y1,
                T z0, T z1, std::uint32_t subdivisions = 1,
                bool reverse = false) {
  const auto box = subdivided_box<T, I>(x0, x1, y0, y1, z0, z1,
                                        subdivisions);
  input_test::append(mesh, box, reverse);
}

template <class T, class I>
performance_fixture<T, I> make_performance_fixture(const std::string &name,
                                                    std::uint32_t size) {
  const auto n = fixture_subdivisions(size);
  performance_fixture<T, I> fixture;

  if (name == "B0") {
    fixture.a = input_test::box<T, I>(T(0), T(1));
    fixture.b = input_test::box<T, I>(T(3), T(4));
  } else if (name == "B1") {
    fixture.a = subdivided_box<T, I>(T(0), T(1), T(0), T(1), T(0), T(1), n);
    fixture.b = subdivided_box<T, I>(T(3), T(4), T(3), T(4), T(3), T(4), n);
  } else if (name == "B2") {
    fixture.a = subdivided_box<T, I>(T(0), T(2), T(0), T(2), T(0), T(2), n);
    fixture.b = subdivided_box<T, I>(T(1), T(3), T(1), T(3), T(1), T(3), n);
  } else if (name == "B3") {
    fixture.a = subdivided_box<T, I>(T(0), T(2), T(0), T(2), T(0), T(2), n);
    fixture.b = subdivided_box<T, I>(T(1), T(3), T(0), T(2), T(0), T(2), n);
  } else if (name == "B4") {
    fixture.a = subdivided_box<T, I>(T(0), T(4), T(0), T(4), T(0), T(4), n);
    fixture.b = subdivided_box<T, I>(T(2), T(6), T(1), T(3), T(1), T(3), n);
  } else if (name == "B5") {
    const T span = T(4 + 3 * (size - 1));
    append_box(fixture.a, T(0), span, T(0), T(4), T(0), T(4), n);
    for (std::uint32_t hole = 0; hole < size; ++hole) {
      const T lo = T(1 + 3 * hole), hi = lo + T(2);
      append_box(fixture.b, lo, hi, T(1), T(3), T(-1), T(5));
    }
  } else if (name == "B6") {
    for (std::uint32_t shell = 0; shell < size; ++shell) {
      const T base = T(3 * shell);
      append_box(fixture.a, base, base + T(2), T(0), T(2), T(0), T(2));
      append_box(fixture.b, base + T(0.5), base + T(1.5), T(0.5), T(1.5),
                 T(0.5), T(1.5));
    }
  } else if (name == "B7") {
    for (std::uint32_t component = 0; component < size + 1; ++component) {
      const T base = T(3 * component);
      append_box(fixture.a, base, base + T(1), T(0), T(1), T(0), T(1));
    }
    append_box(fixture.b, T(0.5), T(1.5), T(0.25), T(0.75), T(0.25), T(0.75), n);
  } else if (name == "B8") {
    const T unit = std::ldexp(T(1), -static_cast<int>(8 + 2 * size));
    const T adjacent = std::nextafter(unit, std::numeric_limits<T>::infinity());
    fixture.a = subdivided_box<T, I>(-unit, unit, -unit, unit, -unit, unit, n);
    fixture.b = subdivided_box<T, I>(T(0), unit + adjacent, -adjacent, adjacent,
                                     -unit, adjacent, n);
  } else {
    throw std::invalid_argument("unknown fixture: " + name);
  }
  return fixture;
}

} // namespace ygor::mesh_boolean::testing
