//YgorIndexRTree.h

#ifndef YGOR_INDEX_RTREE_H_
#define YGOR_INDEX_RTREE_H_

#include <stddef.h>
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
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- rtree: R*-tree spatial indexing data structure -----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class implements an R*-tree-inspired algorithm for spatial indexing of objects in 3D space.
// The R*-tree is an enhancement of the R-tree that uses improved splitting algorithms
// to maintain better tree structure and improve query performance.
//
// The tree indexes objects by their bounding boxes and supports efficient spatial queries such as:
//  - Range queries (find all objects within a region)
//  - Nearest neighbor queries
//  - Intersection queries
//
// NOTE: This implementation uses axis-based splitting that minimizes area and margin, so it is not a *true* R*-tree.
//       In particular, it lacks forced reinsertion, which is a defining R*-tree optimization. So this class is effectively
//       an R-tree with axis-sorted splits. Performance characteristics remain O(log n) insertion/search, suitable for typical
//       spatial indexing workflows.
//
// NOTE: This implementation is templated to work with various spatial types (vec3, etc.).
//
// NOTE: Forced reinsertion is not currently implemented but may be added in the future.
//
template <class T> class rtree {
    public:
        using value_type = T;
        
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
        struct node_base {
            bbox bounds;
            node_base* parent;
            
            node_base();
            virtual ~node_base();
            virtual bool is_leaf() const = 0;
        };
        
        // Internal node containing references to child nodes.
        struct internal_node : public node_base {
            std::vector<node_base*> children;
            
            internal_node();
            ~internal_node() override;
            bool is_leaf() const override;
        };
        
        // Leaf node containing actual data entries.
        struct leaf_node : public node_base {
            std::vector<vec3<T>> entries;
            
            leaf_node();
            bool is_leaf() const override;
        };
        
        //--------------------------------------------------- Data members -------------------------------------------------
        node_base* root;
        size_t max_entries;  // Maximum number of entries per node (M).
        size_t min_entries;  // Minimum number of entries per node (m), typically M/2.
        size_t height;       // Height of the tree.
        size_t size;         // Total number of entries in the tree.
        
        //--------------------------------------------------- Constructors -------------------------------------------------
        rtree();
        rtree(size_t max_node_entries);
        ~rtree();
        
        // Delete copy constructor and assignment to prevent accidental copying.
        rtree(const rtree&) = delete;
        rtree& operator=(const rtree&) = delete;
        
        //--------------------------------------------------- Member functions ---------------------------------------------
        
        // Insert a point into the tree.
        void insert(const vec3<T> &point);
        
        // Search for all points within a bounding box.
        std::vector<vec3<T>> search(const bbox &query_box) const;
        
        // Search for all points within a given radius of a center point.
        std::vector<vec3<T>> search_radius(const vec3<T> &center, T radius) const;
        
        // Find the k nearest neighbors to a query point.
        std::vector<vec3<T>> nearest_neighbors(const vec3<T> &query_point, size_t k) const;
        
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
        
        // Choose the best leaf node to insert a new entry.
        leaf_node* choose_leaf(const vec3<T> &point);
        
        // Split a node that has overflowed.
        node_base* split_node(node_base* node);
        
        // Split an internal node.
        internal_node* split_internal_node(internal_node* node);
        
        // Split a leaf node.
        leaf_node* split_leaf_node(leaf_node* node);
        
        // Adjust the tree after insertion (propagate changes upward).
        void adjust_tree(node_base* node, node_base* split_node);
        
        // Recursively search for entries within a bounding box.
        void search_recursive(node_base* node, const bbox &query_box, std::vector<vec3<T>> &results) const;
        
        // Compute the axis along which to split (for R*-tree split algorithm).
        int choose_split_axis(const std::vector<bbox> &entries) const;
        
        // Compute the best split index along a given axis.
        size_t choose_split_index(const std::vector<bbox> &entries, int axis) const;
        
        // Recursively delete all nodes in the tree.
        void delete_tree(node_base* node);
        
        // Compute the height of a subtree.
        size_t compute_height(node_base* node) const;
};

#endif // YGOR_INDEX_RTREE_H_

