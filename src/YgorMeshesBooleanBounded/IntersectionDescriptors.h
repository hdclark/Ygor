#pragma once

#include "CanonicalSourceManifolds.h"
#include "IntersectionAggregation.h"

#include <cstdint>
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

// Extends an already verified descriptor table with the source-topology loci
// required by Plan 08 Sections 16.4 and 17: cyclic source-vertex fan sectors,
// original source-edge facet adjacency, and transparent internal-diagonal
// adjacency. Component 05 supplies the only topology order; Component 07
// supplies the only local crossing transitions. No coordinate ordering or
// reconstructed geometry is permitted.
template <class T, class I>
bool extend_intersection_descriptors_with_source_topology(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_descriptor_tables &base,
    intersection_descriptor_tables &tables,
    bounded_boolean_error &error);

// Independently reconstructs the cyclic fan sectors and source-facet
// adjacency descriptors. The verifier does not trust producer descriptor IDs,
// fan-group ordinals, or stored source-edge continuation flags.
template <class T, class I>
bool verify_intersection_source_topology_descriptors(
    const canonical_source_manifolds<T, I> &manifolds,
    const std::vector<relation_crossing_record> &crossings,
    const std::vector<relation_event_seed_record> &seeds,
    const event_interning_tables &interning,
    const event_incidence_tables &incidence,
    const intersection_descriptor_tables &base,
    const intersection_descriptor_tables &tables,
    bounded_boolean_error &error);

extern template bool extend_intersection_descriptors_with_source_topology<float, std::uint32_t>(
    const canonical_source_manifolds<float, std::uint32_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, intersection_descriptor_tables &,
    bounded_boolean_error &);
extern template bool extend_intersection_descriptors_with_source_topology<float, std::uint64_t>(
    const canonical_source_manifolds<float, std::uint64_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, intersection_descriptor_tables &,
    bounded_boolean_error &);
extern template bool extend_intersection_descriptors_with_source_topology<double, std::uint32_t>(
    const canonical_source_manifolds<double, std::uint32_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, intersection_descriptor_tables &,
    bounded_boolean_error &);
extern template bool extend_intersection_descriptors_with_source_topology<double, std::uint64_t>(
    const canonical_source_manifolds<double, std::uint64_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, intersection_descriptor_tables &,
    bounded_boolean_error &);

extern template bool verify_intersection_source_topology_descriptors<float, std::uint32_t>(
    const canonical_source_manifolds<float, std::uint32_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, const intersection_descriptor_tables &,
    bounded_boolean_error &);
extern template bool verify_intersection_source_topology_descriptors<float, std::uint64_t>(
    const canonical_source_manifolds<float, std::uint64_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, const intersection_descriptor_tables &,
    bounded_boolean_error &);
extern template bool verify_intersection_source_topology_descriptors<double, std::uint32_t>(
    const canonical_source_manifolds<double, std::uint32_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, const intersection_descriptor_tables &,
    bounded_boolean_error &);
extern template bool verify_intersection_source_topology_descriptors<double, std::uint64_t>(
    const canonical_source_manifolds<double, std::uint64_t> &,
    const std::vector<relation_crossing_record> &,
    const std::vector<relation_event_seed_record> &,
    const event_interning_tables &, const event_incidence_tables &,
    const intersection_descriptor_tables &, const intersection_descriptor_tables &,
    bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
