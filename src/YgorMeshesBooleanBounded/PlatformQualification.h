#pragma once

#include "Outcome.h"

#include <cfenv>
#include <limits>
#include <type_traits>

namespace ygor::mesh_boolean::bounded {
class floating_environment_guard {
  public:
    floating_environment_guard() noexcept : captured_(std::fegetenv(&saved_)==0) { qualified_=captured_&&std::fesetround(FE_TONEAREST)==0&&std::fegetround()==FE_TONEAREST; }
    ~floating_environment_guard(){if(captured_)std::fesetenv(&saved_);}
    bool qualified()const noexcept{return qualified_;}
  private:std::fenv_t saved_{};bool captured_=false,qualified_=false;
};
template<class T,class I>constexpr bool supported_type_profile()noexcept{return(std::is_same_v<T,float>||std::is_same_v<T,double>)&&(std::is_same_v<I,std::uint32_t>||std::is_same_v<I,std::uint64_t>)&&std::numeric_limits<T>::is_iec559&&std::numeric_limits<T>::radix==2;}
}
