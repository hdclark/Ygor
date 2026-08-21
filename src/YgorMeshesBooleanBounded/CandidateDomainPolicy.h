#pragma once

#include "BroadPhaseTypes.h"

namespace ygor::mesh_boolean::bounded {

struct candidate_domain_decision final {
  candidate_domain_disposition disposition = candidate_domain_disposition::included;
  topological_filter_reason reason = topological_filter_reason::not_filtered;
};

inline bool valid_directed_candidate_role(directed_candidate_role role) noexcept {
  return role == directed_candidate_role::a_edge_b_triangle ||
         role == directed_candidate_role::b_edge_a_triangle;
}

inline candidate_domain_decision candidate_domain_v1(
    directed_candidate_role role, canonical_edge_class edge_class) noexcept {
  if (!valid_directed_candidate_role(role) ||
      (edge_class != canonical_edge_class::source_edge &&
       edge_class != canonical_edge_class::facet_internal_diagonal)) {
    return {candidate_domain_disposition::excluded,
            topological_filter_reason::policy_excluded_internal_diagonal};
  }
  return {candidate_domain_disposition::included,
          topological_filter_reason::not_filtered};
}

inline bool candidate_role_matches_operands(directed_candidate_role role,
                                            operand_id edge_operand,
                                            operand_id triangle_operand) noexcept {
  return valid_directed_candidate_role(role) &&
         role_edge_operand(role) == edge_operand &&
         role_triangle_operand(role) == triangle_operand;
}

} // namespace ygor::mesh_boolean::bounded
