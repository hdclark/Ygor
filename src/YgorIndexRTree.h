//YgorIndexRTree.h

#pragma once
#ifndef YGOR_INDEX_RTREE_H_
#define YGOR_INDEX_RTREE_H_

#include <stddef.h>
#include <any>
#include <array>
#include <cmath>
#include <complex>
#include <optional>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- rtree: R*-tree spatial indexing data structure -----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class implements an R*-tree algorithm for spatial indexing of objects in 3D space.
// The R*-tree is an enhancement of the R-tree that uses improved splitting algorithms
// and forced reinsertion to maintain better tree structure and improve query performance.
//
// The tree indexes objects by their bounding boxes and supports efficient spatial queries such as:
//  - Range queries (find all objects within a region)
//  - Nearest neighbor queries
//  - Intersection queries
//
// Users can optionally associate auxiliary data (via std::any) with each inserted point.
// This auxiliary data is not used during spatial queries but can be retrieved after lookups.
//
// Example usage:
//        rtree<double> tree;
//        
//        // Insert with auxiliary data
//        tree.insert(vec3<double>(1.0, 2.0, 3.0), std::string("label_a"));
//        tree.insert(vec3<double>(4.0, 5.0, 6.0), 42);
//        
//        // Search returns entries with aux data preserved
//        auto results = tree.search(query_box);
//        for (const auto& e : results) {
//            std::cout << e.point << " -> " << std::any_cast<std::string>(e.aux_data);
//        }
//        
//        // Or use points-only methods
//        auto points = tree.search_points(query_box);  // std::vector<vec3<T>>
//    

template <class T> class rtree {
    public:
        using value_type = T;
        using entry = index_entry<T>;
        using bbox = index_bbox<T>;
        using node_base = index_node_base<T>;
        using internal_node = index_internal_node<T>;
        using leaf_node = index_leaf_node<T>;
        
        //--------------------------------------------------- Data members -------------------------------------------------
        std::unique_ptr<node_base> root;
        size_t max_entries;  // Maximum number of entries per node (M).
        size_t min_entries;  // Minimum number of entries per node (m), typically M/2.
        size_t height;       // Height of the tree.
        size_t entry_count;  // Total number of entries in the tree.
        
        // R*-tree reinsertion parameters
        size_t reinsert_count;      // Number of entries to reinsert (typically 30% of max_entries).
        bool in_reinsertion;        // Flag to prevent recursive reinsertion within a single insert operation.
                                    // Reset to false at the start of each top-level insert() call.
                                    // This simplified implementation performs reinsertion only for leaf nodes.
        
        //--------------------------------------------------- Constructors -------------------------------------------------
        rtree();
        rtree(size_t max_node_entries);
        ~rtree();
        
        // Delete copy constructor and assignment to prevent accidental copying.
        rtree(const rtree&) = delete;
        rtree& operator=(const rtree&) = delete;
        
        //--------------------------------------------------- Member functions ---------------------------------------------
        
        // Insert a point into the tree (without auxiliary data).
        void insert(const vec3<T> &point);
        
        // Insert a point with auxiliary data into the tree.
        void insert(const vec3<T> &point, std::any aux_data);
        
        // Search for all entries within a bounding box.
        std::vector<entry> search(const bbox &query_box) const;
        
        // Search for all points within a bounding box (returns points only, no aux data).
        std::vector<vec3<T>> search_points(const bbox &query_box) const;
        
        // Search for all entries within a given radius of a center point.
        std::vector<entry> search_radius(const vec3<T> &center, T radius) const;
        
        // Search for all points within a given radius of a center point (returns points only).
        std::vector<vec3<T>> search_radius_points(const vec3<T> &center, T radius) const;
        
        // Find the k nearest neighbor entries to a query point.
        std::vector<entry> nearest_neighbors(const vec3<T> &query_point, size_t k) const;
        
        // Find the k nearest neighbor points to a query point (returns points only).
        std::vector<vec3<T>> nearest_neighbors_points(const vec3<T> &query_point, size_t k) const;
        
        // Check if the tree contains a specific point.
        bool contains(const vec3<T> &point) const;
        
        // Remove all entries from the tree.
        void clear();
        
        // Get the number of entries in the tree.
        size_t get_size() const;
        
        // Get the height of the tree.
        size_t get_height() const;
        
        // Get the bounding box of all entries in the tree.
        bbox get_bounds() const;
        
    private:
        //--------------------------------------------------- Helper functions ---------------------------------------------
        
        // Insert an entry at the leaf level (main insertion).
        void insert_entry_at_leaf(const entry &e);
        
        // Choose the best leaf node to insert a new entry.
        leaf_node* choose_leaf(const vec3<T> &point);
        
        // Choose the best subtree for insertion (used for internal nodes).
        node_base* choose_subtree(node_base* node, const bbox &entry_box, size_t target_level, size_t current_level);
        
        // Handle overflow at a node (using R*-tree forced reinsertion or split).
        // Returns a new split node if split occurred, nullptr otherwise.
        std::unique_ptr<node_base> overflow_treatment(node_base* node, bool is_root);
        
        // Perform forced reinsertion from an overflowing leaf node.
        void reinsert_leaf(leaf_node* node);
        
        // Split an internal node.
        std::unique_ptr<internal_node> split_internal_node(internal_node* node);
        
        // Split a leaf node.
        std::unique_ptr<leaf_node> split_leaf_node(leaf_node* node);
        
        // Adjust the tree after insertion (propagate changes upward).
        void adjust_tree(node_base* node, std::unique_ptr<node_base> split_node);
        
        // Recursively search for entries within a bounding box.
        void search_recursive(const node_base* node, const bbox &query_box, std::vector<entry> &results) const;
        
        // Compute the axis along which to split (for R*-tree split algorithm).
        int choose_split_axis(const std::vector<bbox> &entries) const;
        
        // Compute the best split index along a given axis.
        size_t choose_split_index(const std::vector<bbox> &entries, int axis) const;
        
        // Update the bounding box of a node based on its contents.
        void update_bounds(node_base* node);
        
        // Compute the height of a subtree.
        size_t compute_height(const node_base* node) const;
        
        // Compute the center of a bounding box.
        vec3<T> bbox_center(const bbox &b) const;
};

#endif // YGOR_INDEX_RTREE_H_

