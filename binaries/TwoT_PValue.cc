#include <iostream>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorStats.h"


int main(int  /*argc*/, char ** /*argv*/){
    FUNCINFO("This program takes six consequtive numbers: ");
    FUNCINFO("  <mean A>  <var A>  <N A>   <mean B>  <var B>  <N B> ");
    FUNCINFO(" and spits out the two-tailed, unequal-variance P value denoting whether ");
    FUNCINFO(" they describe the same underlying distribution. ");
    FUNCINFO("Do NOT use std.dev. instead of variance! Remember: var = sigma*sigma ");

    while(true){
        std::cout << std::endl;
        std::string line;
        while(std::getline(std::cin, line)){
            //std::cout << line << std::endl;
            std::stringstream ss(line);

            double meanA,varA,meanB,varB;
            long int numA,numB;
            ss >> meanA;
            ss >> varA;
            ss >> numA;

            ss >> meanB;
            ss >> varB;
            ss >> numB;

            if(ss.fail()){
                 FUNCWARN("Couldn't make sense of the input. Ensure it is in the format:");
                 FUNCINFO("  <mean A>  <var A>  <N A>   <mean B>  <var B>  <N B> ");
                 break; //FUNCERR("Failed to get column numbers. Were 2 provided on every line?");
            }
            std::cout << "A (mean, var, #) = (" << meanA << ", " << varA << ", " << numA << ")    ";
            std::cout << "B (mean, var, #) = (" << meanB << ", " << varB << ", " << numB << ")    ";
            std::cout << std::endl;

            std::cout << "    P = ";
            std::cout << Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA,varA,numA,meanB,varB,numB);
            std::cout << std::endl;

        }
    }
    return 0;
}
