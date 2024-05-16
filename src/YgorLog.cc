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
#include <string>
#include <ctime>
#include <array>
#include <exception>
#include <algorithm>

#include "YgorDefinitions.h"
#include "YgorLog.h"


// Equivalent of YGORWARN macro for emitting warnings from the implementation itself.
#ifndef L_EMIT_SELF_WARN
    #define L_EMIT_SELF_WARN( x )  { std::ostringstream os; \
                                     os << x; \
                                     (*this)({ os, \
                                          ygor::log_level::warn, \
                                          std::this_thread::get_id(), \
                                          YLOG_PRETTY_FUNCTION, \
                                          YLOG_PRETTY_SRC_LOC, \
                                          YLOG_PRETTY_FILENAME }); }
#endif


namespace ygor {

// Global, shared logger object.
logger g_logger;

// Global mutex for cout/cerr terminal message synchronization.
std::mutex g_term_sync;


std::string log_level_to_string(log_level ll){
    std::string out;
    if(ll == log_level::debug){
        out = "D";
    }else if(ll == log_level::info){
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

static
void
increase_log_level_verbosity(log_level &ll){
    switch(ll){
        case log_level::debug:
            // Cannot increase verbosity further.
            break;
        case log_level::info:
            ll = log_level::debug;
            break;
        case log_level::warn:
            ll = log_level::info;
            break;
        case log_level::err:
            ll = log_level::warn;
            break;
    }
}

static
void
decrease_log_level_verbosity(log_level &ll){
    switch(ll){
        case log_level::debug:
            ll = log_level::info;
            break;
        case log_level::info:
            ll = log_level::warn;
            break;
        case log_level::warn:
            ll = log_level::err;
            break;
        case log_level::err:
            // Cannot decrease verbosity further.
            break;
    }
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
                                                  fl(file_name) {}


logger::logger(){
    std::lock_guard<std::mutex> lock(this->m);
    this->callback_id_nidus = static_cast<callback_id_t>(0);

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

    // Verbosity overrides.
    const auto search_for_log_level = [](const std::string &x, log_level &ll) -> bool {
        bool was_found = true;
        if(false){
        }else if(x.find("err")   != std::string::npos){ ll = log_level::err;
        }else if(x.find("warn")  != std::string::npos){ ll = log_level::warn;
        }else if(x.find("info")  != std::string::npos){ ll = log_level::info;
        }else if(x.find("debug") != std::string::npos){ ll = log_level::debug;
        }else{ was_found = false; }
        return was_found;
    };

    if(const char *c_term  = std::getenv("YLOG_VERBOSITY"); nullptr != c_term){
        // Override all emitters.
        //
        // The use-case for this emitter is usually to either fully silence all messages, or enable verbose debugging
        // for all messages.
        std::string x(c_term);
        if( !search_for_log_level(x, this->terminal_emit_min_level)
        ||  !search_for_log_level(x, this->callback_emit_min_level) ){
            L_EMIT_SELF_WARN("Environment variable YLOG_VERBOSITY is set, but does not contain a recognizable log level. Ignoring");
        }
    }
    if(const char *c_term  = std::getenv("YLOG_TERMINAL_VERBOSITY"); nullptr != c_term){
        std::string x(c_term);
        if( !search_for_log_level(x, this->terminal_emit_min_level) ){
            L_EMIT_SELF_WARN("Environment variable YLOG_TERMINAL_VERBOSITY is set, but does not contain a recognizable log level. Ignoring");
        }
    }
    if(const char *c_term  = std::getenv("YLOG_CALLBACK_VERBOSITY"); nullptr != c_term){
        std::string x(c_term);
        if( !search_for_log_level(x, this->callback_emit_min_level) ){
            L_EMIT_SELF_WARN("Environment variable YLOG_CALLBACK_VERBOSITY is set, but does not contain a recognizable log level. Ignoring");
        }
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

logger::~logger() noexcept {
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
    log_level l_terminal_emit_min_level = log_level::debug;
    log_level l_callback_emit_min_level = log_level::debug;
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
        l_terminal_emit_min_level = this->terminal_emit_min_level;
        l_callback_emit_min_level = this->terminal_emit_min_level;
    }

    // Write messages over network.
    // ...

    // Flush to the terminal/console.
    if(l_emit_terminal){
        for(const auto& msg : l_msgs){
            if(msg.ll < l_terminal_emit_min_level) continue;

            const std::time_t t_conv = std::chrono::system_clock::to_time_t(msg.t);

            std::ostream *os = &(std::cout);
            if(msg.ll == log_level::err) os = &(std::cerr);

            std::lock_guard<std::mutex> lock(g_term_sync);
            *os << "--(" << log_level_to_string(msg.ll) << ")";

            if( l_terminal_emit_t ){
                *os << " " << get_localtime_str(t_conv);
            }
            if( l_terminal_emit_tid
            &&  (msg.tid != std::thread::id()) ){
                *os << " thread 0x" << std::hex << msg.tid << std::dec;
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
        if(msg.ll < l_callback_emit_min_level) continue;

        for(const auto &c_p : l_callbacks){
            if(c_p.second){
                try{
                    std::invoke(c_p.second, msg);
                }catch(const std::exception &){ }
            }
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
logger::set_min_level(log_level ll){
    const std::lock_guard<std::mutex> lock(this->m);
    this->terminal_emit_min_level = ll;
    this->callback_emit_min_level = ll;
}

void
logger::increase_verbosity(){
    this->increase_callback_verbosity();
    this->increase_terminal_verbosity();
}

void
logger::decrease_verbosity(){
    this->decrease_callback_verbosity();
    this->decrease_terminal_verbosity();
}


log_level
logger::get_callback_min_level(){
    const std::lock_guard<std::mutex> lock(this->m);
    auto ll = this->callback_emit_min_level;
    return ll;
}

void
logger::set_callback_min_level(log_level ll){
    const std::lock_guard<std::mutex> lock(this->m);
    this->callback_emit_min_level = ll;
}

void
logger::increase_callback_verbosity(){
    const std::lock_guard<std::mutex> lock(this->m);
    increase_log_level_verbosity(this->callback_emit_min_level);
}

void
logger::decrease_callback_verbosity(){
    const std::lock_guard<std::mutex> lock(this->m);
    decrease_log_level_verbosity(this->callback_emit_min_level);
}


log_level
logger::get_terminal_min_level(){
    const std::lock_guard<std::mutex> lock(this->m);
    auto ll = this->terminal_emit_min_level;
    return ll;
}

void
logger::set_terminal_min_level(log_level ll){
    const std::lock_guard<std::mutex> lock(this->m);
    this->terminal_emit_min_level = ll;
}

void
logger::increase_terminal_verbosity(){
    const std::lock_guard<std::mutex> lock(this->m);
    increase_log_level_verbosity(this->terminal_emit_min_level);
}

void
logger::decrease_terminal_verbosity(){
    const std::lock_guard<std::mutex> lock(this->m);
    decrease_log_level_verbosity(this->terminal_emit_min_level);
}


logger::callback_id_t
logger::push_callback( ygor::logger::callback_t &&f ){
    const std::lock_guard<std::mutex> lock(this->m);
    auto l_id = this->callback_id_nidus++;
    callbacks.emplace_back(std::make_pair(l_id, std::move(f)));
    return l_id;
}

bool
logger::pop_callback( logger::callback_id_t id ){
    const std::lock_guard<std::mutex> lock(this->m);

    const auto N_before = this->callbacks.size();
    this->callbacks.erase(
        std::remove_if(this->callbacks.begin(),
                       this->callbacks.end(),
                       [id](const std::pair<logger::callback_id_t, ygor::logger::callback_t> &c){
                           return (c.first == id);
                       }),
                       this->callbacks.end() );

    const auto N_after = this->callbacks.size();
    return (N_before != N_after);
}

scoped_callback::scoped_callback( logger::callback_t &&f ){
    this->callback_id = g_logger.push_callback(std::move(f));
}

scoped_callback::~scoped_callback() noexcept {
    g_logger.pop_callback(this->callback_id);
}



std::string
get_localtime_str( std::time_t t,
                   const std::string &format ){

    std::array<char, 100> t_arr;
    t_arr.fill('\0');
    tm working; // Working space for thread-safe localtime_r() variant of std::localtime().
                // Also works for non-standard localtime_s() variant.

    if(
#if defined(_WIN32) || defined(_WIN64)
        // Note: this is a non-standard localtime_s that does *not* return a pointer.
        (::localtime_s(&working, &t) != 0) // Returns zero on success
#else
        // Note: returns pointer to working tm structure.
        (::localtime_r(&t, &working) == nullptr)
#endif
        //// Note: can also use stream emitter here:
        //ss << std::put_time(std::localtime_r(&t), format.c_str());
    ||  (!std::strftime(t_arr.data(),
                        t_arr.size() - 1UL,
                        format.c_str(),
                        &working)) ){
        throw std::runtime_error("Unable to get current time.");
    }

    return std::string(t_arr.data());
}


} // namespace ygor.

