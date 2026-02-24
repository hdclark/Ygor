
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>

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
    
    SUBCASE("search by bounding box"){
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
    
    SUBCASE("nearest neighbors"){
        rtree<double> tree;
        tree.insert(vec3<double>(1.0, 1.0, 1.0));
        tree.insert(vec3<double>(2.0, 2.0, 2.0));
        tree.insert(vec3<double>(3.0, 3.0, 3.0));
        tree.insert(vec3<double>(10.0, 10.0, 10.0));
        
        vec3<double> query(0.0, 0.0, 0.0);
        auto results = tree.nearest_neighbors(query, 2);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0] == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(results[1] == vec3<double>(2.0, 2.0, 2.0));
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
}
