product_status_or<backend_identity>
make_backend_identity(backend_id id, backend_version version,
                      std::string build_identifier,
                      backend_capabilities capabilities,
                      backend_maturity maturity) {
  backend_identity out;
  out.id = id;
  out.adapter_version = version;
  out.build_identifier = std::move(build_identifier);
  out.capabilities = capabilities;
  out.capability_digest = backend_capability_digest(capabilities);
  out.maturity = maturity;
  auto valid = validate_backend_identity(out);
  if (!valid.has_value())
    return valid.error();
  return out;
}

product_status_or<bool>
validate_backend_identity(const backend_identity &b) noexcept {
  if (b.schema != product_contract_schema_version ||
      b.capabilities.schema != product_contract_schema_version)
    return bad(product_error_code::stale_binding, "backend.schema");
  if (!known(b.id))
    return bad(product_error_code::backend_unavailable, "backend.id");
  if (!known(b.maturity))
    return bad(product_error_code::backend_unqualified, "backend.maturity");
  if (b.adapter_version.major == 0 || b.build_identifier.empty() ||
      b.build_identifier.size() > 256)
    return bad(product_error_code::backend_capability_mismatch,
               "backend.version_or_build");
  if ((b.capabilities.bits & ~capability_mask()) != 0 ||
      b.capability_digest != backend_capability_digest(b.capabilities))
    return bad(product_error_code::backend_capability_mismatch,
               "backend.capability_digest");
  return true;
}

product_status_or<std::vector<std::uint8_t>>
encode_backend_identity(const backend_identity &b) {
  auto valid = validate_backend_identity(b);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder e;
  e.u16(b.schema);
  e.u16(static_cast<std::uint16_t>(b.id));
  encode_backend_version(e, b.adapter_version);
  e.string(b.build_identifier);
  e.u16(b.capabilities.schema);
  e.u64(b.capabilities.bits);
  encode_digest(e, b.capability_digest);
  e.byte(static_cast<std::uint8_t>(b.maturity));
  return record_bytes({{'Y', 'G', 'B', 'B', 'I', 'D', '0', '6'}},
                      product_contract_schema_version, e.bytes());
}

product_status_or<backend_identity>
decode_backend_identity(const std::vector<std::uint8_t> &bytes,
                        const product_decode_limits &limits) {
  auto payload = extract_payload(
      bytes, {{'Y', 'G', 'B', 'B', 'I', 'D', '0', '6'}}, limits);
  if (!payload.has_value())
    return payload.error();
  try {
    reader r(payload.value(), limits);
    backend_identity out;
    out.schema = r.u16();
    out.id = decode_backend_id(r);
    out.adapter_version.major = r.u16();
    out.adapter_version.minor = r.u16();
    out.adapter_version.patch = r.u16();
    out.build_identifier = r.string();
    out.capabilities.schema = r.u16();
    out.capabilities.bits = r.u64();
    out.capability_digest = r.digest_value();
    out.maturity = decode_u8_enum<backend_maturity>(r, known);
    if (r.remaining() != 0)
      throw std::runtime_error("trailing_bytes");
    auto valid = validate_backend_identity(out);
    if (!valid.has_value())
      return valid.error();
    return out;
  } catch (const std::exception &x) {
    auto e = make_product_error(product_error_code::input_contract_error,
                                "backend.decode");
    e.detail = x.what();
    return e;
  }
}

product_status_or<qualification_manifest>
make_qualification_manifest(std::string identifier,
                            std::vector<qualification_profile> profiles) {
  if (identifier.empty())
    return make_product_error(product_error_code::qualification_policy_violation,
                              "qualification.identifier");
  std::sort(profiles.begin(), profiles.end(), profile_less);
  for (std::size_t i = 0; i != profiles.size(); ++i) {
    const auto &p = profiles[i];
    if (!known(p.backend) || !known(p.representation) || !known(p.preparation) ||
        p.workload_profile.empty() || product_digest_is_zero(p.capability_digest))
      return make_product_error(
          product_error_code::qualification_policy_violation,
          "qualification.profile");
    if (i && !profile_less(profiles[i - 1], profiles[i]))
      return make_product_error(
          product_error_code::qualification_policy_violation,
          "qualification.duplicate_profile");
  }
  qualification_manifest m;
  m.identifier = std::move(identifier);
  m.profiles = std::move(profiles);
  const auto payload = encode_manifest_payload(m.identifier, m.profiles);
  m.manifest_digest =
      domain_digest({{'Y', 'G', 'B', 'Q', 'M', 'F', '0', '6'}}, payload);
  return m;
}

product_status_or<bool>
validate_qualification_manifest(const qualification_manifest &m) noexcept {
  try {
    if (m.schema != product_contract_schema_version || m.identifier.empty())
      return bad(product_error_code::qualification_policy_violation,
                 "qualification.schema_or_identifier");
    for (std::size_t i = 0; i != m.profiles.size(); ++i) {
      const auto &p = m.profiles[i];
      if (!known(p.backend) || !known(p.representation) ||
          !known(p.preparation) || p.workload_profile.empty() ||
          product_digest_is_zero(p.capability_digest))
        return bad(product_error_code::qualification_policy_violation,
                   "qualification.profile");
      if (i && !profile_less(m.profiles[i - 1], m.profiles[i]))
        return bad(product_error_code::qualification_policy_violation,
                   "qualification.canonical_order");
    }
    const auto payload = encode_manifest_payload(m.identifier, m.profiles);
    const auto expected =
        domain_digest({{'Y', 'G', 'B', 'Q', 'M', 'F', '0', '6'}}, payload);
    if (m.manifest_digest != expected)
      return bad(product_error_code::stale_binding,
                 "qualification.manifest_digest");
    return true;
  } catch (...) {
    return bad(product_error_code::internal_invariant_error,
               "qualification.validation_exception");
  }
}

product_status_or<bool>
authorize_backend(const boolean_product_options &o, const backend_identity &b,
                  const qualification_manifest *manifest) noexcept {
  auto ov = validate_product_options(o);
  if (!ov.has_value())
    return ov.error();
  auto bv = validate_backend_identity(b);
  if (!bv.has_value())
    return bv.error();
  bool selected = false;
  switch (o.backend.mode) {
  case backend_selection_mode::explicit_backend:
  case backend_selection_mode::diagnostic_compare:
    selected = o.backend.requested_backend && *o.backend.requested_backend == b.id;
    break;
  case backend_selection_mode::explicit_fallback_chain:
    selected = std::find(o.backend.fallback_chain.begin(),
                         o.backend.fallback_chain.end(), b.id) !=
               o.backend.fallback_chain.end();
    break;
  case backend_selection_mode::qualified_default:
    selected = true;
    break;
  }
  if (!selected)
    return bad(product_error_code::backend_unavailable,
               "backend.not_selected");
  if (b.maturity == backend_maturity::deprecated)
    return bad(product_error_code::backend_unqualified,
               "backend.deprecated");
  if (!has_representation_capabilities(b, o.result.representation))
    return bad(product_error_code::backend_capability_mismatch,
               "backend.required_capabilities");

  const bool require_manifest =
      o.backend.mode == backend_selection_mode::qualified_default ||
      o.qualification.mode ==
          qualification_policy_mode::require_qualified_profile;
  if (require_manifest) {
    if (!manifest || !o.qualification.manifest)
      return bad(product_error_code::backend_unqualified,
                 "backend.qualification_manifest_required");
    auto mv = validate_qualification_manifest(*manifest);
    if (!mv.has_value())
      return mv.error();
    if (manifest->identifier != o.qualification.manifest->identifier ||
        manifest->manifest_digest !=
            o.qualification.manifest->manifest_digest)
      return bad(product_error_code::stale_binding,
                 "backend.qualification_manifest_binding");
    if (b.maturity != backend_maturity::qualified)
      return bad(product_error_code::backend_unqualified,
                 "backend.maturity_not_qualified");
    const auto match = std::find_if(
        manifest->profiles.begin(), manifest->profiles.end(),
        [&](const qualification_profile &p) {
          return p.backend == b.id &&
                 p.capability_digest == b.capability_digest &&
                 p.representation == o.result.representation &&
                 p.preparation == o.preparation.mode &&
                 p.workload_profile == o.qualification.workload_profile;
        });
    if (match == manifest->profiles.end())
      return bad(product_error_code::backend_unqualified,
                 "backend.profile_not_qualified");
    return true;
  }
  if ((b.maturity == backend_maturity::experimental ||
       b.maturity == backend_maturity::candidate) &&
      !o.backend.allow_experimental_backend)
    return bad(product_error_code::backend_unqualified,
               "backend.experimental_opt_in_required");
  return true;
}

product_status_or<bool>
validate_backend_provenance(const backend_provenance &p) noexcept {
  if (p.schema != product_contract_schema_version || !known(p.selection))
    return bad(product_error_code::stale_binding, "backend_provenance.schema");
  auto producer = validate_backend_identity(p.producer);
  if (!producer.has_value())
    return producer.error();
  if (p.attempted_backends.empty() ||
      !unique_backend_ids(p.attempted_backends) ||
      std::find(p.attempted_backends.begin(), p.attempted_backends.end(),
                p.producer.id) == p.attempted_backends.end())
    return bad(product_error_code::stale_binding,
               "backend_provenance.attempts");
  if (p.fallback_used) {
    if (p.selection != backend_selection_mode::explicit_fallback_chain ||
        !p.primary_failure ||
        !fallback_permitted_for(p.primary_failure->code))
      return bad(product_error_code::qualification_policy_violation,
                 "backend_provenance.fallback");
  } else if (p.primary_failure) {
    return bad(product_error_code::stale_binding,
               "backend_provenance.primary_failure_without_fallback");
  }
  return true;
}

product_status_or<exact_result_handle>
make_exact_result_handle(operation op, exact_result_topology topology,
                         backend_provenance backend,
                         std::vector<std::uint8_t> canonical_bytes) {
  if (static_cast<unsigned>(op) >
          static_cast<unsigned>(operation::symmetric_difference) ||
      !known(topology))
    return make_product_error(product_error_code::input_contract_error,
                              "exact_result.operation_or_topology");
  auto bp = validate_backend_provenance(backend);
  if (!bp.has_value())
    return bp.error();
  if (canonical_bytes.empty())
    return make_product_error(product_error_code::exact_result_serialization_error,
                              "exact_result.empty_serialization");
  if (canonical_bytes.size() > 64U * 1024U * 1024U)
    return make_product_error(product_error_code::resource_limit,
                               "exact_result.serialization_limit");
  auto exact_bytes =
      validate_canonical_exact_result_bytes(op, topology, backend,
                                            canonical_bytes);
  if (!exact_bytes.has_value())
    return exact_bytes.error();
  canonical_encoder e;
  e.u16(product_contract_schema_version);
  e.byte(static_cast<std::uint8_t>(op));
  e.byte(static_cast<std::uint8_t>(topology));
  e.u16(static_cast<std::uint16_t>(backend.producer.id));
  encode_backend_version(e, backend.producer.adapter_version);
  e.string(backend.producer.build_identifier);
  e.byte(static_cast<std::uint8_t>(backend.producer.maturity));
  encode_digest(e, backend.producer.capability_digest);
  e.byte(static_cast<std::uint8_t>(backend.selection));
  e.boolean(backend.fallback_used);
  e.u64(backend.attempted_backends.size());
  for (const auto id : backend.attempted_backends)
    e.u16(static_cast<std::uint16_t>(id));
  e.byte_string(canonical_bytes);
  try {
    auto storage = std::make_shared<exact_result_storage>();
    storage->selected_operation = op;
    storage->topology = topology;
    storage->backend = std::move(backend);
    storage->canonical_bytes = std::move(canonical_bytes);
    storage->canonical_digest = domain_digest(
        {{'Y', 'G', 'B', 'X', 'R', 'E', 'S', '3'}}, e.bytes());
    return exact_result_handle(std::move(storage));
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "exact_result.allocation");
  }
}
