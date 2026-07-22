#pragma once

#include <cstdint>

namespace ygor::mesh_boolean::bounded {
enum class transaction_state:std::uint8_t{constructed=1,workspace_open=2,work_registered=3,joining=4,verifying=5,commit_ready=6,committed=7,rolling_back=8,rolled_back=9};
class stage_transaction {
  public:
    stage_transaction()noexcept=default;stage_transaction(const stage_transaction&)=delete;stage_transaction&operator=(const stage_transaction&)=delete;~stage_transaction(){if(state_!=transaction_state::committed&&state_!=transaction_state::rolled_back)rollback();}
    bool open()noexcept{return transition(transaction_state::constructed,transaction_state::workspace_open);}
    bool register_work()noexcept{return transition(transaction_state::workspace_open,transaction_state::work_registered);}
    bool begin_join()noexcept{return state_==transaction_state::workspace_open?transition(transaction_state::workspace_open,transaction_state::joining):transition(transaction_state::work_registered,transaction_state::joining);}
    bool begin_verify()noexcept{return transition(transaction_state::joining,transaction_state::verifying);}
    bool ready()noexcept{return transition(transaction_state::verifying,transaction_state::commit_ready);}
    bool commit()noexcept{return transition(transaction_state::commit_ready,transaction_state::committed);}
    void rollback()noexcept{if(state_==transaction_state::committed||state_==transaction_state::rolled_back)return;state_=transaction_state::rolling_back;state_=transaction_state::rolled_back;}
    transaction_state state()const noexcept{return state_;}
  private:bool transition(transaction_state from,transaction_state to)noexcept{if(state_!=from)return false;state_=to;return true;}transaction_state state_=transaction_state::constructed;
};
}
