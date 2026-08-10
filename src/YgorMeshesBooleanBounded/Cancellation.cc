#include "StrictFloatingBuild.h"
#include "../YgorMeshesBooleanBounded.h"

#include <atomic>

struct bounded_boolean_cancellation_source::state { std::atomic<std::uint32_t> reason{0}; };
bounded_boolean_cancellation_source::bounded_boolean_cancellation_source():state_(std::make_shared<state>()){}
bounded_boolean_cancellation_token bounded_boolean_cancellation_source::token()const noexcept{return bounded_boolean_cancellation_token(state_);}
void bounded_boolean_cancellation_source::request_cancel(std::uint32_t reason)noexcept{if(reason==0)reason=1;std::uint32_t expected=0;state_->reason.compare_exchange_strong(expected,reason,std::memory_order_release,std::memory_order_relaxed);}
bounded_boolean_cancellation_token::bounded_boolean_cancellation_token(std::shared_ptr<bounded_boolean_cancellation_source::state>s)noexcept:state_(std::move(s)){}
bool bounded_boolean_cancellation_token::cancellation_requested()const noexcept{return state_&&state_->reason.load(std::memory_order_acquire)!=0;}
std::uint32_t bounded_boolean_cancellation_token::reason()const noexcept{return state_?state_->reason.load(std::memory_order_acquire):0;}
