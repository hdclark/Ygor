//YgorMisc.cc - Miscellaneous routines which show up too many places to bother repeating again and again.
//

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
//#include <ctime>                 //Needed for gen_time_random().
//#include <fstream>               //Needed for fstream (for file checking.)
//#include <set>

#include <stdio.h>               //Needed for popen, pclose.
#include <unistd.h>              //Needed for read.

#include "YgorMisc.h"

/*
void gen_random(std::string *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    (*s).clear();
    for(int i=0; i<len; ++i)   (*s) += alphanum[rand()%(sizeof(alphanum)-1)];
    return;
}

void seed_time(void){
    //Seed the rand() function with the current time.
    srand(time(NULL));
    return;
}

void Dump_Set_to_Screen(const std::set<std::string>::iterator &begin, const std::set<std::string>::iterator &end){
    for(auto i = begin; i != end; ++i){
        std::cout << " " << *i ;
    }
    std::cout << std::endl;
    return;
}
*/


std::string Execute_Command_In_Pipe(const std::string &cmd){
    std::string out;
    auto pipe = popen(cmd.c_str(), "r");
    if(pipe == nullptr) return out;    

    ssize_t nbytes;
    const long int buffsz = 5000;
    char buff[buffsz];

    #ifdef EAGAIN
        while( ((nbytes = read(fileno(pipe), buff, buffsz)) != -1)  || (errno == EAGAIN) ){
    #else
        while( ((nbytes = read(fileno(pipe), buff, buffsz)) != -1) ){
    #endif

        //Check if we have reached the end of the file (ie. "data has run out.")
        if( nbytes == 0 ) break;

        //Otherwise we fill up the buffer to the high-water mark and move on.
        buff[nbytes] = '\0';
        out += std::string(buff,nbytes); //This is done so that in-buffer '\0's don't confuse = operator.
    }
    pclose(pipe);
    return out;
}
