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
constexpr std::array<char, 8> removal_before_tag{{'Y', 'G', 'B', 'N', 'R', 'B', '0', '1'}};
constexpr std::array<char, 8> removal_after_tag{{'Y', 'G', 'B', 'N', 'R', 'A', '0', '1'}};
constexpr std::array<char, 8> removal_edit_tag{{'Y', 'G', 'B', 'N', 'R', 'E', '0', '1'}};
constexpr std::array<char, 8> duplicate_before_tag{{'Y', 'G', 'B', 'N', 'D', 'B', '0', '1'}};
constexpr std::array<char, 8> duplicate_after_tag{{'Y', 'G', 'B', 'N', 'D', 'A', '0', '1'}};
constexpr std::array<char, 8> duplicate_edit_tag{{'Y', 'G', 'B', 'N', 'D', 'E', '0', '1'}};
constexpr std::array<char, 8> duplicate_topology_tag{{'Y', 'G', 'B', 'N', 'D', 'T', '0', '1'}};
constexpr std::array<char, 8> orientation_before_tag{{'Y', 'G', 'B', 'N', 'O', 'B', '0', '1'}};
constexpr std::array<char, 8> orientation_after_tag{{'Y', 'G', 'B', 'N', 'O', 'A', '0', '1'}};
constexpr std::array<char, 8> orientation_edit_tag{{'Y', 'G', 'B', 'N', 'O', 'E', '0', '1'}};
constexpr std::array<char, 8> seam_before_tag{{'Y', 'G', 'B', 'N', 'S', 'B', '0', '1'}};
constexpr std::array<char, 8> seam_after_tag{{'Y', 'G', 'B', 'N', 'S', 'A', '0', '1'}};
constexpr std::array<char, 8> seam_edit_tag{{'Y', 'G', 'B', 'N', 'S', 'E', '0', '1'}};
constexpr std::array<char, 8> seam_topology_tag{{'Y', 'G', 'B', 'N', 'S', 'T', '0', '1'}};
constexpr std::array<char, 8> seam_displacement_tag{{'Y', 'G', 'B', 'N', 'S', 'D', '0', '1'}};
constexpr std::array<char, 8> crack_edit_tag{{'Y', 'G', 'B', 'N', 'C', 'E', '0', '1'}};
constexpr std::array<char, 8> crack_topology_tag{{'Y', 'G', 'B', 'N', 'C', 'T', '0', '1'}};
constexpr std::array<char, 8> crack_displacement_tag{{'Y', 'G', 'B', 'N', 'C', 'D', '0', '1'}};

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
         v <= normalization_defect_code::small_gap_candidate;
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
  const auto removal = normalization_operation_bit(
      normalization_operation::irrelevant_storage_removal);
  const auto duplicates = normalization_operation_bit(
      normalization_operation::exact_duplicate_consolidation);
  const auto orientation = normalization_operation_bit(
      normalization_operation::orientation_repair);
  const auto seam_vertices = normalization_operation_bit(
      normalization_operation::seam_aware_vertex_consolidation);
  const auto crack_closure = normalization_operation_bit(
      normalization_operation::crack_closure);
  if (executable &&
      !((p.mode == normalization_mode::diagnosis_only &&
          p.enabled_operations == 0) ||
         (p.mode == normalization_mode::structural_only &&
           (p.enabled_operations == removal ||
             p.enabled_operations == duplicates ||
             p.enabled_operations == orientation)) ||
         (p.mode == normalization_mode::geometry_changing &&
           (p.enabled_operations == seam_vertices ||
            p.enabled_operations == crack_closure) &&
           p.model_tolerance > 0.0 &&
          p.unit != model_unit::unspecified &&
          p.model_tolerance <= 2147483647.0)))
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
  return true;
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
  const bool diagnosis = r.policy.mode == normalization_mode::diagnosis_only;
  const bool duplicate_repair =
      r.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                              exact_duplicate_consolidation);
  const bool orientation_repair =
      r.policy.enabled_operations == normalization_operation_bit(
                                          normalization_operation::
                                              orientation_repair);
  const bool seam_repair =
      r.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             seam_aware_vertex_consolidation);
  const bool crack_repair =
      r.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::crack_closure);
  if (r.schema != normalization_report_schema || r.producer_version != 1 ||
      r.coordinate > coordinate_tag::binary64 || r.index > index_tag::uint64 ||
      r.policy_digest != normalization_policy_digest(r.policy).value() ||
      !digest_nonzero(r.source_digest) || !digest_nonzero(r.output_digest) ||
      (diagnosis && r.output_digest != r.source_digest) ||
      !canonical_edit_records(r.edits) || (diagnosis && !r.edits.empty()) ||
       !canonical_displacements(r.displacements) ||
       (r.displacement == normalization_displacement_claim::exact_zero
            ? !r.displacements.empty()
            : r.displacement != normalization_displacement_claim::records_present ||
                  (!seam_repair && !crack_repair) || r.displacements.empty()) ||
       !canonical_topology_records(r.topology_changes) ||
       (((!duplicate_repair && !seam_repair && !crack_repair) ||
         !r.prepared_operand_available) &&
         !r.topology_changes.empty()) ||
        ((duplicate_repair || seam_repair || crack_repair) &&
         r.prepared_operand_available &&
         r.topology_changes.size() != r.edits.size()) ||
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
      !canonical_mapping(r.vertices) || !canonical_mapping(r.edges) ||
      !canonical_mapping(r.facets) || !canonical_mapping(r.shells) ||
       (r.prepared_operand_available
             ? (duplicate_repair || seam_repair || crack_repair
                    ? r.shells.status != normalization_map_status::unavailable
                   : r.shells.status != normalization_map_status::total)
            : r.shells.status != normalization_map_status::unavailable) ||
      r.attributes.schema != normalization_report_schema ||
      !canonical_mapping(r.attributes.vertex_normals) ||
      !canonical_mapping(r.attributes.vertex_colours) ||
      !canonical_mapping(r.attributes.involved_faces) ||
      !canonical_mapping(r.attributes.metadata) ||
       (r.edits.empty()
            ? r.reversibility != normalization_reversibility::identity
            : r.reversibility !=
                  (orientation_repair
                       ? normalization_reversibility::fully_reversible
                       : normalization_reversibility::irreversible)) ||
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
       r.strict_certificate->input_digest != r.output_digest ||
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

template <class T>
status_or<bool> vertices_within_tolerance(const vec3<T> &, const vec3<T> &,
                                          double);

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
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t> edges;
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
        ++edges[{x, y}];
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
  std::vector<bool> boundary_vertices(m.vertices.size(), false);
  out.edges.reserve(edges.size());
  for (const auto &edge : edges) {
    out.edges.push_back({{edge.first.first, edge.first.second}});
    if (edge.second == 1) {
      boundary_vertices[static_cast<std::size_t>(edge.first.first)] = true;
      boundary_vertices[static_cast<std::size_t>(edge.first.second)] = true;
      auto a = add({normalization_defect_code::open_boundary_edge,
                    edge.first.first, edge.first.second, 1});
      if (!a.has_value()) return a.error();
    }
  }
  if (b.policy.model_tolerance > 0.0) {
    for (std::uint64_t first = 0; first != m.vertices.size(); ++first) {
      if (!boundary_vertices[static_cast<std::size_t>(first)]) continue;
      for (std::uint64_t second = first + 1; second != m.vertices.size();
           ++second) {
        if (!boundary_vertices[static_cast<std::size_t>(second)] ||
            edges.count({first, second}) != 0)
          continue;
        auto checked = b.checkpoint(12);
        if (!checked.has_value()) return checked.error();
        const auto &a = m.vertices[static_cast<std::size_t>(first)];
        const auto &c = m.vertices[static_cast<std::size_t>(second)];
        if (!std::isfinite(a.x) || !std::isfinite(a.y) ||
            !std::isfinite(a.z) || !std::isfinite(c.x) ||
            !std::isfinite(c.y) || !std::isfinite(c.z))
          continue;
        auto close = vertices_within_tolerance(
            a, c, b.policy.model_tolerance);
        if (!close.has_value()) return close.error();
        if (close.value()) {
          auto added = add({normalization_defect_code::small_gap_candidate,
                            first, second, 0});
          if (!added.has_value()) return added.error();
        }
      }
    }
  }
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
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t> unique_edges;
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
        ++unique_edges[{std::min(a, c), std::max(a, c)}];
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
  std::vector<unsigned char> boundary_vertices(m.vertices.size(), 0);
  result.edges.reserve(unique_edges.size());
  for (const auto &edge : unique_edges) {
    result.edges.push_back({{edge.first.first, edge.first.second}});
    if (edge.second == 1) {
      boundary_vertices[static_cast<std::size_t>(edge.first.first)] = 1;
      boundary_vertices[static_cast<std::size_t>(edge.first.second)] = 1;
      auto added = add({normalization_defect_code::open_boundary_edge,
                        edge.first.first, edge.first.second, 1});
      if (!added.has_value()) return added.error();
    }
  }
  if (budget.policy.model_tolerance > 0.0) {
    for (std::uint64_t first = 0; first != m.vertices.size(); ++first) {
      if (!boundary_vertices[static_cast<std::size_t>(first)]) continue;
      for (std::uint64_t second = first + 1; second != m.vertices.size();
           ++second) {
        if (!boundary_vertices[static_cast<std::size_t>(second)] ||
            unique_edges.count({first, second}) != 0)
          continue;
        auto work = budget.checkpoint(12);
        if (!work.has_value()) return work.error();
        const auto &a = m.vertices[static_cast<std::size_t>(first)];
        const auto &c = m.vertices[static_cast<std::size_t>(second)];
        if (!(std::isfinite(a.x) && std::isfinite(a.y) &&
              std::isfinite(a.z) && std::isfinite(c.x) &&
              std::isfinite(c.y) && std::isfinite(c.z)))
          continue;
        auto close = vertices_within_tolerance(
            a, c, budget.policy.model_tolerance);
        if (!close.has_value()) return close.error();
        if (close.value()) {
          auto added = add({normalization_defect_code::small_gap_candidate,
                            first, second, 0});
          if (!added.has_value()) return added.error();
        }
      }
    }
  }
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

template <class T, class I> struct structural_removal_result {
  fv_surface_mesh<T, I> mesh;
  normalization_mapping vertices;
  normalization_mapping edges;
  normalization_mapping facets;
  normalization_mapping vertex_normals;
  normalization_mapping vertex_colours;
  normalization_mapping involved_faces;
  normalization_mapping metadata;
  std::vector<normalization_edit> edits;
};

template <class T, class I>
digest removed_vertex_before_digest(const fv_surface_mesh<T, I> &source,
                                    std::uint64_t ordinal) {
  canonical_encoder e;
  e.u64(ordinal);
  const auto &v = source.vertices[static_cast<std::size_t>(ordinal)];
  e.floating(v.x);
  e.floating(v.y);
  e.floating(v.z);
  e.boolean(!source.vertex_normals.empty());
  if (!source.vertex_normals.empty()) {
    const auto &n = source.vertex_normals[static_cast<std::size_t>(ordinal)];
    e.floating(n.x);
    e.floating(n.y);
    e.floating(n.z);
  }
  e.boolean(!source.vertex_colours.empty());
  if (!source.vertex_colours.empty())
    e.u32(source.vertex_colours[static_cast<std::size_t>(ordinal)]);
  e.boolean(!source.involved_faces.empty());
  if (!source.involved_faces.empty()) {
    const auto &faces = source.involved_faces[static_cast<std::size_t>(ordinal)];
    e.u64(faces.size());
    for (I face : faces) e.u64(static_cast<std::uint64_t>(face));
  }
  return domain_digest(removal_before_tag, e.bytes());
}

digest removed_vertex_after_digest(std::uint64_t ordinal) {
  canonical_encoder e;
  e.u64(ordinal);
  e.u64(normalization_removed_ordinal);
  return domain_digest(removal_after_tag, e.bytes());
}

digest removal_edit_digest(const normalization_edit &edit) {
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(edit.operation));
  e.u64(edit.canonical_ordinal);
  e.byte(static_cast<std::uint8_t>(edit.entity));
  e.u64(edit.source_ordinal);
  e.u64(edit.prepared_ordinal);
  encode_digest(e, edit.before_evidence_digest);
  encode_digest(e, edit.after_evidence_digest);
  e.byte(static_cast<std::uint8_t>(edit.reversibility));
  return domain_digest(removal_edit_tag, e.bytes());
}

template <class T, class I, class Budget>
status_or<structural_removal_result<T, I>> remove_irrelevant_storage(
    const fv_surface_mesh<T, I> &source,
    const std::vector<std::array<std::uint64_t, 2>> &source_edges, Budget &b) {
  structural_removal_result<T, I> out;
  out.mesh.metadata = source.metadata;
  out.vertices.status = normalization_map_status::total;
  out.vertices.source_to_prepared.assign(source.vertices.size(),
                                          normalization_removed_ordinal);
  std::vector<bool> used(source.vertices.size(), false);
  for (const auto &face : source.faces)
    for (I index : face) used[static_cast<std::size_t>(index)] = true;

  const bool normals = !source.vertex_normals.empty();
  const bool colours = !source.vertex_colours.empty();
  const bool involved = !source.involved_faces.empty();
  for (std::uint64_t i = 0; i != source.vertices.size(); ++i) {
    auto work = b.checkpoint(1);
    if (!work.has_value()) return work.error();
    if (used[static_cast<std::size_t>(i)]) {
      const auto mapped = static_cast<std::uint64_t>(out.mesh.vertices.size());
      out.vertices.source_to_prepared[static_cast<std::size_t>(i)] = mapped;
      out.mesh.vertices.push_back(source.vertices[static_cast<std::size_t>(i)]);
      if (normals)
        out.mesh.vertex_normals.push_back(
            source.vertex_normals[static_cast<std::size_t>(i)]);
      if (colours)
        out.mesh.vertex_colours.push_back(
            source.vertex_colours[static_cast<std::size_t>(i)]);
      if (involved)
        out.mesh.involved_faces.push_back(
            source.involved_faces[static_cast<std::size_t>(i)]);
    } else {
      auto record = b.record();
      if (!record.has_value()) return record.error();
      normalization_edit edit;
      edit.operation = normalization_operation::irrelevant_storage_removal;
      edit.canonical_ordinal = out.edits.size();
      edit.entity = normalization_entity_kind::vertex;
      edit.source_ordinal = i;
      edit.prepared_ordinal = normalization_removed_ordinal;
      edit.before_evidence_digest = removed_vertex_before_digest(source, i);
      edit.after_evidence_digest = removed_vertex_after_digest(i);
      edit.reversibility = normalization_reversibility::irreversible;
      edit.evidence_digest = removal_edit_digest(edit);
      out.edits.push_back(std::move(edit));
    }
  }

  out.mesh.faces = source.faces;
  for (auto &face : out.mesh.faces)
    for (auto &index : face) {
      const auto mapped = out.vertices.source_to_prepared[
          static_cast<std::size_t>(index)];
      if (mapped == normalization_removed_ordinal ||
          mapped > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return normalization_error("normalization_structural_index");
      index = static_cast<I>(mapped);
    }

  out.facets = identity(source.faces.size());
  out.metadata = attribute_identity(!source.metadata.empty(),
                                    source.metadata.size());
  out.vertex_normals = normals ? out.vertices
                               : normalization_mapping{
                                     normalization_map_status::absent, {}};
  out.vertex_colours = colours ? out.vertices
                               : normalization_mapping{
                                     normalization_map_status::absent, {}};
  out.involved_faces = involved ? out.vertices
                                : normalization_mapping{
                                      normalization_map_status::absent, {}};

  std::map<std::array<std::uint64_t, 2>, std::uint64_t> output_edges;
  for (const auto &face : out.mesh.faces)
    for (std::size_t j = 0; j != face.size(); ++j) {
      auto a = static_cast<std::uint64_t>(face[j]);
      auto c = static_cast<std::uint64_t>(face[(j + 1) % face.size()]);
      if (c < a) std::swap(a, c);
      output_edges.emplace(std::array<std::uint64_t, 2>{{a, c}}, 0);
    }
  std::uint64_t edge_ordinal = 0;
  for (auto &entry : output_edges) entry.second = edge_ordinal++;
  out.edges.status = normalization_map_status::total;
  out.edges.source_to_prepared.reserve(source_edges.size());
  for (const auto &edge : source_edges) {
    auto a = out.vertices.source_to_prepared[static_cast<std::size_t>(edge[0])];
    auto c = out.vertices.source_to_prepared[static_cast<std::size_t>(edge[1])];
    if (c < a) std::swap(a, c);
    const auto found = output_edges.find({{a, c}});
    if (found == output_edges.end())
      return normalization_error("normalization_structural_edge");
    out.edges.source_to_prepared.push_back(found->second);
  }
  return out;
}

template <class T, class I> struct exact_duplicate_result {
  fv_surface_mesh<T, I> mesh;
  normalization_mapping vertices;
  normalization_mapping edges;
  normalization_mapping facets;
  normalization_mapping vertex_normals;
  normalization_mapping vertex_colours;
  normalization_mapping involved_faces;
  normalization_mapping metadata;
  std::vector<normalization_edit> edits;
  std::vector<normalization_topology_change> topology_changes;
};

template <class T, class I>
void encode_duplicate_vertex(canonical_encoder &e,
                             const fv_surface_mesh<T, I> &mesh,
                             std::uint64_t ordinal) {
  const auto position = static_cast<std::size_t>(ordinal);
  const auto &vertex = mesh.vertices[position];
  e.floating(vertex.x);
  e.floating(vertex.y);
  e.floating(vertex.z);
  e.boolean(!mesh.vertex_normals.empty());
  if (!mesh.vertex_normals.empty()) {
    const auto &normal = mesh.vertex_normals[position];
    e.floating(normal.x);
    e.floating(normal.y);
    e.floating(normal.z);
  }
  e.boolean(!mesh.vertex_colours.empty());
  if (!mesh.vertex_colours.empty()) e.u32(mesh.vertex_colours[position]);
  e.boolean(!mesh.involved_faces.empty());
  if (!mesh.involved_faces.empty()) {
    e.u64(mesh.involved_faces[position].size());
    for (I facet : mesh.involved_faces[position])
      e.u64(static_cast<std::uint64_t>(facet));
  }
}

template <class T, class I>
digest duplicate_before_digest(const fv_surface_mesh<T, I> &source,
                               normalization_entity_kind entity,
                               std::uint64_t source_ordinal,
                               std::uint64_t retained_source_ordinal) {
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(entity));
  e.u64(source_ordinal);
  e.u64(retained_source_ordinal);
  if (entity == normalization_entity_kind::vertex) {
    encode_duplicate_vertex(e, source, source_ordinal);
    encode_duplicate_vertex(e, source, retained_source_ordinal);
  } else {
    for (std::uint64_t ordinal : {source_ordinal, retained_source_ordinal}) {
      const auto &face = source.faces[static_cast<std::size_t>(ordinal)];
      e.u64(face.size());
      for (I index : face) e.u64(static_cast<std::uint64_t>(index));
    }
  }
  return domain_digest(duplicate_before_tag, e.bytes());
}

template <class T, class I>
digest duplicate_after_digest(const fv_surface_mesh<T, I> &output,
                              normalization_entity_kind entity,
                              std::uint64_t source_ordinal,
                              std::uint64_t prepared_ordinal) {
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(entity));
  e.u64(source_ordinal);
  e.u64(prepared_ordinal);
  if (entity == normalization_entity_kind::vertex) {
    encode_duplicate_vertex(e, output, prepared_ordinal);
  } else {
    const auto &face = output.faces[static_cast<std::size_t>(prepared_ordinal)];
    e.u64(face.size());
    for (I index : face) e.u64(static_cast<std::uint64_t>(index));
  }
  return domain_digest(duplicate_after_tag, e.bytes());
}

digest duplicate_edit_digest(const normalization_edit &edit) {
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(edit.operation));
  e.u64(edit.canonical_ordinal);
  e.byte(static_cast<std::uint8_t>(edit.entity));
  e.u64(edit.source_ordinal);
  e.u64(edit.prepared_ordinal);
  encode_digest(e, edit.before_evidence_digest);
  encode_digest(e, edit.after_evidence_digest);
  e.byte(static_cast<std::uint8_t>(edit.reversibility));
  return domain_digest(duplicate_edit_tag, e.bytes());
}

digest duplicate_topology_digest(const normalization_topology_change &change) {
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(change.operation));
  e.u64(change.source_ordinal);
  e.byte(static_cast<std::uint8_t>(change.entity));
  e.u64(change.prepared_ordinal);
  e.byte(static_cast<std::uint8_t>(change.justification));
  e.u32(change.justification_subcode);
  encode_digest(e, change.before_evidence_digest);
  encode_digest(e, change.after_evidence_digest);
  e.byte(static_cast<std::uint8_t>(change.reversibility));
  return domain_digest(duplicate_topology_tag, e.bytes());
}

template <class T, class I, class Budget>
status_or<exact_duplicate_result<T, I>> consolidate_exact_duplicates(
    const fv_surface_mesh<T, I> &source,
    const std::vector<std::array<std::uint64_t, 2>> &source_edges, Budget &b) {
  exact_duplicate_result<T, I> out;
  const bool normals = !source.vertex_normals.empty();
  const bool colours = !source.vertex_colours.empty();
  const bool involved = !source.involved_faces.empty();
  if ((normals && source.vertex_normals.size() != source.vertices.size()) ||
      (colours && source.vertex_colours.size() != source.vertices.size()) ||
      (involved && source.involved_faces.size() != source.vertices.size()))
    return normalization_error("normalization_exact_duplicate_attributes");

  out.mesh.metadata = source.metadata;
  out.vertices.status = normalization_map_status::total;
  out.vertices.source_to_prepared.reserve(source.vertices.size());
  std::map<coordinate_key<T>, std::uint64_t> representatives;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> duplicate_vertices;
  for (std::uint64_t ordinal = 0; ordinal != source.vertices.size(); ++ordinal) {
    auto work = b.checkpoint(3);
    if (!work.has_value()) return work.error();
    const auto &vertex = source.vertices[static_cast<std::size_t>(ordinal)];
    if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
        !std::isfinite(vertex.z))
      return normalization_error("normalization_exact_duplicate_nonfinite");
    const auto inserted = representatives.emplace(
        bits_key(vertex.x, vertex.y, vertex.z), ordinal);
    if (inserted.second) {
      const auto prepared = static_cast<std::uint64_t>(out.mesh.vertices.size());
      out.vertices.source_to_prepared.push_back(prepared);
      out.mesh.vertices.push_back(vertex);
      if (normals)
        out.mesh.vertex_normals.push_back(
            source.vertex_normals[static_cast<std::size_t>(ordinal)]);
      if (colours)
        out.mesh.vertex_colours.push_back(
            source.vertex_colours[static_cast<std::size_t>(ordinal)]);
    } else {
      const auto retained = inserted.first->second;
      if ((normals && source.vertex_normals[static_cast<std::size_t>(ordinal)] !=
                          source.vertex_normals[static_cast<std::size_t>(retained)]) ||
          (colours && source.vertex_colours[static_cast<std::size_t>(ordinal)] !=
                          source.vertex_colours[static_cast<std::size_t>(retained)]))
        return normalization_error(
            "normalization_exact_duplicate_attribute_conflict");
      out.vertices.source_to_prepared.push_back(
          out.vertices.source_to_prepared[static_cast<std::size_t>(retained)]);
      duplicate_vertices.emplace_back(ordinal, retained);
    }
  }

  out.facets.status = normalization_map_status::total;
  out.facets.source_to_prepared.reserve(source.faces.size());
  std::map<std::vector<std::uint64_t>, std::pair<std::uint64_t, std::uint64_t>>
      retained_facets;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> duplicate_facets;
  for (std::uint64_t ordinal = 0; ordinal != source.faces.size(); ++ordinal) {
    const auto &source_face = source.faces[static_cast<std::size_t>(ordinal)];
    auto required = facet_work(source_face.size(), ordinal);
    if (!required.has_value()) return required.error();
    auto work = b.checkpoint(required.value());
    if (!work.has_value()) return work.error();
    std::vector<I> rewritten;
    rewritten.reserve(source_face.size());
    for (I source_index : source_face) {
      const auto index = static_cast<std::uint64_t>(source_index);
      if (index >= out.vertices.source_to_prepared.size())
        return normalization_error("normalization_exact_duplicate_index");
      const auto prepared = out.vertices.source_to_prepared[
          static_cast<std::size_t>(index)];
      if (prepared > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return normalization_error("normalization_exact_duplicate_index");
      rewritten.push_back(static_cast<I>(prepared));
    }
    if (rewritten.size() < 3 ||
        std::adjacent_find(rewritten.begin(), rewritten.end()) !=
            rewritten.end() ||
        (rewritten.size() > 1 && rewritten.front() == rewritten.back()))
      return normalization_error("normalization_exact_duplicate_collapsed_face");
    const auto key = canonical_facet_key(rewritten);
    const auto found = retained_facets.find(key);
    if (found == retained_facets.end()) {
      const auto prepared = static_cast<std::uint64_t>(out.mesh.faces.size());
      retained_facets.emplace(key, std::make_pair(ordinal, prepared));
      out.facets.source_to_prepared.push_back(prepared);
      out.mesh.faces.push_back(std::move(rewritten));
    } else {
      out.facets.source_to_prepared.push_back(found->second.second);
      duplicate_facets.emplace_back(ordinal, found->second.first);
    }
  }

  if (involved) {
    out.mesh.involved_faces.resize(out.mesh.vertices.size());
    for (std::uint64_t facet = 0; facet != out.mesh.faces.size(); ++facet) {
      if (facet > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return normalization_error("normalization_exact_duplicate_face_index");
      for (I vertex : out.mesh.faces[static_cast<std::size_t>(facet)])
        out.mesh.involved_faces[static_cast<std::size_t>(vertex)].push_back(
            static_cast<I>(facet));
    }
  }

  out.vertex_normals = normals
                           ? out.vertices
                           : normalization_mapping{
                                 normalization_map_status::absent, {}};
  out.vertex_colours = colours
                           ? out.vertices
                           : normalization_mapping{
                                 normalization_map_status::absent, {}};
  out.involved_faces = involved
                           ? out.vertices
                           : normalization_mapping{
                                 normalization_map_status::absent, {}};
  out.metadata = attribute_identity(!source.metadata.empty(),
                                    source.metadata.size());

  std::map<std::array<std::uint64_t, 2>, std::uint64_t> output_edges;
  for (const auto &face : out.mesh.faces)
    for (std::size_t offset = 0; offset != face.size(); ++offset) {
      auto a = static_cast<std::uint64_t>(face[offset]);
      auto c = static_cast<std::uint64_t>(face[(offset + 1) % face.size()]);
      if (c < a) std::swap(a, c);
      output_edges.emplace(std::array<std::uint64_t, 2>{{a, c}}, 0);
    }
  std::uint64_t edge_ordinal = 0;
  for (auto &edge : output_edges) edge.second = edge_ordinal++;
  out.edges.status = normalization_map_status::total;
  out.edges.source_to_prepared.reserve(source_edges.size());
  for (const auto &source_edge : source_edges) {
    auto a = out.vertices.source_to_prepared[
        static_cast<std::size_t>(source_edge[0])];
    auto c = out.vertices.source_to_prepared[
        static_cast<std::size_t>(source_edge[1])];
    if (a == c)
      return normalization_error("normalization_exact_duplicate_collapsed_edge");
    if (c < a) std::swap(a, c);
    const auto found = output_edges.find({{a, c}});
    if (found == output_edges.end())
      return normalization_error("normalization_exact_duplicate_edge");
    out.edges.source_to_prepared.push_back(found->second);
  }

  const auto add_records = [&](normalization_entity_kind entity,
                               std::uint64_t source_ordinal,
                               std::uint64_t retained_source_ordinal,
                               std::uint64_t prepared_ordinal,
                               std::uint32_t subcode) -> status_or<bool> {
    auto edit_record = b.record();
    if (!edit_record.has_value()) return edit_record.error();
    auto topology_record = b.record();
    if (!topology_record.has_value()) return topology_record.error();
    normalization_edit edit;
    edit.operation = normalization_operation::exact_duplicate_consolidation;
    edit.canonical_ordinal = out.edits.size();
    edit.entity = entity;
    edit.source_ordinal = source_ordinal;
    edit.prepared_ordinal = prepared_ordinal;
    edit.before_evidence_digest = duplicate_before_digest(
        source, entity, source_ordinal, retained_source_ordinal);
    edit.after_evidence_digest = duplicate_after_digest(
        out.mesh, entity, source_ordinal, prepared_ordinal);
    edit.reversibility = normalization_reversibility::irreversible;
    edit.evidence_digest = duplicate_edit_digest(edit);
    out.edits.push_back(edit);

    normalization_topology_change change;
    change.operation = normalization_operation::exact_duplicate_consolidation;
    change.source_ordinal = source_ordinal;
    change.entity = entity;
    change.prepared_ordinal = prepared_ordinal;
    change.justification =
        normalization_topology_justification::caller_authorized_repair;
    change.justification_subcode = subcode;
    change.before_evidence_digest = edit.before_evidence_digest;
    change.after_evidence_digest = edit.after_evidence_digest;
    change.reversibility = normalization_reversibility::irreversible;
    change.evidence_digest = duplicate_topology_digest(change);
    out.topology_changes.push_back(std::move(change));
    return true;
  };
  for (const auto &duplicate : duplicate_vertices) {
    auto added = add_records(
        normalization_entity_kind::vertex, duplicate.first, duplicate.second,
        out.vertices.source_to_prepared[static_cast<std::size_t>(duplicate.first)],
        1);
    if (!added.has_value()) return added.error();
  }
  for (const auto &duplicate : duplicate_facets) {
    auto added = add_records(
        normalization_entity_kind::facet, duplicate.first, duplicate.second,
        out.facets.source_to_prepared[static_cast<std::size_t>(duplicate.first)],
        2);
    if (!added.has_value()) return added.error();
  }
  return out;
}

template <class T>
status_or<bool> vertices_within_tolerance(const vec3<T> &a, const vec3<T> &b,
                                          double tolerance) {
  auto ax = decode_coordinate(a.x), ay = decode_coordinate(a.y),
       az = decode_coordinate(a.z);
  auto bx = decode_coordinate(b.x), by = decode_coordinate(b.y),
       bz = decode_coordinate(b.z);
  auto limit = decode_coordinate(tolerance);
  if (!ax.has_value() || !ay.has_value() || !az.has_value() ||
      !bx.has_value() || !by.has_value() || !bz.has_value() ||
      !limit.has_value())
    return normalization_error("normalization_seam_coordinate");
  const auto dx = ax.value().value - bx.value().value;
  const auto dy = ay.value().value - by.value().value;
  const auto dz = az.value().value - bz.value().value;
  const auto squared = dx * dx + dy * dy + dz * dz;
  const auto threshold = limit.value().value * limit.value().value;
  return squared.compare(threshold) <= 0;
}

normalization_rational displacement_bound(double tolerance) {
  int exponent = 0;
  std::frexp(tolerance, &exponent);
  const int squared_exponent = exponent * 2;
  if (squared_exponent > 0)
    return {std::int64_t(1) << squared_exponent, 1};
  if (squared_exponent >= -63)
    return {1, std::uint64_t(1) << -squared_exponent};
  return {1, std::numeric_limits<std::uint64_t>::max()};
}

template <class T, class I>
digest seam_displacement_digest(const fv_surface_mesh<T, I> &source,
                                const fv_surface_mesh<T, I> &output,
                                const normalization_displacement_record &record,
                                double tolerance,
                                normalization_operation operation) {
  canonical_encoder encoder;
  encoder.u64(record.source_vertex);
  encoder.u64(record.prepared_vertex);
  encoder.floating(tolerance);
  encode_duplicate_vertex(encoder, source, record.source_vertex);
  encode_duplicate_vertex(encoder, output, record.prepared_vertex);
  encoder.byte(static_cast<std::uint8_t>(record.kind));
  for (const auto &component : record.exact_components)
    encode_rational(encoder, component);
  encode_rational(encoder, record.squared_distance_bound);
  encoder.byte(static_cast<std::uint8_t>(record.unit));
  return domain_digest(operation == normalization_operation::crack_closure
                           ? crack_displacement_tag
                           : seam_displacement_tag,
                       encoder.bytes());
}

digest seam_edit_digest(const normalization_edit &edit) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(edit.operation));
  encoder.u64(edit.canonical_ordinal);
  encoder.byte(static_cast<std::uint8_t>(edit.entity));
  encoder.u64(edit.source_ordinal);
  encoder.u64(edit.prepared_ordinal);
  encode_digest(encoder, edit.before_evidence_digest);
  encode_digest(encoder, edit.after_evidence_digest);
  encoder.byte(static_cast<std::uint8_t>(edit.reversibility));
  return domain_digest(edit.operation == normalization_operation::crack_closure
                           ? crack_edit_tag
                           : seam_edit_tag,
                       encoder.bytes());
}

digest seam_topology_digest(const normalization_topology_change &change) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(change.operation));
  encoder.u64(change.source_ordinal);
  encoder.byte(static_cast<std::uint8_t>(change.entity));
  encoder.u64(change.prepared_ordinal);
  encoder.byte(static_cast<std::uint8_t>(change.justification));
  encoder.u32(change.justification_subcode);
  encode_digest(encoder, change.before_evidence_digest);
  encode_digest(encoder, change.after_evidence_digest);
  encoder.byte(static_cast<std::uint8_t>(change.reversibility));
  return domain_digest(
      change.operation == normalization_operation::crack_closure
          ? crack_topology_tag
          : seam_topology_tag,
      encoder.bytes());
}

template <class T, class I> struct seam_consolidation_result {
  fv_surface_mesh<T, I> mesh;
  normalization_mapping vertices;
  normalization_mapping edges;
  normalization_mapping facets;
  normalization_mapping vertex_normals;
  normalization_mapping vertex_colours;
  normalization_mapping involved_faces;
  normalization_mapping metadata;
  std::vector<normalization_edit> edits;
  std::vector<normalization_topology_change> topology_changes;
  std::vector<normalization_displacement_record> displacements;
};

template <class T, class I, class Budget>
status_or<seam_consolidation_result<T, I>> consolidate_seam_aware_vertices(
    const fv_surface_mesh<T, I> &source,
    const std::vector<std::array<std::uint64_t, 2>> &source_edges,
    const normalization_policy &policy, normalization_operation operation,
    Budget &resources) {
  seam_consolidation_result<T, I> result;
  const bool crack = operation == normalization_operation::crack_closure;
  const bool normals = !source.vertex_normals.empty();
  const bool colours = !source.vertex_colours.empty();
  const bool involved = !source.involved_faces.empty();
  if ((normals && source.vertex_normals.size() != source.vertices.size()) ||
      (colours && source.vertex_colours.size() != source.vertices.size()) ||
      (involved && source.involved_faces.size() != source.vertices.size()))
    return normalization_error("normalization_seam_attributes");

  result.mesh.metadata = source.metadata;
  result.vertices.status = normalization_map_status::total;
  result.vertices.source_to_prepared.reserve(source.vertices.size());
  std::vector<std::uint64_t> representatives;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> merged;
  std::vector<std::uint64_t> crack_partner(
      source.vertices.size(), normalization_removed_ordinal);
  if (crack) {
    std::map<std::array<std::uint64_t, 2>, std::uint64_t> edge_uses;
    for (const auto &face : source.faces)
      for (std::size_t offset = 0; offset != face.size(); ++offset) {
        auto a = static_cast<std::uint64_t>(face[offset]);
        auto b = static_cast<std::uint64_t>(face[(offset + 1) % face.size()]);
        if (a >= source.vertices.size() || b >= source.vertices.size())
          return normalization_error("normalization_crack_index");
        if (b < a) std::swap(a, b);
        ++edge_uses[{{a, b}}];
      }
    std::vector<bool> boundary(source.vertices.size(), false);
    for (const auto &edge : edge_uses)
      if (edge.second == 1) {
        boundary[static_cast<std::size_t>(edge.first[0])] = true;
        boundary[static_cast<std::size_t>(edge.first[1])] = true;
      }
    for (std::uint64_t first = 0; first != source.vertices.size(); ++first) {
      if (!boundary[static_cast<std::size_t>(first)]) continue;
      for (std::uint64_t second = first + 1; second != source.vertices.size();
           ++second) {
        if (!boundary[static_cast<std::size_t>(second)] ||
            edge_uses.count({{first, second}}) != 0)
          continue;
        auto work = resources.checkpoint(12);
        if (!work.has_value()) return work.error();
        if ((normals && source.vertex_normals[static_cast<std::size_t>(first)] !=
                            source.vertex_normals[static_cast<std::size_t>(second)]) ||
            (colours && source.vertex_colours[static_cast<std::size_t>(first)] !=
                            source.vertex_colours[static_cast<std::size_t>(second)]))
          continue;
        auto close = vertices_within_tolerance(
            source.vertices[static_cast<std::size_t>(first)],
            source.vertices[static_cast<std::size_t>(second)],
            policy.model_tolerance);
        if (!close.has_value()) return close.error();
        if (!close.value()) continue;
        if (crack_partner[static_cast<std::size_t>(first)] !=
                normalization_removed_ordinal ||
            crack_partner[static_cast<std::size_t>(second)] !=
                normalization_removed_ordinal)
          return normalization_error("normalization_crack_ambiguous");
        crack_partner[static_cast<std::size_t>(first)] = second;
        crack_partner[static_cast<std::size_t>(second)] = first;
      }
    }
  }
  for (std::uint64_t ordinal = 0; ordinal != source.vertices.size(); ++ordinal) {
    const auto &vertex = source.vertices[static_cast<std::size_t>(ordinal)];
    if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
        !std::isfinite(vertex.z))
      return normalization_error("normalization_seam_nonfinite");
    std::optional<std::uint64_t> retained;
    if (crack && crack_partner[static_cast<std::size_t>(ordinal)] < ordinal) {
      retained = crack_partner[static_cast<std::size_t>(ordinal)];
    }
    for (std::uint64_t candidate : representatives) {
      if (crack) break;
      auto work = resources.checkpoint(12);
      if (!work.has_value()) return work.error();
      const bool compatible =
          (!normals || source.vertex_normals[static_cast<std::size_t>(ordinal)] ==
                           source.vertex_normals[static_cast<std::size_t>(candidate)]) &&
          (!colours || source.vertex_colours[static_cast<std::size_t>(ordinal)] ==
                           source.vertex_colours[static_cast<std::size_t>(candidate)]);
      if (!compatible) continue;
      auto close = vertices_within_tolerance(vertex,
          source.vertices[static_cast<std::size_t>(candidate)],
          policy.model_tolerance);
      if (!close.has_value()) return close.error();
      if (close.value()) {
        retained = candidate;
        break;
      }
    }
    if (!retained) {
      representatives.push_back(ordinal);
      result.vertices.source_to_prepared.push_back(result.mesh.vertices.size());
      result.mesh.vertices.push_back(vertex);
      if (normals)
        result.mesh.vertex_normals.push_back(
            source.vertex_normals[static_cast<std::size_t>(ordinal)]);
      if (colours)
        result.mesh.vertex_colours.push_back(
            source.vertex_colours[static_cast<std::size_t>(ordinal)]);
    } else {
      result.vertices.source_to_prepared.push_back(
          result.vertices.source_to_prepared[static_cast<std::size_t>(*retained)]);
      merged.emplace_back(ordinal, *retained);
    }
  }

  result.mesh.faces = source.faces;
  for (auto &face : result.mesh.faces) {
    auto work = resources.checkpoint(std::max<std::uint64_t>(1, face.size()));
    if (!work.has_value()) return work.error();
    for (auto &index : face) {
      const auto source_index = static_cast<std::uint64_t>(index);
      if (source_index >= result.vertices.source_to_prepared.size())
        return normalization_error("normalization_seam_index");
      const auto prepared = result.vertices.source_to_prepared[
          static_cast<std::size_t>(source_index)];
      if (prepared > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return normalization_error("normalization_seam_index");
      index = static_cast<I>(prepared);
    }
    if (std::adjacent_find(face.begin(), face.end()) != face.end() ||
        (face.size() > 1 && face.front() == face.back()))
      return normalization_error("normalization_seam_collapsed_face");
  }
  if (involved) {
    result.mesh.involved_faces.resize(result.mesh.vertices.size());
    for (std::uint64_t facet = 0; facet != result.mesh.faces.size(); ++facet) {
      if (facet > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return normalization_error("normalization_seam_face_index");
      for (I vertex : result.mesh.faces[static_cast<std::size_t>(facet)])
        result.mesh.involved_faces[static_cast<std::size_t>(vertex)].push_back(
            static_cast<I>(facet));
    }
  }
  result.facets = identity(source.faces.size());
  result.vertex_normals = normals ? result.vertices
      : normalization_mapping{normalization_map_status::absent, {}};
  result.vertex_colours = colours ? result.vertices
      : normalization_mapping{normalization_map_status::absent, {}};
  result.involved_faces = involved ? result.vertices
      : normalization_mapping{normalization_map_status::absent, {}};
  result.metadata = attribute_identity(!source.metadata.empty(),
                                       source.metadata.size());

  std::map<std::array<std::uint64_t, 2>, std::uint64_t> output_edges;
  for (const auto &face : result.mesh.faces)
    for (std::size_t offset = 0; offset != face.size(); ++offset) {
      auto a = static_cast<std::uint64_t>(face[offset]);
      auto b = static_cast<std::uint64_t>(face[(offset + 1) % face.size()]);
      if (a == b) return normalization_error("normalization_seam_collapsed_edge");
      if (b < a) std::swap(a, b);
      output_edges.emplace(std::array<std::uint64_t, 2>{{a, b}}, 0);
    }
  std::uint64_t edge_ordinal = 0;
  for (auto &entry : output_edges) entry.second = edge_ordinal++;
  result.edges.status = normalization_map_status::total;
  for (const auto &edge : source_edges) {
    auto a = result.vertices.source_to_prepared[static_cast<std::size_t>(edge[0])];
    auto b = result.vertices.source_to_prepared[static_cast<std::size_t>(edge[1])];
    if (a == b) return normalization_error("normalization_seam_collapsed_edge");
    if (b < a) std::swap(a, b);
    const auto found = output_edges.find({{a, b}});
    if (found == output_edges.end())
      return normalization_error("normalization_seam_edge");
    result.edges.source_to_prepared.push_back(found->second);
  }

  for (const auto &entry : merged) {
    auto edit_record = resources.record();
    if (!edit_record.has_value()) return edit_record.error();
    auto topology_record = resources.record();
    if (!topology_record.has_value()) return topology_record.error();
    normalization_edit edit;
    edit.operation = operation;
    edit.canonical_ordinal = result.edits.size();
    edit.entity = normalization_entity_kind::vertex;
    edit.source_ordinal = entry.first;
    edit.prepared_ordinal = result.vertices.source_to_prepared[
        static_cast<std::size_t>(entry.first)];
    edit.before_evidence_digest = duplicate_before_digest(
        source, normalization_entity_kind::vertex, entry.first, entry.second);
    edit.after_evidence_digest = duplicate_after_digest(
        result.mesh, normalization_entity_kind::vertex, entry.first,
        edit.prepared_ordinal);
    edit.reversibility = normalization_reversibility::irreversible;
    edit.evidence_digest = seam_edit_digest(edit);
    result.edits.push_back(edit);

    normalization_topology_change change;
    change.operation = operation;
    change.source_ordinal = entry.first;
    change.entity = normalization_entity_kind::vertex;
    change.prepared_ordinal = edit.prepared_ordinal;
    change.justification =
        normalization_topology_justification::caller_authorized_repair;
    change.justification_subcode = crack ? 2 : 1;
    change.before_evidence_digest = edit.before_evidence_digest;
    change.after_evidence_digest = edit.after_evidence_digest;
    change.reversibility = normalization_reversibility::irreversible;
    change.evidence_digest = seam_topology_digest(change);
    result.topology_changes.push_back(std::move(change));

    const auto &before = source.vertices[static_cast<std::size_t>(entry.first)];
    const auto &after = source.vertices[static_cast<std::size_t>(entry.second)];
    if (bits_key(before.x, before.y, before.z) !=
        bits_key(after.x, after.y, after.z)) {
      auto displacement_record = resources.record();
      if (!displacement_record.has_value()) return displacement_record.error();
      normalization_displacement_record displacement;
      displacement.source_vertex = entry.first;
      displacement.prepared_vertex = edit.prepared_ordinal;
      displacement.kind = normalization_displacement_kind::bounded;
      displacement.squared_distance_bound =
          displacement_bound(policy.model_tolerance);
      displacement.unit = policy.unit;
      displacement.evidence_digest = seam_displacement_digest(
          source, result.mesh, displacement, policy.model_tolerance, operation);
      result.displacements.push_back(std::move(displacement));
    }
  }
  return result;
}

template <class T, class I> struct orientation_repair_result {
  fv_surface_mesh<T, I> mesh;
  std::uint64_t shell_count = 0;
  std::vector<normalization_edit> edits;
};

template <class I>
void encode_oriented_ring(canonical_encoder &encoder,
                          const std::vector<I> &ring) {
  encoder.u64(ring.size());
  for (I index : ring) encoder.u64(static_cast<std::uint64_t>(index));
}

template <class T, class I>
digest orientation_facet_digest(const std::array<char, 8> &tag,
                                const fv_surface_mesh<T, I> &mesh,
                                std::uint64_t ordinal) {
  canonical_encoder encoder;
  encoder.u64(ordinal);
  encode_oriented_ring(encoder, mesh.faces[static_cast<std::size_t>(ordinal)]);
  return domain_digest(tag, encoder.bytes());
}

digest orientation_edit_digest(const normalization_edit &edit) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(edit.operation));
  encoder.u64(edit.canonical_ordinal);
  encoder.byte(static_cast<std::uint8_t>(edit.entity));
  encoder.u64(edit.source_ordinal);
  encoder.u64(edit.prepared_ordinal);
  encode_digest(encoder, edit.before_evidence_digest);
  encode_digest(encoder, edit.after_evidence_digest);
  encoder.byte(static_cast<std::uint8_t>(edit.reversibility));
  return domain_digest(orientation_edit_tag, encoder.bytes());
}

template <class T, class I, class Budget>
status_or<orientation_repair_result<T, I>> repair_orientation(
    const fv_surface_mesh<T, I> &source, Budget &resources,
    cancellation_source *cancel) {
  auto planned = plan_operand_orientation(
      source, cancel, [&](std::uint64_t work) { return resources.checkpoint(work); });
  if (!planned.has_value()) return planned.error();
  if (planned.value().reverse_facets.size() != source.faces.size())
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::input_validation,
                      "normalization_orientation_plan_shape");
  orientation_repair_result<T, I> result;
  result.mesh = source;
  result.shell_count = planned.value().shell_count;
  for (std::uint64_t facet = 0; facet != source.faces.size(); ++facet) {
    auto work = resources.checkpoint(1);
    if (!work.has_value()) return work.error();
    if (!planned.value().reverse_facets[static_cast<std::size_t>(facet)])
      continue;
    auto record = resources.record();
    if (!record.has_value()) return record.error();
    std::reverse(result.mesh.faces[static_cast<std::size_t>(facet)].begin(),
                 result.mesh.faces[static_cast<std::size_t>(facet)].end());
    normalization_edit edit;
    edit.operation = normalization_operation::orientation_repair;
    edit.canonical_ordinal = result.edits.size();
    edit.entity = normalization_entity_kind::facet;
    edit.source_ordinal = facet;
    edit.prepared_ordinal = facet;
    edit.before_evidence_digest = orientation_facet_digest(
        orientation_before_tag, source, facet);
    edit.after_evidence_digest = orientation_facet_digest(
        orientation_after_tag, result.mesh, facet);
    edit.reversibility = normalization_reversibility::fully_reversible;
    edit.evidence_digest = orientation_edit_digest(edit);
    result.edits.push_back(std::move(edit));
  }
  return result;
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
       {std::make_pair<std::uint64_t, std::uint64_t>(report.edits.size(), 144),
         {report.displacements.size(), 112},
         {report.topology_changes.size(), 144},
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

bool same_mapping(const normalization_mapping &a,
                  const normalization_mapping &b) {
  return a.status == b.status &&
         a.source_to_prepared == b.source_to_prepared;
}

template <class T, class I>
digest verifier_removed_vertex_before_digest(
    const fv_surface_mesh<T, I> &source, std::uint64_t ordinal) {
  canonical_encoder encoded;
  encoded.u64(ordinal);
  const auto position = static_cast<std::size_t>(ordinal);
  const std::array<T, 3> point{{source.vertices[position].x,
                                source.vertices[position].y,
                                source.vertices[position].z}};
  for (T component : point) encoded.floating(component);
  const bool normals_present = !source.vertex_normals.empty();
  encoded.boolean(normals_present);
  if (normals_present) {
    const std::array<T, 3> normal{{source.vertex_normals[position].x,
                                   source.vertex_normals[position].y,
                                   source.vertex_normals[position].z}};
    for (T component : normal) encoded.floating(component);
  }
  const bool colours_present = !source.vertex_colours.empty();
  encoded.boolean(colours_present);
  if (colours_present) encoded.u32(source.vertex_colours[position]);
  const bool incidence_present = !source.involved_faces.empty();
  encoded.boolean(incidence_present);
  if (incidence_present) {
    encoded.u64(source.involved_faces[position].size());
    for (I facet : source.involved_faces[position])
      encoded.u64(static_cast<std::uint64_t>(facet));
  }
  return domain_digest(removal_before_tag, encoded.bytes());
}

digest verifier_removed_vertex_after_digest(std::uint64_t ordinal) {
  canonical_encoder encoded;
  for (std::uint64_t value : {ordinal, normalization_removed_ordinal})
    encoded.u64(value);
  return domain_digest(removal_after_tag, encoded.bytes());
}

digest verifier_removal_edit_digest(const normalization_edit &edit) {
  canonical_encoder encoded;
  encoded.byte(static_cast<std::uint8_t>(edit.operation));
  encoded.u64(edit.canonical_ordinal);
  encoded.byte(static_cast<std::uint8_t>(edit.entity));
  encoded.u64(edit.source_ordinal);
  encoded.u64(edit.prepared_ordinal);
  encoded.raw(edit.before_evidence_digest.bytes.data(),
              edit.before_evidence_digest.bytes.size());
  encoded.raw(edit.after_evidence_digest.bytes.data(),
              edit.after_evidence_digest.bytes.size());
  encoded.byte(static_cast<std::uint8_t>(edit.reversibility));
  return domain_digest(removal_edit_tag, encoded.bytes());
}

template <class T, class I>
bool independently_verify_structural_removal(
    const fv_surface_mesh<T, I> &source, const fv_surface_mesh<T, I> &output,
    const normalization_report &report) {
  std::vector<unsigned char> referenced(source.vertices.size(), 0);
  for (const auto &ring : source.faces)
    for (I index : ring) referenced[static_cast<std::size_t>(index)] = 1;

  std::vector<std::uint64_t> vertex_map(source.vertices.size(),
                                        normalization_removed_ordinal);
  std::size_t retained = 0, removed = 0;
  for (std::uint64_t source_ordinal = 0;
       source_ordinal != source.vertices.size(); ++source_ordinal) {
    if (referenced[static_cast<std::size_t>(source_ordinal)]) {
      vertex_map[static_cast<std::size_t>(source_ordinal)] = retained;
      if (retained >= output.vertices.size() ||
          output.vertices[retained] !=
              source.vertices[static_cast<std::size_t>(source_ordinal)] ||
          (!source.vertex_normals.empty() &&
           output.vertex_normals[retained] !=
               source.vertex_normals[static_cast<std::size_t>(source_ordinal)]) ||
          (!source.vertex_colours.empty() &&
           output.vertex_colours[retained] !=
               source.vertex_colours[static_cast<std::size_t>(source_ordinal)]) ||
          (!source.involved_faces.empty() &&
           output.involved_faces[retained] !=
               source.involved_faces[static_cast<std::size_t>(source_ordinal)]))
        return false;
      ++retained;
    } else {
      if (removed >= report.edits.size()) return false;
      normalization_edit expected;
      expected.operation = normalization_operation::irrelevant_storage_removal;
      expected.canonical_ordinal = removed;
      expected.entity = normalization_entity_kind::vertex;
      expected.source_ordinal = source_ordinal;
      expected.prepared_ordinal = normalization_removed_ordinal;
      expected.before_evidence_digest =
          verifier_removed_vertex_before_digest(source, source_ordinal);
      expected.after_evidence_digest =
          verifier_removed_vertex_after_digest(source_ordinal);
      expected.reversibility = normalization_reversibility::irreversible;
      expected.evidence_digest = verifier_removal_edit_digest(expected);
      const auto &actual = report.edits[removed];
      if (actual.operation != expected.operation ||
          actual.canonical_ordinal != expected.canonical_ordinal ||
          actual.entity != expected.entity ||
          actual.source_ordinal != expected.source_ordinal ||
          actual.prepared_ordinal != expected.prepared_ordinal ||
          actual.before_evidence_digest != expected.before_evidence_digest ||
          actual.after_evidence_digest != expected.after_evidence_digest ||
          actual.reversibility != expected.reversibility ||
          actual.evidence_digest != expected.evidence_digest)
        return false;
      ++removed;
    }
  }
  if (retained != output.vertices.size() || removed != report.edits.size() ||
      output.faces.size() != source.faces.size() ||
      output.metadata != source.metadata ||
      output.vertex_normals.size() !=
          (source.vertex_normals.empty() ? 0 : retained) ||
      output.vertex_colours.size() !=
          (source.vertex_colours.empty() ? 0 : retained) ||
      output.involved_faces.size() !=
          (source.involved_faces.empty() ? 0 : retained) ||
      report.vertices.status != normalization_map_status::total ||
      report.vertices.source_to_prepared != vertex_map ||
      !identity_mapping(report.facets, source.faces.size()) ||
      !same_mapping(report.attributes.vertex_normals,
                    source.vertex_normals.empty()
                        ? normalization_mapping{normalization_map_status::absent,
                                                {}}
                        : report.vertices) ||
      !same_mapping(report.attributes.vertex_colours,
                    source.vertex_colours.empty()
                        ? normalization_mapping{normalization_map_status::absent,
                                                {}}
                        : report.vertices) ||
      !same_mapping(report.attributes.involved_faces,
                    source.involved_faces.empty()
                        ? normalization_mapping{normalization_map_status::absent,
                                                {}}
                        : report.vertices) ||
      !same_mapping(report.attributes.metadata,
                    source.metadata.empty()
                        ? normalization_mapping{normalization_map_status::absent,
                                                {}}
                        : identity(source.metadata.size())))
    return false;

  for (std::size_t f = 0; f != source.faces.size(); ++f) {
    if (output.faces[f].size() != source.faces[f].size()) return false;
    for (std::size_t j = 0; j != source.faces[f].size(); ++j)
      if (static_cast<std::uint64_t>(output.faces[f][j]) !=
          vertex_map[static_cast<std::size_t>(source.faces[f][j])])
        return false;
  }

  std::map<std::array<std::uint64_t, 2>, std::uint64_t> output_edges;
  for (const auto &ring : output.faces)
    for (std::size_t j = 0; j != ring.size(); ++j) {
      auto a = static_cast<std::uint64_t>(ring[j]);
      auto b = static_cast<std::uint64_t>(ring[(j + 1) % ring.size()]);
      if (b < a) std::swap(a, b);
      output_edges.emplace(std::array<std::uint64_t, 2>{{a, b}}, 0);
    }
  std::uint64_t ordinal = 0;
  for (auto &edge : output_edges) edge.second = ordinal++;
  if (report.edges.status != normalization_map_status::total ||
      report.edges.source_to_prepared.size() != report.source_edges.size())
    return false;
  for (std::size_t i = 0; i != report.source_edges.size(); ++i) {
    auto a = vertex_map[static_cast<std::size_t>(report.source_edges[i][0])];
    auto b = vertex_map[static_cast<std::size_t>(report.source_edges[i][1])];
    if (b < a) std::swap(a, b);
    const auto found = output_edges.find({{a, b}});
    if (found == output_edges.end() ||
        report.edges.source_to_prepared[i] != found->second)
      return false;
  }
  return true;
}

bool same_edit(const normalization_edit &a, const normalization_edit &b) {
  return a.operation == b.operation &&
         a.canonical_ordinal == b.canonical_ordinal && a.entity == b.entity &&
         a.source_ordinal == b.source_ordinal &&
         a.prepared_ordinal == b.prepared_ordinal &&
         a.before_evidence_digest == b.before_evidence_digest &&
         a.after_evidence_digest == b.after_evidence_digest &&
         a.reversibility == b.reversibility &&
         a.evidence_digest == b.evidence_digest;
}

bool same_topology_change(const normalization_topology_change &a,
                          const normalization_topology_change &b) {
  return a.operation == b.operation && a.source_ordinal == b.source_ordinal &&
         a.entity == b.entity && a.prepared_ordinal == b.prepared_ordinal &&
         a.justification == b.justification &&
         a.justification_subcode == b.justification_subcode &&
         a.before_evidence_digest == b.before_evidence_digest &&
         a.after_evidence_digest == b.after_evidence_digest &&
         a.reversibility == b.reversibility &&
          a.evidence_digest == b.evidence_digest;
}

bool same_displacement(const normalization_displacement_record &a,
                       const normalization_displacement_record &b) {
  return a.source_vertex == b.source_vertex &&
         a.prepared_vertex == b.prepared_vertex && a.kind == b.kind &&
         a.exact_components == b.exact_components &&
         a.squared_distance_bound.numerator ==
             b.squared_distance_bound.numerator &&
         a.squared_distance_bound.denominator ==
             b.squared_distance_bound.denominator &&
         a.unit == b.unit && a.evidence_digest == b.evidence_digest;
}

template <class T, class I>
bool independently_verify_orientation_repair(
    const fv_surface_mesh<T, I> &source, const fv_surface_mesh<T, I> &output,
    const normalization_report &report) {
  const auto same_scalar_bits = [](T a, T b) {
    std::array<std::uint8_t, sizeof(T)> a_bits{}, b_bits{};
    std::memcpy(a_bits.data(), &a, sizeof(T));
    std::memcpy(b_bits.data(), &b, sizeof(T));
    return a_bits == b_bits;
  };
  const auto same_points = [&](const auto &a, const auto &b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i != a.size(); ++i)
      if (!same_scalar_bits(a[i].x, b[i].x) ||
          !same_scalar_bits(a[i].y, b[i].y) ||
          !same_scalar_bits(a[i].z, b[i].z))
        return false;
    return true;
  };
  if (!same_points(output.vertices, source.vertices) ||
      !same_points(output.vertex_normals, source.vertex_normals) ||
      output.vertex_colours != source.vertex_colours ||
      output.involved_faces != source.involved_faces ||
      output.metadata != source.metadata ||
      output.faces.size() != source.faces.size() ||
      !identity_mapping(report.vertices, source.vertices.size()) ||
      !identity_mapping(report.facets, source.faces.size()) ||
      !identity_mapping(report.edges, report.source_edges.size()) ||
      !report.topology_changes.empty() || !report.displacements.empty())
    return false;

  std::size_t edit_ordinal = 0;
  for (std::uint64_t facet = 0; facet != source.faces.size(); ++facet) {
    const auto &before = source.faces[static_cast<std::size_t>(facet)];
    const auto &after = output.faces[static_cast<std::size_t>(facet)];
    if (before == after) continue;
    auto reversed = before;
    std::reverse(reversed.begin(), reversed.end());
    if (after != reversed || edit_ordinal >= report.edits.size()) return false;
    normalization_edit expected;
    expected.operation = normalization_operation::orientation_repair;
    expected.canonical_ordinal = edit_ordinal;
    expected.entity = normalization_entity_kind::facet;
    expected.source_ordinal = facet;
    expected.prepared_ordinal = facet;
    expected.before_evidence_digest = orientation_facet_digest(
        orientation_before_tag, source, facet);
    expected.after_evidence_digest = orientation_facet_digest(
        orientation_after_tag, output, facet);
    expected.reversibility = normalization_reversibility::fully_reversible;
    expected.evidence_digest = orientation_edit_digest(expected);
    if (!same_edit(expected, report.edits[edit_ordinal])) return false;
    ++edit_ordinal;
  }
  return edit_ordinal == report.edits.size() &&
         report.reversibility ==
             (report.edits.empty()
                  ? normalization_reversibility::identity
                  : normalization_reversibility::fully_reversible) &&
         same_mapping(report.attributes.vertex_normals,
                      source.vertex_normals.empty()
                          ? normalization_mapping{
                                normalization_map_status::absent, {}}
                          : identity(source.vertex_normals.size())) &&
         same_mapping(report.attributes.vertex_colours,
                      source.vertex_colours.empty()
                          ? normalization_mapping{
                                normalization_map_status::absent, {}}
                          : identity(source.vertex_colours.size())) &&
         same_mapping(report.attributes.involved_faces,
                      source.involved_faces.empty()
                          ? normalization_mapping{
                                normalization_map_status::absent, {}}
                          : identity(source.involved_faces.size())) &&
         same_mapping(report.attributes.metadata,
                      source.metadata.empty()
                          ? normalization_mapping{
                                normalization_map_status::absent, {}}
                          : identity(source.metadata.size()));
}

template <class T, class I, class Checkpoint>
status_or<bool> independently_construct_orientation_classes(
    const fv_surface_mesh<T, I> &source, std::vector<bool> &facet_parity,
    std::vector<std::vector<std::size_t>> &shells, Checkpoint checkpoint) {
  struct use { std::size_t facet=0; bool increasing=false; };
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::vector<use>> uses;
  for (std::size_t facet = 0; facet != source.faces.size(); ++facet) {
    const auto &ring = source.faces[facet];
    auto allowed = checkpoint(std::max<std::uint64_t>(1, ring.size()));
    if (!allowed.has_value()) return allowed.error();
    if (ring.size() < 3) return false;
    std::set<std::uint64_t> unique;
    for (std::size_t offset = 0; offset != ring.size(); ++offset) {
      const auto a = static_cast<std::uint64_t>(ring[offset]);
      const auto b = static_cast<std::uint64_t>(ring[(offset + 1) % ring.size()]);
      if (a >= source.vertices.size() || b >= source.vertices.size() ||
          a == b || !unique.insert(a).second)
        return false;
      uses[{std::min(a, b), std::max(a, b)}].push_back({facet, a < b});
    }
  }
  std::vector<std::vector<std::pair<std::size_t, bool>>> adjacency(
      source.faces.size());
  for (const auto &entry : uses) {
    auto allowed = checkpoint(1);
    if (!allowed.has_value()) return allowed.error();
    if (entry.second.size() != 2) return false;
    const auto &a = entry.second[0], &b = entry.second[1];
    const bool opposite_assignment = a.increasing == b.increasing;
    adjacency[a.facet].push_back({b.facet, opposite_assignment});
    adjacency[b.facet].push_back({a.facet, opposite_assignment});
  }
  for (auto &neighbors : adjacency) std::sort(neighbors.begin(), neighbors.end());
  std::vector<int> assignment(source.faces.size(), -1);
  for (std::size_t root = 0; root != source.faces.size(); ++root) {
    if (assignment[root] != -1) continue;
    assignment[root] = 0;
    shells.push_back({root});
    for (std::size_t at = 0; at != shells.back().size(); ++at) {
      auto allowed = checkpoint(1);
      if (!allowed.has_value()) return allowed.error();
      const auto facet = shells.back()[at];
      for (const auto &neighbor : adjacency[facet]) {
        const int expected = assignment[facet] ^ int(neighbor.second);
        if (assignment[neighbor.first] == -1) {
          assignment[neighbor.first] = expected;
          shells.back().push_back(neighbor.first);
        } else if (assignment[neighbor.first] != expected) {
          return false;
        }
      }
    }
  }
  facet_parity.resize(source.faces.size());
  for (std::size_t facet = 0; facet != assignment.size(); ++facet)
    facet_parity[facet] = assignment[facet] != 0;
  return true;
}

template <class I>
std::vector<std::uint64_t>
independent_duplicate_facet_key(const std::vector<I> &face) {
  std::vector<std::uint64_t> values;
  values.reserve(face.size());
  for (I index : face) values.push_back(static_cast<std::uint64_t>(index));
  if (values.empty()) return values;
  const auto minimize = [](const std::vector<std::uint64_t> &ring) {
    std::size_t first = 0, second = 1, matched = 0;
    while (first < ring.size() && second < ring.size() &&
           matched < ring.size()) {
      const auto a = ring[(first + matched) % ring.size()];
      const auto b = ring[(second + matched) % ring.size()];
      if (a == b) {
        ++matched;
      } else {
        if (b < a) {
          first += matched + 1;
          if (first == second) ++first;
        } else {
          second += matched + 1;
          if (first == second) ++second;
        }
        matched = 0;
      }
    }
    const auto origin = std::min(first, second) % ring.size();
    std::vector<std::uint64_t> result;
    result.reserve(ring.size());
    for (std::size_t offset = 0; offset != ring.size(); ++offset)
      result.push_back(ring[(origin + offset) % ring.size()]);
    return result;
  };
  auto forward = minimize(values);
  std::reverse(values.begin(), values.end());
  auto reverse = minimize(values);
  return reverse < forward ? reverse : forward;
}

template <class T, class I>
bool independently_construct_exact_duplicate_output(
    const fv_surface_mesh<T, I> &source, fv_surface_mesh<T, I> &output) {
  const bool normals = !source.vertex_normals.empty();
  const bool colours = !source.vertex_colours.empty();
  const bool involved = !source.involved_faces.empty();
  if ((normals && source.vertex_normals.size() != source.vertices.size()) ||
      (colours && source.vertex_colours.size() != source.vertices.size()) ||
      (involved && source.involved_faces.size() != source.vertices.size()))
    return false;
  fv_surface_mesh<T, I> candidate;
  candidate.metadata = source.metadata;
  std::map<coordinate_key<T>, std::uint64_t> first_by_coordinate;
  std::vector<std::uint64_t> vertex_map;
  vertex_map.reserve(source.vertices.size());
  for (std::uint64_t ordinal = 0; ordinal != source.vertices.size(); ++ordinal) {
    const auto &vertex = source.vertices[static_cast<std::size_t>(ordinal)];
    if (!(std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
          std::isfinite(vertex.z)))
      return false;
    coordinate_key<T> key{};
    T components[3] = {vertex.x == T(0) ? T(0) : vertex.x,
                       vertex.y == T(0) ? T(0) : vertex.y,
                       vertex.z == T(0) ? T(0) : vertex.z};
    for (unsigned component = 0; component != 3; ++component)
      std::memcpy(&key[component], &components[component],
                  sizeof(components[component]));
    const auto inserted = first_by_coordinate.emplace(key, ordinal);
    if (inserted.second) {
      vertex_map.push_back(candidate.vertices.size());
      candidate.vertices.push_back(vertex);
      if (normals)
        candidate.vertex_normals.push_back(
            source.vertex_normals[static_cast<std::size_t>(ordinal)]);
      if (colours)
        candidate.vertex_colours.push_back(
            source.vertex_colours[static_cast<std::size_t>(ordinal)]);
    } else {
      const auto retained = inserted.first->second;
      if ((normals && source.vertex_normals[static_cast<std::size_t>(ordinal)] !=
                          source.vertex_normals[static_cast<std::size_t>(retained)]) ||
          (colours && source.vertex_colours[static_cast<std::size_t>(ordinal)] !=
                          source.vertex_colours[static_cast<std::size_t>(retained)]))
        return false;
      vertex_map.push_back(vertex_map[static_cast<std::size_t>(retained)]);
    }
  }
  std::set<std::vector<std::uint64_t>> retained_facets;
  for (const auto &source_face : source.faces) {
    std::vector<I> rewritten;
    rewritten.reserve(source_face.size());
    for (I source_index : source_face) {
      const auto index = static_cast<std::uint64_t>(source_index);
      if (index >= vertex_map.size() ||
          vertex_map[static_cast<std::size_t>(index)] >
              static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return false;
      rewritten.push_back(
          static_cast<I>(vertex_map[static_cast<std::size_t>(index)]));
    }
    if (rewritten.size() < 3 ||
        std::adjacent_find(rewritten.begin(), rewritten.end()) !=
            rewritten.end() ||
        (rewritten.size() > 1 && rewritten.front() == rewritten.back()))
      return false;
    if (retained_facets.insert(
            independent_duplicate_facet_key(rewritten)).second)
      candidate.faces.push_back(std::move(rewritten));
  }
  if (involved) {
    candidate.involved_faces.resize(candidate.vertices.size());
    for (std::uint64_t facet = 0; facet != candidate.faces.size(); ++facet) {
      if (facet > static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return false;
      for (I vertex : candidate.faces[static_cast<std::size_t>(facet)])
        candidate.involved_faces[static_cast<std::size_t>(vertex)].push_back(
            static_cast<I>(facet));
    }
  }
  output = std::move(candidate);
  return true;
}

template <class T, class I>
bool independently_verify_exact_duplicates(
    const fv_surface_mesh<T, I> &source, const fv_surface_mesh<T, I> &output,
    const normalization_report &report) {
  const bool normals = !source.vertex_normals.empty();
  const bool colours = !source.vertex_colours.empty();
  const bool involved = !source.involved_faces.empty();
  if ((normals && source.vertex_normals.size() != source.vertices.size()) ||
      (colours && source.vertex_colours.size() != source.vertices.size()) ||
      (involved && source.involved_faces.size() != source.vertices.size()))
    return false;

  fv_surface_mesh<T, I> expected;
  expected.metadata = source.metadata;
  std::vector<std::uint64_t> vertex_map;
  vertex_map.reserve(source.vertices.size());
  std::map<coordinate_key<T>, std::uint64_t> first_by_coordinate;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> duplicate_vertices;
  const auto independent_coordinate_key = [](const auto &vertex) {
    coordinate_key<T> key{};
    T values[3] = {vertex.x == T(0) ? T(0) : vertex.x,
                   vertex.y == T(0) ? T(0) : vertex.y,
                   vertex.z == T(0) ? T(0) : vertex.z};
    for (unsigned component = 0; component != 3; ++component)
      std::memcpy(&key[component], &values[component], sizeof(values[component]));
    return key;
  };
  for (std::uint64_t ordinal = 0; ordinal != source.vertices.size(); ++ordinal) {
    const auto &vertex = source.vertices[static_cast<std::size_t>(ordinal)];
    if (!(std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
          std::isfinite(vertex.z)))
      return false;
    const auto inserted = first_by_coordinate.emplace(
        independent_coordinate_key(vertex), ordinal);
    if (inserted.second) {
      vertex_map.push_back(expected.vertices.size());
      expected.vertices.push_back(vertex);
      if (normals)
        expected.vertex_normals.push_back(
            source.vertex_normals[static_cast<std::size_t>(ordinal)]);
      if (colours)
        expected.vertex_colours.push_back(
            source.vertex_colours[static_cast<std::size_t>(ordinal)]);
    } else {
      const auto retained = inserted.first->second;
      if ((normals && source.vertex_normals[static_cast<std::size_t>(ordinal)] !=
                          source.vertex_normals[static_cast<std::size_t>(retained)]) ||
          (colours && source.vertex_colours[static_cast<std::size_t>(ordinal)] !=
                          source.vertex_colours[static_cast<std::size_t>(retained)]))
        return false;
      vertex_map.push_back(vertex_map[static_cast<std::size_t>(retained)]);
      duplicate_vertices.emplace_back(ordinal, retained);
    }
  }

  std::vector<std::uint64_t> facet_map;
  facet_map.reserve(source.faces.size());
  std::map<std::vector<std::uint64_t>, std::pair<std::uint64_t, std::uint64_t>>
      first_by_facet;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> duplicate_facets;
  for (std::uint64_t ordinal = 0; ordinal != source.faces.size(); ++ordinal) {
    std::vector<I> rewritten;
    for (I source_index : source.faces[static_cast<std::size_t>(ordinal)]) {
      const auto index = static_cast<std::uint64_t>(source_index);
      if (index >= vertex_map.size()) return false;
      rewritten.push_back(static_cast<I>(vertex_map[static_cast<std::size_t>(index)]));
    }
    if (rewritten.size() < 3 ||
        std::adjacent_find(rewritten.begin(), rewritten.end()) !=
            rewritten.end() ||
        (rewritten.size() > 1 && rewritten.front() == rewritten.back()))
      return false;
    const auto key = independent_duplicate_facet_key(rewritten);
    const auto found = first_by_facet.find(key);
    if (found == first_by_facet.end()) {
      const auto prepared = static_cast<std::uint64_t>(expected.faces.size());
      first_by_facet.emplace(key, std::make_pair(ordinal, prepared));
      facet_map.push_back(prepared);
      expected.faces.push_back(std::move(rewritten));
    } else {
      facet_map.push_back(found->second.second);
      duplicate_facets.emplace_back(ordinal, found->second.first);
    }
  }
  if (involved) {
    expected.involved_faces.resize(expected.vertices.size());
    for (std::uint64_t facet = 0; facet != expected.faces.size(); ++facet)
      for (I vertex : expected.faces[static_cast<std::size_t>(facet)])
        expected.involved_faces[static_cast<std::size_t>(vertex)].push_back(
            static_cast<I>(facet));
  }
  if (!(expected == output) ||
      expected.involved_faces != output.involved_faces ||
      report.vertices.status != normalization_map_status::total ||
      report.vertices.source_to_prepared != vertex_map ||
      report.facets.status != normalization_map_status::total ||
      report.facets.source_to_prepared != facet_map ||
      report.shells.status != normalization_map_status::unavailable ||
      !report.shells.source_to_prepared.empty() ||
      !same_mapping(report.attributes.vertex_normals,
                    normals ? report.vertices
                            : normalization_mapping{
                                  normalization_map_status::absent, {}}) ||
      !same_mapping(report.attributes.vertex_colours,
                    colours ? report.vertices
                            : normalization_mapping{
                                  normalization_map_status::absent, {}}) ||
      !same_mapping(report.attributes.involved_faces,
                    involved ? report.vertices
                             : normalization_mapping{
                                   normalization_map_status::absent, {}}) ||
      !same_mapping(report.attributes.metadata,
                    source.metadata.empty()
                        ? normalization_mapping{normalization_map_status::absent,
                                                {}}
                        : identity(source.metadata.size())))
    return false;

  std::map<std::array<std::uint64_t, 2>, std::uint64_t> output_edges;
  for (const auto &face : expected.faces)
    for (std::size_t offset = 0; offset != face.size(); ++offset) {
      auto a = static_cast<std::uint64_t>(face[offset]);
      auto b = static_cast<std::uint64_t>(face[(offset + 1) % face.size()]);
      if (b < a) std::swap(a, b);
      output_edges.emplace(std::array<std::uint64_t, 2>{{a, b}}, 0);
    }
  std::uint64_t edge_ordinal = 0;
  for (auto &edge : output_edges) edge.second = edge_ordinal++;
  std::vector<std::uint64_t> edge_map;
  for (const auto &edge : report.source_edges) {
    auto a = vertex_map[static_cast<std::size_t>(edge[0])];
    auto b = vertex_map[static_cast<std::size_t>(edge[1])];
    if (a == b) return false;
    if (b < a) std::swap(a, b);
    const auto found = output_edges.find({{a, b}});
    if (found == output_edges.end()) return false;
    edge_map.push_back(found->second);
  }
  if (report.edges.status != normalization_map_status::total ||
      report.edges.source_to_prepared != edge_map)
    return false;

  std::vector<normalization_edit> edits;
  std::vector<normalization_topology_change> changes;
  const auto append = [&](normalization_entity_kind entity,
                          std::uint64_t source_ordinal,
                          std::uint64_t retained_source,
                          std::uint64_t prepared_ordinal,
                          std::uint32_t subcode) {
    normalization_edit edit;
    edit.operation = normalization_operation::exact_duplicate_consolidation;
    edit.canonical_ordinal = edits.size();
    edit.entity = entity;
    edit.source_ordinal = source_ordinal;
    edit.prepared_ordinal = prepared_ordinal;
    edit.before_evidence_digest = duplicate_before_digest(
        source, entity, source_ordinal, retained_source);
    edit.after_evidence_digest = duplicate_after_digest(
        expected, entity, source_ordinal, prepared_ordinal);
    edit.reversibility = normalization_reversibility::irreversible;
    edit.evidence_digest = duplicate_edit_digest(edit);
    edits.push_back(edit);
    normalization_topology_change change;
    change.operation = normalization_operation::exact_duplicate_consolidation;
    change.source_ordinal = source_ordinal;
    change.entity = entity;
    change.prepared_ordinal = prepared_ordinal;
    change.justification =
        normalization_topology_justification::caller_authorized_repair;
    change.justification_subcode = subcode;
    change.before_evidence_digest = edit.before_evidence_digest;
    change.after_evidence_digest = edit.after_evidence_digest;
    change.reversibility = normalization_reversibility::irreversible;
    change.evidence_digest = duplicate_topology_digest(change);
    changes.push_back(std::move(change));
  };
  for (const auto &duplicate : duplicate_vertices)
    append(normalization_entity_kind::vertex, duplicate.first,
           duplicate.second,
           vertex_map[static_cast<std::size_t>(duplicate.first)], 1);
  for (const auto &duplicate : duplicate_facets)
    append(normalization_entity_kind::facet, duplicate.first, duplicate.second,
           facet_map[static_cast<std::size_t>(duplicate.first)], 2);
  if (edits.size() != report.edits.size() ||
      changes.size() != report.topology_changes.size())
    return false;
  for (std::size_t i = 0; i != edits.size(); ++i)
    if (!same_edit(edits[i], report.edits[i])) return false;
  for (std::size_t i = 0; i != changes.size(); ++i)
    if (!same_topology_change(changes[i], report.topology_changes[i]))
      return false;
  return report.reversibility ==
         (edits.empty() ? normalization_reversibility::identity
                        : normalization_reversibility::irreversible);
}

template <class T, class I>
bool independently_verify_seam_consolidation(
    const fv_surface_mesh<T, I> &source, const fv_surface_mesh<T, I> &output,
    const normalization_report &report, normalization_operation operation) {
  const bool crack = operation == normalization_operation::crack_closure;
  const bool normals = !source.vertex_normals.empty();
  const bool colours = !source.vertex_colours.empty();
  const bool involved = !source.involved_faces.empty();
  if ((normals && source.vertex_normals.size() != source.vertices.size()) ||
      (colours && source.vertex_colours.size() != source.vertices.size()) ||
      (involved && source.involved_faces.size() != source.vertices.size()))
    return false;

  fv_surface_mesh<T, I> expected;
  expected.metadata = source.metadata;
  std::vector<std::uint64_t> vertex_map;
  std::vector<std::uint64_t> representatives;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> merged;
  std::vector<std::uint64_t> crack_partner(
      source.vertices.size(), normalization_removed_ordinal);
  if (crack) {
    std::map<std::array<std::uint64_t, 2>, unsigned> edge_uses;
    for (const auto &ring : source.faces)
      for (std::size_t offset = 0; offset != ring.size(); ++offset) {
        auto first = static_cast<std::uint64_t>(ring[offset]);
        auto second = static_cast<std::uint64_t>(ring[(offset + 1) % ring.size()]);
        if (first >= source.vertices.size() || second >= source.vertices.size())
          return false;
        if (second < first) std::swap(first, second);
        ++edge_uses[{{first, second}}];
      }
    std::vector<unsigned char> boundary(source.vertices.size(), 0);
    for (const auto &entry : edge_uses)
      if (entry.second == 1) {
        boundary[static_cast<std::size_t>(entry.first[0])] = 1;
        boundary[static_cast<std::size_t>(entry.first[1])] = 1;
      }
    for (std::uint64_t first = 0; first != source.vertices.size(); ++first) {
      if (!boundary[static_cast<std::size_t>(first)]) continue;
      for (std::uint64_t second = first + 1; second != source.vertices.size();
           ++second) {
        if (!boundary[static_cast<std::size_t>(second)] ||
            edge_uses.count({{first, second}}) != 0)
          continue;
        if ((normals && source.vertex_normals[static_cast<std::size_t>(first)] !=
                            source.vertex_normals[static_cast<std::size_t>(second)]) ||
            (colours && source.vertex_colours[static_cast<std::size_t>(first)] !=
                            source.vertex_colours[static_cast<std::size_t>(second)]))
          continue;
        auto close = vertices_within_tolerance(
            source.vertices[static_cast<std::size_t>(first)],
            source.vertices[static_cast<std::size_t>(second)],
            report.policy.model_tolerance);
        if (!close.has_value()) return false;
        if (!close.value()) continue;
        if (crack_partner[static_cast<std::size_t>(first)] !=
                normalization_removed_ordinal ||
            crack_partner[static_cast<std::size_t>(second)] !=
                normalization_removed_ordinal)
          return false;
        crack_partner[static_cast<std::size_t>(first)] = second;
        crack_partner[static_cast<std::size_t>(second)] = first;
      }
    }
  }
  for (std::uint64_t ordinal = 0; ordinal != source.vertices.size(); ++ordinal) {
    const auto &vertex = source.vertices[static_cast<std::size_t>(ordinal)];
    if (!(std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
          std::isfinite(vertex.z)))
      return false;
    std::optional<std::uint64_t> retained;
    if (crack && crack_partner[static_cast<std::size_t>(ordinal)] < ordinal)
      retained = crack_partner[static_cast<std::size_t>(ordinal)];
    for (std::uint64_t candidate : representatives) {
      if (crack) break;
      if ((normals &&
           source.vertex_normals[static_cast<std::size_t>(ordinal)] !=
               source.vertex_normals[static_cast<std::size_t>(candidate)]) ||
          (colours &&
           source.vertex_colours[static_cast<std::size_t>(ordinal)] !=
               source.vertex_colours[static_cast<std::size_t>(candidate)]))
        continue;
      auto close = vertices_within_tolerance(
          vertex, source.vertices[static_cast<std::size_t>(candidate)],
          report.policy.model_tolerance);
      if (!close.has_value()) return false;
      if (close.value()) {
        retained = candidate;
        break;
      }
    }
    if (retained) {
      vertex_map.push_back(vertex_map[static_cast<std::size_t>(*retained)]);
      merged.emplace_back(ordinal, *retained);
    } else {
      representatives.push_back(ordinal);
      vertex_map.push_back(expected.vertices.size());
      expected.vertices.push_back(vertex);
      if (normals)
        expected.vertex_normals.push_back(
            source.vertex_normals[static_cast<std::size_t>(ordinal)]);
      if (colours)
        expected.vertex_colours.push_back(
            source.vertex_colours[static_cast<std::size_t>(ordinal)]);
    }
  }
  expected.faces = source.faces;
  for (auto &face : expected.faces) {
    for (auto &index : face) {
      const auto ordinal = static_cast<std::uint64_t>(index);
      if (ordinal >= vertex_map.size() ||
          vertex_map[static_cast<std::size_t>(ordinal)] >
              static_cast<std::uint64_t>(std::numeric_limits<I>::max()))
        return false;
      index = static_cast<I>(vertex_map[static_cast<std::size_t>(ordinal)]);
    }
    if (std::adjacent_find(face.begin(), face.end()) != face.end() ||
        (face.size() > 1 && face.front() == face.back()))
      return false;
  }
  if (involved) {
    expected.involved_faces.resize(expected.vertices.size());
    for (std::uint64_t facet = 0; facet != expected.faces.size(); ++facet)
      for (I vertex : expected.faces[static_cast<std::size_t>(facet)])
        expected.involved_faces[static_cast<std::size_t>(vertex)].push_back(
            static_cast<I>(facet));
  }
  if (!(expected == output) || expected.involved_faces != output.involved_faces ||
      report.vertices.source_to_prepared != vertex_map ||
      !identity_mapping(report.facets, source.faces.size()) ||
      report.shells.status != normalization_map_status::unavailable ||
      !report.shells.source_to_prepared.empty() ||
      !same_mapping(report.attributes.vertex_normals,
                    normals ? report.vertices
                            : normalization_mapping{
                                  normalization_map_status::absent, {}}) ||
      !same_mapping(report.attributes.vertex_colours,
                    colours ? report.vertices
                            : normalization_mapping{
                                  normalization_map_status::absent, {}}) ||
      !same_mapping(report.attributes.involved_faces,
                    involved ? report.vertices
                             : normalization_mapping{
                                   normalization_map_status::absent, {}}) ||
      !same_mapping(report.attributes.metadata,
                    source.metadata.empty()
                        ? normalization_mapping{normalization_map_status::absent,
                                                {}}
                        : identity(source.metadata.size())))
    return false;

  std::map<std::array<std::uint64_t, 2>, std::uint64_t> output_edges;
  for (const auto &face : expected.faces)
    for (std::size_t offset = 0; offset != face.size(); ++offset) {
      auto a = static_cast<std::uint64_t>(face[offset]);
      auto b = static_cast<std::uint64_t>(face[(offset + 1) % face.size()]);
      if (a == b) return false;
      if (b < a) std::swap(a, b);
      output_edges.emplace(std::array<std::uint64_t, 2>{{a, b}}, 0);
    }
  std::uint64_t edge_ordinal = 0;
  for (auto &entry : output_edges) entry.second = edge_ordinal++;
  std::vector<std::uint64_t> edge_map;
  for (const auto &edge : report.source_edges) {
    auto a = vertex_map[static_cast<std::size_t>(edge[0])];
    auto b = vertex_map[static_cast<std::size_t>(edge[1])];
    if (a == b) return false;
    if (b < a) std::swap(a, b);
    const auto found = output_edges.find({{a, b}});
    if (found == output_edges.end()) return false;
    edge_map.push_back(found->second);
  }
  if (report.edges.source_to_prepared != edge_map) return false;

  std::vector<normalization_edit> edits;
  std::vector<normalization_topology_change> changes;
  std::vector<normalization_displacement_record> displacements;
  for (const auto &entry : merged) {
    normalization_edit edit;
    edit.operation = operation;
    edit.canonical_ordinal = edits.size();
    edit.entity = normalization_entity_kind::vertex;
    edit.source_ordinal = entry.first;
    edit.prepared_ordinal = vertex_map[static_cast<std::size_t>(entry.first)];
    edit.before_evidence_digest = duplicate_before_digest(
        source, normalization_entity_kind::vertex, entry.first, entry.second);
    edit.after_evidence_digest = duplicate_after_digest(
        expected, normalization_entity_kind::vertex, entry.first,
        edit.prepared_ordinal);
    edit.reversibility = normalization_reversibility::irreversible;
    edit.evidence_digest = seam_edit_digest(edit);
    edits.push_back(edit);
    normalization_topology_change change;
    change.operation = operation;
    change.source_ordinal = entry.first;
    change.entity = normalization_entity_kind::vertex;
    change.prepared_ordinal = edit.prepared_ordinal;
    change.justification =
        normalization_topology_justification::caller_authorized_repair;
    change.justification_subcode = crack ? 2 : 1;
    change.before_evidence_digest = edit.before_evidence_digest;
    change.after_evidence_digest = edit.after_evidence_digest;
    change.reversibility = normalization_reversibility::irreversible;
    change.evidence_digest = seam_topology_digest(change);
    changes.push_back(change);
    const auto &before = source.vertices[static_cast<std::size_t>(entry.first)];
    const auto &after = source.vertices[static_cast<std::size_t>(entry.second)];
    if (bits_key(before.x, before.y, before.z) !=
        bits_key(after.x, after.y, after.z)) {
      normalization_displacement_record displacement;
      displacement.source_vertex = entry.first;
      displacement.prepared_vertex = edit.prepared_ordinal;
      displacement.kind = normalization_displacement_kind::bounded;
      displacement.squared_distance_bound =
          displacement_bound(report.policy.model_tolerance);
      displacement.unit = report.policy.unit;
      displacement.evidence_digest = seam_displacement_digest(
          source, expected, displacement, report.policy.model_tolerance,
          operation);
      displacements.push_back(std::move(displacement));
    }
  }
  if (edits.size() != report.edits.size() ||
      changes.size() != report.topology_changes.size() ||
      displacements.size() != report.displacements.size())
    return false;
  for (std::size_t i = 0; i != edits.size(); ++i)
    if (!same_edit(edits[i], report.edits[i]) ||
        !same_topology_change(changes[i], report.topology_changes[i]))
      return false;
  for (std::size_t i = 0; i != displacements.size(); ++i)
    if (!same_displacement(displacements[i], report.displacements[i]))
      return false;
  return report.displacement ==
             (displacements.empty()
                  ? normalization_displacement_claim::exact_zero
                  : normalization_displacement_claim::records_present) &&
         report.reversibility ==
             (edits.empty() ? normalization_reversibility::identity
                            : normalization_reversibility::irreversible);
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
    std::optional<boolean_error> source_validation_error;
    if (!prepared.has_value()) {
      if (prepared.error().code != boolean_error_code::input_contract_error)
        return prepared.error();
      source_validation_error = prepared.error();
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
    }
    const normalization_report failure_candidate = candidate;
    const auto publish_failure = [&](const boolean_error &error)
        -> status_or<prepared_operand<T, I>> {
      auto failure = failure_candidate;
      auto estimate = estimated_report_bytes(failure);
      if (!estimate.has_value()) return estimate.error();
      if (estimate.value() > policy.resources.max_report_bytes)
        return limit_error("normalization_report_limit", estimate.value(),
                           policy.resources.max_report_bytes);
      failure.report_digest = normalization_report_digest(failure).value();
      auto bytes = encode_normalization_report(failure);
      if (!bytes.has_value()) return bytes.error();
      if (bytes.value().size() > policy.resources.max_report_bytes)
        return limit_error("normalization_report_limit", bytes.value().size(),
                           policy.resources.max_report_bytes);
      published = std::move(failure);
      return error;
    };
    const bool duplicate_repair =
        policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             exact_duplicate_consolidation);
    const bool orientation_repair =
        policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             orientation_repair);
    const bool seam_repair =
        policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             seam_aware_vertex_consolidation);
    const bool crack_repair =
        policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::crack_closure);
    const bool proximity_repair = seam_repair || crack_repair;
    const bool repairable_orientation_error =
        source_validation_error &&
        (source_validation_error->subcode ==
             static_cast<std::uint32_t>(
                 input_validation_subcode::same_direction_uses) ||
         source_validation_error->subcode ==
             static_cast<std::uint32_t>(
                 input_validation_subcode::orientation_mismatch));
    const bool repairable_crack_error =
        source_validation_error &&
        source_validation_error->subcode == static_cast<std::uint32_t>(
                                                input_validation_subcode::boundary_edge);
    if (source_validation_error && !duplicate_repair && !seam_repair &&
        !(crack_repair && repairable_crack_error) &&
        !(orientation_repair && repairable_orientation_error)) {
      return publish_failure(*source_validation_error);
    }

    fv_surface_mesh<T, I> normalized_mesh = source;
    if (policy.mode == normalization_mode::structural_only || proximity_repair) {
      auto structural_work = resources.checkpoint(binding_work.value());
      if (!structural_work.has_value()) return structural_work.error();
      if (proximity_repair) {
        auto seam = consolidate_seam_aware_vertices(
            source, candidate.source_edges, policy,
            crack_repair ? normalization_operation::crack_closure
                         : normalization_operation::seam_aware_vertex_consolidation,
            resources);
        if (!seam.has_value() &&
            seam.error().code == boolean_error_code::resource_limit)
          return seam.error();
        if (!seam.has_value()) return publish_failure(seam.error());
        normalized_mesh = std::move(seam.value().mesh);
        candidate.vertices = std::move(seam.value().vertices);
        candidate.edges = std::move(seam.value().edges);
        candidate.facets = std::move(seam.value().facets);
        candidate.attributes.vertex_normals =
            std::move(seam.value().vertex_normals);
        candidate.attributes.vertex_colours =
            std::move(seam.value().vertex_colours);
        candidate.attributes.involved_faces =
            std::move(seam.value().involved_faces);
        candidate.attributes.metadata = std::move(seam.value().metadata);
        candidate.edits = std::move(seam.value().edits);
        candidate.topology_changes = std::move(seam.value().topology_changes);
        candidate.displacements = std::move(seam.value().displacements);
        candidate.displacement = candidate.displacements.empty()
            ? normalization_displacement_claim::exact_zero
            : normalization_displacement_claim::records_present;
      } else if (duplicate_repair) {
        auto duplicates = consolidate_exact_duplicates(
            source, candidate.source_edges, resources);
        if (!duplicates.has_value() &&
            duplicates.error().code == boolean_error_code::resource_limit)
          return duplicates.error();
        if (!duplicates.has_value())
          return publish_failure(duplicates.error());
        normalized_mesh = std::move(duplicates.value().mesh);
        candidate.vertices = std::move(duplicates.value().vertices);
        candidate.edges = std::move(duplicates.value().edges);
        candidate.facets = std::move(duplicates.value().facets);
        candidate.attributes.vertex_normals =
            std::move(duplicates.value().vertex_normals);
        candidate.attributes.vertex_colours =
            std::move(duplicates.value().vertex_colours);
        candidate.attributes.involved_faces =
            std::move(duplicates.value().involved_faces);
        candidate.attributes.metadata =
            std::move(duplicates.value().metadata);
        candidate.edits = std::move(duplicates.value().edits);
        candidate.topology_changes =
            std::move(duplicates.value().topology_changes);
      } else if (orientation_repair) {
        auto orientation = repair_orientation(source, resources, cancel);
        if (!orientation.has_value() &&
            orientation.error().code == boolean_error_code::resource_limit)
          return orientation.error();
        if (!orientation.has_value())
          return publish_failure(orientation.error());
        normalized_mesh = std::move(orientation.value().mesh);
        candidate.edits = std::move(orientation.value().edits);
      } else {
        auto structural = remove_irrelevant_storage(
            source, candidate.source_edges, resources);
        if (!structural.has_value()) return structural.error();
        normalized_mesh = std::move(structural.value().mesh);
        candidate.vertices = std::move(structural.value().vertices);
        candidate.edges = std::move(structural.value().edges);
        candidate.facets = std::move(structural.value().facets);
        candidate.attributes.vertex_normals =
            std::move(structural.value().vertex_normals);
        candidate.attributes.vertex_colours =
            std::move(structural.value().vertex_colours);
        candidate.attributes.involved_faces =
            std::move(structural.value().involved_faces);
        candidate.attributes.metadata = std::move(structural.value().metadata);
        candidate.edits = std::move(structural.value().edits);
      }
      candidate.reversibility =
          candidate.edits.empty()
              ? normalization_reversibility::identity
              : (orientation_repair
                     ? normalization_reversibility::fully_reversible
                     : normalization_reversibility::irreversible);
      candidate.output_digest =
          preparation_detail::canonical_mesh_digest(normalized_mesh);
      auto output_diagnosis = producer_diagnosis(normalized_mesh, resources);
      if (!output_diagnosis.has_value()) return output_diagnosis.error();
      candidate.unresolved_defects =
          std::move(output_diagnosis.value().defects);
      auto output_prepared = validate_operand_strict(
          normalized_mesh, strict_validation_policy{}, boolean_options{}, kernel,
          verifier, cancel);
      if (!output_prepared.has_value() &&
          output_prepared.error().code !=
              boolean_error_code::input_contract_error)
        return output_prepared.error();
      if (!output_prepared.has_value() && source_validation_error)
        return publish_failure(*source_validation_error);
      if (!output_prepared.has_value())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::input_validation,
                          "normalization_structural_revalidation");
      prepared = std::move(output_prepared);
    }

    candidate.prepared_operand_available = true;
    candidate.strict_certificate = prepared.value().certificate();
    if (!duplicate_repair && !seam_repair && !crack_repair) {
      auto shells = component2_shell_count(normalized_mesh, cancel);
      if (!shells.has_value()) return shells.error();
      auto shell_resources = resources.mapping(shells.value());
      if (!shell_resources.has_value()) return shell_resources.error();
      candidate.shells = identity(shells.value());
    }
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
    state->normalization_source =
        std::make_shared<const fv_surface_mesh<T, I>>(source);
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
          report.policy.resources.max_defect_records ||
      report.edits.size() > report.policy.resources.max_defect_records -
                                  report.unresolved_defects.size() ||
      report.topology_changes.size() >
          report.policy.resources.max_defect_records -
              report.unresolved_defects.size() - report.edits.size())
    return limit_error("normalization_verification_defect_limit",
                       report.unresolved_defects.size() + report.edits.size() +
                           report.topology_changes.size(),
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
  const bool diagnosis =
      report.policy.mode == normalization_mode::diagnosis_only;
  const bool duplicate_repair =
      report.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             exact_duplicate_consolidation);
  const bool orientation_repair =
      report.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             orientation_repair);
  const bool seam_repair =
      report.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::
                                             seam_aware_vertex_consolidation);
  const bool crack_repair =
      report.policy.enabled_operations == normalization_operation_bit(
                                         normalization_operation::crack_closure);
  const bool proximity_repair = seam_repair || crack_repair;
  const auto proximity_operation = crack_repair
      ? normalization_operation::crack_closure
      : normalization_operation::seam_aware_vertex_consolidation;
  const auto attribute_binding_valid = [&](const normalization_mapping &mapping,
                                           bool present,
                                           std::uint64_t count) {
    if (mapping.status != (present ? normalization_map_status::total
                                   : normalization_map_status::absent))
      return false;
    if (!diagnosis) return true;
    return present ? identity_mapping(mapping, count)
                   : mapping.source_to_prepared.empty();
  };
  const auto attribute_identity_valid = [&](const normalization_mapping &mapping,
                                            bool present,
                                            std::uint64_t count) {
    return present ? identity_mapping(mapping, count)
                   : mapping.status == normalization_map_status::absent &&
                         mapping.source_to_prepared.empty();
  };
  if (report.coordinate != coordinate_type(std::is_same<T, float>::value) ||
      report.index != index_type(std::is_same<I, std::uint32_t>::value) ||
      report.source_digest !=
          preparation_detail::canonical_mesh_digest(source) ||
      (diagnosis && report.output_digest != report.source_digest) ||
      report.prepared_operand_available != (output != nullptr) ||
      (output && preparation_detail::canonical_mesh_digest(*output) !=
                     report.output_digest) ||
      !attribute_binding_valid(report.attributes.vertex_normals,
                               !source.vertex_normals.empty(),
                               source.vertex_normals.size()) ||
      !attribute_binding_valid(report.attributes.vertex_colours,
                               !source.vertex_colours.empty(),
                               source.vertex_colours.size()) ||
      !attribute_binding_valid(report.attributes.involved_faces,
                               !source.involved_faces.empty(),
                               source.involved_faces.size()) ||
      !attribute_binding_valid(report.attributes.metadata,
                               !source.metadata.empty(),
                               source.metadata.size()))
    return normalization_error("normalization_report_stale_binding");

  auto independent = verifier_diagnosis(source, verification_resources);
  if (!independent.has_value()) return independent.error();
  if (report.source_edges != independent.value().edges ||
      (diagnosis &&
       !identity_mapping(report.edges, independent.value().edges.size())))
    return normalization_error("normalization_report_edge_map");

  auto registry = component2_registry();
  std::shared_ptr<const exact_kernel_services<T>> kernel =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> verifier = registry;
  auto replay = validate_operand_strict(source, strict_validation_policy{},
                                        boolean_options{}, kernel, verifier,
                                        cancel);
  if (proximity_repair) {
    const auto count = static_cast<std::uint64_t>(source.vertices.size());
    if (count != 0 && count >
        std::numeric_limits<std::uint64_t>::max() / count / 12)
      return limit_error("normalization_verification_work_overflow");
    auto transform_work = verification_resources.checkpoint(count * count * 12);
    if (!transform_work.has_value()) return transform_work.error();
    for (std::size_t i = 0;
         i != report.edits.size() + report.topology_changes.size() +
                  report.displacements.size();
         ++i) {
      auto record = verification_resources.record();
      if (!record.has_value()) return record.error();
    }
  }
  if (proximity_repair && output) {
    if (!independently_verify_seam_consolidation(
            source, *output, report, proximity_operation))
      return normalization_error(crack_repair
                                     ? "normalization_report_crack_replay"
                                     : "normalization_report_seam_replay");
    auto output_diagnosis = verifier_diagnosis(*output, verification_resources);
    if (!output_diagnosis.has_value()) return output_diagnosis.error();
    auto output_replay = validate_operand_strict(
        *output, strict_validation_policy{}, boolean_options{}, kernel, verifier,
        cancel);
    if (!output_replay.has_value())
      return normalization_error("normalization_report_seam_validation");
    if (!report.prepared_operand_available || !report.strict_certificate ||
        report.unresolved_defects != output_diagnosis.value().defects ||
        !same_certificate(output_replay.value().certificate(),
                          *report.strict_certificate))
      return normalization_error("normalization_report_seam_success");
    return true;
  }
  if (proximity_repair && !output) {
    auto expected_defects = independent.value().defects;
    if (!replay.has_value()) {
      if (replay.error().code != boolean_error_code::input_contract_error)
        return replay.error();
      expected_defects.push_back(
          {normalization_defect_code::component2_rejection,
           replay.error().subcode, 0, 0});
      std::sort(expected_defects.begin(), expected_defects.end(),
                [](const auto &a, const auto &b) {
                  return std::tie(a.code, a.primary_ordinal,
                                  a.secondary_ordinal, a.detail) <
                         std::tie(b.code, b.primary_ordinal,
                                  b.secondary_ordinal, b.detail);
                });
      expected_defects.erase(
          std::unique(expected_defects.begin(), expected_defects.end()),
          expected_defects.end());
    }
    if (report.prepared_operand_available || report.strict_certificate ||
        report.output_digest != report.source_digest || !report.edits.empty() ||
        !report.topology_changes.empty() || !report.displacements.empty() ||
        report.displacement != normalization_displacement_claim::exact_zero ||
        report.reversibility != normalization_reversibility::identity ||
        report.shells.status != normalization_map_status::unavailable ||
        !identity_mapping(report.vertices, source.vertices.size()) ||
        !identity_mapping(report.edges, independent.value().edges.size()) ||
        !identity_mapping(report.facets, source.faces.size()) ||
        report.unresolved_defects != expected_defects)
      return normalization_error("normalization_report_seam_failure");
    verification_budget reconstruction_resources{report.policy, cancel};
    auto reconstructed = consolidate_seam_aware_vertices(
        source, report.source_edges, report.policy, proximity_operation,
        reconstruction_resources);
    if (reconstructed.has_value()) {
      auto reconstructed_replay = validate_operand_strict(
          reconstructed.value().mesh, strict_validation_policy{},
          boolean_options{}, kernel, verifier, cancel);
      if (reconstructed_replay.has_value())
        return normalization_error("normalization_report_seam_false_failure");
      if (reconstructed_replay.error().code !=
          boolean_error_code::input_contract_error)
        return reconstructed_replay.error();
    } else if (reconstructed.error().code == boolean_error_code::resource_limit) {
      return reconstructed.error();
    }
    return true;
  }
  if (orientation_repair && output) {
    auto transform_work = verification_resources.checkpoint(binding_work.value());
    if (!transform_work.has_value()) return transform_work.error();
    for (std::size_t i = 0; i != report.edits.size(); ++i) {
      auto record = verification_resources.record();
      if (!record.has_value()) return record.error();
    }
    if (!independently_verify_orientation_repair(source, *output, report))
      return normalization_error("normalization_report_orientation_replay");
    auto output_diagnosis = verifier_diagnosis(*output, verification_resources);
    if (!output_diagnosis.has_value()) return output_diagnosis.error();
    auto output_replay = validate_operand_strict(
        *output, strict_validation_policy{}, boolean_options{}, kernel, verifier,
        cancel);
    if (!output_replay.has_value() &&
        output_replay.error().code != boolean_error_code::input_contract_error)
      return output_replay.error();
    if (!output_replay.has_value())
      return normalization_error("normalization_report_orientation_validation");
    auto shells = component2_shell_count(*output, cancel);
    if (!shells.has_value()) return shells.error();
    if (!report.prepared_operand_available || !report.strict_certificate ||
        report.unresolved_defects != output_diagnosis.value().defects ||
        !identity_mapping(report.shells, shells.value()) ||
        !same_certificate(output_replay.value().certificate(),
                          *report.strict_certificate))
      return normalization_error("normalization_report_orientation_success");
    return true;
  }
  if (duplicate_repair) {
    auto transform_work = verification_resources.checkpoint(binding_work.value());
    if (!transform_work.has_value()) return transform_work.error();
    if (source.vertices.size() >
        std::numeric_limits<std::uint64_t>::max() / 3)
      return limit_error("normalization_verification_work_overflow");
    transform_work = verification_resources.checkpoint(
        static_cast<std::uint64_t>(source.vertices.size()) * 3);
    if (!transform_work.has_value()) return transform_work.error();
    for (std::uint64_t ordinal = 0; ordinal != source.faces.size(); ++ordinal) {
      auto required = facet_work(
          source.faces[static_cast<std::size_t>(ordinal)].size(), ordinal);
      if (!required.has_value()) return required.error();
      transform_work = verification_resources.checkpoint(required.value());
      if (!transform_work.has_value()) return transform_work.error();
    }
  }
  if (duplicate_repair && output) {
    for (std::size_t i = 0;
         i != report.edits.size() + report.topology_changes.size(); ++i) {
      auto record = verification_resources.record();
      if (!record.has_value()) return record.error();
    }
    if (!independently_verify_exact_duplicates(source, *output, report))
      return normalization_error("normalization_report_duplicate_replay");
    auto output_diagnosis = verifier_diagnosis(*output, verification_resources);
    if (!output_diagnosis.has_value()) return output_diagnosis.error();
    auto output_replay = validate_operand_strict(
        *output, strict_validation_policy{}, boolean_options{}, kernel, verifier,
        cancel);
    if (!output_replay.has_value())
      return normalization_error("normalization_report_duplicate_validation");
    if (!report.prepared_operand_available || !report.strict_certificate ||
        report.unresolved_defects != output_diagnosis.value().defects ||
        !same_certificate(output_replay.value().certificate(),
                          *report.strict_certificate))
      return normalization_error("normalization_report_duplicate_success");
    return true;
  }
  if (duplicate_repair && !output) {
    auto expected_defects = independent.value().defects;
    if (!replay.has_value()) {
      if (replay.error().code != boolean_error_code::input_contract_error)
        return replay.error();
      expected_defects.push_back(
          {normalization_defect_code::component2_rejection,
           replay.error().subcode, 0, 0});
      std::sort(expected_defects.begin(), expected_defects.end(),
                [](const auto &a, const auto &b) {
                  return std::tie(a.code, a.primary_ordinal,
                                  a.secondary_ordinal, a.detail) <
                         std::tie(b.code, b.primary_ordinal,
                                  b.secondary_ordinal, b.detail);
                });
      expected_defects.erase(
          std::unique(expected_defects.begin(), expected_defects.end()),
          expected_defects.end());
    }
    if (report.prepared_operand_available || report.strict_certificate ||
        report.output_digest != report.source_digest || !report.edits.empty() ||
        !report.topology_changes.empty() ||
        report.reversibility != normalization_reversibility::identity ||
        report.shells.status != normalization_map_status::unavailable ||
        !report.shells.source_to_prepared.empty() ||
        !identity_mapping(report.vertices, source.vertices.size()) ||
        !identity_mapping(report.edges, independent.value().edges.size()) ||
        !identity_mapping(report.facets, source.faces.size()) ||
        !attribute_identity_valid(report.attributes.vertex_normals,
                                  !source.vertex_normals.empty(),
                                  source.vertex_normals.size()) ||
        !attribute_identity_valid(report.attributes.vertex_colours,
                                  !source.vertex_colours.empty(),
                                  source.vertex_colours.size()) ||
        !attribute_identity_valid(report.attributes.involved_faces,
                                  !source.involved_faces.empty(),
                                  source.involved_faces.size()) ||
        !attribute_identity_valid(report.attributes.metadata,
                                  !source.metadata.empty(),
                                  source.metadata.size()) ||
        report.unresolved_defects != expected_defects)
      return normalization_error("normalization_report_duplicate_failure");
    fv_surface_mesh<T, I> reconstructed;
    if (independently_construct_exact_duplicate_output(source, reconstructed)) {
      auto reconstructed_replay = validate_operand_strict(
          reconstructed, strict_validation_policy{}, boolean_options{}, kernel,
          verifier, cancel);
      if (reconstructed_replay.has_value())
        return normalization_error(
            "normalization_report_duplicate_false_failure");
      if (reconstructed_replay.error().code !=
          boolean_error_code::input_contract_error)
        return reconstructed_replay.error();
    }
    return true;
  }
  if (replay.has_value()) {
    const fv_surface_mesh<T, I> *strict_output = &source;
    std::vector<normalization_defect> expected_defects =
        independent.value().defects;
    if (!diagnosis) {
      if (!output)
        return normalization_error("normalization_report_structural_output");
      auto removal_work = verification_resources.checkpoint(binding_work.value());
      if (!removal_work.has_value()) return removal_work.error();
      removal_work = verification_resources.checkpoint(
          static_cast<std::uint64_t>(source.vertices.size()));
      if (!removal_work.has_value()) return removal_work.error();
      for (std::size_t i = 0; i != report.edits.size(); ++i) {
        auto record = verification_resources.record();
        if (!record.has_value()) return record.error();
      }
      if (!independently_verify_structural_removal(source, *output, report) ||
          report.reversibility !=
              (report.edits.empty()
                   ? normalization_reversibility::identity
                   : normalization_reversibility::irreversible))
        return normalization_error("normalization_report_structural_replay");
      strict_output = output;
      auto output_diagnosis = verifier_diagnosis(*output, verification_resources);
      if (!output_diagnosis.has_value()) return output_diagnosis.error();
      expected_defects = std::move(output_diagnosis.value().defects);
      replay = validate_operand_strict(*output, strict_validation_policy{},
                                       boolean_options{}, kernel, verifier,
                                       cancel);
      if (!replay.has_value())
        return normalization_error("normalization_report_structural_validation");
    } else if (!identity_mapping(report.vertices, source.vertices.size()) ||
               !identity_mapping(report.facets, source.faces.size())) {
      return normalization_error("normalization_report_identity_map");
    }
    auto shells = component2_shell_count(*strict_output, cancel);
    if (!shells.has_value()) return shells.error();
    if (!report.prepared_operand_available || !report.strict_certificate ||
        expected_defects != report.unresolved_defects ||
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
        report.output_digest != report.source_digest || !report.edits.empty() ||
        report.reversibility != normalization_reversibility::identity ||
        !identity_mapping(report.vertices, source.vertices.size()) ||
        !identity_mapping(report.edges, independent.value().edges.size()) ||
        !identity_mapping(report.facets, source.faces.size()) ||
        !attribute_identity_valid(report.attributes.vertex_normals,
                                  !source.vertex_normals.empty(),
                                  source.vertex_normals.size()) ||
        !attribute_identity_valid(report.attributes.vertex_colours,
                                  !source.vertex_colours.empty(),
                                  source.vertex_colours.size()) ||
        !attribute_identity_valid(report.attributes.involved_faces,
                                  !source.involved_faces.empty(),
                                  source.involved_faces.size()) ||
        !attribute_identity_valid(report.attributes.metadata,
                                  !source.metadata.empty(),
                                  source.metadata.size()) ||
        independent.value().defects != report.unresolved_defects)
      return normalization_error("normalization_report_failure_claim");
    if (orientation_repair &&
        (replay.error().subcode == static_cast<std::uint32_t>(
                                       input_validation_subcode::
                                           same_direction_uses) ||
         replay.error().subcode == static_cast<std::uint32_t>(
                                       input_validation_subcode::
                                           orientation_mismatch))) {
      auto search_work = verification_resources.checkpoint(binding_work.value());
      if (!search_work.has_value()) return search_work.error();
      std::vector<bool> parity;
      std::vector<std::vector<std::size_t>> shells;
      auto constructed = independently_construct_orientation_classes(
          source, parity, shells, [&](std::uint64_t work) {
            return verification_resources.checkpoint(work);
          });
      if (!constructed.has_value()) return constructed.error();
      if (constructed.value()) {
        if (shells.size() > 12)
          return limit_error("normalization_orientation_failure_search_limit",
                             shells.size(), 12);
        const std::uint64_t assignments = std::uint64_t(1) << shells.size();
        std::vector<std::size_t> facet_shell(source.faces.size());
        for (std::size_t shell = 0; shell != shells.size(); ++shell)
          for (auto facet : shells[shell]) facet_shell[facet] = shell;
        fv_surface_mesh<T, I> reconstructed = source;
        std::uint64_t assignment_work = binding_work.value();
        const auto facets = static_cast<std::uint64_t>(source.faces.size());
        if (facets != 0 &&
            facets > (std::numeric_limits<std::uint64_t>::max() -
                      assignment_work) /
                         facets)
          return limit_error("normalization_verification_work_overflow");
        assignment_work += facets * facets;
        for (const auto &ring : source.faces) {
          const auto size = static_cast<std::uint64_t>(ring.size());
          if (size != 0 &&
              size > (std::numeric_limits<std::uint64_t>::max() -
                      assignment_work) /
                         size)
            return limit_error("normalization_verification_work_overflow");
          assignment_work += size * size;
        }
        for (std::uint64_t assignment = 0; assignment != assignments;
             ++assignment) {
          search_work = verification_resources.checkpoint(assignment_work);
          if (!search_work.has_value()) return search_work.error();
          for (std::size_t facet = 0; facet != source.faces.size(); ++facet) {
            reconstructed.faces[facet] = source.faces[facet];
            const bool reverse = parity[facet] ^
                (((assignment >> facet_shell[facet]) & 1U) != 0);
            if (reverse)
              std::reverse(reconstructed.faces[facet].begin(),
                           reconstructed.faces[facet].end());
          }
          auto reconstructed_replay = validate_operand_strict(
              reconstructed, strict_validation_policy{}, boolean_options{},
              kernel, verifier, cancel);
          if (reconstructed_replay.has_value())
            return normalization_error(
                "normalization_report_orientation_false_failure");
          if (reconstructed_replay.error().code !=
              boolean_error_code::input_contract_error)
            return reconstructed_replay.error();
        }
      }
    }
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
