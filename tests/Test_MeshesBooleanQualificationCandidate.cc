#define main qualification_candidate_existing_contract_main
#include "Test_MeshesBooleanQualificationCandidate.00.inc"
#include "Test_MeshesBooleanQualificationCandidate.01.inc"
#include "Test_MeshesBooleanQualificationCandidate.02.inc"
#include "Test_MeshesBooleanQualificationCandidate.03.inc"
#undef main

#include "Test_MeshesBooleanQualificationCandidate.04.inc"

int main() {
  harness tests;
  tests.add("P6.10.plan", [] { plan_contracts(); });
  tests.add("P6.10.complete", [] { complete_campaign_contracts(); });
  tests.add("P6.10.unresolved", [] { unresolved_and_missing_contracts(); });
  tests.add("P6.10.resolution", [] { resolution_fail_closed_contracts(); });
  tests.add("P6.10.performance", [] { performance_review_contracts(); });
  tests.add("P6.10.runner", [] { runner_contracts(); });
  return tests.run(std::cout, std::cerr);
}
