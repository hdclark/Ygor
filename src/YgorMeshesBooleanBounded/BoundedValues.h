#pragma once

#include "FiniteInterval.h"
#include "Identity.h"
#include "PrecisionTypes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct bounded_value_tag;
struct provenance_tag;
struct geometric_lineage_tag;
struct precision_ledger_entry_tag;
struct exact_relation_tag;
struct finite_bound_tag;

using bounded_value_id = strong_id<bounded_value_tag>;
using provenance_id = strong_id<provenance_tag>;
using geometric_lineage_id = strong_id<geometric_lineage_tag>;
using precision_ledger_entry_id = strong_id<precision_ledger_entry_tag>;
using exact_relation_id = strong_id<exact_relation_tag>;
using finite_bound_id = strong_id<finite_bound_tag>;

enum class bounded_publication_state : std::uint8_t {
    transaction_local = 1, committed = 2, invalid = 3
};

struct uncertainty_contributors final {
    double inherited_a = 0.0;
    double inherited_b = 0.0;
    double machine_floor = 0.0;
    double construction = 0.0;
    double conditioning = 0.0;
    double conversion = 0.0;
    double prior_cleanup = 0.0;
    double current_cleanup = 0.0;
};

struct bounded_value_identity final {
    std::uint16_t schema_version = 1;
    std::uint16_t provider_version = 1;
    context_owner_token owner{};
    bounded_value_id value{0};
    provenance_id provenance{0};
    geometric_lineage_id lineage{0};
    precision_ledger_entry_id ledger_entry{0};
    std::uint64_t trace_root = 0;
    bounded_publication_state publication = bounded_publication_state::transaction_local;
};

template<class T>
struct bounded_scalar final {
    T rounded_nominal{};
    finite_interval<T> uncertainty_enclosure = finite_interval<T>::singleton(T(0));
    bounded_value_identity identity{};
    uncertainty_contributors contributors{};
};

template<class T>
struct bounded_vec2 final {
    context_owner_token owner{};
    std::array<bounded_scalar<T>, 2> components{};
    T radial_error_upper{};
};

template<class T>
struct bounded_vec3 final {
    context_owner_token owner{};
    std::array<bounded_scalar<T>, 3> components{};
    T radial_error_upper{};
};

template<class T>
struct bounded_point3 final {
    context_owner_token owner{};
    bounded_vec3<T> coordinates{};
    provenance_id provenance{0};
    geometric_lineage_id lineage{0};
};

template<class T>
struct bounded_plane3 final {
    context_owner_token owner{};
    bounded_vec3<T> normal{};
    bounded_scalar<T> offset{};
    bounded_point3<T> anchor{};
    bounded_scalar<T> normal_sq{};
    provenance_id provenance{0};
};

enum class parameter_carrier : std::uint8_t { edge = 1, line = 2, ray = 3, face = 4 };
enum class endpoint_convention : std::uint8_t { closed = 1, open = 2, half_open_a = 3, half_open_b = 4 };
enum class parameter_domain_status : std::uint8_t {
    stable_interior = 1, stable_endpoint = 2, overlaps_boundary = 3, outside = 4, invalid = 5
};

template<class T>
struct bounded_parameter final {
    context_owner_token owner{};
    bounded_scalar<T> value{};
    parameter_carrier carrier = parameter_carrier::edge;
    endpoint_convention endpoints = endpoint_convention::closed;
    parameter_domain_status domain = parameter_domain_status::invalid;
    T domain_margin{};
};

enum class residual_disposition : std::uint8_t { pass = 1, uncertain = 2, fail = 3, invalid = 4 };

template<class T>
struct bounded_residual final {
    context_owner_token owner{};
    bounded_scalar<T> value{};
    T scale{};
    T comparison_boundary{};
    residual_disposition disposition = residual_disposition::invalid;
    uncertainty_contributors contributors{};
};

template<class T>
struct bounded_aabb3 final {
    context_owner_token owner{};
    std::array<finite_interval<T>, 3> axes{{finite_interval<T>::singleton(T(0)),
                                            finite_interval<T>::singleton(T(0)),
                                            finite_interval<T>::singleton(T(0))}};
    finite_bound_id id{0};
    provenance_id provenance{0};
    geometric_lineage_id lineage{0};
    uncertainty_contributors inflation{};
};

template<class Value>
using published_bounded = std::shared_ptr<const Value>;

template<class Value>
struct local_bounded_value final {
    context_owner_token owner{};
    task_local_id<bounded_value_tag> local_id{0};
    task_local_id<struct precision_trace_node_tag> trace_node{0};
    Value value{};
};

} // namespace ygor::mesh_boolean::bounded
