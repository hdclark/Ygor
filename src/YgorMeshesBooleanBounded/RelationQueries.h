#pragma once

#include "SignedFeatureRelations.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ygor::mesh_boolean::bounded {

template <class Record> struct relation_record_range final {
  const Record *data = nullptr;
  std::uint64_t count = 0;
  bool valid = false;

  const Record *begin() const noexcept { return data; }
  const Record *end() const noexcept {
    return count == 0 ? data : data + count;
  }
  bool empty() const noexcept { return count == 0; }
  explicit operator bool() const noexcept { return valid; }
  const Record &operator[](std::uint64_t ordinal) const noexcept {
    return data[ordinal];
  }
};

// Allocation-free, immutable downstream capability for Components 08-10.
// The view deliberately exposes only published Component 07 evidence. It does
// not expose predecessor meshes, relation-stage raw inputs, or any arithmetic
// service from which downstream code could recompute authoritative geometry.
template <class T, class I> class signed_feature_relations_view final {
public:
  signed_feature_relations_view() noexcept = default;
  signed_feature_relations_view(
      const signed_feature_relations<T, I> &artifact,
      const context_owner_token &owner) noexcept
      : artifact_(&artifact), owner_(owner) {}

  bool valid_owner() const noexcept {
    return artifact_ && artifact_->owner().same_owner(owner_) &&
           artifact_->verification() ==
               relation_verification_disposition::independently_verified;
  }

  const context_owner_token &owner() const noexcept { return owner_; }
  relation_verification_disposition verification() const noexcept {
    return valid_owner()
               ? relation_verification_disposition::independently_verified
               : relation_verification_disposition::unverified;
  }

  std::uint16_t schema_version() const noexcept {
    return valid_owner() ? artifact_->schema_version() : 0;
  }
  std::uint16_t provider_version() const noexcept {
    return valid_owner() ? artifact_->provider_version() : 0;
  }
  std::uint16_t graph_policy_version() const noexcept {
    return valid_owner() ? artifact_->graph_policy_version() : 0;
  }
  std::uint16_t truth_policy_version() const noexcept {
    return valid_owner() ? artifact_->truth_policy_version() : 0;
  }
  std::uint16_t codec_version() const noexcept {
    return valid_owner() ? artifact_->codec_version() : 0;
  }
  std::uint16_t verifier_version() const noexcept {
    return valid_owner() ? artifact_->verifier_version() : 0;
  }
  relation_provider_kind provider() const noexcept {
    return valid_owner()
               ? artifact_->provider()
               : relation_provider_kind::canonical_source_feature_relation_graph_v1;
  }
  boolean_operation operation() const noexcept {
    return valid_owner() ? artifact_->operation() : boolean_operation::set_union;
  }
  T residual_boundary() const noexcept {
    return valid_owner() ? artifact_->residual_boundary() : T(0);
  }

  const bounded_boolean_digest *context_digest() const noexcept {
    return valid_owner() ? &artifact_->context_digest() : nullptr;
  }
  const bounded_boolean_digest *precision_digest() const noexcept {
    return valid_owner() ? &artifact_->precision_digest() : nullptr;
  }
  const bounded_boolean_digest *candidate_digest() const noexcept {
    return valid_owner() ? &artifact_->candidate_digest() : nullptr;
  }
  const bounded_boolean_digest *graph_digest() const noexcept {
    return valid_owner() ? &artifact_->graph_digest() : nullptr;
  }
  const bounded_boolean_digest *symbolic_policy_digest() const noexcept {
    return valid_owner() ? &artifact_->symbolic_policy_digest() : nullptr;
  }
  const bounded_boolean_digest *artifact_digest() const noexcept {
    return valid_owner() ? &artifact_->digest() : nullptr;
  }
  const relation_statistics *statistics() const noexcept {
    return valid_owner() ? &artifact_->statistics() : nullptr;
  }
  const relation_replay_evidence *replay_evidence() const noexcept {
    return valid_owner() ? &artifact_->replay_evidence() : nullptr;
  }
  const std::array<relation_predecessor_commitment_record, 6> *
  predecessor_commitments() const noexcept {
    return valid_owner() ? &artifact_->predecessor_commitments() : nullptr;
  }
  const std::vector<relation_resource_evidence_record> &resource_evidence()
      const noexcept {
    return valid_owner() ? artifact_->resource_evidence()
                         : empty_vector<relation_resource_evidence_record>();
  }
  const std::array<relation_section_digest_record, 10> *section_digests()
      const noexcept {
    return valid_owner() ? &artifact_->section_digests() : nullptr;
  }

  const bounded_boolean_digest &digest() const noexcept {
    return valid_owner() ? artifact_->digest() : empty_digest();
  }
  const std::vector<relation_event_seed_record> &event_seeds() const noexcept {
    return valid_owner() ? artifact_->event_seeds() : empty_vector<relation_event_seed_record>();
  }
  const std::vector<relation_construction_record> &constructions() const noexcept {
    return valid_owner() ? artifact_->constructions() : empty_vector<relation_construction_record>();
  }
  const std::vector<relation_construction_ledger_record> &construction_ledger() const noexcept {
    return valid_owner() ? artifact_->construction_ledger() : empty_vector<relation_construction_ledger_record>();
  }
  const std::vector<relation_feature_key> &event_seed_incidence() const noexcept {
    return valid_owner() ? artifact_->event_seed_incidence() : empty_vector<relation_feature_key>();
  }
  const std::vector<relation_event_seed_candidate_incidence_record> &
  event_seed_candidate_incidence() const noexcept {
    return valid_owner() ? artifact_->event_seed_candidate_incidence()
                         : empty_vector<relation_event_seed_candidate_incidence_record>();
  }
  const std::vector<feature_relation_record> &relations() const noexcept {
    return valid_owner() ? artifact_->relations() : empty_vector<feature_relation_record>();
  }
  const std::vector<relation_candidate_disposition_record> &
  candidate_dispositions() const noexcept {
    return valid_owner() ? artifact_->candidate_dispositions()
                         : empty_vector<relation_candidate_disposition_record>();
  }
  const std::vector<relation_interval_evidence_record> &interval_evidence() const noexcept {
    return valid_owner() ? artifact_->interval_evidence()
                         : empty_vector<relation_interval_evidence_record>();
  }
  const std::vector<relation_source_facet_region_record<T>> &
  source_facet_regions() const noexcept {
    return valid_owner() ? artifact_->source_facet_regions()
                         : empty_vector<relation_source_facet_region_record<T>>();
  }
  const std::vector<relation_crossing_record> &crossings() const noexcept {
    return valid_owner() ? artifact_->crossings() : empty_vector<relation_crossing_record>();
  }
  const std::vector<relation_transverse_carrier_membership_record> &
  transverse_carrier_memberships() const noexcept {
    return valid_owner() ? artifact_->transverse_carrier_memberships()
                         : empty_vector<relation_transverse_carrier_membership_record>();
  }
  const std::vector<relation_transverse_carrier_support_record> &
  transverse_carrier_supports() const noexcept {
    return valid_owner() ? artifact_->transverse_carrier_supports()
                         : empty_vector<relation_transverse_carrier_support_record>();
  }
  const std::vector<relation_coplanar_event_node_record> &
  coplanar_event_nodes() const noexcept {
    return valid_owner() ? artifact_->coplanar_event_nodes()
                         : empty_vector<relation_coplanar_event_node_record>();
  }
  const std::vector<relation_coplanar_oriented_arc_record> &
  coplanar_oriented_arcs() const noexcept {
    return valid_owner() ? artifact_->coplanar_oriented_arcs()
                         : empty_vector<relation_coplanar_oriented_arc_record>();
  }
  const std::vector<relation_coplanar_overlap_component_record> &
  coplanar_overlap_components() const noexcept {
    return valid_owner() ? artifact_->coplanar_overlap_components()
                         : empty_vector<relation_coplanar_overlap_component_record>();
  }
  const std::vector<relation_coplanar_support_record> &
  coplanar_supports() const noexcept {
    return valid_owner() ? artifact_->coplanar_supports()
                         : empty_vector<relation_coplanar_support_record>();
  }
  const std::array<relation_source_topology_record, 2> &
  source_topology() const noexcept {
    return valid_owner() ? artifact_->source_topology() : empty_topology();
  }
  const relation_request_graph &request_graph() const noexcept {
    return valid_owner() ? artifact_->request_graph() : empty_graph();
  }

  const canonical_relation_request *request(
      relation_request_id id) const noexcept {
    return at(valid_owner() ? &artifact_->request_graph().requests : nullptr,
              id.ordinal());
  }
  const canonical_relation_dependency *dependency(
      relation_dependency_id id) const noexcept {
    return at(valid_owner() ? &artifact_->request_graph().dependencies : nullptr,
              id.ordinal());
  }
  const relation_imported_geometry_record *imported_geometry(
      relation_imported_geometry_id id) const noexcept {
    return at(valid_owner() ? &artifact_->imported_geometry() : nullptr,
              id.ordinal());
  }
  const relation_bounded_primitive_record *bounded_primitive(
      relation_bounded_primitive_id id) const noexcept {
    return at(valid_owner() ? &artifact_->bounded_primitives() : nullptr,
              id.ordinal());
  }
  const relation_exact_relation_record *exact_relation(
      relation_exact_relation_id id) const noexcept {
    return at(valid_owner() ? &artifact_->exact_relations() : nullptr,
              id.ordinal());
  }
  const relation_truth_lineage_record *truth_lineage(
      relation_truth_lineage_id id) const noexcept {
    return at(valid_owner() ? &artifact_->truth_lineage() : nullptr,
              id.ordinal());
  }
  const relation_interval_evidence_record *interval_evidence(
      relation_interval_evidence_id id) const noexcept {
    return at(valid_owner() ? &artifact_->interval_evidence() : nullptr,
              id.ordinal());
  }
  const relation_source_facet_region_record<T> *source_facet_region(
      relation_source_facet_region_id id) const noexcept {
    return at(valid_owner() ? &artifact_->source_facet_regions() : nullptr,
              id.ordinal());
  }
  const relation_truth_record *truth(std::uint64_t ordinal) const noexcept {
    return at(valid_owner() ? &artifact_->truth_records() : nullptr, ordinal);
  }
  const feature_relation_record *relation(
      feature_relation_id id) const noexcept {
    return at(valid_owner() ? &artifact_->relations() : nullptr, id.ordinal());
  }
  const relation_construction_record *construction(
      relation_construction_id id) const noexcept {
    return at(valid_owner() ? &artifact_->constructions() : nullptr,
              id.ordinal());
  }
  const relation_construction_ledger_record *construction_ledger(
      relation_construction_ledger_id id) const noexcept {
    return at(valid_owner() ? &artifact_->construction_ledger() : nullptr,
              id.ordinal());
  }
  const symbolic_eligibility_record *symbolic_eligibility(
      std::uint64_t ordinal) const noexcept {
    return at(valid_owner() ? &artifact_->symbolic_eligibility() : nullptr,
              ordinal);
  }
  const symbolic_relation_decision_record *symbolic_decision(
      symbolic_relation_decision_id id) const noexcept {
    return at(valid_owner() ? &artifact_->symbolic_decisions() : nullptr,
              id.ordinal());
  }
  const relation_crossing_record *crossing(
      std::uint64_t ordinal) const noexcept {
    return at(valid_owner() ? &artifact_->crossings() : nullptr, ordinal);
  }
  const relation_event_seed_record *event_seed(
      relation_event_seed_id id) const noexcept {
    return at(valid_owner() ? &artifact_->event_seeds() : nullptr,
              id.ordinal());
  }
  const relation_event_seed_candidate_incidence_record *candidate_incidence(
      relation_event_seed_incidence_id id) const noexcept {
    return at(valid_owner() ? &artifact_->event_seed_candidate_incidence()
                            : nullptr,
              id.ordinal());
  }
  const relation_candidate_disposition_record *candidate_disposition(
      relation_candidate_disposition_id id) const noexcept {
    return at(valid_owner() ? &artifact_->candidate_dispositions() : nullptr,
              id.ordinal());
  }
  const relation_candidate_partition_record *partition(
      relation_candidate_partition_id id) const noexcept {
    return at(valid_owner() ? &artifact_->candidate_partitions() : nullptr,
              id.ordinal());
  }
  const relation_transverse_carrier_membership_record *
  transverse_carrier_membership(
      relation_transverse_carrier_membership_id id) const noexcept {
    return at(valid_owner() ? &artifact_->transverse_carrier_memberships()
                            : nullptr,
              id.ordinal());
  }
  const relation_coplanar_support_record *coplanar_support(
      relation_coplanar_support_id id) const noexcept {
    return at(valid_owner() ? &artifact_->coplanar_supports() : nullptr,
              id.ordinal());
  }
  const relation_diagnostic_record *diagnostic(
      relation_diagnostic_id id) const noexcept {
    return at(valid_owner() ? &artifact_->diagnostics() : nullptr,
              id.ordinal());
  }
  const relation_replay_checkpoint_record *replay_checkpoint(
      relation_replay_checkpoint_id id) const noexcept {
    return at(valid_owner() ? &artifact_->replay_checkpoints() : nullptr,
              id.ordinal());
  }

  relation_record_range<canonical_relation_dependency>
  dependencies(relation_request_id id) const noexcept {
    const auto *record = request(id);
    return record ? subrange(artifact_->request_graph().dependencies,
                             record->dependency_begin,
                             record->dependency_count)
                  : invalid_range<canonical_relation_dependency>();
  }
  relation_record_range<relation_request_id>
  reverse_consumers(relation_request_id id) const noexcept {
    const auto *record = request(id);
    return record ? subrange(artifact_->request_graph().reverse_consumers,
                             record->reverse_consumer_begin,
                             record->reverse_consumer_count)
                  : invalid_range<relation_request_id>();
  }
  relation_record_range<candidate_id>
  candidate_witnesses(relation_request_id id) const noexcept {
    const auto *record = request(id);
    return record ? subrange(artifact_->request_graph().candidate_witnesses,
                             record->witness_begin, record->witness_count)
                  : invalid_range<candidate_id>();
  }
  relation_record_range<relation_truth_record>
  relation_truths(feature_relation_id id) const noexcept {
    const auto *record = relation(id);
    return record ? subrange(artifact_->truth_records(), record->truth_begin,
                             record->truth_count)
                  : invalid_range<relation_truth_record>();
  }
  relation_record_range<relation_truth_record>
  construction_residual_truths(relation_construction_id id) const noexcept {
    const auto *record = construction(id);
    return record ? subrange(artifact_->truth_records(),
                             record->residual_truth_begin,
                             record->residual_truth_count)
                  : invalid_range<relation_truth_record>();
  }
  relation_record_range<relation_interval_evidence_record>
  construction_intervals(relation_construction_id id) const noexcept {
    const auto *record = construction(id);
    return record ? subrange(artifact_->interval_evidence(),
                             record->interval_evidence_begin,
                             record->interval_evidence_count)
                  : invalid_range<relation_interval_evidence_record>();
  }
  relation_record_range<relation_source_facet_region_record<T>>
  construction_regions(relation_construction_id id) const noexcept {
    const auto *record = construction(id);
    return record ? subrange(artifact_->source_facet_regions(),
                             record->source_facet_region_begin,
                             record->source_facet_region_count)
                  : invalid_range<relation_source_facet_region_record<T>>();
  }
  relation_record_range<relation_construction_ledger_record>
  construction_witnesses(relation_construction_id id) const noexcept {
    const auto *record = construction(id);
    return record ? subrange(artifact_->construction_ledger(),
                             record->ledger_begin, record->ledger_count)
                  : invalid_range<relation_construction_ledger_record>();
  }
  relation_record_range<relation_feature_key>
  event_incidence(relation_event_seed_id id) const noexcept {
    const auto *record = event_seed(id);
    return record ? subrange(artifact_->event_seed_incidence(),
                             record->incidence_begin, record->incidence_count)
                  : invalid_range<relation_feature_key>();
  }
  relation_record_range<relation_event_seed_candidate_incidence_record>
  event_candidate_incidence(relation_event_seed_id id) const noexcept {
    const auto *record = event_seed(id);
    return record
               ? subrange(artifact_->event_seed_candidate_incidence(),
                          record->candidate_incidence_begin,
                          record->candidate_incidence_count)
               : invalid_range<relation_event_seed_candidate_incidence_record>();
  }
  relation_record_range<feature_relation_id>
  candidate_relations(relation_candidate_disposition_id id) const noexcept {
    const auto *record = candidate_disposition(id);
    return record ? subrange(artifact_->candidate_relation_coverage(),
                             record->relation_begin, record->relation_count)
                  : invalid_range<feature_relation_id>();
  }
  relation_record_range<relation_event_seed_id>
  candidate_event_seeds(relation_candidate_disposition_id id) const noexcept {
    const auto *record = candidate_disposition(id);
    return record ? subrange(artifact_->candidate_event_seed_coverage(),
                             record->event_seed_begin,
                             record->event_seed_count)
                  : invalid_range<relation_event_seed_id>();
  }

private:
  template <class Record>
  static const std::vector<Record> &empty_vector() noexcept {
    static const std::vector<Record> empty;
    return empty;
  }
  static const bounded_boolean_digest &empty_digest() noexcept {
    static const bounded_boolean_digest empty;
    return empty;
  }
  static const std::array<relation_source_topology_record, 2> &
  empty_topology() noexcept {
    static const std::array<relation_source_topology_record, 2> empty{};
    return empty;
  }
  static const relation_request_graph &empty_graph() noexcept {
    static const relation_request_graph empty;
    return empty;
  }
  template <class Record>
  static const Record *at(const std::vector<Record> *records,
                          std::uint64_t ordinal) noexcept {
    if (!records || ordinal >= records->size())
      return nullptr;
    return &(*records)[static_cast<std::size_t>(ordinal)];
  }

  template <class Record>
  static relation_record_range<Record> invalid_range() noexcept {
    return {nullptr, 0, false};
  }

  template <class Record>
  static relation_record_range<Record>
  subrange(const std::vector<Record> &records, std::uint64_t begin,
           std::uint64_t count) noexcept {
    const auto size = static_cast<std::uint64_t>(records.size());
    if (begin > size || count > size - begin)
      return invalid_range<Record>();
    if (count == 0)
      return {nullptr, 0, true};
    return {records.data() + static_cast<std::size_t>(begin), count, true};
  }

  const signed_feature_relations<T, I> *artifact_ = nullptr;
  context_owner_token owner_{};
};

template <class T, class I>
std::pair<const relation_candidate_disposition_record *,
          const relation_candidate_disposition_record *>
candidate_disposition_range(const signed_feature_relations<T, I> &artifact,
                            candidate_id candidate,
                            const context_owner_token &owner) noexcept {
  if (!artifact.owner().same_owner(owner))
    return {nullptr, nullptr};
  const auto &records = artifact.candidate_dispositions();
  const auto first = std::lower_bound(
      records.begin(), records.end(), candidate,
      [](const relation_candidate_disposition_record &record,
         candidate_id value) { return record.candidate < value; });
  const auto last = std::upper_bound(
      first, records.end(), candidate,
      [](candidate_id value,
         const relation_candidate_disposition_record &record) {
        return value < record.candidate;
      });
  const auto begin_index = static_cast<std::size_t>(first - records.begin());
  const auto end_index = static_cast<std::size_t>(last - records.begin());
  const auto *base = records.empty() ? nullptr : records.data();
  return {base ? base + begin_index : nullptr, base ? base + end_index : nullptr};
}

} // namespace ygor::mesh_boolean::bounded
