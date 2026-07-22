#pragma once

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool canonical_halfedge_builder<T, I>::build_feature_groups() {
  if (source_->facets().size() != validated_->facets().size())
    return fail(canonical_halfedge_subcode::facet_group_mismatch,
                bounded_boolean_error_category::internal_invariant_error,
                "source facet domains disagree",
                canonical_halfedge_checkpoint::facet_groups);
  artifact_->facet_groups_.resize(validated_->facets().size());
  artifact_->source_facet_to_group_.resize(validated_->facets().size());
  for (std::uint64_t facet = 0; facet < validated_->facets().size(); ++facet) {
    auto &group = artifact_->facet_groups_[facet];
    const auto &validated_facet = validated_->facets()[facet];
    const auto &source_facet = source_->facets()[facet];
    group.canonical_id = facet;
    group.source_facet = facet;
    group.ring = source_facet.ring;
    group.shell = source_facet.shell;
    group.source_vertices = validated_facet.vertices;
    group.source_semantic_digest = source_facet.semantic_digest;
    group.exact_triangulation_digest = source_facet.exact_digest;
    artifact_->source_facet_to_group_[facet] = facet;
    for (const auto &triangle : artifact_->triangles_) {
      if (triangle.source_facet != facet)
        continue;
      group.triangles.push_back(triangle.canonical_id);
      group.vertices.insert(group.vertices.end(), triangle.vertices.begin(),
                            triangle.vertices.end());
    }
    if (group.triangles.empty())
      return fail(canonical_halfedge_subcode::facet_group_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "source facet has no canonical triangles",
                  canonical_halfedge_checkpoint::facet_groups);
    canonical_sort_unique(group.vertices);
    group.basis = artifact_->triangles_[group.triangles.front()].basis;
    group.bound = artifact_->triangles_[group.triangles.front()].bound;
    for (std::size_t i = 1; i < group.triangles.size(); ++i)
      group.bound = canonical_bound_hull(
          group.bound, artifact_->triangles_[group.triangles[i]].bound);
    for (const auto &edge : artifact_->edges_)
      if (edge.edge_class == canonical_edge_class::facet_internal_diagonal &&
          edge.source_facet == facet)
        group.internal_edges.push_back(edge.canonical_id);
    for (const auto &halfedge : artifact_->halfedges_)
      if (halfedge.edge_class == canonical_edge_class::source_edge &&
          halfedge.source_facet == facet)
        group.boundary_halfedges.push_back(halfedge.canonical_id);
    canonical_sort_unique(group.internal_edges);
    canonical_sort_unique(group.boundary_halfedges);
    canonical_writer writer;
    writer.u16(contract_versions::canonical_halfedge_operand_schema);
    writer.u64(facet);
    writer.u64(group.ring);
    writer.u64(group.shell);
    for (auto value : group.source_vertices)
      writer.u64(value);
    for (auto value : group.triangles)
      writer.u64(value);
    for (auto value : group.internal_edges)
      writer.u64(value);
    for (auto value : group.boundary_halfedges)
      writer.u64(value);
    canonical_halfedge_build_detail::write_digest(writer,
                                                   group.source_semantic_digest);
    canonical_halfedge_build_detail::write_digest(
        writer, group.exact_triangulation_digest);
    group.digest = sha256::digest(writer.bytes());
  }

  artifact_->shell_groups_.resize(validated_->shells().size());
  artifact_->source_shell_to_group_.resize(validated_->shells().size());
  for (std::uint64_t shell = 0; shell < validated_->shells().size(); ++shell) {
    auto &group = artifact_->shell_groups_[shell];
    const auto &predecessor = validated_->shells()[shell];
    group.canonical_id = shell;
    group.source_shell = shell;
    group.facets = predecessor.facets;
    group.intrinsic_orientation = predecessor.intrinsic_orientation;
    group.parent = predecessor.parent;
    group.depth = predecessor.depth;
    group.material_side = predecessor.material_side;
    group.empty_side = predecessor.empty_side;
    artifact_->source_shell_to_group_[shell] = shell;
    for (const auto &triangle : artifact_->triangles_) {
      if (triangle.shell != shell)
        continue;
      group.triangles.push_back(triangle.canonical_id);
      group.vertices.insert(group.vertices.end(), triangle.vertices.begin(),
                            triangle.vertices.end());
    }
    canonical_sort_unique(group.vertices);
    for (const auto &edge : artifact_->edges_) {
      const auto &h0 = artifact_->halfedges_[edge.halfedges[0]];
      const auto &h1 = artifact_->halfedges_[edge.halfedges[1]];
      if (h0.shell != h1.shell)
        return fail(canonical_halfedge_subcode::shell_group_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "canonical edge crosses source shell identity",
                    canonical_halfedge_checkpoint::shell_groups);
      if (h0.shell != shell)
        continue;
      group.edges.push_back(edge.canonical_id);
      if (edge.edge_class == canonical_edge_class::source_edge)
        group.source_edges.push_back(edge.source_undirected_edge);
    }
    canonical_sort_unique(group.source_edges);
    canonical_sort_unique(group.edges);
    if (!group.triangles.empty()) {
      group.bound = artifact_->triangles_[group.triangles.front()].bound;
      for (std::size_t i = 1; i < group.triangles.size(); ++i)
        group.bound = canonical_bound_hull(
            group.bound, artifact_->triangles_[group.triangles[i]].bound);
    }
    canonical_writer writer;
    writer.u16(contract_versions::canonical_source_manifolds_schema);
    writer.u64(shell);
    writer.u8(static_cast<std::uint8_t>(group.intrinsic_orientation));
    writer.u64(static_cast<std::uint64_t>(group.parent));
    writer.u32(group.depth);
    writer.u8(static_cast<std::uint8_t>(group.material_side));
    writer.u8(static_cast<std::uint8_t>(group.empty_side));
    for (auto value : group.vertices)
      writer.u64(value);
    for (auto value : group.source_edges)
      writer.u64(value);
    for (auto value : group.facets)
      writer.u64(value);
    for (auto value : group.triangles)
      writer.u64(value);
    for (auto value : group.edges)
      writer.u64(value);
    group.digest = sha256::digest(writer.bytes());
  }
  for (auto &vertex : artifact_->vertices_) {
    if (vertex.shell >= artifact_->shell_groups_.size())
      return fail(canonical_halfedge_subcode::shell_group_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "canonical vertex source shell is invalid",
                  canonical_halfedge_checkpoint::shell_groups);
    vertex.shell_group = vertex.shell;
  }
  return check_cancel(canonical_halfedge_checkpoint::shell_groups);
}

template <class T, class I>
std::uint64_t canonical_halfedge_builder<T, I>::persistent_bytes() const
    noexcept {
  if (!artifact_)
    return 0;
  std::uint64_t bytes = sizeof(*artifact_);
  auto add = [&](std::uint64_t count, std::uint64_t size) {
    std::uint64_t amount = 0;
    return checked_multiply(count, size, amount) &&
           checked_add(bytes, amount, bytes);
  };
  if (!add(artifact_->vertices_.size(),
           sizeof(canonical_manifold_vertex_record<T>)) ||
      !add(artifact_->triangles_.size(),
           sizeof(canonical_manifold_triangle_record<T>)) ||
      !add(artifact_->halfedges_.size(),
           sizeof(canonical_manifold_halfedge_record)) ||
      !add(artifact_->edges_.size(), sizeof(canonical_manifold_edge_record<T>)) ||
      !add(artifact_->fans_.size(), sizeof(canonical_vertex_fan_record)) ||
      !add(artifact_->facet_groups_.size(),
           sizeof(canonical_source_facet_group_record<T>)) ||
      !add(artifact_->shell_groups_.size(),
           sizeof(canonical_source_shell_group_record<T>)))
    return std::numeric_limits<std::uint64_t>::max();
  for (const auto &fan : artifact_->fans_)
    if (!add(fan.outgoing_halfedges.size(), sizeof(std::uint64_t)))
      return std::numeric_limits<std::uint64_t>::max();
  for (const auto &group : artifact_->facet_groups_)
    if (!add(group.source_vertices.size(), sizeof(std::uint64_t)) ||
        !add(group.vertices.size(), sizeof(std::uint64_t)) ||
        !add(group.triangles.size(), sizeof(std::uint64_t)) ||
        !add(group.internal_edges.size(), sizeof(std::uint64_t)) ||
        !add(group.boundary_halfedges.size(), sizeof(std::uint64_t)))
      return std::numeric_limits<std::uint64_t>::max();
  for (const auto &group : artifact_->shell_groups_)
    if (!add(group.vertices.size(), sizeof(std::uint64_t)) ||
        !add(group.source_edges.size(), sizeof(std::uint64_t)) ||
        !add(group.facets.size(), sizeof(std::uint64_t)) ||
        !add(group.triangles.size(), sizeof(std::uint64_t)) ||
        !add(group.edges.size(), sizeof(std::uint64_t)))
      return std::numeric_limits<std::uint64_t>::max();
  const std::array<const std::vector<std::uint64_t> *, 9> maps{{
      &artifact_->source_vertex_to_vertex_,
      &artifact_->vertex_to_source_vertex_,
      &artifact_->source_triangle_to_triangle_,
      &artifact_->triangle_to_source_triangle_,
      &artifact_->source_directed_use_to_halfedge_,
      &artifact_->source_edge_to_edge_,
      &artifact_->source_diagonal_to_edge_,
      &artifact_->source_facet_to_group_,
      &artifact_->source_shell_to_group_}};
  for (const auto *map : maps)
    if (!add(map->size(), sizeof(std::uint64_t)))
      return std::numeric_limits<std::uint64_t>::max();
  if (!add(artifact_->source_semantic_bytes_.size(), sizeof(std::uint8_t)) ||
      !add(artifact_->exact_topology_bytes_.size(), sizeof(std::uint8_t)) ||
      !add(artifact_->geometry_attachment_bytes_.size(), sizeof(std::uint8_t)) ||
      !add(artifact_->canonical_bytes_.size(), sizeof(std::uint8_t)))
    return std::numeric_limits<std::uint64_t>::max();
  return bytes;
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::finalize_and_verify() {
  artifact_->statistics_.represented_vertices = artifact_->vertices_.size();
  artifact_->statistics_.isolated_vertices_excluded =
      validated_->vertices().size() - artifact_->vertices_.size();
  artifact_->statistics_.triangles = artifact_->triangles_.size();
  artifact_->statistics_.halfedges = artifact_->halfedges_.size();
  artifact_->statistics_.source_edges = validated_->edges().size();
  artifact_->statistics_.internal_diagonals = source_->diagonals().size();
  artifact_->statistics_.fan_transitions = artifact_->halfedges_.size();
  std::uint64_t levels = 0;
  for (std::uint64_t n = artifact_->halfedges_.size(); n > 1; n >>= 1)
    ++levels;
  if (!checked_multiply(artifact_->halfedges_.size(), levels,
                        artifact_->statistics_.pairing_comparisons))
    return fail(canonical_halfedge_subcode::count_overflow,
                bounded_boolean_error_category::resource_limit,
                "canonical pairing comparison count overflow",
                canonical_halfedge_checkpoint::producer_verification);
  artifact_->statistics_.verifier_work =
      artifact_->vertices_.size() + artifact_->triangles_.size() +
      artifact_->halfedges_.size() + artifact_->edges_.size();
  artifact_->statistics_.work_units = counts_.work_units;

  canonical_writer replay_writer;
  replay_writer.u16(contract_versions::canonical_halfedge_operand_schema);
  replay_writer.u8(static_cast<std::uint8_t>(artifact_->operand_));
  canonical_halfedge_build_detail::write_digest(
      replay_writer, validated_->source_presentation_digest());
  canonical_halfedge_build_detail::write_digest(
      replay_writer, source_->replay_presentation_digest());
  artifact_->replay_presentation_digest_ =
      sha256::digest(replay_writer.bytes());

  artifact_->verification_ =
      canonical_halfedge_verification_disposition::independently_verified;
  artifact_->source_semantic_bytes_ =
      encode_canonical_halfedge_source_semantics(*artifact_);
  artifact_->exact_topology_bytes_ =
      encode_canonical_halfedge_exact_topology(*artifact_);
  artifact_->geometry_attachment_bytes_ =
      encode_canonical_halfedge_geometry_attachments(*artifact_);
  artifact_->source_semantic_digest_ =
      sha256::digest(artifact_->source_semantic_bytes_);
  artifact_->exact_topology_digest_ =
      sha256::digest(artifact_->exact_topology_bytes_);
  artifact_->geometry_attachment_digest_ =
      sha256::digest(artifact_->geometry_attachment_bytes_);
  artifact_->canonical_bytes_ = encode_canonical_halfedge_complete(
      *artifact_, artifact_->source_semantic_bytes_,
      artifact_->exact_topology_bytes_, artifact_->geometry_attachment_bytes_);
  artifact_->digest_ = sha256::digest(artifact_->canonical_bytes_);
  if (artifact_->canonical_bytes_.size() >
      capabilities_.maximum_canonical_bytes)
    return fail(canonical_halfedge_subcode::resource_preflight,
                bounded_boolean_error_category::resource_limit,
                "canonical halfedge byte stream exceeds configured limit",
                canonical_halfedge_checkpoint::canonical_encoding);
  if (encode_canonical_halfedge_operand_independent(*artifact_) !=
      artifact_->canonical_bytes_)
    return fail(canonical_halfedge_subcode::canonical_bytes_mismatch,
                bounded_boolean_error_category::internal_invariant_error,
                "independent canonical halfedge encoder disagrees",
                canonical_halfedge_checkpoint::independent_verification);
  std::uint32_t finding = 0;
  if (!verify_canonical_halfedge_operand(*artifact_, *validated_, *source_,
                                         precision_, &finding)) {
    fail(canonical_halfedge_subcode::verifier_rejection,
         bounded_boolean_error_category::internal_invariant_error,
         "independent canonical halfedge verifier rejected artifact",
         canonical_halfedge_checkpoint::independent_verification);
    error_.witnesses[0] = finding;
    error_.witness_count = 1;
    return false;
  }
  const auto used = persistent_bytes();
  if (used == std::numeric_limits<std::uint64_t>::max() ||
      !persistent_reservation_ || used > persistent_reservation_->amount() ||
      !work_reservation_ ||
      artifact_->statistics_.work_units > work_reservation_->amount())
    return fail(canonical_halfedge_subcode::resource_preflight,
                bounded_boolean_error_category::resource_limit,
                "canonical halfedge actual resource use exceeds reservation",
                canonical_halfedge_checkpoint::resource_reconciliation);
  artifact_->statistics_.persistent_bytes = used;
  if (!work_reservation_->commit(artifact_->statistics_.work_units) ||
      !persistent_reservation_->commit(used))
    return fail(canonical_halfedge_subcode::transaction_failure,
                bounded_boolean_error_category::internal_invariant_error,
                "canonical halfedge resource commit failed",
                canonical_halfedge_checkpoint::commit);
  if (temporary_reservation_)
    temporary_reservation_->release();
  return true;
}

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_halfedge_operand<T, I>>>
canonical_halfedge_builder<T, I>::run() {
  try {
    if (!validate_contracts() || !reserve_resources())
      return boolean_outcome<
          std::shared_ptr<const canonical_halfedge_operand<T, I>>>::failure(
          error_);
    artifact_ = std::make_unique<canonical_halfedge_operand<T, I>>();
    artifact_->operand_ = validated_->operand();
    artifact_->owner_ = context_.owner;
    artifact_->validated_operand_digest_ = validated_->digest();
    artifact_->source_triangle_complex_digest_ = source_->digest();
    artifact_->precision_digest_ = precision_.digest();
    artifact_->validated_ = validated_;
    artifact_->source_triangles_ = source_;
    if (!build_vertices() || !build_triangles_and_halfedges() ||
        !build_pairs_and_edges() || !build_vertex_fans() ||
        !build_feature_groups() || !finalize_and_verify())
      return boolean_outcome<
          std::shared_ptr<const canonical_halfedge_operand<T, I>>>::failure(
          error_);
    auto published =
        std::shared_ptr<const canonical_halfedge_operand<T, I>>(
            artifact_.release());
    return boolean_outcome<
        std::shared_ptr<const canonical_halfedge_operand<T, I>>>::success(
        std::move(published));
  } catch (const std::bad_alloc &) {
    fail(canonical_halfedge_subcode::resource_preflight,
         bounded_boolean_error_category::resource_limit,
         "canonical halfedge allocation failed",
         canonical_halfedge_checkpoint::resource_reconciliation);
  } catch (...) {
    fail(canonical_halfedge_subcode::transaction_failure,
         bounded_boolean_error_category::internal_invariant_error,
         "canonical halfedge construction threw unexpectedly",
         canonical_halfedge_checkpoint::commit);
  }
  return boolean_outcome<
      std::shared_ptr<const canonical_halfedge_operand<T, I>>>::failure(error_);
}

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_source_manifolds<T, I>>>
build_canonical_source_manifolds(
    std::shared_ptr<const validated_operand<T, I>> validated_a,
    std::shared_ptr<const validated_operand<T, I>> validated_b,
    std::shared_ptr<const source_triangle_complex<T, I>> source_a,
    std::shared_ptr<const source_triangle_complex<T, I>> source_b,
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    canonical_halfedge_capabilities capabilities) {
  auto a = build_canonical_halfedge_operand(
      std::move(validated_a), std::move(source_a), context, precision,
      capabilities);
  if (!a.has_value())
    return boolean_outcome<
        std::shared_ptr<const canonical_source_manifolds<T, I>>>::failure(
        *a.error());
  auto b = build_canonical_halfedge_operand(
      std::move(validated_b), std::move(source_b), context, precision,
      capabilities);
  if (!b.has_value())
    return boolean_outcome<
        std::shared_ptr<const canonical_source_manifolds<T, I>>>::failure(
        *b.error());
  auto result = std::make_shared<canonical_source_manifolds<T, I>>();
  result->owner_ = context.owner;
  result->a_ = *a.value();
  result->b_ = *b.value();
  canonical_writer writer;
  writer.u32(0x354d5343U); // CSM5
  writer.u16(contract_versions::canonical_source_manifolds_schema);
  writer.sized_bytes(result->a_->canonical_bytes());
  writer.sized_bytes(result->b_->canonical_bytes());
  result->canonical_bytes_ = writer.take();
  result->digest_ = sha256::digest(result->canonical_bytes_);
  std::shared_ptr<const canonical_source_manifolds<T, I>> published = result;
  return boolean_outcome<
      std::shared_ptr<const canonical_source_manifolds<T, I>>>::success(
      std::move(published));
}

} // namespace ygor::mesh_boolean::bounded
