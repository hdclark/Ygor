#pragma once
#include <YgorMeshesBooleanInputTopology.h>
#include <memory>
#include <stdexcept>
namespace input_test {
using namespace ygor;using namespace ygor::mesh_boolean;
inline void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
template<class T,class I>fv_surface_mesh<T,I>tetra(){fv_surface_mesh<T,I>m;m.vertices={{T(0),T(0),T(0)},{T(1),T(0),T(0)},{T(0),T(1),T(0)},{T(0),T(0),T(1)}};m.faces={{I(0),I(2),I(1)},{I(0),I(1),I(3)},{I(1),I(2),I(3)},{I(2),I(0),I(3)}};return m;}
template<class T,class I>fv_surface_mesh<T,I>cube(){fv_surface_mesh<T,I>m;m.vertices={{T(0),T(0),T(0)},{T(1),T(0),T(0)},{T(1),T(1),T(0)},{T(0),T(1),T(0)},{T(0),T(0),T(1)},{T(1),T(0),T(1)},{T(1),T(1),T(1)},{T(0),T(1),T(1)}};m.faces={{I(0),I(3),I(2),I(1)},{I(4),I(5),I(6),I(7)},{I(0),I(1),I(5),I(4)},{I(1),I(2),I(6),I(5)},{I(2),I(3),I(7),I(6)},{I(3),I(0),I(4),I(7)}};return m;}
template<class T,class I>fv_surface_mesh<T,I>third_intersection_prism(){fv_surface_mesh<T,I>m;m.vertices={{T(0),T(0),T(0)},{T(1),T(3),T(0)},{T(0),T(3),T(0)},{T(0),T(0),T(1)},{T(1),T(3),T(1)},{T(0),T(3),T(1)}};m.faces={{I(0),I(2),I(1)},{I(3),I(4),I(5)},{I(0),I(1),I(4),I(3)},{I(1),I(2),I(5),I(4)},{I(2),I(0),I(3),I(5)}};return m;}
template<class T,class I>fv_surface_mesh<T,I>box(T lo,T hi){auto m=cube<T,I>();for(auto&v:m.vertices){v.x=v.x==T(0)?lo:hi;v.y=v.y==T(0)?lo:hi;v.z=v.z==T(0)?lo:hi;}return m;}
template<class T,class I>void append(fv_surface_mesh<T,I>&a,const fv_surface_mesh<T,I>&b,bool reverse=false){I base=static_cast<I>(a.vertices.size());a.vertices.insert(a.vertices.end(),b.vertices.begin(),b.vertices.end());for(auto f:b.faces){for(auto&v:f)v=static_cast<I>(v+base);if(reverse)std::reverse(f.begin(),f.end());a.faces.push_back(std::move(f));}}
template<class T,class I>fv_surface_mesh<T,I>prism(bool collinear){fv_surface_mesh<T,I>m;std::vector<std::array<T,2>>p={{{T(0),T(0)}},{{T(2),T(0)}},{{T(2),T(1)}},{{T(1),T(1)}},{{T(1),T(2)}},{{T(0),T(2)}}};if(collinear)p.insert(p.begin()+1,{{T(1),T(0)}});for(T z:{T(0),T(1)})for(auto q:p)m.vertices.push_back({q[0],q[1],z});std::vector<I>bottom,top;for(std::size_t i=0;i<p.size();++i){bottom.push_back(static_cast<I>(p.size()-1-i));top.push_back(static_cast<I>(p.size()+i));}m.faces.push_back(bottom);m.faces.push_back(top);for(std::size_t i=0;i<p.size();++i)m.faces.push_back({static_cast<I>(i),static_cast<I>((i+1)%p.size()),static_cast<I>(p.size()+(i+1)%p.size()),static_cast<I>(p.size()+i)});return m;}
inline std::shared_ptr<verifier_registry>registry(){auto r=std::make_shared<verifier_registry>();for(auto c:{coordinate_tag::binary32,coordinate_tag::binary64})for(auto i:{index_tag::uint32,index_tag::uint64})require(register_input_topology_verifier(*r,c,i).has_value(),"register");require(r->freeze().has_value(),"freeze");return r;}
template<class T,class I>auto context(fv_surface_mesh<T,I>&a,fv_surface_mesh<T,I>&b,std::shared_ptr<verifier_registry>r,operation op=operation::regularized_union){std::shared_ptr<const exact_kernel_services<T>>k=std::make_shared<exact_kernel<T>>();std::shared_ptr<const verifier_service>v=r;auto c=make_boolean_context(a,b,op,boolean_options{},k,v);require(c.has_value(),"context");return std::move(c.value());}
}
