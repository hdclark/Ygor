#include "YgorMeshesBooleanContract.h"
#include "YgorMeshesBooleanPerformance.h"
#include "External/MD5/md5.h"
#include "YgorMeshesBooleanExecutor.h"
#include <algorithm>
#include <cfenv>
#include <climits>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

#if defined(__FAST_MATH__)
#error "Robust Boolean contract must not be compiled with fast-math"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Robust Boolean contract must not assume finite-only arithmetic"
#endif

namespace ygor {
namespace mesh_boolean {
context_owner_token make_context_owner_token() {
  static std::atomic<std::uint64_t> next{1};
  auto v = next.fetch_add(1);
  if (v == 0 || v == std::numeric_limits<std::uint64_t>::max())
    throw std::overflow_error("Boolean context owner sequence exhausted");
  return context_owner_token(v);
}
std::string digest::hex() const {
  std::ostringstream os;
  os << std::hex << std::setfill('0');
  for (auto b : bytes)
    os << std::setw(2) << unsigned(b);
  return os.str();
}
boolean_error make_error(boolean_error_code c, boolean_stage s, std::string key,
                         std::uint32_t sub) {
  boolean_error e;
  e.code = c;
  e.stage = s;
  e.message_key = std::move(key);
  e.subcode = sub;
  return e;
}
std::string render_error(const boolean_error &e) {
  std::ostringstream os;
  os << "boolean_error[" << unsigned(e.code) << ':' << unsigned(e.stage) << ':'
     << e.subcode << "] " << e.message_key;
  if (!e.detail.empty())
    os << ": " << e.detail;
  return os.str();
}
void canonical_encoder::u16(std::uint16_t v) {
  byte(v >> 8);
  byte(v);
}
void canonical_encoder::u32(std::uint32_t v) {
  for (int s = 24; s >= 0; s -= 8)
    byte(v >> s);
}
void canonical_encoder::u64(std::uint64_t v) {
  for (int s = 56; s >= 0; s -= 8)
    byte(v >> s);
}
void canonical_encoder::signed_magnitude(std::int64_t v) {
  boolean(v < 0);
  std::uint64_t m = v < 0 ? std::uint64_t(-(v + 1)) + 1 : std::uint64_t(v);
  u64(m);
}
void canonical_encoder::raw(const std::uint8_t *p, std::size_t n) {
  if (n)
    bytes_.insert(bytes_.end(), p, p + n);
}
digest md5_digest(const std::uint8_t *p, std::size_t n) {
  MD5::Context c;
  MD5::Init(&c);
  if (n)
    MD5::Update(&c, p, n);
  digest d;
  MD5::Final(d.bytes.data(), &c);
  return d;
}
digest domain_digest(const std::array<char, 8> &tag,
                     const std::vector<std::uint8_t> &payload) {
  MD5::Context c;
  MD5::Init(&c);
  MD5::Update(&c, tag.data(), tag.size());
  if (!payload.empty())
    MD5::Update(&c, payload.data(), payload.size());
  digest d;
  MD5::Final(d.bytes.data(), &c);
  return d;
}

static bool valid_limit(const resource_limit &l) {
  return l.unlimited ? l.value == 0 : l.value != 0;
}
static std::vector<const resource_limit *>
resource_limits(const resource_policy &r) {
  return {&r.authoritative_bytes,
          &r.stage_private_bytes,
          &r.work_units,
          &r.entities_per_store,
          &r.candidates,
          &r.raw_events,
          &r.symbolic_vertices,
          &r.symbolic_curves,
          &r.local_vertices,
          &r.local_atomic_edges,
          &r.local_halfedges,
          &r.local_boundary_walks,
          &r.local_faces,
          &r.local_patches,
          &r.local_certificate_entries,
          &r.reconciliation_requests,
          &r.successor_generations,
          &r.global_vertices,
          &r.global_atomic_edges,
          &r.global_halfedges,
          &r.global_patches,
          &r.source_sheet_members,
          &r.sheet_uses,
          &r.seams,
          &r.seam_sectors,
          &r.source_edge_sectors,
          &r.coincident_memberships,
          &r.side_nodes,
          &r.vertex_occurrences,
          &r.vertex_sectors,
          &r.link_rays,
          &r.link_arcs,
          &r.link_regions,
          &r.side_transitions,
          &r.probe_descriptors,
          &r.mapping_entries,
          &r.arrangement_certificate_entries,
          &r.planar_scratch,
          &r.radial_scratch,
          &r.link_scratch,
          &r.cells,
          &r.classification_transitions,
          &r.seed_certificates,
          &r.patch_side_labels,
          &r.propagation_records,
          &r.selection_decisions,
          &r.selected_patches,
          &r.selected_cycles,
          &r.selected_halfedges,
          &r.selected_edges,
          &r.selected_vertices,
          &r.selected_vertex_occurrences,
          &r.topology_obstructions,
          &r.selection_provenance,
          &r.exact_number_bits,
          &r.diagnostic_records,
          &r.diagnostic_bytes,
          &r.trace_records,
           &r.trace_bytes,
           &r.realization_attempts,
           &r.realization_graph_nodes,
           &r.realization_graph_edges,
           &r.realization_components,
           &r.realization_pair_boxes,
           &r.realization_pair_candidates,
           &r.realization_pair_checks,
           &r.realization_solver_trail,
           &r.realization_component_transcripts,
           &r.realization_verifier_witnesses,
          &r.output_vertices,
          &r.output_faces,
          &r.output_face_indices,
          &r.output_involved_entries,
          &r.output_components,
          &r.output_mappings,
          &r.output_certificate_entries,
          &r.output_canonical_bytes,
          &r.verifier_work,
          &r.verifier_scratch_bytes,
          &r.evidence_records,
          &r.evidence_bytes,
          &r.report_bytes,
          &r.dependency_nodes,
          &r.dependency_edges,
          &r.replay_bytes,
          &r.minimization_work};
}
status_or<bool> validate_options(const boolean_options &o) {
  auto bad = [](const char *k) {
    return status_or<bool>(make_error(boolean_error_code::input_contract_error,
                                      boolean_stage::context_setup, k));
  };
  if (static_cast<unsigned>(o.solids) > 0 ||
      static_cast<unsigned>(o.determinism) > 0 ||
      static_cast<unsigned>(o.verification) > 1 ||
      static_cast<unsigned>(o.tracing.level) > 3 ||
      static_cast<unsigned>(o.classification.strategy) > 0 ||
      o.classification.schema != 1 ||
      o.classification.probe_formula_version != 1 ||
      static_cast<unsigned>(o.realization.semantics) > 0 ||
      static_cast<unsigned>(o.realization.strategy) > 1 ||
      static_cast<unsigned>(o.realization.original_coordinates) > 0 ||
      static_cast<unsigned>(o.realization.topology) > 0 ||
      static_cast<unsigned>(o.realization.pair_certification) > 0 ||
      static_cast<unsigned>(o.realization.certificate_level) > 0 ||
        o.realization.schema != 2 || o.realization.solver_version != 2 ||
       static_cast<unsigned>(o.result_topology) > 0 ||
       static_cast<unsigned>(o.output.topology) > 0 || o.output.schema != 1 ||
      o.output.ordering_version != 1 || o.output.encoding_version != 1)
    return bad("unknown_option_enum");
  if (o.output.include_compact_provenance)
    return bad("output.compact_provenance_not_implemented");
  if (!o.execution.max_threads)
    return bad("execution.max_threads");
  if (!o.execution.max_queued_tasks)
    return bad("execution.max_queued_tasks");
  if (o.realization.strategy != realization_strategy::neighboring_values &&
      o.realization.neighboring_value_radius)
    return bad("realization.neighboring_value_radius");
  for (const auto *limit : resource_limits(o.resources))
    if (!valid_limit(*limit))
      return bad("resources.invalid_limit");
  if (o.tracing.level == trace_level::off &&
      (!o.resources.trace_records.unlimited ||
       !o.resources.trace_bytes.unlimited))
    return bad("tracing.off_with_budget");
  return true;
}
static void encode_limit(canonical_encoder &e, const resource_limit &l) {
  e.boolean(l.unlimited);
  e.u64(l.value);
}
status_or<std::vector<std::uint8_t>> encode_options(const boolean_options &o) {
  auto ok = validate_options(o);
  if (!ok.has_value())
    return ok.error();
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(o.solids));
  e.byte(static_cast<std::uint8_t>(o.determinism));
  e.byte(static_cast<std::uint8_t>(o.verification));
  e.u32(o.execution.max_threads);
  e.u32(o.execution.max_queued_tasks);
  e.byte(static_cast<std::uint8_t>(o.tracing.level));
  e.boolean(o.tracing.collect_noncanonical_timings);
  e.u16(o.classification.schema);
  e.u16(o.classification.probe_formula_version);
  e.byte(static_cast<std::uint8_t>(o.classification.strategy));
  e.u16(o.realization.schema);
  e.u16(o.realization.solver_version);
  e.byte(static_cast<std::uint8_t>(o.realization.semantics));
  e.byte(static_cast<std::uint8_t>(o.realization.strategy));
  e.byte(static_cast<std::uint8_t>(o.realization.original_coordinates));
  e.byte(static_cast<std::uint8_t>(o.realization.topology));
  e.byte(static_cast<std::uint8_t>(o.realization.pair_certification));
  e.byte(static_cast<std::uint8_t>(o.realization.certificate_level));
  e.u32(o.realization.neighboring_value_radius);
  e.byte(static_cast<std::uint8_t>(o.result_topology));
  e.u16(o.output.schema);
  e.u16(o.output.ordering_version);
  e.u16(o.output.encoding_version);
  e.byte(static_cast<std::uint8_t>(o.output.topology));
  e.boolean(o.output.include_compact_provenance);
  e.boolean(o.diagnostics.forward_to_ygor_logger);
  for (const auto *limit : resource_limits(o.resources))
    encode_limit(e, *limit);
  return e.bytes();
}
status_or<std::uint64_t> checked_add(std::uint64_t a, std::uint64_t b,
                                     boolean_stage s) {
  if (b > std::numeric_limits<std::uint64_t>::max() - a) {
    auto e =
        make_error(boolean_error_code::resource_limit, s, "integer_overflow");
    e.requested = b;
    e.current = a;
    return e;
  }
  return a + b;
}
status_or<std::uint64_t> checked_multiply(std::uint64_t a, std::uint64_t b,
                                          boolean_stage s) {
  if (a && b > std::numeric_limits<std::uint64_t>::max() / a) {
    auto e =
        make_error(boolean_error_code::resource_limit, s, "integer_overflow");
    e.requested = b;
    e.current = a;
    return e;
  }
  return a * b;
}
resource_limit resource_accountant::limit_for(resource_kind k) const {
  switch (k) {
  case resource_kind::authoritative_bytes:
    return limits_.authoritative_bytes;
  case resource_kind::stage_private_bytes:
    return limits_.stage_private_bytes;
  case resource_kind::work_units:
    return limits_.work_units;
  case resource_kind::entities:
    return limits_.entities_per_store;
  case resource_kind::candidates:
    return limits_.candidates;
  case resource_kind::raw_events:
    return limits_.raw_events;
  case resource_kind::symbolic_vertices:
    return limits_.symbolic_vertices;
  case resource_kind::symbolic_curves:
    return limits_.symbolic_curves;
  case resource_kind::local_patches:
    return limits_.local_patches;
  case resource_kind::global_halfedges:
    return limits_.global_halfedges;
  case resource_kind::global_patches:
    return limits_.global_patches;
  case resource_kind::cells:
    return limits_.cells;
  case resource_kind::classification_transitions:
    return limits_.classification_transitions;
  case resource_kind::seed_certificates:
    return limits_.seed_certificates;
  case resource_kind::patch_side_labels:
    return limits_.patch_side_labels;
  case resource_kind::propagation_records:
    return limits_.propagation_records;
  case resource_kind::selection_decisions:
    return limits_.selection_decisions;
  case resource_kind::selected_patches:
    return limits_.selected_patches;
  case resource_kind::selected_cycles:
    return limits_.selected_cycles;
  case resource_kind::selected_halfedges:
    return limits_.selected_halfedges;
  case resource_kind::selected_edges:
    return limits_.selected_edges;
  case resource_kind::selected_vertices:
    return limits_.selected_vertices;
  case resource_kind::selected_vertex_occurrences:
    return limits_.selected_vertex_occurrences;
  case resource_kind::topology_obstructions:
    return limits_.topology_obstructions;
  case resource_kind::selection_provenance:
    return limits_.selection_provenance;
  case resource_kind::exact_number_bits:
    return limits_.exact_number_bits;
  case resource_kind::diagnostic_records:
    return limits_.diagnostic_records;
  case resource_kind::diagnostic_bytes:
    return limits_.diagnostic_bytes;
  case resource_kind::trace_records:
    return limits_.trace_records;
  case resource_kind::trace_bytes:
    return limits_.trace_bytes;
  case resource_kind::realization_attempts:
    return limits_.realization_attempts;
  case resource_kind::realization_graph_nodes:
    return limits_.realization_graph_nodes;
  case resource_kind::realization_graph_edges:
    return limits_.realization_graph_edges;
  case resource_kind::realization_components:
    return limits_.realization_components;
  case resource_kind::realization_pair_boxes:
    return limits_.realization_pair_boxes;
  case resource_kind::realization_pair_candidates:
    return limits_.realization_pair_candidates;
  case resource_kind::realization_pair_checks:
    return limits_.realization_pair_checks;
  case resource_kind::realization_solver_trail:
    return limits_.realization_solver_trail;
  case resource_kind::realization_component_transcripts:
    return limits_.realization_component_transcripts;
  case resource_kind::realization_verifier_witnesses:
    return limits_.realization_verifier_witnesses;
  case resource_kind::output_vertices:
    return limits_.output_vertices;
  case resource_kind::output_faces:
    return limits_.output_faces;
  case resource_kind::output_face_indices:
    return limits_.output_face_indices;
  case resource_kind::output_involved_entries:
    return limits_.output_involved_entries;
  case resource_kind::output_components:
    return limits_.output_components;
  case resource_kind::output_mappings:
    return limits_.output_mappings;
  case resource_kind::output_certificate_entries:
    return limits_.output_certificate_entries;
  case resource_kind::output_canonical_bytes:
    return limits_.output_canonical_bytes;
  case resource_kind::verifier_work:
    return limits_.verifier_work;
  case resource_kind::verifier_scratch_bytes:
    return limits_.verifier_scratch_bytes;
  case resource_kind::evidence_records:
    return limits_.evidence_records;
  case resource_kind::evidence_bytes:
    return limits_.evidence_bytes;
  case resource_kind::report_bytes:
    return limits_.report_bytes;
  case resource_kind::dependency_nodes:
    return limits_.dependency_nodes;
  case resource_kind::dependency_edges:
    return limits_.dependency_edges;
  case resource_kind::replay_bytes:
    return limits_.replay_bytes;
  case resource_kind::minimization_work:
    return limits_.minimization_work;
  default:
    return resource_limit{};
  }
}
status_or<bool> resource_accountant::reserve(resource_kind k, std::uint64_t n,
                                             boolean_stage s) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto i = static_cast<std::size_t>(k);
  auto sum = checked_add(used_[i], n, s);
  if (!sum.has_value())
    return sum.error();
  auto lim = limit_for(k);
  switch (k) {
  case resource_kind::local_vertices:
    lim = limits_.local_vertices;
    break;
  case resource_kind::local_atomic_edges:
    lim = limits_.local_atomic_edges;
    break;
  case resource_kind::local_halfedges:
    lim = limits_.local_halfedges;
    break;
  case resource_kind::local_boundary_walks:
    lim = limits_.local_boundary_walks;
    break;
  case resource_kind::local_faces:
    lim = limits_.local_faces;
    break;
  case resource_kind::local_certificate_entries:
    lim = limits_.local_certificate_entries;
    break;
  case resource_kind::reconciliation_requests:
    lim = limits_.reconciliation_requests;
    break;
  case resource_kind::successor_generations:
    lim = limits_.successor_generations;
    break;
  case resource_kind::global_vertices:
    lim = limits_.global_vertices;
    break;
  case resource_kind::global_atomic_edges:
    lim = limits_.global_atomic_edges;
    break;
  case resource_kind::source_sheet_members:
    lim = limits_.source_sheet_members;
    break;
  case resource_kind::sheet_uses:
    lim = limits_.sheet_uses;
    break;
  case resource_kind::seams:
    lim = limits_.seams;
    break;
  case resource_kind::seam_sectors:
    lim = limits_.seam_sectors;
    break;
  case resource_kind::source_edge_sectors:
    lim = limits_.source_edge_sectors;
    break;
  case resource_kind::coincident_memberships:
    lim = limits_.coincident_memberships;
    break;
  case resource_kind::side_nodes:
    lim = limits_.side_nodes;
    break;
  case resource_kind::vertex_occurrences:
    lim = limits_.vertex_occurrences;
    break;
  case resource_kind::vertex_sectors:
    lim = limits_.vertex_sectors;
    break;
  case resource_kind::link_rays:
    lim = limits_.link_rays;
    break;
  case resource_kind::link_arcs:
    lim = limits_.link_arcs;
    break;
  case resource_kind::link_regions:
    lim = limits_.link_regions;
    break;
  case resource_kind::side_transitions:
    lim = limits_.side_transitions;
    break;
  case resource_kind::probe_descriptors:
    lim = limits_.probe_descriptors;
    break;
  case resource_kind::mapping_entries:
    lim = limits_.mapping_entries;
    break;
  case resource_kind::arrangement_certificate_entries:
    lim = limits_.arrangement_certificate_entries;
    break;
  case resource_kind::planar_scratch:
    lim = limits_.planar_scratch;
    break;
  case resource_kind::radial_scratch:
    lim = limits_.radial_scratch;
    break;
  case resource_kind::link_scratch:
    lim = limits_.link_scratch;
    break;
  default:
    break;
  }
  if (!lim.unlimited && sum.value() > lim.value) {
    auto e =
        make_error(boolean_error_code::resource_limit, s, "resource_limit");
    e.current = used_[i];
    e.requested = n;
    e.limit = lim.value;
    return e;
  }
  used_[i] = sum.value();
  performance_count_resource(k, n);
  return true;
}
status_or<resource_reservation>
resource_accountant::reserve_scoped(resource_kind k, std::uint64_t n,
                                    boolean_stage s) {
  auto ok = reserve(k, n, s);
  if (!ok.has_value())
    return ok.error();
  return resource_reservation(*this, k, n);
}
void resource_accountant::release(resource_kind k, std::uint64_t n) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto &i = used_[static_cast<std::size_t>(k)];
  i = n > i ? 0 : i - n;
}
std::uint64_t resource_accountant::used(resource_kind k) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return used_[static_cast<std::size_t>(k)];
}
std::uint64_t
artifact_generation_catalog::latest_generation(artifact_slot slot) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return latest_[static_cast<std::size_t>(slot)].generation;
}
std::shared_ptr<const void>
artifact_generation_catalog::latest(artifact_slot slot) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return latest_[static_cast<std::size_t>(slot)].artifact;
}
status_or<std::uint64_t> artifact_generation_catalog::compare_and_publish(
    artifact_slot slot, std::uint64_t expected, context_owner_token owner,
    std::shared_ptr<const void> artifact) {
  if (owner != owner_ || !artifact)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::context_setup,
                      "artifact_publication_binding");
  std::lock_guard<std::mutex> lock(mutex_);
  auto &e = latest_[static_cast<std::size_t>(slot)];
  if (e.generation != expected)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::context_setup, "stale_artifact_writer");
  if (e.generation == std::numeric_limits<std::uint64_t>::max())
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::context_setup,
                      "artifact_generation_exhaustion");
  e.artifact = std::move(artifact);
  return ++e.generation;
}
resource_reservation::resource_reservation(resource_accountant &a,
                                           resource_kind k, std::uint64_t n)
    : accountant_(&a), kind_(k), amount_(n) {}
resource_reservation::resource_reservation(resource_reservation &&o) noexcept
    : accountant_(o.accountant_), kind_(o.kind_), amount_(o.amount_),
      committed_(o.committed_) {
  o.accountant_ = nullptr;
}
resource_reservation &resource_reservation::
operator=(resource_reservation &&o) noexcept {
  if (this != &o) {
    rollback();
    accountant_ = o.accountant_;
    kind_ = o.kind_;
    amount_ = o.amount_;
    committed_ = o.committed_;
    o.accountant_ = nullptr;
  }
  return *this;
}
resource_reservation::~resource_reservation() { rollback(); }
void resource_reservation::rollback() noexcept {
  if (accountant_ && !committed_)
    accountant_->release(kind_, amount_);
  accountant_ = nullptr;
}

template <class T> static bool runtime_preserves_subnormals() noexcept {
  using bits_type = typename std::conditional<std::is_same<T, float>::value,
                                               std::uint32_t,
                                               std::uint64_t>::type;
  volatile T smallest_normal = std::numeric_limits<T>::min();
  volatile T smallest_subnormal = std::numeric_limits<T>::denorm_min();
  volatile T two = T(2);
  volatile T half_normal = smallest_normal / two;
  volatile T twice_subnormal = smallest_subnormal * two;
  const T half_copy = half_normal;
  const T twice_copy = twice_subnormal;
  bits_type half_bits = 0, twice_bits = 0;
  std::memcpy(&half_bits, &half_copy, sizeof(T));
  std::memcpy(&twice_bits, &twice_copy, sizeof(T));
  const bits_type expected_half = bits_type(1)
                                  << (std::numeric_limits<T>::digits - 2);
  return half_bits == expected_half && twice_bits == bits_type(2);
}

template <class T, class I> static platform_facts capture_platform() {
  std::uint16_t x = 1;
  platform_facts p{};
  p.coordinate = std::is_same<T, float>::value ? coordinate_tag::binary32
                                               : coordinate_tag::binary64;
  p.index = std::is_same<I, std::uint32_t>::value ? index_tag::uint32
                                                  : index_tag::uint64;
  p.coordinate_bytes = sizeof(T);
  p.index_bytes = sizeof(I);
  p.char_bits = CHAR_BIT;
  p.endian = *reinterpret_cast<std::uint8_t *>(&x) ? endian_tag::little
                                                   : endian_tag::big;
  p.uint32_bytes = sizeof(std::uint32_t);
  p.uint64_bytes = sizeof(std::uint64_t);
  p.radix = std::numeric_limits<T>::radix;
  p.digits = std::numeric_limits<T>::digits;
  p.min_exponent = std::numeric_limits<T>::min_exponent;
  p.max_exponent = std::numeric_limits<T>::max_exponent;
  p.iec559 = std::numeric_limits<T>::is_iec559;
  p.has_subnormals = std::numeric_limits<T>::has_denorm != std::denorm_absent;
  p.fast_math = false;
  p.finite_math_only = false;
  p.rounding = rounding_tag::nearest;
  return p;
}
template <class T, class I>
static std::pair<digest, std::uint64_t>
input_digest(const fv_surface_mesh<T, I> &m, std::uint8_t role) {
  canonical_encoder e;
  e.byte(role);
  e.byte(sizeof(T));
  e.byte(sizeof(I));
  e.u64(m.vertices.size());
  for (const auto &v : m.vertices) {
    e.floating(v.x);
    e.floating(v.y);
    e.floating(v.z);
  }
  e.u64(m.faces.size());
  for (const auto &f : m.faces) {
    e.u64(f.size());
    for (I i : f)
      e.u64(i);
  }
  return {domain_digest({{'Y', 'G', 'B', 'I', 'N', 'P', '0', '1'}}, e.bytes()),
          e.bytes().size()};
}
template <class T, class I>
boolean_context<T, I>::boolean_context(
    const fv_surface_mesh<T, I> &a, const fv_surface_mesh<T, I> &b,
    operation op, boolean_options opts, platform_facts pf, replay_descriptor rd,
    context_owner_token own, std::shared_ptr<const exact_kernel_services<T>> k,
    std::shared_ptr<const verifier_service> v, cancellation_source *c,
    diagnostic_consumer d, deterministic_executor_factory executor_factory)
    : a_(&a), b_(&b), contract_(op), options_(std::move(opts)), platform_(pf),
      replay_(std::move(rd)), owner_(own), kernel_(std::move(k)),
      verifiers_(std::move(v)), accountant_(options_.resources),
      artifacts_(owner_), caller_cancel_(c), consumer_(std::move(d)) {
  executor_ = executor_factory
                  ? executor_factory(options_.execution)
                  : std::make_unique<deterministic_executor>(options_.execution);
  if (!executor_)
    throw std::invalid_argument("executor factory returned null");
  const auto level = options_.tracing.level;
  if (options_.tracing.collect_noncanonical_timings ||
      level == trace_level::stages || level == trace_level::full) {
    try {
      performance_ = std::make_shared<performance_collector>();
    } catch (...) {
      // Optional diagnostics never make an otherwise valid context fail.
    }
  }
}
template <class T, class I> boolean_context<T, I>::~boolean_context() = default;
template <class T, class I>
std::shared_ptr<const performance_snapshot>
boolean_context<T, I>::performance() const {
  if (performance_)
    return performance_->snapshot();
  return std::make_shared<const performance_snapshot>();
}
template <
    class T, class I,
    typename std::enable_if<is_supported_boolean_types<T, I>::value, int>::type>
status_or<std::unique_ptr<boolean_context<T, I>>>
make_boolean_context(const fv_surface_mesh<T, I> &a,
                     const fv_surface_mesh<T, I> &b, operation op,
                     const boolean_options &o,
                     std::shared_ptr<const exact_kernel_services<T>> k,
                     std::shared_ptr<const verifier_service> v,
                     cancellation_source *c, diagnostic_consumer d,
                     deterministic_executor_factory executor_factory) {
  auto valid = validate_options(o);
  if (!valid.has_value())
    return valid.error();
  if (static_cast<unsigned>(op) > 4)
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup, "operation");
  if (!k || !v)
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup, "required_service");
  if (c && c->token().cancelled())
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::context_setup, "cancelled");
  auto p = capture_platform<T, I>();
  if (CHAR_BIT != 8 || !p.iec559 || p.radix != 2 ||
      std::fegetround() != FE_TONEAREST)
    return make_error(boolean_error_code::unsupported_platform,
                      boolean_stage::context_setup, "platform");
  if (!p.has_subnormals || !runtime_preserves_subnormals<T>())
    return make_error(boolean_error_code::unsupported_platform,
                      boolean_stage::context_setup, "subnormal_mode");
  if (k->coordinate_type() != p.coordinate)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::context_setup, "kernel_type");
  auto ia = input_digest(a, 0), ib = input_digest(b, 1);
  auto ob = encode_options(o);
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(op));
  e.byte_string(ob.value());
  e.raw(ia.first.bytes.data(), 16);
  e.u64(ia.second);
  e.raw(ib.first.bytes.data(), 16);
  e.u64(ib.second);
  auto kb = k->arithmetic_policy_bytes();
  e.byte_string(kb);
  replay_descriptor rd{
      ia.first,
      ib.first,
      domain_digest({{'Y', 'G', 'B', 'S', 'E', 'T', '0', '1'}}, e.bytes()),
      ia.second,
      ib.second,
      op,
      o,
      p};
  try {
    return std::unique_ptr<boolean_context<T, I>>(new boolean_context<T, I>(
        a, b, op, o, p, std::move(rd), make_context_owner_token(), std::move(k),
        std::move(v), c, std::move(d), std::move(executor_factory)));
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::context_setup, "allocation");
  } catch (const std::exception &x) {
    auto er = make_error(boolean_error_code::internal_invariant_error,
                         boolean_stage::context_setup, "context_exception");
    er.detail = x.what();
    return er;
  }
}
#define INST(T, I)                                                             \
  template class boolean_context<T, I>;                                        \
  template status_or<std::unique_ptr<boolean_context<T, I>>>                   \
  make_boolean_context<T, I, 0>(                                               \
      const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &, operation, \
      const boolean_options &,                                                 \
      std::shared_ptr<const exact_kernel_services<T>>,                         \
      std::shared_ptr<const verifier_service>, cancellation_source *,          \
      diagnostic_consumer, deterministic_executor_factory)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST
} // namespace mesh_boolean
} // namespace ygor
