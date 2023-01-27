
#include <iostream>
#include <cmath>
#include <list>
#include <functional>
#include <tuple>
#include <vector>
#include <random>
#include <sstream>
#include <limits>
#include <chrono>
#include <thread>


#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorPlot.h"
#include "YgorFilesDirs.h"
#include "YgorString.h"

int main(int, char **){

    // Fill a buffer with some samples.  
    samples_1D<double> buffa;
  
    for(double x = 0.0; x < 10.0; x += 0.15){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffa.push_back(x, dx, f, df);
    }

    // Inject metadata data.
    const auto key_A = "key_A";
    const auto val_A = "val_A";

    const auto key_B = "key\tB";
    const auto val_B = "val_B";

    const auto key_C = "key_C";
    const auto val_C = "val\tC";

    const auto key_D = " very long key D with nasty characters \t \n \0 ";
    const auto val_D = " very long val D with nasty characters \t \n \0 ";

    buffa.metadata[key_A] = val_A;
    buffa.metadata[key_B] = val_B;
    buffa.metadata[key_C] = val_C;
    buffa.metadata[key_D] = val_D;

    // Serialize the sample and verify the metadata is transferred correctly.
    {
        samples_1D<double> buffb;

        std::stringstream ss;
        ss << buffa;
        ss >> buffb;

        if(false){
        }else if(buffb.metadata[key_A] != val_A){
            throw std::runtime_error("metadata (key_A, val_A) did not transfer properly");
        }else if(buffb.metadata[key_B] != val_B){
            throw std::runtime_error("metadata (key_B, val_B) did not transfer properly");
        }else if(buffb.metadata[key_C] != val_C){
            throw std::runtime_error("metadata (key_C, val_C) did not transfer properly");
        }else if(buffb.metadata[key_D] != val_D){
            throw std::runtime_error("metadata (key_D, val_D) did not transfer properly");
        }
    }

    // Stringify the sample (i.e., prepare for plotting) and verify the metadata is transferred correctly.
    {
        samples_1D<double> buffb;

        std::stringstream ss;
        buffa.Write_To_Stream(ss);
        buffb.Read_From_Stream(ss);

        if(false){
        }else if(buffb.metadata[key_A] != val_A){
            throw std::runtime_error("metadata (key_A, val_A) did not transfer properly");
        }else if(buffb.metadata[key_B] != val_B){
            throw std::runtime_error("metadata (key_B, val_B) did not transfer properly");
        }else if(buffb.metadata[key_C] != val_C){
            throw std::runtime_error("metadata (key_C, val_C) did not transfer properly");
        }else if(buffb.metadata[key_D] != val_D){
            throw std::runtime_error("metadata (key_D, val_D) did not transfer properly");
        }
    }

    // Display the serialized and stringified outputs.
    //YLOGINFO("Serialized samples_1D: " << buffa);
    //YLOGINFO("Stringified samples_1D: \n" << buffa.Write_To_String());

    YLOGINFO("OK");

    return 0;
}
