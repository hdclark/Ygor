
constexpr std::array<char, 8> request_tag{{'Y', 'G', 'B', 'B', 'R', 'Q', '0', '1'}};
constexpr std::array<char, 8> reference_tag{{'Y', 'G', 'B', 'R', 'E', 'F', '0', '1'}};
constexpr std::array<char, 8> attempt_tag{{'Y', 'G', 'B', 'B', 'A', 'T', '0', '1'}};
constexpr std::array<char, 8> comparison_tag{{'Y', 'G', 'B', 'B', 'C', 'M', '0', '1'}};
constexpr std::array<char, 8> execution_tag{{'Y', 'G', 'B', 'B', 'E', 'X', '0', '1'}};
constexpr std::array<char, 8> registry_tag{{'Y', 'G', 'B', 'B', 'R', 'G', '0', '1'}};
constexpr std::array<char, 8> observed_tag{{'Y', 'G', 'B', 'B', 'O', 'B', '0', '1'}};

bool same_version(const backend_version &a, const backend_version &b) noexcept {
  return a.major == b.major && a.minor == b.minor && a.patch == b.patch;
}

product_error error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

product_error error_from_boolean(const boolean_error &source) {
  auto result = make_product_error(promote_error_code(source.code),
                                   source.message_key, source.subcode);
  result.detail = render_error(source);
  canonical_encoder replay;
  replay.raw(source.replay.setup_digest.bytes.data(),
             source.replay.setup_digest.bytes.size());
  replay.byte_string(source.replay_payload);
  result.replay_binding_digest = domain_digest(
      {{'Y', 'G', 'B', 'B', 'E', 'R', '0', '1'}}, replay.bytes());
  return result;
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

void encode_identity(canonical_encoder &encoder,
                     const backend_identity &identity) {
  auto bytes = encode_backend_identity(identity);
  if (!bytes.has_value())
    throw std::invalid_argument("invalid backend identity");
  encoder.byte_string(bytes.value());
}

void encode_product_error(canonical_encoder &encoder,
                          const product_error &failure) {
  encoder.u16(failure.schema);
  encoder.u16(static_cast<std::uint16_t>(failure.code));
  encoder.u32(failure.subcode);
  encoder.string(failure.message_key);
  encoder.string(failure.detail);
  encoder.boolean(failure.backend.has_value());
  if (failure.backend)
    encode_identity(encoder, *failure.backend);
  encode_digest(encoder, failure.replay_binding_digest);
}

bool limits_valid(const backend_request_limits &limits) noexcept {
  return limits.schema == backend_adapter_schema &&
         limits.max_diagnostic_cells != 0 &&
         limits.max_diagnostic_records != 0 &&
         limits.max_diagnostic_bytes != 0;
}

bool requested_failure_category(const backend_selection_policy &policy,
                                product_error_code code) {
  return std::find(policy.fallback_on.begin(), policy.fallback_on.end(), code) !=
         policy.fallback_on.end();
}

bool checked_multiply_size(std::uint64_t a, std::uint64_t b,
                           std::uint64_t &result) noexcept {
  if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a)
    return false;
  result = a * b;
  return true;
}

template <class T, class I>
digest request_digest_for(const prepared_operand<T, I> &a,
                          const prepared_operand<T, I> &b, operation op,
                          const boolean_options &engine,
                          const boolean_product_options &product,
                          const backend_request_limits &limits) {
  auto engine_bytes = encode_options(engine);
  if (!engine_bytes.has_value())
    throw std::invalid_argument("invalid engine options");
  auto product_bytes = encode_product_options(product);
  if (!product_bytes.has_value())
    throw std::invalid_argument("invalid product options");
  canonical_encoder encoder;
  encoder.u16(backend_adapter_schema);
  encoder.byte(static_cast<std::uint8_t>(op));
  encode_digest(encoder, a.certificate().certificate_digest);
  encode_digest(encoder, b.certificate().certificate_digest);
  encoder.byte_string(engine_bytes.value());
  encoder.byte_string(product_bytes.value());
  encoder.u16(limits.schema);
  encoder.u64(limits.max_diagnostic_cells);
  encoder.u64(limits.max_diagnostic_records);
  encoder.u64(limits.max_diagnostic_bytes);
  return domain_digest(request_tag, encoder.bytes());
}

product_status_or<bool>
validate_execution_state(const backend_execution_state &state,
                         backend_id producer) noexcept {
  if (state.schema != backend_adapter_schema ||
      state.attempted_backends.empty() ||
      std::find(state.attempted_backends.begin(), state.attempted_backends.end(),
                producer) == state.attempted_backends.end())
    return error(product_error_code::stale_binding,
                 "backend.execution_state_binding");
  std::set<std::uint16_t> unique;
  for (const auto id : state.attempted_backends)
    if (!unique.insert(static_cast<std::uint16_t>(id)).second)
      return error(product_error_code::stale_binding,
                   "backend.execution_state_duplicates");
  if (state.primary_failure) {
    if (state.selection != backend_selection_mode::explicit_fallback_chain ||
        !fallback_permitted_for(state.primary_failure->code))
      return error(product_error_code::qualification_policy_violation,
                   "backend.execution_state_primary_failure");
  }
  return true;
}

product_status_or<bool>
validate_preparation_contract(preparation_mode mode, bool normalized_a,
                              bool normalized_b) noexcept {
  if (mode == preparation_mode::diagnosis_only)
    return error(product_error_code::normalization_required,
                 "backend.diagnosis_only_not_executable");
  if (mode == preparation_mode::strict_validation) {
    if (normalized_a || normalized_b)
      return error(product_error_code::stale_binding,
                   "backend.strict_preparation_binding");
  } else if (mode == preparation_mode::normalized) {
    if (!normalized_a || !normalized_b)
      return error(product_error_code::stale_binding,
                   "backend.normalized_preparation_binding");
  }
  return true;
}

template <class T, class I>
product_status_or<exact_result_preparation_binding>
preparation_binding_for(const backend_request<T, I> &request,
                        const boolean_context<T, I> &context) {
  if (!context.preparation_provenance())
    return error(product_error_code::stale_binding,
                 "backend.preparation_provenance_missing");
  exact_result_preparation_binding result;
  result.mode = request.product_options.preparation.mode;
  result.input_digest = context.preparation_provenance()->input_digest;
  result.prepared_digest = context.preparation_provenance()->prepared_digest;
  result.policy_digest = context.preparation_provenance()->policy_digest;
  result.report_digest = context.preparation_provenance()->report_digest;
  result.geometry_changed = context.preparation_provenance()->geometry_changed;
  return result;
}

backend_capabilities exact_backend_capabilities() {
  backend_capabilities capabilities;
  capabilities.set(backend_capability::exact_set_semantics);
  capabilities.set(backend_capability::exact_coordinates);
  capabilities.set(backend_capability::stratified_output);
  capabilities.set(backend_capability::manifold_mesh_output);
  capabilities.set(backend_capability::deterministic_canonical_output);
  capabilities.set(backend_capability::certified_failure_categories);
  capabilities.set(backend_capability::provenance_mapping);
  capabilities.set(backend_capability::strict_prepared_operands);
  capabilities.set(backend_capability::exact_in_T_output);
  capabilities.set(backend_capability::certified_approximate_output);
  return capabilities;
}

backend_capabilities reference_backend_capabilities() {
  backend_capabilities capabilities;
  capabilities.set(backend_capability::exact_set_semantics);
  capabilities.set(backend_capability::exact_coordinates);
  capabilities.set(backend_capability::deterministic_canonical_output);
  capabilities.set(backend_capability::certified_failure_categories);
  capabilities.set(backend_capability::strict_prepared_operands);
  return capabilities;
}

template <class T> exact_scalar decode_value(T value) {
  auto decoded = decode_coordinate(value, boolean_stage::final_verification);
  if (!decoded.has_value())
    throw std::invalid_argument("nonfinite backend coordinate");
  return decoded.value().value;
}

struct exact_box_fixture {
  std::array<exact_scalar, 3> minimum;
  std::array<exact_scalar, 3> maximum;
};

template <class T, class I>
product_status_or<exact_box_fixture>
extract_axis_aligned_box(const fv_surface_mesh<T, I> &mesh) {
  try {
    if (mesh.vertices.size() != 8 || mesh.faces.size() != 6)
      return error(product_error_code::backend_capability_mismatch,
                   "axis_box.shape");
    std::vector<std::array<exact_scalar, 3>> points;
    points.reserve(mesh.vertices.size());
    for (const auto &vertex : mesh.vertices)
      points.push_back({{decode_value(vertex.x), decode_value(vertex.y),
                         decode_value(vertex.z)}});
    exact_box_fixture box;
    box.minimum = points.front();
    box.maximum = points.front();
    for (const auto &point : points)
      for (std::size_t axis = 0; axis != 3; ++axis) {
        if (point[axis].compare(box.minimum[axis]) < 0)
          box.minimum[axis] = point[axis];
        if (point[axis].compare(box.maximum[axis]) > 0)
          box.maximum[axis] = point[axis];
      }
    for (std::size_t axis = 0; axis != 3; ++axis)
      if (box.minimum[axis].compare(box.maximum[axis]) >= 0)
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.degenerate_extent");

    std::array<bool, 8> corners{{false, false, false, false, false, false, false,
                                 false}};
    for (const auto &point : points) {
      unsigned mask = 0;
      for (std::size_t axis = 0; axis != 3; ++axis) {
        if (point[axis] == box.minimum[axis]) {
        } else if (point[axis] == box.maximum[axis]) {
          mask |= 1U << axis;
        } else {
          return error(product_error_code::backend_capability_mismatch,
                       "axis_box.non_corner_vertex");
        }
      }
      if (corners[mask])
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.duplicate_corner");
      corners[mask] = true;
    }
    for (const auto present : corners)
      if (!present)
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.missing_corner");

    std::array<bool, 6> planes{{false, false, false, false, false, false}};
    exact_point3 center{
        (box.minimum[0] + box.maximum[0]) / exact_scalar(2),
        (box.minimum[1] + box.maximum[1]) / exact_scalar(2),
        (box.minimum[2] + box.maximum[2]) / exact_scalar(2)};
    for (const auto &face : mesh.faces) {
      if (face.size() != 4)
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.face_cardinality");
      for (const auto index : face)
        if (static_cast<std::uint64_t>(index) >= points.size())
          return error(product_error_code::stale_binding,
                       "axis_box.face_index");
      std::optional<std::pair<std::size_t, bool>> plane;
      for (std::size_t axis = 0; axis != 3; ++axis) {
        const auto value = points[static_cast<std::size_t>(face[0])][axis];
        bool shared = true;
        for (const auto index : face)
          shared = shared && points[static_cast<std::size_t>(index)][axis] == value;
        if (!shared)
          continue;
        if (value == box.minimum[axis])
          plane = std::make_pair(axis, false);
        else if (value == box.maximum[axis])
          plane = std::make_pair(axis, true);
      }
      if (!plane)
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.nonplanar_face");
      const auto plane_index = plane->first * 2 + (plane->second ? 1U : 0U);
      if (planes[plane_index])
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.duplicate_face_plane");
      planes[plane_index] = true;

      auto point3 = [&](std::size_t position) {
        const auto &point = points[static_cast<std::size_t>(face[position])];
        return exact_point3{point[0], point[1], point[2]};
      };
      auto support = support_plane(point3(0), point3(1), point3(2));
      if (!support.has_value())
        support = support_plane(point3(0), point3(2), point3(3));
      if (!support.has_value() ||
          plane_side_exact(support.value(), center) != exact_sign::negative)
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.orientation");
    }
    for (const auto present : planes)
      if (!present)
        return error(product_error_code::backend_capability_mismatch,
                     "axis_box.missing_face_plane");
    return box;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "axis_box.allocation");
  } catch (const std::exception &exception) {
    auto failure = error(product_error_code::backend_capability_mismatch,
                         "axis_box.conversion");
    failure.detail = exception.what();
    return failure;
  }
}

void sort_unique_exact(std::vector<exact_scalar> &values) {
  std::sort(values.begin(), values.end(),
            [](const auto &a, const auto &b) { return a.compare(b) < 0; });
  values.erase(std::unique(values.begin(), values.end()), values.end());
}

bool point_in_box(const std::array<exact_scalar, 3> &point,
                  const exact_box_fixture &box) {
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (point[axis].compare(box.minimum[axis]) <= 0 ||
        point[axis].compare(box.maximum[axis]) >= 0)
      return false;
  return true;
}

std::size_t cell_index(std::size_t x, std::size_t y, std::size_t z,
                       std::size_t ny, std::size_t nz) {
  return (x * ny + y) * nz + z;
}

std::vector<std::uint8_t>
