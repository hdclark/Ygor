#pragma once

#include "../YgorMeshesBooleanBounded.h"

#include <algorithm>
#include <tuple>

namespace ygor::mesh_boolean::bounded {
inline auto error_key(const bounded_boolean_error&e)noexcept{return std::make_tuple(e.stage,e.checkpoint,static_cast<std::uint8_t>(e.category),e.component,e.subcode,e.witnesses);}
inline const bounded_boolean_error&select_primary(const bounded_boolean_error&a,const bounded_boolean_error&b)noexcept{return error_key(a)<=error_key(b)?a:b;}
}
