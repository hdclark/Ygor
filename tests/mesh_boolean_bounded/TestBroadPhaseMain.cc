#include "BroadPhaseFixtures.h"

#include <cfenv>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char **argv) {
  using test = void (*)();
  const std::map<std::string, test> tests{
      {"domain", &broad_phase_tests::test_domain},
      {"rank_morton", &broad_phase_tests::test_rank_morton},
      {"hierarchy", &broad_phase_tests::test_hierarchy},
      {"overlap", &broad_phase_tests::test_overlap},
      {"traversal", &broad_phase_tests::test_traversal},
      {"known_candidates", &broad_phase_tests::test_known_candidates},
      {"all_pairs", &broad_phase_tests::test_all_pairs_oracle},
      {"provenance", &broad_phase_tests::test_provenance},
      {"canonicalization", &broad_phase_tests::test_canonicalization},
      {"codec_replay", &broad_phase_tests::test_codec_replay},
      {"alternative", &broad_phase_tests::test_alternative_triangulation},
      {"mutation", &broad_phase_tests::test_mutation},
      {"properties", &broad_phase_tests::test_properties},
      {"adversarial", &broad_phase_tests::test_adversarial},
      {"resources", &broad_phase_tests::test_resources_cancellation},
      {"structural", &broad_phase_tests::test_structural_performance},
  };
  try {
    if (std::fesetenv(FE_DFL_ENV) != 0 ||
        std::fesetround(FE_TONEAREST) != 0) {
      std::cerr << "unable to establish broad-phase floating environment\n";
      return 2;
    }
    if (argc != 2 || tests.find(argv[1]) == tests.end()) {
      std::cerr << "expected one broad-phase suite name\n";
      return 2;
    }
    tests.at(argv[1])();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
