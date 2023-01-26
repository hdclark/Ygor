//YgorLog.h - Written by hal clark in 2023.
//
// Routines for intra- and inter-process logging.
//

#pragma once

#include <iostream>
#include <ostream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <vector>
#include <utility>
#include <string>
#include <thread>
#include <functional>

#include "YgorDefinitions.h"

namespace ygor {

enum class log_level {
    info,
    warn,
    err
};

struct log_message {
    log_level ll;
    std::chrono::time_point<std::chrono::system_clock> t;
    std::string msg;
    std::string fn; // function name.
    std::thread::id tid;
    std::string sl; // source location.
    std::string fl; // filename.

    log_message(std::ostringstream &,
            log_level = log_level::info,
            std::thread::id = std::thread::id(),
            std::string func_name = "",
            std::string src_loc   = "",
            std::string file_name = "" );
};

class logger {
    public:
        using callback_t = std::function<void(log_message)>;
    private:
        std::recursive_mutex m;
        std::vector<log_message> msgs;
        std::vector<std::pair<std::thread::id, callback_t>> callbacks;

        void emit();

    public:
        logger();
        ~logger() noexcept;

        // Create and log a message.
        void operator()( log_message );

        // Subscribe when log messages are emitted.
        void push_local_callback( callback_t &&,
                                  std::thread::id = std::this_thread::get_id() );

        // Remove thread's latest callback. Returns true iff a callback was removed.
        bool pop_local_callback( std::thread::id = std::this_thread::get_id() );
};

// Global, shared logger object.
extern logger g_logger;

} // namespace ygor.


// Helpers.
#ifndef __PRETTY_FUNCTION__
    #ifdef __GNUC__ //If using gcc..
        #define __PRETTY_FUNCTION__ __func__
    #else
        #define __PRETTY_FUNCTION__ '(function name not available)'
    #endif
#endif


#ifndef YLOGINFO
    #define YLOGINFO( x )  { std::ostringstream os; \
                             os << x; \
                             ygor::g_logger( { os, \
                                  ygor::log_level::info, \
                                  std::this_thread::get_id(), \
                                  __PRETTY_FUNCTION__ }); }
#endif

#ifndef YLOGWARN
    #define YLOGWARN( x )  { std::ostringstream os; \
                             os << x; \
                             ygor::g_logger( { os, \
                                  ygor::log_level::warn, \
                                  std::this_thread::get_id(), \
                                  __PRETTY_FUNCTION__ }); }
#endif


#ifndef YLOGERR
    #define YLOGERR( x )   { std::ostringstream os; \
                             os << x; \
                             ygor::g_logger( { os, \
                                  ygor::log_level::err, \
                                  std::this_thread::get_id(), \
                                  __PRETTY_FUNCTION__ }); }

#endif

