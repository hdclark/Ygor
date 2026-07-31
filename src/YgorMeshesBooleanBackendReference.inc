encode_reference_payload(const backend_diagnostic_payload &payload) {
  canonical_encoder encoder;
  encoder.u16(payload.schema);
  encoder.string(payload.workload_profile);
  encode_digest(encoder, payload.request_digest);
  encoder.byte(static_cast<std::uint8_t>(payload.selected_operation));
  for (const auto &axis : payload.cuts) {
    encoder.u64(axis.size());
    for (const auto &value : axis)
      encode(encoder, value);
  }
  encoder.u64(payload.occupied_cells.size());
  for (const auto occupied : payload.occupied_cells)
    encoder.byte(occupied);
  encoder.u64(payload.occupied_cell_count);
  encoder.u64(payload.connected_components);
  encoder.u64(payload.boundary_rectangles);
  encode(encoder, payload.exact_volume);
  return encoder.bytes();
}

template <class T, class I>
product_status_or<backend_diagnostic_payload>
compute_reference_payload(const backend_request<T, I> &request,
                          bool honor_cancellation) {
  try {
    if (request.product_options.qualification.workload_profile !=
            axis_aligned_box_workload_profile ||
        request.product_options.preparation.mode !=
            preparation_mode::strict_validation ||
        request.product_options.result.representation !=
            result_representation::exact_in_T_mesh)
      return error(product_error_code::backend_capability_mismatch,
                   "axis_box.workload_profile");
    if (honor_cancellation && request.cancellation &&
        request.cancellation->token().cancelled())
      return error(product_error_code::resource_limit,
                   "axis_box.cancelled");

    auto a = extract_axis_aligned_box(request.operand_a.mesh());
    if (!a.has_value())
      return a.error();
    auto b = extract_axis_aligned_box(request.operand_b.mesh());
    if (!b.has_value())
      return b.error();

    backend_diagnostic_payload result;
    result.workload_profile = axis_aligned_box_workload_profile;
    result.request_digest = request.request_digest;
    result.selected_operation = request.selected_operation;
    for (std::size_t axis = 0; axis != 3; ++axis) {
      result.cuts[axis] = {a.value().minimum[axis], a.value().maximum[axis],
                           b.value().minimum[axis], b.value().maximum[axis]};
      sort_unique_exact(result.cuts[axis]);
    }
    const std::size_t nx = result.cuts[0].size() - 1;
    const std::size_t ny = result.cuts[1].size() - 1;
    const std::size_t nz = result.cuts[2].size() - 1;
    std::uint64_t cells_xy = 0, cells = 0;
    if (!checked_multiply_size(nx, ny, cells_xy) ||
        !checked_multiply_size(cells_xy, nz, cells) ||
        cells > request.limits.max_diagnostic_cells ||
        cells > request.limits.max_diagnostic_records)
      return error(product_error_code::resource_limit,
                   "axis_box.cell_limit");
    result.occupied_cells.assign(static_cast<std::size_t>(cells), 0);
    for (std::size_t x = 0; x != nx; ++x)
      for (std::size_t y = 0; y != ny; ++y)
        for (std::size_t z = 0; z != nz; ++z) {
          if (honor_cancellation && request.cancellation &&
              request.cancellation->token().cancelled())
            return error(product_error_code::resource_limit,
                         "axis_box.cancelled");
          std::array<exact_scalar, 3> midpoint{{
              (result.cuts[0][x] + result.cuts[0][x + 1]) / exact_scalar(2),
              (result.cuts[1][y] + result.cuts[1][y + 1]) / exact_scalar(2),
              (result.cuts[2][z] + result.cuts[2][z + 1]) / exact_scalar(2)}};
          const bool occupied = operation_value(
              request.selected_operation, point_in_box(midpoint, a.value()),
              point_in_box(midpoint, b.value()));
          const auto index = cell_index(x, y, z, ny, nz);
          result.occupied_cells[index] = occupied ? 1U : 0U;
          if (occupied) {
            ++result.occupied_cell_count;
            result.exact_volume =
                result.exact_volume +
                (result.cuts[0][x + 1] - result.cuts[0][x]) *
                    (result.cuts[1][y + 1] - result.cuts[1][y]) *
                    (result.cuts[2][z + 1] - result.cuts[2][z]);
          }
        }

    std::vector<std::uint8_t> visited(result.occupied_cells.size(), 0);
    const std::array<std::array<int, 3>, 6> directions{{
        {{-1, 0, 0}}, {{1, 0, 0}}, {{0, -1, 0}},
        {{0, 1, 0}},  {{0, 0, -1}}, {{0, 0, 1}}}};
    for (std::size_t x = 0; x != nx; ++x)
      for (std::size_t y = 0; y != ny; ++y)
        for (std::size_t z = 0; z != nz; ++z) {
          const auto start = cell_index(x, y, z, ny, nz);
          if (!result.occupied_cells[start] || visited[start])
            continue;
          ++result.connected_components;
          std::queue<std::array<std::size_t, 3>> pending;
          pending.push({{x, y, z}});
          visited[start] = 1;
          while (!pending.empty()) {
            const auto current = pending.front();
            pending.pop();
            for (const auto &direction : directions) {
              const auto xx = static_cast<std::int64_t>(current[0]) + direction[0];
              const auto yy = static_cast<std::int64_t>(current[1]) + direction[1];
              const auto zz = static_cast<std::int64_t>(current[2]) + direction[2];
              if (xx < 0 || yy < 0 || zz < 0 ||
                  xx >= static_cast<std::int64_t>(nx) ||
                  yy >= static_cast<std::int64_t>(ny) ||
                  zz >= static_cast<std::int64_t>(nz)) {
                ++result.boundary_rectangles;
                continue;
              }
              const auto neighbor = cell_index(
                  static_cast<std::size_t>(xx), static_cast<std::size_t>(yy),
                  static_cast<std::size_t>(zz), ny, nz);
              if (!result.occupied_cells[neighbor]) {
                ++result.boundary_rectangles;
                continue;
              }
              if (!visited[neighbor]) {
                visited[neighbor] = 1;
                pending.push({{static_cast<std::size_t>(xx),
                               static_cast<std::size_t>(yy),
                               static_cast<std::size_t>(zz)}});
              }
            }
          }
        }
    result.canonical_bytes = encode_reference_payload(result);
    if (result.canonical_bytes.size() > request.limits.max_diagnostic_bytes)
      return error(product_error_code::resource_limit,
                   "axis_box.byte_limit");
    result.semantic_digest = domain_digest(reference_tag, result.canonical_bytes);
    return result;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "axis_box.reference_allocation");
  } catch (const std::exception &exception) {
    auto failure = error(product_error_code::internal_invariant_error,
                         "axis_box.reference_exception");
    failure.detail = exception.what();
    return failure;
  }
}

std::vector<std::uint8_t>
encode_attempt_body(const backend_identity &identity, backend_adapter_role role,
                    const digest &request, const digest &payload) {
  canonical_encoder encoder;
  encoder.u16(backend_adapter_schema);
  encode_identity(encoder, identity);
  encoder.byte(static_cast<std::uint8_t>(role));
  encode_digest(encoder, request);
  encode_digest(encoder, payload);
  return encoder.bytes();
}

backend_attempt_summary summary_from_failure(const backend_identity &identity,
                                             backend_adapter_role role,
                                             product_error failure) {
  backend_attempt_summary summary;
  summary.requested_backend = identity.id;
  summary.backend = identity;
  summary.role = role;
  summary.failure = std::move(failure);
  canonical_encoder encoder;
  encoder.u16(summary.schema);
  encode_identity(encoder, summary.backend);
  encoder.byte(static_cast<std::uint8_t>(summary.role));
  encoder.boolean(false);
  encode_product_error(encoder, *summary.failure);
  summary.attempt_digest = domain_digest(attempt_tag, encoder.bytes());
  return summary;
}

backend_attempt_summary summary_from_unavailable(backend_id id,
                                                 product_error failure) {
  backend_attempt_summary summary;
  summary.requested_backend = id;
  summary.identity_available = false;
  summary.role = backend_adapter_role::producer;
  summary.failure = std::move(failure);
  canonical_encoder encoder;
  encoder.u16(summary.schema);
  encoder.u16(static_cast<std::uint16_t>(id));
  encoder.boolean(false);
  encoder.byte(static_cast<std::uint8_t>(summary.role));
  encode_product_error(encoder, *summary.failure);
  summary.attempt_digest = domain_digest(attempt_tag, encoder.bytes());
  return summary;
}

template <class T, class I>
backend_attempt_summary summary_from_attempt(const backend_attempt<T, I> &attempt) {
  backend_attempt_summary summary;
  summary.requested_backend = attempt.backend.id;
  summary.backend = attempt.backend;
  summary.role = attempt.role;
  summary.succeeded = true;
  if (attempt.product && *attempt.product)
    summary.payload_digest = (*attempt.product)->exact_result->canonical_digest;
  else if (attempt.diagnostic)
    summary.payload_digest = attempt.diagnostic->semantic_digest;
  summary.attempt_digest = attempt.attempt_digest;
  return summary;
}

std::vector<std::uint8_t>
encode_comparison_body(const backend_comparison_record &comparison) {
  canonical_encoder encoder;
  encoder.u16(comparison.schema);
  encode_identity(encoder, comparison.producer);
  encode_identity(encoder, comparison.comparator);
  encoder.byte(static_cast<std::uint8_t>(comparison.outcome));
  encode_digest(encoder, comparison.request_digest);
  encode_digest(encoder, comparison.expected_digest);
  encode_digest(encoder, comparison.observed_digest);
  encoder.u64(comparison.mismatched_cells);
  encoder.boolean(comparison.exact_volume_matches);
  encoder.boolean(comparison.component_count_matches);
  encoder.boolean(comparison.output_bounds_match);
  encoder.string(comparison.message_key);
  return encoder.bytes();
}

product_status_or<bool>
validate_comparison(const backend_comparison_record &comparison,
                    const digest &request) noexcept {
  try {
    if (comparison.schema != backend_adapter_schema ||
        comparison.request_digest != request ||
        product_digest_is_zero(comparison.expected_digest) ||
        product_digest_is_zero(comparison.observed_digest) ||
        !backend_comparison_digest(comparison).has_value() ||
        comparison.report_digest != backend_comparison_digest(comparison).value())
      return error(product_error_code::stale_binding,
                   "backend.comparison_binding");
    if (comparison.outcome == backend_comparison_outcome::agree &&
        (comparison.mismatched_cells != 0 ||
         !comparison.exact_volume_matches ||
         !comparison.component_count_matches ||
         !comparison.output_bounds_match))
      return error(product_error_code::backend_disagreement,
                   "backend.comparison_false_agreement");
    return true;
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.comparison_validation_exception");
  }
}

template <class T, class I>
product_status_or<backend_comparison_record> compare_reference_to_product(
    const backend_request<T, I> &request,
    const backend_attempt<T, I> &producer,
    const backend_attempt<T, I> &reference) noexcept {
  try {
    if (!producer.product || !*producer.product || !reference.diagnostic)
      return error(product_error_code::stale_binding,
                   "axis_box.comparison_payload");
    backend_comparison_record comparison;
    comparison.producer = producer.backend;
    comparison.comparator = reference.backend;
    comparison.request_digest = request.request_digest;
    comparison.expected_digest = reference.diagnostic->semantic_digest;
    comparison.message_key = "axis_box.comparison";

    const auto &product = **producer.product;
    if (product.representation != result_representation::exact_in_T_mesh ||
        !product.mesh || !product.mesh->success) {
      canonical_encoder observed;
      observed.string("producer_mesh_unavailable");
      comparison.observed_digest = domain_digest(observed_tag, observed.bytes());
      comparison.outcome = backend_comparison_outcome::unsupported;
      comparison.message_key = "axis_box.producer_mesh_unavailable";
      comparison.report_digest =
          domain_digest(comparison_tag, encode_comparison_body(comparison));
      return comparison;
    }
    const auto &mesh = product.mesh->success->mesh;
    std::vector<exact_point3> vertices;
    vertices.reserve(mesh.vertices.size());
    bool bounds_match = true;
    const auto &cuts = reference.diagnostic->cuts;
    for (const auto &vertex : mesh.vertices) {
      exact_point3 exact{decode_value(vertex.x), decode_value(vertex.y),
                         decode_value(vertex.z)};
      vertices.push_back(exact);
      const std::array<exact_scalar, 3> point{{exact.x, exact.y, exact.z}};
      for (std::size_t axis = 0; axis != 3; ++axis)
        if (point[axis].compare(cuts[axis].front()) < 0 ||
            point[axis].compare(cuts[axis].back()) > 0)
          bounds_match = false;
    }
    std::vector<exact_triangle3> triangles;
    triangles.reserve(mesh.faces.size());
    exact_scalar six_volume;
    for (const auto &face : mesh.faces) {
      if (face.size() != 3)
        return error(product_error_code::verifier_disagreement,
                     "axis_box.output_not_triangulated");
      for (const auto index : face)
        if (static_cast<std::uint64_t>(index) >= vertices.size())
          return error(product_error_code::stale_binding,
                       "axis_box.output_index");
      const auto &a = vertices[static_cast<std::size_t>(face[0])];
      const auto &b = vertices[static_cast<std::size_t>(face[1])];
      const auto &c = vertices[static_cast<std::size_t>(face[2])];
      triangles.push_back({a, b, c});
      six_volume = six_volume +
                   a.x * (b.y * c.z - b.z * c.y) -
                   a.y * (b.x * c.z - b.z * c.x) +
                   a.z * (b.x * c.y - b.y * c.x);
    }

    const std::size_t nx = cuts[0].size() - 1;
    const std::size_t ny = cuts[1].size() - 1;
    const std::size_t nz = cuts[2].size() - 1;
    std::vector<std::uint8_t> observed_cells(reference.diagnostic->occupied_cells.size(),
                                             0);
    for (std::size_t x = 0; x != nx; ++x)
      for (std::size_t y = 0; y != ny; ++y)
        for (std::size_t z = 0; z != nz; ++z) {
          if (request.cancellation && request.cancellation->token().cancelled())
            return error(product_error_code::resource_limit,
                         "axis_box.comparison_cancelled");
          exact_point3 midpoint{
              (cuts[0][x] + cuts[0][x + 1]) / exact_scalar(2),
              (cuts[1][y] + cuts[1][y + 1]) / exact_scalar(2),
              (cuts[2][z] + cuts[2][z + 1]) / exact_scalar(2)};
          bool occupied = false;
          if (!triangles.empty()) {
            auto location = classify_point_closed_triangle_shell(midpoint, triangles);
            if (!location.has_value())
              return error_from_boolean(location.error());
            if (location.value() == solid_point_kind::boundary)
              return error(product_error_code::backend_disagreement,
                           "axis_box.cell_probe_on_boundary");
            occupied = location.value() == solid_point_kind::inside;
          }
          const auto index = cell_index(x, y, z, ny, nz);
          observed_cells[index] = occupied ? 1U : 0U;
          if (observed_cells[index] != reference.diagnostic->occupied_cells[index])
            ++comparison.mismatched_cells;
        }

    comparison.exact_volume_matches =
        six_volume == reference.diagnostic->exact_volume * exact_scalar(6);
    comparison.component_count_matches =
        product.mesh->success->summary.components ==
        reference.diagnostic->connected_components;
    comparison.output_bounds_match = bounds_match;
    canonical_encoder observed;
    observed.u64(observed_cells.size());
    for (const auto value : observed_cells)
      observed.byte(value);
    encode(observed, six_volume);
    observed.u64(product.mesh->success->summary.components);
    observed.boolean(bounds_match);
    comparison.observed_digest = domain_digest(observed_tag, observed.bytes());
    comparison.outcome =
        comparison.mismatched_cells == 0 && comparison.exact_volume_matches &&
                comparison.component_count_matches && comparison.output_bounds_match
            ? backend_comparison_outcome::agree
            : backend_comparison_outcome::disagree;
    comparison.message_key = comparison.outcome == backend_comparison_outcome::agree
                                 ? "axis_box.agree"
                                 : "axis_box.disagree";
    comparison.report_digest =
        domain_digest(comparison_tag, encode_comparison_body(comparison));
    return comparison;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "axis_box.comparison_allocation");
  } catch (const std::exception &exception) {
    auto failure = error(product_error_code::internal_invariant_error,
                         "axis_box.comparison_exception");
    failure.detail = exception.what();
    return failure;
  }
}
