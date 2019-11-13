//YgorNetworking.cc.
#include "YgorDefinitions.h"


//Gross fix for compiling without glibc: Define this constant to avoid getting compilation errors regarding
// sleep_for not being part of std::this_thread:...
//#ifndef _GLIBCXX_USE_NANOSLEEP
//    #pragma message "Defining macro _GLIBCXX_USE_NANOSLEEP. In Jan 2013, this was required on some systems to get around sleep_for/std::this_thread copmile errors." 
//    #define _GLIBCXX_USE_NANOSLEEP 1
//#endif



//#define YGORNETWORKING_NO_CHILD         //No children are spawned, so everything is shared. Useful for stateful things (media playback).
//#define YGORNETWORKING_USE_THREAD       //Spawns threads instead of forking. Less safe due to sharing *everything* by default.
#define YGORNETWORKING_USE_THREAD_POOL  //Spawns up to N threads instead of forking. Queues events for the N threads to handle.
//#define YGORNETWORKING_USE_CLONE        //Cherry-picked mix of the best of both above methods.          *Still haven't got it to work!*
//#define YGORNETWORKING_USE_FORK         //Results in no memory sharing. Good for stateless web serving/proxying.

#define YGORNETWORKING_RADIO_NO_CHILD
//#define YGORNETWORKING_RADIO_USE_THREAD
//#define YGORNETWORKING_RADIO_USE_CLONE
//#define YGORNETWORKING_RADIO_USE_FORK

/*
///////////////////////////////
#ifdef __linux__ 
    //Pull in the header files that define sys_clone and CLONE_IO.
    #include <sys/syscall.h>
    #define _GNU_SOURCE 
        #include <sched.h>
        #include <unistd.h>
    #undef _GNU_SOURCE 
 
    //If either is not present, fall back on the fork behaviour.
    #if ! defined(SYS_clone) || ! defined (CLONE_IO) 
        #define fork_clone_io fork 
    #else 
        //CLONE_IO is available, determine which version of sys_clone to use.
        #include <linux/version.h> 
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,49) 
            //Arguments: clone_flags, child_stack, parent_tidptr, child_tidptr.
            #define CLONE_ARGS SIGCHLD|CLONE_IO, 0, NULL, NULL 
        #else   
            #define CLONE_ARGS SIGCHLD|CLONE_IO, 0 
        #endif 
        pid_t fork_clone_io(void);

    #endif 
#else                                
        #define fork_clone_io fork 
#endif 
*/
//////////////////////////////

//Needed for YGORNETWORKING_USE_THREAD
#ifdef YGORNETWORKING_USE_THREAD
    #include <thread>
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
        #include <sched.h>

        #undef _GNU_SOURCE
    #else
        #include <sched.h>
    #endif
#endif    


#ifdef YGORNETWORKING_USE_THREAD_POOL
    #include <thread>
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
        #include <sched.h>

        #undef _GNU_SOURCE
    #else
        #include <sched.h>
    #endif
#endif


#include "YgorContainers.h"
#include <arpa/inet.h>
#include <fcntl.h>               //Needed for fcntl()
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>          //Needed for send()/recv().
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>              //Needed for, amongst other things, fcntl().
#include <cerrno>
#include <chrono>
#include <sched.h>               //Needed for unshare()
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#ifndef USE_OWN_GETIFADDR   //Else: see below for a homebrew...
    #include <ifaddrs.h>
#else
    #include "External/getifaddrs.h"
#endif



//#include <malloc.h>
#include <cstdlib>  //Clang 4.3 on FreeBSD: '/usr/include/malloc.h:3:2: error: "<malloc.h> has been replaced by <stdlib.h>"'

//For FreeBSD missing IPV6_ADD_MEMBERSHIP.
//See: http://code.dyne.org/hdsync/plain/src/pgm/sockaddr.c for origin of this blurb.
#if !defined( IPV6_ADD_MEMBERSHIP )
    #if defined( IPV6_JOIN_GROUP )
        #define  IPV6_ADD_MEMBERSHIP   IPV6_JOIN_GROUP
        #define  IPV6_DROP_MEMBERSHIP  IPV6_LEAVE_GROUP
    #else
        #error "Neither IPV6_ADD_MEMBERSHIP or IPV6_JOIN_GROUP defined."
    #endif
#endif

 
#include "YgorAlgorithms.h"        //Needed for shuffle_list_randomly(...)
#include "YgorFilesDirs.h"         //Needed for LoadFileToString(...), Size_of_File(...)
#include "YgorMisc.h"              //Needed for FUNCINFO, FUNCWARN, FUNCERR macro functions.
#include "YgorNetworking.h"
#include "YgorString.h"            //Needed for Xtostring(), some tokenization routines.


extern bool YGORNETWORKING_VERBOSE;


#ifndef YGORNETWORKING_CLONE_STACK_SIZE
    //Define a 128kB stack. Can be modified at compile time.
    #define YGORNETWORKING_CLONE_STACK_SIZE (128*1024)
#endif

#define PLAUSIBLE_USER_AGENT std::string("Mozilla/5.0 (X11; Linux i686) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1229.94 Safari/537.4")


//Some basic string functions to construct canned requests, responses, etc..

std::string Basic_HTTP_Request_Header(const std::string &hostandport){
    return Basic_HTTP_Request_Header(hostandport, "");
}
std::string Basic_HTTP_Request_Header(const std::string &hostandport, const std::string &request){
    //NOTE: hostandport should be in the form of "127.0.0.1:8080".
    //
    //The following is a sample request from Chromium browser with url "http://127.0.0.1:8080"
    //------------------------------------------------------------------------------------------------------------------
    //GET / HTTP/1.1
    //Host: 127.0.0.1:8080
    //Connection: keep-alive
    //User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.1 (KHTML, like Gecko) Chrome/21.0.1180.57 Safari/537.1
    //Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
    //Accept-Encoding: gzip,deflate,sdch
    //Accept-Language: en-US,en;q=0.8
    //Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
    //
    //------------------------------------------------------------------------------------------------------------------
    //Other resources are: http://upload.wikimedia.org/wikipedia/commons/c/c6/Http_request_telnet_ubuntu.png ,
    // http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol#Example_session ,
    // http://www.jmarshall.com/easy/http/ , etc..
    std::string out;
    out += "GET /";
    out += request;
//    out += " HTTP/1.1\n";
    out += " HTTP/1.0\n";
    out += "Host: ";
    out += hostandport + "\n";
    out += "User-Agent: "_s + PLAUSIBLE_USER_AGENT + "\n"_s;
//    out += "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.1 (KHTML, like Gecko) Chrome/21.0.1180.57 Safari/537.1\n";
    out += "Accept: text/html,application/xhtml+xml,application/xml\n";
    out += "Accept-Language: en-US,en\n";
    out += "Accept-Charset: ISO-8859-1,utf-8\n";
    out += "\n";  //This empty line is required! 
    return out;
}

std::string Basic_HTTP_Response_Text_Header(const std::string &thetext){
    std::string out;
    //out += "HTTP/1.1 200 OK\n";
    out += "HTTP/1.0 200 OK\n";
    out += "Server: Apache\n";
    out += "Content-Language: en\n";
    out += "Content-Length: ";
    out += Xtostring<size_t>(thetext.size()) + "\n";
    out += "Content-Type: text/html; charset=utf-8\n";
    out += "Connection: close\n";
    out += "\n";  //This empty line is required! 
    out += thetext + "\n";
    return out;
}

std::string Basic_HTTP_Response_File_Header(const std::string &filename){
    std::string out;

    const std::string mimetype = "unknown";
///////////////////
//    const std::string thetext = LoadFileToString(filename);
///////////////////
//    std::string thetext;
//    const auto filesize = Size_of_File(filename);
//    const auto mem_ptr  = Get_Piece_of_Binary_File<char>(filename, 0, filesize);
//    thetext.clear();
//    thetext.insert(thetext.size(), mem_ptr.get(), filesize);
////////////////////
    const std::string thetext = LoadBinaryFileToString(filename);

    //out += "HTTP/1.1 200 OK\r\n";
    out += "HTTP/1.0 200 OK\r\n";
    out += "Server: Apache\r\n";
    out += "Content-Language: en\r\n";
    out += "Content-Length: ";
    out += Xtostring<size_t>(thetext.size()) + "\r\n";

    out += "Content-Type: ";
    out += mimetype;
    out += "; charset=utf-8\r\n";
    out += "Connection: close\r\n";

    //Empty line delineating header from content.
    out += "\r\n";

    //Whatever the content is, stuffed into a string.
    out += thetext;
    out += "\r\n";  
    return out;
}

std::string Basic_HTTP_Response_File_Header_Only(const std::string &filename){
    std::string out;
    const std::string mimetype = "unknown";
    const auto thesize = Size_of_File(filename);

    out += "HTTP/1.1 200 OK\r\n";
//    out += "Server: Apache\r\n";
//    out += "Content-Language: en\r\n";

    out += "Content-Type: ";
    out += mimetype;
//    out += "; charset=utf-8\r\n";
    out += "\r\n";

    out += "Accept-Ranges: bytes\r\n";  //Advertise that we can also serve byte-by-byte.

    out += "Content-Length: ";
    out += Xtostring<long long int>(thesize) + "\r\n";

    out += "Connection: close\r\n";   //*Might* be killing mplayer streaming. Unsure. If everything seems OK, uncomment.  BE WEARY OF THIS LINE...
//    out += "Connection: keep-alive\r\n";

    //Empty line delineating header from content.
    out += "\r\n";
    //Don't forget to append the actual file at this point!
    return out;
}

std::string Range_HTTP_Response_File_Header_Only(const std::string &filename, off_t range_l, off_t range_u){
    std::string out;
    const std::string mimetype = "unknown";
    const auto thesize = Size_of_File(filename);

/*   Sample from YouTube.
   HTTP/1.1 206 Partial Content^M
   Last-Modified: Tue, 06 Sep 2011 18:47:36 GMT^M
   Content-Type: video/x-flv^M
   Date: Sun, 20 Jan 2013 22:28:09 GMT^M
   Expires: Sun, 20 Jan 2013 22:28:09 GMT^M
   Cache-Control: private, max-age=22409^M
   Content-Range: bytes 500-999/1018965^M
   Accept-Ranges: bytes^M
   Content-Length: 500^M
   Connection: close^M
   X-Content-Type-Options: nosniff^M
   Server: gvs 1.0^M
*/

    out += "HTTP/1.1 206 Partial Content\r\n";
    out += "Content-Type: "_s + mimetype;
//    out += "; charset=utf-8\r\n";
    out += "\r\n";

    out += "Content-Range: bytes "_s + Xtostring<long long int>(range_l) + "-"_s + Xtostring<long long int>(range_u);
    out += "/"_s + Xtostring<long long int>(thesize) + "\r\n"_s;

    out += "Accept-Ranges: bytes\r\n";

    out += "Content-Length: ";
    out += Xtostring<long long int>(range_u-range_l+1) + "\r\n";

    out += "Connection: close\r\n";    //I think it is somehow more important to close the connection after serving the specified range.  (?) :/ 
//    out += "Connection: keep-alive\r\n";

    out += "\r\n";   //Empty line delineating header from content.

    return out;  //Don't forget to append the actual file at this point!
}



//Some simple, no-nonsense, low-customizability routines for HTTP requests.
std::string Simple_HTTP_Request(const std::string &host, const std::string &request){
    //This function is extremely low-tech, and extremely unreliable. Do not use it for anything important.
    //
    //As an example, requesting 'http://icanhazip.com/' would be called like:
    //   ... = Simple_HTTP_Request("icanhazip.com/", "");

    std::string result;
    auto dialog = [&](int fd, char *host, long int port) -> bool {
        int nbytes;
        const int buffsz = 5000;
        char buff[buffsz];

        std::string In_msg;
        std::string Out_msg;

        //Construct a simple GET request. Dump it to screen.
        Out_msg = Basic_HTTP_Request_Header(std::string(host)+":80",request);

        //Send the request to the server.     
        if( send(fd,Out_msg.c_str(),Out_msg.size(),MSG_NOSIGNAL) == -1){ return false; }

        //Collect the (entire) response. Dump it to screen and sepuku.
        while((nbytes = recv(fd,buff,buffsz-1,0)) != -1){
            if(nbytes == 0) break;
            buff[nbytes]='\0';
            In_msg += std::string(buff,nbytes);
        }
        if(In_msg.size() == 0) return false;  //If something did not work and we got zilch!

        //The resulting response includes the HTTP header ("GET ... " etc..). We can split 
        // based on an empty line between the header and the content, which is required.
        // If the header pops up somewhere it shouldn't be then there is likely an extra
        // empty line prior to (or within) the header.
        auto tokenized = SplitStringToVector(In_msg, R"***(^$)***", 'd');
        In_msg.clear();
        //Skip the first token. This should be the header.
        for(auto it = ++(tokenized.begin()); it != tokenized.end(); ++it) In_msg += *it;
        result = In_msg;
        return true;
    };

    //Now we construct a client class.
    class Server_and_Client oneoff;

    //Initialize the client.
    if(!oneoff.Client_Init(80, dialog)){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to initialize client");
        return result;
    }

    //Attempt to connect to the server and interact with it.
    if(!oneoff.Client_Connect(host)){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to connect");
        return result;
    }
    return result;
}

//----------------------------------------------------------------------------------------------
//--------------------------------- IP Address utilities ---------------------------------------
//----------------------------------------------------------------------------------------------
//The 'local' routines check the local machine for the dhcp/static address.
//
//The output is in format:   <network name>, <address>
//
//An example is 'eth0', '192.168.0.123'.
std::list<std::pair<std::string, std::string>> Get_All_Local_IP4_Addresses(void){
    //Returns a list of <device names, addresses>.
    std::list<std::pair<std::string, std::string>> out;
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    if(getifaddrs(&ifaddr) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to get local IP4 addresses (err 1) - continuing");
        return out;
    }
    for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next){
        if(ifa->ifa_addr == nullptr) continue;
        int family = ifa->ifa_addr->sa_family;
        if(family == AF_INET){
            const auto size = sizeof(struct sockaddr_in);
            int s = getnameinfo(ifa->ifa_addr, size, host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if(s != 0){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to get local IP4 addresses (err 2) - continuing");
                freeifaddrs(ifaddr);
                return out;
            }
            out.emplace_back(ifa->ifa_name, host);
        }
    }
    freeifaddrs(ifaddr);
    return out;
}

std::list<std::pair<std::string, std::string>> Get_All_Local_IP6_Addresses(void){
    //Returns a list of <device names, addresses>.
    std::list<std::pair<std::string, std::string>> out;
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    if(getifaddrs(&ifaddr) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to get local IP6 addresses (err 1) - continuing");
        return out;
    }
    for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next){
        if(ifa->ifa_addr == nullptr) continue;
        int family = ifa->ifa_addr->sa_family;
        if(family == AF_INET6){
            const auto size = sizeof(struct sockaddr_in6);
            int s = getnameinfo(ifa->ifa_addr, size, host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if(s != 0){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to get local IP6 addresses (err 2) - continuing");
                freeifaddrs(ifaddr);
                return out;
            }
            out.emplace_back(ifa->ifa_name, host);
        }
    }
    freeifaddrs(ifaddr);
    return out;
}

std::list<std::pair<std::string, std::string>> Get_All_Local_IP_Addresses(void){
    //Returns a list of <device names, addresses>.
    auto out = Get_All_Local_IP4_Addresses();
    out.splice(out.end(), Get_All_Local_IP6_Addresses());
    return out;
}

//----------------------------------------------------------------------------------------------
//---------------------------- Server_and_Client: a TCP networking class -----------------------
//----------------------------------------------------------------------------------------------
//General helper member functions.
void *Server_and_Client::Get_in_addr(struct sockaddr *sa_in){
    if(sa_in->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa_in)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa_in)->sin6_addr);
}

//Constructors and Destructor.
Server_and_Client::Server_and_Client(void) : SERVER_INITIALIZED(false), SERVER_PORT(3490), SERVER_BACKLOG(100), 
                                             SERVER_yes(1), CLIENT_INITIALIZED(false), CLIENT_PORT(3490)  {

    //Set the default timeouts. The server's accept() timeout needs to be set *prior* to
    // initialization. All send() and recv() timeouts can be adjusted before OR after init.
    //
    //NOTE: These times can be modified to suit, even passed in as parameters to this member
    // if so desired.
    //
    //NOTE: If the total time is zero, it is interpretted as "wait forever."
    //
    //NOTE: They are oddly staggered so we can help diagnose issues by timing things. There
    // is no other significance to the stagger.
    this->SERVER_accept_timeout_sec   = 0;
    this->SERVER_accept_timeout_usec  = 0;

    this->SERVER_recv_timeout_sec     = 60;
    this->SERVER_recv_timeout_usec    = 0;
    this->SERVER_send_timeout_sec     = 63;
    this->SERVER_send_timeout_usec    = 0;

    this->CLIENT_recv_timeout_sec     = 67;
    this->CLIENT_recv_timeout_usec    = 0;
    this->CLIENT_send_timeout_sec     = 70;
    this->CLIENT_send_timeout_usec    = 0;
    


    //Provide some sane default dialog routines. These do not have to be fancy, they just
    // need to work and do something basic but non-trivial.
    this->DEFAULT_SERVER_DIALOG_LAMBDA = [&](int fd, char *host, long int port) -> bool {

        //Upon connection, we immediately send a message to the client.
        if(send(fd, "This is the default dialog response!", 39, MSG_NOSIGNAL) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server unable to send() to client");
            return false;
        }

        //We do not wait around for a response.
        return true;
    };

    this->DEFAULT_CLIENT_DIALOG_LAMBDA = [&](int fd, char *host, long int port) -> bool {
        int numbytes;
        const int BUFFSIZE = 100;
        char buff[BUFFSIZE];

        //Upon connection, we immediately receive a message from the server.
        // Ensure the buffer can easily be null-terminated without loss of any bytes.
        if((numbytes = recv(fd, buff, BUFFSIZE-1, 0)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Client connection issue. Nothing was received");
            return false;
        }
        buff[numbytes] = '\0';
        //if(YGORNETWORKING_VERBOSE) FUNCINFO("Received message from server: '" << buff << "'");

        //We do not send an acknowledgement or response.
        return true;
    };

 }

//----------------------------------------------------------------------------------------------
//Server-specific member functions.
bool Server_and_Client::Server_Init(void){
    return this->Server_Init( this->SERVER_PORT, this->USERS_SERVER_DIALOG_LAMBDA );
}

bool Server_and_Client::Server_Init(std::function<bool (int, char *, long int)> server_dialog){
    return this->Server_Init( this->SERVER_PORT, server_dialog );
}

bool Server_and_Client::Server_Init(long int port, std::function<bool (int, char *, long int)> server_dialog){
    this->SERVER_PORT = port;
    this->USERS_SERVER_DIALOG_LAMBDA = server_dialog;

    //This member function will initialize the server, preparing it to park and listen for
    // incoming connections. 
    //
    //Whether or not a previous initialization has occured, this function will 'start from
    // scratch' by wiping out the previous initialization. This is desirable in case the 
    // network somehow falls out from under our feet after initial initialization.
    //
    //This function returns 'true' if everything was OK and 'false' if any error arises.
    //
    this->SERVER_INITIALIZED = false;

    memset(&this->SERVER_hints, 0, sizeof(SERVER_hints));
    this->SERVER_hints.ai_family     = AF_UNSPEC;
    this->SERVER_hints.ai_socktype   = SOCK_STREAM;
    this->SERVER_hints.ai_flags      = AI_PASSIVE;     //Use my IP

    const std::string PORT_STRING = Xtostring<long int>(this->SERVER_PORT);
    if((this->SERVER_rv = getaddrinfo(nullptr, PORT_STRING.c_str(), &this->SERVER_hints, &this->SERVER_servinfo)) != 0) {
        if(YGORNETWORKING_VERBOSE) FUNCWARN("getaddrinfo returned error " << gai_strerror(this->SERVER_rv) << ". Unable to initialize server");
        freeaddrinfo(this->SERVER_servinfo);
        return false;
    }

    //Loop through all the results and bind to the first we can. Break on success.
    for(this->SERVER_p = this->SERVER_servinfo; this->SERVER_p != nullptr; this->SERVER_p = this->SERVER_p->ai_next){
        if((this->SERVER_sockfd = socket(this->SERVER_p->ai_family, this->SERVER_p->ai_socktype, this->SERVER_p->ai_protocol)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server socket issue. Continuing");
            continue;
        }

        //Attempt to tell the kernel to reuse addresses (so we can quickly reboot the program and reuse the address 
        // if we need to.)
        if(setsockopt(this->SERVER_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&this->SERVER_yes, sizeof(int)) == -1) {
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue (unable to force address reuse). Unable to initialize server");
            freeaddrinfo(this->SERVER_servinfo);
            return false;
        }

        //Attempt to set the waiting server's timeouts.
        //NOTE: These are NOT the send/recv timeouts, this is the accept() timeout!
        struct timeval timeshuttle;// = {0};
        memset(&timeshuttle, 0, sizeof(timeshuttle));
        timeshuttle.tv_sec  = this->SERVER_accept_timeout_sec;
        timeshuttle.tv_usec = this->SERVER_accept_timeout_usec;
        if(setsockopt(this->SERVER_sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeshuttle, sizeof(struct timeval)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue (unable to specify accept timeout). Unable to initialize server");
            freeaddrinfo(this->SERVER_servinfo);
            return false;
        }

        if(bind(this->SERVER_sockfd, this->SERVER_p->ai_addr, this->SERVER_p->ai_addrlen) == -1){
            close(this->SERVER_sockfd);
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server bind issue. Continuing"); //This will commonly occur if the port choice is illegal!
            continue;
        }

        break;
    }

    if(this->SERVER_p == nullptr){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Server bind issue. Unable to initialize server");
        return false;
    }

    //We can free this structure now.
    freeaddrinfo(this->SERVER_servinfo);

    //Set the close-on-exec flag so that server programs can execv and not consume/hold onto the port.
    if(fcntl(this->SERVER_sockfd, F_SETFD, FD_CLOEXEC) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Attempting to set close-on-exec flag failed. This shouldn't happen, so the file descriptor is likely invalid");
    }

    //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
    signal(SIGPIPE, SIG_IGN);

    if(listen(this->SERVER_sockfd, this->SERVER_BACKLOG) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Server listen issue. Unable to initialize server");
        return false;
    }

    //"Reap" all dead processes.
    auto handler_lambda = [](int s) -> void {  while(waitpid(-1, nullptr, WNOHANG) > 0){ }; };
    this->SERVER_sa.sa_handler = handler_lambda; //[](int s) -> void {  while(waitpid(-1, NULL, WNOHANG) > 0){

    sigemptyset(&this->SERVER_sa.sa_mask);
    this->SERVER_sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &this->SERVER_sa, nullptr) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Server sigaction issue. Unable to initialize server");
        return false;
    }

    this->SERVER_INITIALIZED = true;
    return true;
}


bool Server_and_Client::Server_Wait_for_Connection(void){
    //This member function waits for a (single) connection from a client. It forks/clones/threads/etc.. off
    // a process to handle the connection in a separate thread.
    if(this->SERVER_INITIALIZED != true){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Server initialization has not been performed. Unable to wait for connection");
        return false;
    }

    this->SERVER_sin_size = sizeof(this->SERVER_their_addr);
    this->SERVER_new_fd = accept(this->SERVER_sockfd, (struct sockaddr *)&this->SERVER_their_addr, &this->SERVER_sin_size);
    if(this->SERVER_new_fd == -1) {
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Server encountered an accept() issue. Unable to establish connection");
        return false;
    }

    //Set the close-on-exec flag so that servers can execv and not consume/hold onto the port.
    if(fcntl(this->SERVER_new_fd, F_SETFD, FD_CLOEXEC) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Attempting to set accept()'d close-on-exec flag failed. This shouldn't happen, so the file descriptor is likely invalid");
    }

    inet_ntop(this->SERVER_their_addr.ss_family, this->Get_in_addr((struct sockaddr *)&this->SERVER_their_addr), this->SERVER_s, sizeof(this->SERVER_s));
    //Ensure the string is null-terminated. An extra char exists to make this safely able to hold the maximum address size and an '\0'.
    this->SERVER_s[sizeof(this->SERVER_s)-1] = '\0';
    //if(VERBOSE) if(YGORNETWORKING_VERBOSE) FUNCINFO("Received connection from " << this->SERVER_s);

    //Look if the host is in the blocklist. If it is, do not proceed.
    this->Blocklist_Mutex.lock();
    if(this->Blocklist.find(this->SERVER_s) != this->Blocklist.end()){
        this->Blocklist_Mutex.unlock();
        close(this->SERVER_new_fd);
        return true;
    }
    this->Blocklist_Mutex.unlock();

    //Attempt to set the timeouts for the new fd.
    struct timeval timeshuttle;// = {0};
    memset(&timeshuttle, 0, sizeof(timeshuttle));
    timeshuttle.tv_sec  = this->SERVER_recv_timeout_sec;
    timeshuttle.tv_usec = this->SERVER_recv_timeout_usec;
    if(setsockopt(this->SERVER_new_fd, SOL_SOCKET, SO_RCVTIMEO, &timeshuttle, sizeof(struct timeval)) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue. Unable to specify recv timeout");
        return false;
    }
    timeshuttle.tv_sec  = this->SERVER_send_timeout_sec;
    timeshuttle.tv_usec = this->SERVER_send_timeout_usec;
    if(setsockopt(this->SERVER_new_fd, SOL_SOCKET, SO_SNDTIMEO, &timeshuttle, sizeof(struct timeval)) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue. Unable to specify send timeout");
        return false;
    }

/*
//This code is used to try to determine the server's own address (the one being used to talk to the client.)
// I'm pretty sure it doesn't work, and I don't know if I need it right now anyways...
//----------------------
{
//See http://stackoverflow.com/questions/4937529/polling-interface-names-via-siocgifconf-in-linux
//
//and maybe also:
//
//http://linux.die.net/man/7/netdevice
//http://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine
//http://stackoverflow.com/questions/4046616/sockets-how-to-find-out-what-port-and-address-im-assigned
//  in this order.

//TODO: Convert this shit into a separate routine which takes file descriptors (from streams) and returns 
// the list of possible addresses.

//BUT FIRST CHECK: Does this even do what I think it does? How could I test it?? Will it only
// return addresses which could be used to connect to the server?

    struct ifconf conf;
    char data[4096];
    
    struct ifreq *ifr;
    char addrbuf[1024];
    
    conf.ifc_len = sizeof(data);
    conf.ifc_buf = (caddr_t)data;
    if(ioctl(this->SERVER_new_fd,SIOCGIFCONF,&conf) != -1){
//    if(ioctl(this->SERVER_sockfd,SIOCGIFCONF,&conf) != -1){
        int i = 0;
        ifr = (struct ifreq *)data;
        while((char*)ifr < (data + conf.ifc_len)){
            switch(ifr->ifr_addr.sa_family){
                case AF_INET:
                    ++i;
                    printf("%d. %s : %s\n", i, ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr, addrbuf, sizeof(addrbuf)));
                    break;
                case AF_INET6:
                    ++i;
                    printf("%d. %s : %s\n", i, ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in6*)&ifr->ifr_addr)->sin6_addr, addrbuf, sizeof(addrbuf)));
                    break;
            }
            ifr = (struct ifreq*)((char*)ifr + sizeof(*ifr));
        }
    
    }

    //return list of pairs of device names, addresses...
}
//-----------------------
*/



#if defined YGORNETWORKING_USE_FORK
    //Just fork and execute the lambda!
    if(!fork()){
        //----------------------------- Child Process Fence ---------------------------------
        //Stop listening for connections. Let the parent do so!
        close(this->SERVER_sockfd);

        //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
        signal(SIGPIPE, SIG_IGN);

        //Call the user's dialog lambda if it exists. Call the default otherwise.
        if( this->USERS_SERVER_DIALOG_LAMBDA ){
            if(!this->USERS_SERVER_DIALOG_LAMBDA( this->SERVER_new_fd, this->SERVER_s, this->SERVER_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server dialog provided by user failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else if( this->DEFAULT_SERVER_DIALOG_LAMBDA ){
            if(!this->DEFAULT_SERVER_DIALOG_LAMBDA( this->SERVER_new_fd, this->SERVER_s, this->SERVER_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server default dialog failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server could not dialog because no valid dialog routine is known");
        }

        //Now we close up the connection to the client and terminate execution of the child.
        close(this->SERVER_new_fd);

        //Finally, terminate execution of this process.
        exit(0);
        //-----------------------------------------------------------------------------------
    }

    //Only the parent will reach this point! Since the child will deal with the connection, we
    // close the connection fd.
    close(this->SERVER_new_fd);

#elif defined YGORNETWORKING_USE_CLONE

    #pragma message "############################################"
    #pragma message "Clone usage is not yet complete. DO NOT USE!"
    #pragma message "############################################"
    #error

    //Notes:
    //
    // -I found a nice, simple example of clone(2) online at http://www.linuxjournal.com/article/5211.
    //  and http://www.linuxjournal.com/files/linuxjournal.com/linuxjournal/articles/052/5211/5211l2.html.
    //  Here it is:
    //----------------------------------------
    //#include         <--- (Note that these are blank in the example.)
    //#include 
    //
    //int variable, fd;
    //int do_something() {
    //   variable = 42;
    //   close(fd);
    //   _exit(0);
    //}
    //
    //int main(int argc, char *argv[]) {
    //   void **child_stack;
    //   char tempch;
    //
    //   variable = 9;
    //   fd = open("test.file", O_RDONLY);
    //   child_stack = (void **) malloc(16384);
    //   printf("The variable was %d\n", variable);
    //   
    //   clone(do_something, child_stack, CLONE_VM|CLONE_FILES, NULL);
    //   sleep(1);
    //
    //   printf("The variable is now %d\n", variable);
    //   if (read(fd, &tempch, 1) < 1) {
    //      perror("File Read Error");
    //      exit(1);
    //   }
    //   printf("We could read from the file\n");
    //   return 0;
    //}
    //---------------------------------------
    // -Upon getting the threads option to work, I can no longer remember *why* I wanted to use clone(2)
    //  instead. Something about not sharing the file descriptors or something, but this is (in practice)
    //  not an issue. Nonetheless, maybe I should finish it so I can both refer to it and maybe gain some
    //  traction providing a middle-ground between fork() and thread().

if(YGORNETWORKING_VERBOSE) FUNCINFO("Mapping the stack now");

    void *Stack = mmap(nullptr, YGORNETWORKING_CLONE_STACK_SIZE, PROT_READ | PROT_WRITE, \
                                MAP_PRIVATE | MAP_ANON | MAP_GROWSDOWN, -1, 0);
if(YGORNETWORKING_VERBOSE) FUNCINFO("   ...mapping done");

    auto Childs_Lambda = [](void *arg) -> int {

if(YGORNETWORKING_VERBOSE) FUNCINFO("Executing the child's lambda now");

        //----------------------------- Child Process Fence ---------------------------------
        //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
        signal(SIGPIPE, SIG_IGN);

        //Cast the input (void *) to a struct so we can pull out all the parts we need.
        if(arg == nullptr){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Child lambda was passed a nullptr. Unable to cast to struct. Aborting");
            exit(-1);
        }
        struct YgorNetworking_Server_and_Client_Clone_Shuttle *in = reinterpret_cast<struct YgorNetworking_Server_and_Client_Clone_Shuttle *>( arg );

        //Stop listening for connections. Let the parent do so!
        close(in->SERVER_sockfd);

        //Call the user's dialog lambda if it exists. Call the default otherwise.
        if( in->USERS_SERVER_DIALOG_LAMBDA ){
            if(!in->USERS_SERVER_DIALOG_LAMBDA( in->SERVER_new_fd, in->SERVER_s, in->SERVER_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server dialog provided by user failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else if( in->DEFAULT_SERVER_DIALOG_LAMBDA ){
            if(!in->DEFAULT_SERVER_DIALOG_LAMBDA( in->SERVER_new_fd, in->SERVER_s, in->SERVER_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server default dialog failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server could not dialog because no valid dialog routine is known");
        }

        //Now we close up the connection to the client and terminate execution of the child.
        close(in->SERVER_new_fd);

        //Cleanup. Unmap the Stack, delete the struct pointed to by in.
        munmap(in->Stack, YGORNETWORKING_CLONE_STACK_SIZE);
        delete in;  //delete arg; ??

if(YGORNETWORKING_VERBOSE) FUNCINFO("   ...lambda done");


        //Finally, terminate execution of this process.
//        return 0;
        exit(0);
        //-----------------------------------------------------------------------------------
    };

if(YGORNETWORKING_VERBOSE) FUNCINFO("Shuttling the object's data now");

    struct YgorNetworking_Server_and_Client_Clone_Shuttle *out = new YgorNetworking_Server_and_Client_Clone_Shuttle;
    out->SERVER_sockfd                 = this->SERVER_sockfd;
    out->USERS_SERVER_DIALOG_LAMBDA    = this->USERS_SERVER_DIALOG_LAMBDA;
    out->DEFAULT_SERVER_DIALOG_LAMBDA  = this->DEFAULT_SERVER_DIALOG_LAMBDA;
    out->SERVER_new_fd                 = this->SERVER_new_fd;
    out->SERVER_s[INET6_ADDRSTRLEN]    = this->SERVER_s[INET6_ADDRSTRLEN];
    out->SERVER_PORT                   = this->SERVER_PORT;
    out->Stack                         = Stack;
if(YGORNETWORKING_VERBOSE) FUNCINFO("   ...shuttling done");

    if(Stack != nullptr){

if(YGORNETWORKING_VERBOSE) FUNCINFO("Attempting to clone now");
        pid_t pid = clone(Childs_Lambda, Stack, CLONE_VM | SIGCHLD , (void *)(out));
if(YGORNETWORKING_VERBOSE) FUNCINFO("   ...cloning done");

    }else{
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to clone process. Cleaning up and ignoring connection");
        munmap(Stack, YGORNETWORKING_CLONE_STACK_SIZE);
    }

    //Only the parent will reach this point! Since the child will deal with the connection, we
    // close the connection fd.
    close(this->SERVER_new_fd);


/*
    void *stack = malloc( YGORNETWORKING_CLONE_STACK_SIZE );
    if(stack == nullptr){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to allocate stack space for the child process to handle connection");
        return false;
    }

    pid_t pid = clone( &threadFunction, (char*) stack + FIBER_STACK,
                 SIGCHLD | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM, 0 );
         if ( pid == -1 ){
                 perror( "clone" );
                 exit( 2 );
         }
*/        

#elif defined YGORNETWORKING_USE_THREAD
    
    //Because we are sharing memory, we copy some things to avoid having them disappear mid-execution.
    // These are copies which are passed in by value to the child's lambda. Seems to work fairly well.
    const std::string hostcopy(this->SERVER_s);
    const int fdcopy(this->SERVER_new_fd);
    const long int portcopy(this->SERVER_PORT);

    auto Childs_Lambda = [&](std::string thehost, int thefd, long int theport) -> void {
        //----------------------------- Child Process Fence ---------------------------------
        //Ensure the SIGPIPE signal is ignored. Is this necessary? Should the parent call it?   FIXME.
        //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
        signal(SIGPIPE, SIG_IGN);

        //The following functions expect a raw (char *) array. This may need to change if re-writing 
        // is necessary.
        char *thehostcopy = const_cast<char *>(thehost.c_str());

        //Call the user's dialog lambda if it exists. Call the default otherwise.
        //NOTE: I'm unsure of how to properly deal with the lambda functions. Should I copy
        // them too? Should I slap a mutex around them? Wouldn't copying everything be too
        // far of a departure from the 'use threads' mentality?    FIXME.
        if( this->USERS_SERVER_DIALOG_LAMBDA ){
            if(!this->USERS_SERVER_DIALOG_LAMBDA( thefd, thehostcopy, theport )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server dialog provided by user failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else if( this->DEFAULT_SERVER_DIALOG_LAMBDA ){
            if(!this->DEFAULT_SERVER_DIALOG_LAMBDA( thefd, thehostcopy, theport )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server default dialog failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server could not dialog because no valid dialog routine is known");
        }

        //Now we close up the connection to the client and terminate execution of the child.
        close(thefd);

        //Finally, terminate execution of this thread.
        return;
        //-----------------------------------------------------------------------------------
    };

    //Launch the thread.
    std::thread tolaunch;
    tolaunch = std::thread( Childs_Lambda, hostcopy, fdcopy, portcopy );
    tolaunch.detach();

    //Note: The child takes ownership of the new fd. We can safely leave it in the child's hands.


#elif defined YGORNETWORKING_USE_THREAD_POOL

    //Because we are sharing memory, we copy some things to avoid having them disappear mid-execution.
    // These are copies which are passed in by value to the child's lambda. Seems to work fairly well.
    const std::string hostcopy(this->SERVER_s);
    const int fdcopy(this->SERVER_new_fd);
    const long int portcopy(this->SERVER_PORT);

    auto Childs_Lambda = [&,hostcopy,fdcopy,portcopy](void) -> void {
//std::string thehost, int thefd, long int theport
        //----------------------------- Child Process Fence ---------------------------------
        //Ensure the SIGPIPE signal is ignored. Is this necessary? Should the parent call it?   FIXME.
        //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
        signal(SIGPIPE, SIG_IGN);

        //The following functions expect a raw (char *) array. This may need to change if re-writing 
        // is necessary.
        char *thehostcopy = const_cast<char *>(hostcopy.c_str());

        //Call the user's dialog lambda if it exists. Call the default otherwise.
        //NOTE: I'm unsure of how to properly deal with the lambda functions. Should I copy
        // them too? Should I slap a mutex around them? Wouldn't copying everything be too
        // far of a departure from the 'use threads' mentality?    FIXME.
        if( this->USERS_SERVER_DIALOG_LAMBDA ){
            if(!this->USERS_SERVER_DIALOG_LAMBDA( fdcopy, thehostcopy, portcopy )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server dialog provided by user failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else if( this->DEFAULT_SERVER_DIALOG_LAMBDA ){
            if(!this->DEFAULT_SERVER_DIALOG_LAMBDA( fdcopy, thehostcopy, portcopy )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Server default dialog failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server could not dialog because no valid dialog routine is known");
        }

        //Now we close up the connection to the client and terminate execution of the child.
        close(fdcopy);

        //Finally, terminate execution of this thread.
        return;
        //-----------------------------------------------------------------------------------
    };

    //Pass the task to the thread pool.
    this->threadpool.Queue(Childs_Lambda);

    //Note: The child takes ownership of the new fd. We can safely leave it in the child's hands.

#elif defined YGORNETWORKING_NO_CHILD

    //Call the user's dialog lambda if it exists. Call the default otherwise.
    if( this->USERS_SERVER_DIALOG_LAMBDA ){
        if(!this->USERS_SERVER_DIALOG_LAMBDA( this->SERVER_new_fd, this->SERVER_s, this->SERVER_PORT )){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server dialog provided by user failed");
            //This child SHOULD NOT return here. Let it continue to the exit(0);
        }
    }else if( this->DEFAULT_SERVER_DIALOG_LAMBDA ){
        if(!this->DEFAULT_SERVER_DIALOG_LAMBDA( this->SERVER_new_fd, this->SERVER_s, this->SERVER_PORT )){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server default dialog failed");
            //This child SHOULD NOT return here. Let it continue to the exit(0);
        }
    }else{
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Server could not dialog because no valid dialog routine is known");
    }

    //Now we close up the connection to the client.
    close(this->SERVER_new_fd);

#else
    #pragma message " #########################################################################################"
    #pragma message " You must define one of:                                                                  "
    #pragma message "     YGORNETWORKING_USE_FORK     - (No memory sharing, fast, safest.)                       "
    #pragma message "     YGORNETWORKING_USE_CLONE    - (Memory sharing only, fairly safe.)                      "
    #pragma message "     YGORNETWORKING_USE_THREAD   - (Memory sharing, fairly safe.)                           "
    #pragma message "     YGORNETWORKING_NO_CHILD     - (Total memory sharing, (sequentially) fast, least safe.) "
    #pragma message " in order to specify how connections are handled by the server!                           "
    #pragma message " Alternatively, use clone(2) to write your own connection handling scheme.                "
    #pragma message " #########################################################################################"
    #error
#endif

    return true;
}

//----------------------------------------------------------------------------------------------
//Client-specific member functions.
bool Server_and_Client::Client_Init(void){
    return this->Client_Init( this->CLIENT_PORT, this->USERS_CLIENT_DIALOG_LAMBDA );
}

bool Server_and_Client::Client_Init(std::function<bool (int, char *, long int)> client_dialog){
    return this->Client_Init( this->CLIENT_PORT, client_dialog );
}

bool Server_and_Client::Client_Init(long int port, std::function<bool (int, char *, long int)> client_dialog){
    this->CLIENT_PORT    = port;
    this->USERS_CLIENT_DIALOG_LAMBDA = client_dialog;

    //This member function will initialize the client, preparing it to establish a connection.
    //
    //Whether or not a previous initialization has occured, this function will 'start from
    // scratch' by wiping out the previous initialization. This is desirable in case the 
    // network somehow falls out from under our feet after initial initialization.
    //
    //This function returns 'true' if everything was OK and 'false' if any error arises.
    //
    this->CLIENT_INITIALIZED = false;

    //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
    signal(SIGPIPE, SIG_IGN);

    memset(&this->CLIENT_hints, 0, sizeof(this->CLIENT_hints));
    this->CLIENT_hints.ai_family   = AF_UNSPEC;
    this->CLIENT_hints.ai_socktype = SOCK_STREAM;
//    this->CLIENT_hints.ai_socktype = SOCK_STREAM | SOCK_CLOEXEC; //Included so that we can execv a client without holding the port.

    this->CLIENT_INITIALIZED = true;
    return true;
}

bool Server_and_Client::Client_Connect(const std::string &address){
    this->CLIENT_ADDRESS = address;

    if(this->CLIENT_INITIALIZED != true){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Client initialization has not been performed. Unable to establish connection");
        return false;
    }

    const std::string PORT_STRING = Xtostring<long int>(this->CLIENT_PORT);
    if((this->CLIENT_rv = getaddrinfo(this->CLIENT_ADDRESS.c_str(), PORT_STRING.c_str(), &this->CLIENT_hints, &this->CLIENT_servinfo)) != 0) {
        if(YGORNETWORKING_VERBOSE) FUNCWARN("getaddrinfo returned error " << gai_strerror(this->CLIENT_rv) << ". Unable to initialize client");
        return false;
    }

    //Loop through all the results and connect to the first we can.
    for(this->CLIENT_p = this->CLIENT_servinfo; this->CLIENT_p != nullptr; this->CLIENT_p = this->CLIENT_p->ai_next){
        if((this->CLIENT_sockfd = socket(this->CLIENT_p->ai_family, this->CLIENT_p->ai_socktype, this->CLIENT_p->ai_protocol)) == -1) {
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Server socket issue. Continuing");
            continue;
        }

        //Attempt to set the timeouts.
        struct timeval timeshuttle;// = {0};
        memset(&timeshuttle, 0, sizeof(timeshuttle));
        timeshuttle.tv_sec  = this->CLIENT_recv_timeout_sec;
        timeshuttle.tv_usec = this->CLIENT_recv_timeout_usec;
        if (setsockopt(this->CLIENT_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeshuttle, sizeof(struct timeval)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue. Unable to specify recv timeout");
            return false;
        }
        timeshuttle.tv_sec  = this->CLIENT_send_timeout_sec;
        timeshuttle.tv_usec = this->CLIENT_send_timeout_usec;
        if (setsockopt(this->CLIENT_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeshuttle, sizeof(struct timeval)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue. Unable to specify recv timeout");
            return false;
        }

//------------
        //Set the close-on-exec flag so that clients can execv and not consume/hold onto the port.
        if(fcntl(this->CLIENT_sockfd, F_SETFD, FD_CLOEXEC) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Attempting to set client close-on-exec flag failed. This shouldn't happen, so the file descriptor is likely invalid");
            return false;
        }
//fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC); ???
//-------------

        if(connect(this->CLIENT_sockfd, this->CLIENT_p->ai_addr, this->CLIENT_p->ai_addrlen) == -1){
            close(this->CLIENT_sockfd);
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Client connection issue. Continuing");
            continue;
        }
        break;
    }

    if(this->CLIENT_p == nullptr){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Client is unable to connect");
        return false;
    }  

    inet_ntop(this->CLIENT_p->ai_family, this->Get_in_addr((struct sockaddr *)this->CLIENT_p->ai_addr), this->CLIENT_s, sizeof(this->CLIENT_s));
    //if(YGORNETWORKING_VERBOSE) FUNCINFO("Connected to " << this->CLIENT_s);

    //Cleanup stuff we no longer need before talking with the server.
    freeaddrinfo(this->CLIENT_servinfo); 

    //Call the user's dialog lambda if it exists. Call the default otherwise.
    if( this->USERS_CLIENT_DIALOG_LAMBDA ){
        if(!this->USERS_CLIENT_DIALOG_LAMBDA( this->CLIENT_sockfd, this->CLIENT_s, this->CLIENT_PORT )){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Client dialog provided by user failed");
            //This child SHOULD NOT return here. Let it continue to the exit(0);
        }
    }else if( this->DEFAULT_CLIENT_DIALOG_LAMBDA ){
        if(!this->DEFAULT_CLIENT_DIALOG_LAMBDA( this->CLIENT_sockfd, this->CLIENT_s, this->CLIENT_PORT )){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Client default dialog failed");
            //This child SHOULD NOT return here. Let it continue to the exit(0);
        }
    }else{
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Client could not dialog because no valid dialog routine is known");
    }

    //End dialog with server by closing the connection.
    close(this->CLIENT_sockfd);
    return true;
}

//----------------------------------------------------------------------------------------------
//General member functions.
bool Server_and_Client::Set_No_Timeouts(void){
    //This member will remove all server and client timeouts by setting them all to 'infinite' timeout.
    //
    //NOTE: This function will return 'false' if it is too late to set all of the timeouts, 'true' else.
    if(this->SERVER_INITIALIZED || this->CLIENT_INITIALIZED){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Too late to set all timeouts - must be done prior to initialization");
        return false;
    }

    this->SERVER_accept_timeout_sec   = 0;
    this->SERVER_accept_timeout_usec  = 0;

    this->SERVER_recv_timeout_sec     = 0;
    this->SERVER_recv_timeout_usec    = 0;
    this->SERVER_send_timeout_sec     = 0;
    this->SERVER_send_timeout_usec    = 0;

    this->CLIENT_recv_timeout_sec     = 0;
    this->CLIENT_recv_timeout_usec    = 0;
    this->CLIENT_send_timeout_sec     = 0;
    this->CLIENT_send_timeout_usec    = 0;
    return true;
}


void Server_and_Client::Add_To_Blocklist(const std::string &host){
    this->Blocklist_Mutex.lock();
    this->Blocklist.insert(host);
    this->Blocklist_Mutex.unlock();
    return;
}

void Server_and_Client::Remove_From_Blocklist(const std::string &host){
    this->Blocklist_Mutex.lock();
    auto it = this->Blocklist.find(host);
    if(it != this->Blocklist.end()) this->Blocklist.erase(it);
    this->Blocklist_Mutex.unlock();
    return;
}

bool Server_and_Client::Is_In_Blocklist(const std::string &host){
    this->Blocklist_Mutex.lock();
    const bool out = (this->Blocklist.find(host) != this->Blocklist.end());
    this->Blocklist_Mutex.unlock();
    return out;
}



//----------------------------------------------------------------------------------------------
//--------------------------- Beacon_and_Radio: a UDP multicasting class -----------------------
//----------------------------------------------------------------------------------------------
//General helper member functions.
void *Beacon_and_Radio::Get_in_addr(struct sockaddr *sa_in){
    if(sa_in->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa_in)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa_in)->sin6_addr);
}

//Constructors and Destructor.
Beacon_and_Radio::Beacon_and_Radio(void) : RADIO_INITIALIZED(false), RADIO_PORT(3490), /*RADIO_BACKLOG(100), */
                                           RADIO_LOOP(true), RADIO_yes(1), BEACON_INITIALIZED(false), 
                                           BEACON_PORT(3490), BEACON_LOOP(true), BEACON_TTL_HOPS(1) {

    //Set the default timeouts. All send() and recv() timeouts can be adjusted before OR 
    // after init.
    //
    //NOTE: These times can be modified to suit, even passed in as parameters to this member
    // if so desired.
    //NOTE: If the total time is zero, it is interpretted as "wait forever."
    //NOTE: They are oddly staggered so we can help diagnose issues by timing things. There
    // is no other significance to the stagger.
    this->BEACON_send_timeout_sec    = 10;
    this->BEACON_send_timeout_usec   = 0;

    this->RADIO_recv_timeout_sec     = 0; //Wait forever. This is probably most desirable.
    this->RADIO_recv_timeout_usec    = 0;

    //Provide some sane default dialog routines. These do not have to be fancy, they just
    // need to work and do something basic but non-trivial.
    this->DEFAULT_BEACON_DIALOG_LAMBDA = [](int fd, char *host, long int port) -> bool {
        //Send a simple message to all radios. This entire lambda will be looped over forever.
        if(send(fd, "This is the default beacon message!", 38, MSG_NOSIGNAL) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon unable to send()");
            return false;
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCINFO("Sent a message over multicast");
        }

        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        return true;
    };

    this->DEFAULT_RADIO_DIALOG_LAMBDA = [](int fd, char *host, long int port) -> bool {
        int numbytes;
        const int BUFFSIZE = 100;
        char buff[BUFFSIZE];

        //Wait until we timeout (or otherwise error) or we get a message from a broadcasting beacon.
        if((numbytes = recv(fd, buff, BUFFSIZE-1, 0)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon connection issue. Nothing was received");
            return false;
        }
        buff[numbytes] = '\0';

        if(YGORNETWORKING_VERBOSE) FUNCINFO("Received multicast message from beacon: '" << buff << "'");
        return true;
    };

}

//----------------------------------------------------------------------------------------------
//Radio-specific member functions.
bool Beacon_and_Radio::Radio_Init(void){
    return this->Radio_Init( this->RADIO_PORT, this->USERS_RADIO_DIALOG_LAMBDA );
}

bool Beacon_and_Radio::Radio_Init(std::function<bool (int, char *, long int)> radio_dialog){
    return this->Radio_Init( this->RADIO_PORT, radio_dialog );
}

bool Beacon_and_Radio::Radio_Init(long int port, std::function<bool (int, char *, long int)> radio_dialog){
    this->RADIO_PORT = port;
    this->USERS_RADIO_DIALOG_LAMBDA = radio_dialog;

    //This member function will initialize the radio, preparing it to begin transmitting 
    // messages.
    //
    //Whether or not a previous initialization has occured, this function will 'start from
    // scratch' by wiping out the previous initialization. This is desirable in case the 
    // network somehow falls out from under our feet after initial initialization.
    //
    //This function returns 'true' if everything was OK and 'false' if any error arises.
    //
    this->RADIO_INITIALIZED = false;

    memset(&this->RADIO_hints, 0, sizeof(RADIO_hints));
    this->RADIO_hints.ai_family     = AF_UNSPEC;
    this->RADIO_hints.ai_socktype   = SOCK_DGRAM;
    this->RADIO_hints.ai_flags      = AI_PASSIVE;

    const std::string PORT_STRING = Xtostring<long int>(this->RADIO_PORT);
    if((this->RADIO_rv = getaddrinfo(nullptr, PORT_STRING.c_str(), &this->RADIO_hints, &this->RADIO_servinfo)) != 0) {
        if(YGORNETWORKING_VERBOSE) FUNCWARN("getaddrinfo returned error " << gai_strerror(this->RADIO_rv) << ". Unable to initialize radio");
        freeaddrinfo(this->RADIO_servinfo);
        return false;
    }

    //Loop through all the results and bind to the first we can. Break on success.
    for(this->RADIO_p = this->RADIO_servinfo; this->RADIO_p != nullptr; this->RADIO_p = this->RADIO_p->ai_next){
        if((this->RADIO_sockfd = socket(this->RADIO_p->ai_family, this->RADIO_p->ai_socktype, this->RADIO_p->ai_protocol)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio socket issue. Continuing");
            continue;
        }

        //Attempt to tell the kernel to reuse addresses (so we can quickly reboot the program and reuse the address 
        // if we need to.)
        if(setsockopt(this->RADIO_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&this->RADIO_yes, sizeof(int)) == -1) {
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue (unable to force address reuse). Unable to initialize radio");
            freeaddrinfo(this->RADIO_servinfo);
            return false;
        }

        //Try to bind the socket to the suggested address/port. 
        if(bind(this->RADIO_sockfd, this->RADIO_p->ai_addr, this->RADIO_p->ai_addrlen) == -1){
            close(this->RADIO_sockfd);
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio bind issue. Continuing");            
            continue;
        }
        break;
    }

    if(this->RADIO_p == nullptr){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio bind issue. Unable to initialize radio");
        return false;
    }

    //We can free this structure now.
    freeaddrinfo(this->RADIO_servinfo);

    //Set the close-on-exec flag so that radio programs can execv and not consume/hold onto the port.
    if(fcntl(this->RADIO_sockfd, F_SETFD, FD_CLOEXEC) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Attempting to set close-on-exec flag failed. This shouldn't happen, so the file descriptor is likely invalid");
    }

    //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
    signal(SIGPIPE, SIG_IGN);

    //"Reap" all dead processes.
    auto handler_lambda = [](int s) -> void { while(waitpid(-1, nullptr, WNOHANG) > 0){ }; };
    this->RADIO_sa.sa_handler = handler_lambda;

    sigemptyset(&this->RADIO_sa.sa_mask);
    this->RADIO_sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &this->RADIO_sa, nullptr) == -1){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio sigaction issue. Unable to initialize radio");
        return false;
    }

    this->RADIO_INITIALIZED = true;
    return true;
}

bool Beacon_and_Radio::Radio_Tune_In(const std::string &address){
    //This member function begins listening for broadcasts on the given multicast address.
    //
    //When something is received, it is packed into a fork()/exec()/thread and we continue.
    if(this->RADIO_INITIALIZED != true){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio initialization has not been performed. Unable to wait for broadcasts");
        return false;
    }

    //Try to resolve the multicast IP address into something more machine-readable.
    if((this->RADIO_rv = getaddrinfo(address.c_str(), nullptr, &this->RADIO_hints, &this->RADIO_servinfo)) != 0){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("getaddrinfo returned error " << gai_strerror(this->RADIO_rv) << ". Unable to resolve multicast address");
        return false;
    }

    //Send a request to the kernel to join the multicast group. We handle IPv4/v6 separately.
    //For IPv4.
    if(this->RADIO_servinfo->ai_family == AF_INET && this->RADIO_servinfo->ai_addrlen == sizeof(struct sockaddr_in)){
        struct ip_mreq multicastRequest;
        memset(&multicastRequest, 0, sizeof(multicastRequest));

        //The multicast group we want to join.
        memcpy(&multicastRequest.imr_multiaddr, &((struct sockaddr_in*)(this->RADIO_servinfo->ai_addr))->sin_addr,
               sizeof(multicastRequest.imr_multiaddr));

        //Allow any interface present to accept multicasts. Maybe we should specify a specific card here?
        multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

        //Join it.
        if(setsockopt(this->RADIO_sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) != 0){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio initialization has not been performed. Unable to join multicast group");
            return false;
        }

    //For IPv6.
    }else if(this->RADIO_servinfo->ai_family == AF_INET6 && this->RADIO_servinfo->ai_addrlen == sizeof(struct sockaddr_in6)){
        struct ipv6_mreq multicastRequest;
        memset(&multicastRequest, 0, sizeof(multicastRequest));

        //The multicast group we want to join.
        memcpy(&multicastRequest.ipv6mr_multiaddr, &((struct sockaddr_in6*)(this->RADIO_servinfo->ai_addr))->sin6_addr,
               sizeof(multicastRequest.ipv6mr_multiaddr));

        //Allow any interface present to accept multicasts. Maybe we should specify a specific card here?
        multicastRequest.ipv6mr_interface = 0;

        //Join it.
        if(setsockopt(this->RADIO_sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) != 0){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio initialization has not been performed. Unable to join multicast group");
            return false;
        }
    }else{
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio initialization has not been performed. Multicast info is not understandable");
        return false;
    }

    freeaddrinfo(this->RADIO_servinfo);


    //Now, loop over messages. If looping is turned off, we will perform one such recv() and then terminate.
    do{
        //Now fish until we get something. To ensure nothing is lost, we only peek at the message.
        const long int smallbuff_size = 10;
        char smallbuff[smallbuff_size+1];
        struct sockaddr beacon_addr;
        socklen_t beacon_addr_sz = sizeof(beacon_addr);
        memset(&beacon_addr, 0, sizeof(beacon_addr));
        auto peekret = recvfrom(this->RADIO_sockfd, smallbuff, smallbuff_size, MSG_PEEK, &beacon_addr, &beacon_addr_sz);
  
        if(peekret < 0){
            const int errsv = errno;
            if(errsv == ETIMEDOUT){
                //if(VERBOSE) if(YGORNETWORKING_VERBOSE) FUNCINFO("Timed out while waiting to receive multicast message. Consider increasing timeout threshold");
                return false;
            }else{ 
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Failed to receive multicast message. Encountered error: " << strerror(errsv));
                return false;
            }
    
        }else if(peekret == 0){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("recv() returned a message of length 0. Not sure what this means!");
            return false;
        }
        smallbuff[peekret] = '\0';

        if(inet_ntop(beacon_addr.sa_family, this->Get_in_addr(&beacon_addr), this->RADIO_s, sizeof(this->RADIO_s)) == nullptr){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Unable to determine beacon's address. Sending a nullptr hostname instead");
        }
    
#if defined YGORNETWORKING_RADIO_USE_FORK

FUNCINFO("Note: This routine has not been tested. After testing, remove this notice");
        //Just fork and execute the lambda!
        if(!fork()){
            //----------------------------- Child Process Fence ---------------------------------
            //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
            signal(SIGPIPE, SIG_IGN);
    
            //Call the user's dialog lambda if it exists. Call the default otherwise.
            if( this->USERS_RADIO_DIALOG_LAMBDA ){
                if(!this->USERS_RADIO_DIALOG_LAMBDA( this->RADIO_sockfd, this->RADIO_s, this->RADIO_PORT )){
                    if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio dialog provided by user failed");
                    //This child SHOULD NOT return here. Let it continue to the exit(0);
                }
            }else if( this->DEFAULT_RADIO_DIALOG_LAMBDA ){
                if(!this->DEFAULT_RADIO_DIALOG_LAMBDA( this->RADIO_sockfd, this->RADIO_s, this->RADIO_PORT )){
                    if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio default dialog failed");
                    //This child SHOULD NOT return here. Let it continue to the exit(0);
                }
            }else{
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio could not dialog because no valid dialog routine is known");
            }
    
            //Now we close up the socket and terminate execution of the child.
            close(this->RADIO_sockfd);
    
            //Finally, terminate execution of this process.
            exit(0);
            //-----------------------------------------------------------------------------------
        }

        //Read in the data to clear it off the stack.
        {
            const long int smallbuff_size = 10;
            char smallbuff[smallbuff_size+1];
            if(recv(this->RADIO_sockfd, smallbuff, smallbuff_size, 0) == -1){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Encountered an error while clearing data out of UDP stack. This data is not used for anything");
            }
        }

        if(!this->RADIO_LOOP) close(this->RADIO_sockfd);
 
#elif defined YGORNETWORKING_RADIO_USE_CLONE
    
        #pragma message "############################################"
        #pragma message "Clone usage is not yet complete. DO NOT USE!"
        #pragma message "############################################"
        #error
    
        //Because we are sharing memory, we copy some things to avoid having them disappear mid-execution.
        // These are copies which are passed in by value to the child's lambda. Seems to work fairly well.
        const std::string hostcopy(this->RADIO_s);
        const int fdcopy(this->RADIO_sockfd);
        const long int portcopy(this->RADIO_PORT);

        auto Childs_Lambda = [=]/*,hostcopy,fdcopy,portcopy]*/(void) -> int {
            //----------------------------- Child Process Fence ---------------------------------
            //Ensure the SIGPIPE signal is ignored. Is this necessary? Should the parent call it?   FIXME.
            //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
            signal(SIGPIPE, SIG_IGN);

            //The following functions expect a raw (char *) array. This may need to change if re-writing 
            // is necessary.
            char *thehostcopy = const_cast<char *>(hostcopy.c_str());

            //Call the user's dialog lambda if it exists. Call the default otherwise.
            //NOTE: I'm unsure of how to properly deal with the lambda functions. Should I copy
            // them too? Should I slap a mutex around them? Wouldn't copying everything be too
            // far of a departure from the 'use threads' mentality?    FIXME.
            if( this->USERS_RADIO_DIALOG_LAMBDA ){
                if(!this->USERS_RADIO_DIALOG_LAMBDA( fdcopy, thehostcopy, portcopy )){
                    if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio dialog provided by user failed");
                    //This child SHOULD NOT return here. Let it continue to the exit(0);
                }
            }else if( this->DEFAULT_RADIO_DIALOG_LAMBDA ){
                if(!this->DEFAULT_RADIO_DIALOG_LAMBDA( fdcopy, thehostcopy, portcopy )){
                    if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio default dialog failed");
                    //This child SHOULD NOT return here. Let it continue to the exit(0);
                }
            }else{
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio could not dialog because no valid dialog routine is known");
            }

            //Now we close up the connection to the beacon and terminate execution of the child.
//            if(closethefd) close(thefd);
            close(fdcopy);

            //Finally, terminate execution of this thread.
            _exit(0);
            //-----------------------------------------------------------------------------------
        };
  
        void **child_stack;
        child_stack = (void **)malloc(16384);   //How to free this? FIXME FIXME FIXME!
       
        if(clone(Childs_Lambda, child_stack, CLONE_VM /*|CLONE_FILES*/, NULL) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Failed to clone - unable to recv message. Continuing");
        }
   
        //Now we close up the connection to the beacon.
        if(!this->RADIO_LOOP) close(this->RADIO_sockfd);
  
#elif defined YGORNETWORKING_RADIO_USE_THREAD
        
        //Because we are sharing memory, we copy some things to avoid having them disappear mid-execution.
        // These are copies which are passed in by value to the child's lambda. Seems to work fairly well.
        const std::string hostcopy(this->RADIO_s);
        const int fdcopy(this->RADIO_sockfd);
        const long int portcopy(this->RADIO_PORT);

        auto Childs_Lambda = [&](std::string thehost, int thefd, long int theport, bool closethefd) -> void {
            //----------------------------- Child Process Fence ---------------------------------
            //Ensure the SIGPIPE signal is ignored. Is this necessary? Should the parent call it?   FIXME.
            //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
            signal(SIGPIPE, SIG_IGN);
    
            //The following functions expect a raw (char *) array. This may need to change if re-writing 
            // is necessary.
            char *thehostcopy = const_cast<char *>(thehost.c_str());
    
            //Call the user's dialog lambda if it exists. Call the default otherwise.
            //NOTE: I'm unsure of how to properly deal with the lambda functions. Should I copy
            // them too? Should I slap a mutex around them? Wouldn't copying everything be too
            // far of a departure from the 'use threads' mentality?    FIXME.
            if( this->USERS_RADIO_DIALOG_LAMBDA ){
                if(!this->USERS_RADIO_DIALOG_LAMBDA( thefd, thehostcopy, theport )){
                    if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio dialog provided by user failed");
                    //This child SHOULD NOT return here. Let it continue to the exit(0);
                }
            }else if( this->DEFAULT_RADIO_DIALOG_LAMBDA ){
                if(!this->DEFAULT_RADIO_DIALOG_LAMBDA( thefd, thehostcopy, theport )){
                    if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio default dialog failed");
                    //This child SHOULD NOT return here. Let it continue to the exit(0);
                }
            }else{
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio could not dialog because no valid dialog routine is known");
            }
    
            //Now we close up the connection to the beacon and terminate execution of the child.
            if(closethefd) close(thefd);
    
            //Finally, terminate execution of this thread.
            return;
            //-----------------------------------------------------------------------------------
        };
    
        //Launch the thread.
        std::thread tolaunch;
        tolaunch = std::thread( Childs_Lambda, hostcopy, fdcopy, portcopy, !this->RADIO_LOOP );
        tolaunch.detach();

        //Child will close this as/when/iff needed.
        //if(!this->RADIO_LOOP) close(this->RADIO_sockfd);

#elif defined YGORNETWORKING_RADIO_NO_CHILD
 
        //Call the user's dialog lambda if it exists. Call the default otherwise.
        if( this->USERS_RADIO_DIALOG_LAMBDA ){
            if(!this->USERS_RADIO_DIALOG_LAMBDA( this->RADIO_sockfd, this->RADIO_s, this->RADIO_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio dialog provided by user failed");
            }
        }else if( this->DEFAULT_RADIO_DIALOG_LAMBDA ){
            if(!this->DEFAULT_RADIO_DIALOG_LAMBDA( this->RADIO_sockfd, this->RADIO_s, this->RADIO_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio default dialog failed");
            }
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio could not dialog because no valid dialog routine is known");
        }
    
        //Now we close up the connection to the beacon.
        if(!this->RADIO_LOOP) close(this->RADIO_sockfd);
    
#else
    #pragma message " #########################################################################################"
    #pragma message " You must define one of:                                                                  "
    #pragma message "     YGORNETWORKING_USE_FORK     - (No memory sharing, fast, safest.)                       "
    #pragma message "     YGORNETWORKING_USE_CLONE    - (Memory sharing only, fairly safe.)                      "
    #pragma message "     YGORNETWORKING_USE_THREAD   - (Memory sharing, fairly safe.)                           "
    #pragma message "     YGORNETWORKING_NO_CHILD     - (Total memory sharing, (sequentially) fast, least safe.) "
    #pragma message " in order to specify how connections are handled by the radio!                           "
    #pragma message " Alternatively, use clone(2) to write your own connection handling scheme.                "
    #pragma message " #########################################################################################"
    #error
#endif

    }while(this->RADIO_LOOP);

    return true;
}

//----------------------------------------------------------------------------------------------
//Beacon-specific member functions.
bool Beacon_and_Radio::Beacon_Init(void){
    return this->Beacon_Init( this->BEACON_PORT, this->USERS_BEACON_DIALOG_LAMBDA );
}

bool Beacon_and_Radio::Beacon_Init(std::function<bool (int, char *, long int)> beacon_dialog){
    return this->Beacon_Init( this->BEACON_PORT, beacon_dialog );
}

bool Beacon_and_Radio::Beacon_Init(long int port, std::function<bool (int, char *, long int)> beacon_dialog){
    this->BEACON_PORT    = port;
    this->USERS_BEACON_DIALOG_LAMBDA = beacon_dialog;

    //This member function will initialize the beacon, preparing it to establish a connection.
    //
    //Whether or not a previous initialization has occured, this function will 'start from
    // scratch' by wiping out the previous initialization. This is desirable in case the 
    // network somehow falls out from under our feet after initial initialization.
    //
    //This function returns 'true' if everything was OK and 'false' if any error arises.
    //
    this->BEACON_INITIALIZED = false;

    //Ignore SIGPIPE's (as in, if one arises, allow us to handle it locally via the send(...) return error EPIPE.
    signal(SIGPIPE, SIG_IGN);

    memset(&this->BEACON_hints, 0, sizeof(this->BEACON_hints));
    this->BEACON_hints.ai_family   = AF_UNSPEC;
    this->BEACON_hints.ai_socktype = SOCK_DGRAM;

    this->BEACON_INITIALIZED = true;
    return true;
}

bool Beacon_and_Radio::Beacon_Transmit(const std::string &address){
    this->BEACON_ADDRESS = address; //Holds the multicast address.

    if(this->BEACON_INITIALIZED != true){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon initialization has not been performed. Unable to establish connection");
        return false;
    }

    const std::string PORT_STRING = Xtostring<long int>(this->BEACON_PORT);
    if((this->BEACON_rv = getaddrinfo(this->BEACON_ADDRESS.c_str(), PORT_STRING.c_str(), &this->BEACON_hints, &this->BEACON_servinfo)) != 0){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("getaddrinfo returned error " << gai_strerror(this->BEACON_rv) << ". Unable to initialize beacon");
        return false;
    }

    //Loop through all the results and connect to the first we can.
    for(this->BEACON_p = this->BEACON_servinfo; this->BEACON_p != nullptr; this->BEACON_p = this->BEACON_p->ai_next){
        if((this->BEACON_sockfd = socket(this->BEACON_p->ai_family, this->BEACON_p->ai_socktype, this->BEACON_p->ai_protocol)) == -1) {
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Radio socket issue. Continuing");
            continue;
        }

        //Attempt to set the timeouts.
        struct timeval timeshuttle;// = {0};
        memset(&timeshuttle, 0, sizeof(timeshuttle));
        timeshuttle.tv_sec  = this->BEACON_send_timeout_sec;
        timeshuttle.tv_usec = this->BEACON_send_timeout_usec;
        if(setsockopt(this->BEACON_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeshuttle, sizeof(struct timeval)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue. Unable to specify recv timeout");
            return false;
        }

        //Attempt to set the TTL of the messages.
        if(setsockopt(this->BEACON_sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &this->BEACON_TTL_HOPS, sizeof(this->BEACON_TTL_HOPS)) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("setsockopt issue. Unable to specify max TTL (aka number of hops)");
            return false;
        }

        //Set the close-on-exec flag so that beacons can execv and not consume/hold onto the port.
        if(fcntl(this->BEACON_sockfd, F_SETFD, FD_CLOEXEC) == -1){
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Attempting to set beacon close-on-exec flag failed. This shouldn't happen, so the file descriptor is likely invalid");
            return false;
        }

        if(connect(this->BEACON_sockfd, this->BEACON_p->ai_addr, this->BEACON_p->ai_addrlen) == -1){
            close(this->BEACON_sockfd);
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon connection issue. Continuing");
            continue;
        }
        break;
    }

    if(this->BEACON_p == nullptr){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon is unable to connect");
        return false;
    }  

    inet_ntop(this->BEACON_p->ai_family, this->Get_in_addr((struct sockaddr *)this->BEACON_p->ai_addr), this->BEACON_s, sizeof(this->BEACON_s));
    //if(YGORNETWORKING_VERBOSE) FUNCINFO("Connected to " << this->BEACON_s);

    //Cleanup stuff we no longer need before talking with the radio.
    freeaddrinfo(this->BEACON_servinfo); 

    //Loop forever, attempting to send messages. If looping is disabled, send out one message and terminate.
    // The user must handle the beacon frequency in the dialog function.
    do{
        //Call the user's dialog lambda if it exists. Call the default otherwise.
        if( this->USERS_BEACON_DIALOG_LAMBDA ){
            if(!this->USERS_BEACON_DIALOG_LAMBDA( this->BEACON_sockfd, this->BEACON_s, this->BEACON_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon dialog provided by user failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else if( this->DEFAULT_BEACON_DIALOG_LAMBDA ){
            if(!this->DEFAULT_BEACON_DIALOG_LAMBDA( this->BEACON_sockfd, this->BEACON_s, this->BEACON_PORT )){
                if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon default dialog failed");
                //This child SHOULD NOT return here. Let it continue to the exit(0);
            }
        }else{
            if(YGORNETWORKING_VERBOSE) FUNCWARN("Beacon could not dialog because no valid dialog routine is known");
        }

    }while(this->BEACON_LOOP);

    //End dialog with radio by closing the connection.
    close(this->BEACON_sockfd);
    return true;
}

//----------------------------------------------------------------------------------------------
//General member functions.
bool Beacon_and_Radio::Set_No_Timeouts(void){
    //This member will remove all radio and beacon timeouts by setting them all to 'infinite' timeout.
    //
    //NOTE: This function will return 'false' if it is too late to set all of the timeouts, 'true' else.
    if(this->RADIO_INITIALIZED || this->BEACON_INITIALIZED){
        if(YGORNETWORKING_VERBOSE) FUNCWARN("Too late to set all timeouts - must be done prior to initialization");
        return false;
    }

    this->RADIO_recv_timeout_sec      = 0;
    this->RADIO_recv_timeout_usec     = 0;

    this->BEACON_send_timeout_sec     = 0;
    this->BEACON_send_timeout_usec    = 0;
    return true;
}

bool Beacon_and_Radio::Set_No_Loop_Radio(void){
    this->RADIO_LOOP = false;
    return true;
}

bool Beacon_and_Radio::Set_No_Loop_Beacon(void){
    this->BEACON_LOOP = false;
    return true;
}

bool Beacon_and_Radio::Set_TTL_Hops_Beacon(unsigned char hops){
    //This will set the maximum number of hops a message can undergo before expiring. It controls how far a 
    // broadcast will ultimately travel. From
    // http://www.ibiblio.org/pub/linux/docs/howto/other-formats/html_single/Multicast-HOWTO.html :
    //
    // TTL (hops)                      Scope
    // ------------------------------------------------------------------------
    //      0    Restricted to the same host. Won't be output by any interface.
    //      1    Restricted to the same subnet. Won't be forwarded by a router.
    //    <32    Restricted to the same site, organization or department.
    //    <64    Restricted to the same region.
    //   <128    Restricted to the same continent.
    //   <255    Unrestricted in scope. Global.

    this->BEACON_TTL_HOPS = hops;
    return true;
}
