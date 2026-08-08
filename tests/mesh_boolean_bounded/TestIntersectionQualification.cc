#include "BroadPhaseFixtures.h"
#include "qualification/ExactFloatImport.h"

#include "YgorMeshesBooleanBounded/CanonicalBytes.h"
#include "YgorMeshesBooleanBounded/FloatingBits.h"
#include "YgorMeshesBooleanBounded/EventIncidence.h"
#include "YgorMeshesBooleanBounded/EventInterning.h"
#include "YgorMeshesBooleanBounded/EventNormalization.h"
#include "YgorMeshesBooleanBounded/IntersectionBuild.h"
#include "YgorMeshesBooleanBounded/IntersectionPreflight.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/Sha256.h"
#include "YgorMeshesBooleanBounded/SourceEdgeArrangements.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
namespace qualification = ygor::mesh_boolean::qualification;
using broad_phase_tests::require;

namespace {

using relation_type = bounded::signed_feature_relations<double, std::uint32_t>;
using artifact_type =
    bounded::canonical_intersection_complex<double, std::uint32_t>;

struct stage_fixture final {
  broad_phase_tests::built_fixture broad{};
  std::shared_ptr<const relation_type> relations{};
};

std::shared_ptr<const relation_type>
build_relations(broad_phase_tests::built_fixture &fixture) {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::relation_capabilities capabilities;
  capabilities.owner = fixture.predecessor.context.owner;
  capabilities.resources = &resources;
  auto outcome = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities);
  require(outcome.has_value(), "Component 08 qualification relation build failed");
  return *outcome.value();
}

stage_fixture empty_stage_fixture(
    bounded_execution_mode mode = bounded_execution_mode::serial_v1,
    std::uint32_t workers = 1) {
  stage_fixture fixture;
  fixture.broad = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0),
      bounded::source_triangulation_provider_kind::indexed_dependency_v1, true,
      mode, workers);
  fixture.relations = build_relations(fixture.broad);
  require(fixture.relations->event_seeds().empty(),
          "empty Component 08 qualification fixture has event lineage");
  return fixture;
}

stage_fixture transverse_stage_fixture() {
  stage_fixture fixture;
  fixture.broad = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  fixture.relations = build_relations(fixture.broad);
  require(!fixture.relations->event_seeds().empty(),
          "transverse Component 08 fixture lacks event lineage");
  return fixture;
}

bounded::intersection_capabilities stage_capabilities(
    const stage_fixture &fixture, bounded::resource_manager &resources) {
  bounded::intersection_capabilities capabilities;
  capabilities.owner = fixture.broad.predecessor.context.owner;
  capabilities.resources = &resources;
  return capabilities;
}

std::shared_ptr<const artifact_type>
build_stage(const stage_fixture &fixture, bounded::resource_manager &resources,
            bounded::intersection_capabilities capabilities) {
  auto outcome = bounded::build_canonical_intersection_complex(
      fixture.broad.predecessor.context,
      *fixture.broad.predecessor.precision, fixture.relations,
      std::move(capabilities));
  if (!outcome.has_value()) {
    throw std::runtime_error(
        std::string("Component 08 stage build failed: ") +
        outcome.error()->summary + " subcode=" +
        std::to_string(outcome.error()->subcode) + " checkpoint=" +
        std::to_string(outcome.error()->checkpoint));
  }
  (void)resources;
  return *outcome.value();
}

void require_no_live_resources(const bounded::resource_manager &resources,
                               const char *message) {
  for (const auto &counter : resources.snapshot())
    require(counter.reserved == 0 && counter.committed == 0, message);
}

bounded::relation_feature_key feature(bounded::relation_feature_kind kind,
                                      std::uint64_t primary) {
  bounded::relation_feature_key key;
  key.operand = bounded::operand_id::a;
  key.kind = kind;
  key.primary = primary;
  return key;
}

bounded::intersection_occurrence_key occurrence(std::uint32_t slot) {
  bounded::intersection_occurrence_key key;
  key.event.semantic_namespace.bytes[0] = 0x91;
  key.event.event_class = bounded::intersection_event_class::edge_facet_point;
  key.event.first_operand = bounded::operand_id::a;
  key.event.second_operand = bounded::operand_id::b;
  key.event.first_owner =
      feature(bounded::relation_feature_kind::source_edge, 17);
  key.event.second_owner.operand = bounded::operand_id::b;
  key.event.second_owner.kind = bounded::relation_feature_kind::source_facet;
  key.event.second_owner.primary = 4;
  key.event.public_relation.semantic_namespace = key.event.semantic_namespace;
  key.event.public_relation.family =
      bounded::feature_relation_family::source_edge_source_facet;
  key.event.public_relation.first = key.event.first_owner;
  key.event.public_relation.second = key.event.second_owner;
  key.event.public_relation.occurrence = slot + 1;
  key.event.construction_kind =
      bounded::relation_construction_kind::bounded_point;
  key.event.construction_precedence =
      bounded::relation_construction_precedence::source_edge_source_facet_point;
  key.event.authoritative_source_feature = key.event.first_owner;
  key.event.construction_source_provenance = 1000 + slot;
  key.event.construction_geometric_lineage = 2000 + slot;
  key.event.carrier_role =
      bounded::intersection_carrier_role::transverse_endpoint;
  key.event.contact_status = bounded::feature_relation_status::proper_crossing;
  key.event.contact_dimension = bounded::relation_contact_dimension::point;
  key.discriminator.role = bounded::occurrence_role::single_occurrence;
  key.discriminator.component07_occurrence = slot + 1;
  return key;
}

bounded::source_edge_membership_proposal proposal(std::uint32_t slot,
                                                   std::uint32_t numerator) {
  const double parameter = static_cast<double>(numerator) / 64.0;
  bounded::source_edge_membership_proposal value;
  value.key.source_edge =
      feature(bounded::relation_feature_kind::source_edge, 17);
  value.key.occurrence = occurrence(slot);
  value.key.role = bounded::intersection_membership_role::interior;
  value.key.parameter_evidence = bounded::relation_interval_evidence_id{slot};
  value.key.parameter_lineage = 4000 + numerator;
  value.key.relation_lineage = 5000 + slot;
  value.key.facet_use_role = bounded::source_facet_use_role::right_incident;
  value.occurrence = bounded::event_occurrence_id{slot};
  value.event = bounded::event_id{slot};
  value.parameter = value.key.parameter_evidence;
  value.nominal_bits = bounded::to_bits(parameter);
  value.lower_bits = value.nominal_bits;
  value.upper_bits = value.nominal_bits;
  value.domain = bounded::parameter_domain_status::stable_interior;
  value.exact_zero = bounded::exact_relation_status::exact_positive;
  value.exact_one = bounded::exact_relation_status::exact_negative;
  value.exact_equal_eligible = true;
  value.cluster_eligible = true;
  return value;
}

bounded::source_edge_domain_record source_domain() {
  bounded::source_edge_domain_record domain;
  domain.source_edge =
      feature(bounded::relation_feature_kind::source_edge, 17);
  domain.start_vertex =
      feature(bounded::relation_feature_kind::source_vertex, 2);
  domain.end_vertex =
      feature(bounded::relation_feature_kind::source_vertex, 9);
  return domain;
}

std::vector<std::uint8_t> arrangement_projection(
    const bounded::source_edge_arrangement_tables &tables) {
  bounded::canonical_writer writer;
  writer.u16(bounded::contract_versions::intersection_provider);
  writer.u64(tables.memberships.size());
  writer.u64(tables.membership_sequence_index.size());
  for (const auto id : tables.membership_sequence_index)
    writer.u64(id.ordinal());
  for (const auto &membership : tables.memberships) {
    writer.u64(membership.id.ordinal());
    writer.u64(membership.occurrence.ordinal());
    writer.u64(membership.parameter.ordinal());
    writer.u64(membership.ordering_certificate.ordinal());
  }
  writer.u64(tables.sequences.size());
  for (const auto &sequence : tables.sequences) {
    writer.u64(sequence.id.ordinal());
    writer.u64(sequence.clusters.begin);
    writer.u64(sequence.clusters.count);
    writer.u64(sequence.memberships.begin);
    writer.u64(sequence.memberships.count);
    writer.u64(sequence.intervals.begin);
    writer.u64(sequence.intervals.count);
    writer.u64(sequence.comparison_count);
  }
  writer.u64(tables.clusters.size());
  for (const auto &cluster : tables.clusters) {
    writer.u64(cluster.id.ordinal());
    writer.u64(cluster.member_occurrences.begin);
    writer.u64(cluster.member_occurrences.count);
    writer.u64(cluster.membership_ids.begin);
    writer.u64(cluster.membership_ids.count);
    writer.u64(cluster.predecessor.ordinal());
    writer.u64(cluster.successor.ordinal());
    writer.u64(cluster.ordering_certificate.ordinal());
    writer.boolean(cluster.shared_output_coordinate);
    writer.boolean(cluster.separate_output_occurrences);
  }
  writer.u64(tables.cluster_occurrence_index.size());
  for (const auto id : tables.cluster_occurrence_index)
    writer.u64(id.ordinal());
  writer.u64(tables.intervals.size());
  for (const auto &interval : tables.intervals) {
    writer.u64(interval.id.ordinal());
    writer.u8(static_cast<std::uint8_t>(interval.key.left.kind));
    writer.u8(static_cast<std::uint8_t>(interval.key.right.kind));
    writer.u8(static_cast<std::uint8_t>(interval.length_disposition));
  }
  writer.u64(tables.ordering_certificates.size());
  for (const auto &certificate : tables.ordering_certificates) {
    writer.u64(certificate.id.ordinal());
    writer.u8(static_cast<std::uint8_t>(certificate.disposition));
    writer.u64(certificate.first_parameter.ordinal());
    writer.u64(certificate.second_parameter.ordinal());
    writer.boolean(certificate.topology_safe);
  }
  return writer.take();
}

std::vector<std::uint32_t> exact_sorted_numerators(
    const std::vector<bounded::source_edge_membership_proposal> &proposals) {
  struct oracle_record final {
    qualification::ExactRational parameter{};
    std::uint32_t numerator = 0;
    std::uint64_t occurrence = 0;
  };
  std::vector<oracle_record> oracle;
  oracle.reserve(proposals.size());
  for (const auto &value : proposals) {
    const auto decoded = bounded::from_bits<double>(value.nominal_bits);
    const auto exact = qualification::import_exact(decoded);
    require(exact.value >= qualification::ExactRational{0} &&
                exact.value <= qualification::ExactRational{1},
            "exact parameter oracle found an out-of-domain value");
    oracle.push_back(
        {exact.value,
         static_cast<std::uint32_t>(decoded * 64.0),
         value.occurrence.ordinal()});
  }
  std::sort(oracle.begin(), oracle.end(), [](const auto &first,
                                             const auto &second) {
    const auto comparison = first.parameter.compare(second.parameter);
    if (comparison != 0)
      return comparison < 0;
    return first.occurrence < second.occurrence;
  });
  std::vector<std::uint32_t> result;
  result.reserve(oracle.size());
  for (const auto &entry : oracle)
    result.push_back(entry.numerator);
  return result;
}

void verify_exact_oracle(
    const std::vector<bounded::source_edge_membership_proposal> &proposals,
    const bounded::source_edge_arrangement_tables &tables) {
  const auto exact_order = exact_sorted_numerators(proposals);
  require(tables.sequences.size() == 1,
          "exact ordering fixture did not publish one source-edge sequence");
  const auto &sequence = tables.sequences.front();
  require(sequence.memberships.count == exact_order.size(),
          "source-edge membership count differs from exact oracle");

  std::vector<std::uint32_t> published_order;
  published_order.reserve(exact_order.size());
  for (std::uint64_t offset = 0; offset < sequence.memberships.count; ++offset) {
    const auto membership_id = tables.membership_sequence_index.at(
        static_cast<std::size_t>(sequence.memberships.begin + offset));
    const auto &membership =
        tables.memberships.at(static_cast<std::size_t>(membership_id.ordinal()));
    const auto found = std::find_if(
        proposals.begin(), proposals.end(), [&](const auto &proposal_value) {
          return proposal_value.occurrence == membership.occurrence;
        });
    require(found != proposals.end(),
            "published source-edge membership has no oracle proposal");
    const auto parameter = bounded::from_bits<double>(found->nominal_bits);
    published_order.push_back(static_cast<std::uint32_t>(parameter * 64.0));
  }
  require(published_order == exact_order,
          "bounded source-edge order differs from exact rational oracle");

  auto unique_values = exact_order;
  const auto unique_count = static_cast<std::uint64_t>(
      std::distance(unique_values.begin(),
                    std::unique(unique_values.begin(), unique_values.end())));
  require(sequence.clusters.count == unique_count &&
              sequence.intervals.count == unique_count + 1,
          "cluster/partition count differs from exact rational oracle");
}

std::uint64_t next_random(std::uint64_t &state) noexcept {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

void deterministic_shuffle(
    std::vector<bounded::source_edge_membership_proposal> &values,
    std::uint64_t &state) {
  for (std::size_t i = values.size(); i > 1; --i) {
    const auto selected = static_cast<std::size_t>(next_random(state) % i);
    std::swap(values[i - 1], values[selected]);
  }
}

bounded::intersection_subcode build_failure(
    const std::vector<bounded::source_edge_membership_proposal> &proposals) {
  bounded::source_edge_arrangement_tables tables;
  bounded_boolean_error error;
  if (bounded::build_source_edge_arrangements<double>(
          {source_domain()}, proposals, tables, error))
    return bounded::intersection_subcode::internal_invariant;
  return static_cast<bounded::intersection_subcode>(error.subcode);
}

std::vector<bounded::source_edge_membership_proposal> shrink_failure(
    std::vector<bounded::source_edge_membership_proposal> values,
    bounded::intersection_subcode expected) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t i = 0; i < values.size(); ++i) {
      auto candidate = values;
      candidate.erase(candidate.begin() + static_cast<std::ptrdiff_t>(i));
      if (candidate.size() >= 2 && build_failure(candidate) == expected) {
        values = std::move(candidate);
        changed = true;
        break;
      }
    }
  }
  return values;
}

void test_exact_oracle_and_golden() {
  std::vector<bounded::source_edge_membership_proposal> proposals{
      proposal(0, 8), proposal(1, 16), proposal(2, 16), proposal(3, 32),
      proposal(4, 48)};
  bounded::source_edge_arrangement_tables tables;
  bounded_boolean_error error;
  require(bounded::build_source_edge_arrangements<double>(
              {source_domain()}, proposals, tables, error) &&
              bounded::verify_source_edge_arrangements<double>(
                  {source_domain()}, proposals, tables, error),
          "exact rational golden arrangement failed");
  verify_exact_oracle(proposals, tables);
  const auto digest = bounded::sha256::digest(arrangement_projection(tables));
  constexpr const char golden[] =
      "04f0bbdc0fe26dbced0ada9c6fe79109ac2ad34c702c10428d6d416cd9651e5e";
  require(digest.hex() == golden,
          "Component 08 exact source-edge golden digest changed");
}

void test_deterministic_fuzz_and_shrink() {
  std::uint64_t random = 0x6a09e667f3bcc909ULL;
  for (std::uint32_t campaign = 0; campaign < 192; ++campaign) {
    const auto count = std::uint32_t{1} +
                       static_cast<std::uint32_t>(next_random(random) % 24);
    std::vector<bounded::source_edge_membership_proposal> proposals;
    proposals.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto numerator = std::uint32_t{1} +
                             static_cast<std::uint32_t>(next_random(random) % 63);
      proposals.push_back(proposal(i, numerator));
    }

    bounded::source_edge_arrangement_tables first;
    bounded_boolean_error error;
    if (!bounded::build_source_edge_arrangements<double>(
            {source_domain()}, proposals, first, error) ||
        !bounded::verify_source_edge_arrangements<double>(
            {source_domain()}, proposals, first, error)) {
      throw std::runtime_error(
          "deterministic Component 08 fuzz case failed: campaign=" +
          std::to_string(campaign) + " subcode=" +
          std::to_string(error.subcode) + " summary=" + error.summary);
    }
    verify_exact_oracle(proposals, first);

    auto permuted = proposals;
    deterministic_shuffle(permuted, random);
    bounded::source_edge_arrangement_tables second;
    require(bounded::build_source_edge_arrangements<double>(
                {source_domain()}, permuted, second, error) &&
                arrangement_projection(first) == arrangement_projection(second),
            "Component 08 fuzz campaign is input-order dependent");
  }

  std::vector<bounded::source_edge_membership_proposal> failing{
      proposal(0, 7), proposal(1, 19), proposal(2, 31), proposal(3, 31),
      proposal(4, 43), proposal(5, 55)};
  failing[2].exact_equal_eligible = false;
  failing[2].cluster_eligible = false;
  failing[3].exact_equal_eligible = false;
  failing[3].cluster_eligible = false;
  const auto expected = bounded::intersection_subcode::exact_equal_without_evidence;
  require(build_failure(failing) == expected,
          "deterministic shrink seed did not reproduce expected failure");
  const auto minimized = shrink_failure(failing, expected);
  require(minimized.size() == 2 && build_failure(minimized) == expected,
          "Component 08 deterministic shrinker did not find the minimal pair");
}

struct cancellation_state final {
  bounded_boolean_cancellation_source *source = nullptr;
  bounded::intersection_checkpoint target =
      bounded::intersection_checkpoint::context_capability_validation;
  std::uint32_t seen = 0;
};

void cancellation_poll(void *opaque,
                       bounded::intersection_checkpoint checkpoint) noexcept {
  auto &state = *static_cast<cancellation_state *>(opaque);
  ++state.seen;
  if (checkpoint == state.target && state.source)
    state.source->request_cancel(static_cast<std::uint32_t>(checkpoint));
}

void test_resource_boundaries() {
  const auto fixture = empty_stage_fixture();
  bounded::resource_manager reference_resources(
      resource_policy::conservative_defaults());
  const auto reference = build_stage(
      fixture, reference_resources,
      stage_capabilities(fixture, reference_resources));
  require(reference->verification() ==
              bounded::intersection_verification_disposition::
                  independently_verified,
          "Component 08 stage did not publish independently verified output");

  bounded::intersection_preflight_plan plan;
  bounded_boolean_error error;
  auto roomy = stage_capabilities(fixture, reference_resources);
  require(bounded::preflight_intersection_events(*fixture.relations, roomy,
                                                  plan, error),
          "Component 08 qualification preflight failed");

  struct count_case final {
    const char *name = nullptr;
    std::uint64_t bounded::intersection_capabilities::*field = nullptr;
    std::uint64_t required = 0;
  };
  const std::array<count_case, 12> cases{{
      {"events", &bounded::intersection_capabilities::maximum_events,
       plan.estimate.event_count},
      {"occurrences", &bounded::intersection_capabilities::maximum_occurrences,
       plan.estimate.occurrence_count},
      {"seed bindings",
       &bounded::intersection_capabilities::maximum_seed_bindings,
       plan.estimate.seed_binding_count},
      {"incidence", &bounded::intersection_capabilities::maximum_incidence,
       plan.estimate.incidence_count},
      {"memberships", &bounded::intersection_capabilities::maximum_memberships,
       plan.estimate.membership_count},
      {"clusters", &bounded::intersection_capabilities::maximum_clusters,
       plan.estimate.cluster_count},
      {"intervals", &bounded::intersection_capabilities::maximum_intervals,
       plan.estimate.interval_count},
      {"carriers", &bounded::intersection_capabilities::maximum_carriers,
       plan.estimate.carrier_count},
      {"overlaps", &bounded::intersection_capabilities::maximum_overlaps,
       plan.estimate.overlap_count},
      {"aggregates", &bounded::intersection_capabilities::maximum_aggregates,
       plan.estimate.aggregate_count},
      {"descriptors", &bounded::intersection_capabilities::maximum_descriptors,
       plan.estimate.descriptor_count},
      {"work", &bounded::intersection_capabilities::maximum_work_units,
       plan.estimate.work_units},
  }};
  for (const auto &test : cases) {
    if (test.required == 0)
      continue;
    bounded::resource_manager resources(resource_policy::conservative_defaults());
    auto below = stage_capabilities(fixture, resources);
    below.*(test.field) = test.required - 1;
    bounded::intersection_preflight_plan rejected;
    bounded_boolean_error rejected_error;
    require(!bounded::preflight_intersection_events(
                *fixture.relations, below, rejected, rejected_error) &&
                rejected_error.category ==
                    bounded_boolean_error_category::resource_limit,
            test.name);
    require_no_live_resources(resources,
                              "preflight resource rejection leaked a lease");

    auto exact = stage_capabilities(fixture, resources);
    exact.*(test.field) = test.required;
    bounded::intersection_preflight_plan accepted;
    bounded_boolean_error accepted_error;
    require(bounded::preflight_intersection_events(
                *fixture.relations, exact, accepted, accepted_error),
            "exact Component 08 capability boundary must pass");
    exact.*(test.field) = test.required + 1;
    require(bounded::preflight_intersection_events(
                *fixture.relations, exact, accepted, accepted_error),
            "Component 08 capability limit-plus-one must pass");
  }

  {
    bounded::resource_manager resources(resource_policy::conservative_defaults());
    auto below = stage_capabilities(fixture, resources);
    below.maximum_canonical_bytes = plan.canonical_byte_bound - 1;
    bounded::intersection_preflight_plan rejected;
    bounded_boolean_error rejected_error;
    require(!bounded::preflight_intersection_events(
                *fixture.relations, below, rejected, rejected_error) &&
                rejected_error.category ==
                    bounded_boolean_error_category::resource_limit,
            "canonical-byte limit-minus-one must fail preflight");
  }

  const auto snapshot = reference_resources.snapshot();
  const auto persistent_required = snapshot[static_cast<std::size_t>(
      bounded::resource_kind::persistent_bytes)].peak_live;
  const auto temporary_required = snapshot[static_cast<std::size_t>(
      bounded::resource_kind::temporary_bytes)].peak_live;
  std::uint64_t work_required = 0;
  for (std::size_t i = 0; i < snapshot.size(); ++i) {
    if (i == static_cast<std::size_t>(bounded::resource_kind::persistent_bytes) ||
        i == static_cast<std::size_t>(bounded::resource_kind::temporary_bytes))
      continue;
    work_required = std::max(work_required, snapshot[i].peak_live);
  }
  require(persistent_required != 0 && temporary_required != 0 &&
              work_required != 0,
          "Component 08 resource reference did not exercise all policies");

  const auto run_with_policy = [&](resource_policy policy) {
    bounded::resource_manager resources(policy);
    return bounded::build_canonical_intersection_complex(
        fixture.broad.predecessor.context,
        *fixture.broad.predecessor.precision, fixture.relations,
        stage_capabilities(fixture, resources));
  };

  {
    auto policy = resource_policy::conservative_defaults();
    policy.persistent_bytes = {persistent_required - 1, persistent_required - 1};
    require(!run_with_policy(policy).has_value(),
            "persistent-byte limit-minus-one must fail closed");
  }
  {
    auto policy = resource_policy::conservative_defaults();
    policy.temporary_bytes = {temporary_required - 1, temporary_required - 1};
    require(!run_with_policy(policy).has_value(),
            "temporary-byte limit-minus-one must fail closed");
  }
  {
    auto policy = resource_policy::conservative_defaults();
    policy.work_units = {work_required - 1, work_required - 1};
    require(!run_with_policy(policy).has_value(),
            "work limit-minus-one must fail closed");
  }
  for (std::uint64_t increment = 0; increment <= 1; ++increment) {
    auto policy = resource_policy::conservative_defaults();
    policy.persistent_bytes = {persistent_required + increment,
                               persistent_required + increment};
    policy.temporary_bytes = {temporary_required + increment,
                              temporary_required + increment};
    policy.work_units = {work_required + increment, work_required + increment};
    auto outcome = run_with_policy(policy);
    require(outcome.has_value() &&
                (*outcome.value())->canonical_bytes() ==
                    reference->canonical_bytes(),
            "exact/plus-one Component 08 manager boundary changed output");
  }
}

void test_cancellation_matrix() {
  const auto fixture = empty_stage_fixture();
  bounded::resource_manager reference_resources(
      resource_policy::conservative_defaults());
  const auto reference = build_stage(
      fixture, reference_resources,
      stage_capabilities(fixture, reference_resources));

  const auto first = static_cast<std::uint32_t>(
      bounded::intersection_checkpoint::context_capability_validation);
  const auto last = static_cast<std::uint32_t>(
      bounded::intersection_checkpoint::transaction_commit);
  for (std::uint32_t ordinal = first; ordinal <= last; ++ordinal) {
    bounded::resource_manager resources(resource_policy::conservative_defaults());
    bounded_boolean_cancellation_source source;
    auto token = source.token();
    cancellation_state state;
    state.source = &source;
    state.target = static_cast<bounded::intersection_checkpoint>(ordinal);
    bounded::intersection_cancellation_observer observer;
    observer.state = &state;
    observer.poll = &cancellation_poll;
    auto capabilities = stage_capabilities(fixture, resources);
    capabilities.cancellation = &token;
    capabilities.cancellation_observer = &observer;
    auto cancelled = bounded::build_canonical_intersection_complex(
        fixture.broad.predecessor.context,
        *fixture.broad.predecessor.precision, fixture.relations, capabilities);
    require(!cancelled.has_value() &&
                cancelled.error()->category ==
                    bounded_boolean_error_category::cancelled &&
                cancelled.error()->subcode == static_cast<std::uint32_t>(
                    bounded::intersection_subcode::cancelled) &&
                cancelled.error()->checkpoint == ordinal &&
                token.reason() == ordinal && state.seen != 0,
            "Component 08 cancellation checkpoint is not deterministic");
    require_no_live_resources(
        resources, "Component 08 cancellation leaked a resource reservation");
  }

  bounded::resource_manager retry_resources(
      resource_policy::conservative_defaults());
  const auto retry = build_stage(
      fixture, retry_resources,
      stage_capabilities(fixture, retry_resources));
  require(retry->canonical_bytes() == reference->canonical_bytes() &&
              retry->digest() == reference->digest(),
          "retry after Component 08 cancellation matrix is not byte-identical");
}

void test_execution_determinism_and_fail_closed_gate() {
  std::vector<std::shared_ptr<const artifact_type>> artifacts;
  for (const auto &setting :
       std::array<std::pair<bounded_execution_mode, std::uint32_t>, 4>{{
           {bounded_execution_mode::serial_v1, 1},
           {bounded_execution_mode::deterministic_parallel_v1, 1},
           {bounded_execution_mode::deterministic_parallel_v1, 2},
           {bounded_execution_mode::deterministic_parallel_v1, 8},
       }}) {
    const auto fixture = empty_stage_fixture(setting.first, setting.second);
    bounded::resource_manager resources(resource_policy::conservative_defaults());
    artifacts.push_back(build_stage(
        fixture, resources, stage_capabilities(fixture, resources)));
  }
  for (std::size_t i = 1; i < artifacts.size(); ++i)
    require(artifacts[i]->canonical_bytes() == artifacts.front()->canonical_bytes() &&
                artifacts[i]->digest() == artifacts.front()->digest(),
            "Component 08 execution mode or worker count changed semantics");

  const auto transverse = transverse_stage_fixture();
  std::vector<bounded::normalized_event_seed_proposal> normalized;
  bounded_boolean_error adapter_error;
  require(bounded::normalize_event_seed_records(
              transverse.relations->event_seeds(),
              transverse.relations->constructions(), normalized, adapter_error),
          "nonempty source-edge adapter normalization failed");
  bounded::event_interning_tables interning;
  require(bounded::intern_normalized_event_seeds(
              normalized, interning, adapter_error),
          "nonempty source-edge adapter interning failed");
  bounded::event_incidence_tables incidence;
  require(bounded::build_event_incidence(
              *transverse.relations, interning, incidence, adapter_error),
          "nonempty source-edge adapter incidence failed");

  std::vector<bounded::source_edge_membership_proposal> memberships;
  require(bounded::collect_source_edge_membership_proposals(
              transverse.relations->event_seeds(),
              transverse.relations->constructions(),
              transverse.relations->interval_evidence(), interning, incidence,
              memberships, adapter_error),
          "Component 07 source-edge lineage did not adapt into memberships");
  require(memberships.size() == transverse.relations->event_seeds().size(),
          "source-edge adapter did not publish exactly one owned membership per seed");
  for (const auto &membership : memberships) {
    require(membership.key.parameter_lineage != 0 &&
                membership.parameter == membership.key.parameter_evidence &&
                membership.contributions.count != 0 &&
                membership.incident_facet_uses.count == 1,
            "source-edge adapter omitted parameter or incidence lineage");
    require(membership.contributions.begin <= incidence.records.size() &&
                membership.contributions.count <=
                    incidence.records.size() - membership.contributions.begin &&
                membership.incident_facet_uses.begin < incidence.records.size(),
            "source-edge adapter published an invalid direct incidence range");
    const auto &facet =
        incidence.records[membership.incident_facet_uses.begin];
    require(facet.kind == bounded::event_incidence_kind::source_facet &&
                facet.source_feature_owner && !facet.bookkeeping_only,
            "source-edge adapter lost the incident source-facet use");
  }

  auto unrelated_evidence = transverse.relations->interval_evidence();
  const auto &first_seed = transverse.relations->event_seeds().front();
  const auto &first_construction =
      transverse.relations->constructions()[first_seed.construction.ordinal()];
  std::size_t unrelated = unrelated_evidence.size();
  for (std::size_t i = 0; i < unrelated_evidence.size(); ++i) {
    if (i < first_construction.interval_evidence_begin ||
        i >= first_construction.interval_evidence_begin +
                 first_construction.interval_evidence_count) {
      bool owned_by_seed_construction = false;
      for (const auto &seed : transverse.relations->event_seeds()) {
        const auto &construction =
            transverse.relations->constructions()[seed.construction.ordinal()];
        if (i >= construction.interval_evidence_begin &&
            i < construction.interval_evidence_begin +
                    construction.interval_evidence_count) {
          owned_by_seed_construction = true;
          break;
        }
      }
      if (!owned_by_seed_construction) {
        unrelated = i;
        break;
      }
    }
  }
  require(unrelated != unrelated_evidence.size(),
          "source-edge adapter fixture lacks unrelated evidence");
  const auto &authoritative =
      unrelated_evidence[first_construction.interval_evidence_begin];
  unrelated_evidence[unrelated].source_relation = first_seed.source_relation;
  unrelated_evidence[unrelated].kind = authoritative.kind;
  unrelated_evidence[unrelated].occurrence = authoritative.occurrence;
  unrelated_evidence[unrelated].has_rounded_nominal = true;
  unrelated_evidence[unrelated].has_parameter_metadata = true;
  unrelated_evidence[unrelated].within_authorized_boundary = true;
  unrelated_evidence[unrelated].domain = authoritative.domain;
  require(bounded::collect_source_edge_membership_proposals(
              transverse.relations->event_seeds(),
              transverse.relations->constructions(), unrelated_evidence,
              interning, incidence, memberships, adapter_error),
          "unrelated global evidence escaped construction-scoped authority");

  auto missing_constructions = transverse.relations->constructions();
  missing_constructions[first_seed.construction.ordinal()]
      .interval_evidence_count = 0;
  require(!bounded::collect_source_edge_membership_proposals(
              transverse.relations->event_seeds(), missing_constructions,
              transverse.relations->interval_evidence(), interning, incidence,
              memberships, adapter_error) &&
              adapter_error.subcode == static_cast<std::uint32_t>(
                  bounded::intersection_subcode::parameter_invalid),
          "missing construction-scoped parameter evidence did not fail closed");

  auto ambiguous_evidence = transverse.relations->interval_evidence();
  const auto duplicate_ordinal =
      first_construction.interval_evidence_begin + 1;
  require(duplicate_ordinal <
              first_construction.interval_evidence_begin +
                  first_construction.interval_evidence_count,
          "source-edge adapter fixture lacks ambiguity mutation space");
  auto &duplicate = ambiguous_evidence[duplicate_ordinal];
  duplicate.source_relation = authoritative.source_relation;
  duplicate.kind = authoritative.kind;
  duplicate.occurrence = authoritative.occurrence;
  duplicate.has_rounded_nominal = true;
  duplicate.has_parameter_metadata = true;
  duplicate.within_authorized_boundary = true;
  duplicate.rounded_nominal_bits = authoritative.rounded_nominal_bits;
  duplicate.lower_bits = authoritative.lower_bits;
  duplicate.upper_bits = authoritative.upper_bits;
  duplicate.domain = authoritative.domain;
  duplicate.exact_zero = authoritative.exact_zero;
  duplicate.exact_one = authoritative.exact_one;
  require(!bounded::collect_source_edge_membership_proposals(
              transverse.relations->event_seeds(),
              transverse.relations->constructions(), ambiguous_evidence,
              interning, incidence, memberships, adapter_error) &&
              adapter_error.subcode == static_cast<std::uint32_t>(
                  bounded::intersection_subcode::parameter_invalid),
          "ambiguous construction-scoped parameter evidence did not fail closed");

  bounded::resource_manager resources(resource_policy::conservative_defaults());
  auto outcome = bounded::build_canonical_intersection_complex(
      transverse.broad.predecessor.context,
      *transverse.broad.predecessor.precision, transverse.relations,
      stage_capabilities(transverse, resources));
  if (outcome.has_value() ||
      outcome.error()->subcode != static_cast<std::uint32_t>(
          bounded::intersection_subcode::membership_incomplete) ||
      outcome.error()->checkpoint != static_cast<std::uint32_t>(
          bounded::intersection_checkpoint::transverse_carriers)) {
    const auto summary = outcome.has_value()
                             ? std::string("unexpected success")
                             : std::string(outcome.error()->summary) +
                                   " subcode=" +
                                   std::to_string(outcome.error()->subcode) +
                                   " checkpoint=" +
                                   std::to_string(outcome.error()->checkpoint);
    throw std::runtime_error(
        "nonempty Component 08 stage did not advance through source-edge adaptation: " +
        summary);
  }
  require_no_live_resources(
      resources, "fail-closed transverse stage gate leaked resources");
}

std::uint64_t arrangement_storage_bytes(
    const bounded::source_edge_arrangement_tables &tables) {
  return sizeof(tables) +
         tables.memberships.size() * sizeof(tables.memberships.front()) +
         tables.membership_sequence_index.size() *
             sizeof(tables.membership_sequence_index.front()) +
         tables.sequences.size() * sizeof(tables.sequences.front()) +
         tables.clusters.size() * sizeof(tables.clusters.front()) +
         tables.cluster_occurrence_index.size() *
             sizeof(tables.cluster_occurrence_index.front()) +
         tables.cluster_membership_index.size() *
             sizeof(tables.cluster_membership_index.front()) +
         tables.sequence_cluster_index.size() *
             sizeof(tables.sequence_cluster_index.front()) +
         tables.intervals.size() * sizeof(tables.intervals.front()) +
         tables.sequence_interval_index.size() *
             sizeof(tables.sequence_interval_index.front()) +
         tables.ordering_certificates.size() *
             sizeof(tables.ordering_certificates.front());
}

bounded::source_edge_arrangement_tables sparse_arrangement(std::uint32_t count) {
  std::vector<bounded::source_edge_membership_proposal> proposals;
  proposals.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i)
    proposals.push_back(proposal(i, i + 1));
  bounded::source_edge_arrangement_tables tables;
  bounded_boolean_error error;
  require(bounded::build_source_edge_arrangements<double>(
              {source_domain()}, proposals, tables, error),
          "sparse structural arrangement failed");
  return tables;
}

bounded::source_edge_arrangement_tables dense_arrangement(std::uint32_t count) {
  std::vector<bounded::source_edge_membership_proposal> proposals;
  proposals.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i)
    proposals.push_back(proposal(i, 32));
  bounded::source_edge_arrangement_tables tables;
  bounded_boolean_error error;
  require(bounded::build_source_edge_arrangements<double>(
              {source_domain()}, proposals, tables, error),
          "dense structural arrangement failed");
  return tables;
}

void test_structural_performance() {
  const auto sparse32 = sparse_arrangement(32);
  const auto sparse63 = sparse_arrangement(63);
  require(sparse32.sequences.front().comparison_count == 31 &&
              sparse63.sequences.front().comparison_count == 62,
          "well-separated source-edge ordering is not linear after sweep sort");
  require(sparse63.ordering_certificates.size() == 62,
          "sparse ordering published unexpected pair certificates");

  const auto dense24 = dense_arrangement(24);
  const auto dense48 = dense_arrangement(48);
  require(dense24.sequences.front().comparison_count == 24ULL * 23ULL / 2ULL &&
              dense48.sequences.front().comparison_count == 48ULL * 47ULL / 2ULL,
          "dense overlap work is not the exact output-sensitive all-pairs count");
  require(dense48.clusters.size() == 1 &&
              dense48.clusters.front().member_occurrences.count == 48,
          "dense exact-equal cluster lost distinct occurrences");

  const auto sparse_bytes32 = arrangement_storage_bytes(sparse32);
  const auto sparse_bytes63 = arrangement_storage_bytes(sparse63);
  require(sparse_bytes63 > sparse_bytes32 &&
              sparse_bytes63 < sparse_bytes32 * 3,
          "persistent source-edge storage does not scale with output records");
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "exact")
      test_exact_oracle_and_golden();
    if (suite == "all" || suite == "fuzz")
      test_deterministic_fuzz_and_shrink();
    if (suite == "all" || suite == "resources")
      test_resource_boundaries();
    if (suite == "all" || suite == "cancellation")
      test_cancellation_matrix();
    if (suite == "all" || suite == "concurrency")
      test_execution_determinism_and_fail_closed_gate();
    if (suite == "all" || suite == "structural")
      test_structural_performance();
    std::cout << "Component 08 qualification suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
