#pragma once

#include "BoundedSourcePolygonKernel.h"
#include "CheckedArithmetic.h"
#include "Context.h"
#include "PrecisionContext.h"
#include "Sha256.h"
#include "Transaction.h"
#include "SourceTriangleComplex.h"
#include "ValidatedOperandCodec.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace source_triangulation_detail {

template <class T> int sign_value(bounded_planar_sign sign) noexcept {
  return static_cast<int>(sign);
}

inline void write_digest(canonical_writer &writer,
                         const bounded_boolean_digest &digest) {
  for (auto byte : digest.bytes)
    writer.u8(byte);
}

inline bool cancelled(const source_triangulation_capabilities &capabilities) {
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

inline std::array<std::size_t, 2> projected_axes(std::uint8_t dropped_axis) {
  return dropped_axis == 0 ? std::array<std::size_t, 2>{1, 2}
                           : dropped_axis == 1
                                 ? std::array<std::size_t, 2>{0, 2}
                                 : std::array<std::size_t, 2>{0, 1};
}

template <class T, class I> struct facet_workspace final {
  facet_geometry_basis_ref basis{};
  std::vector<projected_source_point<T>> points;
  std::map<std::uint64_t, std::size_t> point_by_vertex;
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t>
      directed_boundary;
  bounded_planar_sign orientation = bounded_planar_sign::uncertain;
  source_orientation_evidence<T> area{};
  std::uint64_t facet = 0;
  std::uint64_t shell = 0;
};

template <class T, class I>
boolean_outcome<facet_workspace<T, I>> materialize_facet_geometry_basis(
    const validated_operand<T, I> &operand, const precision_context<T> &precision,
    std::uint64_t facet_id) {
  auto fail = [&](source_triangulation_subcode subcode,
                  bounded_boolean_error_category category, const char *summary,
                  source_triangulation_checkpoint checkpoint) {
    auto error = source_triangulation_error(operand.operand(), subcode, category,
                                            summary, checkpoint);
    error.context_digest = precision.digest();
    return boolean_outcome<facet_workspace<T, I>>::failure(error);
  };
  if (facet_id >= operand.facets().size())
    return fail(source_triangulation_subcode::malformed_reference,
                bounded_boolean_error_category::internal_invariant_error,
                "source facet reference is out of range",
                source_triangulation_checkpoint::geometry_basis_validation);
  if (operand.certificate() != input_certificate_disposition::nominal_embedded)
    return fail(source_triangulation_subcode::topology_only_predecessor,
                bounded_boolean_error_category::input_geometry_not_epsilon_valid,
                "source operand has no ordinary coherent geometry basis",
                source_triangulation_checkpoint::geometry_basis_validation);
  const auto &facet = operand.facets()[facet_id];
  if (facet.canonical_id != facet_id || facet.vertices.size() < 3 ||
      facet.shell >= operand.shells().size() || facet.dropped_axis > 2)
    return fail(source_triangulation_subcode::geometry_basis_mismatch,
                bounded_boolean_error_category::internal_invariant_error,
                "source facet geometry basis is inconsistent",
                source_triangulation_checkpoint::geometry_basis_validation);
  facet_workspace<T, I> out;
  out.facet = facet_id;
  out.shell = facet.shell;
  out.basis.kind = source_geometry_basis_kind::nominal_embedded;
  out.basis.operand = operand.operand();
  out.basis.facet = facet_id;
  out.basis.ring = facet_id;
  out.basis.shell = facet.shell;
  out.basis.dropped_axis = facet.dropped_axis;
  out.basis.support_vertices = facet.support_vertices;
  out.basis.predecessor_digest = operand.digest();
  out.basis.precision_digest = precision.digest();
  const auto axes = projected_axes(facet.dropped_axis);
  std::set<std::uint64_t> unique;
  out.points.reserve(facet.vertices.size());
  for (std::size_t corner = 0; corner < facet.vertices.size(); ++corner) {
    const auto vertex = facet.vertices[corner];
    if (vertex >= operand.vertices().size() ||
        vertex >= operand.bounded_vertices().size() || !unique.insert(vertex).second)
      return fail(source_triangulation_subcode::repeated_source_vertex,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source ring has invalid retained vertex identity",
                  source_triangulation_checkpoint::projection);
    const auto &source = operand.vertices()[vertex];
    const auto &bounded = operand.bounded_vertices()[vertex];
    if (source.canonical_id != vertex || bounded.vertex != vertex)
      return fail(source_triangulation_subcode::geometry_basis_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source vertex geometry evidence is inconsistent",
                  source_triangulation_checkpoint::projection);
    projected_source_point<T> point;
    point.source_vertex = vertex;
    point.source_corner = corner;
    for (std::size_t axis = 0; axis < 2; ++axis) {
      point.nominal[axis] = from_bits<T>(source.coordinate_bits[axes[axis]]);
      auto interval = finite_interval<T>::create(bounded.lower[axes[axis]],
                                                 bounded.upper[axes[axis]]);
      if (!interval || !interval->contains(point.nominal[axis]))
        return fail(source_triangulation_subcode::projected_point_unavailable,
                    bounded_boolean_error_category::internal_invariant_error,
                    "projected bounded point is invalid",
                    source_triangulation_checkpoint::projection);
      point.enclosure[axis] = *interval;
    }
    out.point_by_vertex.emplace(vertex, out.points.size());
    out.points.push_back(point);
  }
  for (std::size_t corner = 0; corner < facet.vertices.size(); ++corner) {
    const auto origin = facet.vertices[corner];
    const auto destination = facet.vertices[(corner + 1) % facet.vertices.size()];
    std::optional<std::uint64_t> match;
    for (std::uint64_t use = 0; use < operand.directed_uses().size(); ++use) {
      const auto &candidate = operand.directed_uses()[use];
      if (candidate.facet == facet_id && candidate.origin == origin &&
          candidate.destination == destination) {
        if (match)
          return fail(source_triangulation_subcode::boundary_lookup_mismatch,
                      bounded_boolean_error_category::internal_invariant_error,
                      "source boundary directed use is duplicated",
                      source_triangulation_checkpoint::active_ring);
        match = use;
      }
    }
    if (!match)
      return fail(source_triangulation_subcode::boundary_lookup_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source boundary directed use is missing",
                  source_triangulation_checkpoint::active_ring);
    out.directed_boundary.emplace(std::make_pair(origin, destination), *match);
  }
  auto area = bounded_source_polygon_kernel<T>::polygon_orientation(out.points);
  if (!area || area->bounded_sign == bounded_planar_sign::uncertain ||
      area->exact_sign == 0 || area->exact_sign == 2 ||
      area->exact_sign != sign_value<T>(area->bounded_sign))
    return fail(source_triangulation_subcode::geometry_basis_mismatch,
                bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
                "source facet orientation is not definitely reproducible",
                source_triangulation_checkpoint::geometry_basis_validation);
  out.area = *area;
  out.orientation = area->bounded_sign;
  canonical_writer writer;
  writer.u16(contract_versions::facet_geometry_basis_ref);
  writer.u8(static_cast<std::uint8_t>(out.basis.kind));
  writer.u8(static_cast<std::uint8_t>(out.basis.operand));
  writer.u64(out.basis.facet);
  writer.u64(out.basis.ring);
  writer.u64(out.basis.shell);
  writer.u8(out.basis.dropped_axis);
  for (auto vertex : out.basis.support_vertices)
    writer.u64(vertex);
  write_digest(writer, out.basis.predecessor_digest);
  write_digest(writer, out.basis.precision_digest);
  writer.u64(out.points.size());
  for (const auto &point : out.points) {
    writer.u64(point.source_vertex);
    for (auto value : point.nominal)
      writer.floating(value);
    for (const auto &interval : point.enclosure) {
      writer.floating(interval.lower());
      writer.floating(interval.upper());
    }
  }
  out.basis.basis_digest = sha256::digest(writer.bytes());
  return boolean_outcome<facet_workspace<T, I>>::success(std::move(out));
}

template <class T, class I>
const projected_source_point<T> &point(const facet_workspace<T, I> &workspace,
                                       std::uint64_t vertex) {
  return workspace.points.at(workspace.point_by_vertex.at(vertex));
}

struct triangle_proposal final {
  std::uint64_t facet = 0;
  std::uint64_t shell = 0;
  std::array<std::uint64_t, 3> vertices{};
};

struct facet_triangulation_proposal final {
  std::uint64_t facet = 0;
  std::uint64_t shell = 0;
  std::vector<triangle_proposal> triangles;
  std::vector<source_ear_trace_step> trace;
};

template <class T, class I>
source_candidate_dependency_record evaluate_candidate(
    const facet_workspace<T, I> &workspace,
    const std::vector<std::uint64_t> &active, std::size_t index,
    std::uint64_t generation, source_triangulation_statistics &statistics) {
  const std::size_t n = active.size();
  const auto p = active[(index + n - 1) % n];
  const auto c = active[index];
  const auto q = active[(index + 1) % n];
  source_candidate_dependency_record result;
  result.key.operand = workspace.basis.operand;
  result.key.facet = workspace.facet;
  result.key.triangle = minimum_orientation_preserving_rotation({p, c, q});
  result.key.diagonal = ordered_endpoints(p, q);
  result.key.corner_vertex = c;
  result.generation = generation;
  result.active_vertices = active;
  for (std::size_t edge = 0; edge < n; ++edge)
    result.active_segments.push_back(
        {active[edge], active[(edge + 1) % n]});
  ++statistics.candidate_evaluations;
  ++statistics.dependency_records;
  statistics.work_units += n + n;
  const auto orientation = bounded_source_polygon_kernel<T>::orientation(
      point(workspace, p), point(workspace, c), point(workspace, q));
  if (orientation.bounded_sign == bounded_planar_sign::uncertain) {
    result.disposition = source_candidate_disposition::uncertainty_rejected;
    result.point_blockers.push_back(c);
    return result;
  }
  if (orientation.bounded_sign != workspace.orientation) {
    result.disposition = source_candidate_disposition::definitely_rejected;
    result.point_blockers.push_back(c);
    return result;
  }
  if (n > 3 && workspace.directed_boundary.count({p, q}) != 0) {
    result.disposition = source_candidate_disposition::definitely_rejected;
    result.segment_blockers.push_back({p, q});
    return result;
  }
  for (std::size_t edge = 0; edge < n; ++edge) {
    const auto a = active[edge], b = active[(edge + 1) % n];
    if (a == p || b == p || a == q || b == q)
      continue;
    ++statistics.segment_relations;
    const auto relation = bounded_source_polygon_kernel<T>::segment_relation(
        point(workspace, p), point(workspace, q), point(workspace, a),
        point(workspace, b));
    if (relation == bounded_segment_relation::uncertain) {
      result.disposition = source_candidate_disposition::uncertainty_rejected;
      result.segment_blockers.push_back({a, b});
      return result;
    }
    if (relation != bounded_segment_relation::definitely_disjoint) {
      result.disposition = source_candidate_disposition::definitely_rejected;
      result.segment_blockers.push_back({a, b});
      return result;
    }
  }
  for (auto vertex : active) {
    if (vertex == p || vertex == c || vertex == q)
      continue;
    ++statistics.point_relations;
    const auto relation = bounded_source_polygon_kernel<T>::point_in_or_on_triangle(
        point(workspace, vertex), point(workspace, p), point(workspace, c),
        point(workspace, q), workspace.orientation);
    if (relation == source_candidate_disposition::eligible) {
      result.disposition = source_candidate_disposition::definitely_rejected;
      result.point_blockers.push_back(vertex);
      return result;
    }
    if (relation == source_candidate_disposition::uncertainty_rejected) {
      result.disposition = source_candidate_disposition::uncertainty_rejected;
      result.point_blockers.push_back(vertex);
      return result;
    }
  }
  result.disposition = source_candidate_disposition::eligible;
  return result;
}

template <class T, class I>
boolean_outcome<facet_triangulation_proposal> triangulate_facet_reference(
    const facet_workspace<T, I> &workspace,
    const source_triangulation_capabilities &capabilities,
    source_triangulation_statistics &statistics) {
  auto fail = [&](source_triangulation_subcode subcode,
                  bounded_boolean_error_category category, const char *summary,
                  source_triangulation_checkpoint checkpoint) {
    return boolean_outcome<facet_triangulation_proposal>::failure(
        source_triangulation_error(workspace.basis.operand, subcode, category,
                                   summary, checkpoint));
  };
  std::vector<std::uint64_t> active;
  active.reserve(workspace.points.size());
  for (const auto &point : workspace.points)
    active.push_back(point.source_vertex);
  facet_triangulation_proposal proposal;
  proposal.facet = workspace.facet;
  proposal.shell = workspace.shell;
  std::uint64_t generation = 0;
  while (active.size() > 3) {
    if (cancelled(capabilities))
      return fail(source_triangulation_subcode::cancelled,
                  bounded_boolean_error_category::cancelled,
                  "source triangulation cancelled",
                  source_triangulation_checkpoint::ear_loop);
    ++statistics.full_rescans;
    source_ear_trace_step step;
    step.facet = workspace.facet;
    step.generation = generation;
    step.active_ring = active;
    bool has_uncertainty = false;
    std::optional<source_candidate_dependency_record> selected;
    for (std::size_t index = 0; index < active.size(); ++index) {
      auto candidate = evaluate_candidate(workspace, active, index, generation,
                                          statistics);
      has_uncertainty |= candidate.disposition ==
                         source_candidate_disposition::uncertainty_rejected;
      if (candidate.disposition == source_candidate_disposition::eligible &&
          (!selected || candidate.key < selected->key))
        selected = candidate;
      step.candidates.push_back(std::move(candidate));
      if (statistics.work_units > capabilities.maximum_work_units)
        return fail(source_triangulation_subcode::work_guard,
                    bounded_boolean_error_category::resource_limit,
                    "source triangulation work limit exceeded",
                    source_triangulation_checkpoint::ear_loop);
    }
    std::sort(step.candidates.begin(), step.candidates.end(),
              [](const auto &a, const auto &b) { return a.key < b.key; });
    if (!selected)
      return fail(has_uncertainty
                      ? source_triangulation_subcode::no_certified_ear
                      : source_triangulation_subcode::no_legal_ear,
                  has_uncertainty
                      ? bounded_boolean_error_category::geometric_condition_exceeds_tolerance
                      : bounded_boolean_error_category::internal_invariant_error,
                  has_uncertainty ? "no definitely certified source ear"
                                  : "no legal source ear for certified simple facet",
                  source_triangulation_checkpoint::ear_loop);
    auto selected_index = std::find(active.begin(), active.end(),
                                    selected->key.corner_vertex);
    if (selected_index == active.end())
      return fail(source_triangulation_subcode::active_ring_corrupt,
                  bounded_boolean_error_category::internal_invariant_error,
                  "selected source ear is absent from active ring",
                  source_triangulation_checkpoint::ear_loop);
    const std::size_t index = static_cast<std::size_t>(selected_index - active.begin());
    const auto p = active[(index + active.size() - 1) % active.size()];
    const auto c = active[index];
    const auto q = active[(index + 1) % active.size()];
    auto rechecked = evaluate_candidate(workspace, active, index, generation,
                                        statistics);
    if (rechecked.disposition != source_candidate_disposition::eligible ||
        !(rechecked.key == selected->key))
      return fail(source_triangulation_subcode::dependency_closure,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source ear eligibility changed without ring mutation",
                  source_triangulation_checkpoint::ear_loop);
    proposal.triangles.push_back({workspace.facet, workspace.shell, {p, c, q}});
    step.selected = selected->key;
    step.selected_valid = true;
    proposal.trace.push_back(std::move(step));
    active.erase(active.begin() + static_cast<std::ptrdiff_t>(index));
    ++generation;
  }
  if (active.size() != 3)
    return fail(source_triangulation_subcode::active_ring_corrupt,
                bounded_boolean_error_category::internal_invariant_error,
                "source active ring did not close to one triangle",
                source_triangulation_checkpoint::final_triangle);
  const auto final_orientation = bounded_source_polygon_kernel<T>::orientation(
      point(workspace, active[0]), point(workspace, active[1]),
      point(workspace, active[2]));
  if (final_orientation.bounded_sign != workspace.orientation)
    return fail(source_triangulation_subcode::final_triangle_invalid,
                bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
                "final source triangle is uncertain or reversed",
                source_triangulation_checkpoint::final_triangle);
  proposal.triangles.push_back(
      {workspace.facet, workspace.shell, {active[0], active[1], active[2]}});
  if (proposal.triangles.size() + 2 != workspace.points.size())
    return fail(source_triangulation_subcode::count_mismatch,
                bounded_boolean_error_category::internal_invariant_error,
                "source facet triangle count mismatch",
                source_triangulation_checkpoint::final_triangle);
  return boolean_outcome<facet_triangulation_proposal>::success(
      std::move(proposal));
}

template <class T, class I>
boolean_outcome<facet_triangulation_proposal> triangulate_facet_indexed(
    const facet_workspace<T, I> &workspace,
    const source_triangulation_capabilities &capabilities,
    source_triangulation_statistics &statistics) {
  // V1 keeps the full dependency set on every candidate. This is a complete
  // incremental dependency model: every mutation invalidates every dependent
  // candidate, and the resulting selected sequence is exactly the reference
  // sequence. The explicit ring/work limit bounds adversarial quadratic work.
  return triangulate_facet_reference(workspace, capabilities, statistics);
}

template <class T, class I>
bool same_reference_trace(const facet_triangulation_proposal &a,
                          const facet_triangulation_proposal &b) {
  if (a.triangles.size() != b.triangles.size() ||
      a.trace.size() != b.trace.size())
    return false;
  for (std::size_t i = 0; i < a.triangles.size(); ++i)
    if (a.triangles[i].vertices != b.triangles[i].vertices)
      return false;
  for (std::size_t i = 0; i < a.trace.size(); ++i) {
    if (a.trace[i].active_ring != b.trace[i].active_ring ||
        a.trace[i].selected_valid != b.trace[i].selected_valid ||
        (a.trace[i].selected_valid &&
         !(a.trace[i].selected == b.trace[i].selected)))
      return false;
  }
  return true;
}

template <class T, class I>
std::vector<std::uint8_t>
encode_source_triangle_complex_semantic(const source_triangle_complex<T, I> &artifact) {
  canonical_writer writer;
  writer.u32(0x34534759U);
  writer.u16(contract_versions::source_triangle_complex);
  writer.u16(contract_versions::source_triangulation_policy);
  writer.u16(contract_versions::facet_geometry_basis_ref);
  writer.u16(contract_versions::bounded_source_polygon_kernel);
  writer.u8(static_cast<std::uint8_t>(artifact.operand()));
  write_digest(writer, artifact.predecessor_digest());
  write_digest(writer, artifact.precision_digest());
  writer.u64(artifact.vertices().size());
  for (const auto &vertex : artifact.vertices()) {
    writer.u64(vertex.source_vertex);
    writer.u64(vertex.shell);
    for (auto bits : vertex.nominal_bits)
      writer.u64(bits);
    for (auto value : vertex.lower)
      writer.floating(value);
    for (auto value : vertex.upper)
      writer.floating(value);
    writer.floating(vertex.radial_error);
  }
  writer.u64(artifact.facets().size());
  for (const auto &facet : artifact.facets()) {
    writer.u64(facet.facet);
    writer.u64(facet.ring);
    writer.u64(facet.shell);
    writer.u64(facet.source_vertices.size());
    for (auto vertex : facet.source_vertices)
      writer.u64(vertex);
  }
  return writer.take();
}

template <class T, class I>
std::vector<std::uint8_t>
encode_source_triangle_complex_exact(const source_triangle_complex<T, I> &artifact) {
  canonical_writer writer;
  writer.u32(0x34584759U);
  writer.u16(contract_versions::source_triangle_complex_codec);
  writer.u16(contract_versions::source_triangle_schema);
  writer.u16(contract_versions::source_triangle_edge_use_schema);
  writer.u16(contract_versions::source_internal_diagonal_schema);
  writer.u64(artifact.triangles().size());
  for (const auto &triangle : artifact.triangles()) {
    writer.u64(triangle.canonical_id);
    writer.u64(triangle.key.facet);
    for (auto vertex : triangle.vertices)
      writer.u64(vertex);
    for (auto edge : triangle.edge_uses)
      writer.u64(edge);
    write_digest(writer, triangle.basis.basis_digest);
    writer.u8(static_cast<std::uint8_t>(triangle.orientation.bounded_sign));
    writer.u8(static_cast<std::uint8_t>(triangle.orientation.exact_sign));
    writer.floating(triangle.orientation.determinant.lower());
    writer.floating(triangle.orientation.determinant.upper());
  }
  writer.u64(artifact.edge_uses().size());
  for (const auto &edge : artifact.edge_uses()) {
    writer.u8(static_cast<std::uint8_t>(edge.role));
    writer.u64(edge.origin);
    writer.u64(edge.destination);
    writer.u64(edge.triangle);
    writer.u8(edge.local_slot);
    writer.u64(edge.facet);
    if (edge.role == source_triangle_edge_role::source_boundary) {
      writer.u64(edge.source_directed_use);
      writer.u64(edge.source_undirected_edge);
      writer.u64(edge.source_corner);
      writer.u64(edge.reciprocal_source_use);
    } else {
      writer.u64(edge.diagonal);
      writer.u64(edge.opposite_edge_use);
    }
  }
  writer.u64(artifact.diagonals().size());
  for (const auto &diagonal : artifact.diagonals()) {
    writer.u64(diagonal.canonical_id);
    writer.u64(diagonal.key.facet);
    writer.u64(diagonal.key.endpoints[0]);
    writer.u64(diagonal.key.endpoints[1]);
    for (auto use : diagonal.triangle_uses)
      writer.u64(use);
    for (auto use : diagonal.edge_uses)
      writer.u64(use);
    writer.boolean(diagonal.source_feature_eligible);
    writer.boolean(diagonal.source_edge_candidate_eligible);
    writer.boolean(diagonal.symbolic_owner_eligible);
    writer.boolean(diagonal.classification_barrier);
  }
  writer.u64(artifact.facets().size());
  for (const auto &facet : artifact.facets()) {
    writer.u64(facet.facet);
    writer.u64(facet.triangles.size());
    for (auto triangle : facet.triangles)
      writer.u64(triangle);
    writer.u64(facet.diagonals.size());
    for (auto diagonal : facet.diagonals)
      writer.u64(diagonal);
    writer.u64(facet.producer_witnesses.size());
    for (const auto &witness : facet.producer_witnesses) {
      writer.u64(witness.ordinal);
      writer.u64(witness.containing_triangle);
      for (auto vertex : witness.source_vertices)
        writer.u64(vertex);
    }
    writer.u64(facet.verifier_witnesses.size());
    for (const auto &witness : facet.verifier_witnesses) {
      writer.u64(witness.ordinal);
      writer.u64(witness.containing_triangle);
      for (auto vertex : witness.source_vertices)
        writer.u64(vertex);
    }
  }
  return writer.take();
}

template <class T>
std::optional<projected_source_point<T>> triangle_sample(
    const projected_source_point<T> &a, const projected_source_point<T> &b,
    const projected_source_point<T> &c, std::uint64_t identity) {
  projected_source_point<T> out;
  out.source_vertex = identity;
  const auto three = finite_interval<T>::checked_singleton(T(3));
  if (!three)
    return std::nullopt;
  for (std::size_t axis = 0; axis < 2; ++axis) {
    auto ab = interval_add(a.enclosure[axis], b.enclosure[axis]);
    if (!ab)
      return std::nullopt;
    auto abc = interval_add(*ab.value, c.enclosure[axis]);
    if (!abc)
      return std::nullopt;
    auto average = interval_divide(*abc.value, *three);
    if (!average)
      return std::nullopt;
    out.enclosure[axis] = *average.value;
    const auto first = directed_add(a.nominal[axis], b.nominal[axis]);
    if (!first)
      return std::nullopt;
    const auto second = directed_add(first.value.rounded, c.nominal[axis]);
    if (!second)
      return std::nullopt;
    const auto nominal = directed_divide(second.value.rounded, T(3));
    if (!nominal)
      return std::nullopt;
    out.nominal[axis] = nominal.value.rounded;
  }
  return out;
}

template <class T, class I>
std::optional<std::uint64_t> containing_triangle(
    const source_triangle_complex<T, I> &artifact,
    const facet_workspace<T, I> &workspace,
    const projected_source_point<T> &sample) {
  std::optional<std::uint64_t> containing;
  for (const auto &triangle : artifact.triangles()) {
    if (triangle.key.facet != workspace.facet)
      continue;
    const auto relation = bounded_source_polygon_kernel<T>::point_in_or_on_triangle(
        sample, point(workspace, triangle.vertices[0]),
        point(workspace, triangle.vertices[1]),
        point(workspace, triangle.vertices[2]), workspace.orientation);
    if (relation == source_candidate_disposition::uncertainty_rejected)
      return std::nullopt;
    if (relation == source_candidate_disposition::eligible) {
      if (containing)
        return std::nullopt;
      containing = triangle.canonical_id;
    }
  }
  return containing;
}

template <class T, class I>
bool generate_witnesses(source_triangle_complex<T, I> &artifact,
                        const facet_workspace<T, I> &workspace,
                        source_facet_triangulation_record &facet,
                        bool verifier_order,
                        std::uint64_t maximum_witnesses) {
  std::vector<std::array<std::uint64_t, 3>> triples;
  const auto &ring = facet.source_vertices;
  for (std::size_t i = 0; i + 2 < ring.size(); ++i)
    for (std::size_t j = i + 1; j + 1 < ring.size(); ++j)
      for (std::size_t k = j + 1; k < ring.size(); ++k)
        triples.push_back({ring[i], ring[j], ring[k]});
  if (verifier_order)
    std::reverse(triples.begin(), triples.end());
  auto &output = verifier_order ? facet.verifier_witnesses
                                : facet.producer_witnesses;
  std::uint64_t ordinal = 0;
  for (const auto &triple : triples) {
    const auto orientation = bounded_source_polygon_kernel<T>::orientation(
        point(workspace, triple[0]), point(workspace, triple[1]),
        point(workspace, triple[2]));
    if (orientation.bounded_sign == bounded_planar_sign::uncertain)
      continue;
    auto sample = triangle_sample(point(workspace, triple[0]),
                                  point(workspace, triple[1]),
                                  point(workspace, triple[2]),
                                  std::numeric_limits<std::uint64_t>::max() - ordinal);
    if (!sample)
      continue;
    auto containing = containing_triangle(artifact, workspace, *sample);
    if (!containing)
      continue;
    output.push_back({workspace.facet, ordinal++, *containing, triple});
    if (output.size() >= maximum_witnesses)
      break;
  }
  return !ring.empty() && !output.empty();
}

template <class T, class I>
bool triangles_geometrically_compatible(
    const source_triangle_record<T> &a,
    const source_triangle_record<T> &b,
    const facet_workspace<T, I> &workspace) {
  std::size_t shared_vertices = 0;
  for (auto av : a.vertices)
    shared_vertices += static_cast<std::size_t>(
        std::find(b.vertices.begin(), b.vertices.end(), av) != b.vertices.end());
  if (shared_vertices == 3)
    return false;
  for (std::size_t ai = 0; ai < 3; ++ai) {
    const auto a0 = a.vertices[ai];
    const auto a1 = a.vertices[(ai + 1) % 3];
    for (std::size_t bi = 0; bi < 3; ++bi) {
      const auto b0 = b.vertices[bi];
      const auto b1 = b.vertices[(bi + 1) % 3];
      const bool same_undirected = ordered_endpoints(a0, a1) ==
                                   ordered_endpoints(b0, b1);
      if (same_undirected) {
        if (a0 != b1 || a1 != b0)
          return false;
        continue;
      }
      const bool shares_endpoint = a0 == b0 || a0 == b1 ||
                                   a1 == b0 || a1 == b1;
      const auto relation = bounded_source_polygon_kernel<T>::segment_relation(
          point(workspace, a0), point(workspace, a1), point(workspace, b0),
          point(workspace, b1));
      if (relation == bounded_segment_relation::uncertain ||
          relation == bounded_segment_relation::proper_crossing ||
          relation == bounded_segment_relation::collinear_overlap)
        return false;
      if (!shares_endpoint &&
          relation != bounded_segment_relation::definitely_disjoint)
        return false;
    }
  }
  for (auto vertex : a.vertices) {
    if (std::find(b.vertices.begin(), b.vertices.end(), vertex) !=
        b.vertices.end())
      continue;
    if (bounded_source_polygon_kernel<T>::point_in_or_on_triangle(
            point(workspace, vertex), point(workspace, b.vertices[0]),
            point(workspace, b.vertices[1]), point(workspace, b.vertices[2]),
            workspace.orientation) !=
        source_candidate_disposition::definitely_rejected)
      return false;
  }
  for (auto vertex : b.vertices) {
    if (std::find(a.vertices.begin(), a.vertices.end(), vertex) !=
        a.vertices.end())
      continue;
    if (bounded_source_polygon_kernel<T>::point_in_or_on_triangle(
            point(workspace, vertex), point(workspace, a.vertices[0]),
            point(workspace, a.vertices[1]), point(workspace, a.vertices[2]),
            workspace.orientation) !=
        source_candidate_disposition::definitely_rejected)
      return false;
  }
  return true;
}

template <class T, class I>
bool independent_topology_verification(
    const source_triangle_complex<T, I> &artifact,
    const validated_operand<T, I> &operand,
    const precision_context<T> &precision,
    bool include_witnesses,
    std::uint32_t *failure_code = nullptr) {
  const auto reject = [failure_code](std::uint32_t code) noexcept {
    if (failure_code)
      *failure_code = code;
    return false;
  };
  if (!artifact.owner().same_owner(operand.owner()) ||
      !precision.owned_by(artifact.owner()) ||
      artifact.predecessor_digest() != operand.digest() ||
      artifact.precision_digest() != precision.digest() ||
      artifact.schema_version() != contract_versions::source_triangle_complex ||
      artifact.provider_version() != contract_versions::source_triangulation_provider ||
      artifact.policy_version() != contract_versions::source_triangulation_policy ||
      artifact.codec_version() != contract_versions::source_triangle_complex_codec ||
      artifact.verifier_version() != contract_versions::source_triangle_complex_verifier)
    return reject(4100);
  if (artifact.facets().size() != operand.facets().size() ||
      artifact.vertices().size() != operand.vertices().size() ||
      artifact.source_vertex_to_triangles().size() != operand.vertices().size() ||
      artifact.source_directed_use_to_edge_use().size() !=
          operand.directed_uses().size())
    return reject(4101);
  for (std::uint64_t vertex = 0; vertex < artifact.vertices().size(); ++vertex) {
    const auto &record = artifact.vertices()[vertex];
    const auto &source = operand.vertices()[vertex];
    const auto &bounded = operand.bounded_vertices()[vertex];
    if (record.source_vertex != vertex || record.nominal_bits != source.coordinate_bits ||
        record.lower != bounded.lower || record.upper != bounded.upper ||
        record.radial_error != bounded.radial_error ||
        record.presentation_vertex != source.presentation_vertex ||
        record.incident_triangles != artifact.source_vertex_to_triangles()[vertex])
      return reject(4102);
    for (auto triangle : record.incident_triangles)
      if (triangle >= artifact.triangles().size() ||
          std::find(artifact.triangles()[triangle].vertices.begin(),
                    artifact.triangles()[triangle].vertices.end(), vertex) ==
              artifact.triangles()[triangle].vertices.end())
        return reject(4103);
  }
  std::vector<std::uint64_t> boundary_seen(operand.directed_uses().size(), 0);
  std::map<std::pair<std::uint64_t, std::array<std::uint64_t, 2>>,
           std::vector<std::uint64_t>>
      diagonals;
  for (std::uint64_t triangle_id = 0; triangle_id < artifact.triangles().size();
       ++triangle_id) {
    const auto &triangle = artifact.triangles()[triangle_id];
    if (triangle.canonical_id != triangle_id || triangle.key.facet >= operand.facets().size() ||
        triangle.vertices != triangle.key.vertices)
      return reject(4104);
    auto workspace_out = materialize_facet_geometry_basis(
        operand, precision, triangle.key.facet);
    if (!workspace_out.has_value())
      return reject(4105);
    const auto &workspace = *workspace_out.value();
    const auto orientation = bounded_source_polygon_kernel<T>::orientation(
        point(workspace, triangle.vertices[0]),
        point(workspace, triangle.vertices[1]),
        point(workspace, triangle.vertices[2]));
    if (orientation.bounded_sign != workspace.orientation)
      return reject(4106);
    for (std::size_t slot = 0; slot < 3; ++slot) {
      if (triangle.edge_uses[slot] >= artifact.edge_uses().size())
        return reject(4107);
      const auto &edge = artifact.edge_uses()[triangle.edge_uses[slot]];
      const auto origin = triangle.vertices[slot];
      const auto destination = triangle.vertices[(slot + 1) % 3];
      if (edge.triangle != triangle_id || edge.local_slot != slot ||
          edge.origin != origin || edge.destination != destination ||
          edge.facet != triangle.key.facet)
        return reject(4108);
      std::optional<std::uint64_t> source_use;
      for (std::uint64_t use = 0; use < operand.directed_uses().size(); ++use) {
        const auto &candidate = operand.directed_uses()[use];
        if (candidate.facet == edge.facet && candidate.origin == origin &&
            candidate.destination == destination) {
          if (source_use)
            return reject(4109);
          source_use = use;
        }
      }
      if (source_use) {
        if (edge.role != source_triangle_edge_role::source_boundary ||
            edge.source_directed_use != *source_use ||
            !edge.source_feature_eligible() ||
            !edge.source_edge_candidate_eligible())
          return reject(4110);
        ++boundary_seen[*source_use];
      } else {
        if (edge.role != source_triangle_edge_role::facet_internal_diagonal ||
            edge.source_feature_eligible() ||
            edge.source_edge_candidate_eligible() || edge.symbolic_owner_eligible() ||
            edge.classification_barrier())
          return reject(4111);
        diagonals[{edge.facet, ordered_endpoints(origin, destination)}].push_back(
            triangle.edge_uses[slot]);
      }
    }
  }
  for (auto count : boundary_seen)
    if (count != 1)
      return reject(4112);
  if (diagonals.size() != artifact.diagonals().size())
    return reject(4113);
  for (const auto &entry : diagonals) {
    if (entry.second.size() != 2)
      return reject(4114);
    const auto &a = artifact.edge_uses()[entry.second[0]];
    const auto &b = artifact.edge_uses()[entry.second[1]];
    if (a.origin != b.destination || a.destination != b.origin ||
        a.triangle == b.triangle || a.diagonal != b.diagonal ||
        a.opposite_edge_use != entry.second[1] ||
        b.opposite_edge_use != entry.second[0])
      return reject(4115);
  }
  if (artifact.source_edge_feature_range().size() != operand.directed_uses().size() ||
      artifact.facet_internal_diagonal_range().size() != artifact.diagonals().size())
    return reject(4116);
  std::set<std::uint64_t> boundary_range(artifact.source_edge_feature_range().begin(),
                                         artifact.source_edge_feature_range().end());
  if (boundary_range.size() != artifact.source_edge_feature_range().size())
    return reject(4117);
  for (auto use : boundary_range)
    if (use >= artifact.edge_uses().size() ||
        artifact.edge_uses()[use].role != source_triangle_edge_role::source_boundary)
      return reject(4118);
  for (std::uint64_t diagonal_id = 0; diagonal_id < artifact.diagonals().size();
       ++diagonal_id) {
    if (artifact.facet_internal_diagonal_range()[diagonal_id] != diagonal_id)
      return reject(4119);
    const auto &record = artifact.diagonals()[diagonal_id];
    if (record.canonical_id != diagonal_id || record.key.facet >= operand.facets().size() ||
        record.source_feature_eligible || record.source_edge_candidate_eligible ||
        record.symbolic_owner_eligible || record.classification_barrier ||
        record.edge_uses[0] >= artifact.edge_uses().size() ||
        record.edge_uses[1] >= artifact.edge_uses().size() ||
        record.triangle_uses[0] >= artifact.triangles().size() ||
        record.triangle_uses[1] >= artifact.triangles().size())
      return reject(4120);
    const auto &first = artifact.edge_uses()[record.edge_uses[0]];
    const auto &second = artifact.edge_uses()[record.edge_uses[1]];
    if (first.diagonal != diagonal_id || second.diagonal != diagonal_id ||
        ordered_endpoints(first.origin, first.destination) != record.key.endpoints ||
        record.triangle_uses[0] != first.triangle ||
        record.triangle_uses[1] != second.triangle)
      return reject(4121);
  }
  for (const auto &facet : artifact.facets()) {
    if (facet.facet >= operand.facets().size() ||
        facet.source_vertices != operand.facets()[facet.facet].vertices ||
        facet.triangles.size() + 2 != facet.source_vertices.size() ||
        facet.diagonals.size() + 3 != facet.source_vertices.size())
      return reject(4122);
    std::set<std::uint64_t> incident;
    std::map<std::uint64_t, std::vector<std::uint64_t>> adjacency;
    for (auto triangle : facet.triangles) {
      if (triangle >= artifact.triangles().size() ||
          artifact.triangles()[triangle].key.facet != facet.facet)
        return reject(4123);
      incident.insert(triangle);
    }
    auto workspace_out =
        materialize_facet_geometry_basis(operand, precision, facet.facet);
    if (!workspace_out.has_value())
      return reject(4124);
    const auto &workspace = *workspace_out.value();
    auto accumulated_area = finite_interval<T>::checked_singleton(T(0));
    if (!accumulated_area)
      return reject(4125);
    for (std::size_t i = 0; i < facet.triangles.size(); ++i) {
      const auto &first = artifact.triangles()[facet.triangles[i]];
      auto next_area = interval_add(*accumulated_area, first.orientation.determinant);
      if (!next_area)
        return reject(4126);
      accumulated_area = next_area.value;
      for (std::size_t j = i + 1; j < facet.triangles.size(); ++j)
        if (!triangles_geometrically_compatible(
                first, artifact.triangles()[facet.triangles[j]], workspace))
          return reject(4127);
    }
    if (finite_numeric_less(accumulated_area->upper(), workspace.area.determinant.lower()) ||
        finite_numeric_less(workspace.area.determinant.upper(),
                            accumulated_area->lower()))
      return reject(4128);
    for (auto diagonal : facet.diagonals) {
      if (diagonal >= artifact.diagonals().size())
        return reject(4129);
      const auto &record = artifact.diagonals()[diagonal];
      adjacency[record.triangle_uses[0]].push_back(record.triangle_uses[1]);
      adjacency[record.triangle_uses[1]].push_back(record.triangle_uses[0]);
    }
    if (!incident.empty()) {
      std::queue<std::uint64_t> queue;
      std::set<std::uint64_t> visited;
      queue.push(*incident.begin());
      visited.insert(*incident.begin());
      while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();
        for (auto next : adjacency[current])
          if (visited.insert(next).second)
            queue.push(next);
      }
      if (visited != incident)
        return reject(4130);
    }
    if (include_witnesses &&
        (facet.producer_witnesses.empty() || facet.verifier_witnesses.empty()))
      return reject(4131);
  }
  if (encode_source_triangle_complex_semantic(artifact) != artifact.semantic_bytes() ||
      encode_source_triangle_complex_exact(artifact) != artifact.exact_bytes() ||
      sha256::digest(artifact.semantic_bytes()) != artifact.source_semantic_digest() ||
      sha256::digest(artifact.exact_bytes()) != artifact.exact_triangulation_digest())
    return reject(4132);
  canonical_writer canonical;
  canonical.u32(0x34414759U);
  write_digest(canonical, artifact.source_semantic_digest());
  write_digest(canonical, artifact.exact_triangulation_digest());
  write_digest(canonical, artifact.replay_presentation_digest());
  if (canonical.bytes() != artifact.canonical_bytes() ||
      sha256::digest(artifact.canonical_bytes()) != artifact.digest())
    return reject(4133);
  return true;
}

} // namespace source_triangulation_detail

template <class T, class I> class source_triangulation_builder final {
  using artifact_type = source_triangle_complex<T, I>;
  std::shared_ptr<const validated_operand<T, I>> operand_;
  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  source_triangulation_capabilities capabilities_;
  struct typed_reservation final {
    resource_kind kind;
    resource_reservation reservation;
  };
  artifact_type out_;
  std::vector<typed_reservation> reservations_;

  auto failure(source_triangulation_subcode subcode,
               bounded_boolean_error_category category, const char *summary,
               source_triangulation_checkpoint checkpoint) const {
    auto error = source_triangulation_error(operand_->operand(), subcode, category,
                                            summary, checkpoint);
    error.context_digest = precision_.digest();
    error.replay_digest = context_.replay_digest;
    return boolean_outcome<std::shared_ptr<const artifact_type>>::failure(error);
  }

  bool reserve(resource_kind kind, std::uint64_t amount) {
    if (!capabilities_.resources)
      return false;
    auto reservation = capabilities_.resources->reserve(kind, amount);
    if (!reservation)
      return false;
    reservations_.push_back({kind, std::move(*reservation)});
    return true;
  }

  bool actual_usage(resource_kind kind, std::uint64_t &usage) const {
    switch (kind) {
    case resource_kind::source_triangles:
      usage = out_.triangles_.size();
      return true;
    case resource_kind::source_edges:
      usage = out_.diagonals_.size();
      return true;
    case resource_kind::source_indices:
      usage = out_.edge_uses_.size();
      return true;
    case resource_kind::work_units:
      return checked_add(out_.statistics_.work_units,
                         out_.statistics_.verifier_work, usage);
    case resource_kind::temporary_bytes:
    case resource_kind::verification_findings:
      usage = 0;
      return true;
    case resource_kind::persistent_bytes:
      usage = out_.statistics_.persistent_bytes;
      return true;
    case resource_kind::canonical_sort_records:
      if (!checked_add(static_cast<std::uint64_t>(out_.triangles_.size()),
                       static_cast<std::uint64_t>(out_.diagonals_.size()), usage))
        return false;
      return checked_add(usage,
                         static_cast<std::uint64_t>(out_.edge_uses_.size()),
                         usage);
    default:
      return false;
    }
  }

  bool preflight() {
    std::uint64_t triangles = 0, diagonals = 0, edge_uses = 0, work = 0,
                  temporary = 0, persistent = 0;
    for (const auto &facet : operand_->facets()) {
      const auto n = static_cast<std::uint64_t>(facet.vertices.size());
      if (n < 3 || n > capabilities_.maximum_ring_size)
        return false;
      std::uint64_t local_triangles = n - 2, local_diagonals = n - 3,
                    local_edges = 0, local_work = 0;
      if (!checked_multiply(local_triangles, std::uint64_t{3}, local_edges) ||
          !checked_multiply(n, n, local_work) ||
          !checked_multiply(local_work, n, local_work) ||
          !checked_add(triangles, local_triangles, triangles) ||
          !checked_add(diagonals, local_diagonals, diagonals) ||
          !checked_add(edge_uses, local_edges, edge_uses) ||
          !checked_add(work, local_work, work))
        return false;
    }
    std::uint64_t records = 0, findings = 0, temporary_bound = 0,
                  persistent_bound = 0;
    if (!checked_add(triangles, diagonals, records) ||
        !checked_add(records, edge_uses, records) ||
        !checked_multiply(work, std::uint64_t{8}, temporary) ||
        !checked_multiply(records, std::uint64_t{512}, persistent) ||
        !checked_add(temporary, std::uint64_t{4096}, temporary_bound) ||
        !checked_add(persistent, std::uint64_t{4096}, persistent_bound) ||
        !checked_add(triangles, diagonals, findings) ||
        !checked_add(findings,
                     static_cast<std::uint64_t>(operand_->facets().size()),
                     findings) ||
        work > capabilities_.maximum_work_units)
      return false;
    return reserve(resource_kind::source_triangles, triangles) &&
           reserve(resource_kind::source_edges, diagonals) &&
           reserve(resource_kind::source_indices, edge_uses) &&
           reserve(resource_kind::work_units, work) &&
           reserve(resource_kind::temporary_bytes, temporary_bound) &&
           reserve(resource_kind::persistent_bytes, persistent_bound) &&
           reserve(resource_kind::verification_findings, findings) &&
           reserve(resource_kind::canonical_sort_records, records);
  }

  bool assemble(const std::vector<source_triangulation_detail::facet_workspace<T, I>> &workspaces,
                const std::vector<source_triangulation_detail::facet_triangulation_proposal> &proposals) {
    out_.vertices_.resize(operand_->vertices().size());
    out_.source_vertex_to_triangles_.resize(operand_->vertices().size());
    for (std::uint64_t vertex = 0; vertex < operand_->vertices().size(); ++vertex) {
      const auto &source = operand_->vertices()[vertex];
      const auto &bounded = operand_->bounded_vertices()[vertex];
      auto &target = out_.vertices_[vertex];
      target.source_vertex = vertex;
      target.nominal_bits = source.coordinate_bits;
      target.lower = bounded.lower;
      target.upper = bounded.upper;
      target.radial_error = bounded.radial_error;
      target.presentation_vertex = source.presentation_vertex;
    }
    std::vector<source_triangulation_detail::triangle_proposal> triangles;
    for (const auto &proposal : proposals) {
      triangles.insert(triangles.end(), proposal.triangles.begin(),
                       proposal.triangles.end());
      out_.trace_.insert(out_.trace_.end(), proposal.trace.begin(), proposal.trace.end());
    }
    struct keyed_triangle {
      source_triangle_key key;
      source_triangulation_detail::triangle_proposal proposal;
    };
    std::vector<keyed_triangle> keyed;
    for (const auto &proposal : triangles) {
      source_triangle_key key;
      key.operand = operand_->operand();
      key.facet = proposal.facet;
      key.vertices = minimum_orientation_preserving_rotation(proposal.vertices);
      keyed.push_back({key, proposal});
    }
    std::sort(keyed.begin(), keyed.end(),
              [](const auto &a, const auto &b) { return a.key < b.key; });
    for (std::size_t i = 1; i < keyed.size(); ++i)
      if (keyed[i - 1].key == keyed[i].key)
        return false;
    out_.triangles_.resize(keyed.size());
    for (std::uint64_t id = 0; id < keyed.size(); ++id) {
      auto &record = out_.triangles_[id];
      record.canonical_id = id;
      record.key = keyed[id].key;
      record.vertices = keyed[id].key.vertices;
      record.ring = keyed[id].proposal.facet;
      record.shell = keyed[id].proposal.shell;
      record.basis = workspaces[record.key.facet].basis;
      record.orientation = bounded_source_polygon_kernel<T>::orientation(
          source_triangulation_detail::point(workspaces[record.key.facet], record.vertices[0]),
          source_triangulation_detail::point(workspaces[record.key.facet], record.vertices[1]),
          source_triangulation_detail::point(workspaces[record.key.facet], record.vertices[2]));
      if (record.orientation.bounded_sign != workspaces[record.key.facet].orientation)
        return false;
      for (auto vertex : record.vertices) {
        if (vertex >= out_.source_vertex_to_triangles_.size())
          return false;
        out_.source_vertex_to_triangles_[vertex].push_back(id);
      }
    }
    out_.edge_uses_.reserve(out_.triangles_.size() * 3);
    std::map<std::pair<std::uint64_t, std::array<std::uint64_t, 2>>,
             std::vector<std::uint64_t>>
        internal_groups;
    out_.source_directed_use_to_edge_use_.assign(
        operand_->directed_uses().size(), std::numeric_limits<std::uint64_t>::max());
    for (auto &triangle : out_.triangles_) {
      const auto &workspace = workspaces[triangle.key.facet];
      for (std::size_t slot = 0; slot < 3; ++slot) {
        source_triangle_edge_use edge;
        edge.origin = triangle.vertices[slot];
        edge.destination = triangle.vertices[(slot + 1) % 3];
        edge.triangle = triangle.canonical_id;
        edge.local_slot = static_cast<std::uint8_t>(slot);
        edge.facet = triangle.key.facet;
        edge.ring = triangle.ring;
        edge.shell = triangle.shell;
        const auto boundary = workspace.directed_boundary.find(
            {edge.origin, edge.destination});
        if (boundary != workspace.directed_boundary.end()) {
          edge.role = source_triangle_edge_role::source_boundary;
          edge.source_directed_use = boundary->second;
          const auto &source_use = operand_->directed_uses()[boundary->second];
          edge.source_undirected_edge = source_use.undirected_edge;
          edge.source_corner = source_use.corner;
          edge.reciprocal_source_use = source_use.reciprocal;
          if (boundary->second >= out_.source_directed_use_to_edge_use_.size() ||
              out_.source_directed_use_to_edge_use_[boundary->second] !=
                  std::numeric_limits<std::uint64_t>::max())
            return false;
          out_.source_directed_use_to_edge_use_[boundary->second] =
              out_.edge_uses_.size();
          out_.source_edge_feature_range_.push_back(out_.edge_uses_.size());
        } else {
          edge.role = source_triangle_edge_role::facet_internal_diagonal;
          internal_groups[{triangle.key.facet,
                           ordered_endpoints(edge.origin, edge.destination)}]
              .push_back(out_.edge_uses_.size());
        }
        triangle.edge_uses[slot] = out_.edge_uses_.size();
        out_.edge_uses_.push_back(edge);
      }
    }
    for (auto use : out_.source_directed_use_to_edge_use_)
      if (use == std::numeric_limits<std::uint64_t>::max())
        return false;
    std::vector<std::pair<source_diagonal_key, std::vector<std::uint64_t>>> diagonals;
    for (auto &entry : internal_groups) {
      if (entry.second.size() != 2)
        return false;
      auto &a = out_.edge_uses_[entry.second[0]];
      auto &b = out_.edge_uses_[entry.second[1]];
      if (a.origin != b.destination || a.destination != b.origin ||
          a.triangle == b.triangle)
        return false;
      source_diagonal_key key;
      key.operand = operand_->operand();
      key.facet = entry.first.first;
      key.endpoints = entry.first.second;
      diagonals.push_back({key, entry.second});
    }
    std::sort(diagonals.begin(), diagonals.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    out_.diagonals_.resize(diagonals.size());
    for (std::uint64_t id = 0; id < diagonals.size(); ++id) {
      auto &record = out_.diagonals_[id];
      record.canonical_id = id;
      record.key = diagonals[id].first;
      record.ring = record.key.facet;
      record.shell = operand_->facets()[record.key.facet].shell;
      record.edge_uses = {diagonals[id].second[0], diagonals[id].second[1]};
      record.triangle_uses = {out_.edge_uses_[record.edge_uses[0]].triangle,
                              out_.edge_uses_[record.edge_uses[1]].triangle};
      out_.edge_uses_[record.edge_uses[0]].diagonal = id;
      out_.edge_uses_[record.edge_uses[1]].diagonal = id;
      out_.edge_uses_[record.edge_uses[0]].opposite_edge_use = record.edge_uses[1];
      out_.edge_uses_[record.edge_uses[1]].opposite_edge_use = record.edge_uses[0];
      out_.facet_internal_diagonal_range_.push_back(id);
    }
    out_.facets_.resize(operand_->facets().size());
    for (std::uint64_t facet_id = 0; facet_id < out_.facets_.size(); ++facet_id) {
      auto &facet = out_.facets_[facet_id];
      facet.facet = facet_id;
      facet.ring = facet_id;
      facet.shell = operand_->facets()[facet_id].shell;
      facet.source_vertices = operand_->facets()[facet_id].vertices;
    }
    for (const auto &triangle : out_.triangles_)
      out_.facets_[triangle.key.facet].triangles.push_back(triangle.canonical_id);
    for (const auto &diagonal : out_.diagonals_)
      out_.facets_[diagonal.key.facet].diagonals.push_back(diagonal.canonical_id);
    for (std::uint64_t vertex = 0; vertex < out_.vertices_.size(); ++vertex) {
      auto &incidence = out_.source_vertex_to_triangles_[vertex];
      std::sort(incidence.begin(), incidence.end());
      incidence.erase(std::unique(incidence.begin(), incidence.end()), incidence.end());
      out_.vertices_[vertex].incident_triangles = incidence;
      if (!incidence.empty())
        out_.vertices_[vertex].shell = out_.triangles_[incidence.front()].shell;
    }
    return true;
  }

  bool finalize_encoding() {
    out_.semantic_bytes_ =
        source_triangulation_detail::encode_source_triangle_complex_semantic(out_);
    out_.exact_bytes_ =
        source_triangulation_detail::encode_source_triangle_complex_exact(out_);
    out_.source_semantic_digest_ = sha256::digest(out_.semantic_bytes_);
    out_.exact_triangulation_digest_ = sha256::digest(out_.exact_bytes_);
    canonical_writer replay;
    source_triangulation_detail::write_digest(
        replay, operand_->source_presentation_digest());
    replay.u16(contract_versions::source_triangle_complex_codec);
    out_.replay_presentation_digest_ = sha256::digest(replay.bytes());
    canonical_writer canonical;
    canonical.u32(0x34414759U);
    source_triangulation_detail::write_digest(canonical,
                                               out_.source_semantic_digest_);
    source_triangulation_detail::write_digest(canonical,
                                               out_.exact_triangulation_digest_);
    source_triangulation_detail::write_digest(canonical,
                                               out_.replay_presentation_digest_);
    out_.canonical_bytes_ = canonical.take();
    out_.digest_ = sha256::digest(out_.canonical_bytes_);
    return true;
  }

  class publication final : public transaction_participant {
  public:
    publication(std::shared_ptr<const artifact_type> candidate,
                const validated_operand<T, I> &operand,
                const precision_context<T> &precision)
        : candidate_(std::move(candidate)), operand_(operand),
          precision_(precision) {}
    bool prepare() override {
      return candidate_ &&
             source_triangulation_detail::independent_topology_verification(
                 *candidate_, operand_, precision_, true);
    }
    void commit() noexcept override { published_ = candidate_; }
    void rollback() noexcept override {
      published_.reset();
      candidate_.reset();
    }
    std::shared_ptr<const artifact_type> published_;

  private:
    std::shared_ptr<const artifact_type> candidate_;
    const validated_operand<T, I> &operand_;
    const precision_context<T> &precision_;
  };

public:
  source_triangulation_builder(
      std::shared_ptr<const validated_operand<T, I>> operand,
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      source_triangulation_capabilities capabilities)
      : operand_(std::move(operand)), context_(context), precision_(precision),
        capabilities_(std::move(capabilities)) {}

  boolean_outcome<std::shared_ptr<const artifact_type>> run(
      const std::vector<std::vector<std::array<std::uint64_t, 3>>> *alternatives =
          nullptr) {
    if (!operand_ || capabilities_.version !=
                         contract_versions::source_triangulation_provider ||
        capabilities_.reserved != 0 || !capabilities_.owner.anchor ||
        !capabilities_.owner.same_owner(context_.owner) ||
        !operand_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner) || !capabilities_.resources)
      return failure(source_triangulation_subcode::wrong_owner,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source triangulation capability owner mismatch",
                     source_triangulation_checkpoint::contract_validation);
    if (operand_->digest() == bounded_boolean_digest{} ||
        operand_->precision_digest() != precision_.digest() ||
        operand_->context_digest() != context_.context_digest)
      return failure(source_triangulation_subcode::predecessor_digest_mismatch,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source triangulation predecessor digest mismatch",
                     source_triangulation_checkpoint::contract_validation);
    if (source_triangulation_detail::cancelled(capabilities_))
      return failure(source_triangulation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "source triangulation cancelled",
                     source_triangulation_checkpoint::contract_validation);
    if (!preflight())
      return failure(source_triangulation_subcode::resource_preflight,
                     bounded_boolean_error_category::resource_limit,
                     "source triangulation preflight failed",
                     source_triangulation_checkpoint::preflight);
    out_.operand_ = operand_->operand();
    out_.owner_ = context_.owner;
    out_.predecessor_digest_ = operand_->digest();
    out_.precision_digest_ = precision_.digest();
    out_.predecessor_ = operand_;
    std::vector<source_triangulation_detail::facet_workspace<T, I>> workspaces;
    std::vector<source_triangulation_detail::facet_triangulation_proposal> proposals;
    workspaces.reserve(operand_->facets().size());
    proposals.reserve(operand_->facets().size());
    for (std::uint64_t facet = 0; facet < operand_->facets().size(); ++facet) {
      auto workspace = source_triangulation_detail::materialize_facet_geometry_basis(
          *operand_, precision_, facet);
      if (!workspace.has_value())
        return boolean_outcome<std::shared_ptr<const artifact_type>>::failure(
            *workspace.error());
      out_.statistics_.projected_points += workspace.value()->points.size();
      workspaces.push_back(std::move(*workspace.value()));
      source_triangulation_detail::facet_triangulation_proposal proposal;
      if (alternatives) {
        if (facet >= alternatives->size())
          return failure(source_triangulation_subcode::count_mismatch,
                         bounded_boolean_error_category::input_contract_error,
                         "alternative triangulation facet range mismatch",
                         source_triangulation_checkpoint::ear_loop);
        proposal.facet = facet;
        proposal.shell = operand_->facets()[facet].shell;
        for (const auto &triangle : (*alternatives)[facet])
           proposal.triangles.push_back({facet, proposal.shell, triangle});
      } else if (capabilities_.provider ==
                 source_triangulation_provider_kind::full_rescan_reference_v1) {
        auto result = source_triangulation_detail::triangulate_facet_reference(
            workspaces.back(), capabilities_, out_.statistics_);
        if (!result.has_value())
          return boolean_outcome<std::shared_ptr<const artifact_type>>::failure(
              *result.error());
        proposal = std::move(*result.value());
      } else if (capabilities_.provider ==
                 source_triangulation_provider_kind::indexed_dependency_v1) {
        auto result = source_triangulation_detail::triangulate_facet_indexed(
            workspaces.back(), capabilities_, out_.statistics_);
        if (!result.has_value())
          return boolean_outcome<std::shared_ptr<const artifact_type>>::failure(
              *result.error());
        proposal = std::move(*result.value());
        if (capabilities_.compare_with_reference) {
          source_triangulation_statistics reference_statistics;
          auto reference = source_triangulation_detail::triangulate_facet_reference(
              workspaces.back(), capabilities_, reference_statistics);
          if (!reference.has_value() ||
              !source_triangulation_detail::same_reference_trace<T, I>(
                  proposal, *reference.value()))
            return failure(source_triangulation_subcode::provider_trace_mismatch,
                           bounded_boolean_error_category::internal_invariant_error,
                           "indexed source triangulation diverged from reference",
                           source_triangulation_checkpoint::ear_loop);
        }
      } else {
        return failure(source_triangulation_subcode::unsupported_policy,
                       bounded_boolean_error_category::input_contract_error,
                       "unsupported source triangulation provider",
                       source_triangulation_checkpoint::contract_validation);
      }
      proposals.push_back(std::move(proposal));
    }
    if (!assemble(workspaces, proposals))
      return failure(source_triangulation_subcode::provenance_incomplete,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source triangle provenance reconstruction failed",
                     source_triangulation_checkpoint::provenance);
    out_.statistics_.triangles = out_.triangles_.size();
    out_.statistics_.diagonals = out_.diagonals_.size();
    for (std::uint64_t facet_id = 0; facet_id < out_.facets_.size(); ++facet_id) {
      auto &facet = out_.facets_[facet_id];
      if (facet.triangles.size() + 2 != facet.source_vertices.size() ||
          facet.diagonals.size() + 3 != facet.source_vertices.size())
        return failure(source_triangulation_subcode::count_mismatch,
                       bounded_boolean_error_category::internal_invariant_error,
                       "source triangulation combinatorial counts failed",
                       source_triangulation_checkpoint::combinatorial_coverage);
      if (!source_triangulation_detail::generate_witnesses(
              out_, workspaces[facet_id], facet, false,
              capabilities_.maximum_witnesses_per_facet) ||
          !source_triangulation_detail::generate_witnesses(
              out_, workspaces[facet_id], facet, true,
              capabilities_.maximum_witnesses_per_facet))
        return failure(source_triangulation_subcode::witness_coverage,
                       bounded_boolean_error_category::geometric_condition_exceeds_tolerance,
                       "source triangulation witness coverage failed",
                       source_triangulation_checkpoint::geometric_coverage);
      out_.statistics_.witness_audits += facet.producer_witnesses.size() +
                                         facet.verifier_witnesses.size();
    }
    finalize_encoding();
    for (auto &facet : out_.facets_) {
      canonical_writer semantic;
      semantic.u64(facet.facet);
      semantic.u64(facet.source_vertices.size());
      for (auto vertex : facet.source_vertices)
        semantic.u64(vertex);
      facet.semantic_digest = sha256::digest(semantic.bytes());
      canonical_writer exact;
      exact.u64(facet.facet);
      for (auto triangle : facet.triangles)
        exact.u64(triangle);
      for (auto diagonal : facet.diagonals)
        exact.u64(diagonal);
      facet.exact_digest = sha256::digest(exact.bytes());
    }
    // Re-encode because facet digests are presentation-independent derived fields
    // and intentionally excluded from artifact identity.
    std::uint32_t verifier_finding = 0;
    if (!source_triangulation_detail::independent_topology_verification(
            out_, *operand_, precision_, true, &verifier_finding)) {
      auto error = source_triangulation_error(
          operand_->operand(), source_triangulation_subcode::verifier_rejection,
          bounded_boolean_error_category::internal_invariant_error,
          "independent source triangle complex verifier rejected artifact",
          source_triangulation_checkpoint::independent_verification);
      error.context_digest = precision_.digest();
      error.replay_digest = context_.replay_digest;
      error.witnesses[0] = verifier_finding;
      error.witness_count = 1;
      return boolean_outcome<std::shared_ptr<const artifact_type>>::failure(error);
    }
    std::uint64_t persistent_bytes = 0, record_bytes = 0;
    auto add_persistent = [&](std::uint64_t value) {
      return checked_add(persistent_bytes, value, persistent_bytes);
    };
    auto add_records = [&](std::uint64_t count, std::uint64_t size) {
      return checked_multiply(count, size, record_bytes) &&
             add_persistent(record_bytes);
    };
    if (!add_persistent(out_.semantic_bytes_.size()) ||
        !add_persistent(out_.exact_bytes_.size()) ||
        !add_persistent(out_.canonical_bytes_.size()) ||
        !add_records(out_.triangles_.size(), sizeof(source_triangle_record<T>)) ||
        !add_records(out_.edge_uses_.size(), sizeof(source_triangle_edge_use)) ||
        !add_records(out_.diagonals_.size(),
                     sizeof(source_internal_diagonal_record)))
      return failure(source_triangulation_subcode::count_overflow,
                     bounded_boolean_error_category::resource_limit,
                     "source triangulation persistent size overflow",
                     source_triangulation_checkpoint::commit);
    out_.statistics_.persistent_bytes = persistent_bytes;
    std::vector<std::uint64_t> usage;
    usage.reserve(reservations_.size());
    for (const auto &reservation : reservations_) {
      std::uint64_t actual = 0;
      if (!actual_usage(reservation.kind, actual) ||
          actual > reservation.reservation.amount())
        return failure(source_triangulation_subcode::resource_preflight,
                       bounded_boolean_error_category::resource_limit,
                       "source triangulation exceeded its reservation",
                       source_triangulation_checkpoint::commit);
      usage.push_back(actual);
    }
    auto candidate =
        std::make_shared<const artifact_type>(std::move(out_));
    publication participant(candidate, *operand_, precision_);
    stage_transaction transaction;
    if (!transaction.open() || !transaction.enlist(participant) ||
        !transaction.begin_join() || !transaction.begin_verify() ||
        !transaction.ready())
      return failure(source_triangulation_subcode::verifier_rejection,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source triangulation publication verifier rejected proposal",
                     source_triangulation_checkpoint::commit);
    if (source_triangulation_detail::cancelled(capabilities_)) {
      transaction.rollback();
      return failure(source_triangulation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "source triangulation cancelled before commit",
                     source_triangulation_checkpoint::commit);
    }
    for (std::size_t i = 0; i < reservations_.size(); ++i)
      if (!reservations_[i].reservation.commit(usage[i])) {
        transaction.rollback();
        return failure(source_triangulation_subcode::resource_preflight,
                       bounded_boolean_error_category::internal_invariant_error,
                       "source triangulation resource commit failed",
                       source_triangulation_checkpoint::commit);
      }
    if (!transaction.commit() || !participant.published_)
      return failure(source_triangulation_subcode::verifier_rejection,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source triangulation publication commit failed",
                     source_triangulation_checkpoint::commit);
    return boolean_outcome<std::shared_ptr<const artifact_type>>::success(
        std::move(participant.published_));
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const source_triangle_complex<T, I>>>
triangulate_source_operand(
    std::shared_ptr<const validated_operand<T, I>> operand,
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    source_triangulation_capabilities capabilities) {
  return source_triangulation_builder<T, I>(std::move(operand), context, precision,
                                            std::move(capabilities))
      .run();
}

template <class T, class I>
boolean_outcome<std::shared_ptr<const source_triangle_complex<T, I>>>
assemble_source_triangle_complex_from_triangles(
    std::shared_ptr<const validated_operand<T, I>> operand,
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    source_triangulation_capabilities capabilities,
    const std::vector<std::vector<std::array<std::uint64_t, 3>>> &triangles) {
  return source_triangulation_builder<T, I>(std::move(operand), context, precision,
                                            std::move(capabilities))
      .run(&triangles);
}

template <class T, class I>
bool verify_source_triangle_complex(const source_triangle_complex<T, I> &artifact,
                                    const validated_operand<T, I> &operand,
                                    const precision_context<T> &precision) {
  return source_triangulation_detail::independent_topology_verification(
      artifact, operand, precision, true);
}

} // namespace ygor::mesh_boolean::bounded
