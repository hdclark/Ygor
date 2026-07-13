#pragma once
#ifndef YGOR_MESHES_EXACT_ARITHMETIC_H_
#define YGOR_MESHES_EXACT_ARITHMETIC_H_

#include "YgorMeshesBooleanContract.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ygor { namespace mesh_boolean {

enum class integer_sign : std::int8_t { negative=-1, zero=0, positive=1 };
enum class exact_sign : std::int8_t { negative=-1, zero=0, positive=1 };

class big_uint {
    std::vector<std::uint32_t> limbs_; // Little endian, canonical.
    void normalize() noexcept;
public:
    big_uint() = default;
    explicit big_uint(std::uint64_t);
    static status_or<big_uint> from_hex(const std::string&, boolean_stage=boolean_stage::intersection_events);
    bool is_zero() const noexcept { return limbs_.empty(); }
    std::size_t limb_count() const noexcept { return limbs_.size(); }
    std::size_t bit_length() const noexcept;
    std::size_t trailing_zero_bits() const noexcept;
    bool is_power_of_two() const noexcept;
    int compare(const big_uint&) const noexcept;
    bool operator==(const big_uint&o)const noexcept{return limbs_==o.limbs_;}
    bool operator!=(const big_uint&o)const noexcept{return !(*this==o);}
    bool operator<(const big_uint&o)const noexcept{return compare(o)<0;}
    std::uint64_t to_uint64(bool *fits=nullptr) const noexcept;
    std::string to_hex() const;
    std::vector<std::uint8_t> canonical_bytes() const;
    void encode(canonical_encoder&) const;
    big_uint shifted_left(std::size_t) const;
    big_uint shifted_right(std::size_t) const;
    friend big_uint operator+(const big_uint&,const big_uint&);
    friend big_uint operator-(const big_uint&,const big_uint&); // Throws if rhs > lhs.
    friend big_uint operator*(const big_uint&,const big_uint&);
    friend std::pair<big_uint,big_uint> divide(const big_uint&,const big_uint&);
    friend big_uint gcd(big_uint,big_uint);
};

class big_int {
    integer_sign sign_=integer_sign::zero;
    big_uint magnitude_;
public:
    big_int()=default;
    explicit big_int(std::int64_t);
    big_int(integer_sign,big_uint);
    integer_sign sign()const noexcept{return sign_;}
    exact_sign exact_signum()const noexcept{return static_cast<exact_sign>(sign_);}
    const big_uint&magnitude()const noexcept{return magnitude_;}
    bool is_zero()const noexcept{return sign_==integer_sign::zero;}
    int compare(const big_int&)const noexcept;
    big_int negated()const;
    std::string to_string()const;
    void encode(canonical_encoder&)const;
    friend bool operator==(const big_int&a,const big_int&b)noexcept{return a.sign_==b.sign_&&a.magnitude_==b.magnitude_;}
    friend bool operator!=(const big_int&a,const big_int&b)noexcept{return !(a==b);}
    friend big_int operator+(const big_int&,const big_int&);
    friend big_int operator-(const big_int&,const big_int&);
    friend big_int operator*(const big_int&,const big_int&);
    friend std::pair<big_int,big_int> divide(const big_int&,const big_int&); // Truncates toward zero.
};

class exact_rational {
    big_int numerator_;
    big_uint denominator_{1};
    void normalize();
public:
    exact_rational()=default;
    explicit exact_rational(std::int64_t n):numerator_(n){}
    exact_rational(big_int,big_uint);
    const big_int&numerator()const noexcept{return numerator_;}
    const big_uint&denominator()const noexcept{return denominator_;}
    exact_sign sign()const noexcept{return numerator_.exact_signum();}
    bool is_zero()const noexcept{return numerator_.is_zero();}
    bool is_integer()const noexcept{return denominator_==big_uint(1);}
    int compare(const exact_rational&)const;
    exact_rational negated()const{return exact_rational(numerator_.negated(),denominator_);}
    exact_rational abs()const{return sign()==exact_sign::negative?negated():*this;}
    exact_rational pow(std::uint64_t)const;
    big_int trunc()const; big_int floor()const; big_int ceil()const;
    std::string to_string()const;
    void encode(canonical_encoder&)const;
    std::vector<std::uint8_t> canonical_bytes()const;
    digest canonical_hash()const;
    friend bool operator==(const exact_rational&a,const exact_rational&b)noexcept{return a.numerator_==b.numerator_&&a.denominator_==b.denominator_;}
    friend bool operator!=(const exact_rational&a,const exact_rational&b)noexcept{return !(a==b);}
    friend bool operator<(const exact_rational&a,const exact_rational&b){return a.compare(b)<0;}
    friend exact_rational operator+(const exact_rational&,const exact_rational&);
    friend exact_rational operator-(const exact_rational&,const exact_rational&);
    friend exact_rational operator*(const exact_rational&,const exact_rational&);
    friend exact_rational operator/(const exact_rational&,const exact_rational&);
};
using exact_scalar=exact_rational;

enum class coordinate_category:std::uint8_t{positive_zero,negative_zero,normal,subnormal,infinity,nan};
template<class T>struct coordinate_bits;
template<>struct coordinate_bits<float>{std::uint32_t bits=0;};
template<>struct coordinate_bits<double>{std::uint64_t bits=0;};
template<class T>struct decoded_coordinate{coordinate_bits<T> source;coordinate_category category;bool negative=false;std::uint64_t significand=0;std::int32_t exponent2=0;exact_rational value;};

template<class T> coordinate_bits<T> bits_of(T) noexcept;
template<class T> T value_of_bits(coordinate_bits<T>) noexcept;
template<class T> coordinate_category classify_coordinate_bits(coordinate_bits<T>) noexcept;
template<class T> bool is_finite_coordinate_bits(coordinate_bits<T>) noexcept;
template<class T> status_or<decoded_coordinate<T>> decode_coordinate(coordinate_bits<T>,boolean_stage=boolean_stage::input_validation);
template<class T> status_or<decoded_coordinate<T>> decode_coordinate(T v,boolean_stage s=boolean_stage::input_validation){return decode_coordinate(bits_of(v),s);}
template<class T> std::optional<coordinate_bits<T>> round_binary_nearest_even(const exact_rational&);
template<class T> std::optional<coordinate_bits<T>> round_binary_down(const exact_rational&);
template<class T> std::optional<coordinate_bits<T>> round_binary_up(const exact_rational&);
template<class T> std::optional<coordinate_bits<T>> predecessor_bits(coordinate_bits<T>) noexcept;
template<class T> std::optional<coordinate_bits<T>> successor_bits(coordinate_bits<T>) noexcept;
template<class T> std::optional<int> compare_binary_bits(coordinate_bits<T>,const exact_rational&);
template<class T> std::optional<std::vector<coordinate_bits<T>>> finite_neighboring_bits(coordinate_bits<T>,std::uint32_t radius);

void encode(canonical_encoder&,const big_uint&); void encode(canonical_encoder&,const big_int&); void encode(canonical_encoder&,const exact_rational&);

} }
namespace std {
template<>struct hash<ygor::mesh_boolean::big_uint>{size_t operator()(const ygor::mesh_boolean::big_uint&v)const noexcept;};
template<>struct hash<ygor::mesh_boolean::big_int>{size_t operator()(const ygor::mesh_boolean::big_int&v)const noexcept;};
template<>struct hash<ygor::mesh_boolean::exact_rational>{size_t operator()(const ygor::mesh_boolean::exact_rational&v)const noexcept;};
}
#endif
