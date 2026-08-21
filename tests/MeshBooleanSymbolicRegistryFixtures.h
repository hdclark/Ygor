#pragma once
#include "MeshBooleanInputTopologyFixtures.h"
#include <YgorMeshesBooleanSymbolicRegistry.h>
namespace symbolic_test {
using namespace ygor;
using namespace ygor::mesh_boolean;
using input_test::context;
using input_test::cube;
using input_test::require;
template <class T, class I>
auto context(fv_surface_mesh<T, I> &a, fv_surface_mesh<T, I> &b,
             std::shared_ptr<verifier_registry> r, const boolean_options &options,
             cancellation_source *cancel = nullptr) {
  std::shared_ptr<const exact_kernel_services<T>> k =
      std::make_shared<exact_kernel<T>>();
  std::shared_ptr<const verifier_service> v = r;
  auto c = make_boolean_context(a, b, operation::regularized_union, options, k,
                                v, cancel);
  require(c.has_value(), "context");
  return std::move(c.value());
}
inline std::shared_ptr<verifier_registry> registry() {
  auto r = std::make_shared<verifier_registry>();
  for (auto c : {coordinate_tag::binary32, coordinate_tag::binary64})
    for (auto i : {index_tag::uint32, index_tag::uint64}) {
      require(register_input_topology_verifier(*r, c, i).has_value(),
              "input verifier");
      require(register_broad_phase_verifier(*r, c, i).has_value(),
              "broad verifier");
      require(register_intersection_events_verifier(*r, c, i).has_value(),
              "event verifier");
      require(register_symbolic_registry_verifier(*r, c, i).has_value(),
              "symbolic verifier");
    }
  require(r->freeze().has_value(), "freeze");
  return r;
}
template <class T, class I, class X, class Y, class Z>
void translate(fv_surface_mesh<T, I> &m, X x, Y y, Z z) {
  for (auto &p : m.vertices) {
    p.x += static_cast<T>(x);
    p.y += static_cast<T>(y);
    p.z += static_cast<T>(z);
  }
}
} // namespace symbolic_test
