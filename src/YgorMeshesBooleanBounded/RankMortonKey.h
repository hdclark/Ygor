#pragma once

#include "BroadPhasePrimitiveTables.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct rank_morton_build_result final {
  std::array<std::uint64_t, 3> distinct_rank_counts{};
  std::uint16_t active_rank_bits = 0;
  std::vector<std::uint64_t> spatial_order;
};

inline std::uint16_t broad_phase_rank_bit_width(std::uint64_t value) noexcept {
  std::uint16_t width = 0;
  while (value != 0) {
    ++width;
    value >>= 1;
  }
  return width;
}

inline rank_morton_key make_rank_morton_key(
    const std::array<std::uint64_t, 3> &ranks,
    std::uint16_t active_rank_bits) {
  rank_morton_key result;
  result.active_rank_bits = active_rank_bits;
  const std::uint64_t total_bits = static_cast<std::uint64_t>(active_rank_bits) * 3;
  result.words.assign(static_cast<std::size_t>((total_bits + 63) / 64), 0);
  std::uint64_t output_bit = 0;
  for (std::uint16_t remaining = active_rank_bits; remaining != 0; --remaining) {
    const std::uint16_t source_bit = static_cast<std::uint16_t>(remaining - 1);
    for (std::size_t axis = 0; axis < 3; ++axis, ++output_bit) {
      if (((ranks[axis] >> source_bit) & std::uint64_t{1}) == 0)
        continue;
      const auto word = static_cast<std::size_t>(output_bit / 64);
      const auto bit_in_word = static_cast<unsigned>(63 - (output_bit % 64));
      result.words[word] |= (std::uint64_t{1} << bit_in_word);
    }
  }
  return result;
}

template <class T>
bool assign_rank_morton_order(std::vector<broad_phase_triangle_primitive<T>> &triangles,
                              rank_morton_build_result &result,
                              bounded_boolean_error &error) {
  result = rank_morton_build_result{};
  const std::uint64_t count = triangles.size();
  if (count == 0)
    return true;

  std::vector<std::uint64_t> references;
  try {
    references.resize(triangles.size());
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "rank-Morton reference allocation failed",
                              broad_phase_checkpoint::fixed_resource_reservation);
    return false;
  }
  std::iota(references.begin(), references.end(), std::uint64_t{0});

  std::uint64_t maximum_rank = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    std::sort(references.begin(), references.end(),
              [&](std::uint64_t left, std::uint64_t right) {
                const auto &a = triangles[left];
                const auto &b = triangles[right];
                return std::tie(a.axis_keys[axis], a.semantic_key) <
                       std::tie(b.axis_keys[axis], b.semantic_key);
              });
    std::uint64_t rank = 0;
    axis_endpoint_key<T> prior{};
    bool initialized = false;
    for (const auto ordinal : references) {
      const auto key = triangles[ordinal].axis_keys[axis];
      if (initialized && key != prior) {
        if (rank == std::numeric_limits<std::uint64_t>::max()) {
          error = broad_phase_error(broad_phase_subcode::malformed_dense_rank,
                                    bounded_boolean_error_category::index_overflow,
                                    "dense rank overflow",
                                    broad_phase_checkpoint::representability_preflight);
          return false;
        }
        ++rank;
      }
      triangles[ordinal].dense_ranks[axis] = rank;
      prior = key;
      initialized = true;
    }
    result.distinct_rank_counts[axis] = rank + 1;
    maximum_rank = std::max(maximum_rank, rank);
  }

  result.active_rank_bits = broad_phase_rank_bit_width(maximum_rank);
  for (auto &triangle : triangles)
    triangle.morton =
        make_rank_morton_key(triangle.dense_ranks, result.active_rank_bits);

  result.spatial_order = references;
  std::iota(result.spatial_order.begin(), result.spatial_order.end(),
            std::uint64_t{0});
  std::sort(result.spatial_order.begin(), result.spatial_order.end(),
            [&](std::uint64_t left, std::uint64_t right) {
              const auto &a = triangles[left];
              const auto &b = triangles[right];
              return std::tie(a.morton, a.axis_keys[0], a.axis_keys[1],
                              a.axis_keys[2], a.semantic_key) <
                     std::tie(b.morton, b.axis_keys[0], b.axis_keys[1],
                              b.axis_keys[2], b.semantic_key);
            });
  for (std::uint64_t spatial = 0; spatial < result.spatial_order.size(); ++spatial) {
    const auto primitive = result.spatial_order[spatial];
    if (primitive >= triangles.size() ||
        triangles[primitive].spatial_ordinal != broad_phase_invalid_ordinal) {
      error = broad_phase_error(broad_phase_subcode::malformed_spatial_order,
                                bounded_boolean_error_category::internal_invariant_error,
                                "rank-Morton spatial order is not a permutation",
                                broad_phase_checkpoint::operand_a_spatial_order);
      return false;
    }
    triangles[primitive].spatial_ordinal = spatial;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
