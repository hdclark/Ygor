
#include <any>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

#include <YgorMath.h>
#include <YgorIndex.h>
#include <YgorIndexCells.h>

#include "doctest/doctest.h"


TEST_CASE( "cells_index basic construction" ){
    SUBCASE("default constructor"){
        cells_index<double> idx;
        REQUIRE(idx.get_size() == 0);
    }
    
    SUBCASE("constructor with cell size"){
        cells_index<double> idx(2.0);
        REQUIRE(idx.get_size() == 0);
    }
    
    SUBCASE("invalid cell size throws"){
        REQUIRE_THROWS(cells_index<double>(0.0));
        REQUIRE_THROWS(cells_index<double>(-1.0));
    }
}

TEST_CASE( "cells_index entry operations" ){
    SUBCASE("entry default constructor"){
        cells_index<double>::entry e;
        REQUIRE(e.point.x == 0.0);
        REQUIRE(e.point.y == 0.0);
        REQUIRE(e.point.z == 0.0);
        REQUIRE(e.aux_data.has_value() == false);
    }
    
    SUBCASE("entry with point only"){
        vec3<double> point(1.0, 2.0, 3.0);
        cells_index<double>::entry e(point);
        REQUIRE(e.point.x == 1.0);
        REQUIRE(e.point.y == 2.0);
        REQUIRE(e.point.z == 3.0);
        REQUIRE(e.aux_data.has_value() == false);
    }
    
    SUBCASE("entry with point and aux data"){
        vec3<double> point(1.0, 2.0, 3.0);
        int aux_value = 42;
        cells_index<double>::entry e(point, aux_value);
        REQUIRE(e.point.x == 1.0);
        REQUIRE(e.point.y == 2.0);
        REQUIRE(e.point.z == 3.0);
        REQUIRE(e.aux_data.has_value() == true);
        REQUIRE(std::any_cast<int>(e.aux_data) == 42);
    }
    
    SUBCASE("entry equality comparison"){
        vec3<double> point1(1.0, 2.0, 3.0);
        vec3<double> point2(1.0, 2.0, 3.0);
        vec3<double> point3(4.0, 5.0, 6.0);
        
        cells_index<double>::entry e1(point1, 42);
        cells_index<double>::entry e2(point2, 100);
        cells_index<double>::entry e3(point3, 42);
        
        REQUIRE(e1 == e2);
        REQUIRE(!(e1 == e3));
    }
}

TEST_CASE( "cells_index bbox operations" ){
    SUBCASE("bbox construction"){
        cells_index<double>::bbox box1;
        REQUIRE(box1.min.x == 0.0);
        REQUIRE(box1.min.y == 0.0);
        REQUIRE(box1.min.z == 0.0);
        REQUIRE(box1.max.x == 0.0);
        REQUIRE(box1.max.y == 0.0);
        REQUIRE(box1.max.z == 0.0);
        
        vec3<double> min_corner(1.0, 2.0, 3.0);
        vec3<double> max_corner(4.0, 5.0, 6.0);
        cells_index<double>::bbox box2(min_corner, max_corner);
        REQUIRE(box2.min.x == 1.0);
        REQUIRE(box2.min.y == 2.0);
        REQUIRE(box2.min.z == 3.0);
        REQUIRE(box2.max.x == 4.0);
        REQUIRE(box2.max.y == 5.0);
        REQUIRE(box2.max.z == 6.0);
    }
    
    SUBCASE("bbox volume"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 3.0, 4.0);
        cells_index<double>::bbox box(min_corner, max_corner);
        REQUIRE(box.volume() == 24.0);
    }
    
    SUBCASE("bbox contains point"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 2.0, 2.0);
        cells_index<double>::bbox box(min_corner, max_corner);
        
        REQUIRE(box.contains(vec3<double>(1.0, 1.0, 1.0)) == true);
        REQUIRE(box.contains(vec3<double>(0.0, 0.0, 0.0)) == true);
        REQUIRE(box.contains(vec3<double>(2.0, 2.0, 2.0)) == true);
        REQUIRE(box.contains(vec3<double>(3.0, 1.0, 1.0)) == false);
    }
    
    SUBCASE("bbox intersects"){
        vec3<double> min1(0.0, 0.0, 0.0);
        vec3<double> max1(2.0, 2.0, 2.0);
        cells_index<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        cells_index<double>::bbox box2(min2, max2);
        
        vec3<double> min3(3.0, 3.0, 3.0);
        vec3<double> max3(4.0, 4.0, 4.0);
        cells_index<double>::bbox box3(min3, max3);
        
        REQUIRE(box1.intersects(box2) == true);
        REQUIRE(box1.intersects(box3) == false);
    }
}

TEST_CASE( "cells_index insertion and search" ){
    SUBCASE("insert single point"){
        cells_index<double> idx(1.0);
        vec3<double> point(1.0, 2.0, 3.0);
        idx.insert(point);
        
        REQUIRE(idx.get_size() == 1);
        REQUIRE(idx.contains(point) == true);
    }
    
    SUBCASE("insert multiple points"){
        cells_index<double> idx(1.0);
        std::vector<vec3<double>> points = {
            vec3<double>(1.0, 1.0, 1.0),
            vec3<double>(2.0, 2.0, 2.0),
            vec3<double>(3.0, 3.0, 3.0),
            vec3<double>(4.0, 4.0, 4.0),
            vec3<double>(5.0, 5.0, 5.0)
        };
        
        for(const auto& p : points) {
            idx.insert(p);
        }
        
        REQUIRE(idx.get_size() == 5);
        
        for(const auto& p : points) {
            REQUIRE(idx.contains(p) == true);
        }
    }
    
    SUBCASE("search by bounding box returns entries"){
        cells_index<double> idx(1.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0));
        idx.insert(vec3<double>(2.0, 2.0, 2.0));
        idx.insert(vec3<double>(5.0, 5.0, 5.0));
        idx.insert(vec3<double>(10.0, 10.0, 10.0));
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(3.0, 3.0, 3.0));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 2);
        
        bool found_1 = false;
        bool found_2 = false;
        for(const auto& r : results) {
            if(r.point == vec3<double>(1.0, 1.0, 1.0)) found_1 = true;
            if(r.point == vec3<double>(2.0, 2.0, 2.0)) found_2 = true;
        }
        REQUIRE(found_1 == true);
        REQUIRE(found_2 == true);
    }
    
    SUBCASE("search_points returns vec3 directly"){
        cells_index<double> idx(1.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0));
        idx.insert(vec3<double>(2.0, 2.0, 2.0));
        idx.insert(vec3<double>(5.0, 5.0, 5.0));
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(3.0, 3.0, 3.0));
        auto results = idx.search_points(query_box);
        
        REQUIRE(results.size() == 2);
        
        bool found_1 = false;
        bool found_2 = false;
        for(const auto& r : results) {
            if(r == vec3<double>(1.0, 1.0, 1.0)) found_1 = true;
            if(r == vec3<double>(2.0, 2.0, 2.0)) found_2 = true;
        }
        REQUIRE(found_1 == true);
        REQUIRE(found_2 == true);
    }
    
    SUBCASE("search by radius"){
        cells_index<double> idx(2.0);
        idx.insert(vec3<double>(0.0, 0.0, 0.0));
        idx.insert(vec3<double>(1.0, 0.0, 0.0));
        idx.insert(vec3<double>(0.0, 1.0, 0.0));
        idx.insert(vec3<double>(0.0, 0.0, 1.0));
        idx.insert(vec3<double>(5.0, 5.0, 5.0));
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = idx.search_radius(center, 1.5);
        
        REQUIRE(results.size() == 4);
    }
    
    SUBCASE("search_radius_points returns vec3 directly"){
        cells_index<double> idx(2.0);
        idx.insert(vec3<double>(0.0, 0.0, 0.0));
        idx.insert(vec3<double>(1.0, 0.0, 0.0));
        idx.insert(vec3<double>(5.0, 5.0, 5.0));
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = idx.search_radius_points(center, 1.5);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("nearest neighbors returns entries"){
        cells_index<double> idx(5.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0));
        idx.insert(vec3<double>(2.0, 2.0, 2.0));
        idx.insert(vec3<double>(3.0, 3.0, 3.0));
        idx.insert(vec3<double>(10.0, 10.0, 10.0));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = idx.nearest_neighbors(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].point == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(results[1].point == vec3<double>(2.0, 2.0, 2.0));
    }
    
    SUBCASE("nearest_neighbors_points returns vec3 directly"){
        cells_index<double> idx(5.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0));
        idx.insert(vec3<double>(2.0, 2.0, 2.0));
        idx.insert(vec3<double>(3.0, 3.0, 3.0));
        idx.insert(vec3<double>(10.0, 10.0, 10.0));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = idx.nearest_neighbors_points(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0] == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(results[1] == vec3<double>(2.0, 2.0, 2.0));
    }
}

TEST_CASE( "cells_index auxiliary data" ){
    SUBCASE("insert with integer aux data"){
        cells_index<double> idx(1.0);
        vec3<double> point(1.0, 2.0, 3.0);
        idx.insert(point, 42);
        
        REQUIRE(idx.get_size() == 1);
        REQUIRE(idx.contains(point) == true);
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].point == point);
        REQUIRE(results[0].aux_data.has_value() == true);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 42);
    }
    
    SUBCASE("insert with string aux data"){
        cells_index<double> idx(1.0);
        vec3<double> point(1.0, 2.0, 3.0);
        std::string label = "test_point";
        idx.insert(point, label);
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "test_point");
    }
    
    SUBCASE("multiple points with different aux data types"){
        cells_index<double> idx(1.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0), 42);
        idx.insert(vec3<double>(2.0, 2.0, 2.0), std::string("hello"));
        idx.insert(vec3<double>(3.0, 3.0, 3.0), 3.14159);
        idx.insert(vec3<double>(4.0, 4.0, 4.0));
        
        REQUIRE(idx.get_size() == 4);
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 4);
        
        for(const auto& r : results) {
            if(r.point == vec3<double>(1.0, 1.0, 1.0)) {
                REQUIRE(std::any_cast<int>(r.aux_data) == 42);
            } else if(r.point == vec3<double>(2.0, 2.0, 2.0)) {
                REQUIRE(std::any_cast<std::string>(r.aux_data) == "hello");
            } else if(r.point == vec3<double>(3.0, 3.0, 3.0)) {
                REQUIRE(std::any_cast<double>(r.aux_data) == doctest::Approx(3.14159));
            } else if(r.point == vec3<double>(4.0, 4.0, 4.0)) {
                REQUIRE(r.aux_data.has_value() == false);
            }
        }
    }
    
    SUBCASE("nearest neighbors preserves aux data"){
        cells_index<double> idx(5.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0), std::string("closest"));
        idx.insert(vec3<double>(2.0, 2.0, 2.0), std::string("second"));
        idx.insert(vec3<double>(10.0, 10.0, 10.0), std::string("far"));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = idx.nearest_neighbors(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "closest");
        REQUIRE(std::any_cast<std::string>(results[1].aux_data) == "second");
    }
    
    SUBCASE("search_radius preserves aux data"){
        cells_index<double> idx(2.0);
        idx.insert(vec3<double>(0.0, 0.0, 0.0), 100);
        idx.insert(vec3<double>(1.0, 0.0, 0.0), 200);
        idx.insert(vec3<double>(10.0, 10.0, 10.0), 999);
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = idx.search_radius(center, 2.0);
        
        REQUIRE(results.size() == 2);
        
        int sum = 0;
        for(const auto& r : results) {
            sum += std::any_cast<int>(r.aux_data);
        }
        REQUIRE(sum == 300);
    }
}

TEST_CASE( "cells_index clear" ){
    SUBCASE("clear removes all entries"){
        cells_index<double> idx(1.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0));
        idx.insert(vec3<double>(2.0, 2.0, 2.0));
        idx.insert(vec3<double>(3.0, 3.0, 3.0));
        
        REQUIRE(idx.get_size() == 3);
        
        idx.clear();
        
        REQUIRE(idx.get_size() == 0);
        REQUIRE(idx.contains(vec3<double>(1.0, 1.0, 1.0)) == false);
    }
    
    SUBCASE("clear removes aux data as well"){
        cells_index<double> idx(1.0);
        idx.insert(vec3<double>(1.0, 1.0, 1.0), 42);
        idx.insert(vec3<double>(2.0, 2.0, 2.0), std::string("test"));
        
        REQUIRE(idx.get_size() == 2);
        
        idx.clear();
        
        REQUIRE(idx.get_size() == 0);
        
        cells_index<double>::bbox query_box(vec3<double>(-10.0, -10.0, -10.0), vec3<double>(10.0, 10.0, 10.0));
        auto results = idx.search(query_box);
        REQUIRE(results.size() == 0);
    }
}

TEST_CASE( "cells_index large dataset" ){
    SUBCASE("insert many points and verify"){
        cells_index<double> idx(1.0);
        std::vector<vec3<double>> points;
        
        for(int i = 0; i < 10; ++i) {
            for(int j = 0; j < 10; ++j) {
                for(int k = 0; k < 10; ++k) {
                    vec3<double> p(static_cast<double>(i), 
                                   static_cast<double>(j), 
                                   static_cast<double>(k));
                    points.push_back(p);
                    idx.insert(p);
                }
            }
        }
        
        REQUIRE(idx.get_size() == 1000);
        
        for(const auto& p : points) {
            REQUIRE(idx.contains(p) == true);
        }
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(2.0, 2.0, 2.0));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 27);
    }
    
    SUBCASE("large dataset with aux data"){
        cells_index<double> idx(1.0);
        
        for(int i = 0; i < 10; ++i) {
            for(int j = 0; j < 10; ++j) {
                for(int k = 0; k < 10; ++k) {
                    vec3<double> p(static_cast<double>(i), 
                                   static_cast<double>(j), 
                                   static_cast<double>(k));
                    int index = i * 100 + j * 10 + k;
                    idx.insert(p, index);
                }
            }
        }
        
        REQUIRE(idx.get_size() == 1000);
        
        cells_index<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(0.0, 0.0, 0.0));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 0);
        
        query_box = cells_index<double>::bbox(vec3<double>(5.0, 5.0, 5.0), vec3<double>(5.0, 5.0, 5.0));
        results = idx.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 555);
    }
}

TEST_CASE( "cells_index with float type" ){
    SUBCASE("basic operations with float"){
        cells_index<float> idx(1.0f);
        
        idx.insert(vec3<float>(1.0f, 1.0f, 1.0f));
        idx.insert(vec3<float>(2.0f, 2.0f, 2.0f));
        idx.insert(vec3<float>(3.0f, 3.0f, 3.0f));
        
        REQUIRE(idx.get_size() == 3);
        
        cells_index<float>::bbox query_box(vec3<float>(0.0f, 0.0f, 0.0f), vec3<float>(2.5f, 2.5f, 2.5f));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("float with aux data"){
        cells_index<float> idx(1.0f);
        
        idx.insert(vec3<float>(1.0f, 1.0f, 1.0f), std::string("float_point"));
        
        cells_index<float>::bbox query_box(vec3<float>(0.0f, 0.0f, 0.0f), vec3<float>(2.0f, 2.0f, 2.0f));
        auto results = idx.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "float_point");
    }
}

