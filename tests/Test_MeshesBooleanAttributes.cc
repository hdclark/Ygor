#include "MeshBooleanOutputFixtures.h"

#include <YgorMeshesBooleanAttributes.h>
#include <YgorMeshesBooleanExactResult.h>
#include <YgorMeshesBooleanNormalization.h>
#include <YgorMeshesBooleanPreparation.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

using namespace ygor::mesh_boolean;

namespace {

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

digest marked_digest(char marker) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(marker));
  return domain_digest({{'Y', 'G', 'B', 'P', '5', 'A', 'T', marker}},
                       encoder.bytes());
}

backend_identity test_backend() {
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
  auto made = make_backend_identity(backend_id::experimental_exact_v1,
                                    {1, 0, 0}, "p5-attribute-test",
                                    capabilities,
                                    backend_maturity::experimental);
  require(made.has_value(), "backend identity");
  return made.value();
}

exact_result_backend_binding backend_binding() {
  exact_result_backend_binding backend;
  backend.producer = test_backend();
  backend.attempted_backends = {backend_id::experimental_exact_v1};
  return backend;
}

exact_result_preparation_binding preparation_binding() {
  exact_result_preparation_binding preparation;
  preparation.input_digest = marked_digest('i');
  preparation.prepared_digest = marked_digest('p');
  preparation.policy_digest = marked_digest('o');
  preparation.report_digest = marked_digest('r');
  return preparation;
}

attribute_transfer_policy_contract preserve_policy() {
  attribute_transfer_policy_contract policy;
  policy.mode = attribute_transfer_mode::preserve_supported_with_report;
  policy.conflicts = attribute_conflict_policy::report_and_omit;
  return policy;
}

template <class T, class I>
void populate_vertex_attributes(fv_surface_mesh<T, I> &mesh,
                                std::uint32_t colour_base) {
  mesh.vertex_normals.resize(mesh.vertices.size(), vec3<T>(T(1), T(0), T(0)));
  mesh.vertex_colours.resize(mesh.vertices.size());
  for (std::size_t i = 0; i < mesh.vertex_colours.size(); ++i)
    mesh.vertex_colours[i] = colour_base + static_cast<std::uint32_t>(i);
  mesh.metadata["body-channel"] = std::to_string(colour_base);
}

source_attribute_input facet_materials(operand_id operand,
                                       std::size_t facet_count,
                                       const std::string &material) {
  source_attribute_input input;
  input.operand = operand;
  input.body_id = operand == operand_a() ? "body-A" : "body-B";
  for (std::size_t facet = 0; facet < facet_count; ++facet) {
    source_attribute_value_input value;
    value.entity = {operand, attribute_source_entity_kind::facet, facet, 0};
    value.channel = attribute_channel_kind::material;
    value.name = "material";
    value.value.assign(material.begin(), material.end());
    input.values.push_back(std::move(value));
    source_attribute_value_input metadata;
    metadata.entity = {operand, attribute_source_entity_kind::facet, facet, 0};
    metadata.channel = attribute_channel_kind::face_metadata;
    metadata.name = "source-label";
    metadata.value.assign(material.begin(), material.end());
    input.values.push_back(std::move(metadata));
  }
  return input;
}

source_attribute_input vertex_seams(operand_id operand,
                                    std::size_t vertex_count,
                                    std::uint8_t value_byte) {
  source_attribute_input input;
  input.operand = operand;
  input.body_id = operand == operand_a() ? "body-A" : "body-B";
  for (std::size_t vertex = 0; vertex < vertex_count; ++vertex) {
    source_attribute_value_input value;
    value.entity = {operand, attribute_source_entity_kind::vertex, vertex, 0};
    value.channel = attribute_channel_kind::texture_seam;
    value.name = "uv0";
    value.value = {value_byte};
    input.values.push_back(std::move(value));
  }
  return input;
}

template <class T, class I>
auto make_context(fv_surface_mesh<T, I> &a, fv_surface_mesh<T, I> &b,
                  operation op) {
  return classification_test::context(a, b, output_test::registry(), op,
                                      boolean_options{});
}

void source_input_round_trip() {
  source_attribute_input input;
  input.operand = operand_a();
  input.body_id = "body-A";
  input.values.push_back({{operand_a(), attribute_source_entity_kind::edge, 0, 1},
                          attribute_channel_kind::sharp_edge, "crease", {1}});
  auto bytes = encode_source_attribute_input(input);
  require(bytes.has_value(), "source attributes encode");
  auto decoded = decode_source_attribute_input(bytes.value());
  require(decoded.has_value() && decoded.value().values.size() == 1 &&
              decoded.value().values.front().value ==
                  std::vector<std::uint8_t>{1},
          "source attributes canonical round trip");
  auto corrupt = bytes.value();
  corrupt.front() ^= 1U;
  require(!decode_source_attribute_input(corrupt).has_value(),
          "source attribute corruption rejected");
}

void output_mapping_and_replay() {
  using T = double;
  using I = std::uint32_t;
  auto a = input_test::box<T, I>(T(0), T(1));
  auto b = input_test::box<T, I>(T(2), T(3));
  populate_vertex_attributes(a, 100);
  populate_vertex_attributes(b, 200);
  auto context = make_context(a, b, operation::regularized_union);
  product_realization_policy realization;
  realization.semantics = product_realization_semantics::exact_in_T;
  realization.search.strategy = realization_search_strategy::nearest_only;
  auto result = evaluate_boolean_product_result(
      *context, backend_binding(), preparation_binding(),
      result_representation::exact_in_T_mesh, realization, preserve_policy());
  require(result.has_value() && result.value()->mesh,
          "attribute-aware exact mesh publication");
  const auto &published = *result.value();
  require(published.attributes.output_mappings.size() ==
              published.mesh->success->mesh.vertices.size() +
                  published.mesh->success->mesh.faces.size() &&
              published.attributes.exact_result_digest ==
                  published.exact_result->canonical_digest &&
              published.attributes.policy.mode ==
                  attribute_transfer_mode::preserve_supported_with_report &&
              verify_attribute_transfer_report(
                  published.attributes, published.exact_result,
                  &published.mesh->attribute_binding)
                  .has_value(),
          "output vertices and faces bind to exact entities");
  auto has_channel = [&](attribute_channel_kind channel) {
    return std::any_of(published.attributes.transfers.begin(),
                       published.attributes.transfers.end(),
                       [&](const auto &transfer) {
                         return transfer.channel == channel;
                       });
  };
  require(has_channel(attribute_channel_kind::source_body_id) &&
              has_channel(attribute_channel_kind::source_shell_id) &&
              has_channel(attribute_channel_kind::source_facet_id) &&
              has_channel(attribute_channel_kind::vertex_normal) &&
              has_channel(attribute_channel_kind::vertex_colour) &&
              has_channel(attribute_channel_kind::opaque) &&
              has_channel(attribute_channel_kind::construction_provenance),
          "identifier, vertex, opaque, and construction channels transfer");

  auto bytes = encode_attribute_transfer_report(published.attributes);
  require(bytes.has_value(), "attribute report encodes");
  auto decoded = decode_attribute_transfer_report(bytes.value());
  require(decoded.has_value() &&
              verify_serialized_attribute_transfer_report(
                  bytes.value(), published.exact_result,
                  &published.mesh->attribute_binding)
                  .has_value(),
          "attribute report canonical replay");
  auto corrupt = bytes.value();
  corrupt[corrupt.size() / 2] ^= 1U;
  require(!verify_serialized_attribute_transfer_report(
               corrupt, published.exact_result,
               &published.mesh->attribute_binding)
               .has_value(),
          "corrupted attribute report rejected");
  attribute_decode_limits limits;
  limits.max_record_bytes = bytes.value().size() - 1;
  require(!decode_attribute_transfer_report(bytes.value(), limits).has_value(),
          "attribute report resource limit enforced");
  auto mutated = published.attributes;
  require(!mutated.transfers.empty(), "attribute mutation fixture");
  mutated.transfers.front().value.push_back(0xff);
  require(!verify_attribute_transfer_report(
               mutated, published.exact_result,
               &published.mesh->attribute_binding)
               .has_value(),
          "attribute value mutation rejected independently");
}

void coincident_conflicts_and_value_invariance() {
  using T = double;
  using I = std::uint32_t;
  auto a = input_test::cube<T, I>();
  auto b = input_test::cube<T, I>();
  auto context = make_context(a, b, operation::regularized_intersection);
  auto exact = evaluate_boolean_product_result(
      *context, backend_binding(), preparation_binding(),
      result_representation::exact_stratified);
  require(exact.has_value(), "coincident exact result");
  std::array<source_attribute_input, 2> red_blue{
      facet_materials(operand_a(), a.faces.size(), "red"),
      facet_materials(operand_b(), b.faces.size(), "blue")};
  auto catalogs = make_attribute_source_catalogs(*context, &red_blue);
  require(catalogs.has_value(), "explicit material catalogs");
  auto policy = preserve_policy();
  auto report = make_attribute_transfer_report(
      exact.value()->exact_result, policy, catalogs.value());
  require(report.has_value() && report.value().conflicts != 0 &&
              std::any_of(report.value().issues.begin(),
                          report.value().issues.end(), [](const auto &issue) {
                            return issue.kind == attribute_issue_kind::conflict &&
                                   issue.channel ==
                                       attribute_channel_kind::face_metadata;
                          }),
          "coincident unequal material and face metadata are explicit conflicts");
  require(std::any_of(report.value().issues.begin(), report.value().issues.end(),
                      [](const auto &issue) {
                        return issue.kind == attribute_issue_kind::conflict &&
                               issue.channel == attribute_channel_kind::material;
                      }),
          "material conflict is typed");
  auto reject = policy;
  reject.conflicts = attribute_conflict_policy::reject;
  auto rejected = make_attribute_transfer_report(
      exact.value()->exact_result, reject, catalogs.value());
  require(!rejected.has_value() &&
              rejected.error().code ==
                  product_error_code::attribute_transfer_conflict,
          "reject policy fails closed on material conflict");
  auto changed_inputs = red_blue;
  for (auto &value : changed_inputs[1].values)
    value.value.assign({'g', 'r', 'e', 'e', 'n'});
  auto changed_catalogs = make_attribute_source_catalogs(*context, &changed_inputs);
  auto changed_report = make_attribute_transfer_report(
      exact.value()->exact_result, policy, changed_catalogs.value());
  require(changed_report.has_value() &&
              changed_report.value().report_digest !=
                  report.value().report_digest &&
              changed_report.value().exact_result_digest ==
                  report.value().exact_result_digest,
          "attribute values cannot change exact Boolean topology");
}

void seams_sharp_edges_and_removed_faces() {
  using T = double;
  using I = std::uint32_t;
  auto equal_a = input_test::cube<T, I>();
  auto equal_b = input_test::cube<T, I>();
  auto equal_context =
      make_context(equal_a, equal_b, operation::regularized_union);
  auto equal_exact = evaluate_boolean_product_result(
      *equal_context, backend_binding(), preparation_binding(),
      result_representation::exact_stratified);
  require(equal_exact.has_value(), "equal exact result for seams");
  std::array<source_attribute_input, 2> seams{
      vertex_seams(operand_a(), equal_a.vertices.size(), 1),
      vertex_seams(operand_b(), equal_b.vertices.size(), 2)};
  auto seam_catalogs = make_attribute_source_catalogs(*equal_context, &seams);
  auto seam_policy = preserve_policy();
  seam_policy.texture_seams = attribute_texture_seam_policy::require_equal;
  auto seam_report = make_attribute_transfer_report(
      equal_exact.value()->exact_result, seam_policy, seam_catalogs.value());
  require(seam_report.has_value() &&
              std::any_of(seam_report.value().issues.begin(),
                          seam_report.value().issues.end(), [](const auto &issue) {
                            return issue.reason ==
                                   attribute_issue_reason::texture_seam_mismatch;
                          }),
          "texture seam conflicts are retained");

  source_attribute_input sharp_a, sharp_b;
  sharp_a.operand = operand_a(); sharp_a.body_id = "body-A";
  sharp_b.operand = operand_b(); sharp_b.body_id = "body-B";
  sharp_a.values.push_back({{operand_a(), attribute_source_entity_kind::edge, 0, 1},
                            attribute_channel_kind::sharp_edge, "crease", {1}});
  sharp_b.values.push_back({{operand_b(), attribute_source_entity_kind::edge, 0, 1},
                            attribute_channel_kind::sharp_edge, "crease", {0}});
  std::array<source_attribute_input,2> sharp_inputs{sharp_a,sharp_b};
  auto sharp_catalogs = make_attribute_source_catalogs(*equal_context, &sharp_inputs);
  require(sharp_catalogs.has_value(), "sharp edge source catalog");
  auto sharp_report = make_attribute_transfer_report(
      equal_exact.value()->exact_result, preserve_policy(), sharp_catalogs.value());
  require(sharp_report.has_value() &&
              std::any_of(sharp_report.value().transfers.begin(),
                          sharp_report.value().transfers.end(), [](const auto &x) {
                            return x.channel == attribute_channel_kind::sharp_edge &&
                                   x.resolution == attribute_resolution_kind::any_source;
                          }),
          "sharp edges follow deterministic any-source policy");

  auto overlap_a = input_test::cube<T, I>();
  auto overlap_b = input_test::cube<T, I>();
  symbolic_test::translate(overlap_b, T(1), T(0), T(0));
  auto overlap_context =
      make_context(overlap_a, overlap_b, operation::regularized_union);
  auto overlap_exact = evaluate_boolean_product_result(
      *overlap_context, backend_binding(), preparation_binding(),
      result_representation::exact_stratified);
  require(overlap_exact.has_value(), "overlap exact result");
  std::array<source_attribute_input,2> materials{
      facet_materials(operand_a(), overlap_a.faces.size(), "a"),
      facet_materials(operand_b(), overlap_b.faces.size(), "b")};
  auto material_catalogs =
      make_attribute_source_catalogs(*overlap_context, &materials);
  auto material_report = make_attribute_transfer_report(
      overlap_exact.value()->exact_result, preserve_policy(),
      material_catalogs.value());
  require(material_report.has_value() &&
              std::any_of(material_report.value().issues.begin(),
                          material_report.value().issues.end(), [](const auto &x) {
                            return x.reason ==
                                   attribute_issue_reason::removed_internal_entity;
                          }),
          "removed internal faces are reported, not silently dropped");
}

void normalized_sources_compose_to_originals() {
  using T = double;
  using I = std::uint32_t;
  auto source_a = input_test::tetra<T, I>();
  source_a.vertices.push_back(source_a.vertices[0]);
  for (auto &face : source_a.faces)
    for (auto &index : face)
      if (index == I(0))
        index = I(4);
  source_a.vertex_normals.resize(source_a.vertices.size(),
                                 vec3<T>(T(1), T(0), T(0)));
  auto source_b = input_test::tetra<T, I>();
  for (auto &v : source_b.vertices)
    v.x += T(3);
  normalization_policy policy;
  policy.mode = normalization_mode::structural_only;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::exact_duplicate_consolidation);
  normalization_report report_a, report_b;
  auto prepared_a = normalize_operand(source_a, policy, report_a);
  auto prepared_b = normalize_operand(source_b, policy, report_b);
  require(prepared_a.has_value() && prepared_b.has_value(),
          "normalized prepared operands");
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifiers = output_test::registry();
  auto context = make_boolean_context(prepared_a.value(), prepared_b.value(),
                                      operation::regularized_union,
                                      boolean_options{}, kernel, verifiers);
  require(context.has_value(), "prepared attribute context");
  auto catalogs = make_attribute_source_catalogs(*context.value());
  require(catalogs.has_value(), "original source catalogs after normalization");
  std::vector<std::uint64_t> prepared_ids;
  for (const auto &entity : catalogs.value()[0].entities)
    if (entity.source.kind == attribute_source_entity_kind::vertex &&
        (entity.source.primary == 0 || entity.source.primary == 4))
      prepared_ids.push_back(entity.prepared_id);
  require(prepared_ids.size() == 2 && prepared_ids[0] == prepared_ids[1] &&
              prepared_ids[0] != attribute_unmapped_id,
          "normalization many-to-one mapping preserves both original contributors");
}

} // namespace

int main() {
  try {
    source_input_round_trip();
    output_mapping_and_replay();
    coincident_conflicts_and_value_invariance();
    seams_sharp_edges_and_removed_faces();
    normalized_sources_compose_to_originals();
    std::cout << "ok\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
