#include "MeshBooleanInputTopologyFixtures.h"

#include <YgorMeshesBooleanNormalization.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <tuple>

using namespace input_test;

template <class T, class I> void valid_identity() {
  const auto source = tetra<T, I>();
  normalization_policy policy;
  policy.unit = model_unit::millimetre;
  policy.model_tolerance = 0.01;
  normalization_report report;
  auto prepared = normalize_operand(source, policy, report);
  require(prepared.has_value(), "diagnosis-only normalization succeeds");
  require(prepared.value().normalization() != nullptr &&
              prepared.value().normalization()->report_digest ==
                  report.report_digest,
          "prepared operand owns canonical normalization report");
  require(report.prepared_operand_available && report.strict_certificate &&
              report.source_digest == report.output_digest &&
              report.edits.empty() && report.displacements.empty() &&
              report.topology_changes.empty() &&
              report.displacement ==
                  normalization_displacement_claim::exact_zero &&
              report.reversibility == normalization_reversibility::identity,
          "successful diagnosis explicitly claims identity only");
  require(report.vertices.source_to_prepared.size() == source.vertices.size() &&
              report.facets.source_to_prepared.size() == source.faces.size() &&
              report.edges.source_to_prepared.size() == 6 &&
              report.source_edges.front() ==
                  std::array<std::uint64_t, 2>{{0, 1}} &&
              report.source_edges.back() ==
                  std::array<std::uint64_t, 2>{{2, 3}} &&
              report.shells.status == normalization_map_status::total &&
              report.shells.source_to_prepared.size() == 1 &&
              report.attributes.vertex_normals.status ==
                  normalization_map_status::absent &&
              report.attributes.vertex_colours.status ==
                  normalization_map_status::absent &&
              report.attributes.involved_faces.status ==
                  normalization_map_status::absent &&
              report.attributes.metadata.status ==
                  normalization_map_status::absent,
          "identity maps and unavailable/absent maps are explicit");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value(), "normalization report encodes");
  auto decoded = decode_normalization_report(bytes.value());
  require(decoded.has_value(), "normalization report decodes");
  auto reencoded = encode_normalization_report(decoded.value());
  require(reencoded.has_value() && reencoded.value() == bytes.value(),
          "normalization report encoding is canonical");
  require(verify_normalization_report(bytes.value(), source, &source).has_value(),
          "independent report verifier reruns strict validation");

  auto prepared_bytes = encode_prepared_operand(prepared.value());
  require(prepared_bytes.has_value(), "normalized preparation encodes");
  auto prepared_roundtrip =
      decode_prepared_operand<T, I>(prepared_bytes.value());
  require(prepared_roundtrip.has_value() &&
              prepared_roundtrip.value().normalization() != nullptr &&
              prepared_roundtrip.value().normalization()->report_digest ==
                  report.report_digest,
          "normalized report lifetime survives prepared round trip");
}

template <class T, class I> fv_surface_mesh<T, I> attributed_tetra() {
  auto mesh = tetra<T, I>();
  mesh.vertex_normals = {{T(0), T(0), T(-1)}, {T(0), T(-1), T(0)},
                         {T(1), T(1), T(1)}, {T(-1), T(0), T(0)}};
  mesh.vertex_colours = {0x01020304U, 0x11223344U, 0x55667788U,
                         0xAABBCCDDU};
  mesh.recreate_involved_face_index();
  mesh.metadata["alpha"] = "one";
  mesh.metadata["zeta"] = "two";
  return mesh;
}

void attribute_preservation_and_binding() {
  using T = double;
  using I = std::uint32_t;
  const auto source = attributed_tetra<T, I>();
  normalization_report report;
  auto prepared = normalize_operand(source, normalization_policy{}, report);
  require(prepared.has_value(), "attribute-rich source normalizes");
  require(report.attributes.vertex_normals.status ==
                  normalization_map_status::total &&
              report.attributes.vertex_normals.source_to_prepared.size() == 4 &&
              report.attributes.vertex_colours.status ==
                  normalization_map_status::total &&
              report.attributes.vertex_colours.source_to_prepared.size() == 4 &&
              report.attributes.involved_faces.status ==
                  normalization_map_status::total &&
              report.attributes.involved_faces.source_to_prepared.size() == 4 &&
              report.attributes.metadata.status ==
                  normalization_map_status::total &&
              report.attributes.metadata.source_to_prepared.size() == 2,
          "all present attribute channels have explicit identity maps");
  auto prepared_bytes = encode_prepared_operand(prepared.value());
  require(prepared_bytes.has_value(), "attribute-rich preparation encodes");
  auto decoded = decode_prepared_operand<T, I>(prepared_bytes.value());
  require(decoded.has_value() && decoded.value().mesh() == source &&
              decoded.value().mesh().involved_faces == source.involved_faces,
          "prepared v2 roundtrip preserves every mesh field");
  prepared_operand_decode_limits prepared_limits;
  prepared_limits.max_vertex_normals = 1;
  require(!decode_prepared_operand<T, I>(prepared_bytes.value(), prepared_limits)
               .has_value(),
          "prepared decoder limits normal attributes");
  prepared_limits = {};
  prepared_limits.max_vertex_colours = 1;
  require(!decode_prepared_operand<T, I>(prepared_bytes.value(), prepared_limits)
               .has_value(),
          "prepared decoder limits colour attributes");
  prepared_limits = {};
  prepared_limits.max_involved_face_indices = 1;
  require(!decode_prepared_operand<T, I>(prepared_bytes.value(), prepared_limits)
               .has_value(),
          "prepared decoder limits nested involved-face indices");
  prepared_limits = {};
  prepared_limits.max_metadata_entries = 1;
  require(!decode_prepared_operand<T, I>(prepared_bytes.value(), prepared_limits)
               .has_value(),
          "prepared decoder limits metadata entries");
  prepared_limits = {};
  prepared_limits.max_metadata_string_bytes = 2;
  require(!decode_prepared_operand<T, I>(prepared_bytes.value(), prepared_limits)
               .has_value(),
          "prepared decoder limits aggregate metadata bytes");
  auto report_bytes = encode_normalization_report(report);
  require(report_bytes.has_value(), "attribute-rich report encodes");
  const auto rejected = [&](fv_surface_mesh<T, I> changed) {
    return !verify_normalization_report(report_bytes.value(), changed, &source)
                .has_value();
  };
  auto normals = source;
  normals.vertex_normals[0].x = T(0.25);
  require(rejected(normals), "normal mutation invalidates report binding");
  auto colours = source;
  colours.vertex_colours[0] ^= 1U;
  require(rejected(colours), "colour mutation invalidates report binding");
  auto involved = source;
  involved.involved_faces[0].push_back(I(1));
  require(rejected(involved), "involved-face mutation invalidates binding");
  auto metadata = source;
  metadata.metadata["alpha"] = "changed";
  require(rejected(metadata), "metadata mutation invalidates report binding");

  auto malformed = source;
  malformed.vertex_normals.pop_back();
  normalization_report malformed_report;
  auto malformed_result =
      normalize_operand(malformed, normalization_policy{}, malformed_report);
  require(!malformed_result.has_value() &&
              !malformed_report.prepared_operand_available,
          "malformed attribute cardinality yields no prepared operand");
}

void invalid_diagnosis() {
  using T = double;
  using I = std::uint32_t;
  auto source = tetra<T, I>();
  source.vertices.push_back(source.vertices[0]);
  source.vertices.push_back(
      {std::numeric_limits<T>::infinity(), T(0), T(0)});
  source.faces.push_back(source.faces[0]);
  source.faces.push_back({I(0), I(0)});
  source.faces.push_back({I(0), I(99), I(1)});

  normalization_report report;
  auto rejected = normalize_operand(source, normalization_policy{}, report);
  require(!rejected.has_value() &&
              rejected.error().code == boolean_error_code::input_contract_error &&
              !report.prepared_operand_available && !report.strict_certificate,
          "invalid input publishes diagnosis but no prepared operand");
  const auto has = [&](normalization_defect_code code) {
    return std::any_of(report.unresolved_defects.begin(),
                       report.unresolved_defects.end(),
                       [=](const auto &d) { return d.code == code; });
  };
  require(has(normalization_defect_code::nonfinite_coordinate) &&
              has(normalization_defect_code::short_face) &&
              has(normalization_defect_code::index_out_of_range) &&
              has(normalization_defect_code::consecutive_duplicate_index) &&
              has(normalization_defect_code::exact_duplicate_vertex) &&
              has(normalization_defect_code::exact_duplicate_facet) &&
              has(normalization_defect_code::unused_vertex) &&
              has(normalization_defect_code::component2_rejection),
          "raw scan and Component 2 rejection are all represented");
  require(std::is_sorted(
              report.unresolved_defects.begin(), report.unresolved_defects.end(),
              [](const auto &a, const auto &b) {
                return std::tie(a.code, a.primary_ordinal, a.secondary_ordinal,
                                a.detail) <
                       std::tie(b.code, b.primary_ordinal, b.secondary_ordinal,
                                b.detail);
              }),
          "defect records have deterministic canonical order");
  normalization_report repeat;
  require(!normalize_operand(source, normalization_policy{}, repeat).has_value() &&
              repeat.unresolved_defects == report.unresolved_defects &&
              repeat.report_digest == report.report_digest,
          "invalid diagnosis is deterministic");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(
                  bytes.value(), source,
                  static_cast<const fv_surface_mesh<T, I> *>(nullptr))
                  .has_value(),
          "invalid diagnosis report independently verifies");
}

void advisory_findings() {
  auto source = tetra<double, std::uint32_t>();
  source.vertices.push_back(source.vertices[0]);
  normalization_report report;
  auto prepared = normalize_operand(source, normalization_policy{}, report);
  require(prepared.has_value() && report.prepared_operand_available &&
              std::any_of(report.unresolved_defects.begin(),
                          report.unresolved_defects.end(), [](const auto &d) {
                            return d.code ==
                                       normalization_defect_code::
                                           exact_duplicate_vertex ||
                                   d.code ==
                                       normalization_defect_code::unused_vertex;
                          }),
          "strict-valid advisory defects retain unchanged success");
}

void structural_irrelevant_storage_removal() {
  using T = double;
  using I = std::uint32_t;
  auto source = attributed_tetra<T, I>();
  source.vertices.insert(source.vertices.begin() + 1, {T(9), T(8), T(7)});
  source.vertex_normals.insert(source.vertex_normals.begin() + 1,
                               {T(0), T(0), T(1)});
  source.vertex_colours.insert(source.vertex_colours.begin() + 1, 0xDEADBEEFU);
  for (auto &face : source.faces)
    for (auto &index : face)
      if (index >= I(1)) ++index;
  source.recreate_involved_face_index();

  normalization_policy policy;
  policy.mode = normalization_mode::structural_only;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::irrelevant_storage_removal);
  normalization_report report;
  auto prepared = normalize_operand(source, policy, report);
  require(prepared.has_value(), "structural storage removal succeeds");
  const auto expected = attributed_tetra<T, I>();
  require(prepared.value().mesh() == expected &&
              report.source_digest != report.output_digest &&
              report.output_digest == prepared.value().certificate().input_digest &&
              report.displacement ==
                  normalization_displacement_claim::exact_zero &&
              report.displacements.empty() && report.topology_changes.empty() &&
              report.unresolved_defects.empty(),
          "structural removal preserves geometry and strictly validates output");
  const std::vector<std::uint64_t> expected_vertices{
      0, normalization_removed_ordinal, 1, 2, 3};
  require(report.vertices.source_to_prepared == expected_vertices &&
              report.attributes.vertex_normals.source_to_prepared ==
                  expected_vertices &&
              report.attributes.vertex_colours.source_to_prepared ==
                  expected_vertices &&
              report.attributes.involved_faces.source_to_prepared ==
                  expected_vertices &&
              report.facets.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3}),
          "source mappings retain every removal and compaction ordinal");
  require(report.edits.size() == 1 &&
              report.edits[0].operation ==
                  normalization_operation::irrelevant_storage_removal &&
              report.edits[0].entity == normalization_entity_kind::vertex &&
              report.edits[0].source_ordinal == 1 &&
              report.edits[0].prepared_ordinal ==
                  normalization_removed_ordinal &&
              report.edits[0].reversibility ==
                  normalization_reversibility::irreversible &&
              report.reversibility ==
                  normalization_reversibility::irreversible,
          "removed storage has canonical auditable evidence");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(bytes.value(), source, &expected)
                  .has_value(),
          "independent verifier replays structural removal");
  auto prepared_bytes = encode_prepared_operand(prepared.value());
  require(prepared_bytes.has_value() &&
              decode_prepared_operand<T, I>(prepared_bytes.value()).has_value(),
          "structurally normalized preparation round trips");

  auto forged = report;
  forged.vertices.source_to_prepared[1] = 0;
  forged.report_digest = normalization_report_digest(forged).value();
  auto forged_bytes = encode_normalization_report(forged);
  require(forged_bytes.has_value() &&
              !verify_normalization_report(forged_bytes.value(), source,
                                           &expected)
                   .has_value(),
          "verifier rejects forged removal mapping");

  normalization_report repeat;
  auto repeated = normalize_operand(source, policy, repeat);
  require(repeated.has_value() && repeated.value().mesh() == expected &&
              repeat.report_digest == report.report_digest,
          "structural removal is deterministic");

  normalization_report no_op_report;
  auto no_op = normalize_operand(expected, policy, no_op_report);
  require(no_op.has_value() && no_op.value().mesh() == expected &&
              no_op_report.source_digest == no_op_report.output_digest &&
              no_op_report.edits.empty() &&
              no_op_report.reversibility ==
                  normalization_reversibility::identity,
          "structural removal is an explicit identity when storage is packed");

  auto invalid = source;
  invalid.faces.push_back(invalid.faces.front());
  normalization_report invalid_report;
  auto rejected = normalize_operand(invalid, policy, invalid_report);
  require(!rejected.has_value() &&
              rejected.error().code == boolean_error_code::input_contract_error &&
              !invalid_report.prepared_operand_available &&
              invalid_report.edits.empty(),
          "structural removal does not repair a strict-invalid operand");
  auto invalid_forgery = invalid_report;
  std::swap(invalid_forgery.vertices.source_to_prepared[0],
            invalid_forgery.vertices.source_to_prepared[1]);
  invalid_forgery.report_digest =
      normalization_report_digest(invalid_forgery).value();
  auto invalid_forgery_bytes =
      encode_normalization_report(invalid_forgery).value();
  require(!verify_normalization_report(
               invalid_forgery_bytes, invalid,
               static_cast<const fv_surface_mesh<T, I> *>(nullptr))
               .has_value(),
          "structural failure reports cannot forge source mappings");

  normalization_policy limited = policy;
  limited.resources.max_defect_records = 1;
  normalization_report unchanged;
  unchanged.schema = 77;
  auto exhausted = normalize_operand(source, limited, unchanged);
  require(!exhausted.has_value() &&
              exhausted.error().code == boolean_error_code::resource_limit &&
              unchanged.schema == 77,
          "structural edit accounting is bounded and transactional");
}

template <class T, class I> void exact_duplicate_repair_basic() {
  auto source = tetra<T, I>();
  source.vertices.push_back(source.vertices[0]);
  for (auto &face : source.faces)
    for (auto &index : face)
      if (index == I(0)) index = I(4);
  auto duplicate_face = source.faces[0];
  std::reverse(duplicate_face.begin(), duplicate_face.end());
  std::rotate(duplicate_face.begin(), duplicate_face.begin() + 1,
              duplicate_face.end());
  source.faces.push_back(std::move(duplicate_face));

  normalization_policy policy;
  policy.mode = normalization_mode::structural_only;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::exact_duplicate_consolidation);
  normalization_report report;
  auto prepared = normalize_operand(source, policy, report);
  const auto expected = tetra<T, I>();
  require(prepared.has_value() && prepared.value().mesh() == expected,
          "exact duplicate vertices and facets are repaired");
  require(report.vertices.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3, 0}) &&
              report.facets.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3, 0}) &&
              report.edits.size() == 2 &&
              report.topology_changes.size() == 2 &&
              report.edits[0].entity == normalization_entity_kind::vertex &&
              report.edits[0].source_ordinal == 4 &&
              report.edits[0].prepared_ordinal == 0 &&
              report.edits[1].entity == normalization_entity_kind::facet &&
              report.edits[1].source_ordinal == 4 &&
              report.edits[1].prepared_ordinal == 0 &&
              report.displacement ==
                  normalization_displacement_claim::exact_zero &&
              report.displacements.empty() &&
              report.reversibility ==
                  normalization_reversibility::irreversible &&
              report.shells.status == normalization_map_status::unavailable,
          "duplicate repair publishes many-to-one maps and exact evidence");
  require(std::all_of(report.topology_changes.begin(),
                      report.topology_changes.end(), [](const auto &change) {
                        return change.operation ==
                                   normalization_operation::
                                       exact_duplicate_consolidation &&
                               change.justification ==
                                   normalization_topology_justification::
                                       caller_authorized_repair &&
                               change.reversibility ==
                                   normalization_reversibility::irreversible;
                      }),
          "duplicate repair records authorized topology changes");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(bytes.value(), source, &expected)
                  .has_value(),
          "independent verifier replays exact duplicate repair");
  auto prepared_bytes = encode_prepared_operand(prepared.value());
  require(prepared_bytes.has_value(),
          "duplicate-normalized preparation encodes");
  auto prepared_roundtrip =
      decode_prepared_operand<T, I>(prepared_bytes.value());
  require(prepared_roundtrip.has_value() &&
              prepared_roundtrip.value().mesh() == expected &&
              prepared_roundtrip.value().normalization_source() &&
              *prepared_roundtrip.value().normalization_source() == source,
          "duplicate-normalized preparation round trips with source evidence");

  normalization_report diagnosis_report;
  require(!normalize_operand(source, normalization_policy{}, diagnosis_report)
               .has_value(),
          "repairable duplicate source has a failure diagnosis fixture");
  auto false_failure = diagnosis_report;
  false_failure.policy = policy;
  false_failure.policy_digest = normalization_policy_digest(policy).value();
  false_failure.report_digest =
      normalization_report_digest(false_failure).value();
  auto false_failure_bytes = encode_normalization_report(false_failure);
  require(false_failure_bytes.has_value() &&
              !verify_normalization_report(
                   false_failure_bytes.value(), source,
                   static_cast<const fv_surface_mesh<T, I> *>(nullptr))
                   .has_value(),
          "verifier rejects a forged failure for a repairable source");

  normalization_policy record_limited = policy;
  record_limited.resources.max_defect_records = 4;
  normalization_report unpublished;
  unpublished.schema = 77;
  auto exhausted = normalize_operand(source, record_limited, unpublished);
  require(!exhausted.has_value() &&
              exhausted.error().code == boolean_error_code::resource_limit &&
              unpublished.schema == 77,
          "duplicate repair resource failure publishes no partial report");

  normalization_report repeat;
  auto repeated = normalize_operand(source, policy, repeat);
  require(repeated.has_value() && repeat.report_digest == report.report_digest,
          "exact duplicate repair is deterministic");
  normalization_report identity_report;
  auto identity_result = normalize_operand(expected, policy, identity_report);
  auto identity_bytes = encode_normalization_report(identity_report);
  require(identity_result.has_value() && identity_report.edits.empty() &&
              identity_report.topology_changes.empty() &&
              identity_report.source_digest == identity_report.output_digest &&
              identity_bytes.has_value() &&
              verify_normalization_report(identity_bytes.value(), expected,
                                           &expected)
                  .has_value(),
          "exact duplicate repair is idempotent");
}

void exact_duplicate_attributes_and_rejection() {
  using T = double;
  using I = std::uint32_t;
  auto source = attributed_tetra<T, I>();
  source.vertices.push_back(source.vertices[0]);
  source.vertex_normals.push_back(source.vertex_normals[0]);
  source.vertex_colours.push_back(source.vertex_colours[0]);
  for (auto &face : source.faces)
    for (auto &index : face)
      if (index == I(0)) index = I(4);
  source.faces.push_back(source.faces[0]);
  source.recreate_involved_face_index();

  normalization_policy policy;
  policy.mode = normalization_mode::structural_only;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::exact_duplicate_consolidation);
  normalization_report report;
  auto prepared = normalize_operand(source, policy, report);
  const auto expected = attributed_tetra<T, I>();
  require(prepared.has_value() && prepared.value().mesh() == expected &&
              prepared.value().mesh().involved_faces == expected.involved_faces &&
              report.attributes.vertex_normals.source_to_prepared ==
                  report.vertices.source_to_prepared &&
              report.attributes.vertex_colours.source_to_prepared ==
                  report.vertices.source_to_prepared &&
              report.attributes.involved_faces.source_to_prepared ==
                  report.vertices.source_to_prepared,
          "compatible attributes survive and derived incidence is rebuilt");

  auto forged = report;
  forged.topology_changes[0].justification_subcode = 99;
  forged.topology_changes[0].evidence_digest =
      forged.topology_changes[1].evidence_digest;
  forged.report_digest = normalization_report_digest(forged).value();
  auto forged_bytes = encode_normalization_report(forged);
  require(forged_bytes.has_value() &&
              !verify_normalization_report(forged_bytes.value(), source,
                                           &expected)
                   .has_value(),
          "verifier rejects forged duplicate topology evidence");

  auto conflict = source;
  conflict.vertex_normals[4].x = T(0.5);
  normalization_report conflict_report;
  auto rejected = normalize_operand(conflict, policy, conflict_report);
  require(!rejected.has_value() &&
              rejected.error().code == boolean_error_code::input_contract_error &&
              !conflict_report.prepared_operand_available &&
              conflict_report.edits.empty() &&
              conflict_report.topology_changes.empty(),
          "attribute conflicts fail closed without partial repair");
  auto conflict_bytes = encode_normalization_report(conflict_report);
  require(conflict_bytes.has_value() &&
              verify_normalization_report(
                  conflict_bytes.value(), conflict,
                  static_cast<const fv_surface_mesh<T, I> *>(nullptr))
                  .has_value(),
          "failed duplicate repair report independently verifies");

  normalization_report diagnosis;
  auto unchanged = normalize_operand(source, normalization_policy{}, diagnosis);
  require(!unchanged.has_value() && diagnosis.edits.empty() &&
              diagnosis.topology_changes.empty(),
          "duplicate repair is never enabled implicitly");
}

normalization_policy seam_consolidation_policy() {
  normalization_policy policy;
  policy.mode = normalization_mode::geometry_changing;
  policy.unit = model_unit::millimetre;
  policy.model_tolerance = 0.01;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::seam_aware_vertex_consolidation);
  return policy;
}

template <class T, class I> void seam_aware_vertex_consolidation_basic() {
  auto source = attributed_tetra<T, I>();
  source.vertices.push_back(source.vertices[0]);
  source.vertices.back().x += T(0.005);
  source.vertex_normals.push_back(source.vertex_normals[0]);
  source.vertex_colours.push_back(source.vertex_colours[0]);
  for (auto &face : source.faces)
    for (auto &index : face)
      if (index == I(0)) index = I(4);
  source.recreate_involved_face_index();

  normalization_report report;
  auto prepared =
      normalize_operand(source, seam_consolidation_policy(), report);
  const auto expected = attributed_tetra<T, I>();
  require(prepared.has_value() && prepared.value().mesh() == expected &&
              prepared.value().mesh().involved_faces == expected.involved_faces,
          "near duplicate vertices consolidate to the lowest compatible source");
  require(report.vertices.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3, 0}) &&
              report.facets.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3}) &&
              report.edits.size() == 1 &&
              report.edits[0].operation ==
                  normalization_operation::seam_aware_vertex_consolidation &&
              report.edits[0].source_ordinal == 4 &&
              report.topology_changes.size() == 1 &&
              report.displacement ==
                  normalization_displacement_claim::records_present &&
              report.displacements.size() == 1 &&
              report.displacements[0].source_vertex == 4 &&
              report.displacements[0].prepared_vertex == 0 &&
              report.displacements[0].kind ==
                  normalization_displacement_kind::bounded &&
              report.displacements[0].unit == model_unit::millimetre &&
              report.reversibility ==
                  normalization_reversibility::irreversible &&
              report.attributes.vertex_normals.source_to_prepared ==
                  report.vertices.source_to_prepared &&
              report.attributes.vertex_colours.source_to_prepared ==
                  report.vertices.source_to_prepared,
          "seam consolidation records maps, topology, and bounded movement");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(bytes.value(), source, &expected)
                  .has_value(),
          "independent verifier replays seam-aware consolidation");

  auto forged = report;
  forged.displacements[0].squared_distance_bound = {1, 1};
  forged.report_digest = normalization_report_digest(forged).value();
  auto forged_bytes = encode_normalization_report(forged);
  require(forged_bytes.has_value() &&
              !verify_normalization_report(forged_bytes.value(), source,
                                           &expected)
                   .has_value(),
          "verifier rejects forged displacement evidence");

  normalization_report repeat;
  auto repeated = normalize_operand(source, seam_consolidation_policy(), repeat);
  require(repeated.has_value() && repeat.report_digest == report.report_digest,
          "seam-aware consolidation is deterministic");
}

void seam_preservation_and_tolerance_rejection() {
  using T = double;
  using I = std::uint32_t;
  auto source = attributed_tetra<T, I>();
  source.vertices.push_back(source.vertices[0]);
  source.vertices.back().x += 0.005;
  source.vertex_normals.push_back(source.vertex_normals[0]);
  source.vertex_normals.back().x = 0.25;
  source.vertex_colours.push_back(source.vertex_colours[0]);
  for (auto &face : source.faces)
    for (auto &index : face)
      if (index == I(0)) index = I(4);
  source.recreate_involved_face_index();
  normalization_report report;
  auto rejected = normalize_operand(source, seam_consolidation_policy(), report);
  require(rejected.has_value() && rejected.value().mesh() == source &&
              report.prepared_operand_available && report.edits.empty() &&
              report.displacements.empty(),
          "incompatible normal attributes preserve a seam without welding");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(bytes.value(), source, &source)
                  .has_value(),
          "preserved attribute seam independently verifies");

  source.vertex_normals.back() = source.vertex_normals[0];
  source.vertices.back().x = 0.02;
  normalization_report outside;
  auto outside_result =
      normalize_operand(source, seam_consolidation_policy(), outside);
  require(outside_result.has_value() &&
              outside_result.value().mesh() == source && outside.edits.empty(),
          "vertices outside the exact model tolerance are not consolidated");

  auto missing_units = seam_consolidation_policy();
  missing_units.unit = model_unit::unspecified;
  normalization_report unchanged;
  unchanged.schema = 77;
  require(!normalize_operand(source, missing_units, unchanged).has_value() &&
              unchanged.schema == 77,
          "geometry-changing consolidation requires explicit model units");
}

normalization_policy orientation_policy() {
  normalization_policy policy;
  policy.mode = normalization_mode::structural_only;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::orientation_repair);
  return policy;
}

template <class T, class I> void orientation_repair_basic() {
  const auto expected = tetra<T, I>();
  auto source = expected;
  std::reverse(source.faces[0].begin(), source.faces[0].end());

  normalization_report diagnosis;
  auto diagnosed = normalize_operand(source, normalization_policy{}, diagnosis);
  require(!diagnosed.has_value() && diagnosis.edits.empty() &&
              std::any_of(diagnosis.unresolved_defects.begin(),
                          diagnosis.unresolved_defects.end(), [](const auto &d) {
                            return d.code ==
                                       normalization_defect_code::
                                           component2_rejection &&
                                   d.primary_ordinal == static_cast<std::uint64_t>(
                                       input_validation_subcode::
                                           same_direction_uses);
                          }),
          "diagnosis identifies local orientation failure without repair");
  auto false_failure = diagnosis;
  false_failure.policy = orientation_policy();
  false_failure.policy_digest =
      normalization_policy_digest(false_failure.policy).value();
  false_failure.report_digest =
      normalization_report_digest(false_failure).value();
  auto false_failure_bytes = encode_normalization_report(false_failure);
  require(false_failure_bytes.has_value() &&
              !verify_normalization_report(
                   false_failure_bytes.value(), source,
                   static_cast<const fv_surface_mesh<T, I> *>(nullptr))
                   .has_value(),
          "independent bounded search rejects a false orientation failure");

  normalization_report report;
  auto prepared = normalize_operand(source, orientation_policy(), report);
  require(prepared.has_value() && prepared.value().mesh() == expected,
          "facet parity repair restores a locally inconsistent shell");
  require(report.source_digest != report.output_digest &&
              report.displacement ==
                  normalization_displacement_claim::exact_zero &&
              report.displacements.empty() && report.topology_changes.empty() &&
              report.reversibility ==
                  normalization_reversibility::fully_reversible &&
              report.vertices.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3}) &&
              report.facets.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1, 2, 3}) &&
              report.shells.source_to_prepared ==
                  std::vector<std::uint64_t>({0}),
          "orientation repair reports zero movement and identity topology maps");
  require(report.edits.size() == 1 &&
              report.edits[0].operation ==
                  normalization_operation::orientation_repair &&
              report.edits[0].entity == normalization_entity_kind::facet &&
              report.edits[0].source_ordinal == 0 &&
              report.edits[0].prepared_ordinal == 0 &&
              report.edits[0].reversibility ==
                  normalization_reversibility::fully_reversible,
          "reversed facet has canonical reversible evidence");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(bytes.value(), source, &expected)
                  .has_value(),
          "independent verifier replays orientation edits and strict validation");

  auto forged = report;
  forged.edits[0].source_ordinal = 1;
  forged.report_digest = normalization_report_digest(forged).value();
  auto forged_bytes = encode_normalization_report(forged);
  require(forged_bytes.has_value() &&
              !verify_normalization_report(forged_bytes.value(), source,
                                           &expected)
                   .has_value(),
          "independent verifier rejects forged orientation evidence");

  normalization_report identity_report;
  auto identity_result =
      normalize_operand(expected, orientation_policy(), identity_report);
  require(identity_result.has_value() &&
              identity_result.value().mesh() == expected &&
              identity_report.edits.empty() &&
              identity_report.source_digest == identity_report.output_digest &&
              identity_report.reversibility ==
                  normalization_reversibility::identity,
          "orientation repair is idempotent on a valid shell");
}

void orientation_nested_shells_and_fail_closed() {
  using T = double;
  using I = std::uint32_t;
  auto source = box<T, I>(T(0), T(10));
  append(source, box<T, I>(T(2), T(8)));
  auto expected = box<T, I>(T(0), T(10));
  append(expected, box<T, I>(T(2), T(8)), true);
  normalization_report report;
  auto prepared = normalize_operand(source, orientation_policy(), report);
  require(prepared.has_value() && prepared.value().mesh() == expected &&
              report.edits.size() == 6 &&
              report.shells.source_to_prepared ==
                  std::vector<std::uint64_t>({0, 1}),
          "nested cavity polarity is repaired without moving geometry");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value() &&
              verify_normalization_report(bytes.value(), source, &expected)
                  .has_value(),
          "nested polarity repair independently replays");

  auto three_level = box<T, I>(T(0), T(12));
  append(three_level, box<T, I>(T(2), T(10)));
  append(three_level, box<T, I>(T(4), T(8)), true);
  auto three_level_expected = box<T, I>(T(0), T(12));
  append(three_level_expected, box<T, I>(T(2), T(10)), true);
  append(three_level_expected, box<T, I>(T(4), T(8)));
  normalization_report three_level_report;
  auto three_level_result =
      normalize_operand(three_level, orientation_policy(), three_level_report);
  require(three_level_result.has_value() &&
              three_level_result.value().mesh() == three_level_expected &&
              three_level_report.shells.source_to_prepared.size() == 3,
          "nested shell polarity alternates through cavity and island depths");

  auto concave = prism<T, I>(true);
  auto concave_source = concave;
  std::reverse(concave_source.faces[1].begin(), concave_source.faces[1].end());
  std::reverse(concave_source.faces[4].begin(), concave_source.faces[4].end());
  normalization_report concave_report;
  auto concave_result =
      normalize_operand(concave_source, orientation_policy(), concave_report);
  require(concave_result.has_value() && concave_result.value().mesh() == concave,
          "concave polygon shell with collinear chains repairs without remeshing");

  auto reversed_root = tetra<T, I>();
  for (auto &face : reversed_root.faces)
    std::reverse(face.begin(), face.end());
  normalization_report root_report;
  auto root = normalize_operand(reversed_root, orientation_policy(), root_report);
  require(root.has_value() && root.value().mesh() == tetra<T, I>() &&
              root_report.edits.size() == reversed_root.faces.size(),
          "globally reversed material shell is repaired");

  auto open = tetra<T, I>();
  open.faces.pop_back();
  normalization_report failure;
  auto rejected = normalize_operand(open, orientation_policy(), failure);
  require(!rejected.has_value() && !failure.prepared_operand_available &&
              failure.edits.empty() && failure.topology_changes.empty(),
          "orientation repair does not hide an open shell");
  auto failure_bytes = encode_normalization_report(failure);
  require(failure_bytes.has_value() &&
              verify_normalization_report(
                  failure_bytes.value(), open,
                  static_cast<const fv_surface_mesh<T, I> *>(nullptr))
                  .has_value(),
          "non-repairable orientation failure report replays");

  auto limited_policy = orientation_policy();
  limited_policy.resources.max_work_units = 300;
  normalization_report unpublished;
  unpublished.schema = 77;
  auto exhausted =
      normalize_operand(source, limited_policy, unpublished);
  require(!exhausted.has_value() &&
              exhausted.error().code == boolean_error_code::resource_limit &&
              unpublished.schema == 77,
          "orientation planning work is bounded and transactional");
}

void codec_and_verifier_rejection() {
  using T = double;
  using I = std::uint32_t;
  normalization_policy policy;
  policy.unit = model_unit::inch;
  policy.model_tolerance = 0.125;
  auto policy_bytes = encode_normalization_policy(policy);
  require(policy_bytes.has_value() &&
              decode_normalization_policy(policy_bytes.value()).has_value(),
          "policy canonical round trip succeeds");
  auto unknown_policy = policy_bytes.value();
  unknown_policy[10] = 99;
  require(!decode_normalization_policy(unknown_policy).has_value(),
          "unknown policy enum is rejected");
  auto source = tetra<T, I>();
  normalization_report report;
  auto prepared = normalize_operand(source, policy, report);
  require(prepared.has_value(), "mutation fixture normalizes");
  auto bytes = encode_normalization_report(report);
  require(bytes.has_value(), "mutation fixture encodes");
  auto truncated = bytes.value();
  truncated.pop_back();
  require(!decode_normalization_report(truncated).has_value(),
          "truncated report is rejected");
  auto trailing = bytes.value();
  trailing.push_back(0);
  require(!decode_normalization_report(trailing).has_value(),
          "trailing report is rejected");
  auto unknown_boolean = bytes.value();
  unknown_boolean[131] = 2;
  require(!decode_normalization_report(unknown_boolean).has_value(),
          "noncanonical boolean is rejected");
  normalization_decode_limits limits;
  limits.max_mapping_entries = 1;
  auto limited = decode_normalization_report(bytes.value(), limits);
  require(!limited.has_value() &&
              limited.error().code == boolean_error_code::resource_limit,
          "report decoder enforces cardinality limits");
  auto stale_source = source;
  stale_source.vertices[0].x = T(7);
  require(!verify_normalization_report(bytes.value(), stale_source, &source)
               .has_value(),
          "verifier rejects stale source binding");
  auto stale_output = source;
  stale_output.vertices[0].x = T(7);
  require(!verify_normalization_report(bytes.value(), source, &stale_output)
               .has_value(),
          "verifier rejects changed output and zero-change claim");
  auto mutation = bytes.value();
  mutation.back() ^= 1U;
  require(!decode_normalization_report(mutation).has_value(),
          "stale report digest is rejected");
}

void policy_cancellation_and_limits() {
  using T = double;
  using I = std::uint32_t;
  auto source = tetra<T, I>();
  normalization_report unchanged;
  unchanged.schema = 77;

  normalization_policy unsupported_repair;
  unsupported_repair.mode = normalization_mode::structural_only;
  unsupported_repair.enabled_operations = normalization_operation_bit(
      normalization_operation::seam_aware_vertex_consolidation);
  require(!normalize_operand(source, unsupported_repair, unchanged).has_value() &&
              unchanged.schema == 77,
          "unimplemented structural operations fail closed");

  normalization_policy unspecified_tolerance;
  unspecified_tolerance.model_tolerance = 0.01;
  require(!normalize_operand(source, unspecified_tolerance, unchanged)
               .has_value() &&
              unchanged.schema == 77,
          "positive tolerance requires explicit model units");

  cancellation_source cancelled;
  cancelled.cancel();
  require(!normalize_operand(source, normalization_policy{}, unchanged,
                             &cancelled)
               .has_value() &&
              unchanged.schema == 77,
          "cancellation does not publish a partial report");

  normalization_policy work_limited;
  work_limited.resources.max_work_units = 1;
  auto exhausted = normalize_operand(source, work_limited, unchanged);
  require(!exhausted.has_value() &&
              exhausted.error().code == boolean_error_code::resource_limit &&
              unchanged.schema == 77,
          "work exhaustion is typed and transactional");

  normalization_policy report_limited;
  report_limited.resources.max_report_bytes = 1;
  auto oversized = normalize_operand(source, report_limited, unchanged);
  require(!oversized.has_value() &&
              oversized.error().code == boolean_error_code::resource_limit &&
              unchanged.schema == 77,
          "report byte exhaustion is typed and transactional");

  normalization_policy face_limited;
  face_limited.resources.max_work_units = 20;
  fv_surface_mesh<T, I> large_face;
  for (I i = 0; i != I(32); ++i)
    large_face.vertices.push_back({T(i), T(i % 3), T(0)});
  large_face.faces.emplace_back();
  for (I i = 0; i != I(32); ++i) large_face.faces[0].push_back(i);
  auto face_exhausted = normalize_operand(large_face, face_limited, unchanged);
  require(!face_exhausted.has_value() &&
              face_exhausted.error().code == boolean_error_code::resource_limit &&
              unchanged.schema == 77,
          "large canonical facet work is deterministically bounded");
}

void verifier_embedded_limits() {
  using T = double;
  using I = std::uint32_t;
  const auto source = tetra<T, I>();
  normalization_report base;
  require(normalize_operand(source, normalization_policy{}, base).has_value(),
          "embedded-limit fixture normalizes");
  const auto forge = [](normalization_report report) {
    report.policy_digest = normalization_policy_digest(report.policy).value();
    report.report_digest = normalization_report_digest(report).value();
    return encode_normalization_report(report).value();
  };

  auto bytes_limited = base;
  bytes_limited.policy.resources.max_report_bytes = 1;
  auto bytes = forge(bytes_limited);
  auto byte_rejected = verify_normalization_report(bytes, source, &source);
  require(!byte_rejected.has_value() &&
              byte_rejected.error().code == boolean_error_code::resource_limit,
          "verifier enforces embedded report-byte limit");

  auto mappings_limited = base;
  mappings_limited.policy.resources.max_mapping_entries = 1;
  auto mapping_rejected =
      verify_normalization_report(forge(mappings_limited), source, &source);
  require(!mapping_rejected.has_value() &&
              mapping_rejected.error().code == boolean_error_code::resource_limit,
          "verifier enforces embedded mapping limit");

  auto work_limited = base;
  work_limited.policy.resources.max_work_units = 1;
  auto work_rejected =
      verify_normalization_report(forge(work_limited), source, &source);
  require(!work_rejected.has_value() &&
              work_rejected.error().code == boolean_error_code::resource_limit,
          "verifier independently enforces work limit");

  auto invalid = source;
  invalid.faces[0][0] = I(99);
  normalization_report invalid_report;
  require(!normalize_operand(invalid, normalization_policy{}, invalid_report)
               .has_value(),
          "defect-limit fixture diagnoses");
  invalid_report.policy.resources.max_defect_records = 0;
  auto defect_rejected = verify_normalization_report(
      forge(invalid_report), invalid,
      static_cast<const fv_surface_mesh<T, I> *>(nullptr));
  require(!defect_rejected.has_value() &&
              defect_rejected.error().code == boolean_error_code::resource_limit,
          "verifier enforces embedded defect-record limit");

  cancellation_source cancelled;
  cancelled.cancel();
  auto canonical = encode_normalization_report(base);
  require(canonical.has_value() &&
              !verify_normalization_report(canonical.value(), source, &source,
                                           &cancelled)
                   .has_value(),
          "verifier cancellation is checked deterministically");
}

int main() {
  try {
    valid_identity<float, std::uint32_t>();
    valid_identity<float, std::uint64_t>();
    valid_identity<double, std::uint32_t>();
    valid_identity<double, std::uint64_t>();
    invalid_diagnosis();
    advisory_findings();
    structural_irrelevant_storage_removal();
    exact_duplicate_repair_basic<float, std::uint32_t>();
    exact_duplicate_repair_basic<float, std::uint64_t>();
    exact_duplicate_repair_basic<double, std::uint32_t>();
    exact_duplicate_repair_basic<double, std::uint64_t>();
    exact_duplicate_attributes_and_rejection();
    seam_aware_vertex_consolidation_basic<float, std::uint32_t>();
    seam_aware_vertex_consolidation_basic<float, std::uint64_t>();
    seam_aware_vertex_consolidation_basic<double, std::uint32_t>();
    seam_aware_vertex_consolidation_basic<double, std::uint64_t>();
    seam_preservation_and_tolerance_rejection();
    orientation_repair_basic<float, std::uint32_t>();
    orientation_repair_basic<float, std::uint64_t>();
    orientation_repair_basic<double, std::uint32_t>();
    orientation_repair_basic<double, std::uint64_t>();
    orientation_nested_shells_and_fail_closed();
    attribute_preservation_and_binding();
    codec_and_verifier_rejection();
    policy_cancellation_and_limits();
    verifier_embedded_limits();
    std::cout << "PASS mesh normalization\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
