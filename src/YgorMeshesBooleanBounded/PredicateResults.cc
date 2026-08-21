#include "PredicateResults.h"

namespace ygor::mesh_boolean::bounded {

predicate_disposition assemble_predicate_disposition(bounded_sign_status bounded,
                                                      exact_relation_status exact,
                                                      bool alternate_available) noexcept {
    if (bounded == bounded_sign_status::invalid || exact == exact_relation_status::invalid)
        return predicate_disposition::fail_invalid;
    if (bounded == bounded_sign_status::definitely_negative) {
        if (exact == exact_relation_status::exact_positive || exact == exact_relation_status::exact_zero)
            return predicate_disposition::fail_invalid;
        return predicate_disposition::accept_numeric_sign;
    }
    if (bounded == bounded_sign_status::definitely_positive) {
        if (exact == exact_relation_status::exact_negative || exact == exact_relation_status::exact_zero)
            return predicate_disposition::fail_invalid;
        return predicate_disposition::accept_numeric_sign;
    }
    if (exact == exact_relation_status::exact_zero)
        return predicate_disposition::retain_tie_for_consumer_eligibility;
    if (alternate_available) return predicate_disposition::try_permitted_alternate;
    return predicate_disposition::fail_condition_or_tolerance;
}

legacy_predicate_class legacy_classification(bounded_sign_status bounded,
                                             exact_relation_status exact) noexcept {
    if (bounded == bounded_sign_status::invalid || exact == exact_relation_status::invalid)
        return legacy_predicate_class::invalid;
    if (bounded == bounded_sign_status::definitely_negative)
        return legacy_predicate_class::definitely_negative;
    if (bounded == bounded_sign_status::definitely_positive)
        return legacy_predicate_class::definitely_positive;
    if (exact == exact_relation_status::exact_zero)
        return legacy_predicate_class::exact_tie_evidence;
    return legacy_predicate_class::uncertain;
}

} // namespace ygor::mesh_boolean::bounded
