#include "YgorMeshesBooleanVerification.h"
#include <algorithm>
#include <new>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {
bool ordered_unique(const std::vector<invariant_code> &v) {
  return std::adjacent_find(v.begin(), v.end(), [](auto a, auto b) {
           return static_cast<std::uint32_t>(a) >=
                  static_cast<std::uint32_t>(b);
         }) == v.end();
}
void encode_digest(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}
void encode_codes(canonical_encoder &e, const std::vector<invariant_code> &v) {
  e.u64(v.size());
  for (auto c : v)
    e.u32(static_cast<std::uint32_t>(c));
}
boolean_stage stage_for(artifact_slot s) {
  return s == artifact_slot::validated_operands
             ? boolean_stage::input_validation
             : s == artifact_slot::assembled_output
                   ? boolean_stage::output_assembly
                   : s == artifact_slot::final_verification
                         ? boolean_stage::final_verification
                         : boolean_stage::context_setup;
}
} // namespace

digest invariant_set_digest(const verification_spec &s) {
  canonical_encoder e;
  e.u16(s.schema);
  e.u16(s.checker_version);
  e.byte(static_cast<std::uint8_t>(s.level));
  e.byte(static_cast<std::uint8_t>(s.slot));
  e.u64(s.artifact_type_tag);
  e.u16(s.artifact_schema);
  encode_codes(e, s.required_invariants);
  return domain_digest({{'Y', 'G', 'B', 'I', 'N', 'V', '1', '3'}}, e.bytes());
}
status_or<std::vector<std::uint8_t>>
encode_evidence_record(const evidence_record &r) {
  canonical_encoder e;
  const char tag[] = "YGBEVD13";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(r.schema);
  e.byte(static_cast<std::uint8_t>(r.kind));
  e.u32(static_cast<std::uint32_t>(r.invariant));
  e.u64(r.entities.size());
  for (const auto &f : r.entities) {
    e.byte(static_cast<std::uint8_t>(f.index()));
    std::visit(
        [&](const auto &id) {
          using V = typename std::decay<decltype(id)>::type;
          if constexpr (std::is_same<V, original_vertex_ref>::value) {
            e.id(id.operand);
            e.id(id.vertex);
          } else if constexpr (std::is_same<V, facet_ref>::value) {
            e.id(id.operand);
            e.id(id.facet);
          } else
            e.id(id);
        },
        f);
  }
  e.byte_string(r.exact_payload);
  e.u64(r.dependencies.size());
  for (const auto &d : r.dependencies)
    encode_digest(e, d);
  return e.bytes();
}
digest evidence_digest(const evidence_record &r) {
  auto b = encode_evidence_record(r);
  return b.has_value()
             ? domain_digest({{'Y', 'G', 'B', 'E', 'V', 'D', '1', '3'}},
                             b.value())
             : digest{};
}

status_or<bool> verifier_registry::register_verifier(verifier_registration r) {
  if (frozen_)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::context_setup, "verifier_registry_frozen");
  if (!r.callback || !r.artifact_type_tag || !r.artifact_schema ||
      !r.checker_version || r.mandatory.empty() ||
      !ordered_unique(r.mandatory) || !ordered_unique(r.exhaustive) ||
      !std::includes(r.exhaustive.begin(), r.exhaustive.end(),
                     r.mandatory.begin(), r.mandatory.end()))
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup,
                      "invalid_verifier_registration");
  key k{r.slot, r.artifact_type_tag};
  if (entries_.count(k))
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup,
                      "duplicate_verifier_registration");
  entries_.emplace(k, std::move(r));
  return true;
}
status_or<bool> verifier_registry::freeze() {
  if (frozen_)
    return true;
  if (entries_.empty())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup, "empty_verifier_registry");
  canonical_encoder e;
  for (const auto &x : entries_) {
    e.byte(static_cast<std::uint8_t>(x.first.slot));
    e.u64(x.first.type);
    e.u16(x.second.artifact_schema);
    e.u16(x.second.checker_version);
    encode_codes(e, x.second.mandatory);
    encode_codes(e, x.second.exhaustive);
  }
  registry_digest_ =
      domain_digest({{'Y', 'G', 'B', 'R', 'E', 'G', '1', '3'}}, e.bytes());
  frozen_ = true;
  return true;
}
status_or<verification_spec>
verifier_registry::specification(artifact_slot slot, std::uint64_t type,
                                 std::uint16_t schema,
                                 verification_level level) const {
  if (!frozen_)
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup,
                      "verifier_registry_not_frozen");
  auto it = entries_.find({slot, type});
  if (it == entries_.end() || it->second.artifact_schema != schema)
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::context_setup, "verifier_not_registered");
  verification_spec s;
  s.checker_version = it->second.checker_version;
  s.level = level;
  s.slot = slot;
  s.artifact_type_tag = type;
  s.artifact_schema = schema;
  s.required_invariants = level == verification_level::mandatory
                              ? it->second.mandatory
                              : it->second.exhaustive;
  s.invariant_set_digest = invariant_set_digest(s);
  return s;
}
status_or<verification_report>
verifier_registry::verify(const artifact_view &v, const verification_spec &s,
                          const verification_environment_view &env) const
    noexcept {
  try {
    if (!frozen_)
      return make_error(boolean_error_code::internal_invariant_error,
                        stage_for(v.slot), "verifier_registry_not_frozen");
    auto it = entries_.find({v.slot, v.artifact_type_tag});
    if (it == entries_.end())
      return make_error(boolean_error_code::internal_invariant_error,
                        stage_for(v.slot), "verifier_not_registered");
    const auto &reg = it->second;
    const auto &expected = s.level == verification_level::mandatory
                               ? reg.mandatory
                               : reg.exhaustive;
    if (s.schema != 1 || s.checker_version != reg.checker_version ||
        s.slot != v.slot || s.artifact_type_tag != v.artifact_type_tag ||
        s.artifact_schema != v.artifact_schema ||
        s.required_invariants != expected ||
        s.invariant_set_digest != invariant_set_digest(s))
      return make_error(boolean_error_code::internal_invariant_error,
                        stage_for(v.slot), "verification_spec_binding");
    if (v.owner != env.owner || !v.payload || !v.lifetime)
      return make_error(boolean_error_code::internal_invariant_error,
                        stage_for(v.slot), "verification_environment_binding");
    if (env.cancelled && env.cancelled())
      return make_error(boolean_error_code::resource_limit, stage_for(v.slot),
                        "cancelled");
    auto out = reg.callback(v, s, env);
    if (!out.has_value())
      return out;
    auto valid = validate_verification_report(out.value());
    if (!valid.has_value())
      return valid.error();
    return out;
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit, stage_for(v.slot),
                      "verifier_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        stage_for(v.slot), "verifier_exception");
    x.detail = e.what();
    return x;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      stage_for(v.slot), "verifier_unknown_exception");
  }
}

status_or<std::vector<std::uint8_t>>
encode_verification_report(const verification_report &r) {
  canonical_encoder e;
  const char tag[] = "YGBVER13";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(r.schema);
  e.u16(r.checker_version);
  e.byte(static_cast<std::uint8_t>(r.stage));
  e.byte(static_cast<std::uint8_t>(r.slot));
  e.u64(r.artifact_type_tag);
  e.u16(r.artifact_schema);
  encode_digest(e, r.setup_digest);
  encode_digest(e, r.artifact_digest);
  encode_digest(e, r.invariant_set_digest);
  e.byte(static_cast<std::uint8_t>(r.outcome));
  e.u64(r.results.size());
  for (const auto &x : r.results) {
    e.u32(static_cast<std::uint32_t>(x.code));
    e.byte(static_cast<std::uint8_t>(x.status));
    e.u32(x.subcode);
    e.u64(x.entities.size());
    for (const auto &f : x.entities) {
      e.byte(static_cast<std::uint8_t>(f.index()));
      std::visit(
          [&](const auto &id) {
            using V = typename std::decay<decltype(id)>::type;
            if constexpr (std::is_same<V, original_vertex_ref>::value) {
              e.id(id.operand);
              e.id(id.vertex);
            } else if constexpr (std::is_same<V, facet_ref>::value) {
              e.id(id.operand);
              e.id(id.facet);
            } else
              e.id(id);
          },
          f);
    }
  }
  e.u64(r.evidence.size());
  for (const auto &v : r.evidence) {
    auto b = encode_evidence_record(v);
    if (!b.has_value())
      return b.error();
    e.byte_string(b.value());
    encode_digest(e, v.evidence_digest);
  }
  e.u64(r.dependency_digests.size());
  for (const auto &d : r.dependency_digests)
    encode_digest(e, d);
  e.boolean(r.replay.has_value());
  if (r.replay) {
    e.u16(r.replay->schema);
    e.byte(static_cast<std::uint8_t>(r.replay->slot));
    encode_digest(e, r.replay->artifact_digest);
    e.byte_string(r.replay->payload);
    encode_digest(e, r.replay->seed_digest);
  }
  return e.bytes();
}
status_or<bool> validate_verification_report(const verification_report &r) {
  if (r.schema != 1 || r.results.empty())
    return make_error(boolean_error_code::internal_invariant_error, r.stage,
                      "malformed_verification_report");
  bool failed = false;
  std::uint32_t prior = 0;
  for (const auto &x : r.results) {
    auto c = static_cast<std::uint32_t>(x.code);
    if (c <= prior)
      return make_error(boolean_error_code::internal_invariant_error, r.stage,
                        "unordered_verification_results");
    prior = c;
    if (x.status == check_status::failed) {
      if (failed)
        return make_error(boolean_error_code::internal_invariant_error, r.stage,
                          "multiple_causal_failures");
      failed = true;
    } else if (failed && x.status != check_status::not_run_due_to_prior_failure)
      return make_error(boolean_error_code::internal_invariant_error, r.stage,
                        "check_after_failure");
  }
  for (const auto &e : r.evidence)
    if (e.schema != 1 || e.evidence_digest != evidence_digest(e))
      return make_error(boolean_error_code::internal_invariant_error, r.stage,
                        "malformed_evidence");
  if ((r.outcome == verification_outcome::pass) != !failed)
    return make_error(boolean_error_code::internal_invariant_error, r.stage,
                      "verification_outcome_mismatch");
  auto bytes = encode_verification_report(r);
  if (!bytes.has_value())
    return bytes.error();
  auto d =
      domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}}, bytes.value());
  if (d != r.report_digest)
    return make_error(boolean_error_code::internal_invariant_error, r.stage,
                      "verification_report_digest");
  return true;
}

namespace {
struct byte_reader {
  const std::vector<std::uint8_t> &b;
  std::size_t p = 0;
  bool take(std::uint8_t &v) {
    if (p >= b.size())
      return false;
    v = b[p++];
    return true;
  }
  bool u16(std::uint16_t &v) {
    std::uint8_t a, c;
    return take(a) && take(c) &&
           (v = (static_cast<std::uint16_t>(a) << 8) | c, true);
  }
  bool u64(std::uint64_t &v) {
    v = 0;
    for (unsigned i = 0; i < 8; ++i) {
      std::uint8_t x;
      if (!take(x))
        return false;
      v = (v << 8) | x;
    }
    return true;
  }
  bool bytes(std::vector<std::uint8_t> &v) {
    std::uint64_t n;
    if (!u64(n) || n > b.size() - p)
      return false;
    v.assign(b.begin() + p, b.begin() + p + static_cast<std::size_t>(n));
    p += static_cast<std::size_t>(n);
    return true;
  }
  bool dg(digest &d) {
    if (b.size() - p < d.bytes.size())
      return false;
    std::copy(b.begin() + p, b.begin() + p + d.bytes.size(), d.bytes.begin());
    p += d.bytes.size();
    return true;
  }
};
void encode_dependency_node(canonical_encoder &e,
                            const verification_dependency_node &n) {
  e.u64(n.id);
  e.byte(static_cast<std::uint8_t>(n.kind));
  e.byte_string(n.key);
  e.boolean(n.diagnostic_only);
}
void encode_dependency_edge(canonical_encoder &e,
                            const verification_dependency_edge &x) {
  e.u64(x.from);
  e.u64(x.to);
  e.u16(x.relation);
  e.boolean(x.diagnostic_only);
}
} // namespace
status_or<std::vector<std::uint8_t>>
encode_dependency_slice(const dependency_slice &s) {
  canonical_encoder e;
  const char tag[] = "YGBSLI13";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(s.schema);
  e.byte(static_cast<std::uint8_t>(s.slot));
  encode_digest(e, s.artifact_digest);
  e.u64(s.nodes.size());
  for (const auto &n : s.nodes)
    encode_dependency_node(e, n);
  e.u64(s.edges.size());
  for (const auto &x : s.edges)
    encode_dependency_edge(e, x);
  return e.bytes();
}
status_or<dependency_slice>
decode_dependency_slice(const std::vector<std::uint8_t> &b) {
  byte_reader r{b};
  const char tag[] = "YGBSLI13";
  for (char c : tag) {
    if (!c)
      break;
    std::uint8_t x;
    if (!r.take(x) || x != static_cast<std::uint8_t>(c))
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "dependency_slice_tag");
  }
  dependency_slice s;
  std::uint8_t slot;
  if (!r.u16(s.schema) || s.schema != 1 || !r.take(slot) ||
      slot > static_cast<std::uint8_t>(artifact_slot::final_verification) ||
      !r.dg(s.artifact_digest))
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "dependency_slice_header");
  s.slot = static_cast<artifact_slot>(slot);
  std::uint64_t count;
  if (!r.u64(count) || count > b.size())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "dependency_slice_nodes");
  for (std::uint64_t i = 0; i < count; ++i) {
    verification_dependency_node n;
    std::uint8_t kind, diagnostic;
    if (!r.u64(n.id) || !r.take(kind) ||
        kind > static_cast<std::uint8_t>(dependency_node_kind::evidence) ||
        !r.bytes(n.key) || !r.take(diagnostic) || diagnostic > 1)
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "dependency_slice_node");
    n.kind = static_cast<dependency_node_kind>(kind);
    n.diagnostic_only = diagnostic;
    s.nodes.push_back(std::move(n));
  }
  if (!r.u64(count) || count > b.size())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "dependency_slice_edges");
  for (std::uint64_t i = 0; i < count; ++i) {
    verification_dependency_edge x;
    std::uint8_t diagnostic;
    if (!r.u64(x.from) || !r.u64(x.to) || !r.u16(x.relation) ||
        !r.take(diagnostic) || diagnostic > 1)
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "dependency_slice_edge");
    x.diagnostic_only = diagnostic;
    s.edges.push_back(x);
  }
  if (r.p != b.size())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "dependency_slice_trailing");
  auto encoded = encode_dependency_slice(s);
  s.slice_digest = domain_digest({{'Y', 'G', 'B', 'S', 'L', 'I', '1', '3'}},
                                 encoded.value());
  return s;
}
status_or<dependency_slice>
slice_dependencies(const verification_dependency_graph &g,
                   const std::vector<std::uint64_t> &roots,
                   resource_accountant *a) {
  if (g.schema != 1 || roots.empty())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification, "dependency_graph");
  std::map<std::uint64_t, verification_dependency_node> nodes;
  for (const auto &n : g.nodes)
    if (!nodes.emplace(n.id, n).second)
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "duplicate_dependency_node");
  std::set<std::uint64_t> keep(roots.begin(), roots.end());
  for (auto id : roots)
    if (!nodes.count(id))
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "unknown_dependency_root");
  for (bool changed = true; changed;) {
    changed = false;
    for (const auto &e : g.edges)
      if (keep.count(e.from) && !e.diagnostic_only && keep.insert(e.to).second)
        changed = true;
  }
  dependency_slice s;
  s.slot = g.slot;
  s.artifact_digest = g.artifact_digest;
  for (const auto &n : g.nodes)
    if (keep.count(n.id))
      s.nodes.push_back(n);
  for (const auto &e : g.edges)
    if (keep.count(e.from) && keep.count(e.to))
      s.edges.push_back(e);
  std::sort(s.nodes.begin(), s.nodes.end(),
            [](const auto &x, const auto &y) { return x.id < y.id; });
  std::sort(s.edges.begin(), s.edges.end(), [](const auto &x, const auto &y) {
    return std::tie(x.from, x.to, x.relation, x.diagnostic_only) <
           std::tie(y.from, y.to, y.relation, y.diagnostic_only);
  });
  if (a) {
    auto n = a->reserve(resource_kind::dependency_nodes, s.nodes.size(),
                        boolean_stage::final_verification);
    if (!n.has_value())
      return n.error();
    auto e = a->reserve(resource_kind::dependency_edges, s.edges.size(),
                        boolean_stage::final_verification);
    if (!e.has_value()) {
      a->release(resource_kind::dependency_nodes, s.nodes.size());
      return e.error();
    }
  }
  auto encoded = encode_dependency_slice(s);
  s.slice_digest = domain_digest({{'Y', 'G', 'B', 'S', 'L', 'I', '1', '3'}},
                                 encoded.value());
  return s;
}
status_or<std::vector<std::uint8_t>> encode_replay_seed(const replay_seed &s) {
  canonical_encoder e;
  const char tag[] = "YGBRPL13";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(s.schema);
  e.byte(static_cast<std::uint8_t>(s.slot));
  encode_digest(e, s.artifact_digest);
  e.byte_string(s.payload);
  return e.bytes();
}
status_or<replay_seed> decode_replay_seed(const std::vector<std::uint8_t> &b) {
  byte_reader r{b};
  const char tag[] = "YGBRPL13";
  for (char c : tag) {
    if (!c)
      break;
    std::uint8_t x;
    if (!r.take(x) || x != static_cast<std::uint8_t>(c))
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification, "replay_tag");
  }
  replay_seed s;
  std::uint8_t slot;
  if (!r.u16(s.schema) || s.schema != 1 || !r.take(slot) ||
      slot > static_cast<std::uint8_t>(artifact_slot::final_verification) ||
      !r.dg(s.artifact_digest) || !r.bytes(s.payload) || r.p != b.size())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification, "replay_seed");
  s.slot = static_cast<artifact_slot>(slot);
  auto encoded = encode_replay_seed(s);
  s.seed_digest = domain_digest({{'Y', 'G', 'B', 'R', 'P', 'L', '1', '3'}},
                                encoded.value());
  return s;
}
status_or<std::vector<std::uint8_t>>
encode_replay_archive(const verification_replay_archive &a) {
  canonical_encoder e;
  const char tag[] = "YGBARC13";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(a.schema);
  e.byte(static_cast<std::uint8_t>(a.coordinate));
  e.byte(static_cast<std::uint8_t>(a.index));
  encode_digest(e, a.setup_digest);
  encode_digest(e, a.artifact_digest);
  encode_digest(e, a.report_digest);
  e.byte_string(a.operand_a);
  e.byte_string(a.operand_b);
  e.byte_string(a.artifact);
  e.byte_string(a.report);
  e.u64(a.dependencies.size());
  for (const auto &d : a.dependencies)
    encode_digest(e, d);
  return e.bytes();
}
status_or<verification_replay_archive>
decode_replay_archive(const std::vector<std::uint8_t> &b,
                      resource_accountant *accountant) {
  if (accountant) {
    auto charged = accountant->reserve(resource_kind::replay_bytes, b.size(),
                                       boolean_stage::final_verification);
    if (!charged.has_value())
      return charged.error();
  }
  byte_reader r{b};
  const char tag[] = "YGBARC13";
  for (char c : tag) {
    if (!c)
      break;
    std::uint8_t x;
    if (!r.take(x) || x != static_cast<std::uint8_t>(c))
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "replay_archive_tag");
  }
  verification_replay_archive a;
  std::uint8_t coordinate, index;
  if (!r.u16(a.schema) || a.schema != 1 || !r.take(coordinate) ||
      coordinate > static_cast<std::uint8_t>(coordinate_tag::binary64) ||
      !r.take(index) || index > static_cast<std::uint8_t>(index_tag::uint64) ||
      !r.dg(a.setup_digest) || !r.dg(a.artifact_digest) ||
      !r.dg(a.report_digest) || !r.bytes(a.operand_a) ||
      !r.bytes(a.operand_b) || !r.bytes(a.artifact) || !r.bytes(a.report))
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "replay_archive_header");
  a.coordinate = static_cast<coordinate_tag>(coordinate);
  a.index = static_cast<index_tag>(index);
  std::uint64_t count;
  if (!r.u64(count) || count > b.size())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "replay_archive_dependencies");
  for (std::uint64_t i = 0; i < count; ++i) {
    digest d;
    if (!r.dg(d))
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::final_verification,
                        "replay_archive_dependency");
    a.dependencies.push_back(d);
  }
  if (r.p != b.size())
    return make_error(boolean_error_code::input_contract_error,
                      boolean_stage::final_verification,
                      "replay_archive_trailing");
  auto encoded = encode_replay_archive(a);
  a.archive_digest = domain_digest({{'Y', 'G', 'B', 'A', 'R', 'C', '1', '3'}},
                                   encoded.value());
  return a;
}
status_or<canonical_graph_result>
canonicalize_graph_exhaustive(const canonical_graph &g,
                              resource_accountant *accountant,
                              const std::function<bool()> &cancelled) {
  for (const auto &a : g.arcs)
    if (a.from >= g.nodes.size() || a.to >= g.nodes.size())
      return make_error(boolean_error_code::input_contract_error,
                        boolean_stage::input_validation, "canonical_graph_arc");
  std::vector<std::uint64_t> order(g.nodes.size());
  for (std::size_t i = 0; i < order.size(); ++i)
    order[i] = i;
  std::vector<std::uint64_t> colors(g.nodes.size());
  std::vector<std::vector<std::uint8_t>> distinct;
  for (const auto &node : g.nodes)
    distinct.push_back(node.initial_color);
  std::sort(distinct.begin(), distinct.end());
  distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
  for (std::size_t i = 0; i < g.nodes.size(); ++i)
    colors[i] = static_cast<std::uint64_t>(
        std::lower_bound(distinct.begin(), distinct.end(),
                         g.nodes[i].initial_color) -
        distinct.begin());
  for (;;) {
    std::vector<std::vector<std::uint8_t>> signatures(g.nodes.size());
    for (std::size_t i = 0; i < g.nodes.size(); ++i) {
      std::vector<std::tuple<std::uint8_t, std::uint16_t, std::uint64_t>> arcs;
      for (const auto &arc : g.arcs) {
        if (arc.from == i)
          arcs.emplace_back(0, arc.type, colors[arc.to]);
        if (arc.to == i)
          arcs.emplace_back(1, arc.type, colors[arc.from]);
      }
      std::sort(arcs.begin(), arcs.end());
      canonical_encoder signature;
      signature.u64(colors[i]);
      signature.u64(arcs.size());
      for (const auto &arc : arcs) {
        signature.byte(std::get<0>(arc));
        signature.u16(std::get<1>(arc));
        signature.u64(std::get<2>(arc));
      }
      signatures[i] = signature.bytes();
    }
    auto ranked = signatures;
    std::sort(ranked.begin(), ranked.end());
    ranked.erase(std::unique(ranked.begin(), ranked.end()), ranked.end());
    std::vector<std::uint64_t> next(colors.size());
    for (std::size_t i = 0; i < signatures.size(); ++i)
      next[i] = static_cast<std::uint64_t>(
          std::lower_bound(ranked.begin(), ranked.end(), signatures[i]) -
          ranked.begin());
    if (next == colors)
      break;
    colors.swap(next);
    if (accountant) {
      auto charged =
          accountant->reserve(resource_kind::work_units, g.nodes.size(),
                              boolean_stage::input_validation);
      if (!charged.has_value())
        return charged.error();
    }
    if (cancelled && cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::input_validation, "cancelled");
  }
  std::sort(order.begin(), order.end(), [&](auto x, auto y) {
    return colors[x] != colors[y] ? colors[x] < colors[y] : x < y;
  });
  std::vector<std::pair<std::size_t, std::size_t>> groups;
  for (std::size_t i = 0; i < order.size();) {
    std::size_t j = i + 1;
    while (j < order.size() && colors[order[i]] == colors[order[j]])
      ++j;
    groups.push_back({i, j});
    i = j;
  }
  canonical_graph_result best;
  bool have = false;
  boolean_error failure;
  bool failed = false;
  std::function<void(std::size_t)> visit = [&](std::size_t group) {
    if (failed)
      return;
    if (cancelled && cancelled()) {
      failure = make_error(boolean_error_code::resource_limit,
                           boolean_stage::input_validation, "cancelled");
      failed = true;
      return;
    }
    if (group < groups.size()) {
      auto range = groups[group];
      std::sort(order.begin() + range.first, order.begin() + range.second);
      do {
        visit(group + 1);
        if (failed)
          return;
      } while (std::next_permutation(order.begin() + range.first,
                                     order.begin() + range.second));
      return;
    }
    if (accountant) {
      auto charged = accountant->reserve(resource_kind::work_units, 1,
                                         boolean_stage::input_validation);
      if (!charged.has_value()) {
        failure = charged.error();
        failed = true;
        return;
      }
    }
    std::vector<std::uint64_t> label(order.size());
    for (std::size_t i = 0; i < order.size(); ++i)
      label[order[i]] = i;
    std::vector<std::tuple<std::uint64_t, std::uint64_t, std::uint16_t>> arcs;
    for (const auto &a : g.arcs)
      arcs.emplace_back(label[a.from], label[a.to], a.type);
    std::sort(arcs.begin(), arcs.end());
    canonical_encoder e;
    e.u64(order.size());
    for (auto source : order)
      e.byte_string(g.nodes[source].initial_color);
    e.u64(arcs.size());
    for (const auto &a : arcs) {
      e.u64(std::get<0>(a));
      e.u64(std::get<1>(a));
      e.u16(std::get<2>(a));
    }
    if (!have || e.bytes() < best.canonical_bytes ||
        (e.bytes() == best.canonical_bytes && order < best.source_by_label)) {
      best.canonical_bytes = e.bytes();
      best.source_by_label = order;
      have = true;
    }
  };
  visit(0);
  if (failed)
    return failure;
  if (!have && g.nodes.size())
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::input_validation,
                      "canonical_graph_no_leaf");
  return best;
}
} // namespace mesh_boolean
} // namespace ygor
