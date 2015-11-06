//YgorNetworking.h - A few constructs which are useful for basic networking. These are basically
// simple wrappers for BSD-style sockets.

#ifndef HDR_GRD_YGORNETWORKING_H_
#define HDR_GRD_YGORNETWORKING_H_

#include <string>
#include <cstdlib>
#include <functional>
#include <thread>
#include <list>
#include <mutex>
#include <set>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "YgorContainers.h"

#ifdef USE_OWN_GETIFADDR
    //Adapted from http://www.openwall.com/lists/musl/2012/09/19/8.
    struct ifaddrs {
        struct ifaddrs    *ifa_next;
        char        *ifa_name;
        unsigned int     ifa_flags;
        struct sockaddr    *ifa_addr;
        struct sockaddr    *ifa_netmask;
        union {
            struct sockaddr *ifu_broadaddr;
            struct sockaddr *ifu_dstaddr;
        } ifa_ifu;
        #define    ifa_broadaddr ifa_ifu.ifu_broadaddr
        #define    ifa_dstaddr   ifa_ifu.ifu_dstaddr
        void        *ifa_data;
    };
#endif


static bool YGORNETWORKING_VERBOSE(false);

//Some basic string functions to construct canned requests, responses, etc..
std::string Basic_HTTP_Request_Header(const std::string &hostandport);
std::string Basic_HTTP_Request_Header(const std::string &hostandport, const std::string &request);
std::string Basic_HTTP_Response_Text_Header(const std::string &thetext);
std::string Basic_HTTP_Response_File_Header(const std::string &filename);   //Loads file into response.
std::string Basic_HTTP_Response_File_Header_Only(const std::string &filename);
std::string Range_HTTP_Response_File_Header_Only(const std::string &filename, off_t range_l, off_t range_u);

//Some simple, non-nonsense, low-customizability routines for sending HTTP requests.
std::string Simple_HTTP_Request(const std::string &host, const std::string &request);
std::string Simple_HTTPS_Request(const std::string &host, const std::string &port = "", const std::string &request = "");   //Currently implemented using pipe/wget! FIXME


//----------------------------------------------------------------------------------------------
//--------------------------------- IP Address utilities ---------------------------------------
//----------------------------------------------------------------------------------------------
//These routines are fairly hacky. They attempt to determine ip addresses using whatever method
// possible. Output format is <device name> <address>
//
//The 'local' routines check the local machine for the dhcp/static address.
//The 'distant' routines hackishly request the ip from a remote server.
//In between, any NAT which occurs is essentially invisible.
//
//Future plans are to investigate the various hole-punching schemes and (newer) NAT traversal
// protocols.
std::list<std::pair<std::string, std::string>> Get_All_Local_IP4_Addresses(void);
std::list<std::pair<std::string, std::string>> Get_All_Local_IP6_Addresses(void);
std::list<std::pair<std::string, std::string>> Get_All_Local_IP_Addresses(void);

//These 'distant' functions may or may not have an associated name. Do not rely on it...
std::list<std::pair<std::string, std::string>> Get_All_Distant_IP_Addresses(void);



//----------------------------------------------------------------------------------------------
//------------------------------- clone(2) Shuttle Struct --------------------------------------
//----------------------------------------------------------------------------------------------
//This struct is a simple wrapper for converting Server_and_Client class members into an object
// which can be cast to (void *). 
//
//It is used to pass in member variables (and functions...) to the clone(2) function, which 
// requires a (void *)-able function argument.
struct YgorNetworking_Server_and_Client_Clone_Shuttle {
    int SERVER_sockfd; 
    std::function<bool (int, char *, long int)> USERS_SERVER_DIALOG_LAMBDA;
    std::function<bool (int, char *, long int)> DEFAULT_SERVER_DIALOG_LAMBDA;
    int SERVER_new_fd;
    char SERVER_s[INET6_ADDRSTRLEN];
    long int SERVER_PORT; 
    void *Stack; 
};


//----------------------------------------------------------------------------------------------
//---------------------------- Server_and_Client: a TCP networking class -----------------------
//----------------------------------------------------------------------------------------------
//This class can operate as both a client and a server. This is useful in cases where an entity
// is required to pass network requests around. This class should be sufficient for the purposes
// of handling the networking aspects of a simple network 'node' or entity.
//
//This interface is a WIP. If something needs tweaking by the user (timeouts, forking model, 
// etc..) then provide a public member function to call between ..Init() and ..Connect() or
// ..Wait() members. Try to avoid renaming or destroying members if possible.
//
class Server_and_Client {
    private:
        //--------------------------------------------------------------------------------------
        //Experimental things.
        std::mutex Blocklist_Mutex;
        std::set<std::string> Blocklist;
//        #if defined YGORNETWORKING_USE_THREAD_POOL
            taskpool threadpool;
//        #endif

        //--------------------------------------------------------------------------------------
        //Server-specific things.
        bool SERVER_INITIALIZED;              //Denotes whether or not the server has been Init()'d.
        long int SERVER_PORT;                 //Which port to listen on.
        long int SERVER_BACKLOG;              //The number of pending connections in the queue.

        //This is a user-provided dialog function. It handles how the server sends and receives.
        // The argument is a file descriptor, which is needed to send or receive.
        // The second is the host (destination) address.
        // The third is the host (destination) port.
        std::function<bool (int, char *, long int)> USERS_SERVER_DIALOG_LAMBDA;
        std::function<bool (int, char *, long int)> DEFAULT_SERVER_DIALOG_LAMBDA;
   
        //------ The following server-specific things are probably not of interest to the outside.
        int SERVER_sockfd, SERVER_new_fd;     //We listen on sock_fd and get new connections on new_fd.
        int SERVER_yes, SERVER_rv;            //"yes" is a simple int(1). "rv" is a multiple-use return val.
        size_t SERVER_accept_timeout_sec;     //These control how long the server will wait for a connection.
        size_t SERVER_accept_timeout_usec;    // If time runs out, the server will probably exit()!
        size_t SERVER_recv_timeout_sec;       //These control how long the server will wait on a message
        size_t SERVER_recv_timeout_usec;      // (send() and recv()) before failing. This could be useful
        size_t SERVER_send_timeout_sec;       //  for catching disconnected clients/servers but should
        size_t SERVER_send_timeout_usec;      //  not be set too short (to better handle flaky networks.)
        char SERVER_s[INET6_ADDRSTRLEN+1];    //Stores the incoming (client) address (after connecting).
        struct addrinfo SERVER_hints, *SERVER_servinfo, *SERVER_p;
        struct sockaddr_storage SERVER_their_addr;   //Connector's connection's metadata. See also SERVER_s.
        socklen_t SERVER_sin_size;
        struct sigaction SERVER_sa;
        std::thread Connection_Thread;  //Gets launched/detached whenever a connection is established.

        //--------------------------------------------------------------------------------------
        //Client-specific things.
        bool CLIENT_INITIALIZED;              //Denotes whether or not the server has been Init()'d.
        long int CLIENT_PORT;                 //Which port to connect to.
        std::string CLIENT_ADDRESS;           //The address the client should attempt to contact.

        //This is a user-provided dialog function. It handles how the client sends and receives.
        // The argument is a file descriptor, which is needed to send or receive.
        // The second is the host (destination) address.
        // The third is the host (destination) port.
        std::function<bool (int, char *, long int)> USERS_CLIENT_DIALOG_LAMBDA; 
        std::function<bool (int, char *, long int)> DEFAULT_CLIENT_DIALOG_LAMBDA;

        //------ The following client-specific things are probably not of interest to the outside.
        int  CLIENT_sockfd, CLIENT_rv;
        size_t CLIENT_recv_timeout_sec;
        size_t CLIENT_recv_timeout_usec;
        size_t CLIENT_send_timeout_sec;
        size_t CLIENT_send_timeout_usec;
        char CLIENT_s[INET6_ADDRSTRLEN+1];
        struct addrinfo CLIENT_hints, *CLIENT_servinfo, *CLIENT_p;

        //--------------------------------------------------------------------------------------
        //General helper member functions.
        void *Get_in_addr(struct sockaddr *);

    public:
        //Constructors, Destructor.
        Server_and_Client(void);

        //--------------------------------------------------------------------------------------
        //Server-specific member functions.
        bool Server_Init(long int port, std::function<bool (int, char *, long int)> server_dialog);
        bool Server_Init(std::function<bool (int, char *, long int)> server_dialog);
        bool Server_Init(void);

        bool Server_Wait_for_Connection(void);

        //--------------------------------------------------------------------------------------
        //Client-specific member functions.
        bool Client_Init(long int port, std::function<bool (int, char *, long int)> client_dialog);
        bool Client_Init(std::function<bool (int, char *, long int)> client_dialog);
        bool Client_Init(void);

        bool Client_Connect(const std::string &address);

        //--------------------------------------------------------------------------------------
        //General member functions.
        bool Set_No_Timeouts(void);

        //--------------------------------------------------------------------------------------
        //Experimental things: Server ONLY.
        void Add_To_Blocklist(const std::string &host);
        void Remove_From_Blocklist(const std::string &host);
        bool Is_In_Blocklist(const std::string &host);

};


//----------------------------------------------------------------------------------------------
//--------------------------- Beacon_and_Radio: a UDP multicasting class -----------------------
//----------------------------------------------------------------------------------------------
//This class can operate as both a sender and a receiver. It is designed to announce messages
// across networks, and is generally not designed to *respond* to received messages. 
//
//This interface is a WIP. If something needs tweaking by the user (timeouts, forking model, 
// etc..) then provide a public member function to call between ..Init() and ..Connect() or
// ..Wait() members. Try to avoid renaming or destroying members if possible.
//
class Beacon_and_Radio {
    private:
        //--------------------------------------------------------------------------------------
        //Radio-specific things.
        bool RADIO_INITIALIZED;              //Denotes whether or not the radio has been Init()'d.
        long int RADIO_PORT;                 //Which port to listen on.
//        long int RADIO_BACKLOG;              //The number of pending connections in the queue.
        bool RADIO_LOOP;                     //Instead of closing socket, loop forever.

        //This is a user-provided dialog function. It handles how the radio sends messages.
        // The first argument is a file descriptor, which is needed to send.
        // The second is the host (destination) address. *In this case: the multicast address.*
        // The third is the host (destination) port.
        std::function<bool (int, char *, long int)> USERS_RADIO_DIALOG_LAMBDA;
        std::function<bool (int, char *, long int)> DEFAULT_RADIO_DIALOG_LAMBDA;
   
        //------ The following radio-specific things are probably not of interest to the outside.
        int RADIO_sockfd;                    //We listen on sock_fd. UDP is connectionless, so no beacon fd.
        int RADIO_yes, RADIO_rv;             //"yes" is a simple int(1). "rv" is a multiple-use return val.
        size_t RADIO_recv_timeout_sec;       //These control how long the beacon will wait in recv().
        size_t RADIO_recv_timeout_usec;      // They are typically very long.
        char RADIO_s[INET6_ADDRSTRLEN+1];    //Stores the radio's address we end up using.
        struct addrinfo RADIO_hints, *RADIO_servinfo, *RADIO_p;
//        socklen_t RADIO_sin_size;
        struct sigaction RADIO_sa;

        //--------------------------------------------------------------------------------------
        //Beacon-specific things.
        bool BEACON_INITIALIZED;              //Denotes whether or not the radio has been Init()'d.
        long int BEACON_PORT;                 //Which port to connect to.
        std::string BEACON_ADDRESS;           //The multicast address the beacon should tune in to.
        bool BEACON_LOOP;                     //Instead of closing socket, loop forever. User should sleep().
        unsigned char BEACON_TTL_HOPS;        //The number of hops before message expires. Chosing 1 
                                              // will ensure the multicast does not leave a LAN. Max is 255.

        //This is a user-provided dialog function. It handles how the beacon receives.
        // The first argument is a file descriptor, which is needed to receive.
        // The second is the host (destination) address. *In this case: the multicast address.*
        // The third is the host (destination) port.
        std::function<bool (int, char *, long int)> USERS_BEACON_DIALOG_LAMBDA; 
        std::function<bool (int, char *, long int)> DEFAULT_BEACON_DIALOG_LAMBDA;

        //------ The following beacon-specific things are probably not of interest to the outside.
        int  BEACON_sockfd, BEACON_rv;
        size_t BEACON_send_timeout_sec;       //These control how long the beacon will wait in send().
        size_t BEACON_send_timeout_usec;      // It should be very short because UDP is connectionless.
        char BEACON_s[INET6_ADDRSTRLEN+1];    //Holds the intended multicast address. 
        struct addrinfo BEACON_hints, *BEACON_servinfo, *BEACON_p;

        //--------------------------------------------------------------------------------------
        //General helper member functions.
        void *Get_in_addr(struct sockaddr *);

    public:
        //Constructors, Destructor.
        Beacon_and_Radio(void);

        //--------------------------------------------------------------------------------------
        //Radio-specific member functions.
        bool Radio_Init(long int port, std::function<bool (int, char *, long int)> radio_dialog);
        bool Radio_Init(std::function<bool (int, char *, long int)> radio_dialog);
        bool Radio_Init(void);

        bool Radio_Tune_In(const std::string &address); //Valid multicast IP's: [224.0.0.0, 239.255.255.255].

        //--------------------------------------------------------------------------------------
        //Beacon-specific member functions.
        bool Beacon_Init(long int port, std::function<bool (int, char *, long int)> beacon_dialog);
        bool Beacon_Init(std::function<bool (int, char *, long int)> beacon_dialog);
        bool Beacon_Init(void);

        bool Beacon_Transmit(const std::string &address); //Valid multicast IP's: [224.0.0.0, 239.255.255.255].

        //--------------------------------------------------------------------------------------
        //General member functions.
        bool Set_No_Timeouts(void);
        bool Set_No_Loop_Radio(void);
        bool Set_No_Loop_Beacon(void);

        bool Set_TTL_Hops_Beacon(unsigned char); //See source for reasonable values. Usually ~ <32.
};

#endif
