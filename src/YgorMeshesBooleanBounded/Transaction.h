#pragma once

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {
enum class transaction_state:std::uint8_t{constructed=1,workspace_open=2,work_registered=3,joining=4,verifying=5,commit_ready=6,committed=7,rolling_back=8,rolled_back=9};
class transaction_participant {
  public:
    virtual ~transaction_participant()=default;
    virtual bool prepare()=0;
    virtual void commit()noexcept=0;
    virtual void rollback()noexcept=0;
};
class stage_transaction {
  public:
    stage_transaction()noexcept=default;stage_transaction(const stage_transaction&)=delete;stage_transaction&operator=(const stage_transaction&)=delete;~stage_transaction(){if(state_!=transaction_state::committed&&state_!=transaction_state::rolled_back)rollback();}
    bool open()noexcept{return transition(transaction_state::constructed,transaction_state::workspace_open);}
    bool enlist(transaction_participant&participant)noexcept{if(state_!=transaction_state::workspace_open)return false;try{participants_.push_back(&participant);}catch(...){return false;}return true;}
    bool register_work()noexcept{return transition(transaction_state::workspace_open,transaction_state::work_registered);}
    bool begin_join()noexcept{return state_==transaction_state::workspace_open?transition(transaction_state::workspace_open,transaction_state::joining):transition(transaction_state::work_registered,transaction_state::joining);}
    bool begin_verify()noexcept{return transition(transaction_state::joining,transaction_state::verifying);}
    bool ready()noexcept{if(state_!=transaction_state::verifying)return false;while(prepared_!=participants_.size()){auto*participant=participants_[prepared_];try{if(!participant->prepare()){participant->rollback();rollback();return false;}}catch(...){participant->rollback();rollback();return false;}++prepared_;}state_=transaction_state::commit_ready;return true;}
    bool commit()noexcept{if(state_!=transaction_state::commit_ready)return false;for(auto*participant:participants_)participant->commit();state_=transaction_state::committed;participants_.clear();prepared_=0;return true;}
    void rollback()noexcept{if(state_==transaction_state::committed||state_==transaction_state::rolled_back)return;state_=transaction_state::rolling_back;while(prepared_!=0)participants_[--prepared_]->rollback();participants_.clear();state_=transaction_state::rolled_back;}
    transaction_state state()const noexcept{return state_;}
  private:bool transition(transaction_state from,transaction_state to)noexcept{if(state_!=from)return false;state_=to;return true;}transaction_state state_=transaction_state::constructed;std::vector<transaction_participant*>participants_;std::size_t prepared_=0;
};
}
