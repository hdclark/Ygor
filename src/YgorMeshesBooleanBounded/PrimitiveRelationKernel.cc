#include "PrimitiveRelationKernel.h"

namespace ygor::mesh_boolean::bounded {

bounded_boolean_error primitive_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint) {
  return relation_error(subcode,
                        bounded_boolean_error_category::internal_invariant_error,
                        summary, checkpoint);
}

bool valid_relation_truth_record(const relation_truth_record &record) noexcept {
  if (record.reserved != 0 ||
      !registered_rounded_operation(
          static_cast<rounded_operation_code>(record.rounded_formula)) ||
      !valid_exact_formula_code(record.exact_formula))
    return false;

  switch (record.bounded_sign) {
  case bounded_sign_status::definitely_negative:
  case bounded_sign_status::overlaps_boundary:
  case bounded_sign_status::definitely_positive:
    break;
  case bounded_sign_status::invalid:
    return false;
  default:
    return false;
  }

  switch (record.exact_relation) {
  case exact_relation_status::exact_negative:
  case exact_relation_status::exact_zero:
  case exact_relation_status::exact_positive:
  case exact_relation_status::unavailable:
    break;
  case exact_relation_status::invalid:
    return false;
  default:
    return false;
  }

  const bool exact_formula_requested =
      record.exact_formula !=
      static_cast<std::uint16_t>(exact_relation_formula_code::invalid);
  if ((exact_formula_requested &&
       record.exact_relation == exact_relation_status::unavailable) ||
      (!exact_formula_requested &&
       record.exact_relation != exact_relation_status::unavailable))
    return false;

  if ((record.bounded_sign == bounded_sign_status::definitely_negative &&
       (record.exact_relation == exact_relation_status::exact_zero ||
        record.exact_relation == exact_relation_status::exact_positive)) ||
      (record.bounded_sign == bounded_sign_status::definitely_positive &&
       (record.exact_relation == exact_relation_status::exact_zero ||
        record.exact_relation == exact_relation_status::exact_negative)))
    return false;

  switch (record.disposition) {
  case predicate_disposition::accept_numeric_sign:
    return record.bounded_sign == bounded_sign_status::definitely_negative ||
           record.bounded_sign == bounded_sign_status::definitely_positive;
  case predicate_disposition::retain_tie_for_consumer_eligibility:
  case predicate_disposition::route_coplanar_or_coincident:
    return record.bounded_sign == bounded_sign_status::overlaps_boundary &&
           record.exact_relation == exact_relation_status::exact_zero;
  case predicate_disposition::try_permitted_alternate:
  case predicate_disposition::fail_condition_or_tolerance:
    return record.bounded_sign == bounded_sign_status::overlaps_boundary &&
           record.exact_relation != exact_relation_status::exact_zero;
  case predicate_disposition::fail_invalid:
    return false;
  default:
    return false;
  }
  return false;
}

} // namespace ygor::mesh_boolean::bounded
