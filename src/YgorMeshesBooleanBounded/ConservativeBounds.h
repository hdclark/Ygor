#pragma once

#include "BoundedOperations.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template<class T>
struct conservative_bound_record final {
    std::uint16_t schema_version = 1;
    bounded_aabb3<T> bound{};
    std::vector<provenance_id> ordered_provenance;
    std::vector<geometric_lineage_id> ordered_lineages;
};

namespace conservative_bounds_detail {
inline bool add_contributor(double a, double b, double &out) noexcept {
    const auto result = directed_add(a, b);
    if (!result) return false;
    out = result.value.upper;
    return true;
}
inline bool combine_contributors(const uncertainty_contributors &a,
                                 const uncertainty_contributors &b,
                                 uncertainty_contributors &out) noexcept {
    return add_contributor(a.inherited_a, b.inherited_a, out.inherited_a) &&
           add_contributor(a.inherited_b, b.inherited_b, out.inherited_b) &&
           add_contributor(a.machine_floor, b.machine_floor, out.machine_floor) &&
           add_contributor(a.construction, b.construction, out.construction) &&
           add_contributor(a.conditioning, b.conditioning, out.conditioning) &&
           add_contributor(a.conversion, b.conversion, out.conversion) &&
           add_contributor(a.prior_cleanup, b.prior_cleanup, out.prior_cleanup) &&
           add_contributor(a.current_cleanup, b.current_cleanup, out.current_cleanup);
}
inline uncertainty_contributors saturated_contributors() noexcept {
    uncertainty_contributors out;
    out.inherited_a = out.inherited_b = out.machine_floor = out.construction =
        out.conditioning = out.conversion = out.prior_cleanup = out.current_cleanup =
            std::numeric_limits<double>::max();
    return out;
}
template<class Id>
void canonical_insert(std::vector<Id> &values, Id value) {
    const auto position = std::lower_bound(values.begin(), values.end(), value);
    if (position == values.end() || *position != value) values.insert(position, value);
}
} // namespace conservative_bounds_detail

template<class T>
bounded_aabb3<T> point_bound(const bounded_point3<T> &point,
                             finite_bound_id id = finite_bound_id(0)) {
    bounded_aabb3<T> out;
    out.owner = point.owner;
    out.id = id; out.provenance = point.provenance; out.lineage = point.lineage;
    for (unsigned axis = 0; axis < 3; ++axis) {
        out.axes[axis] = point.coordinates.components[axis].uncertainty_enclosure;
        uncertainty_contributors combined;
        if (!conservative_bounds_detail::combine_contributors(
                out.inflation, point.coordinates.components[axis].contributors, combined))
            out.inflation = conservative_bounds_detail::saturated_contributors();
        else out.inflation = combined;
    }
    return out;
}

template<class T>
bounded_aabb3<T> union_bounds(const bounded_aabb3<T> &a, const bounded_aabb3<T> &b,
                              finite_bound_id id = finite_bound_id(0)) {
    if (static_cast<bool>(a.owner.anchor) != static_cast<bool>(b.owner.anchor) ||
        (a.owner.anchor && !a.owner.same_owner(b.owner))) return {};
    bounded_aabb3<T> out = a;
    out.id = id;
    for (unsigned axis = 0; axis < 3; ++axis) {
        out.axes[axis] = interval_hull(a.axes[axis], b.axes[axis]);
    }
    uncertainty_contributors combined;
    if (conservative_bounds_detail::combine_contributors(a.inflation, b.inflation, combined))
        out.inflation = combined;
    else out.inflation = conservative_bounds_detail::saturated_contributors();
    if (a.provenance != b.provenance) out.provenance = provenance_id(0);
    if (a.lineage != b.lineage) out.lineage = geometric_lineage_id(0);
    return out;
}

template<class T>
conservative_bound_record<T> point_bound_record(const bounded_point3<T> &point,
                                                 finite_bound_id id = finite_bound_id(0)) {
    conservative_bound_record<T> out;
    out.bound = point_bound(point, id);
    out.ordered_provenance.push_back(point.provenance);
    out.ordered_lineages.push_back(point.lineage);
    return out;
}

template<class T>
boolean_outcome<conservative_bound_record<T>> union_bound_records(
    const conservative_bound_record<T> &a, const conservative_bound_record<T> &b,
    finite_bound_id id = finite_bound_id(0)) {
    if (a.schema_version != 1 || b.schema_version != 1)
        return boolean_outcome<conservative_bound_record<T>>::failure(
            bounded_operations_detail::arithmetic_error(31906));
    if (static_cast<bool>(a.bound.owner.anchor) != static_cast<bool>(b.bound.owner.anchor) ||
        (a.bound.owner.anchor && !a.bound.owner.same_owner(b.bound.owner)))
        return boolean_outcome<conservative_bound_record<T>>::failure(
            bounded_operations_detail::arithmetic_error(31907));
    conservative_bound_record<T> out;
    out.bound = union_bounds(a.bound, b.bound, id);
    out.ordered_provenance = a.ordered_provenance;
    out.ordered_lineages = a.ordered_lineages;
    for (const auto value : b.ordered_provenance)
        conservative_bounds_detail::canonical_insert(out.ordered_provenance, value);
    for (const auto value : b.ordered_lineages)
        conservative_bounds_detail::canonical_insert(out.ordered_lineages, value);
    return boolean_outcome<conservative_bound_record<T>>::success(std::move(out));
}

template<class T>
bounded_aabb3<T> edge_bound(const bounded_point3<T> &a, const bounded_point3<T> &b,
                            finite_bound_id id = finite_bound_id(0)) {
    return union_bounds(point_bound(a), point_bound(b), id);
}

template<class T>
bounded_aabb3<T> triangle_bound(const bounded_point3<T> &a,
                                const bounded_point3<T> &b,
                                const bounded_point3<T> &c,
                                finite_bound_id id = finite_bound_id(0)) {
    return union_bounds(union_bounds(point_bound(a), point_bound(b)), point_bound(c), id);
}

template<class T>
bool bounds_overlap_closed(const bounded_aabb3<T> &a, const bounded_aabb3<T> &b) noexcept {
    for (unsigned axis = 0; axis < 3; ++axis)
        if (a.axes[axis].upper() < b.axes[axis].lower() || b.axes[axis].upper() < a.axes[axis].lower())
            return false;
    return true;
}

template<class T>
bool bounds_definitely_separated(const bounded_aabb3<T> &a, const bounded_aabb3<T> &b) noexcept {
    return !bounds_overlap_closed(a, b);
}

template<class T>
boolean_outcome<bounded_aabb3<T>> intersect_bounds(const bounded_aabb3<T> &a,
                                                   const bounded_aabb3<T> &b,
                                                   const interval_intersection_proof &proof,
                                                   finite_bound_id id = finite_bound_id(0)) {
    if (static_cast<bool>(a.owner.anchor) != static_cast<bool>(b.owner.anchor) ||
        (a.owner.anchor && !a.owner.same_owner(b.owner)))
        return boolean_outcome<bounded_aabb3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31907));
    if (!bounds_overlap_closed(a, b))
        return boolean_outcome<bounded_aabb3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31901));
    if (proof.left_parent != a.id.ordinal() || proof.right_parent != b.id.ordinal() ||
        proof.constraint != interval_constraint_kind::independent_geometric_bound ||
        proof.guarantee_source == 0 || proof.verifier_path == 0)
        return boolean_outcome<bounded_aabb3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31905));
    bounded_aabb3<T> out = union_bounds(a, b, id);
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto intersection = interval_intersection(a.axes[axis], b.axes[axis], proof);
        if (!intersection)
            return boolean_outcome<bounded_aabb3<T>>::failure(
                bounded_operations_detail::arithmetic_error(31905));
        out.axes[axis] = *intersection.value;
    }
    return boolean_outcome<bounded_aabb3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_aabb3<T>> intersect_bounds(const bounded_aabb3<T> &,
                                                   const bounded_aabb3<T> &,
                                                   finite_bound_id = finite_bound_id(0)) {
    return boolean_outcome<bounded_aabb3<T>>::failure(
        bounded_operations_detail::arithmetic_error(31905));
}

template<class T>
boolean_outcome<bounded_aabb3<T>> inflate_bound(const bounded_aabb3<T> &bound, T amount,
                                                finite_bound_id id = finite_bound_id(0)) {
    if (!std::isfinite(amount) || amount < T(0))
        return boolean_outcome<bounded_aabb3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31902));
    bounded_aabb3<T> out = bound; out.id = id;
    for (unsigned axis = 0; axis < 3; ++axis) {
        auto lower = directed_subtract(bound.axes[axis].lower(), amount);
        auto upper = directed_add(bound.axes[axis].upper(), amount);
        if (!lower || !upper)
            return boolean_outcome<bounded_aabb3<T>>::failure(
                bounded_operations_detail::arithmetic_error(31903));
        out.axes[axis] = *finite_interval<T>::create(lower.value.lower, upper.value.upper);
    }
    double inflated = 0.0;
    if (!conservative_bounds_detail::add_contributor(
            out.inflation.current_cleanup, static_cast<double>(amount), inflated))
        return boolean_outcome<bounded_aabb3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31903));
    out.inflation.current_cleanup = inflated;
    return boolean_outcome<bounded_aabb3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<T> squared_distance_lower_bound(const bounded_aabb3<T> &a,
                                                const bounded_aabb3<T> &b) {
    T total = T(0);
    for (unsigned axis = 0; axis < 3; ++axis) {
        T gap = T(0);
        if (a.axes[axis].upper() < b.axes[axis].lower()) {
            const auto difference = directed_subtract(b.axes[axis].lower(), a.axes[axis].upper());
            if (!difference) return boolean_outcome<T>::failure(bounded_operations_detail::arithmetic_error(31904));
            gap = std::max(T(0), difference.value.lower);
        } else if (b.axes[axis].upper() < a.axes[axis].lower()) {
            const auto difference = directed_subtract(a.axes[axis].lower(), b.axes[axis].upper());
            if (!difference) return boolean_outcome<T>::failure(bounded_operations_detail::arithmetic_error(31904));
            gap = std::max(T(0), difference.value.lower);
        }
        const auto square = directed_multiply(gap, gap);
        if (!square)
            return boolean_outcome<T>::failure(bounded_operations_detail::arithmetic_error(31904));
        const auto sum = directed_add(total, square.value.lower);
        if (!sum) return boolean_outcome<T>::failure(bounded_operations_detail::arithmetic_error(31904));
        total = std::max(T(0), sum.value.lower);
    }
    return boolean_outcome<T>::success(total);
}

} // namespace ygor::mesh_boolean::bounded
