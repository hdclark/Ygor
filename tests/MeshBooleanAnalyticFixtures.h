#pragma once

#include "MeshBooleanOracles.h"
#include "MeshBooleanOutputFixtures.h"

namespace ygor::mesh_boolean::testing {

template <class T, class I>
std::shared_ptr<const published_artifact<assembled_output<T, I>>>
run_box_operation(T a_lo, T a_hi, T b_lo, T b_hi, operation op,
                  const boolean_options &options = {}) {
  auto a = input_test::box<T, I>(a_lo, a_hi);
  auto b = input_test::box<T, I>(b_lo, b_hi);
  auto context = classification_test::context(
      a, b, output_test::registry(), op, options);
  auto result = assemble_boolean_output_artifact(*context);
  if (!result.has_value())
    throw assertion_failure(render_error(result.error()));
  require(result.value()->report.passed(), "mandatory output report passes");
  output_test::output_oracle(*result.value()->payload);
  independently_check_closed_mesh(result.value()->payload->mesh);
  return result.value();
}

template <class T, class I>
std::vector<std::array<T, 3>> sorted_coordinates(const fv_surface_mesh<T, I> &mesh) {
  std::vector<std::array<T, 3>> result;
  result.reserve(mesh.vertices.size());
  for (const auto &v : mesh.vertices)
    result.push_back({v.x, v.y, v.z});
  std::sort(result.begin(), result.end());
  return result;
}

} // namespace ygor::mesh_boolean::testing
