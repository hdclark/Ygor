#include "MeshBooleanCellClassificationFixtures.h"

#include <cmath>
#include <iostream>
#include <limits>

using namespace classification_test;

namespace {
template <class T, class I> void numeric_cases() {
  const T denorm = std::numeric_limits<T>::denorm_min();
  run_analytic_classification_case<T, I>(classification_test::registry(), -T(0), denorm,
                                         T(4) * denorm, T(5) * denorm);

  const T one = T(1);
  const T next = std::nextafter(one, T(2));
  run_analytic_classification_case<T, I>(classification_test::registry(), T(0), one, next,
                                         std::nextafter(next, T(2)));

  const int exponent = std::numeric_limits<T>::max_exponent / 2;
  const T scale = std::ldexp(T(1), exponent);
  run_analytic_classification_case<T, I>(classification_test::registry(), -scale, T(0), scale,
                                         T(2) * scale);
}
} // namespace

int main() {
  try {
    numeric_cases<float, std::uint32_t>();
    numeric_cases<float, std::uint64_t>();
    numeric_cases<double, std::uint32_t>();
    numeric_cases<double, std::uint64_t>();
    std::cout << "ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
