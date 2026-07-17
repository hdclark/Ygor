#include "MeshBooleanExactKernelFixtures.h"
#include <algorithm>
#include <array>
#include <iostream>
using namespace exact_test;
int main(){try{
 prng r;
 for(unsigned i=0;i<1000;++i){
  std::int64_t a=std::int64_t(r.next()%2001)-1000,b=std::int64_t(r.next()%2001)-1000,c=std::int64_t(r.next()%2001)-1000,d=std::int64_t(r.next()%2001)-1000;
  auto A=big_int(a),B=big_int(b),C=big_int(c);require((A+B)+C==A+(B+C),"integer associativity");
  auto x=q(a,std::uint64_t(r.next()%31+1)),y=q(b,std::uint64_t(r.next()%31+1));require((x+y)-y==x,"rational inverse");
  auto p=p2(a,b),u=p2(b,c),v=p2(c,a);const auto exact2=orient2d_exact(p,u,v);require(orient2d(p,u,v)==exact2&&int(exact2)==-int(orient2d_exact(p,v,u)),"orientation2 differential");
  for(auto tag:{coordinate_tag::binary32,coordinate_tag::binary64})if(auto filtered=exact_filter_policy::orient2d(p,u,v,tag))require(*filtered==exact2,"orient2d zero false accepts");
  auto w=p3(a,b,c),i1=p3(b,c,d),i2=p3(c,d,a),i3=p3(d,a,b);const auto exact3=orient3d_exact(w,i1,i2,i3);require(orient3d(w,i1,i2,i3)==exact3,"orientation3 differential");
  for(auto tag:{coordinate_tag::binary32,coordinate_tag::binary64})if(auto filtered=exact_filter_policy::orient3d(w,i1,i2,i3,tag))require(*filtered==exact3,"orient3d zero false accepts");
 }
 std::array<exact_point3,4> tetra{{p3(0,0,0),p3(2,0,0),p3(0,3,0),p3(0,0,5)}};std::array<unsigned,4> permutation{{0,1,2,3}};
 do{unsigned inversions=0;for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)inversions+=permutation[i]>permutation[j];const auto sign=orient3d(tetra[permutation[0]],tetra[permutation[1]],tetra[permutation[2]],tetra[permutation[3]]);require(sign==(inversions%2?exact_sign::negative:exact_sign::positive),"orient3d permutation parity");}while(std::next_permutation(permutation.begin(),permutation.end()));
 for(std::int64_t ax=-2;ax<=2;++ax)for(std::int64_t ay=-2;ay<=2;++ay)for(std::int64_t bx=-2;bx<=2;++bx)for(std::int64_t by=-2;by<=2;++by){const auto a=p2(0,0),b=p2(ax,ay),c=p2(bx,by);if(auto filtered=exact_filter_policy::orient2d(a,b,c,coordinate_tag::binary64))require(*filtered==orient2d_exact(a,b,c),"bounded orient2d differential");}
 for(std::uint32_t bits: {0U,1U,0x007fffffU,0x00800000U,0x3f7fffffU,0x3f800000U,0x7f7fffffU,0x80000000U}){auto decoded=decode_coordinate(from_bits<float>(bits));require(decoded.has_value(),"finite corpus decode");}
 auto overlap=relate_segments(exact_segment3{p3(0,0,0),p3(4,0,0)},exact_segment3{p3(2,0,0),p3(6,0,0)});require(overlap.dimension==intersection_dimension::segment,"axis-aligned 3D overlap");
 std::cout<<"PASS exact properties\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
