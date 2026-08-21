#include <cfenv>
#include <iostream>
#include <stdexcept>
#include <string>

void test_source_triangulation_unit();
void test_source_triangulation_geometry_basis();
void test_source_triangulation_projection();
void test_source_triangulation_coverage();
void test_source_triangulation_dependencies();
void test_source_triangulation_boundary_sharing();
void test_source_triangulation_canonicalization();
void test_source_triangulation_alternatives();
void test_source_triangulation_mutation();
void test_source_triangulation_properties();
void test_source_triangulation_adversarial();

int main(int argc, char **argv) {
  try {
    if (std::fesetenv(FE_DFL_ENV) != 0 ||
        std::fesetround(FE_TONEAREST) != 0)
      throw std::runtime_error("establish source triangulation floating environment");
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "unit") test_source_triangulation_unit();
    if (suite == "all" || suite == "geometry_basis") test_source_triangulation_geometry_basis();
    if (suite == "all" || suite == "projection") test_source_triangulation_projection();
    if (suite == "all" || suite == "coverage") test_source_triangulation_coverage();
    if (suite == "all" || suite == "dependencies") test_source_triangulation_dependencies();
    if (suite == "all" || suite == "boundary") test_source_triangulation_boundary_sharing();
    if (suite == "all" || suite == "canonical") test_source_triangulation_canonicalization();
    if (suite == "all" || suite == "alternatives") test_source_triangulation_alternatives();
    if (suite == "all" || suite == "mutation") test_source_triangulation_mutation();
    if (suite == "all" || suite == "properties") test_source_triangulation_properties();
    if (suite == "all" || suite == "adversarial") test_source_triangulation_adversarial();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
