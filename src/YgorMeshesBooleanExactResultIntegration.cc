#include "YgorMeshesBooleanExactResult.h"
#include "YgorMeshesBooleanExactResultInternal.h"
#include "YgorMeshesBooleanOutput.h"

#include <algorithm>
#include <type_traits>

namespace ygor {
namespace mesh_boolean {
namespace {

product_error error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

product_error product_error_from_boolean(const boolean_error &source) {
  product_error_code code = product_error_code::internal_invariant_error;
  switch (source.code) {
  case boolean_error_code::input_contract_error:
    code = product_error_code::input_contract_error;
    break;
  case boolean_error_code::unsupported_platform:
    code = product_error_code::unsupported_platform;
    break;
  case boolean_error_code::resource_limit:
    code = product_error_code::resource_limit;
    break;
  case boolean_error_code::index_overflow:
    code = product_error_code::index_overflow;
    break;
  case boolean_error_code::result_topology_not_supported:
    code = product_error_code::result_topology_not_supported;
    break;
  case boolean_error_code::output_not_representable:
    code = product_error_code::output_not_representable;
    break;
  case boolean_error_code::internal_invariant_error:
    break;
  }
  auto result = make_product_error(code, source.message_key);
  result.subcode = source.subcode;
  result.detail = render_error(source);
  return result;
}

exact_feature_reference detach_feature(const feature_ref &f) {
  static_assert(std::variant_size<feature_ref>::value == 65,
                "feature_ref mapping changed");
  static_assert(
      std::is_same_v<std::variant_alternative_t<0, feature_ref>, operand_id>);
  static_assert(std::is_same_v<std::variant_alternative_t<42, feature_ref>,
                               selected_patch_id>);
  static_assert(std::is_same_v<std::variant_alternative_t<62, feature_ref>,
                               original_vertex_ref>);
  static_assert(
      std::is_same_v<std::variant_alternative_t<63, feature_ref>, facet_ref>);
  static_assert(std::is_same_v<std::variant_alternative_t<64, feature_ref>,
                               defining_relation_id>);
  exact_feature_reference out;
  out.kind = static_cast<std::uint16_t>(f.index());
  std::visit(
      [&](const auto &x) {
        using X = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<X, original_vertex_ref>) {
          out.primary = x.operand.value_for_debug();
          out.secondary = x.vertex.value_for_debug();
        } else if constexpr (std::is_same_v<X, facet_ref>) {
          out.primary = x.operand.value_for_debug();
          out.secondary = x.facet.value_for_debug();
        } else
          out.primary = x.value_for_debug();
      },
      f);
  return out;
}

} // namespace

template <class T, class I>
product_status_or<exact_stratified_boundary>
detach_exact_stratified_boundary(const selected_exact_boundary<T, I> &s,
                                 exact_result_backend_binding b,
                                 exact_result_preparation_binding p) {
  try {
    if (!selected_exact_boundary_has_canonical_encoding(s))
      return error(product_error_code::stale_binding,
                   "exact_result.selected_canonical_binding");
    if (s.preparation_provenance &&
        (p.mode != preparation_mode::strict_validation ||
         p.input_digest != s.preparation_provenance->input_digest ||
         p.prepared_digest != s.preparation_provenance->prepared_digest ||
         p.policy_digest != s.preparation_provenance->policy_digest ||
         p.report_digest != s.preparation_provenance->report_digest ||
         p.geometry_changed != s.preparation_provenance->geometry_changed))
      return error(product_error_code::stale_binding,
                   "exact_result.preparation_selection_binding");
    if (!s.owner.value_for_debug() || !s.arrangement ||
        !s.arrangement->payload || !s.labeled || !s.labeled->payload ||
        s.arrangement->owner != s.owner || s.labeled->owner != s.owner ||
        s.labeled->payload->arrangement.get() != s.arrangement.get() ||
        s.arrangement_digest != s.arrangement->artifact_digest ||
        s.labeled_digest != s.labeled->artifact_digest ||
        s.artifact_digest == digest{} || s.canonical_bytes.empty() ||
        s.certificate.semantic_digest !=
            domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '1', '0'}},
                          s.canonical_bytes))
      return error(product_error_code::stale_binding,
                   "exact_result.selected_binding");
    const auto &g = *s.arrangement->payload;
    if (!g.symbolic || !g.symbolic->payload || !g.validated ||
        !g.validated->payload || !s.constructions ||
        s.constructions->owner != s.owner)
      return error(product_error_code::stale_binding,
                   "exact_result.dependencies");
    const auto &sy = *g.symbolic->payload;
    exact_stratified_boundary a;
    a.selected_operation = s.selected_operation;
    a.topology = s.topology;
    a.backend = std::move(b);
    a.preparation = std::move(p);
    a.setup_digest = s.setup_digest;
    a.labeled_digest = s.labeled_digest;
    a.arrangement_digest = s.arrangement_digest;
    a.selected_artifact_digest = s.artifact_digest;
    a.selected_semantic_digest = s.certificate.semantic_digest;
    a.decisions = s.decisions;
    a.side_labels = s.labeled->payload->side_labels;
    a.vertex_occurrences = s.vertex_occurrences;
    a.edges = s.edges;
    a.halfedges = s.halfedges;
    a.cycles = s.cycles;
    a.patches = s.patches;
    a.topology_obstructions = s.topology_obstructions;
    a.certificate = s.certificate;
    const auto &validated = *g.validated->payload;
    for (const auto &v : s.vertices) {
      if (v.source.value_for_debug() >= g.vertices.size() ||
          v.symbolic.value_for_debug() >= sy.vertices.size())
        return error(product_error_code::stale_binding,
                     "exact_result.vertex_source");
      const auto &sv = sy.vertices[v.symbolic.value_for_debug()];
      if (sv.id != v.symbolic ||
          g.vertices[v.source.value_for_debug()].symbolic != v.symbolic)
        return error(product_error_code::stale_binding,
                     "exact_result.vertex_symbolic");
      std::vector<original_vertex_ref> originals;
      for (auto original : sv.original_vertices) {
        auto found = std::find_if(
            validated.vertices.begin(), validated.vertices.end(),
            [&](const auto &candidate) { return candidate.id == original; });
        if (found == validated.vertices.end())
          return error(product_error_code::stale_binding,
                       "exact_result.original_vertex_source");
        originals.push_back({found->operand, original});
      }
      std::sort(originals.begin(), originals.end(),
                [](const auto &x, const auto &y) {
                  return std::tie(x.operand, x.vertex) <
                         std::tie(y.operand, y.vertex);
                });
      a.vertices.push_back({v.id, v.source, v.symbolic, sv.point,
                            std::move(originals), sv.constructions});
    }
    for (const auto &e : s.edges) {
      if (e.source.value_for_debug() >= g.edges.size())
        return error(product_error_code::stale_binding,
                     "exact_result.edge_source");
      const auto &x = g.edges[e.source.value_for_debug()];
      a.edge_geometry.push_back({e.id, x.kind, x.curves});
    }
    for (const auto &x : s.patches) {
      if (x.source.value_for_debug() >= g.patches.size())
        return error(product_error_code::stale_binding,
                     "exact_result.patch_source");
      const auto &q = g.patches[x.source.value_for_debug()];
      a.patch_geometry.push_back({x.id, q.plane, q.projected_double_area});
      exact_result_patch_provenance pv;
      pv.patch = x.id;
      for (auto uid : x.provenance) {
        if (uid.value_for_debug() >= g.sheet_uses.size())
          return error(product_error_code::stale_binding,
                       "exact_result.provenance_source");
        const auto &u = g.sheet_uses[uid.value_for_debug()];
        pv.contributors.push_back({u.id, u.member, u.operand, u.shell, u.facet,
                                   u.source_plane_agrees, u.occupied_side,
                                   u.id == x.representative});
      }
      a.provenance.push_back(std::move(pv));
    }
    for (const auto &c : sy.curves)
      a.curves.push_back({c.id, c.kind, c.carrier, c.parent_carrier,
                          c.parameters, c.lower, c.upper, c.facets,
                          c.constructions});
    for (const auto &n : s.constructions->nodes) {
      exact_construction_record x;
      x.id = n.id;
      x.kind = n.kind;
      x.children = n.children;
      for (const auto &f : n.defining_sources)
        x.defining_sources.push_back(detach_feature(f));
      x.defining_relations = n.defining_relations;
      x.exact_result = n.exact_result;
      a.constructions.push_back(std::move(x));
    }
    for (const auto &d : s.constructions->relations) {
      exact_defining_relation_record x;
      x.id = d.id;
      x.kind = d.kind;
      x.formula_version = d.formula_version;
      x.construction = d.construction;
      x.operand_nodes = d.operand_nodes;
      for (const auto &f : d.defining_sources)
        x.defining_sources.push_back(detach_feature(f));
      x.coefficients = d.coefficients;
      x.expected = d.expected;
      a.defining_relations.push_back(std::move(x));
    }
    for (const auto &o : g.vertex_occurrences)
      a.source_vertex_occurrences.push_back({o.id, o.vertex, o.operand, o.shell,
                                             o.incident_halfedges,
                                             o.link_regions});
    for (const auto &r : g.link_rays)
      a.link_rays.push_back({r.id, r.direction, r.antipode});
    for (const auto &x : g.link_arcs)
      a.link_arcs.push_back(
          {x.id, x.occurrence, x.origin, x.destination, x.layers});
    for (const auto &x : g.vertex_sectors)
      a.vertex_sectors.push_back(
          {x.id, x.vertex, x.occurrence, x.region, x.germ, x.boundary_rays,
           x.boundary_arcs, x.witness_direction, x.witness_evidence,
           x.seam_continuations, x.source_edge_continuations});
    auto ok = detail::finalize_exact_stratified_boundary(a);
    if (!ok.has_value())
      return ok.error();
    return a;
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "exact_result.detach_exception");
  }
}

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
publish_exact_boolean_result(boolean_context<T, I> &context,
                             exact_result_backend_binding backend,
                             exact_result_preparation_binding preparation) {
  try {
    auto selected = select_boolean_boundary(context);
    if (!selected.has_value()) {
      auto failure =
          make_product_error(product_error_code::internal_invariant_error,
                             "exact_result.selection_failed");
      failure.detail = render_error(selected.error());
      return failure;
    }
    auto detached = detach_exact_stratified_boundary(*selected.value()->payload,
                                                     backend, preparation);
    if (!detached.has_value())
      return detached.error();
    auto frozen = freeze_exact_stratified_boundary(std::move(detached.value()));
    if (!frozen.has_value())
      return frozen.error();
    auto handle = make_exact_result_handle(frozen.value());
    if (!handle.has_value())
      return handle.error();
    auto authoritative = context.accountant().reserve_scoped(
        resource_kind::authoritative_bytes,
        handle.value()->canonical_bytes.size(),
        boolean_stage::final_verification);
    if (!authoritative.has_value()) {
      auto failure = make_product_error(product_error_code::resource_limit,
                                        "exact_result.authoritative_bytes");
      failure.detail = render_error(authoritative.error());
      return failure;
    }

    boolean_product_result<T, I> result;
    result.operation = operation_contract(frozen.value()->selected_operation);
    result.backend.schema = backend.schema;
    result.backend.producer = backend.producer;
    result.backend.selection = backend.selection;
    result.backend.fallback_used = backend.fallback_used;
    result.backend.attempted_backends = backend.attempted_backends;
    if (backend.primary_failure)
      result.backend.primary_failure = make_product_error(
          *backend.primary_failure, "exact_result.primary_failure");
    result.preparation.schema = preparation.schema;
    result.preparation.mode = preparation.mode;
    result.preparation.input_digest = preparation.input_digest;
    result.preparation.prepared_digest = preparation.prepared_digest;
    result.preparation.policy_digest = preparation.policy_digest;
    result.preparation.report_digest = preparation.report_digest;
    result.preparation.geometry_changed = preparation.geometry_changed;
    result.exact_result = std::move(handle.value());
    canonical_encoder report_binding;
    report_binding.u16(exact_stratified_boundary_checker_version);
    report_binding.raw(selected.value()->report.report_digest.bytes.data(),
                       selected.value()->report.report_digest.bytes.size());
    report_binding.raw(frozen.value()->canonical_digest.bytes.data(),
                       frozen.value()->canonical_digest.bytes.size());
    result.verification.passed = true;
    result.verification.verifier_set_version =
        exact_stratified_boundary_checker_version;
    result.verification.verifier_set_digest = domain_digest(
        {{'Y', 'G', 'B', 'E', 'X', 'V', '0', '1'}}, report_binding.bytes());
    result.verification.report_digest = selected.value()->report.report_digest;
    canonical_encoder attributes;
    attributes.raw(frozen.value()->canonical_digest.bytes.data(),
                   frozen.value()->canonical_digest.bytes.size());
    result.attributes.report_digest = domain_digest(
        {{'Y', 'G', 'B', 'A', 'T', 'R', '0', '1'}}, attributes.bytes());
    auto published = freeze_boolean_product_result(std::move(result));
    if (!published.has_value())
      return published.error();
    authoritative.value().commit();
    return published.value();
  } catch (const std::bad_alloc &) {
    return error(product_error_code::resource_limit, "exact_result.allocation");
  } catch (...) {
    return error(product_error_code::internal_invariant_error,
                 "exact_result.publish_exception");
  }
}

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
evaluate_boolean_product_result(boolean_context<T, I> &context,
                                exact_result_backend_binding backend,
                                 exact_result_preparation_binding preparation,
                                 result_representation representation) {
  if (const auto &bound = context.preparation_provenance()) {
    if (preparation.mode != preparation_mode::strict_validation ||
        preparation.input_digest != bound->input_digest ||
        preparation.prepared_digest != bound->prepared_digest ||
        preparation.policy_digest != bound->policy_digest ||
        preparation.report_digest != bound->report_digest ||
        preparation.geometry_changed != bound->geometry_changed)
      return error(product_error_code::stale_binding,
                   "exact_result.preparation_context_binding");
  }
  if (representation != result_representation::exact_stratified &&
      representation != result_representation::exact_in_T_mesh &&
      representation != result_representation::certified_approximate_mesh)
    return error(product_error_code::input_contract_error,
                 "exact_result.unknown_representation");
  auto exact = publish_exact_boolean_result(context, backend, preparation);
  if (!exact.has_value())
    return exact.error();
  if (representation == result_representation::exact_stratified)
    return exact.value();
  if (representation == result_representation::certified_approximate_mesh)
    return record_failed_realization(
        exact.value(), representation,
        product_realization_semantics::certified_approximate_embedding_v1,
        make_product_error(product_error_code::approximation_policy_rejected,
                           "exact_result.approximation_not_implemented"));

  auto assembled = assemble_boolean_output(context);
  if (!assembled.has_value())
    return record_failed_realization(
        exact.value(), representation,
        product_realization_semantics::exact_in_T,
        product_error_from_boolean(assembled.error()));

  auto result = *exact.value();
  result.representation = result_representation::exact_in_T_mesh;
  certified_mesh_payload<T, I> mesh;
  mesh.success = assembled.value();
  mesh.semantics = product_realization_semantics::exact_in_T;
  mesh.exact_result_digest = result.exact_result->canonical_digest;
  mesh.certificate.semantics = product_realization_semantics::exact_in_T;
  mesh.certificate.backend = result.backend.producer;
  mesh.certificate.exact_result_digest = result.exact_result->canonical_digest;
  canonical_encoder certificate;
  certificate.raw(result.exact_result->canonical_digest.bytes.data(),
                  result.exact_result->canonical_digest.bytes.size());
  certificate.raw(assembled.value()->canonical_output_digest.bytes.data(),
                  assembled.value()->canonical_output_digest.bytes.size());
  mesh.certificate.certificate_digest = domain_digest(
      {{'Y', 'G', 'B', 'E', 'X', 'M', '0', '1'}}, certificate.bytes());
  result.mesh = std::move(mesh);
  result.realization = realization_attempt_record{};
  result.realization->requested = result_representation::exact_in_T_mesh;
  result.realization->semantics = product_realization_semantics::exact_in_T;
  result.realization->succeeded = true;
  return freeze_boolean_product_result(std::move(result));
}

#define YGOR_EXACT_RESULT_DEFINE(T, I)                                         \
  template product_status_or<exact_stratified_boundary>                        \
  detach_exact_stratified_boundary(const selected_exact_boundary<T, I> &,      \
                                   exact_result_backend_binding,               \
                                   exact_result_preparation_binding);          \
  template product_status_or<boolean_product_result_handle<T, I>>              \
  publish_exact_boolean_result(boolean_context<T, I> &,                        \
                               exact_result_backend_binding,                   \
                               exact_result_preparation_binding);              \
  template product_status_or<boolean_product_result_handle<T, I>>              \
  evaluate_boolean_product_result(                                             \
      boolean_context<T, I> &, exact_result_backend_binding,                   \
      exact_result_preparation_binding, result_representation)
YGOR_EXACT_RESULT_DEFINE(float, std::uint32_t);
YGOR_EXACT_RESULT_DEFINE(float, std::uint64_t);
YGOR_EXACT_RESULT_DEFINE(double, std::uint32_t);
YGOR_EXACT_RESULT_DEFINE(double, std::uint64_t);
#undef YGOR_EXACT_RESULT_DEFINE

} // namespace mesh_boolean
} // namespace ygor
