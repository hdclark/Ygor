
#include <any>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

#include <YgorMath.h>
#include <YgorIndexRTree.h>

#include "doctest/doctest.h"


TEST_CASE( "rtree basic construction" ){
    SUBCASE("default constructor"){
        rtree<double> tree;
        REQUIRE(tree.get_size() == 0);
        REQUIRE(tree.get_height() == 0);
    }
    
    SUBCASE("constructor with max entries"){
        rtree<double> tree(16);
        REQUIRE(tree.get_size() == 0);
        REQUIRE(tree.get_height() == 0);
    }
    
    SUBCASE("invalid max entries throws"){
        REQUIRE_THROWS(rtree<double>(1));
        REQUIRE_THROWS(rtree<double>(0));
    }
}

TEST_CASE( "rtree entry operations" ){
    SUBCASE("entry default constructor"){
        rtree<double>::entry e;
        REQUIRE(e.point.x == 0.0);
        REQUIRE(e.point.y == 0.0);
        REQUIRE(e.point.z == 0.0);
        REQUIRE(e.aux_data.has_value() == false);
    }
    
    SUBCASE("entry with point only"){
        vec3<double> point(1.0, 2.0, 3.0);
        rtree<double>::entry e(point);
        REQUIRE(e.point.x == 1.0);
        REQUIRE(e.point.y == 2.0);
        REQUIRE(e.point.z == 3.0);
        REQUIRE(e.aux_data.has_value() == false);
    }
    
    SUBCASE("entry with point and aux data"){
        vec3<double> point(1.0, 2.0, 3.0);
        int aux_value = 42;
        rtree<double>::entry e(point, aux_value);
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
        
        rtree<double>::entry e1(point1, 42);
        rtree<double>::entry e2(point2, 100);  // Different aux data
        rtree<double>::entry e3(point3, 42);    // Different point
        
        // Equality is based on point only, not aux data
        REQUIRE(e1 == e2);
        REQUIRE(!(e1 == e3));
    }
}

TEST_CASE( "rtree bbox operations" ){
    SUBCASE("bbox construction"){
        rtree<double>::bbox box1;
        REQUIRE(box1.min.x == 0.0);
        REQUIRE(box1.min.y == 0.0);
        REQUIRE(box1.min.z == 0.0);
        REQUIRE(box1.max.x == 0.0);
        REQUIRE(box1.max.y == 0.0);
        REQUIRE(box1.max.z == 0.0);
        
        vec3<double> min_corner(1.0, 2.0, 3.0);
        vec3<double> max_corner(4.0, 5.0, 6.0);
        rtree<double>::bbox box2(min_corner, max_corner);
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
        rtree<double>::bbox box(min_corner, max_corner);
        REQUIRE(box.volume() == 24.0);
    }
    
    SUBCASE("bbox surface area"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 3.0, 4.0);
        rtree<double>::bbox box(min_corner, max_corner);
        // Surface area = 2*(2*3 + 3*4 + 4*2) = 2*(6 + 12 + 8) = 52
        REQUIRE(box.surface_area() == 52.0);
    }
    
    SUBCASE("bbox margin"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 3.0, 4.0);
        rtree<double>::bbox box(min_corner, max_corner);
        // Margin = 2 + 3 + 4 = 9
        REQUIRE(box.margin() == 9.0);
    }
    
    SUBCASE("bbox contains point"){
        vec3<double> min_corner(0.0, 0.0, 0.0);
        vec3<double> max_corner(2.0, 2.0, 2.0);
        rtree<double>::bbox box(min_corner, max_corner);
        
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
        rtree<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        rtree<double>::bbox box2(min2, max2);
        
        vec3<double> min3(3.0, 3.0, 3.0);
        vec3<double> max3(4.0, 4.0, 4.0);
        rtree<double>::bbox box3(min3, max3);
        
        REQUIRE(box1.intersects(box2) == true);
        REQUIRE(box2.intersects(box1) == true);
        REQUIRE(box1.intersects(box3) == false);
        REQUIRE(box3.intersects(box1) == false);
    }
    
    SUBCASE("bbox union"){
        vec3<double> min1(0.0, 0.0, 0.0);
        vec3<double> max1(2.0, 2.0, 2.0);
        rtree<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        rtree<double>::bbox box2(min2, max2);
        
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
        rtree<double>::bbox box1(min1, max1);
        
        vec3<double> min2(1.0, 1.0, 1.0);
        vec3<double> max2(3.0, 3.0, 3.0);
        rtree<double>::bbox box2(min2, max2);
        
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
        rtree<double>::bbox box(min1, max1);
        
        box.expand(vec3<double>(3.0, 3.0, 3.0));
        REQUIRE(box.min.x == 0.0);
        REQUIRE(box.min.y == 0.0);
        REQUIRE(box.min.z == 0.0);
        REQUIRE(box.max.x == 3.0);
        REQUIRE(box.max.y == 3.0);
        REQUIRE(box.max.z == 3.0);
    }
}

TEST_CASE( "rtree insertion and search" ){
    SUBCASE("insert single point"){
        rtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        tree.insert(point);
        
        REQUIRE(tree.get_size() == 1);
        REQUIRE(tree.contains(point) == true);
    }
    
    SUBCASE("insert multiple points"){
        rtree<double> tree;
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
    
    SUBCASE("search by bounding box returns entries"){
        rtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        tree.insert(vec3<double>(10.0, 10.0, 10.0));
        
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(3.0, 3.0, 3.0));
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
        rtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(3.0, 3.0, 3.0));
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
        rtree<double> tree;
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
        rtree<double> tree;
        tree.insert(vec3<double>(0.0, 0.0, 0.0));
        tree.insert(vec3<double>(1.0, 0.0, 0.0));
        tree.insert(vec3<double>(5.0, 5.0, 5.0));
        
        vec3<double> center(0.0, 0.0, 0.0);
        auto results = tree.search_radius_points(center, 1.5);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("nearest neighbors returns entries"){
        rtree<double> tree;
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
        rtree<double> tree;
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
}

TEST_CASE( "rtree auxiliary data" ){
    SUBCASE("insert with integer aux data"){
        rtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        tree.insert(point, 42);
        
        REQUIRE(tree.get_size() == 1);
        REQUIRE(tree.contains(point) == true);
        
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].point == point);
        REQUIRE(results[0].aux_data.has_value() == true);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 42);
    }
    
    SUBCASE("insert with string aux data"){
        rtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        std::string label = "test_point";
        tree.insert(point, label);
        
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "test_point");
    }
    
    SUBCASE("insert with struct aux data"){
        struct MyData {
            int id;
            double value;
        };
        
        rtree<double> tree;
        vec3<double> point(1.0, 2.0, 3.0);
        MyData data{123, 45.67};
        tree.insert(point, data);
        
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        auto retrieved = std::any_cast<MyData>(results[0].aux_data);
        REQUIRE(retrieved.id == 123);
        REQUIRE(retrieved.value == doctest::Approx(45.67));
    }
    
    SUBCASE("multiple points with different aux data types"){
        rtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0), 42);
        tree.insert(vec3<double>(2.0, 2.0, 2.0), std::string("hello"));
        tree.insert(vec3<double>(3.0, 3.0, 3.0), 3.14159);
        tree.insert(vec3<double>(4.0, 4.0, 4.0));  // No aux data
        
        REQUIRE(tree.get_size() == 4);
        
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(5.0, 5.0, 5.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 4);
        
        // Find and verify each entry
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
        rtree<double> tree;
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
        rtree<double> tree;
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
        REQUIRE(sum == 300);  // 100 + 200
    }
}

TEST_CASE( "rtree clear" ){
    SUBCASE("clear removes all entries"){
        rtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(3.0, 3.0, 3.0));
        
        REQUIRE(tree.get_size() == 3);
        
        tree.clear();
        
        REQUIRE(tree.get_size() == 0);
        REQUIRE(tree.contains(vec3<double>(1.0, 1.0, 1.0)) == false);
    }
    
    SUBCASE("clear removes aux data as well"){
        rtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0), 42);
        tree.insert(vec3<double>(2.0, 2.0, 2.0), std::string("test"));
        
        REQUIRE(tree.get_size() == 2);
        
        tree.clear();
        
        REQUIRE(tree.get_size() == 0);
        
        rtree<double>::bbox query_box(vec3<double>(-10.0, -10.0, -10.0), vec3<double>(10.0, 10.0, 10.0));
        auto results = tree.search(query_box);
        REQUIRE(results.size() == 0);
    }
}

TEST_CASE( "rtree large dataset" ){
    SUBCASE("insert many points and verify"){
        rtree<double> tree;
        std::vector<vec3<double>> points;
        
        // Create a grid of points
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
        
        // Verify all points are in the tree
        for(const auto& p : points) {
            REQUIRE(tree.contains(p) == true);
        }
        
        // Search for a subset
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(2.0, 2.0, 2.0));
        auto results = tree.search(query_box);
        
        // Should find 3*3*3 = 27 points
        REQUIRE(results.size() == 27);
    }
    
    SUBCASE("large dataset with aux data"){
        rtree<double> tree;
        
        // Create a grid of points with index as aux data
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
        
        // Verify aux data is preserved
        rtree<double>::bbox query_box(vec3<double>(0.0, 0.0, 0.0), vec3<double>(0.0, 0.0, 0.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 0);  // Index 0*100 + 0*10 + 0 = 0
        
        // Check another point
        query_box = rtree<double>::bbox(vec3<double>(5.0, 5.0, 5.0), vec3<double>(5.0, 5.0, 5.0));
        results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<int>(results[0].aux_data) == 555);  // Index 5*100 + 5*10 + 5 = 555
    }
}

TEST_CASE( "rtree forced reinsertion (R*-tree behavior)" ){
    SUBCASE("forced reinsertion triggers on overflow"){
        // Use a small max_entries to force more splits/reinsertions
        rtree<double> tree(4);
        
        // Insert more points than can fit in a single node
        for(int i = 0; i < 20; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 0.0, 0.0), i);
        }
        
        REQUIRE(tree.get_size() == 20);
        
        // Verify all points are still findable
        for(int i = 0; i < 20; ++i) {
            REQUIRE(tree.contains(vec3<double>(static_cast<double>(i), 0.0, 0.0)) == true);
        }
    }
    
    SUBCASE("forced reinsertion preserves aux data"){
        rtree<double> tree(4);
        
        // Insert points that will trigger reinsertion
        for(int i = 0; i < 30; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 
                                     static_cast<double>(i), 
                                     static_cast<double>(i)), i * 10);
        }
        
        REQUIRE(tree.get_size() == 30);
        
        // Verify all aux data is preserved
        rtree<double>::bbox query_box(vec3<double>(-1.0, -1.0, -1.0), vec3<double>(100.0, 100.0, 100.0));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 30);
        
        std::vector<int> found_values;
        for(const auto& r : results) {
            found_values.push_back(std::any_cast<int>(r.aux_data));
        }
        std::sort(found_values.begin(), found_values.end());
        
        for(int i = 0; i < 30; ++i) {
            REQUIRE(found_values[static_cast<size_t>(i)] == i * 10);
        }
    }
    
    SUBCASE("tree structure remains valid after forced reinsertion"){
        rtree<double> tree(4);
        
        // Insert in a pattern that exercises reinsertion
        for(int i = 0; i < 50; ++i) {
            double x = static_cast<double>(i % 10);
            double y = static_cast<double>((i / 10) % 5);
            double z = static_cast<double>(i % 3);
            tree.insert(vec3<double>(x, y, z), i);
        }
        
        REQUIRE(tree.get_size() == 50);
        
        // Verify spatial queries still work correctly
        auto results = tree.nearest_neighbors(vec3<double>(5.0, 2.5, 1.5), 5);
        REQUIRE(results.size() == 5);
        
        // Verify radius search
        auto radius_results = tree.search_radius(vec3<double>(5.0, 2.0, 1.0), 2.0);
        REQUIRE(radius_results.size() > 0);
    }
}

TEST_CASE( "rtree with float type" ){
    SUBCASE("basic operations with float"){
        rtree<float> tree;
        
        tree.insert(vec3<float>(1.0f, 1.0f, 1.0f));
        tree.insert(vec3<float>(2.0f, 2.0f, 2.0f));
        tree.insert(vec3<float>(3.0f, 3.0f, 3.0f));
        
        REQUIRE(tree.get_size() == 3);
        
        rtree<float>::bbox query_box(vec3<float>(0.0f, 0.0f, 0.0f), vec3<float>(2.5f, 2.5f, 2.5f));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 2);
    }
    
    SUBCASE("float with aux data"){
        rtree<float> tree;
        
        tree.insert(vec3<float>(1.0f, 1.0f, 1.0f), std::string("float_point"));
        
        rtree<float>::bbox query_box(vec3<float>(0.0f, 0.0f, 0.0f), vec3<float>(2.0f, 2.0f, 2.0f));
        auto results = tree.search(query_box);
        
        REQUIRE(results.size() == 1);
        REQUIRE(std::any_cast<std::string>(results[0].aux_data) == "float_point");
    }
}

TEST_CASE( "rtree smart pointer memory management" ){
    SUBCASE("tree destruction is clean"){
        // Create and destroy a tree with many insertions
        // This test verifies no memory leaks with smart pointers
        for(int iter = 0; iter < 10; ++iter) {
            rtree<double> tree(4);
            for(int i = 0; i < 100; ++i) {
                tree.insert(vec3<double>(static_cast<double>(i), 
                                         static_cast<double>(i * 2), 
                                         static_cast<double>(i * 3)), i);
            }
            REQUIRE(tree.get_size() == 100);
            // Tree destroyed at end of scope - smart pointers clean up
        }
    }
    
    SUBCASE("clear does not cause double-free"){
        rtree<double> tree(4);
        for(int i = 0; i < 50; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 0.0, 0.0));
        }
        
        tree.clear();
        REQUIRE(tree.get_size() == 0);
        
        // Insert again after clear
        for(int i = 0; i < 30; ++i) {
            tree.insert(vec3<double>(static_cast<double>(i), 1.0, 0.0));
        }
        
        REQUIRE(tree.get_size() == 30);
        
        // Clear again
        tree.clear();
        REQUIRE(tree.get_size() == 0);
    }
}
