product_status_or<digest>
backend_comparison_digest(const backend_comparison_record &comparison) noexcept {
  try {
    if (comparison.schema != backend_adapter_schema)
      return error(product_error_code::stale_binding,
                   "backend.comparison_schema");
    return domain_digest(comparison_tag, encode_comparison_body(comparison));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.comparison_digest_allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.comparison_digest_exception");
  }
}

template <class T, class I>
product_status_or<bool> backend_registry<T, I>::register_backend(
    std::shared_ptr<const boolean_backend<T, I>> adapter) {
  try {
    if (!adapter)
      return error(product_error_code::backend_unavailable,
                   "backend.registry_null");
    auto identity_valid = validate_backend_identity(adapter->identity());
    if (!identity_valid.has_value())
      return identity_valid.error();
    std::lock_guard<std::mutex> lock(mutex_);
    if (frozen_)
      return error(product_error_code::stale_binding,
                   "backend.registry_frozen");
    backend_registry_entry<T, I> entry;
    entry.identity = adapter->identity();
    entry.role = adapter->role();
    entry.adapter = std::move(adapter);
    if (!entries_.emplace(entry.identity.id, std::move(entry)).second)
      return error(product_error_code::backend_capability_mismatch,
                   "backend.registry_duplicate_id");
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.registry_allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.registry_registration_exception");
  }
}

template <class T, class I>
product_status_or<bool> backend_registry<T, I>::freeze() {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frozen_)
      return true;
    if (entries_.empty())
      return error(product_error_code::backend_unavailable,
                   "backend.registry_empty");
    canonical_encoder encoder;
    encoder.u16(backend_adapter_schema);
    encoder.u64(entries_.size());
    for (const auto &entry : entries_) {
      encode_identity(encoder, entry.second.identity);
      encoder.byte(static_cast<std::uint8_t>(entry.second.role));
    }
    registry_digest_ = domain_digest(registry_tag, encoder.bytes());
    frozen_ = true;
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.registry_freeze_allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.registry_freeze_exception");
  }
}

template <class T, class I>
bool backend_registry<T, I>::frozen() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return frozen_;
}

template <class T, class I>
digest backend_registry<T, I>::registry_digest() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return registry_digest_;
}

template <class T, class I>
product_status_or<backend_registry_entry<T, I>>
backend_registry<T, I>::lookup(backend_id id) const {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!frozen_)
      return error(product_error_code::stale_binding,
                   "backend.registry_not_frozen");
    const auto found = entries_.find(id);
    if (found == entries_.end())
      return error(product_error_code::backend_unavailable,
                   "backend.registry_missing");
    const auto &current = found->second.adapter->identity();
    const auto &snapshot = found->second.identity;
    if (current.id != snapshot.id || current.schema != snapshot.schema ||
        current.build_identifier != snapshot.build_identifier ||
        !same_version(current.adapter_version, snapshot.adapter_version))
      return error(product_error_code::backend_unavailable,
                   "backend.version_drift");
    if (current.capabilities.schema != snapshot.capabilities.schema ||
        current.capabilities.bits != snapshot.capabilities.bits ||
        current.capability_digest != snapshot.capability_digest ||
        current.maturity != snapshot.maturity)
      return error(product_error_code::backend_capability_mismatch,
                   "backend.capability_drift");
    if (found->second.adapter->role() != found->second.role)
      return error(product_error_code::stale_binding,
                   "backend.role_drift");
    return found->second;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.registry_lookup_allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.registry_lookup_exception");
  }
}

template <class T, class I>
std::vector<backend_registry_entry<T, I>> backend_registry<T, I>::entries() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<backend_registry_entry<T, I>> result;
  if (!frozen_)
    return result;
  result.reserve(entries_.size());
  for (const auto &entry : entries_)
    result.push_back(entry.second);
  return result;
}

template <class T, class I>
product_status_or<backend_request_handle<T, I>> make_backend_request(
    prepared_operand<T, I> a, prepared_operand<T, I> b, operation op,
    boolean_options engine, boolean_product_options product,
    std::shared_ptr<const exact_kernel_services<T>> kernel,
    std::shared_ptr<const verifier_service> verifiers,
    std::shared_ptr<cancellation_source> cancellation,
    diagnostic_consumer diagnostics,
    deterministic_executor_factory executor_factory,
    const backend_request_limits &limits) {
  try {
    if (!kernel || !verifiers)
      return error(product_error_code::backend_unavailable,
                   "backend.runtime_services");
    if (!limits_valid(limits))
      return error(product_error_code::input_contract_error,
                   "backend.request_limits");
    auto engine_valid = validate_options(engine);
    if (!engine_valid.has_value())
      return error_from_boolean(engine_valid.error());
    auto product_valid = validate_product_options(product);
    if (!product_valid.has_value())
      return product_valid.error();
    auto preparation = validate_preparation_contract(
        product.preparation.mode, a.normalization() != nullptr,
        b.normalization() != nullptr);
    if (!preparation.has_value())
      return preparation.error();
    if (!cancellation)
      cancellation = std::make_shared<cancellation_source>();
    auto valid_a = verify_prepared_operand(a, engine, kernel, verifiers,
                                           cancellation.get());
    if (!valid_a.has_value())
      return error_from_boolean(valid_a.error());
    auto valid_b = verify_prepared_operand(b, engine, kernel, verifiers,
                                           cancellation.get());
    if (!valid_b.has_value())
      return error_from_boolean(valid_b.error());
    const auto request_digest =
        request_digest_for(a, b, op, engine, product, limits);
    return std::make_shared<const backend_request<T, I>>(
        std::move(a), std::move(b), op, std::move(engine), std::move(product),
        limits, std::move(kernel), std::move(verifiers),
        std::move(cancellation), std::move(diagnostics),
        std::move(executor_factory), request_digest);
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.request_allocation");
  } catch (const std::exception &exception) {
    auto failure = error(product_error_code::internal_invariant_error,
                         "backend.request_exception");
    failure.detail = exception.what();
    return failure;
  }
}

template <class T, class I>
product_status_or<bool>
validate_backend_request(const backend_request<T, I> &request) noexcept {
  try {
    if (request.schema != backend_adapter_schema || !request.kernel ||
        !request.verifiers || !request.cancellation ||
        !limits_valid(request.limits))
      return error(product_error_code::stale_binding,
                   "backend.request_binding");
    auto product = validate_product_options(request.product_options);
    if (!product.has_value())
      return product.error();
    auto engine = validate_options(request.engine_options);
    if (!engine.has_value())
      return error_from_boolean(engine.error());
    auto preparation = validate_preparation_contract(
        request.product_options.preparation.mode,
        request.operand_a.normalization() != nullptr,
        request.operand_b.normalization() != nullptr);
    if (!preparation.has_value())
      return preparation.error();
    auto valid_a = verify_prepared_operand(
        request.operand_a, request.engine_options, request.kernel,
        request.verifiers, request.cancellation.get());
    if (!valid_a.has_value())
      return error_from_boolean(valid_a.error());
    auto valid_b = verify_prepared_operand(
        request.operand_b, request.engine_options, request.kernel,
        request.verifiers, request.cancellation.get());
    if (!valid_b.has_value())
      return error_from_boolean(valid_b.error());
    if (request.request_digest !=
        request_digest_for(request.operand_a, request.operand_b,
                           request.selected_operation, request.engine_options,
                           request.product_options, request.limits))
      return error(product_error_code::stale_binding,
                   "backend.request_digest");
    return true;
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.request_validation_exception");
  }
}

template <class T, class I>
product_status_or<std::shared_ptr<const boolean_backend<T, I>>>
make_experimental_exact_backend() {
  auto identity = make_backend_identity(
      backend_id::experimental_exact_v1, {1, 0, 0},
      "ygor-experimental-exact-v1", exact_backend_capabilities(),
      backend_maturity::experimental);
  if (!identity.has_value())
    return identity.error();
  try {
    return std::shared_ptr<const boolean_backend<T, I>>(
        std::make_shared<experimental_exact_backend<T, I>>(
            std::move(identity.value())));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.exact_adapter_allocation");
  }
}

template <class T, class I>
product_status_or<std::shared_ptr<const boolean_backend<T, I>>>
make_axis_aligned_box_reference_backend() {
  auto identity = make_backend_identity(
      backend_id::independent_axis_aligned_box_v1, {1, 0, 0},
      "ygor-independent-axis-aligned-box-v1",
      reference_backend_capabilities(), backend_maturity::candidate);
  if (!identity.has_value())
    return identity.error();
  try {
    return std::shared_ptr<const boolean_backend<T, I>>(
        std::make_shared<axis_aligned_box_reference_backend<T, I>>(
            std::move(identity.value())));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.reference_adapter_allocation");
  }
}

template <class T, class I>
product_status_or<std::shared_ptr<backend_registry<T, I>>>
make_default_backend_registry(bool include_diagnostic_reference) {
  try {
    auto registry = std::make_shared<backend_registry<T, I>>();
    auto exact = make_experimental_exact_backend<T, I>();
    if (!exact.has_value())
      return exact.error();
    auto registered = registry->register_backend(exact.value());
    if (!registered.has_value())
      return registered.error();
    if (include_diagnostic_reference) {
      auto reference = make_axis_aligned_box_reference_backend<T, I>();
      if (!reference.has_value())
        return reference.error();
      registered = registry->register_backend(reference.value());
      if (!registered.has_value())
        return registered.error();
    }
    auto frozen = registry->freeze();
    if (!frozen.has_value())
      return frozen.error();
    return registry;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "backend.default_registry_allocation");
  }
}

template <class T, class I>
product_status_or<backend_execution_result_handle<T, I>>
evaluate_backend_request(const backend_registry<T, I> &registry,
                         const backend_request<T, I> &request,
                         const qualification_manifest *manifest) {
  auto request_valid = validate_backend_request(request);
  if (!request_valid.has_value())
    return request_valid.error();
  if (!registry.frozen())
    return error(product_error_code::stale_binding,
                 "backend.registry_not_frozen");

  backend_execution_result<T, I> result;
  result.request_digest = request.request_digest;
  const auto mode = request.product_options.backend.mode;

  if (mode == backend_selection_mode::explicit_backend ||
      mode == backend_selection_mode::qualified_default) {
    product_status_or<backend_registry_entry<T, I>> selected =
        mode == backend_selection_mode::explicit_backend
            ? registry.lookup(*request.product_options.backend.requested_backend)
            : select_qualified(registry, request, manifest);
    if (!selected.has_value())
      return selected.error();
    backend_execution_state state;
    state.selection = mode;
    state.attempted_backends = {selected.value().identity.id};
    auto attempt = run_selected_producer(selected.value(), request, state, manifest);
    if (!attempt.has_value())
      return attempt.error();
    result.product = *attempt.value().product;
    result.attempts.push_back(summary_from_attempt(attempt.value()));
    return freeze_execution(std::move(result), request);
  }

  if (mode == backend_selection_mode::explicit_fallback_chain) {
    std::optional<product_error> primary_failure;
    std::vector<backend_id> attempted_ids;
    for (const auto id : request.product_options.backend.fallback_chain) {
      attempted_ids.push_back(id);
      auto selected = registry.lookup(id);
      if (!selected.has_value()) {
        auto failure = selected.error();
        result.attempts.push_back(summary_from_unavailable(id, failure));
        if (!primary_failure)
          primary_failure = failure;
        if (!fallback_permitted_for(failure.code) ||
            !requested_failure_category(request.product_options.backend,
                                        failure.code))
          return failure;
        continue;
      }
      backend_execution_state state;
      state.selection = mode;
      state.attempted_backends = attempted_ids;
      state.primary_failure = primary_failure;
      auto attempt =
          run_selected_producer(selected.value(), request, state, manifest);
      if (!attempt.has_value()) {
        auto failure = attempt.error();
        result.attempts.push_back(summary_from_failure(
            selected.value().identity, selected.value().role, failure));
        if (!primary_failure)
          primary_failure = failure;
        if (!fallback_permitted_for(failure.code) ||
            !requested_failure_category(request.product_options.backend,
                                        failure.code))
          return failure;
        continue;
      }
      result.product = *attempt.value().product;
      result.attempts.push_back(summary_from_attempt(attempt.value()));
      return freeze_execution(std::move(result), request);
    }
    if (primary_failure)
      return *primary_failure;
    return error(product_error_code::backend_unavailable,
                 "backend.fallback_exhausted");
  }

  const auto producer_id = *request.product_options.backend.requested_backend;
  auto producer_entry = registry.lookup(producer_id);
  if (!producer_entry.has_value())
    return producer_entry.error();
  backend_execution_state producer_state;
  producer_state.selection = mode;
  // Diagnostic adapters do not participate in producer selection. Keep the
  // durable exact-result provenance bound only to the declared producer; the
  // comparator attempts are retained separately in the execution envelope.
  producer_state.attempted_backends.push_back(producer_id);
  auto producer = run_selected_producer(producer_entry.value(), request,
                                        producer_state, manifest);
  if (!producer.has_value())
    return producer.error();
  result.product = *producer.value().product;
  result.attempts.push_back(summary_from_attempt(producer.value()));

  for (const auto comparator_id :
       request.product_options.backend.diagnostic_backends) {
    auto comparator_entry = registry.lookup(comparator_id);
    if (!comparator_entry.has_value())
      return comparator_entry.error();
    if (comparator_entry.value().role != backend_adapter_role::diagnostic_only)
      return error(product_error_code::backend_capability_mismatch,
                   "backend.comparator_role");
    backend_execution_state comparator_state;
    comparator_state.selection = mode;
    comparator_state.attempted_backends = {producer_id, comparator_id};
    auto comparator = run_adapter(comparator_entry.value(), request,
                                  comparator_state);
    if (!comparator.has_value())
      return comparator.error();
    result.attempts.push_back(summary_from_attempt(comparator.value()));
    auto comparison = comparator_entry.value().adapter->compare(
        request, producer.value(), comparator.value());
    if (!comparison.has_value())
      return comparison.error();
    auto comparison_valid =
        validate_comparison(comparison.value(), request.request_digest);
    if (!comparison_valid.has_value())
      return comparison_valid.error();
    result.comparisons.push_back(std::move(comparison.value()));
  }
  return freeze_execution(std::move(result), request);
}

template <class T, class I>
product_status_or<bool> validate_backend_execution_result(
    const backend_execution_result<T, I> &result,
    const backend_request<T, I> &request) noexcept {
  try {
    if (result.schema != backend_adapter_schema ||
        result.request_digest != request.request_digest || !result.product ||
        result.attempts.empty())
      return error(product_error_code::stale_binding,
                   "backend.execution_binding");
    auto product_valid = validate_product_result(*result.product);
    if (!product_valid.has_value())
      return product_valid.error();
    std::size_t producer_successes = 0;
    for (const auto &attempt : result.attempts) {
      if (attempt.schema != backend_adapter_schema ||
          product_digest_is_zero(attempt.attempt_digest) ||
          (attempt.identity_available &&
           attempt.backend.id != attempt.requested_backend))
        return error(product_error_code::stale_binding,
                     "backend.attempt_summary_binding");
      if (attempt.succeeded && attempt.failure)
        return error(product_error_code::stale_binding,
                     "backend.attempt_summary_outcome");
      if (attempt.succeeded && attempt.role == backend_adapter_role::producer) {
        if (!attempt.identity_available)
          return error(product_error_code::stale_binding,
                       "backend.producer_identity_missing");
        ++producer_successes;
        if (!same_backend_identity(attempt.backend,
                                   result.product->backend.producer))
          return error(product_error_code::stale_binding,
                       "backend.producer_summary_binding");
      }
    }
    if (producer_successes != 1)
      return error(product_error_code::stale_binding,
                   "backend.producer_count");
    std::vector<backend_id> attempted_ids;
    attempted_ids.reserve(result.attempts.size());
    for (const auto &attempt : result.attempts)
      attempted_ids.push_back(attempt.requested_backend);
    std::vector<backend_id> producer_attempted_ids = attempted_ids;
    if (request.product_options.backend.mode ==
        backend_selection_mode::diagnostic_compare) {
      producer_attempted_ids.clear();
      for (const auto &attempt : result.attempts)
        if (attempt.succeeded &&
            attempt.role == backend_adapter_role::producer) {
          producer_attempted_ids.push_back(attempt.requested_backend);
          break;
        }
    }
    if (result.product->backend.selection !=
            request.product_options.backend.mode ||
        result.product->backend.attempted_backends != producer_attempted_ids)
      return error(product_error_code::stale_binding,
                   "backend.execution_provenance");
    const bool expected_fallback =
        request.product_options.backend.mode ==
            backend_selection_mode::explicit_fallback_chain &&
        result.attempts.size() > 1;
    if (result.product->backend.fallback_used != expected_fallback)
      return error(product_error_code::stale_binding,
                   "backend.fallback_provenance");
    if (expected_fallback) {
      if (!result.attempts.front().failure ||
          !result.product->backend.primary_failure ||
          result.product->backend.primary_failure->code !=
              result.attempts.front().failure->code)
        return error(product_error_code::stale_binding,
                     "backend.primary_failure_provenance");
    } else if (result.product->backend.primary_failure) {
      return error(product_error_code::stale_binding,
                   "backend.unexpected_primary_failure");
    }
    for (const auto &comparison : result.comparisons) {
      auto valid = validate_comparison(comparison, request.request_digest);
      if (!valid.has_value())
        return valid.error();
      if (!same_backend_identity(comparison.producer,
                                 result.product->backend.producer))
        return error(product_error_code::stale_binding,
                     "backend.comparison_producer");
    }
    if (request.product_options.backend.mode !=
            backend_selection_mode::diagnostic_compare &&
        !result.comparisons.empty())
      return error(product_error_code::stale_binding,
                   "backend.unrequested_comparison");
    if (result.execution_digest != execution_digest_for(result))
      return error(product_error_code::stale_binding,
                   "backend.execution_digest");
    return true;
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "backend.execution_validation_exception");
  }
}

#define YGOR_BACKEND_DEFINE(T, I)                                              \
  template class backend_registry<T, I>;                                       \
  template product_status_or<backend_request_handle<T, I>>                     \
  make_backend_request(                                                        \
      prepared_operand<T, I>, prepared_operand<T, I>, operation,               \
      boolean_options, boolean_product_options,                                \
      std::shared_ptr<const exact_kernel_services<T>>,                          \
      std::shared_ptr<const verifier_service>,                                  \
      std::shared_ptr<cancellation_source>, diagnostic_consumer,                \
      deterministic_executor_factory, const backend_request_limits &);          \
  template product_status_or<bool> validate_backend_request(                    \
      const backend_request<T, I> &) noexcept;                                  \
  template product_status_or<std::shared_ptr<const boolean_backend<T, I>>>      \
  make_experimental_exact_backend<T, I>();                                      \
  template product_status_or<std::shared_ptr<const boolean_backend<T, I>>>      \
  make_axis_aligned_box_reference_backend<T, I>();                              \
  template product_status_or<std::shared_ptr<backend_registry<T, I>>>           \
  make_default_backend_registry<T, I>(bool);                                    \
  template product_status_or<backend_execution_result_handle<T, I>>             \
  evaluate_backend_request(const backend_registry<T, I> &,                      \
                           const backend_request<T, I> &,                        \
                           const qualification_manifest *);                     \
  template product_status_or<bool> validate_backend_execution_result(           \
      const backend_execution_result<T, I> &,                                   \
      const backend_request<T, I> &) noexcept

YGOR_BACKEND_DEFINE(float, std::uint32_t);
YGOR_BACKEND_DEFINE(float, std::uint64_t);
YGOR_BACKEND_DEFINE(double, std::uint32_t);
YGOR_BACKEND_DEFINE(double, std::uint64_t);
#undef YGOR_BACKEND_DEFINE
