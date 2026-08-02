#include "StrictFloatingBuild.h"
#include "IntersectionBuild.h"

#include "ContextVerifier.h"
#include "CoplanarCarrierArrangements.h"
#include "EventCoordinates.h"
#include "EventIncidence.h"
#include "EventInterning.h"
#include "FacetFacetRelations.h"
#include "IntersectionAggregation.h"
#include "IntersectionCanonicalization.h"
#include "IntersectionCodec.h"
#include "IntersectionDescriptors.h"
#include "IntersectionPreflight.h"
#include "RelationSemanticProjection.h"
#include "SourceEdgeArrangements.h"
#include "Transaction.h"
#include "TransverseCarrierArrangements.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace intersection_build_detail {

inline bounded_boolean_digest intersection_failure_replay_digest(
    const bounded_boolean_digest &invocation_digest,
    const bounded_boolean_error &error) {
  canonical_writer writer;
  writer.u16(contract_versions::intersection_replay_schema);
  for (const auto byte : invocation_digest.bytes)
    writer.u8(byte);
  writer.u16(error.version);
  writer.u8(static_cast<std::uint8_t>(error.category));
  writer.u32(error.subcode);
  writer.u16(error.component);
  writer.u16(error.stage);
  writer.u32(error.checkpoint);
  writer.u8(error.witness_count);
  for (std::size_t i = 0; i < error.witnesses.size(); ++i)
    writer.u64(i < error.witness_count ? error.witnesses[i] : 0);
  return sha256::digest(writer.bytes());
}

inline void bind_intersection_error(
    bounded_boolean_error &error,
    const bounded_boolean_digest &context_digest,
    const bounded_boolean_digest &invocation_digest) {
  error.context_digest = context_digest;
  error.replay_digest =
      intersection_failure_replay_digest(invocation_digest, error);
}

template <class T, class I>
std::vector<source_edge_domain_record> source_edge_domains(
    const signed_feature_relations<T, I> &relations) {
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
      domains.push_back(domain);
    }
  }
  std::sort(domains.begin(), domains.end(), [](const auto &a, const auto &b) {
    return a.source_edge < b.source_edge;
  });
  domains.erase(
      std::unique(domains.begin(), domains.end(), [](const auto &a,
                                                     const auto &b) {
        return a.source_edge == b.source_edge;
      }),
      domains.end());
  return domains;
}

template <class T, class I>
bool has_transverse_lineage(const signed_feature_relations<T, I> &relations) {
  if (relations.source_facet_stage()) {
    for (const auto &record : relations.source_facet_stage()->relations)
      if (record.has_transverse_carrier)
        return true;
  }
  for (const auto &construction : relations.constructions())
    if (construction.kind == relation_construction_kind::bounded_carrier)
      return true;
  return false;
}

template <class T, class I>
bool has_coplanar_lineage(const signed_feature_relations<T, I> &relations) {
  if (!relations.coplanar_event_nodes().empty() ||
      !relations.coplanar_oriented_arcs().empty() ||
      !relations.coplanar_overlap_components().empty())
    return true;
  if (relations.source_facet_stage()) {
    for (const auto &record : relations.source_facet_stage()->relations) {
      if (record.classification ==
              source_facet_support_relation_class::coplanar_same_orientation ||
          record.classification ==
              source_facet_support_relation_class::coplanar_opposite_orientation)
        return true;
    }
  }
  return false;
}

inline bool checked_accumulate(std::uint64_t value,
                               std::uint64_t &total) noexcept {
  return checked_add(total, value, total);
}

inline bool checked_scale(std::uint64_t count, std::uint64_t element_size,
                          std::uint64_t &bytes) noexcept {
  return checked_multiply(count, element_size, bytes);
}

template <class Vector>
bool vector_bytes(const Vector &values, std::uint64_t &bytes) noexcept {
  return checked_scale(static_cast<std::uint64_t>(values.size()),
                       static_cast<std::uint64_t>(sizeof(typename Vector::value_type)),
                       bytes);
}

inline std::uint64_t saturating_add(std::uint64_t a,
                                    std::uint64_t b) noexcept {
  std::uint64_t out = 0;
  return checked_add(a, b, out) ? out
                                : std::numeric_limits<std::uint64_t>::max();
}

inline std::uint64_t saturating_multiply(std::uint64_t a,
                                         std::uint64_t b) noexcept {
  std::uint64_t out = 0;
  return checked_multiply(a, b, out)
             ? out
             : std::numeric_limits<std::uint64_t>::max();
}

struct reservation_record final {
  resource_kind kind = resource_kind::temporary_bytes;
  std::optional<resource_reservation> reservation{};
  std::uint64_t used = 0;
};

} // namespace intersection_build_detail

template <class T, class I> class intersection_builder final {
public:
  intersection_builder(
      const boolean_context<T, I> &context,
      const precision_context<T> &precision,
      std::shared_ptr<const signed_feature_relations<T, I>> relations,
      intersection_capabilities capabilities,
      intersection_codec_limits codec_limits,
      intersection_verifier_limits verifier_limits)
      : context_(context), precision_(precision), relations_(std::move(relations)),
        capabilities_(std::move(capabilities)), codec_limits_(codec_limits),
        verifier_limits_(verifier_limits) {}

  boolean_outcome<
      std::shared_ptr<const canonical_intersection_complex<T, I>>>
  run() {
    try {
      if (!validate_contracts() || !preflight_and_reserve() ||
          !normalize_and_intern() || !attach_coordinates() ||
          !publish_incidence() || !build_source_edges() ||
          !build_transverse() || !build_coplanar() ||
          !build_aggregates_and_descriptors() || !canonicalize() ||
          !encode_and_verify() || !reconcile_resources())
        return failure();

      auto published =
          std::shared_ptr<const canonical_intersection_complex<T, I>>(
              std::move(artifact_));
      if (!check_cancel(intersection_checkpoint::transaction_commit) ||
          !commit_resources() || !transaction_.begin_join() ||
          !transaction_.begin_verify() || !transaction_.ready() ||
          !transaction_.commit()) {
        fail(intersection_subcode::transaction_failure,
             bounded_boolean_error_category::internal_invariant_error,
             "Component 08 transaction could not commit",
             intersection_checkpoint::transaction_commit);
        return failure();
      }
      return outcome_type::success(std::move(published));
    } catch (const std::bad_alloc &) {
      fail(intersection_subcode::resource_preflight,
           bounded_boolean_error_category::resource_limit,
           "Component 08 allocation failed",
           intersection_checkpoint::resource_reservation);
      return failure();
    } catch (...) {
      fail(intersection_subcode::internal_invariant,
           bounded_boolean_error_category::internal_invariant_error,
           "Component 08 construction raised an unexpected exception",
           intersection_checkpoint::transaction_commit);
      return failure();
    }
  }

private:
  using artifact_type = canonical_intersection_complex<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const canonical_intersection_complex<T, I>>>;

  outcome_type failure() {
    intersection_build_detail::bind_intersection_error(
        error_, context_.context_digest, context_.replay_digest);
    return outcome_type::failure(error_);
  }

  bool fail(intersection_subcode subcode,
            bounded_boolean_error_category category, const char *summary,
            intersection_checkpoint checkpoint) {
    if (error_.subcode == 0)
      error_ = intersection_error(subcode, category, summary, checkpoint);
    return false;
  }

  bool check_cancel(intersection_checkpoint checkpoint) {
    return !intersection_cancelled(capabilities_, checkpoint) ||
           fail(intersection_subcode::cancelled,
                bounded_boolean_error_category::cancelled,
                "Component 08 construction cancelled", checkpoint);
  }

  bool validate_contracts() {
    if (!transaction_.open())
      return fail(intersection_subcode::transaction_failure,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 08 transaction did not open",
                  intersection_checkpoint::context_capability_validation);
    if (!relations_ || !verify_context(context_) ||
        !context_.owner.same_owner(capabilities_.owner) ||
        !precision_.owner().same_owner(capabilities_.owner) ||
        !relations_->owner().same_owner(capabilities_.owner) ||
        context_.operation != relations_->operation() ||
        context_.context_digest != relations_->context_digest() ||
        relation_precision_semantic_digest(precision_) !=
            relations_->precision_digest() ||
        !relations_->candidates() ||
        !relations_->candidates()->owner().same_owner(capabilities_.owner))
      return fail(intersection_subcode::wrong_owner,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 08 predecessor handshake failed",
                  intersection_checkpoint::predecessor_validation);
    if (capabilities_.provider_version !=
            contract_versions::intersection_provider ||
        capabilities_.semantic_policy_version !=
            contract_versions::intersection_semantic_policy ||
        capabilities_.codec_version != contract_versions::intersection_codec ||
        capabilities_.verifier_version !=
            contract_versions::intersection_verifier ||
        capabilities_.reserved != 0 || !capabilities_.resources ||
        (capabilities_.cancellation_observer &&
         (capabilities_.cancellation_observer->version !=
              contract_versions::intersection_cancellation_observer ||
          capabilities_.cancellation_observer->reserved16 != 0 ||
          capabilities_.cancellation_observer->reserved32 != 0 ||
          !capabilities_.cancellation_observer->poll)) ||
        (context_.options.execution.mode != bounded_execution_mode::serial_v1 &&
         context_.options.execution.mode !=
             bounded_execution_mode::deterministic_parallel_v1))
      return fail(intersection_subcode::unsupported_version,
                  bounded_boolean_error_category::input_contract_error,
                  "Component 08 capability or execution profile is unsupported",
                  intersection_checkpoint::context_capability_validation);
    if (relations_->verification() !=
        relation_verification_disposition::independently_verified)
      return fail(intersection_subcode::predecessor_not_verified,
                  bounded_boolean_error_category::input_contract_error,
                  "Component 08 requires independently verified Component 07 input",
                  intersection_checkpoint::predecessor_validation);
    if (!check_cancel(intersection_checkpoint::context_capability_validation) ||
        !check_cancel(intersection_checkpoint::predecessor_validation))
      return false;
    return true;
  }

  bool reserve(resource_kind kind, std::uint64_t amount,
               std::uint64_t used = intersection_invalid_ordinal) {
    auto reservation = capabilities_.resources->reserve(kind, amount);
    if (!reservation)
      return fail(intersection_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 08 resource reservation failed",
                  intersection_checkpoint::resource_reservation);
    intersection_build_detail::reservation_record record;
    record.kind = kind;
    record.reservation.emplace(std::move(*reservation));
    record.used = used == intersection_invalid_ordinal ? amount : used;
    reservations_.push_back(std::move(record));
    return true;
  }

  bool preflight_and_reserve() {
    if (!check_cancel(intersection_checkpoint::count_preflight) ||
        !preflight_intersection_events(*relations_, capabilities_, preflight_,
                                       error_))
      return false;

    domains_ = intersection_build_detail::source_edge_domains(*relations_);
    const auto membership_bound = preflight_.estimate.membership_count;
    const auto cluster_bound = preflight_.estimate.cluster_count;
    const auto interval_bound = preflight_.estimate.interval_count;
    const auto carrier_bound = preflight_.estimate.carrier_count;
    const auto overlap_bound = preflight_.estimate.overlap_count;
    const auto aggregate_bound = preflight_.estimate.aggregate_count;
    const auto descriptor_bound = preflight_.estimate.descriptor_count;
    const auto certificate_bound = preflight_.ordering_certificate_count;
    const auto temp_bound = preflight_.estimate.temporary_bytes;
    const auto work_bound = preflight_.estimate.work_units;

    if (!reserve(resource_kind::temporary_bytes, temp_bound, 0) ||
        !reserve(resource_kind::work_units, work_bound, 0) ||
        !reserve(resource_kind::events, preflight_.estimate.event_count) ||
        !reserve(resource_kind::intersection_occurrences,
                 preflight_.estimate.occurrence_count) ||
        !reserve(resource_kind::intersection_incidence,
                 preflight_.estimate.incidence_count) ||
        !reserve(resource_kind::intersection_memberships, membership_bound, 0) ||
        !reserve(resource_kind::intersection_clusters, cluster_bound, 0) ||
        !reserve(resource_kind::intersection_intervals, interval_bound, 0) ||
        !reserve(resource_kind::intersection_carriers, carrier_bound, 0) ||
        !reserve(resource_kind::intersection_overlaps, overlap_bound, 0) ||
        !reserve(resource_kind::intersection_aggregates, aggregate_bound, 0) ||
        !reserve(resource_kind::intersection_descriptors, descriptor_bound, 0) ||
        !reserve(resource_kind::intersection_order_certificates,
                 certificate_bound, 0))
      return false;
    if (!check_cancel(intersection_checkpoint::resource_reservation))
      return false;
    return transaction_.register_work() ||
           fail(intersection_subcode::transaction_failure,
                bounded_boolean_error_category::internal_invariant_error,
                "Component 08 transaction could not register work",
                intersection_checkpoint::resource_reservation);
  }

  bool normalize_and_intern() {
    if (!check_cancel(intersection_checkpoint::seed_normalization) ||
        !normalize_event_seed_records(relations_->event_seeds(),
                                      relations_->constructions(), proposals_,
                                      error_))
      return false;
    if (!check_cancel(intersection_checkpoint::event_grouping) ||
        !intern_normalized_event_seeds(proposals_, interning_, error_))
      return false;
    return check_cancel(intersection_checkpoint::event_occurrence_id_assignment);
  }

  bool attach_coordinates() {
    if (!check_cancel(intersection_checkpoint::authoritative_point_attachment))
      return false;
    return attach_event_coordinates(
        proposals_, relations_->constructions(), relations_->construction_ledger(),
        interning_, coordinates_, error_);
  }

  bool publish_incidence() {
    if (!check_cancel(intersection_checkpoint::incidence_proposals) ||
        !build_event_incidence(*relations_, interning_, incidence_, error_))
      return false;
    return check_cancel(intersection_checkpoint::incidence_publication);
  }

  bool build_source_edges() {
    if (!check_cancel(
            intersection_checkpoint::source_edge_membership_proposals) ||
        !collect_source_edge_membership_proposals(
            relations_->event_seeds(), relations_->interval_evidence(),
            interning_, incidence_, source_memberships_, error_))
      return false;
    if (!check_cancel(intersection_checkpoint::source_edge_ordering) ||
        !build_source_edge_arrangements<T>(domains_, source_memberships_,
                                           source_edges_, error_) ||
        !verify_source_edge_arrangements<T>(domains_, source_memberships_,
                                            source_edges_, error_))
      return false;
    return check_cancel(intersection_checkpoint::source_edge_partition);
  }

  bool build_transverse() {
    if (!check_cancel(intersection_checkpoint::transverse_carriers))
      return false;
    if (intersection_build_detail::has_transverse_lineage(*relations_))
      return fail(
          intersection_subcode::membership_incomplete,
          bounded_boolean_error_category::internal_invariant_error,
          "Component 08 transverse proposal ingestion is not yet integrated",
          intersection_checkpoint::transverse_carriers);
    return build_transverse_carrier_arrangements<T>({}, {}, {}, transverse_,
                                                     error_) &&
           verify_transverse_carrier_arrangements<T>({}, {}, {}, transverse_,
                                                      error_);
  }

  bool build_coplanar() {
    if (!check_cancel(intersection_checkpoint::coplanar_carriers))
      return false;
    if (intersection_build_detail::has_coplanar_lineage(*relations_))
      return fail(
          intersection_subcode::membership_incomplete,
          bounded_boolean_error_category::internal_invariant_error,
          "Component 08 coplanar proposal ingestion is not yet integrated",
          intersection_checkpoint::coplanar_carriers);
    return build_coplanar_carrier_arrangements<T>({}, {}, {}, {}, coplanar_,
                                                   error_) &&
           verify_coplanar_carrier_arrangements<T>({}, {}, {}, {}, coplanar_,
                                                    error_);
  }

  bool build_aggregates_and_descriptors() {
    if (!check_cancel(intersection_checkpoint::aggregate_reconstruction) ||
        !build_intersection_aggregates(
            relations_->event_seeds(), interning_, incidence_, source_edges_,
            transverse_, coplanar_, aggregates_, error_))
      return false;
    if (!check_cancel(intersection_checkpoint::descriptor_derivation) ||
        !build_intersection_descriptors(
            relations_->event_seeds(), interning_, incidence_, source_edges_,
            transverse_, coplanar_, aggregates_, base_descriptors_, error_))
      return false;
    if (!check_cancel(intersection_checkpoint::source_facet_reconciliation) ||
        !extend_intersection_descriptors_with_source_topology(
            *relations_->candidates()->manifolds(), relations_->crossings(),
            relations_->event_seeds(), interning_, incidence_,
            base_descriptors_, descriptors_, error_))
      return false;
    return true;
  }

  intersection_canonicalization_header header() const {
    intersection_canonicalization_header value;
    value.owner = relations_->owner();
    value.operation = relations_->operation();
    value.context_digest = relations_->context_digest();
    value.precision_digest = relations_->precision_digest();
    value.relation_digest = relations_->digest();
    value.source_semantic_digests[0] =
        relations_->candidates()
            ->primitive_table(operand_id::a)
            .source_semantic_digest;
    value.source_semantic_digests[1] =
        relations_->candidates()
            ->primitive_table(operand_id::b)
            .source_semantic_digest;
    value.exact_triangulation_digests[0] =
        relations_->candidates()
            ->primitive_table(operand_id::a)
            .exact_topology_digest;
    value.exact_triangulation_digests[1] =
        relations_->candidates()
            ->primitive_table(operand_id::b)
            .exact_topology_digest;
    return value;
  }

  bool canonicalize() {
    if (!check_cancel(intersection_checkpoint::canonical_remap))
      return false;
    artifact_ = std::make_unique<artifact_type>();
    const auto canonical_header = header();
    if (!canonicalize_intersection_tables(
            canonical_header, interning_, coordinates_, incidence_,
            source_edges_, transverse_, coplanar_, aggregates_, descriptors_,
            *artifact_, error_))
      return false;
    if (!check_cancel(intersection_checkpoint::producer_invariants) ||
        !verify_intersection_canonicalization(
            canonical_header, interning_, coordinates_, incidence_,
            source_edges_, transverse_, coplanar_, aggregates_, descriptors_,
            *artifact_, error_))
      return false;
    return true;
  }

  std::uint64_t verifier_work_bound() const noexcept {
    const auto &s = artifact_->statistics();
    std::uint64_t records = 1024;
    const std::uint64_t values[] = {
        s.seed_count,
        s.event_count,
        s.occurrence_count,
        s.seed_binding_count,
        s.incidence_count,
        s.source_edge_membership_count,
        s.source_edge_sequence_count,
        s.source_edge_cluster_count,
        s.source_edge_interval_count,
        s.transverse_carrier_count,
        s.carrier_membership_count,
        s.carrier_cluster_count,
        s.carrier_span_count,
        s.coplanar_support_count,
        s.overlap_count,
        s.aggregate_count,
        s.descriptor_count,
        s.ordering_certificate_count,
        static_cast<std::uint64_t>(artifact_->canonical_bytes().size())};
    for (const auto value : values)
      records = intersection_build_detail::saturating_add(records, value);
    const auto membership_count = intersection_build_detail::saturating_add(
        s.source_edge_membership_count, s.carrier_membership_count);
    const auto quadratic = intersection_build_detail::saturating_multiply(
        membership_count, membership_count);
    return intersection_build_detail::saturating_add(
        intersection_build_detail::saturating_multiply(records, 32),
        quadratic);
  }

  bool encode_and_verify() {
    if (!check_cancel(intersection_checkpoint::canonical_encoding) ||
        !refresh_intersection_codec(*artifact_, codec_limits_, error_))
      return false;
    const auto work_bound = verifier_work_bound();
    if (work_bound > verifier_limits_.maximum_work_units)
      return fail(intersection_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 08 verifier work limit exceeded",
                  intersection_checkpoint::independent_verification);
    if (!reserve(resource_kind::intersection_verifier_work, work_bound, 0))
      return false;
    if (!check_cancel(intersection_checkpoint::independent_verification) ||
        !finalize_intersection_complex_verification(
            *relations_, *artifact_, codec_limits_, verifier_limits_, error_))
      return false;
    return true;
  }

  bool reconcile_resources() {
    if (!check_cancel(intersection_checkpoint::resource_reconciliation))
      return false;
    const auto &s = artifact_->statistics();
    const auto persistent = s.persistent_bytes;
    const auto canonical =
        static_cast<std::uint64_t>(artifact_->canonical_bytes().size());
    if (canonical > capabilities_.maximum_canonical_bytes ||
        !reserve(resource_kind::persistent_bytes, persistent) ||
        !reserve(resource_kind::replay_bytes, canonical))
      return false;

    for (auto &record : reservations_) {
      switch (record.kind) {
      case resource_kind::events:
        record.used = s.event_count;
        break;
      case resource_kind::intersection_occurrences:
        record.used = s.occurrence_count;
        break;
      case resource_kind::intersection_incidence:
        record.used = s.incidence_count;
        break;
      case resource_kind::intersection_memberships:
        record.used = s.source_edge_membership_count +
                      s.carrier_membership_count;
        break;
      case resource_kind::intersection_clusters:
        record.used = s.source_edge_cluster_count + s.carrier_cluster_count;
        break;
      case resource_kind::intersection_intervals:
        record.used = s.source_edge_interval_count + s.carrier_span_count;
        break;
      case resource_kind::intersection_carriers:
        record.used = s.transverse_carrier_count + s.coplanar_support_count;
        break;
      case resource_kind::intersection_overlaps:
        record.used = s.overlap_count;
        break;
      case resource_kind::intersection_aggregates:
        record.used = s.aggregate_count;
        break;
      case resource_kind::intersection_descriptors:
        record.used = s.descriptor_count;
        break;
      case resource_kind::intersection_order_certificates:
        record.used = s.ordering_certificate_count;
        break;
      case resource_kind::intersection_verifier_work:
        record.used = s.verifier_work_units;
        break;
      case resource_kind::work_units:
        record.used = std::min(record.reservation->amount(),
                               s.verifier_work_units + s.sort_comparisons +
                                   s.incidence_count + s.descriptor_count);
        break;
      default:
        break;
      }
      if (record.reservation && record.used > record.reservation->amount())
        return fail(intersection_subcode::resource_reconciliation_failed,
                    bounded_boolean_error_category::internal_invariant_error,
                    "Component 08 actual resource use exceeds reservation",
                    intersection_checkpoint::resource_reconciliation);
    }
    return true;
  }

  bool commit_resources() {
    for (auto &record : reservations_) {
      if (!record.reservation ||
          !record.reservation->commit(record.used))
        return false;
    }
    return true;
  }

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const signed_feature_relations<T, I>> relations_;
  intersection_capabilities capabilities_;
  intersection_codec_limits codec_limits_;
  intersection_verifier_limits verifier_limits_;
  intersection_preflight_plan preflight_{};
  std::vector<source_edge_domain_record> domains_{};
  std::vector<normalized_event_seed_proposal> proposals_{};
  event_interning_tables interning_{};
  event_coordinate_tables coordinates_{};
  event_incidence_tables incidence_{};
  std::vector<source_edge_membership_proposal> source_memberships_{};
  source_edge_arrangement_tables source_edges_{};
  transverse_carrier_arrangement_tables transverse_{};
  coplanar_carrier_arrangement_tables coplanar_{};
  intersection_aggregate_tables aggregates_{};
  intersection_descriptor_tables base_descriptors_{};
  intersection_descriptor_tables descriptors_{};
  std::unique_ptr<artifact_type> artifact_{};
  stage_transaction transaction_{};
  std::vector<intersection_build_detail::reservation_record> reservations_{};
  bounded_boolean_error error_{};
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_intersection_complex<T, I>>>
build_canonical_intersection_complex(
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    intersection_capabilities capabilities,
    intersection_codec_limits codec_limits,
    intersection_verifier_limits verifier_limits) {
  return intersection_builder<T, I>(
             context, precision, std::move(relations), std::move(capabilities),
             codec_limits, verifier_limits)
      .run();
}

#define YGOR_INSTANTIATE_INTERSECTION_BUILD(T, I)                           \
  template boolean_outcome<                                                 \
      std::shared_ptr<const canonical_intersection_complex<T, I>>>          \
  build_canonical_intersection_complex<T, I>(                               \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      intersection_capabilities, intersection_codec_limits,                 \
      intersection_verifier_limits)

YGOR_INSTANTIATE_INTERSECTION_BUILD(float, std::uint32_t);
YGOR_INSTANTIATE_INTERSECTION_BUILD(float, std::uint64_t);
YGOR_INSTANTIATE_INTERSECTION_BUILD(double, std::uint32_t);
YGOR_INSTANTIATE_INTERSECTION_BUILD(double, std::uint64_t);

#undef YGOR_INSTANTIATE_INTERSECTION_BUILD

} // namespace ygor::mesh_boolean::bounded
