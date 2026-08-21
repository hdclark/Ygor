#include "YgorMeshesBooleanQualificationGeneration.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <initializer_list>
#include <limits>
#include <map>
#include <new>
#include <stdexcept>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

using coordinate = qualification_recipe_coordinate;
using vertex = qualification_recipe_vertex;
using mesh = qualification_recipe_mesh;
using category = qualification_geometry_category;

constexpr std::array<char, 8> descriptor_tag{{'Y', 'G', 'B', 'Q', 'G', 'D', '0', '1'}};
constexpr std::array<char, 8> case_tag{{'Y', 'G', 'B', 'Q', 'G', 'C', '0', '1'}};
constexpr std::array<char, 8> chain_tag{{'Y', 'G', 'B', 'Q', 'G', 'H', '0', '1'}};
constexpr std::array<char, 8> minimization_tag{{'Y', 'G', 'B', 'Q', 'G', 'M', '0', '1'}};
constexpr std::array<char, 8> promotion_tag{{'Y', 'G', 'B', 'Q', 'G', 'P', '0', '1'}};
constexpr std::array<char, 8> provenance_tag{{'Y', 'G', 'B', 'Q', 'G', 'V', '0', '1'}};

unsigned ordinal(qualification_generator_family v) noexcept {
  return static_cast<unsigned>(v);
}
unsigned ordinal(qualification_geometry_category v) noexcept {
  return static_cast<unsigned>(v);
}
unsigned ordinal(qualification_outcome v) noexcept {
  return static_cast<unsigned>(v);
}
unsigned ordinal(product_error_code v) noexcept {
  return static_cast<unsigned>(v);
}

qualification_construction_kind expected_construction(
    qualification_generator_family family) {
  switch (family) {
  case qualification_generator_family::exact_halfspace_skew_convex:
    return qualification_construction_kind::exact_halfspace_polytope;
  case qualification_generator_family::exact_profile_extrusion_concave:
    return qualification_construction_kind::profile_extrusion;
  case qualification_generator_family::exact_coplanar_overlay:
    return qualification_construction_kind::coplanar_overlay;
  case qualification_generator_family::exact_nested_shell_cavity:
    return qualification_construction_kind::nested_shell_cavity;
  case qualification_generator_family::exact_feature_alignment_contact:
    return qualification_construction_kind::feature_alignment;
  case qualification_generator_family::exact_subdivision_refinement:
    return qualification_construction_kind::exact_subdivision;
  case qualification_generator_family::exact_representable_scale_bits:
    return qualification_construction_kind::representable_transform;
  case qualification_generator_family::exact_nondyadic_intersection:
    return qualification_construction_kind::nondyadic_intersection_inputs;
  case qualification_generator_family::exact_thin_sliver_dense:
    return qualification_construction_kind::thin_or_dense_feature;
  case qualification_generator_family::exact_capacity_replay_boundary:
    return qualification_construction_kind::capacity_or_replay_boundary;
  case qualification_generator_family::cadlike_profile_extruded_part:
  case qualification_generator_family::cadlike_multibody_cavity:
  case qualification_generator_family::cadlike_thin_feature:
  case qualification_generator_family::cadlike_dense_tessellation:
  case qualification_generator_family::cadlike_attribute_seam_conflict:
    return qualification_construction_kind::cad_like_model;
  case qualification_generator_family::cadlike_controlled_defect:
    return qualification_construction_kind::controlled_invalid_preparation;
  case qualification_generator_family::count:
    break;
  }
  throw std::logic_error("unknown qualification generator family");
}

bool text(const std::string &s) noexcept {
  return !s.empty() && s.size() <= 1024U &&
         std::find(s.begin(), s.end(), '\0') == s.end();
}

unsigned hex_digit(char c) {
  if ('0' <= c && c <= '9')
    return static_cast<unsigned>(c - '0');
  if ('a' <= c && c <= 'f')
    return 10U + static_cast<unsigned>(c - 'a');
  throw std::logic_error("noncanonical generator digest literal");
}

digest digest_literal(const char *text_value) {
  digest result;
  for (std::size_t i = 0; i != result.bytes.size(); ++i)
    result.bytes[i] = static_cast<std::uint8_t>(
        (hex_digit(text_value[2 * i]) << 4U) |
        hex_digit(text_value[2 * i + 1]));
  return result;
}

std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

exact_rational rational(std::int64_t numerator, std::uint32_t denominator_power = 0) {
  if (!denominator_power)
    return exact_rational(numerator);
  return exact_rational(big_int(numerator),
                        big_uint(1).shifted_left(denominator_power));
}

exact_rational power_of_two(std::int32_t exponent) {
  if (exponent >= 0)
    return exact_rational(
        big_int(integer_sign::positive,
                big_uint(1).shifted_left(static_cast<std::size_t>(exponent))),
        big_uint(1));
  return exact_rational(
      big_int(1),
      big_uint(1).shifted_left(static_cast<std::size_t>(-exponent)));
}

coordinate c(std::int64_t numerator, std::uint32_t denominator_power = 0,
             bool negative_zero = false) {
  coordinate result;
  result.value = rational(numerator, denominator_power);
  result.negative_zero = negative_zero;
  return result;
}

coordinate c(exact_rational value) {
  coordinate result;
  result.value = std::move(value);
  return result;
}

vertex v(coordinate x, coordinate y, coordinate z) {
  return {{{std::move(x), std::move(y), std::move(z)}}};
}

vertex vi(std::int64_t x, std::int64_t y, std::int64_t z) {
  return v(c(x), c(y), c(z));
}

exact_rational component(const vertex &p, std::size_t axis) {
  return p.coordinate[axis].value;
}

mesh make_box(exact_rational x0, exact_rational x1, exact_rational y0,
              exact_rational y1, exact_rational z0, exact_rational z1) {
  mesh result;
  result.vertices = {
      v(c(x0), c(y0), c(z0)), v(c(x1), c(y0), c(z0)),
      v(c(x1), c(y1), c(z0)), v(c(x0), c(y1), c(z0)),
      v(c(x0), c(y0), c(z1)), v(c(x1), c(y0), c(z1)),
      v(c(x1), c(y1), c(z1)), v(c(x0), c(y1), c(z1))};
  result.faces = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                  {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
  return result;
}

mesh make_box(std::int64_t lo, std::int64_t hi) {
  return make_box(exact_rational(lo), exact_rational(hi), exact_rational(lo),
                  exact_rational(hi), exact_rational(lo), exact_rational(hi));
}

void append_mesh(mesh &destination, const mesh &source, bool reverse_faces = false) {
  const std::uint64_t offset = destination.vertices.size();
  destination.vertices.insert(destination.vertices.end(), source.vertices.begin(),
                              source.vertices.end());
  for (auto face : source.faces) {
    for (auto &index : face)
      index += offset;
    if (reverse_faces)
      std::reverse(face.begin(), face.end());
    destination.faces.push_back(std::move(face));
  }
}

mesh make_profile_prism(const std::vector<std::array<exact_rational, 2>> &profile,
                        exact_rational z0, exact_rational z1) {
  mesh result;
  const std::uint64_t count = profile.size();
  for (const auto &p : profile)
    result.vertices.push_back(v(c(p[0]), c(p[1]), c(z0)));
  for (const auto &p : profile)
    result.vertices.push_back(v(c(p[0]), c(p[1]), c(z1)));
  std::vector<std::uint64_t> bottom, top;
  for (std::uint64_t i = 0; i != count; ++i) {
    bottom.push_back(count - 1 - i);
    top.push_back(count + i);
  }
  result.faces.push_back(std::move(bottom));
  result.faces.push_back(std::move(top));
  for (std::uint64_t i = 0; i != count; ++i) {
    const auto next = (i + 1) % count;
    result.faces.push_back({i, next, count + next, count + i});
  }
  return result;
}

mesh make_l_prism(exact_rational dx, exact_rational dy, exact_rational z0,
                  exact_rational z1) {
  std::vector<std::array<exact_rational, 2>> profile{
      {{exact_rational(0) + dx, exact_rational(0) + dy}},
      {{exact_rational(3) + dx, exact_rational(0) + dy}},
      {{exact_rational(3) + dx, exact_rational(1) + dy}},
      {{exact_rational(1) + dx, exact_rational(1) + dy}},
      {{exact_rational(1) + dx, exact_rational(3) + dy}},
      {{exact_rational(0) + dx, exact_rational(3) + dy}}};
  return make_profile_prism(profile, std::move(z0), std::move(z1));
}

mesh make_octagonal_prism(exact_rational z0, exact_rational z1) {
  std::vector<std::array<exact_rational, 2>> profile{
      {{exact_rational(-2), exact_rational(-1)}},
      {{exact_rational(-1), exact_rational(-2)}},
      {{exact_rational(1), exact_rational(-2)}},
      {{exact_rational(2), exact_rational(-1)}},
      {{exact_rational(2), exact_rational(1)}},
      {{exact_rational(1), exact_rational(2)}},
      {{exact_rational(-1), exact_rational(2)}},
      {{exact_rational(-2), exact_rational(1)}}};
  return make_profile_prism(profile, std::move(z0), std::move(z1));
}

mesh triangulate_convex_mesh(const mesh &source) {
  mesh result;
  result.vertices = source.vertices;
  for (const auto &face : source.faces) {
    if (face.size() <= 3) {
      result.faces.push_back(face);
      continue;
    }
    for (std::size_t offset = 1; offset + 1 < face.size(); ++offset)
      result.faces.push_back({face[0], face[offset], face[offset + 1]});
  }
  return result;
}

mesh refine_triangular_mesh(const mesh &source) {
  mesh result;
  result.vertices = source.vertices;
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t> midpoints;
  auto midpoint = [&](std::uint64_t first, std::uint64_t second) {
    const std::pair<std::uint64_t, std::uint64_t> key{
        std::min(first, second), std::max(first, second)};
    const auto found = midpoints.find(key);
    if (found != midpoints.end())
      return found->second;
    vertex point;
    for (std::size_t axis = 0; axis != 3; ++axis) {
      point.coordinate[axis].value =
          (source.vertices[first].coordinate[axis].value +
           source.vertices[second].coordinate[axis].value) / exact_rational(2);
      point.coordinate[axis].negative_zero = false;
    }
    const auto index = static_cast<std::uint64_t>(result.vertices.size());
    result.vertices.push_back(std::move(point));
    midpoints.emplace(key, index);
    return index;
  };
  for (const auto &face : source.faces) {
    if (face.size() != 3)
      throw std::logic_error("triangle refinement requires triangulated input");
    const auto a = face[0], b = face[1], c_value = face[2];
    const auto ab = midpoint(a, b);
    const auto bc = midpoint(b, c_value);
    const auto ca = midpoint(c_value, a);
    result.faces.push_back({a, ab, ca});
    result.faces.push_back({b, bc, ab});
    result.faces.push_back({c_value, ca, bc});
    result.faces.push_back({ab, bc, ca});
  }
  return result;
}

mesh make_triangle_prism() {
  std::vector<std::array<exact_rational, 2>> profile{
      {{exact_rational(0), exact_rational(0)}},
      {{exact_rational(1), exact_rational(3)}},
      {{exact_rational(0), exact_rational(3)}}};
  return make_profile_prism(profile, exact_rational(0), exact_rational(2));
}

mesh transformed_parallelotope(const std::array<std::int64_t, 3> &origin,
                               const std::array<std::int64_t, 3> &u,
                               const std::array<std::int64_t, 3> &w,
                               const std::array<std::int64_t, 3> &q) {
  mesh result;
  auto point = [&](int iu, int iw, int iq) {
    return vi(origin[0] + iu * u[0] + iw * w[0] + iq * q[0],
              origin[1] + iu * u[1] + iw * w[1] + iq * q[1],
              origin[2] + iu * u[2] + iw * w[2] + iq * q[2]);
  };
  result.vertices = {point(0, 0, 0), point(1, 0, 0), point(1, 1, 0),
                     point(0, 1, 0), point(0, 0, 1), point(1, 0, 1),
                     point(1, 1, 1), point(0, 1, 1)};
  result.faces = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                  {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
  return result;
}

std::array<std::int64_t, 3>
cross(const std::array<std::int64_t, 3> &a,
      const std::array<std::int64_t, 3> &b) {
  return {{a[1] * b[2] - a[2] * b[1],
           a[2] * b[0] - a[0] * b[2],
           a[0] * b[1] - a[1] * b[0]}};
}

std::int64_t dot(const std::array<std::int64_t, 3> &a,
                 const std::array<std::int64_t, 3> &b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

std::vector<qualification_exact_halfspace>
parallelotope_halfspaces(const std::array<std::int64_t, 3> &origin,
                         const std::array<std::int64_t, 3> &u,
                         const std::array<std::int64_t, 3> &w,
                         const std::array<std::int64_t, 3> &q) {
  const std::array<std::array<std::int64_t, 3>, 3> rows{
      {cross(w, q), cross(q, u), cross(u, w)}};
  const auto determinant = dot(rows[0], u);
  if (determinant <= 0)
    throw std::logic_error("qualification parallelotope determinant");
  std::vector<qualification_exact_halfspace> result;
  for (const auto &row : rows) {
    const auto at_origin = dot(row, origin);
    result.push_back({exact_rational(-row[0]), exact_rational(-row[1]),
                      exact_rational(-row[2]), exact_rational(-at_origin)});
    result.push_back({exact_rational(row[0]), exact_rational(row[1]),
                      exact_rational(row[2]),
                      exact_rational(determinant + at_origin)});
  }
  return result;
}

void translate(mesh &value, exact_rational dx, exact_rational dy,
               exact_rational dz) {
  for (auto &point : value.vertices) {
    point.coordinate[0].value = point.coordinate[0].value + dx;
    point.coordinate[1].value = point.coordinate[1].value + dy;
    point.coordinate[2].value = point.coordinate[2].value + dz;
  }
}

void scale_power_of_two(mesh &value, std::int32_t exponent) {
  const exact_rational factor = power_of_two(exponent);
  for (auto &point : value.vertices)
    for (auto &axis : point.coordinate)
      axis.value = axis.value * factor;
}

mesh triangulated(mesh value) {
  value = qualification_generation_detail::subdivide_qualification_mesh(value);
  return value;
}

void apply_controlled_defect(mesh &value, qualification_defect_label label) {
  switch (label) {
  case qualification_defect_label::none:
    return;
  case qualification_defect_label::duplicate_vertex_use: {
    value.vertices.push_back(value.vertices.front());
    const auto replacement = value.vertices.size() - 1;
    value.faces.front().front() = replacement;
    return;
  }
  case qualification_defect_label::duplicate_facet:
    value.faces.push_back(value.faces.front());
    return;
  case qualification_defect_label::inconsistent_orientation:
    std::reverse(value.faces.front().begin(), value.faces.front().end());
    return;
  case qualification_defect_label::open_crack:
    value.faces.pop_back();
    return;
  case qualification_defect_label::nonplanar_facet:
    value.vertices[value.faces[1][0]].coordinate[2].value =
        value.vertices[value.faces[1][0]].coordinate[2].value + rational(1, 3);
    return;
  case qualification_defect_label::overlapping_shells: {
    auto second = value;
    translate(second, rational(1, 2), rational(1, 2), rational(1, 2));
    append_mesh(value, second);
    return;
  }
  case qualification_defect_label::sliver_feature:
    value = make_box(exact_rational(0), exact_rational(4), exact_rational(0),
                     rational(1, 20), exact_rational(0), exact_rational(2));
    return;
  case qualification_defect_label::self_intersection: {
    auto second = make_box(exact_rational(1), exact_rational(3),
                           exact_rational(-1), exact_rational(1),
                           exact_rational(1), exact_rational(3));
    append_mesh(value, second);
    return;
  }
  case qualification_defect_label::count:
    break;
  }
  throw std::logic_error("unknown qualification defect");
}

void encode_coordinate(canonical_encoder &encoder,
                       const qualification_recipe_coordinate &value) {
  value.value.encode(encoder);
  encoder.boolean(value.negative_zero);
}

void encode_mesh(canonical_encoder &encoder, const mesh &value) {
  encoder.u64(value.vertices.size());
  for (const auto &point : value.vertices)
    for (const auto &axis : point.coordinate)
      encode_coordinate(encoder, axis);
  encoder.u64(value.faces.size());
  for (const auto &face : value.faces) {
    encoder.u64(face.size());
    for (const auto index : face)
      encoder.u64(index);
  }
}

void encode_halfspace(canonical_encoder &encoder,
                      const qualification_exact_halfspace &value) {
  value.a.encode(encoder);
  value.b.encode(encoder);
  value.c.encode(encoder);
  value.d.encode(encoder);
}

void encode_case_without_digest(canonical_encoder &encoder,
                                const qualification_case_recipe &value) {
  encoder.u16(value.schema);
  encoder.string(value.recipe_identifier);
  encoder.string(value.recipe_version);
  encoder.raw(value.recipe_digest.bytes.data(), value.recipe_digest.bytes.size());
  encoder.byte(static_cast<std::uint8_t>(value.family));
  encoder.byte(static_cast<std::uint8_t>(value.construction));
  encoder.u64(value.ordinal);
  encoder.u64(value.deterministic_seed);
  encoder.byte(static_cast<std::uint8_t>(value.derivation));
  encoder.boolean(static_cast<bool>(value.valid_mutation));
  if (value.valid_mutation)
    encoder.byte(static_cast<std::uint8_t>(*value.valid_mutation));
  encoder.raw(value.parent_case_digest.bytes.data(),
              value.parent_case_digest.bytes.size());
  encoder.u64(value.retained_parameters.size());
  for (const auto parameter : value.retained_parameters)
    encoder.signed_magnitude(parameter);
  encoder.u64(value.geometry_categories.size());
  for (const auto item : value.geometry_categories)
    encoder.byte(static_cast<std::uint8_t>(item));
  encode_mesh(encoder, value.operand_a);
  encode_mesh(encoder, value.operand_b);
  encoder.u64(value.operand_a_halfspaces.size());
  for (const auto &halfspace : value.operand_a_halfspaces)
    encode_halfspace(encoder, halfspace);
  encoder.u64(value.operand_b_halfspaces.size());
  for (const auto &halfspace : value.operand_b_halfspaces)
    encode_halfspace(encoder, halfspace);
  encoder.byte(static_cast<std::uint8_t>(value.expectation.relation));
  encoder.byte(static_cast<std::uint8_t>(value.expectation.defect));
  encoder.u64(value.expectation.allowed_outcomes.size());
  for (const auto outcome : value.expectation.allowed_outcomes)
    encoder.byte(static_cast<std::uint8_t>(outcome));
  encoder.u64(value.expectation.allowed_failure_codes.size());
  for (const auto code : value.expectation.allowed_failure_codes)
    encoder.u16(static_cast<std::uint16_t>(code));
  encoder.boolean(value.expectation.strict_operand_a_expected_valid);
  encoder.boolean(value.expectation.strict_operand_b_expected_valid);
  encoder.boolean(value.expectation.normalization_report_required);
}

void encode_step(canonical_encoder &encoder,
                 const qualification_chain_step_recipe &step) {
  encoder.byte(static_cast<std::uint8_t>(step.selected_operation));
  encoder.byte(static_cast<std::uint8_t>(step.operand_order));
  encoder.byte(static_cast<std::uint8_t>(step.preparation));
  encoder.byte(static_cast<std::uint8_t>(step.requested_result));
  encode_case_without_digest(encoder, step.rhs_case);
  encoder.raw(step.rhs_case.case_digest.bytes.data(),
              step.rhs_case.case_digest.bytes.size());
  encoder.boolean(step.rhs_uses_operand_b);
  encoder.boolean(step.subdivide_accumulator_before_step);
  encoder.boolean(step.continue_after_expected_failure);
  encoder.boolean(step.continue_with_prior_mesh_when_unrealized);
  encoder.u64(step.expected_failure_codes.size());
  for (const auto code : step.expected_failure_codes)
    encoder.u16(static_cast<std::uint16_t>(code));
}

void encode_chain_without_digest(canonical_encoder &encoder,
                                 const qualification_operation_chain_recipe &value) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.version);
  encoder.raw(value.definition_digest.bytes.data(),
              value.definition_digest.bytes.size());
  encoder.u64(value.ordinal);
  encode_case_without_digest(encoder, value.initial_case);
  encoder.raw(value.initial_case.case_digest.bytes.data(),
              value.initial_case.case_digest.bytes.size());
  encoder.boolean(value.initial_uses_operand_b);
  encoder.u64(value.steps.size());
  for (const auto &step : value.steps)
    encode_step(encoder, step);
}

bool halfspace_system_binds_mesh(
    const mesh &value,
    const std::vector<qualification_exact_halfspace> &halfspaces) {
  if (halfspaces.empty())
    return false;
  for (const auto &halfspace : halfspaces) {
    std::size_t active = 0;
    for (const auto &point : value.vertices) {
      const exact_rational lhs =
          halfspace.a * component(point, 0) +
          halfspace.b * component(point, 1) +
          halfspace.c * component(point, 2);
      const int relation = lhs.compare(halfspace.d);
      if (relation > 0)
        return false;
      if (relation == 0)
        ++active;
    }
    if (active < 3)
      return false;
  }
  return true;
}

bool validate_mesh_contract(const mesh &value) {
  if (value.vertices.size() < 4 || value.faces.size() < 4)
    return false;
  for (const auto &point : value.vertices)
    for (const auto &axis : point.coordinate)
      if (axis.negative_zero && !axis.value.is_zero())
        return false;
  for (const auto &face : value.faces) {
    if (face.size() < 3)
      return false;
    for (std::size_t i = 0; i != face.size(); ++i) {
      if (face[i] >= value.vertices.size() ||
          face[i] == face[(i + 1) % face.size()])
        return false;
    }
  }
  return true;
}

qualification_case_expectation valid_expectation(
    qualification_known_relation relation) {
  qualification_case_expectation result;
  result.relation = relation;
  result.allowed_outcomes = {
      qualification_outcome::verified_exact_success,
      qualification_outcome::verified_certified_approximate_success,
      qualification_outcome::expected_typed_failure};
  result.allowed_failure_codes = {
      product_error_code::resource_limit,
      product_error_code::index_overflow,
      product_error_code::result_topology_not_supported,
      product_error_code::output_not_representable};
  return result;
}

qualification_case_expectation invalid_expectation(
    qualification_defect_label defect) {
  qualification_case_expectation result;
  result.relation = qualification_known_relation::controlled_invalid_operand;
  result.defect = defect;
  result.allowed_outcomes = {qualification_outcome::expected_typed_failure};
  result.allowed_failure_codes = {product_error_code::input_contract_error,
                                  product_error_code::normalization_required,
                                  product_error_code::normalization_failed};
  result.strict_operand_b_expected_valid = false;
  result.normalization_report_required = true;
  if (defect == qualification_defect_label::sliver_feature)
    result.strict_operand_b_expected_valid = true;
  return result;
}

struct descriptor_literal {
  qualification_generator_family family;
  const char *identifier;
  const char *version;
  const char *digest_hex;
  std::uint64_t count;
  qualification_corpus_record_kind kind;
  std::initializer_list<category> categories;
};

const descriptor_literal descriptor_literals[] = {
    {qualification_generator_family::exact_halfspace_skew_convex,
     "exact-halfspace-skew-convex-v1", "1",
     "78830142ccc1dfb93223647168d7e7d8", 1400,
     qualification_corpus_record_kind::generated_pair_family,
     {category::non_box_intersection, category::rotated_or_skewed_convex}},
    {qualification_generator_family::exact_profile_extrusion_concave,
     "exact-profile-extrusion-concave-v1", "1",
     "3149a7647e77cadf85fec3869a8b21c4", 1200,
     qualification_corpus_record_kind::generated_pair_family,
     {category::non_box_intersection, category::concave_or_reentrant}},
    {qualification_generator_family::exact_coplanar_overlay,
     "exact-coplanar-overlay-v1", "1",
     "d6a0ad9016a0d844a7feef836679dea4", 1100,
     qualification_corpus_record_kind::generated_pair_family,
     {category::coplanar_overlay, category::non_box_intersection}},
    {qualification_generator_family::exact_nested_shell_cavity,
     "exact-nested-shell-cavity-v1", "1",
     "8e4d6aeddc51371e4363f4f6b7381c34", 1000,
     qualification_corpus_record_kind::generated_pair_family,
     {category::nested_shells_or_cavities, category::disconnected_components}},
    {qualification_generator_family::exact_feature_alignment_contact,
     "exact-feature-alignment-contact-v1", "1",
     "8a3bc745ef382b16135e4e88ffbd6391", 1100,
     qualification_corpus_record_kind::generated_pair_family,
     {category::high_valence_contact, category::stratified_non_manifold}},
    {qualification_generator_family::exact_subdivision_refinement,
     "exact-subdivision-refinement-v1", "1",
     "08edad523d5fde8552920539c224731f", 1000,
     qualification_corpus_record_kind::generated_pair_family,
     {category::alternate_subdivision, category::non_box_intersection}},
    {qualification_generator_family::exact_representable_scale_bits,
     "exact-representable-scale-bits-v1", "1",
     "e928e18eb26b0ff5a109685974f4cb5e", 1000,
     qualification_corpus_record_kind::generated_pair_family,
     {category::scale_extremes, category::floating_point_edge_cases}},
    {qualification_generator_family::exact_nondyadic_intersection,
     "exact-nondyadic-intersection-v1", "1",
     "ec1389ba5785ef23f1066c047bb61849", 1000,
     qualification_corpus_record_kind::generated_pair_family,
     {category::non_dyadic_intersection, category::non_box_intersection}},
    {qualification_generator_family::exact_thin_sliver_dense,
     "exact-thin-sliver-dense-v1", "1",
     "11eae083454697569694bcc7a8350f55", 900,
     qualification_corpus_record_kind::generated_pair_family,
     {category::thin_sliver_or_dense, category::concave_or_reentrant}},
    {qualification_generator_family::exact_capacity_replay_boundary,
     "exact-capacity-replay-boundary-v1", "1",
     "06656e582e690e1091abe157c504fe53", 800,
     qualification_corpus_record_kind::generated_pair_family,
     {category::index_or_resource_boundary, category::serialization_or_replay}},
    {qualification_generator_family::cadlike_profile_extruded_part,
     "cadlike-profile-extruded-part-v1", "1",
     "174e8a446c3b174f5177157fe8314d28", 300,
     qualification_corpus_record_kind::cad_like_pair_family,
     {category::non_box_intersection, category::concave_or_reentrant}},
    {qualification_generator_family::cadlike_multibody_cavity,
     "cadlike-multibody-cavity-v1", "1",
     "f765134caebfab38ab52480af27d0c6a", 250,
     qualification_corpus_record_kind::cad_like_pair_family,
     {category::disconnected_components, category::nested_shells_or_cavities}},
    {qualification_generator_family::cadlike_thin_feature,
     "cadlike-thin-feature-v1", "1",
     "77efe00de7026e051cde922261d4ca5f", 200,
     qualification_corpus_record_kind::cad_like_pair_family,
     {category::thin_sliver_or_dense, category::scale_extremes}},
    {qualification_generator_family::cadlike_dense_tessellation,
     "cadlike-dense-tessellation-v1", "1",
     "130464771bcadc47438af9dcd52dea36", 150,
     qualification_corpus_record_kind::cad_like_pair_family,
     {category::alternate_subdivision, category::high_valence_contact}},
    {qualification_generator_family::cadlike_attribute_seam_conflict,
     "cadlike-attribute-seam-conflict-v1", "1",
     "e129028a79dbb791fb7f9bd2a3fba246", 100,
     qualification_corpus_record_kind::cad_like_pair_family,
     {category::attribute_or_provenance_conflict, category::coplanar_overlay}},
    {qualification_generator_family::cadlike_controlled_defect,
     "cadlike-controlled-defect-v1", "1",
     "e6ae7e6b9a6cb659ade44995dcd7f191", 100,
     qualification_corpus_record_kind::cad_like_pair_family,
     {category::normalization_defect, category::floating_point_edge_cases}}};

qualification_generator_descriptor make_descriptor(const descriptor_literal &literal) {
  qualification_generator_descriptor result;
  result.family = literal.family;
  result.identifier = literal.identifier;
  result.version = literal.version;
  result.recipe_digest = digest_literal(literal.digest_hex);
  result.case_count = literal.count;
  result.corpus_kind = literal.kind;
  result.geometry_categories.assign(literal.categories.begin(),
                                    literal.categories.end());
  std::sort(result.geometry_categories.begin(), result.geometry_categories.end(),
            [](category a, category b) { return ordinal(a) < ordinal(b); });
  canonical_encoder encoder;
  encoder.u16(result.schema);
  encoder.byte(static_cast<std::uint8_t>(result.family));
  encoder.string(result.identifier);
  encoder.string(result.version);
  encoder.raw(result.recipe_digest.bytes.data(), result.recipe_digest.bytes.size());
  encoder.u64(result.first_case_ordinal);
  encoder.u64(result.case_count);
  encoder.byte(static_cast<std::uint8_t>(result.corpus_kind));
  encoder.u32(qualification_generator_algorithm_version);
  encoder.u64(result.geometry_categories.size());
  for (const auto item : result.geometry_categories)
    encoder.byte(static_cast<std::uint8_t>(item));
  result.implementation_digest = domain_digest(descriptor_tag, encoder.bytes());
  return result;
}

qualification_case_recipe generate_case(
    const qualification_generator_descriptor &descriptor, std::uint64_t ordinal_value) {
  qualification_case_recipe result;
  result.recipe_identifier = descriptor.identifier;
  result.recipe_version = descriptor.version;
  result.recipe_digest = descriptor.recipe_digest;
  result.family = descriptor.family;
  result.ordinal = ordinal_value;
  result.deterministic_seed =
      splitmix64((static_cast<std::uint64_t>(ordinal(descriptor.family)) << 56U) ^
                 ordinal_value);
  result.geometry_categories = descriptor.geometry_categories;
  const auto phase = static_cast<unsigned>(result.deterministic_seed % 8U);
  const auto offset = static_cast<std::int64_t>(phase % 5U) - 2;
  result.retained_parameters = {static_cast<std::int64_t>(phase), offset};

  switch (descriptor.family) {
  case qualification_generator_family::exact_halfspace_skew_convex: {
    result.construction = qualification_construction_kind::exact_halfspace_polytope;
    const std::array<std::int64_t, 3> ao{{0, 0, 0}}, au{{2, 0, 0}},
        av{{1, 2, 0}}, aw{{0, 1, 2}};
    const std::array<std::int64_t, 3> bo{{1 + offset / 2, 0, 1}},
        bu{{2, 1, 0}}, bv{{0, 2, 1}}, bw{{1, 0, 2}};
    result.operand_a = transformed_parallelotope(ao, au, av, aw);
    result.operand_b = transformed_parallelotope(bo, bu, bv, bw);
    result.operand_a_halfspaces = parallelotope_halfspaces(ao, au, av, aw);
    result.operand_b_halfspaces = parallelotope_halfspaces(bo, bu, bv, bw);
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_profile_extrusion_concave:
  case qualification_generator_family::cadlike_profile_extruded_part: {
    result.construction = descriptor.family ==
                                  qualification_generator_family::cadlike_profile_extruded_part
                              ? qualification_construction_kind::cad_like_model
                              : qualification_construction_kind::profile_extrusion;
    result.operand_a = make_l_prism(exact_rational(0), exact_rational(0),
                                    exact_rational(0), exact_rational(2));
    result.operand_b = make_l_prism(rational(1, 1), rational(1, 1),
                                    exact_rational(1), exact_rational(3));
    if (descriptor.family ==
        qualification_generator_family::cadlike_profile_extruded_part)
      result.operand_a = triangulated(result.operand_a);
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_coplanar_overlay:
  case qualification_generator_family::cadlike_attribute_seam_conflict: {
    result.construction = descriptor.family ==
                                  qualification_generator_family::cadlike_attribute_seam_conflict
                              ? qualification_construction_kind::cad_like_model
                              : qualification_construction_kind::coplanar_overlay;
    result.operand_a = make_box(exact_rational(0), exact_rational(3),
                                exact_rational(0), exact_rational(2),
                                exact_rational(0), exact_rational(2));
    result.operand_b = make_box(exact_rational(1), exact_rational(4),
                                exact_rational(0), exact_rational(2),
                                exact_rational(0), exact_rational(2));
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_nested_shell_cavity:
  case qualification_generator_family::cadlike_multibody_cavity: {
    result.construction = descriptor.family ==
                                  qualification_generator_family::cadlike_multibody_cavity
                              ? qualification_construction_kind::cad_like_model
                              : qualification_construction_kind::nested_shell_cavity;
    result.operand_a = make_box(0, 6);
    auto cavity = make_box(2, 4);
    append_mesh(result.operand_a, cavity, true);
    if (descriptor.family ==
        qualification_generator_family::cadlike_multibody_cavity) {
      auto component = make_box(8, 10);
      append_mesh(result.operand_a, component);
    }
    result.operand_b = make_box(1, 3);
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_feature_alignment_contact: {
    result.construction = qualification_construction_kind::feature_alignment;
    result.operand_a = make_box(0, 2);
    if (phase % 3U == 0)
      result.operand_b = make_box(2, 4); // face contact
    else if (phase % 3U == 1)
      result.operand_b = make_box(exact_rational(2), exact_rational(4),
                                  exact_rational(2), exact_rational(4),
                                  exact_rational(0), exact_rational(2));
    else
      result.operand_b = make_box(exact_rational(2), exact_rational(4),
                                  exact_rational(2), exact_rational(4),
                                  exact_rational(2), exact_rational(4));
    result.expectation =
        valid_expectation(qualification_known_relation::touching_only);
    break;
  }
  case qualification_generator_family::exact_subdivision_refinement: {
    result.construction = qualification_construction_kind::exact_subdivision;
    result.operand_a = make_l_prism(exact_rational(0), exact_rational(0),
                                    exact_rational(0), exact_rational(2));
    result.operand_b = triangulated(result.operand_a);
    result.expectation =
        valid_expectation(qualification_known_relation::equal_boundaries);
    break;
  }
  case qualification_generator_family::cadlike_dense_tessellation: {
    result.construction = qualification_construction_kind::cad_like_model;
    result.operand_a = make_octagonal_prism(exact_rational(0),
                                            exact_rational(3));
    result.operand_b = triangulate_convex_mesh(result.operand_a);
    const unsigned refinement_rounds = 1U + phase % 2U;
    for (unsigned round = 0; round != refinement_rounds; ++round)
      result.operand_b = refine_triangular_mesh(result.operand_b);
    result.retained_parameters.push_back(refinement_rounds);
    result.retained_parameters.push_back(
        static_cast<std::int64_t>(result.operand_b.faces.size()));
    result.expectation =
        valid_expectation(qualification_known_relation::equal_boundaries);
    break;
  }
  case qualification_generator_family::exact_representable_scale_bits: {
    result.construction = qualification_construction_kind::representable_transform;
    result.operand_a = make_box(0, 2);
    result.operand_b = make_box(1, 3);
    static const std::array<std::int32_t, 8> exponents{{
        -149, -126, -40, -20, -1, 0, 20, 100}};
    const std::int32_t exponent = exponents[phase];
    scale_power_of_two(result.operand_a, exponent);
    scale_power_of_two(result.operand_b, exponent);
    result.operand_a.vertices.front().coordinate[0].negative_zero = true;
    const std::int32_t adjacent_exponent = std::max<std::int32_t>(-149, exponent - 22);
    // Shift the complete x-maximum face by one adjacent representable step.
    // Moving only one corner would turn three box facets non-planar and would
    // make this nominally-valid transform family fail strict Component 2
    // validation.  A whole-face displacement preserves a closed planar B-rep
    // while still exercising adjacent-bit coordinate behaviour.
    const auto adjacent_step = power_of_two(adjacent_exponent);
    for (const auto vertex_index : {std::size_t(1), std::size_t(2),
                                    std::size_t(5), std::size_t(6)})
      result.operand_b.vertices[vertex_index].coordinate[0].value =
          result.operand_b.vertices[vertex_index].coordinate[0].value +
          adjacent_step;
    result.retained_parameters.push_back(exponent);
    result.retained_parameters.push_back(adjacent_exponent);
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_nondyadic_intersection: {
    result.construction =
        qualification_construction_kind::nondyadic_intersection_inputs;
    result.operand_a = make_triangle_prism();
    result.operand_b = make_box(exact_rational(-1), exact_rational(2),
                                exact_rational(1), exact_rational(2),
                                rational(1, 1), rational(3, 1));
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_thin_sliver_dense:
  case qualification_generator_family::cadlike_thin_feature: {
    result.construction = descriptor.family ==
                                  qualification_generator_family::cadlike_thin_feature
                              ? qualification_construction_kind::cad_like_model
                              : qualification_construction_kind::thin_or_dense_feature;
    const auto thickness = rational(1, 10U + static_cast<std::uint32_t>(phase));
    result.operand_a = make_box(exact_rational(0), exact_rational(4),
                                exact_rational(0), thickness, exact_rational(0),
                                exact_rational(2));
    result.operand_b = make_l_prism(rational(1, 2), rational(-1, 2),
                                    exact_rational(1), exact_rational(3));
    result.operand_b = triangulated(result.operand_b);
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::exact_capacity_replay_boundary: {
    result.construction =
        qualification_construction_kind::capacity_or_replay_boundary;
    result.operand_a = triangulate_convex_mesh(make_box(0, 3));
    result.operand_b = triangulate_convex_mesh(
        make_box(exact_rational(1), exact_rational(4),
                 exact_rational(1), exact_rational(4),
                 exact_rational(1), exact_rational(4)));
    const unsigned rounds = phase % 3U;
    for (unsigned i = 0; i != rounds; ++i) {
      result.operand_a = refine_triangular_mesh(result.operand_a);
      result.operand_b = refine_triangular_mesh(result.operand_b);
    }
    result.retained_parameters.push_back(
        static_cast<std::int64_t>(result.operand_a.faces.size() +
                                  result.operand_b.faces.size()));
    result.expectation = valid_expectation(qualification_known_relation::overlapping);
    break;
  }
  case qualification_generator_family::cadlike_controlled_defect: {
    result.construction =
        qualification_construction_kind::controlled_invalid_preparation;
    result.operand_a = make_l_prism(exact_rational(0), exact_rational(0),
                                    exact_rational(0), exact_rational(2));
    result.operand_b = make_box(0, 2);
    const auto defect = static_cast<qualification_defect_label>(
        1U + phase % (static_cast<unsigned>(qualification_defect_label::count) - 1U));
    apply_controlled_defect(result.operand_b, defect);
    result.expectation = invalid_expectation(defect);
    break;
  }
  case qualification_generator_family::count:
    throw std::logic_error("invalid qualification generator family");
  }
  return result;
}

struct chain_literal {
  const char *identifier;
  const char *version;
  const char *digest_hex;
  std::uint64_t count;
  std::uint32_t minimum_steps;
  std::uint32_t maximum_steps;
};

const chain_literal chain_literals[] = {
    {"chain-mixed-csg-reingestion-v1", "1",
     "1b77ed4fae69f3df866a8a1c410eec41", 400, 5, 12},
    {"chain-cavity-coplanar-v1", "1",
     "cd21d496ee0dc3a17f1396263389c23f", 250, 5, 10},
    {"chain-subdivision-association-v1", "1",
     "164773c6c5d307c9482d1c39906e3537", 200, 5, 9},
    {"chain-explicit-preparation-boundary-v1", "1",
     "425c10e4a3c2e2ae80e2a2609777d7af", 150, 5, 8},
    {"chain-transactional-failure-replay-v1", "1",
     "745f22842813119942f91b7c99787b4d", 100, 5, 7}};

const chain_literal *find_chain_literal(const std::string &identifier,
                                        const std::string &version) {
  for (const auto &literal : chain_literals)
    if (identifier == literal.identifier && version == literal.version)
      return &literal;
  return nullptr;
}

void remove_unreferenced_vertices(mesh &value) {
  std::vector<bool> used(value.vertices.size(), false);
  for (const auto &face : value.faces)
    for (const auto index : face)
      if (index < used.size())
        used[index] = true;
  std::vector<std::uint64_t> remap(value.vertices.size(), 0);
  std::vector<vertex> retained;
  for (std::size_t i = 0; i != value.vertices.size(); ++i)
    if (used[i]) {
      remap[i] = retained.size();
      retained.push_back(value.vertices[i]);
    }
  for (auto &face : value.faces)
    for (auto &index : face)
      index = remap[index];
  value.vertices = std::move(retained);
}

bool try_candidate(qualification_case_recipe &current,
                   qualification_case_recipe candidate,
                   const qualification_reproduction_predicate &predicate,
                   std::uint64_t &attempts, std::uint64_t attempt_limit,
                   qualification_minimization_edit edit,
                   std::vector<qualification_minimization_edit> &edits) {
  if (attempts >= attempt_limit)
    return false;
  ++attempts;
  const digest root_parent =
      current.derivation ==
              qualification_recipe_derivation::minimized_regression_candidate
          ? current.parent_case_digest
          : current.case_digest;
  candidate.derivation =
      qualification_recipe_derivation::minimized_regression_candidate;
  candidate.valid_mutation.reset();
  candidate.parent_case_digest = root_parent;
  candidate.case_digest = {};
  auto made = qualification_generation_detail::canonicalize_qualification_case(
      std::move(candidate));
  if (!made.has_value())
    return false;
  if (!predicate(made.value()))
    return false;
  edit.before_digest = current.case_digest;
  edit.after_digest = made.value().case_digest;
  current = std::move(made.value());
  edits.push_back(std::move(edit));
  return true;
}

} // namespace

namespace qualification_generation_detail {

product_error generation_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

qualification_recipe_mesh
subdivide_qualification_mesh(const qualification_recipe_mesh &source) {
  qualification_recipe_mesh result;
  result.vertices = source.vertices;
  for (const auto &face : source.faces) {
    // The generic recipe helper only splits quadrilaterals.  A fan is exact for
    // a convex polygon but is not a valid subdivision for an arbitrary concave
    // profile, so larger rings are deliberately retained unchanged.
    if (face.size() != 4) {
      result.faces.push_back(face);
      continue;
    }
    result.faces.push_back({face[0], face[1], face[2]});
    result.faces.push_back({face[0], face[2], face[3]});
  }
  return result;
}

product_status_or<qualification_case_recipe>
canonicalize_qualification_case(qualification_case_recipe value) {
  if (value.schema != qualification_generation_schema_version ||
      !text(value.recipe_identifier) || !text(value.recipe_version) ||
      value.recipe_digest == digest{} ||
      ordinal(value.family) >= ordinal(qualification_generator_family::count) ||
      !validate_mesh_contract(value.operand_a) ||
      !validate_mesh_contract(value.operand_b) ||
      value.retained_parameters.size() > 1024U ||
      value.geometry_categories.empty())
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.case_contract");

  auto descriptor = find_qualification_generator_descriptor(
      value.recipe_identifier, value.recipe_version);
  if (!descriptor.has_value())
    return descriptor.error();
  if (descriptor.value().family != value.family ||
      descriptor.value().recipe_digest != value.recipe_digest ||
      value.ordinal < descriptor.value().first_case_ordinal ||
      value.ordinal - descriptor.value().first_case_ordinal >=
          descriptor.value().case_count ||
      value.deterministic_seed !=
          splitmix64((static_cast<std::uint64_t>(ordinal(value.family)) << 56U) ^
                     value.ordinal) ||
      value.construction != expected_construction(value.family))
    return generation_error(product_error_code::stale_binding,
                            "qualification_generator.case_recipe_binding");

  const auto derivation_index = static_cast<unsigned>(value.derivation);
  if (derivation_index > static_cast<unsigned>(
                             qualification_recipe_derivation::
                                 minimized_regression_candidate))
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.unknown_derivation");
  if (value.valid_mutation &&
      static_cast<unsigned>(*value.valid_mutation) >=
          static_cast<unsigned>(qualification_valid_mutation::count))
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.unknown_valid_mutation");
  if (value.derivation == qualification_recipe_derivation::inventory_ordinal) {
    if (value.valid_mutation || value.parent_case_digest != digest{})
      return generation_error(product_error_code::stale_binding,
                              "qualification_generator.inventory_derivation_binding");
  } else {
    if (value.parent_case_digest == digest{})
      return generation_error(product_error_code::stale_binding,
                              "qualification_generator.derived_parent_missing");
    if ((value.derivation ==
             qualification_recipe_derivation::valid_fuzz_mutation) !=
        static_cast<bool>(value.valid_mutation))
      return generation_error(product_error_code::stale_binding,
                              "qualification_generator.valid_mutation_binding");
  }

  std::sort(value.geometry_categories.begin(), value.geometry_categories.end(),
            [](category a, category b) { return ordinal(a) < ordinal(b); });
  if (std::adjacent_find(value.geometry_categories.begin(),
                         value.geometry_categories.end()) !=
          value.geometry_categories.end() ||
      value.geometry_categories != descriptor.value().geometry_categories)
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.case_category_binding");

  if (static_cast<unsigned>(value.expectation.relation) >
          static_cast<unsigned>(qualification_known_relation::
                                    controlled_invalid_operand) ||
      static_cast<unsigned>(value.expectation.defect) >=
          static_cast<unsigned>(qualification_defect_label::count))
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.unknown_expectation_enum");
  const bool controlled_defect =
      value.expectation.defect != qualification_defect_label::none;
  if (controlled_defect !=
          (value.expectation.relation ==
           qualification_known_relation::controlled_invalid_operand) ||
      controlled_defect != value.expectation.normalization_report_required)
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.defect_expectation_binding");

  auto &outcomes = value.expectation.allowed_outcomes;
  for (const auto outcome : outcomes)
    if (ordinal(outcome) > ordinal(qualification_outcome::infrastructure_failure))
      return generation_error(product_error_code::qualification_policy_violation,
                              "qualification_generator.unknown_outcome");
  std::sort(outcomes.begin(), outcomes.end(), [](auto a, auto b) {
    return ordinal(a) < ordinal(b);
  });
  if (outcomes.empty() ||
      std::adjacent_find(outcomes.begin(), outcomes.end()) != outcomes.end())
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.case_outcomes");
  auto &codes = value.expectation.allowed_failure_codes;
  for (const auto code : codes)
    if (ordinal(code) > ordinal(product_error_code::verifier_disagreement))
      return generation_error(product_error_code::qualification_policy_violation,
                              "qualification_generator.unknown_failure_code");
  std::sort(codes.begin(), codes.end(),
            [](auto a, auto b) { return ordinal(a) < ordinal(b); });
  if (std::adjacent_find(codes.begin(), codes.end()) != codes.end())
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.case_failure_codes");
  const bool expects_failure =
      std::find(outcomes.begin(), outcomes.end(),
                qualification_outcome::expected_typed_failure) != outcomes.end();
  if (expects_failure != !codes.empty())
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.case_failure_binding");
  if (value.expectation.defect != qualification_defect_label::none &&
      !value.expectation.normalization_report_required)
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.defect_report_binding");
  if (value.construction ==
      qualification_construction_kind::exact_halfspace_polytope) {
    if (value.operand_a_halfspaces.size() < 4 ||
        value.operand_b_halfspaces.size() < 4 ||
        !halfspace_system_binds_mesh(value.operand_a,
                                     value.operand_a_halfspaces) ||
        !halfspace_system_binds_mesh(value.operand_b,
                                     value.operand_b_halfspaces))
      return generation_error(
          product_error_code::qualification_policy_violation,
          "qualification_generator.halfspace_recipe_missing_or_stale");
  } else if (!value.operand_a_halfspaces.empty() ||
             !value.operand_b_halfspaces.empty()) {
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_generator.unexpected_halfspaces");
  }

  if (value.derivation == qualification_recipe_derivation::inventory_ordinal) {
    auto expected = generate_case(descriptor.value(), value.ordinal);
    canonical_encoder expected_encoder, supplied_encoder;
    encode_case_without_digest(expected_encoder, expected);
    encode_case_without_digest(supplied_encoder, value);
    if (expected_encoder.bytes() != supplied_encoder.bytes())
      return generation_error(product_error_code::stale_binding,
                              "qualification_generator.ordinal_mapping_changed");
  }

  canonical_encoder encoder;
  encode_case_without_digest(encoder, value);
  const auto computed = domain_digest(case_tag, encoder.bytes());
  if (value.case_digest != digest{} && value.case_digest != computed)
    return generation_error(product_error_code::stale_binding,
                            "qualification_generator.stale_case_digest");
  value.case_digest = computed;
  return value;
}

product_status_or<qualification_operation_chain_recipe>
canonicalize_qualification_chain(qualification_operation_chain_recipe value) {
  if (value.schema != qualification_generation_schema_version ||
      !text(value.identifier) || !text(value.version) ||
      value.definition_digest == digest{})
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_chain.contract");
  const auto *literal = find_chain_literal(value.identifier, value.version);
  if (!literal || value.definition_digest != digest_literal(literal->digest_hex) ||
      value.ordinal >= literal->count ||
      value.steps.size() < literal->minimum_steps ||
      value.steps.size() > literal->maximum_steps)
    return generation_error(product_error_code::stale_binding,
                            "qualification_chain.definition_binding");
  auto initial = canonicalize_qualification_case(std::move(value.initial_case));
  if (!initial.has_value())
    return initial.error();
  value.initial_case = std::move(initial.value());
  bool saw_normalized_boundary = false;
  bool saw_subdivision = false;
  bool saw_expected_failure = false;
  for (auto &step : value.steps) {
    if (static_cast<unsigned>(step.selected_operation) >
            static_cast<unsigned>(operation::symmetric_difference) ||
        static_cast<unsigned>(step.operand_order) >
            static_cast<unsigned>(qualification_operand_order::b_then_a) ||
        static_cast<unsigned>(step.preparation) >
            static_cast<unsigned>(preparation_mode::normalized) ||
        static_cast<unsigned>(step.requested_result) >
            static_cast<unsigned>(
                result_representation::certified_approximate_mesh))
      return generation_error(product_error_code::qualification_policy_violation,
                              "qualification_chain.unknown_step_enum");
    auto rhs = canonicalize_qualification_case(std::move(step.rhs_case));
    if (!rhs.has_value())
      return rhs.error();
    step.rhs_case = std::move(rhs.value());
    std::sort(step.expected_failure_codes.begin(),
              step.expected_failure_codes.end(),
              [](auto a, auto b) { return ordinal(a) < ordinal(b); });
    if (std::adjacent_find(step.expected_failure_codes.begin(),
                           step.expected_failure_codes.end()) !=
        step.expected_failure_codes.end())
      return generation_error(product_error_code::qualification_policy_violation,
                              "qualification_chain.duplicate_expected_failure");
    for (const auto code : step.expected_failure_codes)
      if (ordinal(code) > ordinal(product_error_code::verifier_disagreement))
        return generation_error(product_error_code::qualification_policy_violation,
                                "qualification_chain.unknown_expected_failure");
    if (step.continue_after_expected_failure !=
        !step.expected_failure_codes.empty())
      return generation_error(product_error_code::qualification_policy_violation,
                              "qualification_chain.failure_continuation_binding");
    saw_normalized_boundary =
        saw_normalized_boundary || step.preparation == preparation_mode::normalized;
    saw_subdivision =
        saw_subdivision || step.subdivide_accumulator_before_step;
    saw_expected_failure =
        saw_expected_failure || !step.expected_failure_codes.empty();
  }
  const bool subdivision_family =
      value.identifier == "chain-subdivision-association-v1";
  const bool preparation_family =
      value.identifier == "chain-explicit-preparation-boundary-v1";
  const bool transactional_family =
      value.identifier == "chain-transactional-failure-replay-v1";
  if (saw_subdivision != subdivision_family)
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_chain.subdivision_family_binding");
  if (saw_normalized_boundary != preparation_family)
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_chain.preparation_family_binding");
  if (saw_expected_failure != transactional_family)
    return generation_error(product_error_code::qualification_policy_violation,
                            "qualification_chain.transaction_family_binding");
  canonical_encoder encoder;
  encode_chain_without_digest(encoder, value);
  const auto computed = domain_digest(chain_tag, encoder.bytes());
  if (value.chain_digest != digest{} && value.chain_digest != computed)
    return generation_error(product_error_code::stale_binding,
                            "qualification_chain.stale_digest");
  value.chain_digest = computed;
  return value;
}

} // namespace qualification_generation_detail

const char *qualification_generator_family_token(
    qualification_generator_family value) noexcept {
  static const char *const names[] = {
      "exact_halfspace_skew_convex",
      "exact_profile_extrusion_concave",
      "exact_coplanar_overlay",
      "exact_nested_shell_cavity",
      "exact_feature_alignment_contact",
      "exact_subdivision_refinement",
      "exact_representable_scale_bits",
      "exact_nondyadic_intersection",
      "exact_thin_sliver_dense",
      "exact_capacity_replay_boundary",
      "cadlike_profile_extruded_part",
      "cadlike_multibody_cavity",
      "cadlike_thin_feature",
      "cadlike_dense_tessellation",
      "cadlike_attribute_seam_conflict",
      "cadlike_controlled_defect"};
  const auto index = ordinal(value);
  return index < ordinal(qualification_generator_family::count) ? names[index]
                                                                : "invalid";
}

const char *qualification_defect_label_token(
    qualification_defect_label value) noexcept {
  static const char *const names[] = {
      "none",          "duplicate_vertex_use", "duplicate_facet",
      "inconsistent_orientation", "open_crack", "nonplanar_facet",
      "overlapping_shells", "sliver_feature", "self_intersection"};
  const auto index = static_cast<unsigned>(value);
  return index < static_cast<unsigned>(qualification_defect_label::count)
             ? names[index]
             : "invalid";
}

std::vector<qualification_generator_descriptor>
qualification_generator_descriptors() {
  std::vector<qualification_generator_descriptor> result;
  result.reserve(sizeof(descriptor_literals) / sizeof(descriptor_literals[0]));
  for (const auto &literal : descriptor_literals)
    result.push_back(make_descriptor(literal));
  return result;
}

product_status_or<qualification_generator_descriptor>
find_qualification_generator_descriptor(const std::string &identifier,
                                        const std::string &version) noexcept {
  try {
    for (const auto &descriptor : qualification_generator_descriptors())
      if (descriptor.identifier == identifier && descriptor.version == version)
        return descriptor;
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_generator.unknown_recipe");
  } catch (const std::bad_alloc &) {
    return qualification_generation_detail::generation_error(
        product_error_code::resource_limit,
        "qualification_generator.descriptor_allocation");
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_generator.descriptor_exception");
  }
}

product_status_or<qualification_case_recipe>
make_qualification_case_recipe(const std::string &identifier,
                               const std::string &version,
                               std::uint64_t ordinal_value) {
  try {
    auto descriptor = find_qualification_generator_descriptor(identifier, version);
    if (!descriptor.has_value())
      return descriptor.error();
    if (ordinal_value < descriptor.value().first_case_ordinal ||
        ordinal_value - descriptor.value().first_case_ordinal >=
            descriptor.value().case_count)
      return qualification_generation_detail::generation_error(
          product_error_code::qualification_policy_violation,
          "qualification_generator.ordinal_out_of_range");
    return qualification_generation_detail::canonicalize_qualification_case(
        generate_case(descriptor.value(), ordinal_value));
  } catch (const std::bad_alloc &) {
    return qualification_generation_detail::generation_error(
        product_error_code::resource_limit,
        "qualification_generator.case_allocation");
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_generator.case_exception");
  }
}

product_status_or<bool>
validate_qualification_case_recipe(const qualification_case_recipe &value) noexcept {
  try {
    auto made = qualification_generation_detail::canonicalize_qualification_case(value);
    if (!made.has_value())
      return made.error();
    if (made.value().case_digest != value.case_digest)
      return qualification_generation_detail::generation_error(
          product_error_code::stale_binding,
          "qualification_generator.case_not_canonical");
    return true;
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_generator.case_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_case_recipe(const qualification_case_recipe &value) {
  auto valid = validate_qualification_case_recipe(value);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder encoder;
  encode_case_without_digest(encoder, value);
  encoder.raw(value.case_digest.bytes.data(), value.case_digest.bytes.size());
  return encoder.bytes();
}

product_status_or<qualification_case_recipe>
make_qualification_valid_fuzz_case(qualification_valid_mutation mutation,
                                   std::uint64_t seed) {
  static const char *const recipes[] = {
      "exact-feature-alignment-contact-v1",
      "exact-coplanar-overlay-v1",
      "exact-subdivision-refinement-v1",
      "exact-subdivision-refinement-v1",
      "exact-nested-shell-cavity-v1",
      "cadlike-multibody-cavity-v1",
      "exact-representable-scale-bits-v1",
      "exact-representable-scale-bits-v1",
      "cadlike-dense-tessellation-v1",
      "exact-nondyadic-intersection-v1"};
  const auto index = static_cast<unsigned>(mutation);
  if (index >= static_cast<unsigned>(qualification_valid_mutation::count))
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_fuzz.unknown_valid_mutation");
  auto descriptor = find_qualification_generator_descriptor(recipes[index], "1");
  if (!descriptor.has_value())
    return descriptor.error();
  auto made = make_qualification_case_recipe(
      descriptor.value().identifier, descriptor.value().version,
      splitmix64(seed ^ index) % descriptor.value().case_count);
  if (!made.has_value())
    return made.error();
  const digest parent = made.value().case_digest;
  made.value().derivation =
      qualification_recipe_derivation::valid_fuzz_mutation;
  made.value().valid_mutation = mutation;
  made.value().parent_case_digest = parent;
  made.value().retained_parameters.push_back(
      static_cast<std::int64_t>(0x6300U + index));
  made.value().case_digest = {};
  return qualification_generation_detail::canonicalize_qualification_case(
      std::move(made.value()));
}

product_status_or<qualification_case_recipe>
make_qualification_invalid_fuzz_case(qualification_defect_label defect,
                                     std::uint64_t seed) {
  const auto index = static_cast<unsigned>(defect);
  if (defect == qualification_defect_label::none ||
      index >= static_cast<unsigned>(qualification_defect_label::count))
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_fuzz.invalid_defect_label");
  auto descriptor = find_qualification_generator_descriptor(
      "cadlike-controlled-defect-v1", "1");
  if (!descriptor.has_value())
    return descriptor.error();
  auto made = make_qualification_case_recipe(
      descriptor.value().identifier, descriptor.value().version,
      splitmix64(seed ^ (std::uint64_t(index) << 32U)) %
          descriptor.value().case_count);
  if (!made.has_value())
    return made.error();
  const digest parent = made.value().case_digest;
  made.value().operand_b = make_box(0, 2);
  apply_controlled_defect(made.value().operand_b, defect);
  made.value().expectation = invalid_expectation(defect);
  made.value().derivation =
      qualification_recipe_derivation::invalid_fuzz_mutation;
  made.value().valid_mutation.reset();
  made.value().parent_case_digest = parent;
  made.value().retained_parameters.push_back(
      static_cast<std::int64_t>(0x6400U + index));
  made.value().case_digest = {};
  return qualification_generation_detail::canonicalize_qualification_case(
      std::move(made.value()));
}

product_status_or<bool> validate_qualification_preparation_observation(
    const qualification_case_recipe &recipe,
    const qualification_preparation_observation &observation) noexcept {
  try {
    auto valid = validate_qualification_case_recipe(recipe);
    if (!valid.has_value())
      return valid.error();
    if (observation.schema != qualification_generation_schema_version ||
        observation.strict_operand_a_valid !=
            recipe.expectation.strict_operand_a_expected_valid ||
        observation.strict_operand_b_valid !=
            recipe.expectation.strict_operand_b_expected_valid)
      return qualification_generation_detail::generation_error(
          product_error_code::verifier_disagreement,
          "qualification_preparation.strict_observation_mismatch");
    if (!recipe.expectation.normalization_report_required) {
      if (observation.normalization_attempted ||
          observation.reported_defect != qualification_defect_label::none ||
          observation.normalization_edit_count ||
          observation.normalization_report_digest != digest{})
        return qualification_generation_detail::generation_error(
            product_error_code::qualification_policy_violation,
            "qualification_preparation.unexpected_normalization");
      return true;
    }
    if (!observation.normalization_attempted ||
        observation.reported_defect != recipe.expectation.defect ||
        observation.normalization_report_digest == digest{})
      return qualification_generation_detail::generation_error(
          product_error_code::verifier_disagreement,
          "qualification_preparation.missing_defect_report");
    if (observation.normalization_succeeded) {
      if (!observation.normalization_edit_count ||
          !observation.prepared_operand_strictly_valid)
        return qualification_generation_detail::generation_error(
            product_error_code::verifier_disagreement,
            "qualification_preparation.success_not_strictly_valid");
    } else if (observation.prepared_operand_strictly_valid) {
      return qualification_generation_detail::generation_error(
          product_error_code::verifier_disagreement,
          "qualification_preparation.failed_but_prepared_valid");
    }
    return true;
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_preparation.observation_exception");
  }
}

product_status_or<qualification_operation_chain_recipe>
make_qualification_operation_chain_recipe(const std::string &identifier,
                                           const std::string &version,
                                           std::uint64_t ordinal_value) {
  try {
    const auto *literal = find_chain_literal(identifier, version);
    if (!literal || ordinal_value >= literal->count)
      return qualification_generation_detail::generation_error(
          product_error_code::qualification_policy_violation,
          "qualification_chain.unknown_or_out_of_range");
    qualification_operation_chain_recipe result;
    result.identifier = identifier;
    result.version = version;
    result.definition_digest = digest_literal(literal->digest_hex);
    result.ordinal = ordinal_value;

    const std::array<const char *, 10> generated_ids{{
        "exact-halfspace-skew-convex-v1",
        "exact-profile-extrusion-concave-v1",
        "exact-coplanar-overlay-v1",
        "exact-nested-shell-cavity-v1",
        "exact-feature-alignment-contact-v1",
        "exact-subdivision-refinement-v1",
        "exact-representable-scale-bits-v1",
        "exact-nondyadic-intersection-v1",
        "exact-thin-sliver-dense-v1",
        "exact-capacity-replay-boundary-v1"}};
    const auto seed_index = static_cast<std::size_t>(ordinal_value % generated_ids.size());
    auto initial = make_qualification_case_recipe(
        generated_ids[seed_index], "1",
        ordinal_value % find_qualification_generator_descriptor(
                            generated_ids[seed_index], "1")
                            .value()
                            .case_count);
    if (!initial.has_value())
      return initial.error();
    result.initial_case = std::move(initial.value());
    result.initial_uses_operand_b = (ordinal_value & 1U) != 0;

    const std::uint32_t span = literal->maximum_steps - literal->minimum_steps + 1;
    const auto step_count = literal->minimum_steps +
                            static_cast<std::uint32_t>(ordinal_value % span);
    for (std::uint32_t step_index = 0; step_index != step_count; ++step_index) {
      qualification_chain_step_recipe step;
      step.selected_operation =
          static_cast<operation>((ordinal_value + step_index) % 5U);
      step.operand_order = ((ordinal_value + step_index) & 1U)
                               ? qualification_operand_order::b_then_a
                               : qualification_operand_order::a_then_b;
      step.preparation = preparation_mode::strict_validation;
      step.requested_result = static_cast<result_representation>(step_index % 3U);
      const auto family_index =
          (seed_index + step_index + 1U) % generated_ids.size();
      auto descriptor = find_qualification_generator_descriptor(
          generated_ids[family_index], "1");
      if (!descriptor.has_value())
        return descriptor.error();
      auto rhs = make_qualification_case_recipe(
          generated_ids[family_index], "1",
          (ordinal_value * 17U + step_index) % descriptor.value().case_count);
      if (!rhs.has_value())
        return rhs.error();
      step.rhs_case = std::move(rhs.value());
      step.rhs_uses_operand_b = ((ordinal_value >> (step_index % 16U)) & 1U) != 0;
      step.continue_with_prior_mesh_when_unrealized =
          step.requested_result != result_representation::exact_in_T_mesh;

      if (identifier == "chain-subdivision-association-v1")
        step.subdivide_accumulator_before_step = (step_index % 2U) == 0;
      if (identifier == "chain-explicit-preparation-boundary-v1" &&
          (step_index == 1U || step_index + 1U == step_count))
        step.preparation = preparation_mode::normalized;
      if (identifier == "chain-transactional-failure-replay-v1" &&
          step_index == step_count / 2U) {
        step.expected_failure_codes = {product_error_code::resource_limit,
                                       product_error_code::output_not_representable};
        step.continue_after_expected_failure = true;
      }
      result.steps.push_back(std::move(step));
    }
    return qualification_generation_detail::canonicalize_qualification_chain(
        std::move(result));
  } catch (const std::bad_alloc &) {
    return qualification_generation_detail::generation_error(
        product_error_code::resource_limit,
        "qualification_chain.allocation");
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_chain.exception");
  }
}

product_status_or<bool> validate_qualification_operation_chain_recipe(
    const qualification_operation_chain_recipe &value) noexcept {
  try {
    auto made = qualification_generation_detail::canonicalize_qualification_chain(value);
    if (!made.has_value())
      return made.error();
    if (made.value().chain_digest != value.chain_digest)
      return qualification_generation_detail::generation_error(
          product_error_code::stale_binding,
          "qualification_chain.not_canonical");
    return true;
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_chain.validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_operation_chain_recipe(
    const qualification_operation_chain_recipe &value) {
  auto valid = validate_qualification_operation_chain_recipe(value);
  if (!valid.has_value())
    return valid.error();
  canonical_encoder encoder;
  encode_chain_without_digest(encoder, value);
  encoder.raw(value.chain_digest.bytes.data(), value.chain_digest.bytes.size());
  return encoder.bytes();
}

product_status_or<qualification_failure_provenance>
make_qualification_failure_provenance(qualification_failure_provenance value) {
  if (value.schema != qualification_generation_schema_version)
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_provenance.schema");
  auto canonicalize = [](std::vector<std::uint64_t> &indices) {
    std::sort(indices.begin(), indices.end());
    return std::adjacent_find(indices.begin(), indices.end()) == indices.end();
  };
  if (!canonicalize(value.operand_a_faces) ||
      !canonicalize(value.operand_b_faces) ||
      !canonicalize(value.operand_a_vertices) ||
      !canonicalize(value.operand_b_vertices))
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_provenance.duplicate_index");
  canonical_encoder encoder;
  encoder.u16(value.schema);
  const auto encode_indices = [&](const std::vector<std::uint64_t> &indices) {
    encoder.u64(indices.size());
    for (const auto index : indices)
      encoder.u64(index);
  };
  encode_indices(value.operand_a_faces);
  encode_indices(value.operand_b_faces);
  encode_indices(value.operand_a_vertices);
  encode_indices(value.operand_b_vertices);
  encoder.boolean(static_cast<bool>(value.chain_step));
  if (value.chain_step)
    encoder.u64(*value.chain_step);
  const auto computed = domain_digest(provenance_tag, encoder.bytes());
  if (value.evidence_digest != digest{} && value.evidence_digest != computed)
    return qualification_generation_detail::generation_error(
        product_error_code::stale_binding,
        "qualification_provenance.stale_digest");
  value.evidence_digest = computed;
  return value;
}

product_status_or<bool> validate_qualification_failure_provenance(
    const qualification_failure_provenance &value) noexcept {
  try {
    auto made = make_qualification_failure_provenance(value);
    if (!made.has_value())
      return made.error();
    if (made.value().evidence_digest != value.evidence_digest)
      return qualification_generation_detail::generation_error(
          product_error_code::stale_binding,
          "qualification_provenance.not_canonical");
    return true;
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_provenance.validation_exception");
  }
}

product_status_or<qualification_minimization_result>
minimize_qualification_case(const qualification_case_recipe &source,
                            const qualification_failure_provenance &provenance,
                            const qualification_reproduction_predicate &predicate,
                            std::uint64_t attempt_limit) {
  if (!predicate || !attempt_limit)
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_minimizer.contract");
  try {
    auto canonical = qualification_generation_detail::canonicalize_qualification_case(source);
    if (!canonical.has_value())
      return canonical.error();
    if (!predicate(canonical.value()))
      return qualification_generation_detail::generation_error(
          product_error_code::qualification_policy_violation,
          "qualification_minimizer.source_does_not_reproduce");
    auto canonical_provenance =
        make_qualification_failure_provenance(provenance);
    if (!canonical_provenance.has_value())
      return canonical_provenance.error();
    const auto &bound_provenance = canonical_provenance.value();
    auto in_range = [](const std::vector<std::uint64_t> &indices,
                       std::size_t bound) {
      return std::all_of(indices.begin(), indices.end(),
                         [&](std::uint64_t index) { return index < bound; });
    };
    if (!in_range(bound_provenance.operand_a_faces,
                  canonical.value().operand_a.faces.size()) ||
        !in_range(bound_provenance.operand_b_faces,
                  canonical.value().operand_b.faces.size()) ||
        !in_range(bound_provenance.operand_a_vertices,
                  canonical.value().operand_a.vertices.size()) ||
        !in_range(bound_provenance.operand_b_vertices,
                  canonical.value().operand_b.vertices.size()))
      return qualification_generation_detail::generation_error(
          product_error_code::qualification_policy_violation,
          "qualification_minimizer.provenance_index_range");

    qualification_minimization_result result;
    result.minimized = std::move(canonical.value());
    auto minimize_mesh_faces = [&](std::uint8_t operand_index) {
      bool changed = true;
      while (changed && result.attempts < attempt_limit) {
        changed = false;
        const auto &target = operand_index == 0 ? result.minimized.operand_a
                                                : result.minimized.operand_b;
        if (target.faces.size() <= 4)
          break;
        const auto &protected_faces = operand_index == 0
                                          ? bound_provenance.operand_a_faces
                                          : bound_provenance.operand_b_faces;
        std::vector<std::size_t> removal_order;
        removal_order.reserve(target.faces.size());
        for (std::size_t reverse = target.faces.size(); reverse != 0; --reverse) {
          const auto index = reverse - 1;
          if (!std::binary_search(protected_faces.begin(), protected_faces.end(),
                                  static_cast<std::uint64_t>(index)))
            removal_order.push_back(index);
        }
        for (std::size_t reverse = target.faces.size(); reverse != 0; --reverse) {
          const auto index = reverse - 1;
          if (std::binary_search(protected_faces.begin(), protected_faces.end(),
                                 static_cast<std::uint64_t>(index)))
            removal_order.push_back(index);
        }
        for (const auto index : removal_order) {
          auto candidate = result.minimized;
          auto &candidate_mesh = operand_index == 0 ? candidate.operand_a
                                                    : candidate.operand_b;
          candidate_mesh.faces.erase(candidate_mesh.faces.begin() + index);
          remove_unreferenced_vertices(candidate_mesh);
          qualification_minimization_edit edit;
          edit.kind = qualification_minimization_edit_kind::remove_face;
          edit.operand = operand_index;
          edit.primary_index = index;
          if (try_candidate(result.minimized, std::move(candidate), predicate,
                            result.attempts, attempt_limit, edit, result.edits)) {
            changed = true;
            break;
          }
        }
      }
    };
    minimize_mesh_faces(1);
    minimize_mesh_faces(0);

    for (std::uint8_t operand_index = 0; operand_index != 2 &&
                                         result.attempts < attempt_limit;
         ++operand_index) {
      auto vertex_count = operand_index == 0 ? result.minimized.operand_a.vertices.size()
                                             : result.minimized.operand_b.vertices.size();
      const auto &protected_vertices = operand_index == 0
                                           ? bound_provenance.operand_a_vertices
                                           : bound_provenance.operand_b_vertices;
      std::vector<std::size_t> vertex_order;
      vertex_order.reserve(vertex_count);
      for (std::size_t vertex_index = 0; vertex_index != vertex_count; ++vertex_index)
        if (!std::binary_search(protected_vertices.begin(),
                                protected_vertices.end(),
                                static_cast<std::uint64_t>(vertex_index)))
          vertex_order.push_back(vertex_index);
      for (std::size_t vertex_index = 0; vertex_index != vertex_count; ++vertex_index)
        if (std::binary_search(protected_vertices.begin(),
                               protected_vertices.end(),
                               static_cast<std::uint64_t>(vertex_index)))
          vertex_order.push_back(vertex_index);
      for (const auto vertex_index : vertex_order) {
        if (result.attempts >= attempt_limit)
          break;
        for (std::size_t axis = 0; axis != 3 && result.attempts < attempt_limit;
             ++axis) {
          auto zero_candidate = result.minimized;
          auto &target = operand_index == 0 ? zero_candidate.operand_a
                                            : zero_candidate.operand_b;
          target.vertices[vertex_index].coordinate[axis].value = exact_rational(0);
          target.vertices[vertex_index].coordinate[axis].negative_zero = false;
          qualification_minimization_edit zero_edit;
          zero_edit.kind =
              qualification_minimization_edit_kind::replace_coordinate_with_zero;
          zero_edit.operand = operand_index;
          zero_edit.primary_index = vertex_index;
          zero_edit.secondary_index = axis;
          if (try_candidate(result.minimized, std::move(zero_candidate), predicate,
                            result.attempts, attempt_limit, zero_edit,
                            result.edits))
            continue;

          auto half_candidate = result.minimized;
          auto &half_target = operand_index == 0 ? half_candidate.operand_a
                                                 : half_candidate.operand_b;
          half_target.vertices[vertex_index].coordinate[axis].value =
              half_target.vertices[vertex_index].coordinate[axis].value /
              exact_rational(2);
          qualification_minimization_edit half_edit;
          half_edit.kind = qualification_minimization_edit_kind::halve_coordinate;
          half_edit.operand = operand_index;
          half_edit.primary_index = vertex_index;
          half_edit.secondary_index = axis;
          try_candidate(result.minimized, std::move(half_candidate), predicate,
                        result.attempts, attempt_limit, half_edit, result.edits);
        }
      }
    }

    if (result.minimized.ordinal != 0 && result.attempts < attempt_limit) {
      auto lower = make_qualification_case_recipe(
          result.minimized.recipe_identifier, result.minimized.recipe_version,
          result.minimized.ordinal / 2U);
      if (lower.has_value()) {
        qualification_minimization_edit edit;
        edit.kind = qualification_minimization_edit_kind::reduce_case_ordinal;
        edit.primary_index = result.minimized.ordinal;
        edit.secondary_index = lower.value().ordinal;
        try_candidate(result.minimized, std::move(lower.value()), predicate,
                      result.attempts, attempt_limit, edit, result.edits);
      }
    }

    canonical_encoder transcript;
    transcript.raw(source.case_digest.bytes.data(), source.case_digest.bytes.size());
    transcript.raw(bound_provenance.evidence_digest.bytes.data(),
                   bound_provenance.evidence_digest.bytes.size());
    transcript.raw(result.minimized.case_digest.bytes.data(),
                   result.minimized.case_digest.bytes.size());
    transcript.u64(result.attempts);
    transcript.u64(result.edits.size());
    for (const auto &edit : result.edits) {
      transcript.byte(static_cast<std::uint8_t>(edit.kind));
      transcript.byte(edit.operand);
      transcript.u64(edit.primary_index);
      transcript.u64(edit.secondary_index);
      transcript.raw(edit.before_digest.bytes.data(), edit.before_digest.bytes.size());
      transcript.raw(edit.after_digest.bytes.data(), edit.after_digest.bytes.size());
    }
    result.transcript_digest = domain_digest(minimization_tag, transcript.bytes());
    return result;
  } catch (const std::bad_alloc &) {
    return qualification_generation_detail::generation_error(
        product_error_code::resource_limit,
        "qualification_minimizer.allocation");
  } catch (...) {
    return qualification_generation_detail::generation_error(
        product_error_code::internal_invariant_error,
        "qualification_minimizer.exception");
  }
}

product_status_or<qualification_minimization_result>
minimize_qualification_case(const qualification_case_recipe &source,
                            const qualification_reproduction_predicate &predicate,
                            std::uint64_t attempt_limit) {
  auto provenance = make_qualification_failure_provenance({});
  if (!provenance.has_value())
    return provenance.error();
  return minimize_qualification_case(source, provenance.value(), predicate,
                                     attempt_limit);
}

product_status_or<qualification_regression_promotion>
make_qualification_regression_promotion(
    const qualification_minimization_result &minimized) {
  if (minimized.schema != qualification_generation_schema_version ||
      minimized.transcript_digest == digest{})
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_promotion.minimization_binding");
  auto valid = validate_qualification_case_recipe(minimized.minimized);
  if (!valid.has_value())
    return valid.error();
  auto bytes = encode_qualification_case_recipe(minimized.minimized);
  if (!bytes.has_value())
    return bytes.error();
  qualification_regression_promotion result;
  result.minimized_case = minimized.minimized;
  result.canonical_case_bytes = std::move(bytes.value());
  result.minimization_transcript_digest = minimized.transcript_digest;
  const std::string prefix = result.minimized_case.case_digest.hex().substr(0, 16);
  result.identifier = "P63-REG-" + prefix;
  result.permanent_test_id = "P6.3.AUTO." + prefix;
  canonical_encoder encoder;
  encoder.u16(result.schema);
  encoder.string(result.identifier);
  encoder.string(result.permanent_test_id);
  encoder.byte_string(result.canonical_case_bytes);
  encoder.raw(result.minimization_transcript_digest.bytes.data(),
              result.minimization_transcript_digest.bytes.size());
  result.artifact_digest = domain_digest(promotion_tag, encoder.bytes());
  return result;
}

product_status_or<bool> promote_qualification_regression(
    const qualification_regression_promotion &promotion,
    const qualification_regression_sink &sink) {
  if (!sink || promotion.schema != qualification_generation_schema_version ||
      !text(promotion.identifier) || !text(promotion.permanent_test_id) ||
      promotion.canonical_case_bytes.empty() ||
      promotion.minimization_transcript_digest == digest{} ||
      promotion.artifact_digest == digest{})
    return qualification_generation_detail::generation_error(
        product_error_code::qualification_policy_violation,
        "qualification_promotion.contract");
  auto valid = validate_qualification_case_recipe(promotion.minimized_case);
  if (!valid.has_value())
    return valid.error();
  auto canonical_bytes =
      encode_qualification_case_recipe(promotion.minimized_case);
  if (!canonical_bytes.has_value())
    return canonical_bytes.error();
  const std::string prefix =
      promotion.minimized_case.case_digest.hex().substr(0, 16);
  if (promotion.identifier != "P63-REG-" + prefix ||
      promotion.permanent_test_id != "P6.3.AUTO." + prefix ||
      promotion.canonical_case_bytes != canonical_bytes.value())
    return qualification_generation_detail::generation_error(
        product_error_code::stale_binding,
        "qualification_promotion.canonical_binding");
  canonical_encoder encoder;
  encoder.u16(promotion.schema);
  encoder.string(promotion.identifier);
  encoder.string(promotion.permanent_test_id);
  encoder.byte_string(promotion.canonical_case_bytes);
  encoder.raw(promotion.minimization_transcript_digest.bytes.data(),
              promotion.minimization_transcript_digest.bytes.size());
  if (domain_digest(promotion_tag, encoder.bytes()) !=
      promotion.artifact_digest)
    return qualification_generation_detail::generation_error(
        product_error_code::stale_binding,
        "qualification_promotion.stale_artifact_digest");
  return sink(promotion);
}

} // namespace mesh_boolean
} // namespace ygor
