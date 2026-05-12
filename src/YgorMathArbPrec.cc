#include "YgorMathArbPrec.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using limbs_t = std::vector<ArbPrec::limb_type>;
constexpr uint64_t limb_base = (uint64_t{1} << 32);

std::atomic<std::size_t> g_default_precision_bits{256};

void trim(limbs_t &limbs){
    while(!limbs.empty() && limbs.back() == 0U){
        limbs.pop_back();
    }
}

bool limbs_are_zero(const limbs_t &limbs){
    return std::all_of(limbs.begin(), limbs.end(), [](auto limb){ return limb == 0U; });
}

int compare(const limbs_t &lhs, const limbs_t &rhs){
    if(lhs.size() != rhs.size()){
        return (lhs.size() < rhs.size()) ? -1 : 1;
    }
    for(std::size_t i = lhs.size(); i-- > 0; ){
        if(lhs[i] != rhs[i]){
            return (lhs[i] < rhs[i]) ? -1 : 1;
        }
    }
    return 0;
}

std::size_t bit_length(const limbs_t &limbs){
    if(limbs.empty()){
        return 0;
    }
    const auto hi = limbs.back();
    std::size_t bits = 32U * (limbs.size() - 1U);
    for(int i = 31; i >= 0; --i){
        if((hi >> i) != 0U){
            return bits + static_cast<std::size_t>(i) + 1U;
        }
    }
    return bits;
}

std::size_t trailing_zero_bits(const limbs_t &limbs){
    std::size_t zeros = 0;
    for(const auto limb : limbs){
        if(limb == 0U){
            zeros += 32U;
            continue;
        }
        for(int i = 0; i < 32; ++i){
            if(((limb >> i) & 1U) != 0U){
                return zeros + static_cast<std::size_t>(i);
            }
        }
    }
    return zeros;
}

bool get_bit(const limbs_t &limbs, std::size_t bit_index){
    const auto limb_index = bit_index / 32U;
    if(limb_index >= limbs.size()){
        return false;
    }
    const auto offset = bit_index % 32U;
    return ((limbs[limb_index] >> offset) & 1U) != 0U;
}

bool any_low_bits_set(const limbs_t &limbs, std::size_t bit_count){
    const auto full_limbs = bit_count / 32U;
    for(std::size_t i = 0; i < std::min(full_limbs, limbs.size()); ++i){
        if(limbs[i] != 0U){
            return true;
        }
    }
    const auto extra_bits = bit_count % 32U;
    if((extra_bits != 0U) && (full_limbs < limbs.size())){
        const auto mask = (uint64_t{1} << extra_bits) - 1U;
        if((limbs[full_limbs] & mask) != 0U){
            return true;
        }
    }
    return false;
}

void add_small(limbs_t &limbs, uint32_t value){
    uint64_t carry = value;
    std::size_t index = 0;
    while(carry != 0U){
        if(index >= limbs.size()){
            limbs.push_back(0U);
        }
        const uint64_t sum = static_cast<uint64_t>(limbs[index]) + carry;
        limbs[index] = static_cast<uint32_t>(sum & 0xFFFFFFFFULL);
        carry = sum >> 32U;
        ++index;
    }
}

void multiply_small(limbs_t &limbs, uint32_t value){
    if(value == 0U || limbs.empty()){
        limbs.clear();
        return;
    }
    uint64_t carry = 0U;
    for(auto &limb : limbs){
        const uint64_t product = static_cast<uint64_t>(limb) * value + carry;
        limb = static_cast<uint32_t>(product & 0xFFFFFFFFULL);
        carry = product >> 32U;
    }
    if(carry != 0U){
        limbs.push_back(static_cast<uint32_t>(carry));
    }
}

limbs_t add(const limbs_t &lhs, const limbs_t &rhs){
    limbs_t out(std::max(lhs.size(), rhs.size()), 0U);
    uint64_t carry = 0U;
    for(std::size_t i = 0; i < out.size(); ++i){
        const uint64_t a = (i < lhs.size()) ? lhs[i] : 0U;
        const uint64_t b = (i < rhs.size()) ? rhs[i] : 0U;
        const uint64_t sum = a + b + carry;
        out[i] = static_cast<uint32_t>(sum & 0xFFFFFFFFULL);
        carry = sum >> 32U;
    }
    if(carry != 0U){
        out.push_back(static_cast<uint32_t>(carry));
    }
    trim(out);
    return out;
}

limbs_t subtract(const limbs_t &lhs, const limbs_t &rhs){
    limbs_t out(lhs);
    int64_t borrow = 0;
    for(std::size_t i = 0; i < out.size(); ++i){
        const int64_t a = out[i];
        const int64_t b = (i < rhs.size()) ? rhs[i] : 0;
        int64_t diff = a - b - borrow;
        if(diff < 0){
            diff += static_cast<int64_t>(limb_base);
            borrow = 1;
        }else{
            borrow = 0;
        }
        out[i] = static_cast<uint32_t>(diff);
    }
    trim(out);
    return out;
}

limbs_t multiply(const limbs_t &lhs, const limbs_t &rhs){
    if(lhs.empty() || rhs.empty()){
        return {};
    }
    limbs_t out(lhs.size() + rhs.size(), 0U);
    for(std::size_t i = 0; i < lhs.size(); ++i){
        uint64_t carry = 0U;
        for(std::size_t j = 0; j < rhs.size(); ++j){
            const uint64_t cur = out[i + j];
            const uint64_t product = static_cast<uint64_t>(lhs[i]) * rhs[j] + cur + carry;
            out[i + j] = static_cast<uint32_t>(product & 0xFFFFFFFFULL);
            carry = product >> 32U;
        }
        std::size_t pos = i + rhs.size();
        while(carry != 0U){
            const uint64_t sum = static_cast<uint64_t>(out[pos]) + carry;
            out[pos] = static_cast<uint32_t>(sum & 0xFFFFFFFFULL);
            carry = sum >> 32U;
            ++pos;
        }
    }
    trim(out);
    return out;
}

limbs_t shift_left(const limbs_t &limbs, std::size_t bits){
    if(limbs.empty() || bits == 0U){
        return limbs;
    }
    const auto limb_shift = bits / 32U;
    const auto bit_shift = bits % 32U;
    limbs_t out(limbs.size() + limb_shift + 1U, 0U);
    uint64_t carry = 0U;
    for(std::size_t i = 0; i < limbs.size(); ++i){
        const uint64_t cur = (static_cast<uint64_t>(limbs[i]) << bit_shift) | carry;
        out[i + limb_shift] = static_cast<uint32_t>(cur & 0xFFFFFFFFULL);
        carry = cur >> 32U;
    }
    out[limbs.size() + limb_shift] = static_cast<uint32_t>(carry);
    trim(out);
    return out;
}

limbs_t shift_right(const limbs_t &limbs, std::size_t bits){
    if(limbs.empty() || bits == 0U){
        return limbs;
    }
    const auto limb_shift = bits / 32U;
    const auto bit_shift = bits % 32U;
    if(limb_shift >= limbs.size()){
        return {};
    }
    limbs_t out(limbs.size() - limb_shift, 0U);
    uint64_t carry = 0U;
    for(std::size_t src = limbs.size(); src-- > limb_shift; ){
        const uint64_t cur = limbs[src];
        const auto dst = src - limb_shift;
        if(bit_shift == 0U){
            out[dst] = static_cast<uint32_t>(cur);
        }else{
            out[dst] = static_cast<uint32_t>((cur >> bit_shift) | (carry << (32U - bit_shift)));
            carry = cur & ((uint64_t{1} << bit_shift) - 1U);
        }
    }
    trim(out);
    return out;
}

void set_bit(limbs_t &limbs, std::size_t bit_index){
    const auto limb_index = bit_index / 32U;
    if(limb_index >= limbs.size()){
        limbs.resize(limb_index + 1U, 0U);
    }
    limbs[limb_index] |= static_cast<uint32_t>(uint32_t{1} << (bit_index % 32U));
}

void divide(const limbs_t &numerator, const limbs_t &denominator, limbs_t &quotient, limbs_t &remainder){
    if(denominator.empty()){
        throw std::domain_error("ArbPrec division by zero");
    }
    if(compare(numerator, denominator) < 0){
        quotient.clear();
        remainder = numerator;
        return;
    }
    remainder = numerator;
    quotient.clear();
    auto shifted_denominator = denominator;
    auto shift = bit_length(numerator) - bit_length(denominator);
    shifted_denominator = shift_left(shifted_denominator, shift);
    for(std::size_t i = shift + 1U; i-- > 0U; ){
        if(compare(remainder, shifted_denominator) >= 0){
            remainder = subtract(remainder, shifted_denominator);
            set_bit(quotient, i);
        }
        if(i == 0U){
            break;
        }
        shifted_denominator = shift_right(shifted_denominator, 1U);
    }
    trim(quotient);
    trim(remainder);
}

std::string limbs_to_hex(const limbs_t &limbs){
    if(limbs.empty()){
        return "0";
    }
    std::ostringstream out;
    out << std::hex << std::nouppercase;
    out << limbs.back();
    for(std::size_t i = limbs.size() - 1U; i-- > 0U; ){
        out << std::setw(8) << std::setfill('0') << limbs[i];
    }
    return out.str();
}

} // namespace

int ArbPrec::compare_abs(const ArbPrec &lhs, const ArbPrec &rhs){
    if(lhs.m_sign == 0 && rhs.m_sign == 0){
        return 0;
    }
    const auto lhs_scale = lhs.m_exponent + static_cast<std::int64_t>(bit_length(lhs.m_limbs));
    const auto rhs_scale = rhs.m_exponent + static_cast<std::int64_t>(bit_length(rhs.m_limbs));
    if(lhs_scale != rhs_scale){
        return (lhs_scale < rhs_scale) ? -1 : 1;
    }
    if(lhs.m_exponent == rhs.m_exponent){
        return compare(lhs.m_limbs, rhs.m_limbs);
    }
    if(lhs.m_exponent < rhs.m_exponent){
        return compare(shift_left(lhs.m_limbs, static_cast<std::size_t>(rhs.m_exponent - lhs.m_exponent)), rhs.m_limbs);
    }
    return compare(lhs.m_limbs, shift_left(rhs.m_limbs, static_cast<std::size_t>(lhs.m_exponent - rhs.m_exponent)));
}

ArbPrec::ArbPrec()
    : m_precision_bits(clamp_precision_bits(0)){
}

ArbPrec::ArbPrec(int value){
    assign_from_signed_integer(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(unsigned int value){
    assign_from_unsigned_integer(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(long value){
    assign_from_signed_integer(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(unsigned long value){
    assign_from_unsigned_integer(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(long long value){
    assign_from_signed_integer(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(unsigned long long value){
    assign_from_unsigned_integer(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(float value){
    assign_from_floating_value(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(double value){
    assign_from_floating_value(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(long double value){
    assign_from_floating_value(value, clamp_precision_bits(0));
}

ArbPrec::ArbPrec(float value, std::size_t precision_bits){
    assign_from_floating_value(value, precision_bits);
}

ArbPrec::ArbPrec(double value, std::size_t precision_bits){
    assign_from_floating_value(value, precision_bits);
}

ArbPrec::ArbPrec(long double value, std::size_t precision_bits){
    assign_from_floating_value(value, precision_bits);
}

ArbPrec::ArbPrec(int sign,
                 std::int64_t exponent,
                 std::vector<limb_type> limbs,
                 std::size_t precision_bits)
    : m_sign(sign),
      m_exponent(exponent),
      m_precision_bits(clamp_precision_bits(precision_bits)),
      m_limbs(std::move(limbs)){
    normalize();
}

void ArbPrec::set_default_precision_bits(std::size_t bits) noexcept{
    g_default_precision_bits.store(clamp_precision_bits(bits));
}

std::size_t ArbPrec::default_precision_bits() noexcept{
    return clamp_precision_bits(g_default_precision_bits.load());
}

std::size_t ArbPrec::precision_bits() const noexcept{
    return m_precision_bits;
}

void ArbPrec::set_precision_bits(std::size_t bits){
    m_precision_bits = clamp_precision_bits(bits);
    normalize();
}

ArbPrec ArbPrec::with_precision_bits(std::size_t bits) const{
    ArbPrec out(*this);
    out.set_precision_bits(bits);
    return out;
}

bool ArbPrec::is_zero() const noexcept{
    return m_sign == 0;
}

bool ArbPrec::is_negative() const noexcept{
    return m_sign < 0;
}

std::string ArbPrec::to_hex_string() const{
    if(m_sign == 0){
        return "0x0p+0";
    }
    std::ostringstream out;
    if(m_sign < 0){
        out << '-';
    }
    out << "0x" << limbs_to_hex(m_limbs) << 'p';
    if(m_exponent >= 0){
        out << '+';
    }
    out << m_exponent;
    return out.str();
}

ArbPrec::operator float() const{
    return static_cast<float>(static_cast<long double>(*this));
}

ArbPrec::operator double() const{
    return static_cast<double>(static_cast<long double>(*this));
}

ArbPrec::operator long double() const{
    if(m_sign == 0){
        return 0.0L;
    }
    long double out = 0.0L;
    for(std::size_t i = m_limbs.size(); i-- > 0U; ){
        out = std::ldexp(out, 32);
        out += static_cast<long double>(m_limbs[i]);
    }
    out = std::ldexp(out, static_cast<int>(m_exponent));
    return (m_sign < 0) ? -out : out;
}

ArbPrec ArbPrec::operator+() const{
    return *this;
}

ArbPrec ArbPrec::operator-() const{
    ArbPrec out(*this);
    out.m_sign = -out.m_sign;
    return out;
}

ArbPrec &ArbPrec::operator+=(const ArbPrec &rhs){
    const auto target_precision = std::max(m_precision_bits, rhs.m_precision_bits);
    if(rhs.m_sign == 0){
        m_precision_bits = target_precision;
        return *this;
    }
    if(m_sign == 0){
        *this = rhs;
        m_precision_bits = target_precision;
        return *this;
    }

    const auto common_exponent = std::min(m_exponent, rhs.m_exponent);
    const auto lhs_limbs = shift_left(m_limbs, static_cast<std::size_t>(m_exponent - common_exponent));
    const auto rhs_limbs = shift_left(rhs.m_limbs, static_cast<std::size_t>(rhs.m_exponent - common_exponent));

    limbs_t out_limbs;
    int out_sign = 0;
    if(m_sign == rhs.m_sign){
        out_limbs = add(lhs_limbs, rhs_limbs);
        out_sign = m_sign;
    }else{
        const auto cmp = compare(lhs_limbs, rhs_limbs);
        if(cmp == 0){
            m_sign = 0;
            m_exponent = 0;
            m_limbs.clear();
            m_precision_bits = target_precision;
            return *this;
        }
        if(cmp > 0){
            out_limbs = subtract(lhs_limbs, rhs_limbs);
            out_sign = m_sign;
        }else{
            out_limbs = subtract(rhs_limbs, lhs_limbs);
            out_sign = rhs.m_sign;
        }
    }

    m_sign = out_sign;
    m_exponent = common_exponent;
    m_precision_bits = target_precision;
    m_limbs = std::move(out_limbs);
    normalize();
    return *this;
}

ArbPrec &ArbPrec::operator-=(const ArbPrec &rhs){
    return (*this += (-rhs));
}

ArbPrec &ArbPrec::operator*=(const ArbPrec &rhs){
    const auto target_precision = std::max(m_precision_bits, rhs.m_precision_bits);
    if(m_sign == 0 || rhs.m_sign == 0){
        m_sign = 0;
        m_exponent = 0;
        m_limbs.clear();
        m_precision_bits = target_precision;
        return *this;
    }
    m_sign *= rhs.m_sign;
    m_exponent += rhs.m_exponent;
    m_precision_bits = target_precision;
    m_limbs = multiply(m_limbs, rhs.m_limbs);
    normalize();
    return *this;
}

ArbPrec &ArbPrec::operator/=(const ArbPrec &rhs){
    if(rhs.m_sign == 0){
        throw std::domain_error("ArbPrec division by zero");
    }
    const auto target_precision = std::max(m_precision_bits, rhs.m_precision_bits);
    if(m_sign == 0){
        m_precision_bits = target_precision;
        return *this;
    }

    constexpr std::size_t guard_bits = 8U;
    const auto shift = target_precision + guard_bits;
    limbs_t quotient;
    limbs_t remainder;
    divide(shift_left(m_limbs, shift), rhs.m_limbs, quotient, remainder);

    m_sign *= rhs.m_sign;
    m_exponent = m_exponent - rhs.m_exponent - static_cast<std::int64_t>(shift);
    m_precision_bits = target_precision;
    m_limbs = std::move(quotient);
    normalize();
    return *this;
}

bool operator==(const ArbPrec &lhs, const ArbPrec &rhs) noexcept{
    return (lhs.m_sign == rhs.m_sign)
        && (lhs.m_exponent == rhs.m_exponent)
        && (lhs.m_limbs == rhs.m_limbs);
}

bool operator!=(const ArbPrec &lhs, const ArbPrec &rhs) noexcept{
    return !(lhs == rhs);
}

bool operator<(const ArbPrec &lhs, const ArbPrec &rhs){
    if(lhs.m_sign != rhs.m_sign){
        return lhs.m_sign < rhs.m_sign;
    }
    if(lhs.m_sign == 0){
        return false;
    }
    const auto cmp = ArbPrec::compare_abs(lhs, rhs);
    return (lhs.m_sign > 0) ? (cmp < 0) : (cmp > 0);
}

bool operator<=(const ArbPrec &lhs, const ArbPrec &rhs){
    return !(rhs < lhs);
}

bool operator>(const ArbPrec &lhs, const ArbPrec &rhs){
    return rhs < lhs;
}

bool operator>=(const ArbPrec &lhs, const ArbPrec &rhs){
    return !(lhs < rhs);
}

std::ostream &operator<<(std::ostream &out, const ArbPrec &value){
    out << value.to_hex_string();
    return out;
}

std::istream &operator>>(std::istream &in, ArbPrec &value){
    std::string token;
    in >> token;
    if(token.empty()){
        return in;
    }
    if((token.find("0x") != std::string::npos) || (token.find('p') != std::string::npos) || (token.find('P') != std::string::npos)){
        value.assign_from_hex_string(token, value.precision_bits());
    }else{
        value = ArbPrec(std::stold(token), value.precision_bits());
    }
    return in;
}

ArbPrec ArbPrec::epsilon_value(std::size_t precision_bits){
    const auto bits = clamp_precision_bits(precision_bits);
    return power_of_two(1 - static_cast<std::int64_t>(bits), bits);
}

ArbPrec ArbPrec::min_value(std::size_t precision_bits){
    const auto bits = clamp_precision_bits(precision_bits);
    return power_of_two(-1048576, bits);
}

ArbPrec ArbPrec::max_value(std::size_t precision_bits){
    const auto bits = clamp_precision_bits(precision_bits);
    return power_of_two(1048576, bits);
}

ArbPrec ArbPrec::lowest_value(std::size_t precision_bits){
    return -max_value(precision_bits);
}

ArbPrec ArbPrec::power_of_two(std::int64_t exponent, std::size_t precision_bits){
    return ArbPrec(1, exponent, limbs_t{1U}, precision_bits);
}

std::size_t ArbPrec::clamp_precision_bits(std::size_t bits) noexcept{
    if(bits == 0U){
        bits = g_default_precision_bits.load();
    }
    return std::max<std::size_t>(bits, 16U);
}

void ArbPrec::normalize(){
    trim(m_limbs);
    if(m_limbs.empty() || limbs_are_zero(m_limbs)){
        m_sign = 0;
        m_exponent = 0;
        m_limbs.clear();
        return;
    }

    const auto tz = trailing_zero_bits(m_limbs);
    if(tz != 0U){
        m_limbs = shift_right(m_limbs, tz);
        m_exponent += static_cast<std::int64_t>(tz);
    }

    auto bits = bit_length(m_limbs);
    if(bits > m_precision_bits){
        const auto remove_bits = bits - m_precision_bits;
        const auto round_bit = get_bit(m_limbs, remove_bits - 1U);
        const auto sticky = (remove_bits > 1U) && any_low_bits_set(m_limbs, remove_bits - 1U);
        m_limbs = shift_right(m_limbs, remove_bits);
        if(round_bit && (sticky || get_bit(m_limbs, 0U))){
            add_small(m_limbs, 1U);
        }
        m_exponent += static_cast<std::int64_t>(remove_bits);
        bits = bit_length(m_limbs);
        if(bits > m_precision_bits){
            m_limbs = shift_right(m_limbs, 1U);
            ++m_exponent;
        }
    }

    trim(m_limbs);
    if(m_limbs.empty()){
        m_sign = 0;
        m_exponent = 0;
    }
}

template <class Float>
void ArbPrec::assign_from_floating_value(Float value, std::size_t precision_bits){
    if(!std::isfinite(static_cast<long double>(value))){
        throw std::invalid_argument("ArbPrec requires finite floating-point input");
    }
    std::ostringstream repr;
    repr << std::hexfloat << std::setprecision(std::numeric_limits<Float>::max_digits10) << value;
    assign_from_hex_string(repr.str(), precision_bits);
}

template void ArbPrec::assign_from_floating_value(float value, std::size_t precision_bits);
template void ArbPrec::assign_from_floating_value(double value, std::size_t precision_bits);
template void ArbPrec::assign_from_floating_value(long double value, std::size_t precision_bits);

void ArbPrec::assign_from_signed_integer(long long value, std::size_t precision_bits){
    const auto bits = clamp_precision_bits(precision_bits);
    if(value == 0){
        m_sign = 0;
        m_exponent = 0;
        m_limbs.clear();
        m_precision_bits = bits;
        return;
    }
    const auto mag = (value < 0) ? static_cast<unsigned long long>(-(value + 1LL)) + 1ULL
                                 : static_cast<unsigned long long>(value);
    assign_from_unsigned_integer(mag, bits);
    if(value < 0){
        m_sign = -1;
    }
}

void ArbPrec::assign_from_unsigned_integer(unsigned long long value, std::size_t precision_bits){
    m_sign = (value == 0ULL) ? 0 : 1;
    m_exponent = 0;
    m_precision_bits = clamp_precision_bits(precision_bits);
    m_limbs.clear();
    if(value == 0ULL){
        return;
    }
    while(value != 0ULL){
        m_limbs.push_back(static_cast<uint32_t>(value & 0xFFFFFFFFULL));
        value >>= 32U;
    }
    normalize();
}

void ArbPrec::assign_from_hex_string(const std::string &repr, std::size_t precision_bits){
    std::string s;
    s.reserve(repr.size());
    for(const auto ch : repr){
        if(!std::isspace(static_cast<unsigned char>(ch))){
            s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }

    m_precision_bits = clamp_precision_bits(precision_bits);
    m_sign = 1;
    m_exponent = 0;
    m_limbs.clear();

    if(s.empty()){
        m_sign = 0;
        return;
    }
    if(s.front() == '+' || s.front() == '-'){
        if(s.front() == '-'){
            m_sign = -1;
        }
        s.erase(s.begin());
    }
    const auto p_pos = s.find('p');
    if(p_pos == std::string::npos){
        throw std::invalid_argument("ArbPrec expected a hexadecimal floating-point string");
    }
    auto mantissa = s.substr(0, p_pos);
    const auto exponent_part = s.substr(p_pos + 1U);
    if(mantissa.rfind("0x", 0U) == 0U){
        mantissa.erase(0U, 2U);
    }
    const auto dot_pos = mantissa.find('.');
    std::string int_part = mantissa;
    std::string frac_part;
    if(dot_pos != std::string::npos){
        int_part = mantissa.substr(0U, dot_pos);
        frac_part = mantissa.substr(dot_pos + 1U);
    }
    std::string digits = int_part + frac_part;
    while(!digits.empty() && digits.front() == '0'){
        digits.erase(digits.begin());
    }
    if(digits.empty()){
        m_sign = 0;
        m_exponent = 0;
        return;
    }

    for(const auto ch : digits){
        multiply_small(m_limbs, 16U);
        if(ch >= '0' && ch <= '9'){
            add_small(m_limbs, static_cast<uint32_t>(ch - '0'));
        }else if(ch >= 'a' && ch <= 'f'){
            add_small(m_limbs, static_cast<uint32_t>(10 + (ch - 'a')));
        }else{
            throw std::invalid_argument("ArbPrec encountered an invalid hexadecimal digit");
        }
    }

    m_exponent = std::stoll(exponent_part) - static_cast<std::int64_t>(4U * frac_part.size());
    normalize();
}

ArbPrec abs(const ArbPrec &value){
    ArbPrec out(value);
    if(out.m_sign < 0){
        out.m_sign = -out.m_sign;
    }
    return out;
}

ArbPrec fabs(const ArbPrec &value){
    return abs(value);
}

ArbPrec sqrt(const ArbPrec &value){
    if(value.m_sign < 0){
        throw std::domain_error("ArbPrec square root requires a non-negative input");
    }
    if(value.m_sign == 0){
        return ArbPrec(0, 0, {}, value.m_precision_bits);
    }

    const auto work_precision = value.m_precision_bits + 16U;
    ArbPrec working = value.with_precision_bits(work_precision);

    ArbPrec estimate;
    const auto approx = static_cast<long double>(value);
    if(std::isfinite(approx) && (approx > 0.0L)){
        estimate = ArbPrec(std::sqrt(approx), work_precision);
    }else{
        const auto highest_bit = value.m_exponent + static_cast<std::int64_t>(bit_length(value.m_limbs)) - 1;
        estimate = ArbPrec::power_of_two(highest_bit / 2, work_precision);
    }
    if(estimate.m_sign == 0){
        estimate = ArbPrec(1.0L, work_precision);
    }

    const auto iterations = std::max<int>(6, static_cast<int>(std::log2(static_cast<double>(work_precision))) + 4);
    for(int i = 0; i < iterations; ++i){
        estimate = (estimate + (working / estimate)) / ArbPrec(2.0L, work_precision);
    }
    estimate.set_precision_bits(value.m_precision_bits);
    return estimate;
}

bool isfinite(const ArbPrec &) noexcept{
    return true;
}
