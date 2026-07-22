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
};
enum class operand_id : std::uint8_t { a = 0, b = 1 };
enum class stage_id : std::uint16_t {
    public_entry = 1, source_capture = 2, context_preflight = 3,
    precision_bootstrap = 4, publication = 20, qualification = 21,
    execution_service = 22,
};
}
