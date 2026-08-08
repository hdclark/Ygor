#include "YgorMeshesBooleanBounded/InputValidation.h"
#include "YgorMeshesBooleanBounded/PrecisionBootstrap.h"
#include "YgorMeshesBooleanBounded/PrecisionVerifier.h"
#include "qualification/ExactGeometryOracle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace bounded = ygor::mesh_boolean::bounded;
namespace exact = ygor::mesh_boolean::qualification;
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

template <class T, class I> fv_surface_mesh<T, I> tetra() {
  fv_surface_mesh<T, I> m;
  m.vertices = {{T(0), T(0), T(0)},
                {T(1), T(0), T(0)},
                {T(0), T(1), T(0)},
                {T(0), T(0), T(1)}};
  m.faces = {{I(0), I(2), I(1)},
             {I(0), I(1), I(3)},
             {I(0), I(3), I(2)},
             {I(1), I(2), I(3)}};
  return m;
}
template <class T, class I>
void append_tetra(fv_surface_mesh<T, I> &m, T x, T y, T z, T size,
                  bool reverse) {
  I base = static_cast<I>(m.vertices.size());
  m.vertices.insert(
      m.vertices.end(),
      {{x, y, z}, {x + size, y, z}, {x, y + size, z}, {x, y, z + size}});
  std::vector<std::vector<I>> faces{{base, I(base + 2), I(base + 1)},
                                    {base, I(base + 1), I(base + 3)},
                                    {base, I(base + 3), I(base + 2)},
                                    {I(base + 1), I(base + 2), I(base + 3)}};
  if (reverse)
    for (auto &f : faces)
      std::reverse(f.begin(), f.end());
  m.faces.insert(m.faces.end(), faces.begin(), faces.end());
}
template <class T, class I>
void append_box(fv_surface_mesh<T, I> &m, T x0, T y0, T z0, T x1, T y1, T z1) {
  I b = static_cast<I>(m.vertices.size());
  m.vertices.insert(m.vertices.end(), {{x0, y0, z0},
                                       {x1, y0, z0},
                                       {x1, y1, z0},
                                       {x0, y1, z0},
                                       {x0, y0, z1},
                                       {x1, y0, z1},
                                       {x1, y1, z1},
                                       {x0, y1, z1}});
  m.faces.insert(m.faces.end(), {{b, I(b + 3), I(b + 2), I(b + 1)},
                                 {I(b + 4), I(b + 5), I(b + 6), I(b + 7)},
                                 {b, I(b + 1), I(b + 5), I(b + 4)},
                                 {I(b + 1), I(b + 2), I(b + 6), I(b + 5)},
                                 {I(b + 2), I(b + 3), I(b + 7), I(b + 6)},
                                 {I(b + 3), b, I(b + 4), I(b + 7)}});
}

template <class T, class I> struct fixture_context {
  bounded::boolean_context<T, I> context;
  std::shared_ptr<const bounded::precision_context<T>> precision;
};
template <class T, class I>
fixture_context<T, I>
make_context_with_options(const fv_surface_mesh<T, I> &mesh,
                          bounded_boolean_options<T> options) {
  fv_surface_mesh<T, I> empty;
  auto pending = bounded::build_pending_invocation(
      mesh, empty, boolean_operation::set_union, options);
  require(pending.has_value(), "pending input context");
  auto preflight = bounded::preflight_precision(*pending.value());
  if (!preflight.has_value())
    throw std::runtime_error(preflight.error()->summary);
  auto context = bounded::finalize_context(
      std::move(*pending.value()),
      bounded::make_precision_bootstrap_record(*preflight.value()));
  if (!context.has_value())
    throw std::runtime_error(context.error()->summary);
  bounded::precision_runtime_capabilities capabilities;
  capabilities.expected_owner = &context.value()->owner;
  auto precision = bounded::build_precision_context(
      *preflight.value(), *context.value(), capabilities);
  if (!precision.has_value())
    throw std::runtime_error(precision.error()->summary);
  return {std::move(*context.value()), std::move(*precision.value())};
}
template <class T, class I>
fixture_context<T, I> make_context(const fv_surface_mesh<T, I> &mesh) {
  bounded_boolean_options<T> options;
  options.tolerance = T(1);
  return make_context_with_options(mesh, options);
}
template <class T, class I> auto validate_a(const fv_surface_mesh<T, I> &mesh) {
  auto fixture = make_context(mesh);
  bounded::resource_manager resources(fixture.context.options.resources);
  bounded::input_validation_capabilities capabilities;
  capabilities.owner = fixture.context.owner;
  capabilities.resources = &resources;
  auto result = bounded::validate_operand(
      bounded::operand_id::a, fixture.context.sources->a, fixture.context,
      *fixture.precision, capabilities);
  return result;
}

void test_empty_and_isolated() {
  fv_surface_mesh<double, std::uint32_t> empty;
  auto accepted = validate_a(empty);
  require(accepted.has_value(), "empty operand accepted");
  require((*accepted.value())->vertices().empty() &&
              (*accepted.value())->shells().empty(),
          "empty artifact");
  empty.vertices = {{1, 2, 3}};
  auto isolated = validate_a(empty);
  require(isolated.has_value() && (*isolated.value())->vertices().empty(),
          "isolated vertices are non-semantic");
  require((*accepted.value())->canonical_bytes() ==
              (*isolated.value())->canonical_bytes(),
          "isolated vertices do not alter semantic bytes");
}

void test_topology_and_normalization() {
  auto mesh = tetra<double, std::uint32_t>();
  mesh.faces[0] = {0, 0, 2, 1, 0};
  auto result = validate_a(mesh);
  if (!result.has_value())
    throw std::runtime_error(result.error()->summary);
  const auto &a = **result.value();
  require(a.vertices().size() == 4 && a.facets().size() == 4 &&
              a.edges().size() == 6 && a.directed_uses().size() == 12,
          "tetra incidence counts");
  require(a.vertex_links().size() == 4 && a.shells().size() == 1,
          "tetra links and shell");
  require(a.certificate() ==
              bounded::input_certificate_disposition::nominal_embedded,
          "nominal certificate");
  require(a.bounded_vertices().size() == a.vertices().size() &&
              a.bounded_vertices()[0].lower[0] <= 0 &&
              a.bounded_vertices()[0].upper[0] >= 0,
          "Component 03 bounded source evidence retained");
  require(a.shells()[0].material_side == bounded::occupied_side::negative &&
              a.shells()[0].empty_side == bounded::occupied_side::positive,
          "occupied side convention");
  bool duplicate = false, closure = false;
  for (const auto &r : a.presentation_normalization()) {
    duplicate |=
        r.action == bounded::ring_position_action::consecutive_duplicate;
    closure |= r.action == bounded::ring_position_action::duplicate_closure;
  }
  require(duplicate && closure, "normalization actions retained");
}


void test_trailing_duplicate_closure_normalization() {
  auto mesh = tetra<double, std::uint32_t>();
  mesh.faces[0] = {0, 2, 1, 0, 0};
  auto result = validate_a(mesh);
  require(result.has_value(),
          "closing vertex followed by duplicates remains valid");
  const auto &records = (*result.value())->presentation_normalization();
  require(records.size() >= 5,
          "trailing duplicate normalization records retained");
  const auto &closure = records[3];
  const auto &trailing = records[4];
  require(closure.source_position == 3 &&
              closure.action ==
                  bounded::ring_position_action::duplicate_closure,
          "retained closing occurrence owns closure action");
  require(trailing.source_position == 4 &&
              trailing.action ==
                  bounded::ring_position_action::consecutive_duplicate,
          "trailing repeated occurrence remains a duplicate action");
  require(closure.canonical_facet == trailing.canonical_facet &&
              closure.retained_corner == trailing.retained_corner,
          "closure and trailing duplicate map to the retained first corner");
}

void test_structural_failures() {
  auto mesh = tetra<double, std::uint32_t>();
  mesh.faces.pop_back();
  auto open = validate_a(mesh);
  require(!open.has_value() &&
              open.error()->category ==
                  bounded_boolean_error_category::input_contract_error &&
              open.error()->subcode ==
                  static_cast<std::uint32_t>(
                      bounded::input_validation_subcode::open_boundary),
          "typed open-boundary failure");
  mesh = tetra<double, std::uint32_t>();
  mesh.faces[0][0] = 99;
  auto index = validate_a(mesh);
  require(!index.has_value() &&
              index.error()->subcode ==
                  static_cast<std::uint32_t>(
                      bounded::input_validation_subcode::out_of_range_index),
          "typed index failure");
  mesh = tetra<double, std::uint32_t>();
  std::reverse(mesh.faces[0].begin(), mesh.faces[0].end());
  auto orientation = validate_a(mesh);
  require(!orientation.has_value() &&
              orientation.error()->subcode ==
                  static_cast<std::uint32_t>(
                      bounded::input_validation_subcode::same_direction_pair),
          "typed orientation failure");
}

void test_presentation_invariance() {
  auto first = tetra<double, std::uint32_t>();
  auto a = validate_a(first);
  require(a.has_value(), "canonical baseline");
  auto second = first;
  std::rotate(second.faces[0].begin(), second.faces[0].begin() + 1,
              second.faces[0].end());
  std::reverse(second.faces.begin(), second.faces.end());
  auto b = validate_a(second);
  require(b.has_value() && (*a.value())->canonical_bytes() ==
                               (*b.value())->canonical_bytes(),
          "facet and ring permutation invariance");
  auto third = first;
  std::swap(third.vertices[0], third.vertices[3]);
  for (auto &face : third.faces)
    for (auto &i : face) {
      if (i == 0)
        i = 3;
      else if (i == 3)
        i = 0;
    }
  auto c = validate_a(third);
  require(c.has_value() && (*a.value())->canonical_bytes() ==
                               (*c.value())->canonical_bytes(),
          "vertex permutation invariance");
}

void test_precanonical_error_permutations() {
  fv_surface_mesh<double, std::uint32_t> mesh;
  append_tetra(mesh, 0., 0., 0., 1., false);
  mesh.faces.pop_back();
  append_tetra(mesh, 10., 0., 0., 1., false);
  std::reverse(mesh.faces[3].begin(), mesh.faces[3].end());
  auto baseline = validate_a(mesh);
  require(!baseline.has_value() &&
              baseline.error()->subcode == static_cast<std::uint32_t>(
                  bounded::input_validation_subcode::open_boundary),
          "canonical pre-canonical error baseline");
  std::reverse(mesh.faces.begin(), mesh.faces.end());
  std::vector<std::uint32_t> remap(mesh.vertices.size());
  auto old_vertices = mesh.vertices;
  for (std::uint32_t source = 0; source < remap.size(); ++source) {
    remap[source] = static_cast<std::uint32_t>(remap.size() - source - 1);
    mesh.vertices[remap[source]] = old_vertices[source];
  }
  for (auto &face : mesh.faces)
    for (auto &vertex : face)
      vertex = remap[vertex];
  auto permuted = validate_a(mesh);
  require(!permuted.has_value() &&
              permuted.error()->category == baseline.error()->category &&
              permuted.error()->checkpoint == baseline.error()->checkpoint &&
              permuted.error()->subcode == baseline.error()->subcode,
          "pre-canonical arbitration is source-order independent");
}

void test_codec_versions_and_long_double() {
  auto result = validate_a(tetra<double, std::uint32_t>());
  require(result.has_value(), "codec baseline");
  bounded::canonical_reader reader((*result.value())->canonical_bytes());
  std::uint32_t magic = 0;
  require(reader.u32(magic) && magic == 0x324f4759U,
          "validated operand codec magic");
  for (int version = 0; version < 20; ++version) {
    std::uint16_t value = 0;
    require(reader.u16(value) && value == 1,
            "validated operand codec version registry");
  }
  require(!(*result.value())->source_presentation_bytes().empty() &&
              (*result.value())->context_digest() != bounded_boolean_digest{},
          "source precision and owner-context links encoded");
  if (std::numeric_limits<long double>::digits >
      std::numeric_limits<double>::digits) {
    const long double first = 1.0L;
    const long double second = std::nextafter(first, 2.0L);
    require(static_cast<double>(first) == static_cast<double>(second),
            "long-double test values narrow equally");
    bounded::canonical_writer a, b;
    a.long_floating(first);
    b.long_floating(second);
    require(a.bytes() != b.bytes(), "long-double codec does not narrow");
  }
}

void test_public_gate() {
  auto mesh = tetra<double, std::uint32_t>();
  fv_surface_mesh<double, std::uint32_t> empty;
  bounded_boolean_options<double> options;
  options.tolerance = 1;
  auto result =
      bounded_boolean(mesh, empty, boolean_operation::set_union, options);
  require(!result.has_value() && result.error()->component == 4 &&
              result.error()->subcode == 2004,
          "public pipeline reaches Component 04 gate");
}

void test_relations_and_nesting() {
  bounded::validation_triangle<double> a{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
      b{{0, 0, 0}, {0, 1, 0}, {0, 0, 1}}, c{{0, 0, 0}, {0, 1, 0}, {1, 0, 0}};
  require(bounded::classify_triangle_relation(a, b) ==
              bounded::validation_relation::edge_contact,
          "triangle edge contact");
  require(bounded::classify_triangle_relation(a, c) ==
              bounded::validation_relation::whole_patch_coincidence,
          "triangle coincidence");
  fv_surface_mesh<double, std::uint32_t> nested;
  append_tetra(nested, 0., 0., 0., 10., false);
  append_tetra(nested, 1., 1., 1., 1., true);
  auto accepted = validate_a(nested);
  require(accepted.has_value(), "nested cavity accepted");
  require((*accepted.value())->shells().size() == 2 &&
              (*accepted.value())->shell_pairs().size() == 1,
          "nested shell records");
  bool cavity = false;
  for (const auto &s : (*accepted.value())->shells())
    cavity |= s.depth == 1 && s.parent >= 0 &&
              s.intrinsic_orientation == bounded::shell_orientation::inward;
  require(cavity, "cavity depth and orientation");
}

void test_six_level_nesting() {
  fv_surface_mesh<double, std::uint32_t> mesh;
  const std::array<double, 6> origin{{0, 8, 10, 10.5, 10.625, 10.65625}};
  const std::array<double, 6> size{{64, 16, 4, 1, .25, .0625}};
  for (std::size_t i = 0; i < origin.size(); ++i)
    append_tetra(mesh, origin[i], origin[i], origin[i], size[i], (i & 1) != 0);
  auto result = validate_a(mesh);
  if (!result.has_value())
    throw std::runtime_error(
        std::string(result.error()->summary) + " " +
        std::to_string(result.error()->witnesses[0]) + " " +
        std::to_string(result.error()->witnesses[1]) + " " +
        std::to_string(result.error()->witnesses[2]) + " " +
        std::to_string(result.error()->witnesses[3]));
  std::array<bool, 6> depths{};
  for (const auto &shell : (*result.value())->shells())
    if (shell.depth < depths.size())
      depths[shell.depth] = true;
  require(std::all_of(depths.begin(), depths.end(),
                      [](bool value) { return value; }),
          "all six nesting depths reconstructed");
}

void test_deterministic_presentation_fuzz() {
  auto baseline_mesh = tetra<double, std::uint32_t>();
  auto baseline = validate_a(baseline_mesh);
  require(baseline.has_value(), "fuzz baseline");
  std::uint32_t state = 0x91e10da5U;
  auto random = [&]() {
    state = state * 1664525U + 1013904223U;
    return state;
  };
  for (int iteration = 0; iteration < 12; ++iteration) {
    auto mesh = baseline_mesh;
    for (std::size_t i = mesh.faces.size(); i > 1; --i)
      std::swap(mesh.faces[i - 1], mesh.faces[random() % i]);
    for (auto &face : mesh.faces) {
      const auto shift = random() % face.size();
      std::rotate(face.begin(), face.begin() + shift, face.end());
    }
    auto result = validate_a(mesh);
    require(result.has_value() && (*result.value())->canonical_bytes() ==
                                      (*baseline.value())->canonical_bytes(),
            "deterministic presentation fuzz invariance");
  }
}

void test_adversarial_uncertainty_gap() {
  fv_surface_mesh<double, std::uint32_t> mesh;
  append_box(mesh, 0., 0., 0., 1., 1., 1.);
  append_box(mesh, 1.005, 0., 0., 2.005, 1., 1.);
  bounded_boolean_options<double> options;
  options.tolerance = .1;
  options.input_precision_a = .01;
  auto fixture = make_context_with_options(mesh, options);
  bounded::resource_manager resources(fixture.context.options.resources);
  bounded::input_validation_capabilities capabilities;
  capabilities.owner = fixture.context.owner;
  capabilities.resources = &resources;
  auto result = bounded::validate_operand(
      bounded::operand_id::a, fixture.context.sources->a, fixture.context,
      *fixture.precision, capabilities);
  require(
      !result.has_value() &&
          result.error()->subcode ==
              static_cast<std::uint32_t>(
                  bounded::input_validation_subcode::uncertainty_incompatible),
      "sub-precision shell gap rejected");
}

void test_exact_relation_oracle() {
  const std::array<std::array<double, 2>, 5> points{
      {{{0, 0}}, {{2, 0}}, {{0, 2}}, {{1, 1}}, {{-1, 3}}}};
  for (const auto &a : points)
    for (const auto &b : points)
      for (const auto &c : points) {
        exact::ExactPoint2 ea{{static_cast<std::int64_t>(a[0])},
                              {static_cast<std::int64_t>(a[1])}};
        exact::ExactPoint2 eb{{static_cast<std::int64_t>(b[0])},
                              {static_cast<std::int64_t>(b[1])}};
        exact::ExactPoint2 ec{{static_cast<std::int64_t>(c[0])},
                              {static_cast<std::int64_t>(c[1])}};
        require(bounded::exact_sign(bounded::exact_orient_2d(a, b, c)) ==
                    exact::orient2d(ea, eb, ec),
                "exact 2D relation oracle agreement");
      }
  exact::ExactPoint3 a{{0}, {0}, {0}}, b{{1}, {0}, {0}}, c{{0}, {1}, {0}},
      d{{0}, {0}, {1}};
  require(bounded::exact_sign(bounded::exact_orient_3d<double>(
              {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1})) ==
              exact::orient3d(a, b, c, d),
          "exact 3D relation oracle agreement");
}

void test_shell_contacts() {
  const std::array<std::array<double, 3>, 3> starts{
      {{1, 1, 1}, {1, 1, 0}, {1, 0, 0}}};
  const std::array<bounded::shell_pair_relation, 3> expected{
      {bounded::shell_pair_relation::authorized_point_contact,
       bounded::shell_pair_relation::authorized_edge_contact,
       bounded::shell_pair_relation::authorized_face_contact}};
  for (std::size_t i = 0; i < starts.size(); ++i) {
    fv_surface_mesh<double, std::uint32_t> mesh;
    append_box(mesh, 0., 0., 0., 1., 1., 1.);
    append_box(mesh, starts[i][0], starts[i][1], starts[i][2], starts[i][0] + 1,
               starts[i][1] + 1, starts[i][2] + 1);
    auto result = validate_a(mesh);
    if (!result.has_value())
      throw std::runtime_error(std::string("zero-volume shell contact ") +
                               std::to_string(i) + " " +
                               result.error()->summary + " " +
                               std::to_string(result.error()->witnesses[0]) +
                               " " +
                               std::to_string(result.error()->witnesses[1]) +
                               " " +
                               std::to_string(result.error()->witnesses[2]) +
                               " " +
                               std::to_string(result.error()->witnesses[3]));
    require((*result.value())->shell_pairs().size() == 1 &&
                (*result.value())->shell_pairs()[0].relation == expected[i],
            "shell contact dimension");
  }
}

void test_forbidden_interaction_and_automorphism() {
  fv_surface_mesh<double, std::uint32_t> overlap;
  append_box(overlap, 0., 0., 0., 2., 2., 2.);
  append_box(overlap, 1., 1., 1., 3., 3., 3.);
  auto rejected = validate_a(overlap);
  require(!rejected.has_value() &&
              (rejected.error()->category ==
                   bounded_boolean_error_category::
                       input_geometry_not_epsilon_valid ||
               rejected.error()->category ==
                   bounded_boolean_error_category::ambiguous_shell_semantics),
          "positive-volume shell overlap rejected");
  fv_surface_mesh<double, std::uint32_t> contact;
  append_box(contact, 0., 0., 0., 1., 1., 1.);
  append_box(contact, 1., 1., 1., 2., 2., 2.);
  auto baseline = validate_a(contact);
  require(baseline.has_value(), "automorphism baseline");
  std::reverse(contact.faces.begin(), contact.faces.end());
  std::vector<std::uint32_t> permutation(contact.vertices.size());
  for (std::uint32_t i = 0; i < permutation.size(); ++i)
    permutation[i] = static_cast<std::uint32_t>(permutation.size() - 1 - i);
  auto vertices = contact.vertices;
  for (std::size_t i = 0; i < vertices.size(); ++i)
    contact.vertices[permutation[i]] = vertices[i];
  for (auto &face : contact.faces)
    for (auto &v : face)
      v = permutation[v];
  auto permuted = validate_a(contact);
  require(permuted.has_value() && (*baseline.value())->canonical_bytes() ==
                                      (*permuted.value())->canonical_bytes(),
          "duplicate-coordinate contact automorphism invariance");
}

void test_precanonical_bow_tie_link() {
  fv_surface_mesh<double, std::uint32_t> mesh;
  mesh.vertices = {{0, 0, 0},  {1, 0, 0},  {0, 1, 0}, {0, 0, 1},
                   {-1, 0, 0}, {0, -1, 0}, {0, 0, -1}};
  mesh.faces = {{0, 2, 1}, {0, 1, 3}, {0, 3, 2}, {1, 2, 3},
                {0, 5, 4}, {0, 4, 6}, {0, 6, 5}, {4, 5, 6}};
  auto result = validate_a(mesh);
  require(!result.has_value() &&
              result.error()->subcode ==
                  static_cast<std::uint32_t>(bounded::input_validation_subcode::
                                                 vertex_link_multiple_cycles),
          "pre-canonical bow-tie source vertex rejected");
}

void test_uncertainty_and_shell_orientation_bounds() {
  auto small = tetra<double, std::uint32_t>();
  for (auto &v : small.vertices) {
    v.x *= .01;
    v.y *= .01;
    v.z *= .01;
  }
  bounded_boolean_options<double> options;
  options.tolerance = .2;
  options.input_precision_a = .02;
  auto fixture = make_context_with_options(small, options);
  bounded::resource_manager resources(fixture.context.options.resources);
  bounded::input_validation_capabilities caps;
  caps.owner = fixture.context.owner;
  caps.resources = &resources;
  auto rejected = bounded::validate_operand(
      bounded::operand_id::a, fixture.context.sources->a, fixture.context,
      *fixture.precision, caps);
  require(
      !rejected.has_value() &&
          (rejected.error()->subcode ==
               static_cast<std::uint32_t>(
                   bounded::input_validation_subcode::collapsed_geometry) ||
           rejected.error()->subcode ==
               static_cast<std::uint32_t>(bounded::input_validation_subcode::
                                              uncertainty_incompatible)),
      "uncertainty-collapsible geometry rejected");
  fv_surface_mesh<double, std::uint32_t> translated;
  append_tetra(translated, 1000000000., 1000000000., 1000000000., 16., false);
  auto accepted = validate_a(translated);
  require(accepted.has_value() &&
              (*accepted.value())->shells()[0].signed_volume >
                  (*accepted.value())->shells()[0].volume_uncertainty,
          "translated shell-local bounded volume accepted");
}

void test_coincidence_contact_parentage_and_adjacent_overlap() {
  fv_surface_mesh<double, std::uint32_t> coincident;
  append_box(coincident, 0., 0., 0., 1., 1., 1.);
  append_box(coincident, 0., 0., 0., 1., 1., 1.);
  auto duplicate = validate_a(coincident);
  require(!duplicate.has_value() &&
              duplicate.error()->category ==
                  bounded_boolean_error_category::ambiguous_shell_semantics,
          "whole-patch shell coincidence rejected");
  fv_surface_mesh<double, std::uint32_t> touching;
  append_tetra(touching, 0., 0., 0., 10., false);
  append_tetra(touching, 0., 1., 1., 1., true);
  auto contact = validate_a(touching);
  require(!contact.has_value() &&
              contact.error()->category ==
                  bounded_boolean_error_category::ambiguous_shell_semantics,
          "contacting shell excluded from containment parentage");
  std::array<double, 2> a{0, 0}, b{2, 0}, c{3, 0}, d{1, 0};
  require(bounded::input_relation_detail::segment_relation_2d(a, b, c, d) == 2,
          "adjacent collinear overlap detected beyond endpoint");
}

void test_uncertainty_boundaries() {
  const std::vector<std::array<double, 2>> triangle{{{0, 0}, {1, 0}, {0, 1}}};
  require(bounded::input_relation_detail::bounded_polygon_area_sign(triangle,
                                                                    0.0) == 1,
          "zero-radius orientation is definite");
  require(bounded::input_relation_detail::bounded_polygon_area_sign(triangle,
                                                                    0.5) == 0,
          "orientation uncertainty boundary fails closed");
  const bounded::validation_point3<double> a{0, 0, 0}, b{1, 0, 0},
      c{0, 1, 0}, above{0, 0, 1};
  require(bounded::input_relation_detail::bounded_orient3(a, b, c, above,
                                                          0.0) != 0,
          "zero-radius plane side is definite");
  require(bounded::input_relation_detail::bounded_orient3(a, b, c, above,
                                                          0.5) == 0,
          "plane-side uncertainty boundary fails closed");
}

void test_folded_vertex_star() {
  fv_surface_mesh<double, std::uint32_t> mesh;
  mesh.vertices = {{0, 0, 1},  {0, 0, -1}, {1, 0, 0},
                   {-1, 0, 0}, {0, 1, 0},  {0, -1, 0}};
  const std::array<std::uint32_t, 4> ring{{2, 3, 4, 5}};
  for (std::size_t i = 0; i < ring.size(); ++i) {
    const auto next = ring[(i + 1) % ring.size()];
    mesh.faces.push_back({0, next, ring[i]});
    mesh.faces.push_back({1, ring[i], next});
  }
  const auto result = validate_a(mesh);
  require(!result.has_value() &&
              result.error()->category ==
                  bounded_boolean_error_category::input_geometry_not_epsilon_valid,
          "topologically closed folded vertex star rejected geometrically");
}

void test_triangle_relation_matrix() {
  using triangle = bounded::validation_triangle<double>;
  const triangle base{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}};
  const std::array<std::pair<triangle, bounded::validation_relation>, 6> cases{{
      {triangle{{3, 0, 0}, {4, 0, 0}, {3, 1, 0}},
       bounded::validation_relation::definitely_disjoint},
      {triangle{{0, 0, 0}, {-1, 0, 1}, {0, -1, 1}},
       bounded::validation_relation::point_contact},
      {triangle{{0, 0, 0}, {2, 0, 0}, {0, 0, 1}},
       bounded::validation_relation::edge_contact},
      {triangle{{.25, -.25, -1}, {.25, .75, 1}, {.25, .75, -1}},
       bounded::validation_relation::transverse_intersection},
      {triangle{{.25, .25, 0}, {1.25, .25, 0}, {.25, 1.25, 0}},
       bounded::validation_relation::coplanar_positive_area_overlap},
      {triangle{{0, 2, 0}, {2, 0, 0}, {0, 0, 0}},
       bounded::validation_relation::whole_patch_coincidence},
  }};
  for (const auto &entry : cases) {
    const auto producer = bounded::classify_triangle_relation(base, entry.first);
    const auto verifier =
        bounded::input_geometry_verifier::triangle_relation(base, entry.first);
    require(producer == entry.second && verifier == entry.second,
            "producer/verifier triangle relation matrix");
  }
}

void test_float_u64_profile() {
  const auto result = validate_a(tetra<float, std::uint64_t>());
  require(result.has_value() && (*result.value())->vertices().size() == 4 &&
              (*result.value())->shells().size() == 1,
          "float/u64 bounded input-validation profile");
}

void test_resources_cancellation_and_mutation() {
  auto mesh = tetra<double, std::uint32_t>();
  auto fixture = make_context(mesh);
  resource_policy tiny = resource_policy::conservative_defaults();
  tiny.work_units = {1, 1};
  bounded::resource_manager limited(tiny);
  bounded::input_validation_capabilities caps;
  caps.owner = fixture.context.owner;
  caps.resources = &limited;
  auto exhausted = bounded::validate_operand(
      bounded::operand_id::a, fixture.context.sources->a, fixture.context,
      *fixture.precision, caps);
  require(!exhausted.has_value() &&
              exhausted.error()->category ==
                  bounded_boolean_error_category::resource_limit,
           "resource preflight failure");
  const auto failed_accounting = limited.snapshot();
  require(std::all_of(failed_accounting.begin(), failed_accounting.end(),
                      [](const auto &counter) {
                        return counter.reserved == 0 && counter.committed == 0;
                      }),
          "failed reservation set rolls back before publication");
  bounded_boolean_cancellation_source source;
  source.request_cancel(9);
  bounded::resource_manager resources(fixture.context.options.resources);
  caps.resources = &resources;
  auto token = source.token();
  caps.cancellation = &token;
  auto cancelled = bounded::validate_operand(
      bounded::operand_id::a, fixture.context.sources->a, fixture.context,
      *fixture.precision, caps);
  require(!cancelled.has_value() &&
              cancelled.error()->category ==
                  bounded_boolean_error_category::cancelled,
          "cancellation rollback");
  caps.cancellation = nullptr;
  bounded::resource_manager normal(fixture.context.options.resources);
  caps.resources = &normal;
  auto valid = bounded::validate_operand(
      bounded::operand_id::a, fixture.context.sources->a, fixture.context,
      *fixture.precision, caps);
  require(valid.has_value(), "mutation baseline");
  const auto accounting = normal.snapshot();
  const auto &temporary = accounting[static_cast<std::size_t>(
      bounded::resource_kind::temporary_bytes)];
  const auto &vertices = accounting[static_cast<std::size_t>(
      bounded::resource_kind::source_vertices)];
  const auto &relations =
      accounting[static_cast<std::size_t>(bounded::resource_kind::relations)];
  require(temporary.reserved == 0 && temporary.committed == 0,
          "temporary reservation fully released");
  require(vertices.reserved == 0 && vertices.committed == 4,
          "referenced vertex usage reconciled");
  require(relations.reserved == 0 &&
              relations.committed == (*valid.value())->relations().size(),
          "relation usage reconciled");
  auto corrupt = **valid.value();
  bounded::validated_operand_test_access::set_shell_depth(corrupt, 0, 3);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "independent depth mutation rejection");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_occupied_side(
      corrupt, 0, bounded::occupied_side::positive);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "independent occupied-side mutation rejection");
  if (!corrupt.relations().empty()) {
    corrupt = **valid.value();
    bounded::validated_operand_test_access::set_relation(
        corrupt, 0, bounded::validation_relation::transverse_intersection);
    require(!bounded::verify_validated_operand(
                corrupt, fixture.context.sources->a, *fixture.precision),
            "independent relation mutation rejection");
  }
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_wedge_embedded(corrupt, 0, false);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "independent wedge mutation rejection");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_star_degree(corrupt, 0, 99);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "independent star mutation rejection");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_wedge_facets(corrupt, 0,
                                                           {99, 98});
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "independent wedge incidence mutation rejection");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_persistent_bytes(corrupt, 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "independent resource evidence mutation rejection");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_directed_origin(corrupt, 0, 3);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "directed-use mutation rejected after re-encoding");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_edge_low(corrupt, 0, 99);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "edge-record mutation rejected after re-encoding");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_link_facet(corrupt, 0, 0, 99);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "vertex-link mutation rejected after re-encoding");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_shell_facet(corrupt, 0, 0, 99);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "shell-membership mutation rejected after re-encoding");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::collapse_decomposition_triangle(
      corrupt, 0, 0);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "decomposition coverage mutation rejected after re-encoding");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_vertex_id(corrupt, 0, 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "non-dense vertex ID mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_facet_id(corrupt, 0, 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "non-dense facet ID mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_edge_id(corrupt, 0, 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "non-dense edge ID mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_shell_id(corrupt, 0, 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "non-dense shell ID mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_presentation_facet(corrupt, 0,
                                                                  99);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "facet presentation correspondence mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_normalization_corner(corrupt, 0,
                                                                    99);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "source-position correspondence mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_support_plane(
      corrupt, 0, 0, corrupt.facets()[0].support_plane[0] + 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "support-plane mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_projection_axis(
      corrupt, 0, static_cast<std::uint8_t>((corrupt.facets()[0].dropped_axis + 1) % 3));
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "projection-axis mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_projected_area(
      corrupt, 0, corrupt.facets()[0].projected_area + 1);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "projected-area mutation rejected");
  require(!(*valid.value())->relations().empty(),
          "uncertainty relation mutation baseline");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::set_uncertainty_separated(
      corrupt, 0, !corrupt.relations()[0].uncertainty_separated);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "uncertainty-separated mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::clear_context_digest(corrupt);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "owner-context link mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::clear_source_digest(corrupt);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "source link mutation rejected");
  corrupt = **valid.value();
  bounded::validated_operand_test_access::clear_precision_digest(corrupt);
  require(!bounded::verify_validated_operand(
              corrupt, fixture.context.sources->a, *fixture.precision),
          "precision link mutation rejected");
}
} // namespace
int main() {
  try {
    bounded::floating_environment_guard guard;
    require(guard.qualified(), "strict floating environment");
    test_empty_and_isolated();
    test_topology_and_normalization();
    test_trailing_duplicate_closure_normalization();
    test_structural_failures();
    test_precanonical_bow_tie_link();
    test_presentation_invariance();
    test_precanonical_error_permutations();
    test_codec_versions_and_long_double();
    test_relations_and_nesting();
    test_six_level_nesting();
    test_shell_contacts();
    test_forbidden_interaction_and_automorphism();
    test_deterministic_presentation_fuzz();
    test_adversarial_uncertainty_gap();
    test_exact_relation_oracle();
    test_uncertainty_and_shell_orientation_bounds();
    test_coincidence_contact_parentage_and_adjacent_overlap();
    test_uncertainty_boundaries();
    test_folded_vertex_star();
    test_triangle_relation_matrix();
    test_float_u64_profile();
    test_resources_cancellation_and_mutation();
    test_public_gate();
    std::cout << "Component 02 input validation tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
