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
#include <ctime>

#include "YgorDefinitions.h"

namespace ygor {

// Log severity or level.
enum class log_level {
    debug = 50,
    info  = 60,
    warn  = 70,
    err   = 80
};

// Helper to convert the above enum into a string.
std::string log_level_to_string(log_level);

// Class representing an individual log message with optional metadata.
struct log_message {
    log_level ll;
    std::chrono::time_point<std::chrono::system_clock> t;
    std::string msg;
    std::string fn; // function name.
    std::thread::id tid;
    int64_t sl; // source location.
    std::string fl; // filename.

    log_message(std::ostringstream &,
            log_level = log_level::info,
            std::thread::id = std::thread::id(),
            std::string func_name = "",
            int64_t src_loc       = -1,
            std::string file_name = "" );
};

// Main logging class. Received log messages and distributes them accordingly.
class logger {
    public:
        using callback_t = std::function<void(log_message)>;
        using callback_id_t = int64_t;
    private:
        std::mutex m;
        std::vector<log_message> msgs;
        std::vector<std::pair<callback_id_t, callback_t>> callbacks;
        callback_id_t callback_id_nidus;

        void emit();

        bool emit_terminal     = true;

        bool terminal_emit_t   = true;
        bool terminal_emit_tid = true;
        bool terminal_emit_fn  = true;
        bool terminal_emit_sl  = false;
        bool terminal_emit_fl  = false;
        log_level terminal_emit_min_level = log_level::info;

        log_level callback_emit_min_level = log_level::info;

    public:
        logger();
        ~logger() noexcept;

        // Create and log a message.
        void operator()( log_message );

        // Query and set the minimum severity a message must have to be emitted (i.e., 'verbosity').
        //
        // Note that this will override defaults and environment variables, which are both set/queried at construction.
        void set_min_level(log_level); // Applies to all emitters.
        void increase_verbosity();
        void decrease_verbosity();

        log_level get_callback_min_level(); // Applies to callback messages only.
        void set_callback_min_level(log_level);
        void increase_callback_verbosity();
        void decrease_callback_verbosity();

        log_level get_terminal_min_level();  // Applies to terminal messages only.
        void set_terminal_min_level(log_level);
        void increase_terminal_verbosity();
        void decrease_terminal_verbosity();

        // Provide a callback for emitted log messages.
        callback_id_t push_callback( callback_t && );

        // Remove callback with the given ID. Returns true iff a callback was removed.
        bool pop_callback( callback_id_t );
};

// Class to tie the lifetime of a callback to a given scope.
// Will add the callback on instantiation, and remove it upon destruction.
class scoped_callback {
    private:
        logger::callback_id_t callback_id;

    public:
        scoped_callback( logger::callback_t && );
        ~scoped_callback() noexcept;
};

// Global, shared logger object.
extern logger g_logger;

// Global mutex for cout/cerr terminal message synchronization.
extern std::mutex g_term_sync;


// Helpers.

// This function implements a thread-safe replacement for std::localtime().
//
// Create a time_t via:
//   const auto t_now = std::chrono::system_clock::now();
//   const auto t_now_coarse = std::chrono::system_clock::to_time_t( t_now );
//
std::string
get_localtime_str( std::time_t time,
                   const std::string &format = "%Y%m%d-%H%M%S" );

} // namespace ygor.


// Helper macros.

#ifndef YLOG_G_TERM_WRAP
    #define YLOG_G_TERM_WRAP( x ) { std::unique_lock<std::mutex> lock(g_term_sync); x; }
#endif


#ifndef YLOG_PRETTY_FUNCTION
    #if defined( __PRETTY_FUNCTION__ )
        #define YLOG_PRETTY_FUNCTION __PRETTY_FUNCTION__
    #elif defined( __FUNCSIG__ )
        #define YLOG_PRETTY_FUNCTION __FUNCSIG__
    #else
        #define YLOG_PRETTY_FUNCTION __func__ // Note: not a macro, but should be available in all modern compilers.
    #endif
#endif

#ifndef YLOG_PRETTY_SRC_LOC
    #if defined( __LINE__ )
        #define YLOG_PRETTY_SRC_LOC __LINE__
    #else
        #define YLOG_PRETTY_SRC_LOC -1
    #endif
#endif

#ifndef YLOG_PRETTY_FILENAME
    #if defined( __FILE__ )
        #define YLOG_PRETTY_FILENAME __FILE__
    #else
        #define YLOG_PRETTY_FILENAME '(filename not available)'
    #endif
#endif


#ifndef YLOGDEBUG
    #define YLOGDEBUG( x )  { std::ostringstream os; \
                              os << x; \
                              ygor::g_logger({ os, \
                                   ygor::log_level::debug, \
                                   std::this_thread::get_id(), \
                                   YLOG_PRETTY_FUNCTION, \
                                   YLOG_PRETTY_SRC_LOC, \
                                   YLOG_PRETTY_FILENAME }); }
#endif


#ifndef YLOGINFO
    #define YLOGINFO( x )  { std::ostringstream os; \
                             os << x; \
                             ygor::g_logger({ os, \
                                  ygor::log_level::info, \
                                  std::this_thread::get_id(), \
                                  YLOG_PRETTY_FUNCTION, \
                                  YLOG_PRETTY_SRC_LOC, \
                                  YLOG_PRETTY_FILENAME }); }
#endif


#ifndef YLOGWARN
    #define YLOGWARN( x )  { std::ostringstream os; \
                             os << x; \
                             ygor::g_logger({ os, \
                                  ygor::log_level::warn, \
                                  std::this_thread::get_id(), \
                                  YLOG_PRETTY_FUNCTION, \
                                  YLOG_PRETTY_SRC_LOC, \
                                  YLOG_PRETTY_FILENAME }); }
#endif


#ifndef YLOGERR
    #define YLOGERR( x )   { std::ostringstream os; \
                             os << x; \
                             ygor::g_logger({ os, \
                                  ygor::log_level::err, \
                                  std::this_thread::get_id(), \
                                  YLOG_PRETTY_FUNCTION, \
                                  YLOG_PRETTY_SRC_LOC, \
                                  YLOG_PRETTY_FILENAME }); }

#endif

