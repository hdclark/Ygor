#include "MeshBooleanAnalyticFixtures.h"
#include "MeshBooleanTestHarness.h"
#include <YgorMeshesBooleanVerification.h>

#include <iostream>
#include <string>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

template <class T, class I>
void require_archive_round_trip(
    const published_artifact<selected_exact_boundary<T, I>> &selected,
    const boolean_context<T, I> &context, const std::string &case_name) {
  const auto encoded = encode_replay_archive(
      selection_test::replay_archive(selected, context));
  require(encoded.has_value(), case_name + " replay archive encodes");
  const auto decoded = decode_replay_archive(encoded.value());
  require(decoded.has_value(), case_name + " replay archive decodes");
  const auto reencoded = encode_replay_archive(decoded.value());
  require(reencoded.has_value(), case_name + " replay archive re-encodes");
  require_equal(reencoded.value(), encoded.value(),
                case_name + " replay archive is canonical");
}

template <class T, class I>
void replay_touching_fixture(T x, T y, T z, const std::string &case_name) {
  for (unsigned presentation = 0; presentation < 2; ++presentation) {
    for (const auto op : {operation::regularized_union,
                          operation::regularized_intersection,
                          operation::a_minus_b, operation::b_minus_a,
                          operation::symmetric_difference}) {
      auto run = [&] {
        auto a = input_test::cube<T, I>();
        auto b = input_test::cube<T, I>();
        symbolic_test::translate(b, x, y, z);
        if (presentation != 0) std::swap(a, b);
        auto context = classification_test::context(
            a, b, output_test::registry(), op);
        const auto selected = select_boolean_boundary(*context);
        require(selected.has_value(), case_name + " selection replays");
        const auto output = assemble_boolean_output(*context);
        return std::make_pair(
            std::move(context),
            std::make_pair(selected.value(), output));
      };

      auto first = run();
      require_archive_round_trip(*first.second.first, *first.first, case_name);
      auto second = run();
      require_equal(first.second.first->payload->canonical_bytes,
                    second.second.first->payload->canonical_bytes,
                    case_name + " selected boundary replays canonically");
      require_equal(first.second.second.has_value(),
                    second.second.second.has_value(),
                    case_name + " publication outcome replays");
      if (first.second.second.has_value()) {
        require_equal(first.second.second.value()->canonical_output_digest,
                      second.second.second.value()->canonical_output_digest,
                      case_name + " canonical output replays");
      } else {
        require_equal(first.second.second.error().code,
                      second.second.second.error().code,
                      case_name + " typed failure replays");
        require_equal(first.second.second.error().stage,
                      second.second.second.error().stage,
                      case_name + " failure stage replays");
      }
    }
  }
}

template <class T, class I> void replay_one_third_fixture() {
  for (unsigned presentation = 0; presentation < 2; ++presentation) {
    auto run = [&] {
      auto a = input_test::cube<T, I>();
      auto b = input_test::third_intersection_prism<T, I>();
      if (presentation != 0) std::swap(a, b);
      auto context = classification_test::context(
          a, b, realization_test::registry(),
          operation::regularized_intersection);
      const auto selected = select_boolean_boundary(*context);
      require(selected.has_value(), "one-third selection replays");
      const auto realized = realize_selected_boundary(*context);
      return std::make_pair(
          std::move(context),
          std::make_pair(selected.value(), realized));
    };

    auto first = run();
    require_archive_round_trip(*first.second.first, *first.first, "one-third");
    auto second = run();
    require_equal(first.second.first->payload->canonical_bytes,
                  second.second.first->payload->canonical_bytes,
                  "one-third selected boundary replays canonically");
    require(!first.second.second.has_value() &&
                !second.second.second.has_value(),
            "one-third replay remains unrepresentable");
    require_equal(first.second.second.error().code,
                  boolean_error_code::output_not_representable,
                  "one-third replay preserves typed failure");
    require_equal(first.second.second.error().code,
                  second.second.second.error().code,
                  "one-third typed failure replays");
  }
}

template <class T, class I> void replay_plan_gap_fixtures_for_type() {
  replay_touching_fixture<T, I>(T(1), T(1), T(1), "vertex-touching");
  replay_touching_fixture<T, I>(T(1), T(1), T(0), "edge-touching");
  replay_one_third_fixture<T, I>();
}

} // namespace

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
  tests.add("C14.REPLAY.corpus.plan_gap_fixtures", [] {
    replay_plan_gap_fixtures_for_type<float, std::uint32_t>();
    replay_plan_gap_fixtures_for_type<float, std::uint64_t>();
    replay_plan_gap_fixtures_for_type<double, std::uint32_t>();
    replay_plan_gap_fixtures_for_type<double, std::uint64_t>();
  });
  return tests.run(std::cout, std::cerr);
}
