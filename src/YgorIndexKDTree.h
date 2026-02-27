//YgorIndexKDTree.h

#pragma once
#ifndef YGOR_INDEX_KDTREE_H_
#define YGOR_INDEX_KDTREE_H_

#include <stddef.h>
#include <any>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------- kdtree: kd-tree spatial indexing data structure ----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class implements a kd-tree for spatial indexing of objects in 3D space.
// The kd-tree recursively partitions space by cycling through the x, y, and z axes.
// At each level, the splitting axis alternates (depth % 3), and the median point
// along that axis is chosen as the partitioning element.
//
// The tree is built by bulk-loading: all points are inserted first, and the tree
// is constructed via a balanced median-split algorithm. Incremental insertions
// trigger a rebuild of the tree.
//
// The index supports efficient spatial queries such as:
//  - Range queries (find all objects within a region)
//  - Nearest neighbor queries
//  - Radius queries
//
// Users can optionally associate auxiliary data (via std::any) with each inserted point.
// This auxiliary data is not used during spatial queries but can be retrieved after lookups.
//
// Example usage:
//        kdtree<double> tree;
//        
//        tree.insert(vec3<double>(1.0, 2.0, 3.0), std::string("label_a"));
//        tree.insert(vec3<double>(4.0, 5.0, 6.0), 42);
//        
//        auto results = tree.search(query_box);
//        auto points = tree.search_points(query_box);
//

template <class T> class kdtree {
    public:
        using value_type = T;
        using entry = index_entry<T>;
        using bbox = index_bbox<T>;
        
    private:
        struct kdtree_node {
            entry data;
            int split_axis;  // 0 = x, 1 = y, 2 = z
            std::unique_ptr<kdtree_node> left;
            std::unique_ptr<kdtree_node> right;
            
            kdtree_node();
            explicit kdtree_node(const entry &e, int axis);
        };
        
        std::unique_ptr<kdtree_node> root;
        std::vector<entry> pending_entries;  // Entries waiting to be built into the tree.
        size_t entry_count;
        bbox bounds;
        bool bounds_initialized;
        bool tree_built;
        
        // Build a balanced kd-tree from a range of entries.
        std::unique_ptr<kdtree_node> build(std::vector<entry> &entries, size_t begin, size_t end, int depth);
        
        // Ensure the tree is built from pending entries.
        void ensure_built();
        
        // Recursively search for entries within a bounding box.
        void search_recursive(const kdtree_node* node, const bbox &query_box, std::vector<entry> &results) const;
        
        // Recursively search for nearest neighbors.
        void nearest_recursive(const kdtree_node* node, const vec3<T> &query_point, size_t k,
                               std::vector<std::pair<T, entry>> &best) const;
        
        // Get the coordinate of a point along a given axis.
        static T get_coord(const vec3<T> &point, int axis);
        
        // Compute the squared distance from a point to a bounding box.
        static T point_to_bbox_sq_dist(const vec3<T> &point, const bbox &box);
        
        // Update the overall bounding box.
        void update_bounds(const vec3<T> &point);
        
    public:
        //--------------------------------------------------- Constructors -------------------------------------------------
        kdtree();
        ~kdtree();
        
        // Delete copy constructor and assignment to prevent accidental copying.
        kdtree(const kdtree&) = delete;
        kdtree& operator=(const kdtree&) = delete;
        
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
        
        // Get the bounding box of all entries in the tree.
        bbox get_bounds() const;
};

#endif // YGOR_INDEX_KDTREE_H_

