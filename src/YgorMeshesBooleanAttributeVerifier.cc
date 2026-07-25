#include "YgorMeshesBooleanAttributes.h"
#include "YgorMeshesBooleanExactResult.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

product_error verification_error(const char *key) {
  return make_product_error(product_error_code::verifier_disagreement, key);
}
bool zero(const digest &d) noexcept { return d == digest{}; }

std::uint64_t global_entity_index(
    const std::array<attribute_source_catalog, 2> &sources, std::size_t role,
    std::uint64_t local) {
  return role == 0 ? local
                   : static_cast<std::uint64_t>(sources[0].entities.size()) +
                         local;
}
std::pair<std::size_t, std::uint64_t> local_entity_index(
    const std::array<attribute_source_catalog, 2> &sources,
    std::uint64_t global) {
  if (global < sources[0].entities.size())
    return {0, global};
  return {1, global - sources[0].entities.size()};
}
std::pair<std::size_t, std::uint64_t> local_value_index(
    const std::array<attribute_source_catalog, 2> &sources,
    std::uint64_t global) {
  if (global < sources[0].values.size())
    return {0, global};
  return {1, global - sources[0].values.size()};
}
const attribute_source_value_record &value_at(
    const std::array<attribute_source_catalog, 2> &sources,
    std::uint64_t global) {
  const auto local = local_value_index(sources, global);
  return sources[local.first].values.at(
      static_cast<std::size_t>(local.second));
}

void encode_raw_ref(canonical_encoder &e, const attribute_raw_entity_ref &r) {
  e.id(r.operand);
  e.byte(static_cast<std::uint8_t>(r.kind));
  e.u64(r.primary);
  e.u64(r.secondary);
}
std::vector<std::uint8_t> value_digest_bytes(
    attribute_channel_kind channel, const std::string &name,
    const std::vector<std::uint8_t> &value) {
  canonical_encoder e;
  e.byte(static_cast<std::uint8_t>(channel));
  e.string(name);
  e.byte_string(value);
  return e.bytes();
}
std::vector<std::uint8_t> source_set_bytes(
    std::vector<std::uint64_t> entities,
    const std::array<attribute_source_catalog, 2> &sources) {
  std::sort(entities.begin(), entities.end());
  entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
  canonical_encoder e;
  e.u64(entities.size());
  for (const auto global : entities) {
    const auto local = local_entity_index(sources, global);
    encode_raw_ref(e, sources[local.first]
                          .entities.at(static_cast<std::size_t>(local.second))
                          .source);
  }
  return e.bytes();
}

bool verify_catalog(const attribute_source_catalog &catalog) {
  if (catalog.schema != attribute_source_input_schema ||
      catalog.operand.value_for_debug() > 1 || catalog.body_id.empty() ||
      !std::is_sorted(catalog.entities.begin(), catalog.entities.end(),
                      [](const auto &a, const auto &b) {
                        return a.source < b.source;
                      }) ||
      !std::is_sorted(catalog.values.begin(), catalog.values.end(),
                      [](const auto &a, const auto &b) {
                        return std::tie(a.source_entity, a.channel, a.name,
                                        a.value) <
                               std::tie(b.source_entity, b.channel, b.name,
                                        b.value);
                      }))
    return false;
  canonical_encoder e;
  e.u16(catalog.schema);
  e.id(catalog.operand);
  e.string(catalog.body_id);
  e.u64(catalog.entities.size());
  for (const auto &entity : catalog.entities) {
    if (entity.source.operand != catalog.operand)
      return false;
    encode_raw_ref(e, entity.source);
    e.u64(entity.prepared_id);
    e.boolean(entity.retained);
  }
  e.u64(catalog.values.size());
  for (const auto &value : catalog.values) {
    if (value.source_entity >= catalog.entities.size() ||
        value.value_digest !=
            domain_digest({{'Y', 'G', 'B', 'A', 'V', 'A', 'L', '1'}},
                          value_digest_bytes(value.channel, value.name,
                                             value.value)))
      return false;
    e.u64(value.source_entity);
    e.byte(static_cast<std::uint8_t>(value.channel));
    e.string(value.name);
    e.byte_string(value.value);
    e.raw(value.value_digest.bytes.data(), value.value_digest.bytes.size());
  }
  return catalog.catalog_digest ==
         domain_digest({{'Y', 'G', 'B', 'A', 'C', 'A', 'T', '1'}},
                       e.bytes());
}

std::vector<std::uint64_t> entities_for_prepared(
    const std::array<attribute_source_catalog, 2> &sources, operand_id operand,
    attribute_source_entity_kind kind, std::uint64_t prepared) {
  const auto role = static_cast<std::size_t>(operand.value_for_debug());
  std::vector<std::uint64_t> out;
  if (role > 1)
    return out;
  for (std::size_t i = 0; i < sources[role].entities.size(); ++i) {
    const auto &entity = sources[role].entities[i];
    if (entity.retained && entity.source.kind == kind &&
        entity.prepared_id == prepared)
      out.push_back(global_entity_index(sources, role, i));
  }
  return out;
}
const attribute_exact_entity_mapping *find_mapping(
    const attribute_transfer_report &report, attribute_target_kind target,
    std::uint64_t id) {
  auto it = std::lower_bound(
      report.exact_mappings.begin(), report.exact_mappings.end(),
      std::make_pair(target, id), [](const auto &m, const auto &key) {
        return std::make_pair(m.target, m.target_id) < key;
      });
  return it != report.exact_mappings.end() && it->target == target &&
                 it->target_id == id
             ? &*it
             : nullptr;
}

bool same_sources(std::vector<std::uint64_t> a,
                  std::vector<std::uint64_t> b) {
  std::sort(a.begin(), a.end());
  a.erase(std::unique(a.begin(), a.end()), a.end());
  std::sort(b.begin(), b.end());
  b.erase(std::unique(b.begin(), b.end()), b.end());
  return a == b;
}

bool verify_mapping_sources(const attribute_transfer_report &report,
                            const exact_stratified_boundary &boundary) {
  if (report.exact_mappings.size() != boundary.vertices.size() +
                                          boundary.edges.size() +
                                          boundary.patches.size())
    return false;
  for (const auto &vertex : boundary.vertices) {
    const auto *mapping = find_mapping(report, attribute_target_kind::exact_vertex,
                                       vertex.id.value_for_debug());
    if (!mapping)
      return false;
    std::vector<std::uint64_t> expected;
    canonical_encoder provenance;
    provenance.u64(vertex.id.value_for_debug());
    for (const auto &source : vertex.original_vertices) {
      auto found = entities_for_prepared(
          report.sources, source.operand,
          attribute_source_entity_kind::vertex,
          source.vertex.value_for_debug());
      expected.insert(expected.end(), found.begin(), found.end());
      provenance.id(source.operand);
      provenance.id(source.vertex);
    }
    for (const auto construction : vertex.constructions)
      provenance.id(construction);
    if (!same_sources(mapping->source_entities, expected) ||
        mapping->provenance_digest !=
            domain_digest({{'Y', 'G', 'B', 'A', 'V', 'P', 'R', '1'}},
                          provenance.bytes()))
      return false;
  }
  for (std::size_t i = 0; i < boundary.edges.size(); ++i) {
    const auto &edge = boundary.edges[i];
    const auto *mapping = find_mapping(report, attribute_target_kind::exact_edge,
                                       edge.id.value_for_debug());
    if (!mapping || i >= boundary.edge_geometry.size())
      return false;
    std::vector<std::uint64_t> expected;
    canonical_encoder provenance;
    provenance.u64(edge.id.value_for_debug());
    for (const auto &source : boundary.edge_geometry[i].contributors) {
      auto found = entities_for_prepared(
          report.sources, source.operand, attribute_source_entity_kind::edge,
          source.edge.value_for_debug());
      expected.insert(expected.end(), found.begin(), found.end());
      provenance.id(source.operand);
      provenance.id(source.shell);
      provenance.id(source.edge);
      for (const auto facet : source.facets)
        provenance.id(facet);
    }
    if (!same_sources(mapping->source_entities, expected) ||
        mapping->provenance_digest !=
            domain_digest({{'Y', 'G', 'B', 'A', 'E', 'P', 'R', '1'}},
                          provenance.bytes()))
      return false;
  }
  for (const auto &patch : boundary.provenance) {
    const auto *mapping = find_mapping(report, attribute_target_kind::exact_patch,
                                       patch.patch.value_for_debug());
    if (!mapping)
      return false;
    std::vector<std::uint64_t> expected;
    canonical_encoder provenance;
    provenance.u64(patch.patch.value_for_debug());
    for (const auto &source : patch.contributors) {
      for (const auto &kind_and_id : {
               std::make_pair(attribute_source_entity_kind::body,
                              std::uint64_t(0)),
               std::make_pair(attribute_source_entity_kind::shell,
                              source.shell.value_for_debug()),
               std::make_pair(attribute_source_entity_kind::facet,
                              source.facet.value_for_debug())}) {
        auto found = entities_for_prepared(report.sources, source.operand,
                                           kind_and_id.first,
                                           kind_and_id.second);
        expected.insert(expected.end(), found.begin(), found.end());
      }
      provenance.id(source.operand);
      provenance.id(source.shell);
      provenance.id(source.facet);
      provenance.boolean(source.representative);
    }
    if (!same_sources(mapping->source_entities, expected) ||
        mapping->provenance_digest !=
            domain_digest({{'Y', 'G', 'B', 'A', 'P', 'P', 'R', '1'}},
                          provenance.bytes()))
      return false;
  }
  return true;
}

bool verify_transfer(const attribute_transfer_report &report,
                     const attribute_transfer_record &transfer,
                     const attribute_exact_entity_mapping &mapping) {
  for (const auto source : transfer.source_values) {
    const auto local = local_value_index(report.sources, source);
    if (local.first > 1 ||
        local.second >= report.sources[local.first].values.size())
      return false;
    const auto entity = global_entity_index(
        report.sources, local.first,
        report.sources[local.first]
            .values[static_cast<std::size_t>(local.second)]
            .source_entity);
    if (!std::binary_search(mapping.source_entities.begin(),
                            mapping.source_entities.end(), entity))
      return false;
  }
  if (transfer.value_digest !=
      domain_digest({{'Y', 'G', 'B', 'A', 'V', 'A', 'L', '1'}},
                    value_digest_bytes(transfer.channel, transfer.name,
                                       transfer.value)))
    return false;
  if (transfer.resolution ==
      attribute_resolution_kind::compact_construction)
    return transfer.value ==
           std::vector<std::uint8_t>(mapping.provenance_digest.bytes.begin(),
                                     mapping.provenance_digest.bytes.end());
  if (transfer.channel == attribute_channel_kind::source_body_id ||
      transfer.channel == attribute_channel_kind::source_shell_id ||
      transfer.channel == attribute_channel_kind::source_facet_id) {
    const auto expected_kind =
        transfer.channel == attribute_channel_kind::source_body_id
            ? attribute_source_entity_kind::body
        : transfer.channel == attribute_channel_kind::source_shell_id
            ? attribute_source_entity_kind::shell
            : attribute_source_entity_kind::facet;
    std::vector<std::uint64_t> entities;
    for (const auto source : mapping.source_entities) {
      const auto local = local_entity_index(report.sources, source);
      if (local.first > 1 ||
          local.second >= report.sources[local.first].entities.size())
        return false;
      if (report.sources[local.first]
              .entities[static_cast<std::size_t>(local.second)]
              .source.kind == expected_kind)
        entities.push_back(source);
    }
    return transfer.value == source_set_bytes(entities, report.sources);
  }
  if (transfer.source_values.empty())
    return false;
  if (transfer.resolution == attribute_resolution_kind::copied ||
      transfer.resolution == attribute_resolution_kind::split_copy ||
      transfer.resolution == attribute_resolution_kind::merged_equal) {
    return std::all_of(transfer.source_values.begin(),
                       transfer.source_values.end(), [&](auto source) {
                         return value_at(report.sources, source).value ==
                                transfer.value;
                       });
  }
  if (transfer.resolution ==
      attribute_resolution_kind::representative_copy)
    return std::any_of(transfer.source_values.begin(),
                       transfer.source_values.end(), [&](auto source) {
                         return value_at(report.sources, source).value ==
                                transfer.value;
                       });
  if (transfer.resolution == attribute_resolution_kind::set_union) {
    std::vector<std::vector<std::uint8_t>> values;
    for (const auto source : transfer.source_values)
      values.push_back(value_at(report.sources, source).value);
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    canonical_encoder union_bytes;
    union_bytes.u64(values.size());
    for (const auto &value : values)
      union_bytes.byte_string(value);
    return transfer.value == union_bytes.bytes();
  }
  if (transfer.resolution == attribute_resolution_kind::any_source) {
    bool any = false;
    for (const auto source : transfer.source_values) {
      const auto &value = value_at(report.sources, source).value;
      if (value.size() != 1 || value.front() > 1)
        return false;
      any = any || value.front() != 0;
    }
    return transfer.value ==
           std::vector<std::uint8_t>{static_cast<std::uint8_t>(any)};
  }
  return false;
}

bool issue_sources_valid(const attribute_transfer_report &report,
                         const attribute_issue_record &issue) {
  for (const auto source : issue.source_entities) {
    const auto local = local_entity_index(report.sources, source);
    if (local.first > 1 ||
        local.second >= report.sources[local.first].entities.size())
      return false;
  }
  for (const auto source : issue.source_values) {
    const auto local = local_value_index(report.sources, source);
    if (local.first > 1 ||
        local.second >= report.sources[local.first].values.size())
      return false;
  }
  if (issue.kind == attribute_issue_kind::conflict &&
      issue.source_values.empty())
    return false;
  return true;
}

bool verify_output(const attribute_transfer_report &report,
                   const attribute_output_binding *output) {
  if (!output)
    return report.output_mappings.empty() && !report.output_digest;
  if (!report.output_digest || *report.output_digest != output->output_digest ||
      report.output_mappings.size() !=
          output->output_vertex_exact_vertices.size() +
              output->output_face_exact_patches.size())
    return false;
  for (const auto &mapping : report.output_mappings) {
    std::uint64_t expected = attribute_unmapped_id;
    attribute_target_kind exact = attribute_target_kind::exact_vertex;
    if (mapping.target == attribute_target_kind::output_vertex &&
        mapping.output_id < output->output_vertex_exact_vertices.size()) {
      expected = output->output_vertex_exact_vertices[mapping.output_id];
      exact = attribute_target_kind::exact_vertex;
    } else if (mapping.target == attribute_target_kind::output_face &&
               mapping.output_id < output->output_face_exact_patches.size()) {
      expected = output->output_face_exact_patches[mapping.output_id];
      exact = attribute_target_kind::exact_patch;
    } else {
      return false;
    }
    const auto *source = find_mapping(report, exact, expected);
    if (!source || mapping.exact_target != exact ||
        mapping.exact_id != expected || mapping.transfers != source->transfers)
      return false;
  }
  return true;
}

} // namespace

product_status_or<bool> verify_attribute_transfer_report(
    const attribute_transfer_report &report, const exact_result_handle &exact,
    const attribute_output_binding *output) noexcept {
  try {
    if (!exact.valid() || report.schema != attribute_transfer_report_schema ||
        report.checker_version != attribute_transfer_checker_version ||
        report.policy.schema != product_contract_schema_version ||
        report.policy_digest !=
            attribute_transfer_policy_digest(report.policy) ||
        report.exact_result_digest != exact->canonical_digest ||
        zero(report.report_digest) || report.canonical_bytes.empty())
      return verification_error("attribute_report.binding");
    if (!verify_catalog(report.sources[0]) ||
        !verify_catalog(report.sources[1]) ||
        report.sources[0].operand != operand_a() ||
        report.sources[1].operand != operand_b())
      return verification_error("attribute_report.catalog");
    auto boundary = read_exact_result(exact);
    if (!boundary.has_value())
      return boundary.error();
    if (!verify_mapping_sources(report, *boundary.value()))
      return verification_error("attribute_report.mapping_sources");
    if (!std::is_sorted(report.exact_mappings.begin(),
                        report.exact_mappings.end(), [](const auto &a,
                                                       const auto &b) {
                          return std::tie(a.target, a.target_id) <
                                 std::tie(b.target, b.target_id);
                        }) ||
        !std::is_sorted(report.output_mappings.begin(),
                        report.output_mappings.end(), [](const auto &a,
                                                        const auto &b) {
                          return std::tie(a.target, a.output_id,
                                          a.exact_target, a.exact_id) <
                                 std::tie(b.target, b.output_id,
                                          b.exact_target, b.exact_id);
                        }) ||
        !std::is_sorted(report.transfers.begin(), report.transfers.end(),
                        [](const auto &a, const auto &b) {
                          return std::tie(a.target, a.target_id, a.channel,
                                          a.name, a.value, a.source_values) <
                                 std::tie(b.target, b.target_id, b.channel,
                                          b.name, b.value, b.source_values);
                        }))
      return verification_error("attribute_report.ordering");
    for (std::size_t i = 0; i < report.transfers.size(); ++i) {
      const auto &transfer = report.transfers[i];
      const auto *mapping =
          find_mapping(report, transfer.target, transfer.target_id);
      if (!mapping ||
          !std::binary_search(mapping->transfers.begin(),
                              mapping->transfers.end(), i) ||
          !verify_transfer(report, transfer, *mapping))
        return verification_error("attribute_report.transfer");
    }
    for (const auto &mapping : report.exact_mappings) {
      if (!std::is_sorted(mapping.source_entities.begin(),
                          mapping.source_entities.end()) ||
          std::adjacent_find(mapping.source_entities.begin(),
                             mapping.source_entities.end()) !=
              mapping.source_entities.end() ||
          !std::is_sorted(mapping.transfers.begin(),
                          mapping.transfers.end()))
        return verification_error("attribute_report.mapping_order");
      for (const auto transfer : mapping.transfers)
        if (transfer >= report.transfers.size() ||
            report.transfers[transfer].target != mapping.target ||
            report.transfers[transfer].target_id != mapping.target_id)
          return verification_error("attribute_report.mapping_transfer");
    }
    std::uint64_t omissions = 0, conflicts = 0;
    for (const auto &issue : report.issues) {
      if (!issue_sources_valid(report, issue))
        return verification_error("attribute_report.issue");
      if (issue.kind == attribute_issue_kind::omission)
        ++omissions;
      else
        ++conflicts;
    }
    if (omissions != report.omissions || conflicts != report.conflicts)
      return verification_error("attribute_report.issue_counts");
    if (report.policy.mode == attribute_transfer_mode::require_lossless &&
        (omissions != 0 || conflicts != 0))
      return verification_error("attribute_report.lossless");
    if (!verify_output(report, output))
      return verification_error("attribute_report.output");
    auto encoded = encode_attribute_transfer_report(report);
    if (!encoded.has_value() || encoded.value() != report.canonical_bytes ||
        report.report_digest !=
            domain_digest({{'Y', 'G', 'B', 'A', 'T', 'D', '0', '1'}},
                          report.canonical_bytes))
      return verification_error("attribute_report.canonical");
    return true;
  } catch (...) {
    return verification_error("attribute_report.exception");
  }
}

} // namespace mesh_boolean
} // namespace ygor
