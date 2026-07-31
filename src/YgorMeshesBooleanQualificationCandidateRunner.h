#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_CANDIDATE_RUNNER_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_CANDIDATE_RUNNER_H_

#include "YgorMeshesBooleanQualificationCandidate.h"

#include <functional>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

using qualification_candidate_case_executor = std::function<
    product_status_or<qualification_candidate_execution_observation>(
        const qualification_candidate_execution_case &)>;

using qualification_candidate_issue_reconciler = std::function<
    product_status_or<std::vector<qualification_candidate_issue>>(
        const qualification_candidate_campaign_plan &,
        const std::vector<qualification_candidate_execution_observation> &,
        std::vector<qualification_candidate_issue>)>;

// The sink is invoked exactly once, after the entire canonical campaign has
// been assembled. Implementations must publish transactionally or reject.
using qualification_candidate_campaign_sink = std::function<
    product_status_or<bool>(const qualification_candidate_campaign_plan &,
                            const qualification_candidate_campaign &)>;

struct qualification_candidate_deferred_execution {
  std::string case_identifier;
  std::string reason;
  digest evidence_digest;
};

struct qualification_candidate_run_options {
  bool requested_complete = true;
  std::vector<qualification_candidate_issue> retained_issues;
  // Deferred entries remain part of the frozen plan. The runner records an
  // infrastructure-failure observation and unresolved blocking issue without
  // invoking the case executor. A later controlled run must execute and
  // reconcile each entry before the campaign can close.
  std::vector<qualification_candidate_deferred_execution> deferred_executions;
  std::function<bool()> cancellation_requested;
};

struct qualification_candidate_run_result {
  qualification_candidate_campaign campaign;
  bool published = false;
};

product_status_or<qualification_candidate_run_result>
run_qualification_candidate_campaign(
    const qualification_candidate_campaign_plan &,
    qualification_candidate_case_executor,
    qualification_candidate_run_options = {},
    qualification_candidate_issue_reconciler = {},
    qualification_candidate_campaign_sink = {});

} // namespace mesh_boolean
} // namespace ygor

#endif
