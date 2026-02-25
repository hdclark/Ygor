//YgorIndexCells.h

#ifndef YGOR_INDEX_CELLS_H_
#define YGOR_INDEX_CELLS_H_

#include <stddef.h>
#include <any>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"


//---------------------------------------------------------------------------------------------------------------------------
//--------------------------- cells_index: cell-based spatial indexing data structure ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class implements a cell-based spatial index for objects in 3D space.
// Space is divided into uniform cubic cells of a user-specified width (cell_size).
// Each cell stores the entries that fall within it.
//
// The index supports efficient spatial queries such as:
//  - Range queries (find all objects within a region)
//  - Nearest neighbor queries (via local neighbourhood scan)
//  - Radius queries
//
// Users can optionally associate auxiliary data (via std::any) with each inserted point.
// This auxiliary data is not used during spatial queries but can be retrieved after lookups.
//
// Example usage:
//        cells_index<double> idx(1.0);  // cells of width 1.0
//        
//        idx.insert(vec3<double>(1.0, 2.0, 3.0), std::string("label_a"));
//        idx.insert(vec3<double>(4.0, 5.0, 6.0), 42);
//        
//        auto results = idx.search(query_box);
//        auto points = idx.search_points(query_box);
//

template <class T> class cells_index {
    public:
        using value_type = T;
        using entry = index_entry<T>;
        using bbox = index_bbox<T>;
        
        //--------------------------------------------------- Data members -------------------------------------------------
    private:
        struct cell_t {
            int64_t x;
            int64_t y;
            int64_t z;
            bool operator==(const cell_t &rhs) const {
                return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
            }
        };
        struct cell_hash_t {
            std::size_t operator()(const cell_t &c) const {
                const auto h0 = std::hash<int64_t>{}(c.x);
                const auto h1 = std::hash<int64_t>{}(c.y);
                const auto h2 = std::hash<int64_t>{}(c.z);
                return h0 ^ (h1 << 1U) ^ (h2 << 2U);
            }
        };
        
        T cell_size;
        T inv_cell_size;
        std::unordered_map<cell_t, std::vector<entry>, cell_hash_t> bins;
        size_t entry_count;
        bbox bounds;
        bool bounds_initialized;
        
        cell_t make_cell(const vec3<T> &v) const;
        void update_bounds(const vec3<T> &point);
        
    public:
        //--------------------------------------------------- Constructors -------------------------------------------------
        cells_index();
        explicit cells_index(T cell_size);
        ~cells_index();
        
        // Delete copy constructor and assignment to prevent accidental copying.
        cells_index(const cells_index&) = delete;
        cells_index& operator=(const cells_index&) = delete;
        
        //--------------------------------------------------- Member functions ---------------------------------------------
        
        // Insert a point into the index (without auxiliary data).
        void insert(const vec3<T> &point);
        
        // Insert a point with auxiliary data into the index.
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
        
        // Check if the index contains a specific point.
        bool contains(const vec3<T> &point) const;
        
        // Remove all entries from the index.
        void clear();
        
        // Get the number of entries in the index.
        size_t get_size() const;
        
        // Get the bounding box of all entries in the index.
        bbox get_bounds() const;
};

#endif // YGOR_INDEX_CELLS_H_

