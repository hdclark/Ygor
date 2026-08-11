#pragma once

#include "RelationSemanticProjection.h"

#include "CoplanarRelationOverlay.h"
#include "RelationCanonicalization.h"
#include "RelationConstructionPolicy.h"
#include "RelationEventSeeds.h"
#include "TransverseRelationEvaluation.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace relation_artifact_assembly_detail {

inline bool usable_truth(const relation_truth_record &truth) noexcept {
  return truth.reserved == 0 &&
         truth.bounded_sign != bounded_sign_status::invalid &&
         truth.exact_relation != exact_relation_status::invalid &&
         truth.disposition != predicate_disposition::fail_invalid;
}

inline feature_relation_status
edge_status(source_edge_contact_class contact,
            source_edge_orientation_relation orientation) noexcept {
  switch (contact) {
  case source_edge_contact_class::none:
    return feature_relation_status::definitely_separated;
  case source_edge_contact_class::proper_crossing:
    return feature_relation_status::proper_crossing;
  case source_edge_contact_class::endpoint_contact:
    return feature_relation_status::endpoint_crossing;
  case source_edge_contact_class::point_contact:
    return feature_relation_status::point_contact;
  case source_edge_contact_class::partial_overlap:
    return feature_relation_status::overlap;
  case source_edge_contact_class::first_contains_second:
  case source_edge_contact_class::second_contains_first:
    return feature_relation_status::containment;
  case source_edge_contact_class::equal:
    return orientation == source_edge_orientation_relation::opposite
               ? feature_relation_status::coincidence_opposite_orientation
               : feature_relation_status::coincidence_same_orientation;
  }
  return feature_relation_status::not_evaluated;
}

inline feature_relation_status
edge_facet_status(source_edge_facet_contact_class contact) noexcept {
  switch (contact) {
  case source_edge_facet_contact_class::none:
    return feature_relation_status::definitely_separated;
  case source_edge_facet_contact_class::proper_face_crossing:
    return feature_relation_status::proper_crossing;
  case source_edge_facet_contact_class::boundary_crossing:
    return feature_relation_status::endpoint_crossing;
  case source_edge_facet_contact_class::endpoint_contact:
  case source_edge_facet_contact_class::coplanar_point_contact:
    return feature_relation_status::point_contact;
  case source_edge_facet_contact_class::tangent_contact:
    return feature_relation_status::tangency;
  case source_edge_facet_contact_class::coplanar_boundary_overlap:
    return feature_relation_status::overlap;
  case source_edge_facet_contact_class::coplanar_containment:
    return feature_relation_status::containment;
  }
  return feature_relation_status::not_evaluated;
}

inline feature_relation_status facet_status(
    source_facet_support_relation_class classification) noexcept {
  switch (classification) {
  case source_facet_support_relation_class::transverse:
    return feature_relation_status::proper_crossing;
  case source_facet_support_relation_class::parallel_separated:
    return feature_relation_status::definitely_separated;
  case source_facet_support_relation_class::coplanar_same_orientation:
    return feature_relation_status::coincidence_same_orientation;
  case source_facet_support_relation_class::coplanar_opposite_orientation:
    return feature_relation_status::coincidence_opposite_orientation;
  }
  return feature_relation_status::not_evaluated;
}

inline feature_relation_status overlay_status(
    coplanar_facet_overlay_class classification) noexcept {
  switch (classification) {
  case coplanar_facet_overlay_class::disjoint:
    return feature_relation_status::definitely_separated;
  case coplanar_facet_overlay_class::point_contact:
    return feature_relation_status::point_contact;
  case coplanar_facet_overlay_class::segment_contact:
    return feature_relation_status::segment_contact;
  case coplanar_facet_overlay_class::area_overlap:
    return feature_relation_status::overlap;
  case coplanar_facet_overlay_class::first_contains_second:
  case coplanar_facet_overlay_class::second_contains_first:
    return feature_relation_status::containment;
  case coplanar_facet_overlay_class::equal_same_orientation:
    return feature_relation_status::coincidence_same_orientation;
  case coplanar_facet_overlay_class::equal_opposite_orientation:
    return feature_relation_status::coincidence_opposite_orientation;
  }
  return feature_relation_status::not_evaluated;
}

inline relation_coplanar_arc_kind
final_coplanar_arc_kind(coplanar_overlap_arc_kind kind) noexcept {
  switch (kind) {
  case coplanar_overlap_arc_kind::interior_boundary:
    return relation_coplanar_arc_kind::interior_boundary;
  case coplanar_overlap_arc_kind::shared_boundary:
    return relation_coplanar_arc_kind::shared_boundary;
  }
  return relation_coplanar_arc_kind::interior_boundary;
}

inline relation_coplanar_component_kind final_coplanar_component_kind(
    coplanar_overlap_component_kind kind) noexcept {
  switch (kind) {
  case coplanar_overlap_component_kind::isolated_point:
    return relation_coplanar_component_kind::isolated_point;
  case coplanar_overlap_component_kind::boundary_segment:
    return relation_coplanar_component_kind::boundary_segment;
  case coplanar_overlap_component_kind::area_boundary:
    return relation_coplanar_component_kind::area_boundary;
  case coplanar_overlap_component_kind::coincident_sheet_boundary:
    return relation_coplanar_component_kind::coincident_sheet_boundary;
  }
  return relation_coplanar_component_kind::isolated_point;
}

inline orientation_relation orientation_from_edge(
    source_edge_orientation_relation value) noexcept {
  switch (value) {
  case source_edge_orientation_relation::same:
    return orientation_relation::same;
  case source_edge_orientation_relation::opposite:
    return orientation_relation::opposite;
  case source_edge_orientation_relation::not_applicable:
    return orientation_relation::indeterminate;
  }
  return orientation_relation::indeterminate;
}

inline orientation_relation orientation_from_status(
    feature_relation_status value) noexcept {
  return value == feature_relation_status::coincidence_same_orientation
             ? orientation_relation::same
         : value == feature_relation_status::coincidence_opposite_orientation
             ? orientation_relation::opposite
             : orientation_relation::indeterminate;
}

inline bool public_contact(feature_relation_status value) noexcept {
  return value != feature_relation_status::definitely_separated &&
         value != feature_relation_status::not_evaluated;
}

inline relation_contact_dimension contact_dimension(
    feature_relation_family family, feature_relation_status status) noexcept {
  switch (status) {
  case feature_relation_status::proper_crossing:
  case feature_relation_status::endpoint_crossing:
  case feature_relation_status::point_contact:
  case feature_relation_status::tangency:
    return relation_contact_dimension::point;
  case feature_relation_status::segment_contact:
    return relation_contact_dimension::curve;
  case feature_relation_status::overlap:
  case feature_relation_status::containment:
  case feature_relation_status::coincidence_same_orientation:
  case feature_relation_status::coincidence_opposite_orientation:
    return family == feature_relation_family::source_facet_source_facet
               ? relation_contact_dimension::area
               : relation_contact_dimension::curve;
  case feature_relation_status::not_evaluated:
  case feature_relation_status::definitely_separated:
    return relation_contact_dimension::none;
  }
  return relation_contact_dimension::none;
}

inline relation_request_key derived_key(const relation_request_key &base,
                                        relation_request_family family,
                                        std::uint64_t directed_use,
                                        std::uint32_t occurrence) noexcept {
  relation_request_key out = base;
  out.family = family;
  out.directed_use = directed_use;
  out.occurrence_discriminator = occurrence;
  out.formula_version = contract_versions::exact_relation_formulas;
  out.policy_version = contract_versions::relation_request_key_schema;
  out.reserved = 0;
  return out;
}

inline relation_request_key imported_geometry_key(
    const bounded_boolean_digest &semantic_namespace,
    const relation_feature_key &feature, relation_record_scope scope) noexcept {
  relation_request_key out;
  out.semantic_namespace = semantic_namespace;
  out.family = relation_request_family::imported_source_geometry;
  out.scope = scope;
  out.first = feature;
  out.second = relation_feature_key{};
  out.second.operand = feature.operand;
  out.formula_version = contract_versions::exact_relation_formulas;
  out.policy_version = contract_versions::relation_request_key_schema;
  return out;
}

inline std::uint64_t tagged_use(std::uint8_t domain,
                                 std::uint8_t category = 0) noexcept {
  return (static_cast<std::uint64_t>(domain) << 56U) |
         (static_cast<std::uint64_t>(category) << 48U);
}

inline std::uint64_t tagged_source_use(
    std::uint8_t domain, std::uint8_t category,
    relation_request_family source_family) noexcept {
  return tagged_use(domain, category) |
         static_cast<std::uint64_t>(source_family);
}

inline std::uint64_t symbolic_directed_use(
    std::uint8_t domain, const symbolic_rule_key &key,
    symbolic_relation_subject_kind subject_kind) noexcept {
  return tagged_use(domain) |
         (static_cast<std::uint64_t>(key.operation) << 0U) |
         (static_cast<std::uint64_t>(key.acting_operand) << 3U) |
         (static_cast<std::uint64_t>(key.relation) << 4U) |
         (static_cast<std::uint64_t>(key.orientation) << 8U) |
         (static_cast<std::uint64_t>(key.ownership_role) << 10U) |
         (static_cast<std::uint64_t>(key.half_open_role) << 13U) |
         (static_cast<std::uint64_t>(key.transition) << 15U) |
         (static_cast<std::uint64_t>(key.occurrence_class) << 17U) |
         (static_cast<std::uint64_t>(subject_kind) << 19U);
}

inline const canonical_relation_request *
find_request(const relation_request_graph &graph,
             const relation_request_key &key) noexcept {
  const auto it = std::lower_bound(
      graph.requests.begin(), graph.requests.end(), key,
      [](const canonical_relation_request &record,
         const relation_request_key &candidate) {
        return record.key < candidate;
      });
  return it == graph.requests.end() || it->key != key ? nullptr : &*it;
}

inline std::vector<candidate_id>
request_witnesses(const relation_request_graph &graph,
                  const canonical_relation_request &request) {
  std::vector<candidate_id> out;
  if (request.witness_begin > graph.candidate_witnesses.size() ||
      request.witness_count >
          graph.candidate_witnesses.size() - request.witness_begin)
    return out;
  out.insert(out.end(),
             graph.candidate_witnesses.begin() +
                 static_cast<std::ptrdiff_t>(request.witness_begin),
             graph.candidate_witnesses.begin() + static_cast<std::ptrdiff_t>(
                                                      request.witness_begin +
                                                      request.witness_count));
  return out;
}

inline std::vector<relation_request_key>
request_dependencies(const relation_request_graph &graph,
                     const canonical_relation_request &request) {
  std::vector<relation_request_key> out;
  if (request.dependency_begin > graph.dependencies.size() ||
      request.dependency_count >
          graph.dependencies.size() - request.dependency_begin)
    return out;
  for (std::uint64_t offset = 0; offset < request.dependency_count; ++offset) {
    const auto &dependency =
        graph.dependencies[request.dependency_begin + offset];
    if (dependency.producer.ordinal() < graph.requests.size())
      out.push_back(graph.requests[dependency.producer.ordinal()].key);
  }
  return out;
}

template <class T>
bool same_source_facet_region(
    const source_facet_point_region_record<T> &a,
    const source_facet_point_region_record<T> &b) {
  canonical_writer first;
  canonical_writer second;
  source_edge_facet_detail::encode_region(first, a);
  source_edge_facet_detail::encode_region(second, b);
  return first.take() == second.take();
}

template <class T>
bool same_geometry_snapshot(const source_edge_geometry_snapshot<T> &a,
                            const source_edge_geometry_snapshot<T> &b) {
  canonical_writer first;
  canonical_writer second;
  source_edge_relation_detail::encode_snapshot(first, a);
  source_edge_relation_detail::encode_snapshot(second, b);
  return first.take() == second.take();
}

template <class T>
void set_construction_components(relation_construction_record &out,
                                 const std::array<T, 3> &nominal,
                                 const std::array<finite_interval<T>, 3> &bounds) {
  out.component_count = 3;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal_bits[axis] = static_cast<std::uint64_t>(to_bits(nominal[axis]));
    out.lower_bits[axis] =
        static_cast<std::uint64_t>(to_bits(bounds[axis].lower()));
    out.upper_bits[axis] =
        static_cast<std::uint64_t>(to_bits(bounds[axis].upper()));
  }
}

template <class T>
void set_construction_components(relation_construction_record &out,
                                 const bounded_geometry_snapshot3<T> &point,
                                 const bounded_geometry_snapshot3<T> &direction) {
  out.component_count = 6;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal_bits[axis] =
        static_cast<std::uint64_t>(to_bits(point.rounded[axis]));
    out.lower_bits[axis] =
        static_cast<std::uint64_t>(to_bits(point.lower[axis]));
    out.upper_bits[axis] =
        static_cast<std::uint64_t>(to_bits(point.upper[axis]));
    out.nominal_bits[axis + 3] =
        static_cast<std::uint64_t>(to_bits(direction.rounded[axis]));
    out.lower_bits[axis + 3] =
        static_cast<std::uint64_t>(to_bits(direction.lower[axis]));
    out.upper_bits[axis + 3] =
        static_cast<std::uint64_t>(to_bits(direction.upper[axis]));
  }
}

template <class T>
void set_construction_components(relation_construction_record &out,
                                 const projected_source_point<T> &point) {
  out.component_count = 2;
  for (std::size_t axis = 0; axis < 2; ++axis) {
    out.nominal_bits[axis] =
        static_cast<std::uint64_t>(to_bits(point.nominal[axis]));
    out.lower_bits[axis] =
        static_cast<std::uint64_t>(to_bits(point.enclosure[axis].lower()));
    out.upper_bits[axis] =
        static_cast<std::uint64_t>(to_bits(point.enclosure[axis].upper()));
  }
}


template <class T>
void set_construction_geometry(
    relation_construction_record &out,
    const relation_construction_policy_detail::geometry_snapshot<T> &geometry) {
  out.kind = geometry.kind;
  out.coordinate_space = geometry.coordinate_space;
  out.component_count = geometry.component_count;
  out.projection_axis = geometry.projection_axis;
  for (std::size_t component = 0; component < geometry.component_count;
       ++component) {
    out.nominal_bits[component] =
        static_cast<std::uint64_t>(to_bits(geometry.nominal[component]));
    out.lower_bits[component] =
        static_cast<std::uint64_t>(to_bits(geometry.lower[component]));
    out.upper_bits[component] =
        static_cast<std::uint64_t>(to_bits(geometry.upper[component]));
  }
  out.source_provenance = geometry.provenance;
  out.geometric_lineage = geometry.lineage;
  out.accepted_source_vertex = geometry.accepted_source_vertex;
  out.finite = geometry.finite;
  out.tolerance_compatible = geometry.tolerance_compatible;
}

template <class T>
void set_construction_ledger_geometry(
    relation_construction_ledger_record &out,
    const relation_construction_policy_detail::geometry_snapshot<T> &geometry) {
  out.coordinate_space = geometry.coordinate_space;
  out.component_count = geometry.component_count;
  out.projection_axis = geometry.projection_axis;
  for (std::size_t component = 0; component < geometry.component_count;
       ++component) {
    out.nominal_bits[component] =
        static_cast<std::uint64_t>(to_bits(geometry.nominal[component]));
    out.lower_bits[component] =
        static_cast<std::uint64_t>(to_bits(geometry.lower[component]));
    out.upper_bits[component] =
        static_cast<std::uint64_t>(to_bits(geometry.upper[component]));
  }
  out.source_provenance = geometry.provenance;
  out.geometric_lineage = geometry.lineage;
  out.accepted_source_vertex = geometry.accepted_source_vertex;
  out.finite = geometry.finite;
  out.tolerance_compatible = geometry.tolerance_compatible;
}

template <class T, class Record>
bool set_construction_certificate(
    Record &out, const construction_operation_certificate<T> &certificate) {
  if (!valid_construction_operation_certificate(certificate))
    return false;
  out.formula_version = certificate.formula_version;
  out.operation = certificate.operation;
  out.conditioning = certificate.conditioning;
  out.tolerance = certificate.tolerance;
  out.precision_trace_root =
      certificate.issued_outputs[0].identity.trace_root;
  out.radial_error_upper_bits =
      static_cast<std::uint64_t>(to_bits(certificate.radial_error_upper));
  out.denominator_lower_bits =
      static_cast<std::uint64_t>(to_bits(certificate.denominator.lower()));
  out.denominator_upper_bits =
      static_cast<std::uint64_t>(to_bits(certificate.denominator.upper()));
  out.conditioning_lower_bits =
      static_cast<std::uint64_t>(to_bits(certificate.conditioning_lower));
  out.tolerance_boundary_bits =
      static_cast<std::uint64_t>(to_bits(certificate.tolerance_boundary));
  for (std::size_t component = 0;
       component < certificate.axis_error_upper.size(); ++component)
    out.axis_error_upper_bits[component] = static_cast<std::uint64_t>(
        to_bits(certificate.axis_error_upper[component]));
  out.ordered_bounded_inputs.clear();
  out.ordered_bounded_inputs.reserve(certificate.ordered_inputs.size());
  for (const auto input : certificate.ordered_inputs)
    out.ordered_bounded_inputs.push_back(input.value.ordinal());
  canonical_writer encoded;
  encode_construction_operation_certificate(encoded, certificate);
  out.certificate_evidence = encoded.take();
  out.precision_evidence_complete = true;
  return !out.certificate_evidence.empty();
}

template <class T>
relation_construction_policy_detail::geometry_snapshot<T>
construction_geometry_from_record(
    const relation_construction_record &record) noexcept {
  relation_construction_policy_detail::geometry_snapshot<T> out;
  out.kind = record.kind;
  out.coordinate_space = record.coordinate_space;
  out.component_count = record.component_count;
  out.projection_axis = record.projection_axis;
  for (std::size_t component = 0; component < record.component_count &&
                                  component < out.nominal.size();
       ++component) {
    using bits_type = floating_uint_t<T>;
    out.nominal[component] =
        from_bits<T>(static_cast<bits_type>(record.nominal_bits[component]));
    out.lower[component] =
        from_bits<T>(static_cast<bits_type>(record.lower_bits[component]));
    out.upper[component] =
        from_bits<T>(static_cast<bits_type>(record.upper_bits[component]));
  }
  out.provenance = record.source_provenance;
  out.lineage = record.geometric_lineage;
  out.accepted_source_vertex = record.accepted_source_vertex;
  out.finite = record.finite;
  out.tolerance_compatible = record.tolerance_compatible;
  return out;
}

inline relation_feature_key sheet_occurrence_feature(
    const relation_feature_key &facet, std::uint32_t occurrence) noexcept {
  relation_feature_key out = facet;
  out.kind = relation_feature_kind::sheet_occurrence;
  out.occurrence = occurrence;
  return out;
}

template <class T>
inline relation_family symbolic_family_for_edge(
    source_edge_contact_class contact,
    const source_edge_point_construction<T> *point) noexcept {
  switch (contact) {
  case source_edge_contact_class::partial_overlap:
  case source_edge_contact_class::first_contains_second:
  case source_edge_contact_class::second_contains_first:
  case source_edge_contact_class::equal:
    return relation_family::equal_edge;
  case source_edge_contact_class::endpoint_contact:
  case source_edge_contact_class::point_contact:
    if (point) {
      const bool first_endpoint = point->first_endpoint_owner_mask != 0;
      const bool second_endpoint = point->second_endpoint_owner_mask != 0;
      if (first_endpoint && second_endpoint)
        return relation_family::vertex_vertex;
      if (first_endpoint || second_endpoint)
        return relation_family::vertex_edge;
    }
    return relation_family::edge_edge;
  default:
    return relation_family::edge_edge;
  }
}

template <class T>
inline relation_family symbolic_family_for_edge_facet(
    const source_edge_facet_event_record<T> &event,
    source_edge_facet_contact_class contact) noexcept {
  if (event.kind == source_edge_facet_event_kind::tangent_contact ||
      contact == source_edge_facet_contact_class::tangent_contact)
    return relation_family::tangent;
  if (contact == source_edge_facet_contact_class::coplanar_point_contact ||
      contact == source_edge_facet_contact_class::coplanar_boundary_overlap ||
      contact == source_edge_facet_contact_class::coplanar_containment)
    return relation_family::coplanar;
  const bool query_endpoint = event.construction.edge_endpoint_owner_mask != 0;
  if (event.region.classification ==
      source_facet_point_region_class::original_vertex)
    return query_endpoint ? relation_family::vertex_vertex
                          : relation_family::vertex_edge;
  if (event.region.classification ==
      source_facet_point_region_class::original_edge)
    return query_endpoint ? relation_family::vertex_edge
                          : relation_family::edge_edge;
  return query_endpoint ? relation_family::vertex_face
                        : relation_family::edge_face;
}

inline relation_family symbolic_family_for_overlay(
    coplanar_facet_overlay_class value) noexcept {
  switch (value) {
  case coplanar_facet_overlay_class::equal_same_orientation:
  case coplanar_facet_overlay_class::equal_opposite_orientation:
    return relation_family::coincident_face;
  case coplanar_facet_overlay_class::segment_contact:
    return relation_family::equal_edge;
  default:
    return relation_family::coplanar;
  }
}

} // namespace relation_artifact_assembly_detail

template <class T, class I> class relation_artifact_assembler final {
public:
  using artifact_type = signed_feature_relations<T, I>;
  using vertex_facet_stage_type = source_vertex_facet_evaluated_stage<T>;
  using edge_stage_type = candidate_source_edge_relation_stage<T>;
  using edge_facet_stage_type = candidate_source_edge_facet_relation_stage<T>;
  using facet_stage_type = candidate_source_facet_relation_stage<T>;
  using overlay_stage_type = candidate_coplanar_overlay_stage<T>;
  using transverse_stage_type = transverse_relation_evaluated_stage<T>;

  relation_artifact_assembler(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const canonical_candidate_stream<T, I>> candidates,
      std::shared_ptr<const vertex_facet_stage_type> vertex_facet_stage,
      std::shared_ptr<const edge_stage_type> edge_stage,
      std::shared_ptr<const edge_facet_stage_type> edge_facet_stage,
      std::shared_ptr<const facet_stage_type> facet_stage,
      std::shared_ptr<const overlay_stage_type> overlay_stage,
      std::shared_ptr<const transverse_stage_type> transverse_stage,
      std::shared_ptr<const relation_execution_authority> execution_authority,
      const relation_capabilities &capabilities)
      : context_(context), precision_(precision), candidates_(std::move(candidates)),
        vertex_facet_stage_(std::move(vertex_facet_stage)),
        edge_stage_(std::move(edge_stage)),
        edge_facet_stage_(std::move(edge_facet_stage)),
        facet_stage_(std::move(facet_stage)),
        overlay_stage_(std::move(overlay_stage)),
        transverse_stage_(std::move(transverse_stage)),
        execution_authority_(std::move(execution_authority)),
        capabilities_(capabilities) {}

  bool assemble(artifact_type &artifact, bounded_boolean_error &error) {
    try {
      if (!check_cancel(error, relation_checkpoint::predecessor_validation) ||
          !validate_inputs(error) ||
          !check_cancel(error, relation_checkpoint::rounded_primitive_evaluation) ||
          !discover_base_relations(error) ||
          !check_cancel(error, relation_checkpoint::exact_relation_evaluation) ||
          !discover_primitive_support(error) ||
          !check_cancel(error, relation_checkpoint::truth_record_assembly) ||
          !check_cancel(error, relation_checkpoint::source_facet_region_evaluation) ||
          !discover_family04_evidence(error) ||
          !check_cancel(error, relation_checkpoint::construction_validation) ||
          !discover_constructions_and_symbolics(error) ||
          !check_cancel(error, relation_checkpoint::downstream_selection_boundary_audit) ||
          !discover_candidate_dispositions(error) ||
          !check_cancel(error, relation_checkpoint::dependency_closure) ||
          !build_graph(error) ||
          !check_cancel(error, relation_checkpoint::graph_finalization) ||
          !publish_imported_geometry(error) ||
          !check_cancel(error, relation_checkpoint::canonical_id_and_reference_remap) ||
          !publish_relations(error) || !publish_family04_evidence(error) ||
          !publish_constructions(error) || !publish_coplanar_topology(error) ||
          !check_cancel(error, relation_checkpoint::crossing_multiplicity) ||
          !check_cancel(error, relation_checkpoint::symbolic_eligibility) ||
          !check_cancel(error, relation_checkpoint::symbolic_matrix_lookup) ||
          !publish_symbolics_and_crossings(error) ||
          !check_cancel(error, relation_checkpoint::event_seed_and_disposition_reconciliation) ||
           !publish_event_seeds(error) ||
           !publish_transverse_carrier_memberships(error) ||
           !publish_downstream_handoff(error) ||
           !publish_candidate_dispositions(error) ||
          !publish_triangle_local_reconciliation(error) ||
          !check_cancel(error, relation_checkpoint::producer_verification))
        return false;

      artifact.owner_ = capabilities_.owner;
      artifact.candidates_ = candidates_;
      artifact.source_edge_stage_ = edge_stage_;
      artifact.source_edge_facet_stage_ = edge_facet_stage_;
      artifact.source_facet_stage_ = facet_stage_;
      artifact.coplanar_overlay_stage_ = overlay_stage_;
      artifact.request_graph_ = std::move(graph_);
      artifact.execution_authority_ = *execution_authority_;
      artifact.imported_geometry_ = std::move(imported_geometry_);
      artifact.bounded_primitives_ = std::move(bounded_primitives_);
      artifact.exact_relations_ = std::move(exact_relations_);
      artifact.truth_lineage_ = std::move(truth_lineage_);
      artifact.interval_evidence_ = std::move(interval_evidence_);
      artifact.source_facet_regions_ = std::move(source_facet_regions_);
      artifact.truth_records_ = std::move(truth_records_);
      artifact.relations_ = std::move(relations_);
      artifact.constructions_ = std::move(constructions_);
      artifact.construction_ledger_ = std::move(construction_ledger_);
      artifact.coplanar_event_nodes_ = std::move(coplanar_event_nodes_);
      artifact.coplanar_oriented_arcs_ = std::move(coplanar_oriented_arcs_);
      artifact.coplanar_overlap_components_ =
          std::move(coplanar_overlap_components_);
      artifact.symbolic_eligibility_ = std::move(eligibility_);
      artifact.symbolic_decisions_ = std::move(decisions_);
      artifact.crossings_ = std::move(crossings_);
      artifact.event_seeds_ = std::move(seed_table_.records);
      artifact.event_seed_incidence_ = std::move(seed_table_.incidence);
      artifact.event_seed_candidate_incidence_ =
          std::move(seed_table_.candidate_incidence);
      artifact.candidate_dispositions_ = std::move(dispositions_);
      artifact.triangle_local_reconciliation_ =
          std::move(triangle_local_reconciliation_);
      artifact.transverse_carrier_memberships_ =
          std::move(transverse_carrier_memberships_);
      artifact.source_topology_ = std::move(source_topology_);
      artifact.transverse_carrier_supports_ =
          std::move(transverse_carrier_supports_);
      artifact.candidate_relation_coverage_ =
          std::move(candidate_relation_coverage_);
      artifact.candidate_event_seed_coverage_ =
          std::move(candidate_event_seed_coverage_);
      artifact.candidate_partitions_ = std::move(candidate_partitions_);
      artifact.context_digest_ = context_.context_digest;
      artifact.precision_digest_ =
          relation_precision_semantic_digest(precision_);
      artifact.candidate_digest_ = candidates_->candidate_digest();
      artifact.graph_digest_ = artifact.request_graph_.semantic_digest;
      artifact.operation_ = context_.operation;
      artifact.residual_boundary_ = precision_.tolerance();
      artifact.symbolic_policy_digest_ = context_.symbolic.digest;
      fill_statistics(artifact);
      artifact.verification_evidence_.id = relation_verifier_evidence_id(0);
      artifact.verification_evidence_.verifier_version =
          contract_versions::relation_verifier;
      artifact.verification_evidence_.graph_reconstructed = true;
      artifact.verification_evidence_.owner_exclusion_checked = true;
      artifact.verification_evidence_.selection_boundary_checked = true;
      artifact.verification_evidence_.candidate_dispositions_complete = true;
      artifact.verification_evidence_.verifier_work_units =
          artifact.statistics_.verifier_work_units;
      artifact.verification_evidence_.semantic_digest = artifact.graph_digest_;
      artifact.verification_ =
          relation_verification_disposition::independently_verified;
      return true;
    } catch (const std::bad_alloc &) {
      error = relation_error(relation_subcode::resource_preflight,
                             bounded_boolean_error_category::resource_limit,
                             "Component 07 final artifact allocation failed",
                             relation_checkpoint::discovery_resource_reservation);
      return false;
    } catch (...) {
      error = relation_error(relation_subcode::internal_invariant,
                             bounded_boolean_error_category::internal_invariant_error,
                             "Component 07 final artifact assembly raised an unexpected exception",
                             relation_checkpoint::canonical_id_and_reference_remap);
      return false;
    }
  }

private:
  enum class base_kind : std::uint8_t {
    vertex_facet = 1,
    edge = 2,
    edge_facet = 3,
    facet = 4,
    overlay = 5
  };

  struct base_descriptor final {
    base_kind kind = base_kind::edge;
    std::uint64_t ordinal = 0;
    relation_request_key key{};
    feature_relation_family family = feature_relation_family::source_edge_source_edge;
    feature_relation_status status = feature_relation_status::not_evaluated;
    relation_record_scope scope = relation_record_scope::public_source_feature;
    std::vector<relation_truth_record> truth;
    std::int32_t numeric_crossing = 0;
    std::uint32_t occurrence = 0;
    std::vector<candidate_id> witnesses;
    bool has_vertex_region = false;
    source_facet_point_region_record<T> vertex_region{};
    source_edge_geometry_snapshot<T> vertex_point{};
  };

  struct construction_descriptor final {
    relation_request_key key{};
    relation_request_key source_relation{};
    relation_request_key authoritative_source_relation{};
    relation_construction_precedence precedence =
        relation_construction_precedence::verification_witness;
    relation_feature_key authoritative_source_feature{};
    relation_construction_record value{};
    relation_construction_ledger_record ledger{};
    std::uint32_t occurrence = 0;
    feature_relation_family seed_family =
        feature_relation_family::source_edge_source_edge;
    std::vector<relation_feature_key> incidence;
    std::vector<relation_request_key> dependencies;
    bool emit_seed = false;
    bool distinct_occurrence = false;
    std::vector<candidate_id> witnesses;
  };

  struct interval_descriptor final {
    relation_request_key key{};
    relation_request_key source_relation{};
    relation_interval_evidence_record value{};
    std::vector<candidate_id> witnesses;
  };

  struct region_descriptor final {
    relation_request_key key{};
    relation_request_key source_relation{};
    relation_source_facet_region_record<T> value{};
    std::vector<candidate_id> witnesses;
  };

  struct symbolic_descriptor final {
    relation_request_key eligibility_key{};
    relation_request_key decision_key{};
    relation_request_key source_relation{};
    relation_request_key construction_key{};
    relation_request_key multiplicity_key{};
    symbolic_rule_key rule_key{};
    symbolic_relation_subject_kind subject_kind =
        symbolic_relation_subject_kind::relation;
    std::uint64_t subject_ordinal = 0;
    std::uint32_t occurrence = 0;
    bool has_construction = false;
    bool has_multiplicity = false;
    std::vector<candidate_id> witnesses;
  };

  struct transverse_membership_descriptor final {
    relation_request_key carrier_relation{};
    relation_request_key member_relation{};
    std::uint32_t occurrence = 0;
    std::uint32_t parameter_occurrence = 0;
    std::uint32_t residual_occurrence = 0;
    std::uint32_t first_region_occurrence = 0;
    std::uint32_t second_region_occurrence = 0;
  };

  struct disposition_descriptor final {
    relation_request_key key{};
    std::vector<relation_request_key> dependencies;
    std::vector<candidate_id> witnesses;
  };

  struct source_fan_group_key final {
    bool boundary_group = false;
    relation_feature_key query_edge{};
    operand_id opposite_operand = operand_id::a;
    std::uint8_t boundary_kind = 0; // 1 source edge, 2 source vertex
    std::uint64_t owner_primary = 0;
    std::uint64_t owner_secondary = 0;
    relation_request_key singleton_relation{};
    std::uint32_t singleton_occurrence = 0;

    friend bool operator<(const source_fan_group_key &a,
                          const source_fan_group_key &b) noexcept {
      return std::tie(a.boundary_group, a.query_edge, a.opposite_operand,
                      a.boundary_kind, a.owner_primary, a.owner_secondary,
                      a.singleton_relation, a.singleton_occurrence) <
             std::tie(b.boundary_group, b.query_edge, b.opposite_operand,
                      b.boundary_kind, b.owner_primary, b.owner_secondary,
                      b.singleton_relation, b.singleton_occurrence);
    }
    friend bool operator==(const source_fan_group_key &a,
                           const source_fan_group_key &b) noexcept {
      return std::tie(a.boundary_group, a.query_edge, a.opposite_operand,
                      a.boundary_kind, a.owner_primary, a.owner_secondary,
                      a.singleton_relation, a.singleton_occurrence) ==
             std::tie(b.boundary_group, b.query_edge, b.opposite_operand,
                      b.boundary_kind, b.owner_primary, b.owner_secondary,
                      b.singleton_relation, b.singleton_occurrence);
    }
  };

  struct crossing_descriptor final {
    source_fan_group_key group{};
    relation_request_key source_relation{};
    feature_relation_id relation{0};
    const source_edge_facet_event_record<T> *event = nullptr;
    std::uint32_t occurrence = 0;
    std::int8_t local_transition = 0;
    std::int8_t symbolic_crossing = 0;
    operand_id half_open_owner = operand_id::a;
  };

  bool check_cancel(bounded_boolean_error &error,
                    relation_checkpoint checkpoint) const {
    if (!relation_cancelled(capabilities_, checkpoint))
      return true;
    error = relation_error(relation_subcode::cancelled,
                           bounded_boolean_error_category::cancelled,
                           "Component 07 final assembly cancelled", checkpoint);
    return false;
  }

  bool fail(bounded_boolean_error &error, relation_subcode subcode,
            const char *summary, relation_checkpoint checkpoint) const {
    error = relation_error(subcode,
                           bounded_boolean_error_category::internal_invariant_error,
                           summary, checkpoint);
    return false;
  }

  bool validate_inputs(bounded_boolean_error &error) {
    if (!candidates_ || !execution_authority_ || !vertex_facet_stage_ || !edge_stage_ ||
        !edge_facet_stage_ || !facet_stage_ || !overlay_stage_ ||
        !transverse_stage_ || !capabilities_.owner.anchor ||
        !candidates_->owner().same_owner(capabilities_.owner) ||
        !vertex_facet_stage_->owner.same_owner(capabilities_.owner) ||
        !edge_stage_->owner.same_owner(capabilities_.owner) ||
        !edge_facet_stage_->owner.same_owner(capabilities_.owner) ||
        !facet_stage_->owner.same_owner(capabilities_.owner) ||
        !overlay_stage_->owner.same_owner(capabilities_.owner) ||
        !execution_authority_->owner.same_owner(capabilities_.owner) ||
        !execution_authority_->closed_before_evaluation ||
        !execution_authority_->independently_verified ||
        !execution_authorizes(*execution_authority_, edge_stage_->request_graph) ||
        !execution_authorizes(*execution_authority_,
                              edge_facet_stage_->request_graph) ||
        !execution_authorizes(*execution_authority_, facet_stage_->request_graph) ||
        edge_stage_->relations.size() != edge_stage_->request_graph.requests.size() ||
        edge_facet_stage_->relations.size() !=
            edge_facet_stage_->request_graph.requests.size() ||
        facet_stage_->relations.size() != facet_stage_->request_graph.requests.size() ||
        overlay_stage_->links.size() != overlay_stage_->overlays.size() ||
        transverse_stage_->expected_population != transverse_stage_->records.size() ||
        transverse_stage_->evaluation_count != transverse_stage_->records.size())
      return fail(error, relation_subcode::predecessor_mismatch,
                  "Component 07 final assembly predecessor handshake failed",
                  relation_checkpoint::predecessor_validation);
    ordered_event_occurrences_.clear();
    for (const auto &entry : edge_facet_stage_->ordered_events) {
      if (!ordered_event_occurrences_
               .emplace(std::make_pair(entry.relation, entry.local_event),
                        entry.canonical_occurrence)
               .second)
        return fail(error, relation_subcode::incompatible_duplicate_request,
                    "Component 07 final assembly event-order key is duplicated",
                    relation_checkpoint::predecessor_validation);
    }
    std::uint64_t expected_events = 0;
    for (const auto &relation : edge_facet_stage_->relations) {
      if (relation.events.size() >
              std::numeric_limits<std::uint64_t>::max() - expected_events)
        return fail(error, relation_subcode::count_overflow,
                    "Component 07 final assembly event count overflowed",
                    relation_checkpoint::count_representability_preflight);
      expected_events += relation.events.size();
    }
    if (expected_events != ordered_event_occurrences_.size())
      return fail(error, relation_subcode::predecessor_mismatch,
                  "Component 07 final assembly event-order table is incomplete",
                  relation_checkpoint::predecessor_validation);
    return true;
  }

  bool canonical_event_occurrence(const base_descriptor &base,
                                  std::uint32_t local_event,
                                  std::uint32_t &occurrence,
                                  bounded_boolean_error &error) const {
    if (base.kind != base_kind::edge_facet ||
        base.ordinal >= edge_facet_stage_->request_graph.requests.size())
      return fail(error, relation_subcode::missing_dependency,
                  "Component 07 canonical event occurrence source is absent",
                  relation_checkpoint::canonical_id_and_reference_remap);
    const auto relation =
        edge_facet_stage_->request_graph.requests[base.ordinal].id;
    const auto found =
        ordered_event_occurrences_.find(std::make_pair(relation, local_event));
    if (found == ordered_event_occurrences_.end())
      return fail(error, relation_subcode::missing_dependency,
                  "Component 07 canonical event occurrence is absent",
                  relation_checkpoint::canonical_id_and_reference_remap);
    occurrence = found->second;
    return true;
  }

  void add_stage_proposal(const relation_request_graph &source,
                          const canonical_relation_request &request) {
    relation_request_proposal proposal;
    proposal.key = request.key;
    proposal.dependencies =
        relation_artifact_assembly_detail::request_dependencies(source, request);
    proposal.candidate_witnesses =
        relation_artifact_assembly_detail::request_witnesses(source, request);
    proposals_.push_back(std::move(proposal));
  }

  void append_truth(base_descriptor &descriptor,
                    const relation_truth_record &truth) {
    if (relation_artifact_assembly_detail::usable_truth(truth))
      descriptor.truth.push_back(truth);
  }

  bool source_vertex_point(operand_id operand, std::uint64_t source_vertex,
                           source_edge_geometry_snapshot<T> &out) const {
    const auto *topology = operand == operand_id::a
                               ? candidates_->manifolds()->a().get()
                               : candidates_->manifolds()->b().get();
    if (!topology ||
        source_vertex >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex = topology->source_vertex_to_vertex()[source_vertex];
    if (vertex >= topology->vertices().size())
      return false;
    const auto &record = topology->vertices()[vertex];
    out.rounded_nominal = record.committed_point;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto interval =
          finite_interval<T>::create(record.lower[axis], record.upper[axis]);
      if (!interval)
        return false;
      out.enclosure[axis] = *interval;
    }
    out.provenance = record.presentation_vertex + 1;
    out.lineage = record.canonical_id + 1;
    return source_edge_relation_detail::valid_snapshot(out);
  }

  bool discover_base_relations(bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    candidate_base_keys_.assign(candidates_->candidates().size(), {});

    for (const auto &request : execution_authority_->graph.requests) {
      if (request.key.family !=
          relation_request_family::source_point_source_facet_region)
        continue;
      relation_request_proposal proposal;
      proposal.key = request.key;
      proposal.candidate_witnesses = request_witnesses(
          execution_authority_->graph, request);
      proposals_.push_back(std::move(proposal));
      const auto *evaluated = find_source_vertex_facet_evaluation(
          *vertex_facet_stage_, request.key);
      if (!evaluated || evaluated->request != request.id)
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 source-vertex/facet evaluated record is absent",
                    relation_checkpoint::predecessor_validation);
      base_descriptor descriptor;
      descriptor.kind = base_kind::vertex_facet;
      descriptor.key = request.key;
      descriptor.family = feature_relation_family::source_vertex_source_facet;
      descriptor.scope = relation_record_scope::public_source_feature;
      descriptor.witnesses = request_witnesses(execution_authority_->graph,
                                                request);
      descriptor.vertex_point = evaluated->point;
      descriptor.truth.push_back(evaluated->support_truth);
      descriptor.has_vertex_region = evaluated->has_region;
      if (evaluated->has_region)
        descriptor.vertex_region = evaluated->region;
      descriptor.status = !evaluated->has_region
                              ? feature_relation_status::definitely_separated
                          : evaluated->region.classification ==
                                    source_facet_point_region_class::outside
                              ? feature_relation_status::definitely_separated
                              : feature_relation_status::point_contact;
      bases_.push_back(std::move(descriptor));
      record_candidate_base(request.key, bases_.back().witnesses);
    }

    for (std::size_t i = 0; i < edge_stage_->request_graph.requests.size(); ++i) {
      const auto &request = edge_stage_->request_graph.requests[i];
      const auto &source = edge_stage_->relations[i];
      add_stage_proposal(edge_stage_->request_graph, request);
      base_descriptor descriptor;
      descriptor.kind = base_kind::edge;
      descriptor.ordinal = i;
      descriptor.key = request.key;
      descriptor.family = feature_relation_family::source_edge_source_edge;
      descriptor.status = edge_status(source.contact, source.orientation);
      descriptor.scope = request.key.scope;
      descriptor.witnesses = request_witnesses(edge_stage_->request_graph, request);
      append_truth(descriptor, source.parallel_truth);
      if (source.has_coplanarity_truth)
        append_truth(descriptor, source.coplanarity_truth);
      if (source.has_collinearity_truth)
        append_truth(descriptor, source.collinearity_truth);
      bases_.push_back(std::move(descriptor));
      record_candidate_base(request.key, bases_.back().witnesses);
    }

    for (std::size_t i = 0; i < edge_facet_stage_->request_graph.requests.size(); ++i) {
      const auto &request = edge_facet_stage_->request_graph.requests[i];
      const auto &source = edge_facet_stage_->relations[i];
      std::array<std::uint64_t, 2> endpoint_vertices{};
      if (!candidate_source_edge_facet_detail::source_edge_vertices(
              *candidates_, request.key.first, endpoint_vertices))
        return fail(error, relation_subcode::source_edge_facet_malformed,
                    "Component 07 source-vertex/facet endpoint lineage is unavailable",
                    relation_checkpoint::source_facet_region_evaluation);
      relation_request_proposal proposal;
      proposal.key = request.key;
      proposal.candidate_witnesses =
          request_witnesses(edge_facet_stage_->request_graph, request);
      for (std::size_t endpoint = 0; endpoint < 2; ++endpoint) {
        const auto vertex_key =
            source_vertex_facet_request_key(
                request.key.semantic_namespace, request.key.first.operand,
                endpoint_vertices[endpoint], request.key.second);
        proposal.dependencies.push_back(vertex_key);
        const auto vertex_relation = std::find_if(
            bases_.begin(), bases_.end(), [&](const base_descriptor &candidate) {
              return candidate.key == vertex_key;
            });
        if (vertex_relation == bases_.end() ||
            vertex_relation->truth.size() != 1 ||
            encode_relation_truth_record_semantics(
                vertex_relation->truth.front()) !=
                encode_relation_truth_record_semantics(
                    source.endpoint_support_truth[endpoint]) ||
            vertex_relation->has_vertex_region !=
                source.has_endpoint_region[endpoint] ||
            (vertex_relation->has_vertex_region &&
             !same_source_facet_region(
                 vertex_relation->vertex_region,
                 source.endpoint_regions[endpoint])))
          return fail(error, relation_subcode::truth_layer_mismatch,
                      "Component 07 edge/facet endpoint did not reuse its source-vertex/facet authority",
                      relation_checkpoint::producer_verification);
      }
      for (const auto dependency : source.boundary_relation_requests) {
        if (dependency.ordinal() >= edge_stage_->request_graph.requests.size())
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 edge/facet boundary dependency is out of range",
                      relation_checkpoint::dependency_closure);
        proposal.dependencies.push_back(
            edge_stage_->request_graph.requests[dependency.ordinal()].key);
      }
      proposals_.push_back(std::move(proposal));
      base_descriptor descriptor;
      descriptor.kind = base_kind::edge_facet;
      descriptor.ordinal = i;
      descriptor.key = request.key;
      descriptor.family = feature_relation_family::source_edge_source_facet;
      descriptor.status = edge_facet_status(source.contact);
      descriptor.scope = request.key.scope;
      descriptor.witnesses =
          request_witnesses(edge_facet_stage_->request_graph, request);
      for (const auto &truth : source.endpoint_support_truth)
        append_truth(descriptor, truth);
      for (const auto &event : source.events)
        descriptor.numeric_crossing += event.numeric_crossing;
      bases_.push_back(std::move(descriptor));
      record_candidate_base(request.key, bases_.back().witnesses);
    }

    for (std::size_t i = 0; i < facet_stage_->request_graph.requests.size(); ++i) {
      const auto &request = facet_stage_->request_graph.requests[i];
      const auto &source = facet_stage_->relations[i];
      relation_request_proposal proposal;
      proposal.key = request.key;
      proposal.candidate_witnesses =
          request_witnesses(facet_stage_->request_graph, request);
      for (const auto dependency : source.edge_facet_consumers) {
        if (dependency.ordinal() >=
            edge_facet_stage_->request_graph.requests.size())
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 facet/facet edge consumer is out of range",
                      relation_checkpoint::dependency_closure);
        proposal.dependencies.push_back(
            edge_facet_stage_->request_graph.requests[dependency.ordinal()].key);
      }
      proposals_.push_back(std::move(proposal));
      base_descriptor descriptor;
      descriptor.kind = base_kind::facet;
      descriptor.ordinal = i;
      descriptor.key = request.key;
      descriptor.family = feature_relation_family::source_facet_source_facet;
      descriptor.status = facet_status(source.classification);
      descriptor.scope = request.key.scope;
      descriptor.witnesses =
          request_witnesses(facet_stage_->request_graph, request);
      append_truth(descriptor, source.parallelism_truth);
      if (source.has_coplanarity_truth)
        append_truth(descriptor, source.coplanarity_truth);
      if (source.has_orientation_truth)
        append_truth(descriptor, source.orientation_truth);
      bases_.push_back(std::move(descriptor));
      record_candidate_base(request.key, bases_.back().witnesses);
    }

    for (std::size_t i = 0; i < overlay_stage_->overlays.size(); ++i) {
      const auto &source = overlay_stage_->overlays[i];
      const auto &link = overlay_stage_->links[i];
      if (link.overlay_ordinal != i ||
          link.support_relation.ordinal() >= facet_stage_->request_graph.requests.size())
        return fail(error, relation_subcode::coplanar_overlay_dependency_missing,
                    "Component 07 overlay support link is malformed",
                    relation_checkpoint::dependency_closure);
      relation_request_key key =
          facet_stage_->request_graph.requests[link.support_relation.ordinal()].key;
      key.family = relation_request_family::coplanar_source_facet_overlay;
      key.directed_use = 0;
      key.occurrence_discriminator = 0;
      relation_request_proposal proposal;
      proposal.key = key;
      proposal.dependencies.push_back(
          facet_stage_->request_graph.requests[link.support_relation.ordinal()].key);
      proposal.candidate_witnesses = request_witnesses(
          facet_stage_->request_graph,
          facet_stage_->request_graph.requests[link.support_relation.ordinal()]);
      for (const auto &boundary : source.boundary_relations) {
        if (boundary.request.ordinal() >= edge_stage_->request_graph.requests.size())
          return fail(error, relation_subcode::coplanar_overlay_dependency_missing,
                      "Component 07 overlay boundary dependency is out of range",
                      relation_checkpoint::dependency_closure);
        proposal.dependencies.push_back(
            edge_stage_->request_graph.requests[boundary.request.ordinal()].key);
      }
      for (const auto &witness : source.vertex_regions) {
        const auto polygon = static_cast<std::size_t>(witness.polygon);
        if (polygon >= 2 ||
            witness.vertex_ordinal >= source.facets[polygon].polygon.size())
          return fail(error, relation_subcode::coplanar_overlay_region_unresolved,
                      "Component 07 coplanar source-vertex/facet witness is malformed",
                      relation_checkpoint::source_facet_region_evaluation);
        const auto &query =
            source.facets[polygon].polygon[witness.vertex_ordinal];
        const auto &query_facet = source.facets[polygon].feature;
        const auto &target_facet = source.facets[1 - polygon].feature;
        const auto vertex_key =
            source_vertex_facet_request_key(
                key.semantic_namespace, query_facet.operand,
                query.source_vertex, target_facet);
        proposal.dependencies.push_back(vertex_key);
        const auto existing = std::find_if(
            bases_.begin(), bases_.end(), [&](const base_descriptor &candidate) {
              return candidate.key == vertex_key;
            });
        if (existing == bases_.end() || !existing->has_vertex_region ||
            !same_source_facet_region(existing->vertex_region,
                                      witness.region))
          return fail(error, relation_subcode::source_facet_region_unresolved,
                      "Component 07 coplanar source-vertex/facet regions conflict",
                      relation_checkpoint::producer_verification);
      }
      proposals_.push_back(std::move(proposal));
      base_descriptor descriptor;
      descriptor.kind = base_kind::overlay;
      descriptor.ordinal = i;
      descriptor.key = key;
      descriptor.family = feature_relation_family::source_facet_source_facet;
      descriptor.status = overlay_status(source.classification);
      descriptor.scope = relation_record_scope::public_source_feature;
      descriptor.witnesses = request_witnesses(
          facet_stage_->request_graph,
          facet_stage_->request_graph.requests[link.support_relation.ordinal()]);
      append_truth(descriptor, source.support_relation.parallelism_truth);
      if (source.support_relation.has_coplanarity_truth)
        append_truth(descriptor, source.support_relation.coplanarity_truth);
      if (source.support_relation.has_orientation_truth)
        append_truth(descriptor, source.support_relation.orientation_truth);
      bases_.push_back(std::move(descriptor));
      record_candidate_base(key, bases_.back().witnesses);
    }

    std::sort(bases_.begin(), bases_.end(),
              [](const base_descriptor &a, const base_descriptor &b) {
                return a.key < b.key;
              });
    std::vector<base_descriptor> merged;
    merged.reserve(bases_.size());
    for (auto &base : bases_) {
      if (merged.empty() || merged.back().key != base.key) {
        merged.push_back(std::move(base));
        continue;
      }
      auto &prior = merged.back();
      if (prior.kind != base_kind::vertex_facet ||
          base.kind != base_kind::vertex_facet ||
          prior.family != base.family || prior.status != base.status ||
          prior.truth.size() != base.truth.size() ||
          prior.has_vertex_region != base.has_vertex_region ||
          !same_geometry_snapshot(prior.vertex_point, base.vertex_point) ||
          (prior.has_vertex_region &&
           !same_source_facet_region(prior.vertex_region,
                                     base.vertex_region)))
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 duplicate source-vertex/facet evidence conflicts",
                    relation_checkpoint::producer_verification);
      for (std::size_t truth = 0; truth < prior.truth.size(); ++truth)
        if (encode_relation_truth_record_semantics(prior.truth[truth]) !=
            encode_relation_truth_record_semantics(base.truth[truth]))
          return fail(error, relation_subcode::truth_layer_mismatch,
                      "Component 07 duplicate source-vertex/facet truth conflicts",
                      relation_checkpoint::producer_verification);
      prior.witnesses.insert(prior.witnesses.end(), base.witnesses.begin(),
                             base.witnesses.end());
      std::sort(prior.witnesses.begin(), prior.witnesses.end());
      prior.witnesses.erase(
          std::unique(prior.witnesses.begin(), prior.witnesses.end()),
          prior.witnesses.end());
    }
    bases_ = std::move(merged);
    for (std::size_t i = 1; i < bases_.size(); ++i)
      if (bases_[i - 1].key == bases_[i].key)
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 final relation producer is duplicated",
                    relation_checkpoint::graph_finalization);
    return true;
  }

  bool discover_primitive_support(bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    imported_keys_.clear();
    for (const auto &base : bases_) {
      std::vector<relation_request_key> imports;
      imports.push_back(imported_geometry_key(
          base.key.semantic_namespace, base.key.first, base.scope));
      if (base.key.second.kind != relation_feature_kind::none)
        imports.push_back(imported_geometry_key(
            base.key.semantic_namespace, base.key.second, base.scope));
      std::sort(imports.begin(), imports.end());
      imports.erase(std::unique(imports.begin(), imports.end()), imports.end());
      for (const auto &key : imports) {
        relation_request_proposal proposal;
        proposal.key = key;
        proposal.candidate_witnesses = base.witnesses;
        proposals_.push_back(std::move(proposal));
        imported_keys_.push_back(key);
      }

      relation_request_proposal closure;
      closure.key = base.key;
      closure.candidate_witnesses = base.witnesses;
      for (std::size_t truth = 0; truth < base.truth.size(); ++truth) {
        if (truth > std::numeric_limits<std::uint32_t>::max())
          return fail(error, relation_subcode::count_overflow,
                      "Component 07 primitive truth ordinal is not representable",
                      relation_checkpoint::count_representability_preflight);
        const auto bounded_key = derived_key(
            base.key, relation_request_family::rounded_bounded_primitive,
            tagged_use(0x10U, static_cast<std::uint8_t>(base.key.family)),
            static_cast<std::uint32_t>(truth));
        relation_request_proposal bounded;
        bounded.key = bounded_key;
        bounded.dependencies = imports;
        bounded.candidate_witnesses = base.witnesses;
        proposals_.push_back(std::move(bounded));
        closure.dependencies.push_back(bounded_key);

        if (base.truth[truth].exact_formula != 0) {
          const auto exact_key = derived_key(
              base.key,
              relation_request_family::exact_stored_coordinate_relation,
              tagged_use(0x11U, static_cast<std::uint8_t>(base.key.family)),
              static_cast<std::uint32_t>(truth));
          relation_request_proposal exact;
          exact.key = exact_key;
          exact.dependencies = imports;
          exact.candidate_witnesses = base.witnesses;
          proposals_.push_back(std::move(exact));
          closure.dependencies.push_back(exact_key);
        }
      }
      proposals_.push_back(std::move(closure));
    }
    std::sort(imported_keys_.begin(), imported_keys_.end());
    imported_keys_.erase(
        std::unique(imported_keys_.begin(), imported_keys_.end()),
        imported_keys_.end());
    return true;
  }


  std::vector<relation_request_key>
  primitive_dependencies(const base_descriptor &base) const {
    using namespace relation_artifact_assembly_detail;
    std::vector<relation_request_key> out;
    out.push_back(imported_geometry_key(base.key.semantic_namespace,
                                        base.key.first, base.scope));
    if (base.key.second.kind != relation_feature_kind::none)
      out.push_back(imported_geometry_key(base.key.semantic_namespace,
                                          base.key.second, base.scope));
    for (std::size_t truth = 0; truth < base.truth.size(); ++truth) {
      const auto ordinal = static_cast<std::uint32_t>(truth);
      out.push_back(derived_key(
          base.key, relation_request_family::rounded_bounded_primitive,
          tagged_use(0x10U, static_cast<std::uint8_t>(base.key.family)),
          ordinal));
      if (base.truth[truth].exact_formula != 0)
        out.push_back(derived_key(
            base.key,
            relation_request_family::exact_stored_coordinate_relation,
            tagged_use(0x11U,
                       static_cast<std::uint8_t>(base.key.family)),
            ordinal));
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
  }

  static bool accepted_residual(const finite_interval<T> &interval,
                                T boundary) noexcept {
    return finite_bits(boundary) && boundary >= T(0) &&
           finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
           !finite_numeric_less(interval.upper(), interval.lower()) &&
           interval.lower() >= -boundary && interval.upper() <= boundary;
  }

  static bool accepted_unit_interval(const finite_interval<T> &interval) noexcept {
    return finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
           !finite_numeric_less(interval.upper(), interval.lower()) &&
           interval.lower() >= T(0) && interval.upper() <= T(1);
  }

  static void set_contributor_bits(
      relation_interval_evidence_record &out,
      const uncertainty_contributors &contributors) noexcept {
    const double values[]{contributors.inherited_a, contributors.inherited_b,
                          contributors.machine_floor,
                          contributors.construction,
                          contributors.conditioning, contributors.conversion,
                          contributors.prior_cleanup,
                          contributors.current_cleanup};
    for (std::size_t i = 0; i < out.contributor_bits.size(); ++i)
      out.contributor_bits[i] =
          static_cast<std::uint64_t>(to_bits(values[i]));
  }

  bool next_interval_occurrence(
      relation_interval_evidence_kind kind,
      std::array<std::uint64_t, 18> &counters, std::uint32_t &out,
      bounded_boolean_error &error) const {
    const auto index = static_cast<std::size_t>(kind);
    if (index == 0 || index >= counters.size() ||
        counters[index] > std::numeric_limits<std::uint32_t>::max())
      return fail(error, relation_subcode::count_overflow,
                  "Component 07 family-04 interval occurrence is not representable",
                  relation_checkpoint::count_representability_preflight);
    out = static_cast<std::uint32_t>(counters[index]++);
    return true;
  }

  bool next_region_occurrence(
      relation_source_facet_region_kind kind,
      std::array<std::uint64_t, 10> &counters, std::uint32_t &out,
      bounded_boolean_error &error) const {
    const auto index = static_cast<std::size_t>(kind);
    if (index == 0 || index >= counters.size() ||
        counters[index] > std::numeric_limits<std::uint32_t>::max())
      return fail(error, relation_subcode::count_overflow,
                  "Component 07 family-04 region occurrence is not representable",
                  relation_checkpoint::count_representability_preflight);
    out = static_cast<std::uint32_t>(counters[index]++);
    return true;
  }

  bool add_interval_evidence(
      const base_descriptor &base, relation_interval_evidence_kind kind,
      std::uint32_t occurrence, std::uint8_t component,
      const finite_interval<T> &interval, bool has_rounded_nominal,
      T rounded_nominal, bool has_parameter_metadata,
      parameter_domain_status domain, T domain_margin,
      exact_relation_status exact_zero, exact_relation_status exact_one,
      const uncertainty_contributors &contributors, std::uint64_t trace_root,
      T comparison_boundary, bool within_authorized_boundary,
      const std::vector<relation_request_key> &dependencies,
      std::vector<relation_request_key> &closure,
      bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    if (!finite_bits(interval.lower()) || !finite_bits(interval.upper()) ||
        finite_numeric_less(interval.upper(), interval.lower()) ||
        (has_rounded_nominal &&
         (!finite_bits(rounded_nominal) || !interval.contains(rounded_nominal))) ||
        !finite_bits(domain_margin) || !finite_bits(comparison_boundary))
      return fail(error, relation_subcode::bounded_operation_invalid,
                  "Component 07 family-04 interval evidence is malformed",
                  relation_checkpoint::source_facet_region_evaluation);

    interval_descriptor descriptor;
    descriptor.key = derived_key(
        base.key, relation_request_family::source_point_source_facet_region,
        tagged_use(0x20U, static_cast<std::uint8_t>(kind)) |
            static_cast<std::uint64_t>(component),
        occurrence);
    descriptor.source_relation = base.key;
    descriptor.value.kind = kind;
    descriptor.value.occurrence = occurrence;
    descriptor.value.component = component;
    descriptor.value.has_rounded_nominal = has_rounded_nominal;
    descriptor.value.has_parameter_metadata = has_parameter_metadata;
    descriptor.value.within_authorized_boundary =
        within_authorized_boundary;
    descriptor.value.rounded_nominal_bits =
        has_rounded_nominal
            ? static_cast<std::uint64_t>(to_bits(rounded_nominal))
            : std::uint64_t{0};
    descriptor.value.lower_bits =
        static_cast<std::uint64_t>(to_bits(interval.lower()));
    descriptor.value.upper_bits =
        static_cast<std::uint64_t>(to_bits(interval.upper()));
    descriptor.value.domain =
        has_parameter_metadata ? domain : parameter_domain_status::invalid;
    descriptor.value.domain_margin_bits =
        has_parameter_metadata
            ? static_cast<std::uint64_t>(to_bits(domain_margin))
            : std::uint64_t{0};
    descriptor.value.exact_zero =
        has_parameter_metadata ? exact_zero
                               : exact_relation_status::unavailable;
    descriptor.value.exact_one =
        has_parameter_metadata ? exact_one
                               : exact_relation_status::unavailable;
    if (has_parameter_metadata)
      set_contributor_bits(descriptor.value, contributors);
    descriptor.value.trace_root = has_parameter_metadata ? trace_root : 0;
    descriptor.value.comparison_boundary_bits =
        static_cast<std::uint64_t>(to_bits(comparison_boundary));
    descriptor.witnesses = base.witnesses;
    if (!valid_relation_request_key(descriptor.key))
      return fail(error, relation_subcode::malformed_request_key,
                  "Component 07 family-04 interval key is malformed",
                  relation_checkpoint::dependency_closure);

    relation_request_proposal proposal;
    proposal.key = descriptor.key;
    proposal.dependencies = dependencies;
    proposal.candidate_witnesses = descriptor.witnesses;
    proposals_.push_back(std::move(proposal));
    closure.push_back(descriptor.key);
    interval_desc_.push_back(std::move(descriptor));
    return true;
  }

  bool add_parameter_evidence(
      const base_descriptor &base, relation_interval_evidence_kind kind,
      std::uint32_t occurrence,
      const source_edge_parameter_evidence<T> &parameter,
      const std::vector<relation_request_key> &dependencies,
      std::vector<relation_request_key> &closure,
      bounded_boolean_error &error) {
    return add_interval_evidence(
        base, kind, occurrence, 0, parameter.enclosure, true,
        parameter.rounded_nominal, true, parameter.domain,
        parameter.domain_margin, parameter.exact_zero, parameter.exact_one,
        parameter.contributors, parameter.trace_root, T(0),
        parameter.domain != parameter_domain_status::invalid, dependencies,
        closure, error);
  }

  bool add_simple_parameter_evidence(
      const base_descriptor &base, relation_interval_evidence_kind kind,
      std::uint32_t occurrence, T rounded,
      const finite_interval<T> &interval,
      const std::vector<relation_request_key> &dependencies,
      std::vector<relation_request_key> &closure,
      bounded_boolean_error &error) {
    return add_interval_evidence(
        base, kind, occurrence, 0, interval, true, rounded, false,
        parameter_domain_status::invalid, T(0),
        exact_relation_status::unavailable,
        exact_relation_status::unavailable, uncertainty_contributors{}, 0,
        T(0), accepted_unit_interval(interval), dependencies, closure, error);
  }

  bool add_plain_interval_evidence(
      const base_descriptor &base, relation_interval_evidence_kind kind,
      std::uint32_t occurrence, std::uint8_t component,
      const finite_interval<T> &interval, T comparison_boundary,
      bool within_authorized_boundary,
      const std::vector<relation_request_key> &dependencies,
      std::vector<relation_request_key> &closure,
      bounded_boolean_error &error) {
    return add_interval_evidence(
        base, kind, occurrence, component, interval, false, T(0), false,
        parameter_domain_status::invalid, T(0),
        exact_relation_status::unavailable,
        exact_relation_status::unavailable, uncertainty_contributors{}, 0,
        comparison_boundary, within_authorized_boundary, dependencies, closure,
        error);
  }

  static void set_region_query(
      relation_source_facet_region_record<T> &out,
      const source_edge_geometry_snapshot<T> &point) noexcept {
    out.query_component_count = 3;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      out.query_nominal_bits[axis] =
          static_cast<std::uint64_t>(to_bits(point.rounded_nominal[axis]));
      out.query_lower_bits[axis] =
          static_cast<std::uint64_t>(to_bits(point.enclosure[axis].lower()));
      out.query_upper_bits[axis] =
          static_cast<std::uint64_t>(to_bits(point.enclosure[axis].upper()));
    }
  }

  static void set_region_query(
      relation_source_facet_region_record<T> &out,
      const projected_source_point<T> &point) noexcept {
    out.query_component_count = 2;
    for (std::size_t axis = 0; axis < 2; ++axis) {
      out.query_nominal_bits[axis] =
          static_cast<std::uint64_t>(to_bits(point.nominal[axis]));
      out.query_lower_bits[axis] = static_cast<std::uint64_t>(
          to_bits(point.enclosure[axis].lower()));
      out.query_upper_bits[axis] = static_cast<std::uint64_t>(
          to_bits(point.enclosure[axis].upper()));
    }
  }

  template <class Point>
  bool add_region_evidence(
      const base_descriptor &base, relation_source_facet_region_kind kind,
      std::uint32_t occurrence, const Point &point,
      const source_facet_point_region_record<T> &region,
      const std::vector<relation_request_key> &dependencies,
      std::vector<relation_request_key> &closure,
      bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    if (!valid_source_facet_point_region_record(region))
      return fail(error, relation_subcode::source_facet_region_unresolved,
                  "Component 07 family-04 region evidence is malformed",
                  relation_checkpoint::source_facet_region_evaluation);
    region_descriptor descriptor;
    descriptor.key = derived_key(
        base.key, relation_request_family::source_point_source_facet_region,
        tagged_use(0x21U, static_cast<std::uint8_t>(kind)), occurrence);
    descriptor.source_relation = base.key;
    descriptor.value.kind = kind;
    descriptor.value.occurrence = occurrence;
    descriptor.value.query_source_identity_valid =
        region.query_source_identity_valid;
    set_region_query(descriptor.value, point);
    descriptor.value.region = region;
    descriptor.witnesses = base.witnesses;
    if (!valid_relation_request_key(descriptor.key))
      return fail(error, relation_subcode::malformed_request_key,
                  "Component 07 family-04 region key is malformed",
                  relation_checkpoint::dependency_closure);
    relation_request_proposal proposal;
    proposal.key = descriptor.key;
    proposal.dependencies = dependencies;
    proposal.candidate_witnesses = descriptor.witnesses;
    proposals_.push_back(std::move(proposal));
    closure.push_back(descriptor.key);
    region_desc_.push_back(std::move(descriptor));
    return true;
  }

  bool add_partition_evidence(
      const base_descriptor &base,
      const source_facet_segment_partition_record<T> &partition,
      relation_source_facet_region_kind breakpoint_region_kind,
      relation_source_facet_region_kind interval_region_kind,
      std::array<std::uint64_t, 18> &interval_counters,
      std::array<std::uint64_t, 10> &region_counters,
      const std::vector<relation_request_key> &dependencies,
      std::vector<relation_request_key> &closure,
      bounded_boolean_error &error) {
    if (!valid_source_facet_segment_partition_record(partition))
      return fail(error, relation_subcode::source_facet_segment_malformed,
                  "Component 07 family-04 source-facet partition is malformed",
                  relation_checkpoint::source_facet_region_evaluation);

    for (const auto &contact : partition.contacts) {
      std::uint32_t occurrence = 0;
      if (!next_interval_occurrence(
              relation_interval_evidence_kind::segment_contact_first_parameter,
              interval_counters, occurrence, error) ||
          !add_simple_parameter_evidence(
              base,
              relation_interval_evidence_kind::segment_contact_first_parameter,
              occurrence, contact.first_rounded_parameter,
              contact.first_parameter, dependencies, closure, error))
        return false;
      if (contact.kind ==
          source_facet_segment_contact_kind::boundary_overlap) {
        if (!next_interval_occurrence(
                relation_interval_evidence_kind::segment_contact_second_parameter,
                interval_counters, occurrence, error) ||
            !add_simple_parameter_evidence(
                base,
                relation_interval_evidence_kind::segment_contact_second_parameter,
                occurrence, contact.second_rounded_parameter,
                contact.second_parameter, dependencies, closure, error))
          return false;
      }
    }

    for (const auto &breakpoint : partition.breakpoints) {
      std::uint32_t interval_occurrence = 0;
      std::uint32_t region_occurrence = 0;
      if (!next_interval_occurrence(
              relation_interval_evidence_kind::segment_breakpoint_parameter,
              interval_counters, interval_occurrence, error) ||
          !add_simple_parameter_evidence(
              base,
              relation_interval_evidence_kind::segment_breakpoint_parameter,
              interval_occurrence, breakpoint.rounded_parameter,
              breakpoint.parameter, dependencies, closure, error) ||
          !next_region_occurrence(breakpoint_region_kind, region_counters,
                                  region_occurrence, error) ||
          !add_region_evidence(base, breakpoint_region_kind,
                               region_occurrence, breakpoint.point,
                               breakpoint.region, dependencies, closure,
                               error))
        return false;
    }

    for (const auto &interval : partition.intervals) {
      std::uint32_t interval_occurrence = 0;
      std::uint32_t region_occurrence = 0;
      if (!next_interval_occurrence(
              relation_interval_evidence_kind::segment_interval_witness_parameter,
              interval_counters, interval_occurrence, error) ||
          !add_simple_parameter_evidence(
              base,
              relation_interval_evidence_kind::segment_interval_witness_parameter,
              interval_occurrence, interval.rounded_witness_parameter,
              interval.witness_parameter, dependencies, closure, error) ||
          !next_region_occurrence(interval_region_kind, region_counters,
                                  region_occurrence, error) ||
          !add_region_evidence(base, interval_region_kind,
                               region_occurrence, interval.witness_point,
                               interval.witness_region, dependencies, closure,
                               error))
        return false;
    }

    for (const auto &witness : partition.triangle_witnesses) {
      std::uint32_t occurrence = 0;
      if (!next_interval_occurrence(
              relation_interval_evidence_kind::segment_triangle_witness_parameter,
              interval_counters, occurrence, error) ||
          !add_plain_interval_evidence(
              base,
              relation_interval_evidence_kind::segment_triangle_witness_parameter,
              occurrence, 0, witness.parameter, T(0),
              accepted_unit_interval(witness.parameter), dependencies,
              closure, error))
        return false;
    }
    return true;
  }

  bool discover_family04_evidence(bounded_boolean_error &error) {
    transverse_evaluated_records_consumed_ = 0;
    interval_desc_.clear();
    region_desc_.clear();
    for (const auto &base : bases_) {
      const auto dependencies = primitive_dependencies(base);
      std::vector<relation_request_key> closure;
      std::array<std::uint64_t, 18> interval_counters{};
      std::array<std::uint64_t, 10> region_counters{};

      switch (base.kind) {
      case base_kind::vertex_facet: {
        if (base.has_vertex_region) {
          std::uint32_t occurrence = 0;
          if (!next_region_occurrence(
                  relation_source_facet_region_kind::source_vertex_source_facet,
                  region_counters, occurrence, error))
            return false;
          region_descriptor descriptor;
          descriptor.key = relation_artifact_assembly_detail::derived_key(
              base.key, relation_request_family::composite_contact,
              relation_artifact_assembly_detail::tagged_use(
                  0x21U,
                  static_cast<std::uint8_t>(
                      relation_source_facet_region_kind::
                          source_vertex_source_facet)),
              occurrence);
          descriptor.source_relation = base.key;
          descriptor.value.kind =
              relation_source_facet_region_kind::source_vertex_source_facet;
          descriptor.value.occurrence = occurrence;
          descriptor.value.query_source_identity_valid = false;
          set_region_query(descriptor.value, base.vertex_point);
          descriptor.value.region = base.vertex_region;
          descriptor.witnesses = base.witnesses;
          relation_request_proposal proposal;
          proposal.key = descriptor.key;
          proposal.dependencies.push_back(base.key);
          proposal.candidate_witnesses = base.witnesses;
          proposals_.push_back(std::move(proposal));
          region_desc_.push_back(std::move(descriptor));
        }
        break;
      }
      case base_kind::edge: {
        if (base.ordinal >= edge_stage_->relations.size())
          return fail(error, relation_subcode::source_edge_relation_malformed,
                      "Component 07 family-04 edge relation is out of range",
                      relation_checkpoint::source_facet_region_evaluation);
        const auto &source = edge_stage_->relations[base.ordinal];
        for (std::uint32_t i = 0; i < source.parameter_count; ++i) {
          std::uint32_t occurrence = 0;
          if (!next_interval_occurrence(
                  relation_interval_evidence_kind::source_edge_first_parameter,
                  interval_counters, occurrence, error) ||
              !add_parameter_evidence(
                  base,
                  relation_interval_evidence_kind::source_edge_first_parameter,
                  occurrence, source.first_parameters[i], dependencies,
                  closure, error) ||
              !next_interval_occurrence(
                  relation_interval_evidence_kind::source_edge_second_parameter,
                  interval_counters, occurrence, error) ||
              !add_parameter_evidence(
                  base,
                  relation_interval_evidence_kind::source_edge_second_parameter,
                  occurrence, source.second_parameters[i], dependencies,
                  closure, error))
            return false;
        }
        for (std::uint32_t point = 0; point < source.point_count; ++point) {
          std::uint32_t first_occurrence = 0;
          std::uint32_t second_occurrence = 0;
          if (!next_interval_occurrence(
                  relation_interval_evidence_kind::source_edge_first_carrier_residual,
                  interval_counters, first_occurrence, error) ||
              !next_interval_occurrence(
                  relation_interval_evidence_kind::source_edge_second_carrier_residual,
                  interval_counters, second_occurrence, error))
            return false;
          for (std::uint8_t axis = 0; axis < 3; ++axis) {
            const auto &first = source.points[point].first_carrier_residual[axis];
            const auto &second =
                source.points[point].second_carrier_residual[axis];
            if (!add_plain_interval_evidence(
                    base,
                    relation_interval_evidence_kind::source_edge_first_carrier_residual,
                    first_occurrence, axis, first, source.residual_boundary,
                    accepted_residual(first, source.residual_boundary),
                    dependencies, closure, error) ||
                !add_plain_interval_evidence(
                    base,
                    relation_interval_evidence_kind::source_edge_second_carrier_residual,
                    second_occurrence, axis, second, source.residual_boundary,
                    accepted_residual(second, source.residual_boundary),
                    dependencies, closure, error))
              return false;
          }
        }
        break;
      }
      case base_kind::edge_facet: {
        if (base.ordinal >= edge_facet_stage_->relations.size())
          return fail(error, relation_subcode::source_edge_facet_malformed,
                      "Component 07 family-04 edge/facet relation is out of range",
                      relation_checkpoint::source_facet_region_evaluation);
        const auto &source = edge_facet_stage_->relations[base.ordinal];
        for (const auto &event : source.events) {
          std::uint32_t parameter_occurrence = 0;
          std::uint32_t residual_occurrence = 0;
          std::uint32_t support_occurrence = 0;
          std::uint32_t region_occurrence = 0;
          if (!next_interval_occurrence(
                  relation_interval_evidence_kind::edge_facet_event_parameter,
                  interval_counters, parameter_occurrence, error) ||
              !add_parameter_evidence(
                  base,
                  relation_interval_evidence_kind::edge_facet_event_parameter,
                  parameter_occurrence, event.parameter, dependencies,
                  closure, error) ||
              !next_interval_occurrence(
                  relation_interval_evidence_kind::edge_facet_edge_carrier_residual,
                  interval_counters, residual_occurrence, error) ||
              !next_interval_occurrence(
                  relation_interval_evidence_kind::edge_facet_support_residual,
                  interval_counters, support_occurrence, error) ||
              !next_region_occurrence(
                  relation_source_facet_region_kind::edge_facet_event,
                  region_counters, region_occurrence, error))
            return false;
          for (std::uint8_t axis = 0; axis < 3; ++axis) {
            const auto &residual =
                event.construction.edge_carrier_residual[axis];
            if (!add_plain_interval_evidence(
                    base,
                    relation_interval_evidence_kind::edge_facet_edge_carrier_residual,
                    residual_occurrence, axis, residual,
                    source.residual_boundary,
                    accepted_residual(residual, source.residual_boundary),
                    dependencies, closure, error))
              return false;
          }
          if (!add_plain_interval_evidence(
                  base,
                  relation_interval_evidence_kind::edge_facet_support_residual,
                  support_occurrence, 0, event.construction.support_residual,
                  source.residual_boundary,
                  accepted_residual(event.construction.support_residual,
                                    source.residual_boundary),
                  dependencies, closure, error) ||
              !add_region_evidence(
                  base, relation_source_facet_region_kind::edge_facet_event,
                  region_occurrence, event.construction.point, event.region,
                  dependencies, closure, error))
            return false;

          for (std::size_t facet_index = 0;
               facet_index < facet_stage_->relations.size(); ++facet_index) {
            const auto &facet_relation = facet_stage_->relations[facet_index];
            if (facet_relation.classification !=
                    source_facet_support_relation_class::transverse ||
                !std::binary_search(facet_relation.edge_facet_consumers.begin(),
                                    facet_relation.edge_facet_consumers.end(),
                                    relation_request_id(base.ordinal)))
              continue;
            if (facet_index >= facet_stage_->request_graph.requests.size())
              return false;
            const auto &carrier_key =
                facet_stage_->request_graph.requests[facet_index].key;
            const transverse_relation_evaluation_key evaluation_key{
                carrier_key, base.key, event.occurrence};
            const auto evaluated = std::lower_bound(
                transverse_stage_->records.begin(), transverse_stage_->records.end(),
                evaluation_key, [](const auto &record, const auto &key) {
                  return record.key < key;
                });
            if (evaluated == transverse_stage_->records.end() ||
                !(evaluated->key == evaluation_key))
              return fail(error,
                          relation_subcode::source_facet_carrier_unresolved,
                          "Component 07 transverse evaluated evidence is absent",
                          relation_checkpoint::construction_validation);
            ++transverse_evaluated_records_consumed_;

            std::uint32_t carrier_parameter_occurrence = 0;
            std::uint32_t carrier_residual_occurrence = 0;
            std::uint32_t first_region_occurrence = 0;
            std::uint32_t second_region_occurrence = 0;
            if (!next_interval_occurrence(
                    relation_interval_evidence_kind::
                        transverse_carrier_parameter,
                    interval_counters, carrier_parameter_occurrence, error) ||
                !next_interval_occurrence(
                    relation_interval_evidence_kind::
                        transverse_carrier_point_residual,
                    interval_counters, carrier_residual_occurrence, error) ||
                !next_region_occurrence(
                    relation_source_facet_region_kind::
                        transverse_carrier_first_facet,
                    region_counters, first_region_occurrence, error) ||
                !next_region_occurrence(
                    relation_source_facet_region_kind::
                        transverse_carrier_second_facet,
                    region_counters, second_region_occurrence, error))
              return false;

            std::vector<relation_request_key> carrier_dependencies{
                base.key, carrier_key};
            interval_descriptor parameter_descriptor;
            parameter_descriptor.key =
                relation_artifact_assembly_detail::derived_key(
                base.key, relation_request_family::authoritative_construction,
                relation_artifact_assembly_detail::tagged_use(0x30U,
                           static_cast<std::uint8_t>(
                               relation_interval_evidence_kind::
                                   transverse_carrier_parameter)),
                carrier_parameter_occurrence);
            parameter_descriptor.source_relation = base.key;
            parameter_descriptor.value.kind =
                relation_interval_evidence_kind::transverse_carrier_parameter;
            parameter_descriptor.value.occurrence =
                carrier_parameter_occurrence;
            parameter_descriptor.value.has_rounded_nominal = true;
            parameter_descriptor.value.rounded_nominal_bits =
                static_cast<std::uint64_t>(
                    to_bits(evaluated->parameter_nominal));
            parameter_descriptor.value.lower_bits = static_cast<std::uint64_t>(
                to_bits(evaluated->parameter.lower()));
            parameter_descriptor.value.upper_bits = static_cast<std::uint64_t>(
                to_bits(evaluated->parameter.upper()));
            set_contributor_bits(parameter_descriptor.value,
                                 evaluated->parameter_contributors);
            parameter_descriptor.value.trace_root =
                evaluated->parameter_trace_root;
            const auto &issued_parameter =
                evaluated->parameter_certificate.issued_outputs[0].identity;
            parameter_descriptor.value.issued_operation =
                issued_parameter.operation;
            parameter_descriptor.value.issued_value =
                issued_parameter.value.ordinal();
            parameter_descriptor.value.issued_ledger_entry =
                issued_parameter.ledger_entry.ordinal();
            for (const auto parent : issued_parameter.ordered_parent_values)
              parameter_descriptor.value.issued_parent_values.push_back(
                  parent.ordinal());
            parameter_descriptor.value.issued_parent_trace_roots =
                issued_parameter.ordered_parent_trace_roots;
            for (const auto parent :
                 issued_parameter.ordered_parent_ledger_entries)
              parameter_descriptor.value.issued_parent_ledger_entries.push_back(
                  parent.ordinal());
            canonical_writer parameter_certificate_writer;
            encode_construction_operation_certificate(
                parameter_certificate_writer,
                evaluated->parameter_certificate);
            parameter_descriptor.value.issued_operation_evidence =
                parameter_certificate_writer.take();
            parameter_descriptor.value.within_authorized_boundary = true;
            parameter_descriptor.witnesses = base.witnesses;
            relation_request_proposal parameter_proposal;
            parameter_proposal.key = parameter_descriptor.key;
            parameter_proposal.dependencies = carrier_dependencies;
            parameter_proposal.candidate_witnesses = base.witnesses;
            proposals_.push_back(std::move(parameter_proposal));
            interval_desc_.push_back(std::move(parameter_descriptor));

            for (std::uint8_t axis = 0; axis < 3; ++axis) {
              interval_descriptor residual_descriptor;
              residual_descriptor.key =
                  relation_artifact_assembly_detail::derived_key(
                  base.key,
                  relation_request_family::authoritative_construction,
                  relation_artifact_assembly_detail::tagged_use(0x31U, axis),
                  carrier_residual_occurrence);
              residual_descriptor.source_relation = base.key;
              residual_descriptor.value.kind = relation_interval_evidence_kind::
                  transverse_carrier_point_residual;
              residual_descriptor.value.occurrence =
                  carrier_residual_occurrence;
              residual_descriptor.value.component = axis;
              residual_descriptor.value.lower_bits =
                  static_cast<std::uint64_t>(
                      to_bits(evaluated->point_carrier_residuals[axis].lower()));
              residual_descriptor.value.upper_bits =
                  static_cast<std::uint64_t>(
                      to_bits(evaluated->point_carrier_residuals[axis].upper()));
              residual_descriptor.value.comparison_boundary_bits =
                  static_cast<std::uint64_t>(to_bits(precision_.tolerance()));
              residual_descriptor.value.within_authorized_boundary = true;
              residual_descriptor.witnesses = base.witnesses;
              relation_request_proposal residual_proposal;
              residual_proposal.key = residual_descriptor.key;
              residual_proposal.dependencies = carrier_dependencies;
              residual_proposal.candidate_witnesses = base.witnesses;
              proposals_.push_back(std::move(residual_proposal));
              interval_desc_.push_back(std::move(residual_descriptor));
            }

            const auto append_carrier_region = [&](
                relation_source_facet_region_kind kind,
                std::uint32_t occurrence,
                const source_facet_point_region_record<T> &region) {
              region_descriptor descriptor;
              descriptor.key = relation_artifact_assembly_detail::derived_key(
                  base.key, relation_request_family::composite_contact,
                  relation_artifact_assembly_detail::tagged_use(
                      0x32U, static_cast<std::uint8_t>(kind)),
                  occurrence);
              descriptor.source_relation = base.key;
              descriptor.value.kind = kind;
              descriptor.value.occurrence = occurrence;
              descriptor.value.query_component_count = 3;
              set_region_query(descriptor.value, event.construction.point);
              descriptor.value.region = region;
              descriptor.witnesses = base.witnesses;
              relation_request_proposal region_proposal;
              region_proposal.key = descriptor.key;
              region_proposal.dependencies = carrier_dependencies;
              region_proposal.candidate_witnesses = base.witnesses;
              proposals_.push_back(std::move(region_proposal));
              region_desc_.push_back(std::move(descriptor));
            };
            append_carrier_region(
                relation_source_facet_region_kind::
                    transverse_carrier_first_facet,
                 first_region_occurrence, evaluated->first_region);
            append_carrier_region(
                relation_source_facet_region_kind::
                    transverse_carrier_second_facet,
                 second_region_occurrence, evaluated->second_region);
            transverse_membership_descriptor membership;
            membership.carrier_relation = carrier_key;
            membership.member_relation = base.key;
            const auto canonical_occurrence = ordered_event_occurrences_.find(
                std::make_pair(relation_request_id(base.ordinal),
                               event.occurrence));
            if (canonical_occurrence == ordered_event_occurrences_.end())
              return fail(error, relation_subcode::missing_dependency,
                          "Component 07 transverse event occurrence is unavailable",
                          relation_checkpoint::construction_validation);
            membership.occurrence = canonical_occurrence->second;
            membership.parameter_occurrence = carrier_parameter_occurrence;
            membership.residual_occurrence = carrier_residual_occurrence;
            membership.first_region_occurrence = first_region_occurrence;
            membership.second_region_occurrence = second_region_occurrence;
            transverse_membership_desc_.push_back(std::move(membership));
          }
        }
        if (source.has_coplanar_partition &&
            !add_partition_evidence(
                base, source.coplanar_partition,
                relation_source_facet_region_kind::edge_facet_partition_breakpoint,
                relation_source_facet_region_kind::edge_facet_partition_interval,
                interval_counters, region_counters, dependencies, closure,
                error))
          return false;
        break;
      }
      case base_kind::facet: {
        if (base.ordinal >= facet_stage_->relations.size())
          return fail(error, relation_subcode::source_facet_relation_malformed,
                      "Component 07 family-04 facet relation is out of range",
                      relation_checkpoint::source_facet_region_evaluation);
        const auto &source = facet_stage_->relations[base.ordinal];
        if (source.has_transverse_carrier) {
          std::uint32_t occurrence = 0;
          if (!next_interval_occurrence(
                  relation_interval_evidence_kind::facet_facet_direction_squared,
                  interval_counters, occurrence, error) ||
              !add_plain_interval_evidence(
                  base,
                  relation_interval_evidence_kind::facet_facet_direction_squared,
                  occurrence, 0, source.transverse_carrier.direction_squared,
                  T(0), source.transverse_carrier.direction_squared.lower() > T(0),
                  dependencies, closure, error))
            return false;
          if (!next_interval_occurrence(
                  relation_interval_evidence_kind::facet_facet_point_plane_residual,
                  interval_counters, occurrence, error))
            return false;
          for (std::uint8_t component = 0; component < 2; ++component) {
            const auto &residual =
                source.transverse_carrier.point_plane_residuals[component];
            if (!add_plain_interval_evidence(
                    base,
                    relation_interval_evidence_kind::facet_facet_point_plane_residual,
                    occurrence, component, residual, source.residual_boundary,
                    accepted_residual(residual, source.residual_boundary),
                    dependencies, closure, error))
              return false;
          }
          if (!next_interval_occurrence(
                  relation_interval_evidence_kind::facet_facet_direction_plane_residual,
                  interval_counters, occurrence, error))
            return false;
          for (std::uint8_t component = 0; component < 2; ++component) {
            const auto &residual =
                source.transverse_carrier.direction_plane_residuals[component];
            if (!add_plain_interval_evidence(
                    base,
                    relation_interval_evidence_kind::facet_facet_direction_plane_residual,
                    occurrence, component, residual, source.residual_boundary,
                    accepted_residual(residual, source.residual_boundary),
                    dependencies, closure, error))
              return false;
          }
        }
        break;
      }
      case base_kind::overlay: {
        if (base.ordinal >= overlay_stage_->overlays.size())
          return fail(error, relation_subcode::coplanar_overlay_malformed,
                      "Component 07 family-04 overlay relation is out of range",
                      relation_checkpoint::source_facet_region_evaluation);
        const auto &source = overlay_stage_->overlays[base.ordinal];
        for (const auto &witness : source.vertex_regions) {
          if (witness.polygon >= source.facets.size() ||
              witness.vertex_ordinal >=
                  source.facets[witness.polygon].polygon.size())
            return fail(error, relation_subcode::coplanar_overlay_region_unresolved,
                        "Component 07 family-04 overlay vertex witness is out of range",
                        relation_checkpoint::source_facet_region_evaluation);
          std::uint32_t occurrence = 0;
          if (!next_region_occurrence(
                  relation_source_facet_region_kind::overlay_vertex_witness,
                  region_counters, occurrence, error) ||
              !add_region_evidence(
                  base,
                  relation_source_facet_region_kind::overlay_vertex_witness,
                  occurrence,
                  source.facets[witness.polygon].polygon[
                      witness.vertex_ordinal],
                  witness.region, dependencies, closure, error))
            return false;
        }
        for (const auto &partition : source.boundary_partitions)
          if (!add_partition_evidence(
                  base, partition.partition,
                  relation_source_facet_region_kind::overlay_partition_breakpoint,
                  relation_source_facet_region_kind::overlay_partition_interval,
                  interval_counters, region_counters, dependencies, closure,
                  error))
            return false;
        break;
      }
      }

      if (!closure.empty()) {
        std::sort(closure.begin(), closure.end());
        closure.erase(std::unique(closure.begin(), closure.end()), closure.end());
        relation_request_proposal consumer;
        consumer.key = base.key;
        consumer.dependencies = closure;
        consumer.candidate_witnesses = base.witnesses;
        proposals_.push_back(std::move(consumer));
      }
    }

    const auto interval_order = [](const interval_descriptor &a,
                                   const interval_descriptor &b) {
      return std::tie(a.source_relation, a.key) <
             std::tie(b.source_relation, b.key);
    };
    const auto region_order = [](const region_descriptor &a,
                                 const region_descriptor &b) {
      return std::tie(a.source_relation, a.key) <
             std::tie(b.source_relation, b.key);
    };
    std::sort(interval_desc_.begin(), interval_desc_.end(), interval_order);
    std::sort(region_desc_.begin(), region_desc_.end(), region_order);
    for (std::size_t i = 1; i < interval_desc_.size(); ++i)
      if (interval_desc_[i - 1].key == interval_desc_[i].key)
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 family-04 interval producer is duplicated",
                    relation_checkpoint::graph_finalization);
    for (std::size_t i = 1; i < region_desc_.size(); ++i)
      if (region_desc_[i - 1].key == region_desc_[i].key)
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 family-04 region producer is duplicated",
                    relation_checkpoint::graph_finalization);
    if (transverse_evaluated_records_consumed_ !=
            transverse_stage_->records.size())
      return fail(error, relation_subcode::missing_dependency,
                  "Component 07 transverse evaluated population was not completely mapped",
                  relation_checkpoint::producer_verification);
    if (interval_desc_.size() > capabilities_.maximum_interval_evidence ||
        region_desc_.size() > capabilities_.maximum_region_records)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 family-04 evidence exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  void record_candidate_base(const relation_request_key &key,
                             const std::vector<candidate_id> &witnesses) {
    for (const auto candidate : witnesses)
      if (candidate.ordinal() < candidate_base_keys_.size())
        candidate_base_keys_[candidate.ordinal()].push_back(key);
  }

  void add_construction_proposal(const construction_descriptor &descriptor) {
    relation_request_proposal proposal;
    proposal.key = descriptor.key;
    proposal.dependencies = descriptor.dependencies;
    proposal.candidate_witnesses = descriptor.witnesses;
    proposals_.push_back(std::move(proposal));
  }

  void add_symbolic_proposals(const symbolic_descriptor &descriptor) {
    relation_request_proposal eligibility;
    eligibility.key = descriptor.eligibility_key;
    eligibility.dependencies.push_back(descriptor.source_relation);
    if (descriptor.has_construction)
      eligibility.dependencies.push_back(descriptor.construction_key);
    if (descriptor.has_multiplicity)
      eligibility.dependencies.push_back(descriptor.multiplicity_key);
    eligibility.candidate_witnesses = descriptor.witnesses;
    proposals_.push_back(std::move(eligibility));

    relation_request_proposal decision;
    decision.key = descriptor.decision_key;
    decision.dependencies.push_back(descriptor.eligibility_key);
    decision.candidate_witnesses = descriptor.witnesses;
    proposals_.push_back(std::move(decision));
  }

  symbolic_rule_key make_symbolic_rule_key(
      relation_family family, orientation_relation orientation,
      operand_id acting_operand, symbolic_ownership_role ownership_role,
      symbolic_half_open_role half_open_role,
      symbolic_transition_orientation transition,
      symbolic_occurrence_class occurrence_class) const noexcept {
    symbolic_rule_key key;
    key.operation = context_.operation;
    key.acting_operand = acting_operand;
    key.relation = family;
    key.orientation = orientation;
    key.ownership_role = ownership_role;
    key.half_open_role = half_open_role;
    key.transition = transition;
    key.occurrence_class = occurrence_class;
    return key;
  }

  void add_symbolic_descriptor(
      const base_descriptor &base, const symbolic_rule_key &rule_key,
      symbolic_relation_subject_kind subject_kind,
      std::uint64_t subject_ordinal, std::uint32_t occurrence,
      const relation_request_key *construction,
      const relation_request_key *multiplicity) {
    using namespace relation_artifact_assembly_detail;
    symbolic_descriptor descriptor;
    descriptor.source_relation = base.key;
    descriptor.occurrence = occurrence;
    descriptor.rule_key = rule_key;
    descriptor.subject_kind = subject_kind;
    descriptor.subject_ordinal = subject_ordinal;
    descriptor.eligibility_key = derived_key(
        base.key, relation_request_family::symbolic_eligibility,
        symbolic_directed_use(12, rule_key, subject_kind), occurrence);
    descriptor.decision_key = derived_key(
        base.key, relation_request_family::symbolic_relation_decision,
        symbolic_directed_use(13, rule_key, subject_kind), occurrence);
    descriptor.witnesses = base.witnesses;
    if (construction) {
      descriptor.construction_key = *construction;
      descriptor.has_construction = true;
    }
    if (multiplicity) {
      descriptor.multiplicity_key = *multiplicity;
      descriptor.has_multiplicity = true;
    }
    symbolics_.push_back(descriptor);
    add_symbolic_proposals(symbolics_.back());
  }

  bool discover_constructions_and_symbolics(bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    using namespace relation_construction_policy_detail;

    const auto append_use =
        [&](const base_descriptor &base, const authority<T> &authority_record,
            const geometry_snapshot<T> &witness_geometry,
            const construction_operation_certificate<T> &witness_certificate,
            relation_construction_precedence witness_precedence,
            std::uint32_t occurrence, feature_relation_family seed_family,
            std::vector<relation_feature_key> incidence, bool emit_seed,
            bool distinct_occurrence) -> bool {
      if (!valid_relation_request_key(authority_record.key))
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 construction authority key is invalid",
                    relation_checkpoint::construction_validation);
      if (!valid_relation_request_key(authority_record.source_relation))
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 construction source relation is invalid",
                    relation_checkpoint::construction_validation);
      if (!valid_geometry(authority_record.geometry))
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 construction authority geometry is invalid",
                     relation_checkpoint::construction_validation);
      if (!valid_construction_operation_certificate(
              authority_record.certificate))
        return fail(error, relation_subcode::bounded_operation_invalid,
                    "Component 07 construction authority certificate is absent",
                    relation_checkpoint::construction_validation);
      if (!valid_geometry(witness_geometry))
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 construction witness geometry is invalid",
                     relation_checkpoint::construction_validation);
      if (!valid_construction_operation_certificate(witness_certificate))
        return fail(error, relation_subcode::bounded_operation_invalid,
                    "Component 07 construction witness certificate is absent",
                    relation_checkpoint::construction_validation);
      if (!compatible_geometry(authority_record.geometry, witness_geometry)) {
        const char *message =
            "Component 07 construction witness is incompatible with its authority";
        switch (witness_precedence) {
        case relation_construction_precedence::accepted_source_vertex:
          message = "Component 07 source-vertex construction witness is incompatible with its authority";
          break;
        case relation_construction_precedence::source_edge_source_edge_point:
          message = "Component 07 edge-edge construction witness is incompatible with its authority";
          break;
        case relation_construction_precedence::source_edge_source_facet_point:
          message = "Component 07 edge-facet construction witness is incompatible with its authority";
          break;
        case relation_construction_precedence::coplanar_overlap_endpoint:
          message = "Component 07 coplanar construction witness is incompatible with its authority";
          break;
        case relation_construction_precedence::source_facet_source_facet_carrier:
          message = "Component 07 facet-carrier construction witness is incompatible with its authority";
          break;
        case relation_construction_precedence::verification_witness:
          break;
        }
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    message, relation_checkpoint::construction_validation);
      }

      construction_descriptor descriptor;
      descriptor.key = authority_record.key;
      descriptor.source_relation = base.key;
      descriptor.authoritative_source_relation =
          authority_record.source_relation;
      descriptor.precedence = authority_record.precedence;
      descriptor.authoritative_source_feature = authority_record.source_feature;
      descriptor.value.precedence = authority_record.precedence;
      descriptor.value.authoritative_source_feature = authority_record.source_feature;
      descriptor.value.defining_feature_count =
          authority_record.key.second.kind == relation_feature_kind::none ? 1 : 2;
      descriptor.value.defining_features[0] = authority_record.key.first;
      if (descriptor.value.defining_feature_count == 2)
        descriptor.value.defining_features[1] = authority_record.key.second;
      set_construction_geometry(descriptor.value, authority_record.geometry);
      if (!set_construction_certificate(descriptor.value,
                                        authority_record.certificate))
        return fail(error, relation_subcode::bounded_operation_invalid,
                    "Component 07 construction authority certificate is invalid",
                    relation_checkpoint::construction_validation);

      descriptor.ledger.precedence = witness_precedence;
      descriptor.ledger.occurrence = occurrence;
      descriptor.ledger.defining_feature_count =
          base.key.second.kind == relation_feature_kind::none ? 1 : 2;
      descriptor.ledger.defining_features[0] = base.key.first;
      if (descriptor.ledger.defining_feature_count == 2)
        descriptor.ledger.defining_features[1] = base.key.second;
      descriptor.ledger.lineage_compatible = true;
      descriptor.ledger.enclosure_compatible = true;
      descriptor.ledger.parameter_compatible = true;
      descriptor.ledger.residual_compatible = true;
      set_construction_ledger_geometry(descriptor.ledger, witness_geometry);
      if (!set_construction_certificate(descriptor.ledger,
                                        witness_certificate))
        return fail(error, relation_subcode::bounded_operation_invalid,
                    "Component 07 construction witness certificate is invalid",
                    relation_checkpoint::construction_validation);

      descriptor.occurrence = occurrence;
      descriptor.seed_family = seed_family;
      descriptor.incidence = std::move(incidence);
      descriptor.emit_seed = emit_seed;
      descriptor.distinct_occurrence = distinct_occurrence;
      descriptor.witnesses = base.witnesses;
      construction_uses_.push_back(std::move(descriptor));
      return true;
    };

    for (const auto &base : bases_) {
      switch (base.kind) {
      case base_kind::vertex_facet:
        break;
      case base_kind::edge: {
        const auto &source = edge_stage_->relations[base.ordinal];
        std::vector<relation_request_key> point_keys;
        for (std::uint32_t point = 0; point < source.point_count; ++point) {
          authority<T> authority_record;
          if (!edge_point_authority(*candidates_, base.key, source, point,
                                    authority_record))
            return fail(error, relation_subcode::source_edge_relation_invariant,
                        "Component 07 edge-point construction authority is unresolved",
                        relation_checkpoint::construction_validation);
          const auto witness_geometry = geometry_from_point(
              source.points[point].point,
              source.points[point].accepted_source_vertex,
              source.points[point].tolerance_compatible);
          const auto witness_precedence =
              source.points[point].accepted_source_vertex
                  ? relation_construction_precedence::accepted_source_vertex
                  : relation_construction_precedence::
                        source_edge_source_edge_point;
          if (!append_use(base, authority_record, witness_geometry,
                          source.points[point].certificate,
                          witness_precedence, point, base.family,
                          {base.key.first, base.key.second},
                          source.contact != source_edge_contact_class::none,
                          source.contact == source_edge_contact_class::equal))
            return false;
          point_keys.push_back(authority_record.key);
        }
        if (source.contact != source_edge_contact_class::none &&
            source.contact != source_edge_contact_class::proper_crossing) {
          if (point_keys.empty()) {
            const auto occurrence_class =
                source.contact == source_edge_contact_class::equal
                    ? symbolic_occurrence_class::shared_source_feature
                    : symbolic_occurrence_class::lower_dimensional_contact;
            const auto rule_key = make_symbolic_rule_key(
                symbolic_family_for_edge<T>(source.contact, nullptr),
                orientation_from_edge(source.orientation),
                base.key.first.operand,
                symbolic_ownership_role::shared_source_feature,
                symbolic_half_open_role::source_edge,
                symbolic_transition_orientation::none, occurrence_class);
            add_symbolic_descriptor(
                base, rule_key, symbolic_relation_subject_kind::relation, 0, 0,
                nullptr, nullptr);
          } else {
            for (std::uint32_t point = 0; point < point_keys.size(); ++point) {
              const auto &construction = source.points[point];
              const bool first_endpoint =
                  construction.first_endpoint_owner_mask != 0;
              const bool second_endpoint =
                  construction.second_endpoint_owner_mask != 0;
              const auto ownership =
                  first_endpoint && second_endpoint
                      ? symbolic_ownership_role::shared_source_feature
                  : second_endpoint
                      ? symbolic_ownership_role::opposite_source_feature
                      : symbolic_ownership_role::acting_source_feature;
              const auto occurrence_class =
                  first_endpoint && second_endpoint
                      ? symbolic_occurrence_class::shared_source_feature
                      : symbolic_occurrence_class::lower_dimensional_contact;
              const auto rule_key = make_symbolic_rule_key(
                  symbolic_family_for_edge(source.contact, &construction),
                  orientation_from_edge(source.orientation),
                  base.key.first.operand, ownership,
                  first_endpoint || second_endpoint
                      ? symbolic_half_open_role::source_endpoint
                      : symbolic_half_open_role::interior,
                  symbolic_transition_orientation::none, occurrence_class);
              add_symbolic_descriptor(
                  base, rule_key,
                  symbolic_relation_subject_kind::event_occurrence, point,
                  point, &point_keys[point], nullptr);
            }
          }
        }
        break;
      }
      case base_kind::edge_facet: {
        const auto &source = edge_facet_stage_->relations[base.ordinal];
        relation_request_key multiplicity = derived_key(
            base.key, relation_request_family::numeric_crossing_multiplicity,
            tagged_use(11, 2), 0);
        relation_request_proposal multiplicity_proposal;
        multiplicity_proposal.key = multiplicity;
        multiplicity_proposal.dependencies.push_back(base.key);
        multiplicity_proposal.candidate_witnesses = base.witnesses;
        proposals_.push_back(std::move(multiplicity_proposal));
        if (!multiplicity_keys_.emplace(base.key, multiplicity).second)
          return fail(error, relation_subcode::incompatible_duplicate_request,
                      "Component 07 multiplicity request is duplicated",
                      relation_checkpoint::crossing_multiplicity);

        for (std::size_t local_event = 0; local_event < source.events.size();
             ++local_event) {
          if (local_event > std::numeric_limits<std::uint32_t>::max())
            return fail(error, relation_subcode::count_overflow,
                        "Component 07 local edge/facet event is not representable",
                        relation_checkpoint::count_representability_preflight);
          const auto &event = source.events[local_event];
          std::uint32_t occurrence = 0;
          if (!canonical_event_occurrence(
                  base, static_cast<std::uint32_t>(local_event), occurrence,
                  error))
            return false;
          authority<T> authority_record;
          if (!edge_facet_event_authority(
                  *candidates_, *edge_stage_, base.key, source, event,
                  occurrence, authority_record))
            return fail(error, relation_subcode::source_edge_facet_invariant,
                        "Component 07 edge/facet construction authority is unresolved",
                        relation_checkpoint::construction_validation);
          const auto witness_geometry = geometry_from_point(
              event.construction.point,
              event.construction.accepted_source_vertex,
              event.construction.tolerance_compatible);
          const auto witness_precedence =
              event.construction.accepted_source_vertex ||
                      event.region.classification ==
                          source_facet_point_region_class::original_vertex
                  ? relation_construction_precedence::accepted_source_vertex
                  : relation_construction_precedence::
                        source_edge_source_facet_point;
          if (!append_use(base, authority_record, witness_geometry,
                          event.construction.certificate,
                          witness_precedence, occurrence, base.family,
                          {base.key.first, base.key.second}, true, false))
            return false;

          if (event.kind != source_edge_facet_event_kind::proper_face_crossing) {
            const auto local_transition = event_local_transition(event);
            const auto transition =
                event.kind == source_edge_facet_event_kind::tangent_contact
                    ? symbolic_transition_orientation::tangent
                : local_transition > 0
                    ? symbolic_transition_orientation::negative_to_positive
                : local_transition < 0
                    ? symbolic_transition_orientation::positive_to_negative
                    : symbolic_transition_orientation::none;
            const bool query_endpoint =
                event.construction.edge_endpoint_owner_mask != 0;
            const bool opposite_vertex =
                event.region.classification ==
                source_facet_point_region_class::original_vertex;
            const bool opposite_edge =
                event.region.classification ==
                source_facet_point_region_class::original_edge;
            const auto ownership =
                query_endpoint && (opposite_vertex || opposite_edge)
                    ? symbolic_ownership_role::shared_source_feature
                : query_endpoint
                    ? symbolic_ownership_role::acting_source_feature
                : opposite_vertex || opposite_edge
                    ? symbolic_ownership_role::opposite_source_feature
                    : symbolic_ownership_role::acting_source_feature;
            const auto rule_key = make_symbolic_rule_key(
                symbolic_family_for_edge_facet(event, source.contact),
                orientation_relation::indeterminate,
                base.key.first.operand, ownership,
                query_endpoint || opposite_vertex
                    ? symbolic_half_open_role::source_endpoint
                : opposite_edge
                    ? symbolic_half_open_role::source_edge
                    : symbolic_half_open_role::interior,
                transition,
                query_endpoint && (opposite_vertex || opposite_edge)
                    ? symbolic_occurrence_class::shared_source_feature
                    : symbolic_occurrence_class::lower_dimensional_contact);
            add_symbolic_descriptor(
                base, rule_key,
                symbolic_relation_subject_kind::event_occurrence, occurrence,
                occurrence, &authority_record.key, &multiplicity);
          }
        }
        break;
      }
      case base_kind::facet: {
        const auto &source = facet_stage_->relations[base.ordinal];
        if (source.has_transverse_carrier) {
          authority<T> authority_record;
          if (!carrier_authority(base.key, source.transverse_carrier,
                                 authority_record))
            return fail(error, relation_subcode::source_facet_carrier_unresolved,
                        "Component 07 facet carrier construction authority is unresolved",
                        relation_checkpoint::construction_validation);
          auto witness_geometry =
              geometry_from_carrier(source.transverse_carrier);
          if (authority_record.geometry.lineage == 0) {
            authority_record.geometry.lineage =
                relation_stable_lineage(base.key, 0x71U);
            witness_geometry.lineage = authority_record.geometry.lineage;
          }
          if (authority_record.geometry.provenance == 0) {
            authority_record.geometry.provenance =
                relation_stable_lineage(base.key, 0x70U);
            witness_geometry.provenance = authority_record.geometry.provenance;
          }
          if (!append_use(
                  base, authority_record, witness_geometry,
                  source.transverse_carrier.certificate,
                  relation_construction_precedence::
                      source_facet_source_facet_carrier,
                  0, base.family, {}, false, false))
            return false;
        }
        if (source.classification ==
                source_facet_support_relation_class::coplanar_same_orientation ||
            source.classification ==
                source_facet_support_relation_class::coplanar_opposite_orientation) {
          for (const auto acting : {operand_id::a, operand_id::b}) {
            const auto rule_key = make_symbolic_rule_key(
                relation_family::coplanar,
                orientation_from_status(base.status), acting,
                symbolic_ownership_role::coincident_sheet_pair,
                symbolic_half_open_role::none,
                symbolic_transition_orientation::none,
                symbolic_occurrence_class::coincident_sheet);
            add_symbolic_descriptor(
                base, rule_key, symbolic_relation_subject_kind::relation, 0, 0,
                nullptr, nullptr);
          }
        }
        break;
      }
      case base_kind::overlay: {
        const auto &source = overlay_stage_->overlays[base.ordinal];
        for (const auto &node : source.event_nodes) {
          if (node.id > std::numeric_limits<std::uint32_t>::max())
            return fail(error, relation_subcode::count_overflow,
                        "Component 07 coplanar event occurrence is not representable",
                        relation_checkpoint::count_representability_preflight);
          authority<T> authority_record;
          if (!overlay_node_authority(*candidates_, *edge_stage_, base.key,
                                      source, node, authority_record))
            return fail(error, relation_subcode::coplanar_overlay_invariant,
                        "Component 07 overlay construction authority is unresolved",
                        relation_checkpoint::construction_validation);
          std::vector<relation_feature_key> incidence{
              base.key.first, base.key.second};
          for (const auto &occurrence : node.occurrences) {
            const auto &facet = source.facets[occurrence.polygon].feature;
            incidence.push_back(sheet_occurrence_feature(
                facet, static_cast<std::uint32_t>(occurrence.polygon)));
          }
          const auto witness_geometry = geometry_from_projected(
              node.representative, source.facets[0].dropped_axis,
              authority_record.precedence ==
                  relation_construction_precedence::accepted_source_vertex);
          if (!append_use(
                  base, authority_record, witness_geometry, node.certificate,
                  relation_construction_precedence::coplanar_overlap_endpoint,
                  static_cast<std::uint32_t>(node.id), base.family,
                  std::move(incidence), true,
                  source.distinct_sheet_occurrences))
            return false;
        }
        if (source.classification != coplanar_facet_overlay_class::disjoint) {
          if (source.overlap_components.empty())
            return fail(error, relation_subcode::coplanar_overlay_invariant,
                        "Component 07 symbolic overlay has no overlap component",
                        relation_checkpoint::symbolic_eligibility);
          for (const auto &component : source.overlap_components) {
            if (component.id > std::numeric_limits<std::uint32_t>::max())
              return fail(error, relation_subcode::count_overflow,
                          "Component 07 symbolic component is not representable",
                          relation_checkpoint::count_representability_preflight);
            const auto component_kind =
                relation_artifact_assembly_detail::final_coplanar_component_kind(
                    component.kind);
            const bool coincident =
                component_kind ==
                    relation_coplanar_component_kind::area_boundary ||
                component_kind ==
                    relation_coplanar_component_kind::coincident_sheet_boundary;
            const auto half_open_role =
                component_kind ==
                    relation_coplanar_component_kind::isolated_point
                    ? symbolic_half_open_role::source_endpoint
                : component_kind ==
                          relation_coplanar_component_kind::boundary_segment
                    ? symbolic_half_open_role::source_edge
                    : symbolic_half_open_role::none;
            for (const auto acting : {operand_id::a, operand_id::b}) {
              const auto rule_key = make_symbolic_rule_key(
                  symbolic_family_for_overlay(source.classification),
                  orientation_from_status(
                      facet_status(source.support_relation.classification)),
                  acting,
                  coincident
                      ? symbolic_ownership_role::coincident_sheet_pair
                      : symbolic_ownership_role::shared_source_feature,
                  half_open_role, symbolic_transition_orientation::none,
                  coincident
                      ? symbolic_occurrence_class::coincident_sheet
                      : symbolic_occurrence_class::lower_dimensional_contact);
              add_symbolic_descriptor(
                  base, rule_key,
                  symbolic_relation_subject_kind::coplanar_component,
                  component.id, static_cast<std::uint32_t>(component.id),
                  nullptr, nullptr);
            }
          }
        }
        break;
      }
      }
    }

    std::sort(
        construction_uses_.begin(), construction_uses_.end(),
        [](const construction_descriptor &a,
           const construction_descriptor &b) {
          return std::tie(a.key, a.precedence,
                          a.authoritative_source_relation, a.source_relation,
                          a.occurrence) <
                 std::tie(b.key, b.precedence,
                          b.authoritative_source_relation, b.source_relation,
                          b.occurrence);
        });

    for (std::size_t begin = 0; begin < construction_uses_.size();) {
      std::size_t end = begin + 1;
      while (end < construction_uses_.size() &&
             construction_uses_[end].key == construction_uses_[begin].key)
        ++end;
      construction_descriptor authoritative = construction_uses_[begin];
      authoritative.dependencies.clear();
      authoritative.witnesses.clear();
      authoritative.emit_seed = false;
      authoritative.incidence.clear();
      authoritative.value.ledger_begin = begin + constructions_desc_.size();
      authoritative.value.ledger_count = (end - begin) + 1;

      const auto authority_geometry =
          construction_geometry_from_record<T>(authoritative.value);
      for (std::size_t index = begin; index < end; ++index) {
        auto &use = construction_uses_[index];
        const auto candidate_geometry =
            construction_geometry_from_record<T>(use.value);
        if (use.precedence != authoritative.precedence ||
            use.authoritative_source_feature !=
                authoritative.authoritative_source_feature ||
            !same_geometry(authority_geometry, candidate_geometry))
          return fail(error, relation_subcode::duplicate_authoritative_producer,
                      "Component 07 construction authorities disagree within one lineage key",
                      relation_checkpoint::construction_validation);
        geometry_snapshot<T> witness;
        witness.kind = use.value.kind;
        witness.coordinate_space = use.ledger.coordinate_space;
        witness.component_count = use.ledger.component_count;
        witness.projection_axis = use.ledger.projection_axis;
        for (std::size_t component = 0;
             component < witness.component_count; ++component) {
          using bits_type = floating_uint_t<T>;
          witness.nominal[component] = from_bits<T>(
              static_cast<bits_type>(use.ledger.nominal_bits[component]));
          witness.lower[component] = from_bits<T>(
              static_cast<bits_type>(use.ledger.lower_bits[component]));
          witness.upper[component] = from_bits<T>(
              static_cast<bits_type>(use.ledger.upper_bits[component]));
        }
        witness.provenance = use.ledger.source_provenance;
        witness.lineage = use.ledger.geometric_lineage;
        witness.accepted_source_vertex = use.ledger.accepted_source_vertex;
        witness.finite = use.ledger.finite;
        witness.tolerance_compatible = use.ledger.tolerance_compatible;
        if (!compatible_geometry(authority_geometry, witness))
          return fail(error, relation_subcode::duplicate_authoritative_producer,
                      "Component 07 secondary construction witness conflicts with authority",
                      relation_checkpoint::construction_validation);
        authoritative.dependencies.push_back(use.source_relation);
        authoritative.dependencies.push_back(
            use.authoritative_source_relation);
        authoritative.witnesses.insert(authoritative.witnesses.end(),
                                       use.witnesses.begin(),
                                       use.witnesses.end());
      }
      std::sort(authoritative.dependencies.begin(),
                authoritative.dependencies.end());
      authoritative.dependencies.erase(
          std::unique(authoritative.dependencies.begin(),
                      authoritative.dependencies.end()),
          authoritative.dependencies.end());
      std::sort(authoritative.witnesses.begin(),
                authoritative.witnesses.end());
      authoritative.witnesses.erase(
          std::unique(authoritative.witnesses.begin(),
                      authoritative.witnesses.end()),
          authoritative.witnesses.end());
      constructions_desc_.push_back(std::move(authoritative));
      add_construction_proposal(constructions_desc_.back());
      begin = end;
    }

    std::sort(symbolics_.begin(), symbolics_.end(),
              [](const symbolic_descriptor &a, const symbolic_descriptor &b) {
                return a.eligibility_key < b.eligibility_key;
              });
    for (std::size_t i = 1; i < symbolics_.size(); ++i)
      if (symbolics_[i - 1].eligibility_key == symbolics_[i].eligibility_key)
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 symbolic eligibility key is duplicated",
                    relation_checkpoint::symbolic_eligibility);

    for (const auto &descriptor : construction_uses_) {
      if (!descriptor.emit_seed)
        continue;
      relation_request_proposal seed;
      seed.key = derived_key(
          descriptor.source_relation, relation_request_family::event_seed,
          descriptor.key.directed_use, descriptor.occurrence);
      seed.dependencies = {descriptor.source_relation, descriptor.key};
      auto symbolic_subject =
          symbolic_relation_subject_kind::event_occurrence;
      std::uint64_t symbolic_subject_ordinal = descriptor.occurrence;
      const auto *seed_base = find_base_descriptor(descriptor.source_relation);
      if (seed_base && seed_base->kind == base_kind::overlay) {
        if (!overlay_component_for_event(descriptor.source_relation,
                                         descriptor.occurrence,
                                         symbolic_subject_ordinal))
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 event-seed symbolic overlap component is absent",
                      relation_checkpoint::symbolic_eligibility);
        symbolic_subject =
            symbolic_relation_subject_kind::coplanar_component;
      }
      const auto *symbolic = find_symbolic_descriptor(
          descriptor.source_relation, symbolic_subject,
          symbolic_subject_ordinal,
          descriptor.source_relation.first.operand);
      if (symbolic)
        seed.dependencies.push_back(symbolic->decision_key);
      seed.candidate_witnesses = descriptor.witnesses;
      seed_request_keys_.push_back(seed.key);
      proposals_.push_back(std::move(seed));
    }
    return true;
  }

  relation_feature_key candidate_edge_feature(
      const canonical_candidate_record<T> &candidate) const {
    const operand_id operand =
        candidate.role == directed_candidate_role::a_edge_b_triangle
            ? operand_id::a
            : operand_id::b;
    const auto &table = candidates_->primitive_table(operand);
    relation_feature_key out;
    out.operand = operand;
    if (candidate.edge.ordinal() >= table.edges.size())
      return out;
    const auto &edge = table.edges[candidate.edge.ordinal()];
    if (edge.edge_class == canonical_edge_class::source_edge) {
      out.kind = relation_feature_kind::source_edge;
      out.primary = edge.semantic_key.primary;
      out.secondary = edge.semantic_key.secondary;
    } else {
      out.kind = relation_feature_kind::facet_internal_diagonal;
      out.primary = edge.source_facet;
      out.secondary = edge.source_diagonal;
    }
    return out;
  }

  relation_feature_key candidate_triangle_feature(
      const canonical_candidate_record<T> &candidate) const {
    const operand_id operand =
        candidate.role == directed_candidate_role::a_edge_b_triangle
            ? operand_id::b
            : operand_id::a;
    const auto &table = candidates_->primitive_table(operand);
    relation_feature_key out;
    out.operand = operand;
    out.kind = relation_feature_kind::source_triangle;
    if (candidate.triangle.ordinal() < table.triangles.size()) {
      const auto &triangle = table.triangles[candidate.triangle.ordinal()];
      out.primary = triangle.source_triangle;
      out.secondary = triangle.source_facet;
    }
    return out;
  }

  bool discover_candidate_dispositions(bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    disposition_desc_.reserve(candidates_->candidates().size());
    for (const auto &candidate : candidates_->candidates()) {
      relation_request_key key;
      key.semantic_namespace = context_.context_digest;
      key.family = relation_request_family::candidate_disposition;
      key.scope = relation_record_scope::bookkeeping_only;
      key.first = candidate_edge_feature(candidate);
      key.second = candidate_triangle_feature(candidate);
      key.directed_use = candidate.id.ordinal();
      key.occurrence_discriminator = 0;
      if (!valid_relation_request_key(key))
        return fail(error, relation_subcode::malformed_request_key,
                    "Component 07 candidate disposition key is malformed",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      disposition_descriptor descriptor;
      descriptor.key = key;
      descriptor.dependencies = candidate_base_keys_[candidate.id.ordinal()];
      std::sort(descriptor.dependencies.begin(), descriptor.dependencies.end());
      descriptor.dependencies.erase(
          std::unique(descriptor.dependencies.begin(), descriptor.dependencies.end()),
          descriptor.dependencies.end());
      descriptor.witnesses.push_back(candidate.id);
      disposition_desc_.push_back(descriptor);
      relation_request_proposal proposal;
      proposal.key = key;
      proposal.dependencies = descriptor.dependencies;
      proposal.candidate_witnesses = descriptor.witnesses;
      proposals_.push_back(std::move(proposal));
    }
    return true;
  }

  bool build_graph(bounded_boolean_error &error) {
    if (proposals_.size() > capabilities_.maximum_requests)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 final request graph exceeds request capacity",
                  relation_checkpoint::count_representability_preflight);
    auto graph = build_relation_request_graph(std::move(proposals_), capabilities_);
    if (!graph.has_value()) {
      error = *graph.error();
      return false;
    }
    graph_ = std::move(*graph.value());
    return true;
  }

  bool publish_imported_geometry(bounded_boolean_error &error) {
    imported_geometry_.clear();
    imported_geometry_.reserve(imported_keys_.size());
    for (const auto &key : imported_keys_) {
      const auto *producer =
          relation_artifact_assembly_detail::find_request(graph_, key);
      if (!producer || key.family !=
                           relation_request_family::imported_source_geometry ||
          key.second.kind != relation_feature_kind::none)
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 imported geometry producer is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      relation_imported_geometry_record record;
      record.id = relation_imported_geometry_id(imported_geometry_.size());
      record.producer = producer->id;
      record.feature = key.first;
      record.scope = key.scope;
      imported_geometry_.push_back(record);
    }
    return true;
  }

  bool publish_relations(bounded_boolean_error &error) {
    truth_records_.clear();
    bounded_primitives_.clear();
    exact_relations_.clear();
    truth_lineage_.clear();
    relations_.clear();
    relations_.reserve(bases_.size());
    for (const auto &descriptor : bases_) {
      const auto *producer =
          relation_artifact_assembly_detail::find_request(graph_, descriptor.key);
      if (!producer)
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 final relation producer is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      feature_relation_record record;
      record.id = feature_relation_id(relations_.size());
      record.producer = producer->id;
      record.family = descriptor.family;
      record.scope = descriptor.scope;
      record.status = descriptor.status;
      record.truth_begin = truth_records_.size();
      record.truth_count = descriptor.truth.size();
      record.numeric_crossing_multiplicity = descriptor.numeric_crossing;
      record.occurrence = descriptor.occurrence;
      relations_.push_back(record);
      for (std::size_t truth = 0; truth < descriptor.truth.size(); ++truth) {
        if (truth > std::numeric_limits<std::uint32_t>::max())
          return fail(error, relation_subcode::count_overflow,
                      "Component 07 primitive truth ordinal is not representable",
                      relation_checkpoint::canonical_id_and_reference_remap);
        const auto &value = descriptor.truth[truth];
        const auto bounded_key = relation_artifact_assembly_detail::derived_key(
            descriptor.key,
            relation_request_family::rounded_bounded_primitive,
            relation_artifact_assembly_detail::tagged_use(
                0x10U, static_cast<std::uint8_t>(descriptor.key.family)),
            static_cast<std::uint32_t>(truth));
        const auto *bounded_producer =
            relation_artifact_assembly_detail::find_request(graph_, bounded_key);
        if (!bounded_producer)
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 bounded primitive producer is absent",
                      relation_checkpoint::canonical_id_and_reference_remap);
        relation_bounded_primitive_record bounded;
        bounded.id = relation_bounded_primitive_id(bounded_primitives_.size());
        bounded.producer = bounded_producer->id;
        bounded.source_relation = record.id;
        bounded.truth_ordinal = static_cast<std::uint32_t>(truth);
        bounded.rounded_nominal_bits = value.rounded_nominal_bits;
        bounded.bounded_sign = value.bounded_sign;
        bounded.disposition = value.disposition;
        bounded.rounded_formula = value.rounded_formula;
        bounded.evidence = value;
        bounded_primitives_.push_back(bounded);

        relation_truth_lineage_record lineage;
        lineage.id = relation_truth_lineage_id(truth_lineage_.size());
        lineage.source_relation = record.id;
        lineage.truth_ordinal = static_cast<std::uint32_t>(truth);
        lineage.bounded_primitive = bounded.id;
        if (value.exact_formula != 0) {
          const auto exact_key = relation_artifact_assembly_detail::derived_key(
              descriptor.key,
              relation_request_family::exact_stored_coordinate_relation,
              relation_artifact_assembly_detail::tagged_use(
                  0x11U, static_cast<std::uint8_t>(descriptor.key.family)),
              static_cast<std::uint32_t>(truth));
          const auto *exact_producer =
              relation_artifact_assembly_detail::find_request(graph_, exact_key);
          if (!exact_producer)
            return fail(error, relation_subcode::missing_dependency,
                        "Component 07 exact relation producer is absent",
                        relation_checkpoint::canonical_id_and_reference_remap);
          relation_exact_relation_record exact;
          exact.id = relation_exact_relation_id(exact_relations_.size());
          exact.producer = exact_producer->id;
          exact.source_relation = record.id;
          exact.truth_ordinal = static_cast<std::uint32_t>(truth);
          exact.status = value.exact_relation;
          exact.exact_formula = value.exact_formula;
          exact.evidence = value;
          exact_relations_.push_back(exact);
          lineage.exact_relation = exact.id;
          lineage.has_exact_relation = true;
        }
        truth_lineage_.push_back(lineage);
        truth_records_.push_back(value);
      }
      if (!relation_ids_.emplace(descriptor.key, record.id).second)
        return fail(error, relation_subcode::incompatible_duplicate_request,
                    "Component 07 final relation key is duplicated",
                    relation_checkpoint::canonical_id_and_reference_remap);
    }
    if (relations_.size() > capabilities_.maximum_relations)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 final relation table exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  bool publish_family04_evidence(bounded_boolean_error &error) {
    interval_evidence_.clear();
    source_facet_regions_.clear();
    interval_evidence_.reserve(interval_desc_.size());
    source_facet_regions_.reserve(region_desc_.size());

    for (const auto &descriptor : interval_desc_) {
      const auto *producer =
          relation_artifact_assembly_detail::find_request(graph_, descriptor.key);
      const auto relation = relation_ids_.find(descriptor.source_relation);
      if (!producer || relation == relation_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 family-04 interval producer or source relation is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      relation_interval_evidence_record record = descriptor.value;
      record.id = relation_interval_evidence_id(interval_evidence_.size());
      record.producer = producer->id;
      record.source_relation = relation->second;
      interval_evidence_.push_back(record);
    }

    for (const auto &descriptor : region_desc_) {
      const auto *producer =
          relation_artifact_assembly_detail::find_request(graph_, descriptor.key);
      const auto relation = relation_ids_.find(descriptor.source_relation);
      if (!producer || relation == relation_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 family-04 region producer or source relation is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      relation_source_facet_region_record<T> record = descriptor.value;
      record.id = relation_source_facet_region_id(source_facet_regions_.size());
      record.producer = producer->id;
      record.source_relation = relation->second;
      source_facet_regions_.push_back(std::move(record));
    }

    if (interval_evidence_.size() > capabilities_.maximum_interval_evidence ||
        source_facet_regions_.size() > capabilities_.maximum_region_records)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 family-04 published evidence exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  bool publish_constructions(bounded_boolean_error &error) {
    struct evidence_range final {
      std::uint64_t begin = 0;
      std::uint64_t count = 0;
    };
    std::vector<evidence_range> interval_ranges(relations_.size());
    std::vector<evidence_range> region_ranges(relations_.size());
    const auto accumulate_range =
        [&](auto &ranges, feature_relation_id relation,
            std::uint64_t index) {
          if (relation.ordinal() >= ranges.size())
            return false;
          auto &range = ranges[relation.ordinal()];
          if (range.count == 0)
            range.begin = index;
          else if (range.begin + range.count != index)
            return false;
          ++range.count;
          return true;
        };
    for (std::size_t index = 0; index < interval_evidence_.size(); ++index)
      if (!accumulate_range(interval_ranges,
                            interval_evidence_[index].source_relation,
                            index))
        return fail(error, relation_subcode::internal_invariant,
                    "Component 07 construction interval evidence is not contiguous",
                    relation_checkpoint::producer_verification);
    for (std::size_t index = 0; index < source_facet_regions_.size(); ++index)
      if (!accumulate_range(region_ranges,
                            source_facet_regions_[index].source_relation,
                            index))
        return fail(error, relation_subcode::internal_invariant,
                    "Component 07 construction region evidence is not contiguous",
                    relation_checkpoint::producer_verification);

    std::uint64_t ledger_count = 0;
    if (!checked_add<std::uint64_t>(
            static_cast<std::uint64_t>(construction_uses_.size()),
            static_cast<std::uint64_t>(constructions_desc_.size()),
            ledger_count) ||
        ledger_count >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()))
      return fail(error, relation_subcode::count_overflow,
                  "Component 07 construction-ledger count is not addressable",
                  relation_checkpoint::count_representability_preflight);

    constructions_.reserve(constructions_desc_.size());
    construction_ledger_.reserve(static_cast<std::size_t>(ledger_count));
    std::size_t use_begin = 0;
    for (const auto &descriptor : constructions_desc_) {
      const auto *producer =
          relation_artifact_assembly_detail::find_request(graph_, descriptor.key);
      const auto source_relation =
          relation_ids_.find(descriptor.authoritative_source_relation);
      if (!producer || source_relation == relation_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 construction producer or authoritative source relation is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      while (use_begin < construction_uses_.size() &&
             construction_uses_[use_begin].key < descriptor.key)
        ++use_begin;
      std::size_t use_end = use_begin;
      while (use_end < construction_uses_.size() &&
             construction_uses_[use_end].key == descriptor.key)
        ++use_end;
      if (use_begin == use_end)
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 construction has no retained witness",
                    relation_checkpoint::producer_verification);

      relation_construction_record record = descriptor.value;
      record.id = relation_construction_id(constructions_.size());
      record.producer = producer->id;
      record.source_relation = source_relation->second;
      record.schema_version = contract_versions::relation_construction_schema;
      record.defining_dependency_begin = producer->dependency_begin;
      record.defining_dependency_count = producer->dependency_count;
      const auto &relation = relations_[record.source_relation.ordinal()];
      record.consumer_begin = producer->reverse_consumer_begin;
      record.consumer_count = producer->reverse_consumer_count;
      record.ledger_begin = construction_ledger_.size();
      record.ledger_count = (use_end - use_begin) + 1;
      record.residual_truth_begin = relation.truth_begin;
      record.residual_truth_count = relation.truth_count;
      record.interval_evidence_begin =
          interval_ranges[record.source_relation.ordinal()].begin;
      record.interval_evidence_count =
          interval_ranges[record.source_relation.ordinal()].count;
      record.source_facet_region_begin =
          region_ranges[record.source_relation.ordinal()].begin;
      record.source_facet_region_count =
          region_ranges[record.source_relation.ordinal()].count;
      constructions_.push_back(record);
      if (!construction_ids_.emplace(descriptor.key, record.id).second)
        return fail(error, relation_subcode::incompatible_duplicate_request,
                    "Component 07 construction key is duplicated",
                    relation_checkpoint::canonical_id_and_reference_remap);

      relation_construction_ledger_record authority_entry;
      authority_entry.id =
          relation_construction_ledger_id(construction_ledger_.size());
      authority_entry.construction = record.id;
      authority_entry.source_relation = record.source_relation;
      authority_entry.precedence = record.precedence;
      authority_entry.coordinate_space = record.coordinate_space;
      authority_entry.compatibility =
          relation_construction_compatibility_disposition::authoritative;
      authority_entry.component_count = record.component_count;
      authority_entry.projection_axis = record.projection_axis;
      authority_entry.nominal_bits = record.nominal_bits;
      authority_entry.lower_bits = record.lower_bits;
      authority_entry.upper_bits = record.upper_bits;
      authority_entry.source_provenance = record.source_provenance;
      authority_entry.geometric_lineage = record.geometric_lineage;
      authority_entry.defining_features = record.defining_features;
      authority_entry.defining_feature_count = record.defining_feature_count;
      authority_entry.formula_version = record.formula_version;
      authority_entry.schema_version =
          contract_versions::relation_construction_ledger_schema;
      authority_entry.precision_trace_root = record.precision_trace_root;
      authority_entry.axis_error_upper_bits = record.axis_error_upper_bits;
      authority_entry.radial_error_upper_bits = record.radial_error_upper_bits;
      authority_entry.denominator_lower_bits = record.denominator_lower_bits;
      authority_entry.denominator_upper_bits = record.denominator_upper_bits;
      authority_entry.conditioning_lower_bits = record.conditioning_lower_bits;
      authority_entry.operation = record.operation;
      authority_entry.conditioning = record.conditioning;
      authority_entry.tolerance = record.tolerance;
      authority_entry.ordered_bounded_inputs = record.ordered_bounded_inputs;
      authority_entry.certificate_evidence = record.certificate_evidence;
      authority_entry.accepted_source_vertex = record.accepted_source_vertex;
      authority_entry.finite = record.finite;
      authority_entry.tolerance_compatible = record.tolerance_compatible;
      authority_entry.authoritative_entry = true;
      authority_entry.lineage_compatible = true;
      authority_entry.enclosure_compatible = true;
      authority_entry.parameter_compatible = true;
      authority_entry.residual_compatible = true;
      authority_entry.precision_evidence_complete =
          record.precision_evidence_complete;
      authority_entry.tolerance_boundary_bits =
          record.tolerance_boundary_bits;
      authority_entry.truth_begin = record.residual_truth_begin;
      authority_entry.truth_count = record.residual_truth_count;
      authority_entry.interval_evidence_begin =
          record.interval_evidence_begin;
      authority_entry.interval_evidence_count =
          record.interval_evidence_count;
      authority_entry.source_facet_region_begin =
          record.source_facet_region_begin;
      authority_entry.source_facet_region_count =
          record.source_facet_region_count;
      construction_ledger_.push_back(authority_entry);

      for (std::size_t index = use_begin; index < use_end; ++index) {
        const auto &use = construction_uses_[index];
        const auto witness_relation = relation_ids_.find(use.source_relation);
        if (witness_relation == relation_ids_.end())
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 construction witness relation is absent",
                      relation_checkpoint::canonical_id_and_reference_remap);
        relation_construction_ledger_record witness = use.ledger;
        witness.id =
            relation_construction_ledger_id(construction_ledger_.size());
        witness.construction = record.id;
        witness.source_relation = witness_relation->second;
        witness.authoritative_entry = false;
        witness.compatibility = relation_construction_compatibility_disposition::
            compatible_witness;
        witness.schema_version =
            contract_versions::relation_construction_ledger_schema;
        const auto &source = relations_[witness.source_relation.ordinal()];
        witness.truth_begin = source.truth_begin;
        witness.truth_count = source.truth_count;
        witness.interval_evidence_begin =
            interval_ranges[witness.source_relation.ordinal()].begin;
        witness.interval_evidence_count =
            interval_ranges[witness.source_relation.ordinal()].count;
        witness.source_facet_region_begin =
            region_ranges[witness.source_relation.ordinal()].begin;
        witness.source_facet_region_count =
            region_ranges[witness.source_relation.ordinal()].count;
        construction_ledger_.push_back(witness);
      }
      use_begin = use_end;
    }
    if (use_begin != construction_uses_.size())
      return fail(error, relation_subcode::internal_invariant,
                  "Component 07 construction witness grouping is incomplete",
                  relation_checkpoint::producer_verification);
    if (constructions_.size() > capabilities_.maximum_constructions ||
        construction_ledger_.size() >
            capabilities_.maximum_construction_ledger)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 construction registry exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  bool publish_coplanar_topology(bounded_boolean_error &error) {
    std::uint64_t nested_count = 0;
    for (const auto &base : bases_) {
      if (base.kind != base_kind::overlay)
        continue;
      if (base.ordinal >= overlay_stage_->overlays.size())
        return fail(error, relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar overlay ordinal is invalid",
                    relation_checkpoint::canonical_id_and_reference_remap);
      const auto relation = relation_ids_.find(base.key);
      if (relation == relation_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 final coplanar relation is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      const auto &source = overlay_stage_->overlays[base.ordinal];
      if (!source.complete_event_lineage ||
          !source.complete_authorized_arc_coverage ||
          !source.complete_overlap_component_assembly)
        return fail(error, relation_subcode::coplanar_overlay_invariant,
                    "Component 07 coplanar topology predecessor is incomplete",
                    relation_checkpoint::producer_verification);

      std::vector<relation_coplanar_event_node_id> node_ids(
          source.event_nodes.size(), relation_coplanar_event_node_id(0));
      for (std::size_t local = 0; local < source.event_nodes.size(); ++local) {
        const auto &node = source.event_nodes[local];
        if (node.id != local || node.occurrences.empty() || node.reserved != 0 ||
            local > std::numeric_limits<std::uint32_t>::max())
          return fail(error, relation_subcode::coplanar_overlay_invariant,
                      "Component 07 coplanar event node is malformed",
                      relation_checkpoint::producer_verification);
        relation_construction_policy_detail::authority<T> node_authority;
        if (!relation_construction_policy_detail::overlay_node_authority(
                *candidates_, *edge_stage_, base.key, source, node,
                node_authority))
          return fail(error, relation_subcode::coplanar_overlay_invariant,
                      "Component 07 coplanar node authority is unresolved",
                      relation_checkpoint::construction_validation);
        const auto construction = construction_ids_.find(node_authority.key);
        if (construction == construction_ids_.end())
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 coplanar node construction is absent",
                      relation_checkpoint::canonical_id_and_reference_remap);

        relation_coplanar_event_node_record record;
        record.id = relation_coplanar_event_node_id(coplanar_event_nodes_.size());
        record.overlay_relation = relation->second;
        record.representative = construction->second;
        record.distinct_sheet_occurrences = source.distinct_sheet_occurrences;
        record.occurrences.reserve(node.occurrences.size());
        for (const auto &occurrence : node.occurrences) {
          if (occurrence.polygon > 1 || occurrence.reserved8 != 0 ||
              occurrence.reserved16 != 0 || occurrence.reserved32 != 0 ||
              !checked_add<std::uint64_t>(nested_count, 1, nested_count))
            return fail(error, relation_subcode::coplanar_overlay_invariant,
                        "Component 07 coplanar node occurrence is malformed",
                        relation_checkpoint::producer_verification);
          relation_coplanar_event_occurrence_record published;
          published.polygon = occurrence.polygon;
          published.edge_ordinal = occurrence.edge_ordinal;
          published.breakpoint_ordinal = occurrence.breakpoint_ordinal;
          published.query_source_vertex_valid =
              occurrence.query_source_vertex_valid;
          published.query_source_vertex = occurrence.query_source_vertex;
          published.event_lineages.reserve(occurrence.event_lineages.size());
          for (const auto &lineage : occurrence.event_lineages) {
            if (lineage.reserved8 != 0 || lineage.reserved16 != 0 ||
                !checked_add<std::uint64_t>(nested_count, 1, nested_count))
              return fail(error, relation_subcode::coplanar_overlay_invariant,
                          "Component 07 coplanar event lineage is malformed",
                          relation_checkpoint::producer_verification);
            published.event_lineages.push_back(
                {lineage.contact_lineage, lineage.endpoint_role, 0, 0});
          }
          record.sheet_mask = static_cast<std::uint8_t>(
              record.sheet_mask | (std::uint8_t{1} << occurrence.polygon));
          record.occurrences.push_back(std::move(published));
        }
        node_ids[local] = record.id;
        coplanar_event_nodes_.push_back(std::move(record));
      }

      std::vector<relation_coplanar_oriented_arc_id> arc_ids(
          source.oriented_arcs.size(), relation_coplanar_oriented_arc_id(0));
      for (std::size_t local = 0; local < source.oriented_arcs.size(); ++local) {
        const auto &arc = source.oriented_arcs[local];
        if (arc.id != local || arc.start_node >= node_ids.size() ||
            arc.end_node >= node_ids.size() || arc.start_node == arc.end_node ||
            arc.occurrences.empty() || arc.reserved8 != 0 ||
            arc.reserved16 != 0 || arc.reserved32 != 0)
          return fail(error, relation_subcode::coplanar_overlay_invariant,
                      "Component 07 coplanar oriented arc is malformed",
                      relation_checkpoint::producer_verification);
        relation_coplanar_oriented_arc_record record;
        record.id =
            relation_coplanar_oriented_arc_id(coplanar_oriented_arcs_.size());
        record.overlay_relation = relation->second;
        record.kind =
            relation_artifact_assembly_detail::final_coplanar_arc_kind(arc.kind);
        record.start_node = node_ids[arc.start_node];
        record.end_node = node_ids[arc.end_node];
        record.occurrences.reserve(arc.occurrences.size());
        for (const auto &occurrence : arc.occurrences) {
          if (occurrence.polygon > 1 || occurrence.start_node >= node_ids.size() ||
              occurrence.end_node >= node_ids.size() ||
              occurrence.start_node == occurrence.end_node ||
              occurrence.reserved8 != 0 || occurrence.reserved16 != 0 ||
              occurrence.reserved32 != 0 ||
              !checked_add<std::uint64_t>(nested_count, 1, nested_count))
            return fail(error, relation_subcode::coplanar_overlay_invariant,
                        "Component 07 coplanar arc occurrence is malformed",
                        relation_checkpoint::producer_verification);
          relation_coplanar_arc_occurrence_record published;
          published.polygon = occurrence.polygon;
          published.edge_ordinal = occurrence.edge_ordinal;
          published.interval_ordinal = occurrence.interval_ordinal;
          published.start_node = node_ids[occurrence.start_node];
          published.end_node = node_ids[occurrence.end_node];
          published.forward_along_source_edge =
              occurrence.forward_along_source_edge;
          record.sheet_mask = static_cast<std::uint8_t>(
              record.sheet_mask | (std::uint8_t{1} << occurrence.polygon));
          record.occurrences.push_back(published);
        }
        record.overlap_lineages.reserve(arc.overlap_lineages.size());
        for (const auto lineage : arc.overlap_lineages) {
          if (lineage.ordinal() >= edge_stage_->request_graph.requests.size() ||
              !checked_add<std::uint64_t>(nested_count, 1, nested_count))
            return fail(error, relation_subcode::coplanar_overlay_dependency_missing,
                        "Component 07 coplanar arc lineage is out of range",
                        relation_checkpoint::dependency_closure);
          const auto &lineage_key =
              edge_stage_->request_graph.requests[lineage.ordinal()].key;
          const auto *published =
              relation_artifact_assembly_detail::find_request(graph_, lineage_key);
          if (!published)
            return fail(error, relation_subcode::missing_dependency,
                        "Component 07 coplanar arc lineage is absent from final graph",
                        relation_checkpoint::canonical_id_and_reference_remap);
          record.overlap_lineages.push_back(published->id);
        }
        arc_ids[local] = record.id;
        coplanar_oriented_arcs_.push_back(std::move(record));
      }

      for (std::size_t local = 0; local < source.overlap_components.size(); ++local) {
        const auto &component = source.overlap_components[local];
        if (component.id != local || component.node_ids.empty() ||
            component.reserved16 != 0 || component.reserved32 != 0)
          return fail(error, relation_subcode::coplanar_overlay_invariant,
                      "Component 07 coplanar overlap component is malformed",
                      relation_checkpoint::producer_verification);
        relation_coplanar_overlap_component_record record;
        record.id = relation_coplanar_overlap_component_id(
            coplanar_overlap_components_.size());
        record.overlay_relation = relation->second;
        record.kind = relation_artifact_assembly_detail::
            final_coplanar_component_kind(component.kind);
        record.sheet_mask = component.sheet_mask;
        record.closed = component.closed;
        record.distinct_sheet_occurrences = source.distinct_sheet_occurrences;
        record.node_ids.reserve(component.node_ids.size());
        for (const auto local_node : component.node_ids) {
          if (local_node >= node_ids.size() ||
              !checked_add<std::uint64_t>(nested_count, 1, nested_count))
            return fail(error, relation_subcode::coplanar_overlay_invariant,
                        "Component 07 coplanar component node is out of range",
                        relation_checkpoint::producer_verification);
          record.node_ids.push_back(node_ids[local_node]);
        }
        record.arc_ids.reserve(component.arc_ids.size());
        for (const auto local_arc : component.arc_ids) {
          if (local_arc >= arc_ids.size() ||
              !checked_add<std::uint64_t>(nested_count, 1, nested_count))
            return fail(error, relation_subcode::coplanar_overlay_invariant,
                        "Component 07 coplanar component arc is out of range",
                        relation_checkpoint::producer_verification);
          record.arc_ids.push_back(arc_ids[local_arc]);
        }
        coplanar_overlap_components_.push_back(std::move(record));
      }
    }
    if (coplanar_event_nodes_.size() > capabilities_.maximum_relations ||
        coplanar_oriented_arcs_.size() > capabilities_.maximum_relations ||
        coplanar_overlap_components_.size() > capabilities_.maximum_relations ||
        nested_count > capabilities_.maximum_dependencies)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 final coplanar topology exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  std::int8_t event_local_transition(
      const source_edge_facet_event_record<T> &event) const noexcept {
    const auto occupancy = [](source_edge_facet_occupancy_state state) {
      return state == source_edge_facet_occupancy_state::occupied
                 ? std::int8_t{1}
             : state == source_edge_facet_occupancy_state::unoccupied
                 ? std::int8_t{0}
                 : std::int8_t{2};
    };
    const auto before = occupancy(event.before);
    const auto after = occupancy(event.after);
    return before <= 1 && after <= 1
               ? static_cast<std::int8_t>(after - before)
               : std::int8_t{0};
  }

  bool source_fan_key(const base_descriptor &base,
                      const source_edge_facet_event_record<T> &event,
                      std::uint32_t occurrence, source_fan_group_key &key,
                      bounded_boolean_error &error) const {
    key = source_fan_group_key{};
    key.query_edge = base.key.first;
    key.opposite_operand = base.key.second.operand;
    const bool source_boundary =
        event.region.classification ==
            source_facet_point_region_class::original_edge ||
        event.region.classification ==
            source_facet_point_region_class::original_vertex;
    if (!source_boundary) {
      key.singleton_relation = base.key;
      key.singleton_occurrence = occurrence;
      return true;
    }
    if (event.kind != source_edge_facet_event_kind::boundary_crossing &&
        event.kind != source_edge_facet_event_kind::tangent_contact)
      return fail(error, relation_subcode::crossing_fan_incomplete,
                  "Component 07 source-boundary event has an unsupported fan role",
                  relation_checkpoint::crossing_multiplicity);
    key.boundary_group = true;
    if (event.region.classification ==
        source_facet_point_region_class::original_vertex) {
      if (event.region.source_vertex_owners.size() != 1)
        return fail(error, relation_subcode::crossing_fan_incomplete,
                    "Component 07 source-vertex fan lineage is incomplete",
                    relation_checkpoint::crossing_multiplicity);
      key.boundary_kind = 2;
      key.owner_primary = event.region.source_vertex_owners.front();
      return true;
    }
    if (event.region.classification !=
            source_facet_point_region_class::original_edge ||
        event.region.source_edge_owners.empty())
      return fail(error, relation_subcode::crossing_fan_incomplete,
                  "Component 07 source-edge fan lineage is incomplete",
                  relation_checkpoint::crossing_multiplicity);
    key.boundary_kind = 1;
    const auto canonical_endpoints = [](const auto &owner) {
      return std::minmax(owner.origin_source_vertex,
                         owner.destination_source_vertex);
    };
    const auto first = canonical_endpoints(event.region.source_edge_owners.front());
    key.owner_primary = first.first;
    key.owner_secondary = first.second;
    for (const auto &owner : event.region.source_edge_owners) {
      const auto endpoints = canonical_endpoints(owner);
      if (endpoints.first != key.owner_primary ||
          endpoints.second != key.owner_secondary)
        return fail(error, relation_subcode::crossing_fan_incomplete,
                    "Component 07 boundary event names incompatible source edges",
                    relation_checkpoint::crossing_multiplicity);
    }
    return true;
  }

  bool source_facet_feature_for_topology(
      const canonical_halfedge_operand<T, I> &topology,
      std::uint64_t source_facet,
      relation_feature_key &feature) const noexcept {
    if (source_facet >= topology.source_facet_to_group().size())
      return false;
    const auto group = topology.source_facet_to_group()[source_facet];
    if (group >= topology.facet_groups().size())
      return false;
    const auto &record = topology.facet_groups()[group];
    if (record.canonical_id != group || record.source_facet != source_facet)
      return false;
    feature = relation_feature_key{};
    feature.operand = topology.operand();
    feature.kind = relation_feature_kind::source_facet;
    feature.primary = source_facet;
    feature.secondary = record.ring;
    return valid_relation_feature_key(feature);
  }

  bool expected_source_fan_facets(
      const source_fan_group_key &key,
      std::vector<relation_feature_key> &facets) const {
    facets.clear();
    if (!key.boundary_group || !candidates_ || !candidates_->manifolds())
      return false;
    const auto topology =
        key.opposite_operand == operand_id::a
            ? candidates_->manifolds()->a()
            : candidates_->manifolds()->b();
    if (!topology || topology->operand() != key.opposite_operand)
      return false;

    if (key.boundary_kind == 1) {
      const auto wanted = std::minmax(key.owner_primary, key.owner_secondary);
      const canonical_manifold_edge_record<T> *match = nullptr;
      for (const auto &edge : topology->edges()) {
        if (edge.edge_class != canonical_edge_class::source_edge ||
            !edge.source_feature_owner ||
            edge.halfedges[0] >= topology->halfedges().size())
          continue;
        const auto &halfedge = topology->halfedges()[edge.halfedges[0]];
        const auto endpoints =
            std::minmax(halfedge.source_origin, halfedge.source_destination);
        if (endpoints.first == wanted.first &&
            endpoints.second == wanted.second) {
          if (match)
            return false;
          match = &edge;
        }
      }
      if (!match)
        return false;
      for (const auto source_facet : match->facets) {
        relation_feature_key feature;
        if (!source_facet_feature_for_topology(*topology, source_facet,
                                               feature))
          return false;
        facets.push_back(feature);
      }
    } else if (key.boundary_kind == 2) {
      if (key.owner_primary >= topology->source_vertex_to_vertex().size())
        return false;
      const auto vertex =
          topology->source_vertex_to_vertex()[key.owner_primary];
      if (vertex >= topology->vertices().size())
        return false;
      const auto fan = topology->vertices()[vertex].fan;
      if (fan >= topology->fans().size())
        return false;
      for (const auto halfedge_id : topology->fans()[fan].outgoing_halfedges) {
        if (halfedge_id >= topology->halfedges().size())
          return false;
        relation_feature_key feature;
        if (!source_facet_feature_for_topology(
                *topology, topology->halfedges()[halfedge_id].source_facet,
                feature))
          return false;
        facets.push_back(feature);
      }
    } else {
      return false;
    }
    std::sort(facets.begin(), facets.end());
    facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
    return facets.size() >= 2;
  }

  const base_descriptor *find_base_descriptor(
      const relation_request_key &key) const noexcept {
    const auto it = std::lower_bound(
        bases_.begin(), bases_.end(), key,
        [](const base_descriptor &record,
           const relation_request_key &candidate) {
          return record.key < candidate;
        });
    return it == bases_.end() || it->key != key ? nullptr : &*it;
  }

  bool overlay_component_for_event(const relation_request_key &key,
                                   std::uint32_t event,
                                   std::uint64_t &component) const noexcept {
    const auto *base = find_base_descriptor(key);
    if (!base || base->kind != base_kind::overlay ||
        base->ordinal >= overlay_stage_->overlays.size())
      return false;
    const auto &overlay = overlay_stage_->overlays[base->ordinal];
    bool found = false;
    for (const auto &candidate : overlay.overlap_components) {
      if (std::find(candidate.node_ids.begin(), candidate.node_ids.end(),
                    static_cast<std::uint64_t>(event)) ==
          candidate.node_ids.end())
        continue;
      if (found)
        return false;
      found = true;
      component = candidate.id;
    }
    return found;
  }

  bool set_truth_symbolic_evidence(
      const relation_truth_record &truth, symbolic_eligibility_reason reason,
      symbolic_eligibility_record &eligibility) const noexcept {
    if (!relation_artifact_assembly_detail::usable_truth(truth) ||
        truth.exact_relation != exact_relation_status::exact_zero ||
        truth.exact_formula == 0)
      return false;
    eligibility.exact_relation = truth.exact_relation;
    eligibility.reason = reason;
    eligibility.evidence_formula_version = truth.exact_formula;
    eligibility.exact_lineage_tie = true;
    eligibility.rounded_nominal_zero =
        from_bits<T>(static_cast<floating_uint_t<T>>(
            truth.rounded_nominal_bits)) == T(0);
    eligibility.inherited_uncertainty =
        truth.bounded_sign == bounded_sign_status::overlaps_boundary;
    return true;
  }

  bool set_region_symbolic_evidence(
      const source_facet_point_region_record<T> &region,
      symbolic_eligibility_record &eligibility) const noexcept {
    if (!region.boundary_ownership_resolved ||
        (region.classification !=
             source_facet_point_region_class::original_edge &&
         region.classification !=
             source_facet_point_region_class::original_vertex))
      return false;
    bool exact = true;
    bool uncertain = false;
    std::uint16_t formula = 0;
    const auto accept_owner = [&](const source_facet_boundary_edge_owner &owner) {
      if (owner.edge_ordinal >= region.orientation_evidence.size())
        return false;
      const auto &evidence = region.orientation_evidence[owner.edge_ordinal];
      if (evidence.exact_sign != 0 || evidence.formula_version == 0)
        return false;
      if (formula == 0)
        formula = evidence.formula_version;
      else if (formula != evidence.formula_version)
        return false;
      uncertain = uncertain ||
                  evidence.bounded_sign == bounded_planar_sign::uncertain;
      return true;
    };
    for (const auto &owner : region.source_edge_owners)
      exact = exact && accept_owner(owner);
    if (!exact || formula == 0 || region.source_edge_owners.empty())
      return false;
    eligibility.exact_relation = exact_relation_status::exact_zero;
    eligibility.reason =
        region.classification == source_facet_point_region_class::original_vertex
            ? symbolic_eligibility_reason::shared_source_endpoint
            : symbolic_eligibility_reason::collinear_source_edge_lineage;
    eligibility.evidence_formula_version = formula;
    eligibility.exact_lineage_tie = true;
    eligibility.rounded_nominal_zero = false;
    eligibility.inherited_uncertainty = uncertain;
    return true;
  }

  bool build_symbolic_eligibility(
      const symbolic_descriptor &descriptor,
      symbolic_eligibility_record &eligibility,
      bounded_boolean_error &error) const {
    eligibility = symbolic_eligibility_record{};
    eligibility.request = descriptor.eligibility_key;
    const auto *base = find_base_descriptor(descriptor.source_relation);
    if (!base)
      return fail(error, relation_subcode::missing_dependency,
                  "Component 07 symbolic source relation is absent",
                  relation_checkpoint::symbolic_eligibility);

    bool evidence = false;
    switch (base->kind) {
    case base_kind::vertex_facet:
      break;
    case base_kind::edge: {
      if (base->ordinal >= edge_stage_->relations.size())
        break;
      const auto &source = edge_stage_->relations[base->ordinal];
      const source_edge_point_construction<T> *point = nullptr;
      if (descriptor.has_construction) {
        if (descriptor.occurrence >= source.point_count)
          break;
        point = &source.points[descriptor.occurrence];
      }
      if (source.contact == source_edge_contact_class::partial_overlap ||
          source.contact == source_edge_contact_class::first_contains_second ||
          source.contact == source_edge_contact_class::second_contains_first ||
          source.contact == source_edge_contact_class::equal) {
        evidence = source.has_collinearity_truth &&
                   set_truth_symbolic_evidence(
                       source.collinearity_truth,
                       source.contact == source_edge_contact_class::equal
                           ? symbolic_eligibility_reason::equal_source_feature_lineage
                           : symbolic_eligibility_reason::collinear_source_edge_lineage,
                       eligibility);
      } else if (source.contact == source_edge_contact_class::endpoint_contact ||
                 source.contact == source_edge_contact_class::point_contact) {
        const auto reason =
            point && point->first_endpoint_owner_mask != 0 &&
                    point->second_endpoint_owner_mask != 0
                ? symbolic_eligibility_reason::shared_source_endpoint
                : symbolic_eligibility_reason::exact_formula_zero;
        if (source.has_collinearity_truth &&
            source.collinearity_truth.exact_relation ==
                exact_relation_status::exact_zero)
          evidence = set_truth_symbolic_evidence(
              source.collinearity_truth, reason, eligibility);
        else if (source.has_coplanarity_truth)
          evidence = set_truth_symbolic_evidence(
              source.coplanarity_truth, reason, eligibility);
      }
      break;
    }
    case base_kind::edge_facet: {
      if (base->ordinal >= edge_facet_stage_->relations.size())
        break;
      const auto &source = edge_facet_stage_->relations[base->ordinal];
      const source_edge_facet_event_record<T> *event = nullptr;
      for (const auto &candidate : source.events)
        if (candidate.occurrence == descriptor.occurrence) {
          if (event)
            return fail(error,
                        relation_subcode::incompatible_duplicate_request,
                        "Component 07 symbolic edge/facet occurrence is duplicated",
                        relation_checkpoint::symbolic_eligibility);
          event = &candidate;
        }
      if (!event)
        break;
      if (event->region.classification ==
              source_facet_point_region_class::original_edge ||
          event->region.classification ==
              source_facet_point_region_class::original_vertex)
        evidence = set_region_symbolic_evidence(event->region, eligibility);
      else {
        for (const auto &truth : source.endpoint_support_truth)
          if (!evidence &&
              truth.exact_relation == exact_relation_status::exact_zero)
            evidence = set_truth_symbolic_evidence(
                truth, symbolic_eligibility_reason::exact_formula_zero,
                eligibility);
      }
      break;
    }
    case base_kind::facet: {
      if (base->ordinal >= facet_stage_->relations.size())
        break;
      const auto &source = facet_stage_->relations[base->ordinal];
      evidence = source.has_coplanarity_truth &&
                 set_truth_symbolic_evidence(
                     source.coplanarity_truth,
                     symbolic_eligibility_reason::coplanar_source_facet_lineage,
                     eligibility);
      break;
    }
    case base_kind::overlay: {
      if (base->ordinal >= overlay_stage_->overlays.size())
        break;
      const auto &source = overlay_stage_->overlays[base->ordinal];
      evidence = source.support_relation.has_coplanarity_truth &&
                 set_truth_symbolic_evidence(
                     source.support_relation.coplanarity_truth,
                     symbolic_eligibility_reason::coincident_source_contract,
                     eligibility);
      break;
    }
    }

    eligibility.representational_tie_evidence = false;
    eligibility.structural_category_eligible = evidence;
    eligibility.tolerance_compatible =
        evidence && (!descriptor.has_construction ||
                     construction_tolerance_compatible(
                         descriptor.construction_key));
    eligibility.separated_realizations_possible = false;
    eligibility.owner_is_original_source_feature =
        base->key.first.kind != relation_feature_kind::facet_internal_diagonal &&
        base->key.second.kind != relation_feature_kind::facet_internal_diagonal;
    if (!evidence || !eligibility.tolerance_compatible ||
        !eligibility.owner_is_original_source_feature)
      return fail(error, relation_subcode::symbolic_ineligible,
                  "Component 07 symbolic request lacks qualified exact and structural evidence",
                  relation_checkpoint::symbolic_eligibility);
    return true;
  }

  bool publish_symbolics_and_crossings(bounded_boolean_error &error) {
    eligibility_.reserve(symbolics_.size());
    decisions_.reserve(symbolics_.size());
    for (const auto &descriptor : symbolics_) {
      symbolic_eligibility_record eligibility;
      if (!build_symbolic_eligibility(descriptor, eligibility, error))
        return false;
      auto decision = resolve_symbolic_relation_decision(
          context_.symbolic, descriptor.rule_key, descriptor.subject_kind,
          descriptor.subject_ordinal, eligibility);
      if (!decision.has_value()) {
        error = *decision.error();
        return false;
      }
      decision.value()->id = symbolic_relation_decision_id(decisions_.size());
      eligibility_.push_back(eligibility);
      decisions_.push_back(std::move(*decision.value()));
      if (!decision_ids_
               .emplace(std::make_tuple(descriptor.source_relation,
                                        descriptor.subject_kind,
                                        descriptor.subject_ordinal,
                                        descriptor.rule_key.acting_operand),
                        decisions_.back().id)
               .second)
        return fail(error, relation_subcode::incompatible_duplicate_request,
                    "Component 07 symbolic decision occurrence is duplicated",
                    relation_checkpoint::symbolic_matrix_lookup);
    }

    std::vector<crossing_descriptor> pending;
    for (const auto &base : bases_) {
      if (base.kind != base_kind::edge_facet)
        continue;
      const auto &source = edge_facet_stage_->relations[base.ordinal];
      const auto relation = relation_ids_.find(base.key);
      if (relation == relation_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 crossing source relation is absent",
                    relation_checkpoint::crossing_multiplicity);
      for (std::size_t local_event = 0; local_event < source.events.size();
           ++local_event) {
        if (local_event > std::numeric_limits<std::uint32_t>::max())
          return fail(error, relation_subcode::count_overflow,
                      "Component 07 local crossing event is not representable",
                      relation_checkpoint::count_representability_preflight);
        const auto &event = source.events[local_event];
        crossing_descriptor descriptor;
        if (!canonical_event_occurrence(
                base, static_cast<std::uint32_t>(local_event),
                descriptor.occurrence, error) ||
            !source_fan_key(base, event, descriptor.occurrence,
                            descriptor.group, error))
          return false;
        descriptor.source_relation = base.key;
        descriptor.relation = relation->second;
        descriptor.event = &event;
        descriptor.local_transition = event_local_transition(event);
        descriptor.half_open_owner = base.key.first.operand;
        const auto decision = decision_ids_.find(std::make_tuple(
            base.key, symbolic_relation_subject_kind::event_occurrence,
            static_cast<std::uint64_t>(descriptor.occurrence),
            base.key.first.operand));
        if (decision != decision_ids_.end()) {
          const auto &symbolic = decisions_[decision->second.ordinal()];
          descriptor.symbolic_crossing =
              symbolic.symbolic_crossing_contribution;
          descriptor.half_open_owner = symbolic.half_open_owner;
        }
        pending.push_back(descriptor);
      }
    }
    std::sort(pending.begin(), pending.end(),
              [](const crossing_descriptor &a,
                 const crossing_descriptor &b) {
                return std::tie(a.group, a.source_relation, a.occurrence) <
                       std::tie(b.group, b.source_relation, b.occurrence);
              });

    for (auto &relation : relations_)
      if (relation.family ==
          feature_relation_family::source_edge_source_facet)
        relation.numeric_crossing_multiplicity = 0;

    for (std::size_t begin = 0, group = 0; begin < pending.size(); ++group) {
      std::size_t end = begin + 1;
      while (end < pending.size() && pending[end].group == pending[begin].group)
        ++end;
      const auto count = end - begin;
      if (count > std::numeric_limits<std::uint32_t>::max())
        return fail(error, relation_subcode::count_overflow,
                    "Component 07 source-fan contribution count is not representable",
                    relation_checkpoint::count_representability_preflight);

      bool complete = true;
      bool has_positive = false;
      bool has_negative = false;
      bool boundary_crossing_group = false;
      bool tangent_group = false;
      for (std::size_t i = begin; i < end; ++i) {
        const auto &event = *pending[i].event;
        if (pending[i].group.boundary_group) {
          boundary_crossing_group =
              boundary_crossing_group ||
              event.kind == source_edge_facet_event_kind::boundary_crossing;
          tangent_group = tangent_group ||
                          event.kind ==
                              source_edge_facet_event_kind::tangent_contact;
          if (event.kind == source_edge_facet_event_kind::boundary_crossing) {
            complete = complete &&
                       (pending[i].local_transition == -1 ||
                        pending[i].local_transition == 1);
            has_positive = has_positive || pending[i].local_transition > 0;
            has_negative = has_negative || pending[i].local_transition < 0;
          } else {
            complete = complete &&
                       event.kind ==
                           source_edge_facet_event_kind::tangent_contact &&
                       pending[i].local_transition == 0 &&
                       event.numeric_crossing == 0;
          }
        } else {
          complete = complete &&
                     (event.kind !=
                          source_edge_facet_event_kind::proper_face_crossing ||
                      (pending[i].local_transition == event.numeric_crossing &&
                       event.numeric_crossing != 0));
        }
        complete = complete &&
                   pending[i].half_open_owner == pending[begin].half_open_owner;
      }
      if (pending[begin].group.boundary_group) {
        std::vector<relation_feature_key> expected_facets;
        std::vector<relation_feature_key> actual_facets;
        if (!expected_source_fan_facets(pending[begin].group,
                                        expected_facets))
          complete = false;
        for (std::size_t i = begin; i < end; ++i)
          actual_facets.push_back(pending[i].source_relation.second);
        std::sort(actual_facets.begin(), actual_facets.end());
        actual_facets.erase(
            std::unique(actual_facets.begin(), actual_facets.end()),
            actual_facets.end());
        complete = complete && count >= 2 &&
                   actual_facets == expected_facets &&
                   boundary_crossing_group != tangent_group;
      }
      if (!complete)
        return fail(error, relation_subcode::crossing_fan_incomplete,
                    "Component 07 source-fan contribution set is incomplete",
                    relation_checkpoint::crossing_multiplicity);

      std::int32_t group_total = 0;
      if (pending[begin].group.boundary_group) {
        if (boundary_crossing_group && has_positive != has_negative)
          group_total = has_positive ? 1 : -1;
      } else {
        group_total = pending[begin].event->numeric_crossing;
      }
      if (group_total < -1 || group_total > 1)
        return fail(error, relation_subcode::crossing_multiplicity_invalid,
                    "Component 07 source-fan crossing total is invalid",
                    relation_checkpoint::crossing_multiplicity);

      const bool has_numeric_owner = group_total != 0;
      for (std::size_t i = begin; i < end; ++i) {
        relation_crossing_record record;
        record.relation = pending[i].relation;
        record.numeric_crossing =
            has_numeric_owner && i == begin ? group_total : 0;
        record.symbolic_crossing =
            i == begin ? pending[i].symbolic_crossing : 0;
        record.half_open_owner = pending[begin].half_open_owner;
        record.occurrence = pending[i].occurrence;
        record.source_fan_group = group;
        record.source_fan_group_size = static_cast<std::uint32_t>(count);
        record.source_fan_group_ordinal =
            static_cast<std::uint32_t>(i - begin);
        record.local_transition = pending[i].local_transition;
        record.numeric_owner = has_numeric_owner && i == begin;
        record.source_fan_resolved = true;
        record.locally_conservative = true;
        crossings_.push_back(record);
        auto &relation = relations_[record.relation.ordinal()];
        std::int64_t sum = static_cast<std::int64_t>(
                               relation.numeric_crossing_multiplicity) +
                           record.numeric_crossing;
        if (sum < std::numeric_limits<std::int32_t>::min() ||
            sum > std::numeric_limits<std::int32_t>::max())
          return fail(error, relation_subcode::count_overflow,
                      "Component 07 relation crossing sum overflowed",
                      relation_checkpoint::crossing_multiplicity);
        relation.numeric_crossing_multiplicity =
            static_cast<std::int32_t>(sum);
      }
      begin = end;
    }
    if (decisions_.size() > capabilities_.maximum_symbolic_decisions)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 symbolic table exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  bool construction_tolerance_compatible(
      const relation_request_key &key) const noexcept {
    const auto it = construction_ids_.find(key);
    return it != construction_ids_.end() &&
           it->second.ordinal() < constructions_.size() &&
           constructions_[it->second.ordinal()].tolerance_compatible;
  }

  bool publish_event_seeds(bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    std::vector<relation_event_seed_proposal> proposals;
    for (const auto &descriptor : construction_uses_) {
      if (!descriptor.emit_seed)
        continue;
      const auto relation = relation_ids_.find(descriptor.source_relation);
      const auto construction = construction_ids_.find(descriptor.key);
      if (relation == relation_ids_.end() ||
          construction == construction_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 event seed source is absent",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      const auto &relation_record = relations_[relation->second.ordinal()];
      const auto &construction_record =
          constructions_[construction->second.ordinal()];
      relation_event_seed_proposal proposal;
      proposal.key.semantic_namespace = context_.context_digest;
      proposal.key.family = descriptor.seed_family;
      proposal.key.first = descriptor.source_relation.first;
      proposal.key.second = descriptor.source_relation.second;
      proposal.key.occurrence = descriptor.occurrence;
      proposal.source_relation = relation->second;
      proposal.construction = construction->second;
      proposal.contact_status = relation_record.status;
      proposal.contact_dimension =
          contact_dimension(relation_record.family, relation_record.status);
      proposal.construction_kind = construction_record.kind;
      proposal.accepted_source_vertex = relation_feature_key{};
      proposal.accepted_source_vertex.operand =
          descriptor.source_relation.first.operand;
      proposal.accepted_source_vertex_reused =
          construction_record.authoritative_source_feature.kind ==
          relation_feature_kind::source_vertex;
      if (proposal.accepted_source_vertex_reused)
        proposal.accepted_source_vertex =
            construction_record.authoritative_source_feature;
      proposal.truth_begin = relation_record.truth_begin;
      proposal.truth_count = relation_record.truth_count;
      proposal.construction_ledger_begin = construction_record.ledger_begin;
      proposal.construction_ledger_count = construction_record.ledger_count;
      proposal.precision_evidence_complete =
          construction_record.finite && construction_record.tolerance_compatible;
      if (construction_record.ledger_begin > construction_ledger_.size() ||
          construction_record.ledger_count >
              construction_ledger_.size() - construction_record.ledger_begin)
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 event seed construction ledger is invalid",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      for (std::uint64_t offset = 0;
           offset < construction_record.ledger_count; ++offset)
        proposal.precision_evidence_complete =
            proposal.precision_evidence_complete &&
            construction_ledger_[construction_record.ledger_begin + offset]
                .precision_evidence_complete;

      proposal.incidence = descriptor.incidence;
      proposal.distinct_occurrence_required = descriptor.distinct_occurrence;
      proposal.half_open_owner = descriptor.source_relation.first.operand;
      auto subject_kind = symbolic_relation_subject_kind::event_occurrence;
      std::uint64_t subject_ordinal = descriptor.occurrence;
      const auto *seed_base = find_base_descriptor(descriptor.source_relation);
      if (seed_base && seed_base->kind == base_kind::overlay) {
        if (!overlay_component_for_event(descriptor.source_relation,
                                         descriptor.occurrence,
                                         subject_ordinal))
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 event seed overlap component is absent",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        subject_kind = symbolic_relation_subject_kind::coplanar_component;
      }
      const auto decision = decision_ids_.find(std::make_tuple(
          descriptor.source_relation, subject_kind, subject_ordinal,
          descriptor.source_relation.first.operand));
      if (decision != decision_ids_.end()) {
        const auto &symbolic = decisions_[decision->second.ordinal()];
        proposal.has_symbolic_decision = true;
        proposal.symbolic_decision = symbolic.id;
        proposal.symbolic_rule_ordinal = symbolic.stable_rule_ordinal;
        proposal.symbolic_exchange_rule_ordinal =
            symbolic.exchange_rule_ordinal;
        proposal.symbolic_subject_kind = symbolic.subject_kind;
        proposal.symbolic_subject_ordinal = symbolic.subject_ordinal;
        proposal.symbolic_occurrence_rank = symbolic.feature_priority;
        proposal.conceptual_side = symbolic.conceptual_side;
        proposal.conceptual_order = symbolic.conceptual_order;
        proposal.symbolic_contact = symbolic.contact_class;
        proposal.symbolic_expected = symbolic.expected_disposition;
        proposal.symbolic_explanation = symbolic.explanation;
        proposal.symbolic_tie_key_schema = symbolic.tie_key_schema;
        proposal.coincident_owner_rank = symbolic.coincident_owner_rank;
        proposal.symbolic_owner_rank_eligible =
            symbolic.owner_rank_eligible;
        proposal.symbolic_crossing =
            symbolic.symbolic_crossing_contribution;
        proposal.half_open_owner = symbolic.half_open_owner;
        proposal.distinct_occurrence_required =
            proposal.distinct_occurrence_required ||
            symbolic.occurrence_separation_required;
      }
      for (const auto &crossing : crossings_)
        if (crossing.relation == proposal.source_relation &&
            crossing.occurrence == descriptor.occurrence) {
          proposal.numeric_crossing += crossing.numeric_crossing;
          proposal.symbolic_crossing = crossing.symbolic_crossing;
          proposal.half_open_owner = crossing.half_open_owner;
        }
      if (proposal.numeric_crossing < -1 || proposal.numeric_crossing > 1)
        return fail(error, relation_subcode::crossing_multiplicity_invalid,
                    "Component 07 event seed crossing is not locally bounded",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);

      for (const auto candidate_id : descriptor.witnesses) {
        if (candidate_id.ordinal() >= candidates_->candidates().size())
          return fail(error, relation_subcode::candidate_disposition_missing,
                      "Component 07 event seed candidate witness is absent",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        const auto &candidate =
            candidates_->candidates()[candidate_id.ordinal()];
        const auto edge_operand =
            candidate.role == directed_candidate_role::a_edge_b_triangle
                ? operand_id::a
                : operand_id::b;
        const auto triangle_operand =
            edge_operand == operand_id::a ? operand_id::b : operand_id::a;
        const auto &edge_table = candidates_->primitive_table(edge_operand);
        const auto &triangle_table =
            candidates_->primitive_table(triangle_operand);
        if (candidate.edge.ordinal() >= edge_table.edges.size() ||
            candidate.triangle.ordinal() >= triangle_table.triangles.size())
          return fail(error, relation_subcode::candidate_disposition_missing,
                      "Component 07 event seed primitive witness is absent",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        const auto &edge = edge_table.edges[candidate.edge.ordinal()];
        const auto &triangle =
            triangle_table.triangles[candidate.triangle.ordinal()];
        relation_event_seed_candidate_incidence_record incidence;
        incidence.candidate = candidate.id;
        incidence.disposition =
            relation_candidate_disposition_id(candidate.id.ordinal());
        incidence.candidate_edge = candidate_edge_feature(candidate);
        incidence.source_triangle = candidate_triangle_feature(candidate);
        incidence.edge_halfedges = {edge.halfedges[0].ordinal(),
                                    edge.halfedges[1].ordinal()};
        incidence.triangle_halfedges = {
            triangle.halfedges[0].ordinal(), triangle.halfedges[1].ordinal(),
            triangle.halfedges[2].ordinal()};
        incidence.internal_diagonal_witness =
            candidate.edge_class == canonical_edge_class::facet_internal_diagonal;
        incidence.source_feature_owner = edge.source_feature_owner;
        proposal.candidate_incidence.push_back(incidence);
      }
      if (!valid_relation_event_seed_key(proposal.key))
        return fail(error, relation_subcode::malformed_request_key,
                    "Component 07 event seed key is malformed",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      proposals.push_back(std::move(proposal));
    }

    auto table = canonicalize_relation_event_seeds(std::move(proposals),
                                                    capabilities_);
    if (!table.has_value()) {
      error = *table.error();
      return false;
    }
    seed_table_ = std::move(*table.value());
    return true;
  }

  bool publish_transverse_carrier_memberships(
      bounded_boolean_error &error) {
    transverse_carrier_memberships_.clear();
    std::sort(transverse_membership_desc_.begin(),
              transverse_membership_desc_.end(), [](const auto &a, const auto &b) {
                return std::tie(a.carrier_relation, a.member_relation,
                                a.occurrence) <
                       std::tie(b.carrier_relation, b.member_relation,
                                b.occurrence);
              });
    for (std::size_t i = 1; i < transverse_membership_desc_.size(); ++i)
      if (std::tie(transverse_membership_desc_[i - 1].carrier_relation,
                   transverse_membership_desc_[i - 1].member_relation,
                   transverse_membership_desc_[i - 1].occurrence) ==
          std::tie(transverse_membership_desc_[i].carrier_relation,
                   transverse_membership_desc_[i].member_relation,
                   transverse_membership_desc_[i].occurrence))
        return fail(error, relation_subcode::duplicate_authoritative_producer,
                    "Component 07 transverse carrier membership is duplicated",
                    relation_checkpoint::producer_verification);

    for (const auto &descriptor : transverse_membership_desc_) {
      const auto carrier_relation = relation_ids_.find(
          descriptor.carrier_relation);
      const auto member_relation = relation_ids_.find(descriptor.member_relation);
      if (carrier_relation == relation_ids_.end() ||
          member_relation == relation_ids_.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 transverse membership relation is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      const relation_construction_record *carrier = nullptr;
      for (const auto &construction : constructions_)
        if (construction.source_relation == carrier_relation->second &&
            construction.kind == relation_construction_kind::bounded_carrier) {
          if (carrier)
            return fail(error,
                        relation_subcode::duplicate_authoritative_producer,
                        "Component 07 transverse membership has multiple carriers",
                        relation_checkpoint::producer_verification);
          carrier = &construction;
        }
      const relation_event_seed_record *seed = nullptr;
      for (const auto &candidate : seed_table_.records)
        if (candidate.source_relation == member_relation->second &&
            candidate.key.occurrence == descriptor.occurrence) {
          if (seed)
            return fail(error,
                        relation_subcode::duplicate_authoritative_producer,
                        "Component 07 transverse membership event seed is ambiguous",
                        relation_checkpoint::producer_verification);
          seed = &candidate;
        }
      const relation_interval_evidence_record *parameter = nullptr;
      std::array<const relation_interval_evidence_record *, 3> residuals{};
      const relation_source_facet_region_record<T> *first_region = nullptr;
      const relation_source_facet_region_record<T> *second_region = nullptr;
      const relation_crossing_record *crossing = nullptr;
      for (const auto &evidence : interval_evidence_) {
        if (evidence.source_relation != member_relation->second)
          continue;
        if (evidence.kind == relation_interval_evidence_kind::
                                 transverse_carrier_parameter &&
            evidence.occurrence == descriptor.parameter_occurrence)
          parameter = &evidence;
        if (evidence.kind == relation_interval_evidence_kind::
                                 transverse_carrier_point_residual &&
            evidence.occurrence == descriptor.residual_occurrence &&
            evidence.component < residuals.size())
          residuals[evidence.component] = &evidence;
      }
      for (const auto &evidence : source_facet_regions_) {
        if (evidence.source_relation != member_relation->second)
          continue;
        if (evidence.kind == relation_source_facet_region_kind::
                                 transverse_carrier_first_facet &&
            evidence.occurrence == descriptor.first_region_occurrence)
          first_region = &evidence;
        if (evidence.kind == relation_source_facet_region_kind::
                                 transverse_carrier_second_facet &&
            evidence.occurrence == descriptor.second_region_occurrence)
          second_region = &evidence;
      }
      for (const auto &candidate : crossings_)
        if (candidate.relation == member_relation->second &&
            candidate.occurrence == descriptor.occurrence) {
          if (crossing)
            return fail(error, relation_subcode::duplicate_authoritative_producer,
                        "Component 07 transverse membership crossing is ambiguous",
                        relation_checkpoint::producer_verification);
          crossing = &candidate;
        }
      if (!carrier || !seed || !crossing || !parameter || !first_region || !second_region ||
          std::find(residuals.begin(), residuals.end(), nullptr) !=
              residuals.end())
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 transverse membership evidence is incomplete",
                    relation_checkpoint::producer_verification);

      relation_transverse_carrier_membership_record record;
      record.id = relation_transverse_carrier_membership_id(
          transverse_carrier_memberships_.size());
      record.carrier_relation = carrier_relation->second;
      record.carrier_construction = carrier->id;
      record.member_relation = member_relation->second;
      record.point_construction = seed->construction;
      record.seed = seed->id;
      record.occurrence = descriptor.occurrence;
      record.parameter = parameter->id;
      for (std::size_t axis = 0; axis < residuals.size(); ++axis)
        record.point_carrier_residuals[axis] = residuals[axis]->id;
      record.first_region = first_region->id;
      record.second_region = second_region->id;
      record.parameter_lineage = parameter->trace_root;
      record.carrier_lineage = carrier->geometric_lineage;
      record.event_lineage = record.point_construction.ordinal() + 1;
      record.numeric_crossing = crossing->numeric_crossing;
      record.local_transition = crossing->local_transition;
      record.numeric_owner = crossing->numeric_owner;
      record.transition = crossing->numeric_owner && crossing->numeric_crossing > 0
                              ? relation_carrier_transition::entering
                          : crossing->numeric_owner && crossing->numeric_crossing < 0
                              ? relation_carrier_transition::leaving
                              : relation_carrier_transition::tangent;
      record.half_open_owner = crossing->half_open_owner;
      record.finite = true;
      record.conditioning_accepted = true;
      record.residuals_accepted = true;
      record.regions_complete = true;
      record.precision_evidence_complete =
          carrier->precision_evidence_complete &&
          seed->precision_evidence_complete;
      if (record.parameter_lineage == 0 || !record.precision_evidence_complete)
        return fail(error, relation_subcode::source_facet_carrier_unresolved,
                    "Component 07 transverse membership precision evidence is incomplete",
                    relation_checkpoint::construction_validation);
      transverse_carrier_memberships_.push_back(record);
    }
    if (transverse_carrier_memberships_.size() >
        capabilities_.maximum_transverse_memberships)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 transverse carrier membership limit exceeded",
                  relation_checkpoint::count_representability_preflight);
    return true;
  }

  const symbolic_descriptor *find_symbolic_descriptor(
      const relation_request_key &source,
      symbolic_relation_subject_kind subject_kind,
      std::uint64_t subject_ordinal, operand_id acting_operand) const {
    for (const auto &descriptor : symbolics_)
      if (descriptor.source_relation == source &&
          descriptor.subject_kind == subject_kind &&
          descriptor.subject_ordinal == subject_ordinal &&
          descriptor.rule_key.acting_operand == acting_operand)
        return &descriptor;
    return nullptr;
  }

  bool publish_candidate_dispositions(bounded_boolean_error &error) {
    std::vector<relation_candidate_disposition_proposal> proposals;
    proposals.reserve(candidates_->candidates().size());
    candidate_relation_coverage_.clear();
    candidate_event_seed_coverage_.clear();
    candidate_partitions_.clear();

    for (const auto &candidate : candidates_->candidates()) {
      relation_candidate_disposition_proposal proposal;
      proposal.candidate = candidate.id;
      proposal.relation_begin = candidate_relation_coverage_.size();
      const auto &keys = candidate_base_keys_[candidate.id.ordinal()];
      std::vector<feature_relation_id> relations;
      for (const auto &key : keys) {
        const auto relation = relation_ids_.find(key);
        if (relation != relation_ids_.end())
          relations.push_back(relation->second);
      }
      std::sort(relations.begin(), relations.end());
      relations.erase(std::unique(relations.begin(), relations.end()),
                      relations.end());
      candidate_relation_coverage_.insert(candidate_relation_coverage_.end(),
                                          relations.begin(), relations.end());
      proposal.relation_count = relations.size();
      proposal.event_seed_begin = candidate_event_seed_coverage_.size();
      std::vector<relation_event_seed_id> seeds;
      for (const auto &incidence : seed_table_.candidate_incidence)
        if (incidence.candidate == candidate.id)
          seeds.push_back(incidence.seed);
      std::sort(seeds.begin(), seeds.end());
      seeds.erase(std::unique(seeds.begin(), seeds.end()), seeds.end());
      candidate_event_seed_coverage_.insert(
          candidate_event_seed_coverage_.end(), seeds.begin(), seeds.end());
      proposal.event_seed_count = seeds.size();

      bool has_public_contact = false;
      bool has_coplanar_or_coincident = false;
      bool has_zero_measure = false;
      bool all_separated = !relations.empty();
      bool has_earlier_witness = false;
      bool has_canonical_contribution = false;
      const feature_relation_record *selected = nullptr;
      for (const auto id : relations) {
        if (id.ordinal() >= relations_.size())
          return fail(error, relation_subcode::missing_dependency,
                      "Component 07 candidate relation coverage is invalid",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        const auto &record = relations_[id.ordinal()];
        all_separated = all_separated &&
                        record.status ==
                            feature_relation_status::definitely_separated;
        if (relation_artifact_assembly_detail::public_contact(record.status)) {
          has_public_contact = true;
          if (!selected || record.producer < selected->producer)
            selected = &record;
        }
        has_coplanar_or_coincident =
            has_coplanar_or_coincident ||
            record.status == feature_relation_status::overlap ||
            record.status == feature_relation_status::containment ||
            record.status ==
                feature_relation_status::coincidence_same_orientation ||
            record.status ==
                feature_relation_status::coincidence_opposite_orientation;
        has_zero_measure =
            has_zero_measure ||
            record.status == feature_relation_status::endpoint_crossing ||
            record.status == feature_relation_status::point_contact ||
            record.status == feature_relation_status::segment_contact ||
            record.status == feature_relation_status::tangency;
        const auto &request =
            graph_.requests[record.producer.ordinal()];
        if (request.witness_count != 0) {
          const auto first =
              graph_.candidate_witnesses[request.witness_begin];
          has_earlier_witness =
              has_earlier_witness || first.ordinal() < candidate.id.ordinal();
          has_canonical_contribution =
              has_canonical_contribution || first == candidate.id;
        }
      }
      for (const auto seed : seeds) {
        const auto &record = seed_table_.records[seed.ordinal()];
        if (record.candidate_incidence_count != 0) {
          const auto &first = seed_table_.candidate_incidence[
              record.candidate_incidence_begin];
          has_earlier_witness = has_earlier_witness ||
                                first.candidate.ordinal() <
                                    candidate.id.ordinal();
          has_canonical_contribution =
              has_canonical_contribution || first.candidate == candidate.id;
        }
      }

      const auto *request = relation_artifact_assembly_detail::find_request(
          graph_, disposition_desc_[candidate.id.ordinal()].key);
      if (!request)
        return fail(error, relation_subcode::missing_dependency,
                    "Component 07 candidate disposition producer is absent",
                    relation_checkpoint::canonical_id_and_reference_remap);
      proposal.bookkeeping_request = request->id;
      if (selected)
        proposal.public_relation = selected->id;

      proposal.coverage_flags = candidate_coverage_complete;
      if (!relations.empty())
        proposal.coverage_flags |= candidate_coverage_relation;
      if (has_public_contact)
        proposal.coverage_flags |= candidate_coverage_public_contact;
      if (!seeds.empty())
        proposal.coverage_flags |= candidate_coverage_event_seed;
      if (has_coplanar_or_coincident)
        proposal.coverage_flags |=
            candidate_coverage_coplanar_or_coincident;
      if (has_zero_measure)
        proposal.coverage_flags |= candidate_coverage_zero_measure;
      if (all_separated)
        proposal.coverage_flags |= candidate_coverage_definitely_separated;
      if (has_earlier_witness && !has_canonical_contribution)
        proposal.coverage_flags |= candidate_coverage_duplicate_discovery;
      if (candidate.edge_class == canonical_edge_class::facet_internal_diagonal)
        proposal.coverage_flags |= candidate_coverage_internal_diagonal;
      proposal.coverage_complete = true;

      if (candidate.edge_class == canonical_edge_class::facet_internal_diagonal) {
        proposal.disposition = candidate_relation_disposition_kind::
            internal_diagonal_bookkeeping_absorbed;
      } else if (has_earlier_witness && !has_canonical_contribution) {
        proposal.disposition = candidate_relation_disposition_kind::
            duplicate_discovery_absorbed;
      } else if (has_coplanar_or_coincident) {
        proposal.disposition = candidate_relation_disposition_kind::
            contributed_coplanar_or_coincident_relation;
      } else if (!seeds.empty()) {
        proposal.disposition =
            candidate_relation_disposition_kind::contributed_event_seeds;
      } else if (has_zero_measure) {
        proposal.disposition = candidate_relation_disposition_kind::
            retained_zero_measure_contact;
      } else if (all_separated) {
        proposal.disposition =
            candidate_relation_disposition_kind::definitely_separated;
      } else {
        proposal.disposition =
            candidate_relation_disposition_kind::primitive_dependency_only;
      }
      proposals.push_back(proposal);
    }
    if (candidate_relation_coverage_.size() >
            capabilities_.maximum_candidate_coverage ||
        candidate_event_seed_coverage_.size() >
            capabilities_.maximum_candidate_coverage)
      return fail(error, relation_subcode::work_limit,
                  "Component 07 candidate coverage exceeds capacity",
                  relation_checkpoint::count_representability_preflight);
    auto records = canonicalize_candidate_dispositions(
        std::move(proposals), candidates_->candidates().size(), capabilities_);
    if (!records.has_value()) {
      error = *records.error();
      return false;
    }
    dispositions_ = std::move(*records.value());

    for (const auto &partition : candidates_->partitions()) {
      if (partition.id.ordinal() != candidate_partitions_.size() ||
          partition.begin > dispositions_.size() ||
          partition.count > dispositions_.size() - partition.begin)
        return fail(error, relation_subcode::candidate_disposition_contradiction,
                    "Component 07 candidate partition is malformed",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      relation_candidate_partition_record record;
      record.id = relation_candidate_partition_id(candidate_partitions_.size());
      record.source_partition = partition.id;
      record.candidate_begin = partition.begin;
      record.candidate_count = partition.count;
      record.disposition_begin = partition.begin;
      record.disposition_count = partition.count;
      record.maximum_records = partition.maximum_records;
      if (partition.count != 0) {
        const auto &first = dispositions_[partition.begin];
        const auto &last =
            dispositions_[partition.begin + partition.count - 1];
        record.relation_begin = first.relation_begin;
        record.relation_count =
            last.relation_begin + last.relation_count - record.relation_begin;
        record.event_seed_begin = first.event_seed_begin;
        record.event_seed_count =
            last.event_seed_begin + last.event_seed_count -
            record.event_seed_begin;
      }
      candidate_partitions_.push_back(record);
    }
    return true;
  }

  bool publish_triangle_local_reconciliation(
      bounded_boolean_error &error) {
    using namespace relation_artifact_assembly_detail;
    triangle_local_reconciliation_.clear();
    triangle_local_reconciliation_.reserve(candidates_->candidates().size());
    const auto &authority = execution_authority_->graph;
    for (const auto &candidate : candidates_->candidates()) {
      const auto edge_operand =
          candidate.role == directed_candidate_role::a_edge_b_triangle
              ? operand_id::a
              : operand_id::b;
      const auto triangle_operand =
          edge_operand == operand_id::a ? operand_id::b : operand_id::a;
      const auto &edge_table = candidates_->primitive_table(edge_operand);
      const auto &triangle_table = candidates_->primitive_table(triangle_operand);
      if (candidate.edge.ordinal() >= edge_table.edges.size() ||
          candidate.triangle.ordinal() >= triangle_table.triangles.size())
        return fail(error, relation_subcode::source_facet_triangle_reconciliation,
                    "Component 07 triangle-local primitive is unavailable",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      const auto &edge = edge_table.edges[candidate.edge.ordinal()];
      const auto &triangle = triangle_table.triangles[candidate.triangle.ordinal()];

      relation_request_key bookkeeping_key;
      bookkeeping_key.semantic_namespace = context_.context_digest;
      bookkeeping_key.family = relation_request_family::source_edge_source_facet;
      bookkeeping_key.scope = relation_record_scope::bookkeeping_only;
      bookkeeping_key.first = candidate_edge_feature(candidate);
      bookkeeping_key.second = candidate_triangle_feature(candidate);
      bookkeeping_key.directed_use = candidate.id.ordinal();
      bookkeeping_key.formula_version = contract_versions::exact_relation_formulas;
      bookkeeping_key.policy_version = contract_versions::relation_request_key_schema;
      const auto *bookkeeping = find_request(authority, bookkeeping_key);
      if (!bookkeeping)
        return fail(error, relation_subcode::source_facet_triangle_reconciliation,
                    "Component 07 triangle-local bookkeeping authority is absent",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);

      const canonical_relation_request *public_composite = nullptr;
      for (const auto &request : authority.requests) {
        if (request.key.family !=
                relation_request_family::source_facet_source_facet ||
            request.witness_begin > authority.candidate_witnesses.size() ||
            request.witness_count >
                authority.candidate_witnesses.size() - request.witness_begin)
          continue;
        bool witnessed = false;
        for (std::uint64_t offset = 0; offset < request.witness_count; ++offset)
          witnessed = witnessed ||
                      authority.candidate_witnesses[request.witness_begin + offset] ==
                          candidate.id;
        if (!witnessed)
          continue;
        if (public_composite &&
            edge.edge_class == canonical_edge_class::facet_internal_diagonal)
          return fail(error,
                      relation_subcode::source_facet_triangle_reconciliation,
                      "Component 07 internal diagonal maps to multiple source-facet composites",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        if (!public_composite)
          public_composite = &request;
      }

      relation_triangle_local_reconciliation_record record;
      record.id = relation_triangle_local_reconciliation_id(
          triangle_local_reconciliation_.size());
      record.candidate = candidate.id;
      record.bookkeeping_request = bookkeeping->id;
      record.discovery_edge = bookkeeping_key.first;
      record.discovery_triangle = bookkeeping_key.second;
      record.edge_halfedges = {edge.halfedges[0].ordinal(),
                               edge.halfedges[1].ordinal()};
      record.triangle_halfedges = {
          triangle.halfedges[0].ordinal(), triangle.halfedges[1].ordinal(),
          triangle.halfedges[2].ordinal()};
      record.internal_diagonal =
          edge.edge_class == canonical_edge_class::facet_internal_diagonal;
      record.source_feature_owner = edge.source_feature_owner;
      record.symbolic_contact_owner = edge.symbolic_contact_owner;
      record.classification_barrier =
          edge.classification_barrier_inside_source_facet;
      record.retained_surface_feature = edge.retained_surface_feature;
      if (record.internal_diagonal &&
          (record.source_feature_owner || record.symbolic_contact_owner ||
           record.classification_barrier || record.retained_surface_feature))
        return fail(error, relation_subcode::source_facet_triangle_reconciliation,
                    "Component 07 internal diagonal acquired public ownership",
                    relation_checkpoint::event_seed_and_disposition_reconciliation);
      if (public_composite) {
        record.public_composite_request = public_composite->id;
        record.owning_source_facet =
            public_composite->key.first.operand == edge_operand
                ? public_composite->key.first
                : public_composite->key.second;
        record.opposite_source_facet =
            public_composite->key.first.operand == triangle_operand
                ? public_composite->key.first
                : public_composite->key.second;
        const auto relation = relation_ids_.find(public_composite->key);
        if (relation == relation_ids_.end())
          return fail(error,
                      relation_subcode::source_facet_triangle_reconciliation,
                      "Component 07 source-facet composite has no public relation",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        record.public_relation = relation->second;
        record.disposition = triangle_local_reconciliation_disposition::
            mapped_to_public_composite;
      } else {
        if (record.internal_diagonal)
          return fail(error,
                      relation_subcode::source_facet_triangle_reconciliation,
                      "Component 07 internal diagonal lacks source-facet reconciliation",
                      relation_checkpoint::event_seed_and_disposition_reconciliation);
        record.disposition =
            triangle_local_reconciliation_disposition::no_public_relation;
        record.no_public_reason = triangle_local_no_public_reason::
            complete_source_facet_classification_has_no_contact;
      }
      record.complete = true;
      triangle_local_reconciliation_.push_back(std::move(record));
    }
    return true;
  }

  void fill_statistics(artifact_type &artifact) const {
    artifact.statistics_.candidate_count = candidates_->candidates().size();
    artifact.statistics_.request_proposal_count =
        artifact.request_graph_.proposal_count;
    artifact.statistics_.unique_request_count =
        artifact.request_graph_.requests.size();
    artifact.statistics_.dependency_count =
        artifact.request_graph_.dependencies.size();
    artifact.statistics_.reverse_consumer_count =
        artifact.request_graph_.reverse_consumers.size();
    artifact.statistics_.candidate_witness_count =
        artifact.request_graph_.candidate_witnesses.size();
    artifact.statistics_.imported_geometry_count =
        artifact.imported_geometry_.size();
    artifact.statistics_.bounded_primitive_count =
        artifact.bounded_primitives_.size();
    artifact.statistics_.exact_relation_count =
        artifact.exact_relations_.size();
    artifact.statistics_.truth_lineage_count =
        artifact.truth_lineage_.size();
    artifact.statistics_.interval_evidence_count =
        artifact.interval_evidence_.size();
    artifact.statistics_.source_facet_region_count =
        artifact.source_facet_regions_.size();
    for (const auto &record : artifact.relations_)
      if (record.scope == relation_record_scope::public_source_feature)
        ++artifact.statistics_.public_relation_count;
      else
        ++artifact.statistics_.bookkeeping_relation_count;
    artifact.statistics_.construction_count = artifact.constructions_.size();
    artifact.statistics_.construction_ledger_count =
        artifact.construction_ledger_.size();
    artifact.statistics_.coplanar_event_node_count =
        artifact.coplanar_event_nodes_.size();
    artifact.statistics_.coplanar_oriented_arc_count =
        artifact.coplanar_oriented_arcs_.size();
    artifact.statistics_.coplanar_overlap_component_count =
        artifact.coplanar_overlap_components_.size();
    artifact.statistics_.symbolic_eligibility_count =
        artifact.symbolic_eligibility_.size();
    artifact.statistics_.symbolic_decision_count =
        artifact.symbolic_decisions_.size();
    artifact.statistics_.crossing_record_count = artifact.crossings_.size();
    artifact.statistics_.event_seed_count = artifact.event_seeds_.size();
    artifact.statistics_.event_seed_candidate_incidence_count =
        artifact.event_seed_candidate_incidence_.size();
    artifact.statistics_.candidate_relation_coverage_count =
        artifact.candidate_relation_coverage_.size();
    artifact.statistics_.candidate_seed_coverage_count =
        artifact.candidate_event_seed_coverage_.size();
    artifact.statistics_.candidate_partition_count =
        artifact.candidate_partitions_.size();
    artifact.statistics_.triangle_local_reconciliation_count =
        artifact.triangle_local_reconciliation_.size();
    artifact.statistics_.transverse_carrier_membership_count =
        artifact.transverse_carrier_memberships_.size();
    artifact.statistics_.sort_comparisons =
        artifact.request_graph_.sort_comparisons;
    artifact.statistics_.verifier_work_units =
        1 + artifact.request_graph_.requests.size() +
        artifact.request_graph_.dependencies.size() +
        artifact.imported_geometry_.size() +
        artifact.bounded_primitives_.size() + artifact.exact_relations_.size() +
        artifact.truth_lineage_.size() + artifact.interval_evidence_.size() +
        artifact.source_facet_regions_.size() + artifact.relations_.size() +
        artifact.constructions_.size() + artifact.construction_ledger_.size() +
        artifact.coplanar_event_nodes_.size() +
        artifact.coplanar_oriented_arcs_.size() +
        artifact.coplanar_overlap_components_.size() +
        artifact.symbolic_decisions_.size() + artifact.crossings_.size() +
        artifact.event_seeds_.size() +
        artifact.transverse_carrier_memberships_.size() +
        artifact.triangle_local_reconciliation_.size() +
        artifact.candidate_dispositions_.size();
  }

  bool publish_downstream_handoff(bounded_boolean_error &error) {
    if (!candidates_->manifolds() || !candidates_->manifolds()->a() ||
        !candidates_->manifolds()->b())
      return fail(error, relation_subcode::predecessor_mismatch,
                  "Component 07 downstream source topology is absent",
                  relation_checkpoint::producer_verification);

    const auto facet_feature = [](const auto &topology,
                                  std::uint64_t source_facet,
                                  relation_feature_key &feature) {
      if (source_facet >= topology.source_facet_to_group().size())
        return false;
      const auto group_id = topology.source_facet_to_group()[source_facet];
      if (group_id >= topology.facet_groups().size())
        return false;
      const auto &group = topology.facet_groups()[group_id];
      if (group.canonical_id != group_id || group.source_facet != source_facet)
        return false;
      feature.operand = topology.operand();
      feature.kind = relation_feature_kind::source_facet;
      feature.primary = source_facet;
      feature.secondary = group.ring;
      return valid_relation_feature_key(feature, false);
    };

    for (const auto operand : {operand_id::a, operand_id::b}) {
      const auto &table = candidates_->primitive_table(operand);
      const auto &topology = operand == operand_id::a
                                 ? *candidates_->manifolds()->a()
                                 : *candidates_->manifolds()->b();
      auto &published = source_topology_[static_cast<std::size_t>(operand)];
      published.operand = operand;
      published.source_triangle_count = table.triangles.size();
      published.canonical_edge_count = topology.edges().size();
      published.source_semantic_digest = table.source_semantic_digest;
      published.exact_topology_digest = table.exact_topology_digest;

      for (const auto &edge : table.edges) {
        if (edge.edge_class != canonical_edge_class::source_edge ||
            !edge.source_feature_owner)
          continue;
        relation_source_edge_domain_record domain;
        domain.canonical_edge = edge.edge.ordinal();
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
        published.source_edges.push_back(std::move(domain));
      }

      for (const auto &vertex : topology.vertices()) {
        if (vertex.canonical_id >= topology.vertices().size() ||
            vertex.fan >= topology.fans().size())
          return fail(error, relation_subcode::predecessor_mismatch,
                      "Component 07 downstream source fan is malformed",
                      relation_checkpoint::producer_verification);
        const auto &fan = topology.fans()[vertex.fan];
        relation_source_vertex_fan_record record;
        record.canonical_vertex = vertex.canonical_id;
        record.source_vertex.operand = operand;
        record.source_vertex.kind = relation_feature_kind::source_vertex;
        record.source_vertex.primary = vertex.source_vertex;
        for (const auto halfedge_id : fan.outgoing_halfedges) {
          if (halfedge_id >= topology.halfedges().size())
            return fail(error, relation_subcode::predecessor_mismatch,
                        "Component 07 downstream source fan halfedge is malformed",
                        relation_checkpoint::producer_verification);
          relation_feature_key facet;
          if (!facet_feature(topology,
                             topology.halfedges()[halfedge_id].source_facet,
                             facet))
            return fail(error, relation_subcode::predecessor_mismatch,
                        "Component 07 downstream source fan facet is malformed",
                        relation_checkpoint::producer_verification);
          if (record.ordered_facets.empty() ||
              !(record.ordered_facets.back() == facet))
            record.ordered_facets.push_back(facet);
        }
        if (record.ordered_facets.size() > 1 &&
            record.ordered_facets.front() == record.ordered_facets.back())
          record.ordered_facets.pop_back();
        published.vertex_fans.push_back(std::move(record));
      }

      for (const auto &edge : topology.edges()) {
        relation_source_edge_adjacency_record record;
        record.canonical_edge = edge.canonical_id;
        record.edge_class = edge.edge_class;
        record.edge.operand = operand;
        if (edge.edge_class == canonical_edge_class::source_edge) {
          record.edge.kind = relation_feature_kind::source_edge;
          record.edge.primary = edge.key.primary;
          record.edge.secondary = edge.key.secondary;
        } else {
          record.edge.kind = relation_feature_kind::facet_internal_diagonal;
          record.edge.primary = edge.source_facet;
          record.edge.secondary = edge.source_diagonal;
        }
        if (!facet_feature(topology, edge.facets[0], record.first_facet) ||
            !facet_feature(topology, edge.facets[1], record.second_facet))
          return fail(error, relation_subcode::predecessor_mismatch,
                      "Component 07 downstream source adjacency is malformed",
                      relation_checkpoint::producer_verification);
        record.source_feature_owner = edge.source_feature_owner;
        record.bookkeeping_only =
            !edge.source_feature_owner && !edge.symbolic_contact_owner &&
            !edge.classification_barrier_inside_source_facet &&
            !edge.retained_surface_feature;
        published.edge_adjacencies.push_back(std::move(record));
      }
    }

    for (const auto &relation : relations_) {
      if (relation.family != feature_relation_family::source_facet_source_facet ||
          relation.status != feature_relation_status::proper_crossing)
        continue;
      if (relation.producer.ordinal() >= graph_.requests.size())
        return fail(error, relation_subcode::predecessor_mismatch,
                    "Component 07 downstream transverse request is malformed",
                    relation_checkpoint::producer_verification);
      const auto &request = graph_.requests[relation.producer.ordinal()];
      const auto stage = std::find_if(
          facet_stage_->relations.begin(), facet_stage_->relations.end(),
          [&](const auto &candidate) {
            return candidate.first_feature == request.key.first &&
                   candidate.second_feature == request.key.second;
          });
      const auto construction = std::find_if(
          constructions_.begin(), constructions_.end(), [&](const auto &candidate) {
            return candidate.source_relation == relation.id &&
                   candidate.kind == relation_construction_kind::bounded_carrier;
          });
      if (stage == facet_stage_->relations.end() ||
          construction == constructions_.end())
        return fail(error, relation_subcode::predecessor_mismatch,
                    "Component 07 downstream transverse support is incomplete",
                    relation_checkpoint::producer_verification);
      relation_transverse_carrier_support_record support;
      support.relation = relation.id;
      support.construction = construction->id;
      support.first_facet = request.key.first;
      support.second_facet = request.key.second;
      for (const auto consumer : stage->edge_facet_consumers) {
        if (consumer.ordinal() >= edge_facet_stage_->relations.size())
          return fail(error, relation_subcode::predecessor_mismatch,
                      "Component 07 downstream transverse consumer is absent",
                      relation_checkpoint::producer_verification);
        support.expected_membership_count +=
            edge_facet_stage_->relations[consumer.ordinal()].events.size();
      }
      support.support_consistent = stage->has_transverse_carrier;
      support.orientation_consistent = stage->has_transverse_carrier;
      support.residuals_accepted = stage->has_transverse_carrier &&
                                   stage->transverse_carrier.residuals_accepted;
      support.precision_evidence_complete =
          construction->precision_evidence_complete;
      transverse_carrier_supports_.push_back(std::move(support));
    }
    return true;
  }

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_candidate_stream<T, I>> candidates_;
  std::shared_ptr<const vertex_facet_stage_type> vertex_facet_stage_;
  std::shared_ptr<const edge_stage_type> edge_stage_;
  std::shared_ptr<const edge_facet_stage_type> edge_facet_stage_;
  std::shared_ptr<const facet_stage_type> facet_stage_;
  std::shared_ptr<const overlay_stage_type> overlay_stage_;
  std::shared_ptr<const transverse_stage_type> transverse_stage_;
  std::shared_ptr<const relation_execution_authority> execution_authority_;
  const relation_capabilities &capabilities_;

  std::vector<relation_request_proposal> proposals_;
  std::vector<base_descriptor> bases_;
  std::vector<construction_descriptor> constructions_desc_;
  std::vector<construction_descriptor> construction_uses_;
  std::vector<interval_descriptor> interval_desc_;
  std::vector<region_descriptor> region_desc_;
  std::vector<symbolic_descriptor> symbolics_;
  std::vector<transverse_membership_descriptor> transverse_membership_desc_;
  std::vector<disposition_descriptor> disposition_desc_;
  std::vector<std::vector<relation_request_key>> candidate_base_keys_;
  std::map<relation_request_key, relation_request_key> multiplicity_keys_;
  std::map<std::pair<relation_request_id, std::uint32_t>, std::uint32_t>
      ordered_event_occurrences_;
  std::vector<relation_request_key> seed_request_keys_;

  relation_request_graph graph_{};
  std::vector<relation_request_key> imported_keys_;
  std::vector<relation_imported_geometry_record> imported_geometry_;
  std::vector<relation_bounded_primitive_record> bounded_primitives_;
  std::vector<relation_exact_relation_record> exact_relations_;
  std::vector<relation_truth_lineage_record> truth_lineage_;
  std::vector<relation_interval_evidence_record> interval_evidence_;
  std::vector<relation_source_facet_region_record<T>> source_facet_regions_;
  std::vector<relation_truth_record> truth_records_;
  std::vector<feature_relation_record> relations_;
  std::vector<relation_construction_record> constructions_;
  std::vector<relation_construction_ledger_record> construction_ledger_;
  std::vector<relation_coplanar_event_node_record> coplanar_event_nodes_;
  std::vector<relation_coplanar_oriented_arc_record> coplanar_oriented_arcs_;
  std::vector<relation_coplanar_overlap_component_record>
      coplanar_overlap_components_;
  std::vector<symbolic_eligibility_record> eligibility_;
  std::vector<symbolic_relation_decision_record> decisions_;
  std::vector<relation_crossing_record> crossings_;
  relation_event_seed_table seed_table_{};
  std::vector<relation_candidate_disposition_record> dispositions_;
  std::vector<relation_triangle_local_reconciliation_record>
      triangle_local_reconciliation_;
  std::vector<relation_transverse_carrier_membership_record>
      transverse_carrier_memberships_;
  std::array<relation_source_topology_record, 2> source_topology_{};
  std::vector<relation_transverse_carrier_support_record>
      transverse_carrier_supports_;
  std::uint64_t transverse_evaluated_records_consumed_ = 0;
  std::vector<feature_relation_id> candidate_relation_coverage_;
  std::vector<relation_event_seed_id> candidate_event_seed_coverage_;
  std::vector<relation_candidate_partition_record> candidate_partitions_;
  std::map<relation_request_key, feature_relation_id> relation_ids_;
  std::map<relation_request_key, relation_construction_id> construction_ids_;
  std::map<std::tuple<relation_request_key,
                      symbolic_relation_subject_kind, std::uint64_t,
                      operand_id>,
           symbolic_relation_decision_id>
      decision_ids_;
};

} // namespace ygor::mesh_boolean::bounded
