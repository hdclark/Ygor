#include <YgorMeshesBooleanService.h>

#include <cstdint>
#include <iostream>
#include <string>

namespace {

using namespace ygor::mesh_boolean;
using mesh_type = fv_surface_mesh<double, std::uint32_t>;

mesh_type make_box(double lo, double hi) {
  mesh_type mesh;
  mesh.vertices = {{lo, lo, lo}, {hi, lo, lo}, {hi, hi, lo}, {lo, hi, lo},
                   {lo, lo, hi}, {hi, lo, hi}, {hi, hi, hi}, {lo, hi, hi}};
  mesh.faces = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
  return mesh;
}

mesh_type make_third_intersection_prism() {
  mesh_type mesh;
  mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 3.0, 0.0}, {0.0, 3.0, 0.0},
                   {0.0, 0.0, 1.0}, {1.0, 3.0, 1.0}, {0.0, 3.0, 1.0}};
  mesh.faces = {{0, 2, 1}, {3, 4, 5}, {0, 1, 4, 3},
                {1, 2, 5, 4}, {2, 0, 3, 5}};
  return mesh;
}

void report_error(const char *where, const product_error &error) {
  std::cerr << where << ": product error "
            << static_cast<unsigned>(error.code) << " (" << error.message_key
            << ')';
  if (!error.detail.empty())
    std::cerr << ": " << error.detail;
  std::cerr << '\n';
}

boolean_service_options experimental_options(result_representation result) {
  boolean_service_options options;

  // The built-in backend is experimental, so ordinary callers must opt in to
  // both its identity/maturity and explicit unqualified use. The conservative
  // default never makes this choice silently.
  options.product.backend.mode = backend_selection_mode::explicit_backend;
  options.product.backend.requested_backend =
      backend_id::experimental_exact_v1;
  options.product.backend.allow_experimental_backend = true;
  options.product.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  options.product.result.representation = result;
  options.product.attributes.mode =
      attribute_transfer_mode::preserve_supported_with_report;

  if (result == result_representation::exact_in_T_mesh) {
    options.product.realization.semantics =
        product_realization_semantics::exact_in_T;
    options.product.realization.search.strategy =
        realization_search_strategy::nearest_only;
  } else if (result ==
             result_representation::certified_approximate_mesh) {
    auto &realization = options.product.realization;
    realization.semantics = product_realization_semantics::
        certified_approximate_embedding_v1;
    realization.search.strategy =
        realization_search_strategy::deterministic_bounded_search;
    realization.search.max_candidates = 27;
    realization.search.max_candidate_evaluations = 100000;
    realization.search.max_search_nodes = 1000000;
    realization.search.max_obligations = 1000000;
    realization.search.max_triangle_pairs = 1000000;
    realization.search.max_predicate_checks = 100000000;
    realization.search.max_verifier_work = 100000000;
    realization.search.max_verifier_records = 10000000;
    realization.search.max_verifier_bytes = 256U * 1024U * 1024U;
    realization.approximation.enabled = true;
    realization.approximation.unit = model_unit::unitless;
    realization.approximation.max_vertex_displacement = 1.0e-12;
    realization.approximation.max_support_plane_deviation = 1.0e-12;
    realization.approximation.declared_model_tolerance = 1.0e-11;
    realization.approximation.candidate_generation_version = 1;
    realization.approximation.candidate_ulp_radius = 1;
    realization.approximation.application_acceptance_metadata =
        "example-unitless-model-tolerance";
  }
  return options;
}

bool check_product(const boolean_product_result<double, std::uint32_t> &product,
                   const char *where) {
  const auto checked = validate_product_result(product);
  if (!checked.has_value()) {
    report_error(where, checked.error());
    return false;
  }
  return true;
}

void report_attributes(
    const boolean_product_result<double, std::uint32_t> &product) {
  std::cout << "attributes: " << product.attributes.transfers.size()
            << " transfers, " << product.attributes.omissions
            << " omissions, " << product.attributes.conflicts
            << " conflicts\n";
}

} // namespace

int main() {
  const mesh_type a = make_box(0.0, 1.0);
  const mesh_type b = make_box(3.0, 4.0);

  // Conservative defaults require a qualified backend/profile. They do not
  // silently select the experimental in-tree backend.
  const auto conservative =
      boolean_operation(a, b, operation::regularized_union);
  if (conservative.has_value() ||
      conservative.error().code != product_error_code::backend_unqualified) {
    std::cerr << "conservative default did not fail closed\n";
    return 1;
  }
  report_error("expected conservative-default rejection", conservative.error());

  // Strict preparation plus exact-in-double mesh output for known-provenance,
  // already-valid B-reps.
  auto strict_options =
      experimental_options(result_representation::exact_in_T_mesh);
  const auto strict =
      boolean_operation(a, b, operation::regularized_union, strict_options);
  if (!strict.has_value()) {
    report_error("strict exact mesh", strict.error());
    return 1;
  }
  const auto &strict_product = *strict.value();
  if (!check_product(strict_product, "strict exact mesh") ||
      !strict_product.mesh || !strict_product.mesh->success ||
      strict_product.backend.producer.maturity != backend_maturity::experimental) {
    std::cerr << "strict exact mesh result was incomplete\n";
    return 1;
  }
  const auto &strict_mesh = strict_product.mesh->success->mesh;
  std::cout << "strict exact mesh: " << strict_mesh.vertices.size()
            << " vertices, " << strict_mesh.faces.size() << " faces\n";
  report_attributes(strict_product);

  // Normalization is a separate, explicit preparation choice. This structural
  // policy removes only irrelevant storage; it is not automatic healing.
  mesh_type normalized_a = a;
  mesh_type normalized_b = b;
  normalized_a.vertices.push_back({99.0, 99.0, 99.0});
  normalized_b.vertices.push_back({98.0, 98.0, 98.0});
  auto normalized_options =
      experimental_options(result_representation::exact_stratified);
  normalization_policy normalization;
  normalization.mode = normalization_mode::structural_only;
  normalization.enabled_operations = normalization_operation_bit(
      normalization_operation::irrelevant_storage_removal);
  normalized_options.normalization = normalization;
  normalized_options.product.preparation.mode = preparation_mode::normalized;
  auto &bound = normalized_options.product.preparation.normalization;
  bound.mode = normalization.mode;
  bound.unit = normalization.unit;
  bound.model_tolerance = normalization.model_tolerance;
  bound.enabled_operations = normalization.enabled_operations;
  bound.nonplanar_facets = normalization.nonplanar_facets;
  const auto normalized = boolean_operation(normalized_a, normalized_b,
                                            operation::regularized_union,
                                            normalized_options);
  if (!normalized.has_value()) {
    report_error("explicit normalization", normalized.error());
    return 1;
  }
  const auto &normalized_product = *normalized.value();
  if (!check_product(normalized_product, "explicit normalization") ||
      normalized_product.preparation.mode != preparation_mode::normalized ||
      normalized_product.preparation.input_digest ==
          normalized_product.preparation.prepared_digest) {
    std::cerr << "normalization provenance was not retained\n";
    return 1;
  }
  std::cout << "explicit structural normalization retained an auditable report\n";

  // A one-third intersection is not exactly representable in binary64. The
  // failed exact-in-T realization remains a successful durable exact result.
  const mesh_type cube = make_box(0.0, 1.0);
  const mesh_type prism = make_third_intersection_prism();
  auto retained_options =
      experimental_options(result_representation::exact_in_T_mesh);
  retained_options.product.result.retain_exact_result_on_realization_failure =
      true;
  const auto retained = boolean_operation(cube, prism,
                                          operation::regularized_intersection,
                                          retained_options);
  if (!retained.has_value()) {
    report_error("retained exact authority", retained.error());
    return 1;
  }
  const auto &retained_product = *retained.value();
  if (!check_product(retained_product, "retained exact authority") ||
      retained_product.representation !=
          result_representation::exact_stratified ||
      retained_product.mesh || !retained_product.exact_result.valid() ||
      !retained_product.realization ||
      !retained_product.realization->failure ||
      retained_product.realization->failure->code !=
          product_error_code::output_not_representable) {
    std::cerr << "finite realization failure erased exact authority\n";
    return 1;
  }
  std::cout << "exact authority retained after exact-in-T realization failure\n";

  // Approximate geometry is requested and labelled separately, with an
  // application-declared tolerance and an independently verified certificate.
  const auto approximate = boolean_operation(
      cube, prism, operation::regularized_intersection,
      experimental_options(result_representation::certified_approximate_mesh));
  if (!approximate.has_value()) {
    report_error("certified approximate mesh", approximate.error());
    return 1;
  }
  const auto &approximate_product = *approximate.value();
  if (!check_product(approximate_product, "certified approximate mesh") ||
      approximate_product.representation !=
          result_representation::certified_approximate_mesh ||
      !approximate_product.mesh || !approximate_product.mesh->success ||
      !approximate_product.mesh->approximate_certificate) {
    std::cerr << "approximate mesh lacked its certificate\n";
    return 1;
  }
  std::cout << "certified approximate mesh: "
            << approximate_product.mesh->success->mesh.vertices.size()
            << " vertices\n";
  report_attributes(approximate_product);

  // Imported STL/OBJ/scan/CAD tessellations of unknown provenance must first be
  // diagnosed and reviewed under an explicit preparation policy. Passing them
  // to the strict path does not imply repair. This open mesh demonstrates the
  // typed failure returned by strict validation.
  mesh_type open = a;
  open.faces.pop_back();
  const auto malformed = boolean_operation(
      open, b, operation::regularized_union,
      experimental_options(result_representation::exact_stratified));
  if (malformed.has_value() ||
      malformed.error().code != product_error_code::input_contract_error) {
    std::cerr << "malformed input did not produce a typed contract error\n";
    return 1;
  }
  report_error("expected strict-input rejection", malformed.error());

  return 0;
}
