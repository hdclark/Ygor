#include "MeshBooleanExactArithmeticFixtures.h"
#include <iostream>
using namespace exact_test;
static void integers(){auto a=big_uint::from_hex("ffffffffffffffffffffffff").value(),b=big_uint::from_hex("100000001").value();auto qr=divide(a,b);require(qr.first*b+qr.second==a,"division identity");require(qr.second<b,"remainder bound");require(gcd(big_uint(48),big_uint(18))==big_uint(6),"gcd");require((big_int(-7)+big_int(12))==big_int(5),"signed add");require(divide(big_int(-17),big_int(5)).first==big_int(-3),"truncation");}
static void rationals(){auto a=q(2,4),b=q(-3,9);require(a==q(1,2)&&b==q(-1,3),"normalization");require(a+b==q(1,6),"addition");require(a*b==q(-1,6),"multiplication");require(a/b==q(-3,2),"division");require(q(-3,2).floor()==big_int(-2)&&q(-3,2).ceil()==big_int(-1),"rounding");require(a.canonical_bytes()==q(1,2).canonical_bytes(),"canonical encoding");}
static void limb_scale_vectors(){
  using namespace exact_arithmetic_test;
  for(const std::size_t limbs:{1U,2U,8U,32U}){
    const auto a=patterned_operand(limbs,1),b=patterned_operand(limbs,2);
    const auto sum=a+b;
    require(sum-a==b&&sum-b==a,"limb-scale addition/subtraction");
    require((a*b)==(b*a),"limb-scale multiplication commutativity");
    require(divide(a*b,a).first==b&&divide(a*b,a).second.is_zero(),"limb-scale exact product division");
  }
  for(const std::size_t limbs:{1U,2U,8U,32U}){
    const auto divisor=patterned_operand(limbs,3);
    const auto quotient=patterned_operand(std::max<std::size_t>(1,limbs/2),4);
    const auto remainder=divisor-big_uint(1);
    const auto dividend=quotient*divisor+remainder;
    const auto actual=divide(dividend,divisor),reference=reference_divide(dividend,divisor);
    require(actual==reference,"bounded differential division");
    require(actual.first*divisor+actual.second==dividend,"quotient/remainder identity");
    require(actual.second<divisor,"quotient/remainder bound");
  }
  const std::vector<std::pair<std::string,std::string>> correction_vectors{
    {"8000000000000000ffffffffffffffff","80000000ffffffff"},
    {"fffffffffffffffeffffffff00000001","ffffffff00000001"},
    {"80000000000000010000000000000000","8000000000000001"}};
  for(const auto&v:correction_vectors){
    const auto a=from_hex(v.first),b=from_hex(v.second);
    require(divide(a,b)==reference_divide(a,b),"adversarial quotient estimate vector");
  }
  prng random;
  for(std::size_t i=0;i<24;++i){
    const auto limbs=1U+static_cast<std::size_t>(random.next()%8U);
    const auto a=patterned_operand(limbs,static_cast<std::uint32_t>(random.next()));
    const auto b=patterned_operand(1U+static_cast<std::size_t>(random.next()%limbs),static_cast<std::uint32_t>(random.next()));
    const auto actual=divide(a,b);
    require(actual==reference_divide(a,b),"deterministic generated division oracle");
    require(actual.first*b+actual.second==a&&actual.second<b,"deterministic generated division invariant");
  }
}
static void gcd_reference_vectors(){
  using namespace exact_arithmetic_test;
  for(const std::size_t limbs:{1U,2U,8U,32U}){
    const auto common=big_uint(45).shifted_left(limbs*3);
    const auto a=patterned_operand(limbs,5)*common,b=patterned_operand(limbs,6)*common;
    const auto actual=gcd(a,b),reference=reference_gcd(a,b);
    require(actual==reference,"bounded differential gcd");
    require(divide(a,actual).second.is_zero()&&divide(b,actual).second.is_zero(),"gcd exact divisibility");
    require(actual==gcd(b,a),"gcd symmetry");
  }
  require(gcd(big_uint(),big_uint(17))==big_uint(17),"gcd zero canonical form");
  require(gcd(big_uint(23),big_uint(23))==big_uint(23),"gcd equal canonical form");
}
static void rational_reference_vectors(){
  using namespace exact_arithmetic_test;
  const std::vector<std::pair<small_rational,small_rational>> values{
    {{2,4},{-3,9}},{{12345,32768},{-6789,65536}},
    {{99991,99989},{99989,99991}},{{1048575,1048576},{-1048577,1048576}}};
  for(const auto&entry:values){
    const auto a=exact(entry.first),b=exact(entry.second);
    require(a+b==exact(add(entry.first,entry.second)),"rational reference addition");
    require(a-b==exact(add(entry.first,{-entry.second.numerator,entry.second.denominator})),"rational reference subtraction");
    require(a*b==exact(multiply(entry.first,entry.second)),"rational reference multiplication");
    require(a/b==exact(exact_arithmetic_test::divide(entry.first,entry.second)),"rational reference division");
    require(a.compare(b)==-b.compare(a),"rational comparison antisymmetry");
    require(a.denominator()!=big_uint()&&gcd(a.numerator().magnitude(),a.denominator())==big_uint(1),"rational canonical reduction");
  }
  const auto dyadic_a=exact_rational(big_int(integer_sign::positive,patterned_operand(8,7)),big_uint(1).shifted_left(257));
  const auto dyadic_b=exact_rational(big_int(integer_sign::negative,patterned_operand(8,8)),big_uint(1).shifted_left(193));
  require((dyadic_a+dyadic_b)-dyadic_b==dyadic_a,"large dyadic cancellation");
  require((dyadic_a*dyadic_b)/dyadic_b==dyadic_a,"large rational multiply/divide cancellation");
  require(q(0).numerator()==big_int()&&q(0).denominator()==big_uint(1),"zero rational canonical form");
}
static void canonical_serialization_vectors(){
  const std::vector<std::uint8_t> positive_half{1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,2};
  const std::vector<std::uint8_t> negative_half{255,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,2};
  const std::vector<std::uint8_t> zero{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1};
  require(q(1,2).canonical_bytes()==positive_half,"positive rational canonical bytes");
  require(q(-1,2).canonical_bytes()==negative_half,"negative rational canonical bytes");
  require(q(0,99).canonical_bytes()==zero,"zero rational canonical bytes");
  require(q(2,4).canonical_hash()==q(1,2).canonical_hash(),"canonical rational hash");
  require(big_uint::from_hex("0100").value().canonical_bytes()==std::vector<std::uint8_t>({1,0}),"unsigned canonical bytes");
  for(const auto&value:{q(0),q(1,2),q(-1,2),q(1234567,65536),q(-99991,99989)})
    require(exact_arithmetic_test::decode_rational(value.canonical_bytes())==value,"canonical rational round trip");
}
static void decoding(){auto pz=decode_coordinate(0.0).value(),nz=decode_coordinate(-0.0).value();require(pz.value==nz.value&&pz.category!=nz.category,"signed zero");require(decode_coordinate(from_bits<float>(1U)).value().value==exact_rational(big_int(1),big_uint(1).shifted_left(149)),"float subnormal");require(decode_coordinate(from_bits<double>(1ULL)).value().value==exact_rational(big_int(1),big_uint(1).shifted_left(1074)),"double subnormal");require(!decode_coordinate(from_bits<double>(0x7ff0000000000000ULL)).has_value(),"infinity rejected");require(!decode_coordinate(from_bits<float>(0x7fc00001U)).has_value(),"nan rejected");}
static exact_rational dyadic(std::uint64_t n,int exponent,bool negative=false){big_int v(negative?integer_sign::negative:integer_sign::positive,big_uint(n));return exponent>=0?exact_rational(big_int(v.sign(),v.magnitude().shifted_left(exponent)),big_uint(1)):exact_rational(v,big_uint(1).shifted_left(-exponent));}
template<class T,class U>static void require_bits(const std::optional<coordinate_bits<T>>&v,U expected,const char*message){require(v.has_value()&&v->bits==expected,message);}
static void binary_classification(){require(classify_coordinate_bits(coordinate_bits<float>{0})==coordinate_category::positive_zero,"classify +0");require(classify_coordinate_bits(coordinate_bits<float>{0x80000000U})==coordinate_category::negative_zero,"classify -0");require(classify_coordinate_bits(coordinate_bits<float>{1})==coordinate_category::subnormal,"classify subnormal");require(classify_coordinate_bits(coordinate_bits<float>{0x00800000U})==coordinate_category::normal,"classify normal");require(!is_finite_coordinate_bits(coordinate_bits<double>{0x7ff0000000000000ULL}),"infinity non-finite");require(!is_finite_coordinate_bits(coordinate_bits<double>{0x7ff0000000000001ULL}),"nan non-finite");}
static void float_conversion(){const auto half_min=dyadic(1,-150),three_halves=dyadic(3,-150),normal_tie=dyadic((std::uint64_t(1)<<24)-1,-150),one_tie=dyadic((std::uint64_t(1)<<24)+1,-24);require_bits(round_binary_nearest_even<float>(q(0)),0U,"float exact zero");require_bits(round_binary_nearest_even<float>(half_min),0U,"float half-min tie");require_bits(round_binary_nearest_even<float>(half_min.negated()),0x80000000U,"float negative half-min tie");require_bits(round_binary_down<float>(half_min),0U,"float tiny down");require_bits(round_binary_up<float>(half_min),1U,"float tiny up");require_bits(round_binary_down<float>(half_min.negated()),0x80000001U,"float negative tiny down");require_bits(round_binary_up<float>(half_min.negated()),0x80000000U,"float negative tiny up");require_bits(round_binary_nearest_even<float>(three_halves),2U,"float subnormal even tie");require_bits(round_binary_nearest_even<float>(three_halves.negated()),0x80000002U,"float negative subnormal even tie");require_bits(round_binary_nearest_even<float>(normal_tie),0x00800000U,"float normal boundary tie");require_bits(round_binary_nearest_even<float>(normal_tie.negated()),0x80800000U,"float negative normal boundary tie");require_bits(round_binary_nearest_even<float>(one_tie),0x3f800000U,"float one tie");require_bits(round_binary_nearest_even<float>(one_tie.negated()),0xbf800000U,"float negative one tie");const std::uint64_t max_sig=(std::uint64_t(1)<<24)-1;auto max_quarter=dyadic(max_sig*4+1,127-23-2),max_mid=dyadic(max_sig*2+1,127-23-1);require_bits(round_binary_nearest_even<float>(max_quarter),0x7f7fffffU,"float maximum rounds finite");require(!round_binary_nearest_even<float>(max_mid),"float overflow tie");require_bits(round_binary_down<float>(max_mid),0x7f7fffffU,"float overflow down");require(!round_binary_up<float>(max_mid),"float overflow up");require(!round_binary_down<float>(max_mid.negated()),"float negative overflow down");require_bits(round_binary_up<float>(max_mid.negated()),0xff7fffffU,"float negative overflow up");}
static void double_conversion(){const auto half_min=dyadic(1,-1075),normal_tie=dyadic((std::uint64_t(1)<<53)-1,-1075),one_tie=dyadic((std::uint64_t(1)<<53)+1,-53);require_bits(round_binary_nearest_even<double>(half_min),0ULL,"double half-min tie");require_bits(round_binary_up<double>(half_min),1ULL,"double minimum subnormal");require_bits(round_binary_nearest_even<double>(normal_tie),0x0010000000000000ULL,"double normal boundary tie");require_bits(round_binary_nearest_even<double>(normal_tie.negated()),0x8010000000000000ULL,"double negative normal boundary tie");require_bits(round_binary_nearest_even<double>(one_tie),0x3ff0000000000000ULL,"double one tie");require_bits(round_binary_nearest_even<double>(one_tie.negated()),0xbff0000000000000ULL,"double negative one tie");const std::uint64_t max_sig=(std::uint64_t(1)<<53)-1;auto max_quarter=dyadic(max_sig*4+1,1023-52-2),max_mid=dyadic(max_sig*2+1,1023-52-1);require_bits(round_binary_nearest_even<double>(max_quarter),0x7fefffffffffffffULL,"double maximum rounds finite");require(!round_binary_nearest_even<double>(max_mid),"double overflow tie");require_bits(round_binary_down<double>(max_mid),0x7fefffffffffffffULL,"double overflow down");require_bits(round_binary_up<double>(max_mid.negated()),0xffefffffffffffffULL,"double negative overflow up");}
static void binary_neighbors_and_compare(){auto pm=predecessor_bits(coordinate_bits<float>{0U}),nm=successor_bits(coordinate_bits<float>{0x80000000U});require(pm&&pm->bits==0x80000001U&&nm&&nm->bits==1U,"zero neighbors");require(!predecessor_bits(coordinate_bits<float>{0xff7fffffU}),"negative finite endpoint");require(!successor_bits(coordinate_bits<float>{0x7f7fffffU}),"positive finite endpoint");require(!predecessor_bits(coordinate_bits<float>{0x7f800000U}),"non-finite predecessor");auto around=finite_neighboring_bits(coordinate_bits<float>{0U},2);require(around&&around->size()==5&&(*around)[0].bits==0x80000002U&&(*around)[1].bits==0x80000001U&&(*around)[2].bits==0U&&(*around)[3].bits==1U&&(*around)[4].bits==2U,"ordered zero neighborhood");require(!finite_neighboring_bits(coordinate_bits<double>{0x7ff0000000000000ULL},1),"non-finite neighborhood");require(compare_binary_bits(coordinate_bits<float>{0x3f800000U},q(1))==std::optional<int>(0),"exact bit compare equal");require(compare_binary_bits(coordinate_bits<float>{0x3f800001U},q(1))==std::optional<int>(1),"exact bit compare greater");require(compare_binary_bits(coordinate_bits<float>{0x80000000U},q(0))==std::optional<int>(0),"negative zero compare");require(!compare_binary_bits(coordinate_bits<float>{0x7f800000U},q(0)),"non-finite compare");for(std::uint64_t bits:{1ULL,0x000fffffffffffffULL,0x0010000000000000ULL,0x3ff0000000000000ULL,0x7fefffffffffffffULL,0x8000000000000001ULL,0xffefffffffffffffULL}){coordinate_bits<double>b{bits};require(round_binary_nearest_even<double>(decode_coordinate(b).value().value)->bits==bits,"double finite round trip");}}
int main(){try{integers();limb_scale_vectors();gcd_reference_vectors();rationals();rational_reference_vectors();canonical_serialization_vectors();decoding();binary_classification();float_conversion();double_conversion();binary_neighbors_and_compare();std::cout<<"PASS exact arithmetic\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
