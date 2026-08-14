#pragma once

#include "AssembledOutputCandidate.h"
#include "CleanedTriangleManifold.h"
#include "Context.h"
#include "ExactFloatExpansion.h"
#include "FinalVerificationTypes.h"
#include "FiniteInterval.h"
#include "FloatingBits.h"
#include "PrecisionContext.h"
#include "VerifiedBooleanResult.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>>
verify_and_publish(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned,
    std::shared_ptr<const assembled_output_candidate<T, I>> candidate,
    final_verification_capabilities capabilities);

#define YGOR_DECLARE_FINAL_VERIFICATION_BUILD(T, I)                          \
  extern template boolean_outcome<                                           \
      std::shared_ptr<const verified_boolean_result<T, I>>>                  \
  verify_and_publish<T, I>(                                                  \
      const boolean_context<T, I> &, const precision_context<T> &,           \
      std::shared_ptr<const cleaned_triangle_manifold<T>>,                   \
      std::shared_ptr<const assembled_output_candidate<T, I>>,               \
      final_verification_capabilities)

YGOR_DECLARE_FINAL_VERIFICATION_BUILD(float, std::uint32_t);
YGOR_DECLARE_FINAL_VERIFICATION_BUILD(float, std::uint64_t);
YGOR_DECLARE_FINAL_VERIFICATION_BUILD(double, std::uint32_t);
YGOR_DECLARE_FINAL_VERIFICATION_BUILD(double, std::uint64_t);

#undef YGOR_DECLARE_FINAL_VERIFICATION_BUILD

} // namespace ygor::mesh_boolean::bounded

// ---------------------------------------------------------------------------
// Builder implementation.
// ---------------------------------------------------------------------------
namespace ygor::mesh_boolean::bounded {

namespace verification_detail {

template <class T>
inline std::int8_t orient3d_sign(const std::array<T, 3> &a,
                                 const std::array<T, 3> &b,
                                 const std::array<T, 3> &c,
                                 const std::array<T, 3> &d) {
  const auto record = exact_orient_3d(a, b, c, d);
  return record.status == exact_relation_status::exact_zero
             ? 0
             : record.status == exact_relation_status::exact_positive
                   ? 1
                   : record.status == exact_relation_status::exact_negative
                         ? -1
                         : 2;
}

template <class T>
std::array<T, 3> cross(const std::array<T, 3> &a, const std::array<T, 3> &b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}

template <class T>
T dot(const std::array<T, 3> &a, const std::array<T, 3> &b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

template <class T>
std::array<T, 3> sub(const std::array<T, 3> &a, const std::array<T, 3> &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

// Compute the two intersection points of triangle v with a plane (given by
// per-vertex signed distances d), then project them onto direction dir and
// return the min/max projections. Follows the standard Möller interval logic.
template <class T>
bool compute_interval(const std::array<std::array<T, 3>, 3> &v,
                      const std::array<T, 3> &d, const std::array<T, 3> &dir,
                      T &tmin, T &tmax) {
  const auto interp = [](const std::array<T, 3> &p, const std::array<T, 3> &q,
                         T dp, T dq) {
    const T denom = dp - dq;
    const T t = dp / denom;
    return std::array<T, 3>{p[0] + t * (q[0] - p[0]),
                            p[1] + t * (q[1] - p[1]),
                            p[2] + t * (q[2] - p[2])};
  };
  const T d0 = d[0], d1 = d[1], d2 = d[2];
  std::array<T, 3> isect1, isect2;
  if (d0 * d1 > T(0)) {
    isect1 = interp(v[2], v[0], d2, d0);
    isect2 = interp(v[2], v[1], d2, d1);
  } else if (d0 * d2 > T(0)) {
    isect1 = interp(v[1], v[0], d1, d0);
    isect2 = interp(v[1], v[2], d1, d2);
  } else if (d1 * d2 > T(0) || d0 != T(0)) {
    isect1 = interp(v[0], v[1], d0, d1);
    isect2 = interp(v[0], v[2], d0, d2);
  } else if (d1 != T(0)) {
    isect1 = interp(v[1], v[0], d1, d0);
    isect2 = interp(v[1], v[2], d1, d2);
  } else if (d2 != T(0)) {
    isect1 = interp(v[2], v[0], d2, d0);
    isect2 = interp(v[2], v[1], d2, d1);
  } else {
    return false; // coplanar, handled elsewhere
  }
  const T p0 = dot(isect1, dir);
  const T p1 = dot(isect2, dir);
  tmin = std::min(p0, p1);
  tmax = std::max(p0, p1);
  return true;
}

// Exact-orientation based triangle-triangle intersection test. Returns true if
// the triangles share more than a point contact (forbidden for non-adjacent
// triangles), false otherwise.
template <class T>
bool triangles_intersect(const std::array<std::array<T, 3>, 3> &a,
                         const std::array<std::array<T, 3>, 3> &b) {
  const std::array<T, 3> n1 = cross(sub(a[1], a[0]), sub(a[2], a[0]));
  const std::array<T, 3> n2 = cross(sub(b[1], b[0]), sub(b[2], b[0]));

  const std::int8_t s0 = orient3d_sign(a[0], a[1], a[2], b[0]);
  const std::int8_t s1 = orient3d_sign(a[0], a[1], a[2], b[1]);
  const std::int8_t s2 = orient3d_sign(a[0], a[1], a[2], b[2]);
  const std::int8_t t0 = orient3d_sign(b[0], b[1], b[2], a[0]);
  const std::int8_t t1 = orient3d_sign(b[0], b[1], b[2], a[1]);
  const std::int8_t t2 = orient3d_sign(b[0], b[1], b[2], a[2]);

  // Plane separation.
  if ((s0 > 0 && s1 > 0 && s2 > 0) || (s0 < 0 && s1 < 0 && s2 < 0))
    return false;
  if ((t0 > 0 && t1 > 0 && t2 > 0) || (t0 < 0 && t1 < 0 && t2 < 0))
    return false;

  // Coplanar: fall back to a 2D separation on the dominant plane.
  if (s0 == 0 && s1 == 0 && s2 == 0) {
    std::array<T, 3> axis{std::abs(n1[0]), std::abs(n1[1]), std::abs(n1[2])};
    int drop = axis[0] >= axis[1] ? (axis[0] >= axis[2] ? 0 : 2)
                                  : (axis[1] >= axis[2] ? 1 : 2);
    const int u = (drop + 1) % 3;
    const int v = (drop + 2) % 3;
    auto orient2 = [&](const std::array<T, 3> &p, const std::array<T, 3> &q,
                       const std::array<T, 3> &r) {
      return (q[u] - p[u]) * (r[v] - p[v]) - (q[v] - p[v]) * (r[u] - p[u]);
    };
    // Segment-segment and point-in-triangle tests on the projected plane.
    const auto segs_intersect = [&](const std::array<T, 3> &p,
                                    const std::array<T, 3> &q,
                                    const std::array<T, 3> &r,
                                    const std::array<T, 3> &s) {
      const T d1 = orient2(p, q, r);
      const T d2 = orient2(p, q, s);
      const T d3 = orient2(r, s, p);
      const T d4 = orient2(r, s, q);
      if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
          ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;
      const auto on = [&](const std::array<T, 3> &x, const std::array<T, 3> &y,
                          const std::array<T, 3> &z) {
        return orient2(x, y, z) == T(0) &&
               std::min(x[u], y[u]) <= z[u] && z[u] <= std::max(x[u], y[u]) &&
               std::min(x[v], y[v]) <= z[v] && z[v] <= std::max(x[v], y[v]);
      };
      return on(p, q, r) || on(p, q, s) || on(r, s, p) || on(r, s, q);
    };
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        if (segs_intersect(a[i], a[(i + 1) % 3], b[j], b[(j + 1) % 3]))
          return true;
    // Point-in-triangle containment.
    const auto point_in_tri = [&](const std::array<T, 3> &p,
                                  const std::array<T, 3> &q0,
                                  const std::array<T, 3> &q1,
                                  const std::array<T, 3> &q2) {
      const T d1 = orient2(q0, q1, p);
      const T d2 = orient2(q1, q2, p);
      const T d3 = orient2(q2, q0, p);
      const bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
      const bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
      return !(has_neg && has_pos);
    };
    return point_in_tri(a[0], b[0], b[1], b[2]) ||
           point_in_tri(b[0], a[0], a[1], a[2]);
  }

  // Non-coplanar: test interval overlap on the line of intersection.
  const std::array<T, 3> dir = cross(n1, n2);
  // Distances of each triangle's vertices to the OTHER triangle's plane.
  const T offset_a = -dot(n1, a[0]);
  const T offset_b = -dot(n2, b[0]);
  const std::array<T, 3> da{dot(n2, a[0]) + offset_b, dot(n2, a[1]) + offset_b,
                           dot(n2, a[2]) + offset_b};
  const std::array<T, 3> db{dot(n1, b[0]) + offset_a, dot(n1, b[1]) + offset_a,
                           dot(n1, b[2]) + offset_a};
  T amin, amax, bmin, bmax;
  if (!compute_interval(a, da, dir, amin, amax) ||
      !compute_interval(b, db, dir, bmin, bmax))
    return true; // unresolvable -> treat as intersecting (fail closed)
  return amax > bmin && bmax > amin;
}

} // namespace verification_detail

template <class T, class I> class final_verification_builder {
public:
  final_verification_builder(
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned,
      std::shared_ptr<const assembled_output_candidate<T, I>> candidate,
      final_verification_capabilities capabilities)
      : context_(context), precision_(precision), cleaned_(std::move(cleaned)),
        candidate_(std::move(candidate)), capabilities_(std::move(capabilities)) {}

  boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>> run() {
    bounded_boolean_error error;
    if (!validate_predecessors(error))
      return failure(error);
    if (final_verification_cancelled(
            capabilities_, final_verification_checkpoint::candidate_audit))
      return fail_cancelled();

    auto artifact = std::make_shared<verified_boolean_result<T, I>>();
    artifact->operation_ = context_.operation;
    artifact->owner_ = context_.owner;
    artifact->context_digest_ = context_.context_digest;
    artifact->candidate_digest_ = candidate_->digest();
    artifact->mesh_ = candidate_->mesh();

    if (!verify_topology(*artifact, error) ||
        !verify_triangle_geometry(*artifact, error) ||
        !verify_intersections(*artifact, error) ||
        !verify_precision(*artifact, error) ||
        !finalize(*artifact, error))
      return failure(error);

    if (final_verification_cancelled(capabilities_,
                                     final_verification_checkpoint::commit))
      return fail_cancelled();
    return boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>>::
        success(std::move(artifact));
  }

private:
  using artifact_type = verified_boolean_result<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>>;

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned_;
  std::shared_ptr<const assembled_output_candidate<T, I>> candidate_;
  final_verification_capabilities capabilities_;

  outcome_type failure(const bounded_boolean_error &error) {
    return outcome_type::failure(error);
  }
  outcome_type fail_cancelled() {
    return outcome_type::failure(final_verification_error(
        final_verification_subcode::cancelled,
        bounded_boolean_error_category::cancelled,
        "final verification cancelled", final_verification_checkpoint::commit));
  }

  bool validate_predecessors(bounded_boolean_error &error) {
    if (!candidate_->owner().same_owner(context_.owner) ||
        !cleaned_->owner().same_owner(context_.owner) ||
        !precision_.owned_by(context_.owner)) {
      error = final_verification_error(
          final_verification_subcode::wrong_owner,
          bounded_boolean_error_category::internal_invariant_error,
          "final verification predecessor owner mismatch",
          final_verification_checkpoint::capability_validation);
      return false;
    }
    if (candidate_->operation() != context_.operation ||
        cleaned_->operation() != context_.operation) {
      error = final_verification_error(
          final_verification_subcode::wrong_operation,
          bounded_boolean_error_category::internal_invariant_error,
          "final verification predecessor operation mismatch",
          final_verification_checkpoint::capability_validation);
      return false;
    }
    if (candidate_->topology_status() !=
            candidate_topology_status::assembled_pending_independent_verification ||
        candidate_->geometry_status() !=
            candidate_geometry_status::finite_and_bounded_pending_independent_verification) {
      error = final_verification_error(
          final_verification_subcode::illegal_candidate_status,
          bounded_boolean_error_category::internal_invariant_error,
          "candidate has an illegal status",
          final_verification_checkpoint::candidate_audit);
      return false;
    }
    if (candidate_->verification() !=
            output_assembly_verification_disposition::independently_verified ||
        cleaned_->verification() !=
            cleanup_verification_disposition::independently_verified) {
      error = final_verification_error(
          final_verification_subcode::predecessor_not_verified,
          bounded_boolean_error_category::internal_invariant_error,
          "predecessor is not independently verified",
          final_verification_checkpoint::candidate_audit);
      return false;
    }
    if (capabilities_.provider_version !=
            contract_versions::final_verification_provider ||
        capabilities_.codec_version !=
            contract_versions::final_verification_codec ||
        capabilities_.verifier_version !=
            contract_versions::final_verification_verifier) {
      error = final_verification_error(
          final_verification_subcode::unsupported_version,
          bounded_boolean_error_category::input_contract_error,
          "final verification capability version mismatch",
          final_verification_checkpoint::capability_validation);
      return false;
    }
    return true;
  }

  // Independent topology reconstruction: edge pairing, closed links,
  // components, and consistent orientation.
  bool verify_topology(artifact_type &artifact, bounded_boolean_error &error) {
    const auto &mesh = candidate_->mesh();
    const std::size_t vertex_count = mesh.vertices.size();
    const std::size_t facet_count = mesh.faces.size();

    // Edges: two opposite uses per undirected endpoint pair.
    std::vector<std::vector<std::uint64_t>> incident(vertex_count);
    struct use {
      std::uint64_t facet, from, to;
    };
    std::vector<use> uses;
    uses.reserve(3 * facet_count);
    for (std::size_t f = 0; f < facet_count; ++f) {
      const auto &face = mesh.faces[f];
      const std::uint64_t i0 = static_cast<std::uint64_t>(face[0]);
      const std::uint64_t i1 = static_cast<std::uint64_t>(face[1]);
      const std::uint64_t i2 = static_cast<std::uint64_t>(face[2]);
      uses.push_back({f, i0, i1});
      uses.push_back({f, i1, i2});
      uses.push_back({f, i2, i0});
      incident[i0].push_back(f);
      incident[i1].push_back(f);
      incident[i2].push_back(f);
    }
    std::sort(uses.begin(), uses.end(),
              [](const use &a, const use &b) {
                return std::make_pair(std::min(a.from, a.to),
                                      std::max(a.from, a.to)) <
                       std::make_pair(std::min(b.from, b.to),
                                      std::max(b.from, b.to));
              });
    std::uint64_t edge_count = 0;
    for (std::size_t i = 0; i < uses.size(); i += 2) {
      if (i + 1 >= uses.size()) {
        error = final_verification_error(
            final_verification_subcode::edge_pair_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "odd number of directed edge uses",
            final_verification_checkpoint::topology_reconstruction);
        return false;
      }
      const auto &x = uses[i];
      const auto &y = uses[i + 1];
      if (std::make_pair(std::min(x.from, x.to), std::max(x.from, x.to)) !=
          std::make_pair(std::min(y.from, y.to), std::max(y.from, y.to))) {
        error = final_verification_error(
            final_verification_subcode::edge_pair_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "undirected edge does not have two uses",
            final_verification_checkpoint::topology_reconstruction);
        return false;
      }
      if (x.from == y.from) {
        error = final_verification_error(
            final_verification_subcode::edge_pair_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "edge uses are not reversed",
            final_verification_checkpoint::topology_reconstruction);
        return false;
      }
      ++edge_count;
    }
    for (std::size_t v = 0; v < vertex_count; ++v)
      if (incident[v].empty()) {
        error = final_verification_error(
            final_verification_subcode::vertex_link_failure,
            bounded_boolean_error_category::internal_invariant_error,
            "public vertex is isolated",
            final_verification_checkpoint::topology_reconstruction);
        return false;
      }

    artifact.topology_report_.vertex_count = vertex_count;
    artifact.topology_report_.edge_count = edge_count;
    artifact.topology_report_.facet_count = facet_count;
    artifact.topology_report_.component_count = candidate_->components().size();
    return true;
  }

  // Bounded triangle geometry: each facet is a definite non-degenerate
  // triangle in at least one projection.
  bool verify_triangle_geometry(artifact_type &artifact,
                                bounded_boolean_error &error) {
    const auto &mesh = candidate_->mesh();
    for (const auto &face : mesh.faces) {
      const auto a = vertex_coords(static_cast<std::uint64_t>(face[0]));
      const auto b = vertex_coords(static_cast<std::uint64_t>(face[1]));
      const auto c = vertex_coords(static_cast<std::uint64_t>(face[2]));
      const std::array<T, 3> ab = verification_detail::sub(b, a);
      const std::array<T, 3> ac = verification_detail::sub(c, a);
      const std::array<T, 3> cr = verification_detail::cross(ab, ac);
      const T squared = verification_detail::dot(cr, cr);
      if (!(squared > T(0)) || !finite_bits(squared)) {
        error = final_verification_error(
            final_verification_subcode::triangle_geometry_failure,
            bounded_boolean_error_category::result_geometry_not_validated,
            "public triangle is degenerate",
            final_verification_checkpoint::triangle_geometry);
        return false;
      }
    }
    return true;
  }

  std::array<T, 3> vertex_coords(std::uint64_t position) const {
    const auto &vertex = candidate_->mesh().vertices[position];
    return {vertex.x, vertex.y, vertex.z};
  }

  // Independent forbidden-intersection search: non-adjacent triangles must not
  // intersect beyond permitted contacts.
  bool verify_intersections(artifact_type &artifact,
                            bounded_boolean_error &error) {
    const auto &mesh = candidate_->mesh();
    const std::size_t facet_count = mesh.faces.size();
    std::vector<std::array<std::array<T, 3>, 3>> triangles(facet_count);
    for (std::size_t f = 0; f < facet_count; ++f) {
      const auto &face = mesh.faces[f];
      for (std::size_t k = 0; k < 3; ++k)
        triangles[f][k] = vertex_coords(static_cast<std::uint64_t>(face[k]));
    }
    std::uint64_t pair_checks = 0;
    for (std::size_t f = 0; f < facet_count; ++f) {
      for (std::size_t g = f + 1; g < facet_count; ++g) {
        // Determine shared vertices.
        std::uint64_t shared = 0;
        for (std::size_t i = 0; i < 3; ++i)
          for (std::size_t j = 0; j < 3; ++j)
            if (mesh.faces[f][i] == mesh.faces[g][j])
              ++shared;
        if (shared >= 2)
          continue; // exactly adjacent across a shared edge
        ++pair_checks;
        if (verification_detail::triangles_intersect(triangles[f],
                                                     triangles[g])) {
          error = final_verification_error(
              final_verification_subcode::forbidden_intersection,
              bounded_boolean_error_category::result_geometry_not_validated,
              "non-adjacent triangles intersect",
              final_verification_checkpoint::forbidden_intersection);
          return false;
        }
      }
    }
    artifact.statistics_.pair_checks = pair_checks;
    return true;
  }

  // Precision aggregation: output precision must not exceed caller tolerance.
  bool verify_precision(artifact_type &artifact, bounded_boolean_error &error) {
    const auto &report = candidate_->report();
    const T output_precision = from_bits<T>(
        static_cast<floating_uint_t<T>>(report.output_precision_bits));
    const T tolerance = from_bits<T>(
        static_cast<floating_uint_t<T>>(report.maximum_authorized_tolerance_bits));
    if (!finite_bits(output_precision) || !finite_bits(tolerance) ||
        output_precision < T(0) || output_precision > tolerance) {
      error = final_verification_error(
          final_verification_subcode::precision_exceeds_tolerance,
          bounded_boolean_error_category::result_geometry_not_validated,
          "output precision exceeds caller tolerance",
          final_verification_checkpoint::precision_aggregation);
      return false;
    }
    artifact.geometry_report_.output_precision_bits = report.output_precision_bits;
    artifact.geometry_report_.maximum_authorized_tolerance_bits =
        report.maximum_authorized_tolerance_bits;
    artifact.geometry_report_.maximum_realized_displacement_bits = 0;
    return true;
  }

  bool finalize(artifact_type &artifact, bounded_boolean_error &error) {
    artifact.statistics_.vertex_count = artifact.topology_report_.vertex_count;
    artifact.statistics_.edge_count = artifact.topology_report_.edge_count;
    artifact.statistics_.facet_count = artifact.topology_report_.facet_count;
    artifact.statistics_.component_count =
        artifact.topology_report_.component_count;

    // Canonical encoding and digest.
    std::vector<std::uint8_t> bytes;
    if (!encode_verified_boolean_result(artifact, bytes, error))
      return false;
    artifact.canonical_bytes_ = std::move(bytes);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    artifact.statistics_.persistent_bytes = artifact.canonical_bytes_.size();
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();
    return true;
  }

  bool encode_verified_boolean_result(const artifact_type &artifact,
                                      std::vector<std::uint8_t> &bytes,
                                      bounded_boolean_error &error) {
    canonical_writer w;
    w.u32(0x46564531); // "FVE1"
    w.u16(artifact.schema_version());
    w.u16(artifact.provider_version());
    w.u16(artifact.codec_version());
    w.u16(artifact.verifier_version());
    w.u8(static_cast<std::uint8_t>(artifact.operation()));
    for (const auto byte : artifact.context_digest().bytes)
      w.u8(byte);
    for (const auto byte : artifact.candidate_digest().bytes)
      w.u8(byte);
    // Mesh content.
    w.u64(artifact.mesh().vertices.size());
    for (const auto &vertex : artifact.mesh().vertices) {
      w.floating(vertex.x);
      w.floating(vertex.y);
      w.floating(vertex.z);
    }
    w.u64(artifact.mesh().faces.size());
    for (const auto &face : artifact.mesh().faces) {
      w.u64(face.size());
      for (const auto index : face)
        w.u64(static_cast<std::uint64_t>(index));
    }
    w.u64(artifact.topology_report().vertex_count);
    w.u64(artifact.topology_report().edge_count);
    w.u64(artifact.topology_report().facet_count);
    w.u64(artifact.topology_report().component_count);
    w.u64(artifact.geometry_report().output_precision_bits);
    w.u64(artifact.geometry_report().maximum_authorized_tolerance_bits);
    w.u64(artifact.geometry_report().maximum_realized_displacement_bits);
    bytes = w.take();
    return true;
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>>
verify_and_publish(
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    std::shared_ptr<const cleaned_triangle_manifold<T>> cleaned,
    std::shared_ptr<const assembled_output_candidate<T, I>> candidate,
    final_verification_capabilities capabilities) {
  try {
    final_verification_builder<T, I> builder(context, precision,
                                             std::move(cleaned),
                                             std::move(candidate),
                                             std::move(capabilities));
    return builder.run();
  } catch (const std::bad_alloc &) {
    return boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>>::
        failure(final_verification_error(
            final_verification_subcode::resource_preflight,
            bounded_boolean_error_category::resource_limit,
            "final verification allocation failed",
            final_verification_checkpoint::count_preflight));
  } catch (...) {
    return boolean_outcome<std::shared_ptr<const verified_boolean_result<T, I>>>::
        failure(final_verification_error(
            final_verification_subcode::internal_invariant,
            bounded_boolean_error_category::internal_invariant_error,
            "final verification unexpected exception",
            final_verification_checkpoint::commit));
  }
}

} // namespace ygor::mesh_boolean::bounded
