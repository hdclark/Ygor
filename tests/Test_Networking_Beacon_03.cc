#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <sys/socket.h>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorNetworking.h"

bool Dialog_Simple_Send(int fd, char *host, long int port){
    //Send a simple message to the radios.
    if(send(fd, "This is the default beacon message!", 38, MSG_NOSIGNAL) == -1){
        YLOGWARN("Beacon unable to send()");
        return false;
    }
    YLOGINFO("Sent message to " << ((host == nullptr) ? "?" : host) );

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    return true;
}


int main(int argc, char **argv){

    Beacon_and_Radio test;

    if(!test.Beacon_Init(12346, &Dialog_Simple_Send)){
        YLOGERR("Unable to initialize beacon");
    }

    test.Set_TTL_Hops_Beacon(5);

    if(!test.Beacon_Transmit("225.0.0.37")){
        YLOGERR("Unable to transmit");
    }

    return 0;
}
