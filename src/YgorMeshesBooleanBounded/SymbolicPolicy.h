#pragma once

#include "ContractVersions.h"
#include "Policies.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {
enum class relation_family : std::uint8_t { vertex_vertex=1,vertex_edge=2,vertex_face=3,edge_edge=4,edge_face=5,equal_edge=6,tangent=7,coplanar=8,coincident_face=9 };
enum class orientation_relation : std::uint8_t { opposite=1,indeterminate=2,same=3 };
struct symbolic_rule { boolean_operation operation;operand_id acting_operand;relation_family relation;orientation_relation orientation;std::uint8_t feature_priority;operand_id half_open_owner;std::int8_t crossing_contribution;operand_id coincident_owner; };
struct symbolic_policy_table { std::vector<symbolic_rule> rules;std::vector<std::uint8_t> bytes;bounded_boolean_digest digest{}; };
inline symbolic_policy_table materialize_symbolic_policy() {
    symbolic_policy_table table;table.rules.reserve(270);canonical_writer w;
    for(std::uint8_t operation=1;operation<=5;++operation)for(std::uint8_t operand=0;operand<2;++operand)for(std::uint8_t family=1;family<=9;++family)for(std::uint8_t orientation=1;orientation<=3;++orientation){
        const auto op=static_cast<boolean_operation>(operation);const auto acting=static_cast<operand_id>(operand);const auto preferred=op==boolean_operation::b_minus_a?operand_id::b:operand_id::a;
        symbolic_rule rule{op,acting,static_cast<relation_family>(family),static_cast<orientation_relation>(orientation),family,acting,static_cast<std::int8_t>(orientation==2?0:(orientation==1?-1:1)),preferred};
        table.rules.push_back(rule);w.u8(operation);w.u8(operand);w.u8(family);w.u8(orientation);w.u8(rule.feature_priority);w.u8(static_cast<std::uint8_t>(rule.half_open_owner));w.u8(static_cast<std::uint8_t>(rule.crossing_contribution));w.u8(static_cast<std::uint8_t>(rule.coincident_owner));
    }
    table.bytes=w.take();table.digest=sha256::digest(table.bytes);return table;
}
inline bool verify_symbolic_policy(const symbolic_policy_table &table) {
    if(table.rules.size()!=270||sha256::digest(table.bytes)!=table.digest)return false;
    std::size_t i=0;for(std::uint8_t operation=1;operation<=5;++operation)for(std::uint8_t operand=0;operand<2;++operand)for(std::uint8_t family=1;family<=9;++family)for(std::uint8_t orientation=1;orientation<=3;++orientation,++i){const auto&r=table.rules[i];if(static_cast<std::uint8_t>(r.operation)!=operation||static_cast<std::uint8_t>(r.acting_operand)!=operand||static_cast<std::uint8_t>(r.relation)!=family||static_cast<std::uint8_t>(r.orientation)!=orientation)return false;}return true;
}
}
