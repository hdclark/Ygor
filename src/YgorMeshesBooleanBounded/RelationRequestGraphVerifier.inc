bool verify_relation_request_graph(const relation_request_graph &graph,
                                   bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = graph_error(subcode, summary,
                        relation_checkpoint::independent_verification);
    return false;
  };
  if (graph.schema_version != contract_versions::relation_request_graph_schema ||
      graph.graph_policy_version != contract_versions::relation_graph_policy ||
      !graph.owner.anchor)
    return fail(relation_subcode::unsupported_version,
                "relation graph version or owner is invalid");
  if (!std::is_sorted(
          graph.requests.begin(), graph.requests.end(),
          [](const canonical_relation_request &a,
             const canonical_relation_request &b) { return a.key < b.key; }))
    return fail(relation_subcode::canonical_order_mismatch,
                "relation requests are not canonically ordered");
  for (std::size_t i = 0; i < graph.requests.size(); ++i) {
    const auto &request = graph.requests[i];
    if (request.id.ordinal() != i || !valid_relation_request_key(request.key) ||
        request.reserved != 0)
      return fail(relation_subcode::canonical_id_mismatch,
                  "relation request identity is malformed");
    if (i != 0 && graph.requests[i - 1].key == request.key)
      return fail(relation_subcode::duplicate_authoritative_producer,
                  "duplicate authoritative relation producer");
    if (request.dependency_begin > graph.dependencies.size() ||
        request.dependency_count >
            graph.dependencies.size() - request.dependency_begin ||
        request.reverse_consumer_begin > graph.reverse_consumers.size() ||
        request.reverse_consumer_count >
            graph.reverse_consumers.size() - request.reverse_consumer_begin ||
        request.witness_begin > graph.candidate_witnesses.size() ||
        request.witness_count >
            graph.candidate_witnesses.size() - request.witness_begin)
      return fail(relation_subcode::unclosed_dependency,
                  "relation request range is out of bounds");
  }
  std::uint64_t expected_dependency_begin = 0;
  std::uint64_t expected_reverse_begin = 0;
  std::uint64_t expected_witness_begin = 0;
  std::vector<std::vector<relation_request_id>> reconstructed_reverse(
      graph.requests.size());
  for (std::size_t request_index = 0; request_index < graph.requests.size();
       ++request_index) {
    const auto &request = graph.requests[request_index];
    if (request.dependency_begin != expected_dependency_begin ||
        request.reverse_consumer_begin != expected_reverse_begin ||
        request.witness_begin != expected_witness_begin)
      return fail(relation_subcode::unclosed_dependency,
                  "relation graph ranges are not contiguous");
    expected_dependency_begin += request.dependency_count;
    expected_reverse_begin += request.reverse_consumer_count;
    expected_witness_begin += request.witness_count;
    for (std::uint64_t offset = 1; offset < request.witness_count; ++offset) {
      const auto previous = graph.candidate_witnesses[
          request.witness_begin + offset - 1];
      const auto current =
          graph.candidate_witnesses[request.witness_begin + offset];
      if (!(previous < current))
        return fail(relation_subcode::canonical_order_mismatch,
                    "relation candidate witnesses are not strictly ordered");
    }
  }
  if (expected_dependency_begin != graph.dependencies.size() ||
      expected_reverse_begin != graph.reverse_consumers.size() ||
      expected_witness_begin != graph.candidate_witnesses.size())
    return fail(relation_subcode::unclosed_dependency,
                "relation graph ranges do not cover their tables");

  for (std::size_t i = 0; i < graph.dependencies.size(); ++i) {
    const auto &dependency = graph.dependencies[i];
    if (dependency.id.ordinal() != i || dependency.reserved != 0 ||
        dependency.producer.ordinal() >= graph.requests.size() ||
        dependency.consumer.ordinal() >= graph.requests.size())
      return fail(relation_subcode::canonical_id_mismatch,
                  "relation dependency identity is malformed");
    if (dependency.consumer.ordinal() >= graph.requests.size())
      return fail(relation_subcode::unclosed_dependency,
                  "relation dependency consumer is out of bounds");
    const auto &consumer_request =
        graph.requests[dependency.consumer.ordinal()];
    if (i < consumer_request.dependency_begin ||
        i >= consumer_request.dependency_begin +
                 consumer_request.dependency_count)
      return fail(relation_subcode::unclosed_dependency,
                  "relation dependency is outside its consumer range");
    reconstructed_reverse[dependency.producer.ordinal()].push_back(
        dependency.consumer);
    if (static_cast<std::uint8_t>(
            graph.requests[dependency.producer.ordinal()].key.family) >=
        static_cast<std::uint8_t>(
            graph.requests[dependency.consumer.ordinal()].key.family))
      return fail(relation_subcode::forward_dependency,
                  "relation dependency violates family precedence");
  }
  for (std::size_t producer = 0; producer < graph.requests.size(); ++producer) {
    const auto &request = graph.requests[producer];
    const auto &expected = reconstructed_reverse[producer];
    if (request.reverse_consumer_count != expected.size())
      return fail(relation_subcode::unclosed_dependency,
                  "relation reverse-consumer count mismatch");
    for (std::size_t offset = 0; offset < expected.size(); ++offset) {
      const auto actual = graph.reverse_consumers[
          request.reverse_consumer_begin + offset];
      if (actual != expected[offset])
        return fail(relation_subcode::unclosed_dependency,
                    "relation reverse-consumer mapping mismatch");
    }
  }
  const auto digest =
      sha256::digest(encode_relation_request_graph_semantics(graph));
  if (digest != graph.semantic_digest)
    return fail(relation_subcode::digest_mismatch,
                "relation graph semantic digest mismatch");
  return true;
}
