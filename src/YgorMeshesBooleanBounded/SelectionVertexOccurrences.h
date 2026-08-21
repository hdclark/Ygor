#pragma once

#include "SelectionTypes.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Component 10 vertex/event occurrence construction. Each local port has
// degree exactly two: one face-corner arc and one edge-mate arc. Closed
// alternating cycles in this graph are exactly the future output vertex
// occurrences. Distinct cycles remain distinct even when their bounded
// coordinate references are bit-identical.
namespace selection_vertex_occurrences {

enum class link_status : std::uint8_t {
  success = 1,
  degree_error = 2,
  open_path = 3,
  branched = 4,
  repeated_port = 5,
  arc_reuse = 6,
};

// One undirected arc connecting two ports.
struct link_arc final {
  std::uint64_t first_port = selection_invalid_ordinal;
  std::uint64_t second_port = selection_invalid_ordinal;
};

// One local port with its two incident arcs.
struct link_port final {
  std::uint64_t corner_arc = selection_invalid_ordinal;
  std::uint64_t mate_arc = selection_invalid_ordinal;
};

// A closed alternating cycle of ports (one vertex occurrence).
struct link_cycle final {
  std::vector<std::uint64_t> ports;
};

// Extract closed alternating cycles from the degree-two link graph. Every port
// must have exactly one corner arc and one mate arc; every arc must connect
// exactly two distinct ports. The result is a set of simple cycles consuming
// each port once. The traversal starts at the least port and always follows
// corner-then-mate so it is deterministic and independent of discovery order.
inline link_status extract_link_cycles(
    const std::vector<link_port> &ports,
    const std::vector<link_arc> &corner_arcs,
    const std::vector<link_arc> &mate_arcs, std::vector<link_cycle> &cycles) {
  cycles.clear();
  const std::uint64_t n = ports.size();

  // Degree-two validation.
  std::vector<std::uint8_t> corner_degree(n, 0);
  std::vector<std::uint8_t> mate_degree(n, 0);
  for (std::uint64_t i = 0; i < n; ++i) {
    const bool has_corner = ports[i].corner_arc != selection_invalid_ordinal;
    const bool has_mate = ports[i].mate_arc != selection_invalid_ordinal;
    if (!has_corner || !has_mate)
      return link_status::degree_error;
    if (ports[i].corner_arc >= corner_arcs.size() ||
        ports[i].mate_arc >= mate_arcs.size())
      return link_status::degree_error;
  }
  for (const auto &arc : corner_arcs) {
    if (arc.first_port >= n || arc.second_port >= n ||
        arc.first_port == arc.second_port)
      return link_status::branched;
    corner_degree[arc.first_port]++;
    corner_degree[arc.second_port]++;
  }
  for (const auto &arc : mate_arcs) {
    if (arc.first_port >= n || arc.second_port >= n ||
        arc.first_port == arc.second_port)
      return link_status::branched;
    mate_degree[arc.first_port]++;
    mate_degree[arc.second_port]++;
  }
  for (std::uint64_t i = 0; i < n; ++i)
    if (corner_degree[i] != 1 || mate_degree[i] != 1)
      return link_status::degree_error;

  // Build the arc -> other-port resolution.
  auto other_port = [&](const link_arc &arc, std::uint64_t port) {
    return arc.first_port == port ? arc.second_port : arc.first_port;
  };

  // Every port participates in exactly one alternating (corner-then-mate)
  // cycle. Because the corner/mate adjacency is a permutation on ports, each
  // connected component of the link graph is exactly one closed cycle.
  std::vector<bool> visited(n, false);
  for (std::uint64_t start = 0; start < n; ++start) {
    if (visited[start])
      continue;
    link_cycle cycle;
    std::uint64_t current = start;
    bool follow_corner = true;
    for (;;) {
      if (visited[current]) {
        if (current == start)
          break; // completed this closed cycle
        return link_status::repeated_port;
      }
      visited[current] = true;
      cycle.ports.push_back(current);
      if (follow_corner) {
        const auto &corner = corner_arcs[ports[current].corner_arc];
        const auto next = other_port(corner, current);
        if (next == current)
          return link_status::branched;
        current = next;
        follow_corner = false;
      } else {
        const auto &mate = mate_arcs[ports[current].mate_arc];
        const auto next = other_port(mate, current);
        if (next == current)
          return link_status::branched;
        current = next;
        follow_corner = true;
      }
      if (current == start)
        break;
    }
    cycles.push_back(std::move(cycle));
  }
  return link_status::success;
}

} // namespace selection_vertex_occurrences

} // namespace ygor::mesh_boolean::bounded
