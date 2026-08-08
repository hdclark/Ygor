#pragma once

#include "CanonicalBytes.h"
#include "Outcome.h"
#include "Sha256.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {
enum class boundary_orientation : std::uint8_t { not_applicable=0, preserve=1, reverse=2 };
struct side_occupancy { bool a_negative,b_negative,a_positive,b_positive; };
struct truth_cell {
    bool result_negative=false, result_positive=false, retain=false;
    boundary_orientation orientation=boundary_orientation::not_applicable;
    std::uint8_t multiplicity=0, owner_priority=0;
};
inline bool operation_value(boolean_operation op,bool a,bool b) noexcept {
    switch(op) {
        case boolean_operation::set_union:return a||b;
        case boolean_operation::intersection:return a&&b;
        case boolean_operation::a_minus_b:return a&&!b;
        case boolean_operation::b_minus_a:return b&&!a;
        case boolean_operation::symmetric_difference:return a!=b;
    }
    return false;
}
inline boolean_operation swap_operands(boolean_operation op) noexcept {
    if(op==boolean_operation::a_minus_b)return boolean_operation::b_minus_a;
    if(op==boolean_operation::b_minus_a)return boolean_operation::a_minus_b;
    return op;
}
inline truth_cell evaluate_truth(boolean_operation op,side_occupancy s) noexcept {
    truth_cell c;
    c.result_negative=operation_value(op,s.a_negative,s.b_negative);
    c.result_positive=operation_value(op,s.a_positive,s.b_positive);
    c.retain=c.result_negative!=c.result_positive;
    c.orientation=!c.retain?boundary_orientation::not_applicable:(c.result_negative?boundary_orientation::preserve:boundary_orientation::reverse);
    c.multiplicity=c.retain?1:0;
    c.owner_priority=op==boolean_operation::b_minus_a?1:0;
    return c;
}
struct truth_table {
    std::array<truth_cell,80> cells{};
    std::vector<std::uint8_t> bytes;
    bounded_boolean_digest digest{};
};
inline truth_table materialize_truth_table() {
    truth_table table; canonical_writer w; std::size_t n=0;
    for(std::uint8_t o=1;o<=5;++o) for(std::uint8_t bits=0;bits<16;++bits) {
        side_occupancy s{bool(bits&1),bool(bits&2),bool(bits&4),bool(bits&8)};
        auto c=evaluate_truth(static_cast<boolean_operation>(o),s); table.cells[n++]=c;
        w.u8(o);w.u8(bits);w.boolean(c.result_negative);w.boolean(c.result_positive);w.boolean(c.retain);w.u8(static_cast<std::uint8_t>(c.orientation));w.u8(c.multiplicity);w.u8(c.owner_priority);
    }
    table.bytes=w.take();table.digest=sha256::digest(table.bytes);return table;
}
inline bounded_boolean_error policy_error(std::uint32_t code,bounded_boolean_error_category category=bounded_boolean_error_category::input_contract_error) {
    bounded_boolean_error e;e.category=category;e.subcode=code;e.stage=3;e.summary="bounded Boolean option validation failed";return e;
}
template<class T>
boolean_outcome<bounded_boolean_options<T>> normalize_options(bounded_boolean_options<T> value) {
    if(!std::isfinite(value.tolerance)||!std::isfinite(value.input_precision_a)||!std::isfinite(value.input_precision_b)||value.tolerance<T(0)||value.input_precision_a<T(0)||value.input_precision_b<T(0))
        return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2001,bounded_boolean_error_category::invalid_tolerance));
    if(value.input_precision_a>value.tolerance||value.input_precision_b>value.tolerance)
        return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2002,bounded_boolean_error_category::invalid_tolerance));
    if(value.solids.version!=1||value.contacts.version!=1||value.output.version!=1||value.verification.version!=1||value.determinism.version!=1||value.execution.version!=1||value.resources.version!=1||value.diagnostics.version!=1)
        return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2003));
    if(value.solids.reserved||value.contacts.reserved||value.output.reserved||value.verification.reserved||value.determinism.reserved||value.execution.reserved||value.resources.reserved||value.diagnostics.reserved)
        return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2004));
    if(value.output.preserve_public_metadata)return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2005));
    if(value.solids.kind!=solid_policy_kind::outward_oriented_alternating_shells_v1||value.contacts.kind!=contact_policy_kind::regularized_symbolic_v1||value.output.kind!=output_policy_kind::triangulated_oriented_manifold_v1||
       (value.verification.level!=verification_level::mandatory_scalable_v1&&value.verification.level!=verification_level::exhaustive_diagnostics_v1)||value.determinism.mode!=determinism_mode::canonical_v1||
       (value.execution.mode!=bounded_execution_mode::serial_v1&&value.execution.mode!=bounded_execution_mode::deterministic_parallel_v1)||
       (value.diagnostics.replay!=replay_retention::digest_only&&value.diagnostics.replay!=replay_retention::full_on_failure&&value.diagnostics.replay!=replay_retention::full_always))
        return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2007));
    const resource_limit_policy limits[]{value.resources.persistent_bytes,value.resources.temporary_bytes,value.resources.source_vertices,value.resources.source_faces,value.resources.source_indices,value.resources.work_units};
    for(auto l:limits)if(l.advisory>l.hard)return boolean_outcome<bounded_boolean_options<T>>::failure(policy_error(2006));
    if(value.tolerance==T(0)) value.tolerance=T(0);
    if(value.input_precision_a==T(0)) value.input_precision_a=T(0);
    if(value.input_precision_b==T(0)) value.input_precision_b=T(0);
    return boolean_outcome<bounded_boolean_options<T>>::success(std::move(value));
}
}
