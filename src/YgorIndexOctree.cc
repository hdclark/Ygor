//YgorIndexOctree.cc.

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"
#include "YgorIndexOctree.h"

//#ifndef YGOR_INDEX_OCTREE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_OCTREE_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------- octree: octree spatial indexing data structure -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ octree_node --------------------------------------------------------

template <class T>
octree<T>::octree_node::octree_node() : is_leaf(true) { }

template <class T>
octree<T>::octree_node::octree_node(const bbox &b) : bounds(b), is_leaf(true) { }

//------------------------------------------------------ Private helpers ----------------------------------------------------

template <class T>
int octree<T>::get_octant(const bbox &node_bounds, const vec3<T> &point) const {
    const auto mid_x = (node_bounds.min.x + node_bounds.max.x) / static_cast<T>(2);
    const auto mid_y = (node_bounds.min.y + node_bounds.max.y) / static_cast<T>(2);
    const auto mid_z = (node_bounds.min.z + node_bounds.max.z) / static_cast<T>(2);
    
    int octant = 0;
    if(point.x >= mid_x) octant |= 1;
    if(point.y >= mid_y) octant |= 2;
    if(point.z >= mid_z) octant |= 4;
    return octant;
}

template <class T>
typename octree<T>::bbox octree<T>::get_octant_bounds(const bbox &parent_bounds, int octant) const {
    const auto mid_x = (parent_bounds.min.x + parent_bounds.max.x) / static_cast<T>(2);
    const auto mid_y = (parent_bounds.min.y + parent_bounds.max.y) / static_cast<T>(2);
    const auto mid_z = (parent_bounds.min.z + parent_bounds.max.z) / static_cast<T>(2);
    
    vec3<T> new_min, new_max;
    
    new_min.x = (octant & 1) ? mid_x : parent_bounds.min.x;
    new_max.x = (octant & 1) ? parent_bounds.max.x : mid_x;
    
    new_min.y = (octant & 2) ? mid_y : parent_bounds.min.y;
    new_max.y = (octant & 2) ? parent_bounds.max.y : mid_y;
    
    new_min.z = (octant & 4) ? mid_z : parent_bounds.min.z;
    new_max.z = (octant & 4) ? parent_bounds.max.z : mid_z;
    
    return bbox(new_min, new_max);
}

template <class T>
void octree<T>::subdivide(octree_node* node, size_t depth) {
    node->is_leaf = false;
    
    for(int i = 0; i < 8; ++i){
        auto child_bounds = get_octant_bounds(node->bounds, i);
        node->children[i] = std::make_unique<octree_node>(child_bounds);
    }
    
    // Re-insert existing entries into children.
    auto old_entries = std::move(node->entries);
    node->entries.clear();
    
    for(auto &e : old_entries){
        int octant = get_octant(node->bounds, e.point);
        insert_into_node(node->children[octant].get(), e, depth + 1);
    }
}

template <class T>
void octree<T>::insert_into_node(octree_node* node, const entry &e, size_t depth) {
    if(node->is_leaf){
        node->entries.push_back(e);
        
        if(node->entries.size() > max_entries_per_node && depth < max_depth){
            subdivide(node, depth);
        }
    }else{
        int octant = get_octant(node->bounds, e.point);
        insert_into_node(node->children[octant].get(), e, depth + 1);
    }
}

template <class T>
void octree<T>::search_recursive(const octree_node* node, const bbox &query_box, std::vector<entry> &results) const {
    if(node == nullptr) return;
    
    if(!node->bounds.intersects(query_box)) return;
    
    if(node->is_leaf){
        for(const auto &e : node->entries){
            if(query_box.contains(e.point)){
                results.push_back(e);
            }
        }
    }else{
        for(int i = 0; i < 8; ++i){
            if(node->children[i] != nullptr){
                search_recursive(node->children[i].get(), query_box, results);
            }
        }
    }
}

template <class T>
void octree<T>::collect_all(const octree_node* node, std::vector<std::pair<T, entry>> &results, const vec3<T> &query_point) const {
    if(node == nullptr) return;
    
    if(node->is_leaf){
        for(const auto &e : node->entries){
            T dist_sq = (e.point - query_point).sq_length();
            results.emplace_back(dist_sq, e);
        }
    }else{
        for(int i = 0; i < 8; ++i){
            if(node->children[i] != nullptr){
                collect_all(node->children[i].get(), results, query_point);
            }
        }
    }
}

template <class T>
void octree<T>::update_bounds(const vec3<T> &point) {
    if(!bounds_initialized){
        bounds = bbox(point, point);
        bounds_initialized = true;
    }else{
        bounds.expand(point);
    }
}

//------------------------------------------------------ Constructors -------------------------------------------------------

template <class T>
octree<T>::octree() : max_entries_per_node(8), max_depth(21), entry_count(0),
                      bounds_initialized(false) { }

template <class T>
octree<T>::octree(size_t max_entries) : max_entries_per_node(max_entries), max_depth(21),
                                        entry_count(0), bounds_initialized(false) {
    if(max_entries_per_node < 1){
        throw std::invalid_argument("Maximum entries per node must be at least 1");
    }
}

template <class T>
octree<T>::octree(size_t max_entries, size_t max_tree_depth)
    : max_entries_per_node(max_entries), max_depth(max_tree_depth),
      entry_count(0), bounds_initialized(false) {
    if(max_entries_per_node < 1){
        throw std::invalid_argument("Maximum entries per node must be at least 1");
    }
}

template <class T>
octree<T>::~octree() = default;

//------------------------------------------------------ Member functions ---------------------------------------------------

template <class T>
void octree<T>::insert(const vec3<T> &point) {
    insert(point, std::any{});
}

template <class T>
void octree<T>::insert(const vec3<T> &point, std::any aux_data) {
    entry e(point, std::move(aux_data));
    
    update_bounds(point);
    
    if(root == nullptr){
        // Create initial root with a generous bounding box centred on the first point.
        const T initial_half = static_cast<T>(1024);
        vec3<T> half(initial_half, initial_half, initial_half);
        root = std::make_unique<octree_node>(bbox(point - half, point + half));
    }else{
        // If the point is outside the current root bounds, expand the root.
        while(!root->bounds.contains(point)){
            // Double the root bounding box in the direction of the new point.
            auto new_root = std::make_unique<octree_node>();
            
            vec3<T> new_min = root->bounds.min;
            vec3<T> new_max = root->bounds.max;
            const auto dx = new_max.x - new_min.x;
            const auto dy = new_max.y - new_min.y;
            const auto dz = new_max.z - new_min.z;
            
            if(point.x < new_min.x) new_min.x -= dx;
            else new_max.x += dx;
            
            if(point.y < new_min.y) new_min.y -= dy;
            else new_max.y += dy;
            
            if(point.z < new_min.z) new_min.z -= dz;
            else new_max.z += dz;
            
            new_root->bounds = bbox(new_min, new_max);
            new_root->is_leaf = false;
            
            // Place old root as a child of the new root.
            int old_octant = get_octant(new_root->bounds, 
                vec3<T>((root->bounds.min.x + root->bounds.max.x) / static_cast<T>(2),
                         (root->bounds.min.y + root->bounds.max.y) / static_cast<T>(2),
                         (root->bounds.min.z + root->bounds.max.z) / static_cast<T>(2)));
            
            // Create child nodes for new root.
            for(int i = 0; i < 8; ++i){
                if(i == old_octant){
                    new_root->children[i] = std::move(root);
                }else{
                    auto child_bounds = get_octant_bounds(new_root->bounds, i);
                    new_root->children[i] = std::make_unique<octree_node>(child_bounds);
                }
            }
            
            root = std::move(new_root);
        }
    }
    
    insert_into_node(root.get(), e, 0);
    ++entry_count;
}

template <class T>
std::vector<typename octree<T>::entry> octree<T>::search(const bbox &query_box) const {
    std::vector<entry> results;
    if(root != nullptr){
        search_recursive(root.get(), query_box, results);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> octree<T>::search_points(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename octree<T>::entry> octree<T>::search_radius(const vec3<T> &center, T radius) const {
    vec3<T> offset(radius, radius, radius);
    bbox query_box(center - offset, center + offset);
    
    auto candidates = search(query_box);
    
    std::vector<entry> results;
    T radius_sq = radius * radius;
    for(const auto& e : candidates){
        if((e.point - center).sq_length() <= radius_sq){
            results.push_back(e);
        }
    }
    return results;
}

template <class T>
std::vector<vec3<T>> octree<T>::search_radius_points(const vec3<T> &center, T radius) const {
    auto entries = search_radius(center, radius);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename octree<T>::entry> octree<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    std::vector<std::pair<T, entry>> all_entries;

    if(root != nullptr){
        collect_all(root.get(), all_entries, query_point);
    }

    std::vector<entry> results;
    if(k == 0 || all_entries.empty()){
        return results;
    }

    const auto cmp = [](const auto &a, const auto &b){
        return a.first < b.first;
    };

    const size_t count = std::min(k, all_entries.size());

    // If we have more entries than needed, use nth_element to select the k closest
    if(all_entries.size() > count){
        std::nth_element(all_entries.begin(), all_entries.begin() + static_cast<std::ptrdiff_t>(count), all_entries.end(), cmp);
        all_entries.resize(count);
    }

    // Ensure the returned neighbors are ordered by increasing distance
    std::sort(all_entries.begin(), all_entries.end(), cmp);

    results.reserve(all_entries.size());
    for(const auto &pair : all_entries){
        results.push_back(pair.second);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> octree<T>::nearest_neighbors_points(const vec3<T> &query_point, size_t k) const {
    auto entries = nearest_neighbors(query_point, k);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
bool octree<T>::contains(const vec3<T> &point) const {
    if(root == nullptr) return false;
    
    bbox point_box(point, point);
    auto results = search(point_box);
    
    const T epsilon = std::numeric_limits<T>::epsilon() * static_cast<T>(10);
    
    for(const auto& result : results){
        if(std::abs(result.point.x - point.x) <= epsilon &&
           std::abs(result.point.y - point.y) <= epsilon &&
           std::abs(result.point.z - point.z) <= epsilon){
            return true;
        }
    }
    return false;
}

template <class T>
void octree<T>::clear() {
    root.reset();
    entry_count = 0;
    bounds_initialized = false;
    bounds = bbox();
}

template <class T>
size_t octree<T>::get_size() const {
    return entry_count;
}

template <class T>
typename octree<T>::bbox octree<T>::get_bounds() const {
    return bounds;
}

#ifndef YGOR_INDEX_OCTREE_DISABLE_ALL_SPECIALIZATIONS
    template class octree<float>;
    template class octree<double>;
#endif

