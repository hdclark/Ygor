#pragma once
#ifndef YGOR_MESHES_BOOLEAN_EXECUTOR_H_
#define YGOR_MESHES_BOOLEAN_EXECUTOR_H_
#include "YgorMeshesBooleanContract.h"
#include "YgorMeshesBooleanPerformance.h"
#include <algorithm>
#include <condition_variable>
#include <future>
#include <thread>

namespace ygor { namespace mesh_boolean {
struct deterministic_task { std::uint64_t key=0; std::function<status_or<bool>()> work; };
class deterministic_executor {
    execution_policy policy_;
public:
    explicit deterministic_executor(execution_policy p):policy_(p){}
    const execution_policy& policy()const{return policy_;}
    status_or<bool> run(std::vector<deterministic_task> tasks, cancellation_token cancel) const {
        std::sort(tasks.begin(),tasks.end(),[](const auto&a,const auto&b){return a.key<b.key;});
        const auto performance_context=capture_performance_task_context();
        std::vector<std::future<status_or<bool>>> active;
        std::size_t next=0;
        while(next<tasks.size()||!active.empty()){
            while(next<tasks.size()&&active.size()<policy_.max_threads){
                auto task=std::move(tasks[next++]);auto key=task.key;auto work=std::move(task.work);
                active.emplace_back(std::async(std::launch::async,[work=std::move(work),key,performance_context]()mutable{
                    performance_scope performance(performance_context.collector,
                        performance_context.stage,performance_context.role,key,false);
                    try{return work();}catch(const std::bad_alloc&){return status_or<bool>(make_error(boolean_error_code::resource_limit,boolean_stage::context_setup,"allocation"));}
                    catch(const std::exception&e){auto x=make_error(boolean_error_code::internal_invariant_error,boolean_stage::context_setup,"task_exception");x.detail=e.what();return status_or<bool>(std::move(x));}
                    catch(...){return status_or<bool>(make_error(boolean_error_code::internal_invariant_error,boolean_stage::context_setup,"task_exception_unknown"));}
                }));
            }
            auto result=active.front().get(); active.erase(active.begin());
            if(!result.has_value()){for(auto&f:active)f.wait();return result.error();}
            if(cancel.cancelled()){for(auto&f:active)f.wait();return make_error(boolean_error_code::resource_limit,boolean_stage::context_setup,"cancelled");}
        }
        return true;
    }
};
} }
#endif
