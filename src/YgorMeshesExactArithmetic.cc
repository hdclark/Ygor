#include "YgorMeshesExactArithmetic.h"
#include "YgorMeshesBooleanPerformance.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace ygor { namespace mesh_boolean {
namespace { void count_integer_operation(std::size_t limbs){performance_count(limbs<=2?performance_counter::small_integer_operations:performance_counter::large_integer_operations);} }
void big_uint::resize(std::size_t n,std::uint32_t value){
  if(n<=inline_capacity_){
    std::array<std::uint32_t,inline_capacity_> next{{value,value}};
    const auto copied=std::min(size_,n);
    for(std::size_t i=0;i<copied;++i)next[i]=limb(i);
    heap_limbs_.clear();inline_limbs_=next;size_=n;return;
  }
  std::vector<std::uint32_t> next(n,value);
  const auto copied=std::min(size_,n);
  for(std::size_t i=0;i<copied;++i)next[i]=limb(i);
  heap_limbs_.swap(next);size_=n;
}
void big_uint::assign(std::size_t n,std::uint32_t value){
  if(n<=inline_capacity_){heap_limbs_.clear();inline_limbs_={{value,value}};size_=n;return;}
  std::vector<std::uint32_t> next(n,value);heap_limbs_.swap(next);size_=n;
}
void big_uint::push_back(std::uint32_t value){const auto old=size_;if(old==std::numeric_limits<std::size_t>::max())throw std::length_error("integer size overflow");resize(old+1);limb(old)=value;}
void big_uint::normalize()noexcept{const auto*current=data();auto normalized=size_;while(normalized&&!current[normalized-1])--normalized;if(normalized<=inline_capacity_&&!heap_limbs_.empty()){for(std::size_t i=0;i<normalized;++i)inline_limbs_[i]=current[i];for(std::size_t i=normalized;i<inline_capacity_;++i)inline_limbs_[i]=0;heap_limbs_.clear();}else if(normalized>inline_capacity_&&heap_limbs_.size()!=normalized)heap_limbs_.resize(normalized);size_=normalized;}
big_uint::big_uint(big_uint&&o)noexcept:inline_limbs_(o.inline_limbs_),heap_limbs_(std::move(o.heap_limbs_)),size_(o.size_){o.inline_limbs_={{0,0}};o.size_=0;}
big_uint&big_uint::operator=(big_uint o){using std::swap;swap(inline_limbs_,o.inline_limbs_);swap(heap_limbs_,o.heap_limbs_);swap(size_,o.size_);return *this;}
big_uint::big_uint(std::uint64_t v){if(v){inline_limbs_[0]=std::uint32_t(v);size_=1;}if(v>>32){inline_limbs_[1]=std::uint32_t(v>>32);size_=2;}}
status_or<big_uint>big_uint::from_hex(const std::string&s,boolean_stage st){try{big_uint r;if(s.empty())return make_error(boolean_error_code::input_contract_error,st,"empty_integer");for(char c:s){unsigned d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:16;if(d==16)return make_error(boolean_error_code::input_contract_error,st,"invalid_integer");r=r.shifted_left(4)+big_uint(d);}return r;}catch(const std::bad_alloc&){return make_error(boolean_error_code::resource_limit,st,"allocation");}}
std::size_t big_uint::bit_length()const noexcept{if(is_zero())return 0;std::uint32_t x=limb(size_-1);std::size_t n=32*(size_-1);while(x){++n;x>>=1;}return n;}
std::size_t big_uint::trailing_zero_bits()const noexcept{if(is_zero())return 0;std::size_t n=0;for(std::size_t i=0;i<size_;++i){auto x=limb(i);if(!x){n+=32;continue;}while(!(x&1)){++n;x>>=1;}break;}return n;}
bool big_uint::is_power_of_two()const noexcept{bool found=false;for(std::size_t i=0;i<size_;++i){const auto x=limb(i);if(!x)continue;if(found||(x&(x-1U)))return false;found=true;}return found;}
int big_uint::compare(const big_uint&o)const noexcept{if(size_!=o.size_)return size_<o.size_?-1:1;for(std::size_t i=size_;i--;){if(limb(i)!=o.limb(i))return limb(i)<o.limb(i)?-1:1;}return 0;}
std::uint64_t big_uint::to_uint64(bool*f)const noexcept{bool ok=size_<=2;if(f)*f=ok;if(!ok)return 0;return (size_?limb(0):0)|(size_>1?std::uint64_t(limb(1))<<32:0);}
std::string big_uint::to_hex()const{if(is_zero())return"0";std::ostringstream os;os<<std::hex<<limb(size_-1);for(std::size_t i=size_-1;i--;)os<<std::setw(8)<<std::setfill('0')<<limb(i);return os.str();}
std::vector<std::uint8_t>big_uint::canonical_bytes()const{std::vector<std::uint8_t>b;for(std::size_t i=bit_length();i;i-=std::min<std::size_t>(8,i)){std::size_t sh=((i-1)/8)*8;auto x=shifted_right(sh);b.push_back(std::uint8_t(x.limb(0)));}return b;}
void big_uint::encode(canonical_encoder&e)const{auto b=canonical_bytes();e.u64(b.size());e.raw(b.data(),b.size());}
big_uint big_uint::shifted_left(std::size_t n)const{if(is_zero())return{};const auto words=n/32,max=std::numeric_limits<std::size_t>::max();if(words>max-size_||words+size_==max)throw std::length_error("integer size overflow");big_uint r;r.assign(words+size_+1,0);unsigned s=n%32;std::uint64_t carry=0;for(std::size_t i=0;i<size_;++i){std::uint64_t v=(std::uint64_t(limb(i))<<s)|carry;r.limb(words+i)=std::uint32_t(v);carry=v>>32;}r.limb(words+size_)=std::uint32_t(carry);r.normalize();return r;}
big_uint big_uint::shifted_right(std::size_t n)const{const auto words=n/32;if(words>=size_)return{};big_uint r;r.resize(size_-words);unsigned s=n%32;std::uint32_t carry=0;for(std::size_t i=size_;i-->words;){auto x=limb(i);r.limb(i-words)=s?(x>>s)|(carry<<(32-s)):x;carry=s?x&((std::uint32_t(1)<<s)-1):0;}r.normalize();return r;}
big_uint operator+(const big_uint&a,const big_uint&b){const auto n=std::max(a.size_,b.size_);count_integer_operation(n);performance_count(performance_counter::limb_additions,n);big_uint r;r.resize(n);std::uint64_t c=0;for(std::size_t i=0;i<n;++i){std::uint64_t v=c+(i<a.size_?a.limb(i):0)+(i<b.size_?b.limb(i):0);r.limb(i)=std::uint32_t(v);c=v>>32;}if(c)r.push_back(std::uint32_t(c));return r;}
big_uint operator-(const big_uint&a,const big_uint&b){count_integer_operation(a.size_);performance_count(performance_counter::limb_additions,a.size_);if(a<b)throw std::invalid_argument("negative unsigned subtraction");big_uint r=a;std::uint64_t borrow=0;for(std::size_t i=0;i<r.size_;++i){std::uint64_t sub=(i<b.size_?b.limb(i):0)+borrow;std::uint64_t av=r.limb(i);r.limb(i)=std::uint32_t(av-sub);borrow=av<sub;}r.normalize();return r;}
big_uint operator*(const big_uint&a,const big_uint&b){count_integer_operation(std::max(a.size_,b.size_));performance_count(performance_counter::limb_multiplications,a.size_*b.size_);if(a.is_zero()||b.is_zero())return{};if(a.size_>std::numeric_limits<std::size_t>::max()-b.size_)throw std::length_error("integer size overflow");big_uint r;r.assign(a.size_+b.size_,0);for(std::size_t i=0;i<a.size_;++i){std::uint64_t c=0;for(std::size_t j=0;j<b.size_;++j){std::uint64_t v=std::uint64_t(a.limb(i))*b.limb(j)+r.limb(i+j)+c;r.limb(i+j)=std::uint32_t(v);c=v>>32;}std::size_t k=i+b.size_;while(c){std::uint64_t v=std::uint64_t(r.limb(k))+c;r.limb(k)=std::uint32_t(v);c=v>>32;++k;}}r.normalize();return r;}
std::pair<big_uint,big_uint>divide(const big_uint&a,const big_uint&b){
  performance_count(performance_counter::division_calls);performance_count(performance_counter::divided_limbs,a.size_);count_integer_operation(std::max(a.size_,b.size_));
  if(b.is_zero())throw std::invalid_argument("division by zero");
  if(a.is_zero()||a<b)return{{},a};
  if(b==big_uint(1))return{a,{}};
  if(a==b)return{big_uint(1),{}};
  if(b.is_power_of_two()){const auto shift=b.trailing_zero_bits();auto q=a.shifted_right(shift);return{std::move(q),a-(q.shifted_left(shift))};}
  if(b.size_==1){
    const std::uint64_t divisor=b.limb(0);std::uint64_t remainder=0;big_uint q;q.resize(a.size_);
    for(std::size_t i=a.size_;i--;){const std::uint64_t current=(remainder<<32)|a.limb(i);q.limb(i)=std::uint32_t(current/divisor);remainder=current%divisor;}
    q.normalize();return{std::move(q),big_uint(remainder)};
  }
  const auto n=b.size_,m=a.size_-n;unsigned normalization=0;for(auto x=b.limb(n-1);(x&0x80000000U)==0;x<<=1)++normalization;
  if(a.size_==std::numeric_limits<std::size_t>::max())throw std::length_error("integer size overflow");
  big_uint v=b.shifted_left(normalization),u=a.shifted_left(normalization);u.resize(a.size_+1);big_uint q;q.assign(m+1,0);
  constexpr std::uint64_t base=std::uint64_t(1)<<32;
  for(std::size_t jj=m+1;jj--;){const auto j=jj;const std::uint64_t numerator=(std::uint64_t(u.limb(j+n))<<32)|u.limb(j+n-1);std::uint64_t qhat=numerator/v.limb(n-1),rhat=numerator%v.limb(n-1);
    if(qhat==base){qhat=base-1;rhat+=v.limb(n-1);}
    while(rhat<base&&qhat*v.limb(n-2)>(rhat<<32)+u.limb(j+n-2)){--qhat;rhat+=v.limb(n-1);}
    std::uint64_t carry=0,borrow=0;
    for(std::size_t i=0;i<n;++i){const std::uint64_t product=qhat*v.limb(i)+carry;carry=product>>32;const std::uint64_t sub=std::uint64_t(std::uint32_t(product))+borrow;const auto current=u.limb(j+i);u.limb(j+i)=std::uint32_t(std::uint64_t(current)-sub);borrow=current<sub;}
    const std::uint64_t top_sub=carry+borrow;const auto top=u.limb(j+n);const bool negative=std::uint64_t(top)<top_sub;u.limb(j+n)=std::uint32_t(std::uint64_t(top)-top_sub);
    if(negative){--qhat;std::uint64_t add_carry=0;for(std::size_t i=0;i<n;++i){const std::uint64_t sum=std::uint64_t(u.limb(j+i))+v.limb(i)+add_carry;u.limb(j+i)=std::uint32_t(sum);add_carry=sum>>32;}u.limb(j+n)=std::uint32_t(std::uint64_t(u.limb(j+n))+add_carry);}
    q.limb(j)=std::uint32_t(qhat);
  }
  q.normalize();u.resize(n);auto remainder=u.shifted_right(normalization);return{std::move(q),std::move(remainder)};
}
big_uint gcd(big_uint a,big_uint b){
  performance_count(performance_counter::gcd_calls);
  if(a.is_zero())return b;
  if(b.is_zero())return a;
  if(a==b)return a;
  if(a.is_power_of_two()&&b.is_power_of_two())return big_uint(1).shifted_left(std::min(a.trailing_zero_bits(),b.trailing_zero_bits()));
  if(a.size_==1||b.size_==1){
    const big_uint&small=a.size_==1?a:b;const big_uint&large=a.size_==1?b:a;
    const auto divisor=small.to_uint64();const auto remainder=divide(large,small).second.to_uint64();
    return big_uint(std::gcd(divisor,remainder));
  }
  const auto common_shift=std::min(a.trailing_zero_bits(),b.trailing_zero_bits());
  a=a.shifted_right(a.trailing_zero_bits());b=b.shifted_right(b.trailing_zero_bits());
  const auto subtract_assign=[](big_uint&larger,const big_uint&smaller){
    count_integer_operation(larger.size_);performance_count(performance_counter::limb_additions,larger.size_);std::uint64_t borrow=0;
    for(std::size_t i=0;i<larger.size_;++i){const std::uint64_t sub=(i<smaller.size_?smaller.limb(i):0)+borrow;const auto value=larger.limb(i);larger.limb(i)=std::uint32_t(std::uint64_t(value)-sub);borrow=value<sub;}
    larger.normalize();
  };
  const auto shift_right_assign=[](big_uint&value,std::size_t bits){
    const auto words=bits/32;const auto shift=unsigned(bits%32);const auto old_size=value.size_,new_size=old_size-words;
    for(std::size_t i=0;i<new_size;++i){const auto source=i+words;value.limb(i)=value.limb(source)>>shift;if(shift&&source+1<old_size)value.limb(i)|=value.limb(source+1)<<(32-shift);}
    if(old_size>big_uint::inline_capacity_&&new_size<=big_uint::inline_capacity_){
      std::array<std::uint32_t,big_uint::inline_capacity_> next{{0,0}};for(std::size_t i=0;i<new_size;++i)next[i]=value.heap_limbs_[i];value.inline_limbs_=next;value.heap_limbs_.clear();value.size_=new_size;
    }else{value.size_=new_size;value.normalize();}
  };
  while(a!=b){
    big_uint*larger=&a,*smaller=&b;if(a.compare(b)<0)std::swap(larger,smaller);
    subtract_assign(*larger,*smaller);shift_right_assign(*larger,larger->trailing_zero_bits());
  }
  return a.shifted_left(common_shift);
}

big_int::big_int(std::int64_t v):sign_(v<0?integer_sign::negative:v?integer_sign::positive:integer_sign::zero),magnitude_(v<0?std::uint64_t(-(v+1))+1:std::uint64_t(v)){}
big_int::big_int(integer_sign s,big_uint m):sign_(m.is_zero()?integer_sign::zero:s),magnitude_(std::move(m)){if(!magnitude_.is_zero()&&s==integer_sign::zero)throw std::invalid_argument("zero sign with magnitude");}
int big_int::compare(const big_int&o)const noexcept{if(sign_!=o.sign_)return int(sign_)<int(o.sign_)?-1:1;int c=magnitude_.compare(o.magnitude_);return sign_==integer_sign::negative?-c:c;}
big_int big_int::negated()const{return big_int(sign_==integer_sign::negative?integer_sign::positive:sign_==integer_sign::positive?integer_sign::negative:integer_sign::zero,magnitude_);}
std::string big_int::to_string()const{return(sign_==integer_sign::negative?"-":"")+magnitude_.to_hex();}
void big_int::encode(canonical_encoder&e)const{e.byte(std::uint8_t(std::int8_t(sign_)));magnitude_.encode(e);}
big_int operator+(const big_int&a,const big_int&b){if(a.sign_==integer_sign::zero)return b;if(b.sign_==integer_sign::zero)return a;if(a.sign_==b.sign_)return big_int(a.sign_,a.magnitude_+b.magnitude_);int c=a.magnitude_.compare(b.magnitude_);return c==0?big_int():c>0?big_int(a.sign_,a.magnitude_-b.magnitude_):big_int(b.sign_,b.magnitude_-a.magnitude_);}
big_int operator-(const big_int&a,const big_int&b){return a+b.negated();}
big_int operator*(const big_int&a,const big_int&b){if(a.is_zero()||b.is_zero())return{};return big_int(a.sign_==b.sign_?integer_sign::positive:integer_sign::negative,a.magnitude_*b.magnitude_);}
std::pair<big_int,big_int>divide(const big_int&a,const big_int&b){if(b.is_zero())throw std::invalid_argument("division by zero");auto qr=divide(a.magnitude_,b.magnitude_);auto qs=a.sign_==b.sign_?integer_sign::positive:integer_sign::negative;return{big_int(qs,std::move(qr.first)),big_int(a.sign_,std::move(qr.second))};}

exact_rational::exact_rational(big_int n,big_uint d):numerator_(std::move(n)),denominator_(std::move(d)){normalize();}
void exact_rational::normalize(){
  performance_count(performance_counter::rational_normalizations);performance_max(performance_counter::max_numerator_limbs,numerator_.magnitude().limb_count());performance_max(performance_counter::max_denominator_limbs,denominator_.limb_count());
  if(denominator_.is_zero())throw std::invalid_argument("zero denominator");
  if(numerator_.is_zero()){denominator_=big_uint(1);return;}
  if(denominator_==big_uint(1))return;
  auto g=gcd(numerator_.magnitude(),denominator_);if(g==big_uint(1))return;
  numerator_=big_int(numerator_.sign(),divide(numerator_.magnitude(),g).first);denominator_=divide(denominator_,g).first;
}
exact_rational exact_rational::reduced(big_int n,big_uint d){
  if(d.is_zero())throw std::invalid_argument("zero denominator");
  exact_rational result;result.numerator_=std::move(n);result.denominator_=result.numerator_.is_zero()?big_uint(1):std::move(d);
  performance_max(performance_counter::max_numerator_limbs,result.numerator_.magnitude().limb_count());performance_max(performance_counter::max_denominator_limbs,result.denominator_.limb_count());return result;
}
exact_rational exact_rational::negated()const{return reduced(numerator_.negated(),denominator_);}
exact_rational exact_rational::abs()const{return sign()==exact_sign::negative?negated():*this;}
namespace{
big_uint exact_quotient(const big_uint&value,const big_uint&factor){if(factor==big_uint(1))return value;auto qr=divide(value,factor);if(!qr.second.is_zero())throw std::logic_error("non-exact rational reduction");return std::move(qr.first);}
void count_cancellation(const big_uint&factor){if(factor!=big_uint(1))performance_count(performance_counter::cross_cancellations);}
int positive_rational_compare(const exact_rational&a,const exact_rational&b){
  const auto an=a.numerator().magnitude().bit_length(),ad=a.denominator().bit_length(),bn=b.numerator().magnitude().bit_length(),bd=b.denominator().bit_length();
  const auto maximum=std::numeric_limits<std::size_t>::max();
  if(an<=maximum-bd&&bn<=maximum-ad){const auto left_bits=an+bd,right_bits=bn+ad;if(left_bits>right_bits&&left_bits-right_bits>1)return 1;if(right_bits>left_bits&&right_bits-left_bits>1)return -1;}
  if(a.denominator().is_power_of_two()&&b.denominator().is_power_of_two()){
    const auto ae=a.denominator().trailing_zero_bits(),be=b.denominator().trailing_zero_bits();
    return ae>=be?a.numerator().magnitude().compare(b.numerator().magnitude().shifted_left(ae-be)):a.numerator().magnitude().shifted_left(be-ae).compare(b.numerator().magnitude());
  }
  big_uint left_n=a.numerator().magnitude(),right_n=b.numerator().magnitude(),left_d=a.denominator(),right_d=b.denominator();
  if(left_n.limb_count()>1&&right_n.limb_count()>1){const auto common=gcd(left_n,right_n);if(common!=big_uint(1)){left_n=exact_quotient(left_n,common);right_n=exact_quotient(right_n,common);}}
  if(left_d.limb_count()>1&&right_d.limb_count()>1){const auto common=gcd(left_d,right_d);if(common!=big_uint(1)){left_d=exact_quotient(left_d,common);right_d=exact_quotient(right_d,common);}}
  return (left_n*right_d).compare(right_n*left_d);
}
}
int exact_rational::compare(const exact_rational&o)const{if(sign()!=o.sign())return int(sign())<int(o.sign())?-1:1;if(is_zero())return 0;const auto magnitude=positive_rational_compare(*this,o);return sign()==exact_sign::negative?-magnitude:magnitude;}
exact_rational exact_rational::pow(std::uint64_t n)const{exact_rational b=*this,r(1);while(n){if(n&1)r=r*b;n>>=1;if(n)b=b*b;}return r;}
big_int exact_rational::trunc()const{return divide(numerator_,big_int(integer_sign::positive,denominator_)).first;}
big_int exact_rational::floor()const{auto q=divide(numerator_,big_int(integer_sign::positive,denominator_));return sign()==exact_sign::negative&&!q.second.is_zero()?q.first-big_int(1):q.first;}
big_int exact_rational::ceil()const{auto q=divide(numerator_,big_int(integer_sign::positive,denominator_));return sign()==exact_sign::positive&&!q.second.is_zero()?q.first+big_int(1):q.first;}
std::string exact_rational::to_string()const{return numerator_.to_string()+(denominator_==big_uint(1)?"":"/"+denominator_.to_hex());}
void exact_rational::encode(canonical_encoder&e)const{numerator_.encode(e);denominator_.encode(e);}std::vector<std::uint8_t>exact_rational::canonical_bytes()const{canonical_encoder e;encode(e);return e.bytes();}digest exact_rational::canonical_hash()const{auto b=canonical_bytes();return domain_digest({{'Y','G','R','A','T','0','0','1'}},b);}
exact_rational operator+(const exact_rational&a,const exact_rational&b){
  if(a.is_zero())return b;
  if(b.is_zero())return a;
  const auto g=gcd(a.denominator_,b.denominator_);count_cancellation(g);
  const auto ad=exact_quotient(a.denominator_,g),bd=exact_quotient(b.denominator_,g);
  auto numerator=a.numerator_*big_int(integer_sign::positive,bd)+b.numerator_*big_int(integer_sign::positive,ad);if(numerator.is_zero())return exact_rational();
  const auto remaining=g==big_uint(1)?big_uint(1):gcd(numerator.magnitude(),g);count_cancellation(remaining);
  numerator=big_int(numerator.sign(),exact_quotient(numerator.magnitude(),remaining));
  return exact_rational::reduced(std::move(numerator),ad*exact_quotient(b.denominator_,remaining));
}
exact_rational operator-(const exact_rational&a,const exact_rational&b){return a+b.negated();}
exact_rational operator*(const exact_rational&a,const exact_rational&b){
  if(a.is_zero()||b.is_zero())return exact_rational();
  const auto left=gcd(a.numerator_.magnitude(),b.denominator_),right=gcd(b.numerator_.magnitude(),a.denominator_);count_cancellation(left);count_cancellation(right);
  const auto magnitude=exact_quotient(a.numerator_.magnitude(),left)*exact_quotient(b.numerator_.magnitude(),right);
  const auto sign=a.numerator_.sign()==b.numerator_.sign()?integer_sign::positive:integer_sign::negative;
  return exact_rational::reduced(big_int(sign,magnitude),exact_quotient(a.denominator_,right)*exact_quotient(b.denominator_,left));
}
exact_rational operator/(const exact_rational&a,const exact_rational&b){
  if(b.is_zero())throw std::invalid_argument("rational division by zero");
  if(a.is_zero())return exact_rational();
  const auto numerators=gcd(a.numerator_.magnitude(),b.numerator_.magnitude()),denominators=gcd(b.denominator_,a.denominator_);count_cancellation(numerators);count_cancellation(denominators);
  const auto magnitude=exact_quotient(a.numerator_.magnitude(),numerators)*exact_quotient(b.denominator_,denominators);
  const auto sign=a.numerator_.sign()==b.numerator_.sign()?integer_sign::positive:integer_sign::negative;
  return exact_rational::reduced(big_int(sign,magnitude),exact_quotient(a.denominator_,denominators)*exact_quotient(b.numerator_.magnitude(),numerators));
}

template<class T>coordinate_bits<T>bits_of(T v)noexcept{coordinate_bits<T>b;std::memcpy(&b.bits,&v,sizeof(v));return b;}template<class T>T value_of_bits(coordinate_bits<T>b)noexcept{T v;std::memcpy(&v,&b.bits,sizeof(v));return v;}
template<class T>coordinate_category classify_coordinate_bits(coordinate_bits<T>b)noexcept{using U=decltype(b.bits);constexpr unsigned total=sizeof(T)*8,fb=std::numeric_limits<T>::digits-1,eb=total-fb-1;const U frac=b.bits&((U(1)<<fb)-1),exp=(b.bits>>fb)&((U(1)<<eb)-1);if(exp==((U(1)<<eb)-1))return frac?coordinate_category::nan:coordinate_category::infinity;if(!exp&&!frac)return b.bits>>(total-1)?coordinate_category::negative_zero:coordinate_category::positive_zero;return exp?coordinate_category::normal:coordinate_category::subnormal;}
template<class T>bool is_finite_coordinate_bits(coordinate_bits<T>b)noexcept{auto c=classify_coordinate_bits(b);return c!=coordinate_category::infinity&&c!=coordinate_category::nan;}
template<class T>status_or<decoded_coordinate<T>>decode_coordinate(coordinate_bits<T>b,boolean_stage st){using U=decltype(b.bits);constexpr unsigned total=sizeof(T)*8,fb=std::numeric_limits<T>::digits-1,eb=total-fb-1;U sign=b.bits>>(total-1),frac=b.bits&((U(1)<<fb)-1),exp=(b.bits>>fb)&((U(1)<<eb)-1);decoded_coordinate<T>r{};r.source=b;r.negative=sign;r.category=classify_coordinate_bits(b);if(r.category==coordinate_category::nan||r.category==coordinate_category::infinity){auto e=make_error(boolean_error_code::input_contract_error,st,r.category==coordinate_category::nan?"nan_coordinate":"infinite_coordinate");e.subcode=std::uint32_t(b.bits);return e;}if(r.category==coordinate_category::positive_zero||r.category==coordinate_category::negative_zero)return r;r.significand=exp?(std::uint64_t(U(1)<<fb)|frac):frac;r.exponent2=exp?std::int32_t(exp)-(std::int32_t((U(1)<<(eb-1))-1))-std::int32_t(fb):std::numeric_limits<T>::min_exponent-std::int32_t(fb)-1;while(!(r.significand&1)){r.significand>>=1;++r.exponent2;}big_uint m(r.significand);big_int n(sign?integer_sign::negative:integer_sign::positive,m);r.value=r.exponent2>=0?exact_rational(big_int(n.sign(),n.magnitude().shifted_left(r.exponent2)),big_uint(1)):exact_rational(n,big_uint(1).shifted_left(-r.exponent2));return r;}

namespace{
enum class binary_rounding{nearest_even,down,up};
template<class T>struct binary_format{using U=decltype(coordinate_bits<T>{}.bits);static constexpr unsigned total=sizeof(T)*8,frac=std::numeric_limits<T>::digits-1,exp=total-frac-1;static constexpr U sign=U(1)<<(total-1),frac_mask=(U(1)<<frac)-1,exp_mask=(U(1)<<exp)-1,max_finite=((exp_mask-1)<<frac)|frac_mask;static constexpr int bias=(int(U(1)<<(exp-1))-1),emin=std::numeric_limits<T>::min_exponent-1,emax=std::numeric_limits<T>::max_exponent-1,subunit=emin-int(frac);};
int compare_ratio_power_two(const big_uint&n,const big_uint&d,int e){return e>=0?n.compare(d.shifted_left(std::size_t(e))):n.shifted_left(std::size_t(-e)).compare(d);}
template<class T>std::optional<coordinate_bits<T>>round_binary(const exact_rational&x,binary_rounding mode){using F=binary_format<T>;using U=typename F::U;const bool negative=x.sign()==exact_sign::negative;if(x.is_zero())return coordinate_bits<T>{0};const auto&n=x.numerator().magnitude();const auto&d=x.denominator();if(compare_ratio_power_two(n,d,F::emax+1)>=0){if((!negative&&mode==binary_rounding::down)||(negative&&mode==binary_rounding::up))return coordinate_bits<T>{U(F::max_finite|(negative?F::sign:0))};return std::nullopt;}int unit=F::subunit;if(compare_ratio_power_two(n,d,F::emin)>=0){int e=int(n.bit_length())-int(d.bit_length());if(compare_ratio_power_two(n,d,e)<0)--e;unit=e-int(F::frac);}big_uint scaled_n=n,scaled_d=d;if(unit<0)scaled_n=scaled_n.shifted_left(std::size_t(-unit));else scaled_d=scaled_d.shifted_left(std::size_t(unit));auto qr=divide(scaled_n,scaled_d);bool increment=false;if(mode==binary_rounding::nearest_even){int half=(qr.second+qr.second).compare(scaled_d);bool fits=false;auto q=qr.first.to_uint64(&fits);increment=half>0||(half==0&&fits&&(q&1));}else increment=negative?(mode==binary_rounding::down):(mode==binary_rounding::up);if(increment&&!qr.second.is_zero())qr.first=qr.first+big_uint(1);bool fits=false;std::uint64_t q=qr.first.to_uint64(&fits);if(!fits)throw std::logic_error("binary significand overflow");if(!q)return coordinate_bits<T>{negative?F::sign:U(0)};const std::uint64_t normal_bit=std::uint64_t(1)<<F::frac;if(unit==F::subunit&&q<normal_bit)return coordinate_bits<T>{U(q)|(negative?F::sign:0)};int e=unit+int(F::frac);if(q==(normal_bit<<1)){q>>=1;++e;}if(e>F::emax)return std::nullopt;U bits=(U(e+F::bias)<<F::frac)|U(q-normal_bit);return coordinate_bits<T>{U(bits|(negative?F::sign:0))};}
}
template<class T>std::optional<coordinate_bits<T>>round_binary_nearest_even(const exact_rational&x){return round_binary<T>(x,binary_rounding::nearest_even);}template<class T>std::optional<coordinate_bits<T>>round_binary_down(const exact_rational&x){return round_binary<T>(x,binary_rounding::down);}template<class T>std::optional<coordinate_bits<T>>round_binary_up(const exact_rational&x){return round_binary<T>(x,binary_rounding::up);}
template<class T>std::optional<coordinate_bits<T>>predecessor_bits(coordinate_bits<T>b)noexcept{using F=binary_format<T>;using U=typename F::U;if(!is_finite_coordinate_bits(b))return std::nullopt;U magnitude=b.bits&~F::sign;if(!magnitude)return coordinate_bits<T>{U(F::sign|1)};if(b.bits&F::sign){if(magnitude==F::max_finite)return std::nullopt;return coordinate_bits<T>{U(b.bits+1)};}return coordinate_bits<T>{U(b.bits-1)};}
template<class T>std::optional<coordinate_bits<T>>successor_bits(coordinate_bits<T>b)noexcept{using F=binary_format<T>;using U=typename F::U;if(!is_finite_coordinate_bits(b))return std::nullopt;U magnitude=b.bits&~F::sign;if(!magnitude)return coordinate_bits<T>{1};if(b.bits&F::sign)return coordinate_bits<T>{U(b.bits-1)};if(magnitude==F::max_finite)return std::nullopt;return coordinate_bits<T>{U(b.bits+1)};}
template<class T>std::optional<int>compare_binary_bits(coordinate_bits<T>b,const exact_rational&x){if(!is_finite_coordinate_bits(b))return std::nullopt;auto d=decode_coordinate(b);if(!d.has_value())return std::nullopt;return d.value().value.compare(x);}
template<class T>std::optional<std::vector<coordinate_bits<T>>>finite_neighboring_bits(coordinate_bits<T>b,std::uint32_t radius){if(!is_finite_coordinate_bits(b))return std::nullopt;std::vector<coordinate_bits<T>>lower,upper;lower.reserve(radius);upper.reserve(radius);auto p=b,s=b;for(std::uint32_t i=0;i<radius;++i){auto n=predecessor_bits(p);if(!n)break;lower.push_back(*n);p=*n;}for(std::uint32_t i=0;i<radius;++i){auto n=successor_bits(s);if(!n)break;upper.push_back(*n);s=*n;}std::vector<coordinate_bits<T>>r;r.reserve(lower.size()+1+upper.size());for(auto i=lower.rbegin();i!=lower.rend();++i)r.push_back(*i);r.push_back(b);r.insert(r.end(),upper.begin(),upper.end());return r;}

#define YGOR_INSTANTIATE_BINARY(T) template coordinate_bits<T>bits_of(T)noexcept;template T value_of_bits(coordinate_bits<T>)noexcept;template coordinate_category classify_coordinate_bits(coordinate_bits<T>)noexcept;template bool is_finite_coordinate_bits(coordinate_bits<T>)noexcept;template status_or<decoded_coordinate<T>>decode_coordinate(coordinate_bits<T>,boolean_stage);template std::optional<coordinate_bits<T>>round_binary_nearest_even(const exact_rational&);template std::optional<coordinate_bits<T>>round_binary_down(const exact_rational&);template std::optional<coordinate_bits<T>>round_binary_up(const exact_rational&);template std::optional<coordinate_bits<T>>predecessor_bits(coordinate_bits<T>)noexcept;template std::optional<coordinate_bits<T>>successor_bits(coordinate_bits<T>)noexcept;template std::optional<int>compare_binary_bits(coordinate_bits<T>,const exact_rational&);template std::optional<std::vector<coordinate_bits<T>>>finite_neighboring_bits(coordinate_bits<T>,std::uint32_t);
YGOR_INSTANTIATE_BINARY(float)
YGOR_INSTANTIATE_BINARY(double)
#undef YGOR_INSTANTIATE_BINARY
void encode(canonical_encoder&e,const big_uint&v){v.encode(e);}void encode(canonical_encoder&e,const big_int&v){v.encode(e);}void encode(canonical_encoder&e,const exact_rational&v){v.encode(e);}
} }
namespace{std::size_t hash_bytes(const std::vector<std::uint8_t>&b)noexcept{std::size_t h=1469598103934665603ULL;for(auto x:b)h=(h^x)*1099511628211ULL;return h;}}
namespace std{size_t hash<ygor::mesh_boolean::big_uint>::operator()(const ygor::mesh_boolean::big_uint&v)const noexcept{return hash_bytes(v.canonical_bytes());}size_t hash<ygor::mesh_boolean::big_int>::operator()(const ygor::mesh_boolean::big_int&v)const noexcept{ygor::mesh_boolean::canonical_encoder e;v.encode(e);return hash_bytes(e.bytes());}size_t hash<ygor::mesh_boolean::exact_rational>::operator()(const ygor::mesh_boolean::exact_rational&v)const noexcept{return hash_bytes(v.canonical_bytes());}}
