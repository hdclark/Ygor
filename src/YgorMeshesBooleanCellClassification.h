#pragma once
#ifndef YGOR_MESHES_BOOLEAN_CELL_CLASSIFICATION_H_
#define YGOR_MESHES_BOOLEAN_CELL_CLASSIFICATION_H_
#include "YgorMeshesBooleanGlobalArrangement.h"

namespace ygor { namespace mesh_boolean {
constexpr std::uint64_t labeled_arrangement_type_tag = 0x5947424c41423039ULL;
constexpr std::uint16_t labeled_arrangement_schema = 9;

enum class operand_location_kind : std::uint8_t { outside, inside };
enum class classification_crossing_kind : std::uint8_t { individual, coincident };
enum class classification_transfer_kind : std::uint8_t { operand_a, operand_b, both };
enum class classification_conflict_kind : std::uint8_t {
  seed_transfer,
  competing_path,
  reverse_arc,
  cycle_composition,
  coincidence_member,
  exterior_attachment
};
enum class classification_path_step_kind : std::uint8_t {
  root_seed,
  directed_transfer,
  direct_seed
};

struct classification_path_step {
  classification_path_step_kind kind = classification_path_step_kind::root_seed;
  classification_region_id from, to;
  std::optional<classification_transition_id> transition;
  seed_certificate_id seed;
  occupancy_pair before, after;
};
struct classification_conflict_path {
  classification_region_id root, terminal;
  std::vector<classification_path_step> steps;
  occupancy_pair terminal_label;
};
struct classification_conflict_certificate {
  std::uint16_t schema = 1;
  classification_conflict_kind kind = classification_conflict_kind::seed_transfer;
  classification_conflict_path established, competing;
  digest replay_digest;
};

struct operand_location_evidence {
  operand_location_kind location = operand_location_kind::outside;
  std::int32_t signed_degree = 0;
  std::vector<facet_id> boundary_sources;
  digest evidence_digest;
};
struct operand_ray_evidence {
  std::uint8_t direction_index = 0;
  exact_vector3 direction;
  std::int32_t signed_degree = 0;
  operand_location_kind location = operand_location_kind::outside;
  std::vector<formal_ray_hit> hits;
  digest evidence_digest;
};
struct seed_classification_certificate {
  seed_certificate_id id;
  open_region_component_id source_component;
  probe_base_stratum_kind base_kind = probe_base_stratum_kind::universe;
  std::uint64_t base_id = 0;
  operand_location_evidence operand_a, operand_b;
  operand_ray_evidence operand_a_primary, operand_a_alternate;
  operand_ray_evidence operand_b_primary, operand_b_alternate;
};
struct classification_region {
  classification_region_id id;
  open_region_component_id source_component;
  occupancy_pair label;
  seed_certificate_id seed;
  std::vector<patch_side_id> patch_sides;
};
struct classification_propagation_record {
  propagation_path_id id;
  classification_region_id region, root;
  std::optional<classification_region_id> predecessor;
  std::optional<classification_transition_id> transition;
  occupancy_pair propagated;
};
struct classification_arc_check {
  classification_transition_id transition;
  propagation_path_id from_path, to_path;
  bool tree_edge = false;
  occupancy_pair transferred;
};
struct classification_cycle_check {
  classification_transition_id closing_transition;
  propagation_path_id from_path, to_path;
  classification_region_id root;
  occupancy_pair from_label, to_label;
};
struct classification_transition {
  classification_transition_id id, reverse;
  side_transition_id source;
  classification_region_id from, to;
  patch_side_id from_side, to_side;
  classification_crossing_kind crossing = classification_crossing_kind::individual;
  classification_transfer_kind transfer = classification_transfer_kind::operand_a;
  occupancy_pair before, after;
  std::vector<sheet_use_id> uses;
  std::optional<coincident_group_id> coincidence;
};
struct patch_side_label {
  patch_side_label_id id;
  patch_side_id source_side;
  global_patch_id patch;
  patch_plane_side side = patch_plane_side::negative;
  classification_region_id region;
  occupancy_pair occupancy;
};
struct global_patch_side_labels {
  global_patch_id patch;
  patch_side_label_id negative, positive;
  occupancy_pair negative_occupancy, positive_occupancy;
};
struct exterior_attachment_hit {
  global_patch_id patch;
  exact_scalar parameter;
  point_region_kind relation = point_region_kind::outside;
  patch_side_id witness_facing_side;
};
struct cell_classification_certificate {
  std::uint64_t regions = 0, seeds = 0, directed_transitions = 0, side_labels = 0;
  classification_region_id exterior_region;
  std::optional<patch_side_id> exterior_attachment_side;
  std::optional<global_patch_id> exterior_target_patch;
  std::optional<exact_point3> exterior_target_witness;
  std::vector<exterior_attachment_hit> exterior_first_hits;
  exact_point3 exterior_witness;
  operand_ray_evidence exterior_operand_a, exterior_operand_b;
  bool exterior_bound_disjoint = false;
  digest semantic_digest;
};

template<class T, class I> struct labeled_arrangement {
  context_owner_token owner;
  digest setup_digest, arrangement_digest, arrangement_semantic_digest,
      validated_digest, artifact_digest;
  std::shared_ptr<const published_artifact<arrangement_complex<T,I>>> arrangement;
  std::shared_ptr<const published_artifact<validated_operands<T,I>>> validated;
  std::shared_ptr<const construction_storage> constructions;
  std::vector<classification_region> regions;
  std::vector<seed_classification_certificate> seeds;
  std::vector<classification_transition> transitions;
  std::vector<classification_propagation_record> propagation;
  std::vector<classification_arc_check> arc_checks;
  std::vector<classification_cycle_check> cycle_checks;
  std::vector<patch_side_label> side_labels;
  std::vector<global_patch_side_labels> patch_labels;
  cell_classification_certificate certificate;
  std::vector<std::uint8_t> canonical_bytes, artifact_bytes;
  const global_patch_side_labels& labels(global_patch_id id) const { return patch_labels.at(id.value_for_debug()); }
};

status_or<bool> register_cell_classification_verifier(verifier_registry&, coordinate_tag, index_tag);
template<class T,class I> status_or<std::shared_ptr<const published_artifact<labeled_arrangement<T,I>>>> classify_arrangement_cells(boolean_context<T,I>&);
#define YGOR_CLASSIFICATION_EXTERN(T,I) extern template status_or<std::shared_ptr<const published_artifact<labeled_arrangement<T,I>>>> classify_arrangement_cells(boolean_context<T,I>&)
YGOR_CLASSIFICATION_EXTERN(float,std::uint32_t);
YGOR_CLASSIFICATION_EXTERN(float,std::uint64_t);
YGOR_CLASSIFICATION_EXTERN(double,std::uint32_t);
YGOR_CLASSIFICATION_EXTERN(double,std::uint64_t);
#undef YGOR_CLASSIFICATION_EXTERN
} }
#endif
