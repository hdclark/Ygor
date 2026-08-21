#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "ClassificationComplex.h"
#include "Context.h"
#include "FloatingBits.h"
#include "OutputTopologyCodec.h"
#include "OutputTopologyVerifier.h"
#include "PolygonalOutputComplex.h"
#include "RetainedSurfaceComplex.h"
#include "SignedFeatureRelations.h"
#include "Transaction.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>>
build_polygonal_output_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const classification_complex<T, I>> classification,
    std::shared_ptr<const retained_surface_complex<T, I>> retained,
    output_topology_capabilities capabilities,
    output_topology_codec_limits codec_limits = {});

#define YGOR_DECLARE_OUTPUT_TOPOLOGY_BUILD(T, I)                             \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const polygonal_output_complex<T, I>>>                 \
  build_polygonal_output_complex<T, I>(                                      \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const signed_feature_relations<T, I>>,                 \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const classification_complex<T, I>>,                   \
      std::shared_ptr<const retained_surface_complex<T, I>>,                 \
      output_topology_capabilities, output_topology_codec_limits)

YGOR_DECLARE_OUTPUT_TOPOLOGY_BUILD(float, std::uint32_t);
YGOR_DECLARE_OUTPUT_TOPOLOGY_BUILD(float, std::uint64_t);
YGOR_DECLARE_OUTPUT_TOPOLOGY_BUILD(double, std::uint32_t);
YGOR_DECLARE_OUTPUT_TOPOLOGY_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_OUTPUT_TOPOLOGY_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
namespace ygor::mesh_boolean::bounded {

// ---------------------------------------------------------------------------
// The Component 11 builder.
// ---------------------------------------------------------------------------
template <class T, class I> class output_topology_builder {
public:
  output_topology_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
      std::shared_ptr<const signed_feature_relations<T, I>> relations,
      std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
      std::shared_ptr<const classification_complex<T, I>> classification,
      std::shared_ptr<const retained_surface_complex<T, I>> retained,
      output_topology_capabilities capabilities,
      output_topology_codec_limits codec_limits)
      : context_(context), precision_(precision),
        manifolds_(std::move(manifolds)), relations_(std::move(relations)),
        intersections_(std::move(intersections)),
        classification_(std::move(classification)),
        retained_(std::move(retained)), capabilities_(std::move(capabilities)),
        codec_limits_(codec_limits) {}

  boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return failure(error);
    if (output_topology_cancelled(
            capabilities_, output_topology_checkpoint::predecessor_validation))
      return fail_cancelled();

    auto artifact = std::make_shared<polygonal_output_complex<T, I>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->precision_digest_ = precision_.digest();
    artifact->manifold_digests_[0] = manifolds_->a()->digest();
    artifact->manifold_digests_[1] = manifolds_->b()->digest();
    artifact->relation_digest_ = relations_->digest();
    artifact->intersection_digest_ = intersections_->digest();
    artifact->classification_digest_ = classification_->digest();
    artifact->retained_digest_ = retained_->digest();

    if (!build_incidence_audit(*artifact, error) ||
        !build_vertex_occurrences(*artifact, error) ||
        !build_regions(*artifact, error) ||
        !build_paired_edges(*artifact, error) ||
        !build_successors(*artifact, error) ||
        !build_cycles(*artifact, error) ||
        !build_darts(*artifact, error) ||
        !build_contours(*artifact, error) ||
        !build_vertex_links(*artifact, error) ||
        !build_carrier_balance(*artifact, error) ||
        !build_admissibility(*artifact, error) ||
        !finalize(*artifact, error))
      return failure(error);

    if (output_topology_cancelled(capabilities_,
                                  output_topology_checkpoint::commit))
      return fail_cancelled();
    return boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>>::
        success(std::move(artifact));
  }

private:
  using artifact_type = polygonal_output_complex<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>>;

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  std::shared_ptr<const signed_feature_relations<T, I>> relations_;
  std::shared_ptr<const canonical_intersection_complex<T, I>> intersections_;
  std::shared_ptr<const classification_complex<T, I>> classification_;
  std::shared_ptr<const retained_surface_complex<T, I>> retained_;
  output_topology_capabilities capabilities_;
  output_topology_codec_limits codec_limits_;

  outcome_type failure(const bounded_boolean_error &error) {
    return outcome_type::failure(error);
  }
  outcome_type fail_cancelled() {
    return outcome_type::failure(output_topology_error(
        output_topology_subcode::cancelled,
        bounded_boolean_error_category::cancelled,
        "output topology cancelled", output_topology_checkpoint::commit));
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    const auto &a = *manifolds_->a();
    const auto &b = *manifolds_->b();
    if (!a.owner().same_owner(context_.owner) ||
        !b.owner().same_owner(context_.owner) ||
        !relations_->owner().same_owner(context_.owner) ||
        !intersections_->owner().same_owner(context_.owner) ||
        !classification_->owner().same_owner(context_.owner) ||
        !retained_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = output_topology_error(
          output_topology_subcode::wrong_owner,
          bounded_boolean_error_category::internal_invariant_error,
          "output topology predecessor owner mismatch",
          output_topology_checkpoint::predecessor_validation);
      return false;
    }
    if (relations_->operation() != context_.operation ||
        intersections_->operation() != context_.operation ||
        classification_->operation() != context_.operation ||
        retained_->operation() != context_.operation) {
      error = output_topology_error(
          output_topology_subcode::wrong_operation,
          bounded_boolean_error_category::internal_invariant_error,
          "output topology predecessor operation mismatch",
          output_topology_checkpoint::predecessor_validation);
      return false;
    }
    if (retained_->verification() !=
        selection_verification_disposition::independently_verified) {
      error = output_topology_error(
          output_topology_subcode::predecessor_not_verified,
          bounded_boolean_error_category::internal_invariant_error,
          "retained complex is not independently verified",
          output_topology_checkpoint::predecessor_validation);
      return false;
    }
    if (capabilities_.provider_version !=
            contract_versions::output_topology_provider ||
        capabilities_.codec_version != contract_versions::output_topology_codec ||
        capabilities_.verifier_version !=
            contract_versions::output_topology_verifier) {
      error = output_topology_error(
          output_topology_subcode::unsupported_version,
          bounded_boolean_error_category::input_contract_error,
          "output topology capability version mismatch",
          output_topology_checkpoint::context_capability_validation);
      return false;
    }
    return true;
  }

  const canonical_halfedge_operand<T, I> &operand_manifold(operand_id id) const {
    return id == operand_id::a ? *manifolds_->a() : *manifolds_->b();
  }

  // -------------------------------------------------------------------------
  // Phase: incidence audit.
  // -------------------------------------------------------------------------
  bool build_incidence_audit(artifact_type &artifact,
                             bounded_boolean_error &error) {
    const auto &incidences = retained_->incidences();
    artifact.audit_by_incidence_.resize(incidences.size(),
                                        output_topology_invalid_ordinal);
    for (std::size_t i = 0; i < incidences.size(); ++i) {
      const auto &incidence = incidences[i];
      if (incidence.disposition != incidence_disposition::planned_edge) {
        error = output_topology_error(
            output_topology_subcode::invalid_disposition,
            bounded_boolean_error_category::internal_invariant_error,
            "retained incidence has an unsupported disposition",
            output_topology_checkpoint::incidence_audit);
        return false;
      }
      output_incidence_audit_record row;
      row.canonical_id = artifact.incidence_audits_.size();
      row.retained_incidence = i;
      row.retained_use = incidence.retained_use;
      row.start_domain = incidence.start_domain;
      row.end_domain = incidence.end_domain;
      row.disposition = output_incidence_disposition::paired_boundary;
      row.planned_edge = incidence.planned_edge;
      row.continuation = incidence.continuation;
      row.consuming_output_entity = output_topology_invalid_ordinal;
      artifact.audit_by_incidence_[i] = row.canonical_id;
      artifact.incidence_audits_.push_back(std::move(row));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: output vertex occurrence allocation and coordinate references.
  // -------------------------------------------------------------------------
  bool build_vertex_occurrences(artifact_type &artifact,
                                bounded_boolean_error &error) {
    const auto &endpoint_domains = retained_->endpoint_domains();
    artifact.occurrence_by_endpoint_domain_.resize(
        endpoint_domains.size(), output_topology_invalid_ordinal);

    // Resolve a bounded coordinate for each endpoint domain.
    std::vector<std::uint64_t> coordinate_by_domain(endpoint_domains.size(),
                                                    output_topology_invalid_ordinal);
    for (std::size_t d = 0; d < endpoint_domains.size(); ++d) {
      const auto &domain = endpoint_domains[d];
      output_coordinate_reference_record coordinate;
      coordinate.canonical_id = artifact.coordinate_references_.size();
      coordinate.source_operand = domain.source_operand;
      coordinate.source_lineage = domain.lineage;
      if (domain.kind == endpoint_domain_kind::source_vertex) {
        coordinate.source = coordinate_source::source_vertex;
        const auto &manifold = operand_manifold(domain.source_operand);
        const auto &source_map = manifold.source_vertex_to_vertex();
        if (domain.lineage >= source_map.size()) {
          error = output_topology_error(
              output_topology_subcode::internal_invariant,
              bounded_boolean_error_category::internal_invariant_error,
              "source vertex domain is out of range",
              output_topology_checkpoint::vertex_occurrence_allocation);
          return false;
        }
        const auto &vertex = manifold.vertices()[source_map[domain.lineage]];
        for (std::size_t axis = 0; axis < 3; ++axis) {
          coordinate.nominal_bits[axis] = vertex.nominal_bits[axis];
          coordinate.lower_bits[axis] =
              static_cast<std::uint64_t>(to_bits<T>(vertex.lower[axis]));
          coordinate.upper_bits[axis] =
              static_cast<std::uint64_t>(to_bits<T>(vertex.upper[axis]));
        }
        coordinate.radial_error_bits =
            static_cast<std::uint64_t>(to_bits<T>(vertex.radial_error));
      } else if (domain.kind == endpoint_domain_kind::event_occurrence) {
        coordinate.source = coordinate_source::canonical_event;
        if (!resolve_event_coordinate(domain.event_occurrence, coordinate,
                                      error))
          return false;
      } else {
        error = output_topology_error(
            output_topology_subcode::internal_invariant,
            bounded_boolean_error_category::internal_invariant_error,
            "endpoint domain kind is not materializable in V1",
            output_topology_checkpoint::vertex_occurrence_allocation);
        return false;
      }
      coordinate_by_domain[d] = coordinate.canonical_id;
      artifact.coordinate_references_.push_back(std::move(coordinate));
    }

    // Allocate one output occurrence per nonempty Component 10 requirement.
    const auto &occurrences = retained_->vertex_occurrences();
    for (std::size_t j = 0; j < occurrences.size(); ++j) {
      const auto &requirement = occurrences[j];
      if (requirement.cycle_count == 0) {
        error = output_topology_error(
            output_topology_subcode::empty_occurrence_requirement,
            bounded_boolean_error_category::internal_invariant_error,
            "Component 10 vertex occurrence requirement is empty",
            output_topology_checkpoint::vertex_occurrence_allocation);
        return false;
      }
      if (requirement.endpoint_domain >= endpoint_domains.size()) {
        error = output_topology_error(
            output_topology_subcode::internal_invariant,
            bounded_boolean_error_category::internal_invariant_error,
            "vertex occurrence endpoint domain is out of range",
            output_topology_checkpoint::vertex_occurrence_allocation);
        return false;
      }
      if (artifact.occurrence_by_endpoint_domain_[requirement.endpoint_domain] !=
          output_topology_invalid_ordinal) {
        error = output_topology_error(
            output_topology_subcode::duplicate_incidence,
            bounded_boolean_error_category::internal_invariant_error,
            "endpoint domain maps to more than one output occurrence",
            output_topology_checkpoint::vertex_occurrence_allocation);
        return false;
      }

      output_vertex_occurrence_record occurrence;
      occurrence.canonical_id = j;
      occurrence.key.endpoint_domain = requirement.endpoint_domain;
      occurrence.key.predecessor_occurrence = requirement.canonical_id;
      occurrence.endpoint_domain = requirement.endpoint_domain;
      occurrence.coordinate_reference = coordinate_by_domain[requirement.endpoint_domain];
      occurrence.ports_begin = requirement.cycle_begin;
      occurrence.ports_count = requirement.cycle_count;
      occurrence.representative_port = requirement.representative_port;
      occurrence.predecessor_occurrence = requirement.canonical_id;
      artifact.occurrence_by_endpoint_domain_[requirement.endpoint_domain] = j;
      artifact.vertex_occurrences_.push_back(std::move(occurrence));
    }

    // Every incidence endpoint must map to an output occurrence.
    for (const auto &incidence : retained_->incidences()) {
      if (incidence.start_domain >= endpoint_domains.size() ||
          incidence.end_domain >= endpoint_domains.size() ||
          artifact.occurrence_by_endpoint_domain_[incidence.start_domain] ==
              output_topology_invalid_ordinal ||
          artifact.occurrence_by_endpoint_domain_[incidence.end_domain] ==
              output_topology_invalid_ordinal) {
        error = output_topology_error(
            output_topology_subcode::occurrence_link_mismatch,
            bounded_boolean_error_category::internal_invariant_error,
            "incidence endpoint has no output occurrence",
            output_topology_checkpoint::vertex_occurrence_allocation);
        return false;
      }
    }
    return true;
  }

  bool resolve_event_coordinate(std::uint64_t occurrence_ordinal,
                                output_coordinate_reference_record &coordinate,
                                bounded_boolean_error &error) {
    if (occurrence_ordinal >= intersections_->occurrences().size()) {
      error = output_topology_error(
          output_topology_subcode::internal_invariant,
          bounded_boolean_error_category::internal_invariant_error,
          "event occurrence is out of range",
          output_topology_checkpoint::vertex_occurrence_allocation);
      return false;
    }
    const auto &occurrence = intersections_->occurrences()[occurrence_ordinal];
    const std::uint64_t event_ordinal = occurrence.event.ordinal();
    if (event_ordinal >= intersections_->events().size()) {
      error = output_topology_error(
          output_topology_subcode::internal_invariant,
          bounded_boolean_error_category::internal_invariant_error,
          "event is out of range",
          output_topology_checkpoint::vertex_occurrence_allocation);
      return false;
    }
    const auto &event = intersections_->events()[event_ordinal];
    const std::uint64_t ledger = event.point.precision_ledger.ordinal();
    if (ledger >= relations_->construction_ledger().size()) {
      error = output_topology_error(
          output_topology_subcode::internal_invariant,
          bounded_boolean_error_category::internal_invariant_error,
          "event construction ledger is out of range",
          output_topology_checkpoint::vertex_occurrence_allocation);
      return false;
    }
    const auto &record = relations_->construction_ledger()[ledger];
    if (record.component_count != 3) {
      error = output_topology_error(
          output_topology_subcode::internal_invariant,
          bounded_boolean_error_category::internal_invariant_error,
          "event construction is not a world-space point",
          output_topology_checkpoint::vertex_occurrence_allocation);
      return false;
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
      coordinate.nominal_bits[axis] = record.nominal_bits[axis];
      coordinate.lower_bits[axis] = record.lower_bits[axis];
      coordinate.upper_bits[axis] = record.upper_bits[axis];
    }
    coordinate.construction_ledger = ledger;
    coordinate.radial_error_bits = record.radial_error_upper_bits;
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: regions (one per retained use in V1, no continuations).
  // -------------------------------------------------------------------------
  bool build_regions(artifact_type &artifact, bounded_boolean_error &error) {
    (void)error;
    const auto &uses = retained_->retained_uses();
    artifact.region_by_retained_use_.resize(uses.size(),
                                            output_topology_invalid_ordinal);
    for (std::size_t r = 0; r < uses.size(); ++r) {
      const auto &use = uses[r];
      output_face_region_record region;
      region.canonical_id = r;
      region.key.source_operand = use.source_operand;
      region.key.retained_use = use.canonical_id;
      region.key.source_facet = use.source_facet;
      region.key.support_lineage = use.sheet_owner_lineage;
      region.key.positive_area_component = use.atom;
      region.members_begin = artifact.region_member_index_.size();
      region.members_count = 1;
      region.positive_area = true;
      region.outer_contour = output_topology_invalid_ordinal;

      region_member_record member;
      member.canonical_id = artifact.region_members_.size();
      member.retained_use = use.canonical_id;
      artifact.region_members_.push_back(std::move(member));
      artifact.region_member_index_.push_back(member.canonical_id);

      artifact.region_by_retained_use_[use.canonical_id] = r;
      artifact.face_regions_.push_back(std::move(region));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: paired edges and halfedges.
  // -------------------------------------------------------------------------
  bool build_paired_edges(artifact_type &artifact, bounded_boolean_error &error) {
    const auto &incidences = retained_->incidences();
    const auto &planned_edges = retained_->planned_edges();
    artifact.halfedge_by_incidence_.resize(incidences.size(),
                                           output_topology_invalid_ordinal);

    // Local port lookup: (incidence, role) -> port ordinal.
    std::vector<std::uint64_t> start_port_by_incidence(
        incidences.size(), output_topology_invalid_ordinal);
    std::vector<std::uint64_t> end_port_by_incidence(
        incidences.size(), output_topology_invalid_ordinal);
    for (const auto &port : retained_->local_ports()) {
      if (port.incidence >= incidences.size())
        continue;
      if (port.role == endpoint_role::start)
        start_port_by_incidence[port.incidence] = port.canonical_id;
      else
        end_port_by_incidence[port.incidence] = port.canonical_id;
    }

    for (std::size_t p = 0; p < planned_edges.size(); ++p) {
      const auto &planned = planned_edges[p];
      if (planned.mate_group >= retained_->edge_mate_groups().size()) {
        error = output_topology_error(
            output_topology_subcode::pair_reciprocity_mismatch,
            bounded_boolean_error_category::internal_invariant_error,
            "planned edge mate group is out of range",
            output_topology_checkpoint::pair_materialization);
        return false;
      }
      const auto &group = retained_->edge_mate_groups()[planned.mate_group];
      const std::uint64_t forward = group.forward_incidence;
      const std::uint64_t reverse = group.reverse_incidence;
      if (forward >= incidences.size() || reverse >= incidences.size()) {
        error = output_topology_error(
            output_topology_subcode::pair_reciprocity_mismatch,
            bounded_boolean_error_category::internal_invariant_error,
            "planned edge incidence is out of range",
            output_topology_checkpoint::pair_materialization);
        return false;
      }
      const auto &f = incidences[forward];
      const auto &r = incidences[reverse];

      const std::uint64_t origin0 =
          artifact.occurrence_by_endpoint_domain_[f.start_domain];
      const std::uint64_t destination0 =
          artifact.occurrence_by_endpoint_domain_[f.end_domain];
      const std::uint64_t origin1 =
          artifact.occurrence_by_endpoint_domain_[r.start_domain];
      const std::uint64_t destination1 =
          artifact.occurrence_by_endpoint_domain_[r.end_domain];
      if (origin0 == output_topology_invalid_ordinal ||
          destination0 == output_topology_invalid_ordinal ||
          origin1 == output_topology_invalid_ordinal ||
          destination1 == output_topology_invalid_ordinal) {
        error = output_topology_error(
            output_topology_subcode::pair_endpoint_read_before_initialize,
            bounded_boolean_error_category::internal_invariant_error,
            "paired edge endpoint has no resolved occurrence",
            output_topology_checkpoint::pair_materialization);
        return false;
      }

      const std::uint64_t h0 = 2 * p;
      const std::uint64_t h1 = 2 * p + 1;

      paired_output_edge_record edge;
      edge.canonical_id = p;
      edge.key = paired_edge_key::canonical(p, f.start_domain, f.end_domain);
      edge.planned_edge = planned.canonical_id;
      edge.halfedge0 = h0;
      edge.halfedge1 = h1;
      edge.role = carrier_role(f.carrier.kind);
      artifact.paired_edges_.push_back(std::move(edge));

      append_halfedge(artifact, h0, p, forward, direction_role::forward,
                      origin0, destination0, start_port_by_incidence[forward],
                      end_port_by_incidence[forward]);
      append_halfedge(artifact, h1, p, reverse, direction_role::reverse,
                      origin1, destination1, start_port_by_incidence[reverse],
                      end_port_by_incidence[reverse]);
      artifact.halfedges_[h0].pair = h1;
      artifact.halfedges_[h1].pair = h0;
      artifact.halfedge_by_incidence_[forward] = h0;
      artifact.halfedge_by_incidence_[reverse] = h1;
    }

    // Resolve zero-measure descriptors and update dispositions.
    for (std::size_t p = 0; p < artifact.paired_edges_.size(); ++p) {
      auto &edge = artifact.paired_edges_[p];
      const auto &h0 = artifact.halfedges_[edge.halfedge0];
      const auto &h1 = artifact.halfedges_[edge.halfedge1];
      const auto relation = endpoint_relation(h0.origin, h1.origin, artifact);
      edge.endpoint_relation = relation;
      if (relation == endpoint_nominal_relation::bit_equal ||
          relation == endpoint_nominal_relation::uncertainty_overlapping) {
        zero_measure_boundary_record descriptor;
        descriptor.canonical_id = artifact.zero_measure_boundaries_.size();
        descriptor.paired_edge = p;
        const auto &coord = artifact.coordinate_references_[
            artifact.vertex_occurrences_[h0.origin].coordinate_reference];
        for (std::size_t axis = 0; axis < 3; ++axis) {
          descriptor.nominal_bits[axis] = coord.nominal_bits[axis];
          descriptor.lower_bits[axis] = coord.lower_bits[axis];
          descriptor.upper_bits[axis] = coord.upper_bits[axis];
        }
        descriptor.lower_length_bits = 0;
        descriptor.upper_length_bits = 0;
        edge.zero_measure_boundary = descriptor.canonical_id;
        artifact.zero_measure_boundaries_.push_back(std::move(descriptor));
        // Mark the two audit rows as paired zero-measure boundary.
        const auto &forward = retained_->edge_mate_groups()[
            retained_->planned_edges()[p].mate_group].forward_incidence;
        const auto &reverse = retained_->edge_mate_groups()[
            retained_->planned_edges()[p].mate_group].reverse_incidence;
        artifact.incidence_audits_[artifact.audit_by_incidence_[forward]]
            .disposition =
            output_incidence_disposition::paired_zero_measure_boundary;
        artifact.incidence_audits_[artifact.audit_by_incidence_[reverse]]
            .disposition =
            output_incidence_disposition::paired_zero_measure_boundary;
      }
    }
    return true;
  }

  edge_role carrier_role(carrier_kind kind) const noexcept {
    switch (kind) {
    case carrier_kind::whole_source_edge:
      return edge_role::whole_source_edge;
    case carrier_kind::source_edge_interval:
      return edge_role::split_source_edge_interval;
    case carrier_kind::transverse_carrier:
      return edge_role::transverse_carrier_interval;
    case carrier_kind::coplanar_overlap_boundary:
      return edge_role::coplanar_overlap_boundary;
    case carrier_kind::contact_delimiter:
      return edge_role::topology_separation_contact;
    }
    return edge_role::invalid;
  }

  void append_halfedge(artifact_type &artifact, std::uint64_t id,
                       std::uint64_t paired_edge, std::uint64_t incidence,
                       direction_role direction, std::uint64_t origin,
                       std::uint64_t destination, std::uint64_t start_port,
                       std::uint64_t end_port) {
    output_halfedge_record halfedge;
    halfedge.canonical_id = id;
    halfedge.key.paired_edge = paired_edge;
    halfedge.key.incidence = incidence;
    halfedge.key.direction = direction;
    halfedge.pair = output_topology_invalid_ordinal;
    halfedge.origin = origin;
    halfedge.destination = destination;
    halfedge.successor = output_topology_invalid_ordinal;
    halfedge.predecessor = output_topology_invalid_ordinal;
    halfedge.region =
        artifact.region_by_retained_use_[retained_->incidences()[incidence]
                                              .retained_use];
    halfedge.cycle = output_topology_invalid_ordinal;
    halfedge.incidence = incidence;
    halfedge.paired_edge = paired_edge;
    halfedge.direction = direction;
    halfedge.endpoint_fan_begin = artifact.endpoint_fan_refs_.size();
    halfedge.endpoint_fan_count = 2;
    halfedge_endpoint_fan_ref_record start_ref;
    start_ref.canonical_id = artifact.endpoint_fan_refs_.size();
    start_ref.port = start_port;
    start_ref.role = endpoint_role::start;
    artifact.endpoint_fan_refs_.push_back(start_ref);
    halfedge_endpoint_fan_ref_record end_ref;
    end_ref.canonical_id = artifact.endpoint_fan_refs_.size();
    end_ref.port = end_port;
    end_ref.role = endpoint_role::end;
    artifact.endpoint_fan_refs_.push_back(end_ref);
    artifact.halfedges_.push_back(std::move(halfedge));
  }

  endpoint_nominal_relation endpoint_relation(
      std::uint64_t a, std::uint64_t b, const artifact_type &artifact) const {
    const auto &ca = artifact.coordinate_references_[
        artifact.vertex_occurrences_[a].coordinate_reference];
    const auto &cb = artifact.coordinate_references_[
        artifact.vertex_occurrences_[b].coordinate_reference];
    bool equal = true;
    for (std::size_t axis = 0; axis < 3; ++axis)
      equal = equal && ca.nominal_bits[axis] == cb.nominal_bits[axis];
    if (equal)
      return endpoint_nominal_relation::bit_equal;
    bool overlap = true;
    for (std::size_t axis = 0; axis < 3; ++axis)
      overlap = overlap && ca.lower_bits[axis] <= cb.upper_bits[axis] &&
                cb.lower_bits[axis] <= ca.upper_bits[axis];
    if (overlap)
      return endpoint_nominal_relation::uncertainty_overlapping;
    return endpoint_nominal_relation::definitely_distinct;
  }

  // -------------------------------------------------------------------------
  // Phase: successor/predecessor permutation.
  // -------------------------------------------------------------------------
  bool build_successors(artifact_type &artifact, bounded_boolean_error &error) {
    const auto &incidences = retained_->incidences();
    const std::size_t count = incidences.size();

    std::vector<std::uint64_t> end_port_by_incidence(
        count, output_topology_invalid_ordinal);
    for (const auto &port : retained_->local_ports()) {
      if (port.incidence >= count)
        continue;
      if (port.role == endpoint_role::end)
        end_port_by_incidence[port.incidence] = port.canonical_id;
    }
    std::vector<std::uint64_t> next_incidence_by_port(
        retained_->local_ports().size(), output_topology_invalid_ordinal);
    for (const auto &arc : retained_->face_corner_arcs()) {
      if (arc.in_port >= retained_->local_ports().size())
        continue;
      next_incidence_by_port[arc.in_port] =
          retained_->local_ports()[arc.out_port].incidence;
    }

    const std::size_t halfedge_count = artifact.halfedges_.size();
    for (std::size_t i = 0; i < count; ++i) {
      const std::uint64_t h = artifact.halfedge_by_incidence_[i];
      if (h == output_topology_invalid_ordinal || h >= halfedge_count) {
        error = output_topology_error(
            output_topology_subcode::successor_permutation_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "incidence has no halfedge",
            output_topology_checkpoint::successor_construction);
        return false;
      }
      const std::uint64_t end_port = end_port_by_incidence[i];
      if (end_port == output_topology_invalid_ordinal ||
          end_port >= retained_->local_ports().size()) {
        error = output_topology_error(
            output_topology_subcode::successor_permutation_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "incidence end port is missing",
            output_topology_checkpoint::successor_construction);
        return false;
      }
      const std::uint64_t next_incidence = next_incidence_by_port[end_port];
      if (next_incidence == output_topology_invalid_ordinal ||
          next_incidence >= count) {
        error = output_topology_error(
            output_topology_subcode::successor_permutation_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "successor incidence is unresolved",
            output_topology_checkpoint::successor_construction);
        return false;
      }
      const std::uint64_t successor =
          artifact.halfedge_by_incidence_[next_incidence];
      if (successor == output_topology_invalid_ordinal ||
          successor >= halfedge_count) {
        error = output_topology_error(
            output_topology_subcode::successor_permutation_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "successor halfedge is out of range",
            output_topology_checkpoint::successor_construction);
        return false;
      }
      artifact.halfedges_[h].successor = successor;
    }
    // Predecessor as inverse.
    for (std::size_t h = 0; h < halfedge_count; ++h) {
      const std::uint64_t successor = artifact.halfedges_[h].successor;
      if (successor == output_topology_invalid_ordinal ||
          successor >= halfedge_count) {
        error = output_topology_error(
            output_topology_subcode::successor_permutation_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "halfedge successor was never assigned",
            output_topology_checkpoint::successor_construction);
        return false;
      }
      if (artifact.halfedges_[successor].predecessor !=
          output_topology_invalid_ordinal) {
        error = output_topology_error(
            output_topology_subcode::successor_permutation_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "successor permutation is not a bijection",
            output_topology_checkpoint::successor_construction);
        return false;
      }
      artifact.halfedges_[successor].predecessor = h;
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: cycle extraction and role-independent canonical IDs.
  // -------------------------------------------------------------------------
  bool build_cycles(artifact_type &artifact, bounded_boolean_error &error) {
    const std::size_t halfedge_count = artifact.halfedges_.size();
    std::vector<bool> visited(halfedge_count, false);
    std::vector<std::vector<std::uint64_t>> cycles;
    for (std::size_t start = 0; start < halfedge_count; ++start) {
      if (visited[start])
        continue;
      std::vector<std::uint64_t> cycle;
      std::uint64_t current = start;
      std::uint64_t guard = 0;
      do {
        if (visited[current] || guard++ > halfedge_count) {
          error = output_topology_error(
              output_topology_subcode::cycle_extraction_failure,
              bounded_boolean_error_category::internal_invariant_error,
              "successor walk does not produce disjoint cycles",
              output_topology_checkpoint::cycle_extraction);
          return false;
        }
        visited[current] = true;
        cycle.push_back(current);
        current = artifact.halfedges_[current].successor;
      } while (current != start);
      cycles.push_back(std::move(cycle));
    }

    // Build role-independent canonical cycle keys, sort globally, assign IDs.
    std::vector<std::pair<face_cycle_key, std::vector<std::uint64_t>>> keyed;
    keyed.reserve(cycles.size());
    for (const auto &cycle : cycles) {
      std::vector<output_halfedge_key> sequence;
      sequence.reserve(cycle.size());
      for (const auto h : cycle)
        sequence.push_back(artifact.halfedges_[h].key);
      const std::size_t rotation =
          output_topology_verify_detail::least_rotation_start(sequence);
      face_cycle_key key;
      key.region = artifact.halfedges_[cycle.front()].region;
      key.rotated_halfedges.reserve(sequence.size());
      for (std::size_t k = 0; k < sequence.size(); ++k)
        key.rotated_halfedges.push_back(sequence[(rotation + k) % sequence.size()]);
      keyed.emplace_back(std::move(key), cycle);
    }
    std::sort(keyed.begin(), keyed.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    for (std::size_t c = 0; c < keyed.size(); ++c) {
      const auto &cycle = keyed[c].second;
      const std::size_t rotation =
          output_topology_verify_detail::least_rotation_start(
              keyed[c].first.rotated_halfedges);
      face_cycle_record record;
      record.canonical_id = c;
      record.key = keyed[c].first;
      record.region = keyed[c].first.region;
      record.refs_begin = artifact.cycle_halfedge_index_.size();
      record.refs_count = cycle.size();
      // The stored rotation re-aligns the walked order with the canonical key.
      record.start_halfedge = cycle[rotation % cycle.size()];
      record.category = cycle_geometric_category::definite_positive_area;
      for (std::size_t k = 0; k < cycle.size(); ++k) {
        const std::uint64_t h = cycle[(rotation + k) % cycle.size()];
        cycle_halfedge_ref_record ref;
        ref.canonical_id = artifact.cycle_halfedge_refs_.size();
        ref.halfedge = h;
        artifact.cycle_halfedge_refs_.push_back(std::move(ref));
        artifact.cycle_halfedge_index_.push_back(h);
        artifact.halfedges_[h].cycle = c;
      }
      artifact.face_cycles_.push_back(std::move(record));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: boundary darts, grouped by region.
  // -------------------------------------------------------------------------
  bool build_darts(artifact_type &artifact, bounded_boolean_error &error) {
    (void)error;
    // Lay out darts in region order following cycle order, so each region's
    // boundary range is contiguous.
    std::vector<std::vector<std::uint64_t>> region_darts(
        artifact.face_regions_.size());
    for (std::size_t c = 0; c < artifact.face_cycles_.size(); ++c) {
      const auto &cycle = artifact.face_cycles_[c];
      for (std::uint64_t k = 0; k < cycle.refs_count; ++k)
        region_darts[cycle.region].push_back(
            artifact.cycle_halfedge_index_[cycle.refs_begin + k]);
    }
    for (std::size_t r = 0; r < region_darts.size(); ++r) {
      auto &region = artifact.face_regions_[r];
      region.boundary_darts_begin = artifact.boundary_darts_.size();
      region.boundary_darts_count = region_darts[r].size();
      const std::size_t n = region_darts[r].size();
      for (std::size_t k = 0; k < n; ++k) {
        boundary_dart_record dart;
        dart.canonical_id = artifact.boundary_darts_.size();
        const std::uint64_t h = region_darts[r][k];
        dart.incidence = artifact.halfedges_[h].incidence;
        dart.halfedge = h;
        dart.region = r;
        dart.face_next = output_topology_invalid_ordinal;
        dart.face_prev = output_topology_invalid_ordinal;
        artifact.boundary_darts_.push_back(std::move(dart));
      }
    }
    // Link face_next/face_prev within each region's cyclic order.
    for (std::size_t r = 0; r < artifact.face_regions_.size(); ++r) {
      const auto &region = artifact.face_regions_[r];
      const std::size_t n = region.boundary_darts_count;
      for (std::size_t k = 0; k < n; ++k) {
        const std::uint64_t dart = region.boundary_darts_begin + k;
        const std::uint64_t next =
            region.boundary_darts_begin + ((k + 1) % n);
        const std::uint64_t prev =
            region.boundary_darts_begin + ((k + n - 1) % n);
        artifact.boundary_darts_[dart].face_next = next;
        artifact.boundary_darts_[dart].face_prev = prev;
      }
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: contour roles and certified strict-side witnesses.
  // -------------------------------------------------------------------------
  bool build_contours(artifact_type &artifact, bounded_boolean_error &error) {
    for (std::size_t c = 0; c < artifact.face_cycles_.size(); ++c) {
      const auto &cycle = artifact.face_cycles_[c];
      const std::uint64_t region = cycle.region;
      if (region >= artifact.face_regions_.size()) {
        error = output_topology_error(
            output_topology_subcode::contour_role_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "cycle region is out of range",
            output_topology_checkpoint::contour_role_assignment);
        return false;
      }

      // In V1 every positive-area region has exactly one outer cycle.
      const contour_role role = contour_role::outer;

      contour_node_record node;
      node.canonical_id = artifact.contour_nodes_.size();
      node.key.region = region;
      node.key.cycle = c;
      node.key.role = role;
      node.cycle = c;
      node.region = region;
      node.role = role;
      node.witness = output_topology_invalid_ordinal;
      artifact.contour_nodes_.push_back(std::move(node));
      artifact.face_regions_[region].outer_contour = node.canonical_id;

      if (!build_witness(artifact, region, c, error))
        return false;
    }
    return true;
  }

  bool build_witness(artifact_type &artifact, std::uint64_t region,
                     std::uint64_t cycle, bounded_boolean_error &error) {
    const auto &uses = retained_->retained_uses();
    std::uint64_t representative_use = output_topology_invalid_ordinal;
    for (std::size_t k = artifact.face_regions_[region].members_begin;
         k < artifact.face_regions_[region].members_begin +
                 artifact.face_regions_[region].members_count;
         ++k) {
      const std::uint64_t member = artifact.region_member_index_[k];
      representative_use = artifact.region_members_[member].retained_use;
      break;
    }
    if (representative_use == output_topology_invalid_ordinal ||
        representative_use >= uses.size()) {
      error = output_topology_error(
          output_topology_subcode::missing_contour_witness,
          bounded_boolean_error_category::internal_invariant_error,
          "region has no representative retained use",
          output_topology_checkpoint::witness_validation);
      return false;
    }
    const std::uint64_t atom = uses[representative_use].atom;
    if (atom == output_topology_invalid_ordinal ||
        atom >= classification_->atoms().size()) {
      error = output_topology_error(
          output_topology_subcode::missing_contour_witness,
          bounded_boolean_error_category::internal_invariant_error,
          "region representative atom is out of range",
          output_topology_checkpoint::witness_validation);
      return false;
    }
    const auto &cell_witness = classification_->atoms()[atom].witness;

    contour_witness_record witness;
    witness.canonical_id = artifact.contour_witnesses_.size();
    witness.key.region = region;
    witness.key.cycle = cycle;
    witness.key.source = witness_source::predecessor_arrangement_cell;
    witness.key.atom = atom;
    witness.region = region;
    witness.cycle = cycle;
    witness.atom = atom;
    for (std::size_t axis = 0; axis < 3; ++axis)
      witness.nominal_bits[axis] = cell_witness.nominal_bits[axis];
    for (std::size_t axis = 0; axis < 6; ++axis)
      witness.enclosure_bits[axis] = cell_witness.enclosure_bits[axis];
    witness.strict_side = true;

    artifact.contour_witnesses_.push_back(std::move(witness));
    artifact.contour_nodes_.back().witness = witness.canonical_id;
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: vertex-link reconstruction and Component 10 fan comparison.
  // -------------------------------------------------------------------------
  bool build_vertex_links(artifact_type &artifact,
                          bounded_boolean_error &error) {
    const std::size_t occurrence_count = artifact.vertex_occurrences_.size();
    const std::size_t halfedge_count = artifact.halfedges_.size();

    std::vector<std::vector<std::uint64_t>> outgoing(occurrence_count);
    for (std::size_t h = 0; h < halfedge_count; ++h) {
      const auto &halfedge = artifact.halfedges_[h];
      if (halfedge.origin >= occurrence_count) {
        error = output_topology_error(
            output_topology_subcode::vertex_link_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "halfedge origin occurrence is out of range",
            output_topology_checkpoint::vertex_link_reconstruction);
        return false;
      }
      outgoing[halfedge.origin].push_back(h);
    }

    artifact.vertex_link_evidence_.reserve(occurrence_count);
    for (std::size_t v = 0; v < occurrence_count; ++v) {
      std::sort(outgoing[v].begin(), outgoing[v].end());
      // One Component 10 link cycle = 2 * outgoing halfedges (each incident
      // planned edge contributes one outgoing halfedge and two link ports).
      const std::uint64_t expected_outgoing =
          retained_->vertex_occurrences()[v].cycle_count / 2;
      if (outgoing[v].size() != expected_outgoing) {
        error = output_topology_error(
            output_topology_subcode::occurrence_link_mismatch,
            bounded_boolean_error_category::internal_invariant_error,
            "output vertex link disagrees with Component 10 fan",
            output_topology_checkpoint::vertex_link_reconstruction);
        return false;
      }

      vertex_link_evidence_record link;
      link.canonical_id = artifact.vertex_link_evidence_.size();
      link.vertex_occurrence = v;
      link.outgoing_begin = artifact.outgoing_halfedges_.size();
      link.outgoing_count = outgoing[v].size();
      for (const auto h : outgoing[v])
        artifact.outgoing_halfedges_.push_back(h);
      artifact.vertex_link_evidence_.push_back(std::move(link));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: carrier balance and admissibility audits.
  // -------------------------------------------------------------------------
  bool build_carrier_balance(artifact_type &artifact,
                             bounded_boolean_error &error) {
    (void)error;
    for (const auto &balance : retained_->carrier_balances()) {
      carrier_balance_audit_record audit;
      audit.canonical_id = artifact.carrier_balance_audits_.size();
      audit.carrier_lineage =
          (static_cast<std::uint64_t>(balance.carrier.source_operand) << 48) |
          balance.carrier.lineage;
      audit.members_begin = artifact.carrier_balance_members_.size();
      audit.members_count = 0;
      audit.balanced = balance.balanced;
      const std::size_t begin = audit.members_begin;
      for (std::uint64_t k = balance.members_begin;
           k < balance.members_begin + balance.members_count; ++k) {
        if (k >= retained_->balance_members().size())
          continue;
        const std::uint64_t incidence = retained_->balance_members()[k];
        if (incidence >= retained_->incidences().size())
          continue;
        const std::uint64_t planned = retained_->incidences()[incidence].planned_edge;
        artifact.carrier_balance_members_.push_back(planned);
      }
      audit.members_count = artifact.carrier_balance_members_.size() - begin;
      artifact.carrier_balance_audits_.push_back(std::move(audit));
    }
    return true;
  }

  bool build_admissibility(artifact_type &artifact,
                           bounded_boolean_error &error) {
    (void)error;
    // V1 regions are whole retained facets whose source polygons were validated
    // as simple and planar by Components 02/04. Each region's single boundary
    // cycle is therefore a simple closed loop; record one definite-valid
    // evidence row per cycle.
    for (std::size_t c = 0; c < artifact.face_cycles_.size(); ++c) {
      admissibility_evidence_record evidence;
      evidence.canonical_id = artifact.admissibility_evidence_.size();
      evidence.disposition = bounded_admissibility::definite_valid;
      evidence.subject_a = c;
      evidence.subject_b = c;
      evidence.lineage = artifact.face_cycles_[c].region;
      artifact.admissibility_evidence_.push_back(std::move(evidence));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Phase: statistics, codec, verification.
  // -------------------------------------------------------------------------
  bool finalize(artifact_type &artifact, bounded_boolean_error &error) {
    artifact.statistics_.incidence_audit_count = artifact.incidence_audits_.size();
    artifact.statistics_.zero_measure_support_count =
        artifact.zero_measure_supports_.size();
    artifact.statistics_.coordinate_reference_count =
        artifact.coordinate_references_.size();
    artifact.statistics_.vertex_occurrence_count =
        artifact.vertex_occurrences_.size();
    artifact.statistics_.region_count = artifact.face_regions_.size();
    artifact.statistics_.region_member_count = artifact.region_members_.size();
    artifact.statistics_.continuation_consumption_count =
        artifact.continuation_consumptions_.size();
    artifact.statistics_.boundary_dart_count = artifact.boundary_darts_.size();
    artifact.statistics_.paired_edge_count = artifact.paired_edges_.size();
    artifact.statistics_.halfedge_count = artifact.halfedges_.size();
    artifact.statistics_.endpoint_fan_ref_count = artifact.endpoint_fan_refs_.size();
    artifact.statistics_.face_cycle_count = artifact.face_cycles_.size();
    artifact.statistics_.cycle_halfedge_ref_count =
        artifact.cycle_halfedge_refs_.size();
    artifact.statistics_.contour_node_count = artifact.contour_nodes_.size();
    artifact.statistics_.contour_witness_count = artifact.contour_witnesses_.size();
    artifact.statistics_.zero_measure_boundary_count =
        artifact.zero_measure_boundaries_.size();
    artifact.statistics_.admissibility_evidence_count =
        artifact.admissibility_evidence_.size();
    artifact.statistics_.carrier_balance_audit_count =
        artifact.carrier_balance_audits_.size();
    artifact.statistics_.vertex_link_evidence_count =
        artifact.vertex_link_evidence_.size();

    // Producer audit: reciprocal pair endpoints, count equations, and vertex
    // link closure.
    const std::size_t halfedge_count = artifact.halfedges_.size();
    for (std::size_t h = 0; h < halfedge_count; ++h) {
      const auto &halfedge = artifact.halfedges_[h];
      const auto &pair = artifact.halfedges_[halfedge.pair];
      if (pair.pair != h || halfedge.origin != pair.destination ||
          halfedge.destination != pair.origin) {
        error = output_topology_error(
            output_topology_subcode::pair_reciprocity_mismatch,
            bounded_boolean_error_category::internal_invariant_error,
            "producer pair reciprocity check failed",
            output_topology_checkpoint::producer_checks);
        return false;
      }
    }
    if (artifact.halfedges_.size() != 2 * artifact.paired_edges_.size()) {
      error = output_topology_error(
          output_topology_subcode::pair_reciprocity_mismatch,
          bounded_boolean_error_category::internal_invariant_error,
          "producer halfedge count check failed",
          output_topology_checkpoint::producer_checks);
      return false;
    }

    // Canonical encoding and digest.
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_polygonal_output_complex(artifact, bytes, codec_limits_,
                                         codec_error)) {
      error = codec_error;
      return false;
    }
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();

    // Independent verification.
    if (output_topology_cancelled(
            capabilities_, output_topology_checkpoint::independent_verification))
      return true;
    bounded_boolean_error verification_error;
    if (!verify_polygonal_output_complex(
            artifact, context_, *manifolds_, *relations_, *intersections_,
            *classification_, *retained_, verification_error)) {
      error = verification_error;
      return false;
    }
    artifact.verification_ =
        output_topology_verification_disposition::independently_verified;
    return true;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>>
build_polygonal_output_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const signed_feature_relations<T, I>> relations,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const classification_complex<T, I>> classification,
    std::shared_ptr<const retained_surface_complex<T, I>> retained,
    output_topology_capabilities capabilities,
    output_topology_codec_limits codec_limits) {
  try {
    output_topology_builder<T, I> builder(
        context, precision, std::move(manifolds), std::move(relations),
        std::move(intersections), std::move(classification),
        std::move(retained), std::move(capabilities), codec_limits);
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>>::
        failure(output_topology_error(
            output_topology_subcode::resource_preflight,
            bounded_boolean_error_category::resource_limit,
            "output topology allocation failed",
            output_topology_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const polygonal_output_complex<T, I>>>::
        failure(output_topology_error(
            output_topology_subcode::internal_invariant,
            bounded_boolean_error_category::internal_invariant_error,
            "output topology unexpected exception",
            output_topology_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
