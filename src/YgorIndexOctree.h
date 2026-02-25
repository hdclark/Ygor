//YgorIndexOctree.h

#ifndef YGOR_INDEX_OCTREE_H_
#define YGOR_INDEX_OCTREE_H_

#include <stddef.h>
#include <any>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------- octree: octree spatial indexing data structure -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class implements an octree for spatial indexing of objects in 3D space.
// The octree recursively divides space into eight octants.
// Each node either stores entries directly (leaf) or delegates to its eight children (internal).
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
//        octree<double> tree;
//        
//        tree.insert(vec3<double>(1.0, 2.0, 3.0), std::string("label_a"));
//        tree.insert(vec3<double>(4.0, 5.0, 6.0), 42);
//        
//        auto results = tree.search(query_box);
//        auto points = tree.search_points(query_box);
//

template <class T> class octree {
    public:
        using value_type = T;
        using entry = index_entry<T>;
        using bbox = index_bbox<T>;
        
    private:
        struct octree_node {
            bbox bounds;
            std::vector<entry> entries;
            std::array<std::unique_ptr<octree_node>, 8> children;
            bool is_leaf;
            
            octree_node();
            explicit octree_node(const bbox &b);
        };
        
        std::unique_ptr<octree_node> root;
        size_t max_entries_per_node;
        size_t max_depth;
        size_t entry_count;
        bbox bounds;
        bool bounds_initialized;
        
        // Determine which octant a point falls into within a given bounding box.
        int get_octant(const bbox &node_bounds, const vec3<T> &point) const;
        
        // Compute the bounding box for a specific octant of a parent box.
        bbox get_octant_bounds(const bbox &parent_bounds, int octant) const;
        
        // Insert an entry into a node, recursively subdividing as needed.
        void insert_into_node(octree_node* node, const entry &e, size_t depth);
        
        // Subdivide a leaf node into eight children.
        void subdivide(octree_node* node, size_t depth);
        
        // Search recursively for entries within a bounding box.
        void search_recursive(const octree_node* node, const bbox &query_box, std::vector<entry> &results) const;
        
        // Collect all entries from a node and its descendants.
        void collect_all(const octree_node* node, std::vector<std::pair<T, entry>> &results, const vec3<T> &query_point) const;
        
        // Update the overall bounding box.
        void update_bounds(const vec3<T> &point);
        
    public:
        //--------------------------------------------------- Constructors -------------------------------------------------
        octree();
        explicit octree(size_t max_entries);
        octree(size_t max_entries, size_t max_tree_depth);
        ~octree();
        
        // Delete copy constructor and assignment to prevent accidental copying.
        octree(const octree&) = delete;
        octree& operator=(const octree&) = delete;
        
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

#endif // YGOR_INDEX_OCTREE_H_

