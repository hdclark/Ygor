              "qualification_performance.cancellation_latency");
    } else {
      for (const auto &entry : value.candidate_samples)
        if (entry.metric(qualification_performance_metric::
                             cancellation_latency_milliseconds) != 0U)
          return performance_error(
              "qualification_performance.unexpected_cancellation_latency");
    }

    if (transaction_kind(descriptor.kind)) {
      if (zero(value.publication_state_before_digest) ||
          value.publication_state_before_digest !=
              value.publication_state_after_digest ||
          zero(value.replay_digest) || !value.limit_or_cancellation_triggered ||
          !value.typed_failure_observed || !value.transaction_rolled_back ||
