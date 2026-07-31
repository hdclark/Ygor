#pragma once

#include "CanonicalIntersectionComplex.h"
#include "FiniteInterval.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// A topology-neutral input record for the Component 08 bounded ordering
// provider.  Identity and semantic tie order are lineage keyed; parameter bits
// are carried only for the non-semantic interval sweep and certified bounded
// comparisons.
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
  // Member ordinals are published cluster-major and occurrence-tie-key minor.
  std::vector<std::uint64_t> ordered_member_ordinals{};
  std::vector<bounded_ordering_cluster> clusters{};
  std::vector<ordering_certificate_record> certificates{};
  std::uint64_t comparison_count = 0;
  std::uint64_t active_pair_count = 0;
};

// Implements Plan 08 Section 12.  The provider never uses ordinary nominal
// comparison as semantic order: endpoint bits are used only to enumerate the
// interval sweep, every relevant pair receives a certificate, cluster
// components are checked pairwise, and the final cluster order is derived from
// certified precedence.
template <class T>
bool build_bounded_carrier_order(
    const std::vector<bounded_ordering_member> &members,
    bounded_ordering_result &result, bounded_boolean_error &error);

// Independently reconstructs the order and requires exact record equality.
// This is intentionally separate from producer invariant checks so later
// Component 08 verification cannot trust stored ordinals or a sorted flag.
template <class T>
bool verify_bounded_carrier_order(
    const std::vector<bounded_ordering_member> &members,
    const bounded_ordering_result &result, bounded_boolean_error &error);

extern template bool build_bounded_carrier_order<float>(
    const std::vector<bounded_ordering_member> &, bounded_ordering_result &,
    bounded_boolean_error &);
extern template bool build_bounded_carrier_order<double>(
    const std::vector<bounded_ordering_member> &, bounded_ordering_result &,
    bounded_boolean_error &);
extern template bool verify_bounded_carrier_order<float>(
    const std::vector<bounded_ordering_member> &,
    const bounded_ordering_result &, bounded_boolean_error &);
extern template bool verify_bounded_carrier_order<double>(
    const std::vector<bounded_ordering_member> &,
    const bounded_ordering_result &, bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
