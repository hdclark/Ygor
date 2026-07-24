#include "YgorMeshesBooleanExactResult.h"
#include "YgorMeshesBooleanExactResultInternal.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> record_tag{
    {'Y', 'G', 'B', 'E', 'X', 'R', '0', '1'}};
constexpr std::array<char, 8> digest_tag{
    {'Y', 'G', 'B', 'E', 'X', 'D', '0', '1'}};
constexpr std::array<char, 8> exact_export_record_tag{
    {'Y', 'G', 'B', 'E', 'X', 'C', '0', '1'}};
constexpr std::array<char, 8> exact_export_digest_tag{
    {'Y', 'G', 'B', 'E', 'C', 'D', '0', '1'}};
constexpr std::uint16_t feature_reference_schema = 1;

product_error error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

struct decode_failure : std::runtime_error {
  product_error_code code;
  explicit decode_failure(
      const char *key, product_error_code failure_code =
                           product_error_code::exact_result_serialization_error)
      : std::runtime_error(key), code(failure_code) {}
};

class reader {
  const std::vector<std::uint8_t> &bytes_;
  const exact_result_decode_limits &limits_;
  std::size_t at_ = 0;
  std::uint64_t entities_ = 0, references_ = 0, exact_text_ = 0,
                string_text_ = 0;

public:
  reader(const std::vector<std::uint8_t> &b,
         const exact_result_decode_limits &limits)
      : bytes_(b), limits_(limits) {
    if (b.size() > limits.max_record_bytes)
      throw decode_failure("exact_result.record_limit",
                           product_error_code::resource_limit);
  }
  std::uint8_t byte() {
    if (at_ == bytes_.size())
      throw decode_failure("exact_result.truncated");
    return bytes_[at_++];
  }
  bool boolean() {
    const auto v = byte();
    if (v > 1)
      throw decode_failure("exact_result.invalid_boolean");
    return v != 0;
  }
  std::uint16_t u16() {
    std::uint16_t v = 0;
    for (unsigned i = 0; i != 2; ++i)
      v = std::uint16_t((v << 8) | byte());
    return v;
  }
  std::uint64_t u64() {
    std::uint64_t v = 0;
    for (unsigned i = 0; i != 8; ++i)
      v = (v << 8) | byte();
    return v;
  }
  void raw(std::uint8_t *out, std::size_t n) {
    if (n > bytes_.size() - at_)
      throw decode_failure("exact_result.truncated");
    std::memcpy(out, bytes_.data() + at_, n);
    at_ += n;
  }
  std::string string(bool exact = false) {
    const auto n = u64();
    const auto limit =
        exact ? limits_.max_exact_hex_bytes : limits_.max_string_bytes;
    auto &total = exact ? exact_text_ : string_text_;
    if (n > limit || total > limit - n || n > bytes_.size() - at_)
      throw decode_failure(exact ? "exact_result.exact_text_limit"
                                 : "exact_result.string_limit",
                           product_error_code::resource_limit);
    total += n;
    std::string s(reinterpret_cast<const char *>(bytes_.data() + at_),
                  static_cast<std::size_t>(n));
    at_ += static_cast<std::size_t>(n);
    return s;
  }
  std::vector<std::uint8_t> byte_string() {
    const auto n = count(false);
    if (n > bytes_.size() - at_)
      throw decode_failure("exact_result.truncated");
    std::vector<std::uint8_t> v(bytes_.begin() + at_, bytes_.begin() + at_ + n);
    at_ += static_cast<std::size_t>(n);
    return v;
  }
  std::uint64_t count(bool entity = true) {
    const auto n = u64();
    auto &total = entity ? entities_ : references_;
    const auto limit = entity ? limits_.max_entities : limits_.max_references;
    if (n > limit || total > limit - n ||
        n > std::numeric_limits<std::size_t>::max())
      throw decode_failure(entity ? "exact_result.entity_limit"
                                  : "exact_result.reference_limit",
                           product_error_code::resource_limit);
    total += n;
    return n;
  }
  bool done() const { return at_ == bytes_.size(); }
};

template <class E> void en(canonical_encoder &e, E v) {
  e.byte(static_cast<std::uint8_t>(v));
}
template <class E> E de(reader &r, std::uint8_t maximum) {
  const auto v = r.byte();
  if (v > maximum)
    throw decode_failure("exact_result.unknown_enum");
  return static_cast<E>(v);
}
template <class Id> void id(canonical_encoder &e, Id v) { e.id(v); }
template <class Id> Id id(reader &r) {
  const auto v = r.u64();
  if (v == Id::invalid_value)
    throw decode_failure("exact_result.invalid_id");
  return Id::from_canonical_value(v);
}
void dg(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}
digest dg(reader &r) {
  digest d;
  r.raw(d.bytes.data(), d.bytes.size());
  return d;
}

template <class T, class F>
void vec(canonical_encoder &e, const std::vector<T> &v, F f) {
  e.u64(v.size());
  for (const auto &x : v)
    f(e, x);
}
template <class T, class F>
std::vector<T> vec(reader &r, F f, bool entity = false) {
  const auto n = r.count(entity);
  std::vector<T> v;
  v.reserve(static_cast<std::size_t>(n));
  for (std::uint64_t i = 0; i != n; ++i)
    v.push_back(f(r));
  return v;
}
template <class T, class F>
void opt(canonical_encoder &e, const std::optional<T> &v, F f) {
  e.boolean(bool(v));
  if (v)
    f(e, *v);
}
template <class T, class F> std::optional<T> opt(reader &r, F f) {
  if (!r.boolean())
    return std::nullopt;
  return f(r);
}
template <class Id> void ids(canonical_encoder &e, const std::vector<Id> &v) {
  vec(e, v, [](canonical_encoder &x, Id y) { id(x, y); });
}
template <class Id> std::vector<Id> ids(reader &r) {
  return vec<Id>(r, [](reader &x) { return id<Id>(x); });
}

void scalar(canonical_encoder &e, const exact_scalar &x) {
  e.byte(static_cast<std::uint8_t>(
      static_cast<std::int8_t>(x.numerator().sign())));
  e.string(x.numerator().magnitude().to_hex());
  e.string(x.denominator().to_hex());
}
exact_scalar scalar(reader &r) {
  const auto sb = r.byte();
  if (sb != 0 && sb != 1 && sb != 255)
    throw decode_failure("exact_result.integer_sign");
  const auto nh = r.string(true), dh = r.string(true);
  auto n = big_uint::from_hex(nh, boolean_stage::boolean_selection);
  auto d = big_uint::from_hex(dh, boolean_stage::boolean_selection);
  if (!n.has_value() || !d.has_value() || d.value().is_zero())
    throw decode_failure("exact_result.invalid_exact");
  const auto sign = static_cast<integer_sign>(static_cast<std::int8_t>(sb));
  if ((n.value().is_zero() && sign != integer_sign::zero) ||
      (!n.value().is_zero() && sign == integer_sign::zero))
    throw decode_failure("exact_result.noncanonical_exact");
  return exact_scalar(big_int(sign, std::move(n.value())),
                      std::move(d.value()));
}
void point(canonical_encoder &e, const exact_point3 &p) {
  scalar(e, p.x);
  scalar(e, p.y);
  scalar(e, p.z);
}
exact_point3 point(reader &r) { return {scalar(r), scalar(r), scalar(r)}; }
void vector3(canonical_encoder &e, const exact_vector3 &p) {
  scalar(e, p.x);
  scalar(e, p.y);
  scalar(e, p.z);
}
exact_vector3 vector3(reader &r) { return {scalar(r), scalar(r), scalar(r)}; }
void line(canonical_encoder &e, const exact_line3 &x) {
  point(e, x.anchor);
  vector3(e, x.direction);
}
exact_line3 line(reader &r) { return {point(r), vector3(r)}; }
void plane(canonical_encoder &e, const exact_plane3 &p) {
  auto bi = [&](const big_int &x) {
    e.byte(static_cast<std::uint8_t>(static_cast<std::int8_t>(x.sign())));
    e.string(x.magnitude().to_hex());
  };
  bi(p.a);
  bi(p.b);
  bi(p.c);
  bi(p.d);
  en(e, p.oriented);
}
big_int bigint(reader &r) {
  const auto sb = r.byte();
  if (sb != 0 && sb != 1 && sb != 255)
    throw decode_failure("exact_result.integer_sign");
  auto m = big_uint::from_hex(r.string(true), boolean_stage::boolean_selection);
  if (!m.has_value())
    throw decode_failure("exact_result.invalid_exact");
  auto s = static_cast<integer_sign>(static_cast<std::int8_t>(sb));
  if ((m.value().is_zero()) != (s == integer_sign::zero))
    throw decode_failure("exact_result.noncanonical_exact");
  return big_int(s, std::move(m.value()));
}
exact_plane3 plane(reader &r) {
  auto a = bigint(r), b = bigint(r), c = bigint(r), d = bigint(r);
  auto o = de<orientation_parity>(r, 255);
  if (o != orientation_parity::agree && o != orientation_parity::opposite)
    throw decode_failure("exact_result.unknown_enum");
  return {std::move(a), std::move(b), std::move(c), std::move(d), o};
}
void interval(canonical_encoder &e, const exact_interval &x) {
  scalar(e, x.lower);
  scalar(e, x.upper);
  e.boolean(x.lower_closed);
  e.boolean(x.upper_closed);
}
exact_interval interval(reader &r) {
  return {scalar(r), scalar(r), r.boolean(), r.boolean()};
}

void feature(canonical_encoder &e, const exact_feature_reference &f) {
  e.u16(f.kind);
  e.u64(f.primary);
  e.u64(f.secondary);
}
exact_feature_reference feature(reader &r) {
  exact_feature_reference f{r.u16(), r.u64(), r.u64()};
  if (f.kind >= std::variant_size<feature_ref>::value)
    throw decode_failure("exact_result.feature_kind");
  if (f.primary == std::numeric_limits<std::uint64_t>::max())
    throw decode_failure("exact_result.feature_id");
  if (f.kind != 62 && f.kind != 63 && f.secondary)
    throw decode_failure("exact_result.feature_secondary");
  return f;
}
void original_vertex(canonical_encoder &e, const original_vertex_ref &v) {
  id(e, v.operand);
  id(e, v.vertex);
}
original_vertex_ref original_vertex(reader &r) {
  return {id<operand_id>(r), id<original_vertex_id>(r)};
}

template <class T> bool ordered_ids(const std::vector<T> &v) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (!v[i].id.valid() || v[i].id.value_for_debug() != i)
      return false;
  return true;
}
template <class Id>
bool refs(const std::vector<Id> &v, std::size_t n, bool strict = false) {
  Id prior;
  for (std::size_t i = 0; i < v.size(); ++i) {
    if (!v[i].valid() || v[i].value_for_debug() >= n ||
        (strict && i && !(prior < v[i])))
      return false;
    prior = v[i];
  }
  return true;
}
template <class Id> bool strict_ids(const std::vector<Id> &v) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (!v[i].valid() || (i && !(v[i - 1] < v[i])))
      return false;
  return true;
}

void backend(canonical_encoder &e, const exact_result_backend_binding &b) {
  e.u16(b.schema);
  e.u16(static_cast<std::uint16_t>(b.producer.id));
  e.u16(b.producer.schema);
  e.u16(b.producer.adapter_version.major);
  e.u16(b.producer.adapter_version.minor);
  e.u16(b.producer.adapter_version.patch);
  e.string(b.producer.build_identifier);
  e.u16(b.producer.capabilities.schema);
  e.u64(b.producer.capabilities.bits);
  dg(e, b.producer.capability_digest);
  en(e, b.producer.maturity);
  en(e, b.selection);
  e.boolean(b.fallback_used);
  vec(e, b.attempted_backends, [](canonical_encoder &x, backend_id y) {
    x.u16(static_cast<std::uint16_t>(y));
  });
  opt(e, b.primary_failure, [](canonical_encoder &x, product_error_code y) {
    x.u16(static_cast<std::uint16_t>(y));
  });
}
exact_result_backend_binding backend(reader &r) {
  exact_result_backend_binding b;
  b.schema = r.u16();
  b.producer.id = static_cast<backend_id>(r.u16());
  b.producer.schema = r.u16();
  b.producer.adapter_version = {r.u16(), r.u16(), r.u16()};
  b.producer.build_identifier = r.string();
  b.producer.capabilities.schema = r.u16();
  b.producer.capabilities.bits = r.u64();
  b.producer.capability_digest = dg(r);
  b.producer.maturity = de<backend_maturity>(r, 3);
  b.selection = de<backend_selection_mode>(r, 3);
  b.fallback_used = r.boolean();
  b.attempted_backends = vec<backend_id>(r, [](reader &x) {
    auto v = x.u16();
    if (v != 1)
      throw decode_failure("exact_result.backend_id");
    return static_cast<backend_id>(v);
  });
  b.primary_failure = opt<product_error_code>(r, [](reader &x) {
    auto v = x.u16();
    if (v >
        static_cast<std::uint16_t>(product_error_code::verifier_disagreement))
      throw decode_failure("exact_result.error_code");
    return static_cast<product_error_code>(v);
  });
  return b;
}
void preparation(canonical_encoder &e,
                 const exact_result_preparation_binding &p) {
  e.u16(p.schema);
  en(e, p.mode);
  dg(e, p.input_digest);
  dg(e, p.prepared_digest);
  dg(e, p.policy_digest);
  dg(e, p.report_digest);
  e.boolean(p.geometry_changed);
}
exact_result_preparation_binding preparation(reader &r) {
  exact_result_preparation_binding p;
  p.schema = r.u16();
  p.mode = de<preparation_mode>(r, 2);
  p.input_digest = dg(r);
  p.prepared_digest = dg(r);
  p.policy_digest = dg(r);
  p.report_digest = dg(r);
  p.geometry_changed = r.boolean();
  return p;
}

void occupancy(canonical_encoder &e, const occupancy_pair &o) {
  e.boolean(o.in_a);
  e.boolean(o.in_b);
}
occupancy_pair occupancy(reader &r) { return {r.boolean(), r.boolean()}; }
void decision(canonical_encoder &e, const patch_selection_decision &d) {
  id(e, d.id);
  id(e, d.patch);
  id(e, d.negative_label);
  id(e, d.positive_label);
  occupancy(e, d.negative_occupancy);
  occupancy(e, d.positive_occupancy);
  e.boolean(d.result_negative);
  e.boolean(d.result_positive);
  en(e, d.kind);
  en(e, d.orientation);
  opt(e, d.selected,
      [](canonical_encoder &x, selected_patch_id y) { id(x, y); });
  opt(e, d.representative,
      [](canonical_encoder &x, sheet_use_id y) { id(x, y); });
  ids(e, d.provenance);
}
patch_selection_decision decision(reader &r) {
  patch_selection_decision d;
  d.id = id<patch_selection_decision_id>(r);
  d.patch = id<global_patch_id>(r);
  d.negative_label = id<patch_side_label_id>(r);
  d.positive_label = id<patch_side_label_id>(r);
  d.negative_occupancy = occupancy(r);
  d.positive_occupancy = occupancy(r);
  d.result_negative = r.boolean();
  d.result_positive = r.boolean();
  d.kind = de<patch_decision_kind>(r, 3);
  d.orientation = de<selected_orientation>(r, 2);
  d.selected = opt<selected_patch_id>(
      r, [](reader &x) { return id<selected_patch_id>(x); });
  d.representative =
      opt<sheet_use_id>(r, [](reader &x) { return id<sheet_use_id>(x); });
  d.provenance = ids<sheet_use_id>(r);
  return d;
}
void side_label(canonical_encoder &e, const patch_side_label &label) {
  id(e, label.id);
  id(e, label.source_side);
  id(e, label.patch);
  en(e, label.side);
  id(e, label.region);
  occupancy(e, label.occupancy);
}
patch_side_label side_label(reader &r) {
  patch_side_label label;
  label.id = id<patch_side_label_id>(r);
  label.source_side = id<patch_side_id>(r);
  label.patch = id<global_patch_id>(r);
  label.side = de<patch_plane_side>(r, 1);
  label.region = id<classification_region_id>(r);
  label.occupancy = occupancy(r);
  return label;
}

void vertex(canonical_encoder &e, const exact_result_vertex &v) {
  id(e, v.id);
  id(e, v.source);
  id(e, v.symbolic);
  point(e, v.coordinate);
  vec(e, v.original_vertices,
      [](canonical_encoder &x, const original_vertex_ref &y) {
        original_vertex(x, y);
      });
  ids(e, v.constructions);
}
exact_result_vertex vertex(reader &r) {
  exact_result_vertex v;
  v.id = id<selected_vertex_id>(r);
  v.source = id<global_vertex_id>(r);
  v.symbolic = id<symbolic_vertex_id>(r);
  v.coordinate = point(r);
  v.original_vertices =
      vec<original_vertex_ref>(r, [](reader &x) { return original_vertex(x); });
  v.constructions = ids<construction_node_id>(r);
  return v;
}
void occurrence(canonical_encoder &e, const selected_vertex_occurrence &o) {
  id(e, o.id);
  id(e, o.vertex);
  ids(e, o.incident_halfedges);
}
selected_vertex_occurrence occurrence(reader &r) {
  selected_vertex_occurrence o;
  o.id = id<selected_vertex_occurrence_id>(r);
  o.vertex = id<selected_vertex_id>(r);
  o.incident_halfedges = ids<selected_halfedge_id>(r);
  return o;
}
void edge(canonical_encoder &e, const selected_edge &x) {
  id(e, x.id);
  id(e, x.source);
  id(e, x.lower);
  id(e, x.upper);
  ids(e, x.uses);
}
selected_edge edge(reader &r) {
  selected_edge x;
  x.id = id<selected_edge_id>(r);
  x.source = id<global_atomic_edge_id>(r);
  x.lower = id<selected_vertex_id>(r);
  x.upper = id<selected_vertex_id>(r);
  x.uses = ids<selected_halfedge_id>(r);
  return x;
}
void edge_geom(canonical_encoder &e, const exact_result_edge_geometry &x) {
  id(e, x.edge);
  en(e, x.kind);
  ids(e, x.curves);
}
exact_result_edge_geometry edge_geom(reader &r) {
  exact_result_edge_geometry x;
  x.edge = id<selected_edge_id>(r);
  x.kind = de<global_edge_kind>(r, 3);
  x.curves = ids<symbolic_curve_id>(r);
  return x;
}
void halfedge(canonical_encoder &e, const selected_halfedge &h) {
  id(e, h.id);
  id(e, h.patch);
  id(e, h.cycle);
  id(e, h.edge);
  id(e, h.origin);
  id(e, h.destination);
  id(e, h.origin_occurrence);
  id(e, h.destination_occurrence);
  id(e, h.next);
  id(e, h.previous);
}
selected_halfedge halfedge(reader &r) {
  selected_halfedge h;
  h.id = id<selected_halfedge_id>(r);
  h.patch = id<selected_patch_id>(r);
  h.cycle = id<selected_cycle_id>(r);
  h.edge = id<selected_edge_id>(r);
  h.origin = id<selected_vertex_id>(r);
  h.destination = id<selected_vertex_id>(r);
  h.origin_occurrence = id<selected_vertex_occurrence_id>(r);
  h.destination_occurrence = id<selected_vertex_occurrence_id>(r);
  h.next = id<selected_halfedge_id>(r);
  h.previous = id<selected_halfedge_id>(r);
  return h;
}
void cycle(canonical_encoder &e, const selected_cycle &c) {
  id(e, c.id);
  id(e, c.patch);
  e.boolean(c.hole);
  ids(e, c.halfedges);
}
selected_cycle cycle(reader &r) {
  selected_cycle c;
  c.id = id<selected_cycle_id>(r);
  c.patch = id<selected_patch_id>(r);
  c.hole = r.boolean();
  c.halfedges = ids<selected_halfedge_id>(r);
  return c;
}
void patch(canonical_encoder &e, const selected_patch &p) {
  id(e, p.id);
  id(e, p.source);
  en(e, p.orientation);
  ids(e, p.cycles);
  id(e, p.representative);
  ids(e, p.provenance);
}
selected_patch patch(reader &r) {
  selected_patch p;
  p.id = id<selected_patch_id>(r);
  p.source = id<global_patch_id>(r);
  p.orientation = de<selected_orientation>(r, 2);
  p.cycles = ids<selected_cycle_id>(r);
  p.representative = id<sheet_use_id>(r);
  p.provenance = ids<sheet_use_id>(r);
  return p;
}
void patch_geom(canonical_encoder &e, const exact_result_patch_geometry &p) {
  id(e, p.patch);
  plane(e, p.plane);
  scalar(e, p.projected_double_area);
}
exact_result_patch_geometry patch_geom(reader &r) {
  exact_result_patch_geometry p;
  p.patch = id<selected_patch_id>(r);
  p.plane = plane(r);
  p.projected_double_area = scalar(r);
  return p;
}
void obstruction(canonical_encoder &e, const topology_obstruction &o) {
  id(e, o.id);
  en(e, o.kind);
  opt(e, o.vertex,
      [](canonical_encoder &x, selected_vertex_id y) { id(x, y); });
  opt(e, o.edge,
      [](canonical_encoder &x, global_atomic_edge_id y) { id(x, y); });
  vec(e, o.occurrences,
      [](canonical_encoder &x, std::uint64_t y) { x.u64(y); });
}
topology_obstruction obstruction(reader &r) {
  topology_obstruction o;
  o.id = id<topology_obstruction_id>(r);
  o.kind = de<topology_obstruction_kind>(r, 3);
  o.vertex = opt<selected_vertex_id>(
      r, [](reader &x) { return id<selected_vertex_id>(x); });
  o.edge = opt<global_atomic_edge_id>(
      r, [](reader &x) { return id<global_atomic_edge_id>(x); });
  o.occurrences = vec<std::uint64_t>(r, [](reader &x) { return x.u64(); });
  return o;
}

void curve(canonical_encoder &e, const exact_result_curve &c) {
  id(e, c.id);
  en(e, c.kind);
  line(e, c.carrier);
  opt(e, c.parent_carrier,
      [](canonical_encoder &x, symbolic_curve_id y) { id(x, y); });
  opt(e, c.parameters,
      [](canonical_encoder &x, const exact_interval &y) { interval(x, y); });
  opt(e, c.lower, [](canonical_encoder &x, symbolic_vertex_id y) { id(x, y); });
  opt(e, c.upper, [](canonical_encoder &x, symbolic_vertex_id y) { id(x, y); });
  ids(e, c.facets);
  ids(e, c.constructions);
}
exact_result_curve curve(reader &r) {
  exact_result_curve c;
  c.id = id<symbolic_curve_id>(r);
  c.kind = de<symbolic_curve_kind>(r, 1);
  c.carrier = line(r);
  c.parent_carrier = opt<symbolic_curve_id>(
      r, [](reader &x) { return id<symbolic_curve_id>(x); });
  c.parameters = opt<exact_interval>(r, [](reader &x) { return interval(x); });
  c.lower = opt<symbolic_vertex_id>(
      r, [](reader &x) { return id<symbolic_vertex_id>(x); });
  c.upper = opt<symbolic_vertex_id>(
      r, [](reader &x) { return id<symbolic_vertex_id>(x); });
  c.facets = ids<facet_id>(r);
  c.constructions = ids<construction_node_id>(r);
  return c;
}
void construction(canonical_encoder &e, const exact_construction_record &c) {
  id(e, c.id);
  en(e, c.kind);
  ids(e, c.children);
  vec(e, c.defining_sources,
      [](canonical_encoder &x, const exact_feature_reference &f) {
        feature(x, f);
      });
  ids(e, c.defining_relations);
  e.byte_string(c.exact_result);
}
exact_construction_record construction(reader &r) {
  exact_construction_record c;
  c.id = id<construction_node_id>(r);
  c.kind = de<construction_kind>(r, 0);
  c.children = ids<construction_node_id>(r);
  c.defining_sources =
      vec<exact_feature_reference>(r, [](reader &x) { return feature(x); });
  c.defining_relations = ids<defining_relation_id>(r);
  c.exact_result = r.byte_string();
  return c;
}
void relation(canonical_encoder &e, const exact_defining_relation_record &d) {
  id(e, d.id);
  en(e, d.kind);
  e.u16(d.formula_version);
  id(e, d.construction);
  ids(e, d.operand_nodes);
  vec(e, d.defining_sources,
      [](canonical_encoder &x, const exact_feature_reference &f) {
        feature(x, f);
      });
  for (const auto &x : d.coefficients)
    scalar(e, x);
  e.byte(static_cast<std::uint8_t>(static_cast<std::int8_t>(d.expected)));
}
exact_defining_relation_record relation(reader &r) {
  exact_defining_relation_record d;
  d.id = id<defining_relation_id>(r);
  d.kind = de<defining_relation_kind>(r, 5);
  d.formula_version = r.u16();
  d.construction = id<construction_node_id>(r);
  d.operand_nodes = ids<construction_node_id>(r);
  d.defining_sources =
      vec<exact_feature_reference>(r, [](reader &x) { return feature(x); });
  for (auto &x : d.coefficients)
    x = scalar(r);
  auto s = r.byte();
  if (s != 0 && s != 1 && s != 255)
    throw decode_failure("exact_result.exact_sign");
  d.expected = static_cast<exact_sign>(static_cast<std::int8_t>(s));
  return d;
}
void contributor(canonical_encoder &e,
                 const exact_result_source_contributor &c) {
  id(e, c.use);
  id(e, c.member);
  id(e, c.operand);
  id(e, c.shell);
  id(e, c.facet);
  e.boolean(c.source_plane_agrees);
  en(e, c.occupied_side);
  e.boolean(c.representative);
}
exact_result_source_contributor contributor(reader &r) {
  exact_result_source_contributor c;
  c.use = id<sheet_use_id>(r);
  c.member = id<source_sheet_member_id>(r);
  c.operand = id<operand_id>(r);
  c.shell = id<shell_id>(r);
  c.facet = id<facet_id>(r);
  c.source_plane_agrees = r.boolean();
  c.occupied_side = de<patch_plane_side>(r, 1);
  c.representative = r.boolean();
  return c;
}
void provenance(canonical_encoder &e, const exact_result_patch_provenance &p) {
  id(e, p.patch);
  vec(e, p.contributors,
      [](canonical_encoder &x, const exact_result_source_contributor &y) {
        contributor(x, y);
      });
}
exact_result_patch_provenance provenance(reader &r) {
  exact_result_patch_provenance p;
  p.patch = id<selected_patch_id>(r);
  p.contributors = vec<exact_result_source_contributor>(
      r, [](reader &x) { return contributor(x); });
  return p;
}
void source_occ(canonical_encoder &e,
                const exact_result_source_vertex_occurrence &o) {
  id(e, o.occurrence);
  id(e, o.vertex);
  id(e, o.operand);
  id(e, o.shell);
  ids(e, o.incident_halfedges);
  ids(e, o.link_regions);
}
exact_result_source_vertex_occurrence source_occ(reader &r) {
  exact_result_source_vertex_occurrence o;
  o.occurrence = id<vertex_occurrence_id>(r);
  o.vertex = id<global_vertex_id>(r);
  o.operand = id<operand_id>(r);
  o.shell = id<shell_id>(r);
  o.incident_halfedges = ids<global_halfedge_id>(r);
  o.link_regions = ids<vertex_sector_id>(r);
  return o;
}
void ray(canonical_encoder &e, const exact_result_link_ray &x) {
  id(e, x.id);
  vector3(e, x.direction);
  id(e, x.antipode);
}
exact_result_link_ray ray(reader &r) {
  exact_result_link_ray x;
  x.id = id<link_ray_id>(r);
  x.direction = vector3(r);
  x.antipode = id<link_ray_id>(r);
  return x;
}
void arc(canonical_encoder &e, const exact_result_link_arc &a) {
  id(e, a.id);
  id(e, a.occurrence);
  id(e, a.origin);
  id(e, a.destination);
  ids(e, a.layers);
}
exact_result_link_arc arc(reader &r) {
  exact_result_link_arc a;
  a.id = id<link_arc_id>(r);
  a.occurrence = id<vertex_occurrence_id>(r);
  a.origin = id<link_ray_id>(r);
  a.destination = id<link_ray_id>(r);
  a.layers = ids<sheet_use_id>(r);
  return a;
}
void sector(canonical_encoder &e, const exact_result_vertex_sector &s) {
  id(e, s.id);
  id(e, s.vertex);
  id(e, s.occurrence);
  id(e, s.region);
  en(e, s.germ);
  ids(e, s.boundary_rays);
  ids(e, s.boundary_arcs);
  vector3(e, s.witness_direction);
  vec(e, s.witness_evidence, [](canonical_encoder &x, exact_sign y) {
    x.byte(static_cast<std::uint8_t>(static_cast<std::int8_t>(y)));
  });
  ids(e, s.seam_continuations);
  ids(e, s.source_edge_continuations);
}
exact_result_vertex_sector sector(reader &r) {
  exact_result_vertex_sector s;
  s.id = id<vertex_sector_id>(r);
  s.vertex = id<global_vertex_id>(r);
  s.occurrence = id<vertex_occurrence_id>(r);
  s.region = id<link_region_id>(r);
  s.germ = de<vertex_germ_kind>(r, 5);
  s.boundary_rays = ids<link_ray_id>(r);
  s.boundary_arcs = ids<link_arc_id>(r);
  s.witness_direction = vector3(r);
  s.witness_evidence = vec<exact_sign>(r, [](reader &x) {
    auto v = x.byte();
    if (v != 0 && v != 1 && v != 255)
      throw decode_failure("exact_result.exact_sign");
    return static_cast<exact_sign>(static_cast<std::int8_t>(v));
  });
  s.seam_continuations = ids<seam_sector_id>(r);
  s.source_edge_continuations = ids<source_edge_sector_id>(r);
  return s;
}
void certificate(canonical_encoder &e, const boolean_selection_certificate &c) {
  id(e, c.id);
  e.u64(c.decisions);
  e.u64(c.discard_exterior);
  e.u64(c.discard_internal);
  e.u64(c.select_preserved);
  e.u64(c.select_reversed);
  e.u64(c.selected_patches);
  e.u64(c.selected_cycles);
  e.u64(c.selected_halfedges);
  e.u64(c.selected_edges);
  e.u64(c.selected_vertices);
  e.u64(c.selected_vertex_occurrences);
  e.u64(c.topology_obstructions);
  e.u64(c.provenance_uses);
  e.u64(c.connected_components);
  en(e, c.topology);
  dg(e, c.semantic_digest);
}
boolean_selection_certificate certificate(reader &r) {
  boolean_selection_certificate c;
  c.id = id<selection_certificate_id>(r);
  c.decisions = r.u64();
  c.discard_exterior = r.u64();
  c.discard_internal = r.u64();
  c.select_preserved = r.u64();
  c.select_reversed = r.u64();
  c.selected_patches = r.u64();
  c.selected_cycles = r.u64();
  c.selected_halfedges = r.u64();
  c.selected_edges = r.u64();
  c.selected_vertices = r.u64();
  c.selected_vertex_occurrences = r.u64();
  c.topology_obstructions = r.u64();
  c.provenance_uses = r.u64();
  c.connected_components = r.u64();
  c.topology = de<selected_boundary_topology>(r, 2);
  c.semantic_digest = dg(r);
  return c;
}

void payload(canonical_encoder &e, const exact_stratified_boundary &a) {
  e.raw(reinterpret_cast<const std::uint8_t *>(record_tag.data()),
        record_tag.size());
  e.u16(a.schema);
  e.u16(exact_stratified_boundary_checker_version);
  e.u16(feature_reference_schema);
  en(e, a.selected_operation);
  en(e, a.topology);
  backend(e, a.backend);
  preparation(e, a.preparation);
  dg(e, a.setup_digest);
  dg(e, a.labeled_digest);
  dg(e, a.arrangement_digest);
  dg(e, a.selected_artifact_digest);
  dg(e, a.selected_semantic_digest);
  vec(e, a.decisions, [](auto &x, const auto &y) { decision(x, y); });
  vec(e, a.side_labels, [](auto &x, const auto &y) { side_label(x, y); });
  vec(e, a.vertices, [](auto &x, const auto &y) { vertex(x, y); });
  vec(e, a.vertex_occurrences,
      [](auto &x, const auto &y) { occurrence(x, y); });
  vec(e, a.edges, [](auto &x, const auto &y) { edge(x, y); });
  vec(e, a.edge_geometry, [](auto &x, const auto &y) { edge_geom(x, y); });
  vec(e, a.halfedges, [](auto &x, const auto &y) { halfedge(x, y); });
  vec(e, a.cycles, [](auto &x, const auto &y) { cycle(x, y); });
  vec(e, a.patches, [](auto &x, const auto &y) { patch(x, y); });
  vec(e, a.patch_geometry, [](auto &x, const auto &y) { patch_geom(x, y); });
  vec(e, a.topology_obstructions,
      [](auto &x, const auto &y) { obstruction(x, y); });
  vec(e, a.curves, [](auto &x, const auto &y) { curve(x, y); });
  vec(e, a.constructions, [](auto &x, const auto &y) { construction(x, y); });
  vec(e, a.defining_relations, [](auto &x, const auto &y) { relation(x, y); });
  vec(e, a.provenance, [](auto &x, const auto &y) { provenance(x, y); });
  vec(e, a.source_vertex_occurrences,
      [](auto &x, const auto &y) { source_occ(x, y); });
  vec(e, a.link_rays, [](auto &x, const auto &y) { ray(x, y); });
  vec(e, a.link_arcs, [](auto &x, const auto &y) { arc(x, y); });
  vec(e, a.vertex_sectors, [](auto &x, const auto &y) { sector(x, y); });
  certificate(e, a.certificate);
}
exact_stratified_boundary payload(reader &r) {
  std::array<std::uint8_t, 8> magic{};
  r.raw(magic.data(), magic.size());
  if (!std::equal(magic.begin(), magic.end(), record_tag.begin()))
    throw decode_failure("exact_result.magic");
  exact_stratified_boundary a;
  a.schema = r.u16();
  if (a.schema != exact_stratified_boundary_schema ||
      r.u16() != exact_stratified_boundary_checker_version ||
      r.u16() != feature_reference_schema)
    throw decode_failure("exact_result.schema");
  a.selected_operation = de<operation>(r, 4);
  a.topology = de<selected_boundary_topology>(r, 2);
  a.backend = backend(r);
  a.preparation = preparation(r);
  a.setup_digest = dg(r);
  a.labeled_digest = dg(r);
  a.arrangement_digest = dg(r);
  a.selected_artifact_digest = dg(r);
  a.selected_semantic_digest = dg(r);
  a.decisions = vec<patch_selection_decision>(
      r, [](auto &x) { return decision(x); }, true);
  a.side_labels =
      vec<patch_side_label>(r, [](auto &x) { return side_label(x); }, true);
  a.vertices =
      vec<exact_result_vertex>(r, [](auto &x) { return vertex(x); }, true);
  a.vertex_occurrences = vec<selected_vertex_occurrence>(
      r, [](auto &x) { return occurrence(x); }, true);
  a.edges = vec<selected_edge>(r, [](auto &x) { return edge(x); }, true);
  a.edge_geometry = vec<exact_result_edge_geometry>(
      r, [](auto &x) { return edge_geom(x); }, true);
  a.halfedges =
      vec<selected_halfedge>(r, [](auto &x) { return halfedge(x); }, true);
  a.cycles = vec<selected_cycle>(r, [](auto &x) { return cycle(x); }, true);
  a.patches = vec<selected_patch>(r, [](auto &x) { return patch(x); }, true);
  a.patch_geometry = vec<exact_result_patch_geometry>(
      r, [](auto &x) { return patch_geom(x); }, true);
  a.topology_obstructions = vec<topology_obstruction>(
      r, [](auto &x) { return obstruction(x); }, true);
  a.curves = vec<exact_result_curve>(r, [](auto &x) { return curve(x); }, true);
  a.constructions = vec<exact_construction_record>(
      r, [](auto &x) { return construction(x); }, true);
  a.defining_relations = vec<exact_defining_relation_record>(
      r, [](auto &x) { return relation(x); }, true);
  a.provenance = vec<exact_result_patch_provenance>(
      r, [](auto &x) { return provenance(x); }, true);
  a.source_vertex_occurrences = vec<exact_result_source_vertex_occurrence>(
      r, [](auto &x) { return source_occ(x); }, true);
  a.link_rays =
      vec<exact_result_link_ray>(r, [](auto &x) { return ray(x); }, true);
  a.link_arcs =
      vec<exact_result_link_arc>(r, [](auto &x) { return arc(x); }, true);
  a.vertex_sectors = vec<exact_result_vertex_sector>(
      r, [](auto &x) { return sector(x); }, true);
  a.certificate = certificate(r);
  return a;
}

digest computed_digest(const exact_stratified_boundary &a) {
  canonical_encoder e;
  payload(e, a);
  return domain_digest(digest_tag, e.bytes());
}
bool nonzero(const digest &d) {
  return std::any_of(d.bytes.begin(), d.bytes.end(),
                     [](std::uint8_t x) { return x; });
}

product_status_or<bool> validate_impl(const exact_stratified_boundary &a) {
  auto fail = [](const char *k) {
    return product_status_or<bool>(
        error(product_error_code::exact_result_serialization_error, k));
  };
  if (a.schema != exact_stratified_boundary_schema ||
      a.selected_operation > operation::symmetric_difference ||
      a.topology > selected_boundary_topology::closed_stratified_nonmanifold)
    return fail("exact_result.schema_or_enum");
  if (a.backend.schema != product_contract_schema_version ||
      a.preparation.schema != product_contract_schema_version ||
      a.preparation.mode > preparation_mode::normalized ||
      (a.preparation.mode == preparation_mode::strict_validation &&
       a.preparation.geometry_changed) ||
      a.backend.attempted_backends.empty() ||
      a.backend.attempted_backends.back() != a.backend.producer.id ||
      (a.backend.fallback_used && a.backend.attempted_backends.size() < 2) ||
      a.backend.primary_failure.has_value() != a.backend.fallback_used)
    return fail("exact_result.binding_schema");
  auto bi = validate_backend_identity(a.backend.producer);
  if (!bi.has_value() || !bi.value())
    return fail("exact_result.backend_binding");
  if (!nonzero(a.setup_digest) || !nonzero(a.labeled_digest) ||
      !nonzero(a.arrangement_digest) || !nonzero(a.selected_artifact_digest) ||
      !nonzero(a.selected_semantic_digest) ||
      !nonzero(a.preparation.input_digest) ||
      !nonzero(a.preparation.prepared_digest) ||
      !nonzero(a.preparation.policy_digest) ||
      !nonzero(a.preparation.report_digest))
    return fail("exact_result.zero_binding");
  if (!ordered_ids(a.decisions) || !ordered_ids(a.side_labels) ||
      !ordered_ids(a.vertices) || !ordered_ids(a.vertex_occurrences) ||
      !ordered_ids(a.edges) || !ordered_ids(a.halfedges) ||
      !ordered_ids(a.cycles) || !ordered_ids(a.patches) ||
      !ordered_ids(a.topology_obstructions) || !ordered_ids(a.constructions) ||
      !ordered_ids(a.defining_relations))
    return fail("exact_result.noncanonical_ids");
  if (a.edge_geometry.size() != a.edges.size() ||
      a.patch_geometry.size() != a.patches.size() ||
      a.provenance.size() != a.patches.size())
    return fail("exact_result.geometry_cardinality");
  std::set<std::uint64_t> curves, rays, arcs, sectors, source_occ;
  for (const auto &x : a.curves)
    if (!x.id.valid() || !curves.insert(x.id.value_for_debug()).second)
      return fail("exact_result.curve_order");
  auto increasing = [](auto const &v) {
    for (std::size_t i = 1; i < v.size(); ++i)
      if (!(v[i - 1].id < v[i].id))
        return false;
    return true;
  };
  if (!increasing(a.curves) || !increasing(a.link_rays) ||
      !increasing(a.link_arcs) || !increasing(a.vertex_sectors))
    return fail("exact_result.noncanonical_order");
  for (std::size_t i = 1; i < a.source_vertex_occurrences.size(); ++i)
    if (!(a.source_vertex_occurrences[i - 1].occurrence <
          a.source_vertex_occurrences[i].occurrence))
      return fail("exact_result.noncanonical_order");
  for (const auto &x : a.source_vertex_occurrences)
    source_occ.insert(x.occurrence.value_for_debug());
  for (const auto &x : a.link_rays)
    rays.insert(x.id.value_for_debug());
  for (const auto &x : a.link_arcs)
    arcs.insert(x.id.value_for_debug());
  for (const auto &x : a.vertex_sectors)
    sectors.insert(x.id.value_for_debug());
  std::vector<unsigned> selected_decisions(a.patches.size());
  std::vector<unsigned> label_uses(a.side_labels.size());
  const operation_contract selected_operation(a.selected_operation);
  for (const auto &d : a.decisions) {
    if (d.negative_label.value_for_debug() >= a.side_labels.size() ||
        d.positive_label.value_for_debug() >= a.side_labels.size() ||
        d.negative_label == d.positive_label)
      return fail("exact_result.decision_labels");
    const auto &negative_label =
        a.side_labels[d.negative_label.value_for_debug()];
    const auto &positive_label =
        a.side_labels[d.positive_label.value_for_debug()];
    if (negative_label.patch != d.patch || positive_label.patch != d.patch ||
        negative_label.side != patch_plane_side::negative ||
        positive_label.side != patch_plane_side::positive ||
        negative_label.occupancy != d.negative_occupancy ||
        positive_label.occupancy != d.positive_occupancy ||
        negative_label.source_side == positive_label.source_side)
      return fail("exact_result.side_label_binding");
    ++label_uses[d.negative_label.value_for_debug()];
    ++label_uses[d.positive_label.value_for_debug()];
    const bool negative = selected_operation.occupied(
                   d.negative_occupancy.in_a, d.negative_occupancy.in_b),
               positive = selected_operation.occupied(
                   d.positive_occupancy.in_a, d.positive_occupancy.in_b);
    const auto expected_kind =
        negative == positive
            ? (negative ? patch_decision_kind::discard_internal
                        : patch_decision_kind::discard_exterior)
            : (negative ? patch_decision_kind::select_preserved
                        : patch_decision_kind::select_reversed);
    const auto expected_orientation =
        negative == positive ? selected_orientation::none
                             : (negative ? selected_orientation::preserved
                                         : selected_orientation::reversed);
    if (d.patch.value_for_debug() != d.id.value_for_debug() ||
        d.result_negative != negative || d.result_positive != positive ||
        d.kind != expected_kind || d.orientation != expected_orientation ||
        !strict_ids(d.provenance) ||
        bool(d.selected) != (negative != positive) ||
        bool(d.selected) != bool(d.representative) ||
        (d.selected &&
         (d.selected->value_for_debug() >= a.patches.size() ||
          a.patches[d.selected->value_for_debug()].source != d.patch ||
          a.patches[d.selected->value_for_debug()].orientation !=
              d.orientation ||
          a.patches[d.selected->value_for_debug()].provenance != d.provenance ||
          a.patches[d.selected->value_for_debug()].representative !=
              *d.representative)))
      return fail("exact_result.decision_binding");
    if (d.selected)
      ++selected_decisions[d.selected->value_for_debug()];
  }
  if (std::any_of(selected_decisions.begin(), selected_decisions.end(),
                  [](auto n) { return n != 1; }))
    return fail("exact_result.decision_usage");
  if (std::any_of(label_uses.begin(), label_uses.end(),
                  [](auto n) { return n != 1; }))
    return fail("exact_result.side_label_usage");
  for (std::size_t i = 0; i < a.vertices.size(); ++i) {
    const auto &v = a.vertices[i];
    if (!std::is_sorted(v.original_vertices.begin(), v.original_vertices.end(),
                        [](const auto &x, const auto &y) {
                          return std::tie(x.operand, x.vertex) <
                                 std::tie(y.operand, y.vertex);
                        }) ||
        std::adjacent_find(
            v.original_vertices.begin(), v.original_vertices.end(),
            [](const auto &x, const auto &y) {
              return x.operand == y.operand && x.vertex == y.vertex;
            }) != v.original_vertices.end() ||
        !strict_ids(v.constructions) ||
        !refs(v.constructions, a.constructions.size()))
      return fail("exact_result.vertex_references");
  }
  for (const auto &c : a.curves) {
    if (c.carrier.direction.x.is_zero() && c.carrier.direction.y.is_zero() &&
        c.carrier.direction.z.is_zero())
      return fail("exact_result.degenerate_curve");
    if (c.parent_carrier && !curves.count(c.parent_carrier->value_for_debug()))
      return fail("exact_result.curve_parent");
    if (c.kind == symbolic_curve_kind::carrier &&
        (c.parent_carrier || c.parameters))
      return fail("exact_result.curve_kind");
    if (c.kind == symbolic_curve_kind::atomic_interval &&
        (!c.parent_carrier || !c.parameters || !c.lower || !c.upper))
      return fail("exact_result.curve_kind");
    if (c.parameters && !(c.parameters->lower < c.parameters->upper))
      return fail("exact_result.curve_interval");
    if (!strict_ids(c.facets) || !strict_ids(c.constructions) ||
        !refs(c.constructions, a.constructions.size()))
      return fail("exact_result.curve_references");
  }
  std::vector<unsigned> edge_uses(a.edges.size()),
      vertex_uses(a.vertices.size()), occ_uses(a.vertex_occurrences.size()),
      cycle_uses(a.cycles.size());
  for (const auto &o : a.vertex_occurrences)
    if (o.incident_halfedges.empty() ||
        !refs(o.incident_halfedges, a.halfedges.size(), true) ||
        o.vertex.value_for_debug() >= a.vertices.size())
      return fail("exact_result.vertex_occurrence");
  for (std::size_t i = 0; i < a.edges.size(); ++i) {
    const auto &x = a.edges[i];
    if (x.lower.value_for_debug() >= a.vertices.size() ||
        x.upper.value_for_debug() >= a.vertices.size() || x.lower == x.upper ||
        x.uses.size() != 2 || !refs(x.uses, a.halfedges.size(), true) ||
        a.edge_geometry[i].edge != x.id ||
        !strict_ids(a.edge_geometry[i].curves) ||
        !std::all_of(a.edge_geometry[i].curves.begin(),
                     a.edge_geometry[i].curves.end(),
                     [&](auto c) { return curves.count(c.value_for_debug()); }))
      return fail("exact_result.edge");
  }
  for (const auto &h : a.halfedges) {
    auto in = [&](auto x, auto n) { return x.value_for_debug() < n; };
    if (!in(h.patch, a.patches.size()) || !in(h.cycle, a.cycles.size()) ||
        !in(h.edge, a.edges.size()) || !in(h.origin, a.vertices.size()) ||
        !in(h.destination, a.vertices.size()) ||
        !in(h.origin_occurrence, a.vertex_occurrences.size()) ||
        !in(h.destination_occurrence, a.vertex_occurrences.size()) ||
        !in(h.next, a.halfedges.size()) ||
        !in(h.previous, a.halfedges.size()) ||
        a.halfedges[h.next.value_for_debug()].previous != h.id ||
        a.halfedges[h.previous.value_for_debug()].next != h.id ||
        a.vertex_occurrences[h.origin_occurrence.value_for_debug()].vertex !=
            h.origin ||
        a.vertex_occurrences[h.destination_occurrence.value_for_debug()]
                .vertex != h.destination)
      return fail("exact_result.halfedge");
    ++edge_uses[h.edge.value_for_debug()];
    ++vertex_uses[h.origin.value_for_debug()];
    ++occ_uses[h.origin_occurrence.value_for_debug()];
  }
  if (std::any_of(edge_uses.begin(), edge_uses.end(),
                  [](auto n) { return n != 2; }) ||
      std::any_of(vertex_uses.begin(), vertex_uses.end(),
                  [](auto n) { return n == 0; }) ||
      std::any_of(occ_uses.begin(), occ_uses.end(),
                  [](auto n) { return n == 0; }))
    return fail("exact_result.topology_usage");
  for (const auto &e : a.edges) {
    const auto &h0 = a.halfedges[e.uses[0].value_for_debug()];
    const auto &h1 = a.halfedges[e.uses[1].value_for_debug()];
    if (h0.edge != e.id || h1.edge != e.id || h0.origin != h1.destination ||
        h0.destination != h1.origin)
      return fail("exact_result.edge_binding");
  }
  for (const auto &o : a.vertex_occurrences) {
    std::vector<selected_halfedge_id> expected;
    for (const auto &h : a.halfedges)
      if (h.origin_occurrence == o.id || h.destination_occurrence == o.id)
        expected.push_back(h.id);
    std::sort(expected.begin(), expected.end());
    expected.erase(std::unique(expected.begin(), expected.end()),
                   expected.end());
    if (expected != o.incident_halfedges)
      return fail("exact_result.occurrence_binding");
  }
  for (const auto &c : a.cycles) {
    if (c.halfedges.size() < 3 ||
        c.patch.value_for_debug() >= a.patches.size() ||
        !refs(c.halfedges, a.halfedges.size()))
      return fail("exact_result.cycle");
    for (std::size_t j = 0; j < c.halfedges.size(); ++j) {
      const auto &h = a.halfedges[c.halfedges[j].value_for_debug()];
      if (h.cycle != c.id || h.patch != c.patch ||
          h.next != c.halfedges[(j + 1) % c.halfedges.size()] ||
          h.previous !=
              c.halfedges[(j + c.halfedges.size() - 1) % c.halfedges.size()])
        return fail("exact_result.cycle_binding");
    }
  }
  for (std::size_t i = 0; i < a.patches.size(); ++i) {
    const auto &p = a.patches[i];
    const auto &geometry = a.patch_geometry[i];
    if (p.orientation == selected_orientation::none || p.cycles.empty() ||
        !refs(p.cycles, a.cycles.size(), true) || !strict_ids(p.provenance) ||
        geometry.patch != p.id ||
        (geometry.plane.a.is_zero() && geometry.plane.b.is_zero() &&
         geometry.plane.c.is_zero()) ||
        geometry.projected_double_area.sign() != exact_sign::positive ||
        a.provenance[i].patch != p.id)
      return fail("exact_result.patch");
    for (auto c : p.cycles)
      ++cycle_uses[c.value_for_debug()];
    bool rep = false;
    std::vector<sheet_use_id> uses;
    for (const auto &x : a.provenance[i].contributors) {
      uses.push_back(x.use);
      if (x.representative) {
        if (rep || x.use != p.representative)
          return fail("exact_result.representative");
        rep = true;
      }
    }
    if (!rep || uses != p.provenance)
      return fail("exact_result.provenance");
  }
  if (std::any_of(cycle_uses.begin(), cycle_uses.end(),
                  [](auto n) { return n != 1; }))
    return fail("exact_result.cycle_usage");
  std::vector<std::uint64_t> relation_owners(a.defining_relations.size());
  for (const auto &c : a.constructions) {
    if (!refs(c.children, c.id.value_for_debug(), true) ||
        !refs(c.defining_relations, a.defining_relations.size(), true) ||
        !std::is_sorted(c.defining_sources.begin(), c.defining_sources.end(),
                        [](const auto &x, const auto &y) {
                          return std::tie(x.kind, x.primary, x.secondary) <
                                 std::tie(y.kind, y.primary, y.secondary);
                        }))
      return fail("exact_result.construction");
    for (const auto relation : c.defining_relations) {
      if (a.defining_relations[relation.value_for_debug()].construction != c.id)
        return fail("exact_result.relation_owner");
      ++relation_owners[relation.value_for_debug()];
    }
  }
  for (const auto &d : a.defining_relations)
    if (d.formula_version != 1 ||
        d.construction.value_for_debug() >= a.constructions.size() ||
        !refs(d.operand_nodes, a.constructions.size(), true) ||
        !std::is_sorted(d.defining_sources.begin(), d.defining_sources.end(),
                        [](const auto &x, const auto &y) {
                          return std::tie(x.kind, x.primary, x.secondary) <
                                 std::tie(y.kind, y.primary, y.secondary);
                        }))
      return fail("exact_result.relation");
  if (std::any_of(relation_owners.begin(), relation_owners.end(),
                  [](auto owners) { return owners != 1; }))
    return fail("exact_result.orphan_relation");
  auto feature_ok = [&](const exact_feature_reference &f) {
    switch (f.kind) {
    case 42:
      return f.primary < a.patches.size();
    case 43:
      return f.primary < a.cycles.size();
    case 44:
      return f.primary < a.halfedges.size();
    case 45:
      return f.primary < a.edges.size();
    case 46:
      return f.primary < a.vertices.size();
    case 47:
      return f.primary < a.vertex_occurrences.size();
    case 48:
      return f.primary < a.topology_obstructions.size();
    case 49:
      return f.primary == 0;
    case 64:
      return f.primary < a.defining_relations.size();
    default:
      return true;
    }
  };
  for (const auto &c : a.constructions)
    if (!std::all_of(c.defining_sources.begin(), c.defining_sources.end(),
                     feature_ok))
      return fail("exact_result.feature_binding");
  for (const auto &v : a.vertices) {
    std::vector<bool> visited(a.constructions.size());
    std::vector<construction_node_id> pending = v.constructions;
    while (!pending.empty()) {
      const auto construction_id = pending.back();
      pending.pop_back();
      if (visited[construction_id.value_for_debug()])
        continue;
      visited[construction_id.value_for_debug()] = true;
      const auto &construction =
          a.constructions[construction_id.value_for_debug()];
      pending.insert(pending.end(), construction.children.begin(),
                     construction.children.end());
      for (const auto relation_id : construction.defining_relations) {
        const auto &relation =
            a.defining_relations[relation_id.value_for_debug()];
        const auto value = relation.coefficients[0] * v.coordinate.x +
                           relation.coefficients[1] * v.coordinate.y +
                           relation.coefficients[2] * v.coordinate.z +
                           relation.coefficients[3];
        if (relation.construction != construction.id ||
            value.sign() != relation.expected)
          return fail("exact_result.defining_relation_replay");
      }
    }
  }
  for (const auto &d : a.defining_relations)
    if (!std::all_of(d.defining_sources.begin(), d.defining_sources.end(),
                     feature_ok))
      return fail("exact_result.feature_binding");
  for (const auto &r : a.link_rays)
    if (r.id == r.antipode || !rays.count(r.antipode.value_for_debug()) ||
        (r.direction.x.is_zero() && r.direction.y.is_zero() &&
         r.direction.z.is_zero()))
      return fail("exact_result.ray_binding");
    else {
      const auto &anti =
          *std::find_if(a.link_rays.begin(), a.link_rays.end(),
                        [&](const auto &x) { return x.id == r.antipode; });
      if (anti.antipode != r.id ||
          !(anti.direction.x == r.direction.x.negated()) ||
          !(anti.direction.y == r.direction.y.negated()) ||
          !(anti.direction.z == r.direction.z.negated()))
        return fail("exact_result.antipode");
    }
  for (const auto &o : a.source_vertex_occurrences)
    if (!o.occurrence.valid() || !o.vertex.valid() || !o.operand.valid() ||
        !o.shell.valid() || !strict_ids(o.incident_halfedges) ||
        !strict_ids(o.link_regions) ||
        !std::all_of(o.link_regions.begin(), o.link_regions.end(), [&](auto x) {
          return sectors.count(x.value_for_debug());
        }))
      return fail("exact_result.source_occurrence");
  for (const auto &x : a.link_arcs)
    if (!x.id.valid() || !source_occ.count(x.occurrence.value_for_debug()) ||
        !rays.count(x.origin.value_for_debug()) ||
        !rays.count(x.destination.value_for_debug()) || !strict_ids(x.layers))
      return fail("exact_result.arc_binding");
  std::map<vertex_occurrence_id, std::vector<vertex_sector_id>>
      reconstructed_sectors;
  for (const auto &s : a.vertex_sectors)
    if (!s.id.valid() || !s.vertex.valid() || !s.region.valid() ||
        !source_occ.count(s.occurrence.value_for_debug()) ||
        !strict_ids(s.boundary_rays) || !strict_ids(s.boundary_arcs) ||
        !strict_ids(s.seam_continuations) ||
        !strict_ids(s.source_edge_continuations) ||
        !std::all_of(s.boundary_rays.begin(), s.boundary_rays.end(),
                     [&](auto x) { return rays.count(x.value_for_debug()); }) ||
        !std::all_of(
            s.boundary_arcs.begin(), s.boundary_arcs.end(), [&](auto x) {
              const auto arc = std::find_if(
                  a.link_arcs.begin(), a.link_arcs.end(),
                  [&](const auto &candidate) { return candidate.id == x; });
              return arc != a.link_arcs.end() &&
                     arc->occurrence == s.occurrence;
            }))
      return fail("exact_result.sector_binding");
    else {
      const auto occurrence =
          std::lower_bound(a.source_vertex_occurrences.begin(),
                           a.source_vertex_occurrences.end(), s.occurrence,
                           [](const auto &candidate, vertex_occurrence_id id) {
                             return candidate.occurrence < id;
                           });
      if (occurrence == a.source_vertex_occurrences.end() ||
          occurrence->occurrence != s.occurrence ||
          occurrence->vertex != s.vertex)
        return fail("exact_result.sector_occurrence_binding");
      reconstructed_sectors[s.occurrence].push_back(s.id);
    }
  for (const auto &occurrence : a.source_vertex_occurrences)
    if (reconstructed_sectors[occurrence.occurrence] != occurrence.link_regions)
      return fail("exact_result.occurrence_sector_reconstruction");
  std::map<selected_vertex_id, std::vector<std::uint64_t>>
      expected_vertex_obstructions;
  for (const auto &occurrence : a.vertex_occurrences)
    expected_vertex_obstructions[occurrence.vertex].push_back(
        occurrence.id.value_for_debug());
  std::map<global_atomic_edge_id, std::vector<std::uint64_t>>
      expected_edge_obstructions;
  for (const auto &edge : a.edges)
    expected_edge_obstructions[edge.source].push_back(
        edge.id.value_for_debug());
  std::vector<topology_obstruction> expected_obstructions;
  for (const auto &entry : expected_vertex_obstructions)
    if (entry.second.size() > 1) {
      topology_obstruction obstruction;
      obstruction.id = topology_obstruction_id::from_canonical_value(
          expected_obstructions.size());
      obstruction.kind =
          topology_obstruction_kind::disconnected_geometric_vertex_link;
      obstruction.vertex = entry.first;
      obstruction.occurrences = entry.second;
      expected_obstructions.push_back(std::move(obstruction));
    }
  for (const auto &entry : expected_edge_obstructions)
    if (entry.second.size() > 1) {
      topology_obstruction obstruction;
      obstruction.id = topology_obstruction_id::from_canonical_value(
          expected_obstructions.size());
      obstruction.kind = topology_obstruction_kind::multiple_edge_occurrences;
      obstruction.edge = entry.first;
      obstruction.occurrences = entry.second;
      expected_obstructions.push_back(std::move(obstruction));
    }
  if (expected_obstructions.size() != a.topology_obstructions.size())
    return fail("exact_result.obstruction_count");
  for (std::size_t i = 0; i != expected_obstructions.size(); ++i) {
    const auto &expected = expected_obstructions[i];
    const auto &actual = a.topology_obstructions[i];
    if (actual.id != expected.id || actual.kind != expected.kind ||
        actual.vertex != expected.vertex || actual.edge != expected.edge ||
        actual.occurrences != expected.occurrences)
      return fail("exact_result.obstruction_reconstruction");
  }
  std::uint64_t discard_exterior = 0, discard_internal = 0,
                select_preserved = 0, select_reversed = 0, provenance_uses = 0;
  for (const auto &d : a.decisions) {
    provenance_uses += d.provenance.size();
    switch (d.kind) {
    case patch_decision_kind::discard_exterior:
      ++discard_exterior;
      break;
    case patch_decision_kind::discard_internal:
      ++discard_internal;
      break;
    case patch_decision_kind::select_preserved:
      ++select_preserved;
      break;
    case patch_decision_kind::select_reversed:
      ++select_reversed;
      break;
    }
  }
  std::uint64_t components = 0;
  std::vector<std::vector<selected_patch_id>> adjacency(a.patches.size());
  for (const auto &e : a.edges) {
    const auto p0 = a.halfedges[e.uses[0].value_for_debug()].patch,
               p1 = a.halfedges[e.uses[1].value_for_debug()].patch;
    adjacency[p0.value_for_debug()].push_back(p1);
    adjacency[p1.value_for_debug()].push_back(p0);
  }
  std::vector<bool> reached(a.patches.size());
  for (const auto &p : a.patches)
    if (!reached[p.id.value_for_debug()]) {
      ++components;
      std::vector<selected_patch_id> pending{p.id};
      reached[p.id.value_for_debug()] = true;
      while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        for (auto next : adjacency[current.value_for_debug()])
          if (!reached[next.value_for_debug()]) {
            reached[next.value_for_debug()] = true;
            pending.push_back(next);
          }
      }
    }
  if (a.certificate.id.value_for_debug() != 0 ||
      a.certificate.decisions != a.decisions.size() ||
      a.certificate.discard_exterior != discard_exterior ||
      a.certificate.discard_internal != discard_internal ||
      a.certificate.select_preserved != select_preserved ||
      a.certificate.select_reversed != select_reversed ||
      a.certificate.provenance_uses != provenance_uses ||
      a.certificate.connected_components != components ||
      a.certificate.selected_vertices != a.vertices.size() ||
      a.certificate.selected_vertex_occurrences !=
          a.vertex_occurrences.size() ||
      a.certificate.selected_edges != a.edges.size() ||
      a.certificate.selected_halfedges != a.halfedges.size() ||
      a.certificate.selected_cycles != a.cycles.size() ||
      a.certificate.selected_patches != a.patches.size() ||
      a.certificate.topology_obstructions != a.topology_obstructions.size() ||
      a.certificate.topology != a.topology ||
      a.certificate.semantic_digest != a.selected_semantic_digest ||
      (a.patches.empty() !=
       (a.topology == selected_boundary_topology::empty)) ||
      (a.topology_obstructions.empty() !=
       (a.topology !=
        selected_boundary_topology::closed_stratified_nonmanifold)))
    return fail("exact_result.certificate");
  if (a.canonical_digest != computed_digest(a))
    return fail("exact_result.digest");
  return true;
}

std::uint64_t entity_capacity_for(index_tag index) {
  return index == index_tag::uint32
             ? std::uint64_t(std::numeric_limits<std::uint32_t>::max())
             : std::numeric_limits<std::uint64_t>::max();
}

std::uint64_t largest_export_collection(const exact_stratified_boundary &a) {
  std::uint64_t largest = 0;
  const std::array<std::size_t, 6> sizes{{
      a.vertices.size(),
      a.vertex_occurrences.size(),
      a.edges.size(),
      a.halfedges.size(),
      a.cycles.size(),
      a.patches.size(),
  }};
  for (const auto size : sizes)
    largest = std::max(largest, static_cast<std::uint64_t>(size));
  return largest;
}

product_status_or<bool> validate_export_capacity(
    const exact_stratified_boundary &a, index_tag index,
    const exact_coordinate_export_limits &limits) {
  if (index != index_tag::uint32 && index != index_tag::uint64)
    return error(product_error_code::input_contract_error,
                 "exact_coordinate_export.index_tag");
  const auto largest = largest_export_collection(a);
  if (largest > limits.max_indexed_entities)
    return error(product_error_code::resource_limit,
                 "exact_coordinate_export.entity_limit");
  // The all-ones value remains reserved as the invalid canonical ID, so the
  // numeric maximum is also the maximum number of dense entities.
  if (largest > entity_capacity_for(index))
    return error(product_error_code::index_overflow,
                 "exact_coordinate_export.index_capacity");
  return true;
}

std::vector<std::uint8_t> exact_export_payload(
    index_tag index, operation op, selected_boundary_topology topology,
    const digest &exact_result_digest,
    const std::vector<std::uint8_t> &exact_result_bytes) {
  canonical_encoder e;
  e.raw(reinterpret_cast<const std::uint8_t *>(exact_export_record_tag.data()),
        exact_export_record_tag.size());
  e.u16(exact_coordinate_export_schema);
  e.u16(exact_coordinate_export_checker_version);
  en(e, index);
  e.u64(entity_capacity_for(index));
  en(e, op);
  en(e, topology);
  dg(e, exact_result_digest);
  e.byte_string(exact_result_bytes);
  return e.bytes();
}

digest exact_export_digest(const std::vector<std::uint8_t> &payload_bytes) {
  return domain_digest(exact_export_digest_tag, payload_bytes);
}

std::vector<std::uint8_t>
exact_export_record(const std::vector<std::uint8_t> &payload_bytes,
                    const digest &canonical_digest) {
  canonical_encoder e;
  e.raw(payload_bytes.data(), payload_bytes.size());
  dg(e, canonical_digest);
  return e.bytes();
}

} // namespace

namespace detail {

product_status_or<bool>
finalize_exact_stratified_boundary(exact_stratified_boundary &a) {
  a.canonical_digest = computed_digest(a);
  return validate_impl(a);
}

} // namespace detail

product_status_or<bool> validate_exact_stratified_boundary(
    const exact_stratified_boundary &a) noexcept {
  try {
    return validate_impl(a);
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "exact_result.validation_exception");
  }
}
product_status_or<std::shared_ptr<const exact_stratified_boundary>>
freeze_exact_stratified_boundary(exact_stratified_boundary a) {
  try {
    a.canonical_digest = computed_digest(a);
    auto ok = validate_impl(a);
    if (!ok.has_value())
      return ok.error();
    return std::shared_ptr<const exact_stratified_boundary>(
        std::make_shared<exact_stratified_boundary>(std::move(a)));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "exact_result.freeze_exception");
  }
}
product_status_or<std::vector<std::uint8_t>>
encode_exact_stratified_boundary(const exact_stratified_boundary &a) {
  try {
    auto ok = validate_impl(a);
    if (!ok.has_value())
      return ok.error();
    canonical_encoder e;
    payload(e, a);
    dg(e, a.canonical_digest);
    return e.bytes();
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_result.encode_exception");
  }
}
product_status_or<std::shared_ptr<const exact_stratified_boundary>>
decode_exact_stratified_boundary(const std::vector<std::uint8_t> &bytes,
                                 const exact_result_decode_limits &limits) {
  try {
    reader r(bytes, limits);
    auto a = payload(r);
    a.canonical_digest = dg(r);
    if (!r.done())
      return error(product_error_code::exact_result_serialization_error,
                   "exact_result.trailing_bytes");
    auto ok = validate_impl(a);
    if (!ok.has_value())
      return ok.error();
    auto encoded = encode_exact_stratified_boundary(a);
    if (!encoded.has_value() || encoded.value() != bytes)
      return error(product_error_code::exact_result_serialization_error,
                   "exact_result.noncanonical_record");
    return std::shared_ptr<const exact_stratified_boundary>(
        std::make_shared<exact_stratified_boundary>(std::move(a)));
  } catch (const decode_failure &e) {
    return error(e.code, e.what());
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_result.decode_exception");
  }
}
product_status_or<bool> verify_serialized_exact_stratified_boundary(
    const std::vector<std::uint8_t> &b,
    const exact_result_decode_limits &l) noexcept {
  try {
    auto x = decode_exact_stratified_boundary(b, l);
    if (!x.has_value())
      return x.error();
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_result.verify_exception");
  }
}

product_status_or<exact_result_handle> make_exact_result_handle(
    const std::shared_ptr<const exact_stratified_boundary> &a) {
  try {
    if (!a)
      return error(product_error_code::input_contract_error,
                   "exact_result.null_boundary");
    auto bytes = encode_exact_stratified_boundary(*a);
    if (!bytes.has_value())
      return bytes.error();
    backend_provenance b;
    b.schema = a->backend.schema;
    b.producer = a->backend.producer;
    b.selection = a->backend.selection;
    b.fallback_used = a->backend.fallback_used;
    b.attempted_backends = a->backend.attempted_backends;
    if (a->backend.primary_failure)
      b.primary_failure = make_product_error(*a->backend.primary_failure,
                                             "exact_result.primary_failure");
    const auto topology =
        a->topology == selected_boundary_topology::empty
            ? exact_result_topology::empty
            : a->topology ==
                      selected_boundary_topology::closed_embedded_two_manifold
                  ? exact_result_topology::closed_embedded_two_manifold
                  : exact_result_topology::stratified_non_manifold;
    return make_exact_result_handle(a->selected_operation, topology,
                                    std::move(b), std::move(bytes.value()));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "exact_result.handle_exception");
  }
}

product_status_or<std::shared_ptr<const exact_stratified_boundary>>
read_exact_result(const exact_result_handle &handle,
                  const exact_result_decode_limits &limits) {
  if (!handle.valid())
    return error(product_error_code::stale_binding,
                 "exact_result.empty_handle");
  auto decoded =
      decode_exact_stratified_boundary(handle->canonical_bytes, limits);
  if (!decoded.has_value())
    return decoded.error();
  const auto &boundary = *decoded.value();
  const auto topology =
      boundary.topology == selected_boundary_topology::empty
          ? exact_result_topology::empty
          : boundary.topology ==
                    selected_boundary_topology::closed_embedded_two_manifold
                ? exact_result_topology::closed_embedded_two_manifold
                : exact_result_topology::stratified_non_manifold;
  if (boundary.selected_operation != handle->selected_operation ||
      topology != handle->topology ||
      !same_backend_identity(boundary.backend.producer,
                             handle->backend.producer))
    return error(product_error_code::stale_binding,
                 "exact_result.handle_binding");
  return decoded.value();
}

product_status_or<exact_coordinate_export_handle> export_exact_coordinates(
    const exact_result_handle &exact, index_tag index,
    const exact_coordinate_export_limits &limits) {
  try {
    if (!exact.valid())
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.empty_exact_result");
    auto boundary = read_exact_result(exact, limits.exact_result);
    if (!boundary.has_value())
      return boundary.error();
    auto capacity = validate_export_capacity(*boundary.value(), index, limits);
    if (!capacity.has_value())
      return capacity.error();
    const std::uint64_t framing_bytes =
        8U + 2U + 2U + 1U + 8U + 1U + 1U + 8U +
        2U * digest{}.bytes.size();
    if (exact->canonical_bytes.size() >
            std::numeric_limits<std::uint64_t>::max() - framing_bytes ||
        exact->canonical_bytes.size() + framing_bytes > limits.max_record_bytes)
      return error(product_error_code::resource_limit,
                   "exact_coordinate_export.record_limit");
    auto payload_bytes = exact_export_payload(
        index, boundary.value()->selected_operation, boundary.value()->topology,
        exact->canonical_digest, exact->canonical_bytes);
    const auto export_digest = exact_export_digest(payload_bytes);
    auto record_bytes = exact_export_record(payload_bytes, export_digest);
    auto result = std::make_shared<exact_coordinate_export>();
    result->index = index;
    result->entity_capacity = entity_capacity_for(index);
    result->selected_operation = boundary.value()->selected_operation;
    result->topology = boundary.value()->topology;
    result->exact_result_digest = exact->canonical_digest;
    result->canonical_digest = export_digest;
    result->exact_result = exact;
    result->boundary = std::move(boundary.value());
    result->canonical_bytes = std::move(record_bytes);
    return exact_coordinate_export_handle(std::move(result));
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "exact_coordinate_export.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_coordinate_export.exception");
  }
}

product_status_or<exact_coordinate_export_handle> decode_exact_coordinate_export(
    const std::vector<std::uint8_t> &bytes,
    const exact_coordinate_export_limits &limits) {
  try {
    exact_result_decode_limits wrapper_limits;
    wrapper_limits.max_record_bytes = limits.max_record_bytes;
    wrapper_limits.max_entities = limits.max_indexed_entities;
    wrapper_limits.max_references = limits.max_record_bytes;
    wrapper_limits.max_exact_hex_bytes = limits.max_record_bytes;
    wrapper_limits.max_string_bytes = limits.max_record_bytes;
    reader r(bytes, wrapper_limits);
    std::array<std::uint8_t, 8> magic{};
    r.raw(magic.data(), magic.size());
    if (!std::equal(magic.begin(), magic.end(), exact_export_record_tag.begin()))
      throw decode_failure("exact_coordinate_export.magic");
    if (r.u16() != exact_coordinate_export_schema ||
        r.u16() != exact_coordinate_export_checker_version)
      throw decode_failure("exact_coordinate_export.schema");
    const auto index = de<index_tag>(r, 1);
    const auto entity_capacity = r.u64();
    if (entity_capacity != entity_capacity_for(index))
      throw decode_failure("exact_coordinate_export.index_capacity");
    const auto op = de<operation>(r, 4);
    const auto topology = de<selected_boundary_topology>(r, 2);
    const auto exact_digest = dg(r);
    auto exact_bytes = r.byte_string();
    const auto export_digest = dg(r);
    if (!r.done())
      throw decode_failure("exact_coordinate_export.trailing_bytes");

    const auto expected_payload =
        exact_export_payload(index, op, topology, exact_digest, exact_bytes);
    if (export_digest != exact_export_digest(expected_payload) ||
        bytes != exact_export_record(expected_payload, export_digest))
      throw decode_failure("exact_coordinate_export.noncanonical_record");
    auto boundary =
        decode_exact_stratified_boundary(exact_bytes, limits.exact_result);
    if (!boundary.has_value())
      return boundary.error();
    if (boundary.value()->selected_operation != op ||
        boundary.value()->topology != topology)
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.exact_metadata_binding");
    auto capacity = validate_export_capacity(*boundary.value(), index, limits);
    if (!capacity.has_value())
      return capacity.error();
    auto exact = make_exact_result_handle(boundary.value());
    if (!exact.has_value())
      return exact.error();
    if (exact.value()->canonical_digest != exact_digest)
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.exact_digest_binding");

    auto result = std::make_shared<exact_coordinate_export>();
    result->index = index;
    result->entity_capacity = entity_capacity;
    result->selected_operation = op;
    result->topology = topology;
    result->exact_result_digest = exact_digest;
    result->canonical_digest = export_digest;
    result->exact_result = std::move(exact.value());
    result->boundary = std::move(boundary.value());
    result->canonical_bytes = bytes;
    return exact_coordinate_export_handle(std::move(result));
  } catch (const decode_failure &e) {
    return error(e.code, e.what());
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "exact_coordinate_export.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_coordinate_export.decode_exception");
  }
}

product_status_or<bool> validate_exact_coordinate_export_binding(
    const exact_coordinate_export_handle &exported,
    const exact_result_handle &exact) noexcept {
  try {
    if (!exported || !exact.valid() || !exported->boundary ||
        !exported->exact_result.valid())
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.empty_binding");
    auto decoded = decode_exact_coordinate_export(exported->canonical_bytes);
    if (!decoded.has_value())
      return decoded.error();
    const auto &verified = *decoded.value();
    if (exported->schema != exact_coordinate_export_schema ||
        exported->index != verified.index ||
        exported->entity_capacity != verified.entity_capacity ||
        exported->selected_operation != verified.selected_operation ||
        exported->topology != verified.topology ||
        exported->exact_result_digest != verified.exact_result_digest ||
        exported->canonical_digest != verified.canonical_digest ||
        exported->exact_result->canonical_digest !=
            verified.exact_result->canonical_digest ||
        exported->exact_result_digest != exact->canonical_digest ||
        exported->exact_result->canonical_bytes != exact->canonical_bytes)
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.binding");
    auto boundary_bytes =
        encode_exact_stratified_boundary(*exported->boundary);
    if (!boundary_bytes.has_value() ||
        boundary_bytes.value() != exported->exact_result->canonical_bytes)
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.boundary_binding");
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "exact_coordinate_export.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_coordinate_export.binding_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_exact_coordinate_export(const exact_coordinate_export_handle &exported) {
  if (!exported)
    return error(product_error_code::input_contract_error,
                 "exact_coordinate_export.empty_handle");
  auto valid =
      validate_exact_coordinate_export_binding(exported, exported->exact_result);
  if (!valid.has_value())
    return valid.error();
  return exported->canonical_bytes;
}

product_status_or<bool> verify_serialized_exact_coordinate_export(
    const std::vector<std::uint8_t> &bytes,
    const exact_coordinate_export_limits &limits) noexcept {
  try {
    if (bytes.size() > limits.max_record_bytes)
      return error(product_error_code::resource_limit,
                   "exact_coordinate_export.verify_record_limit");
    std::size_t at = 0;
    auto byte = [&]() {
      if (at == bytes.size())
        throw decode_failure("exact_coordinate_export.verify_truncated");
      return bytes[at++];
    };
    auto u16 = [&]() {
      std::uint16_t value = 0;
      for (unsigned i = 0; i != 2; ++i)
        value = std::uint16_t((value << 8) | byte());
      return value;
    };
    auto u64 = [&]() {
      std::uint64_t value = 0;
      for (unsigned i = 0; i != 8; ++i)
        value = (value << 8) | byte();
      return value;
    };
    auto read_digest = [&]() {
      digest value;
      for (auto &part : value.bytes)
        part = byte();
      return value;
    };
    for (const auto expected : exact_export_record_tag)
      if (byte() != static_cast<std::uint8_t>(expected))
        throw decode_failure("exact_coordinate_export.verify_magic");
    if (u16() != exact_coordinate_export_schema ||
        u16() != exact_coordinate_export_checker_version)
      throw decode_failure("exact_coordinate_export.verify_schema");
    const auto index_value = byte();
    if (index_value > 1)
      throw decode_failure("exact_coordinate_export.verify_index_tag");
    const auto index = static_cast<index_tag>(index_value);
    if (u64() != entity_capacity_for(index))
      throw decode_failure("exact_coordinate_export.verify_index_capacity");
    const auto operation_value = byte();
    const auto topology_value = byte();
    if (operation_value > 4 || topology_value > 2)
      throw decode_failure("exact_coordinate_export.verify_enum");
    const auto exact_digest = read_digest();
    const auto exact_size = u64();
    if (exact_size > limits.exact_result.max_record_bytes)
      return error(product_error_code::resource_limit,
                   "exact_coordinate_export.verify_exact_record_limit");
    if (exact_size > bytes.size() - at ||
        bytes.size() - at - static_cast<std::size_t>(exact_size) !=
            digest{}.bytes.size())
      throw decode_failure("exact_coordinate_export.verify_exact_size");
    std::vector<std::uint8_t> exact_bytes(
        bytes.begin() + static_cast<std::ptrdiff_t>(at),
        bytes.begin() + static_cast<std::ptrdiff_t>(at + exact_size));
    at += static_cast<std::size_t>(exact_size);
    const auto recorded_export_digest = read_digest();
    if (at != bytes.size())
      throw decode_failure("exact_coordinate_export.verify_trailing_bytes");
    std::vector<std::uint8_t> payload_bytes(bytes.begin(),
                                            bytes.end() - digest{}.bytes.size());
    if (recorded_export_digest != exact_export_digest(payload_bytes))
      throw decode_failure("exact_coordinate_export.verify_digest");
    auto exact_verified = verify_serialized_exact_stratified_boundary(
        exact_bytes, limits.exact_result);
    if (!exact_verified.has_value())
      return exact_verified.error();
    auto boundary =
        decode_exact_stratified_boundary(exact_bytes, limits.exact_result);
    if (!boundary.has_value())
      return boundary.error();
    if (static_cast<std::uint8_t>(boundary.value()->selected_operation) !=
            operation_value ||
        static_cast<std::uint8_t>(boundary.value()->topology) != topology_value)
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.verify_metadata_binding");
    auto capacity = validate_export_capacity(*boundary.value(), index, limits);
    if (!capacity.has_value())
      return capacity.error();
    auto exact = make_exact_result_handle(boundary.value());
    if (!exact.has_value())
      return exact.error();
    if (exact.value()->canonical_digest != exact_digest)
      return error(product_error_code::stale_binding,
                   "exact_coordinate_export.verify_exact_binding");
    return true;
  } catch (const decode_failure &e) {
    return error(e.code, e.what());
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit,
                 "exact_coordinate_export.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_coordinate_export.verify_exception");
  }
}

product_status_or<bool> validate_canonical_exact_result_bytes(
    operation op, exact_result_topology topology,
    const backend_provenance &backend_binding,
    const std::vector<std::uint8_t> &bytes) noexcept {
  try {
    auto decoded = decode_exact_stratified_boundary(bytes);
    if (!decoded.has_value())
      return decoded.error();
    const auto &x = *decoded.value();
    const auto decoded_topology =
        x.topology == selected_boundary_topology::empty
            ? exact_result_topology::empty
            : x.topology ==
                      selected_boundary_topology::closed_embedded_two_manifold
                  ? exact_result_topology::closed_embedded_two_manifold
                  : exact_result_topology::stratified_non_manifold;
    if (x.selected_operation != op || decoded_topology != topology ||
        !same_backend_identity(x.backend.producer, backend_binding.producer) ||
        x.backend.selection != backend_binding.selection ||
        x.backend.fallback_used != backend_binding.fallback_used ||
        x.backend.attempted_backends != backend_binding.attempted_backends ||
        x.backend.primary_failure.has_value() !=
            backend_binding.primary_failure.has_value() ||
        (x.backend.primary_failure &&
         *x.backend.primary_failure != backend_binding.primary_failure->code))
      return error(product_error_code::stale_binding,
                   "exact_result.storage_binding");
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_result.storage_exception");
  }
}

product_status_or<bool> validate_durable_exact_result_bindings(
    const exact_result_handle &handle,
    const backend_provenance &backend_binding,
    const preparation_provenance &preparation_binding) noexcept {
  try {
    auto decoded = read_exact_result(handle);
    if (!decoded.has_value())
      return decoded.error();
    const auto &x = *decoded.value();
    if (!same_backend_identity(x.backend.producer, backend_binding.producer) ||
        x.backend.selection != backend_binding.selection ||
        x.backend.fallback_used != backend_binding.fallback_used ||
        x.backend.attempted_backends != backend_binding.attempted_backends ||
        x.backend.primary_failure.has_value() !=
            backend_binding.primary_failure.has_value() ||
        (x.backend.primary_failure &&
         *x.backend.primary_failure != backend_binding.primary_failure->code) ||
        x.preparation.schema != preparation_binding.schema ||
        x.preparation.mode != preparation_binding.mode ||
        x.preparation.input_digest != preparation_binding.input_digest ||
        x.preparation.prepared_digest != preparation_binding.prepared_digest ||
        x.preparation.policy_digest != preparation_binding.policy_digest ||
        x.preparation.report_digest != preparation_binding.report_digest ||
        x.preparation.geometry_changed != preparation_binding.geometry_changed)
      return error(product_error_code::stale_binding,
                   "exact_result.envelope_binding");
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_result.binding_exception");
  }
}

product_status_or<bool> validate_durable_mesh_selection_binding(
    const exact_result_handle &handle, const digest &selected_digest) noexcept {
  try {
    auto decoded = read_exact_result(handle);
    if (!decoded.has_value())
      return decoded.error();
    if (product_digest_is_zero(selected_digest) ||
        decoded.value()->selected_artifact_digest != selected_digest)
      return error(product_error_code::stale_binding,
                   "exact_result.mesh_selection_binding");
    return true;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::exact_result_serialization_error,
                 "exact_result.mesh_binding_exception");
  }
}

} // namespace mesh_boolean
} // namespace ygor
