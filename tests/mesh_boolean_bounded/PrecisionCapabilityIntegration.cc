#include "YgorMeshesBooleanBounded/PrecisionCapabilities.h"

#include <type_traits>

#ifndef YGOR_PRECISION_CAPABILITY_CONSUMER
#error "A consuming component number is required"
#endif

namespace {
using namespace ygor::mesh_boolean::bounded;

template<int Component, class T>
struct capability_consumer_compile_probe {
    static_assert(Component > 0, "consumer component must be identified");
    static_assert(std::is_same_v<
                  decltype(component_03_capabilities<T>{}.arithmetic),
                  bounded_arithmetic_capability<T>>);
    static_assert(std::is_same_v<
                  decltype(component_03_capabilities<T>{}.bounds),
                  conservative_bounds_capability<T>>);
    static_assert(std::is_same_v<
                  decltype(component_03_capabilities<T>{}.read),
                  precision_read_capability>);
    static_assert(std::is_same_v<
                  decltype(component_03_capabilities<T>{}.mutation),
                  precision_mutation_capability>);
};

template<int Component, class T> struct named_capability;
template<class T> struct named_capability<2, T> { using type = component_02_precision_capabilities<T>; };
template<class T> struct named_capability<4, T> { using type = component_04_precision_capabilities<T>; };
template<class T> struct named_capability<6, T> { using type = component_06_precision_capabilities<T>; };
template<class T> struct named_capability<7, T> { using type = component_07_precision_capabilities<T>; };
template<class T> struct named_capability<13, T> { using type = component_13_precision_capabilities; };
template<class T> struct named_capability<15, T> { using type = component_15_precision_capabilities; };
template<class T> struct named_capability<16, T> { using type = component_16_precision_capabilities<T>; };
template<class T> struct named_capability<17, T> { using type = component_17_precision_capabilities<T>; };

using named_consumer_capability = typename named_capability<
    YGOR_PRECISION_CAPABILITY_CONSUMER, double>::type;
static_assert(std::is_default_constructible_v<named_consumer_capability>);

[[maybe_unused]] void named_factory_probe(const context_owner_token &owner,
                                          precision_ledger &ledger,
                                          tolerance_budget &budget) {
    auto capabilities = make_named_precision_capabilities<double>(owner, ledger, budget);
    (void)capabilities;
}

[[maybe_unused]] capability_consumer_compile_probe<
    YGOR_PRECISION_CAPABILITY_CONSUMER, double> consumer_probe;
} // namespace
