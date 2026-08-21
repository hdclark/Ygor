#pragma once

#include "QualificationTypes.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

// Deterministic SplitMix64 counter-based seed stream (Section 7.2).
//
//   z = root + 0x9E3779B97F4A7C15 * (counter + 1)
//   z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9
//   z = (z ^ (z >> 27)) * 0x94D049BB133111EB
//   result = z ^ (z >> 31)
class deterministic_seed final {
public:
  explicit deterministic_seed(std::uint64_t root) noexcept : root_(root) {}

  std::uint64_t next(std::uint64_t counter) const noexcept {
    std::uint64_t z = root_ + 0x9E3779B97F4A7C15ULL * (counter + 1);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }

  // Rejection-sampling-free bounded integer in [0, limit).
  std::uint64_t bounded(std::uint64_t counter, std::uint64_t limit) const
      noexcept {
    if (limit == 0)
      return 0;
    return next(counter) % limit;
  }

  std::uint64_t root() const noexcept { return root_; }

private:
  std::uint64_t root_;
};

// Derive an independent domain root from a campaign seed and a stable ASCII
// domain tag using a domain-separated SHA-256 word.
inline std::uint64_t derive_seed_root(std::uint64_t campaign_root,
                                      const char *domain) noexcept {
  canonical_writer writer;
  writer.u64(campaign_root);
  for (const char *p = domain; *p; ++p)
    writer.u8(static_cast<std::uint8_t>(*p));
  const auto digest = sha256::digest(writer.bytes());
  std::uint64_t word = 0;
  for (std::size_t i = 0; i < 8; ++i)
    word |= static_cast<std::uint64_t>(digest.bytes[i]) << (8 * i);
  return word;
}

} // namespace ygor::mesh_boolean::bounded
