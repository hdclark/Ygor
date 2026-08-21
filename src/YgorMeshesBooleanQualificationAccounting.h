#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_ACCOUNTING_FACADE_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_ACCOUNTING_FACADE_H_

#include "YgorMeshesBooleanQualificationAccountingImpl.h"

namespace ygor {
namespace mesh_boolean {
namespace qualification_accounting_facade_detail {
template <class Value> struct non_deduced { using type = Value; };
} // namespace qualification_accounting_facade_detail

// Let ordinary concrete service implementations participate in overload
// resolution without forcing callers to spell base-class shared_ptr casts.
// T and I remain authoritative from the product result, and forwarding lands
// in the frozen observer implementation with its exact service contracts.
template <
    class T, class I, class Kernel, class Verifier,
    typename std::enable_if<
        std::is_convertible<
            std::shared_ptr<Kernel>,
            std::shared_ptr<const exact_kernel_services<T>>>::value &&
            std::is_convertible<
                std::shared_ptr<Verifier>,
                std::shared_ptr<const verifier_service>>::value &&
            (!std::is_same<Kernel,
                           const exact_kernel_services<T>>::value ||
             !std::is_same<Verifier, const verifier_service>::value),
        int>::type = 0>
product_status_or<qualification_success_verification>
observe_qualification_product_success(
    const boolean_product_result<T, I> &result, std::shared_ptr<Kernel> kernel,
    std::shared_ptr<Verifier> verifiers,
    std::vector<qualification_guarded_probe_observation> probes = {},
    bool require_chain_reingestion = false,
    typename qualification_accounting_facade_detail::non_deduced<
        qualification_chain_mesh_consumer<T, I>>::type chain_consumer = {}) {
  return observe_qualification_product_success<T, I>(
      result,
      std::shared_ptr<const exact_kernel_services<T>>(std::move(kernel)),
      std::shared_ptr<const verifier_service>(std::move(verifiers)),
      std::move(probes), require_chain_reingestion, std::move(chain_consumer));
}

} // namespace mesh_boolean
} // namespace ygor

#endif
