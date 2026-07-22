#include "YgorMeshesBooleanBounded/FiniteInterval.h"
#include "YgorMeshesBooleanBounded/PrecisionVerifier.h"

#include <cstdint>
#include <cfenv>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace bounded = ygor::mesh_boolean::bounded;

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

template<class T>
void test_numeric_foundation() {
    const auto nonfinite = bounded::finite_interval<T>::singleton(
        std::numeric_limits<T>::infinity());
    require(nonfinite.lower() == -std::numeric_limits<T>::max() &&
            nonfinite.upper() == std::numeric_limits<T>::max(),
            "nonfinite singleton must conservatively fail closed");

    const auto a = *bounded::finite_interval<T>::create(T(-1), T(2));
    const auto b = *bounded::finite_interval<T>::create(T(0), T(3));
    bounded::interval_intersection_proof forged{
        1, 2, bounded::interval_constraint_kind::exact_identity, 7, 9};
    require(!bounded::verify_interval_intersection(a, b, forged) &&
            bounded::interval_intersection(a, b, forged).status ==
                bounded::numeric_status::invalid_argument,
            "forged exact-identity evidence must fail closed");

    bounded::interval_intersection_proof geometric{
        1, 2, bounded::interval_constraint_kind::independent_geometric_bound, 7, 9};
    auto evidence = bounded::verify_interval_intersection(a, b, geometric);
    require(evidence && bounded::interval_intersection(a, b, *evidence),
            "verified interval evidence must permit its bound intersection");
    require(bounded::interval_intersection(a, b, geometric).status ==
                bounded::numeric_status::invalid_argument,
            "legacy aggregate evidence must fail closed");
    const auto other = *bounded::finite_interval<T>::create(T(0), T(1));
    require(bounded::interval_intersection(other, b, *evidence).status ==
                bounded::numeric_status::invalid_argument,
            "interval evidence must be bound to both parent intervals");

    if (!bounded::runtime_floating_profile_qualified<T>()) {
        std::cerr << "runtime qualification failed for " << sizeof(T) << "-byte scalar\n";
        require(false, "strict runtime floating profile known answers");
    }
}

template<class T, class I>
void test_bootstrap_hardening() {
    using pending_type = bounded::pending_invocation<T,I>;
    using pending_view = bounded::pending_boolean_context_view<T,I>;
    static_assert(!std::is_constructible_v<pending_view, pending_type &&>);

    fv_surface_mesh<T,I> a;
    fv_surface_mesh<T,I> b;
    a.vertices.emplace_back(T(-0.0), std::numeric_limits<T>::denorm_min(), T(1));
    b.vertices.emplace_back(T(2), T(3), T(4));
    bounded_boolean_options<T> options;
    options.tolerance = T(1);
    auto pending = bounded::build_pending_invocation(
        a, b, boolean_operation::intersection, options);
    require(pending.has_value(), "build pending invocation");

    bounded::precision_bootstrap_capabilities asserted_strict;
    asserted_strict.strict_build = true;
    require(std::fesetround(FE_DOWNWARD) == 0 &&
            !bounded::preflight_precision(*pending.value(), asserted_strict).has_value(),
            "mutable strict-build assertion cannot override runtime qualification");
    require(std::fesetround(FE_TONEAREST) == 0, "restore nearest-even rounding");

    pending_view owned_view(*pending.value());
    auto preflight = bounded::preflight_precision(owned_view);
    require(preflight.has_value() &&
            bounded::verify_precision_preflight(*preflight.value(), owned_view),
            "owned pending view and independent preflight verification");

    auto nonfinite_record = *preflight.value();
    nonfinite_record.tolerance = std::numeric_limits<T>::infinity();
    const auto nonfinite_bytes = bounded::encode_precision_preflight(nonfinite_record);
    require(!bounded::decode_precision_preflight<T>(nonfinite_bytes).has_value(),
            "codec rejects nonfinite preflight scalar");

    auto negative_record = *preflight.value();
    negative_record.required_machine_floor = T(-1);
    require(!bounded::decode_precision_preflight<T>(
                 bounded::encode_precision_preflight(negative_record)).has_value(),
            "codec rejects negative precision scalar");

    auto inverted_record = *preflight.value();
    inverted_record.operand_a.axis[0].minimum = T(4);
    inverted_record.operand_a.axis[0].maximum = T(3);
    require(!bounded::decode_precision_preflight<T>(
                 bounded::encode_precision_preflight(inverted_record)).has_value(),
            "codec rejects inverted scale axis");

    const auto bootstrap = bounded::make_precision_bootstrap_record(*preflight.value());
    auto context = bounded::finalize_context(std::move(*pending.value()), bootstrap);
    require(context.has_value(), "finalize boolean context");

    auto forged = *preflight.value();
    forged.operand_a.machine_floor = T(0);
    forged.canonical_bytes = bounded::precision_detail::encode_preflight_payload(forged);
    forged.digest = bounded::sha256::digest(forged.canonical_bytes);
    require(!bounded::build_precision_context(forged, *context.value()).has_value(),
            "builder independently reconstructs forged preflight fields");

    auto frozen = bounded::build_precision_context(*preflight.value(), *context.value());
    require(frozen.has_value(), "build verified precision context");

    auto bad_header = *preflight.value();
    bad_header.scalar_profile_version = 0;
    bad_header.canonical_bytes = bounded::precision_detail::encode_preflight_payload(bad_header);
    bad_header.digest = bounded::sha256::digest(bad_header.canonical_bytes);
    require(!bounded::build_precision_context(bad_header, *context.value()).has_value() &&
            !bounded::verify_precision_context(
                **frozen.value(), bad_header, *context.value()),
            "builder and verifier reject every preflight header");

    auto context_bytes = bounded::encode_precision_context(**frozen.value());
    using U = bounded::floating_uint_t<T>;
    const U infinity = bounded::to_bits(std::numeric_limits<T>::infinity());
    constexpr std::size_t tolerance_offset = 16;
    for (std::size_t i = 0; i < sizeof(U); ++i) {
        context_bytes[tolerance_offset + i] =
            static_cast<std::uint8_t>(infinity >> (8 * i));
    }
    require(!bounded::decode_precision_context<T>(context_bytes).has_value(),
            "context codec rejects nonfinite local semantics");

    bounded::precision_context_view<T> owner_view(
        **frozen.value(), bounded::context_owner_token(context.value()->owner));
    require(owner_view.valid(), "precision view stores temporary owner token safely");
}

} // namespace

int main() {
    try {
        require(std::fesetenv(FE_DFL_ENV) == 0 && std::fesetround(FE_TONEAREST) == 0,
                "establish qualified floating environment");
        test_numeric_foundation<float>();
        test_numeric_foundation<double>();
        test_bootstrap_hardening<float, std::uint32_t>();
        test_bootstrap_hardening<double, std::uint64_t>();
        std::cout << "Precision numeric hardening tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
