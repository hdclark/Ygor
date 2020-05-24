
#include <limits>
#include "YgorMath.h"

#define NONIUS_RUNNER
#include <nonius/nonius_single.h++>

NONIUS_BENCHMARK("vec3 to_string and from_string", [](nonius::chronometer meter){
    vec3<double> A;
    const vec3<double> B( std::numeric_limits<double>::min(),
                          std::numeric_limits<double>::denorm_min(),
                          1.0E308 ); 

    const vec3<double> C( 1.234567E-307,
                          std::nexttoward( 0.0,  1.0),
                          std::nexttoward( 0.0, -1.0) );

    const vec3<double> D(  std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity() );

    const vec3<double> E( std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::lowest(),
                          1.1234567890123456789 );

    meter.measure([&](void){ 
        A.from_string( B.to_string() );
        A.from_string( C.to_string() );
        A.from_string( D.to_string() );
        A.from_string( E.to_string() );
        return A;
    });
    return;
});

