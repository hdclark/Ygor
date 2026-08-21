#include "YgorMeshesBooleanBounded/DeterministicSeed.h"
#include "YgorMeshesBooleanBounded/QualificationHarness.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace bounded = ygor::mesh_boolean::bounded;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void test_seed_determinism() {
  bounded::deterministic_seed stream(0x123456789ABCDEF0ULL);
  const std::uint64_t first = stream.next(0);
  const std::uint64_t second = stream.next(1);
  require(first != second, "seed stream produces distinct values");
  // Reconstructing the stream reproduces identical values.
  bounded::deterministic_seed again(0x123456789ABCDEF0ULL);
  require(again.next(0) == first && again.next(1) == second,
          "seed stream is deterministic");
  // Independent domain roots differ.
  const std::uint64_t a = bounded::derive_seed_root(42, "generator");
  const std::uint64_t b = bounded::derive_seed_root(42, "mutation");
  require(a != b, "domain-separated roots differ");
}

void test_harness_release() {
  bounded::qualification_registry registry;
  bounded::qualification_test_descriptor descriptor;
  descriptor.component = 1;
  require(registry.register_test("MBQ.C01.unit.alpha.V1", descriptor,
                                 [] { return true; }),
          "first test registers");
  require(registry.register_test("MBQ.C01.unit.beta.V1", descriptor,
                                 [] { return true; }),
          "second test registers");
  require(!registry.register_test("MBQ.C01.unit.alpha.V1", descriptor,
                                  [] { return true; }),
          "duplicate test ID is rejected");

  auto harness = bounded::run_qualification(registry,
                                            bounded::qualification_scope::release);
  require(harness.complete, "harness completes");
  require(harness.evidence.decision.passed,
          "all-passing release decision passes");
  require(harness.evidence.decision.total_tests == 2 &&
              harness.evidence.decision.passed_tests == 2,
          "release decision counts all tests");
}

void test_harness_fail_closed() {
  bounded::qualification_registry registry;
  bounded::qualification_test_descriptor descriptor;
  descriptor.component = 1;
  registry.register_test("MBQ.C01.unit.good.V1", descriptor,
                         [] { return true; });
  registry.register_test("MBQ.C01.unit.bad.V1", descriptor,
                         [] { return false; });
  auto harness = bounded::run_qualification(registry,
                                            bounded::qualification_scope::release);
  require(harness.evidence.decision.total_tests == 2 &&
              harness.evidence.decision.failed_tests == 1,
          "failing test is counted");
  require(!harness.evidence.decision.passed &&
              harness.evidence.decision.reason ==
                  bounded::release_rejection_reason::required_test_failed,
          "a failing test rejects release");
}

void test_harness_digest_determinism() {
  bounded::qualification_registry registry;
  bounded::qualification_test_descriptor descriptor;
  descriptor.component = 1;
  registry.register_test("MBQ.C01.unit.one.V1", descriptor,
                         [] { return true; });
  registry.register_test("MBQ.C01.unit.two.V1", descriptor,
                         [] { return true; });
  auto first = bounded::run_qualification(registry,
                                          bounded::qualification_scope::release);
  auto second = bounded::run_qualification(registry,
                                           bounded::qualification_scope::release);
  require(first.evidence.digest == second.evidence.digest,
          "qualification evidence digest is deterministic");
  require(first.evidence.canonical_bytes == second.evidence.canonical_bytes,
          "qualification evidence bytes are deterministic");
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "seed")
      test_seed_determinism();
    if (suite == "all" || suite == "release")
      test_harness_release();
    if (suite == "all" || suite == "fail_closed")
      test_harness_fail_closed();
    if (suite == "all" || suite == "digest")
      test_harness_digest_determinism();
    std::cout << "Component 16 qualification suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
