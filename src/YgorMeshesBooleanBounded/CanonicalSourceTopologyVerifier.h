#pragma once

#include "CanonicalBytes.h"
#include "ValidatedOperand.h"

#include <algorithm>
#include <map>
#include <optional>
#include <queue>
#include <set>

namespace ygor::mesh_boolean::bounded {
namespace canonical_verifier_detail {
struct candidate {
  std::vector<std::uint8_t> bytes;
  std::vector<std::uint64_t> vertices;
  std::vector<std::vector<std::uint64_t>> rings;
};
inline std::vector<std::uint8_t>
encode(const candidate &c,
       const std::vector<std::array<std::uint64_t, 3>> &labels) {
  canonical_writer w;
  w.u64(c.vertices.size());
  for (auto v : c.vertices)
    for (auto b : labels[v])
      w.u64(b);
  w.u64(c.rings.size());
  for (const auto &r : c.rings) {
    w.u64(r.size());
    for (auto v : r)
      w.u64(v);
  }
  return w.take();
}
inline std::optional<std::pair<std::uint64_t, std::uint64_t>>
opposite_use(const std::vector<std::vector<std::uint64_t>> &rings,
             std::uint64_t facet, std::uint64_t a, std::uint64_t b) {
  for (std::uint64_t f = 0; f < rings.size(); ++f)
    if (f != facet)
      for (std::uint64_t c = 0; c < rings[f].size(); ++c)
        if (rings[f][c] == b && rings[f][(c + 1) % rings[f].size()] == a)
          return std::make_pair(f, c);
  return std::nullopt;
}
inline candidate
component_minimum(const std::vector<std::uint64_t> &component,
                  const std::vector<std::vector<std::uint64_t>> &source_rings,
                  const std::vector<std::array<std::uint64_t, 3>> &labels) {
  candidate best;
  bool have = false;
  for (auto seed : component)
    for (std::uint64_t shift = 0; shift < source_rings[seed].size(); ++shift) {
      candidate current;
      std::map<std::uint64_t, std::uint64_t> ids;
      std::set<std::uint64_t> seen{seed};
      std::vector<std::pair<std::uint64_t, std::uint64_t>> queue{{seed, shift}};
      for (std::size_t q = 0; q < queue.size(); ++q) {
        auto facet = queue[q].first, start = queue[q].second;
        const auto &source_ring = source_rings[facet];
        std::vector<std::uint64_t> ring;
        for (std::uint64_t i = 0; i < source_ring.size(); ++i) {
          auto source_v = source_ring[(start + i) % source_ring.size()];
          auto inserted = ids.emplace(source_v, ids.size());
          if (inserted.second)
            current.vertices.push_back(source_v);
          ring.push_back(inserted.first->second);
        }
        current.rings.push_back(std::move(ring));
        for (std::uint64_t i = 0; i < source_ring.size(); ++i) {
          auto a = source_ring[(start + i) % source_ring.size()],
               b = source_ring[(start + i + 1) % source_ring.size()];
          auto other = opposite_use(source_rings, facet, a, b);
          if (other && seen.insert(other->first).second)
            queue.push_back(*other);
        }
      }
      current.bytes = encode(current, labels);
      if (!have || current.bytes < best.bytes) {
        best = std::move(current);
        have = true;
      }
    }
  return best;
}
} // namespace canonical_verifier_detail

template <class T, class I>
bool independently_verify_canonical_minimum(
    const validated_operand<T, I> &artifact,
    const immutable_source_mesh<T, I> &source) {
  std::vector<std::vector<std::uint64_t>> rings;
  std::vector<std::array<std::uint64_t, 3>> labels(source.vertex_count());
  std::uint64_t corners = 0;
  for (std::size_t v = 0; v < labels.size(); ++v)
    for (std::size_t k = 0; k < 3; ++k)
      labels[v][k] = source.coordinate_bits()[v * 3 + k];
  for (std::size_t f = 0; f < source.face_count(); ++f) {
    std::vector<std::uint64_t> ring;
    for (auto p = source.face_offsets()[f]; p < source.face_offsets()[f + 1];
         ++p) {
      auto v = static_cast<std::uint64_t>(source.indices()[p]);
      if (ring.empty() || ring.back() != v)
        ring.push_back(v);
    }
    if (ring.size() > 1 && ring.front() == ring.back())
      ring.pop_back();
    corners += ring.size();
    rings.push_back(std::move(ring));
  }
  if (corners != artifact.statistics().canonical_branches)
    return false;
  std::vector<bool> seen(rings.size(), false);
  std::vector<canonical_verifier_detail::candidate> components;
  for (std::uint64_t seed = 0; seed < rings.size(); ++seed)
    if (!seen[seed]) {
      std::vector<std::uint64_t> component{seed};
      seen[seed] = true;
      for (std::size_t q = 0; q < component.size(); ++q) {
        auto f = component[q];
        for (std::uint64_t c = 0; c < rings[f].size(); ++c) {
          auto other = canonical_verifier_detail::opposite_use(
              rings, f, rings[f][c], rings[f][(c + 1) % rings[f].size()]);
          if (other && !seen[other->first]) {
            seen[other->first] = true;
            component.push_back(other->first);
          }
        }
      }
      components.push_back(canonical_verifier_detail::component_minimum(
          component, rings, labels));
    }
  std::sort(components.begin(), components.end(),
            [](const auto &a, const auto &b) { return a.bytes < b.bytes; });
  std::vector<std::array<std::uint64_t, 3>> expected_labels;
  std::vector<std::vector<std::uint64_t>> expected_rings;
  std::uint64_t base = 0;
  for (auto &component : components) {
    for (auto source_v : component.vertices)
      expected_labels.push_back(labels[source_v]);
    for (auto ring : component.rings) {
      for (auto &v : ring)
        v += base;
      expected_rings.push_back(std::move(ring));
    }
    base += component.vertices.size();
  }
  if (expected_labels.size() != artifact.vertices().size() ||
      expected_rings.size() != artifact.facets().size())
    return false;
  for (std::size_t v = 0; v < expected_labels.size(); ++v)
    if (expected_labels[v] != artifact.vertices()[v].coordinate_bits)
      return false;
  for (std::size_t f = 0; f < expected_rings.size(); ++f)
    if (expected_rings[f] != artifact.facets()[f].vertices)
      return false;
  return true;
}
} // namespace ygor::mesh_boolean::bounded
