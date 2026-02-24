//YgorIndexRTree.h

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
// NOTE: This implementation uses modern C++17 features including smart pointers and std::any.
//
// NOTE: This implementation is templated to work with various spatial types (vec3, etc.).
//
template <class T> class rtree {
    public:
        using value_type = T;
        
        // An entry in the tree consisting of a spatial point and optional auxiliary data.
        struct entry {
            vec3<T> point;
            std::any aux_data;
            
            entry();
            entry(const vec3<T> &p);
            entry(const vec3<T> &p, std::any data);
            
            bool operator==(const entry &other) const;
        };
        
        // A bounding box in 3D space defined by min and max corners.
        struct bbox {
            vec3<T> min;
            vec3<T> max;
            
            bbox();
            bbox(const vec3<T> &min_corner, const vec3<T> &max_corner);
            
            // Compute the volume of the bounding box.
            T volume() const;
            
            // Compute the surface area of the bounding box.
            T surface_area() const;
            
            // Compute the margin (sum of edge lengths) of the bounding box.
            T margin() const;
            
            // Check if this bbox contains a point.
            bool contains(const vec3<T> &point) const;
            
            // Check if this bbox intersects another bbox.
            bool intersects(const bbox &other) const;
            
            // Compute the union of two bounding boxes.
            bbox union_with(const bbox &other) const;
            
            // Compute the intersection of two bounding boxes.
            bbox intersection_with(const bbox &other) const;
            
            // Compute the increase in volume if this bbox is expanded to include another.
            T volume_increase(const bbox &other) const;
            
            // Expand this bbox to include another bbox.
            void expand(const bbox &other);
            
            // Expand this bbox to include a point.
            void expand(const vec3<T> &point);
        };
        
        // Base class for all nodes in the tree.
        struct node_base;
        struct internal_node;
        struct leaf_node;
        
        struct node_base {
            bbox bounds;
            internal_node* parent;  // Raw pointer for parent (ownership is top-down)
            
            node_base();
            virtual ~node_base() = default;
            virtual bool is_leaf() const = 0;
        };
        
        // Internal node containing references to child nodes.
        struct internal_node : public node_base {
            std::vector<std::unique_ptr<node_base>> children;
            
            internal_node();
            bool is_leaf() const override;
        };
        
        // Leaf node containing actual data entries.
        struct leaf_node : public node_base {
            std::vector<entry> entries;
            
            leaf_node();
            bool is_leaf() const override;
        };
        
        //--------------------------------------------------- Data members -------------------------------------------------
        std::unique_ptr<node_base> root;
        size_t max_entries;  // Maximum number of entries per node (M).
        size_t min_entries;  // Minimum number of entries per node (m), typically M/2.
        size_t height;       // Height of the tree.
        size_t entry_count;  // Total number of entries in the tree.
        
        // R*-tree reinsertion parameters
        size_t reinsert_count;      // Number of entries to reinsert (typically 30% of max_entries).
        bool in_reinsertion;        // Flag to prevent recursive reinsertion.
        
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

