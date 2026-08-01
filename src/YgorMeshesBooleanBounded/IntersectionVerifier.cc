#include "StrictFloatingBuild.h"
#include "IntersectionVerifier.h"

#include "EventCoordinates.h"
#include "EventIncidence.h"
#include "EventInterning.h"
#include "EventNormalization.h"
#include "IntersectionAggregation.h"
#include "IntersectionDescriptors.h"
#include "Sha256.h"
#include "SourceEdgeArrangements.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error verifier_error(intersection_subcode subcode,
                                     const char *summary) {
  return intersection_error(
      subcode, bounded_boolean_error_category::internal_invariant_error,
      summary, intersection_checkpoint::independent_verification);
}

bool valid_limits(const intersection_verifier_limits &limits) noexcept {
  return limits.version == contract_versions::intersection_verifier &&
         limits.reserved8 == 0 && limits.reserved32 == 0 &&
         limits.maximum_work_units != 0 && limits.maximum_pair_checks != 0;
}

bool add_work(std::uint64_t amount, const intersection_verifier_limits &limits,
              std::uint64_t &work, bounded_boolean_error &error) {
  if (amount > limits.maximum_work_units - work) {
    error = intersection_error(
        intersection_subcode::resource_preflight,
        bounded_boolean_error_category::resource_limit,
        "Component 08 verifier work limit exceeded",
        intersection_checkpoint::independent_verification);
    return false;
  }
  work += amount;
  return true;
}

bool add_pair_checks(std::uint64_t member_count,
                     const intersection_verifier_limits &limits,
                     std::uint64_t &pair_checks, std::uint64_t &work,
                     bounded_boolean_error &error) {
  if (member_count < 2)
    return true;
  const auto first = member_count;
  const auto second = member_count - 1;
  const auto lhs = first % 2 == 0 ? first / 2 : first;
  const auto rhs = first % 2 == 0 ? second : second / 2;
  if (rhs != 0 && lhs > std::numeric_limits<std::uint64_t>::max() / rhs) {
    error = intersection_error(
        intersection_subcode::resource_preflight,
        bounded_boolean_error_category::resource_limit,
        "Component 08 verifier pair-count overflow",
        intersection_checkpoint::independent_verification);
    return false;
  }
  const auto amount = lhs * rhs;
  if (amount > limits.maximum_pair_checks - pair_checks) {
    error = intersection_error(
        intersection_subcode::resource_preflight,
        bounded_boolean_error_category::resource_limit,
        "Component 08 verifier pair-check limit exceeded",
        intersection_checkpoint::independent_verification);
    return false;
  }
  pair_checks += amount;
  return add_work(amount, limits, work, error);
}

bool valid_range(const intersection_range &range, std::size_t size) noexcept {
  return range.begin <= size && range.count <= size - range.begin;
}

template <class Id, class Record>
bool contiguous_ids(const std::vector<Record> &records) noexcept {
  for (std::size_t i = 0; i < records.size(); ++i)
    if (records[i].id != Id{static_cast<std::uint64_t>(i)})
      return false;
  return true;
}

template <class Record, class Less>
bool strictly_sorted(const std::vector<Record> &records, Less less) noexcept {
  for (std::size_t i = 1; i < records.size(); ++i)
    if (!less(records[i - 1], records[i]))
      return false;
  return true;
}

bool valid_operation(boolean_operation operation) noexcept {
  const auto raw = static_cast<std::uint8_t>(operation);
  return raw >= static_cast<std::uint8_t>(boolean_operation::set_union) &&
         raw <= static_cast<std::uint8_t>(boolean_operation::symmetric_difference);
}

bool same_evidence(const intersection_verification_evidence &a,
                   const intersection_verification_evidence &b) noexcept {
  return a.schema_version == b.schema_version &&
         a.verifier_version == b.verifier_version &&
         a.seed_regrouped == b.seed_regrouped &&
         a.incidence_reconstructed == b.incidence_reconstructed &&
         a.arrangements_reconstructed == b.arrangements_reconstructed &&
         a.descriptors_reconstructed == b.descriptors_reconstructed &&
         a.exhaustive_mode == b.exhaustive_mode &&
         a.reserved8 == b.reserved8 &&
         a.reconstructed_digest == b.reconstructed_digest &&
         a.work_units == b.work_units;
}

template <class Relations, class Artifact>
bool valid_header_and_predecessor(
    const Relations &relations, const Artifact &artifact,
    const intersection_verifier_limits &limits,
    bounded_boolean_error &error) {
  if (!valid_limits(limits)) {
    error = verifier_error(intersection_subcode::unsupported_version,
                           "Component 08 verifier limits are malformed");
    return false;
  }
  if (!artifact.owner().same_owner(relations.owner())) {
    error = verifier_error(intersection_subcode::wrong_owner,
                           "Component 08 verifier owner mismatch");
    return false;
  }
  if (relations.verification() !=
          relation_verification_disposition::independently_verified ||
      !relations.candidates() || !relations.candidates()->manifolds() ||
      !relations.candidates()->owner().same_owner(relations.owner())) {
    error = verifier_error(
        intersection_subcode::predecessor_not_verified,
        "Component 08 verifier rejected predecessor publication state");
    return false;
  }
  if (artifact.schema_version() !=
          contract_versions::intersection_artifact_schema ||
      artifact.provider_version() != contract_versions::intersection_provider ||
      artifact.semantic_policy_version() !=
          contract_versions::intersection_semantic_policy ||
      artifact.codec_version() != contract_versions::intersection_codec ||
      artifact.verifier_version() != contract_versions::intersection_verifier ||
      artifact.provider() !=
          intersection_provider_kind::canonical_lineage_event_arrangement_v1 ||
      !valid_operation(artifact.operation()) ||
      artifact.operation() != relations.operation() ||
      artifact.context_digest() != relations.context_digest() ||
      artifact.precision_digest() != relations.precision_digest() ||
      artifact.relation_digest() != relations.digest() ||
      artifact.source_semantic_digests()[0] !=
          relations.candidates()->primitive_table(operand_id::a)
              .source_semantic_digest ||
      artifact.source_semantic_digests()[1] !=
          relations.candidates()->primitive_table(operand_id::b)
              .source_semantic_digest ||
      artifact.exact_triangulation_digests()[0] !=
          relations.candidates()->primitive_table(operand_id::a)
              .exact_topology_digest ||
      artifact.exact_triangulation_digests()[1] !=
          relations.candidates()->primitive_table(operand_id::b)
              .exact_topology_digest) {
    error = verifier_error(
        intersection_subcode::predecessor_mismatch,
        "Component 08 verifier rejected header or predecessor digest");
    return false;
  }
  return true;
}

template <class Artifact>
event_interning_tables project_interning(const Artifact &artifact) {
  event_interning_tables out;
  out.events = artifact.events();
  out.occurrences = artifact.occurrences();
  out.seed_bindings = artifact.seed_bindings();
  out.event_binding_index = artifact.event_binding_index();
  out.occurrence_binding_index = artifact.occurrence_binding_index();
  out.seed_to_event = artifact.seed_to_event();
  out.seed_to_occurrence = artifact.seed_to_occurrence();
  return out;
}

template <class Artifact>
event_coordinate_tables project_coordinates(const Artifact &artifact) {
  event_coordinate_tables out;
  out.construction_witness_index = artifact.construction_witness_index();
  return out;
}

template <class Artifact>
event_incidence_tables project_incidence(const Artifact &artifact) {
  event_incidence_tables out;
  out.records = artifact.incidence();
  out.by_event = artifact.incidence_by_event();
  out.event_ranges = artifact.event_incidence_ranges();
  out.by_occurrence = artifact.incidence_by_occurrence();
  out.occurrence_ranges = artifact.occurrence_incidence_ranges();
  out.by_seed = artifact.incidence_by_seed();
  out.seed_ranges = artifact.seed_incidence_ranges();
  out.seed_candidate_index = artifact.incidence_by_seed_candidate();
  out.seed_candidate_ranges = artifact.seed_candidate_incidence_ranges();
  out.by_relation = artifact.incidence_by_relation();
  out.relation_ranges = artifact.relation_incidence_ranges();
  out.by_candidate = artifact.incidence_by_candidate();
  out.candidate_ranges = artifact.candidate_incidence_ranges();
  out.by_source_feature = artifact.incidence_by_source_feature();
  out.source_feature_ranges = artifact.source_feature_incidence_ranges();
  out.by_halfedge = artifact.incidence_by_halfedge();
  out.halfedge_ranges = artifact.halfedge_incidence_ranges();
  return out;
}

template <class Artifact>
std::uint64_t certificate_split(const Artifact &artifact,
                                bounded_boolean_error &error) {
  const auto invalid = intersection_invalid_ordinal;
  std::uint64_t source_max = invalid;
  std::uint64_t carrier_min = invalid;
  std::vector<bool> used(artifact.ordering_certificates().size(), false);
  auto use_source = [&](ordering_certificate_id id) {
    if (id.ordinal() == invalid)
      return true;
    if (id.ordinal() >= used.size())
      return false;
    used[id.ordinal()] = true;
    source_max = source_max == invalid ? id.ordinal()
                                       : std::max(source_max, id.ordinal());
    return true;
  };
  auto use_carrier = [&](ordering_certificate_id id) {
    if (id.ordinal() == invalid)
      return true;
    if (id.ordinal() >= used.size())
      return false;
    used[id.ordinal()] = true;
    carrier_min = carrier_min == invalid ? id.ordinal()
                                         : std::min(carrier_min, id.ordinal());
    return true;
  };
  for (const auto &record : artifact.source_edge_memberships())
    if (!use_source(record.ordering_certificate))
      goto bad;
  for (const auto &record : artifact.source_edge_clusters())
    if (!use_source(record.ordering_certificate))
      goto bad;
  for (const auto &record : artifact.carrier_memberships())
    if (!use_carrier(record.ordering_certificate))
      goto bad;
  for (const auto &record : artifact.carrier_clusters())
    if (!use_carrier(record.ordering_certificate))
      goto bad;

  {
    std::uint64_t split = 0;
    if (source_max != invalid)
      split = source_max + 1;
    else if (carrier_min != invalid)
      split = carrier_min;
    if (carrier_min != invalid && carrier_min < split)
      goto bad;
    if (!artifact.ordering_certificates().empty()) {
      for (std::size_t i = 0; i < used.size(); ++i)
        if (!used[i])
          goto bad;
    }
    return split;
  }

bad:
  error = verifier_error(
      intersection_subcode::bounded_order_contradiction,
      "Component 08 verifier rejected ordering certificate ownership");
  return invalid;
}

template <class Artifact>
source_edge_arrangement_tables project_source_edges(
    const Artifact &artifact, std::uint64_t split) {
  source_edge_arrangement_tables out;
  out.memberships = artifact.source_edge_memberships();
  out.membership_sequence_index =
      artifact.source_edge_membership_sequence_index();
  out.sequences = artifact.source_edge_sequences();
  out.clusters = artifact.source_edge_clusters();
  out.cluster_occurrence_index = artifact.source_edge_cluster_occurrence_index();
  out.cluster_membership_index = artifact.source_edge_cluster_membership_index();
  out.sequence_cluster_index = artifact.source_edge_sequence_cluster_index();
  out.intervals = artifact.source_edge_intervals();
  out.sequence_interval_index = artifact.source_edge_sequence_interval_index();
  out.ordering_certificates.assign(artifact.ordering_certificates().begin(),
                                   artifact.ordering_certificates().begin() +
                                       static_cast<std::ptrdiff_t>(split));
  return out;
}

template <class Artifact>
transverse_carrier_arrangement_tables project_transverse(
    const Artifact &artifact, std::uint64_t split) {
  transverse_carrier_arrangement_tables out;
  out.carriers = artifact.transverse_carriers();
  out.carrier_relation_provenance = artifact.carrier_relation_provenance();
  out.carrier_candidate_provenance = artifact.carrier_candidate_provenance();
  out.memberships = artifact.carrier_memberships();
  out.carrier_membership_index = artifact.carrier_membership_index();
  out.clusters = artifact.carrier_clusters();
  out.cluster_occurrence_index = artifact.carrier_cluster_occurrence_index();
  out.cluster_membership_index = artifact.carrier_cluster_membership_index();
  out.carrier_cluster_index = artifact.transverse_carrier_cluster_index();
  out.spans = artifact.carrier_active_spans();
  out.carrier_span_index = artifact.transverse_carrier_span_index();
  out.span_relation_provenance = artifact.carrier_span_relation_provenance();
  out.span_region_incidence = artifact.carrier_span_region_incidence();
  out.ordering_certificates.assign(
      artifact.ordering_certificates().begin() +
          static_cast<std::ptrdiff_t>(split),
      artifact.ordering_certificates().end());
  for (std::size_t i = 0; i < out.ordering_certificates.size(); ++i)
    out.ordering_certificates[i].id = ordering_certificate_id{i};
  for (auto &record : out.memberships)
    if (record.ordering_certificate.ordinal() != intersection_invalid_ordinal)
      record.ordering_certificate = ordering_certificate_id{
          record.ordering_certificate.ordinal() - split};
  for (auto &record : out.clusters)
    if (record.ordering_certificate.ordinal() != intersection_invalid_ordinal)
      record.ordering_certificate = ordering_certificate_id{
          record.ordering_certificate.ordinal() - split};
  return out;
}

template <class Artifact>
coplanar_carrier_arrangement_tables project_coplanar(const Artifact &artifact) {
  coplanar_carrier_arrangement_tables out;
  out.supports = artifact.coplanar_supports();
  out.support_relation_provenance =
      artifact.coplanar_support_relation_provenance();
  out.support_candidate_provenance =
      artifact.coplanar_support_candidate_provenance();
  out.support_original_boundary_edge_index =
      artifact.coplanar_support_original_boundary_edge_index();
  out.support_boundary_event_index =
      artifact.coplanar_support_boundary_event_index();
  out.support_boundary_carrier_index =
      artifact.coplanar_support_boundary_carrier_index();
  out.support_overlap_index = artifact.coplanar_support_overlap_index();
  out.support_region_index = artifact.coplanar_support_region_index();
  out.carriers = artifact.overlap_carriers();
  out.carrier_relation_provenance =
      artifact.overlap_carrier_relation_provenance();
  out.carrier_candidate_provenance =
      artifact.overlap_carrier_candidate_provenance();
  out.carrier_source_provenance = artifact.overlap_carrier_source_provenance();
  out.overlaps = artifact.coplanar_overlaps();
  out.overlap_boundary_event_index =
      artifact.coplanar_overlap_boundary_event_index();
  out.overlap_boundary_carrier_index =
      artifact.coplanar_overlap_boundary_carrier_index();
  out.overlap_relation_provenance =
      artifact.coplanar_overlap_relation_provenance();
  out.regions = artifact.coplanar_region_incidence();
  out.region_boundary_event_index =
      artifact.coplanar_region_boundary_event_index();
  out.region_boundary_carrier_index =
      artifact.coplanar_region_boundary_carrier_index();
  out.region_coverage_witness_index =
      artifact.coplanar_region_coverage_witness_index();
  return out;
}

template <class Artifact>
intersection_aggregate_tables project_aggregates(const Artifact &artifact) {
  intersection_aggregate_tables out;
  out.crossing = artifact.crossing_aggregates();
  out.crossing_members = artifact.crossing_aggregate_members();
  out.facet_subtotals = artifact.crossing_facet_subtotals();
  out.facet_subtotal_members = artifact.crossing_facet_subtotal_members();
  out.shell_subtotals = artifact.crossing_shell_subtotals();
  out.shell_subtotal_members = artifact.crossing_shell_subtotal_members();
  out.contact = artifact.contact_aggregates();
  out.contact_members = artifact.contact_aggregate_members();
  return out;
}

bool topology_locus(intersection_descriptor_locus locus) noexcept {
  return locus == intersection_descriptor_locus::source_vertex_sector ||
         locus ==
             intersection_descriptor_locus::source_facet_original_edge_adjacency ||
         locus == intersection_descriptor_locus::
                      transparent_internal_diagonal_adjacency;
}

template <class Artifact>
intersection_descriptor_tables project_descriptors(const Artifact &artifact,
                                                    bool include_topology) {
  intersection_descriptor_tables out;
  for (const auto &record : artifact.descriptors()) {
    if (!include_topology && topology_locus(record.key.locus))
      continue;
    auto copy = record;
    copy.id = intersection_descriptor_id{out.records.size()};
    copy.provenance.begin = out.provenance.size();
    if (valid_range(record.provenance, artifact.descriptor_provenance().size())) {
      out.provenance.insert(
          out.provenance.end(),
          artifact.descriptor_provenance().begin() +
              static_cast<std::ptrdiff_t>(record.provenance.begin),
          artifact.descriptor_provenance().begin() +
              static_cast<std::ptrdiff_t>(record.provenance.begin +
                                          record.provenance.count));
    } else {
      copy.provenance = {std::numeric_limits<std::uint64_t>::max(), 1};
    }
    out.records.push_back(std::move(copy));
  }
  return out;
}

template <class Artifact>
std::vector<source_edge_domain_record> source_domains(const Artifact &artifact,
                                                       bool &ok) {
  std::vector<source_edge_domain_record> out;
  out.reserve(artifact.source_edge_sequences().size());
  for (const auto &sequence : artifact.source_edge_sequences()) {
    source_edge_domain_record domain;
    domain.source_edge = sequence.source_edge;
    domain.start_vertex = sequence.start.source_vertex;
    domain.end_vertex = sequence.end.source_vertex;
    if (sequence.start.source_edge != sequence.source_edge ||
        sequence.end.source_edge != sequence.source_edge ||
        domain.source_edge.kind != relation_feature_kind::source_edge ||
        domain.start_vertex.kind != relation_feature_kind::source_vertex ||
        domain.end_vertex.kind != relation_feature_kind::source_vertex ||
        domain.source_edge.operand != domain.start_vertex.operand ||
        domain.source_edge.operand != domain.end_vertex.operand) {
      ok = false;
      return {};
    }
    out.push_back(domain);
  }
  if (!strictly_sorted(out, [](const auto &a, const auto &b) {
        return a.source_edge < b.source_edge;
      }))
    ok = out.size() < 2;
  return out;
}


template <class T, class I>
std::vector<source_edge_domain_record> predecessor_source_domains(
    const signed_feature_relations<T, I> &relations, bool &ok) {
  std::vector<source_edge_domain_record> domains;
  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto &table = relations.candidates()->primitive_table(operand);
    for (const auto &edge : table.edges) {
      if (edge.edge_class != canonical_edge_class::source_edge ||
          !edge.source_feature_owner)
        continue;
      source_edge_domain_record domain;
      domain.source_edge.operand = operand;
      domain.source_edge.kind = relation_feature_kind::source_edge;
      domain.source_edge.primary = edge.semantic_key.primary;
      domain.source_edge.secondary = edge.semantic_key.secondary;
      domain.start_vertex.operand = operand;
      domain.start_vertex.kind = relation_feature_kind::source_vertex;
      domain.start_vertex.primary = edge.semantic_key.primary;
      domain.end_vertex.operand = operand;
      domain.end_vertex.kind = relation_feature_kind::source_vertex;
      domain.end_vertex.primary = edge.semantic_key.secondary;
      if (!valid_relation_feature_key(domain.source_edge, false) ||
          !valid_relation_feature_key(domain.start_vertex, false) ||
          !valid_relation_feature_key(domain.end_vertex, false)) {
        ok = false;
        return {};
      }
      domains.push_back(domain);
    }
  }
  std::sort(domains.begin(), domains.end(), [](const auto &a, const auto &b) {
    return a.source_edge < b.source_edge;
  });
  for (std::size_t i = 1; i < domains.size(); ++i)
    if (!(domains[i - 1].source_edge < domains[i].source_edge)) {
      ok = false;
      return {};
    }
  return domains;
}

bool same_source_domains(const std::vector<source_edge_domain_record> &a,
                         const std::vector<source_edge_domain_record> &b) {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].source_edge != b[i].source_edge ||
        a[i].start_vertex != b[i].start_vertex ||
        a[i].end_vertex != b[i].end_vertex)
      return false;
  return true;
}
template <class Relations, class Artifact>
bounded_boolean_digest semantic_reconstruction_digest(
    const Relations &relations, const Artifact &artifact) {
  canonical_writer writer;
  writer.u64(0x3156464954434553ULL); // "SECTIFV1", little-endian tag.
  writer.u16(contract_versions::intersection_verifier);
  for (const auto byte : relations.digest().bytes)
    writer.u8(byte);
  for (std::size_t i = 1; i <= 6; ++i)
    for (const auto byte : artifact.section_digests()[i].bytes)
      writer.u8(byte);
  writer.u64(artifact.events().size());
  writer.u64(artifact.occurrences().size());
  writer.u64(artifact.incidence().size());
  writer.u64(artifact.source_edge_intervals().size());
  writer.u64(artifact.carrier_active_spans().size());
  writer.u64(artifact.coplanar_overlaps().size());
  writer.u64(artifact.descriptors().size());
  return sha256::digest(writer.bytes());
}


template <class T, class I>
bool audit_event_partition(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const intersection_verifier_limits &limits, std::uint64_t &pair_checks,
    std::uint64_t &work, bounded_boolean_error &error) {
  std::vector<normalized_event_seed_proposal> proposals;
  bounded_boolean_error local;
  if (!normalize_event_seed_records(relations.event_seeds(),
                                    relations.constructions(), proposals,
                                    local) ||
      !add_work(proposals.size(), limits, work, error)) {
    if (error.summary == nullptr || error.summary[0] == '\0')
      error = verifier_error(
          intersection_subcode::malformed_seed,
          "Component 08 verifier could not normalize predecessor seeds");
    return false;
  }

  if (!verify_event_interning_tables(proposals, interning, local) ||
      !verify_event_coordinates(proposals, relations.constructions(),
                                relations.construction_ledger(), interning,
                                coordinates, local) ||
      !verify_event_incidence_records(
          relations.event_seeds(), relations.event_seed_incidence(),
          relations.event_seed_candidate_incidence(),
          relations.relations().size(),
          relations.candidate_dispositions().size(), interning, incidence,
          local)) {
    error = verifier_error(
        intersection_subcode::verifier_rejection,
        "Component 08 independent event/incidence reconstruction failed");
    return false;
  }

  if (!contiguous_ids<event_id>(artifact.events()) ||
      !contiguous_ids<event_occurrence_id>(artifact.occurrences()) ||
      !contiguous_ids<event_seed_binding_id>(artifact.seed_bindings()) ||
      !contiguous_ids<event_incidence_id>(artifact.incidence()) ||
      artifact.seed_bindings().size() != relations.event_seeds().size() ||
      artifact.seed_to_event().size() != relations.event_seeds().size() ||
      artifact.seed_to_occurrence().size() != relations.event_seeds().size() ||
      !strictly_sorted(artifact.events(), [](const auto &a, const auto &b) {
        return a.key < b.key;
      }) ||
      !strictly_sorted(artifact.occurrences(),
                       [](const auto &a, const auto &b) {
                         return a.key < b.key;
                       }) ||
      !strictly_sorted(artifact.incidence(), [](const auto &a, const auto &b) {
        return a.key < b.key;
      })) {
    error = verifier_error(
        intersection_subcode::occurrence_merge_or_split,
        "Component 08 verifier rejected canonical event partition");
    return false;
  }

  for (const auto &event : artifact.events()) {
    if (!add_pair_checks(event.occurrences.count, limits, pair_checks, work,
                         error))
      return false;
    if (!valid_intersection_event_key(event.key) ||
        event.schema_version != contract_versions::intersection_event_schema ||
        event.reserved16 != 0 || event.reserved32 != 0 ||
        !valid_range(event.construction_witnesses,
                     artifact.construction_witness_index().size()) ||
        !valid_range(event.occurrences, artifact.occurrences().size()) ||
        !valid_range(event.seed_bindings,
                     artifact.event_binding_index().size()) ||
        !valid_range(event.incidence, artifact.incidence_by_event().size()) ||
        !valid_range(event.crossing_aggregates,
                     artifact.crossing_aggregates().size()) ||
        !valid_range(event.contact_aggregates,
                     artifact.contact_aggregates().size())) {
      error = verifier_error(
          intersection_subcode::malformed_event_key,
          "Component 08 verifier rejected event record or range");
      return false;
    }
    if (event.point.schema_version != contract_versions::intersection_event_schema ||
        event.point.reserved16 != 0 || event.point.reserved32 != 0) {
      error = verifier_error(intersection_subcode::missing_authoritative_point,
                             "Component 08 verifier rejected event point");
      return false;
    }
    if (event.point.kind == bounded_point_reference_kind::source_point) {
      if (!valid_relation_feature_key(event.point.source_vertex, false) ||
          event.point.source_vertex.kind != relation_feature_kind::source_vertex ||
          event.point.construction.ordinal() >= relations.constructions().size() ||
          event.point.precision_ledger.ordinal() >=
              relations.construction_ledger().size()) {
        error = verifier_error(
            intersection_subcode::source_vertex_point_mismatch,
            "Component 08 verifier rejected reused source point");
        return false;
      }
    } else if (event.point.kind ==
               bounded_point_reference_kind::constructed_point) {
      if (event.point.construction.ordinal() >= relations.constructions().size() ||
          event.point.precision_ledger.ordinal() >=
              relations.construction_ledger().size()) {
        error = verifier_error(
            intersection_subcode::missing_authoritative_point,
            "Component 08 verifier rejected constructed point lineage");
        return false;
      }
    } else {
      error = verifier_error(intersection_subcode::missing_authoritative_point,
                             "Component 08 verifier rejected point kind");
      return false;
    }
  }

  for (const auto &occurrence : artifact.occurrences()) {
    if (occurrence.event.ordinal() >= artifact.events().size() ||
        occurrence.key.event !=
            artifact.events()[occurrence.event.ordinal()].key ||
        !valid_intersection_occurrence_key(occurrence.key) ||
        occurrence.schema_version !=
            contract_versions::intersection_occurrence_schema ||
        occurrence.reserved16 != 0 ||
        !valid_range(occurrence.seed_bindings,
                     artifact.occurrence_binding_index().size()) ||
        !valid_range(occurrence.incidence,
                     artifact.incidence_by_occurrence().size()) ||
        !valid_range(occurrence.source_edge_memberships,
                     artifact.source_edge_memberships().size()) ||
        !valid_range(occurrence.carrier_memberships,
                     artifact.carrier_memberships().size()) ||
        !valid_range(occurrence.cluster_memberships,
                     artifact.source_edge_clusters().size() +
                         artifact.carrier_clusters().size()) ||
        !valid_range(occurrence.aggregate_contributions,
                     artifact.incidence().size()) ||
        !valid_range(occurrence.descriptors, artifact.descriptors().size())) {
      error = verifier_error(
          intersection_subcode::malformed_occurrence_key,
          "Component 08 verifier rejected occurrence record or range");
      return false;
    }
    if (occurrence.topology_separate &&
        !occurrence.may_share_output_coordinate) {
      // Separation is topological, not a forced coordinate perturbation.
      error = verifier_error(
          intersection_subcode::occurrence_semantic_conflict,
          "Component 08 occurrence separation changed coordinate semantics");
      return false;
    }
  }

  for (std::size_t i = 0; i < artifact.seed_bindings().size(); ++i) {
    const auto &binding = artifact.seed_bindings()[i];
    if (binding.seed.ordinal() >= relations.event_seeds().size() ||
        binding.canonical_seed_ordinal != binding.seed.ordinal() ||
        !(binding.seed_key == relations.event_seeds()[binding.seed.ordinal()].key) ||
        binding.event.ordinal() >= artifact.events().size() ||
        binding.occurrence.ordinal() >= artifact.occurrences().size() ||
        binding.relation.ordinal() >= relations.relations().size() ||
        binding.construction.ordinal() >= relations.constructions().size() ||
        binding.schema_version !=
            contract_versions::intersection_seed_binding_schema ||
        binding.reserved32 != 0 || !binding.compatibility_verified ||
        !valid_range(binding.incidence, artifact.incidence_by_seed().size()) ||
        !valid_range(binding.candidate_incidence,
                     artifact.incidence_by_seed_candidate().size()) ||
        artifact.seed_to_event()[binding.seed.ordinal()] != binding.event ||
        artifact.seed_to_occurrence()[binding.seed.ordinal()] !=
            binding.occurrence) {
      error = verifier_error(
          intersection_subcode::seed_mapping_incomplete,
          "Component 08 verifier rejected seed binding");
      return false;
    }
  }

  return add_work(artifact.events().size() + artifact.occurrences().size() +
                      artifact.seed_bindings().size() + artifact.incidence().size(),
                  limits, work, error);
}

template <class T, class I>
bool audit_source_edges(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const intersection_verifier_limits &limits, std::uint64_t &pair_checks,
    std::uint64_t &work, bounded_boolean_error &error) {
  std::vector<source_edge_membership_proposal> proposals;
  bounded_boolean_error local;
  if (!collect_source_edge_membership_proposals(
          relations.event_seeds(), relations.interval_evidence(), interning,
          incidence, proposals, local)) {
    error = verifier_error(
        intersection_subcode::membership_incomplete,
        "Component 08 verifier could not reconstruct source-edge memberships");
    return false;
  }
  bool domains_ok = true;
  const auto domains = source_domains(artifact, domains_ok);
  const auto predecessor_domains = predecessor_source_domains(relations, domains_ok);
  if (!domains_ok || !same_source_domains(domains, predecessor_domains) ||
      !verify_source_edge_arrangements<T>(domains, proposals, source_edges,
                                          local)) {
    error = verifier_error(
        intersection_subcode::source_edge_partition_invalid,
        "Component 08 source-edge arrangement reconstruction failed");
    return false;
  }

  if (!contiguous_ids<source_edge_membership_id>(
          artifact.source_edge_memberships()) ||
      !contiguous_ids<source_edge_sequence_id>(artifact.source_edge_sequences()) ||
      !contiguous_ids<source_edge_cluster_id>(artifact.source_edge_clusters()) ||
      !contiguous_ids<source_edge_interval_id>(artifact.source_edge_intervals()) ||
      !strictly_sorted(artifact.source_edge_memberships(),
                       [](const auto &a, const auto &b) {
                         return a.key < b.key;
                       }) ||
      !strictly_sorted(artifact.source_edge_sequences(),
                       [](const auto &a, const auto &b) {
                         return a.source_edge < b.source_edge;
                       })) {
    error = verifier_error(
        intersection_subcode::source_edge_sequence_invalid,
        "Component 08 verifier rejected source-edge canonical order");
    return false;
  }

  for (const auto &membership : artifact.source_edge_memberships()) {
    if (membership.occurrence.ordinal() >= artifact.occurrences().size() ||
        membership.event.ordinal() >= artifact.events().size() ||
        membership.parameter.ordinal() >= relations.interval_evidence().size() ||
        !(membership.key.occurrence ==
            artifact.occurrences()[membership.occurrence.ordinal()].key) ||
        membership.key.source_edge.kind != relation_feature_kind::source_edge ||
        (membership.key.occurrence.event.authoritative_source_feature.kind ==
             relation_feature_kind::source_edge &&
         membership.key.source_edge !=
             membership.key.occurrence.event.authoritative_source_feature) ||
        !valid_range(membership.contributions, artifact.incidence().size()) ||
        membership.schema_version !=
            contract_versions::intersection_source_edge_membership_schema ||
        membership.reserved16 != 0) {
      error = verifier_error(
          intersection_subcode::membership_incomplete,
          "Component 08 verifier rejected source-edge membership");
      return false;
    }
    if (membership.internal_diagonal_discovery &&
        !membership.bookkeeping_only) {
      error = verifier_error(
          intersection_subcode::internal_diagonal_public_ownership,
          "Component 08 internal diagonal became public membership");
      return false;
    }
  }

  for (const auto &sequence : artifact.source_edge_sequences()) {
    if (!valid_range(sequence.clusters,
                     artifact.source_edge_sequence_cluster_index().size()) ||
        !valid_range(sequence.memberships,
                     artifact.source_edge_membership_sequence_index().size()) ||
        !valid_range(sequence.intervals,
                     artifact.source_edge_sequence_interval_index().size()) ||
        !valid_range(sequence.aggregates,
                     artifact.crossing_aggregates().size()) ||
        !valid_range(sequence.descriptors, artifact.descriptors().size()) ||
        sequence.schema_version !=
            contract_versions::intersection_source_edge_sequence_schema ||
        sequence.reserved8 != 0 || sequence.reserved32 != 0 ||
        !sequence.canonical_forward) {
      error = verifier_error(
          intersection_subcode::source_edge_sequence_invalid,
          "Component 08 verifier rejected source-edge sequence");
      return false;
    }
  }

  for (const auto &cluster : artifact.source_edge_clusters()) {
    if (!add_pair_checks(cluster.member_occurrences.count, limits, pair_checks,
                         work, error))
      return false;
    if (cluster.sequence.ordinal() >= artifact.source_edge_sequences().size() ||
        cluster.key.source_edge !=
            artifact.source_edge_sequences()[cluster.sequence.ordinal()]
                .source_edge ||
        !valid_range(cluster.member_occurrences,
                     artifact.source_edge_cluster_occurrence_index().size()) ||
        !valid_range(cluster.membership_ids,
                     artifact.source_edge_cluster_membership_index().size()) ||
        cluster.member_occurrences.count != cluster.key.members.size() ||
        cluster.schema_version != contract_versions::intersection_cluster_schema ||
        cluster.key.reserved != 0) {
      error = verifier_error(intersection_subcode::cluster_invalid,
                             "Component 08 verifier rejected source cluster");
      return false;
    }
  }

  for (const auto &interval : artifact.source_edge_intervals()) {
    if (interval.sequence.ordinal() >= artifact.source_edge_sequences().size() ||
        interval.key.source_edge !=
            artifact.source_edge_sequences()[interval.sequence.ordinal()]
                .source_edge ||
        interval.length_disposition == intersection_interval_length::uncertain ||
        interval.length_disposition == intersection_interval_length::overlap ||
        !valid_range(interval.provenance, artifact.incidence().size()) ||
        !valid_range(interval.descriptors, artifact.descriptors().size()) ||
        interval.schema_version !=
            contract_versions::intersection_source_edge_interval_schema ||
        interval.reserved16 != 0) {
      error = verifier_error(
          intersection_subcode::source_edge_partition_invalid,
          "Component 08 verifier rejected source-edge interval");
      return false;
    }
  }

  return add_work(proposals.size() + artifact.source_edge_memberships().size() +
                      artifact.source_edge_clusters().size() +
                      artifact.source_edge_intervals().size(),
                  limits, work, error);
}

template <class T, class I>
bool audit_transverse(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const transverse_carrier_arrangement_tables &transverse,
    const intersection_verifier_limits &limits, std::uint64_t &pair_checks,
    std::uint64_t &work, bounded_boolean_error &error) {
  if (!contiguous_ids<transverse_carrier_id>(artifact.transverse_carriers()) ||
      !contiguous_ids<carrier_membership_id>(artifact.carrier_memberships()) ||
      !contiguous_ids<carrier_cluster_id>(artifact.carrier_clusters()) ||
      !contiguous_ids<carrier_active_span_id>(artifact.carrier_active_spans()) ||
      !strictly_sorted(artifact.transverse_carriers(),
                       [](const auto &a, const auto &b) {
                         return a.key < b.key;
                       })) {
    error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                           "Component 08 verifier rejected transverse IDs/order");
    return false;
  }

  const auto relation_count = relations.relations().size();
  const auto candidate_count = relations.candidate_dispositions().size();
  const auto region_count = relations.source_facet_regions().size();
  std::vector<std::uint64_t> membership_owner(artifact.carrier_memberships().size(),
                                              intersection_invalid_ordinal);
  std::vector<std::uint64_t> cluster_owner(artifact.carrier_clusters().size(),
                                           intersection_invalid_ordinal);
  std::vector<std::uint64_t> span_owner(artifact.carrier_active_spans().size(),
                                        intersection_invalid_ordinal);

  for (const auto &carrier : artifact.transverse_carriers()) {
    if (!valid_transverse_carrier_key(carrier.key) ||
        carrier.construction.ordinal() >= relations.constructions().size() ||
        carrier.key.construction != carrier.construction ||
        carrier.schema_version != contract_versions::intersection_carrier_schema ||
        carrier.reserved16 != 0 || carrier.reserved32 != 0 ||
        !valid_range(carrier.relation_provenance,
                     artifact.carrier_relation_provenance().size()) ||
        !valid_range(carrier.candidate_provenance,
                     artifact.carrier_candidate_provenance().size()) ||
        !valid_range(carrier.memberships,
                     artifact.carrier_membership_index().size()) ||
        !valid_range(carrier.clusters,
                     artifact.transverse_carrier_cluster_index().size()) ||
        !valid_range(carrier.active_spans,
                     artifact.transverse_carrier_span_index().size()) ||
        !valid_range(carrier.region_incidence,
                     artifact.carrier_span_region_incidence().size()) ||
        !valid_range(carrier.aggregates,
                     artifact.crossing_aggregates().size()) ||
        !valid_range(carrier.descriptors, artifact.descriptors().size())) {
      error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                             "Component 08 verifier rejected transverse carrier");
      return false;
    }
    for (std::uint64_t j = 0; j < carrier.relation_provenance.count; ++j) {
      const auto id = artifact.carrier_relation_provenance()[
          carrier.relation_provenance.begin + j];
      if (id.ordinal() >= relation_count) {
        error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                               "Component 08 carrier relation provenance is stale");
        return false;
      }
      const auto status = relations.relations()[id.ordinal()].status;
      if (status == feature_relation_status::coincidence_same_orientation ||
          status == feature_relation_status::coincidence_opposite_orientation ||
          status == feature_relation_status::overlap ||
          status == feature_relation_status::containment) {
        error = verifier_error(intersection_subcode::coplanar_routed_transverse,
                               "Component 08 routed coplanar lineage through transverse carrier");
        return false;
      }
    }
    for (std::uint64_t j = 0; j < carrier.candidate_provenance.count; ++j)
      if (artifact.carrier_candidate_provenance()[
              carrier.candidate_provenance.begin + j]
              .ordinal() >= candidate_count) {
        error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                               "Component 08 carrier candidate provenance is stale");
        return false;
      }
    for (std::uint64_t j = 0; j < carrier.memberships.count; ++j) {
      const auto id = artifact.carrier_membership_index()[carrier.memberships.begin + j];
      if (id.ordinal() >= membership_owner.size() ||
          membership_owner[id.ordinal()] != intersection_invalid_ordinal ||
          artifact.carrier_memberships()[id.ordinal()].carrier != carrier.id) {
        error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                               "Component 08 carrier membership partition is invalid");
        return false;
      }
      membership_owner[id.ordinal()] = carrier.id.ordinal();
    }
    for (std::uint64_t j = 0; j < carrier.clusters.count; ++j) {
      const auto id = artifact.transverse_carrier_cluster_index()[carrier.clusters.begin + j];
      if (id.ordinal() >= cluster_owner.size() ||
          cluster_owner[id.ordinal()] != intersection_invalid_ordinal ||
          artifact.carrier_clusters()[id.ordinal()].carrier != carrier.id) {
        error = verifier_error(intersection_subcode::cluster_invalid,
                               "Component 08 carrier cluster partition is invalid");
        return false;
      }
      cluster_owner[id.ordinal()] = carrier.id.ordinal();
    }
    for (std::uint64_t j = 0; j < carrier.active_spans.count; ++j) {
      const auto id = artifact.transverse_carrier_span_index()[carrier.active_spans.begin + j];
      if (id.ordinal() >= span_owner.size() ||
          span_owner[id.ordinal()] != intersection_invalid_ordinal ||
          artifact.carrier_active_spans()[id.ordinal()].carrier != carrier.id) {
        error = verifier_error(intersection_subcode::transverse_span_invalid,
                               "Component 08 carrier span partition is invalid");
        return false;
      }
      span_owner[id.ordinal()] = carrier.id.ordinal();
    }
  }

  if (std::find(membership_owner.begin(), membership_owner.end(),
                intersection_invalid_ordinal) != membership_owner.end() ||
      std::find(cluster_owner.begin(), cluster_owner.end(),
                intersection_invalid_ordinal) != cluster_owner.end() ||
      std::find(span_owner.begin(), span_owner.end(),
                intersection_invalid_ordinal) != span_owner.end()) {
    error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                           "Component 08 transverse partition omitted a record");
    return false;
  }

  for (const auto &membership : artifact.carrier_memberships()) {
    if (membership.carrier.ordinal() >= artifact.transverse_carriers().size() ||
        membership.occurrence.ordinal() >= artifact.occurrences().size() ||
        membership.event.ordinal() >= artifact.events().size() ||
        membership.parameter.ordinal() >= relations.interval_evidence().size() ||
        membership.schema_version !=
            contract_versions::intersection_carrier_membership_schema ||
        membership.reserved16 != 0 || membership.reserved32 != 0) {
      error = verifier_error(intersection_subcode::transverse_carrier_invalid,
                             "Component 08 verifier rejected carrier membership");
      return false;
    }
  }

  for (const auto &cluster : artifact.carrier_clusters()) {
    if (!add_pair_checks(cluster.occurrence_members.count, limits, pair_checks,
                         work, error))
      return false;
    if (cluster.carrier.ordinal() >= artifact.transverse_carriers().size() ||
        !valid_range(cluster.occurrence_members,
                     artifact.carrier_cluster_occurrence_index().size()) ||
        !valid_range(cluster.membership_members,
                     artifact.carrier_cluster_membership_index().size()) ||
        cluster.occurrence_members.count != cluster.membership_members.count ||
        cluster.schema_version != contract_versions::intersection_cluster_schema) {
      error = verifier_error(intersection_subcode::cluster_invalid,
                             "Component 08 verifier rejected carrier cluster");
      return false;
    }
    for (std::uint64_t j = 0; j < cluster.membership_members.count; ++j) {
      const auto mid = artifact.carrier_cluster_membership_index()[
          cluster.membership_members.begin + j];
      const auto oid = artifact.carrier_cluster_occurrence_index()[
          cluster.occurrence_members.begin + j];
      if (mid.ordinal() >= artifact.carrier_memberships().size() ||
          oid.ordinal() >= artifact.occurrences().size() ||
          artifact.carrier_memberships()[mid.ordinal()].carrier !=
              cluster.carrier ||
          artifact.carrier_memberships()[mid.ordinal()].occurrence != oid) {
        error = verifier_error(intersection_subcode::cluster_invalid,
                               "Component 08 carrier cluster member mismatch");
        return false;
      }
    }
  }

  for (const auto &span : artifact.carrier_active_spans()) {
    if (span.carrier.ordinal() >= artifact.transverse_carriers().size() ||
        span.left.ordinal() >= artifact.carrier_clusters().size() ||
        span.right.ordinal() >= artifact.carrier_clusters().size() ||
        span.left == span.right ||
        artifact.carrier_clusters()[span.left.ordinal()].carrier != span.carrier ||
        artifact.carrier_clusters()[span.right.ordinal()].carrier != span.carrier ||
        span.activation == intersection_span_activation::unresolved ||
        span.activation == intersection_span_activation::invalid ||
        !valid_range(span.region_incidence,
                     artifact.carrier_span_region_incidence().size()) ||
        !valid_range(span.provenance,
                     artifact.carrier_span_relation_provenance().size()) ||
        span.schema_version != contract_versions::intersection_carrier_schema) {
      error = verifier_error(intersection_subcode::transverse_span_invalid,
                             "Component 08 verifier rejected carrier span");
      return false;
    }
    for (std::uint64_t j = 0; j < span.provenance.count; ++j)
      if (artifact.carrier_span_relation_provenance()[span.provenance.begin + j]
              .ordinal() >= relation_count) {
        error = verifier_error(intersection_subcode::transverse_span_invalid,
                               "Component 08 span relation provenance is stale");
        return false;
      }
    for (std::uint64_t j = 0; j < span.region_incidence.count; ++j)
      if (artifact.carrier_span_region_incidence()[span.region_incidence.begin + j]
              .ordinal() >= region_count) {
        error = verifier_error(intersection_subcode::transverse_span_invalid,
                               "Component 08 span region evidence is stale");
        return false;
      }
    if (span.activation == intersection_span_activation::inactive &&
        (span.classification_cut || span.contact_delimiter ||
         span.output_edge_allowed)) {
      error = verifier_error(intersection_subcode::unsupported_gap_connected,
                             "Component 08 connected an inactive carrier gap");
      return false;
    }
  }

  (void)transverse;
  return add_work(artifact.transverse_carriers().size() +
                      artifact.carrier_memberships().size() +
                      artifact.carrier_clusters().size() +
                      artifact.carrier_active_spans().size(),
                  limits, work, error);
}

template <class T, class I>
bool audit_coplanar(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_verifier_limits &limits, std::uint64_t &work,
    bounded_boolean_error &error) {
  if (!contiguous_ids<coplanar_support_id>(artifact.coplanar_supports()) ||
      !contiguous_ids<collinear_overlap_carrier_id>(artifact.overlap_carriers()) ||
      !contiguous_ids<coplanar_overlap_record_id>(artifact.coplanar_overlaps()) ||
      !contiguous_ids<coplanar_region_incidence_id>(
          artifact.coplanar_region_incidence()) ||
      !strictly_sorted(artifact.coplanar_supports(),
                       [](const auto &a, const auto &b) {
                         return a.key < b.key;
                       }) ||
      !strictly_sorted(artifact.overlap_carriers(),
                       [](const auto &a, const auto &b) {
                         return a.key < b.key;
                       }) ||
      !strictly_sorted(artifact.coplanar_overlaps(),
                       [](const auto &a, const auto &b) {
                         return a.key < b.key;
                       })) {
    error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                           "Component 08 verifier rejected coplanar IDs/order");
    return false;
  }

  const auto relation_count = relations.relations().size();
  const auto candidate_count = relations.candidate_dispositions().size();
  const auto component_count = relations.coplanar_overlap_components().size();
  for (const auto &support : artifact.coplanar_supports()) {
    if (!valid_coplanar_support_key(support.key) ||
        support.key.first_facet != support.first_facet ||
        support.key.second_facet != support.second_facet ||
        support.key.support_lineage != support.support_lineage ||
        support.key.opposite_orientation != support.opposite_orientation ||
        support.key.symbolic_owner != support.symbolic_owner ||
        support.first_facet.kind != relation_feature_kind::source_facet ||
        support.second_facet.kind != relation_feature_kind::source_facet ||
        support.first_facet.operand == support.second_facet.operand ||
        support.schema_version != contract_versions::intersection_overlap_schema ||
        support.reserved16 != 0 ||
        !valid_range(support.original_boundary_edges,
                     artifact.coplanar_support_original_boundary_edge_index().size()) ||
        !valid_range(support.boundary_events,
                     artifact.coplanar_support_boundary_event_index().size()) ||
        !valid_range(support.boundary_carriers,
                     artifact.coplanar_support_boundary_carrier_index().size()) ||
        !valid_range(support.overlap_components,
                     artifact.coplanar_support_overlap_index().size()) ||
        !valid_range(support.region_incidence,
                     artifact.coplanar_support_region_index().size()) ||
        !valid_range(support.provenance,
                     artifact.coplanar_support_relation_provenance().size())) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 verifier rejected coplanar support");
      return false;
    }
    for (std::uint64_t j = 0; j < support.provenance.count; ++j)
      if (artifact.coplanar_support_relation_provenance()[support.provenance.begin + j]
              .ordinal() >= relation_count) {
        error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                               "Component 08 coplanar relation provenance is stale");
        return false;
      }
    for (std::uint64_t j = 0; j < support.original_boundary_edges.count; ++j) {
      const auto &feature = artifact.coplanar_support_original_boundary_edge_index()[
          support.original_boundary_edges.begin + j];
      if (feature.kind != relation_feature_kind::source_edge ||
          (feature.operand != support.first_facet.operand &&
           feature.operand != support.second_facet.operand)) {
        error = verifier_error(intersection_subcode::unrelated_feature_incidence,
                               "Component 08 coplanar support owns unrelated boundary edge");
        return false;
      }
    }
  }

  for (const auto &carrier : artifact.overlap_carriers()) {
    if (!valid_collinear_overlap_carrier_key(carrier.key) ||
        carrier.key.first_edge.kind != relation_feature_kind::source_edge ||
        carrier.key.second_edge.kind != relation_feature_kind::source_edge ||
        carrier.start_occurrence.ordinal() >= artifact.occurrences().size() ||
        carrier.end_occurrence.ordinal() >= artifact.occurrences().size() ||
        carrier.first_parameter_evidence.ordinal() >=
            relations.interval_evidence().size() ||
        carrier.second_parameter_evidence.ordinal() >=
            relations.interval_evidence().size() ||
        carrier.symbolic_owner != carrier.key.symbolic_owner ||
        !carrier.parameter_correspondence_verified ||
        carrier.schema_version != contract_versions::intersection_overlap_schema ||
        carrier.reserved8 != 0 || carrier.reserved16 != 0 ||
        !valid_range(carrier.provenance,
                     artifact.overlap_carrier_relation_provenance().size()) ||
        !valid_range(carrier.source_provenance,
                     artifact.overlap_carrier_source_provenance().size()) ||
        !valid_range(carrier.contributions, artifact.incidence().size()) ||
        !valid_range(carrier.descriptors, artifact.descriptors().size())) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 verifier rejected overlap carrier");
      return false;
    }
    if (carrier.zero_length &&
        carrier.start_occurrence != carrier.end_occurrence) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 zero-length overlap has distinct endpoints");
      return false;
    }
  }

  for (const auto &overlap : artifact.coplanar_overlaps()) {
    if (!valid_coplanar_overlap_key(overlap.key) ||
        overlap.support.ordinal() >= artifact.coplanar_supports().size() ||
        overlap.component07_component.ordinal() >= component_count ||
        !(overlap.key.support ==
            artifact.coplanar_supports()[overlap.support.ordinal()].key) ||
        overlap.key.component_kind != overlap.kind ||
        overlap.key.symbolic_owner != overlap.symbolic_owner ||
        overlap.key.sheet_mask != overlap.sheet_mask ||
        overlap.sheet_mask == 0 ||
        overlap.schema_version != contract_versions::intersection_overlap_schema ||
        overlap.reserved16 != 0 ||
        !valid_range(overlap.boundary_events,
                     artifact.coplanar_overlap_boundary_event_index().size()) ||
        !valid_range(overlap.boundary_carriers,
                     artifact.coplanar_overlap_boundary_carrier_index().size()) ||
        !valid_range(overlap.provenance,
                     artifact.coplanar_overlap_relation_provenance().size())) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 verifier rejected overlap component");
      return false;
    }
    const auto &predecessor =
        relations.coplanar_overlap_components()[overlap.component07_component.ordinal()];
    if (predecessor.kind != overlap.kind ||
        predecessor.sheet_mask != overlap.sheet_mask ||
        predecessor.closed != overlap.closed ||
        predecessor.distinct_sheet_occurrences !=
            overlap.distinct_sheet_occurrences) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 overlap disagrees with Component 07 lineage");
      return false;
    }
  }

  for (const auto &region : artifact.coplanar_region_incidence()) {
    if (region.support.ordinal() >= artifact.coplanar_supports().size() ||
        region.component.ordinal() >= artifact.coplanar_overlaps().size() ||
        region.first_facet.kind != relation_feature_kind::source_facet ||
        region.second_facet.kind != relation_feature_kind::source_facet ||
        region.first_triangle.kind != relation_feature_kind::source_triangle ||
        region.second_triangle.kind != relation_feature_kind::source_triangle ||
        region.sheet_mask == 0 || !region.coverage_complete ||
        region.schema_version != contract_versions::intersection_overlap_schema ||
        region.reserved8 != 0 || region.reserved16 != 0 ||
        !valid_range(region.boundary_events,
                     artifact.coplanar_region_boundary_event_index().size()) ||
        !valid_range(region.boundary_carriers,
                     artifact.coplanar_region_boundary_carrier_index().size()) ||
        !valid_range(region.coverage_witnesses,
                     artifact.coplanar_region_coverage_witness_index().size())) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 verifier rejected coplanar region incidence");
      return false;
    }
    for (std::uint64_t j = 0; j < region.coverage_witnesses.count; ++j) {
      const auto &feature = artifact.coplanar_region_coverage_witness_index()[
          region.coverage_witnesses.begin + j];
      if (!valid_relation_feature_key(feature, false)) {
        error = verifier_error(intersection_subcode::unrelated_feature_incidence,
                               "Component 08 region coverage witness is malformed");
        return false;
      }
      if (feature.kind == relation_feature_kind::facet_internal_diagonal &&
          !region.internal_diagonal_coverage_only) {
        error = verifier_error(intersection_subcode::internal_diagonal_public_ownership,
                               "Component 08 internal diagonal escaped coverage-only role");
        return false;
      }
    }
  }

  for (const auto id : artifact.coplanar_support_candidate_provenance())
    if (id.ordinal() >= candidate_count) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 coplanar candidate provenance is stale");
      return false;
    }
  for (const auto id : artifact.overlap_carrier_candidate_provenance())
    if (id.ordinal() >= candidate_count) {
      error = verifier_error(intersection_subcode::overlap_carrier_invalid,
                             "Component 08 overlap candidate provenance is stale");
      return false;
    }

  (void)coplanar;
  return add_work(artifact.coplanar_supports().size() +
                      artifact.overlap_carriers().size() +
                      artifact.coplanar_overlaps().size() +
                      artifact.coplanar_region_incidence().size(),
                  limits, work, error);
}

template <class T, class I>
bool audit_aggregates_and_descriptors(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_verifier_limits &limits, std::uint64_t &work,
    bounded_boolean_error &error) {
  bounded_boolean_error local;
  if (!verify_intersection_aggregates(
          relations.event_seeds(), interning, incidence, source_edges,
          transverse, coplanar, aggregates, local)) {
    error = verifier_error(intersection_subcode::aggregate_mismatch,
                           "Component 08 aggregate reconstruction failed");
    return false;
  }

  const auto base = project_descriptors(artifact, false);
  const auto full = project_descriptors(artifact, true);
  if (!verify_intersection_descriptors(
          relations.event_seeds(), interning, incidence, source_edges,
          transverse, coplanar, aggregates, base, local)) {
    error = verifier_error(intersection_subcode::descriptor_mismatch,
                           "Component 08 base descriptor reconstruction failed");
    return false;
  }
  if (!verify_intersection_source_topology_descriptors(
          *relations.candidates()->manifolds(), relations.crossings(),
          relations.event_seeds(), interning, incidence, base, full, local)) {
    error = verifier_error(
        intersection_subcode::facet_reconciliation_failed,
        "Component 08 source-topology descriptor reconstruction failed");
    return false;
  }

  if (!contiguous_ids<crossing_aggregate_id>(artifact.crossing_aggregates()) ||
      !contiguous_ids<contact_aggregate_id>(artifact.contact_aggregates()) ||
      !contiguous_ids<intersection_descriptor_id>(artifact.descriptors()) ||
      !strictly_sorted(artifact.descriptors(), [](const auto &a, const auto &b) {
        return a.key < b.key;
      })) {
    error = verifier_error(intersection_subcode::descriptor_mismatch,
                           "Component 08 aggregate/descriptor IDs are not canonical");
    return false;
  }

  for (const auto &record : artifact.crossing_aggregates()) {
    if (!valid_range(record.members, artifact.crossing_aggregate_members().size()) ||
        !valid_range(record.facet_subtotals,
                     artifact.crossing_facet_subtotals().size()) ||
        !valid_range(record.shell_subtotals,
                     artifact.crossing_shell_subtotals().size()) ||
        !record.member_order_verified || !record.conserved ||
        record.schema_version != contract_versions::intersection_aggregate_schema ||
        record.reserved8 != 0) {
      error = verifier_error(intersection_subcode::aggregate_mismatch,
                             "Component 08 verifier rejected crossing aggregate");
      return false;
    }
    std::int64_t numeric = 0;
    std::int64_t symbolic = 0;
    for (std::uint64_t j = 0; j < record.members.count; ++j) {
      const auto id = artifact.crossing_aggregate_members()[record.members.begin + j];
      if (id.ordinal() >= artifact.incidence().size()) {
        error = verifier_error(intersection_subcode::aggregate_mismatch,
                               "Component 08 crossing aggregate member is stale");
        return false;
      }
      const auto &member = artifact.incidence()[id.ordinal()];
      if ((member.numeric_crossing > 0 &&
           numeric > std::numeric_limits<std::int64_t>::max() -
                         member.numeric_crossing) ||
          (member.numeric_crossing < 0 &&
           numeric < std::numeric_limits<std::int64_t>::min() -
                         member.numeric_crossing)) {
        error = verifier_error(intersection_subcode::aggregate_mismatch,
                               "Component 08 crossing aggregate overflowed");
        return false;
      }
      numeric += member.numeric_crossing;
      symbolic += member.symbolic_crossing;
    }
    if (numeric != record.numeric_signed_sum ||
        symbolic != record.symbolic_signed_sum) {
      error = verifier_error(intersection_subcode::aggregate_mismatch,
                             "Component 08 crossing aggregate sum mismatch");
      return false;
    }
  }

  for (const auto &record : artifact.contact_aggregates()) {
    if (!valid_range(record.members, artifact.contact_aggregate_members().size()) ||
        !record.reconstructed ||
        record.schema_version != contract_versions::intersection_aggregate_schema ||
        record.reserved16 != 0) {
      error = verifier_error(intersection_subcode::aggregate_mismatch,
                             "Component 08 verifier rejected contact aggregate");
      return false;
    }
    if (record.members.count == 0 &&
        !(record.zero_net_retained || record.tangent_retained ||
          record.coincidence_retained)) {
      error = verifier_error(intersection_subcode::member_erased,
                             "Component 08 retained an empty unclassified contact");
      return false;
    }
  }

  for (const auto &descriptor : artifact.descriptors()) {
    if (!valid_intersection_descriptor_key(descriptor.key) ||
        descriptor.key.category == intersection_descriptor_category::unresolved ||
        descriptor.key.category == intersection_descriptor_category::invalid ||
        !valid_range(descriptor.provenance,
                     artifact.descriptor_provenance().size()) ||
        descriptor.schema_version != contract_versions::intersection_descriptor_schema ||
        descriptor.reserved8 != 0) {
      error = verifier_error(intersection_subcode::descriptor_mismatch,
                             "Component 08 verifier rejected descriptor");
      return false;
    }
    if (descriptor.key.locus ==
            intersection_descriptor_locus::transparent_internal_diagonal_adjacency &&
        (descriptor.signed_crossing_delta != 0 ||
         descriptor.classification_consumable ||
         descriptor.selection_consumable || !descriptor.topology_consumable ||
         !descriptor.continuation_allowed)) {
      error = verifier_error(intersection_subcode::internal_diagonal_public_ownership,
                             "Component 08 internal diagonal descriptor is authoritative");
      return false;
    }
  }

  return add_work(artifact.crossing_aggregates().size() +
                      artifact.contact_aggregates().size() +
                      artifact.descriptors().size() +
                      artifact.descriptor_provenance().size(),
                  limits, work, error);
}

template <class T, class I>
bool audit_statistics_replay_and_partitions(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const intersection_verifier_limits &limits, std::uint64_t &work,
    bounded_boolean_error &error) {
  const auto &s = artifact.statistics();
  const std::uint64_t aggregate_count =
      artifact.crossing_aggregates().size() + artifact.contact_aggregates().size();
  const std::uint64_t overlap_count =
      artifact.overlap_carriers().size() + artifact.coplanar_overlaps().size() +
      artifact.coplanar_region_incidence().size();
  if (s.seed_count != relations.event_seeds().size() ||
      s.event_count != artifact.events().size() ||
      s.occurrence_count != artifact.occurrences().size() ||
      s.seed_binding_count != artifact.seed_bindings().size() ||
      s.incidence_count != artifact.incidence().size() ||
      s.source_edge_membership_count != artifact.source_edge_memberships().size() ||
      s.source_edge_sequence_count != artifact.source_edge_sequences().size() ||
      s.source_edge_cluster_count != artifact.source_edge_clusters().size() ||
      s.source_edge_interval_count != artifact.source_edge_intervals().size() ||
      s.transverse_carrier_count != artifact.transverse_carriers().size() ||
      s.carrier_membership_count != artifact.carrier_memberships().size() ||
      s.carrier_cluster_count != artifact.carrier_clusters().size() ||
      s.carrier_span_count != artifact.carrier_active_spans().size() ||
      s.coplanar_support_count != artifact.coplanar_supports().size() ||
      s.overlap_count != overlap_count || s.aggregate_count != aggregate_count ||
      s.descriptor_count != artifact.descriptors().size() ||
      s.ordering_certificate_count != artifact.ordering_certificates().size() ||
      s.diagnostic_count != artifact.diagnostics().size() ||
      s.replay_checkpoint_count != artifact.replay_checkpoints().size() ||
      s.canonical_bytes != artifact.canonical_bytes().size()) {
    error = verifier_error(intersection_subcode::count_overflow,
                           "Component 08 verifier rejected artifact statistics");
    return false;
  }

  if (!contiguous_ids<ordering_certificate_id>(artifact.ordering_certificates()) ||
      !contiguous_ids<intersection_diagnostic_id>(artifact.diagnostics()) ||
      !contiguous_ids<intersection_replay_checkpoint_id>(
          artifact.replay_checkpoints())) {
    error = verifier_error(intersection_subcode::canonicalization_error,
                           "Component 08 verifier rejected auxiliary ID domains");
    return false;
  }
  for (const auto &certificate : artifact.ordering_certificates()) {
    if (certificate.policy_version !=
            contract_versions::intersection_bounded_ordering_policy ||
        certificate.reserved8 != 0 || !certificate.topology_safe ||
        certificate.disposition == intersection_order_disposition::invalid ||
        certificate.disposition ==
            intersection_order_disposition::unresolved_overlap) {
      error = verifier_error(intersection_subcode::bounded_order_contradiction,
                             "Component 08 verifier rejected ordering certificate");
      return false;
    }
    if (certificate.disposition == intersection_order_disposition::exact_equal &&
        certificate.exact_evidence_lineage == 0) {
      error = verifier_error(intersection_subcode::exact_equal_without_evidence,
                             "Component 08 exact-equal certificate lacks lineage");
      return false;
    }
  }
  for (const auto &diagnostic : artifact.diagnostics()) {
    if (diagnostic.schema_version !=
            contract_versions::intersection_diagnostic_schema ||
        diagnostic.reserved32 != 0 || diagnostic.witness_count > 4 ||
        !diagnostic.retained_finding) {
      error = verifier_error(intersection_subcode::canonicalization_error,
                             "Component 08 verifier rejected diagnostic");
      return false;
    }
  }
  std::uint32_t previous_checkpoint = 0;
  std::uint64_t previous_work = 0;
  for (const auto &checkpoint : artifact.replay_checkpoints()) {
    const auto raw = static_cast<std::uint32_t>(checkpoint.checkpoint);
    if (raw <= previous_checkpoint || raw < 1 || raw > 24 ||
        checkpoint.cumulative_work_units < previous_work ||
        checkpoint.schema_version != contract_versions::intersection_replay_schema ||
        checkpoint.reserved16 != 0 || checkpoint.reserved32 != 0) {
      error = verifier_error(intersection_subcode::canonicalization_error,
                             "Component 08 verifier rejected replay checkpoints");
      return false;
    }
    previous_checkpoint = raw;
    previous_work = checkpoint.cumulative_work_units;
  }

  return add_work(artifact.ordering_certificates().size() +
                      artifact.diagnostics().size() +
                      artifact.replay_checkpoints().size(),
                  limits, work, error);
}

} // namespace

struct intersection_verifier_access final {
  template <class T, class I>
  static void publish(canonical_intersection_complex<T, I> &artifact,
                      const intersection_verification_evidence &evidence) {
    artifact.verification_ =
        intersection_verification_disposition::independently_verified;
    artifact.verification_evidence_ = evidence;
    artifact.statistics_.verifier_work_units = evidence.work_units;
  }
};

template <class T, class I>
bool verify_intersection_complex_independent(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &codec_limits,
    const intersection_verifier_limits &limits,
    intersection_verification_evidence &evidence,
    bounded_boolean_error &error) {
  if (!valid_header_and_predecessor(relations, artifact, limits, error))
    return false;
  bounded_boolean_error local;
  if (!verify_intersection_codec(artifact, codec_limits, local)) {
    error = verifier_error(intersection_subcode::codec_error,
                           "Component 08 verifier rejected canonical codec");
    return false;
  }

  std::uint64_t pair_checks = 0;
  std::uint64_t work = 0;
  const auto interning = project_interning(artifact);
  const auto coordinates = project_coordinates(artifact);
  const auto incidence = project_incidence(artifact);
  if (!audit_event_partition(relations, artifact, interning, coordinates,
                             incidence, limits, pair_checks, work, error))
    return false;

  const auto split = certificate_split(artifact, error);
  if (split == intersection_invalid_ordinal)
    return false;
  const auto source_edges = project_source_edges(artifact, split);
  const auto transverse = project_transverse(artifact, split);
  const auto coplanar = project_coplanar(artifact);
  const auto aggregates = project_aggregates(artifact);
  if (!audit_source_edges(relations, artifact, interning, incidence,
                          source_edges, limits, pair_checks, work, error) ||
      !audit_transverse(relations, artifact, transverse, limits, pair_checks,
                        work, error) ||
      !audit_coplanar(relations, artifact, coplanar, limits, work, error) ||
      !audit_aggregates_and_descriptors(
          relations, artifact, interning, incidence, source_edges, transverse,
          coplanar, aggregates, limits, work, error) ||
      !audit_statistics_replay_and_partitions(relations, artifact, limits,
                                              work, error))
    return false;

  intersection_verification_evidence reconstructed;
  reconstructed.schema_version =
      contract_versions::intersection_exhaustive_evidence_schema;
  reconstructed.verifier_version = contract_versions::intersection_verifier;
  reconstructed.seed_regrouped = true;
  reconstructed.incidence_reconstructed = true;
  reconstructed.arrangements_reconstructed = true;
  reconstructed.descriptors_reconstructed = true;
  reconstructed.exhaustive_mode = limits.exhaustive_test_only;
  reconstructed.reserved8 = 0;
  reconstructed.reconstructed_digest =
      semantic_reconstruction_digest(relations, artifact);
  reconstructed.work_units = work;

  if (artifact.verification() ==
      intersection_verification_disposition::independently_verified) {
    if (!same_evidence(artifact.verification_evidence(), reconstructed) ||
        artifact.statistics().verifier_work_units != work) {
      error = verifier_error(intersection_subcode::verifier_rejection,
                              "Component 08 published verifier evidence mismatch");
      return false;
    }
  } else if (artifact.verification() !=
                 intersection_verification_disposition::not_verified ||
             artifact.verification_evidence().seed_regrouped ||
             artifact.verification_evidence().incidence_reconstructed ||
             artifact.verification_evidence().arrangements_reconstructed ||
             artifact.verification_evidence().descriptors_reconstructed ||
             artifact.verification_evidence().exhaustive_mode ||
             artifact.verification_evidence().work_units != 0 ||
             artifact.verification_evidence().reconstructed_digest !=
                 bounded_boolean_digest{} ||
             artifact.statistics().verifier_work_units != 0) {
    error = verifier_error(intersection_subcode::verifier_rejection,
                           "Component 08 unverified artifact forged evidence");
    return false;
  }

  evidence = reconstructed;
  return true;
}

template <class T, class I>
bool finalize_intersection_complex_verification(
    const signed_feature_relations<T, I> &relations,
    canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &codec_limits,
    const intersection_verifier_limits &limits,
    bounded_boolean_error &error) {
  intersection_verification_evidence evidence;
  if (!verify_intersection_complex_independent(relations, artifact, codec_limits,
                                               limits, evidence, error))
    return false;
  if (artifact.verification() ==
      intersection_verification_disposition::independently_verified)
    return true;

  auto candidate = artifact;
  intersection_verifier_access::publish(candidate, evidence);
  if (!refresh_intersection_codec(candidate, codec_limits, error))
    return false;
  intersection_verification_evidence confirmed;
  if (!verify_intersection_complex_independent(
          relations, candidate, codec_limits, limits, confirmed, error) ||
      !same_evidence(evidence, confirmed)) {
    if (error.summary == nullptr || error.summary[0] == '\0')
      error = verifier_error(
          intersection_subcode::verifier_rejection,
          "Component 08 post-publication verification failed");
    return false;
  }
  artifact = std::move(candidate);
  return true;
}

template <class T, class I>
bool decode_intersection_complex_verified_private(
    const std::vector<std::uint8_t> &bytes,
    const intersection_canonicalization_header &expectations,
    const signed_feature_relations<T, I> &relations,
    const intersection_codec_limits &codec_limits,
    const intersection_verifier_limits &verifier_limits,
    canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error) {
  canonical_intersection_complex<T, I> candidate;
  if (!decode_intersection_complex_private(bytes, expectations, codec_limits,
                                           candidate, error))
    return false;
  if (!finalize_intersection_complex_verification(
          relations, candidate, codec_limits, verifier_limits, error))
    return false;
  artifact = std::move(candidate);
  return true;
}

#define YGOR_INSTANTIATE_INTERSECTION_VERIFIER(T, I)                         \
  template bool verify_intersection_complex_independent<T, I>(              \
      const signed_feature_relations<T, I> &,                               \
      const canonical_intersection_complex<T, I> &,                         \
      const intersection_codec_limits &,                                    \
      const intersection_verifier_limits &,                                 \
      intersection_verification_evidence &, bounded_boolean_error &);       \
  template bool finalize_intersection_complex_verification<T, I>(           \
      const signed_feature_relations<T, I> &,                               \
      canonical_intersection_complex<T, I> &,                               \
      const intersection_codec_limits &,                                    \
      const intersection_verifier_limits &, bounded_boolean_error &);       \
  template bool decode_intersection_complex_verified_private<T, I>(        \
      const std::vector<std::uint8_t> &,                                    \
      const intersection_canonicalization_header &,                         \
      const signed_feature_relations<T, I> &,                               \
      const intersection_codec_limits &,                                    \
      const intersection_verifier_limits &,                                \
      canonical_intersection_complex<T, I> &, bounded_boolean_error &)

YGOR_INSTANTIATE_INTERSECTION_VERIFIER(float, std::uint32_t);
YGOR_INSTANTIATE_INTERSECTION_VERIFIER(float, std::uint64_t);
YGOR_INSTANTIATE_INTERSECTION_VERIFIER(double, std::uint32_t);
YGOR_INSTANTIATE_INTERSECTION_VERIFIER(double, std::uint64_t);

#undef YGOR_INSTANTIATE_INTERSECTION_VERIFIER

} // namespace ygor::mesh_boolean::bounded
