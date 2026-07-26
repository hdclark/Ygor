#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_ACCOUNTING_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_ACCOUNTING_H_

#include "YgorMeshesBooleanPreparation.h"
#include "YgorMeshesBooleanProductContractResult.h"
#include "YgorMeshesBooleanQualification.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_accounting_schema_version = 1;
constexpr std::uint32_t qualification_accounting_checker_version = 1;

enum class qualification_check_state : std::uint8_t {
  not_run,
  passed,
  failed
};

enum class qualification_check_kind : std::uint8_t {
  product_contract,
  exact_result_binding,
  representation_semantics,
  strict_reingestion,
  independent_topology,
  certificate_replay,
  guarded_occupancy,
  embedding,
  orientation,
  shell_nesting,
  attribute_mapping,
  approximation_bounds,
  chain_reingestion
};

enum class qualification_false_success_kind : std::uint8_t {
  semantic_mislabeling,
  stale_exact_result_binding,
  strict_reingestion_rejected,
  incorrect_topology,
  certificate_replay_failed,
  incorrect_occupancy,
  incorrect_embedding,
  incorrect_orientation,
  incorrect_shell_nesting,
  incorrect_attribute_mapping,
  approximation_bound_violation
};

struct qualification_verification_check {
  qualification_check_kind kind = qualification_check_kind::product_contract;
  qualification_check_state state = qualification_check_state::not_run;
  std::string message_key;
  digest evidence_digest;
};

struct qualification_guarded_probe_observation {
  std::uint16_t schema = qualification_accounting_schema_version;
  std::string identifier;
  digest point_digest;
  bool expected_occupied = false;
  bool observed_occupied = false;
  bool observed_on_boundary = false;
  digest probe_digest;
};

struct qualification_mesh_topology_reconstruction {
  std::uint16_t schema = qualification_accounting_schema_version;
  std::uint64_t vertices = 0;
  std::uint64_t faces = 0;
  std::uint64_t edges = 0;
  std::uint64_t connected_components = 0;
  std::int64_t euler_characteristic = 0;
  bool face_rings_valid = false;
  bool indices_in_range = false;
  bool every_vertex_referenced = false;
  bool closed_two_uses_per_edge = false;
  bool opposite_edge_directions = false;
  digest incidence_digest;
  digest reconstruction_digest;

  bool passed() const noexcept {
    return face_rings_valid && indices_in_range && every_vertex_referenced &&
           closed_two_uses_per_edge && opposite_edge_directions;
  }
};

struct qualification_success_verification {
  std::uint16_t schema = qualification_accounting_schema_version;
  std::uint32_t checker_version = qualification_accounting_checker_version;
  bool mesh_published = false;
  bool chain_reingestion_required = false;
  result_representation representation = result_representation::exact_stratified;
  digest exact_result_digest;
  digest output_digest;
  qualification_mesh_topology_reconstruction topology;
  std::vector<qualification_guarded_probe_observation> probes;
  std::vector<qualification_verification_check> checks;
  digest verification_digest;
};

struct qualification_case_observation {
  std::uint16_t schema = qualification_accounting_schema_version;
  std::string identifier;
  digest case_digest;
  qualification_dimension_key dimensions;
  std::vector<qualification_outcome> expected_outcomes;
  std::vector<product_error_code> expected_failure_codes;
  bool published_success = false;
  std::optional<product_error> failure;
  bool backend_disagreement = false;
  bool verifier_disagreement = false;
  bool nondeterministic = false;
  bool timeout_or_resource_limit = false;
  bool infrastructure_failure = false;
  std::optional<qualification_success_verification> success_verification;
  digest observation_digest;
};

struct qualification_case_accounting {
  std::uint16_t schema = qualification_accounting_schema_version;
  qualification_case_observation observation;
  qualification_outcome outcome = qualification_outcome::infrastructure_failure;
  bool blocking = true;
  bool safe_failure = false;
  std::vector<qualification_false_success_kind> false_success_reasons;
  std::vector<std::uint8_t> canonical_bytes;
  digest accounting_digest;
};

struct qualification_accounting_campaign {
  std::uint16_t schema = qualification_accounting_schema_version;
  std::uint32_t checker_version = qualification_accounting_checker_version;
  std::string identifier;
  std::vector<qualification_case_accounting> records;
  std::vector<qualification_outcome_count> counts;
  std::uint64_t total_case_count = 0;
  std::uint64_t safe_failure_count = 0;
  std::uint64_t false_success_count = 0;
  std::uint64_t blocking_issue_count = 0;
  bool complete = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest campaign_digest;
};

const char *qualification_check_kind_token(qualification_check_kind) noexcept;
const char *qualification_false_success_kind_token(
    qualification_false_success_kind) noexcept;

product_status_or<qualification_verification_check>
make_qualification_verification_check(qualification_verification_check);
product_status_or<qualification_guarded_probe_observation>
make_qualification_guarded_probe_observation(
    qualification_guarded_probe_observation);
product_status_or<qualification_success_verification>
make_qualification_success_verification(qualification_success_verification);
product_status_or<bool> validate_qualification_success_verification(
    const qualification_success_verification &) noexcept;

product_status_or<qualification_case_observation>
make_qualification_case_observation(qualification_case_observation);
product_status_or<bool> validate_qualification_case_observation(
    const qualification_case_observation &) noexcept;
product_status_or<qualification_case_accounting>
account_qualification_case(qualification_case_observation);
product_status_or<bool> validate_qualification_case_accounting(
    const qualification_case_accounting &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_case_accounting(const qualification_case_accounting &);

product_status_or<qualification_accounting_campaign>
make_qualification_accounting_campaign(
    std::string, std::vector<qualification_case_accounting>, bool complete = true);
product_status_or<bool> validate_qualification_accounting_campaign(
    const qualification_accounting_campaign &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_accounting_campaign(
    const qualification_accounting_campaign &);
bool qualification_false_success_gate_passes(
    const qualification_accounting_campaign &) noexcept;

product_status_or<qualification_result_summary>
make_qualification_result_summary_from_accounting(
    const qualification_accounting_campaign &, digest manifest_digest,
    std::string run_identifier, std::string repository_commit,
    std::string started_utc, std::string finished_utc,
    std::vector<qualification_artifact_reference> artifacts = {});

namespace qualification_accounting_detail {
product_error accounting_error(product_error_code, const char *);
product_status_or<qualification_mesh_topology_reconstruction>
canonicalize_topology_reconstruction(
    qualification_mesh_topology_reconstruction);
digest mesh_observation_digest(const digest &, const char *);
} // namespace qualification_accounting_detail

template <class T, class I>
product_status_or<qualification_mesh_topology_reconstruction>
reconstruct_qualification_mesh_topology(const fv_surface_mesh<T, I> &mesh) {
  static_assert(std::is_floating_point<T>::value,
                "qualification meshes require floating coordinates");
  static_assert(std::is_unsigned<I>::value,
                "qualification meshes require unsigned indices");
  qualification_mesh_topology_reconstruction result;
  result.vertices = mesh.vertices.size();
  result.faces = mesh.faces.size();
  result.face_rings_valid = true;
  result.indices_in_range = true;
  result.every_vertex_referenced = mesh.vertices.empty();
  result.closed_two_uses_per_edge = true;
  result.opposite_edge_directions = true;

  using edge = std::pair<std::uint64_t, std::uint64_t>;
  struct edge_use {
    std::uint64_t face = 0;
    std::uint64_t from = 0;
    std::uint64_t to = 0;
  };
  std::map<edge, std::vector<edge_use>> uses;
  std::vector<bool> referenced(mesh.vertices.size(), false);
  std::vector<std::vector<std::uint64_t>> adjacency(mesh.faces.size());
  try {
    for (std::size_t face_index = 0; face_index != mesh.faces.size();
         ++face_index) {
      const auto &ring = mesh.faces[face_index];
      if (ring.size() < 3) {
        result.face_rings_valid = false;
        continue;
      }
      std::set<std::uint64_t> distinct;
      for (std::size_t j = 0; j != ring.size(); ++j) {
        const auto from = static_cast<std::uint64_t>(ring[j]);
        const auto to = static_cast<std::uint64_t>(ring[(j + 1) % ring.size()]);
        if (from >= mesh.vertices.size() || to >= mesh.vertices.size()) {
          result.indices_in_range = false;
          continue;
        }
        referenced[from] = true;
        distinct.insert(from);
        if (from == to)
          result.face_rings_valid = false;
        uses[{std::min(from, to), std::max(from, to)}].push_back(
            {static_cast<std::uint64_t>(face_index), from, to});
      }
      if (distinct.size() < 3)
        result.face_rings_valid = false;
    }
    result.every_vertex_referenced =
        std::all_of(referenced.begin(), referenced.end(),
                    [](bool value) { return value; });
    canonical_encoder incidence;
    incidence.u64(uses.size());
    for (const auto &entry : uses) {
      incidence.u64(entry.first.first);
      incidence.u64(entry.first.second);
      incidence.u64(entry.second.size());
      if (entry.second.size() != 2)
        result.closed_two_uses_per_edge = false;
      if (entry.second.size() == 2) {
        const auto &a = entry.second[0];
        const auto &b = entry.second[1];
        if (a.from != b.to || a.to != b.from)
          result.opposite_edge_directions = false;
        if (a.face < adjacency.size() && b.face < adjacency.size()) {
          adjacency[a.face].push_back(b.face);
          adjacency[b.face].push_back(a.face);
        }
      }
      for (const auto &use : entry.second) {
        incidence.u64(use.face);
        incidence.u64(use.from);
        incidence.u64(use.to);
      }
    }
    result.edges = uses.size();
    result.incidence_digest =
        domain_digest({{'Y', 'G', 'B', 'Q', 'T', 'I', '0', '1'}},
                      incidence.bytes());

    std::vector<bool> seen(mesh.faces.size(), false);
    for (std::size_t start = 0; start != seen.size(); ++start) {
      if (seen[start])
        continue;
      ++result.connected_components;
      std::vector<std::uint64_t> pending{static_cast<std::uint64_t>(start)};
      seen[start] = true;
      while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        for (const auto next : adjacency[current])
          if (!seen[next]) {
            seen[next] = true;
            pending.push_back(next);
          }
      }
    }
    if (result.vertices > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        result.edges > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        result.faces > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
      return qualification_accounting_detail::accounting_error(
          product_error_code::resource_limit,
          "qualification_accounting.topology_count_range");
    const auto vertices = static_cast<std::int64_t>(result.vertices);
    const auto edges = static_cast<std::int64_t>(result.edges);
    const auto faces = static_cast<std::int64_t>(result.faces);
    result.euler_characteristic = vertices - edges + faces;
  } catch (const std::bad_alloc &) {
    return qualification_accounting_detail::accounting_error(
        product_error_code::resource_limit,
        "qualification_accounting.topology_allocation");
  }
  return qualification_accounting_detail::canonicalize_topology_reconstruction(
      std::move(result));
}

template <class T, class I>
using qualification_chain_mesh_consumer = std::function<product_status_or<bool>(
    const fv_surface_mesh<T, I> &)>;

template <class T, class I>
product_status_or<qualification_success_verification>
observe_qualification_product_success(
    const boolean_product_result<T, I> &result,
    std::shared_ptr<const exact_kernel_services<T>> kernel,
    std::shared_ptr<const verifier_service> verifiers,
    std::vector<qualification_guarded_probe_observation> probes = {},
    bool require_chain_reingestion = false,
    qualification_chain_mesh_consumer<T, I> chain_consumer = {}) {
  qualification_success_verification observation;
  observation.representation = result.representation;
  observation.chain_reingestion_required = require_chain_reingestion;
  observation.mesh_published = static_cast<bool>(result.mesh);
  if (result.exact_result.valid())
    observation.exact_result_digest = result.exact_result->canonical_digest;
  if (result.mesh)
    observation.output_digest = result.mesh->output_semantic_digest;
  else if (result.exact_result.valid())
    observation.output_digest = result.exact_result->canonical_digest;
  observation.probes = std::move(probes);

  auto append_check = [&](qualification_check_kind kind,
                          qualification_check_state state,
                          const char *key, const digest &basis)
      -> product_status_or<bool> {
    qualification_verification_check check;
    check.kind = kind;
    check.state = state;
    check.message_key = key;
    check.evidence_digest =
        qualification_accounting_detail::mesh_observation_digest(basis, key);
    auto made = make_qualification_verification_check(std::move(check));
    if (!made.has_value())
      return made.error();
    observation.checks.push_back(std::move(made.value()));
    return true;
  };

  const auto product_validation = validate_product_result(result);
  auto appended = append_check(
      qualification_check_kind::product_contract,
      product_validation.has_value() ? qualification_check_state::passed
                                     : qualification_check_state::failed,
      product_validation.has_value()
          ? "qualification_accounting.product_contract_passed"
          : "qualification_accounting.product_contract_failed",
      observation.output_digest);
  if (!appended.has_value())
    return appended.error();

  bool exact_binding = result.exact_result.valid() &&
                       observation.exact_result_digest != digest{};
  if (result.mesh)
    exact_binding = exact_binding &&
                    result.mesh->exact_result_digest ==
                        observation.exact_result_digest &&
                    result.mesh->certificate.exact_result_digest ==
                        observation.exact_result_digest;
  appended = append_check(
      qualification_check_kind::exact_result_binding,
      exact_binding ? qualification_check_state::passed
                    : qualification_check_state::failed,
      exact_binding ? "qualification_accounting.exact_binding_passed"
                    : "qualification_accounting.exact_binding_failed",
      observation.exact_result_digest);
  if (!appended.has_value())
    return appended.error();

  bool representation_ok = false;
  if (result.representation == result_representation::exact_stratified)
    representation_ok = !result.mesh;
  else if (result.mesh)
    representation_ok =
        (result.representation == result_representation::exact_in_T_mesh &&
         result.mesh->semantics == product_realization_semantics::exact_in_T) ||
        (result.representation ==
             result_representation::certified_approximate_mesh &&
         result.mesh->semantics ==
             product_realization_semantics::certified_approximate_embedding_v1);
  appended = append_check(
      qualification_check_kind::representation_semantics,
      representation_ok ? qualification_check_state::passed
                        : qualification_check_state::failed,
      representation_ok ? "qualification_accounting.representation_passed"
                        : "qualification_accounting.representation_failed",
      observation.output_digest);
  if (!appended.has_value())
    return appended.error();

  bool attributes_ok = false;
  if (result.exact_result.valid()) {
    const attribute_output_binding *binding =
        result.mesh ? &result.mesh->attribute_binding : nullptr;
    attributes_ok =
        verify_attribute_transfer_report(result.attributes, result.exact_result,
                                         binding)
            .has_value();
  }
  appended = append_check(
      qualification_check_kind::attribute_mapping,
      attributes_ok ? qualification_check_state::passed
                    : qualification_check_state::failed,
      attributes_ok ? "qualification_accounting.attributes_passed"
                    : "qualification_accounting.attributes_failed",
      observation.output_digest);
  if (!appended.has_value())
    return appended.error();

  if (!result.mesh)
    return make_qualification_success_verification(std::move(observation));

  const auto &payload = *result.mesh;
  if (!payload.success)
    return qualification_accounting_detail::accounting_error(
        product_error_code::verifier_disagreement,
        "qualification_accounting.mesh_payload_missing");
  auto topology = reconstruct_qualification_mesh_topology(payload.success->mesh);
  if (!topology.has_value())
    return topology.error();
  observation.topology = std::move(topology.value());
  appended = append_check(
      qualification_check_kind::independent_topology,
      observation.topology.passed() ? qualification_check_state::passed
                                    : qualification_check_state::failed,
      observation.topology.passed()
          ? "qualification_accounting.topology_passed"
          : "qualification_accounting.topology_failed",
      observation.topology.reconstruction_digest);
  if (!appended.has_value())
    return appended.error();

  qualification_check_state strict_state = qualification_check_state::not_run;
  digest strict_basis = observation.topology.reconstruction_digest;
  if (kernel && verifiers) {
    auto prepared = validate_operand_strict(
        payload.success->mesh, strict_validation_policy{}, boolean_options{},
        std::move(kernel), std::move(verifiers));
    strict_state = prepared.has_value() ? qualification_check_state::passed
                                        : qualification_check_state::failed;
    if (prepared.has_value())
      strict_basis = prepared.value().certificate().certificate_digest;
  }
  appended = append_check(
      qualification_check_kind::strict_reingestion, strict_state,
      strict_state == qualification_check_state::passed
          ? "qualification_accounting.strict_reingestion_passed"
          : strict_state == qualification_check_state::failed
                ? "qualification_accounting.strict_reingestion_failed"
                : "qualification_accounting.strict_reingestion_not_run",
      strict_basis);
  if (!appended.has_value())
    return appended.error();
  for (const auto kind : {qualification_check_kind::embedding,
                          qualification_check_kind::orientation,
                          qualification_check_kind::shell_nesting}) {
    appended = append_check(
        kind, strict_state,
        strict_state == qualification_check_state::passed
            ? "qualification_accounting.strict_geometry_passed"
            : strict_state == qualification_check_state::failed
                  ? "qualification_accounting.strict_geometry_failed"
                  : "qualification_accounting.strict_geometry_not_run",
        strict_basis);
    if (!appended.has_value())
      return appended.error();
  }

  bool certificate_ok = false;
  bool approximation_ok =
      result.representation != result_representation::certified_approximate_mesh;
  if (result.representation == result_representation::exact_in_T_mesh) {
    certificate_ok =
        payload.certificate.certificate_digest ==
            strict_mesh_certificate_digest(payload) &&
        payload.output_semantic_digest ==
            payload.success->canonical_output_digest &&
        payload.output_semantic_digest == payload.success->summary.semantic_digest;
    if (certificate_ok) {
      for (std::size_t i = 0; i != payload.strict_vertices.size(); ++i) {
        if (i >= payload.success->mesh.vertices.size()) {
          certificate_ok = false;
          break;
        }
        const auto &point = payload.success->mesh.vertices[i];
        const std::array<T, 3> values{{point.x, point.y, point.z}};
        std::array<std::uint64_t, 3> bits{{0, 0, 0}};
        for (std::size_t axis = 0; axis != 3; ++axis)
          std::memcpy(&bits[axis], &values[axis], sizeof(T));
        if (payload.strict_vertices[i].output_vertex != i ||
            payload.strict_vertices[i].accepted_bits != bits) {
          certificate_ok = false;
          break;
        }
      }
    }
  } else if (result.representation ==
             result_representation::certified_approximate_mesh) {
    if (payload.approximate_certificate && result.exact_result.valid()) {
      const auto replay = verify_certified_approximate_embedding(
          result.exact_result, payload.policy, *payload.success,
          *payload.approximate_certificate);
      certificate_ok = replay.has_value();
      approximation_ok = replay.has_value();
    }
  }
  appended = append_check(
      qualification_check_kind::certificate_replay,
      certificate_ok ? qualification_check_state::passed
                     : qualification_check_state::failed,
      certificate_ok ? "qualification_accounting.certificate_passed"
                     : "qualification_accounting.certificate_failed",
      payload.certificate.certificate_digest);
  if (!appended.has_value())
    return appended.error();
  if (result.representation ==
      result_representation::certified_approximate_mesh) {
    appended = append_check(
        qualification_check_kind::approximation_bounds,
        approximation_ok ? qualification_check_state::passed
                         : qualification_check_state::failed,
        approximation_ok ? "qualification_accounting.approximation_passed"
                         : "qualification_accounting.approximation_failed",
        payload.certificate.certificate_digest);
    if (!appended.has_value())
      return appended.error();
  }

  qualification_check_state probe_state = qualification_check_state::not_run;
  if (!observation.probes.empty()) {
    probe_state = std::all_of(
                      observation.probes.begin(), observation.probes.end(),
                      [](const auto &probe) {
                        return !probe.observed_on_boundary &&
                               probe.expected_occupied == probe.observed_occupied;
                      })
                      ? qualification_check_state::passed
                      : qualification_check_state::failed;
  }
  appended = append_check(
      qualification_check_kind::guarded_occupancy, probe_state,
      probe_state == qualification_check_state::passed
          ? "qualification_accounting.probes_passed"
          : probe_state == qualification_check_state::failed
                ? "qualification_accounting.probes_failed"
                : "qualification_accounting.probes_not_run",
      observation.output_digest);
  if (!appended.has_value())
    return appended.error();

  if (require_chain_reingestion) {
    qualification_check_state chain_state = qualification_check_state::not_run;
    if (chain_consumer) {
      const auto before = qualification_accounting_detail::mesh_observation_digest(
          observation.output_digest, "qualification_accounting.chain_before");
      auto accepted = chain_consumer(payload.success->mesh);
      chain_state = accepted.has_value() && accepted.value()
                        ? qualification_check_state::passed
                        : qualification_check_state::failed;
      appended = append_check(
          qualification_check_kind::chain_reingestion, chain_state,
          chain_state == qualification_check_state::passed
              ? "qualification_accounting.chain_reingestion_passed"
              : "qualification_accounting.chain_reingestion_failed",
          before);
    } else {
      appended = append_check(
          qualification_check_kind::chain_reingestion, chain_state,
          "qualification_accounting.chain_reingestion_not_run",
          observation.output_digest);
    }
    if (!appended.has_value())
      return appended.error();
  }
  return make_qualification_success_verification(std::move(observation));
}

} // namespace mesh_boolean
} // namespace ygor

#endif
