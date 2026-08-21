#pragma once

#include "BoundedOperations.h"
#include "PrecisionLedger.h"
#include "Resources.h"

#include <map>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct budget_proposal_tag;
struct budget_reservation_tag;
struct budget_commit_tag;
struct displacement_certificate_tag;
struct topology_authorization_tag;
using budget_proposal_id = strong_id<budget_proposal_tag>;
using budget_reservation_id = strong_id<budget_reservation_tag>;
using budget_commit_id = strong_id<budget_commit_tag>;
using displacement_certificate_id = strong_id<displacement_certificate_tag>;
using topology_authorization_id = strong_id<topology_authorization_tag>;

enum class length_certificate_kind : std::uint8_t {
    vertex_displacement = 1,
    patch_hausdorff_deviation = 2,
    feature_thickness_or_clearance = 3,
    swept_displacement = 4,
    whole_component_deviation = 5
};
enum class cleanup_operation_kind : std::uint8_t {
    move_vertex = 1, collapse_feature = 2, remove_patch = 3, remove_component = 4
};
enum class length_unit_kind : std::uint8_t { coordinate_length = 1 };

struct length_deviation_certificate final {
    std::uint16_t schema_version = 1;
    displacement_certificate_id id{0};
    length_certificate_kind kind = length_certificate_kind::vertex_displacement;
    length_unit_kind units = length_unit_kind::coordinate_length;
    double upper_length = 0.0;
    std::vector<std::uint8_t> evidence;
};

struct topology_authorization final {
    std::uint16_t schema_version = 1;
    topology_authorization_id id{0};
    context_owner_token owner{};
    cleanup_operation_kind operation = cleanup_operation_kind::move_vertex;
    std::uint64_t transaction_owner = 0;
    std::vector<std::uint8_t> evidence;
    std::vector<std::uint8_t> authentication;
};

struct budget_length_evidence final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    std::uint64_t transaction_owner = 0;
    std::vector<double> cumulative_lengths;
    std::vector<std::uint8_t> evidence;
};

struct tolerance_budget_proposal final {
    budget_proposal_id id{0};
    cleanup_operation_kind operation = cleanup_operation_kind::move_vertex;
    std::vector<geometric_lineage_id> ordered_lineages;
    std::vector<double> requested_costs;
    length_deviation_certificate certificate{};
    budget_length_evidence before_evidence{};
    topology_authorization authorization{};
    context_owner_token owner{};
    std::uint64_t transaction_owner = 0;
    std::vector<std::uint8_t> canonical_merge_key;
};

struct tolerance_budget_commit final {
    budget_commit_id id{0};
    budget_proposal_id proposal{0};
    std::vector<double> actual_costs;
    budget_length_evidence after_evidence{};
    std::vector<precision_ledger_entry_id> ledger_entries;
    tolerance_budget_proposal proposal_record{};
    std::vector<double> cumulative_totals;
};

struct tolerance_lineage_total final {
    geometric_lineage_id lineage{0};
    double committed_displacement = 0.0;
};

struct tolerance_budget_snapshot final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    double tolerance = 0.0;
    std::vector<tolerance_lineage_total> lineages;
    std::vector<tolerance_budget_commit> commits;
    std::vector<tolerance_budget_proposal> reservations;
    double global_realized_displacement = 0.0;
    std::size_t active_reservations = 0;
};

class tolerance_budget;
struct tolerance_budget_state;

class tolerance_reservation final {
  public:
    tolerance_reservation() noexcept = default;
    tolerance_reservation(const tolerance_reservation &) = delete;
    tolerance_reservation &operator=(const tolerance_reservation &) = delete;
    tolerance_reservation(tolerance_reservation &&other) noexcept;
    tolerance_reservation &operator=(tolerance_reservation &&other) noexcept;
    ~tolerance_reservation();
    budget_reservation_id id() const noexcept { return id_; }
    budget_proposal_id proposal() const noexcept { return proposal_; }
    bool active() const noexcept { return static_cast<bool>(state_); }
  private:
    tolerance_reservation(std::shared_ptr<tolerance_budget_state> state, budget_reservation_id id,
                           const tolerance_budget_proposal &proposal);
    void release() noexcept;
    std::shared_ptr<tolerance_budget_state> state_;
    budget_reservation_id id_{0};
    budget_proposal_id proposal_{0};
    std::uint64_t transaction_owner_ = 0;
    std::vector<geometric_lineage_id> lineages_;
    std::vector<double> reserved_;
    std::optional<resource_reservation> proposal_resource_;
    std::optional<resource_reservation> reservation_resource_;
    friend class tolerance_budget;
};

class tolerance_budget final {
  public:
    tolerance_budget(context_owner_token owner, double tolerance);
    tolerance_budget(context_owner_token owner, double tolerance, resource_manager *resources,
                     resource_cancellation_checkpoint cancellation = {});
    boolean_outcome<topology_authorization> authorize_topology(
        cleanup_operation_kind operation, std::uint64_t transaction_owner,
        std::vector<std::uint8_t> evidence);
    boolean_outcome<tolerance_reservation> reserve(const tolerance_budget_proposal &proposal);
    boolean_outcome<budget_commit_id> commit(tolerance_reservation &&reservation,
                                             std::vector<double> reported_actual_costs,
                                             budget_length_evidence after_evidence,
                                             std::uint64_t transaction_owner,
                                             const precision_ledger &ledger,
                                             std::vector<precision_ledger_entry_id> ledger_entries);
    std::shared_ptr<const tolerance_budget_snapshot> snapshot() const;
    bool can_commit_stage() const noexcept;
    const context_owner_token &owner() const noexcept;
  private:
    void rollback(budget_reservation_id id) noexcept;
    std::shared_ptr<tolerance_budget_state> state_;
    friend class tolerance_reservation;
};

} // namespace ygor::mesh_boolean::bounded
