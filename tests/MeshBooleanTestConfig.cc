#include "MeshBooleanTestConfig.h"

#include <cstdlib>
#include <stdexcept>

namespace ygor::mesh_boolean::testing {
namespace {
std::uint64_t mix(std::uint64_t x) noexcept {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31U);
}

void fold(std::uint64_t &state, const std::string &text) noexcept {
  for (const unsigned char c : text)
    state = mix(state ^ c);
}
} // namespace

test_config load_test_config() {
  test_config result;
  if (const char *tier = std::getenv("YGOR_BOOLEAN_TEST_TIER")) {
    const std::string value(tier);
    if (value == "continuous")
      result.tier = test_tier::continuous;
    else if (value == "extended") {
      result.tier = test_tier::extended;
      result.generated_cases = 32;
    } else if (value == "qualification") {
      result.tier = test_tier::qualification;
      result.generated_cases = 128;
    } else
      throw std::runtime_error("invalid YGOR_BOOLEAN_TEST_TIER");
  }
  if (const char *directory = std::getenv("YGOR_BOOLEAN_ARTIFACT_DIR"))
    result.artifact_directory = directory;
  return result;
}

std::uint64_t derive_stream(const test_config &config,
                            const std::string &test_id,
                            std::uint64_t ordinal,
                            const std::string &purpose) {
  std::uint64_t state = mix(config.seed_high) ^ mix(config.seed_low) ^ mix(ordinal);
  fold(state, test_id);
  fold(state, purpose);
  return mix(state);
}

std::uint64_t next_random(std::uint64_t &state) noexcept {
  state ^= state >> 12U;
  state ^= state << 25U;
  state ^= state >> 27U;
  return state * 0x2545f4914f6cdd1dULL;
}

const char *to_string(test_tier tier) noexcept {
  switch (tier) {
  case test_tier::continuous: return "continuous";
  case test_tier::extended: return "extended";
  case test_tier::qualification: return "qualification";
  }
  return "invalid";
}

} // namespace ygor::mesh_boolean::testing
