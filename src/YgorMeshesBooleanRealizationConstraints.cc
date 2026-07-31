#include "YgorMeshesBooleanRealization.h"
#include "YgorMeshesBooleanExecutor.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace detail {
namespace {

struct disjoint_set {
  explicit disjoint_set(std::size_t n) : parent(n) {
    std::iota(parent.begin(), parent.end(), std::size_t(0));
  }
  std::size_t root(std::size_t x) {
    while (parent[x] != x) {
      parent[x] = parent[parent[x]];
      x = parent[x];
    }
    return x;
  }
  void join(std::size_t a, std::size_t b) {
    a = root(a);
    b = root(b);
    if (a != b)
      parent[std::max(a, b)] = std::min(a, b);
  }
  std::vector<std::size_t> parent;
};

struct flat_pair_node {
  exact_point3 lower, upper;
  std::size_t begin = 0, end = 0;
  std::size_t left = std::numeric_limits<std::size_t>::max();
  std::size_t right = std::numeric_limits<std::size_t>::max();
  bool leaf() const { return left == std::numeric_limits<std::size_t>::max(); }
};

bool overlap(const exact_scalar &alo, const exact_scalar &ahi,
             const exact_scalar &blo, const exact_scalar &bhi) {
  return !(ahi < blo) && !(bhi < alo);
}

} // namespace

realization_solver_result solve_realization_constraint_components(
    std::vector<realization_solver_variable> variables,
    std::vector<realization_solver_constraint> constraints,
    const realization_constraint_evaluator &evaluate, std::uint64_t node_limit,
    std::uint64_t component_limit, std::uint64_t trail_limit,
    const std::function<bool()> &cancelled, deterministic_executor *executor) {
  realization_solver_result result;
  std::sort(variables.begin(), variables.end(), [](const auto &a, const auto &b) {
    return a.id < b.id;
  });
  std::sort(constraints.begin(), constraints.end(), [](const auto &a, const auto &b) {
    return a.id < b.id;
  });
  if (!evaluate || std::any_of(variables.begin(), variables.end(), [](const auto &v) {
        return v.domain_size == 0;
      }))
    return result;
  std::map<std::uint64_t, std::size_t> index;
  for (std::size_t i = 0; i < variables.size(); ++i)
    if (!index.emplace(variables[i].id, i).second)
      return result;
  disjoint_set sets(variables.size());
  for (auto &constraint : constraints) {
    std::sort(constraint.variables.begin(), constraint.variables.end());
    constraint.variables.erase(
        std::unique(constraint.variables.begin(), constraint.variables.end()),
        constraint.variables.end());
    for (auto id : constraint.variables)
      if (!index.count(id))
        return result;
    for (std::size_t i = 1; i < constraint.variables.size(); ++i)
      sets.join(index[constraint.variables[0]], index[constraint.variables[i]]);
  }
  std::map<std::size_t, realization_solver_component_result> grouped;
  for (std::size_t i = 0; i < variables.size(); ++i)
    grouped[sets.root(i)].variables.push_back(variables[i].id);
  for (const auto &constraint : constraints) {
    if (constraint.variables.empty()) {
      if (!evaluate(constraint.id, {}))
        return result;
      continue;
    }
    grouped[sets.root(index[constraint.variables.front()])].constraints.push_back(
        constraint.id);
  }
  if (grouped.size() > component_limit ||
      std::any_of(grouped.begin(), grouped.end(), [&](const auto &entry) {
        return entry.second.variables.size() > trail_limit;
      })) {
    result.limited = true;
    return result;
  }
  std::map<std::uint64_t, realization_solver_constraint> constraint_by_id;
  for (const auto &constraint : constraints)
    constraint_by_id.emplace(constraint.id, constraint);
  std::map<std::uint64_t, realization_solver_variable> variable_by_id;
  for (const auto &variable : variables)
    variable_by_id.emplace(variable.id, variable);

  struct component_work {
    realization_solver_component_result component;
    bool accepted = false;
    bool limited = false;
    bool singleton = false;
  };
  std::vector<component_work> work;
  work.reserve(grouped.size());
  for (auto &entry : grouped) {
    component_work item;
    item.component = std::move(entry.second);
    auto &component = item.component;
    std::sort(component.variables.begin(), component.variables.end());
    std::sort(component.constraints.begin(), component.constraints.end());
    work.push_back(std::move(item));
  }

  const auto solve_component = [&](component_work &item,
                                   const std::function<bool()> &stop,
                                   std::uint64_t component_node_limit) {
    auto &component = item.component;
    std::map<std::uint64_t, std::uint64_t> degree;
    for (auto constraint_id : component.constraints)
      for (auto variable_id : constraint_by_id.at(constraint_id).variables)
        ++degree[variable_id];
    auto order = component.variables;
    std::sort(order.begin(), order.end(), [&](auto a, auto b) {
      const auto &x = variable_by_id.at(a);
      const auto &y = variable_by_id.at(b);
      if (x.domain_size != y.domain_size)
        return x.domain_size < y.domain_size;
      if (degree[a] != degree[b])
        return degree[a] > degree[b];
      return a < b;
    });
    component.variable_order = order;
    item.singleton = std::all_of(
        order.begin(), order.end(), [&](auto id) {
          return variable_by_id.at(id).domain_size == 1;
        });
    if (item.singleton) {
      const auto canonical_nodes = component.variables.size() + 1;
      if (canonical_nodes > component_node_limit) {
        item.limited = true;
        return;
      }
      for (auto constraint_id : component.constraints) {
        if (stop && stop()) {
          item.limited = true;
          return;
        }
        const auto &constraint = constraint_by_id.at(constraint_id);
        std::vector<std::pair<std::uint64_t, std::uint64_t>> values;
        values.reserve(constraint.variables.size());
        for (auto id : constraint.variables)
          values.push_back({id, 0});
        if (!evaluate(constraint_id, values))
          return;
      }
      component.accepted_ranks.assign(component.variables.size(), 0);
      component.visited_nodes = canonical_nodes;
      component.complete_assignments = 1;
      item.accepted = true;
      return;
    }
    std::map<std::uint64_t, std::uint64_t> assignment;
    std::map<std::uint64_t, std::uint64_t> accepted_assignment;
    bool accepted = false;
    std::function<void(std::size_t)> dfs = [&](std::size_t depth) {
      if (accepted || item.limited)
        return;
      if (stop && stop()) {
        item.limited = true;
        return;
      }
      if (component.visited_nodes == component_node_limit) {
        item.limited = true;
        return;
      }
      ++component.visited_nodes;
      if (depth == order.size()) {
        ++component.complete_assignments;
        accepted = true;
        accepted_assignment = assignment;
        return;
      }
      const auto variable = order[depth];
      for (std::uint64_t rank = 0;
           rank < variable_by_id.at(variable).domain_size;
           ++rank) {
        assignment[variable] = rank;
        std::optional<std::uint64_t> conflict;
        for (auto constraint_id : component.constraints) {
          const auto &constraint = constraint_by_id.at(constraint_id);
          if (std::all_of(constraint.variables.begin(), constraint.variables.end(),
                          [&](auto id) { return assignment.count(id) != 0; })) {
            std::vector<std::pair<std::uint64_t, std::uint64_t>> values;
            for (auto id : constraint.variables)
              values.push_back({id, assignment[id]});
            if (!evaluate(constraint_id, values)) {
              conflict = constraint_id;
              break;
            }
          }
        }
        if (!conflict)
          dfs(depth + 1);
        else
          component.rejected_prefix_witnesses.push_back(*conflict);
        assignment.erase(variable);
        if (accepted || item.limited)
          break;
      }
    };
    dfs(0);
    if (!accepted)
      return;
    for (auto id : component.variables)
      component.accepted_ranks.push_back(accepted_assignment[id]);
    item.accepted = true;
  };

  if (executor && !work.empty() &&
      node_limit == std::numeric_limits<std::uint64_t>::max()) {
    std::vector<deterministic_task> tasks;
    tasks.reserve(work.size());
    for (std::size_t i = 0; i < work.size(); ++i) {
      const auto key = work[i].component.variables.front();
      tasks.push_back({key, [&, i](cancellation_token task_cancel) -> status_or<bool> {
        if (task_cancel.cancelled() || (cancelled && cancelled()))
          return make_error(boolean_error_code::resource_limit,
                            boolean_stage::geometry_realization,
                            "cancelled");
        solve_component(work[i], [&] {
          return task_cancel.cancelled() || (cancelled && cancelled());
        }, std::numeric_limits<std::uint64_t>::max());
        if (cancelled && cancelled())
          return make_error(boolean_error_code::resource_limit,
                            boolean_stage::geometry_realization,
                            "cancelled");
        if (work[i].limited)
          return make_error(boolean_error_code::resource_limit,
                            boolean_stage::geometry_realization,
                            "solver_component_limited");
        if (!work[i].accepted)
          return make_error(boolean_error_code::output_not_representable,
                            boolean_stage::geometry_realization,
                            "solver_component_rejected");
        return true;
      }});
    }
    cancellation_source dispatch_cancellation;
    const auto dispatched = executor->run(std::move(tasks),
                                          dispatch_cancellation.token());
    if (!dispatched.has_value()) {
      if (dispatched.error().code == boolean_error_code::resource_limit &&
          dispatched.error().message_key == "allocation")
        throw std::bad_alloc();
      if (dispatched.error().message_key == "cancelled") {
        result.limited = true;
        return result;
      }
      if (dispatched.error().message_key == "solver_component_limited" ||
          dispatched.error().message_key == "solver_component_rejected") {
        // Canonical merge below maps the private component outcome.
      } else {
      throw std::runtime_error(dispatched.error().message_key);
      }
    }
  } else {
    std::uint64_t remaining_nodes = node_limit;
    for (auto &item : work) {
      solve_component(item, cancelled, remaining_nodes);
      if (item.component.visited_nodes > remaining_nodes) {
        item.limited = true;
        break;
      }
      remaining_nodes -= item.component.visited_nodes;
      if (item.limited || !item.accepted) break;
    }
  }

  for (auto &item : work) {
    const auto remaining = result.visited_nodes > node_limit
                               ? 0
                               : node_limit - result.visited_nodes;
    if (item.limited || item.component.visited_nodes > remaining) {
      if (!item.singleton)
        result.visited_nodes += std::min(item.component.visited_nodes, remaining);
      result.limited = true;
      return result;
    }
    result.visited_nodes += item.component.visited_nodes;
    result.complete_assignments += item.component.complete_assignments;
    if (!item.accepted)
      return result;
    result.components.push_back(std::move(item.component));
  }
  result.accepted = true;
  return result;
}

std::vector<realization_triangle_pair> conservative_realization_triangle_pairs(
    const std::vector<realization_domain_box> &input,
    std::uint64_t *overlap_checks, std::uint64_t max_overlap_checks,
    std::uint64_t max_candidates, bool *limited) {
  std::vector<std::size_t> order(input.size());
  std::iota(order.begin(), order.end(), std::size_t(0));
  std::sort(order.begin(), order.end(), [&](auto a, auto b) {
    const auto &x = input[a];
    const auto &y = input[b];
    if (x.lower.x != y.lower.x)
      return x.lower.x < y.lower.x;
    return x.triangle < y.triangle;
  });
  std::vector<flat_pair_node> nodes;
  nodes.reserve(input.empty() ? 0 : 2 * input.size() - 1);
  std::function<std::size_t(std::size_t, std::size_t)> build =
      [&](std::size_t begin, std::size_t end) {
        const auto id = nodes.size();
        nodes.push_back({});
        auto &node = nodes[id];
        node.begin = begin;
        node.end = end;
        node.lower = input[order[begin]].lower;
        node.upper = input[order[begin]].upper;
        for (std::size_t i = begin + 1; i < end; ++i) {
          const auto &box = input[order[i]];
          if (box.lower.x < node.lower.x) node.lower.x = box.lower.x;
          if (box.lower.y < node.lower.y) node.lower.y = box.lower.y;
          if (box.lower.z < node.lower.z) node.lower.z = box.lower.z;
          if (node.upper.x < box.upper.x) node.upper.x = box.upper.x;
          if (node.upper.y < box.upper.y) node.upper.y = box.upper.y;
          if (node.upper.z < box.upper.z) node.upper.z = box.upper.z;
        }
        if (end - begin > 1) {
          const auto middle = begin + (end - begin) / 2;
          const auto left = build(begin, middle);
          const auto right = build(middle, end);
          nodes[id].left = left;
          nodes[id].right = right;
        }
        return id;
      };
  if (!input.empty())
    build(0, input.size());
  auto boxes_overlap = [&](const flat_pair_node &a, const flat_pair_node &b) {
    return overlap(a.lower.x, a.upper.x, b.lower.x, b.upper.x) &&
           overlap(a.lower.y, a.upper.y, b.lower.y, b.upper.y) &&
           overlap(a.lower.z, a.upper.z, b.lower.z, b.upper.z);
  };
  auto exact_boxes_overlap = [&](const realization_domain_box &a,
                                 const realization_domain_box &b) {
    return overlap(a.lower.x, a.upper.x, b.lower.x, b.upper.x) &&
           overlap(a.lower.y, a.upper.y, b.lower.y, b.upper.y) &&
           overlap(a.lower.z, a.upper.z, b.lower.z, b.upper.z);
  };
  std::vector<std::pair<std::size_t, std::size_t>> stack;
  if (!nodes.empty())
    stack.push_back({0, 0});
  std::vector<realization_triangle_pair> pairs;
  std::uint64_t checks = 0;
  if (limited)
    *limited = false;
  const auto checked_overlap = [&](const flat_pair_node &a,
                                   const flat_pair_node &b,
                                   bool &limit_hit) {
    if (checks == max_overlap_checks) {
      limit_hit = true;
      return false;
    }
    ++checks;
    return boxes_overlap(a, b);
  };
  while (!stack.empty()) {
    auto pair = stack.back();
    stack.pop_back();
    const auto &a = nodes[pair.first];
    const auto &b = nodes[pair.second];
    if (pair.first == pair.second) {
      if (a.leaf())
        continue;
      stack.push_back({a.right, a.right});
      stack.push_back({a.left, a.right});
      stack.push_back({a.left, a.left});
      continue;
    }
    bool limit_hit = false;
    if (!checked_overlap(a, b, limit_hit)) {
      if (limit_hit) {
        if (limited) *limited = true;
        if (overlap_checks) *overlap_checks = checks;
        return {};
      }
      continue;
    }
    if (a.leaf() && b.leaf()) {
      const auto &x = input[order[a.begin]];
      const auto &y = input[order[b.begin]];
      if (!exact_boxes_overlap(x, y))
        continue;
      if (pairs.size() == max_candidates) {
        if (limited) *limited = true;
        if (overlap_checks) *overlap_checks = checks;
        return {};
      }
      const auto ordered = std::minmax(x.triangle, y.triangle);
      pairs.push_back({ordered.first, ordered.second});
      continue;
    }
    const bool split_a = !a.leaf() &&
                         (b.leaf() || a.end - a.begin >= b.end - b.begin);
    if (split_a) {
      stack.push_back({a.right, pair.second});
      stack.push_back({a.left, pair.second});
    } else {
      stack.push_back({pair.first, b.right});
      stack.push_back({pair.first, b.left});
    }
  }
  std::sort(pairs.begin(), pairs.end(), [](const auto &a, const auto &b) {
    return std::tie(a.lower, a.upper) < std::tie(b.lower, b.upper);
  });
  pairs.erase(std::unique(pairs.begin(), pairs.end(), [](const auto &a, const auto &b) {
                return a.lower == b.lower && a.upper == b.upper;
              }),
              pairs.end());
  if (overlap_checks)
    *overlap_checks = checks;
  return pairs;
}

} // namespace detail
} // namespace mesh_boolean
} // namespace ygor
