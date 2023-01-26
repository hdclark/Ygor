
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

#include "YgorMisc.h"
#include "YgorLog.h"

int main(int, char **){
  
    const auto defaultprecision = std::cout.precision();

    std::cout << std::endl;
    std::cout << "long double:" << std::endl;
    std::cout.precision(std::numeric_limits<long double>::digits10 + 1);
    std::cout << "  machine epsilon = " << std::numeric_limits<long double>::epsilon() << std::endl;
    std::cout << "  sqrt(machine epsilon) = " << std::sqrt(std::numeric_limits<long double>::epsilon()) << std::endl;
    std::cout << "  pow(sqrt(machine epsilon),2) = " << std::pow(std::sqrt(std::numeric_limits<long double>::epsilon()),2.0) << std::endl;

    std::cout << std::endl;
    std::cout << "double:" << std::endl;
    std::cout.precision(std::numeric_limits<double>::digits10 + 1);
    std::cout << "  machine epsilon = " << std::numeric_limits<double>::epsilon() << std::endl;
    std::cout << "  sqrt(machine epsilon) = " << std::sqrt(std::numeric_limits<double>::epsilon()) << std::endl;
    std::cout << "  pow(sqrt(machine epsilon),2) = " << std::pow(std::sqrt(std::numeric_limits<double>::epsilon()),2.0) << std::endl;

    std::cout << std::endl;
    std::cout << "float:" << std::endl;
    std::cout.precision(std::numeric_limits<float>::digits10 + 1);
    std::cout << "  machine epsilon = " << std::numeric_limits<float>::epsilon() << std::endl;
    std::cout << "  sqrt(machine epsilon) = " << std::sqrt(std::numeric_limits<float>::epsilon()) << std::endl;
    std::cout << "  pow(sqrt(machine epsilon),2) = " << std::pow(std::sqrt(std::numeric_limits<float>::epsilon()),2.0) << std::endl;
    std::cout << std::endl;

    std::cout.precision(defaultprecision);

    std::cout << "Endianness: ";
    if constexpr (YgorEndianness::Host == YgorEndianness::Little){
        std::cout << "little";
    }else if constexpr (YgorEndianness::Host == YgorEndianness::Big){
        std::cout << "big";
    }else{
        std::cout << "(unknown)";
    }
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Size of int = " << sizeof(int) << std::endl;
    std::cout << "Size of long int = " << sizeof(long int) << std::endl;
    std::cout << "Size of long long int = " << sizeof(long long int) << std::endl;
    std::cout << "Size of float = " << sizeof(float) << std::endl;
    std::cout << "Size of double = " << sizeof(double) << std::endl;
    std::cout << "Size of char = " << sizeof(char) << std::endl;
    std::cout << "Size of unsigned char = " << sizeof(unsigned char) << std::endl;
    std::cout << std::endl;

    return 0;
}
