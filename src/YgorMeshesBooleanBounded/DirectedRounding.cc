#include "DirectedRounding.h"

namespace ygor::mesh_boolean::bounded {
template directed_operation_result<float> directed_add(float, float) noexcept;
template directed_operation_result<double> directed_add(double, double) noexcept;
template directed_operation_result<float> directed_subtract(float, float) noexcept;
template directed_operation_result<double> directed_subtract(double, double) noexcept;
template directed_operation_result<float> directed_multiply(float, float) noexcept;
template directed_operation_result<double> directed_multiply(double, double) noexcept;
template directed_operation_result<float> directed_divide(float, float) noexcept;
template directed_operation_result<double> directed_divide(double, double) noexcept;
} // namespace ygor::mesh_boolean::bounded
