template <class T, class I> class experimental_exact_backend final
    : public boolean_backend<T, I> {
  backend_identity identity_;

public:
  explicit experimental_exact_backend(backend_identity identity)
      : identity_(std::move(identity)) {}

  const backend_identity &identity() const noexcept override { return identity_; }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::producer;
  }

  product_status_or<backend_attempt<T, I>>
  evaluate(const backend_request<T, I> &request,
           const backend_execution_state &state) const override {
    auto state_valid = validate_execution_state(state, identity_.id);
    if (!state_valid.has_value())
      return state_valid.error();
    auto context = make_boolean_context(
        request.operand_a, request.operand_b, request.selected_operation,
        request.engine_options, request.kernel, request.verifiers,
        request.cancellation.get(), request.diagnostics);
    if (!context.has_value())
      return error_from_boolean(context.error());
    auto preparation = preparation_binding_for(request, *context.value());
    if (!preparation.has_value())
      return preparation.error();
    exact_result_backend_binding backend;
    backend.producer = identity_;
    backend.selection = state.selection;
    backend.fallback_used = state.primary_failure.has_value();
    backend.attempted_backends = state.attempted_backends;
    if (state.primary_failure)
      backend.primary_failure = state.primary_failure->code;
    auto result = evaluate_boolean_product_result(
        *context.value(), std::move(backend), std::move(preparation.value()),
        request.product_options.result.representation,
        request.product_options.realization);
    if (!result.has_value())
      return result.error();

    backend_attempt<T, I> attempt;
    attempt.backend = identity_;
    attempt.role = role();
    attempt.request_digest = request.request_digest;
    attempt.product = result.value();
    const auto payload = result.value()->exact_result->canonical_digest;
    attempt.canonical_bytes = encode_attempt_body(identity_, attempt.role,
                                                  attempt.request_digest, payload);
    attempt.attempt_digest = domain_digest(attempt_tag, attempt.canonical_bytes);
    return attempt;
  }

  product_status_or<bool>
  verify(const backend_request<T, I> &request,
         const backend_attempt<T, I> &attempt) const noexcept override {
    try {
      if (attempt.schema != backend_adapter_schema ||
          !same_backend_identity(attempt.backend, identity_) ||
          attempt.role != role() || attempt.request_digest != request.request_digest ||
          !attempt.product || !*attempt.product || attempt.diagnostic)
        return error(product_error_code::stale_binding,
                     "exact_backend.attempt_binding");
      const auto &product = **attempt.product;
      auto valid = validate_product_result(product);
      if (!valid.has_value())
        return valid.error();
      if (product.operation.selected_operation() != request.selected_operation ||
          !same_backend_identity(product.backend.producer, identity_))
        return error(product_error_code::backend_disagreement,
                     "exact_backend.semantic_binding");
      if (product.representation !=
          request.product_options.result.representation) {
        if (product.representation != result_representation::exact_stratified ||
            !product.realization || product.realization->succeeded ||
            product.realization->requested !=
                request.product_options.result.representation)
          return error(product_error_code::backend_disagreement,
                       "exact_backend.representation_binding");
      }
      const auto payload = product.exact_result->canonical_digest;
      const auto bytes = encode_attempt_body(identity_, attempt.role,
                                             attempt.request_digest, payload);
      if (attempt.canonical_bytes != bytes ||
          attempt.attempt_digest != domain_digest(attempt_tag, bytes))
        return error(product_error_code::verifier_disagreement,
                     "exact_backend.attempt_digest");
      return true;
    } catch (...) {
      return error(product_error_code::internal_invariant_error,
                   "exact_backend.verify_exception");
    }
  }
};

template <class T, class I> class axis_aligned_box_reference_backend final
    : public boolean_backend<T, I> {
  backend_identity identity_;

public:
  explicit axis_aligned_box_reference_backend(backend_identity identity)
      : identity_(std::move(identity)) {}

  const backend_identity &identity() const noexcept override { return identity_; }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::diagnostic_only;
  }

  product_status_or<backend_attempt<T, I>>
  evaluate(const backend_request<T, I> &request,
           const backend_execution_state &state) const override {
    auto state_valid = validate_execution_state(state, identity_.id);
    if (!state_valid.has_value())
      return state_valid.error();
    auto payload = compute_reference_payload(request, true);
    if (!payload.has_value())
      return payload.error();
    backend_attempt<T, I> attempt;
    attempt.backend = identity_;
    attempt.role = role();
    attempt.request_digest = request.request_digest;
    attempt.diagnostic = std::move(payload.value());
    attempt.canonical_bytes = encode_attempt_body(
        identity_, attempt.role, attempt.request_digest,
        attempt.diagnostic->semantic_digest);
    attempt.attempt_digest = domain_digest(attempt_tag, attempt.canonical_bytes);
    return attempt;
  }

  product_status_or<bool>
  verify(const backend_request<T, I> &request,
         const backend_attempt<T, I> &attempt) const noexcept override {
    try {
      if (attempt.schema != backend_adapter_schema ||
          !same_backend_identity(attempt.backend, identity_) ||
          attempt.role != role() || attempt.request_digest != request.request_digest ||
          attempt.product || !attempt.diagnostic)
        return error(product_error_code::stale_binding,
                     "axis_box.attempt_binding");
      auto expected = compute_reference_payload(request, false);
      if (!expected.has_value())
        return expected.error();
      const auto &actual = *attempt.diagnostic;
      if (actual.schema != expected.value().schema ||
          actual.workload_profile != expected.value().workload_profile ||
          actual.request_digest != expected.value().request_digest ||
          actual.selected_operation != expected.value().selected_operation ||
          actual.cuts != expected.value().cuts ||
          actual.occupied_cells != expected.value().occupied_cells ||
          actual.occupied_cell_count != expected.value().occupied_cell_count ||
          actual.connected_components != expected.value().connected_components ||
          actual.boundary_rectangles != expected.value().boundary_rectangles ||
          actual.exact_volume != expected.value().exact_volume ||
          actual.canonical_bytes != expected.value().canonical_bytes ||
          actual.semantic_digest != expected.value().semantic_digest)
        return error(product_error_code::verifier_disagreement,
                     "axis_box.evidence_corruption");
      const auto bytes = encode_attempt_body(identity_, attempt.role,
                                             attempt.request_digest,
                                             actual.semantic_digest);
      if (attempt.canonical_bytes != bytes ||
          attempt.attempt_digest != domain_digest(attempt_tag, bytes))
        return error(product_error_code::verifier_disagreement,
                     "axis_box.attempt_digest");
      return true;
    } catch (...) {
      return error(product_error_code::internal_invariant_error,
                   "axis_box.verify_exception");
    }
  }

  product_status_or<backend_comparison_record>
  compare(const backend_request<T, I> &request,
          const backend_attempt<T, I> &producer,
          const backend_attempt<T, I> &diagnostic) const noexcept override {
    return compare_reference_to_product(request, producer, diagnostic);
  }
};

template <class T, class I>
product_status_or<backend_attempt<T, I>> run_adapter(
    const backend_registry_entry<T, I> &entry,
    const backend_request<T, I> &request,
    const backend_execution_state &state) {
  auto attempt = entry.adapter->evaluate(request, state);
  if (!attempt.has_value()) {
    auto failure = attempt.error();
    if (!failure.backend)
      failure.backend = entry.identity;
    return failure;
  }
  if (!same_backend_identity(attempt.value().backend, entry.identity) ||
      attempt.value().role != entry.role)
    return error(product_error_code::stale_binding,
                 "backend.adapter_identity_binding");
  auto verified = entry.adapter->verify(request, attempt.value());
  if (!verified.has_value()) {
    auto failure = verified.error();
    if (!failure.backend)
      failure.backend = entry.identity;
    return failure;
  }
  if (!verified.value()) {
    auto failure = error(product_error_code::verifier_disagreement,
                         "backend.adapter_verification_false");
    failure.backend = entry.identity;
    return failure;
  }
  return attempt.value();
}

template <class T, class I>
product_status_or<backend_registry_entry<T, I>> select_qualified(
    const backend_registry<T, I> &registry,
    const backend_request<T, I> &request,
    const qualification_manifest *manifest) {
  for (const auto &snapshot : registry.entries()) {
    auto current = registry.lookup(snapshot.identity.id);
    if (!current.has_value())
      return current.error();
    const auto &entry = current.value();
    if (entry.role != backend_adapter_role::producer)
      continue;
    auto authorized = authorize_backend(request.product_options, entry.identity,
                                        manifest);
    if (authorized.has_value())
      return entry;
    if (authorized.error().code != product_error_code::backend_unqualified &&
        authorized.error().code !=
            product_error_code::backend_capability_mismatch &&
        authorized.error().code !=
            product_error_code::qualification_policy_violation)
      return authorized.error();
  }
  return error(product_error_code::backend_unqualified,
               "backend.no_qualified_default");
}

template <class T, class I>
product_status_or<backend_attempt<T, I>> run_selected_producer(
    const backend_registry_entry<T, I> &entry,
    const backend_request<T, I> &request,
    const backend_execution_state &state,
    const qualification_manifest *manifest) {
  if (entry.role != backend_adapter_role::producer)
    return error(product_error_code::backend_capability_mismatch,
                 "backend.diagnostic_selected_as_producer");
  auto authorized = authorize_backend(request.product_options, entry.identity,
                                      manifest);
  if (!authorized.has_value())
    return authorized.error();
  return run_adapter(entry, request, state);
}

template <class T, class I>
digest execution_digest_for(const backend_execution_result<T, I> &result) {
  canonical_encoder encoder;
  encoder.u16(result.schema);
  encode_digest(encoder, result.request_digest);
  encode_digest(encoder, result.product->exact_result->canonical_digest);
  encoder.u64(result.attempts.size());
  for (const auto &attempt : result.attempts) {
    encoder.u16(static_cast<std::uint16_t>(attempt.requested_backend));
    encoder.boolean(attempt.identity_available);
    if (attempt.identity_available)
      encode_identity(encoder, attempt.backend);
    encoder.byte(static_cast<std::uint8_t>(attempt.role));
    encoder.boolean(attempt.succeeded);
    encoder.boolean(attempt.failure.has_value());
    if (attempt.failure)
      encode_product_error(encoder, *attempt.failure);
    encode_digest(encoder, attempt.payload_digest);
    encode_digest(encoder, attempt.attempt_digest);
  }
  encoder.u64(result.comparisons.size());
  for (const auto &comparison : result.comparisons)
    encode_digest(encoder, comparison.report_digest);
  return domain_digest(execution_tag, encoder.bytes());
}

template <class T, class I>
product_status_or<backend_execution_result_handle<T, I>> freeze_execution(
    backend_execution_result<T, I> result,
    const backend_request<T, I> &request) {
  result.execution_digest = execution_digest_for(result);
  auto valid = validate_backend_execution_result(result, request);
  if (!valid.has_value())
    return valid.error();
  try {
    return std::make_shared<const backend_execution_result<T, I>>(
        std::move(result));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.execution_allocation");
  }
}
