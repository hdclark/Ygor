#include "MeshBooleanAnalyticFixtures.h"
#include "MeshBooleanTestHarness.h"
#include <YgorMeshesBooleanVerification.h>

#include <iostream>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

int main() {
  harness tests;
  tests.add("C14.REPLAY.archive.codec", [] {
    auto a = input_test::box<double, std::uint32_t>(0, 1);
    auto b = input_test::box<double, std::uint32_t>(3, 4);
    auto context = classification_test::context(
        a, b, selection_test::registry(), operation::regularized_union);
    const auto selected = select_boolean_boundary(*context);
    require(selected.has_value(), "replay source selection succeeds");
    const auto archive = selection_test::replay_archive(*selected.value(), *context);
    const auto encoded = encode_replay_archive(archive);
    require(encoded.has_value(), "replay archive encodes");
    const auto decoded = decode_replay_archive(encoded.value());
    require(decoded.has_value(), "replay archive decodes");
    const auto reencoded = encode_replay_archive(decoded.value());
    require(reencoded.has_value(), "decoded replay re-encodes");
    require_equal(reencoded.value(), encoded.value(), "replay codec is canonical");
    auto malformed = encoded.value();
    malformed.push_back(0);
    require(!decode_replay_archive(malformed).has_value(),
            "replay codec rejects trailing bytes");
  });
  tests.add("C14.REPLAY.semantic.rerun", [] {
    const auto first = run_box_operation<double, std::uint32_t>(
        0, 1, 3, 4, operation::regularized_union);
    const auto second = run_box_operation<double, std::uint32_t>(
        0, 1, 3, 4, operation::regularized_union);
    require_equal(first->payload->canonical_bytes,
                  second->payload->canonical_bytes,
                  "fresh reconstruction reproduces semantic output");
    require_equal(first->report.report_digest, second->report.report_digest,
                  "fresh reconstruction reproduces verification report");
  });
  return tests.run(std::cout, std::cerr);
}
