product_error make_product_error(product_error_code code, std::string key,
                                 std::uint32_t subcode) {
  product_error e;
  e.code = code;
  e.subcode = subcode;
  e.message_key = std::move(key);
  return e;
}

product_error_code promote_error_code(boolean_error_code code) noexcept {
  switch (code) {
  case boolean_error_code::input_contract_error:
    return product_error_code::input_contract_error;
  case boolean_error_code::unsupported_platform:
    return product_error_code::unsupported_platform;
  case boolean_error_code::resource_limit:
    return product_error_code::resource_limit;
  case boolean_error_code::index_overflow:
    return product_error_code::index_overflow;
  case boolean_error_code::result_topology_not_supported:
    return product_error_code::result_topology_not_supported;
  case boolean_error_code::output_not_representable:
    return product_error_code::output_not_representable;
  case boolean_error_code::internal_invariant_error:
    return product_error_code::internal_invariant_error;
  }
  return product_error_code::internal_invariant_error;
}

bool fallback_permitted_for(product_error_code code) noexcept {
  switch (code) {
  case product_error_code::input_contract_error:
  case product_error_code::unsupported_platform:
  case product_error_code::resource_limit:
  case product_error_code::index_overflow:
  case product_error_code::result_topology_not_supported:
  case product_error_code::output_not_representable:
  case product_error_code::normalization_required:
  case product_error_code::normalization_failed:
  case product_error_code::backend_unavailable:
  case product_error_code::backend_unqualified:
  case product_error_code::attribute_transfer_conflict:
  case product_error_code::approximation_policy_rejected:
    return true;
  case product_error_code::internal_invariant_error:
  case product_error_code::backend_capability_mismatch:
  case product_error_code::backend_disagreement:
  case product_error_code::exact_result_serialization_error:
  case product_error_code::qualification_policy_violation:
  case product_error_code::stale_binding:
  case product_error_code::replay_mismatch:
  case product_error_code::verifier_disagreement:
    return false;
  }
  return false;
}

bool product_digest_is_zero(const digest &d) noexcept {
  for (const auto b : d.bytes)
    if (b != 0)
      return false;
  return true;
}

bool same_backend_identity(const backend_identity &a,
                           const backend_identity &b) noexcept {
  return same_identity(a, b);
}
