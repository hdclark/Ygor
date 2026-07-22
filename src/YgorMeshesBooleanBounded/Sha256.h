#pragma once

#include "../YgorMeshesBooleanBounded.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {
class sha256 {
  public:
    sha256() noexcept;
    void update(const void *data, std::size_t size) noexcept;
    void update(const std::vector<std::uint8_t> &bytes) noexcept { update(bytes.data(), bytes.size()); }
    bounded_boolean_digest finish() noexcept;
    static bounded_boolean_digest digest(const std::vector<std::uint8_t> &bytes) noexcept;
  private:
    void transform(const std::uint8_t *block) noexcept;
    std::array<std::uint32_t, 8> state_;
    std::array<std::uint8_t, 64> block_{};
    std::uint64_t byte_count_ = 0;
    std::size_t block_size_ = 0;
    bool finished_ = false;
};
}
