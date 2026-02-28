
#include <any>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

#include <YgorMath.h>
#include <YgorIndex.h>
#include <YgorIndexKDTree.h>

#include "doctest/doctest.h"


TEST_CASE( "kdtree basic construction" ){
    SUBCASE("default constructor"){
        kdtree<double> tree;
        REQUIRE(tree.get_size() == 0);
    }
}


TEST_CASE( "kdtree entry operations" ){
    SUBCASE("entry default constructor"){
        kdtree<double>::entry e;
        REQUIRE(e.point.x == 0.0);
        REQUIRE(e.point.y == 0.0);
        REQUIRE(e.point.z == 0.0);
        REQUIRE(e.aux_data.has_value() == false);
    }
    
    SUBCASE("entry with point only"){
        vec3<double> point(1.0, 2.0, 3.0);
        kdtree<double>::entry e(point);
        REQUIRE(e.point.x == 1.0);
        REQUIRE(e.point.y == 2.0);
        REQUIRE(e.point.z == 3.0);
        REQUIRE(e.aux_data.has_value() == false);
    }
    
    SUBCASE("entry with point and aux data"){
        vec3<double> point(1.0, 2.0, 3.0);
        int aux_value = 42;
        kdtree<double>::entry e(point, aux_value);
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
        
        kdtree<double>::entry e1(point1, 42);
        kdtree<double>::entry e2(point2, 100);
        kdtree<double>::entry e3(point3, 42);
        
        REQUIRE(e1 == e2);
        REQUIRE(!(e1 == e3));
    }
}

TEST_CASE( "kdtree bbox operations" ){
    SUBCASE("bbox construction"){
        kdtree<double>::bbox box1;
        REQUIRE(box1.min.x == 0.0);
        REQUIRE(box1.min.y == 0.0);
        REQUIRE(box1.min.z == 0.0);
        REQUIRE(box1.max.x == 0.0);
        REQUIRE(box1.max.y == 0.0);
        REQUIRE(box1.max.z == 0.0);
        
        vec3<double> min_corner(1.0, 2.0, 3.0);
        vec3<double> max_corner(4.0, 5.0, 6.0);
        kdtree<double>::bbox box2(min_corner, max_corner);
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
        kdtree<double>::bbox box(min_corner, max_corner);
        REQUIRE(box.volume() == 24.0);
    }
    
    SUBCASE("bbox surface area"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 3.0, 4.0);
        kdtree<double>::bbox box(min_corner, max_corner);
        REQUIRE(box.surface_area() == 52.0);
    }
    
    SUBCASE("bbox margin"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 3.0, 4.0);
        kdtree<double>::bbox box(min_corner, max_corner);
        REQUIRE(box.margin() == 9.0);
    }
    
    SUBCASE("bbox contains point"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 2.0, 2.0);
        kdtree<double>::bbox box(min_corner, max_corner);
        
        REQUIRE(box.contains(vec3<double>(1.0, 1.0, 1.0)) == true);
        REQUIRE(box.contains(vec3<double>(0.0, 0.0, 0.0)) == true);
        REQUIRE(box.contains(vec3<double>(2.0, 2.0, 2.0)) == true);
        REQUIRE(box.contains(vec3<double>(3.0, 1.0, 1.0)) == false);
        REQUIRE(box.contains(vec3<double>(1.0, 3.0, 1.0)) == false);
        REQUIRE(box.contains(vec3<double>(1.0, 1.0, 3.0)) == false);
    }
    
    SUBCASE("bbox intersects"){
        vec3<double> min1(0.0, 0.0, 0.0);
        vec3<double> max1(2.0, 2.0, 2.0);
        kdtree<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        kdtree<double>::bbox box2(min2, max2);
        
        vec3<double> min3(3.0, 3.0, 3.0);
        vec3<double> max3(4.0, 4.0, 4.0);
        kdtree<double>::bbox box3(min3, max3);
        
        REQUIRE(box1.intersects(box2) == true);
        REQUIRE(box2.intersects(box1) == true);
        REQUIRE(box1.intersects(box3) == false);
        REQUIRE(box3.intersects(box1) == false);
    }
    
    SUBCASE("bbox union"){
        vec3<double> min1(0.0, 0.0, 0.0);
        vec3<double> max1(2.0, 2.0, 2.0);
        kdtree<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        kdtree<double>::bbox box2(min2, max2);
        
        auto box_union = box1.union_with(box2);
        REQUIRE(box_union.min.x == 0.0);
        REQUIRE(box_union.min.y == 0.0);
        REQUIRE(box_union.min.z == 0.0);
        REQUIRE(box_union.max.x == 3.0);
        REQUIRE(box_union.max.y == 3.0);
        REQUIRE(box_union.max.z == 3.0);
    }
    
    SUBCASE("bbox expand with another box"){
        vec3<double> min1(0.0, 0.0, 0.0);
        vec3<double> max1(2.0, 2.0, 2.0);
        kdtree<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        kdtree<double>::bbox box2(min2, max2);
        
        box1.expand(box2);
        REQUIRE(box1.min.x == 0.0);
        REQUIRE(box1.min.y == 0.0);
        REQUIRE(box1.min.z == 0.0);
        REQUIRE(box1.max.x == 3.0);
        REQUIRE(box1.max.y == 3.0);
        REQUIRE(box1.max.z == 3.0);
    }
    
    SUBCASE("bbox expand with point"){
        vec3<double> min1(0.0, 0.0, 0.0);
        vec3<double> max1(2.0, 2.0, 2.0);
        kdtree<double>::bbox box(min1, max1);
        
        box.expand(vec3<double>(3.0, 3.0, 3.0));
        REQUIRE(box.min.x == 0.0);
        REQUIRE(box.min.y == 0.0);
        REQUIRE(box.min.z == 0.0);
        REQUIRE(box.max.x == 3.0);
        REQUIRE(box.max.y == 3.0);
        REQUIRE(box.max.z == 3.0);
    }
}

TEST_CASE( "kdtree insertion and search" ){
    SUBCASE("insert single point"){
        kdtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        tree.insert(point);
        
        REQUIRE(tree.get_size() == 1);
        REQUIRE(tree.contains(point) == true);
    }
    
    SUBCASE("insert multiple points"){
        kdtree<double> tree;
        std::vector<vec3<double>> points = {
            vec3<double>(1.0, 1.0, 1.0),
            vec3<double>(2.0, 2.0, 2.0),
            vec3<double>(3.0, 3.0, 3.0),
            vec3<double>(4.0, 4.0, 4.0),
            vec3<double>(5.0, 5.0, 5.0)
        };
        
        for(const auto& p : points) {
            tree.insert(p);
        }
        
        REQUIRE(tree.get_size() == 5);
        
        for(const auto& p : points) {
            REQUIRE(tree.contains(p) == true);
        }
    }

    SUBCASE("contains tolerates numerically equivalent floating-point queries"){
        kdtree<double> tree;
        const double stored = 0.1 + 0.2;
        tree.insert(vec3<double>(stored, stored, stored));

        REQUIRE(tree.contains(vec3<double>(stored, stored, stored)) == true);
        REQUIRE(tree.contains(vec3<double>(0.3, 0.3, 0.3)) == true);
    }
    
    SUBCASE("insert non-finite point throws"){
        kdtree<double> tree;
        REQUIRE_THROWS(tree.insert(vec3<double>(std::numeric_limits<double>::infinity(), 0.0, 0.0)));
        REQUIRE_THROWS(tree.insert(vec3<double>(0.0, std::numeric_limits<double>::quiet_NaN(), 0.0)));
    }
    
    SUBCASE("search by bounding box returns entries"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        tree.insert(vec3<double>(10.0, 10.0, 10.0));
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(3.0, 3.0, 3.0));
        auto results = tree.search(query_box);
        
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
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(3.0, 3.0, 3.0));
        auto results = tree.search_points(query_box);
        
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
        kdtree<double> tree;
        tree.insert(vec3<double>(0.0, 0.0, 0.0));
        tree.insert(vec3<double>(1.0, 0.0, 0.0));
        tree.insert(vec3<double>(0.0, 1.0, 0.0));
        tree.insert(vec3<double>(0.0, 0.0, 1.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = tree.search_radius(center, 1.5);
        
        REQUIRE(results.size() == 4);
    }
    
    SUBCASE("search_radius_points returns vec3 directly"){
        kdtree<double> tree;
        tree.insert(vec3<double>(0.0, 0.0, 0.0));
        tree.insert(vec3<double>(1.0, 0.0, 0.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = tree.search_radius_points(center, 1.5);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("nearest neighbors returns entries"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(3.0, 3.0, 3.0));
        tree.insert(vec3<double>(10.0, 10.0, 10.0));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = tree.nearest_neighbors(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].point == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(results[1].point == vec3<double>(2.0, 2.0, 2.0));
    }
    
    SUBCASE("nearest_neighbors_points returns vec3 directly"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(3.0, 3.0, 3.0));
        tree.insert(vec3<double>(10.0, 10.0, 10.0));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = tree.nearest_neighbors_points(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0] == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(results[1] == vec3<double>(2.0, 2.0, 2.0));
    }
    
    SUBCASE("nearest neighbors k=0 returns empty"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        
        auto results = tree.nearest_neighbors(vec3<double>(0.0, 0.0, 0.0), 0);
        REQUIRE(results.empty());
    }
    
    SUBCASE("nearest neighbors k > size returns all"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        
        auto results = tree.nearest_neighbors(vec3<double>(0.0, 0.0, 0.0), 10);
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("nearest neighbors on empty tree"){
        kdtree<double> tree;
        auto results = tree.nearest_neighbors(vec3<double>(0.0, 0.0, 0.0), 5);
        REQUIRE(results.empty());
    }
    
    SUBCASE("search on empty tree"){
        kdtree<double> tree;
        kdtree<double>::bbox query_box(vec3<double>(-1.0, -1.0, -1.0), vec3<double>(1.0, 1.0, 1.0));
        auto results = tree.search(query_box);
        REQUIRE(results.empty());
    }
    
    SUBCASE("contains on empty tree"){
        kdtree<double> tree;
        REQUIRE(tree.contains(vec3<double>(1.0, 1.0, 1.0)) == false);
    }
}

TEST_CASE( "kdtree auxiliary data" ){
    SUBCASE("insert with integer aux data"){
        kdtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        tree.insert(point, 42);
        
        REQUIRE(tree.get_size() == 1);
        REQUIRE(tree.contains(point) == true);
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].point == point);
        REQUIRE(results[0].aux_data.has_value() == true);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 42);
    }
    
    SUBCASE("insert with string aux data"){
        kdtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        std::string label = "test_point";
        tree.insert(point, label);
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "test_point");
    }
    
    SUBCASE("insert with struct aux data"){
        struct MyData {
            int id;
            double value;
        };
        
        kdtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        MyData data{123, 45.67};
        tree.insert(point, data);
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        auto retrieved = std::any_cast<MyData>(results[0].aux_data);
        REQUIRE(retrieved.id == 123);
        REQUIRE(retrieved.value == doctest::Approx(45.67));
    }
    
    SUBCASE("multiple points with different aux data types"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0), 42);
        tree.insert(vec3<double>(2.0, 2.0, 2.0), std::string("hello"));
        tree.insert(vec3<double>(3.0, 3.0, 3.0), 3.14159);
        tree.insert(vec3<double>(4.0, 4.0, 4.0));
        
        REQUIRE(tree.get_size() == 4);
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
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
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0), std::string("closest"));
        tree.insert(vec3<double>(2.0, 2.0, 2.0), std::string("second"));
        tree.insert(vec3<double>(10.0, 10.0, 10.0), std::string("far"));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = tree.nearest_neighbors(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "closest");
        REQUIRE(std::any_cast<std::string>(results[1].aux_data) == "second");
    }
    
    SUBCASE("search_radius preserves aux data"){
        kdtree<double> tree;
        tree.insert(vec3<double>(0.0, 0.0, 0.0), 100);
        tree.insert(vec3<double>(1.0, 0.0, 0.0), 200);
        tree.insert(vec3<double>(10.0, 10.0, 10.0), 999);
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = tree.search_radius(center, 2.0);
        
        REQUIRE(results.size() == 2);
        
        int sum = 0;
        for(const auto& r : results) {
            sum += std::any_cast<int>(r.aux_data);
        }
        REQUIRE(sum == 300);
    }
}

TEST_CASE( "kdtree clear" ){
    SUBCASE("clear removes all entries"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(3.0, 3.0, 3.0));
        
        REQUIRE(tree.get_size() == 3);
        
        tree.clear();
        
        REQUIRE(tree.get_size() == 0);
        REQUIRE(tree.contains(vec3<double>(1.0, 1.0, 1.0)) == false);
    }
    
    SUBCASE("clear removes aux data as well"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0), 42);
        tree.insert(vec3<double>(2.0, 2.0, 2.0), std::string("test"));
        
        REQUIRE(tree.get_size() == 2);
        
        tree.clear();
        
        REQUIRE(tree.get_size() == 0);
        
        kdtree<double>::bbox query_box(vec3<double>(-10.0, -10.0, -10.0), vec3<double>(10.0, 10.0, 10.0));
        auto results = tree.search(query_box);
        REQUIRE(results.size() == 0);
    }
    
    SUBCASE("insert after clear works correctly"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        
        tree.clear();
        REQUIRE(tree.get_size() == 0);
        
        tree.insert(vec3<double>(3.0, 3.0, 3.0));
        tree.insert(vec3<double>(4.0, 4.0, 4.0));
        
        REQUIRE(tree.get_size() == 2);
        REQUIRE(tree.contains(vec3<double>(3.0, 3.0, 3.0)) == true);
        REQUIRE(tree.contains(vec3<double>(4.0, 4.0, 4.0)) == true);
        REQUIRE(tree.contains(vec3<double>(1.0, 1.0, 1.0)) == false);
    }
}

TEST_CASE( "kdtree large dataset" ){
    SUBCASE("insert many points and verify"){
        kdtree<double> tree;
        std::vector<vec3<double>> points;
        
        for(int i = 0; i < 10; ++i) {
            for(int j = 0; j < 10; ++j) {
                for(int k = 0; k < 10; ++k) {
                    vec3<double> p(static_cast<double>(i), 
                                   static_cast<double>(j), 
                                   static_cast<double>(k));
                    points.push_back(p);
                    tree.insert(p);
                }
            }
        }
        
        REQUIRE(tree.get_size() == 1000);
        
        for(const auto& p : points) {
            REQUIRE(tree.contains(p) == true);
        }
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(2.0, 2.0, 2.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 27);
    }
    
    SUBCASE("large dataset with aux data"){
        kdtree<double> tree;
        
        for(int i = 0; i < 10; ++i) {
            for(int j = 0; j < 10; ++j) {
                for(int k = 0; k < 10; ++k) {
                    vec3<double> p(static_cast<double>(i), 
                                   static_cast<double>(j), 
                                   static_cast<double>(k));
                    int index = i * 100 + j * 10 + k;
                    tree.insert(p, index);
                }
            }
        }
        
        REQUIRE(tree.get_size() == 1000);
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(0.0, 0.0, 0.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 0);
        
        query_box = kdtree<double>::bbox(vec3<double>(5.0, 5.0, 5.0), vec3<double>(5.0, 5.0, 5.0));
        results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 555);
    }
    
    SUBCASE("large dataset nearest neighbors"){
        kdtree<double> tree;
        
        for(int i = 0; i < 10; ++i) {
            for(int j = 0; j < 10; ++j) {
                for(int k = 0; k < 10; ++k) {
                    vec3<double> p(static_cast<double>(i), 
                                   static_cast<double>(j), 
                                   static_cast<double>(k));
                    tree.insert(p);
                }
            }
        }
        
        REQUIRE(tree.get_size() == 1000);
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = tree.nearest_neighbors(query, 1);
        
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].point == vec3<double>(0.0, 0.0, 0.0));
    }
    
    SUBCASE("large dataset radius search"){
        kdtree<double> tree;
        
        for(int i = 0; i < 10; ++i) {
            for(int j = 0; j < 10; ++j) {
                for(int k = 0; k < 10; ++k) {
                    vec3<double> p(static_cast<double>(i), 
                                   static_cast<double>(j), 
                                   static_cast<double>(k));
                    tree.insert(p);
                }
            }
        }
        
        // Radius search from origin with radius 1.0
        // Points within distance 1.0: (0,0,0), (1,0,0), (0,1,0), (0,0,1)
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = tree.search_radius(center, 1.0);
        REQUIRE(results.size() == 4);
    }
}

TEST_CASE( "kdtree with negative coordinates" ){
    SUBCASE("insert and search with negative coordinates"){
        kdtree<double> tree;
        tree.insert(vec3<double>(-1.0, -2.0, -3.0));
        tree.insert(vec3<double>(-4.0, -5.0, -6.0));
        tree.insert(vec3<double>(1.0, 2.0, 3.0));
        
        REQUIRE(tree.get_size() == 3);
        
        kdtree<double>::bbox query_box(vec3<double>(-5.0, -6.0, -7.0), vec3<double>(0.0, 0.0, 0.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("nearest neighbors with negative coordinates"){
        kdtree<double> tree;
        tree.insert(vec3<double>(-1.0, -1.0, -1.0));
        tree.insert(vec3<double>(-5.0, -5.0, -5.0));
        tree.insert(vec3<double>(10.0, 10.0, 10.0));
        
        vec3<double> query(-2.0, -2.0, -2.0);
        auto results = tree.nearest_neighbors(query, 1);
        
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].point == vec3<double>(-1.0, -1.0, -1.0));
    }
}

TEST_CASE( "kdtree deferred build behavior" ){
    SUBCASE("multiple inserts then query triggers single build"){
        kdtree<double> tree;
        
        for(int i = 0; i < 100; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 0.0, 0.0));
        }
        
        REQUIRE(tree.get_size() == 100);
        
        // First query triggers the build.
        auto results = tree.nearest_neighbors(vec3<double>(50.0, 0.0, 0.0), 3);
        REQUIRE(results.size() == 3);
        REQUIRE(results[0].point == vec3<double>(50.0, 0.0, 0.0));
    }
    
    SUBCASE("insert after query triggers rebuild"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 0.0, 0.0));
        tree.insert(vec3<double>(2.0, 0.0, 0.0));
        
        // Query triggers build.
        REQUIRE(tree.contains(vec3<double>(1.0, 0.0, 0.0)) == true);
        
        // Insert more, which invalidates the tree.
        tree.insert(vec3<double>(3.0, 0.0, 0.0));
        
        // Next query triggers rebuild.
        REQUIRE(tree.contains(vec3<double>(3.0, 0.0, 0.0)) == true);
        REQUIRE(tree.get_size() == 3);
    }
}

TEST_CASE( "kdtree get_bounds" ){
    SUBCASE("bounds of single point"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 2.0, 3.0));
        
        auto b = tree.get_bounds();
        REQUIRE(b.min.x == 1.0);
        REQUIRE(b.min.y == 2.0);
        REQUIRE(b.min.z == 3.0);
        REQUIRE(b.max.x == 1.0);
        REQUIRE(b.max.y == 2.0);
        REQUIRE(b.max.z == 3.0);
    }
    
    SUBCASE("bounds of multiple points"){
        kdtree<double> tree;
        tree.insert(vec3<double>(1.0, 2.0, 3.0));
        tree.insert(vec3<double>(4.0, 5.0, 6.0));
        tree.insert(vec3<double>(-1.0, 0.0, 1.0));
        
        auto b = tree.get_bounds();
        REQUIRE(b.min.x == -1.0);
        REQUIRE(b.min.y == 0.0);
        REQUIRE(b.min.z == 1.0);
        REQUIRE(b.max.x == 4.0);
        REQUIRE(b.max.y == 5.0);
        REQUIRE(b.max.z == 6.0);
    }
}

TEST_CASE( "kdtree with float type" ){
    SUBCASE("basic operations with float"){
        kdtree<float> tree;
        
        tree.insert(vec3<float>(1.0f, 1.0f, 1.0f));
        tree.insert(vec3<float>(2.0f, 2.0f, 2.0f));
        tree.insert(vec3<float>(3.0f, 3.0f, 3.0f));
        
        REQUIRE(tree.get_size() == 3);
        
        kdtree<float>::bbox query_box(vec3<float>(0.0f, 0.0f, 0.0f), vec3<float>(2.5f, 2.5f, 2.5f));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("float with aux data"){
        kdtree<float> tree;
        
        tree.insert(vec3<float>(1.0f, 1.0f, 1.0f), std::string("float_point"));
        
        kdtree<float>::bbox query_box(vec3<float>(0.0f, 0.0f, 0.0f), vec3<float>(2.0f, 2.0f, 2.0f));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "float_point");
    }
    
    SUBCASE("float nearest neighbors"){
        kdtree<float> tree;
        tree.insert(vec3<float>(1.0f, 1.0f, 1.0f));
        tree.insert(vec3<float>(5.0f, 5.0f, 5.0f));
        tree.insert(vec3<float>(10.0f, 10.0f, 10.0f));
        
        auto results = tree.nearest_neighbors(vec3<float>(0.0f, 0.0f, 0.0f), 1);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].point == vec3<float>(1.0f, 1.0f, 1.0f));
    }
}

TEST_CASE( "kdtree smart pointer memory management" ){
    SUBCASE("tree destruction is clean"){
        for(int iter = 0; iter < 10; ++iter) {
            kdtree<double> tree;
            for(int i = 0; i < 100; ++i) {
                tree.insert(vec3<double>(static_cast<double>(i), 
                                         static_cast<double>(i * 2), 
                                         static_cast<double>(i * 3)), i);
            }
            REQUIRE(tree.get_size() == 100);
            // Force tree build by querying.
            auto results = tree.nearest_neighbors(vec3<double>(0.0, 0.0, 0.0), 1);
            REQUIRE(results.size() == 1);
        }
    }
    
    SUBCASE("clear does not cause double-free"){
        kdtree<double> tree;
        for(int i = 0; i < 50; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 0.0, 0.0));
        }
        
        // Force tree build.
        REQUIRE(tree.contains(vec3<double>(0.0, 0.0, 0.0)) == true);
        
        tree.clear();
        REQUIRE(tree.get_size() == 0);
        
        for(int i = 0; i < 30; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 1.0, 0.0));
        }
        
        REQUIRE(tree.get_size() == 30);
        
        tree.clear();
        REQUIRE(tree.get_size() == 0);
    }
}

TEST_CASE( "kdtree collinear points" ){
    SUBCASE("all points on x axis"){
        kdtree<double> tree;
        for(int i = 0; i < 20; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 0.0, 0.0), i);
        }
        
        REQUIRE(tree.get_size() == 20);
        
        for(int i = 0; i < 20; ++i) {
            REQUIRE(tree.contains(vec3<double>(static_cast<double>(i), 0.0, 0.0)) == true);
        }
        
        auto results = tree.nearest_neighbors(vec3<double>(5.0, 0.0, 0.0), 3);
        REQUIRE(results.size() == 3);
        REQUIRE(results[0].point == vec3<double>(5.0, 0.0, 0.0));
    }
    
    SUBCASE("all points at same location"){
        kdtree<double> tree;
        for(int i = 0; i < 10; ++i) {
            tree.insert(vec3<double>(1.0, 1.0, 1.0), i);
        }
        
        REQUIRE(tree.get_size() == 10);
        
        kdtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(2.0, 2.0, 2.0));
        auto results = tree.search(query_box);
        REQUIRE(results.size() == 10);
        
        auto nn = tree.nearest_neighbors(vec3<double>(0.0, 0.0, 0.0), 5);
        REQUIRE(nn.size() == 5);
    }
}

TEST_CASE( "kdtree correctness cross-check" ){
    SUBCASE("nearest neighbors matches brute force"){
        kdtree<double> tree;
        std::vector<vec3<double>> points;
        
        // Create a moderate set of scattered points.
        for(int i = 0; i < 50; ++i) {
            double x = static_cast<double>(i % 10) * 1.1;
            double y = static_cast<double>((i / 10) % 5) * 2.2;
            double z = static_cast<double>(i % 7) * 0.5;
            vec3<double> p(x, y, z);
            points.push_back(p);
            tree.insert(p);
        }
        
        vec3<double> query(3.3, 4.4, 1.5);
        size_t k = 5;
        
        // KD-tree result.
        auto kd_results = tree.nearest_neighbors(query, k);
        
        // Brute force result.
        std::vector<std::pair<double, vec3<double>>> brute;
        for(const auto& p : points) {
            double dist_sq = (p - query).sq_length();
            brute.emplace_back(dist_sq, p);
        }
        std::sort(brute.begin(), brute.end(),
                  [](const auto& a, const auto& b){ return a.first < b.first; });
        
        REQUIRE(kd_results.size() == k);
        for(size_t i = 0; i < k; ++i) {
            REQUIRE(kd_results[i].point == brute[i].second);
        }
    }
    
    SUBCASE("range search matches brute force"){
        kdtree<double> tree;
        std::vector<vec3<double>> points;
        
        for(int i = 0; i < 100; ++i) {
            double x = static_cast<double>(i % 10);
            double y = static_cast<double>((i / 10) % 10);
            double z = static_cast<double>(i % 3);
            vec3<double> p(x, y, z);
            points.push_back(p);
            tree.insert(p);
        }
        
        kdtree<double>::bbox query_box(vec3<double>(2.0, 3.0, 0.0), vec3<double>(7.0, 8.0, 2.0));
        
        // KD-tree result.
        auto kd_results = tree.search(query_box);
        
        // Brute force result.
        std::vector<vec3<double>> brute_results;
        for(const auto& p : points) {
            if(query_box.contains(p)){
                brute_results.push_back(p);
            }
        }
        
        REQUIRE(kd_results.size() == brute_results.size());
    }
}
