#include "MeshBooleanOutputFixtures.h"
#include <YgorMeshesBooleanExactResult.h>
#include <YgorMeshesBooleanPreparation.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ygor::mesh_boolean;

namespace {

std::string executable_path;

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

digest marked_digest(char marker) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(marker));
  return domain_digest({{'Y', 'G', 'B', 'P', '1', 'T', 'S', 'T'}},
                       encoder.bytes());
}

backend_identity test_backend() {
  backend_capabilities capabilities;
  capabilities.set(backend_capability::exact_set_semantics);
  capabilities.set(backend_capability::exact_coordinates);
  capabilities.set(backend_capability::stratified_output);
  capabilities.set(backend_capability::deterministic_canonical_output);
  capabilities.set(backend_capability::certified_failure_categories);
  capabilities.set(backend_capability::provenance_mapping);
  capabilities.set(backend_capability::strict_prepared_operands);
  auto made = make_backend_identity(backend_id::experimental_exact_v1,
                                    {1, 0, 0}, "p1-test", capabilities,
                                    backend_maturity::experimental);
  require(made.has_value(), "backend identity");
  return made.value();
}

exact_stratified_boundary empty_boundary() {
  exact_stratified_boundary boundary;
  boundary.backend.producer = test_backend();
  boundary.backend.attempted_backends = {backend_id::experimental_exact_v1};
  boundary.preparation.input_digest = marked_digest('i');
  boundary.preparation.prepared_digest = marked_digest('p');
  boundary.preparation.policy_digest = marked_digest('o');
  boundary.preparation.report_digest = marked_digest('r');
  boundary.setup_digest = marked_digest('s');
  boundary.labeled_digest = marked_digest('l');
  boundary.arrangement_digest = marked_digest('a');
  boundary.selected_artifact_digest = marked_digest('x');
  boundary.selected_semantic_digest = marked_digest('m');
  boundary.certificate.id = selection_certificate_id::from_canonical_value(0);
  boundary.certificate.semantic_digest = boundary.selected_semantic_digest;
  return boundary;
}

void round_trip_and_lifetime() {
  exact_result_handle durable;
  std::vector<std::uint8_t> canonical_bytes;
  {
    auto frozen = freeze_exact_stratified_boundary(empty_boundary());
    require(frozen.has_value(), "freeze empty exact result");
    auto made = make_exact_result_handle(frozen.value());
    require(made.has_value(), "typed durable handle");
    durable = made.value();
    canonical_bytes = durable->canonical_bytes;
  }
  const auto &bytes = canonical_bytes;
  require(!bytes.empty(), "encode exact result");
  auto decoded = decode_exact_stratified_boundary(bytes);
  require(decoded.has_value(), "decode exact result");
  auto encoded_again = encode_exact_stratified_boundary(*decoded.value());
  require(encoded_again.has_value() && encoded_again.value() == bytes,
          "canonical round trip");
  require(verify_serialized_exact_stratified_boundary(bytes).has_value(),
          "independent serialized verifier");
  const auto replay_path =
      std::filesystem::temp_directory_path() /
      ("ygor-exact-result-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()) +
       ".bin");
  {
    std::ofstream output(replay_path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    require(output.good(), "write cross-process replay fixture");
  }
  const auto command = "\"" + executable_path + "\" --verify-file \"" +
                       replay_path.string() + "\"";
  const auto child_status = std::system(command.c_str());
  std::filesystem::remove(replay_path);
  require(child_status == 0, "cross-process exact-result replay");

  auto retained = read_exact_result(durable);
  require(retained.has_value() &&
              retained.value()->topology == selected_boundary_topology::empty &&
              durable.use_count() == 1,
          "handle owns result independently");

  product_realization_policy exact_coordinates;
  auto float_request = request_later_realization<float, std::uint32_t>(
      durable, result_representation::exact_stratified, exact_coordinates);
  auto double_request = request_later_realization<double, std::uint64_t>(
      durable, result_representation::exact_stratified, exact_coordinates);
  require(float_request.has_value() && double_request.has_value() &&
              float_request.value().coordinate == coordinate_tag::binary32 &&
              double_request.value().coordinate == coordinate_tag::binary64 &&
              float_request.value().exact_result_digest ==
                  durable->canonical_digest,
          "later realization requests bind coordinate and index types");
  auto export32 = export_exact_coordinates<std::uint32_t>(durable);
  auto export64 = export_exact_coordinates<std::uint64_t>(durable);
  require(export32.has_value() && export64.has_value() &&
              export32.value()->index == index_tag::uint32 &&
              export64.value()->index == index_tag::uint64 &&
              export32.value()->entity_capacity ==
                  std::numeric_limits<std::uint32_t>::max() &&
              export32.value()->canonical_bytes !=
                  export64.value()->canonical_bytes,
          "exact-coordinate export binds index width");
  auto decoded_export =
      decode_exact_coordinate_export(export32.value()->canonical_bytes);
  require(decoded_export.has_value() && decoded_export.value()->boundary &&
              decoded_export.value()->boundary->vertices.empty() &&
              verify_serialized_exact_coordinate_export(
                  export32.value()->canonical_bytes)
                  .has_value() &&
              validate_exact_coordinate_export_binding(decoded_export.value(),
                                                       durable)
                  .has_value(),
          "exact-coordinate export canonical round trip and binding");
  const auto export_replay_path =
      std::filesystem::temp_directory_path() /
      ("ygor-exact-coordinate-export-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()) +
       ".bin");
  {
    std::ofstream output(export_replay_path, std::ios::binary);
    const auto &export_bytes = export32.value()->canonical_bytes;
    output.write(reinterpret_cast<const char *>(export_bytes.data()),
                 export_bytes.size());
    require(output.good(), "write exact-coordinate export replay fixture");
  }
  const auto export_command = "\"" + executable_path +
                              "\" --verify-export-file \"" +
                              export_replay_path.string() + "\"";
  const auto export_child_status = std::system(export_command.c_str());
  std::filesystem::remove(export_replay_path);
  require(export_child_status == 0,
          "cross-process exact-coordinate export replay");
  product_realization_policy invalid_mesh_policy;
  invalid_mesh_policy.semantics = product_realization_semantics::exact_in_T;
  require(
      !request_later_realization<double, std::uint64_t>(
           durable, result_representation::exact_in_T_mesh, invalid_mesh_policy)
           .has_value(),
      "later realization rejects incomplete nested policy");
}

void malformed_and_limits() {
  auto frozen = freeze_exact_stratified_boundary(empty_boundary());
  require(frozen.has_value(), "freeze exact result for mutation");
  auto encoded = encode_exact_stratified_boundary(*frozen.value());
  require(encoded.has_value(), "encode exact result for mutation");

  auto corrupt = encoded.value();
  corrupt[corrupt.size() / 2] ^= 1;
  require(!decode_exact_stratified_boundary(corrupt).has_value(),
          "corruption rejected");
  auto truncated = encoded.value();
  truncated.pop_back();
  require(!decode_exact_stratified_boundary(truncated).has_value(),
          "truncation rejected");
  auto trailing = encoded.value();
  trailing.push_back(0);
  require(!decode_exact_stratified_boundary(trailing).has_value(),
          "trailing bytes rejected");

  exact_result_decode_limits limits;
  limits.max_record_bytes = encoded.value().size() - 1;
  auto limited = decode_exact_stratified_boundary(encoded.value(), limits);
  require(!limited.has_value() &&
              limited.error().code == product_error_code::resource_limit,
          "record resource limit is typed");

  auto handle = make_exact_result_handle(frozen.value());
  require(handle.has_value(), "make exact result for export mutation");
  auto exported = export_exact_coordinates<std::uint64_t>(handle.value());
  require(exported.has_value(), "make exact-coordinate export for mutation");
  auto export_corrupt = exported.value()->canonical_bytes;
  export_corrupt[export_corrupt.size() / 2] ^= 1U;
  require(!decode_exact_coordinate_export(export_corrupt).has_value(),
          "exact-coordinate export corruption rejected");
  auto export_truncated = exported.value()->canonical_bytes;
  export_truncated.pop_back();
  require(!decode_exact_coordinate_export(export_truncated).has_value(),
          "exact-coordinate export truncation rejected");
  exact_coordinate_export_limits export_limits;
  export_limits.max_record_bytes = exported.value()->canonical_bytes.size() - 1;
  auto export_limited = decode_exact_coordinate_export(
      exported.value()->canonical_bytes, export_limits);
  require(!export_limited.has_value() &&
              export_limited.error().code == product_error_code::resource_limit,
          "exact-coordinate export record limit is typed");
  export_limits.max_record_bytes = exported.value()->canonical_bytes.size();
  auto exact_limit_export = export_exact_coordinates<std::uint64_t>(
      handle.value(), export_limits);
  require(exact_limit_export.has_value() &&
              exact_limit_export.value()->canonical_bytes.size() ==
                  export_limits.max_record_bytes,
          "exact-coordinate export accepts exact record limit");
  exact_coordinate_export_limits inner_limits;
  inner_limits.exact_result.max_record_bytes =
      handle.value()->canonical_bytes.size() - 1;
  auto inner_limited = verify_serialized_exact_coordinate_export(
      exported.value()->canonical_bytes, inner_limits);
  require(!inner_limited.has_value() &&
              inner_limited.error().code == product_error_code::resource_limit,
          "exact-coordinate verifier types embedded record limit");
  auto stale_export =
      std::make_shared<exact_coordinate_export>(*exported.value());
  stale_export->exact_result_digest.bytes[0] ^= 1U;
  require(!validate_exact_coordinate_export_binding(stale_export,
                                                    handle.value())
               .has_value(),
          "exact-coordinate export stale binding rejected");
}

void stale_bindings() {
  auto boundary = empty_boundary();
  auto frozen = freeze_exact_stratified_boundary(boundary);
  require(frozen.has_value(), "freeze exact result for binding test");
  auto handle = make_exact_result_handle(frozen.value());
  require(handle.has_value(), "make exact handle for binding test");

  backend_provenance backend;
  backend.producer = boundary.backend.producer;
  backend.attempted_backends = boundary.backend.attempted_backends;
  preparation_provenance preparation;
  preparation.input_digest = boundary.preparation.input_digest;
  preparation.prepared_digest = boundary.preparation.prepared_digest;
  preparation.policy_digest = boundary.preparation.policy_digest;
  preparation.report_digest = boundary.preparation.report_digest;
  require(validate_durable_exact_result_bindings(handle.value(), backend,
                                                 preparation)
              .has_value(),
          "matching envelope bindings");
  preparation.report_digest.bytes[0] ^= 1;
  require(!validate_durable_exact_result_bindings(handle.value(), backend,
                                                  preparation)
               .has_value(),
          "stale preparation binding rejected");
  preparation.report_digest = boundary.preparation.report_digest;
  backend.producer.build_identifier += "-stale";
  require(!validate_durable_exact_result_bindings(handle.value(), backend,
                                                  preparation)
               .has_value(),
          "stale backend binding rejected");
}

void retain_nonmanifold_after_context_destruction() {
  auto durable = [] {
    auto a = input_test::box<double, std::uint32_t>(0.0, 2.0);
    auto b = input_test::box<double, std::uint32_t>(1.0, 3.0);
    auto context = classification_test::context(
        a, b, selection_test::registry(), operation::symmetric_difference,
        boolean_options{});
    auto selected = select_boolean_boundary(*context);
    require(selected.has_value(), "select stratified exact boundary");
    require(selected.value()->payload->topology ==
                selected_boundary_topology::closed_stratified_nonmanifold,
            "stratified fixture topology");

    exact_result_backend_binding backend;
    backend.producer = test_backend();
    backend.attempted_backends = {backend_id::experimental_exact_v1};
    exact_result_preparation_binding preparation;
    preparation.input_digest = marked_digest('u');
    preparation.prepared_digest = marked_digest('v');
    preparation.policy_digest = marked_digest('w');
    preparation.report_digest = marked_digest('z');
    auto published = evaluate_boolean_product_result(
        *context, std::move(backend), std::move(preparation),
        result_representation::exact_in_T_mesh);
    require(published.has_value() &&
                published.value()->representation ==
                    result_representation::exact_stratified &&
                published.value()->realization &&
                !published.value()->realization->succeeded &&
                published.value()->realization->failure &&
                published.value()->realization->failure->code ==
                    product_error_code::result_topology_not_supported,
            "actual mesh rejection retains exact result");
    const auto &bytes = published.value()->exact_result->canonical_bytes;
    for (auto limited :
         {exact_result_decode_limits{bytes.size(), 1, 32U * 1024U * 1024U,
                                     8U * 1024U * 1024U, 16U * 1024U},
          exact_result_decode_limits{bytes.size(), 4U * 1024U * 1024U, 0,
                                     8U * 1024U * 1024U, 16U * 1024U},
          exact_result_decode_limits{bytes.size(), 4U * 1024U * 1024U,
                                     32U * 1024U * 1024U, 1, 16U * 1024U},
          exact_result_decode_limits{bytes.size(), 4U * 1024U * 1024U,
                                     32U * 1024U * 1024U, 8U * 1024U * 1024U,
                                     1}}) {
      auto rejected = decode_exact_stratified_boundary(bytes, limited);
      require(!rejected.has_value() &&
                  rejected.error().code == product_error_code::resource_limit,
              "all exact-result decode limits are typed");
    }
    auto decoded = read_exact_result(published.value()->exact_result);
    require(decoded.has_value(), "read published stratified result");
    auto exported = export_exact_coordinates<std::uint32_t>(
        published.value()->exact_result);
    require(exported.has_value() &&
                exported.value()->topology ==
                    selected_boundary_topology::closed_stratified_nonmanifold &&
                exported.value()->boundary->vertex_occurrences.size() ==
                    decoded.value()->vertex_occurrences.size() &&
                exported.value()->boundary->halfedges.size() ==
                    decoded.value()->halfedges.size() &&
                !exported.value()->boundary->topology_obstructions.empty(),
            "exact-coordinate export preserves non-manifold occurrences");
    auto encoded_export = encode_exact_coordinate_export(exported.value());
    require(encoded_export.has_value(),
            "encode non-empty exact-coordinate export");
    auto decoded_export =
        decode_exact_coordinate_export(encoded_export.value());
    require(decoded_export.has_value() &&
                decoded_export.value()->canonical_bytes ==
                    exported.value()->canonical_bytes,
            "non-empty exact-coordinate export round trip");
    auto mutated_export =
        std::make_shared<exact_coordinate_export>(*exported.value());
    auto mutated_boundary = std::make_shared<exact_stratified_boundary>(
        *mutated_export->boundary);
    require(!mutated_boundary->vertices.empty(),
            "exact-coordinate boundary mutation fixture");
    mutated_boundary->vertices.front().coordinate.x =
        mutated_boundary->vertices.front().coordinate.x + exact_scalar(1);
    mutated_export->boundary = std::move(mutated_boundary);
    require(!validate_exact_coordinate_export_binding(
                 mutated_export, published.value()->exact_result)
                 .has_value(),
            "mutated in-memory exact-coordinate boundary rejected");
    exact_coordinate_export_limits export_limits;
    export_limits.max_indexed_entities = 1;
    auto limited_export = decode_exact_coordinate_export(
        exported.value()->canonical_bytes, export_limits);
    require(!limited_export.has_value() &&
                limited_export.error().code ==
                    product_error_code::resource_limit,
            "exact-coordinate export entity capacity is enforced");
    auto stale_certificate = *decoded.value();
    ++stale_certificate.certificate.selected_vertices;
    require(!freeze_exact_stratified_boundary(std::move(stale_certificate))
                 .has_value(),
            "stale selection certificate rejected");
    auto stale_relation = *decoded.value();
    auto constructed = std::find_if(
        stale_relation.vertices.begin(), stale_relation.vertices.end(),
        [](const auto &vertex) { return !vertex.constructions.empty(); });
    require(constructed != stale_relation.vertices.end(),
            "constructed vertex mutation fixture");
    constructed->coordinate.x = constructed->coordinate.x + exact_scalar(1);
    require(!freeze_exact_stratified_boundary(std::move(stale_relation))
                 .has_value(),
            "defining relation mutation rejected");
    auto orphan_relation = *decoded.value();
    auto relation_owner = std::find_if(
        orphan_relation.constructions.begin(),
        orphan_relation.constructions.end(), [](const auto &construction) {
          return !construction.defining_relations.empty();
        });
    require(relation_owner != orphan_relation.constructions.end(),
            "defining relation mutation fixture");
    relation_owner->defining_relations.clear();
    require(!freeze_exact_stratified_boundary(std::move(orphan_relation))
                 .has_value(),
            "orphan defining relation rejected");
    auto stale_link = *decoded.value();
    auto linked_occurrence = std::find_if(
        stale_link.source_vertex_occurrences.begin(),
        stale_link.source_vertex_occurrences.end(), [](const auto &occurrence) {
          return !occurrence.link_regions.empty();
        });
    require(linked_occurrence != stale_link.source_vertex_occurrences.end(),
            "spherical-link mutation fixture");
    linked_occurrence->link_regions.clear();
    require(
        !freeze_exact_stratified_boundary(std::move(stale_link)).has_value(),
        "spherical-link rebinding rejected");
    return published.value()->exact_result;
  }();
  auto retained = read_exact_result(durable);
  require(retained.has_value() &&
              retained.value()->topology ==
                  selected_boundary_topology::closed_stratified_nonmanifold &&
              !retained.value()->topology_obstructions.empty(),
          "nonmanifold exact result retained without context");
}

void product_evaluator_mesh_paths() {
  auto run = [](auto a, auto b, operation op,
                product_error_code expected_failure) {
    auto context = classification_test::context(a, b, output_test::registry(),
                                                op, boolean_options{});
    exact_result_backend_binding backend;
    backend.producer = test_backend();
    backend.attempted_backends = {backend_id::experimental_exact_v1};
    exact_result_preparation_binding preparation;
    preparation.input_digest = marked_digest('1');
    preparation.prepared_digest = marked_digest('2');
    preparation.policy_digest = marked_digest('3');
    preparation.report_digest = marked_digest('4');
    auto invalid = evaluate_boolean_product_result(
        *context, backend, preparation,
        static_cast<result_representation>(0xff));
    require(!invalid.has_value() &&
                invalid.error().code ==
                    product_error_code::input_contract_error,
            "unknown product representation rejected");
    auto result = evaluate_boolean_product_result(
        *context, std::move(backend), std::move(preparation),
        result_representation::exact_in_T_mesh);
    require(result.has_value(), "product evaluator result");
    if (expected_failure == product_error_code::internal_invariant_error) {
      require(result.value()->representation ==
                      result_representation::exact_in_T_mesh &&
                  result.value()->mesh && result.value()->realization &&
                  result.value()->realization->succeeded &&
                  result.value()->mesh->obligation_count >=
                      result.value()->mesh->strict_vertices.size() * 3 &&
                  result.value()->mesh->defining_relation_obligation_count <=
                      result.value()->mesh->obligation_count &&
                  validate_product_result(*result.value()).has_value(),
              "successful exact-in-T product publication");
      auto changed = *result.value();
      auto changed_success =
          std::make_shared<boolean_success<double, std::uint32_t>>(
              *changed.mesh->success);
      require(!changed_success->mesh.vertices.empty(),
              "strict envelope mutation fixture");
      changed_success->mesh.vertices[0].x =
          std::nextafter(changed_success->mesh.vertices[0].x,
                         std::numeric_limits<double>::infinity());
      changed.mesh->success = std::move(changed_success);
      require(!validate_product_result(changed).has_value(),
              "one-ULP mesh mutation rejected by strict binding");
      changed = *result.value();
      changed.mesh->realization_canonical_bytes.back() ^= 1;
      require(!validate_product_result(changed).has_value(),
              "realization evidence mutation rejected");

      std::shared_ptr<const exact_kernel_services<double>> reingest_kernel =
          std::make_shared<exact_kernel<double>>();
      std::shared_ptr<const verifier_service> reingest_verifiers =
          output_test::registry();
      const auto reingested = validate_operand_strict(
          result.value()->mesh->success->mesh, strict_validation_policy{},
          boolean_options{}, std::move(reingest_kernel),
          std::move(reingest_verifiers));
      require(reingested.has_value(),
              "strict envelope mesh passes Component 2 re-ingestion");
    } else {
      require(result.value()->representation ==
                      result_representation::exact_stratified &&
                  result.value()->exact_result.valid() &&
                  result.value()->realization &&
                  result.value()->realization->failure &&
                  result.value()->realization->failure->code ==
                      expected_failure &&
                  !product_digest_is_zero(
                      result.value()->realization->failure
                          ->replay_binding_digest),
              "failed realization retains exact authority");
      auto exact_export = export_exact_coordinates<std::uint64_t>(
          result.value()->exact_result);
      require(exact_export.has_value() &&
                  !exact_export.value()->boundary->vertices.empty(),
              "failed finite realization remains exactly exportable");
      if (expected_failure == product_error_code::output_not_representable) {
        const auto third = exact_scalar(1) / exact_scalar(3);
        const auto has_third = std::any_of(
            exact_export.value()->boundary->vertices.begin(),
            exact_export.value()->boundary->vertices.end(),
            [&](const auto &vertex) {
              return vertex.coordinate.x == third ||
                     vertex.coordinate.y == third ||
                     vertex.coordinate.z == third;
            });
        require(has_third,
                "non-dyadic exact coordinate exports without rounding");
        auto encoded = encode_exact_coordinate_export(exact_export.value());
        require(encoded.has_value(), "encode non-dyadic exact export");
        auto decoded = decode_exact_coordinate_export(encoded.value());
        require(decoded.has_value() &&
                    std::any_of(
                        decoded.value()->boundary->vertices.begin(),
                        decoded.value()->boundary->vertices.end(),
                        [&](const auto &vertex) {
                          return vertex.coordinate.x == third ||
                                 vertex.coordinate.y == third ||
                                 vertex.coordinate.z == third;
                        }),
                "non-dyadic exact coordinate survives canonical replay");
      }
    }
  };

  auto disjoint_a = input_test::cube<double, std::uint32_t>();
  auto disjoint_b = input_test::cube<double, std::uint32_t>();
  symbolic_test::translate(disjoint_b, 3.0, 0.0, 0.0);
  run(disjoint_a, disjoint_b, operation::regularized_union,
      product_error_code::internal_invariant_error);
  run(input_test::cube<double, std::uint32_t>(),
      input_test::third_intersection_prism<double, std::uint32_t>(),
      operation::regularized_intersection,
      product_error_code::output_not_representable);
}

void prepared_product_binding() {
  using T = double;
  using I = std::uint32_t;
  auto verifiers = output_test::registry();
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = verifiers;
  auto make_prepared = [&](const fv_surface_mesh<T, I> &mesh) {
    auto made = validate_operand_strict(mesh, strict_validation_policy{},
                                        boolean_options{}, kernel, verifier);
    require(made.has_value(), "prepare product operand");
    return made.value();
  };
  auto a = make_prepared(input_test::box<T, I>(T(0), T(1)));
  auto b = make_prepared(input_test::box<T, I>(T(2), T(3)));
  auto context = make_boolean_context(
      a, b, operation::regularized_union, boolean_options{}, kernel, verifier);
  require(context.has_value() && context.value()->preparation_provenance(),
          "prepared product context provenance");
  const auto &bound = *context.value()->preparation_provenance();
  exact_result_preparation_binding preparation;
  preparation.mode = preparation_mode::strict_validation;
  preparation.input_digest = bound.input_digest;
  preparation.prepared_digest = bound.prepared_digest;
  preparation.policy_digest = bound.policy_digest;
  preparation.report_digest = bound.report_digest;
  preparation.geometry_changed = bound.geometry_changed;
  exact_result_backend_binding backend;
  backend.producer = test_backend();
  backend.attempted_backends = {backend_id::experimental_exact_v1};
  auto stale = preparation;
  stale.report_digest.bytes[0] ^= 1U;
  auto selected = select_boolean_boundary(*context.value());
  require(selected.has_value(), "prepared direct detach fixture");
  auto direct_rejected = detach_exact_stratified_boundary(
      *selected.value()->payload, backend, stale);
  require(!direct_rejected.has_value() &&
              direct_rejected.error().code == product_error_code::stale_binding,
          "direct detach rejects foreign preparation provenance");
  auto wrong_mode = preparation;
  wrong_mode.mode = preparation_mode::normalized;
  auto mode_rejected = detach_exact_stratified_boundary(
      *selected.value()->payload, backend, wrong_mode);
  require(!mode_rejected.has_value() &&
              mode_rejected.error().code == product_error_code::stale_binding,
          "direct detach rejects foreign preparation mode");
  auto mutated_selection = *selected.value()->payload;
  mutated_selection.preparation_provenance->report_digest.bytes[0] ^= 1U;
  auto matching_mutation = preparation;
  matching_mutation.report_digest =
      mutated_selection.preparation_provenance->report_digest;
  auto unverified = detach_exact_stratified_boundary(
      mutated_selection, backend, matching_mutation);
  require(!unverified.has_value() &&
              unverified.error().code == product_error_code::stale_binding,
          "detach rejects unverified selected preparation provenance");
  auto rejected = evaluate_boolean_product_result(
      *context.value(), backend, stale, result_representation::exact_stratified);
  require(!rejected.has_value() &&
              rejected.error().code == product_error_code::stale_binding,
          "product evaluator rejects foreign preparation provenance");
  auto published = evaluate_boolean_product_result(
      *context.value(), std::move(backend), preparation,
      result_representation::exact_stratified);
  require(published.has_value() &&
              published.value()->preparation.report_digest ==
                  bound.report_digest,
          "product result retains prepared context provenance");
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--verify-file") {
    std::ifstream input(argv[2], std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                    std::istreambuf_iterator<char>());
    return input.bad() || !verify_serialized_exact_stratified_boundary(bytes)
                               .has_value()
               ? 1
               : 0;
  }
  if (argc == 3 && std::string(argv[1]) == "--verify-export-file") {
    std::ifstream input(argv[2], std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                    std::istreambuf_iterator<char>());
    return input.bad() ||
                   !verify_serialized_exact_coordinate_export(bytes).has_value()
               ? 1
               : 0;
  }
  executable_path = argv[0];
  try {
    round_trip_and_lifetime();
    malformed_and_limits();
    stale_bindings();
    retain_nonmanifold_after_context_destruction();
    product_evaluator_mesh_paths();
    prepared_product_binding();
    std::cout << "ok\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
