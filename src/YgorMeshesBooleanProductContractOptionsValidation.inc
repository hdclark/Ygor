product_status_or<bool>
validate_product_options(const boolean_product_options &o) noexcept {
  try {
    if (!schemas_current(o.schemas) ||
        o.backend.schema != product_contract_schema_version ||
        o.preparation.schema != product_contract_schema_version ||
        o.preparation.normalization.schema != product_contract_schema_version ||
        o.result.schema != product_contract_schema_version ||
        o.realization.schema != product_contract_schema_version ||
        o.realization.search.schema != product_contract_schema_version ||
        o.realization.approximation.schema != product_contract_schema_version ||
        o.attributes.schema != product_contract_schema_version ||
        o.qualification.schema != product_contract_schema_version)
      return bad(product_error_code::stale_binding, "product_options.schema");
    if (!known(o.backend.mode) || !known(o.preparation.mode) ||
        !known(o.preparation.normalization.mode) ||
        !known(o.preparation.normalization.unit) ||
        !known(o.result.representation) ||
        !known(o.realization.semantics) ||
        !known(o.realization.search.strategy) ||
        !known(o.realization.approximation.unit) ||
        !known(o.attributes.mode) || !known(o.attributes.conflicts) ||
        !known(o.qualification.mode))
      return bad(product_error_code::input_contract_error,
                 "product_options.unknown_enum");
    if (!o.mandatory_verification)
      return bad(product_error_code::qualification_policy_violation,
                 "product_options.verification_mandatory");

    if (o.backend.requested_backend && !known(*o.backend.requested_backend))
      return bad(product_error_code::backend_unavailable,
                 "product_options.requested_backend");
    if (!unique_backend_ids(o.backend.diagnostic_backends) ||
        !unique_backend_ids(o.backend.fallback_chain) ||
        !unique_error_codes(o.backend.fallback_on))
      return bad(product_error_code::input_contract_error,
                 "product_options.backend_duplicates");
    for (const auto code : o.backend.fallback_on)
      if (!fallback_permitted_for(code))
        return bad(product_error_code::qualification_policy_violation,
                   "product_options.forbidden_fallback_category");

    switch (o.backend.mode) {
    case backend_selection_mode::explicit_backend:
      if (!o.backend.requested_backend || !o.backend.diagnostic_backends.empty() ||
          !o.backend.fallback_chain.empty() || !o.backend.fallback_on.empty())
        return bad(product_error_code::input_contract_error,
                   "product_options.explicit_backend_shape");
      break;
    case backend_selection_mode::qualified_default:
      if (o.backend.requested_backend || !o.backend.diagnostic_backends.empty() ||
          !o.backend.fallback_chain.empty() || !o.backend.fallback_on.empty() ||
          o.backend.allow_experimental_backend)
        return bad(product_error_code::qualification_policy_violation,
                   "product_options.qualified_default_shape");
      break;
    case backend_selection_mode::diagnostic_compare:
      if (!o.backend.requested_backend ||
          o.backend.diagnostic_backends.empty() ||
          !o.backend.fallback_chain.empty() || !o.backend.fallback_on.empty())
        return bad(product_error_code::input_contract_error,
                   "product_options.diagnostic_compare_shape");
      if (std::find(o.backend.diagnostic_backends.begin(),
                    o.backend.diagnostic_backends.end(),
                    *o.backend.requested_backend) !=
          o.backend.diagnostic_backends.end())
        return bad(product_error_code::input_contract_error,
                   "product_options.diagnostic_producer_repeated");
      break;
    case backend_selection_mode::explicit_fallback_chain:
      if (o.backend.requested_backend ||
          !o.backend.diagnostic_backends.empty() ||
          o.backend.fallback_chain.size() < 2 || o.backend.fallback_on.empty())
        return bad(product_error_code::input_contract_error,
                   "product_options.fallback_shape");
      break;
    }

    const auto &n = o.preparation.normalization;
    if (!finite_nonnegative(n.model_tolerance) ||
        (n.enabled_operations & ~normalization_mask()) != 0 ||
        (n.model_tolerance > 0.0 && n.unit == model_unit::unspecified))
      return bad(product_error_code::input_contract_error,
                 "product_options.normalization_policy");
    switch (o.preparation.mode) {
    case preparation_mode::strict_validation:
      if (n.mode != normalization_mode::disabled ||
          n.unit != model_unit::unspecified || n.model_tolerance != 0.0 ||
          n.enabled_operations != 0)
        return bad(product_error_code::normalization_required,
                   "product_options.strict_with_normalization");
      break;
    case preparation_mode::diagnosis_only:
      if (n.mode != normalization_mode::diagnosis_only)
        return bad(product_error_code::normalization_failed,
                   "product_options.diagnosis_mode");
      break;
    case preparation_mode::normalized:
      if (n.mode != normalization_mode::structural_only &&
          n.mode != normalization_mode::geometry_changing)
        return bad(product_error_code::normalization_failed,
                   "product_options.normalized_mode");
      if (n.enabled_operations == 0)
        return bad(product_error_code::normalization_failed,
                   "product_options.normalization_empty");
      if (n.mode == normalization_mode::structural_only &&
          ((n.enabled_operations & geometry_changing_normalization_mask()) != 0 ||
           n.unit != model_unit::unspecified || n.model_tolerance != 0.0))
        return bad(product_error_code::normalization_failed,
                   "product_options.structural_normalization_scope");
      if (n.mode == normalization_mode::geometry_changing &&
          (n.unit == model_unit::unspecified || n.model_tolerance <= 0.0 ||
           (n.enabled_operations & geometry_changing_normalization_mask()) == 0))
        return bad(product_error_code::normalization_failed,
                   "product_options.geometry_normalization_tolerance");
      break;
    }

    const auto &a = o.realization.approximation;
    const auto &search = o.realization.search;
    const bool approximate_limits_disabled =
        search.max_obligations == 0 && search.max_triangle_pairs == 0 &&
        search.max_predicate_checks == 0 && search.max_verifier_work == 0 &&
        search.max_verifier_records == 0 && search.max_verifier_bytes == 0;
    if (!finite_nonnegative(a.max_vertex_displacement) ||
        !finite_nonnegative(a.max_axis_displacement_x) ||
        !finite_nonnegative(a.max_axis_displacement_y) ||
        !finite_nonnegative(a.max_axis_displacement_z) ||
        !finite_nonnegative(a.max_support_plane_deviation) ||
        !finite_nonnegative(a.declared_model_tolerance) ||
        a.candidate_generation_version != 1)
      return bad(product_error_code::approximation_policy_rejected,
                 "product_options.approximation_nonfinite");
    switch (o.result.representation) {
    case result_representation::exact_stratified:
      if (o.realization.semantics !=
              product_realization_semantics::not_requested ||
          o.realization.search.strategy != realization_search_strategy::none ||
          o.realization.search.max_candidates != 0 ||
          o.realization.search.max_candidate_evaluations != 0 ||
          o.realization.search.max_search_nodes != 0 ||
          !approximate_limits_disabled || a.enabled)
        return bad(product_error_code::input_contract_error,
                   "product_options.exact_stratified_realization");
      break;
    case result_representation::exact_in_T_mesh:
      if (o.realization.semantics !=
              product_realization_semantics::exact_in_T ||
          (o.realization.search.strategy !=
               realization_search_strategy::nearest_only &&
           o.realization.search.strategy !=
               realization_search_strategy::neighboring_values) ||
          (o.realization.search.strategy ==
               realization_search_strategy::nearest_only &&
            (o.realization.search.max_candidates != 0 ||
             o.realization.search.max_candidate_evaluations != 0 ||
             o.realization.search.max_search_nodes != 0 ||
             !approximate_limits_disabled)) ||
           (o.realization.search.strategy ==
                realization_search_strategy::neighboring_values &&
            o.realization.search.max_candidates == 0) ||
           !approximate_limits_disabled || a.enabled)
        return bad(product_error_code::input_contract_error,
                   "product_options.exact_in_T_semantics");
      break;
    case result_representation::certified_approximate_mesh:
      if (o.realization.semantics !=
              product_realization_semantics::
                  certified_approximate_embedding_v1 ||
          o.realization.search.strategy !=
              realization_search_strategy::deterministic_bounded_search ||
           !a.enabled || a.unit == model_unit::unspecified ||
           a.max_vertex_displacement <= 0.0 ||
           a.max_support_plane_deviation < 0.0 ||
           a.declared_model_tolerance <= 0.0 ||
           a.candidate_ulp_radius == 0 ||
          o.realization.search.max_candidates == 0 ||
          o.realization.search.max_candidate_evaluations == 0 ||
            o.realization.search.max_search_nodes == 0 ||
           o.realization.search.max_obligations == 0 ||
           o.realization.search.max_triangle_pairs == 0 ||
           o.realization.search.max_predicate_checks == 0 ||
           o.realization.search.max_verifier_work == 0 ||
           o.realization.search.max_verifier_records == 0 ||
           o.realization.search.max_verifier_bytes == 0 ||
          a.application_acceptance_metadata.empty())
        return bad(product_error_code::approximation_policy_rejected,
                    "product_options.approximation_contract");
      if (a.candidate_ulp_radius >
              (o.realization.search.max_candidate_evaluations - 1) / 2)
        return bad(product_error_code::approximation_policy_rejected,
                   "product_options.candidate_radius_cap");
      if ((!a.has_max_axis_displacement_x &&
           a.max_axis_displacement_x != 0.0) ||
          (!a.has_max_axis_displacement_y &&
           a.max_axis_displacement_y != 0.0) ||
          (!a.has_max_axis_displacement_z &&
           a.max_axis_displacement_z != 0.0))
        return bad(product_error_code::approximation_policy_rejected,
                   "product_options.axis_bound_presence");
      break;
    }
    if (!o.result.retain_exact_result_on_realization_failure)
      return bad(product_error_code::qualification_policy_violation,
                 "product_options.exact_retention_required");
    if (o.attributes.mode == attribute_transfer_mode::require_lossless &&
        o.attributes.conflicts != attribute_conflict_policy::reject)
      return bad(product_error_code::attribute_transfer_conflict,
                 "product_options.lossless_conflict_policy");
    if (o.qualification.manifest) {
      if (o.qualification.manifest->schema !=
              product_contract_schema_version ||
          o.qualification.manifest->identifier.empty() ||
          product_digest_is_zero(
              o.qualification.manifest->manifest_digest))
        return bad(product_error_code::qualification_policy_violation,
                   "product_options.manifest_reference");
    }
    if (o.qualification.mode ==
            qualification_policy_mode::allow_explicit_unqualified &&
        o.backend.mode == backend_selection_mode::qualified_default)
      return bad(product_error_code::qualification_policy_violation,
                 "product_options.unqualified_default");
    return true;
  } catch (...) {
    return bad(product_error_code::internal_invariant_error,
               "product_options.validation_exception");
  }
}
