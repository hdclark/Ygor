
#include <limits>
#include <utility>
#include <iostream>

#include <YgorTime.h>

#include "doctest/doctest.h"


TEST_CASE( "time_mark string IO" ){
    time_mark tm;
    double frac_seconds = 1.23;

    REQUIRE(tm.Read_from_string("2021-08-29 20:27:26.000000"));
    REQUIRE(tm.Read_from_string("2021-08-29 20:27:26.000000", &frac_seconds));
    REQUIRE(frac_seconds == 0.0);
    REQUIRE(tm.Dump_as_string() == "20210829-202726");

    REQUIRE(tm.Read_from_string("21-08-29 20:27:26.123456"));
    REQUIRE(tm.Read_from_string("21-08-29 20:27:26.123456", &frac_seconds));
    REQUIRE(frac_seconds == 0.123456);
    REQUIRE(tm.Dump_as_string() == "20210829-202726");

    // ISO 8601
    REQUIRE(tm.Read_from_string("2025-08-29T20:27:26.23456Z", &frac_seconds));
    REQUIRE(frac_seconds == 0.23456);

    REQUIRE(!tm.Read_from_string("2021-0-31 20:27:26")); // Non-existent month.
    REQUIRE(!tm.Read_from_string("2021-09-0 20:27:26")); // Non-existent day.
    REQUIRE(!tm.Read_from_string("2021-09-32 20:27:26")); // Non-existent day.
}

TEST_CASE( "time_mark string IO" ){
    time_mark tm;
    double frac_seconds = 1.23;

    // UNIX time extraction.
    frac_seconds = 0.0;
    tm.Set_unix_epoch();
    REQUIRE(0.0 == tm.As_UNIX_time());
    REQUIRE(0.0 == tm.As_UNIX_time(frac_seconds));
    frac_seconds = 0.345;
    REQUIRE(0.345 == tm.As_UNIX_time(frac_seconds));

    tm.Read_from_string("20090213-233130");
    REQUIRE(doctest::Approx(1234567890.0).epsilon(1.0) == tm.As_UNIX_time());
    frac_seconds = 0.123;
    REQUIRE(doctest::Approx(1234567890.123).epsilon(1.0) == tm.As_UNIX_time(frac_seconds));
}
