#include <iostream>
#include <string>

#include <sys/socket.h>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorNetworking.h"

bool Dialog_Simple_Echo(int fd, char *host, long int port){
    int numbytes;
    const int BUFFSIZE = 100;
    char buff[BUFFSIZE];

    //Wait until we timeout (or otherwise error) or we get a message from a broadcasting beacon.
    if((numbytes = recv(fd, buff, BUFFSIZE-1, 0)) == -1){
        YLOGWARN("Beacon connection issue. Nothing was received");
        return false;
    }
    buff[numbytes] = '\0';
    YLOGINFO("Received message from beacon at " << host << ": '" << buff << "'");
    return true;
}


int main(int argc, char **argv){

    Beacon_and_Radio test;

    if(!test.Radio_Init(12346, &Dialog_Simple_Echo)){
        YLOGERR("Unable to initialize radio");
    }

    if(!test.Radio_Tune_In("225.0.0.37")){
        YLOGERR("Unable to tune in the radio");
    }

    return 0;
}
