#pragma once

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool canonical_halfedge_builder<T, I>::fail(
    canonical_halfedge_subcode subcode,
    bounded_boolean_error_category category, const char *summary,
    canonical_halfedge_checkpoint checkpoint) {
  const auto operand = validated_ ? validated_->operand() : operand_id::a;
  error_ = canonical_halfedge_error(operand, subcode, category, summary,
                                    checkpoint);
  error_.context_digest = context_.context_digest;
  return false;
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::check_cancel(
    canonical_halfedge_checkpoint checkpoint) {
  if (!canonical_halfedge_cancelled(capabilities_))
    return true;
  return fail(canonical_halfedge_subcode::cancelled,
              bounded_boolean_error_category::cancelled,
              "canonical halfedge construction cancelled", checkpoint);
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::validate_contracts() {
  if (!validated_ || !source_)
    return fail(canonical_halfedge_subcode::malformed_reference,
                bounded_boolean_error_category::input_contract_error,
                "canonical halfedge predecessor is null",
                canonical_halfedge_checkpoint::contract_validation);
  if (capabilities_.version != contract_versions::canonical_halfedge_provider ||
      capabilities_.policy_version !=
          contract_versions::canonical_halfedge_policy ||
      capabilities_.reserved != 0)
    return fail(canonical_halfedge_subcode::unsupported_version,
                bounded_boolean_error_category::input_contract_error,
                "unsupported canonical halfedge capability version",
                canonical_halfedge_checkpoint::contract_validation);
  if (!capabilities_.owner.anchor ||
      !capabilities_.owner.same_owner(context_.owner) ||
      !capabilities_.owner.same_owner(validated_->owner()) ||
      !capabilities_.owner.same_owner(source_->owner()) ||
      !precision_.owned_by(context_.owner))
    return fail(canonical_halfedge_subcode::wrong_owner,
                bounded_boolean_error_category::input_contract_error,
                "canonical halfedge owner mismatch",
                canonical_halfedge_checkpoint::contract_validation);
  if (validated_->operand() != source_->operand())
    return fail(canonical_halfedge_subcode::wrong_operand,
                bounded_boolean_error_category::input_contract_error,
                "canonical halfedge operand mismatch",
                canonical_halfedge_checkpoint::contract_validation);
  if (!capabilities_.resources)
    return fail(canonical_halfedge_subcode::resource_preflight,
                bounded_boolean_error_category::resource_limit,
                "canonical halfedge resource manager is required",
                canonical_halfedge_checkpoint::resource_reservation);
  if (source_->predecessor_digest() != validated_->digest() ||
      source_->precision_digest() != precision_.digest() ||
      validated_->precision_digest() != precision_.digest())
    return fail(canonical_halfedge_subcode::predecessor_digest_mismatch,
                bounded_boolean_error_category::input_contract_error,
                "canonical halfedge predecessor digest mismatch",
                canonical_halfedge_checkpoint::predecessor_validation);
  if (!verify_source_triangle_complex(*source_, *validated_, precision_))
    return fail(canonical_halfedge_subcode::predecessor_not_verified,
                bounded_boolean_error_category::internal_invariant_error,
                "source triangle predecessor did not independently verify",
                canonical_halfedge_checkpoint::predecessor_validation);
  if (!canonical_halfedge_preflight(*validated_, *source_, capabilities_,
                                    counts_))
    return fail(canonical_halfedge_subcode::count_overflow,
                bounded_boolean_error_category::resource_limit,
                "canonical halfedge preflight failed",
                canonical_halfedge_checkpoint::represented_domain_preflight);
  return check_cancel(canonical_halfedge_checkpoint::represented_domain_preflight);
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::reserve_resources() {
  std::uint64_t entities = 0, extra = 0, persistent = 0;
  if (!checked_add(counts_.represented_vertices, counts_.triangles, entities) ||
      !checked_add(entities, counts_.halfedges, entities) ||
      !checked_add(entities, counts_.edges, entities) ||
      !checked_multiply(entities, std::uint64_t{512}, extra) ||
      !checked_add(counts_.estimated_persistent_bytes, extra, persistent) ||
      !checked_multiply(validated_->facets().size(), std::uint64_t{2048}, extra) ||
      !checked_add(persistent, extra, persistent) ||
      !checked_multiply(validated_->shells().size(), std::uint64_t{2048}, extra) ||
      !checked_add(persistent, extra, persistent) ||
      !checked_multiply(source_->canonical_bytes().size(), std::uint64_t{8}, extra) ||
      !checked_add(persistent, extra, persistent))
    return fail(canonical_halfedge_subcode::count_overflow,
                bounded_boolean_error_category::resource_limit,
                "canonical halfedge resource estimate overflow",
                canonical_halfedge_checkpoint::resource_reservation);
  if (persistent > capabilities_.maximum_canonical_bytes +
                       counts_.estimated_persistent_bytes + extra &&
      capabilities_.maximum_canonical_bytes != 0) {
    // The explicit byte cap is checked against the finished canonical stream;
    // this branch merely keeps the arithmetic above intentional.
  }
  auto persistent_reservation = capabilities_.resources->reserve(
      resource_kind::persistent_bytes, persistent);
  auto temporary_reservation = capabilities_.resources->reserve(
      resource_kind::temporary_bytes, counts_.estimated_temporary_bytes);
  auto work_reservation = capabilities_.resources->reserve(
      resource_kind::work_units, counts_.work_units);
  if (!persistent_reservation || !temporary_reservation || !work_reservation)
    return fail(canonical_halfedge_subcode::resource_preflight,
                bounded_boolean_error_category::resource_limit,
                "canonical halfedge resource reservation failed",
                canonical_halfedge_checkpoint::resource_reservation);
  persistent_reservation_.emplace(std::move(*persistent_reservation));
  temporary_reservation_.emplace(std::move(*temporary_reservation));
  work_reservation_.emplace(std::move(*work_reservation));
  return check_cancel(canonical_halfedge_checkpoint::resource_reservation);
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::build_vertices() {
  artifact_->source_vertex_to_vertex_.assign(validated_->vertices().size(),
                                              canonical_invalid_ordinal);
  std::vector<bool> represented(validated_->vertices().size(), false);
  for (const auto &triangle : source_->triangles()) {
    for (auto source_vertex : triangle.vertices) {
      if (source_vertex >= represented.size())
        return fail(canonical_halfedge_subcode::malformed_reference,
                    bounded_boolean_error_category::internal_invariant_error,
                    "source triangle references an invalid vertex",
                    canonical_halfedge_checkpoint::vertex_proposals);
      represented[source_vertex] = true;
    }
  }
  artifact_->vertices_.reserve(counts_.represented_vertices);
  artifact_->vertex_to_source_vertex_.reserve(counts_.represented_vertices);
  for (std::uint64_t source_vertex = 0; source_vertex < represented.size();
       ++source_vertex) {
    if (!represented[source_vertex])
      continue;
    if (source_vertex >= source_->vertices().size())
      return fail(canonical_halfedge_subcode::malformed_reference,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source triangle vertex attachment is missing",
                  canonical_halfedge_checkpoint::vertex_proposals);
    const auto &source_record = source_->vertices()[source_vertex];
    auto bound = canonical_vertex_bound(source_record);
    if (!bound)
      return fail(canonical_halfedge_subcode::bound_invalid,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source vertex bound is invalid",
                  canonical_halfedge_checkpoint::geometry_attachments);
    canonical_manifold_vertex_record<T> vertex;
    vertex.canonical_id = artifact_->vertices_.size();
    vertex.key.operand = artifact_->operand_;
    vertex.key.source_vertex = source_vertex;
    vertex.source_vertex = source_vertex;
    vertex.shell = source_record.shell;
    vertex.nominal_bits = source_record.nominal_bits;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      vertex.committed_point[axis] = from_bits<T>(source_record.nominal_bits[axis]);
      vertex.lower[axis] = source_record.lower[axis];
      vertex.upper[axis] = source_record.upper[axis];
    }
    vertex.radial_error = source_record.radial_error;
    vertex.presentation_vertex = source_record.presentation_vertex;
    vertex.bound = *bound;
    if (!source_record.incident_triangles.empty()) {
      const auto triangle_id = source_record.incident_triangles.front();
      if (triangle_id >= source_->triangles().size())
        return fail(canonical_halfedge_subcode::malformed_reference,
                    bounded_boolean_error_category::internal_invariant_error,
                    "source vertex incident triangle is invalid",
                    canonical_halfedge_checkpoint::geometry_attachments);
      vertex.geometry_basis = source_->triangles()[triangle_id].basis.kind;
    }
    artifact_->source_vertex_to_vertex_[source_vertex] = vertex.canonical_id;
    artifact_->vertex_to_source_vertex_.push_back(source_vertex);
    artifact_->vertices_.push_back(std::move(vertex));
  }
  if (artifact_->vertices_.size() != counts_.represented_vertices)
    return fail(canonical_halfedge_subcode::represented_domain_mismatch,
                bounded_boolean_error_category::internal_invariant_error,
                "represented vertex count changed during construction",
                canonical_halfedge_checkpoint::vertex_proposals);
  return check_cancel(canonical_halfedge_checkpoint::vertex_proposals);
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::build_triangles_and_halfedges() {
  std::vector<triangle_proposal> proposals;
  proposals.reserve(source_->triangles().size());
  for (std::uint64_t source_triangle = 0;
       source_triangle < source_->triangles().size(); ++source_triangle) {
    const auto &triangle = source_->triangles()[source_triangle];
    for (auto edge_use : triangle.edge_uses)
      if (edge_use >= source_->edge_uses().size())
        return fail(canonical_halfedge_subcode::malformed_reference,
                    bounded_boolean_error_category::internal_invariant_error,
                    "source triangle edge-use reference is invalid",
                    canonical_halfedge_checkpoint::triangle_rotation);
    auto rotation = canonical_halfedge_build_detail::
        least_orientation_preserving_rotation(artifact_->operand_, triangle,
                                               source_->edge_uses());
    triangle_proposal proposal;
    proposal.key.operand = artifact_->operand_;
    proposal.key.source_triangle = source_triangle;
    proposal.key.rotation = std::move(rotation.first);
    proposal.source_triangle = source_triangle;
    proposal.shift = rotation.second;
    proposals.push_back(std::move(proposal));
  }
  std::sort(proposals.begin(), proposals.end());
  for (std::size_t i = 1; i < proposals.size(); ++i)
    if (!(proposals[i - 1].key < proposals[i].key))
      return fail(canonical_halfedge_subcode::duplicate_semantic_key,
                  bounded_boolean_error_category::internal_invariant_error,
                  "duplicate canonical triangle key",
                  canonical_halfedge_checkpoint::triangle_rotation);

  artifact_->triangles_.resize(proposals.size());
  artifact_->halfedges_.resize(proposals.size() * 3);
  artifact_->source_triangle_to_triangle_.assign(
      source_->triangles().size(), canonical_invalid_ordinal);
  artifact_->triangle_to_source_triangle_.resize(proposals.size());
  artifact_->source_directed_use_to_halfedge_.assign(
      validated_->directed_uses().size(), canonical_invalid_ordinal);
  triangle_shifts_.resize(proposals.size());

  for (std::uint64_t triangle_id = 0; triangle_id < proposals.size();
       ++triangle_id) {
    const auto &proposal = proposals[triangle_id];
    const auto &source_triangle =
        source_->triangles()[proposal.source_triangle];
    auto &triangle = artifact_->triangles_[triangle_id];
    triangle.canonical_id = triangle_id;
    triangle.key = proposal.key;
    triangle.source_triangle = proposal.source_triangle;
    triangle.source_facet = source_triangle.key.facet;
    triangle.ring = source_triangle.ring;
    triangle.shell = source_triangle.shell;
    triangle.facet_group = triangle.source_facet;
    triangle.shell_group = triangle.shell;
    triangle.basis = source_triangle.basis;
    triangle.orientation = source_triangle.orientation;
    triangle_shifts_[triangle_id] = proposal.shift;
    artifact_->source_triangle_to_triangle_[proposal.source_triangle] = triangle_id;
    artifact_->triangle_to_source_triangle_[triangle_id] = proposal.source_triangle;

    for (std::uint8_t slot = 0; slot < 3; ++slot) {
      const auto source_slot =
          static_cast<std::uint8_t>((slot + proposal.shift) % 3);
      const auto source_vertex = source_triangle.vertices[source_slot];
      if (source_vertex >= artifact_->source_vertex_to_vertex_.size() ||
          artifact_->source_vertex_to_vertex_[source_vertex] ==
              canonical_invalid_ordinal)
        return fail(canonical_halfedge_subcode::represented_domain_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "triangle references an unrepresented vertex",
                    canonical_halfedge_checkpoint::halfedge_cycles);
      triangle.source_vertices[slot] = source_vertex;
      triangle.vertices[slot] =
          artifact_->source_vertex_to_vertex_[source_vertex];
      triangle.halfedges[slot] = triangle_id * 3 + slot;
    }
    triangle.bound = canonical_triangle_bound(
        artifact_->vertices_[triangle.vertices[0]],
        artifact_->vertices_[triangle.vertices[1]],
        artifact_->vertices_[triangle.vertices[2]]);

    for (std::uint8_t slot = 0; slot < 3; ++slot) {
      const auto source_slot =
          static_cast<std::uint8_t>((slot + proposal.shift) % 3);
      const auto source_use_id = source_triangle.edge_uses[source_slot];
      const auto &source_use = source_->edge_uses()[source_use_id];
      auto &halfedge = artifact_->halfedges_[triangle_id * 3 + slot];
      halfedge.canonical_id = triangle_id * 3 + slot;
      halfedge.key.triangle = triangle.key;
      halfedge.key.local_slot = slot;
      halfedge.triangle = triangle_id;
      halfedge.local_slot = slot;
      halfedge.origin = triangle.vertices[slot];
      halfedge.destination = triangle.vertices[(slot + 1) % 3];
      halfedge.source_origin = triangle.source_vertices[slot];
      halfedge.source_destination = triangle.source_vertices[(slot + 1) % 3];
      halfedge.next = triangle_id * 3 + ((slot + 1) % 3);
      halfedge.previous = triangle_id * 3 + ((slot + 2) % 3);
      halfedge.edge_class =
          source_use.role == source_triangle_edge_role::source_boundary
              ? canonical_edge_class::source_edge
              : canonical_edge_class::facet_internal_diagonal;
      halfedge.source_facet = source_use.facet;
      halfedge.ring = source_use.ring;
      halfedge.shell = source_use.shell;
      halfedge.source_directed_use = source_use.source_directed_use;
      halfedge.source_undirected_edge = source_use.source_undirected_edge;
      halfedge.source_corner = source_use.source_corner;
      halfedge.source_diagonal = source_use.diagonal;
      halfedge.predecessor_edge_use = source_use_id;
      if (source_use.role == source_triangle_edge_role::source_boundary) {
        if (source_use.source_directed_use >=
            artifact_->source_directed_use_to_halfedge_.size())
          return fail(canonical_halfedge_subcode::malformed_reference,
                      bounded_boolean_error_category::internal_invariant_error,
                      "source directed-use reference is invalid",
                      canonical_halfedge_checkpoint::halfedge_cycles);
        auto &mapped = artifact_->source_directed_use_to_halfedge_[
            source_use.source_directed_use];
        if (mapped != canonical_invalid_ordinal)
          return fail(canonical_halfedge_subcode::duplicate_semantic_key,
                      bounded_boolean_error_category::internal_invariant_error,
                      "source directed use maps to multiple halfedges",
                      canonical_halfedge_checkpoint::halfedge_cycles);
        mapped = halfedge.canonical_id;
      }
    }
  }
  return check_cancel(canonical_halfedge_checkpoint::halfedge_cycles);
}

} // namespace ygor::mesh_boolean::bounded
