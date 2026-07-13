#pragma once
#include "MeshBooleanSelectionFixtures.h"
#include <YgorMeshesBooleanRealization.h>

namespace realization_test {
using namespace selection_test;

inline std::shared_ptr<verifier_registry> registry() {
  auto r = std::make_shared<verifier_registry>();
  for (auto c : {coordinate_tag::binary32, coordinate_tag::binary64})
    for (auto i : {index_tag::uint32, index_tag::uint64}) {
      require(register_input_topology_verifier(*r, c, i).has_value(), "input verifier");
      require(register_broad_phase_verifier(*r, c, i).has_value(), "broad verifier");
      require(register_intersection_events_verifier(*r, c, i).has_value(), "event verifier");
      require(register_symbolic_registry_verifier(*r, c, i).has_value(), "symbolic verifier");
      require(register_local_refinement_verifier(*r, c, i).has_value(), "local verifier");
      require(register_global_arrangement_verifier(*r, c, i).has_value(), "arrangement verifier");
      require(register_cell_classification_verifier(*r, c, i).has_value(), "classification verifier");
      require(register_boolean_selection_verifier(*r, c, i).has_value(), "selection verifier");
      require(register_geometry_realization_verifier(*r, c, i).has_value(), "realization verifier");
    }
  require(r->freeze().has_value(), "freeze");
  return r;
}

template <class T, class I, class Mutate>
void mutation_rejected(verifier_registry &r, boolean_context<T, I> &c,
                       const realized_boundary<T, I> &source, Mutate mutate,
                       const char *message) {
  auto changed = std::make_shared<realized_boundary<T, I>>(source);
  mutate(*changed);
  const auto type = realized_boundary_type_tag +
      (static_cast<std::uint64_t>(coordinate_type<T>()) << 8) +
      static_cast<std::uint64_t>(index_type<I>());
  auto spec = r.specification(artifact_slot::realized_boundary, type,
                              realized_boundary_schema,
                              verification_level::mandatory);
  require(spec.has_value(), "realization mutation specification");
  artifact_view view{c.owner(), artifact_slot::realized_boundary, type,
                     realized_boundary_schema, 1, changed->artifact_digest,
                     changed, changed.get()};
  verification_environment_view env{c.owner(), c.replay().setup,
      c.contract().selected_operation(), &c.options(), coordinate_type<T>(),
      index_type<I>(), &c.kernel(), {}, &c.accountant(), [&] { return c.cancelled(); }};
  auto checked = r.verify(view, spec.value(), env);
  if (!checked.has_value())
    throw std::runtime_error(std::string(message) + ": " + render_error(checked.error()));
  require(!checked.value().passed(), message);
}

template <class T, class I>
void realization_oracle(const realized_boundary<T, I> &a) {
  require(a.vertices.size() == a.selected->payload->vertices.size(), "selected vertex materialization");
  require(a.axis_domains.size() == 3 * a.vertices.size(), "axis domain coverage");
  require(a.halfedges.size() == 3 * a.triangles.size(), "triangle halfedge coverage");
  require(a.search.accepted_assignment && !a.search.exhausted, "accepted exhaustive search");
  require(a.certificate.vertices == a.vertices.size(), "vertex certificate count");
  require(a.certificate.triangles == a.triangles.size(), "triangle certificate count");
  require(a.certificate.obligations == a.obligations.size(), "obligation certificate count");
  for (const auto &v : a.vertices) {
    require(v.symbolic == a.selected->payload->vertices[v.selected.value_for_debug()].symbolic,
            "direct selected symbolic binding");
    const auto &symbolic = a.symbolic->payload->vertices[v.symbolic.value_for_debug()];
    require(v.exact_coordinate == symbolic.point, "exact coordinate binding");
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto &domain = a.axis_domains[3 * v.id.value_for_debug() + axis];
      require(domain.vertex == v.id && domain.axis == axis && !domain.values.empty(),
              "accepted domain binding");
      require(v.accepted_axis_rank[axis] < domain.values.size(), "accepted rank range");
      require(v.accepted_bits[axis].bits == domain.values[v.accepted_axis_rank[axis]].bits.bits,
              "accepted domain member");
    }
    if (v.preserved_source_bits)
      for (std::size_t axis = 0; axis < 3; ++axis)
        require(v.accepted_bits[axis].bits == (*v.preserved_source_bits)[axis].bits,
                "original source bits preserved");
  }
  for (const auto &o : a.obligations)
    require(o.expected == o.actual, "realization obligation discharged");
}
}
