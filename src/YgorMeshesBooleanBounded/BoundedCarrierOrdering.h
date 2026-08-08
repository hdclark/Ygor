#pragma once

#include "CanonicalIntersectionComplex.h"
#include "FiniteInterval.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Topology-neutral input for the shared Component 08 ordering provider.
// Parameter bits are non-semantic sweep/comparison evidence only; identity and
// tie order are supplied by immutable occurrence lineage.
struct bounded_ordering_member final {
  std::uint64_t input_ordinal = 0;
  relation_interval_evidence_id parameter{intersection_invalid_ordinal};
  intersection_occurrence_key occurrence{};
  std::uint64_t nominal_bits = 0;
  std::uint64_t lower_bits = 0;
  std::uint64_t upper_bits = 0;
  std::uint64_t exact_evidence_lineage = 0;
  std::uint64_t comparison_evidence_lineage = 0;
  std::uint64_t cluster_lineage = 0;
  bool exact_equal_eligible = false;
  bool unresolved_cluster_eligible = false;
  bool topology_interchangeable = false;
  std::uint8_t reserved8 = 0;
};

struct bounded_ordering_pair_certificate final {
  std::uint64_t first_input_ordinal = 0;
  std::uint64_t second_input_ordinal = 0;
  ordering_certificate_id certificate{intersection_invalid_ordinal};
};

struct bounded_ordering_cluster final {
  intersection_cluster_equivalence equivalence =
      intersection_cluster_equivalence::exact_parameter_coincidence;
  intersection_range members{};
  std::uint64_t canonical_key_ordinal = 0;
  bool topology_interchangeable = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct bounded_ordering_result final {
  // Cluster-major and occurrence-tie-key-minor input ordinals.
  std::vector<std::uint64_t> ordered_member_ordinals{};
  std::vector<bounded_ordering_cluster> clusters{};
  std::vector<ordering_certificate_record> certificates{};
  std::vector<bounded_ordering_pair_certificate> pair_certificates{};
  std::uint64_t comparison_count = 0;
  std::uint64_t active_pair_count = 0;
};

// Implements Plan 08 Section 12.  Endpoint bits establish only a private sweep
// order. Every topology-relevant pair receives a bounded comparison
// certificate, clusters are verified as all-pairs cliques, and final cluster
// order is derived from certified precedence.
template <class T>
bool build_bounded_carrier_order(
    const std::vector<bounded_ordering_member> &members,
    intersection_checkpoint checkpoint, bounded_ordering_result &result,
    bounded_boolean_error &error);

// Independently reconstructs the complete provider result and requires exact
// equality, so verification cannot trust stored ordinals or a sorted flag.
template <class T>
bool verify_bounded_carrier_order(
    const std::vector<bounded_ordering_member> &members,
    intersection_checkpoint checkpoint, const bounded_ordering_result &result,
    bounded_boolean_error &error);

extern template bool build_bounded_carrier_order<float>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    bounded_ordering_result &, bounded_boolean_error &);
extern template bool build_bounded_carrier_order<double>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    bounded_ordering_result &, bounded_boolean_error &);
extern template bool verify_bounded_carrier_order<float>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    const bounded_ordering_result &, bounded_boolean_error &);
extern template bool verify_bounded_carrier_order<double>(
    const std::vector<bounded_ordering_member> &, intersection_checkpoint,
    const bounded_ordering_result &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
