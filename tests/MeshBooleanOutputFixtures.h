#pragma once
#include "MeshBooleanRealizationFixtures.h"
#include <YgorMeshesBooleanOutput.h>

namespace output_test {
using namespace realization_test;

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
      require(register_local_refinement_verifier(*r, c, i).has_value(),
              "local verifier");
      require(register_global_arrangement_verifier(*r, c, i).has_value(),
              "arrangement verifier");
      require(register_cell_classification_verifier(*r, c, i).has_value(),
              "classification verifier");
      require(register_boolean_selection_verifier(*r, c, i).has_value(),
              "selection verifier");
      require(register_geometry_realization_verifier(*r, c, i).has_value(),
              "realization verifier");
      require(register_boolean_output_verifier(*r, c, i).has_value(),
              "output verifier");
    }
  require(r->freeze().has_value(), "freeze");
  return r;
}

template <class T, class I, class Mutate>
void mutation_rejected(verifier_registry &r, boolean_context<T, I> &c,
                       const assembled_output<T, I> &source, Mutate mutate,
                       const char *message) {
  auto changed = std::make_shared<assembled_output<T, I>>(source);
  mutate(*changed);
  const auto type = assembled_output_type_tag +
                    (static_cast<std::uint64_t>(coordinate_type<T>()) << 8) +
                    static_cast<std::uint64_t>(index_type<I>());
  auto spec =
      r.specification(artifact_slot::assembled_output, type,
                      assembled_output_schema, verification_level::mandatory);
  require(spec.has_value(), "output mutation specification");
  artifact_view view{c.owner(), artifact_slot::assembled_output,
                     type,      assembled_output_schema,
                     1,         changed->artifact_digest,
                     changed,   changed.get()};
  verification_environment_view env{c.owner(),
                                    c.replay().setup,
                                    c.contract().selected_operation(),
                                    &c.options(),
                                    coordinate_type<T>(),
                                    index_type<I>(),
                                    &c.kernel(),
                                    {},
                                    &c.accountant(),
                                    [&] { return c.cancelled(); }};
  auto checked = r.verify(view, spec.value(), env);
  if (!checked.has_value())
    throw std::runtime_error(std::string(message) + ": " +
                             render_error(checked.error()));
  require(!checked.value().passed(), message);
}

template <class T, class I>
void output_oracle(const assembled_output<T, I> &a) {
  require(a.mesh.vertices.size() == a.vertices.size(),
          "vertex mapping coverage");
  require(a.mesh.faces.size() == a.faces.size(), "face mapping coverage");
  require(a.mesh.involved_faces.size() == a.mesh.vertices.size(),
          "reverse incidence size");
  require(a.mesh.vertex_normals.empty() && a.mesh.vertex_colours.empty() &&
              a.mesh.metadata.empty(),
          "empty optional public fields");
  std::vector<std::vector<I>> expected(a.mesh.vertices.size());
  for (std::size_t f = 0; f < a.mesh.faces.size(); ++f) {
    require(a.mesh.faces[f].size() == 3, "triangular public face");
    for (auto v : a.mesh.faces[f])
      expected[v].push_back(static_cast<I>(f));
  }
  require(expected == a.mesh.involved_faces, "complete reverse incidence");
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    require(a.vertices[i].id.value_for_debug() == i &&
                a.vertices[i].public_index == static_cast<I>(i),
            "dense first-use vertex mapping");
  require(a.certificate.vertices == a.mesh.vertices.size() &&
              a.certificate.faces == a.mesh.faces.size(),
          "certificate counts");
}
} // namespace output_test
