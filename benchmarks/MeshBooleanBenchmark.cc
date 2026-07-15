#include "MeshBooleanAnalyticFixtures.h"

#include <chrono>
#include <cstdint>
#include <iostream>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

int main() {
  using clock = std::chrono::steady_clock;
  constexpr std::size_t repetitions = 5;
  for (const auto op : {operation::regularized_union,
                        operation::regularized_intersection,
                        operation::a_minus_b, operation::b_minus_a,
                        operation::symmetric_difference}) {
    std::uint64_t vertices = 0, faces = 0;
    const auto begin = clock::now();
    for (std::size_t i = 0; i < repetitions; ++i) {
      const auto result = run_box_operation<double, std::uint32_t>(
          0, 1, 3, 4, op);
      vertices += result->payload->mesh.vertices.size();
      faces += result->payload->mesh.faces.size();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock::now() - begin);
    std::cout << "BENCH\tschema=1\top=" << static_cast<unsigned>(op)
              << "\trepetitions=" << repetitions
              << "\tmicroseconds=" << elapsed.count()
              << "\tvertices=" << vertices << "\tfaces=" << faces << '\n';
  }
}
