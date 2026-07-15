#include "MeshBooleanTestConfig.h"
#include "MeshBooleanTestHarness.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#ifndef YGOR_BOOLEAN_TEST_DATA_DIR
#error "YGOR_BOOLEAN_TEST_DATA_DIR must be supplied by CMake"
#endif

using namespace ygor::mesh_boolean::testing;

namespace {
std::vector<std::vector<std::string>> read_tsv(const std::string &path,
                                               std::size_t fields) {
  std::ifstream input(path);
  require(bool(input), "cannot open qualification inventory: " + path);
  std::vector<std::vector<std::string>> rows;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::vector<std::string> row;
    std::istringstream stream(line);
    std::string field;
    while (std::getline(stream, field, '\t')) row.push_back(field);
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
      require(kinds.count(kind) == 1, std::string("inventory test kind: ") + kind);
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
                  row[1] == "predicate_vector",
              "known corpus record kind");
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
