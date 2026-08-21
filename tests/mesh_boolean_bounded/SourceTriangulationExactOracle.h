#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace source_triangulation_tests {
inline std::uint64_t exact_expected_triangle_count(std::size_t ring_size) {
  return ring_size >= 3 ? static_cast<std::uint64_t>(ring_size - 2) : 0;
}
inline std::uint64_t exact_expected_diagonal_count(std::size_t ring_size) {
  return ring_size >= 3 ? static_cast<std::uint64_t>(ring_size - 3) : 0;
}
} // namespace source_triangulation_tests
