product_status_or<bool>
validate_product_replay_binding(const product_replay_binding &r,
                                const boolean_product_options &o,
                                const backend_identity &b,
                                const exact_result_handle *exact) noexcept {
  if (r.schema != product_contract_schema_version)
    return bad(product_error_code::stale_binding, "replay.schema");
  auto ov = validate_product_options(o);
  if (!ov.has_value())
    return ov.error();
  auto bv = validate_backend_identity(b);
  if (!bv.has_value())
    return bv.error();
  if (r.options_digest != product_options_digest(o) || r.producer != b.id ||
      !same_version(r.adapter_version, b.adapter_version) ||
      r.build_identifier != b.build_identifier ||
      r.capability_digest != b.capability_digest)
    return bad(product_error_code::replay_mismatch, "replay.backend_binding");
  if (o.backend.mode == backend_selection_mode::explicit_fallback_chain) {
    if (r.fallback_chain != o.backend.fallback_chain)
      return bad(product_error_code::replay_mismatch,
                 "replay.fallback_chain");
  } else if (!r.fallback_chain.empty()) {
    return bad(product_error_code::replay_mismatch,
               "replay.unexpected_fallback_chain");
  }
  if (exact) {
    if (!exact->valid() || r.exact_result_digest != exact->get().canonical_digest)
      return bad(product_error_code::replay_mismatch,
                 "replay.exact_result_binding");
  }
  if (product_digest_is_zero(r.preparation_policy_digest) ||
      product_digest_is_zero(r.realization_policy_digest) ||
      product_digest_is_zero(r.attribute_policy_digest) ||
      product_digest_is_zero(r.verifier_set_digest))
    return bad(product_error_code::replay_mismatch,
               "replay.required_digest_missing");
  if (o.qualification.manifest) {
    if (r.qualification_manifest_digest !=
        o.qualification.manifest->manifest_digest)
      return bad(product_error_code::replay_mismatch,
                 "replay.qualification_binding");
  } else if (!product_digest_is_zero(r.qualification_manifest_digest)) {
    return bad(product_error_code::replay_mismatch,
               "replay.unexpected_qualification_manifest");
  }
  return true;
}

product_status_or<std::vector<std::uint8_t>>
encode_product_replay_binding(const product_replay_binding &r) {
  if (r.schema != product_contract_schema_version || !known(r.producer) ||
      !unique_backend_ids(r.fallback_chain) || r.build_identifier.empty())
    return make_product_error(product_error_code::input_contract_error,
                              "replay.encode_contract");
  canonical_encoder e;
  e.u16(r.schema);
  encode_digest(e, r.options_digest);
  encode_digest(e, r.preparation_policy_digest);
  encode_digest(e, r.preparation_report_digest);
  e.u16(static_cast<std::uint16_t>(r.producer));
  encode_backend_version(e, r.adapter_version);
  e.string(r.build_identifier);
  encode_digest(e, r.capability_digest);
  e.u64(r.fallback_chain.size());
  for (const auto id : r.fallback_chain)
    e.u16(static_cast<std::uint16_t>(id));
  encode_digest(e, r.exact_result_digest);
  encode_digest(e, r.realization_policy_digest);
  encode_digest(e, r.attribute_policy_digest);
  encode_digest(e, r.verifier_set_digest);
  encode_digest(e, r.qualification_manifest_digest);
  return record_bytes({{'Y', 'G', 'B', 'P', 'R', 'P', '0', '3'}},
                      product_contract_schema_version, e.bytes());
}

product_status_or<product_replay_binding>
decode_product_replay_binding(const std::vector<std::uint8_t> &bytes,
                              const product_decode_limits &limits) {
  auto payload = extract_payload(
      bytes, {{'Y', 'G', 'B', 'P', 'R', 'P', '0', '3'}}, limits);
  if (!payload.has_value())
    return payload.error();
  try {
    reader r(payload.value(), limits);
    product_replay_binding out;
    out.schema = r.u16();
    if (out.schema != product_contract_schema_version)
      throw std::runtime_error("unknown_replay_schema");
    out.options_digest = r.digest_value();
    out.preparation_policy_digest = r.digest_value();
    out.preparation_report_digest = r.digest_value();
    out.producer = decode_backend_id(r);
    out.adapter_version.major = r.u16();
    out.adapter_version.minor = r.u16();
    out.adapter_version.patch = r.u16();
    out.build_identifier = r.string();
    out.capability_digest = r.digest_value();
    const auto n = r.u64();
    if (n > limits.max_vector_elements)
      throw std::runtime_error("vector_limit");
    out.fallback_chain.reserve(static_cast<std::size_t>(n));
    for (std::uint64_t i = 0; i != n; ++i)
      out.fallback_chain.push_back(decode_backend_id(r));
    out.exact_result_digest = r.digest_value();
    out.realization_policy_digest = r.digest_value();
    out.attribute_policy_digest = r.digest_value();
    out.verifier_set_digest = r.digest_value();
    out.qualification_manifest_digest = r.digest_value();
    if (r.remaining() != 0)
      throw std::runtime_error("trailing_bytes");
    if (!unique_backend_ids(out.fallback_chain))
      throw std::runtime_error("duplicate_backend");
    return out;
  } catch (const std::exception &x) {
    auto e = make_product_error(product_error_code::input_contract_error,
                                "replay.decode");
    e.detail = x.what();
    return e;
  }
}
