          value.partial_publication_observed)
        return performance_error(
            "qualification_performance.transactional_failure_evidence");
    } else if (!zero(value.publication_state_before_digest) ||
               !zero(value.publication_state_after_digest) ||
               !zero(value.replay_digest) ||
               value.limit_or_cancellation_triggered ||
               value.typed_failure_observed || value.transaction_rolled_back ||
               value.partial_publication_observed) {
      return performance_error(
          "qualification_performance.unexpected_transaction_evidence");
    }

    const auto calculated = observation_digest(value);
    if (!zero(value.observation_digest) &&
