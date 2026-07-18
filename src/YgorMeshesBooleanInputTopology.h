#pragma once
#ifndef YGOR_MESHES_BOOLEAN_INPUT_TOPOLOGY_H_
#define YGOR_MESHES_BOOLEAN_INPUT_TOPOLOGY_H_
#include "YgorMeshesBooleanTransaction.h"
#include "YgorMeshesBooleanVerification.h"
#include "YgorMeshesExactKernel.h"
#include <functional>
#include <optional>

namespace ygor { namespace mesh_boolean {

namespace input_topology_detail {
struct facet_sweep_entry {
  exact_box3 bounds;
  operand_id operand;
};
struct facet_sweep_counters {
  std::uint64_t sort_work = 0;
  std::uint64_t build_nodes = 0;
  std::uint64_t index_visits = 0;
  std::uint64_t leaf_tests = 0;
  std::uint64_t conservative_candidates = 0;
};
struct facet_sweep_result {
  std::vector<std::pair<std::size_t, std::size_t>> candidates;
  facet_sweep_counters counters;
};
facet_sweep_result
verifier_facet_sweep_candidates(const std::vector<facet_sweep_entry> &entries);

template <class T, class Less = std::less<T>>
std::size_t minimal_cyclic_rotation(const std::vector<T> &values,
                                    Less less = Less{}) {
  if (values.empty()) return 0;
  const std::size_t n = values.size();
  std::size_t first = 0, second = 1, matched = 0;
  while (first < n && second < n && matched < n) {
    const auto &a = values[(first + matched) % n];
    const auto &b = values[(second + matched) % n];
    if (!less(a, b) && !less(b, a)) {
      ++matched;
      continue;
    }
    if (less(b, a)) {
      first += matched + 1;
      if (first == second) ++first;
    } else {
      second += matched + 1;
      if (first == second) ++second;
    }
    matched = 0;
  }
  return std::min(first, second) % n;
}
} // namespace input_topology_detail

constexpr std::uint64_t validated_operands_type_tag=0x5947425641543032ULL; // YGBVAT02
constexpr std::uint16_t validated_operands_schema=1;
enum class raw_vertex_disposition:std::uint8_t{retained,unused_removed};
enum class input_validation_subcode:std::uint32_t{nonfinite_coordinate=1,index_out_of_range=2,short_ring=3,repeated_vertex=4,zero_length_edge=5,degenerate_facet=6,nonplanar_facet=7,boundary_edge=8,nonmanifold_edge=9,same_direction_uses=10,disconnected_vertex_link=11,orientation_mismatch=12,self_intersection=13,shell_contact=14,ambiguous_nesting=15,duplicate_facet=16};
enum class shell_orientation:std::uint8_t{outward,inward};
enum class shell_contribution:std::uint8_t{material_boundary,cavity_boundary};
enum class validation_evidence_kind:std::uint8_t{coordinate_scan,facet_audit,triangulation,edge_pairing,vertex_links,embeddedness,nesting};
struct validation_evidence{std::uint16_t schema=1;validation_evidence_kind kind=validation_evidence_kind::coordinate_scan;operand_id operand;std::uint64_t checked=0;digest evidence_digest;};

template<class T>struct source_vertex_provenance{operand_id operand;std::uint64_t raw_vertex_ordinal=0;raw_vertex_disposition disposition=raw_vertex_disposition::unused_removed;std::optional<original_vertex_id>canonical_vertex;std::array<coordinate_bits<T>,3>raw_bits;exact_point3 exact_coordinate;};
struct validated_edge_use{edge_use_id id;operand_id operand;facet_id facet;std::uint64_t ring_offset=0;original_vertex_id origin,destination;edge_use_id previous,next,twin;undirected_edge_id edge;shell_id shell;};
struct validated_edge{undirected_edge_id id;operand_id operand;original_vertex_id first,second;std::array<edge_use_id,2>uses;shell_id shell;};
struct validated_facet{facet_id id;operand_id operand;std::vector<original_vertex_id>ring;std::vector<edge_use_id>edge_uses;exact_plane3 plane;projection_axis projection=projection_axis::drop_z;exact_scalar projected_double_area;std::vector<std::array<original_vertex_id,3>>triangles;std::vector<facet_id>neighbors;shell_id shell;exact_box3 bounds;std::uint64_t raw_face_ordinal=0;};
struct validated_edge_geometry{exact_segment3 segment;exact_box3 bounds;exact_box2 projected_bounds;};
struct validated_triangle_geometry{exact_triangle3 triangle;exact_box3 bounds;};
struct validated_facet_geometry{
    facet_id facet;operand_id operand;exact_plane3 plane;
    projection_axis projection=projection_axis::drop_z;exact_vector3 oriented_normal;
    std::vector<exact_point3>ring3;std::vector<exact_point2>ring2;
    std::vector<validated_edge_geometry>edges;
    std::vector<validated_triangle_geometry>triangles;
    std::vector<std::vector<facet_id>>vertex_fans;
};
template<class T>struct validated_vertex{original_vertex_id id;operand_id operand;std::array<T,3>raw_coordinate;std::array<coordinate_bits<T>,3>raw_bits;exact_point3 exact_coordinate;shell_id shell;std::vector<edge_use_id>ordered_outgoing_link;};
struct validated_shell{shell_id id;operand_id operand;std::vector<facet_id>facets;std::vector<undirected_edge_id>edges;std::vector<original_vertex_id>vertices;exact_scalar oriented_six_volume;exact_box3 bounds;std::optional<shell_id>parent;std::vector<shell_id>children;std::uint32_t depth=0;shell_orientation orientation=shell_orientation::outward;shell_contribution contribution=shell_contribution::material_boundary;};
struct validated_operand{operand_id operand;std::vector<original_vertex_id>vertices;std::vector<facet_id>facets;std::vector<shell_id>shells;std::uint64_t raw_vertex_count=0,raw_face_count=0;std::optional<exact_box3>bounds;digest semantic_digest;};

template<class T,class I>struct validated_operands{
    context_owner_token owner;digest setup_digest,kernel_policy_digest,artifact_digest;
    std::array<validated_operand,2>operands;
    std::vector<source_vertex_provenance<T>>provenance;
    std::vector<validated_vertex<T>>vertices;
    std::vector<validated_facet>facets;
    std::vector<validated_edge_use>edge_uses;
    std::vector<validated_edge>edges;
    std::vector<validated_shell>shells;
    std::vector<validation_evidence>evidence;
    // Derived immutable geometry and lookup metadata. These are intentionally
    // excluded from canonical serialization and checked against authoritative
    // records by the Component 2 verifier.
    std::vector<validated_facet_geometry>facet_geometry;
    std::array<std::vector<std::uint64_t>,2>raw_vertex_provenance;
    std::array<std::vector<facet_id>,2>raw_facets;
};

status_or<bool>register_input_topology_verifier(verifier_registry&,coordinate_tag,index_tag);
template<class T,class I>status_or<std::shared_ptr<const published_artifact<validated_operands<T,I>>>>validate_operands(boolean_context<T,I>&);

extern template status_or<std::shared_ptr<const published_artifact<validated_operands<float,std::uint32_t>>>>validate_operands(boolean_context<float,std::uint32_t>&);
extern template status_or<std::shared_ptr<const published_artifact<validated_operands<float,std::uint64_t>>>>validate_operands(boolean_context<float,std::uint64_t>&);
extern template status_or<std::shared_ptr<const published_artifact<validated_operands<double,std::uint32_t>>>>validate_operands(boolean_context<double,std::uint32_t>&);
extern template status_or<std::shared_ptr<const published_artifact<validated_operands<double,std::uint64_t>>>>validate_operands(boolean_context<double,std::uint64_t>&);

} }
#endif
