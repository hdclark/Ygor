#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

#if __cplusplus < 201703L
#error "Mesh Boolean qualification requires C++17"
#endif
#ifdef __FAST_MATH__
#error "Mesh Boolean qualification must not be compiled with fast-math"
#endif

namespace ygor::mesh_boolean::testing {

class assertion_failure final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

struct test_case {
  std::string id;
  std::function<void()> body;
};

class harness {
  std::vector<test_case> cases_;

public:
  void add(std::string id, std::function<void()> body);
  int run(std::ostream &out, std::ostream &error) const;
};

void require(bool condition, const std::string &message);

template <class A, class B>
void require_equal(const A &actual, const B &expected,
                   const std::string &message) {
  if (!(actual == expected))
    throw assertion_failure(message);
}

} // namespace ygor::mesh_boolean::testing
