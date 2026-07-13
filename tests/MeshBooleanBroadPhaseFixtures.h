#pragma once
#include "MeshBooleanInputTopologyFixtures.h"
#include <YgorMeshesBooleanBroadPhase.h>
namespace broad_test {
using namespace input_test;
inline std::shared_ptr<verifier_registry>registry(){auto r=std::make_shared<verifier_registry>();for(auto c:{coordinate_tag::binary32,coordinate_tag::binary64})for(auto i:{index_tag::uint32,index_tag::uint64}){require(register_input_topology_verifier(*r,c,i).has_value(),"input register");require(register_broad_phase_verifier(*r,c,i).has_value(),"broad register");}require(r->freeze().has_value(),"freeze");return r;}
template<class T,class I>std::vector<facet_candidate_key>exhaustive(const validated_operands<T,I>&v){std::vector<facet_candidate_key>out;for(const auto&a:v.facets)if(a.operand==operand_a())for(const auto&b:v.facets)if(b.operand==operand_b()){exact_feature_bound3 x{{a.bounds.minimum.x,a.bounds.maximum.x,true,true},{a.bounds.minimum.y,a.bounds.maximum.y,true,true},{a.bounds.minimum.z,a.bounds.maximum.z,true,true}},y{{b.bounds.minimum.x,b.bounds.maximum.x,true,true},{b.bounds.minimum.y,b.bounds.maximum.y,true,true},{b.bounds.minimum.z,b.bounds.maximum.z,true,true}};if(exact_bounds_overlap(x,y))out.push_back({a.id,b.id});}return out;}
}
