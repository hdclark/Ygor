#pragma once
#include <YgorMeshesExactKernel.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace exact_test {
using namespace ygor::mesh_boolean;
inline void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
inline exact_rational q(std::int64_t n,std::uint64_t d=1){return exact_rational(big_int(n),big_uint(d));}
inline exact_point2 p2(std::int64_t x,std::int64_t y){return{q(x),q(y)};}inline exact_point3 p3(std::int64_t x,std::int64_t y,std::int64_t z){return{q(x),q(y),q(z)};}
template<class T,class U>T from_bits(U u){T v;std::memcpy(&v,&u,sizeof(v));return v;}
struct prng{std::uint64_t state=0x9e3779b97f4a7c15ULL;std::uint64_t next(){state^=state>>12;state^=state<<25;state^=state>>27;return state*2685821657736338717ULL;}};
}
