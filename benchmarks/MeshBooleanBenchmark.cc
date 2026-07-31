#include "MeshBooleanPerformanceSupport.h"
#include "MeshBooleanExactArithmeticFixtures.h"
#include <YgorMeshesBooleanPerformance.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

struct command_line {
  std::vector<std::string> fixtures{"B0"};
  std::uint32_t size = 1, threads = 1, warmups = 1, repetitions = 5;
  std::uint32_t arithmetic_limbs = 8;
  std::string suite = "mesh";
  std::string arithmetic_case = "all";
  std::string operation_name = "all";
  std::string verification_name = "mandatory";
  std::string type_name = "double-u32";
};

std::string value_after(const std::string &argument, const std::string &name,
                        int &index, int argc, char **argv) {
  const auto prefix = name + "=";
  if (argument.compare(0, prefix.size(), prefix) == 0)
    return argument.substr(prefix.size());
  if (argument == name && index + 1 < argc)
    return argv[++index];
  throw std::invalid_argument("missing value for " + name);
}

std::uint32_t unsigned_value(const std::string &text, const char *name,
                             bool allow_zero = false) {
  std::size_t consumed = 0;
  const auto value = std::stoull(text, &consumed);
  if (consumed != text.size() || value > std::numeric_limits<std::uint32_t>::max() ||
      (!allow_zero && value == 0))
    throw std::invalid_argument(std::string("invalid ") + name + ": " + text);
  return static_cast<std::uint32_t>(value);
}

command_line parse_command_line(int argc, char **argv) {
  command_line result;
  for (int i = 1; i < argc; ++i) {
    const std::string argument(argv[i]);
    if (argument == "--help") {
      std::cout
          << "MeshBooleanBenchmark [--suite mesh|exact-arithmetic]\n"
             "  [--fixture B0|...|B8|all] [--size 1|2|3]\n"
             "  [--operation union|intersection|a-minus-b|b-minus-a|xor|all]\n"
             "  [--verification mandatory|exhaustive] [--type float-u32|float-u64|double-u32|double-u64]\n"
             "  [--arithmetic-case add|subtract|multiply|divide|gcd|rational|all]\n"
             "  [--limbs 1|2|8|32|128]\n"
             "  [--threads N] [--warmup N] [--repetitions N]\n";
      std::exit(0);
    } else if (argument.rfind("--suite", 0) == 0) {
      result.suite = value_after(argument, "--suite", i, argc, argv);
    } else if (argument.rfind("--fixture", 0) == 0) {
      const auto value = value_after(argument, "--fixture", i, argc, argv);
      result.fixtures.clear();
      if (value == "all")
        for (const auto *name : performance_fixture_names())
          result.fixtures.emplace_back(name);
      else
        result.fixtures.push_back(value);
    } else if (argument.rfind("--size", 0) == 0) {
      result.size = unsigned_value(value_after(argument, "--size", i, argc, argv), "size");
    } else if (argument.rfind("--operation", 0) == 0) {
      result.operation_name = value_after(argument, "--operation", i, argc, argv);
    } else if (argument.rfind("--verification", 0) == 0) {
      result.verification_name = value_after(argument, "--verification", i, argc, argv);
    } else if (argument.rfind("--type", 0) == 0) {
      result.type_name = value_after(argument, "--type", i, argc, argv);
    } else if (argument.rfind("--arithmetic-case", 0) == 0) {
      result.arithmetic_case = value_after(argument, "--arithmetic-case", i, argc, argv);
    } else if (argument.rfind("--limbs", 0) == 0) {
      result.arithmetic_limbs = unsigned_value(
          value_after(argument, "--limbs", i, argc, argv), "limbs");
    } else if (argument.rfind("--threads", 0) == 0) {
      result.threads = unsigned_value(value_after(argument, "--threads", i, argc, argv), "threads");
    } else if (argument.rfind("--warmup", 0) == 0) {
      result.warmups = unsigned_value(value_after(argument, "--warmup", i, argc, argv), "warmup", true);
    } else if (argument.rfind("--repetitions", 0) == 0) {
      result.repetitions = unsigned_value(value_after(argument, "--repetitions", i, argc, argv), "repetitions");
    } else {
      throw std::invalid_argument("unknown argument: " + argument);
    }
  }
  if (result.suite == "mesh") {
    checked_performance_size(result.size);
    for (const auto &fixture : result.fixtures)
      make_performance_fixture<double, std::uint32_t>(fixture, result.size);
  } else if (result.suite == "exact-arithmetic") {
    const std::vector<std::uint32_t> valid{1, 2, 8, 32, 128};
    if (std::find(valid.begin(), valid.end(), result.arithmetic_limbs) == valid.end())
      throw std::invalid_argument("limbs must be one of 1, 2, 8, 32, or 128");
  } else {
    throw std::invalid_argument("unknown suite: " + result.suite);
  }
  return result;
}

std::vector<std::pair<std::string, operation>> operations(const std::string &name) {
  const std::vector<std::pair<std::string, operation>> all{
      {"union", operation::regularized_union},
      {"intersection", operation::regularized_intersection},
      {"a-minus-b", operation::a_minus_b},
      {"b-minus-a", operation::b_minus_a},
      {"xor", operation::symmetric_difference}};
  if (name == "all")
    return all;
  for (const auto &entry : all)
    if (entry.first == name)
      return {entry};
  throw std::invalid_argument("unknown operation: " + name);
}

std::int64_t median(std::vector<std::int64_t> values) {
  std::sort(values.begin(), values.end());
  const auto middle = values.size() / 2;
  if (values.size() % 2 != 0)
    return values[middle];
  return (values[middle - 1] + values[middle]) / 2;
}

std::int64_t median_absolute_deviation(const std::vector<std::int64_t> &values,
                                       std::int64_t center) {
  std::vector<std::int64_t> deviations;
  deviations.reserve(values.size());
  for (const auto value : values)
    deviations.push_back(value >= center ? value - center : center - value);
  return median(std::move(deviations));
}

struct arithmetic_observation {
  std::uint64_t checksum = 1469598103934665603ULL;
  std::uint64_t nanoseconds = 0;
  performance_counter_snapshot counters;
};

arithmetic_observation observe_arithmetic(const std::string &name,
                                          std::size_t limbs) {
  using namespace exact_arithmetic_test;
  const auto a = patterned_operand(limbs, 11);
  const auto b = patterned_operand(limbs, 17);
  const auto divisor = patterned_operand(limbs, 23);
  const auto quotient = patterned_operand(std::max<std::size_t>(1, limbs / 2), 29);
  const auto dividend = quotient * divisor + (divisor - big_uint(1));
  const auto common = big_uint(45).shifted_left(limbs + 1);
  const auto gcd_a = a * common, gcd_b = b * common;
  const auto denominator_a = big_uint(1).shifted_left(limbs * 32 + 1);
  const auto denominator_b = big_uint(1).shifted_left(limbs * 32 - 1);
  const exact_rational rational_a(big_int(integer_sign::positive, a), denominator_a);
  const exact_rational rational_b(big_int(integer_sign::negative, b), denominator_b);
  const std::size_t batch = std::max<std::size_t>(1, 64 / limbs);

  big_uint unsigned_result;
  exact_rational rational_result;
  int comparison = 0;
  performance_collector collector;
  {
    performance_scope scope(&collector, boolean_stage::intersection_events,
                            performance_role::producer);
    for (std::size_t i = 0; i < batch; ++i) {
      if (name == "add")
        unsigned_result = a + b;
      else if (name == "subtract")
        unsigned_result = (a + b) - a;
      else if (name == "multiply")
        unsigned_result = a * b;
      else if (name == "divide") {
        const auto result = divide(dividend, divisor);
        unsigned_result = result.first + result.second;
      } else if (name == "gcd")
        unsigned_result = gcd(gcd_a, gcd_b);
      else if (name == "rational") {
        const auto sum = rational_a + rational_b;
        const auto difference = rational_a - rational_b;
        const auto product = rational_a * rational_b;
        const auto ratio = rational_a / rational_b;
        comparison += rational_a.compare(rational_b);
        rational_result = (sum + difference) * product / ratio;
      } else
        throw std::invalid_argument("unknown arithmetic case: " + name);
    }
  }
  arithmetic_observation result;
  result.checksum = checksum_bytes(result.checksum,
                                   name == "rational"
                                       ? rational_result.canonical_bytes()
                                       : unsigned_result.canonical_bytes());
  result.checksum ^= static_cast<std::uint64_t>(comparison);
  const auto snapshot = collector.snapshot();
  const auto &stage = snapshot->stage(boolean_stage::intersection_events);
  result.nanoseconds = stage.producer_nanoseconds;
  result.counters = stage.producer;
  return result;
}

std::vector<std::string> arithmetic_cases(const std::string &selected) {
  const std::vector<std::string> all{
      "add", "subtract", "multiply", "divide", "gcd", "rational"};
  if (selected == "all")
    return all;
  if (std::find(all.begin(), all.end(), selected) == all.end())
    throw std::invalid_argument("unknown arithmetic case: " + selected);
  return {selected};
}

void run_arithmetic_case(const command_line &cli, const std::string &name) {
  for (std::uint32_t i = 0; i < cli.warmups; ++i)
    (void)observe_arithmetic(name, cli.arithmetic_limbs);
  std::vector<std::int64_t> timings;
  arithmetic_observation reference;
  for (std::uint32_t i = 0; i < cli.repetitions; ++i) {
    auto observed = observe_arithmetic(name, cli.arithmetic_limbs);
    timings.push_back(static_cast<std::int64_t>(observed.nanoseconds));
    if (i == 0)
      reference = observed;
    else if (observed.checksum != reference.checksum ||
             observed.counters.values != reference.counters.values)
      throw std::runtime_error("non-deterministic arithmetic repetition");
  }
  const auto timing = median(timings);
  const auto mad = median_absolute_deviation(timings, timing);
  const auto value = [&](performance_counter counter) {
    return reference.counters.value(counter);
  };
  std::cout << "ARITH_BENCH\tschema=1\tcase=" << name
            << "\tlimbs=" << cli.arithmetic_limbs
            << "\twarmup=" << cli.warmups
            << "\trepetitions=" << cli.repetitions
            << "\tmedian_ns=" << timing << "\tmad_ns=" << mad
            << "\tchecksum=" << reference.checksum
            << "\tsmall_integer_ops=" << value(performance_counter::small_integer_operations)
            << "\tlarge_integer_ops=" << value(performance_counter::large_integer_operations)
            << "\tlimb_additions=" << value(performance_counter::limb_additions)
            << "\tlimb_multiplications=" << value(performance_counter::limb_multiplications)
            << "\tdivision_calls=" << value(performance_counter::division_calls)
            << "\tdivided_limbs=" << value(performance_counter::divided_limbs)
            << "\tgcd_calls=" << value(performance_counter::gcd_calls)
            << "\trational_normalizations=" << value(performance_counter::rational_normalizations)
            << "\tcross_cancellations=" << value(performance_counter::cross_cancellations)
            << "\tmax_numerator_limbs=" << value(performance_counter::max_numerator_limbs)
            << "\tmax_denominator_limbs=" << value(performance_counter::max_denominator_limbs)
            << "\tallocation_count=" << value(performance_counter::allocation_count)
            << '\n';
}

void run_arithmetic(const command_line &cli) {
  for (const auto &name : arithmetic_cases(cli.arithmetic_case))
    run_arithmetic_case(cli, name);
}

std::string digest_list(const performance_observation &observation) {
  std::string result;
  for (const auto &entry : observation.semantic_digests) {
    if (!result.empty())
      result += ',';
    result += entry.first + ':' + entry.second.hex();
  }
  return result.empty() ? "none" : result;
}

std::uint64_t counter(const performance_observation &observation,
                      performance_counter value) {
  return observation.producer_counters.value(value) +
         observation.verifier_counters.value(value);
}

template <class T, class I>
void run_case(const command_line &cli, const std::string &fixture,
              const std::string &operation_name, operation op,
              verification_level verification, const char *coordinate,
              const char *index) {
  boolean_options options;
  options.verification = verification;
  options.execution.max_threads = cli.threads;
  options.tracing.collect_noncanonical_timings = true;

  for (std::uint32_t i = 0; i < cli.warmups; ++i)
    (void)observe_performance_fixture<T, I>(fixture, cli.size, op, options);

  using clock = std::chrono::steady_clock;
  std::vector<std::int64_t> timings;
  std::vector<std::int64_t> producer_timings, verifier_timings;
  constexpr std::size_t stage_count =
      static_cast<std::size_t>(boolean_stage::final_verification) + 1;
  std::array<std::vector<std::int64_t>, stage_count> stage_producer_timings,
      stage_verifier_timings;
  performance_observation reference;
  for (std::uint32_t i = 0; i < cli.repetitions; ++i) {
    const auto begin = clock::now();
    auto observed = observe_performance_fixture<T, I>(fixture, cli.size, op, options);
    timings.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                          clock::now() - begin)
                          .count());
    producer_timings.push_back(static_cast<std::int64_t>(observed.producer_nanoseconds / 1000));
    verifier_timings.push_back(static_cast<std::int64_t>(observed.verifier_nanoseconds / 1000));
    for (std::size_t stage = 0; stage < stage_count; ++stage) {
      stage_producer_timings[stage].push_back(
          static_cast<std::int64_t>(observed.stage_producer_nanoseconds[stage] / 1000));
      stage_verifier_timings[stage].push_back(
          static_cast<std::int64_t>(observed.stage_verifier_nanoseconds[stage] / 1000));
    }
    if (i == 0)
      reference = std::move(observed);
    else if (observed.typed_outcome() != reference.typed_outcome() ||
             observed.output_identity != reference.output_identity ||
             !(observed.counters == reference.counters) ||
             observed.producer_counters.values != reference.producer_counters.values ||
             observed.verifier_counters.values != reference.verifier_counters.values ||
             observed.producer_counters.resources != reference.producer_counters.resources ||
             observed.verifier_counters.resources != reference.verifier_counters.resources)
      throw std::runtime_error("non-deterministic measured repetition");
  }
  const auto total_median = median(timings);
  const auto total_mad = median_absolute_deviation(timings, total_median);
  const auto producer_median = median(producer_timings);
  const auto producer_mad = median_absolute_deviation(producer_timings, producer_median);
  const auto verifier_median = median(verifier_timings);
  const auto verifier_mad = median_absolute_deviation(verifier_timings, verifier_median);
  std::array<std::int64_t, stage_count> stage_producer_medians{{}},
      stage_producer_mads{{}}, stage_verifier_medians{{}},
      stage_verifier_mads{{}};
  for (std::size_t stage = 0; stage < stage_count; ++stage) {
    stage_producer_medians[stage] = median(stage_producer_timings[stage]);
    stage_producer_mads[stage] = median_absolute_deviation(
        stage_producer_timings[stage], stage_producer_medians[stage]);
    stage_verifier_medians[stage] = median(stage_verifier_timings[stage]);
    stage_verifier_mads[stage] = median_absolute_deviation(
        stage_verifier_timings[stage], stage_verifier_medians[stage]);
  }
  const auto &c = reference.counters;
  std::cout << "BENCH\tschema=2\tfixture=" << fixture << "\tsize=" << cli.size
            << "\top=" << operation_name << "\tT=" << coordinate
            << "\tI=" << index << "\tverification=" << cli.verification_name
            << "\tthreads=" << cli.threads << "\twarmup=" << cli.warmups
            << "\trepetitions=" << cli.repetitions
            << "\tinput_a=" << reference.input_a_digest.hex()
            << "\tinput_b=" << reference.input_b_digest.hex()
            << "\tsetup=" << reference.setup_digest.hex()
            << "\toutcome=" << reference.typed_outcome()
            << "\tstage_semantic=" << digest_list(reference)
            << "\tcanonical_identity=" << reference.output_identity.hex()
            << "\tcanonical_bytes=" << reference.canonical_output_bytes.size()
            << "\ttotal_median_us=" << total_median << "\ttotal_mad_us=" << total_mad
            << "\tproducer_median_us=" << producer_median
            << "\tproducer_mad_us=" << producer_mad
            << "\tverifier_median_us=" << verifier_median
            << "\tverifier_mad_us=" << verifier_mad
            << "\tsnapshot=" << (reference.snapshot_collected ? "schema1" : "unavailable")
            << "\ttiming_source=core_stages_plus_wall_total"
            << "\tinput_vertices=" << c.input_vertices << "\tinput_facets=" << c.input_facets
            << "\tinput_shells=" << c.input_shells
            << "\tbroad_node_pairs=" << c.broad_node_pairs
            << "\tbroad_facet_pairs=" << c.broad_facet_pairs
            << "\tbroad_final_candidates=" << c.broad_final_candidates
            << "\tevent_candidates=" << c.event_candidates << "\tevent_points=" << c.event_points
            << "\tevent_intervals=" << c.event_intervals << "\tevent_regions=" << c.event_regions
            << "\tevent_carriers=" << c.event_carriers
            << "\tsymbolic_vertices=" << c.symbolic_vertices
            << "\tsymbolic_curves=" << c.symbolic_curves
            << "\tlocal_facets=" << c.local_facets << "\tlocal_shared_edges=" << c.local_shared_edges
            << "\tlocal_patches=" << c.local_patches << "\tglobal_vertices=" << c.global_vertices
            << "\tglobal_edges=" << c.global_edges << "\tglobal_patches=" << c.global_patches
            << "\tglobal_halfedges=" << c.global_halfedges
            << "\tclassification_regions=" << c.classification_regions
            << "\tclassification_labels=" << c.classification_labels
            << "\tclassification_probes=" << c.classification_probes
            << "\tselected_patches=" << c.selected_patches << "\tselected_edges=" << c.selected_edges
            << "\tselected_components=" << c.selected_components
            << "\trealization_vertices=" << c.realization_vertices
            << "\trealization_obligations=" << c.realization_obligations
            << "\trealization_pair_candidates=" << c.realization_pair_candidates
            << "\trealization_pair_checks=" << c.realization_pair_checks
            << "\trealization_components=" << c.realization_components
            << "\toutput_vertices=" << c.output_vertices << "\toutput_faces=" << c.output_faces
            << "\toutput_components=" << c.output_components
            << "\tauthoritative_bytes=" << c.authoritative_bytes
            << "\tstage_private_bytes=" << c.stage_private_bytes
            << "\tverifier_work=" << c.verifier_work
            << "\tverifier_scratch_bytes=" << c.verifier_scratch_bytes
            << "\toutput_canonical_accounted=" << c.output_canonical_bytes
            << "\tsmall_integer_ops=" << counter(reference, performance_counter::small_integer_operations)
            << "\tlarge_integer_ops=" << counter(reference, performance_counter::large_integer_operations)
            << "\tlimb_additions=" << counter(reference, performance_counter::limb_additions)
            << "\tlimb_multiplications=" << counter(reference, performance_counter::limb_multiplications)
            << "\tdivision_calls=" << counter(reference, performance_counter::division_calls)
            << "\tdivided_limbs=" << counter(reference, performance_counter::divided_limbs)
            << "\tgcd_calls=" << counter(reference, performance_counter::gcd_calls)
            << "\trational_normalizations=" << counter(reference, performance_counter::rational_normalizations)
            << "\tcross_cancellations=" << counter(reference, performance_counter::cross_cancellations)
            << "\tmax_numerator_limbs=" << counter(reference, performance_counter::max_numerator_limbs)
            << "\tmax_denominator_limbs=" << counter(reference, performance_counter::max_denominator_limbs)
            << "\torient2d_calls=" << counter(reference, performance_counter::orient2d_calls)
            << "\torient3d_calls=" << counter(reference, performance_counter::orient3d_calls)
            << "\tfilter_accepts=" << counter(reference, performance_counter::filter_accepts)
            << "\tfilter_fallbacks=" << counter(reference, performance_counter::filter_fallbacks)
            << "\texact_fallbacks=" << counter(reference, performance_counter::exact_fallbacks)
            << "\tgeometric_exact_divisions=" << counter(reference, performance_counter::geometric_exact_divisions)
            << "\tsupport_plane_constructions=" << counter(reference, performance_counter::support_plane_constructions)
            << "\tpoint_in_polygon_edge_tests=" << counter(reference, performance_counter::point_in_polygon_edge_tests)
            << "\tring_edge_candidate_pairs=" << counter(reference, performance_counter::ring_edge_candidate_pairs)
            << "\texact_ring_edge_tests=" << counter(reference, performance_counter::exact_ring_edge_tests)
            << "\tear_candidates=" << counter(reference, performance_counter::ear_candidates)
            << "\texact_ear_tests=" << counter(reference, performance_counter::exact_ear_tests)
            << "\tself_embedding_candidate_pairs=" << counter(reference, performance_counter::self_embedding_candidate_pairs)
            << "\texact_facet_pair_tests=" << counter(reference, performance_counter::exact_facet_pair_tests)
            << "\tshell_location_queries=" << counter(reference, performance_counter::shell_location_queries)
            << "\tcanonicalization_refinements=" << counter(reference, performance_counter::canonicalization_refinements)
            << "\tcanonicalization_branches=" << counter(reference, performance_counter::canonicalization_branches)
            << "\tbroad_build_comparisons=" << counter(reference, performance_counter::broad_phase_build_comparisons)
            << "\tbroad_verifier_checks=" << counter(reference, performance_counter::broad_phase_verifier_candidate_checks)
            << "\tplane_relation_classes=" << counter(reference, performance_counter::plane_relation_classes)
            << "\texact_carrier_polygon_tests=" << counter(reference, performance_counter::exact_carrier_polygon_tests)
            << "\traw_events=" << counter(reference, performance_counter::raw_events)
            << "\tduplicate_derivations=" << counter(reference, performance_counter::duplicate_derivations)
            << "\texact_equality_checks=" << counter(reference, performance_counter::exact_equality_checks)
            << "\thash_bucket_probes=" << counter(reference, performance_counter::hash_bucket_probes)
            << "\tcanonical_key_encodings=" << counter(reference, performance_counter::canonical_key_encodings)
            << "\tconstraint_pairs=" << counter(reference, performance_counter::candidate_constraint_pairs)
            << "\texact_constraint_intersections=" << counter(reference, performance_counter::exact_constraint_intersections)
            << "\tdcel_entities=" << counter(reference, performance_counter::dcel_entities)
            << "\treconciliation_passes=" << counter(reference, performance_counter::reconciliation_passes)
            << "\tpatch_witness_slabs=" << counter(reference, performance_counter::patch_witness_slabs)
            << "\tpatch_witness_crossings=" << counter(reference, performance_counter::patch_witness_crossings)
            << "\tprobe_constraints=" << counter(reference, performance_counter::probe_constraints)
            << "\tray_box_candidates=" << counter(reference, performance_counter::ray_box_candidates)
            << "\texact_ray_facet_tests=" << counter(reference, performance_counter::exact_ray_facet_tests)
            << "\taccepted_ray_hits=" << counter(reference, performance_counter::accepted_ray_hits)
            << "\talternate_rays=" << counter(reference, performance_counter::alternate_rays)
            << "\treconstructed_rays=" << counter(reference, performance_counter::reconstructed_rays)
            << "\trealization_axis_candidates=" << counter(reference, performance_counter::realization_axis_candidates)
            << "\trealization_exact_pair_checks=" << counter(reference, performance_counter::realization_exact_pair_checks)
            << "\trealization_solver_nodes=" << counter(reference, performance_counter::realization_solver_nodes)
            << "\trealization_rejected_prefixes=" << counter(reference, performance_counter::realization_rejected_prefixes)
            << "\trealization_complete_assignments=" << counter(reference, performance_counter::realization_complete_assignments)
            << "\tallocation_count=" << counter(reference, performance_counter::allocation_count)
            << "\tcopied_artifact_bytes=" << counter(reference, performance_counter::copied_artifact_bytes);
  const std::array<const char *, stage_count> stage_names{{
      "context_setup", "input_validation", "broad_phase",
      "intersection_events", "symbolic_registry", "local_refinement",
      "global_arrangement", "cell_classification", "boolean_selection",
      "topology_preflight", "geometry_realization", "output_assembly",
      "final_verification"}};
  for (std::size_t stage = 0; stage < stage_count; ++stage)
    std::cout << '\t' << stage_names[stage] << "_producer_median_us="
              << stage_producer_medians[stage] << '\t' << stage_names[stage]
              << "_producer_mad_us=" << stage_producer_mads[stage] << '\t'
              << stage_names[stage] << "_verifier_median_us="
              << stage_verifier_medians[stage] << '\t' << stage_names[stage]
              << "_verifier_mad_us=" << stage_verifier_mads[stage];
  std::cout << '\n';
}

template <class T, class I>
void run_selected(const command_line &cli, verification_level verification,
                  const char *coordinate, const char *index) {
  for (const auto &fixture : cli.fixtures)
    for (const auto &op : operations(cli.operation_name))
      run_case<T, I>(cli, fixture, op.first, op.second, verification, coordinate,
                     index);
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto cli = parse_command_line(argc, argv);
    if (cli.suite == "exact-arithmetic") {
      run_arithmetic(cli);
      return 0;
    }
    const auto verification = cli.verification_name == "mandatory"
                                  ? verification_level::mandatory
                                  : cli.verification_name == "exhaustive"
                                        ? verification_level::exhaustive
                                        : throw std::invalid_argument(
                                              "unknown verification level: " +
                                              cli.verification_name);
    if (cli.type_name == "float-u32")
      run_selected<float, std::uint32_t>(cli, verification, "binary32", "uint32");
    else if (cli.type_name == "float-u64")
      run_selected<float, std::uint64_t>(cli, verification, "binary32", "uint64");
    else if (cli.type_name == "double-u32")
      run_selected<double, std::uint32_t>(cli, verification, "binary64", "uint32");
    else if (cli.type_name == "double-u64")
      run_selected<double, std::uint64_t>(cli, verification, "binary64", "uint64");
    else
      throw std::invalid_argument("unknown type pair: " + cli.type_name);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "MeshBooleanBenchmark: " << error.what() << '\n';
    return 2;
  }
}
