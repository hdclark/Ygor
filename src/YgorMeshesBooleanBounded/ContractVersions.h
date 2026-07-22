#pragma once

#include <cstdint>

namespace ygor::mesh_boolean::bounded {
struct contract_versions final {
    static constexpr std::uint16_t public_api = 1;
    static constexpr std::uint16_t type_profile = 1;
    static constexpr std::uint16_t source_read = 1;
    static constexpr std::uint16_t source_snapshot = 1;
    static constexpr std::uint16_t source_bundle = 1;
    static constexpr std::uint16_t canonical_bytes = 1;
    static constexpr std::uint16_t sha256 = 1;
    static constexpr std::uint16_t policies = 1;
    static constexpr std::uint16_t truth_table = 1;
    static constexpr std::uint16_t symbolic_policy = 1;
    static constexpr std::uint16_t identities = 1;
    static constexpr std::uint16_t errors = 1;
    static constexpr std::uint16_t resources = 1;
    static constexpr std::uint16_t cancellation = 1;
    static constexpr std::uint16_t replay = 1;
    static constexpr std::uint16_t platform = 1;
    static constexpr std::uint16_t context = 1;
    static constexpr std::uint16_t transaction = 1;
    static constexpr std::uint16_t verifier = 1;
    static constexpr std::uint16_t precision_bootstrap_schema = 1;
    static constexpr std::uint16_t precision_bootstrap_provider = 1;
    static constexpr std::uint16_t precision_scalar_profile = 1;
    static constexpr std::uint16_t floating_bits = 1;
    static constexpr std::uint16_t directed_rounding = 1;
    static constexpr std::uint16_t finite_interval_schema = 1;
    static constexpr std::uint16_t finite_interval_provider = 1;
    static constexpr std::uint16_t exact_expansion_core = 1;
    static constexpr std::uint16_t exact_expansion_adapter = 1;
    static constexpr std::uint16_t rounded_operation_graphs = 1;
    static constexpr std::uint16_t exact_relation_formulas = 1;
    static constexpr std::uint16_t bounded_values = 1;
    static constexpr std::uint16_t predicate_truth_layers = 1;
    static constexpr std::uint16_t construction_conditioning = 1;
    static constexpr std::uint16_t precision_trace = 1;
    static constexpr std::uint16_t precision_ledger = 1;
    static constexpr std::uint16_t tolerance_certificates = 1;
    static constexpr std::uint16_t tolerance_budget = 1;
    static constexpr std::uint16_t precision_import = 1;
    static constexpr std::uint16_t precision_codec = 1;
    static constexpr std::uint16_t precision_verifier = 1;
};
enum class operand_id : std::uint8_t { a = 0, b = 1 };
enum class stage_id : std::uint16_t {
    public_entry = 1, source_capture = 2, context_preflight = 3,
    precision_bootstrap = 4, publication = 20, qualification = 21,
    execution_service = 22,
};
enum class precision_checkpoint : std::uint32_t {
    pending_context_validation = 1, source_bit_scan = 2, non_finite_rejection = 3,
    scale_derivation = 4, machine_floor_derivation = 5, precision_normalization = 6,
    preflight_encoding = 7, frozen_context_handshake = 8, context_construction = 9,
    source_import = 10, rounded_operation = 11, exact_relation = 12,
    predicate_assembly = 13, construction_conditioning = 14, trace_finalization = 15,
    ledger_snapshot = 16, tolerance_reservation = 17, tolerance_commit = 18,
    conservative_bounds = 19, prior_precision_import = 20, independent_verification = 21,
    canonical_encoding = 22, publication_commit = 23,
};
}
