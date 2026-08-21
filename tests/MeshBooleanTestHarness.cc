#include "MeshBooleanTestHarness.h"

#include <algorithm>
#include <exception>
#include <iostream>

namespace ygor::mesh_boolean::testing {

void harness::add(std::string id, std::function<void()> body) {
  if (id.empty() || !body)
    throw std::invalid_argument("test case requires an id and body");
  cases_.push_back({std::move(id), std::move(body)});
}

int harness::run(std::ostream &out, std::ostream &error) const {
  auto ordered = cases_;
  std::sort(ordered.begin(), ordered.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
  for (std::size_t i = 1; i < ordered.size(); ++i)
    if (ordered[i - 1].id == ordered[i].id) {
      error << "duplicate test id: " << ordered[i].id << '\n';
      return 1;
    }
  std::size_t failed = 0;
  for (const auto &entry : ordered) {
    try {
      entry.body();
      out << "PASS\t" << entry.id << '\n';
    } catch (const std::exception &e) {
      ++failed;
      error << "FAIL\t" << entry.id << '\t' << e.what() << '\n';
    } catch (...) {
      ++failed;
      error << "FAIL\t" << entry.id << "\tunknown exception\n";
    }
  }
  out << "SUMMARY\t" << ordered.size() << '\t' << failed << '\n';
  return failed == 0 ? 0 : 1;
}

void require(bool condition, const std::string &message) {
  if (!condition)
    throw assertion_failure(message);
}

} // namespace ygor::mesh_boolean::testing
