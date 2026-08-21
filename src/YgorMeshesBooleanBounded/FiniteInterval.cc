#include "FiniteInterval.h"

namespace ygor::mesh_boolean::bounded {
template class finite_interval<float>;
template class finite_interval<double>;
template interval_operation_result<float> interval_add(const finite_interval<float> &, const finite_interval<float> &) noexcept;
template interval_operation_result<double> interval_add(const finite_interval<double> &, const finite_interval<double> &) noexcept;
template interval_operation_result<float> interval_subtract(const finite_interval<float> &, const finite_interval<float> &) noexcept;
template interval_operation_result<double> interval_subtract(const finite_interval<double> &, const finite_interval<double> &) noexcept;
template interval_operation_result<float> interval_multiply(const finite_interval<float> &, const finite_interval<float> &) noexcept;
template interval_operation_result<double> interval_multiply(const finite_interval<double> &, const finite_interval<double> &) noexcept;
template interval_operation_result<float> interval_divide(const finite_interval<float> &, const finite_interval<float> &) noexcept;
template interval_operation_result<double> interval_divide(const finite_interval<double> &, const finite_interval<double> &) noexcept;
} // namespace ygor::mesh_boolean::bounded
