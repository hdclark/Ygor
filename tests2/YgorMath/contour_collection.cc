
#include <limits>
#include <list>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"

TEST_CASE( "contour_collection metadata operations" ){
    contour_collection<double> cc;

    cc.contours.emplace_back();
    cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    cc.contours.back().metadata["test1"] = "A";
    cc.contours.back().metadata["test2"] = "B";
    cc.contours.back().metadata["test3"] = "B";

    cc.contours.emplace_back();
    cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    cc.contours.back().metadata["test1"] = "A";
    cc.contours.back().metadata["test2"] = "A";
    cc.contours.back().metadata["test3"] = "B";

    cc.contours.emplace_back();
    cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    cc.contours.back().metadata["test1"] = "B";
    cc.contours.back().metadata["test2"] = "A";
    cc.contours.back().metadata["test3"] = "A";

    const auto count_occurrences = [](const std::list<std::string> &l, const std::string &s){
        long int N = 0;
        for(const auto& o : l) N += (o == s) ? 1 : 0;
        return N;
    };

    SUBCASE("get_all_values_for_key"){
        const auto vals1 = cc.get_all_values_for_key("test1");
        REQUIRE( vals1.size() == 3 );
        REQUIRE( count_occurrences(vals1, "A") == 2 );
        REQUIRE( count_occurrences(vals1, "B") == 1 );
        REQUIRE( count_occurrences(vals1, "C") == 0 );

        const auto vals2 = cc.get_all_values_for_key("test2");
        REQUIRE( vals2.size() == 3 );
        REQUIRE( count_occurrences(vals2, "A") == 2 );
        REQUIRE( count_occurrences(vals2, "B") == 1 );
        REQUIRE( count_occurrences(vals2, "C") == 0 );

        const auto vals3 = cc.get_all_values_for_key("test3");
        REQUIRE( vals3.size() == 3 );
        REQUIRE( count_occurrences(vals3, "A") == 1 );
        REQUIRE( count_occurrences(vals3, "B") == 2 );
        REQUIRE( count_occurrences(vals3, "C") == 0 );
    }

    SUBCASE("get_distinct_values_for_key"){
        const auto vals1 = cc.get_distinct_values_for_key("test1");
        REQUIRE( vals1.size() == 2 );
        REQUIRE( count_occurrences(vals1, "A") == 1 );
        REQUIRE( count_occurrences(vals1, "B") == 1 );
        REQUIRE( count_occurrences(vals1, "C") == 0 );

        const auto vals2 = cc.get_distinct_values_for_key("test2");
        REQUIRE( vals2.size() == 2 );
        REQUIRE( count_occurrences(vals2, "A") == 1 );
        REQUIRE( count_occurrences(vals2, "B") == 1 );
        REQUIRE( count_occurrences(vals2, "C") == 0 );

        const auto vals3 = cc.get_distinct_values_for_key("test3");
        REQUIRE( vals3.size() == 2 );
        REQUIRE( count_occurrences(vals3, "A") == 1 );
        REQUIRE( count_occurrences(vals3, "B") == 1 );
        REQUIRE( count_occurrences(vals3, "C") == 0 );
    }

    SUBCASE("get_dominant_value_for_key"){
        const auto val1 = cc.get_dominant_value_for_key("test1");
        REQUIRE( val1.value_or("(failed)") == "A");

        const auto val2 = cc.get_dominant_value_for_key("test2");
        REQUIRE( val2.value_or("(failed)") == "A");

        const auto val3 = cc.get_dominant_value_for_key("test3");
        REQUIRE( val3.value_or("(failed)") == "B");
    }

}

