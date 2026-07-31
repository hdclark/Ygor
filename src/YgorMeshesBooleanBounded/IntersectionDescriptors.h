#pragma once

#include "IntersectionAggregation.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct intersection_descriptor_tables final {
  std::vector<intersection_descriptor_record> records{};
  std::vector<event_incidence_id> provenance{};
};

// Implements the versioned Plan 08 Section 16.3 derivation table for the
// topology loci that are already canonicalized by the source-edge,
// transverse-carrier, coplanar-carrier, occurrence, and aggregate providers.
// Descriptor identity and provenance are lineage based; coordinates and
// aggregate cancellation are never used as topology identity.
bool build_intersection_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    intersection_descriptor_tables &tables, bounded_boolean_error &error);

// Independently reconstructs every expected descriptor and exact provenance
// range. It does not trust descriptor IDs, sorted flags, or producer ranges.
bool verify_intersection_descriptors(
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &tables,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
