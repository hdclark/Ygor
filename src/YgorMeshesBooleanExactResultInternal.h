#pragma once
#ifndef YGOR_MESHES_BOOLEAN_EXACT_RESULT_INTERNAL_H_
#define YGOR_MESHES_BOOLEAN_EXACT_RESULT_INTERNAL_H_

#include "YgorMeshesBooleanExactResult.h"

namespace ygor {
namespace mesh_boolean {
namespace detail {

product_status_or<bool>
finalize_exact_stratified_boundary(exact_stratified_boundary &);

} // namespace detail
} // namespace mesh_boolean
} // namespace ygor

#endif
