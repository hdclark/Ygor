
#include <iostream>
#include <cmath>
#include <limits>

int main(int, char **){
  
    std::cout << std::endl;
    std::cout << "double:" << std::endl;
    std::cout << "  machine epsilon = " << std::numeric_limits<double>::epsilon() << std::endl;
    std::cout << "  sqrt(machine epsilon) = " << std::sqrt(std::numeric_limits<double>::epsilon()) << std::endl;
    std::cout << "  pow(sqrt(machine epsilon),2) = " << std::pow(std::sqrt(std::numeric_limits<double>::epsilon()),2.0) << std::endl;
    std::cout << std::endl;
    std::cout << "float:" << std::endl;
    std::cout << "  machine epsilon = " << std::numeric_limits<float>::epsilon() << std::endl;
    std::cout << "  sqrt(machine epsilon) = " << std::sqrt(std::numeric_limits<float>::epsilon()) << std::endl;
    std::cout << "  pow(sqrt(machine epsilon),2) = " << std::pow(std::sqrt(std::numeric_limits<float>::epsilon()),2.0) << std::endl;
    std::cout << std::endl;

    return 0;
}
