#include "YgorMeshesBooleanNormalization.h"

#include "YgorMeshesBooleanInputTopology.h"
#include "YgorMeshesBooleanPreparationInternal.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Boolean normalization must not be compiled with fast-math"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Boolean normalization must not assume finite-only arithmetic"
#endif

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> policy_tag{{'Y', 'G', 'B', 'N', 'P', 'O', '0', '1'}};
constexpr std::array<char, 8> report_tag{{'Y', 'G', 'B', 'N', 'R', 'P', '0', '1'}};
constexpr std::array<char, 8> report_digest_tag{{'Y', 'G', 'B', 'N', 'R', 'D', '0', '1'}};
constexpr std::array<char, 8> certificate_tag{{'Y', 'G', 'B', 'P', 'C', 'E', '0', '1'}};

boolean_error normalization_error(const char *key,
                                  std::uint32_t subcode = 0) {
  return make_error(boolean_error_code::input_contract_error,
                    boolean_stage::input_validation, key, subcode);
}

boolean_error limit_error(const char *key, std::uint64_t requested = 0,
                          std::uint64_t limit = 0) {
  auto e = make_error(boolean_error_code::resource_limit,
                      boolean_stage::input_validation, key);
  e.requested = requested;
  e.limit = limit;
  return e;
}

void encode_digest(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}

bool digest_nonzero(const digest &d) {
  return std::any_of(d.bytes.begin(), d.bytes.end(),
                     [](std::uint8_t b) { return b != 0; });
}

coordinate_tag coordinate_type(bool is_float) {
  return is_float ? coordinate_tag::binary32 : coordinate_tag::binary64;
}

index_tag index_type(bool is_u32) {
  return is_u32 ? index_tag::uint32 : index_tag::uint64;
}

bool known(normalization_mode v) {
  return v >= normalization_mode::disabled &&
         v <= normalization_mode::geometry_changing;
}
bool known(model_unit v) {
  return v >= model_unit::unspecified && v <= model_unit::foot;
}
bool known(normalization_cancellation_policy v) {
  return v == normalization_cancellation_policy::deterministic_checkpoints;
}
bool known(normalization_defect_code v) {
  return v >= normalization_defect_code::nonfinite_coordinate &&
         v <= normalization_defect_code::component2_rejection;
}
bool known(normalization_map_status v) {
  return v >= normalization_map_status::total &&
         v <= normalization_map_status::absent;
}
bool known(normalization_operation v) {
  return v >= normalization_operation::irrelevant_storage_removal &&
         v < normalization_operation::count;
}
bool known(normalization_entity_kind v) {
  return v >= normalization_entity_kind::vertex &&
         v <= normalization_entity_kind::attribute;
}
bool known(normalization_reversibility v) {
  return v >= normalization_reversibility::identity &&
         v <= normalization_reversibility::irreversible;
}
bool known(normalization_topology_justification v) {
  return v >= normalization_topology_justification::structural_canonicalization &&
         v <= normalization_topology_justification::defect_resolution;
}
bool known(normalization_displacement_kind v) {
  return v >= normalization_displacement_kind::exact &&
         v <= normalization_displacement_kind::bounded;
}

status_or<bool> validate_policy(const normalization_policy &p,
                                bool executable) {
  if (p.schema != normalization_policy_schema || !known(p.mode) ||
      !known(p.unit) || !std::isfinite(p.model_tolerance) ||
      p.model_tolerance < 0.0 || p.edit_ordering_version != 1 ||
      p.diagnosis_version != 1 || !known(p.cancellation) ||
      p.checkpoint_interval == 0 || p.resources.max_report_bytes == 0 ||
      (p.model_tolerance > 0.0 && p.unit == model_unit::unspecified) ||
      (p.model_tolerance == 0.0 && std::signbit(p.model_tolerance)) ||
      (p.enabled_operations &
       ~((std::uint64_t(1) <<
          static_cast<unsigned>(normalization_operation::count)) -
         1)) != 0)
    return normalization_error("normalization_policy_malformed");
  if (executable &&
      (p.mode != normalization_mode::diagnosis_only ||
       p.enabled_operations != 0))
    return normalization_error("normalization_policy_unsupported");
  return true;
}

void encode_policy_body(canonical_encoder &e, const normalization_policy &p) {
  e.u16(p.schema);
  e.byte(static_cast<std::uint8_t>(p.mode));
  e.byte(static_cast<std::uint8_t>(p.unit));
  e.floating(p.model_tolerance);
  e.u64(p.enabled_operations);
  e.u16(p.edit_ordering_version);
  e.u16(p.diagnosis_version);
  e.u64(p.resources.max_work_units);
  e.u64(p.resources.max_defect_records);
  e.u64(p.resources.max_mapping_entries);
  e.u64(p.resources.max_report_bytes);
  e.byte(static_cast<std::uint8_t>(p.cancellation));
  e.u32(p.checkpoint_interval);
}

void encode_certificate_body(canonical_encoder &e,
                             const strict_validation_certificate &c) {
  e.u16(c.schema);
  e.u16(c.checker_version);
  e.byte(static_cast<std::uint8_t>(c.coordinate));
  e.byte(static_cast<std::uint8_t>(c.index));
  encode_digest(e, c.input_digest);
  encode_digest(e, c.prepared_digest);
  encode_digest(e, c.validation_options_digest);
  encode_digest(e, c.policy_digest);
  encode_digest(e, c.semantic_digest);
  encode_digest(e, c.validation_artifact_digest);
  encode_digest(e, c.validation_report_digest);
  encode_digest(e, c.invariant_set_digest);
  encode_digest(e, c.kernel_policy_digest);
  e.boolean(c.geometry_changed);
}

digest certificate_digest_for(const strict_validation_certificate &c) {
  canonical_encoder e;
  encode_certificate_body(e, c);
  return domain_digest(certificate_tag, e.bytes());
}

bool same_certificate(const strict_validation_certificate &a,
                      const strict_validation_certificate &b) {
  return a.schema == b.schema && a.checker_version == b.checker_version &&
         a.coordinate == b.coordinate && a.index == b.index &&
         a.input_digest == b.input_digest &&
         a.prepared_digest == b.prepared_digest &&
         a.validation_options_digest == b.validation_options_digest &&
         a.policy_digest == b.policy_digest &&
         a.semantic_digest == b.semantic_digest &&
         a.validation_artifact_digest == b.validation_artifact_digest &&
         a.validation_report_digest == b.validation_report_digest &&
         a.invariant_set_digest == b.invariant_set_digest &&
         a.kernel_policy_digest == b.kernel_policy_digest &&
         a.geometry_changed == b.geometry_changed &&
         a.certificate_digest == b.certificate_digest;
}

class reader {
  const std::vector<std::uint8_t> &bytes_;
  std::uint64_t limit_;
  std::size_t at_ = 0;

public:
  reader(const std::vector<std::uint8_t> &bytes, std::uint64_t limit)
      : bytes_(bytes), limit_(limit) {
    if (bytes.size() > limit_) throw std::length_error("record_limit");
  }
  std::uint8_t byte() {
    if (at_ == bytes_.size()) throw std::runtime_error("truncated");
    return bytes_[at_++];
  }
  std::uint16_t u16() {
    std::uint16_t v = 0;
    for (unsigned i = 0; i != 2; ++i) v = std::uint16_t((v << 8) | byte());
    return v;
  }
  std::uint32_t u32() {
    std::uint32_t v = 0;
    for (unsigned i = 0; i != 4; ++i) v = (v << 8) | byte();
    return v;
  }
  std::uint64_t u64() {
    std::uint64_t v = 0;
    for (unsigned i = 0; i != 8; ++i) v = (v << 8) | byte();
    return v;
  }
  std::int64_t signed_magnitude() {
    const bool negative = boolean();
    const auto magnitude = u64();
    if ((!negative && magnitude > std::uint64_t(std::numeric_limits<std::int64_t>::max())) ||
        (negative && magnitude >
                         std::uint64_t(std::numeric_limits<std::int64_t>::max()) +
                             1))
      throw std::runtime_error("signed_magnitude");
    if (!negative) return static_cast<std::int64_t>(magnitude);
    if (magnitude == 0) throw std::runtime_error("negative_zero");
    if (magnitude ==
        std::uint64_t(std::numeric_limits<std::int64_t>::max()) + 1)
      return std::numeric_limits<std::int64_t>::min();
    return -static_cast<std::int64_t>(magnitude);
  }
  bool boolean() {
    const auto v = byte();
    if (v > 1) throw std::runtime_error("boolean");
    return v != 0;
  }
  double floating64() {
    const auto bits = u64();
    double value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
  }
  digest digest_value() {
    digest d;
    for (auto &b : d.bytes) b = byte();
    return d;
  }
  std::vector<std::uint8_t> byte_string(std::uint64_t max) {
    const auto n = u64();
    if (n > max || n > bytes_.size() - at_)
      throw std::length_error("byte_string");
    std::vector<std::uint8_t> result(bytes_.begin() + at_,
                                     bytes_.begin() + at_ + n);
    at_ += static_cast<std::size_t>(n);
    return result;
  }
  std::uint64_t count(std::uint64_t max) {
    const auto n = u64();
    if (n > max || n > std::numeric_limits<std::size_t>::max())
      throw std::length_error("count");
    return n;
  }
  void tag(const std::array<char, 8> &tag_value) {
    for (char c : tag_value)
      if (byte() != static_cast<std::uint8_t>(c))
        throw std::runtime_error("tag");
  }
  bool done() const { return at_ == bytes_.size(); }
};

normalization_policy decode_policy_body(reader &r) {
  normalization_policy p;
  p.schema = r.u16();
  p.mode = static_cast<normalization_mode>(r.byte());
  p.unit = static_cast<model_unit>(r.byte());
  p.model_tolerance = r.floating64();
  p.enabled_operations = r.u64();
  p.edit_ordering_version = r.u16();
  p.diagnosis_version = r.u16();
  p.resources.max_work_units = r.u64();
  p.resources.max_defect_records = r.u64();
  p.resources.max_mapping_entries = r.u64();
  p.resources.max_report_bytes = r.u64();
  p.cancellation = static_cast<normalization_cancellation_policy>(r.byte());
  p.checkpoint_interval = r.u32();
  return p;
}

strict_validation_certificate decode_certificate(reader &r) {
  strict_validation_certificate c;
  c.schema = r.u16();
  c.checker_version = r.u16();
  c.coordinate = static_cast<coordinate_tag>(r.byte());
  c.index = static_cast<index_tag>(r.byte());
  c.input_digest = r.digest_value();
  c.prepared_digest = r.digest_value();
  c.validation_options_digest = r.digest_value();
  c.policy_digest = r.digest_value();
  c.semantic_digest = r.digest_value();
  c.validation_artifact_digest = r.digest_value();
  c.validation_report_digest = r.digest_value();
  c.invariant_set_digest = r.digest_value();
  c.kernel_policy_digest = r.digest_value();
  c.geometry_changed = r.boolean();
  c.certificate_digest = r.digest_value();
  return c;
}

void encode_mapping(canonical_encoder &e, const normalization_mapping &m) {
  e.byte(static_cast<std::uint8_t>(m.status));
  e.u64(m.source_to_prepared.size());
  for (auto v : m.source_to_prepared) e.u64(v);
}

normalization_mapping decode_mapping(reader &r, std::uint64_t max) {
  normalization_mapping m;
  m.status = static_cast<normalization_map_status>(r.byte());
  const auto n = r.count(max);
  m.source_to_prepared.reserve(static_cast<std::size_t>(n));
  for (std::uint64_t i = 0; i != n; ++i)
    m.source_to_prepared.push_back(r.u64());
  return m;
}

void encode_rational(canonical_encoder &e, const normalization_rational &r) {
  e.signed_magnitude(r.numerator);
  e.u64(r.denominator);
}

normalization_rational decode_rational(reader &r) {
  return {r.signed_magnitude(), r.u64()};
}

void encode_report_body(canonical_encoder &e, const normalization_report &r) {
  e.u16(r.schema);
  e.u16(r.producer_version);
  e.byte(static_cast<std::uint8_t>(r.coordinate));
  e.byte(static_cast<std::uint8_t>(r.index));
  canonical_encoder pe;
  encode_policy_body(pe, r.policy);
  e.byte_string(pe.bytes());
  encode_digest(e, r.policy_digest);
  encode_digest(e, r.source_digest);
  encode_digest(e, r.output_digest);
  e.boolean(r.prepared_operand_available);
  e.u64(r.edits.size());
  for (const auto &x : r.edits) {
    e.byte(static_cast<std::uint8_t>(x.operation));
    e.u64(x.canonical_ordinal);
    e.byte(static_cast<std::uint8_t>(x.entity));
    e.u64(x.source_ordinal);
    e.u64(x.prepared_ordinal);
    encode_digest(e, x.before_evidence_digest);
    encode_digest(e, x.after_evidence_digest);
    e.byte(static_cast<std::uint8_t>(x.reversibility));
    encode_digest(e, x.evidence_digest);
  }
  e.byte(static_cast<std::uint8_t>(r.displacement));
  e.u64(r.displacements.size());
  for (const auto &x : r.displacements) {
    e.u64(x.source_vertex);
    e.u64(x.prepared_vertex);
    e.byte(static_cast<std::uint8_t>(x.kind));
    for (const auto &component : x.exact_components)
      encode_rational(e, component);
    encode_rational(e, x.squared_distance_bound);
    e.byte(static_cast<std::uint8_t>(x.unit));
    encode_digest(e, x.evidence_digest);
  }
  e.u64(r.topology_changes.size());
  for (const auto &x : r.topology_changes) {
    e.byte(static_cast<std::uint8_t>(x.operation));
    e.u64(x.source_ordinal);
    e.byte(static_cast<std::uint8_t>(x.entity));
    e.u64(x.prepared_ordinal);
    e.byte(static_cast<std::uint8_t>(x.justification));
    e.u32(x.justification_subcode);
    encode_digest(e, x.before_evidence_digest);
    encode_digest(e, x.after_evidence_digest);
    e.byte(static_cast<std::uint8_t>(x.reversibility));
    encode_digest(e, x.evidence_digest);
  }
  e.u64(r.unresolved_defects.size());
  for (const auto &x : r.unresolved_defects) {
    e.u16(static_cast<std::uint16_t>(x.code));
    e.u64(x.primary_ordinal);
    e.u64(x.secondary_ordinal);
    e.u64(x.detail);
  }
  encode_mapping(e, r.vertices);
  e.u64(r.source_edges.size());
  for (const auto &edge : r.source_edges) {
    e.u64(edge[0]);
    e.u64(edge[1]);
  }
  encode_mapping(e, r.edges);
  encode_mapping(e, r.facets);
  encode_mapping(e, r.shells);
  e.u16(r.attributes.schema);
  encode_mapping(e, r.attributes.vertex_normals);
  encode_mapping(e, r.attributes.vertex_colours);
  encode_mapping(e, r.attributes.involved_faces);
  encode_mapping(e, r.attributes.metadata);
  e.byte(static_cast<std::uint8_t>(r.reversibility));
  e.boolean(r.strict_certificate.has_value());
  if (r.strict_certificate) {
    encode_certificate_body(e, *r.strict_certificate);
    encode_digest(e, r.strict_certificate->certificate_digest);
  }
}

bool canonical_defects(const std::vector<normalization_defect> &v) {
  const auto less = [](const normalization_defect &a,
                       const normalization_defect &b) {
    return std::tie(a.code, a.primary_ordinal, a.secondary_ordinal, a.detail) <
           std::tie(b.code, b.primary_ordinal, b.secondary_ordinal, b.detail);
  };
  return std::is_sorted(v.begin(), v.end(), less) &&
         std::adjacent_find(v.begin(), v.end()) == v.end();
}

bool identity_mapping(const normalization_mapping &m, std::uint64_t size) {
  if (m.status != normalization_map_status::total ||
      m.source_to_prepared.size() != size)
    return false;
  for (std::uint64_t i = 0; i != size; ++i)
    if (m.source_to_prepared[static_cast<std::size_t>(i)] != i) return false;
  return true;
}

bool canonical_mapping(const normalization_mapping &m) {
  if (!known(m.status)) return false;
  if (m.status != normalization_map_status::total)
    return m.source_to_prepared.empty();
  return identity_mapping(m, m.source_to_prepared.size());
}

bool canonical_rational(const normalization_rational &r) {
  if (r.denominator == 0) return false;
  const auto magnitude = r.numerator < 0
                             ? std::uint64_t(-(r.numerator + 1)) + 1
                             : std::uint64_t(r.numerator);
  return std::gcd(magnitude, r.denominator) == 1;
}

bool canonical_edit_records(const std::vector<normalization_edit> &records) {
  for (const auto &record : records)
    if (!known(record.operation) || !known(record.entity) ||
        !known(record.reversibility) ||
        !digest_nonzero(record.before_evidence_digest) ||
        !digest_nonzero(record.after_evidence_digest) ||
        !digest_nonzero(record.evidence_digest))
      return false;
  const auto less = [](const auto &a, const auto &b) {
    return std::tie(a.canonical_ordinal, a.operation, a.entity,
                    a.source_ordinal, a.prepared_ordinal) <
           std::tie(b.canonical_ordinal, b.operation, b.entity,
                    b.source_ordinal, b.prepared_ordinal);
  };
  return std::is_sorted(records.begin(), records.end(), less) &&
         std::adjacent_find(records.begin(), records.end(),
                            [&](const auto &a, const auto &b) {
                              return !less(a, b) && !less(b, a);
                            }) == records.end();
}

bool canonical_topology_records(
    const std::vector<normalization_topology_change> &records) {
  for (const auto &record : records)
    if (!known(record.operation) || !known(record.entity) ||
        !known(record.justification) || !known(record.reversibility) ||
        !digest_nonzero(record.before_evidence_digest) ||
        !digest_nonzero(record.after_evidence_digest) ||
        !digest_nonzero(record.evidence_digest))
      return false;
  const auto less = [](const auto &a, const auto &b) {
    return std::tie(a.operation, a.entity, a.source_ordinal,
                    a.prepared_ordinal, a.justification,
                    a.justification_subcode) <
           std::tie(b.operation, b.entity, b.source_ordinal,
                    b.prepared_ordinal, b.justification,
                    b.justification_subcode);
  };
  return std::is_sorted(records.begin(), records.end(), less) &&
         std::adjacent_find(records.begin(), records.end(),
                            [&](const auto &a, const auto &b) {
                              return !less(a, b) && !less(b, a);
                            }) == records.end();
}

bool canonical_displacements(
    const std::vector<normalization_displacement_record> &records) {
  for (const auto &record : records) {
    if (!known(record.kind) || !known(record.unit) ||
        !canonical_rational(record.squared_distance_bound) ||
        record.squared_distance_bound.numerator < 0 ||
        !digest_nonzero(record.evidence_digest))
      return false;
    for (const auto &component : record.exact_components)
      if (!canonical_rational(component)) return false;
  }
  const auto less = [](const auto &a, const auto &b) {
    return std::tie(a.source_vertex, a.prepared_vertex) <
           std::tie(b.source_vertex, b.prepared_vertex);
  };
  return std::is_sorted(records.begin(), records.end(), less) &&
         std::adjacent_find(records.begin(), records.end(),
                            [&](const auto &a, const auto &b) {
                              return !less(a, b) && !less(b, a);
                            }) == records.end();
}

status_or<bool> validate_report_shape(const normalization_report &r) {
  auto policy = validate_policy(r.policy, true);
  if (!policy.has_value()) return policy.error();
  if (r.schema != normalization_report_schema || r.producer_version != 1 ||
      r.coordinate > coordinate_tag::binary64 || r.index > index_tag::uint64 ||
      r.policy_digest != normalization_policy_digest(r.policy).value() ||
      !digest_nonzero(r.source_digest) || r.output_digest != r.source_digest ||
      !canonical_edit_records(r.edits) || !r.edits.empty() ||
      r.displacement != normalization_displacement_claim::exact_zero ||
      !canonical_displacements(r.displacements) || !r.displacements.empty() ||
      !canonical_topology_records(r.topology_changes) ||
      !r.topology_changes.empty() ||
      !canonical_defects(r.unresolved_defects) ||
      !known(r.vertices.status) || !known(r.edges.status) ||
      !known(r.facets.status) || !known(r.shells.status) ||
      r.vertices.status != normalization_map_status::total ||
      r.edges.status != normalization_map_status::total ||
      r.facets.status != normalization_map_status::total ||
      r.source_edges.size() != r.edges.source_to_prepared.size() ||
      !std::is_sorted(r.source_edges.begin(), r.source_edges.end()) ||
      std::adjacent_find(r.source_edges.begin(), r.source_edges.end()) !=
          r.source_edges.end() ||
      std::any_of(r.source_edges.begin(), r.source_edges.end(),
                  [](const auto &edge) { return edge[1] < edge[0]; }) ||
      !canonical_mapping(r.shells) ||
      (r.prepared_operand_available
           ? r.shells.status != normalization_map_status::total
           : r.shells.status != normalization_map_status::unavailable) ||
      r.attributes.schema != normalization_report_schema ||
      !canonical_mapping(r.attributes.vertex_normals) ||
      !canonical_mapping(r.attributes.vertex_colours) ||
      !canonical_mapping(r.attributes.involved_faces) ||
      !canonical_mapping(r.attributes.metadata) ||
      r.reversibility != normalization_reversibility::identity ||
      r.strict_certificate.has_value() != r.prepared_operand_available)
    return normalization_error("normalization_report_malformed");
  for (const auto &d : r.unresolved_defects)
    if (!known(d.code))
      return normalization_error("normalization_report_defect_enum");
  if (r.strict_certificate &&
      (r.strict_certificate->schema != prepared_operand_schema ||
       r.strict_certificate->geometry_changed ||
       r.strict_certificate->coordinate != r.coordinate ||
       r.strict_certificate->index != r.index ||
       r.strict_certificate->input_digest != r.source_digest ||
       r.strict_certificate->prepared_digest != r.output_digest ||
       r.strict_certificate->certificate_digest !=
           certificate_digest_for(*r.strict_certificate)))
    return normalization_error("normalization_report_certificate");
  return true;
}

struct budget {
  const normalization_policy &policy;
  cancellation_source *cancel;
  std::uint64_t work = 0;
  std::uint64_t records = 0;
  std::uint64_t mappings = 0;

  status_or<bool> checkpoint(std::uint64_t n = 1) {
    if (work > policy.resources.max_work_units ||
        n > policy.resources.max_work_units - work)
      return limit_error("normalization_work_limit", n,
                         policy.resources.max_work_units);
    work += n;
    if ((work % policy.checkpoint_interval) < n && cancel &&
        cancel->token().cancelled())
      return limit_error("normalization_cancelled");
    return true;
  }
  status_or<bool> record() {
    if (records == policy.resources.max_defect_records)
      return limit_error("normalization_defect_limit", records + 1,
                         policy.resources.max_defect_records);
    ++records;
    return true;
  }
  status_or<bool> mapping(std::uint64_t n) {
    if (mappings > policy.resources.max_mapping_entries ||
        n > policy.resources.max_mapping_entries - mappings)
      return limit_error("normalization_mapping_limit", n,
                         policy.resources.max_mapping_entries);
    mappings += n;
    return true;
  }
};

status_or<std::uint64_t> facet_work(std::uint64_t vertices,
                                    std::uint64_t prior_facets) {
  std::uint64_t levels = 1;
  auto comparison_domain = std::max(vertices, prior_facets);
  while (comparison_domain > 1) {
    comparison_domain = (comparison_domain + 1) / 2;
    ++levels;
  }
  if (levels > (std::numeric_limits<std::uint64_t>::max() - 8) / 2)
    return limit_error("normalization_work_overflow");
  const auto factor = levels * 2 + 8;
  if (vertices > std::numeric_limits<std::uint64_t>::max() / factor)
    return limit_error("normalization_work_overflow");
  return std::max<std::uint64_t>(1, vertices * factor);
}

status_or<std::uint64_t> sort_work(std::uint64_t records) {
  std::uint64_t levels = 1, remaining = records;
  while (remaining > 1) {
    remaining = (remaining + 1) / 2;
    ++levels;
  }
  if (records > std::numeric_limits<std::uint64_t>::max() / levels)
    return limit_error("normalization_work_overflow");
  return records * levels;
}

template <class T, class I>
status_or<std::uint64_t>
mesh_binding_work(const fv_surface_mesh<T, I> &mesh) {
  std::uint64_t total = 1;
  const auto add = [&](std::uint64_t count,
                       std::uint64_t width = 1) -> status_or<bool> {
    if (count > std::numeric_limits<std::uint64_t>::max() / width)
      return limit_error("normalization_work_overflow");
    const auto amount = count * width;
    if (amount > std::numeric_limits<std::uint64_t>::max() - total)
      return limit_error("normalization_work_overflow");
    total += amount;
    return true;
  };
  for (const auto entry :
       {std::make_pair<std::uint64_t, std::uint64_t>(mesh.vertices.size(), 3),
        {mesh.vertex_normals.size(), 3}, {mesh.vertex_colours.size(), 1},
        {mesh.faces.size(), 1}, {mesh.involved_faces.size(), 1},
        {mesh.metadata.size(), 1}}) {
    auto added = add(entry.first, entry.second);
    if (!added.has_value()) return added.error();
  }
  for (const auto &face : mesh.faces) {
    auto added = add(face.size());
    if (!added.has_value()) return added.error();
  }
  for (const auto &faces : mesh.involved_faces) {
    auto added = add(faces.size());
    if (!added.has_value()) return added.error();
  }
  for (const auto &metadata : mesh.metadata) {
    auto key = add(metadata.first.size());
    if (!key.has_value()) return key.error();
    auto value = add(metadata.second.size());
    if (!value.has_value()) return value.error();
  }
  return total;
}

template <class I>
std::vector<std::uint64_t> canonical_facet_key(const std::vector<I> &face) {
  std::vector<std::uint64_t> raw;
  raw.reserve(face.size());
  for (I i : face) raw.push_back(static_cast<std::uint64_t>(i));
  if (raw.empty()) return raw;
  const auto rotate_minimum = [](const std::vector<std::uint64_t> &values) {
    const auto start = input_topology_detail::minimal_cyclic_rotation(values);
    std::vector<std::uint64_t> result;
    result.reserve(values.size());
    for (std::size_t i = 0; i != values.size(); ++i)
      result.push_back(values[(start + i) % values.size()]);
    return result;
  };
  auto forward = rotate_minimum(raw);
  std::reverse(raw.begin(), raw.end());
  auto reverse = rotate_minimum(raw);
  return reverse < forward ? reverse : forward;
}

template <class T>
using coordinate_key =
    std::array<typename std::conditional<sizeof(T) == 4, std::uint32_t,
                                         std::uint64_t>::type,
               3>;

template <class T> coordinate_key<T> bits_key(T x, T y, T z) {
  using B = typename coordinate_key<T>::value_type;
  coordinate_key<T> result{};
  T values[3] = {x == T(0) ? T(0) : x, y == T(0) ? T(0) : y,
                 z == T(0) ? T(0) : z};
  for (unsigned i = 0; i != 3; ++i)
    std::memcpy(&result[i], &values[i], sizeof(B));
  return result;
}

struct diagnosis_result {
  std::vector<normalization_defect> defects;
  std::vector<std::array<std::uint64_t, 2>> edges;
};

template <class T, class I>
status_or<diagnosis_result> producer_diagnosis(const fv_surface_mesh<T, I> &m,
                                               budget &b) {
  diagnosis_result out;
  std::vector<bool> used(m.vertices.size(), false);
  std::map<coordinate_key<T>, std::uint64_t> coordinates;
  std::set<std::pair<std::uint64_t, std::uint64_t>> edges;
  std::map<std::vector<std::uint64_t>, std::uint64_t> facets;
  auto add = [&](normalization_defect d) -> status_or<bool> {
    auto allowed = b.record();
    if (!allowed.has_value()) return allowed.error();
    out.defects.push_back(d);
    return true;
  };
  for (std::uint64_t i = 0; i != m.vertices.size(); ++i) {
    auto checked = b.checkpoint(3);
    if (!checked.has_value()) return checked.error();
    const auto &v = m.vertices[static_cast<std::size_t>(i)];
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) {
      auto a = add({normalization_defect_code::nonfinite_coordinate, i, 0, 0});
      if (!a.has_value()) return a.error();
    } else {
      const auto key = bits_key(v.x, v.y, v.z);
      const auto inserted = coordinates.emplace(key, i);
      if (!inserted.second) {
        auto a = add({normalization_defect_code::exact_duplicate_vertex, i,
                      inserted.first->second, 0});
        if (!a.has_value()) return a.error();
      }
    }
  }
  for (std::uint64_t f = 0; f != m.faces.size(); ++f) {
    const auto &face = m.faces[static_cast<std::size_t>(f)];
    auto work = facet_work(face.size(), f);
    if (!work.has_value()) return work.error();
    auto checked = b.checkpoint(work.value());
    if (!checked.has_value()) return checked.error();
    if (face.size() < 3) {
      auto a = add({normalization_defect_code::short_face, f, 0, face.size()});
      if (!a.has_value()) return a.error();
    }
    bool indices_valid = true;
    for (std::size_t j = 0; j != face.size(); ++j) {
      const auto index = static_cast<std::uint64_t>(face[j]);
      if (index >= m.vertices.size()) {
        indices_valid = false;
        auto a = add({normalization_defect_code::index_out_of_range, f, j,
                      index});
        if (!a.has_value()) return a.error();
      } else {
        used[static_cast<std::size_t>(index)] = true;
      }
      if (face.size() > 1 && face[j] == face[(j + 1) % face.size()]) {
        auto a = add({normalization_defect_code::consecutive_duplicate_index,
                      f, j, index});
        if (!a.has_value()) return a.error();
      }
    }
    const auto key = canonical_facet_key(face);
    const auto inserted = facets.emplace(key, f);
    if (!inserted.second) {
      auto a = add({normalization_defect_code::exact_duplicate_facet, f,
                    inserted.first->second, 0});
      if (!a.has_value()) return a.error();
    }
    if (indices_valid && face.size() > 1)
      for (std::size_t j = 0; j != face.size(); ++j) {
        auto x = static_cast<std::uint64_t>(face[j]);
        auto y = static_cast<std::uint64_t>(face[(j + 1) % face.size()]);
        if (y < x) std::swap(x, y);
        edges.emplace(x, y);
      }
  }
  for (std::uint64_t i = 0; i != used.size(); ++i) {
    auto checked = b.checkpoint(1);
    if (!checked.has_value()) return checked.error();
    if (!used[static_cast<std::size_t>(i)]) {
      auto a = add({normalization_defect_code::unused_vertex, i, 0, 0});
      if (!a.has_value()) return a.error();
    }
  }
  for (auto mappings : {m.vertices.size(), m.faces.size(), edges.size(),
                        edges.size(),
                        m.vertex_normals.size(), m.vertex_colours.size(),
                        m.involved_faces.size(), m.metadata.size()}) {
    auto allowed = b.mapping(mappings);
    if (!allowed.has_value()) return allowed.error();
  }
  out.edges.reserve(edges.size());
  for (const auto &edge : edges) out.edges.push_back({{edge.first, edge.second}});
  auto sorting = sort_work(out.defects.size());
  if (!sorting.has_value()) return sorting.error();
  auto sort_allowed = b.checkpoint(sorting.value());
  if (!sort_allowed.has_value()) return sort_allowed.error();
  std::sort(out.defects.begin(), out.defects.end(),
            [](const auto &a, const auto &c) {
              return std::tie(a.code, a.primary_ordinal, a.secondary_ordinal,
                              a.detail) <
                     std::tie(c.code, c.primary_ordinal, c.secondary_ordinal,
                              c.detail);
            });
  out.defects.erase(std::unique(out.defects.begin(), out.defects.end()),
                    out.defects.end());
  return out;
}

struct verification_budget {
  const normalization_policy &policy;
  cancellation_source *cancel;
  std::uint64_t work = 0;
  std::uint64_t records = 0;

  status_or<bool> checkpoint(std::uint64_t amount) {
    if (work > policy.resources.max_work_units ||
        amount > policy.resources.max_work_units - work)
      return limit_error("normalization_verification_work_limit", amount,
                         policy.resources.max_work_units);
    work += amount;
    if ((work % policy.checkpoint_interval) < amount && cancel &&
        cancel->token().cancelled())
      return limit_error("normalization_verification_cancelled");
    return true;
  }
  status_or<bool> record() {
    if (records >= policy.resources.max_defect_records)
      return limit_error("normalization_verification_defect_limit", records + 1,
                         policy.resources.max_defect_records);
    ++records;
    return true;
  }
};

template <class T, class I>
status_or<diagnosis_result>
verifier_diagnosis(const fv_surface_mesh<T, I> &m, verification_budget &budget) {
  diagnosis_result result;
  const auto verifier_coordinate_key = [](T x, T y, T z) {
    coordinate_key<T> key{};
    T values[3] = {x == T(0) ? T(0) : x, y == T(0) ? T(0) : y,
                   z == T(0) ? T(0) : z};
    for (unsigned component = 0; component != 3; ++component)
      std::memcpy(&key[component], &values[component], sizeof(values[component]));
    return key;
  };
  const auto verifier_facet_key = [](const std::vector<I> &ring) {
    std::vector<std::uint64_t> values;
    values.reserve(ring.size());
    for (I value : ring) values.push_back(static_cast<std::uint64_t>(value));
    if (values.empty()) return values;
    const auto independently_minimize = [](const std::vector<std::uint64_t> &v) {
      std::size_t first = 0, second = 1, matched = 0;
      while (first < v.size() && second < v.size() && matched < v.size()) {
        const auto a = v[(first + matched) % v.size()];
        const auto b = v[(second + matched) % v.size()];
        if (a == b) {
          ++matched;
          continue;
        }
        if (b < a) {
          first += matched + 1;
          if (first == second) ++first;
        } else {
          second += matched + 1;
          if (first == second) ++second;
        }
        matched = 0;
      }
      const auto origin = std::min(first, second) % v.size();
      std::vector<std::uint64_t> out;
      out.reserve(v.size());
      for (std::size_t i = 0; i != v.size(); ++i)
        out.push_back(v[(origin + i) % v.size()]);
      return out;
    };
    auto forward = independently_minimize(values);
    std::reverse(values.begin(), values.end());
    auto reverse = independently_minimize(values);
    return reverse < forward ? reverse : forward;
  };
  std::vector<unsigned char> referenced(m.vertices.size(), 0);
  std::map<coordinate_key<T>, std::uint64_t> prior_points;
  std::set<std::pair<std::uint64_t, std::uint64_t>> unique_edges;
  std::map<std::vector<std::uint64_t>, std::uint64_t> prior_faces;
  const auto add = [&](normalization_defect defect) -> status_or<bool> {
    auto allowed = budget.record();
    if (!allowed.has_value()) return allowed.error();
    result.defects.push_back(defect);
    return true;
  };
  for (std::uint64_t ordinal = 0; ordinal != m.vertices.size(); ++ordinal) {
    auto work = budget.checkpoint(3);
    if (!work.has_value()) return work.error();
    const auto &p = m.vertices[static_cast<std::size_t>(ordinal)];
    if (!(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z))) {
      auto added = add(
          {normalization_defect_code::nonfinite_coordinate, ordinal, 0, 0});
      if (!added.has_value()) return added.error();
    } else {
      auto inserted =
          prior_points.emplace(verifier_coordinate_key(p.x, p.y, p.z), ordinal);
      if (!inserted.second) {
        auto added = add({normalization_defect_code::exact_duplicate_vertex,
                          ordinal, inserted.first->second, 0});
        if (!added.has_value()) return added.error();
      }
    }
  }
  for (std::uint64_t ordinal = 0; ordinal != m.faces.size(); ++ordinal) {
    const auto &ring = m.faces[static_cast<std::size_t>(ordinal)];
    auto required = facet_work(ring.size(), ordinal);
    if (!required.has_value()) return required.error();
    auto work = budget.checkpoint(required.value());
    if (!work.has_value()) return work.error();
    if (ring.size() < 3) {
      auto added = add({normalization_defect_code::short_face, ordinal, 0,
                        ring.size()});
      if (!added.has_value()) return added.error();
    }
    bool safe = true;
    for (std::size_t offset = 0; offset != ring.size(); ++offset) {
      const auto value = static_cast<std::uint64_t>(ring[offset]);
      if (value >= m.vertices.size()) {
        safe = false;
        auto added = add({normalization_defect_code::index_out_of_range,
                          ordinal, offset, value});
        if (!added.has_value()) return added.error();
      } else
        referenced[static_cast<std::size_t>(value)] = 1;
      if (ring.size() > 1 && ring[offset] == ring[(offset + 1) % ring.size()]) {
        auto added = add(
            {normalization_defect_code::consecutive_duplicate_index, ordinal,
             offset, value});
        if (!added.has_value()) return added.error();
      }
    }
    auto inserted = prior_faces.emplace(verifier_facet_key(ring), ordinal);
    if (!inserted.second) {
      auto added = add({normalization_defect_code::exact_duplicate_facet,
                        ordinal, inserted.first->second, 0});
      if (!added.has_value()) return added.error();
    }
    if (safe && ring.size() > 1)
      for (std::size_t offset = 0; offset != ring.size(); ++offset) {
        auto a = static_cast<std::uint64_t>(ring[offset]);
        auto c = static_cast<std::uint64_t>(ring[(offset + 1) % ring.size()]);
        unique_edges.emplace(std::min(a, c), std::max(a, c));
      }
  }
  for (std::uint64_t ordinal = 0; ordinal != referenced.size(); ++ordinal) {
    auto work = budget.checkpoint(1);
    if (!work.has_value()) return work.error();
    if (!referenced[static_cast<std::size_t>(ordinal)]) {
      auto added = add(
          {normalization_defect_code::unused_vertex, ordinal, 0, 0});
      if (!added.has_value()) return added.error();
    }
  }
  result.edges.reserve(unique_edges.size());
  for (const auto &edge : unique_edges)
    result.edges.push_back({{edge.first, edge.second}});
  auto sorting = sort_work(result.defects.size());
  if (!sorting.has_value()) return sorting.error();
  auto sort_allowed = budget.checkpoint(sorting.value());
  if (!sort_allowed.has_value()) return sort_allowed.error();
  std::sort(result.defects.begin(), result.defects.end(),
            [](const auto &a, const auto &b) {
              return std::tie(a.code, a.primary_ordinal, a.secondary_ordinal,
                              a.detail) <
                     std::tie(b.code, b.primary_ordinal, b.secondary_ordinal,
                              b.detail);
            });
  result.defects.erase(
      std::unique(result.defects.begin(), result.defects.end()),
      result.defects.end());
  return result;
}

normalization_mapping identity(std::uint64_t count) {
  normalization_mapping result;
  result.status = normalization_map_status::total;
  result.source_to_prepared.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i != count; ++i)
    result.source_to_prepared.push_back(i);
  return result;
}

normalization_mapping attribute_identity(bool present, std::uint64_t count) {
  if (!present)
    return {normalization_map_status::absent, {}};
  return identity(count);
}

std::shared_ptr<verifier_registry> component2_registry() {
  auto registry = std::make_shared<verifier_registry>();
  for (auto c : {coordinate_tag::binary32, coordinate_tag::binary64})
    for (auto i : {index_tag::uint32, index_tag::uint64}) {
      auto registered = register_input_topology_verifier(*registry, c, i);
      if (!registered.has_value()) throw std::runtime_error("registry");
    }
  auto frozen = registry->freeze();
  if (!frozen.has_value()) throw std::runtime_error("registry_freeze");
  return registry;
}

template <class T, class I>
status_or<std::uint64_t>
component2_shell_count(const fv_surface_mesh<T, I> &source,
                       cancellation_source *cancel) {
  auto registry = component2_registry();
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  fv_surface_mesh<T, I> empty;
  auto context = make_boolean_context(source, empty, operation::regularized_union,
                                      boolean_options{}, kernel, verifier,
                                      cancel);
  if (!context.has_value()) return context.error();
  auto validated = validate_operands(*context.value());
  if (!validated.has_value()) return validated.error();
  return validated.value()->payload->operands[0].shells.size();
}

status_or<std::uint64_t>
estimated_report_bytes(const normalization_report &report) {
  std::uint64_t total = 1024;
  const auto add_product = [&](std::uint64_t count,
                               std::uint64_t width) -> status_or<bool> {
    if (count > std::numeric_limits<std::uint64_t>::max() / width)
      return limit_error("normalization_report_size_overflow");
    const auto amount = count * width;
    if (amount > std::numeric_limits<std::uint64_t>::max() - total)
      return limit_error("normalization_report_size_overflow");
    total += amount;
    return true;
  };
  for (const auto &entry :
       {std::make_pair<std::uint64_t, std::uint64_t>(report.edits.size(), 80),
        {report.displacements.size(), 112},
        {report.topology_changes.size(), 80},
        {report.unresolved_defects.size(), 32},
        {report.source_edges.size(), 16}}) {
    auto added = add_product(entry.first, entry.second);
    if (!added.has_value()) return added.error();
  }
  for (const auto *mapping :
       {&report.vertices, &report.edges, &report.facets, &report.shells,
        &report.attributes.vertex_normals, &report.attributes.vertex_colours,
        &report.attributes.involved_faces, &report.attributes.metadata}) {
    auto added = add_product(mapping->source_to_prepared.size(), 8);
    if (!added.has_value()) return added.error();
  }
  return total;
}

status_or<std::uint64_t>
report_mapping_entries(const normalization_report &report) {
  std::uint64_t total = report.source_edges.size();
  for (const auto *mapping :
       {&report.vertices, &report.edges, &report.facets, &report.shells,
        &report.attributes.vertex_normals, &report.attributes.vertex_colours,
        &report.attributes.involved_faces, &report.attributes.metadata}) {
    const auto size = mapping->source_to_prepared.size();
    if (size > std::numeric_limits<std::uint64_t>::max() - total)
      return limit_error("normalization_mapping_overflow");
    total += size;
  }
  return total;
}

} // namespace

status_or<std::vector<std::uint8_t>>
encode_normalization_policy(const normalization_policy &p) {
  auto valid = validate_policy(p, false);
  if (!valid.has_value()) return valid.error();
  canonical_encoder e;
  e.raw(reinterpret_cast<const std::uint8_t *>(policy_tag.data()),
        policy_tag.size());
  encode_policy_body(e, p);
  return e.bytes();
}

status_or<normalization_policy>
decode_normalization_policy(const std::vector<std::uint8_t> &bytes) {
  try {
    reader r(bytes, 1024);
    r.tag(policy_tag);
    auto result = decode_policy_body(r);
    if (!r.done()) return normalization_error("normalization_policy_trailing");
    auto valid = validate_policy(result, false);
    if (!valid.has_value()) return valid.error();
    auto canonical = encode_normalization_policy(result);
    if (!canonical.has_value() || canonical.value() != bytes)
      return normalization_error("normalization_policy_noncanonical");
    return result;
  } catch (const std::length_error &) {
    return limit_error("normalization_policy_decode_limit");
  } catch (const std::exception &) {
    return normalization_error("normalization_policy_decode");
  }
}

status_or<digest> normalization_policy_digest(const normalization_policy &p) {
  auto encoded = encode_normalization_policy(p);
  if (!encoded.has_value()) return encoded.error();
  return domain_digest(policy_tag, encoded.value());
}

status_or<digest> normalization_report_digest(const normalization_report &r) {
  auto shape = validate_report_shape(r);
  if (!shape.has_value()) return shape.error();
  canonical_encoder body;
  encode_report_body(body, r);
  return domain_digest(report_digest_tag, body.bytes());
}

status_or<std::vector<std::uint8_t>>
encode_normalization_report(const normalization_report &r) {
  try {
    auto shape = validate_report_shape(r);
    if (!shape.has_value()) return shape.error();
    auto expected = normalization_report_digest(r);
    if (!expected.has_value()) return expected.error();
    if (r.report_digest != expected.value())
      return normalization_error("normalization_report_stale_digest");
    canonical_encoder e;
    e.raw(reinterpret_cast<const std::uint8_t *>(report_tag.data()),
          report_tag.size());
    encode_report_body(e, r);
    encode_digest(e, r.report_digest);
    return e.bytes();
  } catch (const std::bad_alloc &) {
    return limit_error("normalization_report_encode_allocation");
  }
}

status_or<normalization_report> decode_normalization_report(
    const std::vector<std::uint8_t> &bytes,
    const normalization_decode_limits &limits) {
  try {
    reader r(bytes, limits.max_record_bytes);
    r.tag(report_tag);
    normalization_report out;
    out.schema = r.u16();
    out.producer_version = r.u16();
    out.coordinate = static_cast<coordinate_tag>(r.byte());
    out.index = static_cast<index_tag>(r.byte());
    auto policy_bytes = r.byte_string(1024);
    auto policy = decode_normalization_policy([&] {
      canonical_encoder e;
      e.raw(reinterpret_cast<const std::uint8_t *>(policy_tag.data()),
            policy_tag.size());
      e.raw(policy_bytes.data(), policy_bytes.size());
      return e.bytes();
    }());
    if (!policy.has_value()) return policy.error();
    out.policy = policy.value();
    out.policy_digest = r.digest_value();
    out.source_digest = r.digest_value();
    out.output_digest = r.digest_value();
    out.prepared_operand_available = r.boolean();
    const auto edits = r.count(limits.max_edits);
    out.edits.reserve(static_cast<std::size_t>(edits));
    for (std::uint64_t i = 0; i != edits; ++i) {
      normalization_edit edit;
      edit.operation = static_cast<normalization_operation>(r.byte());
      edit.canonical_ordinal = r.u64();
      edit.entity = static_cast<normalization_entity_kind>(r.byte());
      edit.source_ordinal = r.u64();
      edit.prepared_ordinal = r.u64();
      edit.before_evidence_digest = r.digest_value();
      edit.after_evidence_digest = r.digest_value();
      edit.reversibility =
          static_cast<normalization_reversibility>(r.byte());
      edit.evidence_digest = r.digest_value();
      out.edits.push_back(std::move(edit));
    }
    out.displacement =
        static_cast<normalization_displacement_claim>(r.byte());
    const auto displaced = r.count(limits.max_displacements);
    out.displacements.reserve(static_cast<std::size_t>(displaced));
    for (std::uint64_t i = 0; i != displaced; ++i) {
      normalization_displacement_record displacement;
      displacement.source_vertex = r.u64();
      displacement.prepared_vertex = r.u64();
      displacement.kind =
          static_cast<normalization_displacement_kind>(r.byte());
      for (auto &component : displacement.exact_components)
        component = decode_rational(r);
      displacement.squared_distance_bound = decode_rational(r);
      displacement.unit = static_cast<model_unit>(r.byte());
      displacement.evidence_digest = r.digest_value();
      out.displacements.push_back(std::move(displacement));
    }
    const auto changes = r.count(limits.max_topology_changes);
    out.topology_changes.reserve(static_cast<std::size_t>(changes));
    for (std::uint64_t i = 0; i != changes; ++i) {
      normalization_topology_change change;
      change.operation = static_cast<normalization_operation>(r.byte());
      change.source_ordinal = r.u64();
      change.entity = static_cast<normalization_entity_kind>(r.byte());
      change.prepared_ordinal = r.u64();
      change.justification =
          static_cast<normalization_topology_justification>(r.byte());
      change.justification_subcode = r.u32();
      change.before_evidence_digest = r.digest_value();
      change.after_evidence_digest = r.digest_value();
      change.reversibility =
          static_cast<normalization_reversibility>(r.byte());
      change.evidence_digest = r.digest_value();
      out.topology_changes.push_back(std::move(change));
    }
    const auto defects = r.count(limits.max_defect_records);
    out.unresolved_defects.reserve(static_cast<std::size_t>(defects));
    for (std::uint64_t i = 0; i != defects; ++i)
      out.unresolved_defects.push_back(
          {static_cast<normalization_defect_code>(r.u16()), r.u64(), r.u64(),
           r.u64()});
    out.vertices = decode_mapping(r, limits.max_mapping_entries);
    const auto source_edges = r.count(limits.max_mapping_entries);
    out.source_edges.reserve(static_cast<std::size_t>(source_edges));
    for (std::uint64_t i = 0; i != source_edges; ++i)
      out.source_edges.push_back({{r.u64(), r.u64()}});
    out.edges = decode_mapping(r, limits.max_mapping_entries);
    out.facets = decode_mapping(r, limits.max_mapping_entries);
    out.shells = decode_mapping(r, limits.max_mapping_entries);
    out.attributes.schema = r.u16();
    out.attributes.vertex_normals =
        decode_mapping(r, limits.max_mapping_entries);
    out.attributes.vertex_colours =
        decode_mapping(r, limits.max_mapping_entries);
    out.attributes.involved_faces =
        decode_mapping(r, limits.max_mapping_entries);
    out.attributes.metadata = decode_mapping(r, limits.max_mapping_entries);
    std::uint64_t mapping_entries = 0;
    for (auto size : {out.vertices.source_to_prepared.size(),
                      out.source_edges.size(),
                      out.edges.source_to_prepared.size(),
                      out.facets.source_to_prepared.size(),
                      out.shells.source_to_prepared.size(),
                      out.attributes.vertex_normals.source_to_prepared.size(),
                      out.attributes.vertex_colours.source_to_prepared.size(),
                      out.attributes.involved_faces.source_to_prepared.size(),
                      out.attributes.metadata.source_to_prepared.size()}) {
      if (size > limits.max_mapping_entries - mapping_entries)
        throw std::length_error("mapping_total");
      mapping_entries += static_cast<std::uint64_t>(size);
    }
    out.reversibility = static_cast<normalization_reversibility>(r.byte());
    if (r.boolean()) out.strict_certificate = decode_certificate(r);
    out.report_digest = r.digest_value();
    if (!r.done()) return normalization_error("normalization_report_trailing");
    auto shape = validate_report_shape(out);
    if (!shape.has_value()) return shape.error();
    auto expected = normalization_report_digest(out);
    if (!expected.has_value() || expected.value() != out.report_digest)
      return normalization_error("normalization_report_stale_digest");
    auto canonical = encode_normalization_report(out);
    if (!canonical.has_value() || canonical.value() != bytes)
      return normalization_error("normalization_report_noncanonical");
    return out;
  } catch (const std::length_error &) {
    return limit_error("normalization_report_decode_limit");
  } catch (const std::bad_alloc &) {
    return limit_error("normalization_report_decode_allocation");
  } catch (const std::exception &) {
    return normalization_error("normalization_report_decode");
  }
}

template <class T, class I>
status_or<prepared_operand<T, I>> normalize_operand(
    const fv_surface_mesh<T, I> &source, const normalization_policy &policy,
    normalization_report &published, cancellation_source *cancel) {
  auto valid_policy = validate_policy(policy, true);
  if (!valid_policy.has_value()) return valid_policy.error();
  if (cancel && cancel->token().cancelled())
    return limit_error("normalization_cancelled");
  try {
    budget resources{policy, cancel};
    auto binding_work = mesh_binding_work(source);
    if (!binding_work.has_value()) return binding_work.error();
    auto binding_allowed = resources.checkpoint(binding_work.value());
    if (!binding_allowed.has_value()) return binding_allowed.error();
    auto diagnosed = producer_diagnosis(source, resources);
    if (!diagnosed.has_value()) return diagnosed.error();

    normalization_report candidate;
    candidate.coordinate = coordinate_type(std::is_same<T, float>::value);
    candidate.index = index_type(std::is_same<I, std::uint32_t>::value);
    candidate.policy = policy;
    candidate.policy_digest = normalization_policy_digest(policy).value();
    candidate.source_digest =
        preparation_detail::canonical_mesh_digest(source);
    candidate.output_digest = candidate.source_digest;
    candidate.unresolved_defects = std::move(diagnosed.value().defects);
    candidate.vertices = identity(source.vertices.size());
    candidate.source_edges = diagnosed.value().edges;
    candidate.edges = identity(candidate.source_edges.size());
    candidate.facets = identity(source.faces.size());
    candidate.shells.status = normalization_map_status::unavailable;
    candidate.attributes.vertex_normals = attribute_identity(
        !source.vertex_normals.empty(), source.vertex_normals.size());
    candidate.attributes.vertex_colours = attribute_identity(
        !source.vertex_colours.empty(), source.vertex_colours.size());
    candidate.attributes.involved_faces = attribute_identity(
        !source.involved_faces.empty(), source.involved_faces.size());
    candidate.attributes.metadata =
        attribute_identity(!source.metadata.empty(), source.metadata.size());

    auto registry = component2_registry();
    std::shared_ptr<const exact_kernel_services<T>> kernel =
        std::make_shared<exact_kernel<T>>();
    std::shared_ptr<const verifier_service> verifier = registry;
    auto prepared = validate_operand_strict(source, strict_validation_policy{},
                                            boolean_options{}, kernel, verifier,
                                            cancel);
    if (!prepared.has_value()) {
      if (prepared.error().code != boolean_error_code::input_contract_error)
        return prepared.error();
      auto record = resources.record();
      if (!record.has_value()) return record.error();
      candidate.unresolved_defects.push_back(
          {normalization_defect_code::component2_rejection,
           prepared.error().subcode, 0, 0});
      std::sort(candidate.unresolved_defects.begin(),
                candidate.unresolved_defects.end(), [](const auto &a, const auto &b) {
                  return std::tie(a.code, a.primary_ordinal,
                                  a.secondary_ordinal, a.detail) <
                         std::tie(b.code, b.primary_ordinal,
                                  b.secondary_ordinal, b.detail);
                });
      candidate.unresolved_defects.erase(
          std::unique(candidate.unresolved_defects.begin(),
                      candidate.unresolved_defects.end()),
          candidate.unresolved_defects.end());
      auto estimate = estimated_report_bytes(candidate);
      if (!estimate.has_value()) return estimate.error();
      if (estimate.value() > policy.resources.max_report_bytes)
        return limit_error("normalization_report_limit", estimate.value(),
                           policy.resources.max_report_bytes);
      candidate.report_digest = normalization_report_digest(candidate).value();
      auto bytes = encode_normalization_report(candidate);
      if (!bytes.has_value()) return bytes.error();
      if (bytes.value().size() > policy.resources.max_report_bytes)
        return limit_error("normalization_report_limit", bytes.value().size(),
                           policy.resources.max_report_bytes);
      published = std::move(candidate);
      return prepared.error();
    }

    candidate.prepared_operand_available = true;
    candidate.strict_certificate = prepared.value().certificate();
    auto shells = component2_shell_count(source, cancel);
    if (!shells.has_value()) return shells.error();
    auto shell_resources = resources.mapping(shells.value());
    if (!shell_resources.has_value()) return shell_resources.error();
    candidate.shells = identity(shells.value());
    auto estimate = estimated_report_bytes(candidate);
    if (!estimate.has_value()) return estimate.error();
    if (estimate.value() > policy.resources.max_report_bytes)
      return limit_error("normalization_report_limit", estimate.value(),
                         policy.resources.max_report_bytes);
    candidate.report_digest = normalization_report_digest(candidate).value();
    auto bytes = encode_normalization_report(candidate);
    if (!bytes.has_value()) return bytes.error();
    if (bytes.value().size() > policy.resources.max_report_bytes)
      return limit_error("normalization_report_limit", bytes.value().size(),
                         policy.resources.max_report_bytes);
    auto state = std::make_shared<typename prepared_operand<T, I>::state>();
    state->mesh = prepared.value().mesh();
    state->policy = prepared.value().policy();
    state->certificate = prepared.value().certificate();
    state->normalization =
        std::make_shared<const normalization_report>(candidate);
    prepared_operand<T, I> normalized(std::move(state));
    published = std::move(candidate);
    return normalized;
  } catch (const std::bad_alloc &) {
    return limit_error("normalization_allocation");
  } catch (const std::exception &e) {
    auto error = make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::input_validation,
                            "normalization_exception");
    error.detail = e.what();
    return error;
  }
}

template <class T, class I>
status_or<bool> verify_normalization_report(
    const std::vector<std::uint8_t> &bytes,
    const fv_surface_mesh<T, I> &source,
    const fv_surface_mesh<T, I> *output, cancellation_source *cancel) {
  if (cancel && cancel->token().cancelled())
    return limit_error("normalization_verification_cancelled");
  auto decoded = decode_normalization_report(bytes);
  if (!decoded.has_value()) return decoded.error();
  const auto &report = decoded.value();
  if (bytes.size() > report.policy.resources.max_report_bytes)
    return limit_error("normalization_verification_report_limit", bytes.size(),
                       report.policy.resources.max_report_bytes);
  if (report.unresolved_defects.size() >
      report.policy.resources.max_defect_records)
    return limit_error("normalization_verification_defect_limit",
                       report.unresolved_defects.size(),
                       report.policy.resources.max_defect_records);
  auto mapping_entries = report_mapping_entries(report);
  if (!mapping_entries.has_value()) return mapping_entries.error();
  if (mapping_entries.value() > report.policy.resources.max_mapping_entries)
    return limit_error("normalization_verification_mapping_limit",
                       mapping_entries.value(),
                       report.policy.resources.max_mapping_entries);
  verification_budget verification_resources{report.policy, cancel};
  auto binding_work = mesh_binding_work(source);
  if (!binding_work.has_value()) return binding_work.error();
  auto binding_allowed = verification_resources.checkpoint(binding_work.value());
  if (!binding_allowed.has_value()) return binding_allowed.error();
  if (report.coordinate != coordinate_type(std::is_same<T, float>::value) ||
      report.index != index_type(std::is_same<I, std::uint32_t>::value) ||
      report.source_digest !=
          preparation_detail::canonical_mesh_digest(source) ||
      report.output_digest != report.source_digest ||
      report.prepared_operand_available != (output != nullptr) ||
      (output && preparation_detail::canonical_mesh_digest(*output) !=
                     report.output_digest) ||
      !identity_mapping(report.vertices, source.vertices.size()) ||
      !identity_mapping(report.facets, source.faces.size()) ||
      report.attributes.vertex_normals.status !=
          (source.vertex_normals.empty() ? normalization_map_status::absent
                                         : normalization_map_status::total) ||
      (source.vertex_normals.empty()
           ? !report.attributes.vertex_normals.source_to_prepared.empty()
           : !identity_mapping(report.attributes.vertex_normals,
                               source.vertex_normals.size())) ||
      report.attributes.vertex_colours.status !=
          (source.vertex_colours.empty() ? normalization_map_status::absent
                                         : normalization_map_status::total) ||
      (source.vertex_colours.empty()
           ? !report.attributes.vertex_colours.source_to_prepared.empty()
           : !identity_mapping(report.attributes.vertex_colours,
                               source.vertex_colours.size())) ||
      report.attributes.involved_faces.status !=
          (source.involved_faces.empty() ? normalization_map_status::absent
                                         : normalization_map_status::total) ||
      (source.involved_faces.empty()
           ? !report.attributes.involved_faces.source_to_prepared.empty()
           : !identity_mapping(report.attributes.involved_faces,
                               source.involved_faces.size())) ||
      report.attributes.metadata.status !=
          (source.metadata.empty() ? normalization_map_status::absent
                                   : normalization_map_status::total) ||
      (source.metadata.empty()
           ? !report.attributes.metadata.source_to_prepared.empty()
           : !identity_mapping(report.attributes.metadata,
                               source.metadata.size())))
    return normalization_error("normalization_report_stale_binding");

  auto independent = verifier_diagnosis(source, verification_resources);
  if (!independent.has_value()) return independent.error();
  if (report.source_edges != independent.value().edges ||
      !identity_mapping(report.edges, independent.value().edges.size()))
    return normalization_error("normalization_report_edge_map");

  auto registry = component2_registry();
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  auto replay = validate_operand_strict(source, strict_validation_policy{},
                                        boolean_options{}, kernel, verifier,
                                        cancel);
  if (replay.has_value()) {
    auto shells = component2_shell_count(source, cancel);
    if (!shells.has_value()) return shells.error();
    if (!report.prepared_operand_available || !report.strict_certificate ||
        independent.value().defects != report.unresolved_defects ||
        !identity_mapping(report.shells, shells.value()) ||
        !same_certificate(replay.value().certificate(),
                          *report.strict_certificate))
      return normalization_error("normalization_report_success_claim");
  } else {
    if (replay.error().code != boolean_error_code::input_contract_error)
      return replay.error();
    independent.value().defects.push_back(
        {normalization_defect_code::component2_rejection,
         replay.error().subcode, 0, 0});
    std::sort(independent.value().defects.begin(),
              independent.value().defects.end(),
              [](const auto &a, const auto &b) {
                return std::tie(a.code, a.primary_ordinal,
                                a.secondary_ordinal, a.detail) <
                       std::tie(b.code, b.primary_ordinal,
                                b.secondary_ordinal, b.detail);
              });
    independent.value().defects.erase(
        std::unique(independent.value().defects.begin(),
                    independent.value().defects.end()),
        independent.value().defects.end());
    if (report.prepared_operand_available || report.strict_certificate ||
        output || report.shells.status != normalization_map_status::unavailable ||
        !report.shells.source_to_prepared.empty() ||
        independent.value().defects != report.unresolved_defects)
      return normalization_error("normalization_report_failure_claim");
  }
  return true;
}

#define YGOR_NORMALIZATION_INSTANTIATE(T, I)                                    \
  template status_or<prepared_operand<T, I>> normalize_operand(                 \
      const fv_surface_mesh<T, I> &, const normalization_policy &,              \
      normalization_report &, cancellation_source *);                           \
  template status_or<bool> verify_normalization_report(                         \
      const std::vector<std::uint8_t> &, const fv_surface_mesh<T, I> &,          \
      const fv_surface_mesh<T, I> *, cancellation_source *)

YGOR_NORMALIZATION_INSTANTIATE(float, std::uint32_t);
YGOR_NORMALIZATION_INSTANTIATE(float, std::uint64_t);
YGOR_NORMALIZATION_INSTANTIATE(double, std::uint32_t);
YGOR_NORMALIZATION_INSTANTIATE(double, std::uint64_t);

#undef YGOR_NORMALIZATION_INSTANTIATE

} // namespace mesh_boolean
} // namespace ygor
