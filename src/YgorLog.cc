//YgorLog.cc - A part of Ygor, 2023. Written by hal clark.

#include <iostream>
#include <ostream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>
#include <cstdlib>
#include <thread>
#include <iomanip>

#include "YgorDefinitions.h"
#include "YgorLog.h"



namespace ygor {

// Global, shared logger object.
logger g_logger;


std::string log_level_to_string(log_level ll){
    std::string out;
    if(ll == log_level::info){
        out = "I";
    }else if(ll == log_level::warn){
        out = "W";
    }else if(ll == log_level::err){
        out = "E";
    }else{
        out = "?";
    }
    return out;
}

log_message::log_message(std::ostringstream &os,
                         log_level log_level,
                         std::thread::id id,
                         std::string func_name,
                         int64_t src_loc,
                         std::string file_name) : ll(log_level),
                                                  t( std::chrono::system_clock::now() ),
                                                  msg( os.str() ),
                                                  fn(func_name),
                                                  tid( id ),
                                                  sl(src_loc),
                                                  fl(file_name) {};


logger::logger(){
    std::lock_guard<std::mutex> lock(this->m);

    // Determine logging preferences from environment variables.
    // ...
    //std::string address;
    //std::string port;
    //if(const char *c_address  = std::getenv("YGOR_LOG_ADDRESS"); nullptr != c_address){
    //    address = std::string(c_address);
    //}
    //if(const char *c_port  = std::getenv("YGOR_LOG_PORT"); nullptr != c_port){
    //    port = std::string(c_port);
    //}
    //
    // ...

    // Override the message sinks.
    if(const char *c_method  = std::getenv("YLOG_METHODS"); nullptr != c_method){
        std::string x(c_method);
        this->emit_terminal =  (x.find("terminal") != std::string::npos)
                            || (x.find("console")  != std::string::npos)
                            || (x.find("stdout")   != std::string::npos);
    }

    // Override the default terminal emission format.
    if(const char *c_term  = std::getenv("YLOG_TERMINAL"); nullptr != c_term){
        std::string x(c_term);
        this->terminal_emit_t   = (x.find("time")     != std::string::npos);
        this->terminal_emit_tid = (x.find("thread")   != std::string::npos);
        this->terminal_emit_fn  = (x.find("function") != std::string::npos);
        this->terminal_emit_sl  = (x.find("source")   != std::string::npos);
        this->terminal_emit_fl  = (x.find("filename") != std::string::npos);
    }

    // Cache info about the host, process, etc.
    //
    // ...
    

    // std::cout << "*** Initialized logger ***" << std::endl;
}

logger::~logger(){
    this->emit();
}


void
logger::emit(){
    // Move or make copies of all relevant shared state.
    //
    // Copying here is painful, but copying and locking only during the copy helps avoid logging delays and deadlock
    // inside callbacks.
    //
    // Alternatively we could probably use shared_ptr and some sort of immutability handshake. In practice, I don't
    // ever want any of these logging facilities to cause or contribute to memory issues.
    decltype(this->msgs) l_msgs;
    decltype(this->callbacks) l_callbacks;
    bool l_emit_terminal     = false;
    bool l_terminal_emit_t   = false;
    bool l_terminal_emit_tid = false;
    bool l_terminal_emit_fn  = false;
    bool l_terminal_emit_fl  = false;
    bool l_terminal_emit_sl  = false;
    {
        // Use a recursive lock here because this routine can cause program to exit, and thus the destructor to be called,
        // but the destructor calls this routine to flush messages.
        std::lock_guard<std::mutex> lock(this->m);
        std::swap(this->msgs, l_msgs);
        l_callbacks = this->callbacks;
        l_emit_terminal     = this->emit_terminal;
        l_terminal_emit_t   = this->terminal_emit_t;
        l_terminal_emit_tid = this->terminal_emit_tid;
        l_terminal_emit_fn  = this->terminal_emit_fn;
        l_terminal_emit_fl  = this->terminal_emit_fl;
        l_terminal_emit_sl  = this->terminal_emit_sl;
    }

    // Write messages over network.
    // ...

    // Flush to the console.
    if(l_emit_terminal){
        for(const auto& msg : l_msgs){
            const std::time_t t_conv = std::chrono::system_clock::to_time_t(msg.t);

            std::ostream *os = &(std::cout);
            if(msg.ll == log_level::err) os = &(std::cerr);

            *os << "--(" << log_level_to_string(msg.ll) << ")";

            if( l_terminal_emit_t ){
                *os << " " << std::put_time(std::localtime(&t_conv), "%Y%m%d-%H%M%S");
            }
            if( l_terminal_emit_tid
            &&  (msg.tid != std::thread::id()) ){
                *os << " thread " << std::hex << msg.tid << std::dec;
            }
            if( l_terminal_emit_fn
            &&  !msg.fn.empty() ){
                *os << " function '" << msg.fn << "'";
            }
            if( l_terminal_emit_fl
            &&  !msg.fl.empty() ){
                *os << " file '" << msg.fl << "'";
            }
            if( l_terminal_emit_sl
            &&  (0 <= msg.sl) ){
                *os << " line " << msg.sl;
            }

            *os << ": " << msg.msg << ".";

            if(msg.ll == log_level::err){
                *os << " Terminating program.";
            }

            *os << std::endl;
            os->flush();

            if(msg.ll == log_level::err){
                msgs.clear();
                std::exit(-1);
            }
        }
    }

    // Perform callbacks.
    //
    // Note: this should always be executed last since they can do anything, e.g., terminate the process.
    // Emitting messages to other log sinks first helps ensure messages are not lost.
    for(const auto& msg : l_msgs){
        for(const auto &c_p : l_callbacks){
            if(c_p.second) std::invoke(c_p.second, msg);
        }
    }
}

void
logger::operator()(log_message lm){
    {
        const std::lock_guard<std::mutex> lock(this->m);
        msgs.emplace_back(lm);
    }

    // If the error is critical, call the destructor to forward messages, flush, and terminate ASAP.
    if(lm.ll == log_level::err){
        this->emit();

    // Otherwise either defer or emit eagerly, depending on queue size and priorities.
    }else{
        // Note: uniformly emitting eagerly for now. This behaviour is subject to change!! TODO
        this->emit();
    }
}


void
logger::push_local_callback( ygor::logger::callback_t &&f,
                             std::thread::id id ){
    const std::lock_guard<std::mutex> lock(this->m);
    callbacks.emplace_back(std::make_pair(id, std::move(f)));
}

bool
logger::pop_local_callback( std::thread::id id ){
    const std::lock_guard<std::mutex> lock(this->m);

    // Remove only the most recently pushed callback for the given thread.
    bool found = false;
    const auto N = this->callbacks.size();
    for(size_t i = 0U; i < N; ++i){
        const size_t j = (N - 1U) - i;
        if(this->callbacks[j].first == id){
            found = true;
            const auto it = std::next( std::begin(this->callbacks), j );
            this->callbacks.erase(it);
            break;
        }
    }
    return found;
}

} // namespace ygor.

