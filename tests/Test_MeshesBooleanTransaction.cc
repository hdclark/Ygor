#include <YgorMeshesBooleanTransaction.h>

#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace ygor::mesh_boolean;

namespace {

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct large_artifact {
  static unsigned copies;
  static unsigned moves;
  static unsigned live;

  explicit large_artifact(std::size_t size = 1024 * 1024)
      : bytes(size, std::uint8_t{0x5a}) {
    ++live;
  }
  large_artifact(const large_artifact &other) : bytes(other.bytes) {
    ++copies;
    ++live;
  }
  large_artifact(large_artifact &&other) noexcept
      : bytes(std::move(other.bytes)) {
    ++moves;
    ++live;
  }
  ~large_artifact() { --live; }

  std::vector<std::uint8_t> bytes;
};

unsigned large_artifact::copies = 0;
unsigned large_artifact::moves = 0;
unsigned large_artifact::live = 0;

enum class verifier_behavior { pass, fail, stale_binding };

class recording_verifier final : public verifier_service {
public:
  explicit recording_verifier(verifier_behavior behavior)
      : behavior_(behavior) {}

  status_or<verification_report>
  verify(const artifact_view &view, const verification_spec &spec,
         const verification_environment_view &) const noexcept override {
    seen_payload = view.payload;
    verification_report report;
    report.checker_version = spec.checker_version;
    report.owner = behavior_ == verifier_behavior::stale_binding
                       ? make_context_owner_token()
                       : view.owner;
    report.stage = boolean_stage::input_validation;
    report.slot = view.slot;
    report.artifact_type_tag = view.artifact_type_tag;
    report.artifact_schema = view.artifact_schema;
    report.artifact_digest = view.artifact_digest;
    report.invariant_set_digest = spec.invariant_set_digest;
    report.outcome = behavior_ == verifier_behavior::fail
                         ? verification_outcome::invariant_failure
                         : verification_outcome::pass;
    return report;
  }

  mutable const void *seen_payload = nullptr;

private:
  verifier_behavior behavior_;
};

constexpr std::uint64_t test_type = 0x59474254584e3031ULL;
constexpr std::uint16_t test_schema = 1;

verification_spec spec() {
  verification_spec value;
  value.slot = artifact_slot::validated_operands;
  value.artifact_type_tag = test_type;
  value.artifact_schema = test_schema;
  const std::uint8_t bytes[] = {'t', 'r', 'a', 'n', 's', 'a',
                                'c', 't', 'i', 'o', 'n'};
  value.invariant_set_digest = md5_digest(bytes, sizeof(bytes));
  return value;
}

verification_environment_view environment(context_owner_token owner,
                                          resource_accountant &accountant) {
  verification_environment_view value;
  value.owner = owner;
  value.accountant = &accountant;
  return value;
}

digest artifact_digest(std::uint8_t value) {
  return md5_digest(&value, 1);
}

void one_allocation_success() {
  large_artifact::copies = large_artifact::moves = 0;
  const auto owner = make_context_owner_token();
  resource_accountant accountant(resource_policy{});
  recording_verifier verifier(verifier_behavior::pass);
  stage_transaction<large_artifact> tx(
      owner, boolean_stage::input_validation,
      artifact_slot::validated_operands, std::make_unique<large_artifact>());
  const auto *draft = &tx.draft();
  auto reservation = accountant.reserve_scoped(
      resource_kind::authoritative_bytes, draft->bytes.size(),
      boolean_stage::input_validation);
  require(reservation.has_value(), "success reservation");
  tx.stage_reservation(std::move(reservation.value()));
  auto frozen = tx.freeze_and_verify(test_type, test_schema, 1,
                                     artifact_digest(1), spec(),
                                     environment(owner, accountant), verifier);
  require(frozen.has_value(), "freeze success");
  require(verifier.seen_payload == draft, "verifier allocation identity");
  auto published = tx.publish();
  require(published.has_value(), "publish success");
  require(published.value()->payload.get() == draft,
          "published allocation identity");
  require(large_artifact::copies == 0 && large_artifact::moves == 0,
          "artifact copied or moved");
  require(accountant.used(resource_kind::authoritative_bytes) ==
              draft->bytes.size(),
          "successful reservation not committed");
  static_assert(std::is_const<typename std::remove_reference<
                    decltype(*published.value()->payload)>::type>::value,
                "published payload must be immutable");
}

void verification_failure_rolls_back() {
  const auto owner = make_context_owner_token();
  resource_accountant accountant(resource_policy{});
  recording_verifier verifier(verifier_behavior::fail);
  stage_transaction<large_artifact> tx(
      owner, boolean_stage::input_validation,
      artifact_slot::validated_operands, std::make_unique<large_artifact>(64));
  auto reservation = accountant.reserve_scoped(
      resource_kind::authoritative_bytes, 64,
      boolean_stage::input_validation);
  require(reservation.has_value(), "failure reservation");
  tx.stage_reservation(std::move(reservation.value()));
  const auto live_before = large_artifact::live;
  auto frozen = tx.freeze_and_verify(test_type, test_schema, 1,
                                     artifact_digest(2), spec(),
                                     environment(owner, accountant), verifier);
  require(!frozen.has_value() && tx.state() == transaction_state::failed,
          "verification failure accepted");
  require(large_artifact::live + 1 == live_before,
          "failed candidate retained");
  require(accountant.used(resource_kind::authoritative_bytes) == 0,
          "failed verification reservation retained");
}

void stale_binding_rolls_back() {
  const auto owner = make_context_owner_token();
  resource_accountant accountant(resource_policy{});
  recording_verifier verifier(verifier_behavior::stale_binding);
  stage_transaction<large_artifact> tx(
      owner, boolean_stage::input_validation,
      artifact_slot::validated_operands, std::make_unique<large_artifact>(32));
  auto reservation = accountant.reserve_scoped(
      resource_kind::authoritative_bytes, 32,
      boolean_stage::input_validation);
  require(reservation.has_value(), "binding reservation");
  tx.stage_reservation(std::move(reservation.value()));
  auto frozen = tx.freeze_and_verify(test_type, test_schema, 1,
                                     artifact_digest(3), spec(),
                                     environment(owner, accountant), verifier);
  require(!frozen.has_value() &&
              frozen.error().message_key == "verification_binding",
          "stale binding accepted");
  require(accountant.used(resource_kind::authoritative_bytes) == 0,
          "stale binding reservation retained");
}

void generations_are_isolated() {
  const auto owner = make_context_owner_token();
  artifact_generation_catalog catalog(owner);
  resource_accountant first_accountant(resource_policy{});
  resource_accountant stale_accountant(resource_policy{});
  recording_verifier verifier(verifier_behavior::pass);

  stage_transaction<large_artifact> first(
      owner, boolean_stage::input_validation,
      artifact_slot::validated_operands, std::make_unique<large_artifact>(16));
  first.draft().bytes[0] = 1;
  auto first_frozen = first.freeze_and_verify(
      test_type, test_schema, 1, artifact_digest(4), spec(),
      environment(owner, first_accountant), verifier);
  require(first_frozen.has_value(), "first freeze");
  auto generation_one = first.compare_and_publish(catalog, 0);
  require(generation_one.has_value(), "first generation publish");
  const auto *first_address = generation_one.value()->payload.get();
  const auto first_bytes = generation_one.value()->payload->bytes;

  stage_transaction<large_artifact> stale(
      owner, boolean_stage::input_validation,
      artifact_slot::validated_operands, std::make_unique<large_artifact>(16));
  auto stale_reservation = stale_accountant.reserve_scoped(
      resource_kind::authoritative_bytes, 16,
      boolean_stage::input_validation);
  require(stale_reservation.has_value(), "stale reservation");
  stale.stage_reservation(std::move(stale_reservation.value()));
  auto stale_frozen = stale.freeze_and_verify(
      test_type, test_schema, 1, artifact_digest(5), spec(),
      environment(owner, stale_accountant), verifier);
  require(stale_frozen.has_value(), "stale freeze");
  auto stale_publish = stale.compare_and_publish(catalog, 0);
  require(!stale_publish.has_value(), "stale writer published");
  require(large_artifact::live == 1, "stale candidate retained");
  require(stale_accountant.used(resource_kind::authoritative_bytes) == 0,
          "stale writer reservation retained");
  require(generation_one.value()->payload.get() == first_address &&
              generation_one.value()->payload->bytes == first_bytes,
          "stale writer mutated prior generation");

  resource_accountant second_accountant(resource_policy{});
  stage_transaction<large_artifact> second(
      owner, boolean_stage::input_validation,
      artifact_slot::validated_operands, std::make_unique<large_artifact>(16));
  second.draft().bytes[0] = 2;
  auto second_frozen = second.freeze_and_verify(
      test_type, test_schema, 2, artifact_digest(6), spec(),
      environment(owner, second_accountant), verifier);
  require(second_frozen.has_value(), "second freeze");
  auto generation_two =
      second.compare_and_publish(catalog, 1, generation_one.value());
  require(generation_two.has_value() && generation_two.value()->generation == 2,
          "second generation publish");
  require(generation_two.value()->prior_generation.get() ==
              generation_one.value().get(),
          "prior generation not retained");
  require(generation_two.value()->payload.get() != first_address &&
              generation_one.value()->payload->bytes == first_bytes,
          "successor mutated prior generation");
}

} // namespace

int main() {
  struct test_case {
    const char *name;
    void (*run)();
  } tests[] = {{"one_allocation_success", one_allocation_success},
               {"verification_failure_rolls_back",
                verification_failure_rolls_back},
               {"stale_binding_rolls_back", stale_binding_rolls_back},
               {"generations_are_isolated", generations_are_isolated}};
  int failures = 0;
  for (const auto &test : tests) {
    try {
      test.run();
      std::cout << "PASS " << test.name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
    }
  }
  return failures == 0 ? 0 : 1;
}
