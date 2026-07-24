product_status_or<std::vector<std::uint8_t>>
encode_product_options(const boolean_product_options &o) {
  auto valid = validate_product_options(o);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder e;
  e.u16(o.schemas.options);
  e.u16(o.schemas.artifact);
  e.u16(o.schemas.error);
  e.u16(o.schemas.certificate);
  e.u16(o.schemas.replay);
  e.u16(o.backend.schema);
  e.byte(static_cast<std::uint8_t>(o.backend.mode));
  e.boolean(o.backend.requested_backend.has_value());
  if (o.backend.requested_backend)
    e.u16(static_cast<std::uint16_t>(*o.backend.requested_backend));
  e.u64(o.backend.diagnostic_backends.size());
  for (const auto id : o.backend.diagnostic_backends)
    e.u16(static_cast<std::uint16_t>(id));
  e.u64(o.backend.fallback_chain.size());
  for (const auto id : o.backend.fallback_chain)
    e.u16(static_cast<std::uint16_t>(id));
  e.u64(o.backend.fallback_on.size());
  for (const auto code : o.backend.fallback_on)
    e.u16(static_cast<std::uint16_t>(code));
  e.boolean(o.backend.allow_experimental_backend);
  e.u16(o.preparation.schema);
  e.byte(static_cast<std::uint8_t>(o.preparation.mode));
  e.u16(o.preparation.normalization.schema);
  e.byte(static_cast<std::uint8_t>(o.preparation.normalization.mode));
  e.byte(static_cast<std::uint8_t>(o.preparation.normalization.unit));
  encode_double(e, o.preparation.normalization.model_tolerance);
  e.u64(o.preparation.normalization.enabled_operations);
  e.u16(o.result.schema);
  e.byte(static_cast<std::uint8_t>(o.result.representation));
  e.boolean(o.result.retain_exact_result_on_realization_failure);
  e.u16(o.realization.schema);
  e.byte(static_cast<std::uint8_t>(o.realization.semantics));
  e.u16(o.realization.search.schema);
  e.byte(static_cast<std::uint8_t>(o.realization.search.strategy));
  e.u64(o.realization.search.max_candidates);
  e.u64(o.realization.search.max_candidate_evaluations);
  e.u64(o.realization.search.max_search_nodes);
  e.u64(o.realization.search.max_obligations);
  e.u64(o.realization.search.max_triangle_pairs);
  e.u64(o.realization.search.max_predicate_checks);
  e.u64(o.realization.search.max_verifier_work);
  e.u64(o.realization.search.max_verifier_records);
  e.u64(o.realization.search.max_verifier_bytes);
  e.u16(o.realization.approximation.schema);
  e.boolean(o.realization.approximation.enabled);
  e.byte(static_cast<std::uint8_t>(o.realization.approximation.unit));
  encode_double(e, o.realization.approximation.max_vertex_displacement);
  e.boolean(o.realization.approximation.has_max_axis_displacement_x);
  encode_double(e, o.realization.approximation.max_axis_displacement_x);
  e.boolean(o.realization.approximation.has_max_axis_displacement_y);
  encode_double(e, o.realization.approximation.max_axis_displacement_y);
  e.boolean(o.realization.approximation.has_max_axis_displacement_z);
  encode_double(e, o.realization.approximation.max_axis_displacement_z);
  encode_double(e, o.realization.approximation.max_support_plane_deviation);
  e.boolean(o.realization.approximation.allow_original_vertex_movement);
  e.u16(o.realization.approximation.candidate_generation_version);
  e.u32(o.realization.approximation.candidate_ulp_radius);
  encode_double(e, o.realization.approximation.declared_model_tolerance);
  e.string(o.realization.approximation.application_acceptance_metadata);
  e.u16(o.attributes.schema);
  e.byte(static_cast<std::uint8_t>(o.attributes.mode));
  e.byte(static_cast<std::uint8_t>(o.attributes.conflicts));
  e.u16(o.qualification.schema);
  e.byte(static_cast<std::uint8_t>(o.qualification.mode));
  e.string(o.qualification.workload_profile);
  e.boolean(o.qualification.manifest.has_value());
  if (o.qualification.manifest) {
    e.u16(o.qualification.manifest->schema);
    e.string(o.qualification.manifest->identifier);
    encode_digest(e, o.qualification.manifest->manifest_digest);
  }
  e.boolean(o.mandatory_verification);
  return record_bytes({{'Y', 'G', 'B', 'P', 'O', 'P', '0', '6'}},
                      product_contract_schema_version, e.bytes());
}

product_status_or<boolean_product_options>
decode_product_options(const std::vector<std::uint8_t> &bytes,
                       const product_decode_limits &limits) {
  auto payload = extract_payload(
      bytes, {{'Y', 'G', 'B', 'P', 'O', 'P', '0', '6'}}, limits);
  if (!payload.has_value())
    return payload.error();
  try {
    reader r(payload.value(), limits);
    boolean_product_options o;
    o.schemas.options = r.u16();
    o.schemas.artifact = r.u16();
    o.schemas.error = r.u16();
    o.schemas.certificate = r.u16();
    o.schemas.replay = r.u16();
    o.backend.schema = r.u16();
    o.backend.mode = decode_u8_enum<backend_selection_mode>(r, known);
    if (r.boolean())
      o.backend.requested_backend = decode_backend_id(r);
    auto read_ids = [&](std::vector<backend_id> &out) {
      const auto n = r.u64();
      if (n > limits.max_vector_elements)
        throw std::runtime_error("vector_limit");
      out.reserve(static_cast<std::size_t>(n));
      for (std::uint64_t i = 0; i != n; ++i)
        out.push_back(decode_backend_id(r));
    };
    read_ids(o.backend.diagnostic_backends);
    read_ids(o.backend.fallback_chain);
    const auto errors = r.u64();
    if (errors > limits.max_vector_elements)
      throw std::runtime_error("vector_limit");
    o.backend.fallback_on.reserve(static_cast<std::size_t>(errors));
    for (std::uint64_t i = 0; i != errors; ++i)
      o.backend.fallback_on.push_back(decode_error_code(r));
    o.backend.allow_experimental_backend = r.boolean();
    o.preparation.schema = r.u16();
    o.preparation.mode = decode_u8_enum<preparation_mode>(r, known);
    o.preparation.normalization.schema = r.u16();
    o.preparation.normalization.mode =
        decode_u8_enum<normalization_mode>(r, known);
    o.preparation.normalization.unit = decode_u8_enum<model_unit>(r, known);
    o.preparation.normalization.model_tolerance = r.floating64();
    o.preparation.normalization.enabled_operations = r.u64();
    o.result.schema = r.u16();
    o.result.representation = decode_u8_enum<result_representation>(r, known);
    o.result.retain_exact_result_on_realization_failure = r.boolean();
    o.realization.schema = r.u16();
    o.realization.semantics =
        decode_u8_enum<product_realization_semantics>(r, known);
    o.realization.search.schema = r.u16();
    o.realization.search.strategy =
        decode_u8_enum<realization_search_strategy>(r, known);
    o.realization.search.max_candidates = r.u64();
    o.realization.search.max_candidate_evaluations = r.u64();
    o.realization.search.max_search_nodes = r.u64();
    o.realization.search.max_obligations = r.u64();
    o.realization.search.max_triangle_pairs = r.u64();
    o.realization.search.max_predicate_checks = r.u64();
    o.realization.search.max_verifier_work = r.u64();
    o.realization.search.max_verifier_records = r.u64();
    o.realization.search.max_verifier_bytes = r.u64();
    o.realization.approximation.schema = r.u16();
    o.realization.approximation.enabled = r.boolean();
    o.realization.approximation.unit = decode_u8_enum<model_unit>(r, known);
    o.realization.approximation.max_vertex_displacement = r.floating64();
    o.realization.approximation.has_max_axis_displacement_x = r.boolean();
    o.realization.approximation.max_axis_displacement_x = r.floating64();
    o.realization.approximation.has_max_axis_displacement_y = r.boolean();
    o.realization.approximation.max_axis_displacement_y = r.floating64();
    o.realization.approximation.has_max_axis_displacement_z = r.boolean();
    o.realization.approximation.max_axis_displacement_z = r.floating64();
    o.realization.approximation.max_support_plane_deviation = r.floating64();
    o.realization.approximation.allow_original_vertex_movement = r.boolean();
    o.realization.approximation.candidate_generation_version = r.u16();
    o.realization.approximation.candidate_ulp_radius = r.u32();
    o.realization.approximation.declared_model_tolerance = r.floating64();
    o.realization.approximation.application_acceptance_metadata = r.string();
    o.attributes.schema = r.u16();
    o.attributes.mode = decode_u8_enum<attribute_transfer_mode>(r, known);
    o.attributes.conflicts =
        decode_u8_enum<attribute_conflict_policy>(r, known);
    o.qualification.schema = r.u16();
    o.qualification.mode =
        decode_u8_enum<qualification_policy_mode>(r, known);
    o.qualification.workload_profile = r.string();
    if (r.boolean()) {
      qualification_manifest_reference ref;
      ref.schema = r.u16();
      ref.identifier = r.string();
      ref.manifest_digest = r.digest_value();
      o.qualification.manifest = std::move(ref);
    }
    o.mandatory_verification = r.boolean();
    if (r.remaining() != 0)
      throw std::runtime_error("trailing_bytes");
    auto valid = validate_product_options(o);
    if (!valid.has_value())
      return valid.error();
    return o;
  } catch (const std::exception &x) {
    auto e = make_product_error(product_error_code::input_contract_error,
                                "product_options.decode");
    e.detail = x.what();
    return e;
  }
}

digest product_options_digest(const boolean_product_options &o) {
  const auto bytes = encode_product_options(o);
  if (!bytes.has_value())
    return {};
  return domain_digest({{'Y', 'G', 'B', 'P', 'O', 'D', '0', '6'}},
                       bytes.value());
}

digest backend_capability_digest(const backend_capabilities &c) {
  canonical_encoder e;
  e.u16(c.schema);
  e.u64(c.bits);
  return domain_digest({{'Y', 'G', 'B', 'C', 'A', 'P', '0', '6'}}, e.bytes());
}
