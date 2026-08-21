#include "MeshBooleanTestConfig.h"
#include "MeshBooleanTestHarness.h"
#include "YgorMeshesBooleanQualificationCorpus.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#ifndef YGOR_BOOLEAN_TEST_DATA_DIR
#error "YGOR_BOOLEAN_TEST_DATA_DIR must be supplied by CMake"
#endif

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {
std::vector<std::vector<std::string>> read_tsv(const std::string &path,
                                                std::size_t fields) {
  std::ifstream input(path);
  require(bool(input), "cannot open qualification inventory: " + path);
  std::vector<std::vector<std::string>> rows;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    std::vector<std::string> row;
    std::istringstream stream(line);
    std::string field;
    while (std::getline(stream, field, '\t'))
      row.push_back(field);
    require_equal(row.size(), fields, "inventory row has exact field count");
    rows.push_back(std::move(row));
  }
  require(!rows.empty(), "qualification inventory is nonempty");
  return rows;
}

void unique_sorted_ids(const std::vector<std::vector<std::string>> &rows) {
  std::string previous;
  for (const auto &row : rows) {
    require(!row[0].empty(), "inventory id is nonempty");
    require(previous.empty() || previous < row[0],
            "inventory ids are sorted and unique");
    previous = row[0];
  }
}

std::vector<std::string> split(const std::string &text, char separator = ',') {
  std::vector<std::string> result;
  std::istringstream stream(text);
  std::string item;
  while (std::getline(stream, item, separator)) {
    require(!item.empty(), "inventory list item is nonempty");
    result.push_back(item);
  }
  require(!result.empty(), "inventory list is nonempty");
  return result;
}

std::uint64_t parse_u64(const std::string &text) {
  std::size_t consumed = 0;
  const auto value = std::stoull(text, &consumed, 10);
  require(consumed == text.size(), "canonical unsigned integer");
  return value;
}

std::uint32_t parse_u32(const std::string &text) {
  const auto value = parse_u64(text);
  require(value <= std::numeric_limits<std::uint32_t>::max(),
          "unsigned integer fits uint32");
  return static_cast<std::uint32_t>(value);
}

unsigned hex_digit(char c) {
  if ('0' <= c && c <= '9')
    return static_cast<unsigned>(c - '0');
  if ('a' <= c && c <= 'f')
    return 10U + static_cast<unsigned>(c - 'a');
  throw std::runtime_error("inventory digest is lower-case hexadecimal");
}

digest parse_digest(const std::string &text) {
  require_equal(text.size(), std::size_t(32), "digest has 16 bytes");
  digest result;
  for (std::size_t i = 0; i != result.bytes.size(); ++i)
    result.bytes[i] = static_cast<std::uint8_t>(
        (hex_digit(text[2 * i]) << 4U) | hex_digit(text[2 * i + 1]));
  require(result != digest{}, "inventory digest is nonzero");
  return result;
}

digest policy_digest(const std::string &kind, const std::string &token) {
  canonical_encoder encoder;
  encoder.string(kind);
  encoder.string(token);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'P', '6', '2', '1'}},
                       encoder.bytes());
}

qualification_corpus_record_kind parse_record_kind(const std::string &text) {
  if (text == "generated_pair_family")
    return qualification_corpus_record_kind::generated_pair_family;
  if (text == "cad_like_pair_family")
    return qualification_corpus_record_kind::cad_like_pair_family;
  if (text == "operation_chain_family")
    return qualification_corpus_record_kind::operation_chain_family;
  if (text == "minimized_regression")
    return qualification_corpus_record_kind::minimized_regression;
  throw std::runtime_error("unknown corpus record kind: " + text);
}

qualification_corpus_source parse_source(const std::string &text) {
  if (text == "generated_construction_known")
    return qualification_corpus_source::generated_construction_known;
  if (text == "internally_generated_cad_like")
    return qualification_corpus_source::internally_generated_cad_like;
  if (text == "licensed_external")
    return qualification_corpus_source::licensed_external;
  if (text == "private_external")
    return qualification_corpus_source::private_external;
  if (text == "minimized_regression")
    return qualification_corpus_source::minimized_regression;
  throw std::runtime_error("unknown corpus source: " + text);
}

qualification_redistribution parse_redistribution(const std::string &text) {
  if (text == "repository_embedded")
    return qualification_redistribution::repository_embedded;
  if (text == "content_addressed_external")
    return qualification_redistribution::content_addressed_external;
  if (text == "private_digest_only")
    return qualification_redistribution::private_digest_only;
  throw std::runtime_error("unknown redistribution policy: " + text);
}

qualification_geometry_category parse_category(const std::string &text) {
  for (const auto category : required_qualification_geometry_categories())
    if (text == qualification_geometry_category_token(category))
      return category;
  throw std::runtime_error("unknown geometry category: " + text);
}

operation parse_operation(const std::string &text) {
  if (text == "union")
    return operation::regularized_union;
  if (text == "intersection")
    return operation::regularized_intersection;
  if (text == "a_minus_b")
    return operation::a_minus_b;
  if (text == "b_minus_a")
    return operation::b_minus_a;
  if (text == "symmetric_difference")
    return operation::symmetric_difference;
  throw std::runtime_error("unknown Boolean operation: " + text);
}

qualification_operand_order parse_operand_order(const std::string &text) {
  if (text == "a_then_b")
    return qualification_operand_order::a_then_b;
  if (text == "b_then_a")
    return qualification_operand_order::b_then_a;
  throw std::runtime_error("unknown operand order: " + text);
}

qualification_operation_coverage parse_operation_coverage(
    const std::string &text) {
  const auto separator = text.find(':');
  require(separator != std::string::npos, "operation coverage has order");
  return {parse_operation(text.substr(0, separator)),
          parse_operand_order(text.substr(separator + 1))};
}

qualification_type_binding parse_type(const std::string &text) {
  const auto separator = text.find(':');
  require(separator != std::string::npos, "type specialization is paired");
  const auto coordinate = text.substr(0, separator);
  const auto index = text.substr(separator + 1);
  qualification_type_binding result;
  if (coordinate == "binary32")
    result.coordinate = coordinate_tag::binary32;
  else if (coordinate == "binary64")
    result.coordinate = coordinate_tag::binary64;
  else
    throw std::runtime_error("unknown coordinate type: " + coordinate);
  if (index == "uint32")
    result.index = index_tag::uint32;
  else if (index == "uint64")
    result.index = index_tag::uint64;
  else
    throw std::runtime_error("unknown index type: " + index);
  return result;
}

qualification_result_mode_binding parse_result_mode(const std::string &text) {
  qualification_result_mode_binding result;
  if (text == "exact_stratified") {
    result.representation = result_representation::exact_stratified;
    result.semantics = product_realization_semantics::not_requested;
  } else if (text == "exact_in_T_mesh") {
    result.representation = result_representation::exact_in_T_mesh;
    result.semantics = product_realization_semantics::exact_in_T;
  } else if (text == "certified_approximate_mesh") {
    result.representation = result_representation::certified_approximate_mesh;
    result.semantics =
        product_realization_semantics::certified_approximate_embedding_v1;
  } else {
    throw std::runtime_error("unknown result mode: " + text);
  }
  result.policy_digest = policy_digest("result", text);
  return result;
}

qualification_preparation_binding parse_preparation(const std::string &text) {
  qualification_preparation_binding result;
  if (text == "strict_validation") {
    result.mode = preparation_mode::strict_validation;
  } else if (text == "diagnosis_only") {
    result.mode = preparation_mode::diagnosis_only;
  } else if (text == "normalized") {
    result.mode = preparation_mode::normalized;
    result.tolerance_unit = model_unit::unitless;
    const double tolerance = 0x1p-30;
    std::memcpy(&result.model_tolerance_binary64_bits, &tolerance,
                sizeof(tolerance));
  } else {
    throw std::runtime_error("unknown preparation policy: " + text);
  }
  result.policy_digest = policy_digest("preparation", text);
  return result;
}

qualification_outcome parse_outcome(const std::string &text) {
  if (text == "verified_exact_success")
    return qualification_outcome::verified_exact_success;
  if (text == "verified_certified_approximate_success")
    return qualification_outcome::verified_certified_approximate_success;
  if (text == "expected_typed_failure")
    return qualification_outcome::expected_typed_failure;
  if (text == "timeout_or_resource_limit")
    return qualification_outcome::timeout_or_resource_limit;
  throw std::runtime_error("invalid expected outcome: " + text);
}

product_error_code parse_error_code(const std::string &text) {
  static const std::map<std::string, product_error_code> codes{
      {"input_contract_error", product_error_code::input_contract_error},
      {"resource_limit", product_error_code::resource_limit},
      {"index_overflow", product_error_code::index_overflow},
      {"result_topology_not_supported",
       product_error_code::result_topology_not_supported},
      {"output_not_representable",
       product_error_code::output_not_representable},
      {"normalization_required",
       product_error_code::normalization_required},
      {"normalization_failed", product_error_code::normalization_failed},
      {"attribute_transfer_conflict",
       product_error_code::attribute_transfer_conflict},
      {"replay_mismatch", product_error_code::replay_mismatch},
      {"verifier_disagreement", product_error_code::verifier_disagreement}};
  const auto found = codes.find(text);
  if (found == codes.end())
    throw std::runtime_error("unknown expected product error: " + text);
  return found->second;
}

qualification_corpus_inventory load_corpus_inventory() {
  const auto rows = read_tsv(std::string(YGOR_BOOLEAN_TEST_DATA_DIR) +
                                 "/corpus/inventory.tsv",
                             23);
  unique_sorted_ids(rows);
  qualification_corpus_inventory inventory;
  inventory.identifier = "mesh-boolean-permanent-corpus";
  inventory.version = "1";
  for (const auto &row : rows) {
    qualification_corpus_record record;
    record.identifier = row[0];
    record.kind = parse_record_kind(row[1]);
    record.schema = static_cast<std::uint16_t>(parse_u32(row[2]));
    record.source = parse_source(row[3]);
    record.redistribution = parse_redistribution(row[4]);
    record.license_or_provenance = row[5];
    record.recipe_identifier = row[6];
    record.recipe_version = row[7];
    record.recipe_digest = parse_digest(row[8]);
    record.first_case_ordinal = parse_u64(row[9]);
    record.case_count = parse_u64(row[10]);
    record.minimum_chain_steps = parse_u32(row[11]);
    record.maximum_chain_steps = parse_u32(row[12]);
    for (const auto &token : split(row[13]))
      record.geometry_categories.push_back(parse_category(token));
    for (const auto &token : split(row[14]))
      record.operations.push_back(parse_operation_coverage(token));
    for (const auto &token : split(row[15]))
      record.type_specializations.push_back(parse_type(token));
    for (const auto &token : split(row[16]))
      record.result_modes.push_back(parse_result_mode(token));
    for (const auto &token : split(row[17]))
      record.preparation_policies.push_back(parse_preparation(token));
    for (const auto &token : split(row[18]))
      record.expected_outcomes.push_back(parse_outcome(token));
    if (row[19] != "none")
      for (const auto &token : split(row[19]))
        record.expected_failure_codes.push_back(parse_error_code(token));
    record.expectation_identifier = row[20];
    record.expectation_digest = parse_digest(row[21]);
    record.permanent_test_id = row[22];
    inventory.records.push_back(std::move(record));
  }
  auto made = make_qualification_corpus_inventory(std::move(inventory));
  require(made.has_value(),
          made.has_value() ? "corpus inventory made"
                           : made.error().message_key);
  return std::move(made.value());
}

void require_permanent_regressions(
    const qualification_corpus_inventory &inventory) {
  std::set<std::string> present;
  for (const auto &record : inventory.records)
    if (record.kind == qualification_corpus_record_kind::minimized_regression)
      present.insert(record.permanent_test_id);
  const std::set<std::string> required{
      "C14.E2E.disjoint.double.u32",
      "C14.E2E.equal.identities",
      "C14.REPLAY.archive.codec",
      "C14.PLANGAP.G1a",
      "C14.PLANGAP.G1b",
      "C14.PLANGAP.G2a",
      "P2.3.NORMALIZATION.sliver",
      "P2.3.NORMALIZATION.self_intersection",
      "C14.PLANGAP.G3",
      "C14.PLANGAP.G4a",
      "C14.PLANGAP.G4b",
      "C14.PLANGAP.G5a",
      "C14.PLANGAP.G6",
      "C14.PLANGAP.G7a",
      "C14.PLANGAP.G7b",
      "C14.PLANGAP.G8solver",
      "C14.PLANGAP.G8pairs",
      "C14.PLANGAP.G9a",
      "C14.PLANGAP.G9b",
      "C14.PLANGAP.G9c",
      "C14.PLANGAP.G9d"};
  require(present == required,
          "all existing and plan-gap minimized regressions are permanent");
}

void clear_inventory_bindings(qualification_corpus_inventory &inventory) {
  inventory.generated_pair_count = 0;
  inventory.cad_like_pair_count = 0;
  inventory.operation_chain_count = 0;
  inventory.minimized_regression_count = 0;
  inventory.record_set_digest = {};
  inventory.category_coverage_digest = {};
  inventory.expected_outcome_digest = {};
  inventory.inventory_digest = {};
}

void clear_record_binding(qualification_corpus_record &record) {
  record.record_digest = {};
}
} // namespace

int main() {
  harness tests;
  tests.add("C14.QUAL.requirements.inventory", [] {
    const auto rows = read_tsv(
        std::string(YGOR_BOOLEAN_TEST_DATA_DIR) + "/requirements.tsv", 6);
    unique_sorted_ids(rows);
    std::set<std::string> kinds;
    for (const auto &row : rows) {
      require(!row[1].empty() && !row[2].empty() && !row[3].empty(),
              "requirement row binds owner, clause, and test");
      kinds.insert(row[4]);
    }
    for (const auto *kind : {"positive", "negative", "property", "oracle",
                             "mutation", "qualification"})
      require(kinds.count(kind) == 1,
              std::string("inventory test kind: ") + kind);
  });
  tests.add("C14.QUAL.degeneracy.matrix", [] {
    const auto rows = read_tsv(
        std::string(YGOR_BOOLEAN_TEST_DATA_DIR) + "/degeneracies.tsv", 7);
    unique_sorted_ids(rows);
    for (const auto &row : rows)
      require(row[6] == "covered" || row[6].rfind("not_applicable:", 0) == 0,
              "degeneracy cell is explained");
  });
  tests.add("C14.QUAL.corpus.manifest", [] {
    const auto rows = read_tsv(std::string(YGOR_BOOLEAN_TEST_DATA_DIR) +
                                   "/corpus/manifest.tsv",
                               6);
    unique_sorted_ids(rows);
    for (const auto &row : rows)
      require(row[1] == "boolean_case" || row[1] == "codec_golden" ||
                  row[1] == "predicate_vector" ||
                  row[1] == "normalization_case" ||
                  row[1] == "qualification_inventory",
              "known corpus record kind");
  });
  tests.add("C14.QUAL.corpus.inventory.P6.2", [] {
    const auto inventory = load_corpus_inventory();
    require(validate_qualification_corpus_inventory(inventory).has_value(),
            "canonical corpus inventory validates");
    require(inventory.generated_pair_count == 10500,
            "construction-known pair floor is exceeded");
    require(inventory.cad_like_pair_count == 1100,
            "CAD-like pair floor is exceeded");
    require(inventory.operation_chain_count == 1100,
            "operation-chain floor is exceeded");
    require(inventory.minimized_regression_count == 21,
            "permanent regression inventory count");
    require_permanent_regressions(inventory);

    auto bindings = make_qualification_corpus_bindings(inventory);
    require(bindings.has_value() &&
                bindings.value().size() == inventory.records.size(),
            "every inventory record emits one P6.1 corpus binding");
    std::uint64_t bound_cases = 0;
    for (const auto &binding : bindings.value()) {
      require(binding.corpus_digest != digest{} &&
                  binding.category_coverage_digest != digest{} &&
                  binding.expected_outcome_digest != digest{},
              "corpus binding is fully digest-bound");
      bound_cases += binding.case_count;
    }
    require(bound_cases == 12721,
            "all pair, chain, and regression recipes are bound");
  });
  tests.add("C14.QUAL.corpus.inventory.fail_closed", [] {
    const auto inventory = load_corpus_inventory();

    auto stale = inventory;
    ++stale.records.front().case_count;
    require(!validate_qualification_corpus_inventory(stale).has_value(),
            "record mutation invalidates inventory binding");

    auto below_floor = inventory;
    auto generated = std::find_if(
        below_floor.records.begin(), below_floor.records.end(),
        [](const auto &record) {
          return record.kind ==
                 qualification_corpus_record_kind::generated_pair_family;
        });
    require(generated != below_floor.records.end(), "generated family exists");
    generated->case_count = 1;
    clear_record_binding(*generated);
    clear_inventory_bindings(below_floor);
    require(!make_qualification_corpus_inventory(below_floor).has_value(),
            "generated-pair floor is enforced");

    auto short_chain = inventory;
    auto chain = std::find_if(short_chain.records.begin(),
                              short_chain.records.end(), [](const auto &record) {
      return record.kind ==
             qualification_corpus_record_kind::operation_chain_family;
    });
    require(chain != short_chain.records.end(), "chain family exists");
    chain->minimum_chain_steps = 4;
    clear_record_binding(*chain);
    clear_inventory_bindings(short_chain);
    require(!make_qualification_corpus_inventory(short_chain).has_value(),
            "five-step chain floor is enforced");

    auto missing_category = inventory;
    for (auto &record : missing_category.records) {
      record.geometry_categories.erase(
          std::remove(record.geometry_categories.begin(),
                      record.geometry_categories.end(),
                      qualification_geometry_category::serialization_or_replay),
          record.geometry_categories.end());
      clear_record_binding(record);
    }
    clear_inventory_bindings(missing_category);
    require(!make_qualification_corpus_inventory(missing_category).has_value(),
            "required geometry-category coverage is enforced");

    auto vague_failure = inventory;
    auto typed = std::find_if(vague_failure.records.begin(),
                              vague_failure.records.end(), [](const auto &record) {
      return std::find(record.expected_outcomes.begin(),
                       record.expected_outcomes.end(),
                       qualification_outcome::expected_typed_failure) !=
             record.expected_outcomes.end();
    });
    require(typed != vague_failure.records.end(), "typed-failure family exists");
    typed->expected_failure_codes.clear();
    clear_record_binding(*typed);
    clear_inventory_bindings(vague_failure);
    require(!make_qualification_corpus_inventory(vague_failure).has_value(),
            "typed failures require explicit product error codes");
  });
  tests.add("C14.QUAL.configuration", [] {
    const auto config = load_test_config();
    require(config.generated_cases > 0, "configured generated-case budget");
    require(std::string(to_string(config.tier)) != "invalid",
            "configured qualification tier");
  });
  const auto result = tests.run(std::cout, std::cerr);
  if (result == 0) {
    const auto config = load_test_config();
    std::cout << "QUALIFICATION\tschema=1\ttier=" << to_string(config.tier)
              << "\tstatus=pass\n";
  }
  return result;
}
