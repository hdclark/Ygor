#pragma once

#include "CheckedArithmetic.h"

#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace ygor::mesh_boolean::bounded {
class canonical_writer {
  public:
    void u8(std::uint8_t value) { bytes_.push_back(value); }
    void u16(std::uint16_t value) { integer(value); }
    void u32(std::uint32_t value) { integer(value); }
    void u64(std::uint64_t value) { integer(value); }
    void boolean(bool value) { u8(value ? 1 : 0); }
    template<class T> void floating(T value) {
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
        using U = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
        U bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        integer(bits);
    }
    void long_floating(long double value) {
        static_assert(std::numeric_limits<long double>::radix == 2,
                      "canonical long-double encoding requires binary radix");
        boolean(std::signbit(value));
        value = std::fabs(value);
        if (value == 0) {
            u8(0);
            return;
        }
        if (!std::isfinite(value)) {
            u8(std::isnan(value) ? 3 : 2);
            return;
        }
        u8(1);
        int exponent = 0;
        long double fraction = std::frexp(value, &exponent);
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(exponent)));
        u16(static_cast<std::uint16_t>(std::numeric_limits<long double>::digits));
        std::uint8_t byte = 0;
        for (int bit = 0; bit < std::numeric_limits<long double>::digits; ++bit) {
            fraction = std::ldexp(fraction, 1);
            if (fraction >= 1) {
                byte |= static_cast<std::uint8_t>(1U << (bit % 8));
                fraction -= 1;
            }
            if (bit % 8 == 7 || bit + 1 == std::numeric_limits<long double>::digits) {
                u8(byte);
                byte = 0;
            }
        }
    }
    bool sized_bytes(const std::vector<std::uint8_t> &bytes) {
        std::uint64_t final_size = 0;
        if (!checked_add<std::uint64_t>(bytes_.size(), 8, final_size) ||
            !checked_add<std::uint64_t>(final_size, bytes.size(), final_size)) return false;
        u64(bytes.size());
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        return true;
    }
    const std::vector<std::uint8_t> &bytes() const noexcept { return bytes_; }
    std::vector<std::uint8_t> take() noexcept { return std::move(bytes_); }
  private:
    template<class U> void integer(U value) {
        for (std::size_t i = 0; i < sizeof(U); ++i) {
            bytes_.push_back(static_cast<std::uint8_t>(value & U(0xff)));
            value = static_cast<U>(value >> 8);
        }
    }
    std::vector<std::uint8_t> bytes_;
};

class canonical_reader {
  public:
    explicit canonical_reader(const std::vector<std::uint8_t> &bytes) noexcept : bytes_(bytes) {}
    bool u8(std::uint8_t &out) { return integer(out); }
    bool u16(std::uint16_t &out) { return integer(out); }
    bool u32(std::uint32_t &out) { return integer(out); }
    bool u64(std::uint64_t &out) { return integer(out); }
    bool boolean(bool &out) {
        std::uint8_t value = 0;
        if (!u8(value) || value > 1) return false;
        out = value != 0;
        return true;
    }
    bool fixed_bytes(std::size_t count, std::vector<std::uint8_t> &out) {
        if (bytes_.size() - position_ < count) return false;
        out.assign(bytes_.begin() + position_, bytes_.begin() + position_ + count);
        position_ += count;
        return true;
    }
    bool sized_bytes(std::vector<std::uint8_t> &out, std::uint64_t maximum) {
        std::uint64_t count = 0;
        if (!u64(count) || count > maximum || count > remaining()) return false;
        return fixed_bytes(static_cast<std::size_t>(count), out);
    }
    template<class T> bool floating(T &out) {
        using U = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
        U bits = 0;
        if (!integer(bits)) return false;
        std::memcpy(&out, &bits, sizeof(out));
        return true;
    }
    bool complete() const noexcept { return position_ == bytes_.size(); }
    std::size_t remaining() const noexcept { return bytes_.size() - position_; }
  private:
    template<class U> bool integer(U &out) {
        if (bytes_.size() - position_ < sizeof(U)) return false;
        out = 0;
        for (std::size_t i = 0; i < sizeof(U); ++i) out |= static_cast<U>(bytes_[position_++]) << (8 * i);
        return true;
    }
    const std::vector<std::uint8_t> &bytes_;
    std::size_t position_ = 0;
};
}
