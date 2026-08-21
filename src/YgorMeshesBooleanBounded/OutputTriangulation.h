#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "Context.h"
#include "ExactFloatExpansion.h"
#include "FiniteInterval.h"
#include "FloatingBits.h"
#include "PolygonalOutputComplex.h"
#include "RetainedSurfaceComplex.h"
#include "TriangulationCodec.h"
#include "TriangulationVerifier.h"
#include "TriangulatedOutputComplex.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>>
build_triangulated_output_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const retained_surface_complex<T, I>> retained,
    std::shared_ptr<const polygonal_output_complex<T, I>> polygonal,
    output_triangulation_capabilities capabilities,
    output_triangulation_codec_limits codec_limits = {});

#define YGOR_DECLARE_OUTPUT_TRIANGULATION_BUILD(T, I)                        \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const triangulated_output_complex<T, I>>>              \
  build_triangulated_output_complex<T, I>(                                   \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const retained_surface_complex<T, I>>,                 \
      std::shared_ptr<const polygonal_output_complex<T, I>>,                 \
      output_triangulation_capabilities, output_triangulation_codec_limits)

YGOR_DECLARE_OUTPUT_TRIANGULATION_BUILD(float, std::uint32_t);
YGOR_DECLARE_OUTPUT_TRIANGULATION_BUILD(float, std::uint64_t);
YGOR_DECLARE_OUTPUT_TRIANGULATION_BUILD(double, std::uint32_t);
YGOR_DECLARE_OUTPUT_TRIANGULATION_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_OUTPUT_TRIANGULATION_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
namespace ygor::mesh_boolean::bounded {

namespace triangulation_detail {

inline std::array<std::size_t, 2> projected_axes(std::uint8_t dropped_axis) {
  return dropped_axis == 0 ? std::array<std::size_t, 2>{1, 2}
                           : dropped_axis == 1
                                 ? std::array<std::size_t, 2>{0, 2}
                                 : std::array<std::size_t, 2>{0, 1};
}

template <class T> struct local_point final {
  std::uint64_t occurrence = 0;
  std::array<T, 2> nominal{};
  std::array<finite_interval<T>, 2> enclosure{};
  std::array<std::uint64_t, 2> nominal_bits{};
  std::array<std::uint64_t, 2> lower_bits{};
  std::array<std::uint64_t, 2> upper_bits{};
};

struct local_edge final {
  bool is_boundary = false;
  std::uint64_t halfedge = triangulation_invalid_ordinal;
};

template <class T>
std::int8_t exact_orient_sign(const std::array<T, 2> &a,
                              const std::array<T, 2> &b,
                              const std::array<T, 2> &c) noexcept {
  const auto record = exact_orient_2d(a, b, c);
  return record.status == exact_relation_status::exact_zero
             ? 0
             : record.status == exact_relation_status::exact_positive
                   ? 1
                   : record.status == exact_relation_status::exact_negative
                         ? -1
                         : 2;
}

template <class T>
std::optional<finite_interval<T>> interval_determinant(
    const local_point<T> &a, const local_point<T> &b, const local_point<T> &c) {
  auto bax = interval_subtract(b.enclosure[0], a.enclosure[0]);
  auto bay = interval_subtract(b.enclosure[1], a.enclosure[1]);
  auto cax = interval_subtract(c.enclosure[0], a.enclosure[0]);
  auto cay = interval_subtract(c.enclosure[1], a.enclosure[1]);
  if (!bax || !bay || !cax || !cay)
    return std::nullopt;
  auto positive = interval_multiply(*bax, *cay);
  auto negative = interval_multiply(*bay, *cax);
  if (!positive || !negative)
    return std::nullopt;
  auto result = interval_subtract(*positive.value, *negative.value);
  return result ? result.value : std::nullopt;
}

template <class T>
planar_predicate_disposition classify_determinant(
    std::int8_t exact_sign, const finite_interval<T> &determinant) {
  if (finite_numeric_less(T(0), determinant.lower()))
    return planar_predicate_disposition::definite_positive;
  if (finite_numeric_less(determinant.upper(), T(0)))
    return planar_predicate_disposition::definite_negative;
  if (exact_sign == 0 && determinant.lower() == T(0) &&
      determinant.upper() == T(0))
    return planar_predicate_disposition::exact_zero;
  return planar_predicate_disposition::uncertain;
}

} // namespace triangulation_detail

// ---------------------------------------------------------------------------
// The Component 12 builder.
// ---------------------------------------------------------------------------
template <class T, class I> class output_triangulation_builder {
public:
  output_triangulation_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
      std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
      std::shared_ptr<const retained_surface_complex<T, I>> retained,
      std::shared_ptr<const polygonal_output_complex<T, I>> polygonal,
      output_triangulation_capabilities capabilities,
      output_triangulation_codec_limits codec_limits)
      : context_(context), precision_(precision),
        manifolds_(std::move(manifolds)), intersections_(std::move(intersections)),
        retained_(std::move(retained)), polygonal_(std::move(polygonal)),
        capabilities_(std::move(capabilities)), codec_limits_(codec_limits) {}

  boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return failure(error);
    if (output_triangulation_cancelled(
            capabilities_, output_triangulation_checkpoint::predecessor_validation))
      return fail_cancelled();

    auto artifact = std::make_shared<triangulated_output_complex<T, I>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->precision_digest_ = precision_.digest();
    artifact->manifold_digests_[0] = manifolds_->a()->digest();
    artifact->manifold_digests_[1] = manifolds_->b()->digest();
    artifact->intersection_digest_ = intersections_->digest();
    artifact->retained_digest_ = retained_->digest();
    artifact->polygonal_digest_ = polygonal_->digest();

    artifact->boundary_assignment_by_halfedge_.resize(
        polygonal_->halfedges().size(), triangulation_invalid_ordinal);

    for (std::size_t region = 0; region < polygonal_->face_regions().size();
         ++region) {
      if (!triangulate_region(*artifact, region, error))
        return failure(error);
      if (output_triangulation_cancelled(
              capabilities_, output_triangulation_checkpoint::region_completion))
        return fail_cancelled();
    }

    if (!finalize(*artifact, error))
      return failure(error);
    if (output_triangulation_cancelled(capabilities_,
                                       output_triangulation_checkpoint::commit))
      return fail_cancelled();
    return boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>>::
        success(std::move(artifact));
  }

private:
  using artifact_type = triangulated_output_complex<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>>;
  using local_point = triangulation_detail::local_point<T>;
  using local_edge = triangulation_detail::local_edge;

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  std::shared_ptr<const canonical_intersection_complex<T, I>> intersections_;
  std::shared_ptr<const retained_surface_complex<T, I>> retained_;
  std::shared_ptr<const polygonal_output_complex<T, I>> polygonal_;
  output_triangulation_capabilities capabilities_;
  output_triangulation_codec_limits codec_limits_;

  outcome_type failure(const bounded_boolean_error &error) {
    return outcome_type::failure(error);
  }
  outcome_type fail_cancelled() {
    return outcome_type::failure(output_triangulation_error(
        output_triangulation_subcode::cancelled,
        bounded_boolean_error_category::cancelled,
        "output triangulation cancelled",
        output_triangulation_checkpoint::commit));
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    const auto &a = *manifolds_->a();
    const auto &b = *manifolds_->b();
    if (!a.owner().same_owner(context_.owner) ||
        !b.owner().same_owner(context_.owner) ||
        !intersections_->owner().same_owner(context_.owner) ||
        !retained_->owner().same_owner(context_.owner) ||
        !polygonal_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = output_triangulation_error(
          output_triangulation_subcode::wrong_owner,
          bounded_boolean_error_category::internal_invariant_error,
          "output triangulation predecessor owner mismatch",
          output_triangulation_checkpoint::predecessor_validation);
      return false;
    }
    if (intersections_->operation() != context_.operation ||
        retained_->operation() != context_.operation ||
        polygonal_->operation() != context_.operation) {
      error = output_triangulation_error(
          output_triangulation_subcode::wrong_operation,
          bounded_boolean_error_category::internal_invariant_error,
          "output triangulation predecessor operation mismatch",
          output_triangulation_checkpoint::predecessor_validation);
      return false;
    }
    if (polygonal_->verification() !=
        output_topology_verification_disposition::independently_verified) {
      error = output_triangulation_error(
          output_triangulation_subcode::predecessor_not_verified,
          bounded_boolean_error_category::internal_invariant_error,
          "polygonal complex is not independently verified",
          output_triangulation_checkpoint::predecessor_validation);
      return false;
    }
    if (capabilities_.provider_version !=
            contract_versions::output_triangulation_provider ||
        capabilities_.codec_version !=
            contract_versions::output_triangulation_codec ||
        capabilities_.verifier_version !=
            contract_versions::output_triangulation_verifier) {
      error = output_triangulation_error(
          output_triangulation_subcode::unsupported_version,
          bounded_boolean_error_category::input_contract_error,
          "output triangulation capability version mismatch",
          output_triangulation_checkpoint::context_capability_validation);
      return false;
    }
    return true;
  }

  const canonical_halfedge_operand<T, I> &operand_manifold(operand_id id) const {
    return id == operand_id::a ? *manifolds_->a() : *manifolds_->b();
  }

  std::uint64_t representative_retained_use(std::uint64_t region) const {
    const auto &record = polygonal_->face_regions()[region];
    if (record.members_count == 0)
      return triangulation_invalid_ordinal;
    const std::uint64_t member =
        polygonal_->region_member_index()[record.members_begin];
    return polygonal_->region_members()[member].retained_use;
  }

  // -------------------------------------------------------------------------
  // Triangulate one positive-area region (one outer contour, no holes in V1).
  // -------------------------------------------------------------------------
  bool triangulate_region(artifact_type &artifact, std::uint64_t region,
                          bounded_boolean_error &error) {
    const auto &region_record = polygonal_->face_regions()[region];
    if (!region_record.positive_area) {
      error = output_triangulation_error(
          output_triangulation_subcode::unsupported_contour_hierarchy,
          bounded_boolean_error_category::internal_invariant_error,
          "zero-measure region triangulation is unsupported in V1",
          output_triangulation_checkpoint::region_task_creation);
      return false;
    }

    // Ordered boundary halfedges and occurrences.
    std::vector<std::uint64_t> boundary_halfedges;
    if (!collect_boundary(region, boundary_halfedges, error))
      return false;
    const std::size_t n = boundary_halfedges.size();
    if (n < 3) {
      error = output_triangulation_error(
          output_triangulation_subcode::invalid_region_range,
          bounded_boolean_error_category::internal_invariant_error,
          "region boundary is degenerate",
          output_triangulation_checkpoint::contour_construction);
      return false;
    }

    std::vector<std::uint64_t> occurrences;
    occurrences.reserve(n);
    for (const auto h : boundary_halfedges)
      occurrences.push_back(polygonal_->halfedges()[h].origin);

    // Support frame and projection.
    std::uint8_t dropped_axis = 0;
    operand_id source_operand = operand_id::a;
    std::uint64_t source_facet = 0;
    if (!resolve_support_frame(region, dropped_axis, source_operand, source_facet,
                               error))
      return false;

    std::vector<local_point> points;
    if (!project_occurrences(artifact, region, occurrences, dropped_axis, points,
                             error))
      return false;

    // Record the support frame.
    support_frame_record frame;
    frame.canonical_id = artifact.support_frames_.size();
    frame.region = region;
    frame.disposition = support_frame_disposition::canonical_source_facet_fallback;
    frame.source_operand = source_operand;
    frame.source_facet = source_facet;
    frame.dropped_axis = dropped_axis;
    artifact.support_frames_.push_back(std::move(frame));

    // Region polygon orientation sign (exact).
    std::vector<std::array<T, 2>> nominals;
    nominals.reserve(n);
    for (const auto &point : points)
      nominals.push_back(point.nominal);
    const auto area_record = exact_polygon_area_2d(nominals);
    if (area_record.status == exact_relation_status::exact_zero ||
        (area_record.status != exact_relation_status::exact_positive &&
         area_record.status != exact_relation_status::exact_negative)) {
      error = output_triangulation_error(
          output_triangulation_subcode::orientation_escalation_exhausted,
          bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
          "region polygon area is not definitely signed",
          output_triangulation_checkpoint::contour_construction);
      return false;
    }
    const std::int8_t polygon_sign =
        area_record.status == exact_relation_status::exact_positive ? 1 : -1;

    // Record polygon-area predicate evidence (three layers).
    record_area_evidence(artifact, region, points, polygon_sign);

    // Ear clipping.
    if (!ear_clip(artifact, region, points, boundary_halfedges, polygon_sign,
                  error))
      return false;

    // Coverage certificate.
    const std::uint64_t triangle_count = n - 2;
    const std::uint64_t diagonal_count = n - 3;
    coverage_certificate_record certificate;
    certificate.canonical_id = artifact.coverage_certificates_.size();
    certificate.region = region;
    certificate.disposition = coverage_disposition::verified;
    certificate.vertex_count = n;
    certificate.edge_count = n + diagonal_count;
    certificate.face_count = triangle_count;
    certificate.hole_count = 0;
    certificate.triangle_begin = artifact.triangles_.size() - triangle_count;
    certificate.triangle_count = triangle_count;
    certificate.diagonal_begin = artifact.diagonals_.size() - diagonal_count;
    certificate.diagonal_count = diagonal_count;
    artifact.coverage_certificates_.push_back(std::move(certificate));
    return true;
  }

  bool collect_boundary(std::uint64_t region,
                        std::vector<std::uint64_t> &boundary_halfedges,
                        bounded_boolean_error &error) {
    const auto &record = polygonal_->face_regions()[region];
    if (record.outer_contour == output_topology_invalid_ordinal ||
        record.outer_contour >= polygonal_->contour_nodes().size()) {
      error = output_triangulation_error(
          output_triangulation_subcode::invalid_region_range,
          bounded_boolean_error_category::internal_invariant_error,
          "region has no outer contour",
          output_triangulation_checkpoint::contour_construction);
      return false;
    }
    const auto &contour = polygonal_->contour_nodes()[record.outer_contour];
    if (contour.role != contour_role::outer) {
      error = output_triangulation_error(
          output_triangulation_subcode::unsupported_contour_hierarchy,
          bounded_boolean_error_category::internal_invariant_error,
          "region outer contour has an unexpected role",
          output_triangulation_checkpoint::contour_role_audit);
      return false;
    }
    const auto &cycle = polygonal_->face_cycles()[contour.cycle];
    for (std::uint64_t k = 0; k < cycle.refs_count; ++k)
      boundary_halfedges.push_back(
          polygonal_->cycle_halfedge_index()[cycle.refs_begin + k]);
    return true;
  }

  bool resolve_support_frame(std::uint64_t region, std::uint8_t &dropped_axis,
                             operand_id &source_operand,
                             std::uint64_t &source_facet,
                             bounded_boolean_error &error) {
    const std::uint64_t use = representative_retained_use(region);
    if (use == triangulation_invalid_ordinal || use >= retained_->retained_uses().size()) {
      error = output_triangulation_error(
          output_triangulation_subcode::support_frame_unavailable,
          bounded_boolean_error_category::internal_invariant_error,
          "region has no representative retained use",
          output_triangulation_checkpoint::support_frame_selection);
      return false;
    }
    const auto &retained_use = retained_->retained_uses()[use];
    source_operand = retained_use.source_operand;
    source_facet = retained_use.source_facet;
    const auto &manifold = operand_manifold(source_operand);
    const auto &facet_to_group = manifold.source_facet_to_group();
    if (source_facet >= facet_to_group.size()) {
      error = output_triangulation_error(
          output_triangulation_subcode::support_frame_unavailable,
          bounded_boolean_error_category::internal_invariant_error,
          "region source facet is out of range",
          output_triangulation_checkpoint::support_frame_selection);
      return false;
    }
    const auto group = facet_to_group[source_facet];
    if (group >= manifold.facet_groups().size()) {
      error = output_triangulation_error(
          output_triangulation_subcode::support_frame_unavailable,
          bounded_boolean_error_category::internal_invariant_error,
          "region source facet group is out of range",
          output_triangulation_checkpoint::support_frame_selection);
      return false;
    }
    dropped_axis = manifold.facet_groups()[group].basis.dropped_axis;
    if (dropped_axis > 2) {
      error = output_triangulation_error(
          output_triangulation_subcode::support_frame_unavailable,
          bounded_boolean_error_category::internal_invariant_error,
          "region support frame dropped axis is invalid",
          output_triangulation_checkpoint::support_frame_selection);
      return false;
    }
    return true;
  }

  bool project_occurrences(artifact_type &artifact, std::uint64_t region,
                           const std::vector<std::uint64_t> &occurrences,
                           std::uint8_t dropped_axis,
                           std::vector<local_point> &points,
                           bounded_boolean_error &error) {
    const auto axes = triangulation_detail::projected_axes(dropped_axis);
    for (const auto occurrence : occurrences) {
      if (occurrence >= polygonal_->vertex_occurrences().size() ||
          polygonal_->vertex_occurrences()[occurrence].coordinate_reference ==
              output_topology_invalid_ordinal ||
          polygonal_->vertex_occurrences()[occurrence].coordinate_reference >=
              polygonal_->coordinate_references().size()) {
        error = output_triangulation_error(
            output_triangulation_subcode::projection_non_finite,
            bounded_boolean_error_category::internal_invariant_error,
            "region occurrence coordinate reference is out of range",
            output_triangulation_checkpoint::projected_occurrence_construction);
        return false;
      }
      const auto &coordinate = polygonal_->coordinate_references()[
          polygonal_->vertex_occurrences()[occurrence].coordinate_reference];
      local_point point;
      point.occurrence = occurrence;
      for (std::size_t axis = 0; axis < 2; ++axis) {
        const std::size_t src_axis = axes[axis];
        const auto nominal = from_bits<T>(
            static_cast<floating_uint_t<T>>(coordinate.nominal_bits[src_axis]));
        const auto lower = from_bits<T>(
            static_cast<floating_uint_t<T>>(coordinate.lower_bits[src_axis]));
        const auto upper = from_bits<T>(
            static_cast<floating_uint_t<T>>(coordinate.upper_bits[src_axis]));
        auto enclosure = finite_interval<T>::create(lower, upper);
        if (!enclosure || !enclosure->contains(nominal) || !finite_bits(nominal)) {
          error = output_triangulation_error(
              output_triangulation_subcode::projection_non_finite,
              bounded_boolean_error_category::internal_invariant_error,
              "projected occurrence coordinate is invalid",
              output_triangulation_checkpoint::projected_occurrence_construction);
          return false;
        }
        point.nominal[axis] = nominal;
        point.enclosure[axis] = *enclosure;
        point.nominal_bits[axis] = coordinate.nominal_bits[src_axis];
        point.lower_bits[axis] = coordinate.lower_bits[src_axis];
        point.upper_bits[axis] = coordinate.upper_bits[src_axis];
      }
      points.push_back(std::move(point));
    }

    // Emit projected occurrence records.
    for (const auto &point : points) {
      projected_occurrence_record record;
      record.canonical_id = artifact.projected_occurrences_.size();
      record.key.region = region;
      record.key.component11_occurrence = point.occurrence;
      record.region = region;
      record.component11_occurrence = point.occurrence;
      record.nominal_bits = point.nominal_bits;
      record.lower_bits = point.lower_bits;
      record.upper_bits = point.upper_bits;
      artifact.projected_occurrences_.push_back(std::move(record));
    }
    return true;
  }

  void record_area_evidence(artifact_type &artifact, std::uint64_t region,
                            const std::vector<local_point> &points,
                            std::int8_t polygon_sign) {
    // Conservative interval polygon area (shoelace) for the uncertainty layer.
    std::optional<finite_interval<T>> total =
        finite_interval<T>::checked_singleton(T(0));
    std::vector<std::array<T, 2>> nominals;
    nominals.reserve(points.size());
    for (const auto &point : points)
      nominals.push_back(point.nominal);
    for (std::size_t i = 0; i < points.size(); ++i) {
      const auto &a = points[i];
      const auto &b = points[(i + 1) % points.size()];
      auto positive = interval_multiply(a.enclosure[0], b.enclosure[1]);
      auto negative = interval_multiply(b.enclosure[0], a.enclosure[1]);
      if (!positive || !negative || !total)
        break;
      auto edge = interval_subtract(*positive.value, *negative.value);
      if (!edge)
        break;
      auto next = interval_add(*total, *edge.value);
      if (!next)
        break;
      total = next.value;
    }
    (void)nominals;
    planar_predicate_evidence_record evidence;
    evidence.canonical_id = artifact.predicate_evidence_.size();
    evidence.kind = predicate_query_kind::polygon_area;
    evidence.region = region;
    evidence.operands[0] = points.front().occurrence;
    evidence.operand_count = 1;
    evidence.exact_sign = polygon_sign;
    if (total) {
      evidence.enclosure_lower_bits[0] =
          static_cast<std::uint64_t>(to_bits<T>(total->lower()));
      evidence.enclosure_upper_bits[0] =
          static_cast<std::uint64_t>(to_bits<T>(total->upper()));
    }
    evidence.disposition = polygon_sign > 0
                               ? planar_predicate_disposition::definite_positive
                               : planar_predicate_disposition::definite_negative;
    artifact.predicate_evidence_.push_back(std::move(evidence));
  }

  // -------------------------------------------------------------------------
  // Deterministic indexed ear decomposition.
  // -------------------------------------------------------------------------
  struct node_record {
    std::uint64_t prev = triangulation_invalid_ordinal;
    std::uint64_t next = triangulation_invalid_ordinal;
    std::uint64_t occurrence = 0;
    local_edge incoming{};
  };

  bool ear_clip(artifact_type &artifact, std::uint64_t region,
                const std::vector<local_point> &points,
                const std::vector<std::uint64_t> &boundary_halfedges,
                std::int8_t polygon_sign, bounded_boolean_error &error) {
    const std::size_t n = points.size();
    std::vector<node_record> nodes(n);
    for (std::size_t i = 0; i < n; ++i) {
      nodes[i].prev = (i + n - 1) % n;
      nodes[i].next = (i + 1) % n;
      nodes[i].occurrence = points[i].occurrence;
      nodes[i].incoming = {true, boundary_halfedges[(i + n - 1) % n]};
    }
    std::uint64_t active = n;

    while (active > 3) {
      bool clipped = false;
      std::uint64_t head = 0;
      // Find a canonical head: the node with the smallest occurrence ordinal.
      for (std::uint64_t i = 1; i < n; ++i)
        if (nodes[i].occurrence != triangulation_invalid_ordinal &&
            (nodes[head].occurrence == triangulation_invalid_ordinal ||
             points[i].occurrence < points[head].occurrence))
          head = i;

      std::uint64_t current = head;
      for (std::uint64_t step = 0; step < active; ++step) {
        const std::uint64_t p = nodes[current].prev;
        const std::uint64_t c = current;
        const std::uint64_t nx = nodes[current].next;

        const std::int8_t orient = triangulation_detail::exact_orient_sign(
            points[p].nominal, points[c].nominal, points[nx].nominal);
        if (orient != polygon_sign) {
          current = nx;
          continue;
        }

        // No other active occurrence may be strictly inside the ear.
        bool blocked = false;
        std::uint64_t probe = nodes[nx].next;
        while (probe != p) {
          const std::int8_t s1 = triangulation_detail::exact_orient_sign(
              points[p].nominal, points[c].nominal, points[probe].nominal);
          const std::int8_t s2 = triangulation_detail::exact_orient_sign(
              points[c].nominal, points[nx].nominal, points[probe].nominal);
          const std::int8_t s3 = triangulation_detail::exact_orient_sign(
              points[nx].nominal, points[p].nominal, points[probe].nominal);
          if (s1 == polygon_sign && s2 == polygon_sign && s3 == polygon_sign) {
            blocked = true;
            break;
          }
          probe = nodes[probe].next;
        }
        if (blocked) {
          current = nx;
          continue;
        }

        if (!clip_ear(artifact, region, points, nodes, p, c, nx, error))
          return false;
        clipped = true;
        --active;
        break;
      }
      if (!clipped) {
        error = output_triangulation_error(
            output_triangulation_subcode::ear_candidate_stalled,
            bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
            "ear decomposition stalled with no admissible ear",
            output_triangulation_checkpoint::ear_decomposition);
        return false;
      }
    }

    // Final triangle.
    std::uint64_t head = 0;
    for (std::uint64_t i = 1; i < n; ++i)
      if (nodes[i].occurrence != triangulation_invalid_ordinal &&
          (nodes[head].occurrence == triangulation_invalid_ordinal ||
           nodes[i].occurrence < nodes[head].occurrence))
        head = i;
    const std::uint64_t a = head;
    const std::uint64_t b = nodes[a].next;
    const std::uint64_t c = nodes[b].next;
    return emit_triangle(artifact, region, points, nodes, a, b, c, error);
  }

  bool clip_ear(artifact_type &artifact, std::uint64_t region,
                const std::vector<local_point> &points,
                std::vector<node_record> &nodes, std::uint64_t p, std::uint64_t c,
                std::uint64_t nx, bounded_boolean_error &error) {
    // Allocate closing diagonal (p, nx).
    std::array<std::uint64_t, 2> diagonal_halfedges{triangulation_invalid_ordinal,
                                                    triangulation_invalid_ordinal};
    if (!allocate_diagonal(artifact, region, points[p].occurrence,
                           points[nx].occurrence, diagonal_halfedges))
      return false;

    // Triangle (p, c, nx) uses incoming[c], incoming[nx], and the reverse
    // diagonal halfedge (nx -> p).
    const auto &diag = artifact.diagonals_.back();
    const std::uint64_t reverse = diag.halfedges[1]; // nx -> p
    const std::array<local_edge, 3> edges{
        nodes[c].incoming, nodes[nx].incoming,
        local_edge{false, reverse}};
    const std::array<std::uint64_t, 3> corners{
        points[p].occurrence, points[c].occurrence, points[nx].occurrence};

    // Record the triangle before mutating the ring.
    std::uint64_t triangle = artifact.triangles_.size();
    if (!record_triangle(artifact, region, corners, edges))
      return false;

    // Assign the two boundary edge uses.
    if (!assign_edge(artifact, nodes[c].incoming, triangle, error) ||
        !assign_edge(artifact, nodes[nx].incoming, triangle, error))
      return false;

    // Splice the ring: p -> nx (forward diagonal), remove c.
    nodes[nx].prev = p;
    nodes[p].next = nx;
    nodes[nx].incoming = local_edge{false, diag.halfedges[0]}; // p -> nx
    nodes[c].occurrence = triangulation_invalid_ordinal;
    return true;
  }

  bool emit_triangle(artifact_type &artifact, std::uint64_t region,
                     const std::vector<local_point> &points,
                     const std::vector<node_record> &nodes, std::uint64_t a,
                     std::uint64_t b, std::uint64_t c,
                     bounded_boolean_error &error) {
    const std::array<local_edge, 3> edges{nodes[b].incoming, nodes[c].incoming,
                                          nodes[a].incoming};
    const std::array<std::uint64_t, 3> corners{
        points[a].occurrence, points[b].occurrence, points[c].occurrence};
    const std::uint64_t triangle = artifact.triangles_.size();
    if (!record_triangle(artifact, region, corners, edges))
      return false;
    for (const auto &edge : edges)
      if (edge.is_boundary)
        if (!assign_edge(artifact, edge, triangle, error))
          return false;
    return true;
  }

  bool allocate_diagonal(artifact_type &artifact, std::uint64_t region,
                         std::uint64_t endpoint_a, std::uint64_t endpoint_b,
                         std::array<std::uint64_t, 2> &halfedges_out) {
    const std::uint64_t diagonal = artifact.diagonals_.size();
    const std::uint64_t h0 = artifact.internal_halfedges_.size();
    const std::uint64_t h1 = h0 + 1;

    triangulation_halfedge_record forward;
    forward.canonical_id = h0;
    forward.pair = h1;
    forward.origin = endpoint_a;
    forward.destination = endpoint_b;
    forward.region = region;
    forward.diagonal = diagonal;
    artifact.internal_halfedges_.push_back(std::move(forward));

    triangulation_halfedge_record reverse;
    reverse.canonical_id = h1;
    reverse.pair = h0;
    reverse.origin = endpoint_b;
    reverse.destination = endpoint_a;
    reverse.region = region;
    reverse.diagonal = diagonal;
    artifact.internal_halfedges_.push_back(std::move(reverse));

    internal_diagonal_record record;
    record.canonical_id = diagonal;
    record.key = internal_diagonal_key::canonical(region, endpoint_a, endpoint_b);
    record.region = region;
    record.halfedges = {h0, h1};
    record.endpoints = {endpoint_a, endpoint_b};
    record.state = diagonal_state::both_triangle_sides_assigned;
    artifact.diagonals_.push_back(std::move(record));

    halfedges_out = {h0, h1};
    return true;
  }

  bool record_triangle(artifact_type &artifact, std::uint64_t region,
                       const std::array<std::uint64_t, 3> &corners,
                       const std::array<local_edge, 3> &edges) {
    output_triangle_record triangle;
    triangle.canonical_id = artifact.triangles_.size();
    triangle.key = output_triangle_key::canonical(region, corners);
    triangle.region = region;
    triangle.corners = corners;
    triangle.category = triangle_geometric_category::definite_positive_area;

    for (std::size_t k = 0; k < 3; ++k) {
      triangle_corner_ref_record corner_ref;
      corner_ref.canonical_id = artifact.corner_refs_.size();
      corner_ref.triangle = triangle.canonical_id;
      corner_ref.occurrence = corners[k];
      triangle.corner_refs[k] = corner_ref.canonical_id;
      artifact.corner_refs_.push_back(std::move(corner_ref));

      triangle_edge_use_ref_record edge_use;
      edge_use.canonical_id = artifact.edge_use_refs_.size();
      edge_use.triangle = triangle.canonical_id;
      edge_use.is_boundary = edges[k].is_boundary;
      edge_use.halfedge = edges[k].halfedge;
      triangle.edge_use_refs[k] = edge_use.canonical_id;
      artifact.edge_use_refs_.push_back(std::move(edge_use));
    }

    artifact.triangles_.push_back(std::move(triangle));
    return true;
  }

  bool assign_edge(artifact_type &artifact, const local_edge &edge,
                   std::uint64_t triangle, bounded_boolean_error &error) {
    if (!edge.is_boundary)
      return true; // internal halfedges are balanced via their pair uses.
    if (edge.halfedge >= artifact.boundary_assignment_by_halfedge_.size() ||
        artifact.boundary_assignment_by_halfedge_[edge.halfedge] !=
            triangulation_invalid_ordinal) {
      error = output_triangulation_error(
          output_triangulation_subcode::boundary_multiply_assigned,
          bounded_boolean_error_category::internal_invariant_error,
          "boundary halfedge is multiply assigned",
          output_triangulation_checkpoint::region_completion);
      return false;
    }
    boundary_assignment_record assignment;
    assignment.canonical_id = artifact.boundary_assignments_.size();
    assignment.component11_halfedge = edge.halfedge;
    assignment.triangle = triangle;
    assignment.kind = boundary_assignment_kind::triangle;
    artifact.boundary_assignment_by_halfedge_[edge.halfedge] =
        assignment.canonical_id;
    artifact.boundary_assignments_.push_back(std::move(assignment));
    return true;
  }

  bool finalize(artifact_type &artifact, bounded_boolean_error &error) {
    artifact.statistics_.region_count =
        artifact.coverage_certificates_.size();
    artifact.statistics_.support_frame_count = artifact.support_frames_.size();
    artifact.statistics_.projected_occurrence_count =
        artifact.projected_occurrences_.size();
    artifact.statistics_.predicate_evidence_count =
        artifact.predicate_evidence_.size();
    artifact.statistics_.internal_diagonal_count = artifact.diagonals_.size();
    artifact.statistics_.internal_halfedge_count =
        artifact.internal_halfedges_.size();
    artifact.statistics_.triangle_count = artifact.triangles_.size();
    artifact.statistics_.corner_ref_count = artifact.corner_refs_.size();
    artifact.statistics_.edge_use_ref_count = artifact.edge_use_refs_.size();
    artifact.statistics_.residual_cell_count = 0;
    artifact.statistics_.cleanup_obligation_count =
        artifact.cleanup_certificates_.size();
    artifact.statistics_.boundary_assignment_count =
        artifact.boundary_assignments_.size();
    artifact.statistics_.coverage_certificate_count =
        artifact.coverage_certificates_.size();

    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_triangulated_output_complex(artifact, bytes, codec_limits_,
                                            codec_error)) {
      error = codec_error;
      return false;
    }
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();

    if (output_triangulation_cancelled(
            capabilities_, output_triangulation_checkpoint::independent_verification))
      return true;
    bounded_boolean_error verification_error;
    if (!verify_triangulated_output_complex(
            artifact, context_, *manifolds_, *intersections_, *retained_,
            *polygonal_, verification_error)) {
      error = verification_error;
      return false;
    }
    artifact.verification_ =
        triangulation_verification_disposition::independently_verified;
    return true;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>>
build_triangulated_output_complex(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const retained_surface_complex<T, I>> retained,
    std::shared_ptr<const polygonal_output_complex<T, I>> polygonal,
    output_triangulation_capabilities capabilities,
    output_triangulation_codec_limits codec_limits) {
  try {
    output_triangulation_builder<T, I> builder(
        context, precision, std::move(manifolds), std::move(intersections),
        std::move(retained), std::move(polygonal), std::move(capabilities),
        codec_limits);
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>>::
        failure(output_triangulation_error(
            output_triangulation_subcode::resource_preflight,
            bounded_boolean_error_category::resource_limit,
            "output triangulation allocation failed",
            output_triangulation_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const triangulated_output_complex<T, I>>>::
        failure(output_triangulation_error(
            output_triangulation_subcode::internal_invariant,
            bounded_boolean_error_category::internal_invariant_error,
            "output triangulation unexpected exception",
            output_triangulation_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
