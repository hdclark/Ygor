#include "PrecisionTrace.h"

#include "CanonicalBytes.h"

#include <cmath>
#include <map>
#include <set>

namespace ygor::mesh_boolean::bounded {
namespace {
bounded_boolean_error trace_error(std::uint32_t code) {
    bounded_boolean_error e;
    e.category = bounded_boolean_error_category::internal_invariant_error;
    e.stage = 4;
    e.checkpoint = 15;
    e.component = 3;
    e.subcode = code;
    e.summary = "precision trace finalization failed";
    return e;
}

bool valid_contributors(const uncertainty_contributors &value) {
    const double values[]{value.inherited_a, value.inherited_b, value.machine_floor,
                          value.construction, value.conditioning, value.conversion,
                          value.prior_cleanup, value.current_cleanup};
    for (const double item : values)
        if (!std::isfinite(item) || item < 0.0) return false;
    return true;
}

bool valid_trace_key(const precision_trace_key &key) {
    const auto descriptor = rounded_operation_descriptor_for(
        static_cast<rounded_operation_code>(key.operation_code));
    if (descriptor.code == rounded_operation_code::invalid ||
        !valid_exact_formula_code(key.exact_formula_code) ||
        key.parents.size() < descriptor.minimum_arity ||
        key.parents.size() > descriptor.maximum_arity) return false;
    for (std::size_t i = 1; i < key.provenance.size(); ++i)
        if (key.provenance[i - 1] >= key.provenance[i]) return false;
    return true;
}

void encode_contributors(canonical_writer &writer, const uncertainty_contributors &value) {
    writer.floating(value.inherited_a); writer.floating(value.inherited_b);
    writer.floating(value.machine_floor); writer.floating(value.construction);
    writer.floating(value.conditioning); writer.floating(value.conversion);
    writer.floating(value.prior_cleanup); writer.floating(value.current_cleanup);
}

std::vector<std::uint8_t> structural_key(
    const local_precision_trace_node &node,
    const std::vector<std::vector<std::uint8_t>> &parent_keys) {
    canonical_writer writer;
    writer.u16(node.key.operation_code);
    writer.u16(node.key.exact_formula_code);
    writer.u64(node.key.parents.size());
    for (const auto parent : node.key.parents) writer.sized_bytes(parent_keys[parent]);
    writer.sized_bytes(node.key.result_bytes);
    writer.sized_bytes(node.key.contributor_bytes);
    writer.u64(node.key.provenance.size());
    for (const auto provenance : node.key.provenance) writer.u64(provenance);
    encode_contributors(writer, node.contributors);
    return writer.take();
}
}

local_trace_node_id local_precision_trace::append(precision_trace_key key,
                                                  uncertainty_contributors contributors) {
    const local_trace_node_id id{static_cast<std::uint64_t>(nodes_.size())};
    nodes_.push_back({id, std::move(key), contributors});
    return id;
}

boolean_outcome<precision_trace_snapshot> finalize_precision_trace(
    const local_precision_trace &trace, local_trace_node_id root, precision_trace_id committed_id) {
    if (!trace.owner().anchor || root.ordinal >= trace.nodes().size())
        return boolean_outcome<precision_trace_snapshot>::failure(trace_error(31501));
    std::vector<bool> reachable(trace.nodes().size(), false);
    std::vector<std::uint64_t> stack{root.ordinal};
    while (!stack.empty()) {
        const auto id = stack.back();
        stack.pop_back();
        if (id >= trace.nodes().size())
            return boolean_outcome<precision_trace_snapshot>::failure(trace_error(31502));
        if (trace.nodes()[id].id.ordinal != id || !valid_trace_key(trace.nodes()[id].key) ||
            !valid_contributors(trace.nodes()[id].contributors))
            return boolean_outcome<precision_trace_snapshot>::failure(trace_error(31504));
        if (reachable[id]) continue;
        reachable[id] = true;
        for (const auto parent : trace.nodes()[id].key.parents) {
            if (parent >= id)
                return boolean_outcome<precision_trace_snapshot>::failure(trace_error(31503));
            stack.push_back(parent);
        }
    }
    std::vector<std::uint64_t> depth(trace.nodes().size(), 0);
    std::vector<std::vector<std::uint8_t>> structural(trace.nodes().size());
    std::vector<const local_precision_trace_node *> ordered;
    for (std::size_t i = 0; i < reachable.size(); ++i) {
        if (!reachable[i]) continue;
        for (const auto parent : trace.nodes()[i].key.parents)
            depth[i] = std::max(depth[i], depth[parent] + 1);
        structural[i] = structural_key(trace.nodes()[i], structural);
        ordered.push_back(&trace.nodes()[i]);
    }
    std::sort(ordered.begin(), ordered.end(), [&](const auto *a, const auto *b) {
        if (depth[a->id.ordinal] != depth[b->id.ordinal])
            return depth[a->id.ordinal] < depth[b->id.ordinal];
        if (structural[a->id.ordinal] != structural[b->id.ordinal])
            return structural[a->id.ordinal] < structural[b->id.ordinal];
        return false;
    });
    precision_trace_snapshot out;
    out.owner = trace.owner();
    out.id = committed_id;
    std::map<std::uint64_t, std::uint64_t> remap;
    for (const auto *node : ordered) {
        precision_trace_key key = node->key;
        for (auto &parent : key.parents) parent = remap.at(parent);
        const auto existing = std::find_if(out.nodes.begin(), out.nodes.end(), [&](const auto &candidate) {
            return candidate.key == key &&
                   candidate.contributors.inherited_a == node->contributors.inherited_a &&
                   candidate.contributors.inherited_b == node->contributors.inherited_b &&
                   candidate.contributors.machine_floor == node->contributors.machine_floor &&
                   candidate.contributors.construction == node->contributors.construction &&
                   candidate.contributors.conditioning == node->contributors.conditioning &&
                   candidate.contributors.conversion == node->contributors.conversion &&
                   candidate.contributors.prior_cleanup == node->contributors.prior_cleanup &&
                   candidate.contributors.current_cleanup == node->contributors.current_cleanup;
        });
        std::uint64_t canonical = 0;
        if (existing != out.nodes.end()) canonical = existing->id.ordinal();
        else {
            canonical = out.nodes.size();
            out.nodes.push_back({precision_trace_node_id(canonical), std::move(key), node->contributors});
        }
        remap[node->id.ordinal] = canonical;
    }
    out.root = precision_trace_node_id(remap.at(root.ordinal));
    return boolean_outcome<precision_trace_snapshot>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
