#pragma once

#include "MeshBooleanPerformanceFixtures.h"

#include <cstdint>
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::testing {

struct performance_counters {
  std::uint64_t input_vertices = 0, input_facets = 0, input_shells = 0;
  std::uint64_t broad_node_pairs = 0, broad_facet_pairs = 0,
                broad_final_candidates = 0;
  std::uint64_t event_candidates = 0, event_points = 0, event_intervals = 0,
                event_regions = 0, event_carriers = 0;
  std::uint64_t symbolic_vertices = 0, symbolic_curves = 0;
  std::uint64_t local_facets = 0, local_shared_edges = 0, local_patches = 0;
  std::uint64_t global_vertices = 0, global_edges = 0, global_patches = 0,
                global_halfedges = 0;
  std::uint64_t classification_regions = 0, classification_labels = 0,
                classification_probes = 0;
  std::uint64_t selected_patches = 0, selected_edges = 0,
                selected_components = 0;
  std::uint64_t realization_vertices = 0, realization_obligations = 0,
                realization_pair_candidates = 0,
                realization_pair_checks = 0, realization_components = 0;
  std::uint64_t output_vertices = 0, output_faces = 0, output_components = 0;
  std::uint64_t authoritative_bytes = 0, stage_private_bytes = 0,
                verifier_work = 0, verifier_scratch_bytes = 0,
                output_canonical_bytes = 0;

  bool operator==(const performance_counters &o) const {
    return input_vertices == o.input_vertices && input_facets == o.input_facets &&
           input_shells == o.input_shells && broad_node_pairs == o.broad_node_pairs &&
           broad_facet_pairs == o.broad_facet_pairs &&
           broad_final_candidates == o.broad_final_candidates &&
           event_candidates == o.event_candidates && event_points == o.event_points &&
           event_intervals == o.event_intervals && event_regions == o.event_regions &&
           event_carriers == o.event_carriers &&
           symbolic_vertices == o.symbolic_vertices && symbolic_curves == o.symbolic_curves &&
           local_facets == o.local_facets && local_shared_edges == o.local_shared_edges &&
           local_patches == o.local_patches && global_vertices == o.global_vertices &&
           global_edges == o.global_edges && global_patches == o.global_patches &&
           global_halfedges == o.global_halfedges &&
           classification_regions == o.classification_regions &&
           classification_labels == o.classification_labels &&
           classification_probes == o.classification_probes &&
           selected_patches == o.selected_patches && selected_edges == o.selected_edges &&
           selected_components == o.selected_components &&
           realization_vertices == o.realization_vertices &&
           realization_obligations == o.realization_obligations &&
           realization_pair_candidates == o.realization_pair_candidates &&
           realization_pair_checks == o.realization_pair_checks &&
           realization_components == o.realization_components &&
           output_vertices == o.output_vertices && output_faces == o.output_faces &&
           output_components == o.output_components &&
           authoritative_bytes == o.authoritative_bytes &&
           stage_private_bytes == o.stage_private_bytes &&
           verifier_work == o.verifier_work &&
           verifier_scratch_bytes == o.verifier_scratch_bytes &&
           output_canonical_bytes == o.output_canonical_bytes;
  }
};

struct performance_observation {
  bool success = false;
  boolean_error_code error_code = boolean_error_code::internal_invariant_error;
  boolean_stage error_stage = boolean_stage::context_setup;
  std::uint32_t error_subcode = 0;
  std::string error_key;
  digest input_a_digest, input_b_digest, setup_digest, output_identity;
  std::vector<std::pair<std::string, digest>> semantic_digests;
  std::vector<std::uint8_t> canonical_output_bytes;
  performance_counters counters;
  bool snapshot_collected = false;
  std::uint64_t producer_nanoseconds = 0, verifier_nanoseconds = 0;
  std::array<std::uint64_t,
             static_cast<std::size_t>(boolean_stage::final_verification) + 1>
      stage_producer_nanoseconds{{}}, stage_verifier_nanoseconds{{}};
  performance_counter_snapshot producer_counters, verifier_counters;

  std::string typed_outcome() const {
    if (success)
      return "success";
    return "error:" + std::to_string(static_cast<unsigned>(error_code)) + ":" +
           std::to_string(static_cast<unsigned>(error_stage)) + ":" +
           std::to_string(error_subcode) + ":" + error_key;
  }
};

template <class Artifact, class T, class I>
std::shared_ptr<const published_artifact<Artifact>> latest_artifact(
    const boolean_context<T, I> &context, artifact_slot slot) {
  return std::static_pointer_cast<const published_artifact<Artifact>>(
      context.artifacts().latest(slot));
}

template <class T, class I>
performance_observation observe_performance_fixture(const std::string &name,
                                                    std::uint32_t size,
                                                    operation op,
                                                    boolean_options options) {
  auto fixture = make_performance_fixture<T, I>(name, size);
  performance_observation observed;
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifiers = output_test::registry();
  auto made = make_boolean_context(fixture.a, fixture.b, op, options, kernel,
                                   verifiers);
  if (!made.has_value()) {
    observed.error_code = made.error().code;
    observed.error_stage = made.error().stage;
    observed.error_subcode = made.error().subcode;
    observed.error_key = made.error().message_key;
    observed.setup_digest = made.error().replay.setup_digest;
    return observed;
  }

  auto context = std::move(made.value());
  observed.input_a_digest = context->replay().input_a;
  observed.input_b_digest = context->replay().input_b;
  observed.setup_digest = context->replay().setup;
  auto result = assemble_boolean_output_artifact(*context);
  std::shared_ptr<const published_artifact<assembled_output<T, I>>> output;
  if (!result.has_value()) {
    observed.error_code = result.error().code;
    observed.error_stage = result.error().stage;
    observed.error_subcode = result.error().subcode;
    observed.error_key = result.error().message_key;
  } else {
    observed.success = true;
    output = result.value();
    observed.output_identity =
        result.value()->payload->public_result->canonical_output_digest;
    observed.canonical_output_bytes = result.value()->payload->canonical_bytes;
  }

  if (!output)
    output = latest_artifact<assembled_output<T, I>>(*context,
                                                     artifact_slot::assembled_output);
  auto realized = output ? output->payload->realized
                         : latest_artifact<realized_boundary<T, I>>(
                               *context, artifact_slot::realized_boundary);
  auto selected = realized ? realized->payload->selected
                           : latest_artifact<selected_exact_boundary<T, I>>(
                                 *context, artifact_slot::selected_exact_boundary);
  auto labeled = selected ? selected->payload->labeled
                          : latest_artifact<labeled_arrangement<T, I>>(
                                *context, artifact_slot::labeled_arrangement);
  auto arrangement = labeled ? labeled->payload->arrangement
                             : latest_artifact<arrangement_complex<T, I>>(
                                   *context, artifact_slot::arrangement_complex);
  auto local = arrangement ? arrangement->payload->refined
                           : latest_artifact<refined_facet_patches<T, I>>(
                                 *context, artifact_slot::refined_facet_patches);
  auto symbolic = local ? local->payload->symbolic
                        : latest_artifact<symbolic_complex<T, I>>(
                              *context, artifact_slot::symbolic_complex);
  auto events = symbolic ? symbolic->payload->raw_events
                         : latest_artifact<raw_event_set<T, I>>(
                               *context, artifact_slot::raw_event_set);
  auto broad = events ? events->payload->candidates
                      : latest_artifact<candidate_stream<T, I>>(
                            *context, artifact_slot::candidate_stream);
  auto validated = broad ? broad->payload->validated
                         : arrangement ? arrangement->payload->validated
                                       : latest_artifact<validated_operands<T, I>>(
                                             *context, artifact_slot::validated_operands);
  if (validated) {
    const auto &p = *validated->payload;
    observed.counters.input_vertices = p.vertices.size();
    observed.counters.input_facets = p.facets.size();
    observed.counters.input_shells = p.shells.size();
    observed.semantic_digests.push_back({"input_a", p.operands[0].semantic_digest});
    observed.semantic_digests.push_back({"input_b", p.operands[1].semantic_digest});
  }
  if (broad) {
    const auto &p = *broad->payload;
    observed.counters.broad_node_pairs = p.implementation_statistics.node_pair_tests;
    observed.counters.broad_facet_pairs = p.implementation_statistics.facet_pair_tests;
    observed.counters.broad_final_candidates = p.statistics.final_candidates;
  }
  if (events) {
    const auto &p = *events->payload;
    observed.counters.event_candidates = p.classifications.size();
    observed.counters.event_points = p.points.size();
    observed.counters.event_intervals = p.intervals.size();
    observed.counters.event_regions = p.regions.size();
    observed.counters.event_carriers = p.carriers.size();
  }
  if (symbolic) {
    observed.counters.symbolic_vertices = symbolic->payload->vertices.size();
    observed.counters.symbolic_curves = symbolic->payload->curves.size();
  }
  if (local) {
    observed.counters.local_facets = local->payload->facets.size();
    observed.counters.local_shared_edges = local->payload->shared_edges.size();
    for (const auto &facet : local->payload->facets)
      observed.counters.local_patches += facet.patches.size();
  }
  if (arrangement) {
    const auto &p = *arrangement->payload;
    observed.counters.global_vertices = p.vertices.size();
    observed.counters.global_edges = p.edges.size();
    observed.counters.global_patches = p.patches.size();
    observed.counters.global_halfedges = p.halfedges.size();
    observed.semantic_digests.push_back({"arrangement", p.certificate.semantic_digest});
  }
  if (labeled) {
    const auto &p = *labeled->payload;
    observed.counters.classification_regions = p.regions.size();
    observed.counters.classification_labels = p.side_labels.size();
    observed.counters.classification_probes = p.arrangement->payload->probes.size();
    observed.semantic_digests.push_back({"classification", p.certificate.semantic_digest});
  }
  if (selected) {
    const auto &p = *selected->payload;
    observed.counters.selected_patches = p.patches.size();
    observed.counters.selected_edges = p.edges.size();
    observed.counters.selected_components = p.certificate.connected_components;
    observed.semantic_digests.push_back({"selection", p.certificate.semantic_digest});
  }
  if (realized) {
    const auto &p = *realized->payload;
    observed.counters.realization_vertices = p.vertices.size();
    observed.counters.realization_obligations = p.obligations.size();
    observed.counters.realization_pair_candidates = p.pair_candidates.size();
    observed.counters.realization_components = p.components.size();
    observed.counters.realization_pair_checks = p.search.pair_checks;
    observed.semantic_digests.push_back({"realization", p.certificate.semantic_digest});
  }
  if (output) {
    const auto &p = *output->payload;
    observed.counters.output_vertices = p.mesh.vertices.size();
    observed.counters.output_faces = p.mesh.faces.size();
    observed.counters.output_components = p.components.size();
    observed.semantic_digests.push_back({"output", p.certificate.semantic_digest});
  }

  auto &accountant = context->accountant();
  observed.counters.authoritative_bytes = accountant.used(resource_kind::authoritative_bytes);
  observed.counters.stage_private_bytes = accountant.used(resource_kind::stage_private_bytes);
  observed.counters.verifier_work = accountant.used(resource_kind::verifier_work);
  observed.counters.verifier_scratch_bytes = accountant.used(resource_kind::verifier_scratch_bytes);
  observed.counters.output_canonical_bytes = accountant.used(resource_kind::output_canonical_bytes);
  const auto snapshot = context->performance();
  observed.snapshot_collected = snapshot->collected;
  for (std::size_t stage_index = 0; stage_index < snapshot->stages.size();
       ++stage_index) {
    const auto &stage = snapshot->stages[stage_index];
    observed.stage_producer_nanoseconds[stage_index] = stage.producer_nanoseconds;
    observed.stage_verifier_nanoseconds[stage_index] = stage.verifier_nanoseconds;
    observed.producer_nanoseconds += stage.producer_nanoseconds;
    observed.verifier_nanoseconds += stage.verifier_nanoseconds;
    for (std::size_t i = 0; i < observed.producer_counters.values.size(); ++i) {
      const auto counter = static_cast<performance_counter>(i);
      if (counter == performance_counter::max_numerator_limbs ||
          counter == performance_counter::max_denominator_limbs) {
        observed.producer_counters.values[i] =
            std::max(observed.producer_counters.values[i], stage.producer.values[i]);
        observed.verifier_counters.values[i] =
            std::max(observed.verifier_counters.values[i], stage.verifier.values[i]);
      } else {
        observed.producer_counters.values[i] += stage.producer.values[i];
        observed.verifier_counters.values[i] += stage.verifier.values[i];
      }
    }
    for (std::size_t i = 0; i < observed.producer_counters.resources.size(); ++i) {
      observed.producer_counters.resources[i] += stage.producer.resources[i];
      observed.verifier_counters.resources[i] += stage.verifier.resources[i];
    }
  }
  return observed;
}

} // namespace ygor::mesh_boolean::testing
