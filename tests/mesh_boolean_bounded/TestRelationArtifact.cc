#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/RelationVerifier.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using broad_phase_tests::built_fixture;
using broad_phase_tests::diagnostic;
using broad_phase_tests::require;

namespace ygor::mesh_boolean::bounded {

struct relation_artifact_test_access final {
  template <class T, class I>
  static signed_feature_relations<T, I>
  copy(const signed_feature_relations<T, I> &artifact) {
    return artifact;
  }

  template <class T, class I>
  static auto &imported_geometry(signed_feature_relations<T, I> &artifact) {
    return artifact.imported_geometry_;
  }

  template <class T, class I>
  static auto &bounded_primitives(signed_feature_relations<T, I> &artifact) {
    return artifact.bounded_primitives_;
  }

  template <class T, class I>
  static auto &exact_relations(signed_feature_relations<T, I> &artifact) {
    return artifact.exact_relations_;
  }

  template <class T, class I>
  static auto &truth_lineage(signed_feature_relations<T, I> &artifact) {
    return artifact.truth_lineage_;
  }

  template <class T, class I>
  static auto &interval_evidence(signed_feature_relations<T, I> &artifact) {
    return artifact.interval_evidence_;
  }

  template <class T, class I>
  static auto &source_facet_regions(signed_feature_relations<T, I> &artifact) {
    return artifact.source_facet_regions_;
  }

  template <class T, class I>
  static auto &constructions(signed_feature_relations<T, I> &artifact) {
    return artifact.constructions_;
  }

  template <class T, class I>
  static auto &construction_ledger(signed_feature_relations<T, I> &artifact) {
    return artifact.construction_ledger_;
  }

  template <class T, class I>
  static auto &crossings(signed_feature_relations<T, I> &artifact) {
    return artifact.crossings_;
  }

  template <class T, class I>
  static auto &event_seeds(signed_feature_relations<T, I> &artifact) {
    return artifact.event_seeds_;
  }

  template <class T, class I>
  static auto &event_seed_incidence(
      signed_feature_relations<T, I> &artifact) {
    return artifact.event_seed_incidence_;
  }

  template <class T, class I>
  static auto &event_seed_candidate_incidence(
      signed_feature_relations<T, I> &artifact) {
    return artifact.event_seed_candidate_incidence_;
  }

  template <class T, class I>
  static auto &candidate_relation_coverage(
      signed_feature_relations<T, I> &artifact) {
    return artifact.candidate_relation_coverage_;
  }

  template <class T, class I>
  static auto &candidate_event_seed_coverage(
      signed_feature_relations<T, I> &artifact) {
    return artifact.candidate_event_seed_coverage_;
  }

  template <class T, class I>
  static auto &candidate_partitions(
      signed_feature_relations<T, I> &artifact) {
    return artifact.candidate_partitions_;
  }

  template <class T, class I>
  static auto &symbolic_eligibility(
      signed_feature_relations<T, I> &artifact) {
    return artifact.symbolic_eligibility_;
  }

  template <class T, class I>
  static auto &symbolic_decisions(
      signed_feature_relations<T, I> &artifact) {
    return artifact.symbolic_decisions_;
  }

  template <class T, class I>
  static auto &coplanar_event_nodes(
      signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_event_nodes_;
  }

  template <class T, class I>
  static auto &coplanar_oriented_arcs(
      signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_oriented_arcs_;
  }

  template <class T, class I>
  static auto &coplanar_overlap_components(
      signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_overlap_components_;
  }

  template <class T, class I>
  static auto &dispositions(signed_feature_relations<T, I> &artifact) {
    return artifact.candidate_dispositions_;
  }

  template <class T, class I>
  static auto &canonical_bytes(signed_feature_relations<T, I> &artifact) {
    return artifact.canonical_bytes_;
  }

  template <class T, class I>
  static void set_owner(signed_feature_relations<T, I> &artifact,
                        context_owner_token owner) {
    artifact.owner_ = std::move(owner);
  }

  template <class T, class I>
  static void repair_codec(signed_feature_relations<T, I> &artifact) {
    artifact.canonical_bytes_ = encode_signed_feature_relations(artifact);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace {

bounded::relation_capabilities capabilities(
    built_fixture &fixture, bounded::resource_manager *resources = nullptr) {
  bounded::relation_capabilities out;
  out.owner = fixture.predecessor.context.owner;
  out.resources = resources ? resources : fixture.predecessor.resources.get();
  return out;
}

std::shared_ptr<const bounded::signed_feature_relations<double, std::uint32_t>>
build_artifact(built_fixture &fixture,
               bounded::resource_manager *resources = nullptr) {
  auto result = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities(fixture, resources));
  if (!result.has_value())
    throw std::runtime_error(diagnostic(*result.error()));
  return *result.value();
}

built_fixture overlapping_fixture() {
  return broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
}

built_fixture symbolic_fixture() {
  return broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0));
}


void require_no_live_resources(const bounded::resource_manager &resources,
                               const char *message) {
  const auto snapshot = resources.snapshot();
  for (const auto &counter : snapshot)
    if (counter.reserved != 0 || counter.committed != 0)
      throw std::runtime_error(message);
}

void test_empty_artifact_and_decode() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  require(fixture.artifact->candidates().empty(),
          "empty relation fixture must have no candidates");
  bounded::resource_manager resources(
      resource_policy::conservative_defaults());
  const auto artifact = build_artifact(fixture, &resources);
  require(artifact->request_graph().requests.empty() &&
              artifact->relations().empty() && artifact->constructions().empty() &&
              artifact->candidate_dispositions().empty(),
          "empty candidates publish a canonical empty relation artifact");
  require(artifact->source_edge_stage() && artifact->source_edge_facet_stage() &&
              artifact->source_facet_stage() && artifact->coplanar_overlay_stage(),
          "empty artifact retains all verified detailed stages");
  bounded_boolean_error verify_error;
  require(bounded::verify_signed_feature_relations(*artifact, verify_error),
          "empty relation artifact independently verifies");

  auto decode_fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      artifact->canonical_bytes(), decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, &decode_resources));
  if (!decoded.has_value())
    throw std::runtime_error(diagnostic(*decoded.error()));
  require((*decoded.value())->canonical_bytes() == artifact->canonical_bytes(),
          "empty relation decode reproduces canonical bytes");
}

void test_nonempty_determinism_and_decode() {
  auto first_fixture = overlapping_fixture();
  auto second_fixture = overlapping_fixture();
  bounded::resource_manager first_resources(
      resource_policy::conservative_defaults());
  bounded::resource_manager second_resources(
      resource_policy::conservative_defaults());
  const auto first = build_artifact(first_fixture, &first_resources);
  const auto second = build_artifact(second_fixture, &second_resources);

  require(!first->relations().empty() &&
              !first->imported_geometry().empty() &&
              first->bounded_primitives().size() ==
                  first->truth_records().size() &&
              first->truth_lineage().size() == first->truth_records().size() &&
              !first->interval_evidence().empty() &&
              !first->source_facet_regions().empty() &&
              !first->constructions().empty() &&
              !first->construction_ledger().empty() &&
              !first->event_seeds().empty() &&
              !first->event_seed_candidate_incidence().empty() &&
              first->candidate_dispositions().size() ==
                  first_fixture.artifact->candidates().size() &&
              first->candidate_partitions().size() ==
                  first_fixture.artifact->partitions().size(),
          "nonempty artifact publishes primitive support, complete event incidence, and candidate partitions");
  std::size_t expected_exact = 0;
  for (const auto &truth : first->truth_records())
    expected_exact += truth.exact_formula != 0 ? 1U : 0U;
  require(first->exact_relations().size() == expected_exact &&
              first->statistics().imported_geometry_count ==
                  first->imported_geometry().size() &&
              first->statistics().bounded_primitive_count ==
                  first->bounded_primitives().size() &&
              first->statistics().exact_relation_count ==
                  first->exact_relations().size() &&
              first->statistics().truth_lineage_count ==
                  first->truth_lineage().size() &&
              first->statistics().interval_evidence_count ==
                  first->interval_evidence().size() &&
              first->statistics().source_facet_region_count ==
                  first->source_facet_regions().size() &&
              first->statistics().construction_count ==
                  first->constructions().size() &&
              first->statistics().construction_ledger_count ==
                  first->construction_ledger().size() &&
              first->statistics().event_seed_candidate_incidence_count ==
                  first->event_seed_candidate_incidence().size() &&
              first->statistics().candidate_relation_coverage_count ==
                  first->candidate_relation_coverage().size() &&
              first->statistics().candidate_seed_coverage_count ==
                  first->candidate_event_seed_coverage().size() &&
              first->statistics().candidate_partition_count ==
                  first->candidate_partitions().size(),
          "primitive, construction, event-incidence, and candidate-coverage counts reconstruct from final records");

  std::size_t expected_ledger_begin = 0;
  for (const auto &construction : first->constructions()) {
    require(construction.id.ordinal() < first->constructions().size() &&
                construction.ledger_begin == expected_ledger_begin &&
                construction.ledger_count >= 2 &&
                construction.ledger_begin + construction.ledger_count <=
                    first->construction_ledger().size() &&
                construction.source_relation.ordinal() <
                    first->relations().size() &&
                construction.precision_evidence_complete &&
                construction.finite && construction.tolerance_compatible,
            "each authoritative construction owns one complete contiguous ledger range");
    const auto &authority =
        first->construction_ledger()[construction.ledger_begin];
    require(authority.id.ordinal() == construction.ledger_begin &&
                authority.construction == construction.id &&
                authority.source_relation == construction.source_relation &&
                authority.precedence == construction.precedence &&
                authority.synthetic_authority &&
                authority.lineage_compatible &&
                authority.enclosure_compatible &&
                authority.parameter_compatible &&
                authority.residual_compatible &&
                authority.precision_evidence_complete,
            "construction ledger begins with the reconstructed synthetic authority");
    for (std::uint64_t offset = 1; offset < construction.ledger_count; ++offset) {
      const auto &witness =
          first->construction_ledger()[construction.ledger_begin + offset];
      require(witness.id.ordinal() == construction.ledger_begin + offset &&
                  witness.construction == construction.id &&
                  witness.source_relation.ordinal() < first->relations().size() &&
                  !witness.synthetic_authority &&
                  witness.lineage_compatible &&
                  witness.enclosure_compatible &&
                  witness.parameter_compatible &&
                  witness.residual_compatible &&
                  witness.precision_evidence_complete,
              "every construction witness retains complete compatibility evidence");
    }
    expected_ledger_begin += construction.ledger_count;
  }
  require(expected_ledger_begin == first->construction_ledger().size(),
          "construction registry partitions the complete witness ledger");

  std::size_t expected_candidate_incidence = 0;
  for (const auto &seed : first->event_seeds()) {
    require(seed.schema_version ==
                bounded::contract_versions::relation_event_seed_schema &&
                seed.candidate_incidence_begin == expected_candidate_incidence &&
                seed.candidate_incidence_count != 0 &&
                seed.precision_evidence_complete &&
                seed.contact_dimension != bounded::relation_contact_dimension::none,
            "each event seed retains complete contact, precision, and candidate incidence");
    for (std::uint64_t offset = 0; offset < seed.candidate_incidence_count;
         ++offset) {
      const auto &incidence = first->event_seed_candidate_incidence()[
          seed.candidate_incidence_begin + offset];
      require(incidence.id.ordinal() ==
                  seed.candidate_incidence_begin + offset &&
                  incidence.seed == seed.id &&
                  incidence.disposition.ordinal() ==
                      incidence.candidate.ordinal() &&
                  incidence.schema_version ==
                      bounded::contract_versions::
                          relation_event_seed_incidence_schema,
              "event-seed incidence retains canonical candidate and disposition identity");
    }
    expected_candidate_incidence += seed.candidate_incidence_count;
  }
  require(expected_candidate_incidence ==
              first->event_seed_candidate_incidence().size(),
          "event seeds partition the complete candidate-incidence table");

  std::size_t relation_coverage = 0;
  std::size_t seed_coverage = 0;
  for (const auto &disposition : first->candidate_dispositions()) {
    require(disposition.relation_begin == relation_coverage &&
                disposition.event_seed_begin == seed_coverage &&
                disposition.coverage_complete &&
                (disposition.coverage_flags &
                 bounded::candidate_coverage_complete) != 0 &&
                disposition.schema_version ==
                    bounded::contract_versions::
                        relation_candidate_disposition_schema,
            "each candidate publishes canonical complete relation and seed coverage");
    relation_coverage += disposition.relation_count;
    seed_coverage += disposition.event_seed_count;
  }
  require(relation_coverage == first->candidate_relation_coverage().size() &&
              seed_coverage ==
                  first->candidate_event_seed_coverage().size(),
          "candidate dispositions partition the complete coverage tables");
  for (std::size_t i = 0; i < first->candidate_partitions().size(); ++i) {
    const auto &published = first->candidate_partitions()[i];
    const auto &source = first_fixture.artifact->partitions()[i];
    require(published.id.ordinal() == i &&
                published.source_partition == source.id &&
                published.candidate_begin == source.begin &&
                published.candidate_count == source.count &&
                published.disposition_begin == source.begin &&
                published.disposition_count == source.count &&
                published.maximum_records == source.maximum_records,
            "candidate partitions preserve Component 06 canonical boundaries");
  }

  require(first->canonical_bytes() == second->canonical_bytes() &&
              first->digest() == second->digest(),
          "different runtime owner anchors produce identical relation semantics");
  require(first->statistics().persistent_bytes != 0 &&
              first->statistics().verifier_work_units != 0 &&
              first->verification_evidence().graph_reconstructed &&
              first->verification_evidence().owner_exclusion_checked &&
              first->verification_evidence().selection_boundary_checked &&
              first->verification_evidence().candidate_dispositions_complete,
          "published relation artifact carries complete verification evidence");

  auto owner_changed = bounded::relation_artifact_test_access::copy(*first);
  const auto original_bytes = bounded::encode_signed_feature_relations(owner_changed);
  bounded::relation_artifact_test_access::set_owner(
      owner_changed, bounded::context_owner_token::create());
  require(bounded::encode_signed_feature_relations(owner_changed) ==
              original_bytes,
          "runtime owner anchor is absent from relation semantic bytes");

  auto decode_fixture = overlapping_fixture();
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      first->canonical_bytes(), decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, &decode_resources));
  if (!decoded.has_value())
    throw std::runtime_error(diagnostic(*decoded.error()));
  require((*decoded.value())->digest() == first->digest(),
          "nonempty relation decode rebuilds and independently verifies artifact");
}

void test_matched_mutation_rejection() {
  auto fixture = overlapping_fixture();
  bounded::resource_manager resources(
      resource_policy::conservative_defaults());
  const auto artifact = build_artifact(fixture, &resources);
  bounded_boolean_error error;

  auto disposition_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!disposition_mutation.candidate_dispositions().empty(),
          "mutation fixture requires candidate dispositions");
  auto &dispositions =
      bounded::relation_artifact_test_access::dispositions(disposition_mutation);
  dispositions.front().candidate = bounded::candidate_id(
      disposition_mutation.candidate_dispositions().size());
  bounded::relation_artifact_test_access::repair_codec(disposition_mutation);
  require(!bounded::verify_signed_feature_relations(disposition_mutation, error),
          "matched candidate-disposition mutation is rejected");

  auto import_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!import_mutation.imported_geometry().empty(),
          "mutation fixture requires imported geometry");
  auto &import_record =
      bounded::relation_artifact_test_access::imported_geometry(import_mutation)
          .front();
  ++import_record.feature.primary;
  bounded::relation_artifact_test_access::repair_codec(import_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(import_mutation, error),
          "matched imported-geometry mutation is independently rejected");

  auto bounded_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!bounded_mutation.bounded_primitives().empty(),
          "mutation fixture requires bounded primitives");
  auto &bounded_record =
      bounded::relation_artifact_test_access::bounded_primitives(
          bounded_mutation)
          .front();
  bounded_record.rounded_nominal_bits ^= std::uint64_t{1};
  bounded::relation_artifact_test_access::repair_codec(bounded_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(bounded_mutation, error),
          "matched bounded-primitive mutation is independently rejected");

  if (!artifact->exact_relations().empty()) {
    auto exact_mutation =
        bounded::relation_artifact_test_access::copy(*artifact);
    auto &exact_record =
        bounded::relation_artifact_test_access::exact_relations(exact_mutation)
            .front();
    exact_record.exact_formula ^= std::uint16_t{1};
    bounded::relation_artifact_test_access::repair_codec(exact_mutation);
    error = bounded_boolean_error{};
    require(!bounded::verify_signed_feature_relations(exact_mutation, error),
            "matched exact-relation mutation is independently rejected");
  }

  auto interval_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!interval_mutation.interval_evidence().empty(),
          "mutation fixture requires interval evidence");
  auto &interval_record =
      bounded::relation_artifact_test_access::interval_evidence(
          interval_mutation)
          .front();
  interval_record.lower_bits ^= std::uint64_t{1};
  bounded::relation_artifact_test_access::repair_codec(interval_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(interval_mutation, error),
          "matched interval-evidence mutation is independently rejected");

  auto region_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!region_mutation.source_facet_regions().empty(),
          "mutation fixture requires source-facet region evidence");
  auto &region_record =
      bounded::relation_artifact_test_access::source_facet_regions(
          region_mutation)
          .front();
  region_record.query_nominal_bits[0] ^= std::uint64_t{1};
  bounded::relation_artifact_test_access::repair_codec(region_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(region_mutation, error),
          "matched source-facet-region mutation is independently rejected");

  auto construction_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!construction_mutation.constructions().empty(),
          "mutation fixture requires authoritative constructions");
  auto &construction_record =
      bounded::relation_artifact_test_access::constructions(
          construction_mutation)
          .front();
  ++construction_record.authoritative_source_feature.primary;
  bounded::relation_artifact_test_access::repair_codec(construction_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(construction_mutation, error),
          "matched construction-authority mutation is independently rejected");

  auto ledger_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(ledger_mutation.construction_ledger().size() >= 2,
          "mutation fixture requires construction witness ledger entries");
  auto &ledger =
      bounded::relation_artifact_test_access::construction_ledger(
          ledger_mutation);
  const auto witness = std::find_if(
      ledger.begin(), ledger.end(),
      [](const auto &record) { return !record.synthetic_authority; });
  require(witness != ledger.end(),
          "mutation fixture requires a non-authority construction witness");
  witness->enclosure_compatible = false;
  bounded::relation_artifact_test_access::repair_codec(ledger_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(ledger_mutation, error),
          "matched construction-ledger compatibility mutation is independently rejected");

  auto seed_table_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!seed_table_mutation.event_seeds().empty(),
          "mutation fixture requires event seeds");
  bounded::relation_artifact_test_access::event_seeds(seed_table_mutation)
      .clear();
  bounded::relation_artifact_test_access::event_seed_incidence(
      seed_table_mutation)
      .clear();
  bounded::relation_artifact_test_access::repair_codec(seed_table_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(seed_table_mutation, error),
          "matched missing event-seed table is independently rejected");

  auto occurrence_separation_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!occurrence_separation_mutation.event_seeds().empty(),
          "mutation fixture requires an event seed");
  auto &mutated_seed =
      bounded::relation_artifact_test_access::event_seeds(
          occurrence_separation_mutation)
          .front();
  mutated_seed.distinct_occurrence_required =
      !mutated_seed.distinct_occurrence_required;
  bounded::relation_artifact_test_access::repair_codec(
      occurrence_separation_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(
              occurrence_separation_mutation, error),
          "matched event-seed occurrence-separation mutation is independently rejected");

  auto candidate_incidence_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!candidate_incidence_mutation.event_seed_candidate_incidence().empty(),
          "mutation fixture requires event-seed candidate incidence");
  auto &candidate_incidence =
      bounded::relation_artifact_test_access::event_seed_candidate_incidence(
          candidate_incidence_mutation)
          .front();
  ++candidate_incidence.triangle_halfedges[0];
  bounded::relation_artifact_test_access::repair_codec(
      candidate_incidence_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(
              candidate_incidence_mutation, error),
          "matched event-seed candidate-halfedge mutation is independently rejected");

  auto candidate_coverage_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!candidate_coverage_mutation.candidate_relation_coverage().empty(),
          "mutation fixture requires candidate relation coverage");
  auto &coverage =
      bounded::relation_artifact_test_access::candidate_relation_coverage(
          candidate_coverage_mutation)
          .front();
  coverage = bounded::feature_relation_id(
      static_cast<std::uint64_t>(artifact->relations().size()));
  bounded::relation_artifact_test_access::repair_codec(
      candidate_coverage_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(
              candidate_coverage_mutation, error),
          "matched candidate relation-coverage mutation is independently rejected");

  auto candidate_partition_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!candidate_partition_mutation.candidate_partitions().empty(),
          "mutation fixture requires candidate partitions");
  ++bounded::relation_artifact_test_access::candidate_partitions(
         candidate_partition_mutation)
         .front()
         .maximum_records;
  bounded::relation_artifact_test_access::repair_codec(
      candidate_partition_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(
              candidate_partition_mutation, error),
          "matched candidate-partition mutation is independently rejected");

  auto lineage_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!lineage_mutation.truth_lineage().empty(),
          "mutation fixture requires truth lineage");
  auto &lineage_record =
      bounded::relation_artifact_test_access::truth_lineage(lineage_mutation)
          .front();
  lineage_record.truth_ordinal += 1U;
  bounded::relation_artifact_test_access::repair_codec(lineage_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(lineage_mutation, error),
          "matched truth-lineage mutation is independently rejected");

  if (!artifact->crossings().empty()) {
    auto crossing_mutation =
        bounded::relation_artifact_test_access::copy(*artifact);
    auto &crossing =
        bounded::relation_artifact_test_access::crossings(crossing_mutation)
            .front();
    crossing.numeric_crossing = crossing.numeric_crossing == 1 ? -1 : 1;
    bounded::relation_artifact_test_access::repair_codec(crossing_mutation);
    error = bounded_boolean_error{};
    require(!bounded::verify_signed_feature_relations(crossing_mutation, error),
            "matched crossing multiplicity mutation is rejected");

    auto fan_mutation =
        bounded::relation_artifact_test_access::copy(*artifact);
    auto &fan =
        bounded::relation_artifact_test_access::crossings(fan_mutation).front();
    ++fan.source_fan_group_size;
    bounded::relation_artifact_test_access::repair_codec(fan_mutation);
    error = bounded_boolean_error{};
    require(!bounded::verify_signed_feature_relations(fan_mutation, error),
            "matched source-fan cardinality mutation is rejected");
  }

  auto symbolic_source = symbolic_fixture();
  bounded::resource_manager symbolic_resources(
      resource_policy::conservative_defaults());
  const auto symbolic_artifact =
      build_artifact(symbolic_source, &symbolic_resources);
  bool saw_source_vertex_authority = false;
  bool saw_cross_family_construction = false;
  for (const auto &construction : symbolic_artifact->constructions()) {
    if (construction.precedence !=
        bounded::relation_construction_precedence::accepted_source_vertex)
      continue;
    saw_source_vertex_authority = true;
    const auto authority_family = symbolic_artifact->relations()[
        construction.source_relation.ordinal()].family;
    for (std::uint64_t offset = 1; offset < construction.ledger_count; ++offset) {
      const auto &witness = symbolic_artifact->construction_ledger()[
          construction.ledger_begin + offset];
      saw_cross_family_construction =
          saw_cross_family_construction ||
          symbolic_artifact->relations()[witness.source_relation.ordinal()].family !=
              authority_family;
    }
  }
  require(saw_source_vertex_authority && saw_cross_family_construction,
          "accepted source vertices are deduplicated across edge-facet and coplanar relation families");
  require(!symbolic_artifact->symbolic_eligibility().empty() &&
              symbolic_artifact->symbolic_eligibility().size() ==
                  symbolic_artifact->symbolic_decisions().size(),
          "qualified exact ties publish complete symbolic evidence and decisions");
  bool saw_facet_lineage = false;
  bool saw_coincident_contract = false;
  for (const auto &eligibility :
       symbolic_artifact->symbolic_eligibility()) {
    require(eligibility.exact_relation ==
                    bounded::exact_relation_status::exact_zero &&
                eligibility.reason !=
                    bounded::symbolic_eligibility_reason::none &&
                eligibility.evidence_formula_version != 0 &&
                eligibility.exact_lineage_tie &&
                eligibility.structural_category_eligible &&
                eligibility.tolerance_compatible &&
                !eligibility.separated_realizations_possible &&
                eligibility.owner_is_original_source_feature,
            "symbolic eligibility retains qualified exact and structural evidence");
    saw_facet_lineage =
        saw_facet_lineage ||
        eligibility.reason ==
            bounded::symbolic_eligibility_reason::coplanar_source_facet_lineage;
    saw_coincident_contract =
        saw_coincident_contract ||
        eligibility.reason ==
            bounded::symbolic_eligibility_reason::coincident_source_contract;
  }
  require(saw_facet_lineage && saw_coincident_contract,
          "symbolic fixture covers support and overlay eligibility categories");

  bool saw_acting_owner = false;
  bool saw_opposite_owner = false;
  bool saw_shared_owner = false;
  bool saw_coincident_pair = false;
  const auto inspect_symbolic_decisions =
      [&](const bounded::signed_feature_relations<double, std::uint32_t>
              &candidate_artifact,
          bool require_dual_counterparts) {
        for (const auto &decision : candidate_artifact.symbolic_decisions()) {
          saw_acting_owner = saw_acting_owner ||
              decision.rule_key.ownership_role ==
                  bounded::symbolic_ownership_role::acting_source_feature;
          saw_opposite_owner = saw_opposite_owner ||
              decision.rule_key.ownership_role ==
                  bounded::symbolic_ownership_role::opposite_source_feature;
          saw_shared_owner = saw_shared_owner ||
              decision.rule_key.ownership_role ==
                  bounded::symbolic_ownership_role::shared_source_feature;
          saw_coincident_pair = saw_coincident_pair ||
              decision.rule_key.ownership_role ==
                  bounded::symbolic_ownership_role::coincident_sheet_pair;
          require(
              bounded::valid_symbolic_rule_key(decision.rule_key) &&
                  bounded::valid_symbolic_tie_key_description(decision.tie_key) &&
                  decision.tie_key.feature_priority ==
                      decision.feature_priority &&
                  decision.tie_key.preferred_operand ==
                      decision.coincident_owner_rank &&
                  decision.exchanged_rule_key ==
                      bounded::exchange_symbolic_rule_key(decision.rule_key) &&
                  decision.exchange_rule_ordinal ==
                      bounded::symbolic_rule_ordinal(
                          decision.exchanged_rule_key),
              "symbolic decisions publish complete frozen rule and tie-key consequences");
          const bool dual_subject =
              decision.subject_kind ==
                  bounded::symbolic_relation_subject_kind::coplanar_component ||
              decision.matrix_family == bounded::relation_family::coplanar ||
              decision.matrix_family ==
                  bounded::relation_family::coincident_face;
          if (!require_dual_counterparts || !dual_subject ||
              decision.exchanged_rule_key.operation !=
                  candidate_artifact.operation())
            continue;
          const auto counterpart = std::find_if(
              candidate_artifact.symbolic_decisions().begin(),
              candidate_artifact.symbolic_decisions().end(),
              [&](const auto &candidate) {
                return candidate.rule_key == decision.exchanged_rule_key &&
                       candidate.subject_kind == decision.subject_kind &&
                       candidate.subject_ordinal == decision.subject_ordinal;
              });
          require(counterpart != candidate_artifact.symbolic_decisions().end() &&
                      counterpart->exchange_rule_ordinal ==
                          decision.stable_rule_ordinal,
                  "same-operation operand exchange publishes its exact counterpart");
        }
      };
  inspect_symbolic_decisions(*symbolic_artifact, true);
  require(saw_acting_owner && saw_coincident_pair,
          "supported artifact fixtures publish acting-feature and coincident-sheet symbolic roles");
  require(!saw_opposite_owner && !saw_shared_owner,
          "unsupported isolated boundary contacts are not fabricated merely to populate symbolic roles");

  std::size_t expected_nodes = 0;
  std::size_t expected_arcs = 0;
  std::size_t expected_components = 0;
  for (const auto &overlay :
       symbolic_artifact->coplanar_overlay_stage()->overlays) {
    expected_nodes += overlay.event_nodes.size();
    expected_arcs += overlay.oriented_arcs.size();
    expected_components += overlay.overlap_components.size();
  }
  require(expected_nodes != 0 && expected_arcs != 0 &&
              expected_components != 0 &&
              symbolic_artifact->coplanar_event_nodes().size() ==
                  expected_nodes &&
              symbolic_artifact->coplanar_oriented_arcs().size() ==
                  expected_arcs &&
              symbolic_artifact->coplanar_overlap_components().size() ==
                  expected_components &&
              symbolic_artifact->statistics().coplanar_event_node_count ==
                  expected_nodes &&
              symbolic_artifact->statistics().coplanar_oriented_arc_count ==
                  expected_arcs &&
              symbolic_artifact->statistics()
                      .coplanar_overlap_component_count == expected_components,
          "coplanar topology is published as complete first-class artifact tables");

  auto topology_decode_fixture = symbolic_fixture();
  bounded::resource_manager topology_decode_resources(
      resource_policy::conservative_defaults());
  auto topology_decoded = bounded::decode_signed_feature_relations(
      symbolic_artifact->canonical_bytes(),
      topology_decode_fixture.predecessor.context,
      *topology_decode_fixture.predecessor.precision,
      topology_decode_fixture.artifact,
      capabilities(topology_decode_fixture, &topology_decode_resources));
  require(topology_decoded.has_value() &&
              (*topology_decoded.value())->canonical_bytes() ==
                  symbolic_artifact->canonical_bytes() &&
              (*topology_decoded.value())->coplanar_event_nodes().size() ==
                  expected_nodes &&
              (*topology_decoded.value())->coplanar_oriented_arcs().size() ==
                  expected_arcs &&
              (*topology_decoded.value())->coplanar_overlap_components().size() ==
                  expected_components,
          "coplanar topology codec rebuilds identical first-class tables");

  auto node_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &node = bounded::relation_artifact_test_access::coplanar_event_nodes(
      node_mutation).front();
  node.sheet_mask ^= 1U;
  bounded::relation_artifact_test_access::repair_codec(node_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(node_mutation, error),
          "matched coplanar event-node mutation is independently rejected");

  auto arc_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &arc = bounded::relation_artifact_test_access::coplanar_oriented_arcs(
      arc_mutation).front();
  arc.start_node = arc.end_node;
  bounded::relation_artifact_test_access::repair_codec(arc_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(arc_mutation, error),
          "matched coplanar oriented-arc mutation is independently rejected");

  auto component_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &component =
      bounded::relation_artifact_test_access::coplanar_overlap_components(
          component_mutation).front();
  component.closed = !component.closed;
  bounded::relation_artifact_test_access::repair_codec(component_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(component_mutation, error),
          "matched coplanar component mutation is independently rejected");

  auto symbolic_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &symbolic = bounded::relation_artifact_test_access::symbolic_eligibility(
      symbolic_mutation).front();
  symbolic.rounded_nominal_zero = !symbolic.rounded_nominal_zero;
  bounded::relation_artifact_test_access::repair_codec(symbolic_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(symbolic_mutation, error),
          "matched symbolic evidence mutation is independently rejected");

  auto decision_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &decision = bounded::relation_artifact_test_access::symbolic_decisions(
      decision_mutation).front();
  decision.tie_key.components[0] =
      bounded::symbolic_tie_key_component::operand_priority;
  bounded::relation_artifact_test_access::repair_codec(decision_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(decision_mutation, error),
          "matched symbolic tie-key mutation is independently rejected");

  auto seed_symbolic_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &symbolic_seeds =
      bounded::relation_artifact_test_access::event_seeds(
          seed_symbolic_mutation);
  const auto symbolic_seed_it = std::find_if(
      symbolic_seeds.begin(), symbolic_seeds.end(),
      [](const auto &seed) { return seed.has_symbolic_decision; });
  require(symbolic_seed_it != symbolic_seeds.end(),
          "symbolic mutation fixture requires a symbolic event seed");
  auto &symbolic_seed = *symbolic_seed_it;
  symbolic_seed.conceptual_order =
      symbolic_seed.conceptual_order ==
              bounded::symbolic_offset_disposition::negative
          ? bounded::symbolic_offset_disposition::positive
          : bounded::symbolic_offset_disposition::negative;
  bounded::relation_artifact_test_access::repair_codec(
      seed_symbolic_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(seed_symbolic_mutation,
                                                    error),
          "matched symbolic seed consequence mutation is independently rejected");

  auto trailing = artifact->canonical_bytes();
  trailing.push_back(0);
  auto decode_fixture = overlapping_fixture();
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      trailing, decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, &decode_resources));
  require(!decoded.has_value() &&
              decoded.error()->subcode ==
                  static_cast<std::uint32_t>(bounded::relation_subcode::codec_error),
          "relation decode rejects trailing data before publication");
  require_no_live_resources(
      decode_resources,
      "failed relation decode must not reserve or commit resources");
}

void test_resource_boundary_and_cancellation() {
  auto reference_fixture = overlapping_fixture();
  bounded::resource_manager reference_resources(
      resource_policy::conservative_defaults());
  const auto reference = build_artifact(reference_fixture, &reference_resources);
  const auto reference_snapshot = reference_resources.snapshot();
  const auto persistent_used =
      reference_snapshot[static_cast<std::size_t>(
          bounded::resource_kind::persistent_bytes)]
          .committed;
  require(persistent_used == reference->statistics().persistent_bytes &&
              persistent_used > 0,
          "relation persistent accounting is exact and nonzero");

  auto limited_fixture = overlapping_fixture();
  auto limited_policy = resource_policy::conservative_defaults();
  limited_policy.persistent_bytes.hard = persistent_used - 1;
  limited_policy.persistent_bytes.advisory = persistent_used - 1;
  bounded::resource_manager limited_resources(limited_policy);
  auto limited = bounded::build_signed_feature_relations(
      limited_fixture.predecessor.context,
      *limited_fixture.predecessor.precision, limited_fixture.artifact,
      capabilities(limited_fixture, &limited_resources));
  require(!limited.has_value() &&
              limited.error()->category ==
                  bounded_boolean_error_category::resource_limit,
          "relation persistent limit-minus-one fails transactionally");
  require_no_live_resources(
      limited_resources,
      "resource-limited relation build must release every lease");

  auto ledger_limited_fixture = overlapping_fixture();
  bounded::resource_manager ledger_limited_resources(
      resource_policy::conservative_defaults());
  auto ledger_limited_caps =
      capabilities(ledger_limited_fixture, &ledger_limited_resources);
  ledger_limited_caps.maximum_construction_ledger =
      reference->construction_ledger().size() - 1;
  auto ledger_limited = bounded::build_signed_feature_relations(
      ledger_limited_fixture.predecessor.context,
      *ledger_limited_fixture.predecessor.precision,
      ledger_limited_fixture.artifact, ledger_limited_caps);
  require(!ledger_limited.has_value() &&
              ledger_limited.error()->category ==
                  bounded_boolean_error_category::resource_limit,
          "construction-ledger limit-minus-one fails before publication");
  require_no_live_resources(
      ledger_limited_resources,
      "construction-ledger capability failure must release every lease");

  auto incidence_limited_fixture = overlapping_fixture();
  bounded::resource_manager incidence_limited_resources(
      resource_policy::conservative_defaults());
  auto incidence_limited_caps =
      capabilities(incidence_limited_fixture, &incidence_limited_resources);
  incidence_limited_caps.maximum_event_seed_incidence =
      reference->event_seed_candidate_incidence().size() - 1;
  auto incidence_limited = bounded::build_signed_feature_relations(
      incidence_limited_fixture.predecessor.context,
      *incidence_limited_fixture.predecessor.precision,
      incidence_limited_fixture.artifact, incidence_limited_caps);
  require(!incidence_limited.has_value() &&
              incidence_limited.error()->category ==
                  bounded_boolean_error_category::resource_limit,
          "event-seed candidate-incidence limit-minus-one fails before publication");
  require_no_live_resources(
      incidence_limited_resources,
      "event-seed incidence capability failure must release every lease");

  auto coverage_limited_fixture = overlapping_fixture();
  bounded::resource_manager coverage_limited_resources(
      resource_policy::conservative_defaults());
  auto coverage_limited_caps =
      capabilities(coverage_limited_fixture, &coverage_limited_resources);
  const auto maximum_coverage = std::max(
      reference->candidate_relation_coverage().size(),
      reference->candidate_event_seed_coverage().size());
  require(maximum_coverage != 0,
          "coverage resource fixture requires nonempty candidate coverage");
  coverage_limited_caps.maximum_candidate_coverage = maximum_coverage - 1;
  auto coverage_limited = bounded::build_signed_feature_relations(
      coverage_limited_fixture.predecessor.context,
      *coverage_limited_fixture.predecessor.precision,
      coverage_limited_fixture.artifact, coverage_limited_caps);
  require(!coverage_limited.has_value() &&
              coverage_limited.error()->category ==
                  bounded_boolean_error_category::resource_limit,
          "candidate coverage limit-minus-one fails before publication");
  require_no_live_resources(
      coverage_limited_resources,
      "candidate coverage capability failure must release every lease");

  auto cancelled_fixture = overlapping_fixture();
  bounded::resource_manager cancelled_resources(
      resource_policy::conservative_defaults());
  bounded_boolean_cancellation_source source;
  auto token = source.token();
  source.request_cancel(7);
  auto cancelled_caps = capabilities(cancelled_fixture, &cancelled_resources);
  cancelled_caps.cancellation = &token;
  auto cancelled = bounded::build_signed_feature_relations(
      cancelled_fixture.predecessor.context,
      *cancelled_fixture.predecessor.precision, cancelled_fixture.artifact,
      cancelled_caps);
  require(!cancelled.has_value() &&
              cancelled.error()->category ==
                  bounded_boolean_error_category::cancelled,
          "pre-cancelled relation build publishes nothing");
  require_no_live_resources(
      cancelled_resources,
      "cancelled relation build must release every lease");

  bounded::resource_manager retry_resources(
      resource_policy::conservative_defaults());
  const auto retry = build_artifact(cancelled_fixture, &retry_resources);
  require(retry->canonical_bytes() == reference->canonical_bytes(),
          "retry after cancellation reproduces canonical relation bytes");
}

} // namespace

int main() {
  try {
    test_empty_artifact_and_decode();
    test_nonempty_determinism_and_decode();
    test_matched_mutation_rejection();
    test_resource_boundary_and_cancellation();
    std::cout << "Component 07 final artifact qualification checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
