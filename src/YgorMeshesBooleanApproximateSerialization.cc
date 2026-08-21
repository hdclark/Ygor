#include "YgorMeshesBooleanApproximate.h"

#include <iomanip>
#include <sstream>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> envelope_tag{{'Y', 'G', 'B', 'A', 'E', 'M', '0', '1'}};

class byte_reader {
public:
  byte_reader(const std::vector<std::uint8_t> &bytes,
              const approximate_decode_limits &limits,
              approximate_verifier_budget *budget = nullptr)
      : bytes_(bytes), limits_(limits), budget_(budget) {}

  std::uint8_t u8() {
    need(1);
    return bytes_[position_++];
  }
  std::uint16_t u16() {
    std::uint16_t value = 0;
    for (unsigned i = 0; i < 2; ++i)
      value = static_cast<std::uint16_t>((value << 8) | u8());
    return value;
  }
  std::uint32_t u32() {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
      value = (value << 8) | u8();
    return value;
  }
  std::uint64_t u64() {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i)
      value = (value << 8) | u8();
    return value;
  }
  bool boolean() {
    const auto value = u8();
    if (value > 1)
      throw std::runtime_error("noncanonical_boolean");
    return value != 0;
  }
  std::vector<std::uint8_t> bytes(std::uint64_t limit) {
    const auto size = u64();
    if (size > limit || size > remaining())
      throw std::length_error("byte_string_limit");
    charge_bytes(size);
    std::vector<std::uint8_t> output(
        bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
        bytes_.begin() + static_cast<std::ptrdiff_t>(position_ + size));
    position_ += static_cast<std::size_t>(size);
    return output;
  }
  digest digest_value() {
    digest output;
    for (auto &byte : output.bytes)
      byte = u8();
    return output;
  }
  exact_scalar rational() {
    const auto sign_byte = u8();
    if (sign_byte != 0 && sign_byte != 1 && sign_byte != 0xff)
      throw std::runtime_error("integer_sign");
    const auto numerator_bytes = bytes(limits_.max_exact_integer_bytes);
    const auto denominator_bytes = bytes(limits_.max_exact_integer_bytes);
    auto from_bytes = [](const std::vector<std::uint8_t> &input) {
      if (input.empty())
        return big_uint{};
      if (input.front() == 0)
        throw std::runtime_error("noncanonical_integer");
      std::ostringstream hex;
      hex << std::hex << std::setfill('0');
      for (const auto byte : input)
        hex << std::setw(2) << static_cast<unsigned>(byte);
      auto parsed = big_uint::from_hex(hex.str(),
                                       boolean_stage::final_verification);
      if (!parsed.has_value())
        throw std::runtime_error("integer_decode");
      return parsed.value();
    };
    const auto magnitude = from_bytes(numerator_bytes);
    const auto denominator = from_bytes(denominator_bytes);
    if (denominator.is_zero())
      throw std::runtime_error("zero_denominator");
    integer_sign sign = integer_sign::zero;
    if (sign_byte == 1)
      sign = integer_sign::positive;
    else if (sign_byte == 0xff)
      sign = integer_sign::negative;
    if ((sign == integer_sign::zero) != magnitude.is_zero())
      throw std::runtime_error("noncanonical_sign");
    return exact_scalar(big_int(sign, magnitude), denominator);
  }
  exact_point3 point() { return {rational(), rational(), rational()}; }
  std::uint64_t count(std::uint64_t limit) {
    const auto value = u64();
    if (value > limit || value > limits_.max_references ||
        records_ > limits_.max_references - value)
      throw std::length_error("entity_limit");
    records_ += value;
    if (budget_ &&
        !budget_->records(value, "approximate_serialization.record_limit"))
      throw std::length_error("policy_record_limit");
    return value;
  }
  void work(std::uint64_t amount = 1) {
    if (budget_ &&
        !budget_->work(amount, "approximate_serialization.work_limit"))
      throw std::length_error("policy_work_limit");
  }
  std::size_t remaining() const { return bytes_.size() - position_; }

private:
  void need(std::size_t count) {
    if (count > remaining())
      throw std::runtime_error("truncated");
    charge_bytes(count);
  }
  void charge_bytes(std::size_t count) {
    if (budget_ &&
        (!budget_->bytes(count, "approximate_serialization.byte_limit") ||
         !budget_->work(count, "approximate_serialization.work_limit")))
      throw std::length_error("policy_byte_or_work_limit");
  }
  const std::vector<std::uint8_t> &bytes_;
  const approximate_decode_limits &limits_;
  std::size_t position_ = 0;
  std::uint64_t records_ = 0;
  approximate_verifier_budget *budget_ = nullptr;
};

template <class Id> Id identifier(byte_reader &reader) {
  return Id::from_canonical_value(reader.u64());
}

template <class Value, class Read>
std::vector<Value> vector_of(byte_reader &reader, std::uint64_t limit,
                             Read read) {
  const auto count = reader.count(limit);
  std::vector<Value> output;
  output.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i < count; ++i) {
    reader.work();
    output.push_back(read());
  }
  return output;
}

template <class T, class I>
certified_approximate_certificate<T, I> decode_certificate(
    const std::vector<std::uint8_t> &bytes,
    const product_realization_policy &policy,
    const approximate_decode_limits &limits,
    approximate_verifier_budget &budget) {
  byte_reader reader(bytes, limits, &budget);
  certified_approximate_certificate<T, I> c;
  c.schema = reader.u16();
  c.coordinate = static_cast<coordinate_tag>(reader.u8());
  c.index = static_cast<index_tag>(reader.u8());
  c.exact_result_digest = reader.digest_value();
  c.selected_boundary_digest = reader.digest_value();
  c.policy_digest = reader.digest_value();
  c.policy = policy;
  c.exact_vertex_occurrences = reader.u64();
  c.exact_edges = reader.u64();
  c.exact_halfedges = reader.u64();
  c.exact_cycles = reader.u64();
  c.exact_patches = reader.u64();
  c.vertices = vector_of<approximate_vertex_evidence<T>>(
      reader, limits.max_entities, [&] {
        approximate_vertex_evidence<T> value;
        value.selected = identifier<selected_vertex_id>(reader);
        value.exact_target = reader.point();
        for (auto &bits : value.output_bits) {
          const auto raw = reader.u64();
          if constexpr (std::is_same<T, float>::value) {
            if (raw > std::numeric_limits<std::uint32_t>::max())
              throw std::runtime_error("binary32_bits");
            bits.bits = static_cast<std::uint32_t>(raw);
          } else {
            bits.bits = raw;
          }
        }
        value.displacement = reader.point();
        value.squared_displacement = reader.rational();
        for (auto &rank : value.accepted_axis_ranks)
          rank = reader.u32();
        value.accepted_candidate_rank = reader.u64();
        value.original_vertex = reader.boolean();
        return value;
      });
  c.triangles = vector_of<approximate_triangle_evidence>(
      reader, limits.max_entities, [&] {
        approximate_triangle_evidence value;
        value.triangle = identifier<realization_triangle_id>(reader);
        value.patch = identifier<selected_patch_id>(reader);
        for (auto &vertex : value.vertices)
          vertex = identifier<selected_vertex_id>(reader);
        value.projection = static_cast<projection_axis>(reader.u8());
        value.exact_orientation =
            static_cast<exact_sign>(static_cast<std::int8_t>(reader.u8()));
        for (auto &deviation : value.support_plane_deviations)
          deviation = reader.rational();
        for (auto &deviation :
             value.normalized_squared_support_plane_deviations)
          deviation = reader.rational();
        return value;
      });
  c.occurrence_maps = vector_of<approximate_occurrence_map>(
      reader, limits.max_entities, [&] {
        approximate_occurrence_map value;
        value.occurrence = identifier<selected_vertex_occurrence_id>(reader);
        value.output_vertex = identifier<selected_vertex_id>(reader);
        value.exact_cyclic_halfedges = vector_of<selected_halfedge_id>(
            reader, limits.max_references,
            [&] { return identifier<selected_halfedge_id>(reader); });
        value.output_cyclic_triangles = vector_of<realization_triangle_id>(
            reader, limits.max_references,
            [&] { return identifier<realization_triangle_id>(reader); });
        return value;
      });
  c.patch_adjacency = vector_of<approximate_patch_adjacency>(
      reader, limits.max_entities, [&] {
        approximate_patch_adjacency value;
        value.edge = identifier<selected_edge_id>(reader);
        value.exact_radial_patches = vector_of<selected_patch_id>(
            reader, limits.max_references,
            [&] { return identifier<selected_patch_id>(reader); });
        value.output_incident_triangles = vector_of<realization_triangle_id>(
            reader, limits.max_references,
            [&] { return identifier<realization_triangle_id>(reader); });
        return value;
      });
  c.obligations = vector_of<approximate_obligation_evidence>(
      reader, limits.max_references, [&] {
        approximate_obligation_evidence value;
        value.kind = static_cast<approximate_obligation_kind>(reader.u8());
        value.vertices = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.triangles = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.exact_entities = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.passed = reader.boolean();
        value.measured_value = reader.rational();
        value.allowed_value = reader.rational();
        return value;
      });
  c.relaxed_relations = vector_of<approximate_relaxed_relation>(
      reader, limits.max_references, [&] {
        approximate_relaxed_relation value;
        value.kind = static_cast<approximate_relaxed_relation_kind>(reader.u8());
        value.exact_entity = reader.u64();
        value.axis = reader.u8();
        value.exact_value = reader.rational();
        value.emitted_value = reader.rational();
        value.absolute_deviation = reader.rational();
        value.allowed_deviation = reader.rational();
        if (reader.boolean())
          value.defining_relation = identifier<defining_relation_id>(reader);
        value.relation_kind = static_cast<defining_relation_kind>(reader.u8());
        value.relation_formula_version = reader.u16();
        value.relation_expected =
            static_cast<exact_sign>(static_cast<std::int8_t>(reader.u8()));
        value.exact_residual = reader.rational();
        value.emitted_residual = reader.rational();
        return value;
      });
  c.defining_relations = vector_of<approximate_defining_relation_evidence>(
      reader, limits.max_references, [&] {
        approximate_defining_relation_evidence value;
        value.vertex = identifier<selected_vertex_id>(reader);
        value.relation = identifier<defining_relation_id>(reader);
        value.kind = static_cast<defining_relation_kind>(reader.u8());
        value.formula_version = reader.u16();
        value.expected =
            static_cast<exact_sign>(static_cast<std::int8_t>(reader.u8()));
        value.exact_residual = reader.rational();
        value.emitted_residual = reader.rational();
        value.normalized_squared_residual_change = reader.rational();
        value.allowed_squared_residual_change = reader.rational();
        value.exact = reader.boolean();
        value.passed = reader.boolean();
        return value;
      });
  c.components = vector_of<approximate_constraint_component>(
      reader, limits.max_entities, [&] {
        approximate_constraint_component value;
        value.id = reader.u64();
        value.variables = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.variable_order = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.obligations = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.accepted_ranks = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.rejected_prefix_witnesses = vector_of<std::uint64_t>(
            reader, limits.max_references, [&] { return reader.u64(); });
        value.visited_nodes = reader.u64();
        value.complete_assignments = reader.u64();
        value.graph_digest = reader.digest_value();
        value.transcript_digest = reader.digest_value();
        return value;
      });
  c.search.candidate_generation_version = reader.u16();
  c.search.candidate_ulp_radius = reader.u32();
  c.search.candidate_cap = reader.u64();
  c.search.candidate_evaluation_limit = reader.u64();
  c.search.candidate_evaluations = reader.u64();
  c.search.search_node_limit = reader.u64();
  c.search.obligation_limit = reader.u64();
  c.search.triangle_pair_limit = reader.u64();
  c.search.predicate_check_limit = reader.u64();
  c.search.predicate_checks = reader.u64();
  c.search.verifier_work_limit = reader.u64();
  c.search.verifier_record_limit = reader.u64();
  c.search.verifier_byte_limit = reader.u64();
  c.search.generated_axis_candidates = reader.u64();
  c.search.generated_point_candidates = reader.u64();
  c.search.visited_nodes = reader.u64();
  c.search.complete_assignments = reader.u64();
  c.search.accepted_candidate_ranks = vector_of<std::uint64_t>(
      reader, limits.max_references, [&] { return reader.u64(); });
  c.search.candidate_domain_digest = reader.digest_value();
  c.search.transcript_digest = reader.digest_value();
  c.maximum_squared_vertex_displacement = reader.rational();
  for (auto &value : c.maximum_axis_displacements)
    value = reader.rational();
  c.maximum_support_plane_deviation = reader.rational();
  c.maximum_normalized_squared_support_plane_deviation = reader.rational();
  if (reader.remaining() != 0)
    throw std::runtime_error("certificate_trailing_bytes");
  c.canonical_bytes = bytes;
  c.certificate_digest = domain_digest(
      {{'Y', 'G', 'B', 'A', 'C', 'E', 'R', '1'}}, bytes);
  return c;
}

template <class T, class I>
std::vector<std::uint8_t> canonical_output(const boolean_success<T, I> &output) {
  canonical_encoder encoder;
  encoder.u64(output.mesh.vertices.size());
  for (const auto &point : output.mesh.vertices) {
    encoder.floating(point.x);
    encoder.floating(point.y);
    encoder.floating(point.z);
  }
  encoder.u64(output.mesh.faces.size());
  for (const auto &face : output.mesh.faces) {
    encoder.u64(face.size());
    for (const auto vertex : face)
      encoder.u64(static_cast<std::uint64_t>(vertex));
  }
  return encoder.bytes();
}

template <class T, class I>
boolean_success<T, I> decode_output(const std::vector<std::uint8_t> &bytes,
                                     const exact_stratified_boundary &boundary,
                                     const approximate_decode_limits &limits,
                                     approximate_verifier_budget &budget) {
  byte_reader reader(bytes, limits, &budget);
  boolean_success<T, I> output;
  output.selected_operation = boundary.selected_operation;
  output.selected_boundary_digest = boundary.selected_artifact_digest;
  const auto vertices = reader.count(limits.max_entities);
  output.mesh.vertices.reserve(static_cast<std::size_t>(vertices));
  for (std::uint64_t i = 0; i < vertices; ++i) {
    std::array<T, 3> coordinates;
    for (auto &coordinate : coordinates) {
      if constexpr (std::is_same<T, float>::value) {
        coordinate_bits<float> bits{reader.u32()};
        if (!is_finite_coordinate_bits(bits))
          throw std::runtime_error("nonfinite_output");
        coordinate = value_of_bits(bits);
      } else {
        coordinate_bits<double> bits{reader.u64()};
        if (!is_finite_coordinate_bits(bits))
          throw std::runtime_error("nonfinite_output");
        coordinate = value_of_bits(bits);
      }
    }
    output.mesh.vertices.push_back(
        {coordinates[0], coordinates[1], coordinates[2]});
  }
  const auto faces = reader.count(limits.max_entities);
  output.mesh.faces.reserve(static_cast<std::size_t>(faces));
  for (std::uint64_t face = 0; face < faces; ++face) {
    const auto size = reader.count(limits.max_references);
    std::vector<I> indices;
    indices.reserve(static_cast<std::size_t>(size));
    for (std::uint64_t i = 0; i < size; ++i) {
      const auto index = reader.u64();
      if (index >= vertices || index > std::numeric_limits<I>::max())
        throw std::runtime_error("output_index");
      indices.push_back(static_cast<I>(index));
    }
    output.mesh.faces.push_back(std::move(indices));
  }
  if (reader.remaining() != 0)
    throw std::runtime_error("output_trailing_bytes");
  output.canonical_output_digest = domain_digest(
      {{'Y', 'G', 'B', 'A', 'O', 'U', 'T', '1'}}, bytes);
  output.summary.vertices = vertices;
  output.summary.faces = faces;
  output.summary.face_indices = 3 * faces;
  output.summary.components = boundary.certificate.connected_components;
  output.summary.semantic_digest = output.canonical_output_digest;
  return output;
}

product_error serialization_error(product_error_code code, const char *key,
                                  const std::exception *exception = nullptr) {
  auto error = make_product_error(code, key);
  if (exception)
    error.detail = exception->what();
  return error;
}

} // namespace

template <class T, class I>
product_status_or<std::vector<std::uint8_t>>
encode_certified_approximate_embedding(
    const exact_result_handle &exact, const product_realization_policy &policy,
    const boolean_success<T, I> &output,
    const certified_approximate_certificate<T, I> &certificate) {
  auto verified = verify_certified_approximate_embedding(
      exact, policy, output, certificate);
  if (!verified.has_value())
    return verified.error();
  const auto output_bytes = canonical_output(output);
  std::uint64_t total_bytes = 12;
  auto add_bytes = [&](std::uint64_t count) {
    if (count > policy.search.max_verifier_bytes ||
        total_bytes > policy.search.max_verifier_bytes - count)
      return false;
    total_bytes += count;
    return true;
  };
  if (!add_bytes(8) || !add_bytes(exact->canonical_bytes.size()) ||
      !add_bytes(8) || !add_bytes(output_bytes.size()) || !add_bytes(8) ||
      !add_bytes(certificate.canonical_bytes.size()))
    return serialization_error(product_error_code::resource_limit,
                               "approximate_serialization.policy_byte_limit");
  canonical_encoder encoder;
  encoder.raw(reinterpret_cast<const std::uint8_t *>(envelope_tag.data()),
              envelope_tag.size());
  encoder.u16(certified_approximate_embedding_schema);
  encoder.byte(static_cast<std::uint8_t>(exact_result_coordinate_type<T>()));
  encoder.byte(static_cast<std::uint8_t>(exact_result_index_type<I>()));
  encoder.byte_string(exact->canonical_bytes);
  encoder.byte_string(output_bytes);
  encoder.byte_string(certificate.canonical_bytes);
  return encoder.bytes();
}

template <class T, class I>
product_status_or<bool> verify_serialized_certified_approximate_embedding(
    const std::vector<std::uint8_t> &bytes,
    const product_realization_policy &policy,
    const approximate_decode_limits &limits) noexcept {
  approximate_verifier_budget budget(policy.search);
  try {
    boolean_product_options options;
    options.result.representation =
        result_representation::certified_approximate_mesh;
    options.realization = policy;
    if (!validate_product_options(options).has_value())
      return serialization_error(product_error_code::approximation_policy_rejected,
                                 "approximate_serialization.policy");
    if (bytes.size() > limits.max_record_bytes)
      return serialization_error(product_error_code::resource_limit,
                                  "approximate_serialization.record_limit");
    auto effective = limits;
    byte_reader reader(bytes, effective, &budget);
    for (const auto expected : envelope_tag)
      if (reader.u8() != static_cast<std::uint8_t>(expected))
        return serialization_error(
            product_error_code::exact_result_serialization_error,
            "approximate_serialization.magic");
    if (reader.u16() != certified_approximate_embedding_schema ||
        reader.u8() !=
            static_cast<std::uint8_t>(exact_result_coordinate_type<T>()) ||
        reader.u8() != static_cast<std::uint8_t>(exact_result_index_type<I>()))
      return serialization_error(product_error_code::stale_binding,
                                 "approximate_serialization.schema");
    const auto exact_bytes = reader.bytes(effective.max_exact_result_bytes);
    const auto output_bytes = reader.bytes(effective.max_output_bytes);
    const auto certificate_bytes = reader.bytes(effective.max_certificate_bytes);
    if (reader.remaining() != 0)
      return serialization_error(
          product_error_code::exact_result_serialization_error,
          "approximate_serialization.trailing_bytes");
    exact_result_decode_limits exact_limits;
    exact_limits.max_record_bytes = effective.max_exact_result_bytes;
    exact_limits.max_entities = effective.max_entities;
    exact_limits.max_references = effective.max_references;
    exact_limits.max_exact_hex_bytes = effective.max_exact_integer_bytes;
    auto boundary = decode_exact_stratified_boundary(exact_bytes, exact_limits);
    if (!boundary.has_value())
      return boundary.error();
    auto exact = make_exact_result_handle(boundary.value());
    if (!exact.has_value())
      return exact.error();
    auto output = decode_output<T, I>(output_bytes, *boundary.value(), effective,
                                      budget);
    auto certificate =
        decode_certificate<T, I>(certificate_bytes, policy, effective, budget);
    auto verified = detail::verify_certified_approximate_embedding_with_budget(
        exact.value(), policy, output, certificate, budget);
    if (!verified.has_value())
      return verified.error();
    canonical_encoder canonical;
    canonical.raw(reinterpret_cast<const std::uint8_t *>(envelope_tag.data()),
                  envelope_tag.size());
    canonical.u16(certified_approximate_embedding_schema);
    canonical.byte(static_cast<std::uint8_t>(exact_result_coordinate_type<T>()));
    canonical.byte(static_cast<std::uint8_t>(exact_result_index_type<I>()));
    canonical.byte_string(exact.value()->canonical_bytes);
    canonical.byte_string(canonical_output(output));
    canonical.byte_string(certificate.canonical_bytes);
    if (canonical.bytes() != bytes)
      return serialization_error(
          product_error_code::exact_result_serialization_error,
          "approximate_serialization.noncanonical");
    return true;
  } catch (const std::length_error &error) {
    return serialization_error(product_error_code::resource_limit,
                                budget.failed() ? budget.failure_key
                                                : "approximate_serialization.limit",
                                &error);
  } catch (const std::bad_alloc &) {
    return serialization_error(product_error_code::resource_limit,
                               "approximate_serialization.allocation");
  } catch (const std::exception &error) {
    return serialization_error(product_error_code::exact_result_serialization_error,
                               "approximate_serialization.decode", &error);
  } catch (...) {
    return serialization_error(product_error_code::exact_result_serialization_error,
                               "approximate_serialization.exception");
  }
}

#define YGOR_APPROXIMATE_SERIALIZATION_DEFINE(T, I)                           \
  template product_status_or<std::vector<std::uint8_t>>                       \
  encode_certified_approximate_embedding(                                     \
      const exact_result_handle &, const product_realization_policy &,        \
      const boolean_success<T, I> &,                                          \
      const certified_approximate_certificate<T, I> &);                       \
  template product_status_or<bool>                                            \
  verify_serialized_certified_approximate_embedding<T, I>(                    \
      const std::vector<std::uint8_t> &, const product_realization_policy &,  \
      const approximate_decode_limits &) noexcept
YGOR_APPROXIMATE_SERIALIZATION_DEFINE(float, std::uint32_t);
YGOR_APPROXIMATE_SERIALIZATION_DEFINE(float, std::uint64_t);
YGOR_APPROXIMATE_SERIALIZATION_DEFINE(double, std::uint32_t);
YGOR_APPROXIMATE_SERIALIZATION_DEFINE(double, std::uint64_t);
#undef YGOR_APPROXIMATE_SERIALIZATION_DEFINE

} // namespace mesh_boolean
} // namespace ygor
