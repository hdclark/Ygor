namespace {

constexpr std::uint64_t capability_mask() noexcept {
  return (std::uint64_t(1)
          << static_cast<unsigned>(backend_capability::count)) -
         1;
}
constexpr std::uint64_t normalization_mask() noexcept {
  return (std::uint64_t(1)
          << static_cast<unsigned>(normalization_operation::count)) -
         1;
}
constexpr std::uint64_t geometry_changing_normalization_mask() noexcept {
  return normalization_operation_bit(
             normalization_operation::seam_aware_vertex_consolidation) |
         normalization_operation_bit(normalization_operation::crack_closure) |
         normalization_operation_bit(
             normalization_operation::nonplanar_facet_handling) |
         normalization_operation_bit(normalization_operation::sliver_handling) |
         normalization_operation_bit(
             normalization_operation::self_intersection_repair);
}

bool known(backend_id v) noexcept {
  return v == backend_id::experimental_exact_v1;
}
bool known(backend_maturity v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(backend_maturity::deprecated);
}
bool known(backend_selection_mode v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(backend_selection_mode::explicit_fallback_chain);
}
bool known(preparation_mode v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(preparation_mode::normalized);
}
bool known(normalization_mode v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(normalization_mode::geometry_changing);
}
bool known(model_unit v) noexcept {
  return static_cast<unsigned>(v) <= static_cast<unsigned>(model_unit::foot);
}
bool known(result_representation v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(result_representation::certified_approximate_mesh);
}
bool known(product_realization_semantics v) noexcept {
  return static_cast<unsigned>(v) <= static_cast<unsigned>(
                                           product_realization_semantics::
                                               certified_approximate_embedding_v1);
}
bool known(realization_search_strategy v) noexcept {
  return static_cast<unsigned>(v) <= static_cast<unsigned>(
                                           realization_search_strategy::
                                               deterministic_bounded_search);
}
bool known(attribute_transfer_mode v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_transfer_mode::require_lossless);
}
bool known(attribute_conflict_policy v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(attribute_conflict_policy::reject);
}
bool known(qualification_policy_mode v) noexcept {
  return static_cast<unsigned>(v) <= static_cast<unsigned>(
                                           qualification_policy_mode::
                                               allow_explicit_unqualified);
}
bool known(product_error_code v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(product_error_code::verifier_disagreement);
}
bool known(exact_result_topology v) noexcept {
  return static_cast<unsigned>(v) <=
         static_cast<unsigned>(exact_result_topology::stratified_non_manifold);
}

product_status_or<bool> bad(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

bool finite_nonnegative(double v) noexcept {
  return std::isfinite(v) && v >= 0.0;
}

bool schemas_current(const product_schema_versions &s) noexcept {
  return s.options == product_contract_schema_version &&
         s.artifact == product_contract_schema_version &&
         s.error == product_contract_schema_version &&
         s.certificate == product_contract_schema_version &&
         s.replay == product_contract_schema_version;
}

bool unique_backend_ids(const std::vector<backend_id> &ids) {
  std::set<std::uint16_t> seen;
  for (const auto id : ids) {
    if (!known(id) || !seen.insert(static_cast<std::uint16_t>(id)).second)
      return false;
  }
  return true;
}

bool unique_error_codes(const std::vector<product_error_code> &codes) {
  std::set<std::uint16_t> seen;
  for (const auto code : codes) {
    if (!known(code) ||
        !seen.insert(static_cast<std::uint16_t>(code)).second)
      return false;
  }
  return true;
}

void encode_digest(canonical_encoder &e, const digest &d) {
  e.raw(d.bytes.data(), d.bytes.size());
}
void encode_backend_version(canonical_encoder &e, const backend_version &v) {
  e.u16(v.major);
  e.u16(v.minor);
  e.u16(v.patch);
}
void encode_double(canonical_encoder &e, double v) { e.floating(v); }

std::vector<std::uint8_t>
record_bytes(const std::array<char, 8> &tag, std::uint16_t schema,
             const std::vector<std::uint8_t> &payload) {
  canonical_encoder e;
  e.raw(reinterpret_cast<const std::uint8_t *>(tag.data()), tag.size());
  e.u16(schema);
  e.u64(payload.size());
  e.raw(payload.data(), payload.size());
  return e.bytes();
}

class reader {
  const std::vector<std::uint8_t> &bytes_;
  std::size_t at_ = 0;
  const product_decode_limits &limits_;

  void require(std::size_t n) {
    if (n > bytes_.size() - at_)
      throw std::runtime_error("truncated_record");
  }

public:
  reader(const std::vector<std::uint8_t> &b,
         const product_decode_limits &limits)
      : bytes_(b), limits_(limits) {}
  std::size_t remaining() const noexcept { return bytes_.size() - at_; }
  std::uint8_t byte() {
    require(1);
    return bytes_[at_++];
  }
  bool boolean() {
    const auto v = byte();
    if (v > 1)
      throw std::runtime_error("invalid_bool");
    return v != 0;
  }
  std::uint16_t u16() {
    require(2);
    const auto v = (std::uint16_t(bytes_[at_]) << 8) |
                   std::uint16_t(bytes_[at_ + 1]);
    at_ += 2;
    return v;
  }
  std::uint32_t u32() {
    require(4);
    std::uint32_t v = 0;
    for (unsigned i = 0; i != 4; ++i)
      v = (v << 8) | bytes_[at_++];
    return v;
  }
  std::uint64_t u64() {
    require(8);
    std::uint64_t v = 0;
    for (unsigned i = 0; i != 8; ++i)
      v = (v << 8) | bytes_[at_++];
    return v;
  }
  double floating64() {
    const auto bits = u64();
    double v = 0.0;
    static_assert(sizeof(v) == sizeof(bits), "binary64 required");
    std::memcpy(&v, &bits, sizeof(v));
    return v;
  }
  digest digest_value() {
    require(16);
    digest d;
    std::copy(bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
              bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + 16),
              d.bytes.begin());
    at_ += 16;
    return d;
  }
  std::string string() {
    const auto n = u64();
    if (n > limits_.max_string_bytes || n > remaining())
      throw std::runtime_error("string_limit");
    std::string s(reinterpret_cast<const char *>(bytes_.data() + at_),
                  static_cast<std::size_t>(n));
    at_ += static_cast<std::size_t>(n);
    return s;
  }
  std::vector<std::uint8_t> raw(std::size_t n) {
    require(n);
    std::vector<std::uint8_t> out(bytes_.begin() +
                                      static_cast<std::ptrdiff_t>(at_),
                                  bytes_.begin() +
                                      static_cast<std::ptrdiff_t>(at_ + n));
    at_ += n;
    return out;
  }
};

product_status_or<std::vector<std::uint8_t>>
extract_payload(const std::vector<std::uint8_t> &bytes,
                const std::array<char, 8> &expected,
                const product_decode_limits &limits) {
  try {
    if (bytes.size() > limits.max_record_bytes)
      return make_product_error(product_error_code::resource_limit,
                                "decode.record_limit");
    reader r(bytes, limits);
    for (const char c : expected)
      if (r.byte() != static_cast<std::uint8_t>(c))
        return make_product_error(product_error_code::input_contract_error,
                                  "decode.domain_tag");
    if (r.u16() != product_contract_schema_version)
      return make_product_error(product_error_code::stale_binding,
                                "decode.schema");
    const auto n = r.u64();
    if (n > limits.max_record_bytes || n != r.remaining())
      return make_product_error(product_error_code::input_contract_error,
                                "decode.payload_length");
    return r.raw(static_cast<std::size_t>(n));
  } catch (const std::exception &x) {
    auto e = make_product_error(product_error_code::input_contract_error,
                                "decode.malformed");
    e.detail = x.what();
    return e;
  }
}

backend_id decode_backend_id(reader &r) {
  const auto v = static_cast<backend_id>(r.u16());
  if (!known(v))
    throw std::runtime_error("unknown_backend_id");
  return v;
}
product_error_code decode_error_code(reader &r) {
  const auto v = static_cast<product_error_code>(r.u16());
  if (!known(v))
    throw std::runtime_error("unknown_error_code");
  return v;
}

template <class E> E decode_u8_enum(reader &r, bool (*is_known)(E) noexcept) {
  const auto v = static_cast<E>(r.byte());
  if (!is_known(v))
    throw std::runtime_error("unknown_enum");
  return v;
}

std::vector<std::uint8_t>
encode_manifest_payload(const std::string &identifier,
                        const std::vector<qualification_profile> &profiles) {
  canonical_encoder e;
  e.u16(product_contract_schema_version);
  e.string(identifier);
  e.u64(profiles.size());
  for (const auto &p : profiles) {
    e.u16(static_cast<std::uint16_t>(p.backend));
    encode_digest(e, p.capability_digest);
    e.byte(static_cast<std::uint8_t>(p.representation));
    e.byte(static_cast<std::uint8_t>(p.preparation));
    e.string(p.workload_profile);
  }
  return e.bytes();
}

bool profile_less(const qualification_profile &a,
                  const qualification_profile &b) {
  return std::tie(a.backend, a.capability_digest, a.representation,
                  a.preparation, a.workload_profile) <
         std::tie(b.backend, b.capability_digest, b.representation,
                  b.preparation, b.workload_profile);
}

bool same_version(const backend_version &a, const backend_version &b) noexcept {
  return a.major == b.major && a.minor == b.minor && a.patch == b.patch;
}

bool same_identity(const backend_identity &a, const backend_identity &b) noexcept {
  return a.schema == b.schema && a.id == b.id &&
         same_version(a.adapter_version, b.adapter_version) &&
         a.build_identifier == b.build_identifier &&
         a.capabilities.schema == b.capabilities.schema &&
         a.capabilities.bits == b.capabilities.bits &&
         a.capability_digest == b.capability_digest && a.maturity == b.maturity;
}

bool has_common_product_capabilities(const backend_identity &b) noexcept {
  return b.capabilities.has(backend_capability::exact_set_semantics) &&
         b.capabilities.has(
             backend_capability::deterministic_canonical_output) &&
         b.capabilities.has(
             backend_capability::certified_failure_categories) &&
         b.capabilities.has(backend_capability::provenance_mapping) &&
         b.capabilities.has(backend_capability::strict_prepared_operands);
}

bool has_representation_capabilities(const backend_identity &b,
                                     result_representation r) noexcept {
  if (!has_common_product_capabilities(b))
    return false;
  switch (r) {
  case result_representation::exact_stratified:
    return b.capabilities.has(backend_capability::exact_coordinates) &&
           b.capabilities.has(backend_capability::stratified_output);
  case result_representation::exact_in_T_mesh:
    return b.capabilities.has(backend_capability::exact_coordinates) &&
           b.capabilities.has(backend_capability::stratified_output) &&
           b.capabilities.has(backend_capability::manifold_mesh_output) &&
           b.capabilities.has(backend_capability::exact_in_T_output);
  case result_representation::certified_approximate_mesh:
    return b.capabilities.has(backend_capability::stratified_output) &&
           b.capabilities.has(backend_capability::manifold_mesh_output) &&
           b.capabilities.has(
               backend_capability::certified_approximate_output);
  }
  return false;
}

} // namespace
