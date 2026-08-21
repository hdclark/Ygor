#pragma once

#include "CoplanarCarrierArrangements.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct intersection_aggregate_tables final {
  std::vector<crossing_aggregate_record> crossing{};
  std::vector<event_incidence_id> crossing_members{};
  std::vector<crossing_subtotal_record> facet_subtotals{};
  std::vector<event_incidence_id> facet_subtotal_members{};
  std::vector<crossing_subtotal_record> shell_subtotals{};
  std::vector<event_incidence_id> shell_subtotal_members{};
  std::vector<contact_aggregate_record> contact{};
  std::vector<event_incidence_id> contact_members{};
};

// Reconstructs immutable member-preserving crossing/contact aggregates from
// canonical Component 07 seed contributions and Component 08 incidence and
// arrangement records. No aggregate member is synthesized from coordinates,
// nominal equality, or aggregate cancellation.
bool build_intersection_aggregates(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    intersection_aggregate_tables &tables, bounded_boolean_error &error);

// Independently validates every expected locus, member set, checked sum,
// source-facet/shell subtotal, symbolic-owner state, and zero-net contact
// retention. The verifier does not invoke the producer.
bool verify_intersection_aggregates(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &tables, bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
