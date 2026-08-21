#include "YgorMeshesBooleanBounded/DeterministicExecution.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace bounded = ygor::mesh_boolean::bounded;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

using error_t = bounded_boolean_error;

std::uint64_t serial_sum(bounded::execution_capabilities capabilities) {
  std::uint64_t total = 0;
  const std::uint64_t n = 1 << 16;
  auto worker = [&](std::uint64_t begin, std::uint64_t end,
                    const bounded_boolean_cancellation_token &) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = begin; i < end; ++i)
      sum += i;
    return sum;
  };
  auto reduce = [](std::uint64_t acc, const std::uint64_t &next) {
    return acc + next;
  };
  auto result = bounded::execute_parallel(n, worker, reduce, std::uint64_t(0),
                                          capabilities);
  require(result.has_value(), "parallel execution succeeded");
  return *result.value();
}

void test_serial_equivalence() {
  bounded::resource_manager resources(resource_policy::conservative_defaults());

  // Serial reference.
  bounded::execution_capabilities serial;
  serial.maximum_workers = 0;
  serial.resources = &resources;
  const std::uint64_t reference = serial_sum(serial);

  // Parallel with 1, 2, and 4 workers must equal the serial reference.
  for (std::uint32_t workers : {1u, 2u, 4u}) {
    bounded::execution_capabilities parallel;
    parallel.maximum_workers = workers;
    parallel.requested_workers = workers;
    parallel.resources = &resources;
    require(serial_sum(parallel) == reference,
            "parallel sum equals the serial reference");
  }
}

void test_canonical_order() {
  bounded::resource_manager resources(resource_policy::conservative_defaults());

  // Non-commutative reduce: build a string of per-item digits. The reduce is
  // applied in ascending task order, so the parallel result must equal the
  // serial concatenation byte-for-byte.
  const std::uint64_t n = 1000;
  auto worker = [](std::uint64_t begin, std::uint64_t end,
                   const bounded_boolean_cancellation_token &) {
    std::string out;
    for (std::uint64_t i = begin; i < end; ++i)
      out += static_cast<char>('a' + (i % 26));
    return out;
  };
  auto reduce = [](std::string acc, const std::string &next) {
    return acc + next;
  };

  std::string expected;
  expected.reserve(n);
  for (std::uint64_t i = 0; i < n; ++i)
    expected += static_cast<char>('a' + (i % 26));

  bounded::execution_capabilities parallel;
  parallel.maximum_workers = 4;
  parallel.requested_workers = 4;
  parallel.resources = &resources;
  auto result = bounded::execute_parallel(n, worker, reduce, std::string{},
                                          parallel);
  require(result.has_value(), "string execution succeeded");
  require(*result.value() == expected,
          "parallel string concatenation preserves canonical order");
}

void test_determinism() {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::execution_capabilities parallel;
  parallel.maximum_workers = 4;
  parallel.requested_workers = 4;
  parallel.resources = &resources;
  require(serial_sum(parallel) == serial_sum(parallel),
          "parallel execution is deterministic across runs");
}

void test_cancellation() {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded_boolean_cancellation_source source;
  const auto token = source.token();
  const std::uint64_t n = 1 << 20;
  auto worker = [&](std::uint64_t begin, std::uint64_t end,
                    const bounded_boolean_cancellation_token &) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = begin; i < end; ++i)
      sum += i;
    return sum;
  };
  auto reduce = [](std::uint64_t acc, const std::uint64_t &next) {
    return acc + next;
  };
  bounded::execution_capabilities capabilities;
  capabilities.maximum_workers = 4;
  capabilities.requested_workers = 4;
  capabilities.resources = &resources;
  capabilities.cancellation = &token;
  source.request_cancel();
  auto result = bounded::execute_parallel(n, worker, reduce, std::uint64_t(0),
                                          capabilities);
  require(!result.has_value(), "pre-cancelled execution returns failure");
  require(result.error()->category ==
              bounded_boolean_error_category::cancelled,
          "pre-cancelled execution reports cancellation");
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "serial")
      test_serial_equivalence();
    if (suite == "all" || suite == "order")
      test_canonical_order();
    if (suite == "all" || suite == "determinism")
      test_determinism();
    if (suite == "all" || suite == "cancellation")
      test_cancellation();
    std::cout << "Component 17 execution suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
