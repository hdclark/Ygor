#include "YgorMeshesExactKernel.h"
#include <cfenv>
#include <cmath>
#include <cfloat>
#include <limits>
#if defined(__FAST_MATH__)
#error "Exact predicate filters must not be compiled with fast-math"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Exact predicate filters must not assume finite-only arithmetic"
#endif
namespace ygor { namespace mesh_boolean { namespace exact_filter_policy {

const std::uint16_t orient2d_proof_version=1;
const std::uint16_t orient3d_proof_version=1;
const std::uint16_t plane_side_proof_version=1;
const std::uint16_t dot_sign_proof_version=1;

namespace {
// Proof v1 accepts only exactly representable, normal dyadics. Differences are
// converted after exact subtraction, so the floating DAG starts with exact
// operands. With contraction disabled, each product/addition rounds once.
// The 32*epsilon permanent bounds exceed gamma_15, including computation of
// the permanent itself, for both IEC binary32 and binary64.
template<class F> bool exact_normal(const exact_scalar&x,F&out){
  if(x.is_zero()){out=F(0);return true;}
  const auto&n=x.numerator();const auto&d=x.denominator();
  if(!d.is_power_of_two())return false;
  const auto digits=std::numeric_limits<F>::digits;
  if(n.magnitude().bit_length()>static_cast<std::size_t>(digits))return false;
  bool fits=false;const auto magnitude=n.magnitude().to_uint64(&fits);if(!fits)return false;
  const auto exponent=-static_cast<int>(d.bit_length()-1);
  out=std::ldexp(static_cast<F>(magnitude),exponent);
  if(n.sign()==integer_sign::negative)out=-out;
  return std::isnormal(out);
}
template<class F>std::optional<exact_sign>certify(F determinant,F permanent)noexcept{
  if(!std::isnormal(determinant)||!std::isfinite(permanent)||permanent<F(0))return std::nullopt;
  const F bound=F(32)*std::numeric_limits<F>::epsilon()*permanent;
  if(!std::isfinite(bound)||!(std::fabs(determinant)>bound))return std::nullopt;
  return determinant<F(0)?exact_sign::negative:exact_sign::positive;
}
template<class F>bool valid_product(F result,F a,F b)noexcept{return std::isnormal(result)||(result==F(0)&&(a==F(0)||b==F(0)));}
template<class F>bool known_zero_product(F a,F b)noexcept{return a==F(0)||b==F(0);}
template<class F>std::optional<exact_sign>orient2_impl(const exact_point2&a,const exact_point2&b,const exact_point2&c){
  F ux,uy,vx,vy;if(!exact_normal(b.x-a.x,ux)||!exact_normal(b.y-a.y,uy)||!exact_normal(c.x-a.x,vx)||!exact_normal(c.y-a.y,vy))return std::nullopt;
  const F p=ux*vy,q=uy*vx,det=p-q;if(!valid_product(p,ux,vy)||!valid_product(q,uy,vx)||(!std::isnormal(det)&&det!=F(0)))return std::nullopt;
  return certify(det,std::fabs(p)+std::fabs(q));
}
template<class F>std::optional<exact_sign>orient3_impl(const exact_point3&a,const exact_point3&b,const exact_point3&c,const exact_point3&d){
  F ux,uy,uz,vx,vy,vz,wx,wy,wz;
  if(!exact_normal(b.x-a.x,ux)||!exact_normal(b.y-a.y,uy)||!exact_normal(b.z-a.z,uz)||!exact_normal(c.x-a.x,vx)||!exact_normal(c.y-a.y,vy)||!exact_normal(c.z-a.z,vz)||!exact_normal(d.x-a.x,wx)||!exact_normal(d.y-a.y,wy)||!exact_normal(d.z-a.z,wz))return std::nullopt;
  const F p0=vy*wz,v0=vz*wy,p1=vz*wx,v1=vx*wz,p2=vx*wy,v2=vy*wx;
  const F c0=p0-v0,c1=p1-v1,c2=p2-v2;
  if((!std::isnormal(c0)&&!(c0==F(0)&&known_zero_product(vy,wz)&&known_zero_product(vz,wy)))||(!std::isnormal(c1)&&!(c1==F(0)&&known_zero_product(vz,wx)&&known_zero_product(vx,wz)))||(!std::isnormal(c2)&&!(c2==F(0)&&known_zero_product(vx,wy)&&known_zero_product(vy,wx))))return std::nullopt;
  const F t0=ux*c0,t1=uy*c1,t2=uz*c2,sum=t0+t1,det=sum+t2;
  const F permanent=std::fabs(ux)*(std::fabs(p0)+std::fabs(v0))+std::fabs(uy)*(std::fabs(p1)+std::fabs(v1))+std::fabs(uz)*(std::fabs(p2)+std::fabs(v2));
  if(!valid_product(p0,vy,wz)||!valid_product(v0,vz,wy)||!valid_product(p1,vz,wx)||!valid_product(v1,vx,wz)||!valid_product(p2,vx,wy)||!valid_product(v2,vy,wx)||!valid_product(t0,ux,c0)||!valid_product(t1,uy,c1)||!valid_product(t2,uz,c2)||(!std::isnormal(sum)&&sum!=F(0)))return std::nullopt;
  return certify(det,permanent);
}
template<class F>std::optional<exact_sign>plane_impl(const exact_plane3&p,const exact_point3&q){
  F a,b,c,d,x,y,z;if(!exact_normal(exact_scalar(p.a,big_uint(1)),a)||!exact_normal(exact_scalar(p.b,big_uint(1)),b)||!exact_normal(exact_scalar(p.c,big_uint(1)),c)||!exact_normal(exact_scalar(p.d,big_uint(1)),d)||!exact_normal(q.x,x)||!exact_normal(q.y,y)||!exact_normal(q.z,z))return std::nullopt;
  const F ax=a*x,by=b*y,cz=c*z,sum0=ax+by,sum1=sum0+cz,det=sum1+d,permanent=std::fabs(ax)+std::fabs(by)+std::fabs(cz)+std::fabs(d);
  if(!valid_product(ax,a,x)||!valid_product(by,b,y)||!valid_product(cz,c,z)||(!std::isnormal(sum0)&&sum0!=F(0))||(!std::isnormal(sum1)&&sum1!=F(0)))return std::nullopt;
  auto result=certify(det,permanent);if(result&&p.oriented==orientation_parity::opposite)*result=(*result==exact_sign::positive?exact_sign::negative:exact_sign::positive);return result;
}
template<class F>std::optional<exact_sign>dot2_impl(const exact_vector2&a,const exact_vector2&b){
  F ax,ay,bx,by;if(!exact_normal(a.x,ax)||!exact_normal(a.y,ay)||!exact_normal(b.x,bx)||!exact_normal(b.y,by))return std::nullopt;const F p=ax*bx,q=ay*by;if(!valid_product(p,ax,bx)||!valid_product(q,ay,by))return std::nullopt;return certify(p+q,std::fabs(p)+std::fabs(q));
}
template<class F>std::optional<exact_sign>dot3_impl(const exact_vector3&a,const exact_vector3&b){
  F ax,ay,az,bx,by,bz;if(!exact_normal(a.x,ax)||!exact_normal(a.y,ay)||!exact_normal(a.z,az)||!exact_normal(b.x,bx)||!exact_normal(b.y,by)||!exact_normal(b.z,bz))return std::nullopt;const F p=ax*bx,q=ay*by,r=az*bz;if(!valid_product(p,ax,bx)||!valid_product(q,ay,by)||!valid_product(r,az,bz))return std::nullopt;return certify((p+q)+r,std::fabs(p)+std::fabs(q)+std::fabs(r));
}
template<class FloatFn>std::optional<exact_sign>dispatch(coordinate_tag tag,FloatFn fn)noexcept{
  if(std::fegetround()!=FE_TONEAREST||FLT_EVAL_METHOD!=0||!std::numeric_limits<float>::is_iec559||!std::numeric_limits<double>::is_iec559)return std::nullopt;
  volatile float f=std::numeric_limits<float>::denorm_min();volatile double g=std::numeric_limits<double>::denorm_min();if(f*1.0f==0.0f||g*1.0==0.0)return std::nullopt;
  fenv_t saved;if(std::fegetenv(&saved)!=0||std::feclearexcept(FE_ALL_EXCEPT)!=0)return std::nullopt;
  try{auto result=tag==coordinate_tag::binary32?fn(float{}):fn(double{});if(std::fesetenv(&saved)!=0)return std::nullopt;return result;}catch(...){std::fesetenv(&saved);return std::nullopt;}
}
}
std::optional<exact_sign>orient2d(const exact_point2&a,const exact_point2&b,const exact_point2&c,coordinate_tag t)noexcept{return dispatch(t,[&](auto f){return orient2_impl<decltype(f)>(a,b,c);});}
std::optional<exact_sign>orient3d(const exact_point3&a,const exact_point3&b,const exact_point3&c,const exact_point3&d,coordinate_tag t)noexcept{return dispatch(t,[&](auto f){return orient3_impl<decltype(f)>(a,b,c,d);});}
std::optional<exact_sign>plane_side(const exact_plane3&p,const exact_point3&q,coordinate_tag t)noexcept{return dispatch(t,[&](auto f){return plane_impl<decltype(f)>(p,q);});}
std::optional<exact_sign>dot_sign(const exact_vector2&a,const exact_vector2&b,coordinate_tag t)noexcept{return dispatch(t,[&](auto f){return dot2_impl<decltype(f)>(a,b);});}
std::optional<exact_sign>dot_sign(const exact_vector3&a,const exact_vector3&b,coordinate_tag t)noexcept{return dispatch(t,[&](auto f){return dot3_impl<decltype(f)>(a,b);});}
} } }
