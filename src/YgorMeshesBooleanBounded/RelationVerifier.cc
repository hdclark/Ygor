#include "StrictFloatingBuild.h"
#include "RelationVerifier.h"
#include "RelationCandidateEvidenceVerifier.h"
#include "RelationReplay.h"
#include "RelationVerificationRecords.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error verifier_error(relation_subcode subcode,
                                     const char *summary) {
  return relation_error(subcode,
                        bounded_boolean_error_category::internal_invariant_error,
                        summary,
                        relation_checkpoint::independent_verification);
}

const canonical_relation_request *find_request(
    const relation_request_graph &graph,
    const relation_request_key &key) noexcept {
  const auto it = std::lower_bound(
      graph.requests.begin(), graph.requests.end(), key,
      [](const canonical_relation_request &record,
         const relation_request_key &candidate) {
        return record.key < candidate;
      });
  return it == graph.requests.end() || it->key != key ? nullptr : &*it;
}

bool request_has_dependency(const relation_request_graph &graph,
                            const canonical_relation_request &consumer,
                            relation_request_id producer) noexcept {
  if (consumer.dependency_begin > graph.dependencies.size() ||
      consumer.dependency_count >
          graph.dependencies.size() - consumer.dependency_begin)
    return false;
  for (std::uint64_t offset = 0; offset < consumer.dependency_count; ++offset)
    if (graph.dependencies[consumer.dependency_begin + offset].producer ==
        producer)
      return true;
  return false;
}

feature_relation_status edge_status(
    source_edge_contact_class contact,
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

feature_relation_status edge_facet_status(
    source_edge_facet_contact_class contact) noexcept {
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

feature_relation_status facet_status(
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

feature_relation_status overlay_status(
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

bool verifier_truth_is_zero(const relation_truth_record &truth) noexcept {
  return truth.bounded_sign == bounded_sign_status::overlaps_boundary &&
         truth.exact_relation == exact_relation_status::exact_zero &&
         truth.disposition ==
             predicate_disposition::retain_tie_for_consumer_eligibility;
}

bool verifier_truth_is_nonzero(const relation_truth_record &truth) noexcept {
  return (truth.bounded_sign == bounded_sign_status::definitely_negative ||
          truth.bounded_sign == bounded_sign_status::definitely_positive) &&
         truth.disposition == predicate_disposition::accept_numeric_sign &&
         truth.exact_relation != exact_relation_status::exact_zero &&
         truth.exact_relation != exact_relation_status::invalid;
}

template <class T>
bool verifier_parameter_before(const source_edge_parameter_evidence<T> &a,
                               const source_edge_parameter_evidence<T> &b) noexcept {
  return finite_numeric_less(a.enclosure.upper(), b.enclosure.lower());
}

template <class T>
bool verifier_parameter_at(const source_edge_parameter_evidence<T> &parameter,
                           T endpoint) noexcept {
  return endpoint == T(0)
             ? parameter.exact_zero == exact_relation_status::exact_zero
             : parameter.exact_one == exact_relation_status::exact_zero;
}

template <class T>
bool verifier_derive_edge_classification(
    const source_edge_relation_record<T> &source,
    source_edge_support_class &support, source_edge_contact_class &contact,
    source_edge_orientation_relation &orientation) noexcept {
  orientation = source_edge_orientation_relation::not_applicable;
  if (verifier_truth_is_nonzero(source.parallel_truth)) {
    if (!source.has_coplanarity_truth)
      return false;
    if (verifier_truth_is_nonzero(source.coplanarity_truth)) {
      support = source_edge_support_class::skew_separated;
      contact = source_edge_contact_class::none;
      return source.parameter_count == 0 && source.point_count == 0;
    }
    if (!verifier_truth_is_zero(source.coplanarity_truth) ||
        source.parameter_count != 1)
      return false;
    support = source_edge_support_class::nonparallel_coplanar;
    const auto &first = source.first_parameters[0];
    const auto &second = source.second_parameters[0];
    if (first.domain == parameter_domain_status::outside ||
        second.domain == parameter_domain_status::outside) {
      contact = source_edge_contact_class::none;
      return source.point_count == 0;
    }
    if (source.point_count != 1)
      return false;
    contact = first.domain == parameter_domain_status::stable_interior &&
                      second.domain == parameter_domain_status::stable_interior
                  ? source_edge_contact_class::proper_crossing
                  : source_edge_contact_class::endpoint_contact;
    return true;
  }

  if (!verifier_truth_is_zero(source.parallel_truth) ||
      !source.has_collinearity_truth)
    return false;
  if (verifier_truth_is_nonzero(source.collinearity_truth)) {
    support = source_edge_support_class::parallel_separated;
    contact = source_edge_contact_class::none;
    return source.parameter_count == 0 && source.point_count == 0;
  }
  if (!verifier_truth_is_zero(source.collinearity_truth) ||
      source.parameter_count == 0)
    return false;
  support = source_edge_support_class::collinear;
  const auto &q0 = source.first_parameters[0];
  const auto &q1 = source.parameter_count > 1
                       ? source.first_parameters[1]
                       : source.first_parameters[0];
  bool geometry_orientation = false;
  for (std::size_t axis = 0; axis < 3 && !geometry_orientation; ++axis) {
    const auto first_start = source.first_start.rounded_nominal[axis];
    const auto first_end = source.first_end.rounded_nominal[axis];
    const auto second_start = source.second_start.rounded_nominal[axis];
    const auto second_end = source.second_end.rounded_nominal[axis];
    if (first_start == first_end || second_start == second_end)
      continue;
    orientation = ((first_end > first_start) == (second_end > second_start))
                      ? source_edge_orientation_relation::same
                      : source_edge_orientation_relation::opposite;
    geometry_orientation = true;
  }
  if (!geometry_orientation ||
      (source.parameter_count > 1 &&
       ((orientation == source_edge_orientation_relation::same &&
         !verifier_parameter_before(q0, q1)) ||
        (orientation == source_edge_orientation_relation::opposite &&
         !verifier_parameter_before(q1, q0)))))
    return false;
  if (source.point_count == 0) {
    contact = source_edge_contact_class::none;
    return source.parameter_count == 2;
  }
  if (source.point_count == 1) {
    contact = source.parameter_count == 1
                  ? source_edge_contact_class::point_contact
                  : source_edge_contact_class::partial_overlap;
    return source.parameter_count == 1;
  }
  if (source.point_count != 2 || source.parameter_count != 2)
    return false;
  const auto &minimum = orientation == source_edge_orientation_relation::same
                            ? q0
                            : q1;
  const auto &maximum = orientation == source_edge_orientation_relation::same
                            ? q1
                            : q0;
  if (verifier_parameter_at(minimum, T(0)) &&
      verifier_parameter_at(maximum, T(1)))
    contact = source_edge_contact_class::equal;
  else if (minimum.enclosure.lower() > T(0) &&
           maximum.enclosure.upper() < T(1))
    contact = source_edge_contact_class::first_contains_second;
  else if (minimum.enclosure.upper() < T(0) &&
           maximum.enclosure.lower() > T(1))
    contact = source_edge_contact_class::second_contains_first;
  else
    contact = source_edge_contact_class::partial_overlap;
  return true;
}

template <class T>
bool verifier_derive_edge_facet_classification(
    const source_edge_facet_relation_record<T> &source,
    source_edge_facet_support_class &support,
    source_edge_facet_contact_class &contact) noexcept {
  const bool first_nonzero =
      verifier_truth_is_nonzero(source.endpoint_support_truth[0]);
  const bool second_nonzero =
      verifier_truth_is_nonzero(source.endpoint_support_truth[1]);
  const bool first_zero =
      verifier_truth_is_zero(source.endpoint_support_truth[0]);
  const bool second_zero =
      verifier_truth_is_zero(source.endpoint_support_truth[1]);
  if (first_nonzero && second_nonzero &&
      source.endpoint_support_truth[0].bounded_sign ==
          source.endpoint_support_truth[1].bounded_sign) {
    support = source_edge_facet_support_class::definitely_separated_same_side;
    contact = source_edge_facet_contact_class::none;
    return source.events.empty() && !source.has_coplanar_partition;
  }
  if (first_nonzero && second_nonzero &&
      source.endpoint_support_truth[0].bounded_sign !=
          source.endpoint_support_truth[1].bounded_sign) {
    support = source_edge_facet_support_class::transverse_support_crossing;
    if (source.events.empty()) {
      contact = source_edge_facet_contact_class::none;
      return !source.has_coplanar_partition;
    }
    if (source.events.size() != 1 || source.has_coplanar_partition)
      return false;
    const auto &event = source.events.front();
    if (event.region.classification == source_facet_point_region_class::outside)
      return false;
    const bool interior = event.region.classification ==
                          source_facet_point_region_class::interior;
    contact = interior ? source_edge_facet_contact_class::proper_face_crossing
                       : source_edge_facet_contact_class::boundary_crossing;
    return event.kind ==
           (interior ? source_edge_facet_event_kind::proper_face_crossing
                     : source_edge_facet_event_kind::boundary_crossing);
  }
  if (first_zero != second_zero) {
    support = source_edge_facet_support_class::endpoint_support_tie;
    const auto endpoint = first_zero ? std::size_t{0} : std::size_t{1};
    if (!source.has_endpoint_region[endpoint] ||
        source.has_coplanar_partition)
      return false;
    const auto region = source.endpoint_regions[endpoint].classification;
    if (region == source_facet_point_region_class::outside) {
      contact = source_edge_facet_contact_class::none;
      return source.events.empty();
    }
    if (source.events.size() != 1)
      return false;
    const bool boundary = region == source_facet_point_region_class::original_edge ||
                          region == source_facet_point_region_class::original_vertex;
    contact = boundary ? source_edge_facet_contact_class::tangent_contact
                       : source_edge_facet_contact_class::endpoint_contact;
    return source.events.front().kind ==
           (boundary ? source_edge_facet_event_kind::tangent_contact
                     : source_edge_facet_event_kind::endpoint_contact);
  }
  if (!first_zero || !second_zero || !source.has_coplanar_partition ||
      !source.events.empty())
    return false;
  support = source_edge_facet_support_class::coplanar_support;
  bool overlap = false;
  bool interior = false;
  for (const auto &interval : source.coplanar_partition.intervals) {
    overlap = overlap || interval.classification ==
                             source_facet_segment_interval_class::original_edge_overlap;
    interior = interior || interval.classification ==
                               source_facet_segment_interval_class::interior;
  }
  contact = overlap
                ? source_edge_facet_contact_class::coplanar_boundary_overlap
            : interior
                ? source_edge_facet_contact_class::coplanar_containment
            : !source.coplanar_partition.contacts.empty()
                ? source_edge_facet_contact_class::coplanar_point_contact
                : source_edge_facet_contact_class::none;
  return true;
}

template <class T>
bool verifier_derive_facet_classification(
    const source_facet_source_facet_relation_record<T> &source,
    source_facet_support_relation_class &classification) noexcept {
  if (source.parallelism_truth.exact_relation ==
      exact_relation_status::exact_positive) {
    classification = source_facet_support_relation_class::transverse;
    if (!source.has_transverse_carrier || source.has_coplanarity_truth ||
        source.has_orientation_truth ||
        source.transverse_carrier.direction_squared.lower() <= T(0) ||
        !source.transverse_carrier.residuals_accepted)
      return false;
    return true;
  }
  if (source.parallelism_truth.exact_relation !=
          exact_relation_status::exact_zero ||
      !source.has_coplanarity_truth || source.has_transverse_carrier)
    return false;
  if (source.coplanarity_truth.exact_relation ==
      exact_relation_status::exact_zero) {
    if (!source.has_orientation_truth)
      return false;
    if (source.orientation_truth.exact_relation ==
        exact_relation_status::exact_positive)
      classification =
          source_facet_support_relation_class::coplanar_same_orientation;
    else if (source.orientation_truth.exact_relation ==
             exact_relation_status::exact_negative)
      classification =
          source_facet_support_relation_class::coplanar_opposite_orientation;
    else
      return false;
    return true;
  }
  if ((source.coplanarity_truth.exact_relation ==
           exact_relation_status::exact_positive ||
       source.coplanarity_truth.exact_relation ==
           exact_relation_status::exact_negative) &&
      verifier_truth_is_nonzero(source.coplanarity_truth) &&
      !source.has_orientation_truth) {
    classification = source_facet_support_relation_class::parallel_separated;
    return true;
  }
  return false;
}

template <class T>
bool verifier_derive_overlay_classification(
    const source_facet_coplanar_overlay_record<T> &source,
    coplanar_facet_overlay_class &classification) noexcept {
  source_facet_support_relation_class support;
  if (!verifier_derive_facet_classification(source.support_relation, support) ||
      (support != source_facet_support_relation_class::
                      coplanar_same_orientation &&
       support != source_facet_support_relation_class::
                      coplanar_opposite_orientation))
    return false;
  bool has_point = false;
  bool has_segment = false;
  std::uint64_t proper_crossings = 0;
  for (const auto &boundary : source.boundary_relations) {
    source_edge_support_class edge_support;
    source_edge_contact_class edge_contact;
    source_edge_orientation_relation edge_orientation;
    if (!verifier_derive_edge_classification(
            boundary.relation, edge_support, edge_contact, edge_orientation))
      return false;
    proper_crossings +=
        edge_contact == source_edge_contact_class::proper_crossing ? 1U : 0U;
    has_segment = has_segment ||
                  edge_contact == source_edge_contact_class::partial_overlap ||
                  edge_contact ==
                      source_edge_contact_class::first_contains_second ||
                  edge_contact ==
                      source_edge_contact_class::second_contains_first ||
                  edge_contact == source_edge_contact_class::equal;
    has_point = has_point ||
                edge_contact == source_edge_contact_class::endpoint_contact ||
                edge_contact == source_edge_contact_class::point_contact;
  }
  std::array<std::uint64_t, 2> interior{};
  std::array<std::uint64_t, 2> outside{};
  std::array<std::uint64_t, 2> boundary{};
  for (const auto &witness : source.vertex_regions) {
    if (witness.polygon > 1)
      return false;
    switch (witness.region.classification) {
    case source_facet_point_region_class::interior:
      ++interior[witness.polygon];
      break;
    case source_facet_point_region_class::outside:
      ++outside[witness.polygon];
      break;
    case source_facet_point_region_class::original_edge:
    case source_facet_point_region_class::original_vertex:
      ++boundary[witness.polygon];
      break;
    }
  }
  bool coincident_component = false;
  bool segment_component = false;
  bool point_component = false;
  for (const auto &component : source.overlap_components) {
    coincident_component = coincident_component ||
        (component.kind ==
             coplanar_overlap_component_kind::coincident_sheet_boundary &&
         component.closed && component.sheet_mask == 3);
    segment_component = segment_component ||
        component.kind == coplanar_overlap_component_kind::boundary_segment;
    point_component = point_component ||
        component.kind == coplanar_overlap_component_kind::isolated_point;
  }
  const bool all_first_boundary =
      boundary[0] == source.facets[0].polygon.size();
  const bool all_second_boundary =
      boundary[1] == source.facets[1].polygon.size();
  if (all_first_boundary && all_second_boundary && coincident_component) {
    classification =
        support ==
                source_facet_support_relation_class::coplanar_same_orientation
            ? coplanar_facet_overlay_class::equal_same_orientation
            : coplanar_facet_overlay_class::equal_opposite_orientation;
  } else if (proper_crossings != 0 ||
             (interior[0] != 0 && interior[1] != 0)) {
    classification = coplanar_facet_overlay_class::area_overlap;
  } else {
    const bool first_in_second = outside[0] == 0 && interior[0] != 0;
    const bool second_in_first = outside[1] == 0 && interior[1] != 0;
    if (first_in_second && !second_in_first)
      classification = coplanar_facet_overlay_class::second_contains_first;
    else if (second_in_first && !first_in_second)
      classification = coplanar_facet_overlay_class::first_contains_second;
    else if (interior[0] != 0 || interior[1] != 0)
      return false;
    else if (has_segment || segment_component)
      classification = coplanar_facet_overlay_class::segment_contact;
    else if (has_point || point_component)
      classification = coplanar_facet_overlay_class::point_contact;
    else
      classification = coplanar_facet_overlay_class::disjoint;
  }
  return true;
}

relation_coplanar_arc_kind verifier_coplanar_arc_kind(
    coplanar_overlap_arc_kind kind) noexcept {
  return kind == coplanar_overlap_arc_kind::shared_boundary
             ? relation_coplanar_arc_kind::shared_boundary
             : relation_coplanar_arc_kind::interior_boundary;
}

relation_coplanar_component_kind verifier_coplanar_component_kind(
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

relation_request_key verifier_imported_geometry_key(
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

relation_request_key verifier_derived_key(const relation_request_key &base,
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

std::uint64_t verifier_tagged_use(std::uint8_t domain,
                                  std::uint8_t category = 0) noexcept {
  return (static_cast<std::uint64_t>(domain) << 56U) |
         (static_cast<std::uint64_t>(category) << 48U);
}

std::uint64_t verifier_symbolic_directed_use(
    std::uint8_t domain, const symbolic_rule_key &key,
    symbolic_relation_subject_kind subject_kind) noexcept {
  return verifier_tagged_use(domain) |
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

orientation_relation verifier_orientation_from_edge(
    source_edge_orientation_relation value) noexcept {
  return value == source_edge_orientation_relation::same
             ? orientation_relation::same
         : value == source_edge_orientation_relation::opposite
             ? orientation_relation::opposite
             : orientation_relation::indeterminate;
}

orientation_relation verifier_orientation_from_status(
    feature_relation_status value) noexcept {
  return value == feature_relation_status::coincidence_same_orientation
             ? orientation_relation::same
         : value == feature_relation_status::coincidence_opposite_orientation
             ? orientation_relation::opposite
             : orientation_relation::indeterminate;
}

template <class T>
relation_family verifier_symbolic_family_for_edge(
    source_edge_contact_class contact,
    const source_edge_point_construction<T> *point) noexcept {
  if (contact == source_edge_contact_class::partial_overlap ||
      contact == source_edge_contact_class::first_contains_second ||
      contact == source_edge_contact_class::second_contains_first ||
      contact == source_edge_contact_class::equal)
    return relation_family::equal_edge;
  if (point) {
    const bool first_endpoint = point->first_endpoint_owner_mask != 0;
    const bool second_endpoint = point->second_endpoint_owner_mask != 0;
    if (first_endpoint && second_endpoint)
      return relation_family::vertex_vertex;
    if (first_endpoint || second_endpoint)
      return relation_family::vertex_edge;
  }
  return relation_family::edge_edge;
}

template <class T>
relation_family verifier_symbolic_family_for_edge_facet(
    const source_edge_facet_event_record<T> &event,
    source_edge_facet_contact_class contact) noexcept {
  if (event.kind == source_edge_facet_event_kind::tangent_contact ||
      contact == source_edge_facet_contact_class::tangent_contact)
    return relation_family::tangent;
  if (contact == source_edge_facet_contact_class::coplanar_point_contact ||
      contact == source_edge_facet_contact_class::coplanar_boundary_overlap ||
      contact == source_edge_facet_contact_class::coplanar_containment)
    return relation_family::coplanar;
  const bool endpoint = event.construction.edge_endpoint_owner_mask != 0;
  if (event.region.classification ==
      source_facet_point_region_class::original_vertex)
    return endpoint ? relation_family::vertex_vertex
                    : relation_family::vertex_edge;
  if (event.region.classification ==
      source_facet_point_region_class::original_edge)
    return endpoint ? relation_family::vertex_edge
                    : relation_family::edge_edge;
  return endpoint ? relation_family::vertex_face : relation_family::edge_face;
}

relation_family verifier_symbolic_family_for_overlay(
    coplanar_facet_overlay_class value) noexcept {
  return value == coplanar_facet_overlay_class::equal_same_orientation ||
                 value ==
                     coplanar_facet_overlay_class::equal_opposite_orientation
             ? relation_family::coincident_face
         : value == coplanar_facet_overlay_class::segment_contact
             ? relation_family::equal_edge
             : relation_family::coplanar;
}

symbolic_rule_key verifier_symbolic_rule_key(
    boolean_operation operation, operand_id acting, relation_family family,
    orientation_relation orientation, symbolic_ownership_role ownership,
    symbolic_half_open_role half_open,
    symbolic_transition_orientation transition,
    symbolic_occurrence_class occurrence) noexcept {
  symbolic_rule_key key;
  key.operation = operation;
  key.acting_operand = acting;
  key.relation = family;
  key.orientation = orientation;
  key.ownership_role = ownership;
  key.half_open_role = half_open;
  key.transition = transition;
  key.occurrence_class = occurrence;
  return key;
}

template <class T>
bool finite_construction_component(const relation_construction_record &record,
                                   std::size_t component) noexcept {
  using bits_type = floating_uint_t<T>;
  const auto nominal = from_bits<T>(static_cast<bits_type>(record.nominal_bits[component]));
  const auto lower = from_bits<T>(static_cast<bits_type>(record.lower_bits[component]));
  const auto upper = from_bits<T>(static_cast<bits_type>(record.upper_bits[component]));
  return finite_bits(nominal) && finite_bits(lower) && finite_bits(upper) &&
         !finite_numeric_less(upper, lower) &&
         !finite_numeric_less(nominal, lower) &&
         !finite_numeric_less(upper, nominal);
}

bool valid_operation(boolean_operation operation) noexcept {
  const auto raw = static_cast<std::uint8_t>(operation);
  return raw >= 1 && raw <= 5;
}

bool verifier_symbolic_source_family(
    relation_request_family family) noexcept {
  return family == relation_request_family::source_edge_source_edge ||
         family == relation_request_family::source_edge_source_facet ||
         family == relation_request_family::source_facet_source_facet ||
         family == relation_request_family::coplanar_source_facet_overlay;
}

template <class T>
bool verifier_set_truth_symbolic_evidence(
    const relation_truth_record &truth, symbolic_eligibility_reason reason,
    symbolic_eligibility_record &eligibility) noexcept {
  if (truth.exact_relation != exact_relation_status::exact_zero ||
      truth.exact_formula == 0 ||
      truth.bounded_sign == bounded_sign_status::invalid ||
      truth.disposition == predicate_disposition::fail_invalid)
    return false;
  eligibility.exact_relation = exact_relation_status::exact_zero;
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

template <class T>
bool verifier_set_region_symbolic_evidence(
    const source_facet_point_region_record<T> &region,
    symbolic_eligibility_record &eligibility) noexcept {
  if (!region.boundary_ownership_resolved ||
      (region.classification !=
           source_facet_point_region_class::original_edge &&
       region.classification !=
           source_facet_point_region_class::original_vertex) ||
      region.source_edge_owners.empty())
    return false;
  std::uint16_t formula = 0;
  bool inherited_uncertainty = false;
  for (const auto &owner : region.source_edge_owners) {
    if (owner.edge_ordinal >= region.orientation_evidence.size())
      return false;
    const auto &evidence = region.orientation_evidence[owner.edge_ordinal];
    if (evidence.exact_sign != 0 || evidence.formula_version == 0 ||
        (formula != 0 && formula != evidence.formula_version))
      return false;
    formula = evidence.formula_version;
    inherited_uncertainty =
        inherited_uncertainty ||
        evidence.bounded_sign == bounded_planar_sign::uncertain;
  }
  eligibility.exact_relation = exact_relation_status::exact_zero;
  eligibility.reason =
      region.classification == source_facet_point_region_class::original_vertex
          ? symbolic_eligibility_reason::shared_source_endpoint
          : symbolic_eligibility_reason::collinear_source_edge_lineage;
  eligibility.evidence_formula_version = formula;
  eligibility.exact_lineage_tie = true;
  eligibility.rounded_nominal_zero = false;
  eligibility.inherited_uncertainty = inherited_uncertainty;
  return true;
}

bool verifier_symbolic_eligibility_equal(
    const symbolic_eligibility_record &a,
    const symbolic_eligibility_record &b) noexcept {
  return a.request == b.request && a.exact_relation == b.exact_relation &&
         a.reason == b.reason &&
         a.evidence_formula_version == b.evidence_formula_version &&
         a.exact_lineage_tie == b.exact_lineage_tie &&
         a.representational_tie_evidence ==
             b.representational_tie_evidence &&
         a.structural_category_eligible == b.structural_category_eligible &&
         a.tolerance_compatible == b.tolerance_compatible &&
         a.rounded_nominal_zero == b.rounded_nominal_zero &&
         a.inherited_uncertainty == b.inherited_uncertainty &&
         a.separated_realizations_possible ==
             b.separated_realizations_possible &&
         a.owner_is_original_source_feature ==
             b.owner_is_original_source_feature &&
         a.reserved8 == b.reserved8 && a.reserved == b.reserved;
}

struct verifier_source_fan_group_key final {
  bool boundary_group = false;
  relation_feature_key query_edge{};
  operand_id opposite_operand = operand_id::a;
  std::uint8_t boundary_kind = 0;
  std::uint64_t owner_primary = 0;
  std::uint64_t owner_secondary = 0;
  relation_request_key singleton_relation{};
  std::uint32_t singleton_occurrence = 0;

  friend bool operator<(const verifier_source_fan_group_key &a,
                        const verifier_source_fan_group_key &b) noexcept {
    return std::tie(a.boundary_group, a.query_edge, a.opposite_operand,
                    a.boundary_kind, a.owner_primary, a.owner_secondary,
                    a.singleton_relation, a.singleton_occurrence) <
           std::tie(b.boundary_group, b.query_edge, b.opposite_operand,
                    b.boundary_kind, b.owner_primary, b.owner_secondary,
                    b.singleton_relation, b.singleton_occurrence);
  }
  friend bool operator==(const verifier_source_fan_group_key &a,
                         const verifier_source_fan_group_key &b) noexcept {
    return std::tie(a.boundary_group, a.query_edge, a.opposite_operand,
                    a.boundary_kind, a.owner_primary, a.owner_secondary,
                    a.singleton_relation, a.singleton_occurrence) ==
           std::tie(b.boundary_group, b.query_edge, b.opposite_operand,
                    b.boundary_kind, b.owner_primary, b.owner_secondary,
                    b.singleton_relation, b.singleton_occurrence);
  }
};

template <class T> struct verifier_crossing_descriptor final {
  verifier_source_fan_group_key group{};
  relation_request_key source_relation{};
  feature_relation_id relation{0};
  const source_edge_facet_event_record<T> *event = nullptr;
  std::uint32_t occurrence = 0;
  std::int8_t local_transition = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
};

template <class T>
std::int8_t verifier_local_transition(
    const source_edge_facet_event_record<T> &event) noexcept {
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

template <class T>
bool verifier_source_fan_key(const relation_request_key &relation,
                             const source_edge_facet_event_record<T> &event,
                             std::uint32_t occurrence,
                             verifier_source_fan_group_key &key) {
  key = verifier_source_fan_group_key{};
  key.query_edge = relation.first;
  key.opposite_operand = relation.second.operand;
  const bool source_boundary =
      event.region.classification ==
          source_facet_point_region_class::original_edge ||
      event.region.classification ==
          source_facet_point_region_class::original_vertex;
  if (!source_boundary) {
    key.singleton_relation = relation;
    key.singleton_occurrence = occurrence;
    return true;
  }
  if (event.kind != source_edge_facet_event_kind::boundary_crossing &&
      event.kind != source_edge_facet_event_kind::tangent_contact)
    return false;
  key.boundary_group = true;
  if (event.region.classification ==
      source_facet_point_region_class::original_vertex) {
    if (event.region.source_vertex_owners.size() != 1)
      return false;
    key.boundary_kind = 2;
    key.owner_primary = event.region.source_vertex_owners.front();
    return true;
  }
  if (event.region.classification !=
          source_facet_point_region_class::original_edge ||
      event.region.source_edge_owners.empty())
    return false;
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
      return false;
  }
  return true;
}

template <class T, class I>
bool verifier_source_facet_feature(
    const canonical_halfedge_operand<T, I> &topology,
    std::uint64_t source_facet,
    relation_feature_key &feature) noexcept {
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

template <class T, class I>
bool verifier_expected_source_fan_facets(
    const canonical_candidate_stream<T, I> &candidates,
    const verifier_source_fan_group_key &key,
    std::vector<relation_feature_key> &facets) {
  facets.clear();
  if (!key.boundary_group || !candidates.manifolds())
    return false;
  const auto topology =
      key.opposite_operand == operand_id::a ? candidates.manifolds()->a()
                                            : candidates.manifolds()->b();
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
      if (endpoints.first == wanted.first && endpoints.second == wanted.second) {
        if (match)
          return false;
        match = &edge;
      }
    }
    if (!match)
      return false;
    for (const auto source_facet : match->facets) {
      relation_feature_key feature;
      if (!verifier_source_facet_feature(*topology, source_facet, feature))
        return false;
      facets.push_back(feature);
    }
  } else if (key.boundary_kind == 2) {
    if (key.owner_primary >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex = topology->source_vertex_to_vertex()[key.owner_primary];
    if (vertex >= topology->vertices().size())
      return false;
    const auto fan = topology->vertices()[vertex].fan;
    if (fan >= topology->fans().size())
      return false;
    for (const auto halfedge_id : topology->fans()[fan].outgoing_halfedges) {
      if (halfedge_id >= topology->halfedges().size())
        return false;
      relation_feature_key feature;
      if (!verifier_source_facet_feature(
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

template <class T> struct verifier_geometry_snapshot final {
  relation_construction_kind kind = relation_construction_kind::bounded_point;
  relation_construction_coordinate_space coordinate_space =
      relation_construction_coordinate_space::world_3d;
  std::uint8_t component_count = 0;
  std::uint8_t projection_axis = 3;
  std::array<T, 6> nominal{};
  std::array<T, 6> lower{};
  std::array<T, 6> upper{};
  std::uint64_t provenance = 0;
  std::uint64_t lineage = 0;
  bool accepted_source_vertex = false;
  bool finite = false;
  bool tolerance_compatible = false;
};

template <class T> struct verifier_construction_authority final {
  relation_request_key key{};
  relation_request_key source_relation{};
  relation_construction_precedence precedence =
      relation_construction_precedence::verification_witness;
  relation_feature_key source_feature{};
  verifier_geometry_snapshot<T> geometry{};
  construction_operation_certificate<T> certificate{};
  std::uint32_t source_occurrence = 0;
};

std::uint64_t verifier_construction_use_tag(std::uint8_t category) noexcept {
  return (std::uint64_t{10} << 56U) |
         (static_cast<std::uint64_t>(category) << 48U);
}

relation_request_key verifier_construction_key(
    const relation_request_key &source, std::uint8_t category,
    std::uint32_t occurrence, const relation_feature_key *first = nullptr,
    const relation_feature_key *second = nullptr) noexcept {
  auto out = source;
  out.family = relation_request_family::authoritative_construction;
  out.directed_use = verifier_construction_use_tag(category);
  out.occurrence_discriminator = occurrence;
  out.formula_version = contract_versions::exact_relation_formulas;
  out.policy_version = contract_versions::relation_construction_registry_policy;
  out.reserved = 0;
  if (first)
    out.first = *first;
  if (second)
    out.second = *second;
  return out;
}

template <class T>
bool verifier_valid_geometry(
    const verifier_geometry_snapshot<T> &geometry) noexcept {
  const bool valid_count =
      (geometry.kind == relation_construction_kind::bounded_point &&
       (geometry.component_count == 2 || geometry.component_count == 3)) ||
      (geometry.kind == relation_construction_kind::bounded_carrier &&
       geometry.component_count == 6);
  if (!valid_count || !geometry.finite || !geometry.tolerance_compatible)
    return false;
  if (geometry.coordinate_space ==
      relation_construction_coordinate_space::source_facet_projection) {
    if (geometry.component_count != 2 || geometry.projection_axis > 2)
      return false;
  } else if (geometry.coordinate_space ==
             relation_construction_coordinate_space::world_3d) {
    if ((geometry.component_count != 3 && geometry.component_count != 6) ||
        geometry.projection_axis != 3)
      return false;
  } else {
    return false;
  }
  for (std::size_t i = 0; i < geometry.component_count; ++i)
    if (!finite_bits(geometry.nominal[i]) || !finite_bits(geometry.lower[i]) ||
        !finite_bits(geometry.upper[i]) ||
        finite_numeric_less(geometry.upper[i], geometry.lower[i]) ||
        finite_numeric_less(geometry.nominal[i], geometry.lower[i]) ||
        finite_numeric_less(geometry.upper[i], geometry.nominal[i]))
      return false;
  return true;
}

template <class T>
verifier_geometry_snapshot<T> verifier_geometry_from_point(
    const source_edge_geometry_snapshot<T> &point, bool accepted_source_vertex,
    bool tolerance_compatible) noexcept {
  verifier_geometry_snapshot<T> out;
  out.component_count = 3;
  out.provenance = point.provenance;
  out.lineage = point.lineage;
  out.accepted_source_vertex = accepted_source_vertex;
  out.finite = true;
  out.tolerance_compatible = tolerance_compatible;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal[axis] = point.rounded_nominal[axis];
    out.lower[axis] = point.enclosure[axis].lower();
    out.upper[axis] = point.enclosure[axis].upper();
  }
  return out;
}

template <class T>
verifier_geometry_snapshot<T> verifier_geometry_from_carrier(
    const source_facet_transverse_carrier<T> &carrier) noexcept {
  verifier_geometry_snapshot<T> out;
  out.kind = relation_construction_kind::bounded_carrier;
  out.component_count = 6;
  out.finite = true;
  out.tolerance_compatible = carrier.residuals_accepted;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal[axis] = carrier.point.rounded[axis];
    out.lower[axis] = carrier.point.lower[axis];
    out.upper[axis] = carrier.point.upper[axis];
    out.nominal[axis + 3] = carrier.direction.rounded[axis];
    out.lower[axis + 3] = carrier.direction.lower[axis];
    out.upper[axis + 3] = carrier.direction.upper[axis];
  }
  return out;
}

template <class T>
verifier_geometry_snapshot<T> verifier_geometry_from_projected(
    const projected_source_point<T> &point, std::uint8_t dropped_axis,
    bool accepted_source_vertex = false) noexcept {
  verifier_geometry_snapshot<T> out;
  out.coordinate_space =
      relation_construction_coordinate_space::source_facet_projection;
  out.component_count = 2;
  out.projection_axis = dropped_axis;
  out.accepted_source_vertex = accepted_source_vertex;
  out.finite = true;
  out.tolerance_compatible = true;
  for (std::size_t axis = 0; axis < 2; ++axis) {
    out.nominal[axis] = point.nominal[axis];
    out.lower[axis] = point.enclosure[axis].lower();
    out.upper[axis] = point.enclosure[axis].upper();
  }
  return out;
}

template <class T>
bool verifier_same_geometry(const verifier_geometry_snapshot<T> &a,
                            const verifier_geometry_snapshot<T> &b) noexcept {
  if (a.kind != b.kind || a.coordinate_space != b.coordinate_space ||
      a.component_count != b.component_count ||
      a.projection_axis != b.projection_axis || a.provenance != b.provenance ||
      a.lineage != b.lineage ||
      a.accepted_source_vertex != b.accepted_source_vertex ||
      a.finite != b.finite ||
      a.tolerance_compatible != b.tolerance_compatible)
    return false;
  for (std::size_t i = 0; i < a.component_count; ++i)
    if (to_bits(a.nominal[i]) != to_bits(b.nominal[i]) ||
        to_bits(a.lower[i]) != to_bits(b.lower[i]) ||
        to_bits(a.upper[i]) != to_bits(b.upper[i]))
      return false;
  return true;
}

template <class T>
bool verifier_compatible_geometry(
    const verifier_geometry_snapshot<T> &authority,
    const verifier_geometry_snapshot<T> &witness) noexcept {
  if (!verifier_valid_geometry(authority) ||
      !verifier_valid_geometry(witness) || authority.kind != witness.kind)
    return false;
  const auto contained = [](T nominal, T lower, T upper) {
    return !finite_numeric_less(nominal, lower) &&
           !finite_numeric_less(upper, nominal);
  };
  if (authority.coordinate_space == witness.coordinate_space) {
    if (authority.component_count != witness.component_count ||
        authority.projection_axis != witness.projection_axis)
      return false;
    for (std::size_t i = 0; i < authority.component_count; ++i)
      if (!contained(authority.nominal[i], witness.lower[i], witness.upper[i]))
        return false;
    return true;
  }
  if (authority.coordinate_space !=
          relation_construction_coordinate_space::world_3d ||
      witness.coordinate_space !=
          relation_construction_coordinate_space::source_facet_projection ||
      authority.component_count != 3 || witness.component_count != 2 ||
      witness.projection_axis > 2)
    return false;
  std::size_t projected = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (axis == witness.projection_axis)
      continue;
    if (!contained(authority.nominal[axis], witness.lower[projected],
                   witness.upper[projected]))
      return false;
    ++projected;
  }
  return true;
}

relation_feature_key verifier_source_vertex_feature(
    operand_id operand, std::uint64_t source_vertex) noexcept {
  relation_feature_key out;
  out.operand = operand;
  out.kind = relation_feature_kind::source_vertex;
  out.primary = source_vertex;
  return out;
}

template <class T, class I>
bool verifier_source_vertex_geometry(
    const canonical_candidate_stream<T, I> &candidates, operand_id operand,
    std::uint64_t source_vertex,
    verifier_geometry_snapshot<T> &geometry) noexcept {
  if (!candidates.manifolds())
    return false;
  const auto topology = operand == operand_id::a ? candidates.manifolds()->a()
                                                 : candidates.manifolds()->b();
  if (!topology || source_vertex >= topology->source_vertex_to_vertex().size())
    return false;
  const auto dense = topology->source_vertex_to_vertex()[source_vertex];
  if (dense >= topology->vertices().size())
    return false;
  const auto &vertex = topology->vertices()[dense];
  if (vertex.source_vertex != source_vertex)
    return false;
  geometry = {};
  geometry.component_count = 3;
  geometry.provenance = source_vertex + 1;
  geometry.lineage = (static_cast<std::uint64_t>(operand) << 63U) |
                     (source_vertex + 1);
  geometry.accepted_source_vertex = true;
  geometry.finite = true;
  geometry.tolerance_compatible = true;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    geometry.nominal[axis] = vertex.committed_point[axis];
    geometry.lower[axis] = vertex.lower[axis];
    geometry.upper[axis] = vertex.upper[axis];
  }
  return verifier_valid_geometry(geometry);
}

template <class T, class I>
bool verifier_endpoint_vertex(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_feature_key &edge, std::uint8_t mask, operand_id &operand,
    std::uint64_t &source_vertex) noexcept {
  if (!valid_relation_feature_key(edge) ||
      edge.kind != relation_feature_kind::source_edge || mask == 0 || mask > 2 ||
      !candidates.manifolds())
    return false;
  const auto topology = edge.operand == operand_id::a
                            ? candidates.manifolds()->a()
                            : candidates.manifolds()->b();
  if (!topology || !topology->owner().same_owner(candidates.owner()))
    return false;
  const auto &table = candidates.primitive_table(edge.operand);
  const broad_phase_edge_primitive<T> *primitive = nullptr;
  for (const auto &candidate : table.edges) {
    if (candidate.edge_class != canonical_edge_class::source_edge ||
        !candidate.source_feature_owner)
      continue;
    relation_feature_key feature;
    feature.operand = edge.operand;
    feature.kind = relation_feature_kind::source_edge;
    feature.primary = candidate.semantic_key.primary;
    feature.secondary = candidate.semantic_key.secondary;
    if (feature != edge)
      continue;
    if (primitive)
      return false;
    primitive = &candidate;
  }
  if (!primitive)
    return false;
  const auto endpoint = primitive->endpoints[mask - 1];
  if (endpoint.ordinal() >= topology->vertices().size())
    return false;
  const auto &vertex = topology->vertices()[endpoint.ordinal()];
  operand = edge.operand;
  source_vertex = vertex.source_vertex;
  return true;
}

template <class T, class I>
bool verifier_edge_point_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_request_key &source_key,
    const source_edge_relation_record<T> &source, std::uint32_t point_ordinal,
    verifier_construction_authority<T> &out) noexcept {
  if (point_ordinal >= source.points.size())
    return false;
  const auto &point = source.points[point_ordinal];
  out = {};
  out.source_relation = source_key;
  out.source_occurrence = point_ordinal;
  out.certificate = point.certificate;
  operand_id vertex_operand = operand_id::a;
  std::uint64_t vertex = 0;
  bool has_vertex = false;
  if (point.first_endpoint_owner_mask != 0) {
    has_vertex = verifier_endpoint_vertex(
        candidates, source_key.first, point.first_endpoint_owner_mask,
        vertex_operand, vertex);
  } else if (point.second_endpoint_owner_mask != 0) {
    has_vertex = verifier_endpoint_vertex(
        candidates, source_key.second, point.second_endpoint_owner_mask,
        vertex_operand, vertex);
  }
  if (point.accepted_source_vertex) {
    if (!has_vertex ||
        !verifier_source_vertex_geometry(
            candidates, vertex_operand, vertex, out.geometry))
      return false;
    out.precedence = relation_construction_precedence::accepted_source_vertex;
    out.source_feature = verifier_source_vertex_feature(
        vertex_operand, vertex);
    out.key = verifier_construction_key(
        source_key, 1, 0, &out.source_feature);
    out.key.second = {};
    out.key.second.operand = vertex_operand;
    out.key.scope = relation_record_scope::public_source_feature;
    return valid_relation_request_key(out.key);
  }
  out.precedence =
      relation_construction_precedence::source_edge_source_edge_point;
  out.source_feature = source_key.first;
  out.geometry = verifier_geometry_from_point(
      point.point, false, point.tolerance_compatible);
  out.key = verifier_construction_key(
      source_key, 2, point_ordinal);
  return valid_relation_request_key(out.key) &&
         verifier_valid_geometry(out.geometry);
}

template <class T, class I>
bool verifier_edge_relation_point_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    relation_request_id request, const source_edge_geometry_snapshot<T> &witness,
    verifier_construction_authority<T> &out) noexcept {
  if (request.ordinal() >= edge_stage.relations.size() ||
      request.ordinal() >= edge_stage.request_graph.requests.size())
    return false;
  const auto &relation = edge_stage.relations[request.ordinal()];
  std::uint32_t match = std::numeric_limits<std::uint32_t>::max();
  for (std::uint32_t point = 0; point < relation.points.size(); ++point) {
    const auto &candidate = relation.points[point].point;
    if (candidate.provenance != 0 && candidate.lineage != 0 &&
        candidate.provenance == witness.provenance &&
        candidate.lineage == witness.lineage) {
      if (match != std::numeric_limits<std::uint32_t>::max())
        return false;
      match = point;
    }
  }
  return match != std::numeric_limits<std::uint32_t>::max() &&
         verifier_edge_point_authority(
             candidates, edge_stage.request_graph.requests[request.ordinal()].key,
             relation, match, out);
}

template <class T, class I>
bool verifier_edge_facet_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const relation_request_key &source_key,
    const source_edge_facet_relation_record<T> &source,
    const source_edge_facet_event_record<T> &event, std::uint32_t occurrence,
    verifier_construction_authority<T> &out) noexcept {
  out = {};
  out.certificate = event.construction.certificate;
  operand_id vertex_operand = source_key.first.operand;
  std::uint64_t vertex = 0;
  bool has_vertex = false;
  if (event.construction.edge_endpoint_owner_mask != 0)
    has_vertex = verifier_endpoint_vertex(
        candidates, source_key.first,
        event.construction.edge_endpoint_owner_mask, vertex_operand, vertex);
  if (!has_vertex && event.region.classification ==
                         source_facet_point_region_class::original_vertex) {
    if (event.region.source_vertex_owners.size() != 1)
      return false;
    vertex = event.region.source_vertex_owners.front();
    vertex_operand = source_key.second.operand;
    has_vertex = true;
  }
  if (event.construction.accepted_source_vertex || has_vertex) {
    if (!has_vertex ||
        !verifier_source_vertex_geometry(
            candidates, vertex_operand, vertex, out.geometry))
      return false;
    out.source_feature = verifier_source_vertex_feature(
        vertex_operand, vertex);
    out.key = verifier_construction_key(
        source_key, 1, 0, &out.source_feature);
    out.key.second = {};
    out.key.second.operand = vertex_operand;
    out.key.scope = relation_record_scope::public_source_feature;
    out.source_relation = source_key;
    out.precedence = relation_construction_precedence::accepted_source_vertex;
    return valid_relation_request_key(out.key);
  }
  if (event.region.classification ==
      source_facet_point_region_class::original_edge) {
    for (const auto request : source.boundary_relation_requests)
      if (verifier_edge_relation_point_authority(
              candidates, edge_stage, request, event.construction.point, out))
        return true;
    return false;
  }
  out.key = verifier_construction_key(
      source_key, 3, occurrence);
  out.source_relation = source_key;
  out.precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  out.source_feature = source_key.first;
  out.geometry = verifier_geometry_from_point(
      event.construction.point, false,
      event.construction.tolerance_compatible);
  out.source_occurrence = occurrence;
  return valid_relation_request_key(out.key) &&
         verifier_valid_geometry(out.geometry);
}

template <class T>
bool verifier_carrier_authority(
    const relation_request_key &source_key,
    const source_facet_transverse_carrier<T> &carrier,
    verifier_construction_authority<T> &out) noexcept {
  out = {};
  out.key = verifier_construction_key(source_key, 5, 0);
  out.source_relation = source_key;
  out.precedence =
      relation_construction_precedence::source_facet_source_facet_carrier;
  out.source_feature = source_key.first;
  out.geometry =
      verifier_geometry_from_carrier(carrier);
  out.certificate = carrier.certificate;
  return valid_relation_request_key(out.key) &&
         verifier_valid_geometry(out.geometry);
}

template <class T, class I>
bool verifier_overlay_node_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const relation_request_key &source_key,
    const source_facet_coplanar_overlay_record<T> &source,
    const coplanar_overlap_event_node<T> &node,
    verifier_construction_authority<T> &out) noexcept {
  bool has_vertex = false;
  bool ambiguous_vertex = false;
  operand_id vertex_operand = operand_id::a;
  std::uint64_t vertex = 0;
  for (const auto &occurrence : node.occurrences) {
    if (!occurrence.query_source_vertex_valid)
      continue;
    if (occurrence.polygon > 1)
      return false;
    const auto operand = source.facets[occurrence.polygon].feature.operand;
    if (has_vertex && (operand != vertex_operand ||
                       occurrence.query_source_vertex != vertex)) {
      ambiguous_vertex = true;
      continue;
    }
    has_vertex = true;
    vertex_operand = operand;
    vertex = occurrence.query_source_vertex;
  }
  if (has_vertex && !ambiguous_vertex) {
    out = {};
    out.certificate = node.certificate;
    out.source_feature = verifier_source_vertex_feature(
        vertex_operand, vertex);
    out.key = verifier_construction_key(
        source_key, 1, 0, &out.source_feature);
    out.key.second = {};
    out.key.second.operand = vertex_operand;
    out.key.scope = relation_record_scope::public_source_feature;
    out.source_relation = source_key;
    out.precedence = relation_construction_precedence::accepted_source_vertex;
    return verifier_source_vertex_geometry(
               candidates, vertex_operand, vertex, out.geometry) &&
           valid_relation_request_key(out.key);
  }
  relation_request_id lineage_request{0};
  std::uint8_t endpoint_role = 0;
  bool has_lineage = false;
  bool ambiguous_lineage = false;
  for (const auto &occurrence : node.occurrences)
    for (const auto &lineage : occurrence.event_lineages) {
      relation_request_id request{0};
      if (lineage.contact_lineage == 0 ||
          ((lineage.contact_lineage - 1) & 1U) != 0)
        return false;
      request = relation_request_id((lineage.contact_lineage - 1) / 2);
      if (!has_lineage) {
        lineage_request = request;
        endpoint_role = lineage.endpoint_role;
        has_lineage = true;
      } else if (request != lineage_request ||
                 endpoint_role != lineage.endpoint_role) {
        ambiguous_lineage = true;
      }
    }
  if (has_lineage && !ambiguous_lineage &&
      lineage_request.ordinal() < edge_stage.relations.size() &&
      endpoint_role < edge_stage.relations[lineage_request.ordinal()].points.size()) {
    const auto &key =
        edge_stage.request_graph.requests[lineage_request.ordinal()].key;
    const auto &point =
        edge_stage.relations[lineage_request.ordinal()].points[endpoint_role];
    out = {};
    out.key = verifier_construction_key(
        key, 2, endpoint_role);
    out.source_relation = key;
    out.precedence =
        relation_construction_precedence::source_edge_source_edge_point;
    out.source_feature = key.first;
    out.geometry = verifier_geometry_from_point(
        point.point, point.accepted_source_vertex, point.tolerance_compatible);
    out.certificate = point.certificate;
    out.source_occurrence = endpoint_role;
    if (valid_relation_request_key(out.key) &&
        verifier_valid_geometry(out.geometry))
      return true;
  }
  out = {};
  out.key = verifier_construction_key(
      source_key, 4, static_cast<std::uint32_t>(node.id));
  out.source_relation = source_key;
  out.precedence = relation_construction_precedence::coplanar_overlap_endpoint;
  out.source_feature = source_key.first;
  out.geometry = verifier_geometry_from_projected(
      node.representative, source.facets[0].dropped_axis);
  out.certificate = node.certificate;
  out.source_occurrence = static_cast<std::uint32_t>(node.id);
  return valid_relation_request_key(out.key) &&
         verifier_valid_geometry(out.geometry);
}

} // namespace

template <class T, class I>
bool verify_signed_feature_relations(
    const signed_feature_relations<T, I> &artifact,
    bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = verifier_error(subcode, summary);
    return false;
  };
  if (artifact.schema_version_ != contract_versions::relation_artifact_schema ||
      artifact.provider_version_ != contract_versions::relation_provider ||
      artifact.graph_policy_version_ != contract_versions::relation_graph_policy ||
      artifact.truth_policy_version_ != contract_versions::relation_truth_policy ||
      artifact.codec_version_ != contract_versions::relation_codec ||
      artifact.verifier_version_ != contract_versions::relation_verifier ||
      artifact.provider_ !=
          relation_provider_kind::canonical_source_feature_relation_graph_v1 ||
      artifact.verification_ !=
          relation_verification_disposition::independently_verified ||
      !artifact.owner_.anchor ||
      !artifact.request_graph_.owner.same_owner(artifact.owner_) ||
      !valid_operation(artifact.operation_) ||
      !finite_bits(artifact.residual_boundary_) ||
      artifact.residual_boundary_ < T(0) ||
      artifact.symbolic_policy_digest_ != materialize_symbolic_policy().digest)
    return fail(relation_subcode::unsupported_version,
                "Component 07 version, provider, policy, or owner mismatch");

  bounded_boolean_error graph_error;
  if (!verify_relation_request_graph(artifact.request_graph_, graph_error)) {
    error = graph_error;
    return false;
  }
  const auto &authority = artifact.execution_authority_;
  if (authority.schema_version !=
          contract_versions::relation_execution_authority_schema ||
      authority.graph_policy_version != contract_versions::relation_graph_policy ||
      !authority.owner.same_owner(artifact.owner_) ||
      !authority.closed_before_evaluation || !authority.independently_verified ||
      authority.reserved != 0 ||
      !verify_relation_request_graph(authority.graph, graph_error) ||
      authority.semantic_digest != sha256::digest(
          encode_relation_execution_authority_semantics(authority)))
    return fail(relation_subcode::unclosed_dependency,
                "Component 07 pre-evaluation execution authority is invalid");
  if (artifact.graph_digest_ != artifact.request_graph_.semantic_digest)
    return fail(relation_subcode::digest_mismatch,
                "Component 07 graph digest mismatch");
  if (!artifact.candidates_ || !artifact.source_edge_stage_ ||
      !artifact.source_edge_facet_stage_ || !artifact.source_facet_stage_ ||
      !artifact.coplanar_overlay_stage_)
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 detailed predecessor stages are missing");
  if (!execution_authorizes(authority,
                            artifact.source_edge_stage_->request_graph) ||
      !execution_authorizes(authority,
                            artifact.source_edge_facet_stage_->request_graph) ||
      !execution_authorizes(authority,
                            artifact.source_facet_stage_->request_graph))
    return fail(relation_subcode::unclosed_dependency,
                "Component 07 numerical producer escaped execution authority");
  for (const auto &link : artifact.coplanar_overlay_stage_->links) {
    if (link.support_relation.ordinal() >=
        artifact.source_facet_stage_->request_graph.requests.size())
      return fail(relation_subcode::unclosed_dependency,
                  "Component 07 coplanar producer has no support authority");
    auto key = artifact.source_facet_stage_->request_graph
                   .requests[link.support_relation.ordinal()]
                   .key;
    key.family = relation_request_family::coplanar_source_facet_overlay;
    const auto planned = std::lower_bound(
        authority.graph.requests.begin(), authority.graph.requests.end(), key,
        [](const canonical_relation_request &request,
           const relation_request_key &candidate) {
          return request.key < candidate;
        });
    if (planned == authority.graph.requests.end() || planned->key != key)
      return fail(relation_subcode::unclosed_dependency,
                  "Component 07 coplanar evaluation escaped execution authority");
  }
  if (!artifact.candidates_->owner().same_owner(artifact.owner_) ||
      artifact.candidates_->candidate_digest() != artifact.candidate_digest_ ||
      artifact.candidates_->verification() !=
          broad_phase_verification_disposition::independently_verified)
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 candidate predecessor handshake failed");

  std::map<std::pair<relation_request_id, std::uint32_t>, std::uint32_t>
      canonical_edge_facet_occurrences;
  for (const auto &entry : artifact.source_edge_facet_stage_->ordered_events)
    if (!canonical_edge_facet_occurrences
             .emplace(std::make_pair(entry.relation, entry.local_event),
                      entry.canonical_occurrence)
             .second)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 canonical event occurrence is duplicated");
  const auto canonical_event_occurrence =
      [&](relation_request_id relation, std::uint32_t local_event,
          std::uint32_t &occurrence) {
        const auto found = canonical_edge_facet_occurrences.find(
            std::make_pair(relation, local_event));
        if (found == canonical_edge_facet_occurrences.end())
          return false;
        occurrence = found->second;
        return true;
      };

  struct verifier_overlay_descriptor final {
    relation_request_key key{};
    std::size_t ordinal = 0;
  };
  std::vector<verifier_overlay_descriptor> overlay_descriptors;
  overlay_descriptors.reserve(artifact.coplanar_overlay_stage_->overlays.size());
  if (artifact.coplanar_overlay_stage_->links.size() !=
      artifact.coplanar_overlay_stage_->overlays.size())
    return fail(relation_subcode::coplanar_overlay_invariant,
                "Component 07 coplanar overlay links are incomplete");
  for (std::size_t i = 0;
       i < artifact.coplanar_overlay_stage_->overlays.size(); ++i) {
    const auto &link = artifact.coplanar_overlay_stage_->links[i];
    if (link.overlay_ordinal != i || link.reserved != 0 ||
        link.support_relation.ordinal() >=
            artifact.source_facet_stage_->request_graph.requests.size())
      return fail(relation_subcode::coplanar_overlay_dependency_missing,
                  "Component 07 coplanar overlay support link is malformed");
    auto key = artifact.source_facet_stage_->request_graph
                   .requests[link.support_relation.ordinal()]
                   .key;
    key.family = relation_request_family::coplanar_source_facet_overlay;
    key.directed_use = 0;
    key.occurrence_discriminator = 0;
    overlay_descriptors.push_back({key, i});
  }
  std::sort(overlay_descriptors.begin(), overlay_descriptors.end(),
            [](const verifier_overlay_descriptor &a,
               const verifier_overlay_descriptor &b) { return a.key < b.key; });
  for (std::size_t i = 1; i < overlay_descriptors.size(); ++i)
    if (overlay_descriptors[i - 1].key == overlay_descriptors[i].key)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 coplanar overlay key is duplicated");

  const auto vertex_facet_count = static_cast<std::size_t>(std::count_if(
      artifact.execution_authority_.graph.requests.begin(),
      artifact.execution_authority_.graph.requests.end(),
      [](const canonical_relation_request &request) {
        return request.key.family ==
               relation_request_family::source_point_source_facet_region;
      }));
  const std::size_t expected_relations = vertex_facet_count +
      artifact.source_edge_stage_->relations.size() +
      artifact.source_edge_facet_stage_->relations.size() +
      artifact.source_facet_stage_->relations.size() +
      artifact.coplanar_overlay_stage_->overlays.size();
  if (artifact.relations_.size() != expected_relations)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 final relation table is incomplete");

  std::map<relation_request_key, feature_relation_id> relation_by_key;
  for (std::size_t i = 0; i < artifact.relations_.size(); ++i) {
    const auto &record = artifact.relations_[i];
    if (record.id.ordinal() != i ||
        record.producer.ordinal() >= artifact.request_graph_.requests.size() ||
        record.status == feature_relation_status::not_evaluated ||
        record.truth_begin > artifact.truth_records_.size() ||
        record.truth_count > artifact.truth_records_.size() - record.truth_begin ||
        record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 feature relation is malformed");
    const auto &producer =
        artifact.request_graph_.requests[record.producer.ordinal()];
    if (producer.key.scope != record.scope ||
        (record.scope == relation_record_scope::public_source_feature &&
         producer.key.scope != relation_record_scope::public_source_feature) ||
        !relation_by_key.emplace(producer.key, record.id).second)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 relation producer mapping is inconsistent");

    feature_relation_status expected = feature_relation_status::not_evaluated;
    switch (producer.key.family) {
    case relation_request_family::source_point_source_facet_region: {
      if (producer.key.first.kind != relation_feature_kind::source_vertex ||
          producer.key.second.kind != relation_feature_kind::source_facet ||
          record.family !=
              feature_relation_family::source_vertex_source_facet ||
          record.truth_count != 1 ||
          record.truth_begin >= artifact.truth_records_.size())
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source-vertex/facet relation is malformed");
      const auto &truth = artifact.truth_records_[record.truth_begin];
      const relation_source_facet_region_record<T> *region = nullptr;
      for (const auto &candidate : artifact.source_facet_regions_)
        if (candidate.source_relation == record.id &&
            candidate.kind == relation_source_facet_region_kind::
                                  source_vertex_source_facet) {
          if (region)
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 source-vertex/facet region is duplicated");
          region = &candidate;
        }
      if (source_edge_relation_detail::accepted_nonzero<T>(truth)) {
        if (region)
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 off-support source vertex has region evidence");
        expected = feature_relation_status::definitely_separated;
      } else if (source_edge_relation_detail::zero_tie<T>(truth) && region) {
        expected = region->region.classification ==
                           source_facet_point_region_class::outside
                       ? feature_relation_status::definitely_separated
                       : feature_relation_status::point_contact;
      } else {
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source-vertex/facet truth is incomplete");
      }
      break;
    }
    case relation_request_family::source_edge_source_edge: {
      const auto *request = find_request(artifact.source_edge_stage_->request_graph,
                                         producer.key);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 edge relation does not map to its detailed producer");
      const auto &source =
          artifact.source_edge_stage_->relations[request->id.ordinal()];
      source_edge_support_class derived_support;
      source_edge_contact_class derived_contact;
      source_edge_orientation_relation derived_orientation;
      if (!verifier_derive_edge_classification(
              source, derived_support, derived_contact, derived_orientation) ||
          source.support != derived_support || source.contact != derived_contact ||
          source.orientation != derived_orientation)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge category does not reconstruct from truth and parameter evidence");
      expected = edge_status(derived_contact, derived_orientation);
      if (record.family != feature_relation_family::source_edge_source_edge)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge relation family mismatch");
      break;
    }
    case relation_request_family::source_edge_source_facet: {
      const auto *request = find_request(
          artifact.source_edge_facet_stage_->request_graph, producer.key);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 edge/facet relation does not map to its detailed producer");
      const auto &source = artifact.source_edge_facet_stage_->relations[
          request->id.ordinal()];
      source_edge_facet_support_class derived_support;
      source_edge_facet_contact_class derived_contact;
      if (!verifier_derive_edge_facet_classification(
              source, derived_support, derived_contact) ||
          source.support != derived_support || source.contact != derived_contact)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge/facet category does not reconstruct from truth, region, event, and partition evidence");
      expected = edge_facet_status(derived_contact);
      if (record.family != feature_relation_family::source_edge_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge/facet relation family mismatch");
      break;
    }
    case relation_request_family::source_facet_source_facet: {
      const auto *request = find_request(artifact.source_facet_stage_->request_graph,
                                         producer.key);
      if (!request || request->id.ordinal() >=
                          artifact.source_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 facet relation does not map to its detailed producer");
      const auto &source =
          artifact.source_facet_stage_->relations[request->id.ordinal()];
      source_facet_support_relation_class derived_classification;
      if (!verifier_derive_facet_classification(source,
                                                derived_classification) ||
          source.classification != derived_classification)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 facet category does not reconstruct from exact support and carrier evidence");
      expected = facet_status(derived_classification);
      if (record.family != feature_relation_family::source_facet_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 facet relation family mismatch");
      break;
    }
    case relation_request_family::coplanar_source_facet_overlay: {
      const auto descriptor = std::lower_bound(
          overlay_descriptors.begin(), overlay_descriptors.end(), producer.key,
          [](const verifier_overlay_descriptor &candidate,
             const relation_request_key &key) { return candidate.key < key; });
      if (descriptor == overlay_descriptors.end() ||
          !(descriptor->key == producer.key))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 overlay relation does not map to its exact support lineage");
      const auto &source =
          artifact.coplanar_overlay_stage_->overlays[descriptor->ordinal];
      coplanar_facet_overlay_class derived_classification;
      if (!verifier_derive_overlay_classification(source,
                                                  derived_classification) ||
          source.classification != derived_classification)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 overlay category does not reconstruct from boundary, region, and component topology");
      expected = overlay_status(derived_classification);
      if (record.family != feature_relation_family::source_facet_source_facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 overlay relation family mismatch");
      break;
    }
    default:
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 generic relation has a non-authoritative producer family");
    }
    if (record.status != expected)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 generic relation status disagrees with detailed evidence");
  }

  std::vector<relation_request_key> expected_imports;
  expected_imports.reserve(artifact.relations_.size() * 2U);
  for (const auto &relation : artifact.relations_) {
    const auto &key = artifact.request_graph_.requests[
                          relation.producer.ordinal()]
                          .key;
    expected_imports.push_back(verifier_imported_geometry_key(
        key.semantic_namespace, key.first, relation.scope));
    if (key.second.kind != relation_feature_kind::none)
      expected_imports.push_back(verifier_imported_geometry_key(
          key.semantic_namespace, key.second, relation.scope));
  }
  std::sort(expected_imports.begin(), expected_imports.end());
  expected_imports.erase(
      std::unique(expected_imports.begin(), expected_imports.end()),
      expected_imports.end());
  if (artifact.imported_geometry_.size() != expected_imports.size() ||
      artifact.bounded_primitives_.size() != artifact.truth_records_.size() ||
      artifact.truth_lineage_.size() != artifact.truth_records_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 primitive support tables are incomplete");
  for (std::size_t i = 0; i < expected_imports.size(); ++i) {
    const auto &record = artifact.imported_geometry_[i];
    const auto *producer = find_request(artifact.request_graph_,
                                        expected_imports[i]);
    if (!producer || record.id.ordinal() != i ||
        record.producer != producer->id ||
        record.feature != expected_imports[i].first ||
        record.scope != expected_imports[i].scope || record.reserved8 != 0 ||
        record.reserved16 != 0 || record.reserved32 != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 imported geometry record does not reconstruct");
  }

  std::size_t exact_ordinal = 0;
  for (const auto &relation : artifact.relations_) {
    const auto &relation_request = artifact.request_graph_.requests[
        relation.producer.ordinal()];
    for (std::uint64_t local = 0; local < relation.truth_count; ++local) {
      const auto truth_index = relation.truth_begin + local;
      if (truth_index >= artifact.truth_records_.size() ||
          local > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 primitive truth range is malformed");
      const auto &truth = artifact.truth_records_[truth_index];
      const auto &bounded = artifact.bounded_primitives_[truth_index];
      const auto &lineage = artifact.truth_lineage_[truth_index];
      const auto bounded_key = verifier_derived_key(
          relation_request.key,
          relation_request_family::rounded_bounded_primitive,
          verifier_tagged_use(
              0x10U, static_cast<std::uint8_t>(relation_request.key.family)),
          static_cast<std::uint32_t>(local));
      const auto *bounded_producer =
          find_request(artifact.request_graph_, bounded_key);
      if (!bounded_producer || bounded.id.ordinal() != truth_index ||
          bounded.producer != bounded_producer->id ||
          bounded.source_relation != relation.id ||
          bounded.truth_ordinal != local ||
          bounded.rounded_nominal_bits != truth.rounded_nominal_bits ||
          bounded.bounded_sign != truth.bounded_sign ||
          bounded.disposition != truth.disposition ||
          bounded.rounded_formula != truth.rounded_formula ||
          encode_relation_truth_record_semantics(bounded.evidence) !=
              encode_relation_truth_record_semantics(truth) ||
          bounded.reserved16 != 0 || bounded.reserved32 != 0 ||
          lineage.id.ordinal() != truth_index ||
          lineage.source_relation != relation.id ||
          lineage.truth_ordinal != local ||
          lineage.bounded_primitive != bounded.id || lineage.reserved8 != 0 ||
          lineage.reserved16 != 0 || lineage.reserved32 != 0 ||
          !request_has_dependency(artifact.request_graph_, relation_request,
                                  bounded.producer))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 bounded primitive lineage does not reconstruct");

      const auto check_import_dependency =
          [&](const relation_feature_key &feature) {
            const auto key = verifier_imported_geometry_key(
                relation_request.key.semantic_namespace, feature,
                relation.scope);
            const auto *import = find_request(artifact.request_graph_, key);
            return import && request_has_dependency(
                                 artifact.request_graph_, *bounded_producer,
                                 import->id);
          };
      if (!check_import_dependency(relation_request.key.first) ||
          (relation_request.key.second.kind != relation_feature_kind::none &&
           !check_import_dependency(relation_request.key.second)))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 bounded primitive omits imported geometry lineage");

      if (truth.exact_formula != 0) {
        if (exact_ordinal >= artifact.exact_relations_.size())
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 exact relation table is truncated");
        const auto &exact = artifact.exact_relations_[exact_ordinal];
        const auto exact_key = verifier_derived_key(
            relation_request.key,
            relation_request_family::exact_stored_coordinate_relation,
            verifier_tagged_use(
                0x11U, static_cast<std::uint8_t>(relation_request.key.family)),
            static_cast<std::uint32_t>(local));
        const auto *exact_producer =
            find_request(artifact.request_graph_, exact_key);
        if (!exact_producer || exact.id.ordinal() != exact_ordinal ||
            exact.producer != exact_producer->id ||
            exact.source_relation != relation.id ||
             exact.truth_ordinal != local || exact.status != truth.exact_relation ||
             exact.exact_formula != truth.exact_formula ||
             encode_relation_truth_record_semantics(exact.evidence) !=
                 encode_relation_truth_record_semantics(truth) ||
            exact.reserved16 != 0 || exact.reserved32 != 0 ||
            !lineage.has_exact_relation ||
            lineage.exact_relation != exact.id ||
            !request_has_dependency(artifact.request_graph_, relation_request,
                                    exact.producer))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 exact relation lineage does not reconstruct");
        const auto check_exact_import =
            [&](const relation_feature_key &feature) {
              const auto key = verifier_imported_geometry_key(
                  relation_request.key.semantic_namespace, feature,
                  relation.scope);
              const auto *import = find_request(artifact.request_graph_, key);
              return import && request_has_dependency(
                                   artifact.request_graph_, *exact_producer,
                                   import->id);
            };
        if (!check_exact_import(relation_request.key.first) ||
            (relation_request.key.second.kind != relation_feature_kind::none &&
             !check_exact_import(relation_request.key.second)))
          return fail(relation_subcode::missing_dependency,
                      "Component 07 exact relation omits imported geometry lineage");
        ++exact_ordinal;
      } else if (lineage.has_exact_relation) {
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 unavailable exact truth has an exact producer");
      }
    }
  }
  if (exact_ordinal != artifact.exact_relations_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 exact relation table has trailing records");

  for (const auto &truth : artifact.truth_records_)
    if (truth.reserved != 0 ||
        truth.bounded_sign == bounded_sign_status::invalid ||
        truth.exact_relation == exact_relation_status::invalid ||
        truth.disposition == predicate_disposition::fail_invalid)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 truth record is invalid");

  struct expected_interval_evidence final {
    relation_request_key key{};
    feature_relation_id source_relation{0};
    relation_interval_evidence_record value{};
  };
  struct expected_region_evidence final {
    relation_request_key key{};
    feature_relation_id source_relation{0};
    relation_source_facet_region_record<T> value{};
  };
  std::vector<expected_interval_evidence> expected_intervals;
  std::vector<expected_region_evidence> expected_regions;

  const auto accepted_unit_interval = [](const finite_interval<T> &interval) {
    return finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
           !finite_numeric_less(interval.upper(), interval.lower()) &&
           interval.lower() >= T(0) && interval.upper() <= T(1);
  };
  const auto accepted_residual = [](const finite_interval<T> &interval,
                                    T boundary) {
    return finite_bits(boundary) && boundary >= T(0) &&
           finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
           !finite_numeric_less(interval.upper(), interval.lower()) &&
           interval.lower() >= -boundary && interval.upper() <= boundary;
  };
  const auto set_contributors = [](relation_interval_evidence_record &out,
                                   const uncertainty_contributors &value) {
    const double contributors[]{value.inherited_a, value.inherited_b,
                                value.machine_floor, value.construction,
                                value.conditioning, value.conversion,
                                value.prior_cleanup, value.current_cleanup};
    for (std::size_t i = 0; i < out.contributor_bits.size(); ++i)
      out.contributor_bits[i] =
          static_cast<std::uint64_t>(to_bits(contributors[i]));
  };
  const auto primitive_dependencies = [&](const feature_relation_record &relation,
                                          const relation_request_key &base,
                                          std::vector<relation_request_id> &out) {
    out.clear();
    const auto add = [&](const relation_request_key &key) {
      const auto *request = find_request(artifact.request_graph_, key);
      if (!request) return false;
      out.push_back(request->id);
      return true;
    };
    if (!add(verifier_imported_geometry_key(base.semantic_namespace, base.first,
                                            relation.scope)) ||
        (base.second.kind != relation_feature_kind::none &&
         !add(verifier_imported_geometry_key(base.semantic_namespace,
                                             base.second, relation.scope))))
      return false;
    for (std::uint64_t local = 0; local < relation.truth_count; ++local) {
      if (local > std::numeric_limits<std::uint32_t>::max()) return false;
      const auto truth_index = relation.truth_begin + local;
      if (truth_index >= artifact.truth_records_.size()) return false;
      if (!add(verifier_derived_key(
              base, relation_request_family::rounded_bounded_primitive,
              verifier_tagged_use(0x10U,
                                  static_cast<std::uint8_t>(base.family)),
              static_cast<std::uint32_t>(local))))
        return false;
      if (artifact.truth_records_[truth_index].exact_formula != 0 &&
          !add(verifier_derived_key(
              base,
              relation_request_family::exact_stored_coordinate_relation,
              verifier_tagged_use(0x11U,
                                  static_cast<std::uint8_t>(base.family)),
              static_cast<std::uint32_t>(local))))
        return false;
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return true;
  };
  const auto exact_dependencies = [&](const canonical_relation_request &request,
                                      std::vector<relation_request_id> expected) {
    if (request.dependency_begin > artifact.request_graph_.dependencies.size() ||
        request.dependency_count >
            artifact.request_graph_.dependencies.size() - request.dependency_begin)
      return false;
    std::vector<relation_request_id> actual;
    actual.reserve(request.dependency_count);
    for (std::uint64_t offset = 0; offset < request.dependency_count; ++offset)
      actual.push_back(artifact.request_graph_
                           .dependencies[request.dependency_begin + offset]
                           .producer);
    std::sort(actual.begin(), actual.end());
    actual.erase(std::unique(actual.begin(), actual.end()), actual.end());
    std::sort(expected.begin(), expected.end());
    expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
    return actual == expected;
  };
  const auto next_interval = [](relation_interval_evidence_kind kind,
                                 std::array<std::uint64_t, 18> &counters,
                                std::uint32_t &out) {
    const auto index = static_cast<std::size_t>(kind);
    if (index == 0 || index >= counters.size() ||
        counters[index] > std::numeric_limits<std::uint32_t>::max())
      return false;
    out = static_cast<std::uint32_t>(counters[index]++);
    return true;
  };
  const auto next_region = [](relation_source_facet_region_kind kind,
                               std::array<std::uint64_t, 10> &counters,
                              std::uint32_t &out) {
    const auto index = static_cast<std::size_t>(kind);
    if (index == 0 || index >= counters.size() ||
        counters[index] > std::numeric_limits<std::uint32_t>::max())
      return false;
    out = static_cast<std::uint32_t>(counters[index]++);
    return true;
  };
  const auto append_interval = [&](const relation_request_key &base,
                                   feature_relation_id source_relation,
                                   relation_interval_evidence_kind kind,
                                   std::uint32_t occurrence,
                                   std::uint8_t component,
                                   const finite_interval<T> &interval,
                                   bool has_rounded_nominal, T rounded,
                                   bool has_parameter_metadata,
                                   parameter_domain_status domain,
                                   T domain_margin,
                                   exact_relation_status exact_zero,
                                   exact_relation_status exact_one,
                                   const uncertainty_contributors &contributors,
                                   std::uint64_t trace_root,
                                   T comparison_boundary,
                                   bool within_authorized_boundary) {
    if (!finite_bits(interval.lower()) || !finite_bits(interval.upper()) ||
        finite_numeric_less(interval.upper(), interval.lower()) ||
        (has_rounded_nominal &&
         (!finite_bits(rounded) || !interval.contains(rounded))) ||
        !finite_bits(domain_margin) || !finite_bits(comparison_boundary))
      return false;
    expected_interval_evidence expected;
    expected.key = verifier_derived_key(
        base, relation_request_family::source_point_source_facet_region,
        verifier_tagged_use(0x20U, static_cast<std::uint8_t>(kind)) |
            static_cast<std::uint64_t>(component),
        occurrence);
    expected.source_relation = source_relation;
    expected.value.kind = kind;
    expected.value.occurrence = occurrence;
    expected.value.component = component;
    expected.value.has_rounded_nominal = has_rounded_nominal;
    expected.value.has_parameter_metadata = has_parameter_metadata;
    expected.value.within_authorized_boundary = within_authorized_boundary;
    expected.value.rounded_nominal_bits =
        has_rounded_nominal ? static_cast<std::uint64_t>(to_bits(rounded)) : 0;
    expected.value.lower_bits =
        static_cast<std::uint64_t>(to_bits(interval.lower()));
    expected.value.upper_bits =
        static_cast<std::uint64_t>(to_bits(interval.upper()));
    expected.value.domain = has_parameter_metadata
                                ? domain
                                : parameter_domain_status::invalid;
    expected.value.domain_margin_bits =
        has_parameter_metadata
            ? static_cast<std::uint64_t>(to_bits(domain_margin))
            : 0;
    expected.value.exact_zero = has_parameter_metadata
                                    ? exact_zero
                                    : exact_relation_status::unavailable;
    expected.value.exact_one = has_parameter_metadata
                                   ? exact_one
                                   : exact_relation_status::unavailable;
    if (has_parameter_metadata)
      set_contributors(expected.value, contributors);
    expected.value.trace_root = has_parameter_metadata ? trace_root : 0;
    expected.value.comparison_boundary_bits =
        static_cast<std::uint64_t>(to_bits(comparison_boundary));
    expected_intervals.push_back(std::move(expected));
    return true;
  };
  const auto append_parameter = [&](const relation_request_key &base,
                                    feature_relation_id source_relation,
                                    relation_interval_evidence_kind kind,
                                    std::uint32_t occurrence,
                                    const source_edge_parameter_evidence<T> &p) {
    return append_interval(base, source_relation, kind, occurrence, 0,
                           p.enclosure, true, p.rounded_nominal, true, p.domain,
                           p.domain_margin, p.exact_zero, p.exact_one,
                           p.contributors, p.trace_root, T(0),
                           p.domain != parameter_domain_status::invalid);
  };
  const auto append_simple_parameter =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_interval_evidence_kind kind, std::uint32_t occurrence,
          T rounded, const finite_interval<T> &interval) {
        return append_interval(
            base, source_relation, kind, occurrence, 0, interval, true, rounded,
            false, parameter_domain_status::invalid, T(0),
            exact_relation_status::unavailable,
            exact_relation_status::unavailable, uncertainty_contributors{}, 0,
            T(0), accepted_unit_interval(interval));
      };
  const auto append_plain =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_interval_evidence_kind kind, std::uint32_t occurrence,
          std::uint8_t component, const finite_interval<T> &interval,
          T comparison_boundary, bool accepted) {
        return append_interval(
            base, source_relation, kind, occurrence, component, interval,
            false, T(0), false, parameter_domain_status::invalid, T(0),
            exact_relation_status::unavailable,
            exact_relation_status::unavailable, uncertainty_contributors{}, 0,
            comparison_boundary, accepted);
      };
  const auto append_region_snapshot =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_source_facet_region_kind kind, std::uint32_t occurrence,
          const source_edge_geometry_snapshot<T> &point,
          const source_facet_point_region_record<T> &region) {
        if (!valid_source_facet_point_region_record(region)) return false;
        expected_region_evidence expected;
        expected.key = verifier_derived_key(
            base, relation_request_family::source_point_source_facet_region,
            verifier_tagged_use(0x21U, static_cast<std::uint8_t>(kind)),
            occurrence);
        expected.source_relation = source_relation;
        expected.value.kind = kind;
        expected.value.occurrence = occurrence;
        expected.value.query_component_count = 3;
        expected.value.query_source_identity_valid =
            region.query_source_identity_valid;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          expected.value.query_nominal_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.rounded_nominal[axis]));
          expected.value.query_lower_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].lower()));
          expected.value.query_upper_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].upper()));
        }
        expected.value.region = region;
        expected_regions.push_back(std::move(expected));
        return true;
      };
  const auto append_region_projected =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          relation_source_facet_region_kind kind, std::uint32_t occurrence,
          const projected_source_point<T> &point,
          const source_facet_point_region_record<T> &region) {
        if (!valid_source_facet_point_region_record(region)) return false;
        expected_region_evidence expected;
        expected.key = verifier_derived_key(
            base, relation_request_family::source_point_source_facet_region,
            verifier_tagged_use(0x21U, static_cast<std::uint8_t>(kind)),
            occurrence);
        expected.source_relation = source_relation;
        expected.value.kind = kind;
        expected.value.occurrence = occurrence;
        expected.value.query_component_count = 2;
        expected.value.query_source_identity_valid =
            region.query_source_identity_valid;
        for (std::size_t axis = 0; axis < 2; ++axis) {
          expected.value.query_nominal_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.nominal[axis]));
          expected.value.query_lower_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].lower()));
          expected.value.query_upper_bits[axis] =
              static_cast<std::uint64_t>(to_bits(point.enclosure[axis].upper()));
        }
        expected.value.region = region;
        expected_regions.push_back(std::move(expected));
        return true;
      };
  const auto append_partition =
      [&](const relation_request_key &base, feature_relation_id source_relation,
          const source_facet_segment_partition_record<T> &partition,
          relation_source_facet_region_kind breakpoint_kind,
          relation_source_facet_region_kind interval_kind,
          std::array<std::uint64_t, 18> &interval_counters,
          std::array<std::uint64_t, 10> &region_counters) {
        if (!valid_source_facet_segment_partition_record(partition)) return false;
        for (const auto &contact : partition.contacts) {
          std::uint32_t occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_contact_first_parameter,
                  interval_counters, occurrence) ||
              !append_simple_parameter(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_contact_first_parameter,
                  occurrence, contact.first_rounded_parameter,
                  contact.first_parameter))
            return false;
          if (contact.kind ==
              source_facet_segment_contact_kind::boundary_overlap) {
            if (!next_interval(
                    relation_interval_evidence_kind::segment_contact_second_parameter,
                    interval_counters, occurrence) ||
                !append_simple_parameter(
                    base, source_relation,
                    relation_interval_evidence_kind::segment_contact_second_parameter,
                    occurrence, contact.second_rounded_parameter,
                    contact.second_parameter))
              return false;
          }
        }
        for (const auto &breakpoint : partition.breakpoints) {
          std::uint32_t interval_occurrence = 0, region_occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_breakpoint_parameter,
                  interval_counters, interval_occurrence) ||
              !append_simple_parameter(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_breakpoint_parameter,
                  interval_occurrence, breakpoint.rounded_parameter,
                  breakpoint.parameter) ||
              !next_region(breakpoint_kind, region_counters,
                           region_occurrence) ||
              !append_region_projected(base, source_relation, breakpoint_kind,
                                       region_occurrence, breakpoint.point,
                                       breakpoint.region))
            return false;
        }
        for (const auto &interval : partition.intervals) {
          std::uint32_t interval_occurrence = 0, region_occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_interval_witness_parameter,
                  interval_counters, interval_occurrence) ||
              !append_simple_parameter(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_interval_witness_parameter,
                  interval_occurrence, interval.rounded_witness_parameter,
                  interval.witness_parameter) ||
              !next_region(interval_kind, region_counters,
                           region_occurrence) ||
              !append_region_projected(base, source_relation, interval_kind,
                                       region_occurrence,
                                       interval.witness_point,
                                       interval.witness_region))
            return false;
        }
        for (const auto &witness : partition.triangle_witnesses) {
          std::uint32_t occurrence = 0;
          if (!next_interval(
                  relation_interval_evidence_kind::segment_triangle_witness_parameter,
                  interval_counters, occurrence) ||
              !append_plain(
                  base, source_relation,
                  relation_interval_evidence_kind::segment_triangle_witness_parameter,
                  occurrence, 0, witness.parameter, T(0),
                  accepted_unit_interval(witness.parameter)))
            return false;
        }
        return true;
      };

  for (const auto &relation : artifact.relations_) {
    const auto &base = artifact.request_graph_.requests[relation.producer.ordinal()].key;
    std::array<std::uint64_t, 18> interval_counters{};
    std::array<std::uint64_t, 10> region_counters{};
    switch (base.family) {
    case relation_request_family::source_point_source_facet_region: {
      for (const auto &published : artifact.source_facet_regions_) {
        if (published.source_relation != relation.id ||
            published.kind != relation_source_facet_region_kind::
                                  source_vertex_source_facet)
          continue;
        expected_region_evidence expected;
        expected.key = verifier_derived_key(
            base, relation_request_family::composite_contact,
            verifier_tagged_use(
                0x21U,
                static_cast<std::uint8_t>(
                    relation_source_facet_region_kind::
                        source_vertex_source_facet)),
            0);
        expected.source_relation = relation.id;
        expected.value = published;
        expected.value.id = relation_source_facet_region_id{0};
        expected.value.producer = relation_request_id{0};
        expected.value.source_relation = feature_relation_id{0};
        expected_regions.push_back(std::move(expected));
      }
      break;
    }
    case relation_request_family::source_edge_source_edge: {
      const auto *request = find_request(artifact.source_edge_stage_->request_graph,
                                         base);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 edge source is absent");
      const auto &source =
          artifact.source_edge_stage_->relations[request->id.ordinal()];
      for (std::uint32_t i = 0; i < source.parameter_count; ++i) {
        std::uint32_t occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::source_edge_first_parameter,
                interval_counters, occurrence) ||
            !append_parameter(
                base, relation.id,
                relation_interval_evidence_kind::source_edge_first_parameter,
                occurrence, source.first_parameters[i]) ||
            !next_interval(
                relation_interval_evidence_kind::source_edge_second_parameter,
                interval_counters, occurrence) ||
            !append_parameter(
                base, relation.id,
                relation_interval_evidence_kind::source_edge_second_parameter,
                occurrence, source.second_parameters[i]))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 edge parameters do not reconstruct");
      }
      for (std::uint32_t point = 0; point < source.point_count; ++point) {
        std::uint32_t first_occurrence = 0, second_occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::source_edge_first_carrier_residual,
                interval_counters, first_occurrence) ||
            !next_interval(
                relation_interval_evidence_kind::source_edge_second_carrier_residual,
                interval_counters, second_occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 edge residual occurrence overflow");
        for (std::uint8_t axis = 0; axis < 3; ++axis) {
          const auto &first = source.points[point].first_carrier_residual[axis];
          const auto &second = source.points[point].second_carrier_residual[axis];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::source_edge_first_carrier_residual,
                  first_occurrence, axis, first, source.residual_boundary,
                  accepted_residual(first, source.residual_boundary)) ||
              !append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::source_edge_second_carrier_residual,
                  second_occurrence, axis, second, source.residual_boundary,
                  accepted_residual(second, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 edge residuals do not reconstruct");
        }
      }
      break;
    }
    case relation_request_family::source_edge_source_facet: {
      const auto *request = find_request(
          artifact.source_edge_facet_stage_->request_graph, base);
      if (!request || request->id.ordinal() >=
                          artifact.source_edge_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 edge/facet source is absent");
      const auto &source = artifact.source_edge_facet_stage_->relations[
          request->id.ordinal()];
      for (const auto &event : source.events) {
        std::uint32_t parameter_occurrence = 0, residual_occurrence = 0;
        std::uint32_t support_occurrence = 0, region_occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::edge_facet_event_parameter,
                interval_counters, parameter_occurrence) ||
            !append_parameter(
                base, relation.id,
                relation_interval_evidence_kind::edge_facet_event_parameter,
                parameter_occurrence, event.parameter) ||
            !next_interval(
                relation_interval_evidence_kind::edge_facet_edge_carrier_residual,
                interval_counters, residual_occurrence) ||
            !next_interval(
                relation_interval_evidence_kind::edge_facet_support_residual,
                interval_counters, support_occurrence) ||
            !next_region(relation_source_facet_region_kind::edge_facet_event,
                         region_counters, region_occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 edge/facet occurrence overflow");
        for (std::uint8_t axis = 0; axis < 3; ++axis) {
          const auto &residual = event.construction.edge_carrier_residual[axis];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::edge_facet_edge_carrier_residual,
                  residual_occurrence, axis, residual,
                  source.residual_boundary,
                  accepted_residual(residual, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 edge/facet carrier residual does not reconstruct");
        }
        if (!append_plain(
                base, relation.id,
                relation_interval_evidence_kind::edge_facet_support_residual,
                support_occurrence, 0, event.construction.support_residual,
                source.residual_boundary,
                accepted_residual(event.construction.support_residual,
                                  source.residual_boundary)) ||
            !append_region_snapshot(
                base, relation.id,
                relation_source_facet_region_kind::edge_facet_event,
                region_occurrence, event.construction.point, event.region))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 edge/facet event does not reconstruct");
      }
      if (source.has_coplanar_partition &&
          !append_partition(
              base, relation.id, source.coplanar_partition,
              relation_source_facet_region_kind::edge_facet_partition_breakpoint,
              relation_source_facet_region_kind::edge_facet_partition_interval,
              interval_counters, region_counters))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 family-04 edge/facet partition does not reconstruct");
      // Transverse records are verified later from their construction, carrier,
      // region, crossing, and Component 03 operation lineage.  Register their
      // independently keyed slots here without invoking the producer builder.
      for (const auto &published : artifact.interval_evidence_) {
        if (published.source_relation != relation.id ||
            (published.kind != relation_interval_evidence_kind::
                                   transverse_carrier_parameter &&
             published.kind != relation_interval_evidence_kind::
                                   transverse_carrier_point_residual))
          continue;
        expected_interval_evidence expected;
        expected.source_relation = relation.id;
        expected.value = published;
        expected.value.id = relation_interval_evidence_id{0};
        expected.value.producer = relation_request_id{0};
        expected.value.source_relation = feature_relation_id{0};
        expected.key = verifier_derived_key(
            base, relation_request_family::authoritative_construction,
            published.kind == relation_interval_evidence_kind::
                                  transverse_carrier_parameter
                ? verifier_tagged_use(
                      0x30U, static_cast<std::uint8_t>(published.kind))
                : verifier_tagged_use(0x31U, published.component),
            published.occurrence);
        expected_intervals.push_back(std::move(expected));
      }
      for (const auto &published : artifact.source_facet_regions_) {
        if (published.source_relation != relation.id ||
            (published.kind != relation_source_facet_region_kind::
                                   transverse_carrier_first_facet &&
             published.kind != relation_source_facet_region_kind::
                                   transverse_carrier_second_facet))
          continue;
        expected_region_evidence expected;
        expected.source_relation = relation.id;
        expected.value = published;
        expected.value.id = relation_source_facet_region_id{0};
        expected.value.producer = relation_request_id{0};
        expected.value.source_relation = feature_relation_id{0};
        expected.key = verifier_derived_key(
            base, relation_request_family::composite_contact,
            verifier_tagged_use(0x32U,
                                static_cast<std::uint8_t>(published.kind)),
            published.occurrence);
        expected_regions.push_back(std::move(expected));
      }
      break;
    }
    case relation_request_family::source_facet_source_facet: {
      const auto *request = find_request(
          artifact.source_facet_stage_->request_graph, base);
      if (!request || request->id.ordinal() >=
                          artifact.source_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 facet source is absent");
      const auto &source =
          artifact.source_facet_stage_->relations[request->id.ordinal()];
      if (source.has_transverse_carrier) {
        std::uint32_t occurrence = 0;
        if (!next_interval(
                relation_interval_evidence_kind::facet_facet_direction_squared,
                interval_counters, occurrence) ||
            !append_plain(
                base, relation.id,
                relation_interval_evidence_kind::facet_facet_direction_squared,
                occurrence, 0, source.transverse_carrier.direction_squared,
                T(0), source.transverse_carrier.direction_squared.lower() > T(0)))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 carrier conditioning does not reconstruct");
        if (!next_interval(
                relation_interval_evidence_kind::facet_facet_point_plane_residual,
                interval_counters, occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 point-plane occurrence overflow");
        for (std::uint8_t component = 0; component < 2; ++component) {
          const auto &residual =
              source.transverse_carrier.point_plane_residuals[component];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::facet_facet_point_plane_residual,
                  occurrence, component, residual, source.residual_boundary,
                  accepted_residual(residual, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 point-plane residual does not reconstruct");
        }
        if (!next_interval(
                relation_interval_evidence_kind::facet_facet_direction_plane_residual,
                interval_counters, occurrence))
          return fail(relation_subcode::count_overflow,
                      "Component 07 family-04 direction-plane occurrence overflow");
        for (std::uint8_t component = 0; component < 2; ++component) {
          const auto &residual =
              source.transverse_carrier.direction_plane_residuals[component];
          if (!append_plain(
                  base, relation.id,
                  relation_interval_evidence_kind::facet_facet_direction_plane_residual,
                  occurrence, component, residual, source.residual_boundary,
                  accepted_residual(residual, source.residual_boundary)))
            return fail(relation_subcode::verifier_rejection,
                        "Component 07 family-04 direction-plane residual does not reconstruct");
        }
      }
      break;
    }
    case relation_request_family::coplanar_source_facet_overlay: {
      const auto descriptor = std::lower_bound(
          overlay_descriptors.begin(), overlay_descriptors.end(), base,
          [](const verifier_overlay_descriptor &candidate,
             const relation_request_key &key) { return candidate.key < key; });
      if (descriptor == overlay_descriptors.end() || !(descriptor->key == base))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 family-04 overlay source is absent");
      const auto &source =
          artifact.coplanar_overlay_stage_->overlays[descriptor->ordinal];
      for (const auto &witness : source.vertex_regions) {
        if (witness.polygon >= source.facets.size() ||
            witness.vertex_ordinal >=
                source.facets[witness.polygon].polygon.size())
          return fail(relation_subcode::coplanar_overlay_region_unresolved,
                      "Component 07 family-04 overlay witness is out of range");
        std::uint32_t occurrence = 0;
        if (!next_region(
                relation_source_facet_region_kind::overlay_vertex_witness,
                region_counters, occurrence) ||
            !append_region_projected(
                base, relation.id,
                relation_source_facet_region_kind::overlay_vertex_witness,
                occurrence,
                source.facets[witness.polygon].polygon[witness.vertex_ordinal],
                witness.region))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 overlay vertex region does not reconstruct");
      }
      for (const auto &partition : source.boundary_partitions)
        if (!append_partition(
                base, relation.id, partition.partition,
                relation_source_facet_region_kind::overlay_partition_breakpoint,
                relation_source_facet_region_kind::overlay_partition_interval,
                interval_counters, region_counters))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 family-04 overlay partition does not reconstruct");
      break;
    }
    default:
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 family-04 source family is invalid");
    }
  }

  std::sort(expected_intervals.begin(), expected_intervals.end(),
            [](const expected_interval_evidence &a,
               const expected_interval_evidence &b) {
              return std::tie(a.source_relation, a.key) <
                     std::tie(b.source_relation, b.key);
            });
  std::sort(expected_regions.begin(), expected_regions.end(),
            [](const expected_region_evidence &a,
               const expected_region_evidence &b) {
              return std::tie(a.source_relation, a.key) <
                     std::tie(b.source_relation, b.key);
            });
  if (artifact.interval_evidence_.size() != expected_intervals.size() ||
      artifact.source_facet_regions_.size() != expected_regions.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 family-04 evidence tables are incomplete");

  const auto same_interval = [](const relation_interval_evidence_record &a,
                                const relation_interval_evidence_record &b) {
    return a.kind == b.kind && a.occurrence == b.occurrence &&
           a.component == b.component &&
           a.has_rounded_nominal == b.has_rounded_nominal &&
           a.has_parameter_metadata == b.has_parameter_metadata &&
           a.within_authorized_boundary == b.within_authorized_boundary &&
           a.rounded_nominal_bits == b.rounded_nominal_bits &&
           a.lower_bits == b.lower_bits && a.upper_bits == b.upper_bits &&
           a.domain == b.domain &&
           a.domain_margin_bits == b.domain_margin_bits &&
           a.exact_zero == b.exact_zero && a.exact_one == b.exact_one &&
           a.contributor_bits == b.contributor_bits &&
           a.trace_root == b.trace_root &&
           a.issued_operation == b.issued_operation &&
           a.issued_value == b.issued_value &&
           a.issued_ledger_entry == b.issued_ledger_entry &&
           a.issued_parent_values == b.issued_parent_values &&
           a.issued_parent_trace_roots == b.issued_parent_trace_roots &&
           a.issued_parent_ledger_entries ==
               b.issued_parent_ledger_entries &&
           a.issued_operation_evidence == b.issued_operation_evidence &&
           a.comparison_boundary_bits == b.comparison_boundary_bits &&
           a.reserved8 == 0 && a.reserved16 == 0 && a.reserved32 == 0;
  };
  for (std::size_t i = 0; i < expected_intervals.size(); ++i) {
    const auto &expected = expected_intervals[i];
    const auto &record = artifact.interval_evidence_[i];
    const auto *producer = find_request(artifact.request_graph_, expected.key);
    if (!producer || record.id.ordinal() != i ||
        record.producer != producer->id ||
        record.source_relation != expected.source_relation ||
        !same_interval(record, expected.value))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 interval evidence does not reconstruct");
    const auto &base_request = artifact.request_graph_.requests[
        artifact.relations_[record.source_relation.ordinal()].producer.ordinal()];
    std::vector<relation_request_id> dependencies;
    const bool transverse =
        record.kind ==
            relation_interval_evidence_kind::transverse_carrier_parameter ||
        record.kind == relation_interval_evidence_kind::
                           transverse_carrier_point_residual;
    if (!primitive_dependencies(
            artifact.relations_[record.source_relation.ordinal()],
            base_request.key, dependencies) ||
        (!transverse && !exact_dependencies(*producer, dependencies)) ||
        (transverse &&
         (!request_has_dependency(artifact.request_graph_, *producer,
                                  base_request.id) ||
          producer->dependency_count != 2)) ||
        (!request_has_dependency(artifact.request_graph_, base_request,
                                 producer->id) &&
         !transverse))
      return fail(relation_subcode::missing_dependency,
                  "Component 07 interval evidence dependency closure is incomplete");
  }

  const auto region_bytes = [](const source_facet_point_region_record<T> &region) {
    canonical_writer writer;
    source_edge_facet_detail::encode_region(writer, region);
    return writer.take();
  };
  for (std::size_t i = 0; i < expected_regions.size(); ++i) {
    const auto &expected = expected_regions[i];
    const auto &record = artifact.source_facet_regions_[i];
    const auto *producer = find_request(artifact.request_graph_, expected.key);
    if (!producer || record.id.ordinal() != i ||
        record.producer != producer->id ||
        record.source_relation != expected.source_relation ||
        record.kind != expected.value.kind ||
        record.occurrence != expected.value.occurrence ||
        record.query_component_count != expected.value.query_component_count ||
        record.query_source_identity_valid !=
            expected.value.query_source_identity_valid ||
        record.query_nominal_bits != expected.value.query_nominal_bits ||
        record.query_lower_bits != expected.value.query_lower_bits ||
        record.query_upper_bits != expected.value.query_upper_bits ||
        !valid_source_facet_point_region_record(record.region) ||
        region_bytes(record.region) != region_bytes(expected.value.region) ||
        record.reserved8 != 0 || record.reserved16 != 0 ||
        record.reserved32 != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 source-facet region evidence does not reconstruct");
    const auto &base_request = artifact.request_graph_.requests[
        artifact.relations_[record.source_relation.ordinal()].producer.ordinal()];
    std::vector<relation_request_id> dependencies;
    const bool transverse =
        record.kind == relation_source_facet_region_kind::
                           transverse_carrier_first_facet ||
        record.kind == relation_source_facet_region_kind::
                           transverse_carrier_second_facet;
    const bool source_vertex =
        record.kind ==
        relation_source_facet_region_kind::source_vertex_source_facet;
    if (!primitive_dependencies(
            artifact.relations_[record.source_relation.ordinal()],
            base_request.key, dependencies) ||
        (!transverse && !source_vertex &&
         !exact_dependencies(*producer, dependencies)) ||
        (transverse &&
         (!request_has_dependency(artifact.request_graph_, *producer,
                                  base_request.id) ||
          producer->dependency_count != 2)) ||
        (source_vertex &&
         (!request_has_dependency(artifact.request_graph_, *producer,
                                  base_request.id) ||
          producer->dependency_count != 1)) ||
        (!request_has_dependency(artifact.request_graph_, base_request,
                                 producer->id) &&
         !transverse && !source_vertex))
      return fail(relation_subcode::missing_dependency,
                  "Component 07 source-facet region dependency closure is incomplete");
  }

  struct expected_construction_use final {
    relation_request_key key{};
    relation_request_key source_relation{};
    relation_request_key authoritative_source_relation{};
    relation_construction_precedence authority_precedence =
        relation_construction_precedence::verification_witness;
    relation_feature_key authoritative_source_feature{};
    verifier_geometry_snapshot<T> authority_geometry{};
    construction_operation_certificate<T> authority_certificate{};
    relation_construction_precedence witness_precedence =
        relation_construction_precedence::verification_witness;
    verifier_geometry_snapshot<T> witness_geometry{};
    construction_operation_certificate<T> witness_certificate{};
    std::uint32_t occurrence = 0;
  };
  std::vector<expected_construction_use> expected_uses;
  const auto append_expected_use =
      [&](const relation_request_key &source_relation,
          const verifier_construction_authority<T> &authority,
           relation_construction_precedence witness_precedence,
           const verifier_geometry_snapshot<T> &witness,
           const construction_operation_certificate<T> &witness_certificate,
           std::uint32_t occurrence) {
        if (!valid_relation_request_key(authority.key) ||
            !valid_relation_request_key(authority.source_relation) ||
            !verifier_valid_geometry(
                authority.geometry) ||
             !verifier_valid_geometry(witness) ||
             !valid_construction_operation_certificate(authority.certificate) ||
             !valid_construction_operation_certificate(witness_certificate) ||
            !verifier_compatible_geometry(
                authority.geometry, witness))
          return false;
        expected_construction_use use;
        use.key = authority.key;
        use.source_relation = source_relation;
        use.authoritative_source_relation = authority.source_relation;
        use.authority_precedence = authority.precedence;
        use.authoritative_source_feature = authority.source_feature;
        use.authority_geometry = authority.geometry;
        use.authority_certificate = authority.certificate;
        use.witness_precedence = witness_precedence;
        use.witness_geometry = witness;
        use.witness_certificate = witness_certificate;
        use.occurrence = occurrence;
        expected_uses.push_back(std::move(use));
        return true;
      };

  for (std::size_t relation_index = 0;
       relation_index < artifact.source_edge_stage_->relations.size();
       ++relation_index) {
    if (relation_index >=
        artifact.source_edge_stage_->request_graph.requests.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 edge construction source request is absent");
    const auto &source =
        artifact.source_edge_stage_->relations[relation_index];
    const auto &key =
        artifact.source_edge_stage_->request_graph.requests[relation_index].key;
    for (std::uint32_t point = 0; point < source.point_count; ++point) {
      verifier_construction_authority<T> authority;
      if (!verifier_edge_point_authority(
              *artifact.candidates_, key, source, point, authority) ||
          !append_expected_use(
              key, authority,
              source.points[point].accepted_source_vertex
                  ? relation_construction_precedence::accepted_source_vertex
                  : relation_construction_precedence::
                        source_edge_source_edge_point,
              verifier_geometry_from_point(
                  source.points[point].point,
                  source.points[point].accepted_source_vertex,
                   source.points[point].tolerance_compatible),
               source.points[point].certificate,
               point))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge construction authority does not reconstruct");
    }
  }

  for (std::size_t relation_index = 0;
       relation_index < artifact.source_edge_facet_stage_->relations.size();
       ++relation_index) {
    if (relation_index >=
        artifact.source_edge_facet_stage_->request_graph.requests.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 edge/facet construction source request is absent");
    const auto &source =
        artifact.source_edge_facet_stage_->relations[relation_index];
    const auto &request = artifact.source_edge_facet_stage_->request_graph
                              .requests[relation_index];
    for (std::size_t local = 0; local < source.events.size(); ++local) {
      if (local > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::count_overflow,
                    "Component 07 edge/facet construction occurrence overflows");
      std::uint32_t occurrence = 0;
      if (!canonical_event_occurrence(
              request.id, static_cast<std::uint32_t>(local), occurrence))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 edge/facet canonical occurrence is absent");
      const auto &event = source.events[local];
      verifier_construction_authority<T> authority;
      if (!verifier_edge_facet_authority(
              *artifact.candidates_, *artifact.source_edge_stage_, request.key,
              source, event, occurrence, authority) ||
          !append_expected_use(
              request.key, authority,
              event.construction.accepted_source_vertex ||
                      event.region.classification ==
                          source_facet_point_region_class::original_vertex
                  ? relation_construction_precedence::accepted_source_vertex
                  : relation_construction_precedence::
                        source_edge_source_facet_point,
              verifier_geometry_from_point(
                  event.construction.point,
                  event.construction.accepted_source_vertex,
                   event.construction.tolerance_compatible),
               event.construction.certificate,
               occurrence))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 edge/facet construction authority does not reconstruct");
    }
  }

  for (std::size_t relation_index = 0;
       relation_index < artifact.source_facet_stage_->relations.size();
       ++relation_index) {
    if (relation_index >=
        artifact.source_facet_stage_->request_graph.requests.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 facet construction source request is absent");
    const auto &source =
        artifact.source_facet_stage_->relations[relation_index];
    if (!source.has_transverse_carrier)
      continue;
    const auto &key =
        artifact.source_facet_stage_->request_graph.requests[relation_index].key;
    verifier_construction_authority<T> authority;
    if (!verifier_carrier_authority(
            key, source.transverse_carrier, authority))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 carrier construction authority does not reconstruct");
    auto witness = verifier_geometry_from_carrier(
        source.transverse_carrier);
    if (authority.geometry.lineage == 0) {
      authority.geometry.lineage =
          relation_stable_lineage(key, 0x71U);
      witness.lineage = authority.geometry.lineage;
    }
    if (authority.geometry.provenance == 0) {
      authority.geometry.provenance =
          relation_stable_lineage(key, 0x70U);
      witness.provenance = authority.geometry.provenance;
    }
    if (
        !append_expected_use(
            key, authority,
            relation_construction_precedence::
                source_facet_source_facet_carrier,
            witness, source.transverse_carrier.certificate, 0))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 carrier construction authority does not reconstruct");
  }

  for (const auto &descriptor : overlay_descriptors) {
    const auto &source =
        artifact.coplanar_overlay_stage_->overlays[descriptor.ordinal];
    for (const auto &node : source.event_nodes) {
      if (node.id > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::count_overflow,
                    "Component 07 overlay construction occurrence overflows");
      verifier_construction_authority<T> authority;
      if (!verifier_overlay_node_authority(
              *artifact.candidates_, *artifact.source_edge_stage_,
              descriptor.key, source, node, authority) ||
          !append_expected_use(
              descriptor.key, authority,
              relation_construction_precedence::coplanar_overlap_endpoint,
              verifier_geometry_from_projected(
                  node.representative, source.facets[0].dropped_axis,
                   authority.precedence ==
                       relation_construction_precedence::accepted_source_vertex),
               node.certificate,
               static_cast<std::uint32_t>(node.id)))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 overlay construction authority does not reconstruct");
    }
  }

  std::sort(expected_uses.begin(), expected_uses.end(),
            [](const expected_construction_use &a,
               const expected_construction_use &b) {
              return std::tie(a.key, a.authority_precedence,
                              a.authoritative_source_relation,
                              a.source_relation, a.occurrence) <
                     std::tie(b.key, b.authority_precedence,
                              b.authoritative_source_relation,
                              b.source_relation, b.occurrence);
            });

  struct evidence_range final {
    std::uint64_t begin = 0;
    std::uint64_t count = 0;
  };
  std::vector<evidence_range> interval_ranges(artifact.relations_.size());
  std::vector<evidence_range> region_ranges(artifact.relations_.size());
  const auto accumulate_range = [](auto &ranges, feature_relation_id relation,
                                   std::uint64_t index) {
    if (relation.ordinal() >= ranges.size()) return false;
    auto &range = ranges[relation.ordinal()];
    if (range.count == 0)
      range.begin = index;
    else if (range.begin + range.count != index)
      return false;
    ++range.count;
    return true;
  };
  for (std::size_t index = 0; index < artifact.interval_evidence_.size();
       ++index)
    if (!accumulate_range(interval_ranges,
                          artifact.interval_evidence_[index].source_relation,
                          index))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 construction interval evidence is not contiguous");
  for (std::size_t index = 0; index < artifact.source_facet_regions_.size();
       ++index)
    if (!accumulate_range(region_ranges,
                          artifact.source_facet_regions_[index].source_relation,
                          index))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 construction region evidence is not contiguous");

  const auto geometry_matches_record =
      [](const verifier_geometry_snapshot<T> &geometry,
         const relation_construction_record &record) {
        if (record.kind != geometry.kind ||
            record.coordinate_space != geometry.coordinate_space ||
            record.component_count != geometry.component_count ||
            record.projection_axis != geometry.projection_axis ||
            record.source_provenance != geometry.provenance ||
            record.geometric_lineage != geometry.lineage ||
            record.accepted_source_vertex != geometry.accepted_source_vertex ||
            record.finite != geometry.finite ||
            record.tolerance_compatible != geometry.tolerance_compatible)
          return false;
        for (std::size_t component = 0;
             component < geometry.component_count; ++component)
          if (record.nominal_bits[component] !=
                  static_cast<std::uint64_t>(to_bits(geometry.nominal[component])) ||
              record.lower_bits[component] !=
                  static_cast<std::uint64_t>(to_bits(geometry.lower[component])) ||
              record.upper_bits[component] !=
                  static_cast<std::uint64_t>(to_bits(geometry.upper[component])))
            return false;
        for (std::size_t component = geometry.component_count;
             component < record.nominal_bits.size(); ++component)
          if (record.nominal_bits[component] != 0 ||
              record.lower_bits[component] != 0 ||
              record.upper_bits[component] != 0)
            return false;
        return true;
      };
  const auto geometry_matches_ledger =
      [](const verifier_geometry_snapshot<T> &geometry,
         const relation_construction_ledger_record &record) {
        if (record.coordinate_space != geometry.coordinate_space ||
            record.component_count != geometry.component_count ||
            record.projection_axis != geometry.projection_axis ||
            record.source_provenance != geometry.provenance ||
            record.geometric_lineage != geometry.lineage ||
            record.accepted_source_vertex != geometry.accepted_source_vertex ||
            record.finite != geometry.finite ||
            record.tolerance_compatible != geometry.tolerance_compatible)
          return false;
        for (std::size_t component = 0;
             component < geometry.component_count; ++component)
          if (record.nominal_bits[component] !=
                  static_cast<std::uint64_t>(to_bits(geometry.nominal[component])) ||
              record.lower_bits[component] !=
                  static_cast<std::uint64_t>(to_bits(geometry.lower[component])) ||
              record.upper_bits[component] !=
                  static_cast<std::uint64_t>(to_bits(geometry.upper[component])))
            return false;
        for (std::size_t component = geometry.component_count;
             component < record.nominal_bits.size(); ++component)
          if (record.nominal_bits[component] != 0 ||
              record.lower_bits[component] != 0 ||
              record.upper_bits[component] != 0)
            return false;
        return true;
      };
  const auto valid_tolerance_bits = [](std::uint64_t bits) {
    using bits_type = floating_uint_t<T>;
    const auto value = from_bits<T>(static_cast<bits_type>(bits));
    return finite_bits(value) && value >= T(0);
  };
  const auto certificate_matches = [](
      const auto &record,
      const construction_operation_certificate<T> &certificate) {
    if (!valid_construction_operation_certificate(certificate) ||
        record.formula_version != certificate.formula_version ||
        record.operation != certificate.operation ||
        record.conditioning != certificate.conditioning ||
        record.tolerance != certificate.tolerance ||
        record.precision_trace_root !=
            certificate.issued_outputs[0].identity.trace_root ||
        record.radial_error_upper_bits != static_cast<std::uint64_t>(
            to_bits(certificate.radial_error_upper)) ||
        record.denominator_lower_bits != static_cast<std::uint64_t>(
            to_bits(certificate.denominator.lower())) ||
        record.denominator_upper_bits != static_cast<std::uint64_t>(
            to_bits(certificate.denominator.upper())) ||
        record.conditioning_lower_bits != static_cast<std::uint64_t>(
            to_bits(certificate.conditioning_lower)) ||
        record.tolerance_boundary_bits != static_cast<std::uint64_t>(
            to_bits(certificate.tolerance_boundary)) ||
        record.ordered_bounded_inputs.size() !=
            certificate.ordered_inputs.size())
      return false;
    for (std::size_t component = 0;
         component < certificate.axis_error_upper.size(); ++component)
      if (record.axis_error_upper_bits[component] !=
          static_cast<std::uint64_t>(
              to_bits(certificate.axis_error_upper[component])))
        return false;
    for (std::size_t input = 0; input < certificate.ordered_inputs.size(); ++input)
      if (record.ordered_bounded_inputs[input] !=
          certificate.ordered_inputs[input].value.ordinal())
        return false;
    canonical_writer encoded;
    encode_construction_operation_certificate(encoded, certificate);
    return record.certificate_evidence == encoded.take();
  };
  const auto expected_evidence =
      [&](feature_relation_id relation, std::uint64_t truth_begin,
          std::uint64_t truth_count, std::uint64_t interval_begin,
          std::uint64_t interval_count, std::uint64_t region_begin,
          std::uint64_t region_count) {
        if (relation.ordinal() >= artifact.relations_.size()) return false;
        const auto &source = artifact.relations_[relation.ordinal()];
        return truth_begin == source.truth_begin &&
               truth_count == source.truth_count &&
               interval_begin == interval_ranges[relation.ordinal()].begin &&
               interval_count == interval_ranges[relation.ordinal()].count &&
               region_begin == region_ranges[relation.ordinal()].begin &&
               region_count == region_ranges[relation.ordinal()].count;
      };

  std::size_t expected_group_count = 0;
  for (std::size_t begin = 0; begin < expected_uses.size();) {
    ++expected_group_count;
    std::size_t end_group = begin + 1;
    while (end_group < expected_uses.size() &&
           expected_uses[end_group].key == expected_uses[begin].key)
      ++end_group;
    begin = end_group;
  }
  if (artifact.constructions_.size() != expected_group_count ||
      artifact.construction_ledger_.size() !=
          expected_uses.size() + expected_group_count)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 construction registry or ledger count is incomplete");

  std::map<relation_request_key, relation_construction_id> construction_by_key;
  std::size_t construction_index = 0;
  std::size_t ledger_index = 0;
  for (std::size_t begin = 0; begin < expected_uses.size();) {
    std::size_t end_group = begin + 1;
    while (end_group < expected_uses.size() &&
           expected_uses[end_group].key == expected_uses[begin].key)
      ++end_group;
    const auto &expected = expected_uses[begin];
    canonical_writer expected_authority_certificate_writer;
    encode_construction_operation_certificate(
        expected_authority_certificate_writer, expected.authority_certificate);
    const auto expected_authority_certificate_bytes =
        expected_authority_certificate_writer.take();
    for (std::size_t index = begin; index < end_group; ++index) {
      canonical_writer candidate_certificate_writer;
      encode_construction_operation_certificate(
          candidate_certificate_writer,
          expected_uses[index].authority_certificate);
      if (expected_uses[index].authority_precedence !=
              expected.authority_precedence ||
          expected_uses[index].authoritative_source_feature !=
              expected.authoritative_source_feature ||
          !verifier_same_geometry(
              expected.authority_geometry,
              expected_uses[index].authority_geometry) ||
           !verifier_compatible_geometry(
               expected.authority_geometry,
               expected_uses[index].witness_geometry) ||
          candidate_certificate_writer.take() !=
              expected_authority_certificate_bytes)
        return fail(relation_subcode::duplicate_authoritative_producer,
                     "Component 07 reconstructed construction authorities disagree");
    }

    const auto &record = artifact.constructions_[construction_index];
    const auto *producer = find_request(artifact.request_graph_, expected.key);
    const auto source_relation =
        relation_by_key.find(expected.authoritative_source_relation);
    if (!producer || source_relation == relation_by_key.end() ||
        record.id.ordinal() != construction_index ||
        record.producer != producer->id ||
        record.source_relation != source_relation->second ||
        record.precedence != expected.authority_precedence ||
        record.authoritative_source_feature !=
            expected.authoritative_source_feature ||
        record.compatibility !=
            relation_construction_compatibility_disposition::authoritative ||
        record.schema_version != contract_versions::relation_construction_schema ||
        record.defining_feature_count !=
            (producer->key.second.kind == relation_feature_kind::none ? 1 : 2) ||
        record.defining_features[0] != producer->key.first ||
        (record.defining_feature_count == 2 &&
         record.defining_features[1] != producer->key.second) ||
        record.defining_dependency_begin != producer->dependency_begin ||
        record.defining_dependency_count != producer->dependency_count ||
        !valid_tolerance_bits(record.radial_error_upper_bits) ||
        !valid_tolerance_bits(record.conditioning_lower_bits) ||
        !certificate_matches(record, expected.authority_certificate) ||
        !geometry_matches_record(expected.authority_geometry, record) ||
        !record.precision_evidence_complete ||
        !valid_tolerance_bits(record.tolerance_boundary_bits) ||
        record.consumer_begin != producer->reverse_consumer_begin ||
        record.consumer_count != producer->reverse_consumer_count ||
        record.ledger_begin != ledger_index ||
        record.ledger_count != (end_group - begin) + 1 ||
        !expected_evidence(
            record.source_relation, record.residual_truth_begin,
            record.residual_truth_count, record.interval_evidence_begin,
            record.interval_evidence_count, record.source_facet_region_begin,
            record.source_facet_region_count) ||
        record.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 authoritative construction does not reconstruct");
    for (std::size_t component = 0; component < record.component_count;
         ++component)
      if (!finite_construction_component<T>(record, component))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 construction enclosure is not finite and ordered");

    std::vector<relation_request_id> expected_dependencies;
    for (std::size_t index = begin; index < end_group; ++index) {
      const auto *witness_source =
          find_request(artifact.request_graph_,
                       expected_uses[index].source_relation);
      const auto *authority_source =
          find_request(artifact.request_graph_,
                       expected_uses[index].authoritative_source_relation);
      if (!witness_source || !authority_source)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 construction source dependency is absent");
      expected_dependencies.push_back(witness_source->id);
      expected_dependencies.push_back(authority_source->id);
    }
    if (!exact_dependencies(*producer, expected_dependencies))
      return fail(relation_subcode::missing_dependency,
                  "Component 07 construction dependency closure is incomplete");

    const auto &authority_entry = artifact.construction_ledger_[ledger_index];
    if (authority_entry.id.ordinal() != ledger_index ||
        authority_entry.construction != record.id ||
        authority_entry.source_relation != record.source_relation ||
        authority_entry.precedence != record.precedence ||
        authority_entry.compatibility !=
            relation_construction_compatibility_disposition::authoritative ||
        authority_entry.schema_version !=
            contract_versions::relation_construction_ledger_schema ||
        authority_entry.formula_version != record.formula_version ||
        authority_entry.defining_features != record.defining_features ||
        authority_entry.defining_feature_count != record.defining_feature_count ||
        authority_entry.precision_trace_root != record.precision_trace_root ||
        !certificate_matches(authority_entry,
                             expected.authority_certificate) ||
        authority_entry.occurrence != 0 ||
        !geometry_matches_ledger(expected.authority_geometry,
                                 authority_entry) ||
        !authority_entry.authoritative_entry ||
        !authority_entry.lineage_compatible ||
        !authority_entry.enclosure_compatible ||
        !authority_entry.parameter_compatible ||
        !authority_entry.residual_compatible ||
        !authority_entry.precision_evidence_complete ||
        authority_entry.tolerance_boundary_bits !=
            record.tolerance_boundary_bits ||
        !expected_evidence(
            authority_entry.source_relation, authority_entry.truth_begin,
            authority_entry.truth_count,
            authority_entry.interval_evidence_begin,
            authority_entry.interval_evidence_count,
            authority_entry.source_facet_region_begin,
            authority_entry.source_facet_region_count) ||
        authority_entry.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 synthetic construction authority ledger entry is malformed");
    ++ledger_index;

    for (std::size_t index = begin; index < end_group; ++index) {
      const auto &expected_witness = expected_uses[index];
      const auto witness_relation =
          relation_by_key.find(expected_witness.source_relation);
      if (witness_relation == relation_by_key.end())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 construction witness source relation is absent");
      const auto &witness = artifact.construction_ledger_[ledger_index];
      const auto *witness_request =
          find_request(artifact.request_graph_, expected_witness.source_relation);
      if (!witness_request)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 construction witness request is absent");
      std::array<relation_feature_key, 2> witness_features{};
      witness_features[0] = witness_request->key.first;
      const auto witness_feature_count =
          witness_request->key.second.kind == relation_feature_kind::none ? 1 : 2;
      if (witness_feature_count == 2)
        witness_features[1] = witness_request->key.second;
      if (witness.id.ordinal() != ledger_index ||
          witness.construction != record.id ||
          witness.source_relation != witness_relation->second ||
          witness.precedence != expected_witness.witness_precedence ||
          witness.compatibility != relation_construction_compatibility_disposition::
                                       compatible_witness ||
          witness.schema_version !=
              contract_versions::relation_construction_ledger_schema ||
          witness.defining_features != witness_features ||
          witness.defining_feature_count != witness_feature_count ||
          witness.occurrence != expected_witness.occurrence ||
          !certificate_matches(witness,
                               expected_witness.witness_certificate) ||
          !geometry_matches_ledger(expected_witness.witness_geometry,
                                   witness) ||
          witness.authoritative_entry ||
          !witness.lineage_compatible ||
          !witness.enclosure_compatible ||
          !witness.parameter_compatible ||
          !witness.residual_compatible ||
          !witness.precision_evidence_complete ||
          !expected_evidence(
              witness.source_relation, witness.truth_begin,
              witness.truth_count, witness.interval_evidence_begin,
              witness.interval_evidence_count,
              witness.source_facet_region_begin,
              witness.source_facet_region_count) ||
          witness.reserved != 0)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 construction witness ledger entry does not reconstruct");
      ++ledger_index;
    }

    if (!construction_by_key.emplace(expected.key, record.id).second)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 construction producer is duplicated");
    ++construction_index;
    begin = end_group;
  }
  if (construction_index != artifact.constructions_.size() ||
      ledger_index != artifact.construction_ledger_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 construction registry has trailing records");

  std::size_t expected_node = 0;
  std::size_t expected_arc = 0;
  std::size_t expected_component = 0;
  for (const auto &descriptor : overlay_descriptors) {
    const auto relation = relation_by_key.find(descriptor.key);
    if (relation == relation_by_key.end())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 coplanar final relation is absent");
    const auto &source =
        artifact.coplanar_overlay_stage_->overlays[descriptor.ordinal];
    if (!source.complete_event_lineage ||
        !source.complete_authorized_arc_coverage ||
        !source.complete_overlap_component_assembly)
      return fail(relation_subcode::coplanar_overlay_invariant,
                  "Component 07 coplanar predecessor topology is incomplete");
    const auto node_begin = expected_node;
    const auto arc_begin = expected_arc;

    for (std::size_t local = 0; local < source.event_nodes.size(); ++local) {
      if (expected_node >= artifact.coplanar_event_nodes_.size() ||
          local > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar event-node table is incomplete");
      const auto &expected = source.event_nodes[local];
      const auto &record = artifact.coplanar_event_nodes_[expected_node];
      verifier_construction_authority<T> node_authority;
      if (!verifier_overlay_node_authority(
              *artifact.candidates_, *artifact.source_edge_stage_,
              descriptor.key, source, expected, node_authority))
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 coplanar event-node authority is unresolved");
      const auto construction = construction_by_key.find(node_authority.key);
      if (expected.id != local || construction == construction_by_key.end() ||
          record.id.ordinal() != expected_node ||
          record.overlay_relation != relation->second ||
          record.representative != construction->second ||
          record.occurrences.size() != expected.occurrences.size() ||
          record.distinct_sheet_occurrences !=
              source.distinct_sheet_occurrences ||
          record.reserved16 != 0 || record.reserved32 != 0)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar event node does not reconstruct");
      std::uint8_t sheet_mask = 0;
      for (std::size_t occurrence = 0;
           occurrence < expected.occurrences.size(); ++occurrence) {
        const auto &a = expected.occurrences[occurrence];
        const auto &b = record.occurrences[occurrence];
        if (a.polygon > 1 || b.polygon != a.polygon ||
            b.edge_ordinal != a.edge_ordinal ||
            b.breakpoint_ordinal != a.breakpoint_ordinal ||
            b.query_source_vertex_valid != a.query_source_vertex_valid ||
            b.query_source_vertex != a.query_source_vertex ||
            b.event_lineages.size() != a.event_lineages.size() ||
            b.reserved8 != 0 || b.reserved16 != 0 || b.reserved32 != 0)
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar node occurrence does not reconstruct");
        sheet_mask = static_cast<std::uint8_t>(
            sheet_mask | (std::uint8_t{1} << a.polygon));
        for (std::size_t lineage = 0; lineage < a.event_lineages.size();
             ++lineage) {
          const auto &x = a.event_lineages[lineage];
          const auto &y = b.event_lineages[lineage];
          if (y.contact_lineage != x.contact_lineage ||
              y.endpoint_role != x.endpoint_role || y.reserved8 != 0 ||
              y.reserved16 != 0)
            return fail(relation_subcode::coplanar_overlay_invariant,
                        "Component 07 final coplanar event lineage does not reconstruct");
        }
      }
      if (record.sheet_mask != sheet_mask)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar node sheet mask is inconsistent");
      ++expected_node;
    }

    for (std::size_t local = 0; local < source.oriented_arcs.size(); ++local) {
      if (expected_arc >= artifact.coplanar_oriented_arcs_.size())
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar arc table is incomplete");
      const auto &expected = source.oriented_arcs[local];
      const auto &record = artifact.coplanar_oriented_arcs_[expected_arc];
      if (expected.id != local || expected.start_node >= source.event_nodes.size() ||
          expected.end_node >= source.event_nodes.size() ||
          record.id.ordinal() != expected_arc ||
          record.overlay_relation != relation->second ||
          record.kind != verifier_coplanar_arc_kind(expected.kind) ||
          record.start_node.ordinal() != node_begin + expected.start_node ||
          record.end_node.ordinal() != node_begin + expected.end_node ||
          record.occurrences.size() != expected.occurrences.size() ||
          record.overlap_lineages.size() != expected.overlap_lineages.size() ||
          record.reserved8 != 0 || record.reserved16 != 0 ||
          record.reserved32 != 0)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar oriented arc does not reconstruct");
      std::uint8_t sheet_mask = 0;
      for (std::size_t occurrence = 0;
           occurrence < expected.occurrences.size(); ++occurrence) {
        const auto &a = expected.occurrences[occurrence];
        const auto &b = record.occurrences[occurrence];
        if (a.polygon > 1 || a.start_node >= source.event_nodes.size() ||
            a.end_node >= source.event_nodes.size() ||
            b.polygon != a.polygon || b.edge_ordinal != a.edge_ordinal ||
            b.interval_ordinal != a.interval_ordinal ||
            b.start_node.ordinal() != node_begin + a.start_node ||
            b.end_node.ordinal() != node_begin + a.end_node ||
            b.forward_along_source_edge != a.forward_along_source_edge ||
            b.reserved8 != 0 || b.reserved16 != 0 || b.reserved32 != 0)
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar arc occurrence does not reconstruct");
        sheet_mask = static_cast<std::uint8_t>(
            sheet_mask | (std::uint8_t{1} << a.polygon));
      }
      if (record.sheet_mask != sheet_mask)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar arc sheet mask is inconsistent");
      for (std::size_t lineage = 0;
           lineage < expected.overlap_lineages.size(); ++lineage) {
        const auto source_id = expected.overlap_lineages[lineage];
        if (source_id.ordinal() >=
            artifact.source_edge_stage_->request_graph.requests.size())
          return fail(relation_subcode::coplanar_overlay_dependency_missing,
                      "Component 07 coplanar source lineage is out of range");
        const auto &key = artifact.source_edge_stage_->request_graph
                              .requests[source_id.ordinal()]
                              .key;
        const auto *published = find_request(artifact.request_graph_, key);
        if (!published || record.overlap_lineages[lineage] != published->id)
          return fail(relation_subcode::coplanar_overlay_dependency_missing,
                      "Component 07 final coplanar arc lineage does not reconstruct");
      }
      ++expected_arc;
    }

    for (std::size_t local = 0; local < source.overlap_components.size(); ++local) {
      if (expected_component >=
          artifact.coplanar_overlap_components_.size())
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar component table is incomplete");
      const auto &expected = source.overlap_components[local];
      const auto &record =
          artifact.coplanar_overlap_components_[expected_component];
      if (expected.id != local || record.id.ordinal() != expected_component ||
          record.overlay_relation != relation->second ||
          record.kind != verifier_coplanar_component_kind(expected.kind) ||
          record.node_ids.size() != expected.node_ids.size() ||
          record.arc_ids.size() != expected.arc_ids.size() ||
          record.sheet_mask != expected.sheet_mask ||
          record.closed != expected.closed ||
          record.distinct_sheet_occurrences !=
              source.distinct_sheet_occurrences ||
          record.reserved8 != 0 || record.reserved16 != 0 ||
          record.reserved32 != 0)
        return fail(relation_subcode::coplanar_overlay_invariant,
                    "Component 07 final coplanar component does not reconstruct");
      for (std::size_t node = 0; node < expected.node_ids.size(); ++node)
        if (expected.node_ids[node] >= source.event_nodes.size() ||
            record.node_ids[node].ordinal() !=
                node_begin + expected.node_ids[node])
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar component node does not reconstruct");
      for (std::size_t arc = 0; arc < expected.arc_ids.size(); ++arc)
        if (expected.arc_ids[arc] >= source.oriented_arcs.size() ||
            record.arc_ids[arc].ordinal() != arc_begin + expected.arc_ids[arc])
          return fail(relation_subcode::coplanar_overlay_invariant,
                      "Component 07 final coplanar component arc does not reconstruct");
      ++expected_component;
    }
  }
  if (expected_node != artifact.coplanar_event_nodes_.size() ||
      expected_arc != artifact.coplanar_oriented_arcs_.size() ||
      expected_component != artifact.coplanar_overlap_components_.size())
    return fail(relation_subcode::coplanar_overlay_invariant,
                "Component 07 final coplanar topology has trailing records");

  if (artifact.symbolic_eligibility_.size() !=
      artifact.symbolic_decisions_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 symbolic eligibility/decision counts disagree");
  const auto symbolic = materialize_symbolic_policy();
  std::map<std::tuple<relation_request_key,
                      symbolic_relation_subject_kind, std::uint64_t,
                      operand_id>,
           const symbolic_relation_decision_record *>
      symbolic_by_subject;
  for (std::size_t i = 0; i < artifact.symbolic_decisions_.size(); ++i) {
    const auto &eligibility = artifact.symbolic_eligibility_[i];
    const auto &decision = artifact.symbolic_decisions_[i];
    if (eligibility.request.family !=
            relation_request_family::symbolic_eligibility ||
        !find_request(artifact.request_graph_, eligibility.request) ||
        decision.id.ordinal() != i ||
        decision.operation != artifact.operation_ ||
        !decision.nominal_geometry_unchanged || decision.reserved8 != 0 ||
        decision.schema_version !=
            contract_versions::relation_symbolic_decision_schema ||
        decision.reserved != 0)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic record violates its publication boundary");
    const auto *eligibility_request =
        find_request(artifact.request_graph_, eligibility.request);
    const relation_request_key *source_key = nullptr;
    const canonical_relation_request *construction_request = nullptr;
    if (!eligibility_request)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic eligibility request is absent");
    for (std::uint64_t offset = 0;
         offset < eligibility_request->dependency_count; ++offset) {
      const auto dependency_index =
          eligibility_request->dependency_begin + offset;
      if (dependency_index >= artifact.request_graph_.dependencies.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic dependency range is malformed");
      const auto producer_id =
          artifact.request_graph_.dependencies[dependency_index].producer;
      if (producer_id.ordinal() >= artifact.request_graph_.requests.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic dependency producer is absent");
      const auto &producer =
          artifact.request_graph_.requests[producer_id.ordinal()];
      if (verifier_symbolic_source_family(producer.key.family)) {
        if (source_key)
          return fail(relation_subcode::duplicate_authoritative_producer,
                      "Component 07 symbolic eligibility has multiple source relations");
        source_key = &producer.key;
      } else if (producer.key.family ==
                 relation_request_family::authoritative_construction) {
        if (construction_request)
          return fail(relation_subcode::duplicate_authoritative_producer,
                      "Component 07 symbolic eligibility has multiple constructions");
        construction_request = &producer;
      }
    }
    if (!source_key)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic eligibility source relation is absent");

    bool construction_tolerance_compatible = true;
    if (construction_request) {
      const relation_construction_record *construction = nullptr;
      for (const auto &candidate : artifact.constructions_)
        if (candidate.producer == construction_request->id) {
          if (construction)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic construction producer is duplicated");
          construction = &candidate;
        }
      if (!construction)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic construction record is absent");
      construction_tolerance_compatible = construction->tolerance_compatible;
    }

    symbolic_eligibility_record expected_eligibility;
    expected_eligibility.request = eligibility.request;
    bool reconstructed_evidence = false;
    const auto occurrence = eligibility.request.occurrence_discriminator;
    symbolic_rule_key expected_rule_key;
    symbolic_relation_subject_kind expected_subject_kind =
        symbolic_relation_subject_kind::relation;
    std::uint64_t expected_subject_ordinal = 0;
    bool reconstructed_rule = false;
    switch (source_key->family) {
    case relation_request_family::source_edge_source_edge: {
      const auto *source_request =
          find_request(artifact.source_edge_stage_->request_graph, *source_key);
      if (!source_request ||
          source_request->id.ordinal() >=
              artifact.source_edge_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge source is absent");
      const auto &source = artifact.source_edge_stage_->relations[
          source_request->id.ordinal()];
      source_edge_support_class derived_support;
      source_edge_contact_class derived_contact;
      source_edge_orientation_relation derived_orientation;
      if (!verifier_derive_edge_classification(
              source, derived_support, derived_contact, derived_orientation))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 symbolic edge category is unresolved");
      (void)derived_support;
      const source_edge_point_construction<T> *point = nullptr;
      if (construction_request) {
        if (occurrence >= source.point_count)
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 symbolic edge occurrence is out of range");
        point = &source.points[occurrence];
      }
      if (derived_contact == source_edge_contact_class::partial_overlap ||
          derived_contact == source_edge_contact_class::first_contains_second ||
          derived_contact == source_edge_contact_class::second_contains_first ||
          derived_contact == source_edge_contact_class::equal) {
        reconstructed_evidence =
            source.has_collinearity_truth &&
            verifier_set_truth_symbolic_evidence<T>(
                source.collinearity_truth,
                derived_contact == source_edge_contact_class::equal
                    ? symbolic_eligibility_reason::equal_source_feature_lineage
                    : symbolic_eligibility_reason::collinear_source_edge_lineage,
                expected_eligibility);
      } else if (derived_contact ==
                      source_edge_contact_class::endpoint_contact ||
                 derived_contact == source_edge_contact_class::point_contact) {
        const auto reason =
            point && point->first_endpoint_owner_mask != 0 &&
                    point->second_endpoint_owner_mask != 0
                ? symbolic_eligibility_reason::shared_source_endpoint
                : symbolic_eligibility_reason::exact_formula_zero;
        if (source.has_collinearity_truth &&
            source.collinearity_truth.exact_relation ==
                exact_relation_status::exact_zero)
          reconstructed_evidence = verifier_set_truth_symbolic_evidence<T>(
              source.collinearity_truth, reason, expected_eligibility);
        else if (source.has_coplanarity_truth)
          reconstructed_evidence = verifier_set_truth_symbolic_evidence<T>(
              source.coplanarity_truth, reason, expected_eligibility);
      }
      if (point) {
        const bool first_endpoint = point->first_endpoint_owner_mask != 0;
        const bool second_endpoint = point->second_endpoint_owner_mask != 0;
        expected_rule_key = verifier_symbolic_rule_key(
            artifact.operation_, source_key->first.operand,
            verifier_symbolic_family_for_edge(derived_contact, point),
            verifier_orientation_from_edge(derived_orientation),
            first_endpoint && second_endpoint
                ? symbolic_ownership_role::shared_source_feature
            : second_endpoint
                ? symbolic_ownership_role::opposite_source_feature
                : symbolic_ownership_role::acting_source_feature,
            first_endpoint || second_endpoint
                ? symbolic_half_open_role::source_endpoint
                : symbolic_half_open_role::interior,
            symbolic_transition_orientation::none,
            first_endpoint && second_endpoint
                ? symbolic_occurrence_class::shared_source_feature
                : symbolic_occurrence_class::lower_dimensional_contact);
        expected_subject_kind =
            symbolic_relation_subject_kind::event_occurrence;
        expected_subject_ordinal = occurrence;
      } else {
        expected_rule_key = verifier_symbolic_rule_key(
            artifact.operation_, source_key->first.operand,
            verifier_symbolic_family_for_edge<T>(derived_contact, nullptr),
            verifier_orientation_from_edge(derived_orientation),
            symbolic_ownership_role::shared_source_feature,
            symbolic_half_open_role::source_edge,
            symbolic_transition_orientation::none,
            derived_contact == source_edge_contact_class::equal
                ? symbolic_occurrence_class::shared_source_feature
                : symbolic_occurrence_class::lower_dimensional_contact);
      }
      reconstructed_rule = true;
      break;
    }
    case relation_request_family::source_edge_source_facet: {
      const auto *source_request = find_request(
          artifact.source_edge_facet_stage_->request_graph, *source_key);
      if (!source_request ||
          source_request->id.ordinal() >=
              artifact.source_edge_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge/facet source is absent");
      const auto &source = artifact.source_edge_facet_stage_->relations[
          source_request->id.ordinal()];
      source_edge_facet_support_class derived_support;
      source_edge_facet_contact_class derived_contact;
      if (!verifier_derive_edge_facet_classification(
              source, derived_support, derived_contact))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 symbolic edge/facet category is unresolved");
      (void)derived_support;
      const source_edge_facet_event_record<T> *event = nullptr;
      for (std::size_t local_event = 0; local_event < source.events.size();
           ++local_event) {
        if (local_event > std::numeric_limits<std::uint32_t>::max())
          return fail(relation_subcode::count_overflow,
                      "Component 07 symbolic edge/facet local event overflowed");
        std::uint32_t candidate_occurrence = 0;
        if (!canonical_event_occurrence(
                source_request->id, static_cast<std::uint32_t>(local_event),
                candidate_occurrence))
          return fail(relation_subcode::missing_dependency,
                      "Component 07 symbolic edge/facet event order is absent");
        if (candidate_occurrence == occurrence) {
          if (event)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic edge/facet occurrence is duplicated");
          event = &source.events[local_event];
        }
      }
      if (!event)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge/facet occurrence is absent");
      if (event->region.classification ==
              source_facet_point_region_class::original_edge ||
          event->region.classification ==
              source_facet_point_region_class::original_vertex)
        reconstructed_evidence = verifier_set_region_symbolic_evidence(
            event->region, expected_eligibility);
      else
        for (const auto &truth : source.endpoint_support_truth)
          if (!reconstructed_evidence &&
              truth.exact_relation == exact_relation_status::exact_zero)
            reconstructed_evidence = verifier_set_truth_symbolic_evidence<T>(
                truth, symbolic_eligibility_reason::exact_formula_zero,
                expected_eligibility);
      if (event->kind == source_edge_facet_event_kind::proper_face_crossing)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 proper edge/facet crossing has a symbolic decision");
      const auto local_transition = verifier_local_transition(*event);
      const auto transition =
          event->kind == source_edge_facet_event_kind::tangent_contact
              ? symbolic_transition_orientation::tangent
          : local_transition > 0
              ? symbolic_transition_orientation::negative_to_positive
          : local_transition < 0
              ? symbolic_transition_orientation::positive_to_negative
              : symbolic_transition_orientation::none;
      const bool query_endpoint =
          event->construction.edge_endpoint_owner_mask != 0;
      const bool opposite_vertex =
          event->region.classification ==
          source_facet_point_region_class::original_vertex;
      const bool opposite_edge =
          event->region.classification ==
          source_facet_point_region_class::original_edge;
      const auto ownership =
          query_endpoint && (opposite_vertex || opposite_edge)
              ? symbolic_ownership_role::shared_source_feature
          : query_endpoint
              ? symbolic_ownership_role::acting_source_feature
          : opposite_vertex || opposite_edge
              ? symbolic_ownership_role::opposite_source_feature
              : symbolic_ownership_role::acting_source_feature;
      expected_rule_key = verifier_symbolic_rule_key(
          artifact.operation_, source_key->first.operand,
          verifier_symbolic_family_for_edge_facet(*event, derived_contact),
          orientation_relation::indeterminate, ownership,
          query_endpoint || opposite_vertex
              ? symbolic_half_open_role::source_endpoint
          : opposite_edge ? symbolic_half_open_role::source_edge
                          : symbolic_half_open_role::interior,
          transition,
          query_endpoint && (opposite_vertex || opposite_edge)
              ? symbolic_occurrence_class::shared_source_feature
              : symbolic_occurrence_class::lower_dimensional_contact);
      expected_subject_kind =
          symbolic_relation_subject_kind::event_occurrence;
      expected_subject_ordinal = occurrence;
      reconstructed_rule = true;
      break;
    }
    case relation_request_family::source_facet_source_facet: {
      const auto *source_request =
          find_request(artifact.source_facet_stage_->request_graph, *source_key);
      if (!source_request ||
          source_request->id.ordinal() >=
              artifact.source_facet_stage_->relations.size())
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic facet source is absent");
      const auto &source = artifact.source_facet_stage_->relations[
          source_request->id.ordinal()];
      source_facet_support_relation_class derived_classification;
      if (!verifier_derive_facet_classification(source,
                                                derived_classification))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 symbolic facet category is unresolved");
      reconstructed_evidence =
          source.has_coplanarity_truth &&
          verifier_set_truth_symbolic_evidence<T>(
              source.coplanarity_truth,
              symbolic_eligibility_reason::coplanar_source_facet_lineage,
              expected_eligibility);
      if (decision.acting_operand != operand_id::a &&
          decision.acting_operand != operand_id::b)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 symbolic facet operand is invalid");
      expected_rule_key = verifier_symbolic_rule_key(
          artifact.operation_, decision.acting_operand,
          relation_family::coplanar,
          verifier_orientation_from_status(facet_status(derived_classification)),
          symbolic_ownership_role::coincident_sheet_pair,
          symbolic_half_open_role::none,
          symbolic_transition_orientation::none,
          symbolic_occurrence_class::coincident_sheet);
      reconstructed_rule = true;
      break;
    }
    case relation_request_family::coplanar_source_facet_overlay: {
      const auto descriptor = std::lower_bound(
          overlay_descriptors.begin(), overlay_descriptors.end(), *source_key,
          [](const verifier_overlay_descriptor &candidate,
             const relation_request_key &key) { return candidate.key < key; });
      if (descriptor == overlay_descriptors.end() ||
          !(descriptor->key == *source_key))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic overlay source lineage is absent");
      const auto &source =
          artifact.coplanar_overlay_stage_->overlays[descriptor->ordinal];
      coplanar_facet_overlay_class derived_classification;
      if (!verifier_derive_overlay_classification(source,
                                                  derived_classification))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 symbolic overlay category is unresolved");
      reconstructed_evidence =
          source.support_relation.has_coplanarity_truth &&
          verifier_set_truth_symbolic_evidence<T>(
              source.support_relation.coplanarity_truth,
              symbolic_eligibility_reason::coincident_source_contract,
              expected_eligibility);
      const coplanar_overlap_component *component = nullptr;
      for (const auto &candidate : source.overlap_components)
        if (candidate.id == occurrence) {
          if (component)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic overlay component is duplicated");
          component = &candidate;
        }
      if (!component)
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic overlay component is absent");
      const auto component_kind =
          verifier_coplanar_component_kind(component->kind);
      const bool coincident =
          component_kind == relation_coplanar_component_kind::area_boundary ||
          component_kind ==
              relation_coplanar_component_kind::coincident_sheet_boundary;
      expected_rule_key = verifier_symbolic_rule_key(
          artifact.operation_, decision.acting_operand,
          verifier_symbolic_family_for_overlay(derived_classification),
          verifier_orientation_from_status(overlay_status(derived_classification)),
          coincident ? symbolic_ownership_role::coincident_sheet_pair
                     : symbolic_ownership_role::shared_source_feature,
          component_kind == relation_coplanar_component_kind::isolated_point
              ? symbolic_half_open_role::source_endpoint
          : component_kind ==
                    relation_coplanar_component_kind::boundary_segment
              ? symbolic_half_open_role::source_edge
              : symbolic_half_open_role::none,
          symbolic_transition_orientation::none,
          coincident ? symbolic_occurrence_class::coincident_sheet
                     : symbolic_occurrence_class::lower_dimensional_contact);
      expected_subject_kind =
          symbolic_relation_subject_kind::coplanar_component;
      expected_subject_ordinal = component->id;
      reconstructed_rule = true;
      break;
    }
    default:
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic source family is invalid");
    }
    expected_eligibility.representational_tie_evidence = false;
    expected_eligibility.structural_category_eligible =
        reconstructed_evidence;
    expected_eligibility.tolerance_compatible =
        reconstructed_evidence && construction_tolerance_compatible;
    expected_eligibility.separated_realizations_possible = false;
    expected_eligibility.owner_is_original_source_feature =
        source_key->first.kind !=
            relation_feature_kind::facet_internal_diagonal &&
        source_key->second.kind !=
            relation_feature_kind::facet_internal_diagonal;
    if (!reconstructed_evidence ||
        !verifier_symbolic_eligibility_equal(eligibility,
                                             expected_eligibility))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic eligibility does not reconstruct from predecessor truth");
    if (!reconstructed_rule || decision.rule_key != expected_rule_key ||
        decision.exchanged_rule_key !=
            exchange_symbolic_rule_key(expected_rule_key) ||
        decision.subject_kind != expected_subject_kind ||
        decision.subject_ordinal != expected_subject_ordinal)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic rule key or subject does not reconstruct");

    const auto expected_eligibility_key = verifier_derived_key(
        *source_key, relation_request_family::symbolic_eligibility,
        verifier_symbolic_directed_use(12, expected_rule_key,
                                       expected_subject_kind),
        occurrence);
    const auto expected_decision_key = verifier_derived_key(
        *source_key, relation_request_family::symbolic_relation_decision,
        verifier_symbolic_directed_use(13, expected_rule_key,
                                       expected_subject_kind),
        occurrence);
    if (eligibility.request != expected_eligibility_key ||
        decision.request != eligibility.request)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic request identity does not reconstruct");

    bounded_boolean_error symbolic_error;
    if (!verify_symbolic_relation_decision(symbolic, eligibility, decision,
                                           symbolic_error)) {
      error = symbolic_error;
      return false;
    }
    const canonical_relation_request *decision_request = nullptr;
    if (eligibility_request)
      for (const auto &candidate : artifact.request_graph_.requests)
        if (candidate.key.family ==
                relation_request_family::symbolic_relation_decision &&
            request_has_dependency(artifact.request_graph_, candidate,
                                   eligibility_request->id)) {
          if (decision_request)
            return fail(relation_subcode::duplicate_authoritative_producer,
                        "Component 07 symbolic decision request is duplicated");
          decision_request = &candidate;
        }
    if (!decision_request || decision_request->key != expected_decision_key ||
        decision_request->dependency_count != 1)
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic decision request does not reconstruct");
    if (!symbolic_by_subject
             .emplace(std::make_tuple(*source_key, expected_subject_kind,
                                      expected_subject_ordinal,
                                      expected_rule_key.acting_operand),
                      &decision)
             .second)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "Component 07 symbolic subject and operand are duplicated");
  }

  std::uint64_t expected_symbolic_count = 0;
  const auto require_symbolic =
      [&](const relation_request_key &source,
          symbolic_relation_subject_kind subject_kind,
          std::uint64_t subject_ordinal, operand_id acting) {
        ++expected_symbolic_count;
        return symbolic_by_subject.find(std::make_tuple(
                   source, subject_kind, subject_ordinal, acting)) !=
               symbolic_by_subject.end();
      };
  for (std::size_t i = 0;
       i < artifact.source_edge_stage_->request_graph.requests.size(); ++i) {
    const auto &request = artifact.source_edge_stage_->request_graph.requests[i];
    if (i >= artifact.source_edge_stage_->relations.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic edge relation table is incomplete");
    const auto &source = artifact.source_edge_stage_->relations[i];
    source_edge_support_class support;
    source_edge_contact_class contact;
    source_edge_orientation_relation orientation;
    if (!verifier_derive_edge_classification(source, support, contact,
                                             orientation))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic edge population is unresolved");
    (void)support;
    (void)orientation;
    if (contact == source_edge_contact_class::none ||
        contact == source_edge_contact_class::proper_crossing)
      continue;
    if (source.points.empty()) {
      if (!require_symbolic(request.key,
                            symbolic_relation_subject_kind::relation, 0,
                            request.key.first.operand))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge relation decision is absent");
    } else {
      for (std::size_t point = 0; point < source.points.size(); ++point)
        if (!require_symbolic(
                request.key,
                symbolic_relation_subject_kind::event_occurrence, point,
                request.key.first.operand))
          return fail(relation_subcode::missing_dependency,
                      "Component 07 symbolic edge occurrence decision is absent");
    }
  }
  for (std::size_t i = 0;
       i < artifact.source_edge_facet_stage_->request_graph.requests.size(); ++i) {
    const auto &request =
        artifact.source_edge_facet_stage_->request_graph.requests[i];
    if (i >= artifact.source_edge_facet_stage_->relations.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic edge/facet relation table is incomplete");
    const auto &source = artifact.source_edge_facet_stage_->relations[i];
    for (std::size_t local_event = 0; local_event < source.events.size();
         ++local_event) {
      if (source.events[local_event].kind ==
          source_edge_facet_event_kind::proper_face_crossing)
        continue;
      std::uint32_t occurrence = 0;
      if (local_event > std::numeric_limits<std::uint32_t>::max() ||
          !canonical_event_occurrence(
              request.id, static_cast<std::uint32_t>(local_event), occurrence))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge/facet occurrence order is absent");
      if (!require_symbolic(
              request.key, symbolic_relation_subject_kind::event_occurrence,
              occurrence, request.key.first.operand))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic edge/facet decision is absent");
    }
  }
  for (std::size_t i = 0;
       i < artifact.source_facet_stage_->request_graph.requests.size(); ++i) {
    const auto &request = artifact.source_facet_stage_->request_graph.requests[i];
    if (i >= artifact.source_facet_stage_->relations.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 symbolic facet relation table is incomplete");
    source_facet_support_relation_class classification;
    if (!verifier_derive_facet_classification(
            artifact.source_facet_stage_->relations[i], classification))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic facet population is unresolved");
    if (classification !=
            source_facet_support_relation_class::coplanar_same_orientation &&
        classification !=
            source_facet_support_relation_class::coplanar_opposite_orientation)
      continue;
    for (const auto acting : {operand_id::a, operand_id::b})
      if (!require_symbolic(request.key,
                            symbolic_relation_subject_kind::relation, 0,
                            acting))
        return fail(relation_subcode::missing_dependency,
                    "Component 07 symbolic facet operand decision is absent");
  }
  for (const auto &descriptor : overlay_descriptors) {
    const auto &source =
        artifact.coplanar_overlay_stage_->overlays[descriptor.ordinal];
    coplanar_facet_overlay_class classification;
    if (!verifier_derive_overlay_classification(source, classification))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 symbolic overlay population is unresolved");
    if (classification == coplanar_facet_overlay_class::disjoint)
      continue;
    for (const auto &component : source.overlap_components)
      for (const auto acting : {operand_id::a, operand_id::b})
        if (!require_symbolic(
                descriptor.key,
                symbolic_relation_subject_kind::coplanar_component,
                component.id, acting))
          return fail(relation_subcode::missing_dependency,
                      "Component 07 symbolic overlay operand decision is absent");
  }
  if (expected_symbolic_count != symbolic_by_subject.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 symbolic decision table has trailing subjects");

  std::vector<verifier_crossing_descriptor<T>> expected_crossings;
  for (std::size_t i = 0;
       i < artifact.source_edge_facet_stage_->request_graph.requests.size(); ++i) {
    const auto &request =
        artifact.source_edge_facet_stage_->request_graph.requests[i];
    const auto relation = relation_by_key.find(request.key);
    if (relation == relation_by_key.end() ||
        i >= artifact.source_edge_facet_stage_->relations.size())
      return fail(relation_subcode::missing_dependency,
                  "Component 07 crossing detailed producer is absent");
    const auto &source = artifact.source_edge_facet_stage_->relations[i];
    for (std::size_t local_event = 0; local_event < source.events.size();
         ++local_event) {
      if (local_event > std::numeric_limits<std::uint32_t>::max())
        return fail(relation_subcode::count_overflow,
                    "Component 07 crossing local event overflowed");
      const auto &event = source.events[local_event];
      verifier_crossing_descriptor<T> descriptor;
      if (!canonical_event_occurrence(
              request.id, static_cast<std::uint32_t>(local_event),
              descriptor.occurrence) ||
          !verifier_source_fan_key(request.key, event, descriptor.occurrence,
                                   descriptor.group))
        return fail(relation_subcode::crossing_fan_incomplete,
                    "Component 07 verifier could not reconstruct source-fan lineage");
      descriptor.source_relation = request.key;
      descriptor.relation = relation->second;
      descriptor.event = &event;
      descriptor.local_transition = verifier_local_transition(event);
      descriptor.half_open_owner = request.key.first.operand;
      const auto symbolic = symbolic_by_subject.find(std::make_tuple(
          request.key, symbolic_relation_subject_kind::event_occurrence,
          descriptor.occurrence, request.key.first.operand));
      if (symbolic != symbolic_by_subject.end()) {
        descriptor.symbolic_crossing =
            symbolic->second->symbolic_crossing_contribution;
        descriptor.half_open_owner = symbolic->second->half_open_owner;
      }
      expected_crossings.push_back(descriptor);
    }
  }
  std::sort(expected_crossings.begin(), expected_crossings.end(),
            [](const verifier_crossing_descriptor<T> &a,
               const verifier_crossing_descriptor<T> &b) {
              return std::tie(a.group, a.source_relation, a.occurrence) <
                     std::tie(b.group, b.source_relation, b.occurrence);
            });
  if (artifact.crossings_.size() != expected_crossings.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 crossing contribution table is incomplete");

  std::vector<std::int64_t> crossing_sums(artifact.relations_.size(), 0);
  std::size_t published = 0;
  for (std::size_t begin = 0, group = 0; begin < expected_crossings.size(); ++group) {
    std::size_t end = begin + 1;
    while (end < expected_crossings.size() &&
           expected_crossings[end].group == expected_crossings[begin].group)
      ++end;
    const auto count = end - begin;
    bool has_positive = false;
    bool has_negative = false;
    bool complete = true;
    bool boundary_crossing_group = false;
    bool tangent_group = false;
    for (std::size_t i = begin; i < end; ++i) {
      const auto &event = *expected_crossings[i].event;
      if (expected_crossings[i].group.boundary_group) {
        boundary_crossing_group =
            boundary_crossing_group ||
            event.kind == source_edge_facet_event_kind::boundary_crossing;
        tangent_group = tangent_group ||
                        event.kind ==
                            source_edge_facet_event_kind::tangent_contact;
        if (event.kind == source_edge_facet_event_kind::boundary_crossing) {
          complete = complete &&
                     (expected_crossings[i].local_transition == -1 ||
                      expected_crossings[i].local_transition == 1);
          has_positive = has_positive ||
                         expected_crossings[i].local_transition > 0;
          has_negative = has_negative ||
                         expected_crossings[i].local_transition < 0;
        } else {
          complete = complete &&
                     event.kind ==
                         source_edge_facet_event_kind::tangent_contact &&
                     expected_crossings[i].local_transition == 0 &&
                     event.numeric_crossing == 0;
        }
      } else {
        complete = complete &&
                   (event.kind !=
                        source_edge_facet_event_kind::proper_face_crossing ||
                    (expected_crossings[i].local_transition ==
                         event.numeric_crossing &&
                     event.numeric_crossing != 0));
      }
      complete = complete &&
                 expected_crossings[i].half_open_owner ==
                     expected_crossings[begin].half_open_owner;
    }
    if (expected_crossings[begin].group.boundary_group) {
      std::vector<relation_feature_key> expected_facets;
      std::vector<relation_feature_key> actual_facets;
      if (!artifact.candidates_ ||
          !verifier_expected_source_fan_facets(
              *artifact.candidates_, expected_crossings[begin].group,
              expected_facets))
        complete = false;
      for (std::size_t i = begin; i < end; ++i)
        actual_facets.push_back(expected_crossings[i].source_relation.second);
      std::sort(actual_facets.begin(), actual_facets.end());
      actual_facets.erase(
          std::unique(actual_facets.begin(), actual_facets.end()),
          actual_facets.end());
      complete = complete && count >= 2 &&
                 actual_facets == expected_facets &&
                 boundary_crossing_group != tangent_group;
    }
    if (!complete)
      return fail(relation_subcode::crossing_fan_incomplete,
                  "Component 07 independently reconstructed source fan is incomplete");

    std::int32_t group_total = 0;
    if (expected_crossings[begin].group.boundary_group) {
      if (boundary_crossing_group && has_positive != has_negative)
        group_total = has_positive ? 1 : -1;
    } else {
      group_total = expected_crossings[begin].event->numeric_crossing;
    }
    const bool has_numeric_owner = group_total != 0;
    for (std::size_t i = begin; i < end; ++i, ++published) {
      const auto &expected = expected_crossings[i];
      const auto &record = artifact.crossings_[published];
      const auto ordinal = static_cast<std::uint32_t>(i - begin);
      const auto expected_numeric =
          has_numeric_owner && i == begin ? group_total : 0;
      const auto expected_symbolic =
          i == begin ? expected.symbolic_crossing : 0;
      if (record.relation != expected.relation ||
          record.numeric_crossing != expected_numeric ||
          record.symbolic_crossing != expected_symbolic ||
          record.half_open_owner !=
              expected_crossings[begin].half_open_owner ||
          record.occurrence != expected.occurrence ||
          record.source_fan_group != group ||
          record.source_fan_group_size != count ||
          record.source_fan_group_ordinal != ordinal ||
          record.local_transition != expected.local_transition ||
          record.numeric_owner != (has_numeric_owner && i == begin) ||
          !record.source_fan_resolved || !record.locally_conservative ||
          record.reserved16 != 0 || record.reserved32 != 0)
        return fail(relation_subcode::crossing_conservation_failed,
                    "Component 07 crossing/source-fan record does not reconstruct");
      crossing_sums[record.relation.ordinal()] += record.numeric_crossing;
    }
    begin = end;
  }
  for (std::size_t i = 0; i < artifact.relations_.size(); ++i)
    if (crossing_sums[i] !=
        artifact.relations_[i].numeric_crossing_multiplicity)
      return fail(relation_subcode::crossing_conservation_failed,
                  "Component 07 local numeric crossing conservation failed");

  using transverse_membership_key =
      std::tuple<feature_relation_id, feature_relation_id, std::uint32_t>;
  std::vector<transverse_membership_key> expected_transverse_memberships;
  for (std::size_t member_index = 0;
       member_index < artifact.source_edge_facet_stage_->relations.size();
       ++member_index) {
    if (member_index >=
        artifact.source_edge_facet_stage_->request_graph.requests.size())
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 verifier transverse member request is absent");
    const auto &member =
        artifact.source_edge_facet_stage_->relations[member_index];
    const auto &member_key = artifact.source_edge_facet_stage_->request_graph
                                 .requests[member_index]
                                 .key;
    const auto member_relation = relation_by_key.find(member_key);
    if (member_relation == relation_by_key.end())
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 verifier transverse member relation is absent");
    for (const auto &event : member.events) {
      std::uint32_t occurrence = 0;
      if (!canonical_event_occurrence(relation_request_id(member_index),
                                      event.occurrence, occurrence))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 verifier transverse occurrence is absent");
      for (std::size_t carrier_index = 0;
           carrier_index < artifact.source_facet_stage_->relations.size();
           ++carrier_index) {
        const auto &carrier =
            artifact.source_facet_stage_->relations[carrier_index];
        if (carrier.classification !=
                source_facet_support_relation_class::transverse ||
            !carrier.has_transverse_carrier ||
            !std::binary_search(carrier.edge_facet_consumers.begin(),
                                carrier.edge_facet_consumers.end(),
                                relation_request_id(member_index)))
          continue;
        if (carrier_index >=
            artifact.source_facet_stage_->request_graph.requests.size())
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 verifier transverse carrier request is absent");
        const auto &carrier_key = artifact.source_facet_stage_->request_graph
                                      .requests[carrier_index]
                                      .key;
        const auto carrier_relation = relation_by_key.find(carrier_key);
        if (carrier_relation == relation_by_key.end())
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 verifier transverse carrier relation is absent");
        expected_transverse_memberships.emplace_back(
            carrier_relation->second, member_relation->second, occurrence);
      }
    }
  }
  std::sort(expected_transverse_memberships.begin(),
            expected_transverse_memberships.end());
  if (std::adjacent_find(expected_transverse_memberships.begin(),
                         expected_transverse_memberships.end()) !=
      expected_transverse_memberships.end())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 expected transverse membership is duplicated");
  if (artifact.transverse_carrier_memberships_.size() !=
      expected_transverse_memberships.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 transverse membership population is incomplete");

  for (std::size_t i = 0;
       i < artifact.transverse_carrier_memberships_.size(); ++i) {
    const auto &record = artifact.transverse_carrier_memberships_[i];
    if (std::make_tuple(record.carrier_relation, record.member_relation,
                        record.occurrence) !=
        expected_transverse_memberships[i])
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse membership key set does not reconstruct");
    if (record.id.ordinal() != i ||
        record.carrier_relation.ordinal() >= artifact.relations_.size() ||
        record.member_relation.ordinal() >= artifact.relations_.size() ||
        record.carrier_construction.ordinal() >=
            artifact.constructions_.size() ||
        record.point_construction.ordinal() >= artifact.constructions_.size() ||
        record.seed.ordinal() >= artifact.event_seeds_.size() ||
        record.parameter.ordinal() >= artifact.interval_evidence_.size() ||
        record.first_region.ordinal() >= artifact.source_facet_regions_.size() ||
        record.second_region.ordinal() >= artifact.source_facet_regions_.size() ||
        record.schema_version !=
            contract_versions::relation_transverse_carrier_membership_schema ||
        record.reserved16 != 0 || record.reserved32 != 0 || !record.finite ||
        !record.conditioning_accepted || !record.residuals_accepted ||
         !record.regions_complete || !record.precision_evidence_complete ||
         record.parameter_lineage == 0 || record.carrier_lineage == 0 ||
         record.event_lineage == 0 ||
         (record.transition != relation_carrier_transition::entering &&
          record.transition != relation_carrier_transition::leaving &&
          record.transition != relation_carrier_transition::tangent))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse carrier membership is malformed");
    const auto &carrier =
        artifact.constructions_[record.carrier_construction.ordinal()];
    const auto &seed = artifact.event_seeds_[record.seed.ordinal()];
    const auto &parameter = artifact.interval_evidence_[record.parameter.ordinal()];
    const auto &first = artifact.source_facet_regions_[record.first_region.ordinal()];
    const auto &second = artifact.source_facet_regions_[record.second_region.ordinal()];
    const relation_crossing_record *crossing = nullptr;
    for (const auto &candidate : artifact.crossings_)
      if (candidate.relation == record.member_relation &&
          candidate.occurrence == record.occurrence) {
        if (crossing)
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 transverse membership crossing is ambiguous");
        crossing = &candidate;
      }
    if (carrier.source_relation != record.carrier_relation ||
        carrier.kind != relation_construction_kind::bounded_carrier ||
        carrier.geometric_lineage != record.carrier_lineage ||
        seed.source_relation != record.member_relation ||
        seed.construction != record.point_construction ||
        seed.key.occurrence != record.occurrence ||
        parameter.source_relation != record.member_relation ||
        parameter.kind != relation_interval_evidence_kind::
                              transverse_carrier_parameter ||
         parameter.trace_root != record.parameter_lineage ||
        parameter.issued_operation != rounded_operation_code::divide ||
        parameter.issued_value == 0 || parameter.issued_ledger_entry == 0 ||
        parameter.issued_parent_values.size() != 2 ||
        parameter.issued_parent_trace_roots.size() != 2 ||
        parameter.issued_parent_ledger_entries.size() != 2 ||
        parameter.issued_operation_evidence.empty() ||
        first.source_relation != record.member_relation ||
        second.source_relation != record.member_relation ||
        first.kind != relation_source_facet_region_kind::
                          transverse_carrier_first_facet ||
        second.kind != relation_source_facet_region_kind::
                           transverse_carrier_second_facet ||
        first.region.classification ==
            source_facet_point_region_class::outside ||
         second.region.classification ==
             source_facet_point_region_class::outside)
      return fail(relation_subcode::verifier_rejection,
                   "Component 07 transverse carrier membership does not reconstruct");
    if (!crossing)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse membership crossing is absent");
    const auto expected_transition = crossing->numeric_owner &&
                                             crossing->numeric_crossing > 0
                                         ? relation_carrier_transition::entering
                                     : crossing->numeric_owner &&
                                               crossing->numeric_crossing < 0
                                         ? relation_carrier_transition::leaving
                                         : relation_carrier_transition::tangent;
    const auto &carrier_relation = artifact.relations_[record.carrier_relation.ordinal()];
    if (carrier_relation.producer.ordinal() >= artifact.request_graph_.requests.size())
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse carrier request is absent");
    const auto &carrier_key = artifact.request_graph_.requests[
        carrier_relation.producer.ordinal()].key;
    const auto query_matches = [&](const auto &region) {
      if (region.query_component_count != 3)
        return false;
      const auto &point = artifact.constructions_[record.point_construction.ordinal()];
      for (std::size_t axis = 0; axis < 3; ++axis)
        if (region.query_nominal_bits[axis] != point.nominal_bits[axis] ||
            region.query_lower_bits[axis] != point.lower_bits[axis] ||
            region.query_upper_bits[axis] != point.upper_bits[axis])
          return false;
      return true;
    };
    if (record.event_lineage != record.point_construction.ordinal() + 1 ||
        record.numeric_crossing != crossing->numeric_crossing ||
        record.local_transition != crossing->local_transition ||
        record.numeric_owner != crossing->numeric_owner ||
        record.transition != expected_transition ||
        record.half_open_owner != crossing->half_open_owner ||
        first.region.source_facet != carrier_key.first.primary ||
        first.region.ring != carrier_key.first.secondary ||
        second.region.source_facet != carrier_key.second.primary ||
        second.region.ring != carrier_key.second.secondary ||
        !query_matches(first) || !query_matches(second))
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse membership facet or query evidence does not reconstruct");
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto residual = record.point_carrier_residuals[axis];
      if (residual.ordinal() >= artifact.interval_evidence_.size())
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 transverse carrier residual is absent");
      const auto &evidence = artifact.interval_evidence_[residual.ordinal()];
      if (evidence.source_relation != record.member_relation ||
          evidence.kind != relation_interval_evidence_kind::
                               transverse_carrier_point_residual ||
          evidence.component != axis || !evidence.within_authorized_boundary)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 transverse carrier residual does not reconstruct");
    }
  }

  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto index = static_cast<std::size_t>(operand);
    const auto &published = artifact.source_topology_[index];
    const auto &table = artifact.candidates_->primitive_table(operand);
    const auto &topology = operand == operand_id::a
                               ? *artifact.candidates_->manifolds()->a()
                               : *artifact.candidates_->manifolds()->b();
    const auto facet_feature = [&](std::uint64_t source_facet,
                                   relation_feature_key &feature) {
      if (source_facet >= topology.source_facet_to_group().size())
        return false;
      const auto group_id = topology.source_facet_to_group()[source_facet];
      if (group_id >= topology.facet_groups().size())
        return false;
      const auto &group = topology.facet_groups()[group_id];
      if (group.canonical_id != group_id || group.source_facet != source_facet)
        return false;
      feature = {};
      feature.operand = operand;
      feature.kind = relation_feature_kind::source_facet;
      feature.primary = source_facet;
      feature.secondary = group.ring;
      return valid_relation_feature_key(feature, false);
    };

    std::vector<relation_source_edge_domain_record> expected_source_edges;
    for (const auto &edge : table.edges) {
      if (edge.edge_class != canonical_edge_class::source_edge ||
          !edge.source_feature_owner)
        continue;
      relation_source_edge_domain_record record;
      record.canonical_edge = edge.edge.ordinal();
      record.source_edge.operand = operand;
      record.source_edge.kind = relation_feature_kind::source_edge;
      record.source_edge.primary = edge.semantic_key.primary;
      record.source_edge.secondary = edge.semantic_key.secondary;
      record.start_vertex.operand = operand;
      record.start_vertex.kind = relation_feature_kind::source_vertex;
      record.start_vertex.primary = edge.semantic_key.primary;
      record.end_vertex.operand = operand;
      record.end_vertex.kind = relation_feature_kind::source_vertex;
      record.end_vertex.primary = edge.semantic_key.secondary;
      expected_source_edges.push_back(std::move(record));
    }

    std::vector<relation_source_vertex_fan_record> expected_vertex_fans;
    expected_vertex_fans.reserve(topology.vertices().size());
    for (std::size_t i = 0; i < topology.vertices().size(); ++i) {
      const auto &vertex = topology.vertices()[i];
      if (vertex.canonical_id != i || vertex.fan >= topology.fans().size())
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source topology fan predecessor is malformed");
      const auto &fan = topology.fans()[vertex.fan];
      if (fan.canonical_id != vertex.fan || fan.vertex != vertex.canonical_id)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source topology fan identity is malformed");
      relation_source_vertex_fan_record record;
      record.canonical_vertex = vertex.canonical_id;
      record.source_vertex.operand = operand;
      record.source_vertex.kind = relation_feature_kind::source_vertex;
      record.source_vertex.primary = vertex.source_vertex;
      for (const auto halfedge_id : fan.outgoing_halfedges) {
        if (halfedge_id >= topology.halfedges().size())
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 source topology fan halfedge is malformed");
        const auto &halfedge = topology.halfedges()[halfedge_id];
        if (halfedge.canonical_id != halfedge_id ||
            halfedge.origin != vertex.canonical_id)
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 source topology fan ownership is malformed");
        relation_feature_key facet;
        if (!facet_feature(halfedge.source_facet, facet))
          return fail(relation_subcode::verifier_rejection,
                      "Component 07 source topology fan facet is malformed");
        if (record.ordered_facets.empty() ||
            record.ordered_facets.back() != facet)
          record.ordered_facets.push_back(facet);
      }
      if (record.ordered_facets.size() > 1 &&
          record.ordered_facets.front() == record.ordered_facets.back())
        record.ordered_facets.pop_back();
      expected_vertex_fans.push_back(std::move(record));
    }

    std::vector<relation_source_edge_adjacency_record> expected_adjacencies;
    expected_adjacencies.reserve(topology.edges().size());
    for (std::size_t i = 0; i < topology.edges().size(); ++i) {
      const auto &edge = topology.edges()[i];
      if (edge.canonical_id != i)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source topology edge identity is malformed");
      relation_source_edge_adjacency_record record;
      record.canonical_edge = edge.canonical_id;
      record.edge_class = edge.edge_class;
      record.edge.operand = operand;
      if (edge.edge_class == canonical_edge_class::source_edge) {
        record.edge.kind = relation_feature_kind::source_edge;
        record.edge.primary = edge.key.primary;
        record.edge.secondary = edge.key.secondary;
      } else if (edge.edge_class ==
                 canonical_edge_class::facet_internal_diagonal) {
        record.edge.kind = relation_feature_kind::facet_internal_diagonal;
        record.edge.primary = edge.source_facet;
        record.edge.secondary = edge.source_diagonal;
      } else {
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source topology edge class is malformed");
      }
      if (!facet_feature(edge.facets[0], record.first_facet) ||
          !facet_feature(edge.facets[1], record.second_facet))
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 source topology adjacency facet is malformed");
      record.source_feature_owner = edge.source_feature_owner;
      record.bookkeeping_only =
          !edge.source_feature_owner && !edge.symbolic_contact_owner &&
          !edge.classification_barrier_inside_source_facet &&
          !edge.retained_surface_feature;
      expected_adjacencies.push_back(std::move(record));
    }

    if (published.operand != operand ||
        published.schema_version !=
            contract_versions::relation_downstream_topology_schema ||
        published.reserved16 != 0 || published.reserved32 != 0 ||
        published.source_triangle_count != table.triangles.size() ||
        published.canonical_edge_count != topology.edges().size() ||
        published.source_semantic_digest != table.source_semantic_digest ||
        published.exact_topology_digest != table.exact_topology_digest ||
        published.source_edges.size() != expected_source_edges.size() ||
        published.vertex_fans.size() != expected_vertex_fans.size() ||
        published.edge_adjacencies.size() != expected_adjacencies.size())
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 downstream source topology does not reconstruct");
    for (std::size_t i = 0; i < expected_source_edges.size(); ++i) {
      const auto &record = published.source_edges[i];
      const auto &expected = expected_source_edges[i];
      if (record.source_edge != expected.source_edge ||
          record.start_vertex != expected.start_vertex ||
          record.end_vertex != expected.end_vertex ||
          record.canonical_edge != expected.canonical_edge ||
          record.schema_version != expected.schema_version ||
          record.reserved16 != expected.reserved16 ||
          record.reserved32 != expected.reserved32)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 downstream source-edge domain does not reconstruct");
    }
    for (std::size_t i = 0; i < expected_vertex_fans.size(); ++i) {
      const auto &record = published.vertex_fans[i];
      const auto &expected = expected_vertex_fans[i];
      if (record.source_vertex != expected.source_vertex ||
          record.ordered_facets != expected.ordered_facets ||
          record.canonical_vertex != expected.canonical_vertex ||
          record.schema_version != expected.schema_version ||
          record.reserved16 != expected.reserved16 ||
          record.reserved32 != expected.reserved32)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 downstream source-vertex fan does not reconstruct");
    }
    for (std::size_t i = 0; i < expected_adjacencies.size(); ++i) {
      const auto &record = published.edge_adjacencies[i];
      const auto &expected = expected_adjacencies[i];
      if (record.edge != expected.edge ||
          record.first_facet != expected.first_facet ||
          record.second_facet != expected.second_facet ||
          record.canonical_edge != expected.canonical_edge ||
          record.edge_class != expected.edge_class ||
          record.source_feature_owner != expected.source_feature_owner ||
          record.bookkeeping_only != expected.bookkeeping_only ||
          record.reserved8 != expected.reserved8 ||
          record.schema_version != expected.schema_version ||
          record.reserved32 != expected.reserved32)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 downstream source adjacency does not reconstruct");
    }
  }

  std::vector<relation_transverse_carrier_support_record>
      expected_transverse_supports;
  for (const auto &relation : artifact.relations_) {
    if (relation.family != feature_relation_family::source_facet_source_facet ||
        relation.status != feature_relation_status::proper_crossing)
      continue;
    if (relation.producer.ordinal() >= artifact.request_graph_.requests.size())
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse support request is absent");
    const auto &request =
        artifact.request_graph_.requests[relation.producer.ordinal()];
    if (request.id != relation.producer ||
        request.key.family !=
            relation_request_family::source_facet_source_facet)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse support request is malformed");
    const source_facet_source_facet_relation_record<T> *facet = nullptr;
    for (const auto &candidate : artifact.source_facet_stage_->relations) {
      if (candidate.first_feature != request.key.first ||
          candidate.second_feature != request.key.second)
        continue;
      if (facet)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 transverse facet classification is ambiguous");
      facet = &candidate;
    }
    if (!facet ||
        facet->classification != source_facet_support_relation_class::transverse ||
        !facet->has_transverse_carrier)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse facet classification is incomplete");
    const relation_construction_record *construction = nullptr;
    for (const auto &candidate : artifact.constructions_) {
      if (candidate.source_relation != relation.id ||
          candidate.kind != relation_construction_kind::bounded_carrier)
        continue;
      if (construction)
        return fail(relation_subcode::verifier_rejection,
                    "Component 07 transverse carrier construction is ambiguous");
      construction = &candidate;
    }
    if (!construction)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 transverse carrier construction is absent");
    relation_transverse_carrier_support_record expected;
    expected.relation = relation.id;
    expected.construction = construction->id;
    expected.first_facet = request.key.first;
    expected.second_facet = request.key.second;
    expected.expected_membership_count =
        static_cast<std::uint64_t>(std::count_if(
        artifact.transverse_carrier_memberships_.begin(),
        artifact.transverse_carrier_memberships_.end(), [&](const auto &record) {
          return record.carrier_relation == relation.id;
        }));
    expected.support_consistent = facet->has_transverse_carrier;
    expected.orientation_consistent = facet->has_transverse_carrier;
    expected.residuals_accepted = facet->transverse_carrier.residuals_accepted;
    expected.precision_evidence_complete =
        construction->precision_evidence_complete;
    expected_transverse_supports.push_back(std::move(expected));
  }
  if (artifact.transverse_carrier_supports_.size() !=
      expected_transverse_supports.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 downstream transverse support table is incomplete");
  for (std::size_t i = 0; i < expected_transverse_supports.size(); ++i) {
    const auto &support = artifact.transverse_carrier_supports_[i];
    const auto &expected = expected_transverse_supports[i];
    if (support.relation != expected.relation ||
        support.construction != expected.construction ||
        support.first_facet != expected.first_facet ||
        support.second_facet != expected.second_facet ||
        support.expected_membership_count !=
            expected.expected_membership_count ||
        support.support_consistent != expected.support_consistent ||
        support.orientation_consistent != expected.orientation_consistent ||
        support.residuals_accepted != expected.residuals_accepted ||
        support.precision_evidence_complete !=
            expected.precision_evidence_complete ||
        support.schema_version != expected.schema_version ||
        support.reserved16 != expected.reserved16 ||
        support.reserved32 != expected.reserved32)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 downstream transverse support does not reconstruct");
    const auto &construction =
        artifact.constructions_[support.construction.ordinal()];
    if (construction.id != support.construction ||
        construction.kind != relation_construction_kind::bounded_carrier ||
        construction.source_relation != support.relation)
      return fail(relation_subcode::verifier_rejection,
                  "Component 07 downstream transverse construction does not reconstruct");
  }

  if (!verify_relation_event_candidate_evidence(artifact, error))
    return false;
  const auto expected_candidates = artifact.candidates_->candidates().size();

  if (artifact.statistics_.candidate_count != expected_candidates ||
      artifact.statistics_.unique_request_count !=
          artifact.request_graph_.requests.size() ||
      artifact.statistics_.dependency_count !=
          artifact.request_graph_.dependencies.size() ||
      artifact.statistics_.reverse_consumer_count !=
          artifact.request_graph_.reverse_consumers.size() ||
      artifact.statistics_.candidate_witness_count !=
          artifact.request_graph_.candidate_witnesses.size() ||
      artifact.statistics_.imported_geometry_count !=
          artifact.imported_geometry_.size() ||
      artifact.statistics_.bounded_primitive_count !=
          artifact.bounded_primitives_.size() ||
      artifact.statistics_.exact_relation_count !=
          artifact.exact_relations_.size() ||
      artifact.statistics_.truth_lineage_count !=
          artifact.truth_lineage_.size() ||
      artifact.statistics_.interval_evidence_count !=
          artifact.interval_evidence_.size() ||
      artifact.statistics_.source_facet_region_count !=
          artifact.source_facet_regions_.size() ||
      artifact.statistics_.public_relation_count +
              artifact.statistics_.bookkeeping_relation_count !=
          artifact.relations_.size() ||
      artifact.statistics_.construction_count !=
          artifact.constructions_.size() ||
      artifact.statistics_.construction_ledger_count !=
          artifact.construction_ledger_.size() ||
      artifact.statistics_.transverse_carrier_membership_count !=
          artifact.transverse_carrier_memberships_.size() ||
      artifact.statistics_.coplanar_event_node_count !=
          artifact.coplanar_event_nodes_.size() ||
      artifact.statistics_.coplanar_oriented_arc_count !=
          artifact.coplanar_oriented_arcs_.size() ||
      artifact.statistics_.coplanar_overlap_component_count !=
          artifact.coplanar_overlap_components_.size() ||
      artifact.statistics_.symbolic_eligibility_count !=
          artifact.symbolic_eligibility_.size() ||
      artifact.statistics_.symbolic_decision_count !=
          artifact.symbolic_decisions_.size() ||
      artifact.statistics_.crossing_record_count != artifact.crossings_.size() ||
      artifact.statistics_.event_seed_count != artifact.event_seeds_.size() ||
      artifact.statistics_.event_seed_candidate_incidence_count !=
          artifact.event_seed_candidate_incidence_.size() ||
      artifact.statistics_.candidate_relation_coverage_count !=
          artifact.candidate_relation_coverage_.size() ||
      artifact.statistics_.candidate_seed_coverage_count !=
          artifact.candidate_event_seed_coverage_.size() ||
      artifact.statistics_.candidate_partition_count !=
          artifact.candidate_partitions_.size() ||
      artifact.statistics_.diagnostic_count != artifact.diagnostics_.size() ||
      artifact.statistics_.replay_checkpoint_count !=
          artifact.replay_checkpoints_.size())
    return fail(relation_subcode::verifier_rejection,
                "Component 07 statistics do not reconstruct from records");

  const auto &evidence = artifact.verification_evidence_;
  if (evidence.id.ordinal() != 0 ||
      evidence.verifier_version != contract_versions::relation_verifier ||
      !evidence.graph_reconstructed || !evidence.owner_exclusion_checked ||
      !evidence.selection_boundary_checked ||
      !evidence.candidate_dispositions_complete || evidence.reserved != 0 ||
      evidence.semantic_digest != artifact.graph_digest_)
    return fail(relation_subcode::verifier_rejection,
                "Component 07 verification evidence is incomplete");

  if (!verify_relation_replay_bundle(artifact, error))
    return false;

  auto owner_changed = artifact;
  owner_changed.owner_ = context_owner_token::create();
  owner_changed.request_graph_.owner = owner_changed.owner_;
  if (encode_signed_feature_relations(owner_changed) != artifact.canonical_bytes_)
    return fail(relation_subcode::owner_in_semantics,
                "runtime owner token contaminated Component 07 semantic bytes");
  return verify_relation_codec(artifact, error);
}

template bool verify_signed_feature_relations<float, std::uint32_t>(
    const signed_feature_relations<float, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_signed_feature_relations<float, std::uint64_t>(
    const signed_feature_relations<float, std::uint64_t> &,
    bounded_boolean_error &);
template bool verify_signed_feature_relations<double, std::uint32_t>(
    const signed_feature_relations<double, std::uint32_t> &,
    bounded_boolean_error &);
template bool verify_signed_feature_relations<double, std::uint64_t>(
    const signed_feature_relations<double, std::uint64_t> &,
    bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
