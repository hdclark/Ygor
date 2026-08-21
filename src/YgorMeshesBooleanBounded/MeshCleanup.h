#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "CleanupCodec.h"
#include "CleanupVerifier.h"
#include "Context.h"
#include "FloatingBits.h"
#include "PolygonalOutputComplex.h"
#include "RetainedSurfaceComplex.h"
#include "TriangulatedOutputComplex.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>>
build_cleaned_triangle_manifold(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const retained_surface_complex<T, I>> retained,
    std::shared_ptr<const polygonal_output_complex<T, I>> polygonal,
    std::shared_ptr<const triangulated_output_complex<T, I>> triangulated,
    cleanup_capabilities capabilities, cleanup_codec_limits codec_limits = {});

#define YGOR_DECLARE_CLEANUP_BUILD(T, I)                                     \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const cleaned_triangle_manifold<T>>>                   \
  build_cleaned_triangle_manifold<T, I>(                                     \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const canonical_source_manifolds<T, I>>,               \
      std::shared_ptr<const canonical_intersection_complex<T, I>>,           \
      std::shared_ptr<const retained_surface_complex<T, I>>,                 \
      std::shared_ptr<const polygonal_output_complex<T, I>>,                 \
      std::shared_ptr<const triangulated_output_complex<T, I>>,              \
      cleanup_capabilities, cleanup_codec_limits)

YGOR_DECLARE_CLEANUP_BUILD(float, std::uint32_t);
YGOR_DECLARE_CLEANUP_BUILD(float, std::uint64_t);
YGOR_DECLARE_CLEANUP_BUILD(double, std::uint32_t);
YGOR_DECLARE_CLEANUP_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_CLEANUP_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
namespace ygor::mesh_boolean::bounded {

template <class T, class I> class cleanup_builder {
public:
  cleanup_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
      std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
      std::shared_ptr<const retained_surface_complex<T, I>> retained,
      std::shared_ptr<const polygonal_output_complex<T, I>> polygonal,
      std::shared_ptr<const triangulated_output_complex<T, I>> triangulated,
      cleanup_capabilities capabilities, cleanup_codec_limits codec_limits)
      : context_(context), precision_(precision),
        manifolds_(std::move(manifolds)), intersections_(std::move(intersections)),
        retained_(std::move(retained)), polygonal_(std::move(polygonal)),
        triangulated_(std::move(triangulated)),
        capabilities_(std::move(capabilities)), codec_limits_(codec_limits) {}

  boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return failure(error);
    if (cleanup_cancelled(capabilities_,
                          cleanup_checkpoint::predecessor_validation))
      return fail_cancelled();

    auto artifact = std::make_shared<cleaned_triangle_manifold<T>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->precision_digest_ = precision_.digest();
    artifact->polygonal_digest_ = polygonal_->digest();
    artifact->triangulated_digest_ = triangulated_->digest();

    if (!build_vertices(*artifact, error) ||
        !build_triangles(*artifact, error) ||
        !build_edges(*artifact, error) ||
        !build_links_components(*artifact, error) ||
        !finalize(*artifact, error))
      return failure(error);

    if (cleanup_cancelled(capabilities_, cleanup_checkpoint::commit))
      return fail_cancelled();
    return boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>>::
        success(std::move(artifact));
  }

private:
  using artifact_type = cleaned_triangle_manifold<T>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>>;

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  std::shared_ptr<const canonical_intersection_complex<T, I>> intersections_;
  std::shared_ptr<const retained_surface_complex<T, I>> retained_;
  std::shared_ptr<const polygonal_output_complex<T, I>> polygonal_;
  std::shared_ptr<const triangulated_output_complex<T, I>> triangulated_;
  cleanup_capabilities capabilities_;
  cleanup_codec_limits codec_limits_;

  outcome_type failure(const bounded_boolean_error &error) {
    return outcome_type::failure(error);
  }
  outcome_type fail_cancelled() {
    return outcome_type::failure(
        cleanup_error(cleanup_subcode::cancelled,
                      bounded_boolean_error_category::cancelled,
                      "cleanup cancelled", cleanup_checkpoint::commit));
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    const auto &a = *manifolds_->a();
    const auto &b = *manifolds_->b();
    if (!a.owner().same_owner(context_.owner) ||
        !b.owner().same_owner(context_.owner) ||
        !intersections_->owner().same_owner(context_.owner) ||
        !retained_->owner().same_owner(context_.owner) ||
        !polygonal_->owner().same_owner(context_.owner) ||
        !triangulated_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = cleanup_error(cleanup_subcode::wrong_owner,
                            bounded_boolean_error_category::internal_invariant_error,
                            "cleanup predecessor owner mismatch",
                            cleanup_checkpoint::predecessor_validation);
      return false;
    }
    if (intersections_->operation() != context_.operation ||
        retained_->operation() != context_.operation ||
        polygonal_->operation() != context_.operation ||
        triangulated_->operation() != context_.operation) {
      error = cleanup_error(cleanup_subcode::wrong_operation,
                            bounded_boolean_error_category::internal_invariant_error,
                            "cleanup predecessor operation mismatch",
                            cleanup_checkpoint::predecessor_validation);
      return false;
    }
    if (triangulated_->verification() !=
        triangulation_verification_disposition::independently_verified) {
      error = cleanup_error(cleanup_subcode::predecessor_not_verified,
                            bounded_boolean_error_category::internal_invariant_error,
                            "triangulated complex is not independently verified",
                            cleanup_checkpoint::predecessor_validation);
      return false;
    }
    if (capabilities_.provider_version != contract_versions::cleanup_provider ||
        capabilities_.codec_version != contract_versions::cleanup_codec ||
        capabilities_.verifier_version != contract_versions::cleanup_verifier) {
      error = cleanup_error(cleanup_subcode::unsupported_version,
                            bounded_boolean_error_category::input_contract_error,
                            "cleanup capability version mismatch",
                            cleanup_checkpoint::context_capability_validation);
      return false;
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Vertices: one cleaned vertex per Component 11 output occurrence.
  // -------------------------------------------------------------------------
  bool build_vertices(artifact_type &artifact, bounded_boolean_error &error) {
    const auto &occurrences = polygonal_->vertex_occurrences();
    artifact.vertex_by_occurrence_.resize(occurrences.size(),
                                          cleanup_invalid_ordinal);
    for (std::size_t o = 0; o < occurrences.size(); ++o) {
      const auto &occurrence = occurrences[o];
      if (occurrence.coordinate_reference == output_topology_invalid_ordinal ||
          occurrence.coordinate_reference >=
              polygonal_->coordinate_references().size()) {
        error = cleanup_error(cleanup_subcode::import_failure,
                              bounded_boolean_error_category::internal_invariant_error,
                              "output occurrence coordinate reference is invalid",
                              cleanup_checkpoint::mutable_complex_construction);
        return false;
      }
      const auto &coordinate =
          polygonal_->coordinate_references()[occurrence.coordinate_reference];

      cleaned_vertex_record vertex;
      vertex.canonical_id = artifact.vertices_.size();
      vertex.key.component11_occurrence = occurrence.canonical_id;
      vertex.component11_occurrence = occurrence.canonical_id;
      vertex.nominal_bits = coordinate.nominal_bits;
      vertex.lower_bits = coordinate.lower_bits;
      vertex.upper_bits = coordinate.upper_bits;
      vertex.radial_error_bits = coordinate.radial_error_bits;
      vertex.component = cleanup_invalid_ordinal;
      artifact.vertex_by_occurrence_[occurrence.canonical_id] = vertex.canonical_id;
      artifact.vertices_.push_back(std::move(vertex));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Triangles: one cleaned triangle per Component 12 triangle, corners remapped.
  // -------------------------------------------------------------------------
  bool build_triangles(artifact_type &artifact, bounded_boolean_error &error) {
    const auto &source = triangulated_->triangles();
    artifact.triangle_by_source_.resize(source.size(), cleanup_invalid_ordinal);
    for (std::size_t t = 0; t < source.size(); ++t) {
      const auto &source_triangle = source[t];
      cleaned_triangle_record triangle;
      triangle.canonical_id = artifact.triangles_.size();
      std::array<std::uint64_t, 3> corners{};
      for (std::size_t k = 0; k < 3; ++k) {
        const std::uint64_t occurrence = source_triangle.corners[k];
        if (occurrence >= artifact.vertex_by_occurrence_.size() ||
            artifact.vertex_by_occurrence_[occurrence] == cleanup_invalid_ordinal) {
          error = cleanup_error(cleanup_subcode::import_failure,
                                bounded_boolean_error_category::internal_invariant_error,
                                "triangle corner occurrence is out of range",
                                cleanup_checkpoint::mutable_complex_construction);
          return false;
        }
        corners[k] = artifact.vertex_by_occurrence_[occurrence];
      }
      triangle.key = cleaned_triangle_key::canonical(corners);
      triangle.corners = corners;
      triangle.category = source_triangle.category;
      if (triangle.category != triangle_geometric_category::definite_positive_area) {
        error = cleanup_error(cleanup_subcode::final_obligation_remains,
                              bounded_boolean_error_category::result_geometry_not_validated,
                              "cleanup-required triangle is unsupported in V1 cleanup",
                              cleanup_checkpoint::defect_scan);
        return false;
      }
      triangle.component = cleanup_invalid_ordinal;
      artifact.triangle_by_source_[source_triangle.canonical_id] =
          triangle.canonical_id;
      artifact.triangles_.push_back(std::move(triangle));
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Edges and halfedges reconstructed from triangle corner connectivity.
  // -------------------------------------------------------------------------
  bool build_edges(artifact_type &artifact, bounded_boolean_error &error) {
    // Group directed corner uses by unordered endpoint pair.
    std::map<std::pair<std::uint64_t, std::uint64_t>, std::vector<std::uint64_t>>
        uses_by_edge;

    for (std::size_t t = 0; t < artifact.triangles_.size(); ++t) {
      for (std::size_t k = 0; k < 3; ++k) {
        const std::uint64_t origin = artifact.triangles_[t].corners[k];
        const std::uint64_t destination =
            artifact.triangles_[t].corners[(k + 1) % 3];
        const auto key = std::make_pair(std::min(origin, destination),
                                        std::max(origin, destination));
        uses_by_edge[key].push_back(use_index(t, k));
      }
    }

    // Each undirected edge must have exactly two directed uses.
    std::uint64_t edge_ordinal = 0;
    for (const auto &entry : uses_by_edge) {
      const auto &uses = entry.second;
      if (uses.size() != 2) {
        error = cleanup_error(cleanup_subcode::pair_cell_fan_contradiction,
                              bounded_boolean_error_category::internal_invariant_error,
                              "cleaned edge does not have exactly two uses",
                              cleanup_checkpoint::pair_cell_fan_audit);
        return false;
      }

      const std::uint64_t a = entry.first.first;
      const std::uint64_t b = entry.first.second;
      const std::uint64_t h0 = artifact.halfedges_.size();
      const std::uint64_t h1 = h0 + 1;

      cleaned_paired_edge_record edge;
      edge.canonical_id = edge_ordinal;
      edge.key = cleaned_edge_key::canonical(a, b);
      edge.halfedges = {h0, h1};
      edge.endpoints = {a, b};
      artifact.paired_edges_.push_back(std::move(edge));

      // Identify the forward (a -> b) and reverse (b -> a) directed uses.
      std::uint64_t forward_use = cleanup_invalid_ordinal;
      std::uint64_t reverse_use = cleanup_invalid_ordinal;
      for (const auto use : uses) {
        const std::uint64_t triangle = use / 3;
        const std::uint64_t origin = artifact.triangles_[triangle].corners[use % 3];
        if (origin == a)
          forward_use = use;
        else
          reverse_use = use;
      }
      if (forward_use == cleanup_invalid_ordinal ||
          reverse_use == cleanup_invalid_ordinal) {
        error = cleanup_error(cleanup_subcode::pair_cell_fan_contradiction,
                              bounded_boolean_error_category::internal_invariant_error,
                              "cleaned edge uses are not reversed",
                              cleanup_checkpoint::pair_cell_fan_audit);
        return false;
      }

      const auto bind_use = [&](std::uint64_t use, std::uint64_t halfedge,
                                std::uint64_t origin, std::uint64_t destination,
                                std::uint64_t pair) {
        const std::uint64_t triangle = use / 3;
        const std::uint64_t k = use % 3;
        artifact.triangles_[triangle].halfedges[k] = halfedge;
        cleaned_halfedge_record record;
        record.canonical_id = halfedge;
        record.pair = pair;
        record.origin = origin;
        record.destination = destination;
        record.edge = edge_ordinal;
        record.triangle = triangle;
        record.next = cleanup_invalid_ordinal;
        artifact.halfedges_.push_back(std::move(record));
      };
      // Push in canonical slot order: h0 (a -> b) then h1 (b -> a).
      bind_use(forward_use, h0, a, b, h1);
      bind_use(reverse_use, h1, b, a, h0);
      artifact.paired_edges_.back().triangles = {forward_use / 3, reverse_use / 3};
      ++edge_ordinal;
    }

    // Set next-in-triangle for each halfedge.
    for (std::size_t t = 0; t < artifact.triangles_.size(); ++t) {
      const auto &halfedges = artifact.triangles_[t].halfedges;
      for (std::size_t k = 0; k < 3; ++k) {
        if (halfedges[k] >= artifact.halfedges_.size()) {
          error = cleanup_error(cleanup_subcode::pair_cell_fan_contradiction,
                                bounded_boolean_error_category::internal_invariant_error,
                                "triangle halfedge slot is out of range",
                                cleanup_checkpoint::pair_cell_fan_audit);
          return false;
        }
        artifact.halfedges_[halfedges[k]].next = halfedges[(k + 1) % 3];
      }
    }
    return true;
  }

  static std::uint64_t use_index(std::uint64_t triangle, std::uint64_t corner) {
    return triangle * 3 + corner;
  }

  // -------------------------------------------------------------------------
  // Vertex links and connected components.
  // -------------------------------------------------------------------------
  bool build_links_components(artifact_type &artifact,
                              bounded_boolean_error &error) {
    const std::size_t vertex_count = artifact.vertices_.size();
    const std::size_t triangle_count = artifact.triangles_.size();

    // Outgoing halfedges per vertex.
    std::vector<std::vector<std::uint64_t>> outgoing(vertex_count);
    for (std::size_t h = 0; h < artifact.halfedges_.size(); ++h) {
      const auto &halfedge = artifact.halfedges_[h];
      if (halfedge.origin >= vertex_count) {
        error = cleanup_error(cleanup_subcode::pair_cell_fan_contradiction,
                              bounded_boolean_error_category::internal_invariant_error,
                              "halfedge origin is out of range",
                              cleanup_checkpoint::pair_cell_fan_audit);
        return false;
      }
      outgoing[halfedge.origin].push_back(h);
    }
    for (auto &list : outgoing)
      std::sort(list.begin(), list.end());
    for (std::size_t v = 0; v < vertex_count; ++v) {
      artifact.vertices_[v].link_begin = artifact.link_index_.size();
      artifact.vertices_[v].link_count = outgoing[v].size();
      for (const auto h : outgoing[v])
        artifact.link_index_.push_back(h);
    }

    // Connected components via shared-edge triangle adjacency.
    std::vector<std::int64_t> component_of(triangle_count, -1);
    std::vector<std::vector<std::uint64_t>> adjacency(triangle_count);
    for (const auto &edge : artifact.paired_edges_) {
      const std::uint64_t t0 = artifact.halfedges_[edge.halfedges[0]].triangle;
      const std::uint64_t t1 = artifact.halfedges_[edge.halfedges[1]].triangle;
      adjacency[t0].push_back(t1);
      adjacency[t1].push_back(t0);
    }
    std::uint64_t component_ordinal = 0;
    for (std::size_t t = 0; t < triangle_count; ++t) {
      if (component_of[t] != -1)
        continue;
      std::vector<std::uint64_t> stack{t};
      component_of[t] = static_cast<std::int64_t>(component_ordinal);
      std::vector<std::uint64_t> members;
      while (!stack.empty()) {
        const std::uint64_t current = stack.back();
        stack.pop_back();
        members.push_back(current);
        for (const auto next : adjacency[current])
          if (component_of[next] == -1) {
            component_of[next] = static_cast<std::int64_t>(component_ordinal);
            stack.push_back(next);
          }
      }
      std::sort(members.begin(), members.end());

      // Per-component Euler: collect distinct vertices and edges.
      std::vector<std::uint64_t> component_vertices;
      std::vector<std::uint64_t> component_edges;
      for (const auto member : members) {
        artifact.triangles_[member].component = component_ordinal;
        for (const auto corner : artifact.triangles_[member].corners)
          component_vertices.push_back(corner);
        for (const auto halfedge : artifact.triangles_[member].halfedges)
          component_edges.push_back(artifact.halfedges_[halfedge].edge);
      }
      std::sort(component_vertices.begin(), component_vertices.end());
      component_vertices.erase(
          std::unique(component_vertices.begin(), component_vertices.end()),
          component_vertices.end());
      std::sort(component_edges.begin(), component_edges.end());
      component_edges.erase(
          std::unique(component_edges.begin(), component_edges.end()),
          component_edges.end());

      cleaned_component_record component;
      component.canonical_id = component_ordinal;
      component.triangles_begin = artifact.component_triangle_index_.size();
      component.triangles_count = members.size();
      component.vertex_count = component_vertices.size();
      component.edge_count = component_edges.size();
      component.face_count = members.size();
      component.euler_chi = static_cast<std::int64_t>(component_vertices.size()) -
                            static_cast<std::int64_t>(component_edges.size()) +
                            static_cast<std::int64_t>(members.size());
      for (const auto member : members)
        artifact.component_triangle_index_.push_back(member);
      for (const auto vertex : component_vertices)
        artifact.vertices_[vertex].component = component_ordinal;
      artifact.components_.push_back(std::move(component));
      ++component_ordinal;
    }
    return true;
  }

  bool finalize(artifact_type &artifact, bounded_boolean_error &error) {
    artifact.status_ = artifact.vertices_.empty()
                           ? final_cleanup_status::verified_empty
                           : final_cleanup_status::verified_clean;

    artifact.statistics_.vertex_count = artifact.vertices_.size();
    artifact.statistics_.paired_edge_count = artifact.paired_edges_.size();
    artifact.statistics_.halfedge_count = artifact.halfedges_.size();
    artifact.statistics_.triangle_count = artifact.triangles_.size();
    artifact.statistics_.component_count = artifact.components_.size();
    artifact.statistics_.obligation_count =
        artifact.obligation_dispositions_.size();
    artifact.statistics_.action_count = artifact.actions_.size();

    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_cleaned_triangle_manifold(artifact, bytes, codec_limits_,
                                          codec_error)) {
      error = codec_error;
      return false;
    }
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();

    if (cleanup_cancelled(capabilities_,
                          cleanup_checkpoint::independent_verification))
      return true;
    bounded_boolean_error verification_error;
    if (!verify_cleaned_triangle_manifold(
            artifact, context_, precision_, *polygonal_, *triangulated_,
            verification_error)) {
      error = verification_error;
      return false;
    }
    artifact.verification_ = cleanup_verification_disposition::independently_verified;
    return true;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>>
build_cleaned_triangle_manifold(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    std::shared_ptr<const canonical_intersection_complex<T, I>> intersections,
    std::shared_ptr<const retained_surface_complex<T, I>> retained,
    std::shared_ptr<const polygonal_output_complex<T, I>> polygonal,
    std::shared_ptr<const triangulated_output_complex<T, I>> triangulated,
    cleanup_capabilities capabilities, cleanup_codec_limits codec_limits) {
  try {
    cleanup_builder<T, I> builder(context, precision, std::move(manifolds),
                                  std::move(intersections), std::move(retained),
                                  std::move(polygonal), std::move(triangulated),
                                  std::move(capabilities), codec_limits);
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>>::
        failure(cleanup_error(cleanup_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "cleanup allocation failed",
                              cleanup_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const cleaned_triangle_manifold<T>>>::
        failure(cleanup_error(cleanup_subcode::internal_invariant,
                              bounded_boolean_error_category::internal_invariant_error,
                              "cleanup unexpected exception",
                              cleanup_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
