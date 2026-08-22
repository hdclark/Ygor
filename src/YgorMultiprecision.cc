//YgorMultiprecision.cc - Exact integer/rational helpers for Ygor BSP code.

#include "YgorMultiprecision.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

ExactInt::ExactInt() = default;
ExactInt::ExactInt(int64_t v) {
    if(v == 0) return;
    sign_ = (v < 0) ? -1 : 1;
    uint64_t mag = (v < 0) ? uint64_t(-(v + 1)) + 1U : uint64_t(v);
    while(mag != 0) { limbs_.push_back(uint32_t(mag % base)); mag /= base; }
}
int ExactInt::sign() const { return sign_; }
bool ExactInt::is_zero() const { return sign_ == 0; }
ExactInt ExactInt::abs() const { ExactInt r(*this); if(r.sign_ < 0) r.sign_ = 1; return r; }
void ExactInt::normalize(){ while(!limbs_.empty() && limbs_.back()==0) limbs_.pop_back(); if(limbs_.empty()) sign_=0; }
int ExactInt::abs_compare(const ExactInt &a,const ExactInt &b){ if(a.limbs_.size()!=b.limbs_.size()) return a.limbs_.size()<b.limbs_.size()?-1:1; for(size_t i=a.limbs_.size(); i-- >0;){ if(a.limbs_[i]!=b.limbs_[i]) return a.limbs_[i]<b.limbs_[i]?-1:1;} return 0; }
ExactInt ExactInt::abs_add(const ExactInt&a,const ExactInt&b){ ExactInt r; r.sign_=1; uint64_t carry=0; const size_t n=std::max(a.limbs_.size(),b.limbs_.size()); r.limbs_.resize(n); for(size_t i=0;i<n;++i){ uint64_t s=carry; if(i<a.limbs_.size())s+=a.limbs_[i]; if(i<b.limbs_.size())s+=b.limbs_[i]; r.limbs_[i]=uint32_t(s%base); carry=s/base;} if(carry)r.limbs_.push_back(uint32_t(carry)); r.normalize(); return r; }
ExactInt ExactInt::abs_sub(const ExactInt&a,const ExactInt&b){ ExactInt r; r.sign_=1; r.limbs_.resize(a.limbs_.size()); int64_t borrow=0; for(size_t i=0;i<a.limbs_.size();++i){ int64_t s=int64_t(a.limbs_[i])-borrow-(i<b.limbs_.size()?b.limbs_[i]:0); if(s<0){s+=base; borrow=1;}else borrow=0; r.limbs_[i]=uint32_t(s);} r.normalize(); return r; }
ExactInt operator-(ExactInt v){ if(v.sign_!=0) v.sign_=-v.sign_; return v; }
ExactInt& ExactInt::operator+=(const ExactInt&rhs){ if(rhs.sign_==0)return *this; if(sign_==0){*this=rhs; return *this;} if(sign_==rhs.sign_){ *this=abs_add(*this,rhs); sign_=rhs.sign_; return *this;} const int cmp=abs_compare(*this,rhs); if(cmp==0){*this=ExactInt(); return *this;} if(cmp>0){ int s=sign_; *this=abs_sub(*this,rhs); sign_=s;} else { int s=rhs.sign_; *this=abs_sub(rhs,*this); sign_=s;} return *this; }
ExactInt& ExactInt::operator-=(const ExactInt&rhs){ return (*this += -ExactInt(rhs)); }
ExactInt& ExactInt::operator*=(const ExactInt&rhs){ if(is_zero()||rhs.is_zero()){*this=ExactInt();return *this;} ExactInt r; r.sign_=sign_*rhs.sign_; r.limbs_.assign(limbs_.size()+rhs.limbs_.size(),0); for(size_t i=0;i<limbs_.size();++i){ uint64_t carry=0; for(size_t j=0;j<rhs.limbs_.size()||carry;++j){ unsigned long long cur=r.limbs_[i+j]+carry; if(j<rhs.limbs_.size()) cur += (unsigned long long)limbs_[i]*rhs.limbs_[j]; r.limbs_[i+j]=uint32_t(cur%base); carry=cur/base; }} r.normalize(); *this=std::move(r); return *this; }
void ExactInt::mul_small(uint32_t m){ if(is_zero()||m==1)return; if(m==0){*this=ExactInt();return;} uint64_t carry=0; for(auto &x:limbs_){ uint64_t cur=uint64_t(x)*m+carry; x=uint32_t(cur%base); carry=cur/base;} if(carry)limbs_.push_back(uint32_t(carry)); }
uint32_t ExactInt::div_small(uint32_t d){ uint64_t rem=0; for(size_t i=limbs_.size(); i-- >0;){ uint64_t cur=limbs_[i]+rem*base; limbs_[i]=uint32_t(cur/d); rem=cur%d;} normalize(); return uint32_t(rem); }
void ExactInt::div_mod_abs(const ExactInt&a,const ExactInt&b,ExactInt&q,ExactInt&r){ if(b.is_zero()) throw std::invalid_argument("ExactInt division by zero."); q=ExactInt(); r=ExactInt(); if(abs_compare(a,b)<0){r=a; return;} q.sign_=1; q.limbs_.assign(a.limbs_.size(),0); for(size_t i=a.limbs_.size(); i-- >0;){ r.mul_small(base); r += ExactInt(int64_t(a.limbs_[i])); uint32_t lo=0,hi=base-1,best=0; while(lo<=hi){ uint32_t mid=lo+(hi-lo)/2; ExactInt prod=b; prod.mul_small(mid); if(abs_compare(prod,r)<=0){best=mid; lo=mid+1;} else hi=mid-1;} q.limbs_[i]=best; ExactInt sub=b; sub.mul_small(best); r -= sub; } q.normalize(); r.normalize(); }
ExactInt& ExactInt::operator/=(const ExactInt&rhs){ int s=sign_*rhs.sign_; ExactInt q,r; div_mod_abs(abs(),rhs.abs(),q,r); q.sign_=q.is_zero()?0:s; *this=std::move(q); return *this; }
ExactInt& ExactInt::operator%=(const ExactInt&rhs){ int s=sign_; ExactInt q,r; div_mod_abs(abs(),rhs.abs(),q,r); r.sign_=r.is_zero()?0:s; *this=std::move(r); return *this; }
ExactInt& ExactInt::operator<<=(int bits){ if(bits<0) throw std::invalid_argument("negative ExactInt shift"); while(bits>=29){ mul_small(1U<<29); bits-=29;} if(bits) mul_small(1U<<bits); return *this; }
long double ExactInt::to_long_double() const{ long double v=0; for(size_t i=limbs_.size(); i-- >0;) v=v*base+limbs_[i]; return sign_<0?-v:v; }
bool operator<(const ExactInt&a,const ExactInt&b){ if(a.sign_!=b.sign_) return a.sign_<b.sign_; if(a.sign_==0)return false; int cmp=ExactInt::abs_compare(a,b); return a.sign_>0?cmp<0:cmp>0; }
bool operator==(const ExactInt&a,const ExactInt&b){ return a.sign_==b.sign_ && a.limbs_==b.limbs_; }
bool operator!=(const ExactInt&a,const ExactInt&b){return !(a==b);} bool operator>(const ExactInt&a,const ExactInt&b){return b<a;} bool operator<=(const ExactInt&a,const ExactInt&b){return !(b<a);} bool operator>=(const ExactInt&a,const ExactInt&b){return !(a<b);} 
ExactInt abs_exact_int(ExactInt v){return v.abs();}
ExactInt gcd_exact_int(ExactInt a,ExactInt b){ a=abs_exact_int(a); b=abs_exact_int(b); while(b!=0){ ExactInt r=a%b; a=b; b=r;} return (a==0)?ExactInt(1):a; }

ExactScalar::ExactScalar(ExactInt num, ExactInt den):n(std::move(num)),d(std::move(den)){normalize();}
void ExactScalar::normalize(){ if(d==0) throw std::invalid_argument("ExactScalar: zero denominator."); if(d<0){n=-n; d=-d;} ExactInt g=gcd_exact_int(n,d); n/=g; d/=g; }
int ExactScalar::sign() const{return (n>0)?1:((n<0)?-1:0);} bool ExactScalar::is_zero() const{return n==0;}
ExactScalar operator-(const ExactScalar&a){return ExactScalar(-a.n,a.d);} ExactScalar operator+(const ExactScalar&a,const ExactScalar&b){return ExactScalar(a.n*b.d+b.n*a.d,a.d*b.d);} ExactScalar operator-(const ExactScalar&a,const ExactScalar&b){return ExactScalar(a.n*b.d-b.n*a.d,a.d*b.d);} ExactScalar operator*(const ExactScalar&a,const ExactScalar&b){return ExactScalar(a.n*b.n,a.d*b.d);} ExactScalar operator/(const ExactScalar&a,const ExactScalar&b){ if(b.n==0) throw std::invalid_argument("ExactScalar division by zero."); return ExactScalar(a.n*b.d,a.d*b.n);} bool operator==(const ExactScalar&a,const ExactScalar&b){return a.n==b.n&&a.d==b.d;} bool operator!=(const ExactScalar&a,const ExactScalar&b){return !(a==b);} bool operator<(const ExactScalar&a,const ExactScalar&b){return a.n*b.d < b.n*a.d;} bool operator<=(const ExactScalar&a,const ExactScalar&b){return !(b<a);} bool operator>(const ExactScalar&a,const ExactScalar&b){return b<a;} bool operator>=(const ExactScalar&a,const ExactScalar&b){return !(a<b);} 
ExactPoint2 operator+(const ExactPoint2&a,const ExactPoint2&b){return{a.x+b.x,a.y+b.y};} ExactPoint2 operator-(const ExactPoint2&a,const ExactPoint2&b){return{a.x-b.x,a.y-b.y};} ExactPoint2 operator*(const ExactPoint2&p,const ExactScalar&s){return{p.x*s,p.y*s};} bool operator==(const ExactPoint2&a,const ExactPoint2&b){return a.x==b.x&&a.y==b.y;} bool exact_point2_less(const ExactPoint2&a,const ExactPoint2&b){return std::tie(a.x,a.y)<std::tie(b.x,b.y);} ExactScalar cross_exact_2d(const ExactPoint2&a,const ExactPoint2&b){return a.x*b.y-a.y*b.x;} ExactScalar orient_exact_2d(const ExactPoint2&a,const ExactPoint2&b,const ExactPoint2&c){return cross_exact_2d(b-a,c-a);} 
ExactPoint3 operator+(const ExactPoint3&a,const ExactPoint3&b){return{a.x+b.x,a.y+b.y,a.z+b.z};} ExactPoint3 operator-(const ExactPoint3&a,const ExactPoint3&b){return{a.x-b.x,a.y-b.y,a.z-b.z};} ExactPoint3 operator*(const ExactPoint3&p,const ExactScalar&s){return{p.x*s,p.y*s,p.z*s};} bool operator==(const ExactPoint3&a,const ExactPoint3&b){return a.x==b.x&&a.y==b.y&&a.z==b.z;} bool exact_point_less(const ExactPoint3&a,const ExactPoint3&b){return std::tie(a.x,a.y,a.z)<std::tie(b.x,b.y,b.z);} ExactScalar dot_exact(const ExactPoint3&a,const ExactPoint3&b){return a.x*b.x+a.y*b.y+a.z*b.z;} ExactPoint3 cross_exact(const ExactPoint3&a,const ExactPoint3&b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
ExactPlane3 ExactPlane3::from_points(const ExactPoint3&p0,const ExactPoint3&p1,const ExactPoint3&p2){ const ExactPoint3 n=cross_exact(p1-p0,p2-p0); if(n.x.is_zero()&&n.y.is_zero()&&n.z.is_zero()) throw std::invalid_argument("ExactPlane3::from_points: degenerate plane."); return{n.x,n.y,n.z,-dot_exact(n,p0)}; }
ExactScalar ExactPlane3::eval(const ExactPoint3&p) const{return a*p.x+b*p.y+c*p.z+d;}

bool ygor_multiprecision_self_test(){ ExactInt a=1; a<<=80; ExactInt b=1; b<<=40; if((a/b)!=(ExactInt(1)<<40)) return false; if(gcd_exact_int(ExactInt(48),ExactInt(18))!=ExactInt(6)) return false; if(ExactScalar(ExactInt(2),ExactInt(4))!=ExactScalar(1)/ExactScalar(2)) return false; const auto p=ExactPlane3::from_points({0,0,0},{1,0,0},{0,1,0}); return p.eval({0,0,1}).sign()>0; }
