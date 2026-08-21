#pragma once

#include "AssembledOutputCandidate.h"
#include "CleanedTriangleManifold.h"
#include "Context.h"
#include "FloatingBits.h"
#include "OutputAssemblyCodec.h"
#include "OutputAssemblyVerifier.h"
#include "PrecisionContext.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>>
assemble_output_candidate(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned,
    output_assembly_capabilities capabilities,
    output_assembly_codec_limits codec_limits = {});

#define YGOR_DECLARE_OUTPUT_ASSEMBLY_BUILD(T, I)                             \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const assembled_output_candidate<T, I>>>               \
  assemble_output_candidate<T, I>(                                           \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const cleaned_triangle_manifold<T>>,                   \
      output_assembly_capabilities, output_assembly_codec_limits)

YGOR_DECLARE_OUTPUT_ASSEMBLY_BUILD(float, std::uint32_t);
YGOR_DECLARE_OUTPUT_ASSEMBLY_BUILD(float, std::uint64_t);
YGOR_DECLARE_OUTPUT_ASSEMBLY_BUILD(double, std::uint32_t);
YGOR_DECLARE_OUTPUT_ASSEMBLY_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_OUTPUT_ASSEMBLY_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
namespace ygor::mesh_boolean::bounded {

namespace assembly_detail {

// One component's vertex/triangle member lists plus canonical labeling.
template <class T> struct component_workspace {
  std::vector<std::uint64_t> cleaned_vertices;  // cleaned vertex ordinals
  std::vector<std::uint64_t> cleaned_triangles; // cleaned triangle ordinals
  std::vector<std::uint32_t> local_vertex_order; // index into cleaned_vertices
  std::vector<std::uint8_t> content_bytes;       // canonical component content
  std::uint64_t edge_count = 0;
  std::int64_t euler_chi = 0;
};

// Graph for canonical labeling: vertex nodes and corner nodes.
struct labeling_graph {
  std::uint32_t vertex_count = 0;
  std::uint32_t corner_count = 0;
  std::vector<std::uint32_t> kind;             // 0 = vertex, 1 = corner
  std::vector<std::uint32_t> corner_vertex;    // corner -> vertex (local)
  std::vector<std::vector<std::pair<std::uint32_t, std::uint32_t>>> adjacency;
};

template <class U>
inline std::vector<U> sort_and_dedupe(std::vector<U> v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
  return v;
}

} // namespace assembly_detail

template <class T, class I> class output_assembly_builder {
public:
  output_assembly_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned,
      output_assembly_capabilities capabilities,
      output_assembly_codec_limits codec_limits)
      : context_(context), precision_(precision), cleaned_(std::move(cleaned)),
        capabilities_(std::move(capabilities)), codec_limits_(codec_limits) {}

  boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return failure(error);
    if (output_assembly_cancelled(capabilities_,
                                  output_assembly_checkpoint::predecessor_validation))
      return fail_cancelled();

    auto artifact = std::make_shared<assembled_output_candidate<T, I>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->precision_digest_ = precision_.digest();
    artifact->cleaned_digest_ = cleaned_->digest();

    if (!build_components(*artifact, error) ||
        !build_report(*artifact, error) ||
        !finalize(*artifact, error))
      return failure(error);

    if (output_assembly_cancelled(capabilities_, output_assembly_checkpoint::commit))
      return fail_cancelled();
    return boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>>::
        success(std::move(artifact));
  }

private:
  using artifact_type = assembled_output_candidate<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>>;

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned_;
  output_assembly_capabilities capabilities_;
  output_assembly_codec_limits codec_limits_;

  outcome_type failure(const bounded_boolean_error &error) {
    return outcome_type::failure(error);
  }
  outcome_type fail_cancelled() {
    return outcome_type::failure(output_assembly_error(
        output_assembly_subcode::cancelled,
        bounded_boolean_error_category::cancelled,
        "output assembly cancelled", output_assembly_checkpoint::commit));
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    if (!cleaned_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = output_assembly_error(
          output_assembly_subcode::wrong_owner,
          bounded_boolean_error_category::internal_invariant_error,
          "output assembly predecessor owner mismatch",
          output_assembly_checkpoint::predecessor_validation);
      return false;
    }
    if (cleaned_->operation() != context_.operation) {
      error = output_assembly_error(
          output_assembly_subcode::wrong_operation,
          bounded_boolean_error_category::internal_invariant_error,
          "output assembly predecessor operation mismatch",
          output_assembly_checkpoint::predecessor_validation);
      return false;
    }
    if (cleaned_->verification() !=
        cleanup_verification_disposition::independently_verified) {
      error = output_assembly_error(
          output_assembly_subcode::predecessor_not_verified,
          bounded_boolean_error_category::internal_invariant_error,
          "cleaned manifold is not independently verified",
          output_assembly_checkpoint::predecessor_validation);
      return false;
    }
    if (capabilities_.provider_version !=
            contract_versions::output_assembly_provider ||
        capabilities_.codec_version != contract_versions::output_assembly_codec ||
        capabilities_.verifier_version !=
            contract_versions::output_assembly_verifier) {
      error = output_assembly_error(
          output_assembly_subcode::unsupported_version,
          bounded_boolean_error_category::input_contract_error,
          "output assembly capability version mismatch",
          output_assembly_checkpoint::context_capability_validation);
      return false;
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Component reconstruction and canonical labeling.
  // -------------------------------------------------------------------------
  bool build_components(artifact_type &artifact, bounded_boolean_error &error) {
    const std::size_t component_count = cleaned_->components().size();
    std::vector<assembly_detail::component_workspace<T>> workspaces(
        component_count);
    for (std::size_t c = 0; c < component_count; ++c) {
      auto &workspace = workspaces[c];
      const auto &component = cleaned_->components()[c];
      workspace.edge_count = component.edge_count;
      workspace.euler_chi = component.euler_chi;
      for (std::uint64_t k = component.triangles_begin;
           k < component.triangles_begin + component.triangles_count; ++k)
        workspace.cleaned_triangles.push_back(
            cleaned_->component_triangle_index()[k]);
      std::vector<std::uint64_t> vertex_set;
      for (const auto triangle : workspace.cleaned_triangles)
        for (const auto vertex : cleaned_->triangles()[triangle].corners)
          vertex_set.push_back(vertex);
      workspace.cleaned_vertices = assembly_detail::sort_and_dedupe(vertex_set);

      if (!canonical_label_component(workspace, error))
        return false;
    }

    // Sort components by canonical content bytes.
    std::vector<std::uint64_t> component_order(component_count);
    for (std::size_t c = 0; c < component_count; ++c)
      component_order[c] = c;
    std::sort(component_order.begin(), component_order.end(),
              [&](std::uint64_t a, std::uint64_t b) {
                const auto &ca = workspaces[a].content_bytes;
                const auto &cb = workspaces[b].content_bytes;
                if (ca != cb)
                  return ca < cb;
                return workspaces[a].cleaned_vertices <
                       workspaces[b].cleaned_vertices;
              });

    // Assign global vertex/facet positions.
    std::uint64_t vertex_offset = 0;
    std::uint64_t facet_offset = 0;
    std::vector<std::uint64_t> vertex_position_by_cleaned(
        cleaned_->vertices().size(), assembly_invalid_ordinal);
    std::vector<std::uint64_t> facet_position_by_cleaned(
        cleaned_->triangles().size(), assembly_invalid_ordinal);
    std::vector<std::array<std::uint64_t, 3>> facet_corners;
    std::vector<std::uint64_t> facet_component;

    for (const auto c : component_order) {
      const auto &workspace = workspaces[c];
      assembly_component_record record;
      record.canonical_id = c;
      record.vertex_begin = vertex_offset;
      record.vertex_count = workspace.cleaned_vertices.size();
      record.facet_begin = facet_offset;
      record.facet_count = workspace.cleaned_triangles.size();
      record.edge_count = workspace.edge_count;
      record.euler_chi = workspace.euler_chi;
      artifact.components_.push_back(record);

      for (std::size_t i = 0; i < workspace.cleaned_vertices.size(); ++i) {
        const std::uint64_t cleaned_vertex =
            workspace.cleaned_vertices[workspace.local_vertex_order[i]];
        vertex_position_by_cleaned[cleaned_vertex] = vertex_offset + i;
      }
      for (const auto triangle : workspace.cleaned_triangles) {
        const auto &cleaned_triangle = cleaned_->triangles()[triangle];
        std::array<std::uint64_t, 3> corners{};
        for (std::size_t k = 0; k < 3; ++k)
          corners[k] =
              vertex_position_by_cleaned[cleaned_triangle.corners[k]];
        facet_position_by_cleaned[triangle] = facet_corners.size();
        facet_corners.push_back(corners);
        facet_component.push_back(c);
      }
      vertex_offset += workspace.cleaned_vertices.size();
      facet_offset += workspace.cleaned_triangles.size();
    }

    // Build the public mesh in canonical position order.
    std::vector<std::uint64_t> cleaned_by_position(cleaned_->vertices().size(),
                                                   assembly_invalid_ordinal);
    for (std::size_t o = 0; o < cleaned_->vertices().size(); ++o) {
      const std::uint64_t position = vertex_position_by_cleaned[o];
      if (position == assembly_invalid_ordinal) {
        error = output_assembly_error(
            output_assembly_subcode::permutation_non_bijective,
            bounded_boolean_error_category::internal_invariant_error,
            "cleaned vertex has no public position",
            output_assembly_checkpoint::vertex_permutation);
        return false;
      }
      if (cleaned_by_position[position] != assembly_invalid_ordinal) {
        error = output_assembly_error(
            output_assembly_subcode::permutation_non_bijective,
            bounded_boolean_error_category::internal_invariant_error,
            "public position is assigned more than once",
            output_assembly_checkpoint::vertex_permutation);
        return false;
      }
      cleaned_by_position[position] = o;
    }
    for (std::size_t p = 0; p < cleaned_->vertices().size(); ++p) {
      const std::uint64_t v = cleaned_by_position[p];
      const auto &vertex = cleaned_->vertices()[v];
      vec3<T> coordinate;
      coordinate.x = from_bits<T>(static_cast<floating_uint_t<T>>(vertex.nominal_bits[0]));
      coordinate.y = from_bits<T>(static_cast<floating_uint_t<T>>(vertex.nominal_bits[1]));
      coordinate.z = from_bits<T>(static_cast<floating_uint_t<T>>(vertex.nominal_bits[2]));
      artifact.mesh_.vertices.push_back(coordinate);

      coordinate_copy_record copy;
      copy.canonical_id = artifact.coordinate_copies_.size();
      copy.public_vertex = p;
      copy.cleaned_occurrence = vertex.component11_occurrence;
      copy.output_bits = vertex.nominal_bits;
      copy.disposition = coordinate_copy_disposition::exact_bits_preserved;
      artifact.coordinate_copies_.push_back(std::move(copy));
    }
    for (const auto &corners : facet_corners) {
      // Forward cyclic rotation to the lexicographically smallest triple.
      std::array<std::uint64_t, 3> rotated = corners;
      for (std::size_t r = 1; r < 3; ++r) {
        const std::array<std::uint64_t, 3> candidate{
            corners[r], corners[(r + 1) % 3], corners[(r + 2) % 3]};
        if (candidate < rotated)
          rotated = candidate;
      }
      std::vector<I> face;
      face.reserve(3);
      for (const auto corner : rotated) {
        if (corner > std::numeric_limits<I>::max()) {
          error = output_assembly_error(
              output_assembly_subcode::public_index_capacity,
              bounded_boolean_error_category::index_overflow,
              "public vertex position does not fit the index type",
              output_assembly_checkpoint::public_mesh_construction);
          return false;
        }
        face.push_back(static_cast<I>(corner));
      }
      artifact.mesh_.faces.push_back(std::move(face));
    }

    // Maps.
    artifact.vertex_by_occurrence_.resize(cleaned_->vertices().size(),
                                          assembly_invalid_ordinal);
    artifact.occurrence_by_vertex_.resize(cleaned_->vertices().size(),
                                          assembly_invalid_ordinal);
    for (std::size_t o = 0; o < cleaned_->vertices().size(); ++o) {
      const std::uint64_t position = vertex_position_by_cleaned[o];
      artifact.vertex_by_occurrence_[o] = position;
      artifact.occurrence_by_vertex_[position] = o;
    }
    artifact.facet_by_triangle_.resize(cleaned_->triangles().size(),
                                       assembly_invalid_ordinal);
    artifact.triangle_by_facet_.resize(cleaned_->triangles().size(),
                                       assembly_invalid_ordinal);
    for (std::size_t t = 0; t < cleaned_->triangles().size(); ++t) {
      const std::uint64_t facet = facet_position_by_cleaned[t];
      artifact.facet_by_triangle_[t] = facet;
      artifact.triangle_by_facet_[facet] = t;
    }
    return true;
  }

  // Canonically label one component's vertices via equitable refinement.
  bool canonical_label_component(
      assembly_detail::component_workspace<T> &workspace,
      bounded_boolean_error &error) {
    const std::uint32_t vertex_count =
        static_cast<std::uint32_t>(workspace.cleaned_vertices.size());
    const std::uint32_t triangle_count =
        static_cast<std::uint32_t>(workspace.cleaned_triangles.size());
    const std::uint32_t corner_count = 3 * triangle_count;

    // Map cleaned vertex ordinal -> local vertex index.
    std::vector<std::uint32_t> local_by_cleaned(cleaned_->vertices().size(),
                                                std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t i = 0; i < vertex_count; ++i)
      local_by_cleaned[workspace.cleaned_vertices[i]] = i;

    assembly_detail::labeling_graph graph;
    graph.vertex_count = vertex_count;
    graph.corner_count = corner_count;
    graph.kind.resize(vertex_count + corner_count);
    graph.corner_vertex.resize(corner_count);
    for (std::uint32_t i = 0; i < vertex_count; ++i)
      graph.kind[i] = 0;
    for (std::uint32_t i = 0; i < corner_count; ++i)
      graph.kind[vertex_count + i] = 1;
    graph.adjacency.resize(vertex_count + corner_count);

    // Initial vertex colors from coordinate bits.
    std::vector<std::vector<std::uint32_t>> colors(vertex_count + corner_count);
    for (std::uint32_t i = 0; i < vertex_count; ++i) {
      const auto &vertex = cleaned_->vertices()[workspace.cleaned_vertices[i]];
      colors[i] = {0};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        colors[i].push_back(
            static_cast<std::uint32_t>(vertex.nominal_bits[axis] >> 32));
        colors[i].push_back(
            static_cast<std::uint32_t>(vertex.nominal_bits[axis] & 0xffffffff));
      }
    }
    for (std::uint32_t i = 0; i < corner_count; ++i)
      colors[vertex_count + i] = {1};

    // Build corner relations.
    for (std::uint32_t t = 0; t < triangle_count; ++t) {
      const auto &cleaned_triangle =
          cleaned_->triangles()[workspace.cleaned_triangles[t]];
      for (std::uint32_t k = 0; k < 3; ++k) {
        const std::uint32_t corner = vertex_count + 3 * t + k;
        const std::uint32_t vertex = local_by_cleaned[cleaned_triangle.corners[k]];
        graph.corner_vertex[3 * t + k] = vertex;
        // corner -> vertex (type 0), vertex -> corner (type 1).
        graph.adjacency[corner].push_back({0, vertex});
        graph.adjacency[vertex].push_back({1, corner});
        // next/prev within triangle (types 2, 3).
        const std::uint32_t next = vertex_count + 3 * t + ((k + 1) % 3);
        const std::uint32_t prev = vertex_count + 3 * t + ((k + 2) % 3);
        graph.adjacency[corner].push_back({2, next});
        graph.adjacency[corner].push_back({3, prev});
      }
    }

    // Assign dense color IDs from distinct initial color vectors.
    refine_colors(graph, colors);

    // Order vertices by final color (stable: color, then cleaned vertex key).
    std::vector<std::uint32_t> vertex_order(vertex_count);
    for (std::uint32_t i = 0; i < vertex_count; ++i)
      vertex_order[i] = i;
    std::sort(vertex_order.begin(), vertex_order.end(),
              [&](std::uint32_t a, std::uint32_t b) {
                if (colors[a] != colors[b])
                  return colors[a] < colors[b];
                return workspace.cleaned_vertices[a] <
                       workspace.cleaned_vertices[b];
              });

    // Verify the labeling is discrete (each vertex has a distinct color).
    for (std::uint32_t i = 1; i < vertex_count; ++i)
      if (colors[vertex_order[i]] == colors[vertex_order[i - 1]]) {
        error = output_assembly_error(
            output_assembly_subcode::automorphism_inconsistency,
            bounded_boolean_error_category::internal_invariant_error,
            "canonical labeling did not resolve vertex automorphisms",
            output_assembly_checkpoint::canonical_labeling);
        return false;
      }

    workspace.local_vertex_order = vertex_order;

    // Build component content bytes: vertex coordinate bits in canonical order,
    // then rotated facet triples in canonical order.
    canonical_writer content;
    content.u64(vertex_count);
    for (const auto i : vertex_order) {
      const auto &vertex = cleaned_->vertices()[workspace.cleaned_vertices[i]];
      for (std::size_t axis = 0; axis < 3; ++axis)
        content.u64(vertex.nominal_bits[axis]);
    }
    content.u64(triangle_count);
    std::vector<std::array<std::uint32_t, 3>> triples;
    for (const auto triangle : workspace.cleaned_triangles) {
      const auto &cleaned_triangle = cleaned_->triangles()[triangle];
      std::array<std::uint32_t, 3> triple{};
      for (std::size_t k = 0; k < 3; ++k) {
        const std::uint32_t local = local_by_cleaned[cleaned_triangle.corners[k]];
        // Find local vertex's rank in the canonical order.
        triple[k] = static_cast<std::uint32_t>(
            std::distance(vertex_order.begin(),
                          std::find(vertex_order.begin(), vertex_order.end(), local)));
      }
      // Forward cyclic rotation to lexicographic minimum.
      std::array<std::uint32_t, 3> best = triple;
      for (std::size_t r = 1; r < 3; ++r) {
        const std::array<std::uint32_t, 3> candidate{
            triple[r], triple[(r + 1) % 3], triple[(r + 2) % 3]};
        if (candidate < best)
          best = candidate;
      }
      triples.push_back(best);
    }
    std::sort(triples.begin(), triples.end());
    for (const auto &triple : triples) {
      content.u32(triple[0]);
      content.u32(triple[1]);
      content.u32(triple[2]);
    }
    workspace.content_bytes = content.take();
    return true;
  }

  // Full-signature equitable refinement to a stable partition.
  void refine_colors(assembly_detail::labeling_graph &graph,
                     std::vector<std::vector<std::uint32_t>> &colors) {
    const std::size_t node_count = colors.size();
    std::vector<std::uint64_t> color_id(node_count);
    auto assign_ids = [&]() {
      std::vector<std::vector<std::uint32_t>> distinct;
      for (const auto &color : colors) {
        auto it = std::lower_bound(distinct.begin(), distinct.end(), color);
        if (it == distinct.end() || *it != color)
          distinct.insert(it, color);
      }
      for (std::size_t i = 0; i < node_count; ++i)
        color_id[i] = static_cast<std::uint64_t>(
            std::lower_bound(distinct.begin(), distinct.end(), colors[i]) -
            distinct.begin());
      return distinct.size();
    };
    assign_ids();

    for (std::uint64_t round = 0; round <= node_count; ++round) {
      std::vector<std::vector<std::uint32_t>> signatures(node_count);
      bool stable = true;
      for (std::size_t i = 0; i < node_count; ++i) {
        signatures[i].reserve(colors[i].size() + 2 * graph.adjacency[i].size() + 1);
        signatures[i].insert(signatures[i].end(), colors[i].begin(),
                             colors[i].end());
        std::vector<std::pair<std::uint32_t, std::uint64_t>> runs;
        for (const auto &relation : graph.adjacency[i])
          runs.push_back({relation.first, color_id[relation.second]});
        std::sort(runs.begin(), runs.end());
        for (const auto &run : runs) {
          signatures[i].push_back(run.first);
          signatures[i].push_back(static_cast<std::uint32_t>(run.second & 0xffffffff));
          signatures[i].push_back(static_cast<std::uint32_t>(run.second >> 32));
        }
        if (signatures[i] != colors[i])
          stable = false;
      }
      colors = std::move(signatures);
      assign_ids();
      if (stable)
        break;
    }
  }

  bool build_report(artifact_type &artifact, bounded_boolean_error &error) {
    (void)error;
    artifact.report_.vertex_count = cleaned_->vertices().size();
    artifact.report_.edge_count = cleaned_->paired_edges().size();
    artifact.report_.facet_count = cleaned_->triangles().size();
    artifact.report_.component_count = cleaned_->components().size();
    T maximum_radial = T(0);
    for (const auto &vertex : cleaned_->vertices()) {
      const T radial =
          from_bits<T>(static_cast<floating_uint_t<T>>(vertex.radial_error_bits));
      if (radial > maximum_radial)
        maximum_radial = radial;
    }
    artifact.report_.output_precision_bits =
        static_cast<std::uint64_t>(to_bits<T>(maximum_radial));
    artifact.report_.maximum_authorized_tolerance_bits =
        static_cast<std::uint64_t>(to_bits<T>(precision_.tolerance()));
    return true;
  }

  bool finalize(artifact_type &artifact, bounded_boolean_error &error) {
    artifact.statistics_.component_count = artifact.components_.size();
    artifact.statistics_.vertex_count = artifact.mesh_.vertices.size();
    artifact.statistics_.facet_count = artifact.mesh_.faces.size();

    std::vector<std::uint8_t> content;
    if (!encode_public_mesh_content(artifact, content, error))
      return false;
    artifact.public_content_bytes_ = std::move(content);

    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_assembled_output_candidate(artifact, bytes, codec_limits_,
                                           codec_error)) {
      error = codec_error;
      return false;
    }
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();

    if (output_assembly_cancelled(
            capabilities_, output_assembly_checkpoint::independent_verification))
      return true;
    bounded_boolean_error verification_error;
    if (!verify_assembled_output_candidate(artifact, context_, precision_,
                                           *cleaned_, verification_error)) {
      error = verification_error;
      return false;
    }
    artifact.verification_ =
        output_assembly_verification_disposition::independently_verified;
    return true;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>>
assemble_output_candidate(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned,
    output_assembly_capabilities capabilities,
    output_assembly_codec_limits codec_limits) {
  try {
    output_assembly_builder<T, I> builder(context, precision, std::move(cleaned),
                                          std::move(capabilities), codec_limits);
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>>::
        failure(output_assembly_error(
            output_assembly_subcode::resource_preflight,
            bounded_boolean_error_category::resource_limit,
            "output assembly allocation failed",
            output_assembly_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const assembled_output_candidate<T, I>>>::
        failure(output_assembly_error(
            output_assembly_subcode::internal_invariant,
            bounded_boolean_error_category::internal_invariant_error,
            "output assembly unexpected exception",
            output_assembly_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
