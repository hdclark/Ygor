#pragma once

#include "BoundedValues.h"
#include "Outcome.h"

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct precision_trace_tag;
struct precision_trace_node_tag;
using precision_trace_id = strong_id<precision_trace_tag>;
using precision_trace_node_id = strong_id<precision_trace_node_tag>;
using local_trace_node_id = task_local_id<precision_trace_node_tag>;

struct precision_trace_key final {
    std::uint16_t operation_code = 0;
    std::uint16_t exact_formula_code = 0;
    std::vector<std::uint64_t> parents;
    std::vector<std::uint8_t> result_bytes;
    std::vector<std::uint8_t> contributor_bytes;
    std::vector<std::uint64_t> provenance;
    friend bool operator<(const precision_trace_key &a, const precision_trace_key &b) noexcept {
        return std::tie(a.operation_code, a.exact_formula_code, a.parents, a.result_bytes,
                        a.contributor_bytes, a.provenance) <
               std::tie(b.operation_code, b.exact_formula_code, b.parents, b.result_bytes,
                        b.contributor_bytes, b.provenance);
    }
    friend bool operator==(const precision_trace_key &a, const precision_trace_key &b) noexcept {
        return !(a < b) && !(b < a);
    }
};

struct local_precision_trace_node final {
    local_trace_node_id id{0};
    precision_trace_key key{};
    uncertainty_contributors contributors{};
};

struct committed_precision_trace_node final {
    precision_trace_node_id id{0};
    precision_trace_key key{};
    uncertainty_contributors contributors{};
};

struct precision_trace_snapshot final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    precision_trace_id id{0};
    std::vector<committed_precision_trace_node> nodes;
    precision_trace_node_id root{0};
};

class local_precision_trace final {
  public:
    explicit local_precision_trace(context_owner_token owner) : owner_(std::move(owner)) {}
    local_trace_node_id append(precision_trace_key key, uncertainty_contributors contributors = {});
    const context_owner_token &owner() const noexcept { return owner_; }
    const std::vector<local_precision_trace_node> &nodes() const noexcept { return nodes_; }
  private:
    context_owner_token owner_;
    std::vector<local_precision_trace_node> nodes_;
};

boolean_outcome<precision_trace_snapshot> finalize_precision_trace(
    const local_precision_trace &trace, local_trace_node_id root, precision_trace_id committed_id);

} // namespace ygor::mesh_boolean::bounded
