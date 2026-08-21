#pragma once

#include "YgorMath.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>

enum class boolean_operation : std::uint8_t {
    set_union = 1,
    intersection = 2,
    a_minus_b = 3,
    b_minus_a = 4,
    symmetric_difference = 5,
};

enum class solid_policy_kind : std::uint8_t { outward_oriented_alternating_shells_v1 = 1 };
enum class contact_policy_kind : std::uint8_t { regularized_symbolic_v1 = 1 };
enum class output_policy_kind : std::uint8_t { triangulated_oriented_manifold_v1 = 1 };
enum class verification_level : std::uint8_t { mandatory_scalable_v1 = 1, exhaustive_diagnostics_v1 = 2 };
enum class determinism_mode : std::uint8_t { canonical_v1 = 1 };
enum class bounded_execution_mode : std::uint8_t { serial_v1 = 1, deterministic_parallel_v1 = 2 };
enum class replay_retention : std::uint8_t { digest_only = 1, full_on_failure = 2, full_always = 3 };

struct solid_policy {
    std::uint16_t version = 1;
    solid_policy_kind kind = solid_policy_kind::outward_oriented_alternating_shells_v1;
    std::uint32_t reserved = 0;
};
struct contact_policy {
    std::uint16_t version = 1;
    contact_policy_kind kind = contact_policy_kind::regularized_symbolic_v1;
    std::uint32_t reserved = 0;
};
struct output_policy {
    std::uint16_t version = 1;
    output_policy_kind kind = output_policy_kind::triangulated_oriented_manifold_v1;
    bool preserve_public_metadata = false;
    std::uint32_t reserved = 0;
};
struct verification_policy {
    std::uint16_t version = 1;
    verification_level level = verification_level::mandatory_scalable_v1;
    std::uint32_t reserved = 0;
};
struct determinism_policy {
    std::uint16_t version = 1;
    determinism_mode mode = determinism_mode::canonical_v1;
    std::uint32_t reserved = 0;
};
struct execution_policy {
    std::uint16_t version = 1;
    bounded_execution_mode mode = bounded_execution_mode::serial_v1;
    std::uint32_t requested_workers = 0;
    std::uint32_t reserved = 0;
};
struct resource_limit_policy {
    std::uint64_t advisory = 0;
    std::uint64_t hard = 0;
};
struct resource_policy {
    std::uint16_t version = 1;
    resource_limit_policy persistent_bytes;
    resource_limit_policy temporary_bytes;
    resource_limit_policy source_vertices;
    resource_limit_policy source_faces;
    resource_limit_policy source_indices;
    resource_limit_policy work_units;
    resource_limit_policy precision_trace_nodes;
    resource_limit_policy precision_ledger_records;
    resource_limit_policy budget_proposals;
    resource_limit_policy budget_reservations;
    resource_limit_policy budget_commits;
    resource_limit_policy precision_verifier_work;
    std::uint32_t reserved = 0;

    static constexpr resource_policy conservative_defaults() noexcept {
        return {1, {1ULL << 29, 1ULL << 30}, {1ULL << 29, 1ULL << 30},
                {10'000'000, 20'000'000}, {10'000'000, 20'000'000},
                {60'000'000, 120'000'000}, {1ULL << 34, 1ULL << 35},
                {4'000'000, 8'000'000}, {4'000'000, 8'000'000},
                {1'000'000, 2'000'000}, {1'000'000, 2'000'000},
                {1'000'000, 2'000'000}, {1ULL << 32, 1ULL << 33}, 0};
    }
};
struct diagnostic_policy {
    std::uint16_t version = 1;
    std::uint32_t secondary_finding_capacity = 64;
    replay_retention replay = replay_retention::digest_only;
    std::uint32_t reserved = 0;
};

template<class T>
struct bounded_boolean_options {
    T tolerance = T(0);
    T input_precision_a = T(0);
    T input_precision_b = T(0);
    solid_policy solids{};
    contact_policy contacts{};
    output_policy output{};
    verification_policy verification{};
    determinism_policy determinism{};
    execution_policy execution{};
    resource_policy resources = resource_policy::conservative_defaults();
    diagnostic_policy diagnostics{};
};

enum class bounded_boolean_error_category : std::uint8_t {
    input_contract_error = 1,
    input_geometry_not_epsilon_valid = 2,
    unsupported_platform = 3,
    invalid_tolerance = 4,
    ambiguous_shell_semantics = 5,
    geometric_condition_exceeds_tolerance = 6,
    cleanup_budget_exceeded = 7,
    result_geometry_not_validated = 8,
    index_overflow = 9,
    resource_limit = 10,
    cancelled = 11,
    internal_invariant_error = 12,
};

struct bounded_boolean_digest {
    std::array<std::uint8_t, 32> bytes{};
    std::string hex() const;
    friend bool operator==(const bounded_boolean_digest &a, const bounded_boolean_digest &b) noexcept { return a.bytes == b.bytes; }
    friend bool operator!=(const bounded_boolean_digest &a, const bounded_boolean_digest &b) noexcept { return !(a == b); }
};

struct bounded_boolean_error {
    std::uint16_t version = 1;
    bounded_boolean_error_category category = bounded_boolean_error_category::internal_invariant_error;
    std::uint32_t subcode = 0;
    std::uint16_t component = 1;
    std::uint16_t stage = 0;
    std::uint32_t checkpoint = 0;
    bounded_boolean_digest context_digest{};
    bounded_boolean_digest replay_digest{};
    std::array<std::uint64_t, 4> witnesses{};
    std::uint8_t witness_count = 0;
    const char *summary = "bounded Boolean invariant failure";
};

class bounded_boolean_cancellation_token;
class bounded_boolean_cancellation_source {
  public:
    bounded_boolean_cancellation_source();
    bounded_boolean_cancellation_token token() const noexcept;
    void request_cancel(std::uint32_t reason = 1) noexcept;
  private:
    struct state;
    std::shared_ptr<state> state_;
    friend class bounded_boolean_cancellation_token;
};
class bounded_boolean_cancellation_token {
  public:
    bounded_boolean_cancellation_token() noexcept = default;
    bool cancellation_requested() const noexcept;
    std::uint32_t reason() const noexcept;
  private:
    explicit bounded_boolean_cancellation_token(std::shared_ptr<bounded_boolean_cancellation_source::state>) noexcept;
    std::shared_ptr<bounded_boolean_cancellation_source::state> state_;
    friend class bounded_boolean_cancellation_source;
};

template<class T, class I>
struct bounded_boolean_success {
    fv_surface_mesh<T, I> mesh;
    T output_precision = T(0);
    T maximum_authorized_tolerance = T(0);
    T maximum_realized_displacement = T(0);
    bounded_boolean_digest digest{};
};

template<class T, class I>
class bounded_boolean_result {
  public:
    using success_type = bounded_boolean_success<T, I>;
    explicit bounded_boolean_result(success_type value) : value_(std::move(value)) {}
    explicit bounded_boolean_result(bounded_boolean_error error) : value_(std::move(error)) {}
    bool has_value() const noexcept { return std::holds_alternative<success_type>(value_); }
    const success_type *value() const noexcept { return std::get_if<success_type>(&value_); }
    const bounded_boolean_error *error() const noexcept { return std::get_if<bounded_boolean_error>(&value_); }
  private:
    std::variant<success_type, bounded_boolean_error> value_;
};

template<class T, class I>
bounded_boolean_result<T, I> bounded_boolean(const fv_surface_mesh<T, I> &a,
                                              const fv_surface_mesh<T, I> &b,
                                              boolean_operation operation,
                                              const bounded_boolean_options<T> &options);
template<class T, class I>
bounded_boolean_result<T, I> bounded_boolean(const fv_surface_mesh<T, I> &a,
                                              const fv_surface_mesh<T, I> &b,
                                              boolean_operation operation,
                                              const bounded_boolean_options<T> &options,
                                              const bounded_boolean_cancellation_token &token);
