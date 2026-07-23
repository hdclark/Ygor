#include <YgorMeshesBooleanProductContract.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
using namespace ygor::mesh_boolean;
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static digest test_digest(const char suffix){canonical_encoder e;e.byte(static_cast<std::uint8_t>(suffix));return domain_digest({{'Y','G','B','T','E','S','T',suffix}},e.bytes());}
static backend_capabilities experimental_capabilities(){backend_capabilities c;c.set(backend_capability::exact_set_semantics);c.set(backend_capability::exact_coordinates);c.set(backend_capability::stratified_output);c.set(backend_capability::manifold_mesh_output);c.set(backend_capability::deterministic_canonical_output);c.set(backend_capability::certified_failure_categories);c.set(backend_capability::provenance_mapping);c.set(backend_capability::strict_prepared_operands);c.set(backend_capability::exact_in_T_output);return c;}
static backend_identity experimental_identity(){auto made=make_backend_identity(backend_id::experimental_exact_v1,{1,0,0},"test-build",experimental_capabilities(),backend_maturity::experimental);require(made.has_value(),"experimental identity");return made.value();}
static backend_provenance experimental_provenance(){backend_provenance p;p.producer=experimental_identity();p.selection=backend_selection_mode::explicit_backend;p.attempted_backends={backend_id::experimental_exact_v1};return p;}
static boolean_product_options explicit_experimental_options(){boolean_product_options o;o.backend.mode=backend_selection_mode::explicit_backend;o.backend.requested_backend=backend_id::experimental_exact_v1;o.backend.allow_experimental_backend=true;o.qualification.mode=qualification_policy_mode::allow_explicit_unqualified;return o;}
static void product_policy_round_trip() {
  boolean_product_options o;
  require(validate_product_options(o).has_value(),
          "default product options are a valid fail-closed request");
  auto encoded = encode_product_options(o);
  require(encoded.has_value(), "product options encode");
  auto decoded = decode_product_options(encoded.value());
  require(decoded.has_value(), "product options decode");
  require(product_options_digest(decoded.value()) == product_options_digest(o),
          "product options canonical round trip");

  auto unknown = encoded.value();
  require(unknown.size() > 30, "product option vector size");
  unknown[30] = 0xff;
  require(!decode_product_options(unknown).has_value(),
          "unknown selection enum rejected");
  auto trailing = encoded.value();
  trailing.push_back(0);
  require(!decode_product_options(trailing).has_value(),
          "trailing bytes rejected");
  product_decode_limits limits;
  limits.max_record_bytes = encoded.value().size() - 1;
  require(!decode_product_options(encoded.value(), limits).has_value(),
          "decode limit enforced before allocation");
}

static void product_policy_validation() {
  auto strict = boolean_product_options{};
  strict.preparation.normalization.mode = normalization_mode::diagnosis_only;
  require(!validate_product_options(strict).has_value(),
          "strict preparation cannot hide normalization");

  auto diagnosis = boolean_product_options{};
  diagnosis.preparation.mode = preparation_mode::diagnosis_only;
  diagnosis.preparation.normalization.mode = normalization_mode::diagnosis_only;
  require(validate_product_options(diagnosis).has_value(),
          "diagnosis-only preparation is non-mutating");

  auto normalized = boolean_product_options{};
  normalized.preparation.mode = preparation_mode::normalized;
  normalized.preparation.normalization.mode =
      normalization_mode::geometry_changing;
  normalized.preparation.normalization.unit = model_unit::millimetre;
  normalized.preparation.normalization.model_tolerance = 0.01;
  normalized.preparation.normalization.enabled_operations =
      normalization_operation_bit(normalization_operation::crack_closure);
  require(validate_product_options(normalized).has_value(),
          "explicit geometry-changing normalization contract");
  normalized.preparation.normalization.model_tolerance =
      std::numeric_limits<double>::quiet_NaN();
  require(!validate_product_options(normalized).has_value(),
          "non-finite normalization tolerance rejected");

  auto exact_t = boolean_product_options{};
  exact_t.result.representation = result_representation::exact_in_T_mesh;
  exact_t.realization.semantics = product_realization_semantics::exact_in_T;
  exact_t.realization.search.strategy = realization_search_strategy::nearest_only;
  require(validate_product_options(exact_t).has_value(),
          "strict exact-in-T result contract");
  exact_t.realization.search.strategy =
      realization_search_strategy::neighboring_values;
  exact_t.realization.search.max_candidates = 32;
  require(validate_product_options(exact_t).has_value(),
          "search policy does not change exact semantics");
  require(exact_t.result.representation ==
              result_representation::exact_in_T_mesh &&
              exact_t.realization.semantics ==
                  product_realization_semantics::exact_in_T,
          "search and semantic fields remain separate");

  auto approximate = boolean_product_options{};
  approximate.result.representation =
      result_representation::certified_approximate_mesh;
  approximate.realization.semantics =
      product_realization_semantics::certified_approximate_embedding_v1;
  approximate.realization.search.strategy =
      realization_search_strategy::deterministic_bounded_search;
  require(!validate_product_options(approximate).has_value(),
          "approximate output requires an explicit acceptance policy");
  approximate.realization.approximation.enabled = true;
  approximate.realization.approximation.unit = model_unit::millimetre;
  approximate.realization.approximation.max_vertex_displacement = 0.001;
  approximate.realization.approximation.declared_model_tolerance = 0.01;
  approximate.realization.search.max_candidates = 64;
  approximate.realization.search.max_backtracks = 256;
  approximate.realization.approximation.application_acceptance_metadata =
      "fixture-model-tolerance";
  require(validate_product_options(approximate).has_value(),
          "explicit approximate contract accepted");
  approximate.result.retain_exact_result_on_realization_failure = false;
  require(!validate_product_options(approximate).has_value(),
          "exact authority cannot be discarded");

  auto attributes = boolean_product_options{};
  attributes.attributes.mode = attribute_transfer_mode::require_lossless;
  require(!validate_product_options(attributes).has_value(),
          "lossless attributes require conflict rejection");
  attributes.attributes.conflicts = attribute_conflict_policy::reject;
  require(validate_product_options(attributes).has_value(),
          "lossless attribute contract with rejection");

  require(!fallback_permitted_for(product_error_code::internal_invariant_error),
          "invariant errors never fall back");
  require(!fallback_permitted_for(product_error_code::verifier_disagreement),
          "verifier disagreement never falls back");
  require(fallback_permitted_for(product_error_code::backend_unavailable),
          "declared backend unavailability may be listed for fallback");
}

static void backend_maturity_and_qualification() {
  const auto experimental = experimental_identity();
  auto explicit_options = explicit_experimental_options();
  require(authorize_backend(explicit_options, experimental).has_value(),
          "experimental backend requires and accepts explicit opt-in");
  explicit_options.backend.allow_experimental_backend = false;
  require(!authorize_backend(explicit_options, experimental).has_value(),
          "experimental backend is not silently selected");

  boolean_product_options default_options;
  require(!authorize_backend(default_options, experimental).has_value(),
          "qualified default rejects experimental backend");

  auto qualified_made = make_backend_identity(
      backend_id::experimental_exact_v1, {1, 0, 0}, "qualified-test-build",
      experimental_capabilities(), backend_maturity::qualified);
  require(qualified_made.has_value(), "qualified identity shape");
  const auto qualified = qualified_made.value();
  qualification_profile profile;
  profile.backend = qualified.id;
  profile.capability_digest = qualified.capability_digest;
  profile.representation = result_representation::exact_stratified;
  profile.preparation = preparation_mode::strict_validation;
  profile.workload_profile = "contract-fixture";
  auto manifest_made =
      make_qualification_manifest("contract-manifest", {profile});
  require(manifest_made.has_value(), "qualification manifest");
  auto manifest = manifest_made.value();
  default_options.qualification.workload_profile = "contract-fixture";
  default_options.qualification.manifest = qualification_manifest_reference{
      product_contract_schema_version, manifest.identifier,
      manifest.manifest_digest};
  require(authorize_backend(default_options, qualified, &manifest).has_value(),
          "qualified default requires exact manifest profile binding");
  manifest.manifest_digest.bytes[0] ^= 1;
  require(!authorize_backend(default_options, qualified, &manifest).has_value(),
          "mutated qualification manifest rejected");

  auto stale = experimental;
  stale.capability_digest.bytes[0] ^= 1;
  require(!validate_backend_identity(stale).has_value(),
          "stale capability digest rejected");
  stale = experimental;
  stale.capabilities.bits |= std::uint64_t(1) << 63;
  stale.capability_digest = backend_capability_digest(stale.capabilities);
  require(!validate_backend_identity(stale).has_value(),
          "unknown capability rejected");
}

static void exact_result_lifetime_and_envelope() {
  auto provenance = experimental_provenance();
  std::vector<std::uint8_t> bytes{1, 2, 3, 4};
  auto made = make_exact_result_handle(
      operation::regularized_union,
      exact_result_topology::closed_embedded_two_manifold, provenance,
      std::move(bytes));
  require(made.has_value(), "durable exact result handle");
  auto exact = made.value();
  auto retained = exact;
  require(exact.use_count() >= 2 && retained->canonical_bytes.size() == 4,
          "exact result owns immutable storage independently of context");
  require(!product_digest_is_zero(exact->canonical_digest),
          "exact result digest");
  require(!make_exact_result_handle(operation::regularized_union,
                                    exact_result_topology::empty, provenance, {})
               .has_value(),
          "empty exact serialization rejected");

  boolean_product_result<double, std::uint64_t> result;
  result.selected_operation = operation::regularized_union;
  result.backend = provenance;
  result.exact_result = exact;
  result.preparation.input_digest = test_digest('a');
  result.preparation.prepared_digest = test_digest('b');
  result.preparation.policy_digest = test_digest('c');
  result.preparation.report_digest = test_digest('l');
  result.attributes.report_digest = test_digest('m');
  result.verification.passed = true;
  result.verification.verifier_set_version = 1;
  result.verification.verifier_set_digest = test_digest('d');
  result.verification.report_digest = test_digest('e');
  require(validate_product_result(result).has_value(),
          "verified exact stratified envelope");

  result.realization = realization_attempt_record{};
  result.realization->requested = result_representation::exact_in_T_mesh;
  result.realization->semantics = product_realization_semantics::exact_in_T;
  result.realization->failure = make_product_error(
      product_error_code::output_not_representable, "fixture");
  require(validate_product_result(result).has_value(),
          "failed mesh realization retains exact success");

  auto success = std::make_shared<boolean_success<double, std::uint64_t>>();
  success->selected_operation = operation::regularized_union;
  certified_mesh_payload<double, std::uint64_t> payload;
  payload.success = success;
  payload.semantics = product_realization_semantics::exact_in_T;
  payload.exact_result_digest = exact->canonical_digest;
  payload.certificate.semantics = product_realization_semantics::exact_in_T;
  payload.certificate.exact_result_digest = exact->canonical_digest;
  payload.certificate.certificate_digest = test_digest('f');
  result.representation = result_representation::exact_in_T_mesh;
  result.mesh = payload;
  result.realization->succeeded = true;
  result.realization->failure.reset();
  require(validate_product_result(result).has_value(),
          "exact-in-T mesh bound to exact authority");

  result.representation = result_representation::certified_approximate_mesh;
  require(!validate_product_result(result).has_value(),
          "exact mesh cannot be relabeled approximate");
}

static void replay_contract() {
  auto options = explicit_experimental_options();
  const auto backend = experimental_identity();
  const auto provenance = experimental_provenance();
  auto exact_made = make_exact_result_handle(
      operation::regularized_union,
      exact_result_topology::closed_embedded_two_manifold, provenance,
      {9, 8, 7});
  require(exact_made.has_value(), "replay exact handle");
  auto exact = exact_made.value();

  product_replay_binding replay;
  replay.options_digest = product_options_digest(options);
  replay.preparation_policy_digest = test_digest('g');
  replay.preparation_report_digest = test_digest('h');
  replay.producer = backend.id;
  replay.adapter_version = backend.adapter_version;
  replay.build_identifier = backend.build_identifier;
  replay.capability_digest = backend.capability_digest;
  replay.exact_result_digest = exact->canonical_digest;
  replay.realization_policy_digest = test_digest('i');
  replay.attribute_policy_digest = test_digest('j');
  replay.verifier_set_digest = test_digest('k');
  require(validate_product_replay_binding(replay, options, backend, &exact)
              .has_value(),
          "replay cross-layer bindings");
  auto encoded = encode_product_replay_binding(replay);
  require(encoded.has_value(), "replay encode");
  auto decoded = decode_product_replay_binding(encoded.value());
  require(decoded.has_value(), "replay decode");
  require(validate_product_replay_binding(decoded.value(), options, backend,
                                          &exact)
              .has_value(),
          "replay canonical round trip");
  decoded.value().capability_digest.bytes[0] ^= 1;
  require(!validate_product_replay_binding(decoded.value(), options, backend,
                                           &exact)
               .has_value(),
          "stale backend replay binding rejected");
  auto trailing = encoded.value();
  trailing.push_back(0);
  require(!decode_product_replay_binding(trailing).has_value(),
          "replay trailing bytes rejected");
}

int main(){struct T{const char*n;void(*f)();};T ts[]={{"round",product_policy_round_trip},{"validation",product_policy_validation},{"backend",backend_maturity_and_qualification},{"exact",exact_result_lifetime_and_envelope},{"replay",replay_contract}};int n=0;for(auto&t:ts)try{t.f();std::cout<<"PASS "<<t.n<<"\n";}catch(const std::exception&e){++n;std::cerr<<"FAIL "<<t.n<<": "<<e.what()<<"\n";}return n?1:0;}
