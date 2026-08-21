#pragma once

#include "MeshBooleanTestHarness.h"
#include <YgorMath.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <type_traits>

namespace ygor::mesh_boolean::testing {

struct topology_summary {
  std::uint64_t vertices = 0;
  std::uint64_t faces = 0;
  std::uint64_t edges = 0;
  std::uint64_t components = 0;
  std::int64_t euler_characteristic = 0;
};

template <class T, class I>
topology_summary independently_check_closed_mesh(const fv_surface_mesh<T, I> &mesh) {
  static_assert(std::is_floating_point<T>::value, "floating coordinate required");
  static_assert(std::is_unsigned<I>::value, "unsigned index required");
  topology_summary result;
  result.vertices = mesh.vertices.size();
  result.faces = mesh.faces.size();
  if (mesh.faces.empty()) {
    require(mesh.vertices.empty(), "empty boundary has no vertices");
    return result;
  }

  using edge = std::pair<std::uint64_t, std::uint64_t>;
  std::map<edge, std::vector<std::pair<std::uint64_t, std::uint64_t>>> uses;
  std::vector<std::vector<std::uint64_t>> adjacency(mesh.faces.size());
  for (std::size_t f = 0; f < mesh.faces.size(); ++f) {
    const auto &ring = mesh.faces[f];
    require(ring.size() >= 3, "public face has at least three vertices");
    for (std::size_t j = 0; j < ring.size(); ++j) {
      const auto a = static_cast<std::uint64_t>(ring[j]);
      const auto b = static_cast<std::uint64_t>(ring[(j + 1) % ring.size()]);
      require(a < mesh.vertices.size() && b < mesh.vertices.size(),
              "public face index is in range");
      require(a != b, "public edge is non-degenerate");
      uses[{std::min(a, b), std::max(a, b)}].push_back({a, b});
    }
  }
  for (const auto &entry : uses) {
    require(entry.second.size() == 2, "every public edge has two uses");
    require(entry.second[0].first == entry.second[1].second &&
                entry.second[0].second == entry.second[1].first,
            "public edge uses have opposite direction");
  }
  result.edges = uses.size();

  std::map<edge, std::vector<std::size_t>> incident;
  for (std::size_t f = 0; f < mesh.faces.size(); ++f)
    for (std::size_t j = 0; j < mesh.faces[f].size(); ++j) {
      auto a = static_cast<std::uint64_t>(mesh.faces[f][j]);
      auto b = static_cast<std::uint64_t>(mesh.faces[f][(j + 1) % mesh.faces[f].size()]);
      incident[{std::min(a, b), std::max(a, b)}].push_back(f);
    }
  for (const auto &entry : incident) {
    const auto &pair = entry.second;
    adjacency[pair[0]].push_back(pair[1]);
    adjacency[pair[1]].push_back(pair[0]);
  }
  std::vector<bool> seen(mesh.faces.size(), false);
  for (std::size_t start = 0; start < seen.size(); ++start) {
    if (seen[start]) continue;
    ++result.components;
    std::vector<std::size_t> pending{start};
    seen[start] = true;
    while (!pending.empty()) {
      const auto current = pending.back();
      pending.pop_back();
      for (const auto next : adjacency[current])
        if (!seen[next]) {
          seen[next] = true;
          pending.push_back(next);
        }
    }
  }
  result.euler_characteristic = static_cast<std::int64_t>(result.vertices) -
                                static_cast<std::int64_t>(result.edges) +
                                static_cast<std::int64_t>(result.faces);
  return result;
}

} // namespace ygor::mesh_boolean::testing
