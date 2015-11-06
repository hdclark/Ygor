
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

#include "YgorMisc.h"
#include "YgorNetworking.h"


int main(int argc, char **argv){
    const auto ips = Get_All_Local_IP4_Addresses();

    for(auto l_it = ips.begin(); l_it != ips.end(); ++l_it){
        std::cout << l_it->first << " ---> " << l_it->second << std::endl;
    }

    return 0;
};
