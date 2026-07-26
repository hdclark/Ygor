#include "YgorMeshesBooleanQualificationComparison.h"

#include "YgorMeshesBooleanPreparation.h"
#include "YgorMeshesExactKernel.h"

#include <algorithm>
#include <limits>
#include <new>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> case_tag{{'Y', 'G', 'B', 'Q', 'C', 'C', '0', '1'}};
constexpr std::array<char, 8> workload_tag{{'Y', 'G', 'B', 'Q', 'C', 'W', '0', '1'}};
constexpr std::array<char, 8> attempt_evidence_tag{{'Y', 'G', 'B', 'Q', 'C', 'A', '0', '1'}};
constexpr std::array<char, 8> probe_tag{{'Y', 'G', 'B', 'Q', 'C', 'P', '0', '1'}};
constexpr std::array<char, 8> difference_tag{{'Y', 'G', 'B', 'Q', 'C', 'D', '0', '1'}};
constexpr std::array<char, 8> minimization_tag{{'Y', 'G', 'B', 'Q', 'C', 'M', '0', '1'}};
constexpr std::array<char, 8> resolution_tag{{'Y', 'G', 'B', 'Q', 'C', 'R', '0', '1'}};
constexpr std::array<char, 8> evidence_tag{{'Y', 'G', 'B', 'Q', 'C', 'E', '0', '1'}};
constexpr std::array<char, 8> campaign_tag{{'Y', 'G', 'B', 'Q', 'C', 'G', '0', '1'}};
constexpr std::array<char, 8> payload_tag{{'Y', 'G', 'B', 'Q', 'C', 'L', '0', '1'}};
constexpr std::array<char, 8> observed_probe_tag{{'Y', 'G', 'B', 'Q', 'C', 'O', '0', '1'}};
constexpr std::array<char, 8> failure_tag{{'Y', 'G', 'B', 'Q', 'C', 'F', '0', '1'}};

product_error comparison_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

product_error comparison_error(product_error_code code, const std::string &key) {
  return make_product_error(code, key);
}

bool digest_zero(const digest &value) noexcept {
  return value == digest{};
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

void encode_identity(canonical_encoder &encoder,
                     const backend_identity &identity) {
  auto bytes = encode_backend_identity(identity);
  if (!bytes.has_value())
    throw std::invalid_argument("invalid backend identity");
  encoder.byte_string(bytes.value());
}

void encode_error(canonical_encoder &encoder, const product_error &failure) {
  encoder.u16(failure.schema);
  encoder.u16(static_cast<std::uint16_t>(failure.code));
  encoder.u32(failure.subcode);
  encoder.string(failure.message_key);
  encoder.string(failure.detail);
  encoder.boolean(failure.backend.has_value());
  if (failure.backend)
    encode_identity(encoder, *failure.backend);
  encode_digest(encoder, failure.replay_binding_digest);
}

void encode_point(canonical_encoder &encoder, const exact_point3 &point) {
  encode(encoder, point.x);
  encode(encoder, point.y);
  encode(encoder, point.z);
}

void encode_box(canonical_encoder &encoder,
                const qualification_axis_aligned_box &box) {
  for (const auto value : box.minimum)
    encoder.signed_magnitude(value);
  for (const auto value : box.maximum)
    encoder.signed_magnitude(value);
}

bool valid_box(const qualification_axis_aligned_box &box) noexcept {
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (box.minimum[axis] >= box.maximum[axis])
      return false;
  return true;
}

std::vector<std::uint8_t>
case_body(const qualification_backend_comparison_case &record) {
  canonical_encoder encoder;
  encoder.u16(record.schema);
  encoder.string(record.identifier);
  encoder.string(record.workload_profile);
  encoder.u64(record.ordinal);
  encoder.byte(static_cast<std::uint8_t>(record.selected_operation));
  encode_box(encoder, record.operand_a);
  encode_box(encoder, record.operand_b);
  encoder.u16(record.limits.schema);
  encoder.u64(record.limits.max_diagnostic_cells);
  encoder.u64(record.limits.max_diagnostic_records);
  encoder.u64(record.limits.max_diagnostic_bytes);
  return encoder.bytes();
}

qualification_backend_comparison_case
canonical_case(qualification_backend_comparison_case record) {
  record.canonical_bytes = case_body(record);
  record.case_digest = domain_digest(case_tag, record.canonical_bytes);
  return record;
}

void encode_attempt(canonical_encoder &encoder,
                    const qualification_backend_attempt_evidence &attempt) {
  encoder.u16(attempt.schema);
  encode_identity(encoder, attempt.backend);
  encoder.byte(static_cast<std::uint8_t>(attempt.role));
  encode_digest(encoder, attempt.request_digest);
  encoder.boolean(attempt.evaluation_succeeded);
  encoder.boolean(attempt.verification_succeeded);
  encoder.boolean(attempt.evaluation_failure.has_value());
  if (attempt.evaluation_failure)
    encode_error(encoder, *attempt.evaluation_failure);
  encoder.boolean(attempt.verification_failure.has_value());
  if (attempt.verification_failure)
    encode_error(encoder, *attempt.verification_failure);
  encoder.byte_string(attempt.attempt_canonical_bytes);
  encoder.byte_string(attempt.exact_result_canonical_bytes);
  encoder.byte_string(attempt.realization_canonical_bytes);
  encoder.byte_string(attempt.output_canonical_bytes);
  encoder.byte_string(attempt.diagnostic_canonical_bytes);
  encode_digest(encoder, attempt.payload_digest);
}

qualification_backend_attempt_evidence
canonical_attempt(qualification_backend_attempt_evidence attempt) {
  canonical_encoder encoder;
  encode_attempt(encoder, attempt);
  attempt.evidence_digest =
      domain_digest(attempt_evidence_tag, encoder.bytes());
  return attempt;
}

qualification_guarded_probe_evidence
canonical_probe(qualification_guarded_probe_evidence probe) {
  canonical_encoder encoder;
  encoder.u16(probe.schema);
  for (const auto value : probe.cell_index)
    encoder.u64(value);
  encode_point(encoder, probe.open_cell_minimum);
  encode_point(encoder, probe.open_cell_maximum);
  encode_point(encoder, probe.midpoint);
  encoder.boolean(probe.comparator_occupied);
  encoder.byte(static_cast<std::uint8_t>(probe.producer_classification));
  encoder.boolean(probe.producer_occupied);
  encoder.boolean(probe.classifications_match);
  probe.probe_digest = domain_digest(probe_tag, encoder.bytes());
  return probe;
}

qualification_semantic_difference
canonical_difference(qualification_semantic_difference difference) {
  canonical_encoder encoder;
  encoder.u16(difference.schema);
  encoder.byte(static_cast<std::uint8_t>(difference.kind));
  encoder.boolean(difference.material);
  encoder.boolean(difference.cell_index.has_value());
  if (difference.cell_index)
    for (const auto value : *difference.cell_index)
      encoder.u64(value);
  encode_digest(encoder, difference.expected_digest);
  encode_digest(encoder, difference.observed_digest);
  encoder.string(difference.message_key);
  difference.difference_digest =
      domain_digest(difference_tag, encoder.bytes());
  return difference;
}

std::vector<std::uint8_t>
minimization_body(const qualification_comparison_minimization &record) {
  canonical_encoder encoder;
  encoder.u16(record.schema);
  encode_digest(encoder, record.original_case_digest);
  encoder.byte_string(record.minimized_case.canonical_bytes);
  encoder.u64(record.attempts);
  encoder.u64(record.edits.size());
  for (const auto &edit : record.edits) {
    encoder.byte(edit.operand);
    encoder.byte(edit.axis);
    encoder.boolean(edit.maximum);
    encoder.signed_magnitude(edit.before);
    encoder.signed_magnitude(edit.after);
    encode_digest(encoder, edit.before_digest);
    encode_digest(encoder, edit.after_digest);
  }
  return encoder.bytes();
}

qualification_comparison_minimization
canonical_minimization(qualification_comparison_minimization record) {
  record.transcript_digest =
      domain_digest(minimization_tag, minimization_body(record));
  return record;
}

std::vector<std::uint8_t>
resolution_body(const qualification_disagreement_resolution &resolution) {
  canonical_encoder encoder;
  encoder.u16(resolution.schema);
  encoder.byte(static_cast<std::uint8_t>(resolution.producer));
  encoder.byte(static_cast<std::uint8_t>(resolution.comparator));
  encoder.string(resolution.reviewer);
  encoder.string(resolution.rationale);
  encode_digest(encoder, resolution.evidence_digest);
  return encoder.bytes();
}

qualification_disagreement_resolution
canonical_resolution(qualification_disagreement_resolution resolution) {
  resolution.resolution_digest =
      domain_digest(resolution_tag, resolution_body(resolution));
  return resolution;
}

bool valid_assessment(qualification_backend_assessment value) noexcept {
  return value >= qualification_backend_assessment::correct &&
         value <= qualification_backend_assessment::unresolved;
}

bool resolution_pair_valid(
    const qualification_disagreement_resolution &resolution) noexcept {
  if (!valid_assessment(resolution.producer) ||
      !valid_assessment(resolution.comparator) ||
      resolution.producer == qualification_backend_assessment::unresolved ||
      resolution.comparator == qualification_backend_assessment::unresolved)
    return false;
  if (resolution.producer == qualification_backend_assessment::correct &&
      resolution.comparator == qualification_backend_assessment::incorrect)
    return true;
  if (resolution.producer == qualification_backend_assessment::incorrect &&
      resolution.comparator == qualification_backend_assessment::correct)
    return true;
  if (resolution.producer == qualification_backend_assessment::policy_different &&
      resolution.comparator ==
          qualification_backend_assessment::policy_different)
    return true;
  if (resolution.producer == qualification_backend_assessment::correct &&
      resolution.comparator == qualification_backend_assessment::unsupported)
    return true;
  if (resolution.producer == qualification_backend_assessment::unsupported &&
      resolution.comparator == qualification_backend_assessment::correct)
    return true;
  return false;
}

boolean_product_options comparison_product_options(
    const qualification_backend_comparison_case &comparison_case) {
  boolean_product_options product;
  product.backend.mode = backend_selection_mode::diagnostic_compare;
  product.backend.requested_backend = backend_id::experimental_exact_v1;
  product.backend.allow_experimental_backend = true;
  product.backend.diagnostic_backends = {
      backend_id::independent_axis_aligned_box_v1};
  product.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  product.qualification.workload_profile = comparison_case.workload_profile;
  product.preparation.mode = preparation_mode::strict_validation;
  product.result.representation = result_representation::exact_in_T_mesh;
  product.realization.semantics = product_realization_semantics::exact_in_T;
  product.realization.search.strategy =
      realization_search_strategy::nearest_only;
  return product;
}

product_status_or<std::pair<std::vector<std::uint8_t>,
                            std::vector<std::uint8_t>>>
comparison_option_bytes(
    const qualification_backend_comparison_case &comparison_case) {
  auto engine = encode_options(boolean_options{});
  if (!engine.has_value())
    return comparison_error(promote_error_code(engine.error().code),
                            engine.error().message_key);
  auto product = encode_product_options(
      comparison_product_options(comparison_case));
  if (!product.has_value())
    return product.error();
  return std::make_pair(std::move(engine.value()),
                        std::move(product.value()));
}

void encode_comparison_record(canonical_encoder &encoder,
                              const backend_comparison_record &record) {
  encoder.u16(record.schema);
  encode_identity(encoder, record.producer);
  encode_identity(encoder, record.comparator);
  encoder.byte(static_cast<std::uint8_t>(record.outcome));
  encode_digest(encoder, record.request_digest);
  encode_digest(encoder, record.expected_digest);
  encode_digest(encoder, record.observed_digest);
  encoder.u64(record.mismatched_cells);
  encoder.boolean(record.exact_volume_matches);
  encoder.boolean(record.component_count_matches);
  encoder.boolean(record.output_bounds_match);
  encoder.string(record.message_key);
  encode_digest(encoder, record.report_digest);
}

std::vector<std::uint8_t>
evidence_body(const qualification_backend_comparison_evidence &record) {
  canonical_encoder encoder;
  encoder.u16(record.schema);
  encoder.u32(record.checker_version);
  encoder.byte_string(record.comparison_case.canonical_bytes);
  encoder.byte_string(record.engine_options_canonical_bytes);
  encoder.byte_string(record.product_options_canonical_bytes);
  for (const auto extent : record.probe_grid_shape)
    encoder.u64(extent);
  encode_attempt(encoder, record.producer_attempt);
  encode_digest(encoder, record.producer_attempt.evidence_digest);
  encode_attempt(encoder, record.comparator_attempt);
  encode_digest(encoder, record.comparator_attempt.evidence_digest);
  encode_comparison_record(encoder, record.comparison);
  encoder.u64(record.probes.size());
  for (const auto &probe : record.probes)
    encode_digest(encoder, probe.probe_digest);
  encoder.u64(record.differences.size());
  for (const auto &difference : record.differences)
    encode_digest(encoder, difference.difference_digest);
  encoder.boolean(record.minimization.has_value());
  if (record.minimization)
    encode_digest(encoder, record.minimization->transcript_digest);
  encoder.boolean(record.resolution.has_value());
  if (record.resolution)
    encode_digest(encoder, record.resolution->resolution_digest);
  encoder.boolean(record.material_disagreement);
  encoder.boolean(record.blocking);
  return encoder.bytes();
}

qualification_backend_comparison_evidence
canonical_evidence(qualification_backend_comparison_evidence record) {
  record.canonical_bytes = evidence_body(record);
  record.evidence_digest = domain_digest(evidence_tag, record.canonical_bytes);
  return record;
}

std::vector<std::uint8_t>
campaign_body(const qualification_backend_comparison_campaign &campaign) {
  canonical_encoder encoder;
  encoder.u16(campaign.schema);
  encoder.u32(campaign.checker_version);
  encoder.string(campaign.identifier);
  encoder.string(campaign.workload_profile);
  encode_digest(encoder, campaign.workload_digest);
  encode_identity(encoder, campaign.producer);
  encode_identity(encoder, campaign.comparator);
  encoder.u64(campaign.records.size());
  for (const auto &record : campaign.records)
    encode_digest(encoder, record.evidence_digest);
  encoder.u64(campaign.agreement_count);
  encoder.u64(campaign.disagreement_count);
  encoder.u64(campaign.unsupported_count);
  encoder.u64(campaign.blocking_issue_count);
  encoder.boolean(campaign.complete);
  return encoder.bytes();
}

qualification_backend_comparison_campaign
canonical_campaign(qualification_backend_comparison_campaign campaign) {
  campaign.canonical_bytes = campaign_body(campaign);
  campaign.campaign_digest =
      domain_digest(campaign_tag, campaign.canonical_bytes);
  return campaign;
}

qualification_semantic_difference make_difference(
    qualification_semantic_difference_kind kind, const char *key,
    const digest &expected = {}, const digest &observed = {},
    std::optional<std::array<std::uint64_t, 3>> cell = std::nullopt) {
  qualification_semantic_difference difference;
  difference.kind = kind;
  difference.cell_index = cell;
  difference.expected_digest = expected;
  difference.observed_digest = observed;
  difference.message_key = key;
  return canonical_difference(std::move(difference));
}

digest failure_digest(const product_error &failure) {
  canonical_encoder encoder;
  encode_error(encoder, failure);
  return domain_digest(failure_tag, encoder.bytes());
}

digest bool_digest(bool value) {
  canonical_encoder encoder;
  encoder.boolean(value);
  return domain_digest(observed_probe_tag, encoder.bytes());
}

bool strict_midpoint(const qualification_guarded_probe_evidence &probe) {
  const std::array<const exact_scalar *, 3> lo{{&probe.open_cell_minimum.x,
                                                &probe.open_cell_minimum.y,
                                                &probe.open_cell_minimum.z}};
  const std::array<const exact_scalar *, 3> hi{{&probe.open_cell_maximum.x,
                                                &probe.open_cell_maximum.y,
                                                &probe.open_cell_maximum.z}};
  const std::array<const exact_scalar *, 3> mid{{&probe.midpoint.x,
                                                 &probe.midpoint.y,
                                                 &probe.midpoint.z}};
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (lo[axis]->compare(*mid[axis]) >= 0 ||
        mid[axis]->compare(*hi[axis]) >= 0)
      return false;
  return true;
}

template <class T>
product_status_or<T> exact_integer_coordinate(std::int64_t value) {
  const exact_rational exact(value);
  auto rounded = round_binary_nearest_even<T>(exact);
  if (!rounded)
    return comparison_error(product_error_code::output_not_representable,
                            "qualification_comparison.coordinate_range");
  auto comparison = compare_binary_bits<T>(*rounded, exact);
  if (!comparison || *comparison != 0)
    return comparison_error(product_error_code::output_not_representable,
                            "qualification_comparison.coordinate_not_exact");
  return value_of_bits<T>(*rounded);
}

template <class T, class I>
product_status_or<fv_surface_mesh<T, I>>
materialize_box(const qualification_axis_aligned_box &box) {
  try {
    std::array<T, 3> lo, hi;
    for (std::size_t axis = 0; axis != 3; ++axis) {
      auto l = exact_integer_coordinate<T>(box.minimum[axis]);
      auto h = exact_integer_coordinate<T>(box.maximum[axis]);
      if (!l.has_value())
        return l.error();
      if (!h.has_value())
        return h.error();
      lo[axis] = l.value();
      hi[axis] = h.value();
    }
    fv_surface_mesh<T, I> mesh;
    mesh.vertices = {{lo[0], lo[1], lo[2]}, {hi[0], lo[1], lo[2]},
                     {hi[0], hi[1], lo[2]}, {lo[0], hi[1], lo[2]},
                     {lo[0], lo[1], hi[2]}, {hi[0], lo[1], hi[2]},
                     {hi[0], hi[1], hi[2]}, {lo[0], hi[1], hi[2]}};
    mesh.faces = {{I(0), I(3), I(2), I(1)},
                  {I(4), I(5), I(6), I(7)},
                  {I(0), I(1), I(5), I(4)},
                  {I(1), I(2), I(6), I(5)},
                  {I(2), I(3), I(7), I(6)},
                  {I(3), I(0), I(4), I(7)}};
    return mesh;
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.box_allocation");
  }
}

template <class T, class I>
qualification_backend_attempt_evidence build_attempt_evidence(
    const backend_identity &identity, backend_adapter_role role,
    const digest &request_digest,
    const product_status_or<backend_attempt<T, I>> &evaluation,
    const boolean_backend<T, I> &adapter,
    const backend_request<T, I> &request,
    std::optional<backend_attempt<T, I>> &successful_attempt) {
  qualification_backend_attempt_evidence evidence;
  evidence.backend = identity;
  evidence.role = role;
  evidence.request_digest = request_digest;
  if (!evaluation.has_value()) {
    evidence.evaluation_failure = evaluation.error();
    return canonical_attempt(std::move(evidence));
  }
  evidence.evaluation_succeeded = true;
  successful_attempt = evaluation.value();
  evidence.attempt_canonical_bytes = evaluation.value().canonical_bytes;
  if (evaluation.value().product && *evaluation.value().product) {
    const auto &product = **evaluation.value().product;
    evidence.exact_result_canonical_bytes =
        product.exact_result->canonical_bytes;
    if (product.mesh) {
      evidence.realization_canonical_bytes =
          product.mesh->realization_canonical_bytes;
      evidence.output_canonical_bytes = product.mesh->output_canonical_bytes;
    }
  }
  if (evaluation.value().diagnostic)
    evidence.diagnostic_canonical_bytes =
        evaluation.value().diagnostic->canonical_bytes;

  auto verified = adapter.verify(request, evaluation.value());
  if (!verified.has_value()) {
    evidence.verification_failure = verified.error();
  } else if (!verified.value()) {
    evidence.verification_failure = comparison_error(
        product_error_code::verifier_disagreement,
        "qualification_comparison.adapter_verification_false");
  } else {
    evidence.verification_succeeded = true;
  }
  canonical_encoder payload;
  payload.byte_string(evidence.attempt_canonical_bytes);
  payload.byte_string(evidence.exact_result_canonical_bytes);
  payload.byte_string(evidence.realization_canonical_bytes);
  payload.byte_string(evidence.output_canonical_bytes);
  payload.byte_string(evidence.diagnostic_canonical_bytes);
  evidence.payload_digest = domain_digest(payload_tag, payload.bytes());
  return canonical_attempt(std::move(evidence));
}

template <class T, class I>
product_status_or<std::vector<qualification_guarded_probe_evidence>>
build_probes(const backend_attempt<T, I> &producer,
             const backend_attempt<T, I> &comparator) {
  try {
    if (!comparator.diagnostic)
      return std::vector<qualification_guarded_probe_evidence>{};
    std::vector<exact_triangle3> triangles;
    if (producer.product && *producer.product) {
      const auto &product = **producer.product;
      if (product.mesh && product.mesh->success) {
        const auto &mesh = product.mesh->success->mesh;
        std::vector<exact_point3> vertices;
        vertices.reserve(mesh.vertices.size());
        for (const auto &vertex : mesh.vertices) {
          auto x = decode_coordinate(vertex.x);
          auto y = decode_coordinate(vertex.y);
          auto z = decode_coordinate(vertex.z);
          if (!x.has_value() || !y.has_value() || !z.has_value())
            return comparison_error(product_error_code::unsupported_platform,
                                    "qualification_comparison.decode_output");
          vertices.push_back({x.value().value, y.value().value,
                              z.value().value});
        }
        for (const auto &face : mesh.faces) {
          if (face.size() != 3)
            return comparison_error(product_error_code::verifier_disagreement,
                                    "qualification_comparison.nontriangle_output");
          for (const auto index : face)
            if (static_cast<std::uint64_t>(index) >= vertices.size())
              return comparison_error(product_error_code::stale_binding,
                                      "qualification_comparison.output_index");
          triangles.push_back({vertices[static_cast<std::size_t>(face[0])],
                               vertices[static_cast<std::size_t>(face[1])],
                               vertices[static_cast<std::size_t>(face[2])]});
        }
      }
    }
    const auto &diagnostic = *comparator.diagnostic;
    if (diagnostic.cuts[0].size() < 2 || diagnostic.cuts[1].size() < 2 ||
        diagnostic.cuts[2].size() < 2)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.cut_grid");
    const std::size_t nx = diagnostic.cuts[0].size() - 1;
    const std::size_t ny = diagnostic.cuts[1].size() - 1;
    const std::size_t nz = diagnostic.cuts[2].size() - 1;
    if (nx != 0 && ny > std::numeric_limits<std::size_t>::max() / nx)
      return comparison_error(product_error_code::resource_limit,
                              "qualification_comparison.probe_count");
    const auto nxy = nx * ny;
    if (nxy != 0 && nz > std::numeric_limits<std::size_t>::max() / nxy)
      return comparison_error(product_error_code::resource_limit,
                              "qualification_comparison.probe_count");
    if (diagnostic.occupied_cells.size() != nxy * nz)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.occupied_cells");
    std::vector<qualification_guarded_probe_evidence> probes;
    probes.reserve(diagnostic.occupied_cells.size());
    for (std::size_t x = 0; x != nx; ++x)
      for (std::size_t y = 0; y != ny; ++y)
        for (std::size_t z = 0; z != nz; ++z) {
          const auto linear = (x * ny + y) * nz + z;
          qualification_guarded_probe_evidence probe;
          probe.cell_index = {{x, y, z}};
          probe.open_cell_minimum = {diagnostic.cuts[0][x],
                                     diagnostic.cuts[1][y],
                                     diagnostic.cuts[2][z]};
          probe.open_cell_maximum = {diagnostic.cuts[0][x + 1],
                                     diagnostic.cuts[1][y + 1],
                                     diagnostic.cuts[2][z + 1]};
          probe.midpoint = {
              (probe.open_cell_minimum.x + probe.open_cell_maximum.x) /
                  exact_scalar(2),
              (probe.open_cell_minimum.y + probe.open_cell_maximum.y) /
                  exact_scalar(2),
              (probe.open_cell_minimum.z + probe.open_cell_maximum.z) /
                  exact_scalar(2)};
          probe.comparator_occupied = diagnostic.occupied_cells[linear] != 0;
          if (triangles.empty()) {
            probe.producer_classification = solid_point_kind::outside;
          } else {
            auto location =
                classify_point_closed_triangle_shell(probe.midpoint, triangles);
            if (!location.has_value())
              return comparison_error(promote_error_code(location.error().code),
                                      location.error().message_key);
            probe.producer_classification = location.value();
          }
          probe.producer_occupied =
              probe.producer_classification == solid_point_kind::inside;
          probe.classifications_match =
              probe.producer_classification != solid_point_kind::boundary &&
              probe.producer_occupied == probe.comparator_occupied;
          probes.push_back(canonical_probe(std::move(probe)));
        }
    return probes;
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.probe_allocation");
  }
}

backend_comparison_record failure_comparison(
    const qualification_backend_attempt_evidence &producer,
    const qualification_backend_attempt_evidence &comparator,
    const digest &request_digest) {
  backend_comparison_record record;
  record.producer = producer.backend;
  record.comparator = comparator.backend;
  record.outcome = backend_comparison_outcome::unsupported;
  record.request_digest = request_digest;
  record.expected_digest = comparator.evaluation_failure
                               ? failure_digest(*comparator.evaluation_failure)
                               : comparator.payload_digest;
  record.observed_digest = producer.evaluation_failure
                               ? failure_digest(*producer.evaluation_failure)
                               : producer.payload_digest;
  record.message_key = "qualification_comparison.attempt_unavailable";
  auto made = backend_comparison_digest(record);
  if (made.has_value())
    record.report_digest = made.value();
  return record;
}

void add_attempt_differences(qualification_backend_comparison_evidence &record) {
  if (!record.producer_attempt.evaluation_succeeded)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::producer_evaluation_failure,
        "qualification_comparison.producer_evaluation_failure", {},
        record.producer_attempt.evaluation_failure
            ? failure_digest(*record.producer_attempt.evaluation_failure)
            : digest{}));
  if (!record.comparator_attempt.evaluation_succeeded)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::comparator_evaluation_failure,
        "qualification_comparison.comparator_evaluation_failure", {},
        record.comparator_attempt.evaluation_failure
            ? failure_digest(*record.comparator_attempt.evaluation_failure)
            : digest{}));
  if (record.producer_attempt.evaluation_succeeded &&
      !record.producer_attempt.verification_succeeded)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::producer_verification_failure,
        "qualification_comparison.producer_verification_failure", {},
        record.producer_attempt.verification_failure
            ? failure_digest(*record.producer_attempt.verification_failure)
            : digest{}));
  if (record.comparator_attempt.evaluation_succeeded &&
      !record.comparator_attempt.verification_succeeded)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::comparator_verification_failure,
        "qualification_comparison.comparator_verification_failure", {},
        record.comparator_attempt.verification_failure
            ? failure_digest(*record.comparator_attempt.verification_failure)
            : digest{}));
}

void add_comparison_differences(
    qualification_backend_comparison_evidence &record) {
  for (const auto &probe : record.probes)
    if (!probe.classifications_match)
      record.differences.push_back(make_difference(
          qualification_semantic_difference_kind::occupancy,
          "qualification_comparison.occupancy", bool_digest(probe.comparator_occupied),
          bool_digest(probe.producer_occupied), probe.cell_index));
  if (record.comparison.outcome == backend_comparison_outcome::unsupported)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::comparison_unsupported,
        "qualification_comparison.unsupported",
        record.comparison.expected_digest, record.comparison.observed_digest));
  if (!record.comparison.exact_volume_matches &&
      record.comparison.outcome == backend_comparison_outcome::disagree)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::exact_volume,
        "qualification_comparison.exact_volume",
        record.comparison.expected_digest, record.comparison.observed_digest));
  if (!record.comparison.component_count_matches &&
      record.comparison.outcome == backend_comparison_outcome::disagree)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::connected_components,
        "qualification_comparison.connected_components",
        record.comparison.expected_digest, record.comparison.observed_digest));
  if (!record.comparison.output_bounds_match &&
      record.comparison.outcome == backend_comparison_outcome::disagree)
    record.differences.push_back(make_difference(
        qualification_semantic_difference_kind::output_bounds,
        "qualification_comparison.output_bounds",
        record.comparison.expected_digest, record.comparison.observed_digest));
}

std::vector<std::int64_t> reduction_candidates(std::int64_t value) {
  std::vector<std::int64_t> result;
  if (value != 0)
    result.push_back(0);
  if (value > 1)
    result.push_back(1);
  if (value < -1)
    result.push_back(-1);
  if (value / 2 != value && value / 2 != 0)
    result.push_back(value / 2);
  std::sort(result.begin(), result.end(), [](std::int64_t a, std::int64_t b) {
    const auto abs_a = a < 0 ? std::uint64_t(-(a + 1)) + 1 : std::uint64_t(a);
    const auto abs_b = b < 0 ? std::uint64_t(-(b + 1)) + 1 : std::uint64_t(b);
    return std::tie(abs_a, a) < std::tie(abs_b, b);
  });
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}

bool valid_minimization_transcript(
    const qualification_comparison_minimization &record,
    const qualification_backend_comparison_case &source) {
  if (record.schema != qualification_backend_comparison_schema_version ||
      record.original_case_digest != source.case_digest ||
      record.attempts < record.edits.size() ||
      digest_zero(record.transcript_digest) ||
      canonical_minimization(record).transcript_digest !=
          record.transcript_digest)
    return false;
  auto source_valid = validate_qualification_backend_comparison_case(source);
  auto minimized_valid =
      validate_qualification_backend_comparison_case(record.minimized_case);
  if (!source_valid.has_value() || !minimized_valid.has_value())
    return false;
  auto current = source;
  for (const auto &edit : record.edits) {
    if (edit.operand >= 2 || edit.axis >= 3 || edit.before == edit.after ||
        edit.before_digest != current.case_digest)
      return false;
    auto &box = edit.operand == 0 ? current.operand_a : current.operand_b;
    auto &coordinate = edit.maximum ? box.maximum[edit.axis]
                                    : box.minimum[edit.axis];
    if (coordinate != edit.before)
      return false;
    const auto candidates = reduction_candidates(edit.before);
    if (std::find(candidates.begin(), candidates.end(), edit.after) ==
        candidates.end())
      return false;
    coordinate = edit.after;
    auto canonical =
        make_qualification_backend_comparison_case(std::move(current));
    if (!canonical.has_value() ||
        canonical.value().case_digest != edit.after_digest)
      return false;
    current = std::move(canonical.value());
  }
  return current.case_digest == record.minimized_case.case_digest &&
         current.canonical_bytes == record.minimized_case.canonical_bytes;
}

} // namespace

const char *qualification_backend_assessment_token(
    qualification_backend_assessment assessment) noexcept {
  switch (assessment) {
  case qualification_backend_assessment::correct:
    return "correct";
  case qualification_backend_assessment::incorrect:
    return "incorrect";
  case qualification_backend_assessment::unsupported:
    return "unsupported";
  case qualification_backend_assessment::policy_different:
    return "policy-different";
  case qualification_backend_assessment::unresolved:
    return "unresolved";
  }
  return "unknown";
}

product_status_or<qualification_backend_comparison_case>
make_qualification_backend_comparison_case(
    qualification_backend_comparison_case record) {
  try {
    if (record.schema != qualification_backend_comparison_schema_version ||
        record.identifier.empty() ||
        record.workload_profile != qualification_axis_box_comparison_profile ||
        record.selected_operation > operation::symmetric_difference ||
        !valid_box(record.operand_a) || !valid_box(record.operand_b) ||
        record.limits.schema != backend_adapter_schema ||
        record.limits.max_diagnostic_cells == 0 ||
        record.limits.max_diagnostic_records == 0 ||
        record.limits.max_diagnostic_bytes == 0)
      return comparison_error(product_error_code::qualification_policy_violation,
                              "qualification_comparison.case_contract");
    return canonical_case(std::move(record));
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.case_allocation");
  }
}

product_status_or<bool> validate_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &record) noexcept {
  try {
    auto made = make_qualification_backend_comparison_case(record);
    if (!made.has_value())
      return made.error();
    if (made.value().canonical_bytes != record.canonical_bytes ||
        made.value().case_digest != record.case_digest)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.case_binding");
    return true;
  } catch (...) {
    return comparison_error(product_error_code::internal_invariant_error,
                            "qualification_comparison.case_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &record) {
  auto valid = validate_qualification_backend_comparison_case(record);
  if (!valid.has_value())
    return valid.error();
  return record.canonical_bytes;
}

std::vector<qualification_backend_comparison_case>
make_qualification_backend_comparison_workload() {
  struct seed {
    const char *name;
    qualification_axis_aligned_box a;
    qualification_axis_aligned_box b;
    operation selected_operation;
  };
  const auto box = [](std::array<std::int64_t, 3> minimum,
                      std::array<std::int64_t, 3> maximum) {
    qualification_axis_aligned_box result;
    result.minimum = minimum;
    result.maximum = maximum;
    return result;
  };
  // The diagnostic profile is deliberately narrow and non-empty: one
  // deterministic representative for every public Boolean operation.  Empty
  // finite-T realization qualification is a separate result-mode concern and
  // must not be silently folded into backend semantic comparison.
  const auto overlap_a = box({{0, 0, 0}}, {{4, 5, 6}});
  const auto overlap_b = box({{3, 2, 1}}, {{7, 8, 9}});
  const std::vector<seed> seeds{
      {"overlap_union", overlap_a, overlap_b,
       operation::regularized_union},
      {"overlap_intersection", overlap_a, overlap_b,
       operation::regularized_intersection},
      {"overlap_a_minus_b", overlap_a, overlap_b, operation::a_minus_b},
      {"overlap_b_minus_a", overlap_a, overlap_b, operation::b_minus_a},
      {"disjoint_symmetric_difference",
       box({{0, 0, 0}}, {{2, 3, 4}}),
       box({{5, 6, 7}}, {{8, 10, 12}}),
       operation::symmetric_difference}};
  std::vector<qualification_backend_comparison_case> result;
  result.reserve(seeds.size());
  std::uint64_t ordinal = 0;
  for (const auto &entry : seeds) {
    qualification_backend_comparison_case record;
    record.identifier = std::string("p6.4-axis-box-") + entry.name;
    record.ordinal = ordinal++;
    record.selected_operation = entry.selected_operation;
    record.operand_a = entry.a;
    record.operand_b = entry.b;
    auto made = make_qualification_backend_comparison_case(std::move(record));
    if (!made.has_value())
      throw std::logic_error("invalid frozen P6.4 workload");
    result.push_back(std::move(made.value()));
  }
  return result;
}

digest qualification_backend_comparison_workload_digest(
    const std::vector<qualification_backend_comparison_case> &workload) {
  canonical_encoder encoder;
  encoder.u64(workload.size());
  for (const auto &record : workload)
    encode_digest(encoder, record.case_digest);
  return domain_digest(workload_tag, encoder.bytes());
}

product_status_or<qualification_comparison_minimization>
minimize_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &source,
    const qualification_backend_comparison_reproducer &reproducer,
    std::uint64_t attempt_limit) {
  try {
    auto valid = validate_qualification_backend_comparison_case(source);
    if (!valid.has_value())
      return valid.error();
    if (!reproducer || attempt_limit == 0 || !reproducer(source))
      return comparison_error(product_error_code::qualification_policy_violation,
                              "qualification_comparison.not_reproduced");
    qualification_comparison_minimization result;
    result.original_case_digest = source.case_digest;
    result.minimized_case = source;
    bool changed = true;
    while (changed && result.attempts < attempt_limit) {
      changed = false;
      for (std::uint8_t operand = 0; operand != 2 &&
                                      result.attempts < attempt_limit;
           ++operand) {
        for (std::uint8_t axis = 0; axis != 3 &&
                                     result.attempts < attempt_limit;
             ++axis) {
          for (std::uint8_t maximum = 0;
               maximum != 2 && result.attempts < attempt_limit; ++maximum) {
            auto &box = operand == 0 ? result.minimized_case.operand_a
                                     : result.minimized_case.operand_b;
            const auto current = maximum ? box.maximum[axis] : box.minimum[axis];
            for (const auto candidate : reduction_candidates(current)) {
              if (result.attempts >= attempt_limit)
                break;
              auto proposed = result.minimized_case;
              auto &proposed_box =
                  operand == 0 ? proposed.operand_a : proposed.operand_b;
              (maximum ? proposed_box.maximum[axis]
                       : proposed_box.minimum[axis]) = candidate;
              auto made = make_qualification_backend_comparison_case(
                  std::move(proposed));
              ++result.attempts;
              if (!made.has_value() || !reproducer(made.value()))
                continue;
              qualification_comparison_minimization_edit edit;
              edit.operand = operand;
              edit.axis = axis;
              edit.maximum = maximum != 0;
              edit.before = current;
              edit.after = candidate;
              edit.before_digest = result.minimized_case.case_digest;
              edit.after_digest = made.value().case_digest;
              result.edits.push_back(std::move(edit));
              result.minimized_case = std::move(made.value());
              changed = true;
              break;
            }
          }
        }
      }
    }
    if (!reproducer(result.minimized_case))
      return comparison_error(product_error_code::backend_disagreement,
                              "qualification_comparison.minimized_not_reproduced");
    return canonical_minimization(std::move(result));
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.minimization_allocation");
  } catch (const std::exception &exception) {
    auto failure = comparison_error(product_error_code::internal_invariant_error,
                                    "qualification_comparison.minimization_exception");
    failure.detail = exception.what();
    return failure;
  }
}

product_status_or<qualification_backend_comparison_evidence>
resolve_qualification_backend_disagreement(
    qualification_backend_comparison_evidence record,
    qualification_comparison_minimization minimization,
    qualification_disagreement_resolution resolution) {
  try {
    auto valid = validate_qualification_backend_comparison_evidence(record);
    if (!valid.has_value())
      return valid.error();
    if (record.comparison.outcome == backend_comparison_outcome::agree ||
        minimization.original_case_digest != record.comparison_case.case_digest ||
        minimization.schema != qualification_backend_comparison_schema_version ||
        minimization.minimized_case.canonical_bytes.empty() ||
        digest_zero(minimization.transcript_digest) ||
        resolution.schema != qualification_backend_comparison_schema_version ||
        resolution.reviewer.empty() || resolution.rationale.empty() ||
        digest_zero(resolution.evidence_digest) ||
        !resolution_pair_valid(resolution))
      return comparison_error(product_error_code::qualification_policy_violation,
                              "qualification_comparison.resolution_contract");
    if (!valid_minimization_transcript(minimization,
                                       record.comparison_case))
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.minimization_binding");
    auto canonical_minimized = canonical_minimization(minimization);
    auto canonical_resolved = canonical_resolution(std::move(resolution));
    record.minimization = std::move(canonical_minimized);
    record.resolution = std::move(canonical_resolved);
    record.blocking = false;
    return canonical_evidence(std::move(record));
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.resolution_allocation");
  }
}

product_status_or<bool> validate_qualification_backend_comparison_evidence(
    const qualification_backend_comparison_evidence &record) noexcept {
  try {
    if (record.schema != qualification_backend_comparison_schema_version ||
        record.checker_version !=
            qualification_backend_comparison_checker_version)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.evidence_schema");
    auto valid_case =
        validate_qualification_backend_comparison_case(record.comparison_case);
    if (!valid_case.has_value())
      return valid_case.error();
    auto expected_options =
        comparison_option_bytes(record.comparison_case);
    if (!expected_options.has_value())
      return expected_options.error();
    if (record.engine_options_canonical_bytes !=
            expected_options.value().first ||
        record.product_options_canonical_bytes !=
            expected_options.value().second)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.option_binding");
    auto attempt_contract_valid = [](
        const qualification_backend_attempt_evidence &attempt) {
      if (attempt.schema != qualification_backend_comparison_schema_version ||
          digest_zero(attempt.request_digest) ||
          attempt.role > backend_adapter_role::diagnostic_only ||
          attempt.evaluation_succeeded ==
              attempt.evaluation_failure.has_value() ||
          (!attempt.evaluation_succeeded &&
           (attempt.verification_succeeded ||
            attempt.verification_failure.has_value() ||
            !attempt.attempt_canonical_bytes.empty() ||
            !attempt.exact_result_canonical_bytes.empty() ||
            !attempt.realization_canonical_bytes.empty() ||
            !attempt.output_canonical_bytes.empty() ||
            !attempt.diagnostic_canonical_bytes.empty() ||
            !digest_zero(attempt.payload_digest))) ||
          (attempt.evaluation_succeeded &&
           (attempt.attempt_canonical_bytes.empty() ||
            attempt.verification_succeeded ==
                attempt.verification_failure.has_value())))
        return false;
      auto identity_valid = validate_backend_identity(attempt.backend);
      if (!identity_valid.has_value() || !identity_valid.value())
        return false;
      if (attempt.evaluation_succeeded) {
        canonical_encoder payload;
        payload.byte_string(attempt.attempt_canonical_bytes);
        payload.byte_string(attempt.exact_result_canonical_bytes);
        payload.byte_string(attempt.realization_canonical_bytes);
        payload.byte_string(attempt.output_canonical_bytes);
        payload.byte_string(attempt.diagnostic_canonical_bytes);
        if (attempt.payload_digest !=
            domain_digest(payload_tag, payload.bytes()))
          return false;
        if (attempt.role == backend_adapter_role::producer &&
            (attempt.exact_result_canonical_bytes.empty() ||
             attempt.output_canonical_bytes.empty()))
          return false;
        if (attempt.role == backend_adapter_role::diagnostic_only &&
            attempt.diagnostic_canonical_bytes.empty())
          return false;
      }
      return true;
    };
    if (!attempt_contract_valid(record.producer_attempt) ||
        !attempt_contract_valid(record.comparator_attempt) ||
        record.producer_attempt.role != backend_adapter_role::producer ||
        record.producer_attempt.backend.id !=
            backend_id::experimental_exact_v1 ||
        record.producer_attempt.backend.maturity ==
            backend_maturity::deprecated ||
        record.comparator_attempt.role !=
            backend_adapter_role::diagnostic_only ||
        record.comparator_attempt.backend.id !=
            backend_id::independent_axis_aligned_box_v1 ||
        (record.comparator_attempt.backend.maturity !=
             backend_maturity::candidate &&
         record.comparator_attempt.backend.maturity !=
             backend_maturity::qualified))
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.attempt_contract");
    const auto producer = canonical_attempt(record.producer_attempt);
    const auto comparator = canonical_attempt(record.comparator_attempt);
    if (producer.evidence_digest != record.producer_attempt.evidence_digest ||
        comparator.evidence_digest != record.comparator_attempt.evidence_digest ||
        record.producer_attempt.request_digest != record.comparison.request_digest ||
        record.comparator_attempt.request_digest != record.comparison.request_digest ||
        !same_backend_identity(record.producer_attempt.backend,
                               record.comparison.producer) ||
        !same_backend_identity(record.comparator_attempt.backend,
                               record.comparison.comparator))
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.attempt_binding");
    auto comparison_digest = backend_comparison_digest(record.comparison);
    if (!comparison_digest.has_value() ||
        comparison_digest.value() != record.comparison.report_digest)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.comparison_binding");
    std::set<std::array<std::uint64_t, 3>> cells;
    for (const auto &probe : record.probes) {
      const auto canonical = canonical_probe(probe);
      if (canonical.probe_digest != probe.probe_digest ||
          !strict_midpoint(probe) || !cells.insert(probe.cell_index).second ||
          (probe.producer_classification == solid_point_kind::boundary &&
           probe.classifications_match) ||
          (probe.producer_classification != solid_point_kind::boundary &&
           probe.producer_occupied !=
               (probe.producer_classification == solid_point_kind::inside)))
        return comparison_error(product_error_code::stale_binding,
                                "qualification_comparison.probe_binding");
    }
    const bool has_probe_grid =
        std::any_of(record.probe_grid_shape.begin(),
                    record.probe_grid_shape.end(),
                    [](std::uint64_t extent) { return extent != 0; });
    if (!has_probe_grid) {
      if (!record.probes.empty())
        return comparison_error(product_error_code::stale_binding,
                                "qualification_comparison.probe_grid_missing");
    } else {
      std::uint64_t expected_probe_count = 1;
      for (const auto extent : record.probe_grid_shape) {
        if (extent == 0 ||
            expected_probe_count >
                std::numeric_limits<std::uint64_t>::max() / extent)
          return comparison_error(product_error_code::stale_binding,
                                  "qualification_comparison.probe_grid");
        expected_probe_count *= extent;
      }
      if (record.probes.size() != expected_probe_count)
        return comparison_error(product_error_code::stale_binding,
                                "qualification_comparison.probe_completeness");
      for (std::uint64_t x = 0; x != record.probe_grid_shape[0]; ++x)
        for (std::uint64_t y = 0; y != record.probe_grid_shape[1]; ++y)
          for (std::uint64_t z = 0; z != record.probe_grid_shape[2]; ++z)
            if (cells.find({{x, y, z}}) == cells.end())
              return comparison_error(product_error_code::stale_binding,
                                      "qualification_comparison.probe_gap");
    }
    bool material = false;
    for (const auto &difference : record.differences) {
      const auto canonical = canonical_difference(difference);
      if (canonical.difference_digest != difference.difference_digest)
        return comparison_error(product_error_code::stale_binding,
                                "qualification_comparison.difference_binding");
      material = material || difference.material;
    }
    if (record.material_disagreement != material)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.material_binding");
    if (record.minimization &&
        !valid_minimization_transcript(*record.minimization,
                                       record.comparison_case))
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.minimization_binding");
    if (record.comparison.outcome == backend_comparison_outcome::agree) {
      if (record.blocking || record.minimization || record.resolution || material)
        return comparison_error(product_error_code::qualification_policy_violation,
                                "qualification_comparison.false_agreement");
    } else if (record.resolution) {
      const auto canonical = canonical_resolution(*record.resolution);
      if (canonical.resolution_digest != record.resolution->resolution_digest ||
          !resolution_pair_valid(*record.resolution) || !record.minimization ||
          record.blocking)
        return comparison_error(product_error_code::qualification_policy_violation,
                                "qualification_comparison.invalid_resolution");
    } else if (!record.blocking) {
      return comparison_error(product_error_code::qualification_policy_violation,
                              "qualification_comparison.unresolved_not_blocking");
    }
    const auto canonical = canonical_evidence(record);
    if (canonical.canonical_bytes != record.canonical_bytes ||
        canonical.evidence_digest != record.evidence_digest)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.evidence_binding");
    return true;
  } catch (...) {
    return comparison_error(product_error_code::internal_invariant_error,
                            "qualification_comparison.evidence_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_backend_comparison_evidence(
    const qualification_backend_comparison_evidence &record) {
  auto valid = validate_qualification_backend_comparison_evidence(record);
  if (!valid.has_value())
    return valid.error();
  return record.canonical_bytes;
}

product_status_or<qualification_backend_comparison_campaign>
make_qualification_backend_comparison_campaign(
    std::string identifier,
    std::vector<qualification_backend_comparison_evidence> records) {
  try {
    if (identifier.empty() || records.empty())
      return comparison_error(product_error_code::qualification_policy_violation,
                              "qualification_comparison.campaign_contract");
    std::sort(records.begin(), records.end(), [](const auto &a, const auto &b) {
      return a.comparison_case.ordinal < b.comparison_case.ordinal;
    });
    qualification_backend_comparison_campaign campaign;
    campaign.identifier = std::move(identifier);
    campaign.records = std::move(records);
    campaign.producer = campaign.records.front().producer_attempt.backend;
    campaign.comparator = campaign.records.front().comparator_attempt.backend;
    std::set<std::string> identifiers;
    for (const auto &record : campaign.records) {
      auto valid = validate_qualification_backend_comparison_evidence(record);
      if (!valid.has_value())
        return valid.error();
      if (!identifiers.insert(record.comparison_case.identifier).second ||
          !same_backend_identity(campaign.producer,
                                 record.producer_attempt.backend) ||
          !same_backend_identity(campaign.comparator,
                                 record.comparator_attempt.backend))
        return comparison_error(product_error_code::qualification_policy_violation,
                                "qualification_comparison.campaign_binding");
      switch (record.comparison.outcome) {
      case backend_comparison_outcome::agree:
        ++campaign.agreement_count;
        break;
      case backend_comparison_outcome::disagree:
        ++campaign.disagreement_count;
        break;
      case backend_comparison_outcome::unsupported:
        ++campaign.unsupported_count;
        break;
      }
      if (record.blocking)
        ++campaign.blocking_issue_count;
    }
    const auto workload = make_qualification_backend_comparison_workload();
    campaign.workload_digest =
        qualification_backend_comparison_workload_digest(workload);
    campaign.complete = campaign.records.size() == workload.size();
    if (campaign.complete)
      for (std::size_t i = 0; i != workload.size(); ++i)
        if (campaign.records[i].comparison_case.case_digest !=
            workload[i].case_digest) {
          campaign.complete = false;
          break;
        }
    return canonical_campaign(std::move(campaign));
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.campaign_allocation");
  }
}

product_status_or<bool> validate_qualification_backend_comparison_campaign(
    const qualification_backend_comparison_campaign &campaign) noexcept {
  try {
    auto rebuilt = make_qualification_backend_comparison_campaign(
        campaign.identifier, campaign.records);
    if (!rebuilt.has_value())
      return rebuilt.error();
    const auto &canonical = rebuilt.value();
    if (canonical.workload_profile != campaign.workload_profile ||
        canonical.workload_digest != campaign.workload_digest ||
        !same_backend_identity(canonical.producer, campaign.producer) ||
        !same_backend_identity(canonical.comparator, campaign.comparator) ||
        canonical.agreement_count != campaign.agreement_count ||
        canonical.disagreement_count != campaign.disagreement_count ||
        canonical.unsupported_count != campaign.unsupported_count ||
        canonical.blocking_issue_count != campaign.blocking_issue_count ||
        canonical.complete != campaign.complete ||
        canonical.canonical_bytes != campaign.canonical_bytes ||
        canonical.campaign_digest != campaign.campaign_digest)
      return comparison_error(product_error_code::stale_binding,
                              "qualification_comparison.campaign_canonical");
    return true;
  } catch (...) {
    return comparison_error(product_error_code::internal_invariant_error,
                            "qualification_comparison.campaign_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_backend_comparison_campaign(
    const qualification_backend_comparison_campaign &campaign) {
  auto valid = validate_qualification_backend_comparison_campaign(campaign);
  if (!valid.has_value())
    return valid.error();
  return campaign.canonical_bytes;
}

bool qualification_backend_comparison_gate_passes(
    const qualification_backend_comparison_campaign &campaign) noexcept {
  auto valid = validate_qualification_backend_comparison_campaign(campaign);
  return valid.has_value() && campaign.complete &&
         campaign.blocking_issue_count == 0 && campaign.unsupported_count == 0;
}

template <class T, class I>
product_status_or<qualification_backend_comparison_evidence>
run_qualification_backend_comparison_case(
    const qualification_backend_comparison_case &comparison_case,
    const backend_registry<T, I> &registry,
    std::shared_ptr<const exact_kernel_services<T>> kernel,
    std::shared_ptr<const verifier_service> verifiers,
    std::shared_ptr<cancellation_source> cancellation,
    diagnostic_consumer diagnostics,
    deterministic_executor_factory executor_factory) {
  try {
    auto valid_case =
        validate_qualification_backend_comparison_case(comparison_case);
    if (!valid_case.has_value())
      return valid_case.error();
    if (!kernel || !verifiers || !registry.frozen())
      return comparison_error(product_error_code::qualification_policy_violation,
                              "qualification_comparison.services");
    auto mesh_a = materialize_box<T, I>(comparison_case.operand_a);
    auto mesh_b = materialize_box<T, I>(comparison_case.operand_b);
    if (!mesh_a.has_value())
      return mesh_a.error();
    if (!mesh_b.has_value())
      return mesh_b.error();
    strict_validation_policy strict;
    boolean_options engine;
    auto prepared_a = validate_operand_strict(
        mesh_a.value(), strict, engine, kernel, verifiers,
        cancellation.get());
    if (!prepared_a.has_value())
      return comparison_error(promote_error_code(prepared_a.error().code),
                              prepared_a.error().message_key);
    auto prepared_b = validate_operand_strict(
        mesh_b.value(), strict, engine, kernel, verifiers,
        cancellation.get());
    if (!prepared_b.has_value())
      return comparison_error(promote_error_code(prepared_b.error().code),
                              prepared_b.error().message_key);

    auto product = comparison_product_options(comparison_case);
    auto option_bytes = comparison_option_bytes(comparison_case);
    if (!option_bytes.has_value())
      return option_bytes.error();

    auto request = make_backend_request(
        std::move(prepared_a.value()), std::move(prepared_b.value()),
        comparison_case.selected_operation, engine, product, kernel, verifiers,
        cancellation, diagnostics, executor_factory, comparison_case.limits);
    if (!request.has_value())
      return request.error();
    auto producer_entry = registry.lookup(backend_id::experimental_exact_v1);
    auto comparator_entry =
        registry.lookup(backend_id::independent_axis_aligned_box_v1);
    if (!producer_entry.has_value())
      return producer_entry.error();
    if (!comparator_entry.has_value())
      return comparator_entry.error();
    if (!producer_entry.value().adapter || !comparator_entry.value().adapter ||
        producer_entry.value().role != backend_adapter_role::producer ||
        comparator_entry.value().role != backend_adapter_role::diagnostic_only ||
        (comparator_entry.value().identity.maturity !=
             backend_maturity::candidate &&
         comparator_entry.value().identity.maturity !=
             backend_maturity::qualified))
      return comparison_error(product_error_code::backend_capability_mismatch,
                              "qualification_comparison.adapter_contract");

    backend_execution_state producer_state;
    producer_state.selection = backend_selection_mode::diagnostic_compare;
    // Preserve the same durable exact-result provenance contract as the
    // product backend dispatcher: diagnostic adapters are not producer
    // attempts and therefore must not appear in the producer binding.
    producer_state.attempted_backends = {
        backend_id::experimental_exact_v1};
    backend_execution_state comparator_state;
    comparator_state.selection = backend_selection_mode::diagnostic_compare;
    comparator_state.attempted_backends = {
        backend_id::experimental_exact_v1,
        backend_id::independent_axis_aligned_box_v1};
    auto producer_evaluation = producer_entry.value().adapter->evaluate(
        *request.value(), producer_state);
    auto comparator_evaluation = comparator_entry.value().adapter->evaluate(
        *request.value(), comparator_state);
    std::optional<backend_attempt<T, I>> producer_attempt;
    std::optional<backend_attempt<T, I>> comparator_attempt;

    qualification_backend_comparison_evidence evidence;
    evidence.comparison_case = comparison_case;
    evidence.engine_options_canonical_bytes =
        std::move(option_bytes.value().first);
    evidence.product_options_canonical_bytes =
        std::move(option_bytes.value().second);
    evidence.producer_attempt = build_attempt_evidence(
        producer_entry.value().identity, producer_entry.value().role,
        request.value()->request_digest, producer_evaluation,
        *producer_entry.value().adapter, *request.value(), producer_attempt);
    evidence.comparator_attempt = build_attempt_evidence(
        comparator_entry.value().identity, comparator_entry.value().role,
        request.value()->request_digest, comparator_evaluation,
        *comparator_entry.value().adapter, *request.value(), comparator_attempt);

    if (producer_attempt && comparator_attempt &&
        evidence.producer_attempt.verification_succeeded &&
        evidence.comparator_attempt.verification_succeeded) {
      auto comparison = comparator_entry.value().adapter->compare(
          *request.value(), *producer_attempt, *comparator_attempt);
      if (!comparison.has_value())
        return comparison.error();
      evidence.comparison = std::move(comparison.value());
      auto probes = build_probes(*producer_attempt, *comparator_attempt);
      if (!probes.has_value())
        return probes.error();
      evidence.probes = std::move(probes.value());
      if (comparator_attempt->diagnostic)
        for (std::size_t axis = 0; axis != 3; ++axis)
          evidence.probe_grid_shape[axis] =
              comparator_attempt->diagnostic->cuts[axis].size() - 1;
    } else {
      evidence.comparison = failure_comparison(
          evidence.producer_attempt, evidence.comparator_attempt,
          request.value()->request_digest);
    }

    add_attempt_differences(evidence);
    add_comparison_differences(evidence);
    evidence.material_disagreement =
        std::any_of(evidence.differences.begin(), evidence.differences.end(),
                    [](const auto &difference) { return difference.material; });
    evidence.blocking =
        evidence.comparison.outcome != backend_comparison_outcome::agree;
    if (evidence.comparison.outcome == backend_comparison_outcome::agree &&
        evidence.material_disagreement)
      return comparison_error(product_error_code::backend_disagreement,
                              "qualification_comparison.hidden_difference");
    evidence = canonical_evidence(std::move(evidence));
    auto valid = validate_qualification_backend_comparison_evidence(evidence);
    if (!valid.has_value())
      return valid.error();
    return evidence;
  } catch (const std::bad_alloc &) {
    return comparison_error(product_error_code::resource_limit,
                            "qualification_comparison.run_allocation");
  } catch (const std::exception &exception) {
    auto failure = comparison_error(product_error_code::internal_invariant_error,
                                    "qualification_comparison.run_exception");
    failure.detail = exception.what();
    return failure;
  }
}

#define YGOR_QUALIFICATION_COMPARISON_INSTANTIATE(T, I)                        \
  template product_status_or<qualification_backend_comparison_evidence>       \
  run_qualification_backend_comparison_case(                                   \
      const qualification_backend_comparison_case &,                           \
      const backend_registry<T, I> &,                                           \
      std::shared_ptr<const exact_kernel_services<T>>,                          \
      std::shared_ptr<const verifier_service>,                                  \
      std::shared_ptr<cancellation_source>, diagnostic_consumer,                \
      deterministic_executor_factory)
YGOR_QUALIFICATION_COMPARISON_INSTANTIATE(float, std::uint32_t);
YGOR_QUALIFICATION_COMPARISON_INSTANTIATE(float, std::uint64_t);
YGOR_QUALIFICATION_COMPARISON_INSTANTIATE(double, std::uint32_t);
YGOR_QUALIFICATION_COMPARISON_INSTANTIATE(double, std::uint64_t);
#undef YGOR_QUALIFICATION_COMPARISON_INSTANTIATE

} // namespace mesh_boolean
} // namespace ygor
