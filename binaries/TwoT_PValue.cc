#include <iostream>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorStats.h"


int main(int  /*argc*/, char ** /*argv*/){
    YLOGINFO("This program takes six consequtive numbers: ");
    YLOGINFO("  <mean A>  <var A>  <N A>   <mean B>  <var B>  <N B> ");
    YLOGINFO(" and spits out the two-tailed, unequal-variance P value denoting whether ");
    YLOGINFO(" they describe the same underlying distribution. ");
    YLOGINFO("Do NOT use std.dev. instead of variance! Remember: var = sigma*sigma ");

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
                 YLOGWARN("Couldn't make sense of the input. Ensure it is in the format:");
                 YLOGINFO("  <mean A>  <var A>  <N A>   <mean B>  <var B>  <N B> ");
                 break; //YLOGERR("Failed to get column numbers. Were 2 provided on every line?");
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
