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
                         std::string src_loc,
                         std::string file_name) : ll(log_level),
                                                  t( std::chrono::system_clock::now() ),
                                                  msg( os.str() ),
                                                  fn(func_name),
                                                  tid( id ),
                                                  sl(src_loc),
                                                  fl(file_name) {};


logger::logger(){
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

    // Cache info about the host, process, etc.
    //
    // ...
    

    // std::cout << "*** Initialized logger ***" << std::endl;
}

logger::~logger(){
    const std::lock_guard<std::recursive_mutex> lock(this->m);
    this->emit();
}


void
logger::emit(){
    // Use a recursive lock here because this routine can cause program to exit, and thus the destructor to be called,
    // but the destructor calls this routine to flush messages.
    const std::lock_guard<std::recursive_mutex> lock(this->m);

    // Write messages over network.
    // ...

    // Flush to the console.
    for(const auto& msg : this->msgs){
        const std::time_t t_conv = std::chrono::system_clock::to_time_t(msg.t);

        std::ostream *os = &(std::cout);
        if(msg.ll == log_level::err) os = &(std::cerr);

        *os << "--(" << log_level_to_string(msg.ll) << ")";
        *os << " " << std::put_time(std::localtime(&t_conv), "%Y%m%d-%H%M%S");
        if( msg.tid != std::thread::id() ) *os << " thread " << std::hex << msg.tid << std::dec;
        if( !msg.fn.empty() ) *os << " function '" << msg.fn << "'";
        if( !msg.fl.empty() ) *os << " file '" << msg.fl << "'";
        if( !msg.sl.empty() ) *os << " line " << msg.sl;
        *os << ": " << msg.msg << ".";

        if(msg.ll == log_level::err) *os << " Terminating program.";
        *os << std::endl;
        os->flush();

        if(msg.ll == log_level::err){
            msgs.clear();
            std::exit(-1);
        }
    }

    // Perform callbacks.
    //
    // Note: this should always be executed last since they can do anything, e.g., terminate the process.
    // Emitting messages to other log sinks first helps ensure messages are not lost.
    for(const auto& msg : this->msgs){
        for(const auto &c_p : this->callbacks){
            if(c_p.second) std::invoke(c_p.second, msg);
        }
    }

    msgs.clear();
}

void
logger::operator()(log_message lm){
    const std::lock_guard<std::recursive_mutex> lock(this->m);
    msgs.emplace_back(lm);

    // If the error is critical, call the destructor to forward messages, flush, and terminate ASAP.
    if(msgs.back().ll == log_level::err){
        this->emit();
    }

    // Emit eagerly.
    //
    // NOTE: this behaviour is subject to change!! TODO
    this->emit();
}


void
logger::push_local_callback( ygor::logger::callback_t &&f,
                             std::thread::id id ){
    const std::lock_guard<std::recursive_mutex> lock(this->m);
    callbacks.emplace_back(std::make_pair(id, std::move(f))); //<std::thread::id,
                                          //ygor::logger::callback_t>(id, f));
}

bool
logger::pop_local_callback( std::thread::id id ){
    const std::lock_guard<std::recursive_mutex> lock(this->m);

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

