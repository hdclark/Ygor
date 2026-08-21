#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ygor::mesh_boolean::testing {

enum class test_tier { continuous, extended, qualification };

struct test_config {
  test_tier tier = test_tier::continuous;
  std::uint64_t seed_high = 0x59474f52424f4f4cULL;
  std::uint64_t seed_low = 0x434f4d5031340001ULL;
  std::size_t generated_cases = 8;
  std::string artifact_directory;
};

test_config load_test_config();
std::uint64_t derive_stream(const test_config &, const std::string &test_id,
                            std::uint64_t ordinal,
                            const std::string &purpose);
std::uint64_t next_random(std::uint64_t &state) noexcept;
const char *to_string(test_tier) noexcept;

} // namespace ygor::mesh_boolean::testing
