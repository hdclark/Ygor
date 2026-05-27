//YgorIndexOctree.cc.

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
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
T octree<T>::point_to_bbox_sq_dist(const vec3<T> &point, const bbox &box) {
    return box.squared_distance_to(point);
}

template <class T>
int octree<T>::get_octant(const bbox &node_bounds, const vec3<T> &point) const {
    const auto mid = node_bounds.center();

    int octant = 0;
    if(point.x >= mid.x) octant |= 1;
    if(point.y >= mid.y) octant |= 2;
    if(point.z >= mid.z) octant |= 4;
    return octant;
}

template <class T>
std::optional<int> octree<T>::get_octant(const bbox &node_bounds, const bbox &entry_box) const {
    const auto mid = node_bounds.center();

    const bool fits_left_x = entry_box.max.x <= mid.x;
    const bool fits_right_x = entry_box.min.x >= mid.x;
    if(fits_left_x == fits_right_x) return std::nullopt;

    const bool fits_left_y = entry_box.max.y <= mid.y;
    const bool fits_right_y = entry_box.min.y >= mid.y;
    if(fits_left_y == fits_right_y) return std::nullopt;

    const bool fits_left_z = entry_box.max.z <= mid.z;
    const bool fits_right_z = entry_box.min.z >= mid.z;
    if(fits_left_z == fits_right_z) return std::nullopt;

    int octant = 0;
    if(fits_right_x) octant |= 1;
    if(fits_right_y) octant |= 2;
    if(fits_right_z) octant |= 4;
    return octant;
}

template <class T>
typename octree<T>::bbox octree<T>::get_octant_bounds(const bbox &parent_bounds, int octant) const {
    const auto mid = parent_bounds.center();

    vec3<T> new_min, new_max;

    new_min.x = (octant & 1) ? mid.x : parent_bounds.min.x;
    new_max.x = (octant & 1) ? parent_bounds.max.x : mid.x;

    new_min.y = (octant & 2) ? mid.y : parent_bounds.min.y;
    new_max.y = (octant & 2) ? parent_bounds.max.y : mid.y;

    new_min.z = (octant & 4) ? mid.z : parent_bounds.min.z;
    new_max.z = (octant & 4) ? parent_bounds.max.z : mid.z;

    return bbox(new_min, new_max);
}

template <class T>
void octree<T>::subdivide(octree_node* node, size_t depth) {
    node->is_leaf = false;

    for(int i = 0; i < 8; ++i){
        auto child_bounds = get_octant_bounds(node->bounds, i);
        node->children[i] = std::make_unique<octree_node>(child_bounds);
    }

    auto old_entries = std::move(node->entries);
    node->entries.clear();

    for(auto &e : old_entries){
        if(const auto octant = get_octant(node->bounds, e.box); octant.has_value()){
            insert_into_node(node->children[*octant].get(), e, depth + 1);
        }else{
            node->entries.push_back(e);
        }
    }
}

template <class T>
void octree<T>::insert_into_node(octree_node* node, const entry &e, size_t depth) {
    if(node->is_leaf){
        node->entries.push_back(e);

        if(node->entries.size() > max_entries_per_node && depth < max_depth){
            subdivide(node, depth);
        }
        return;
    }

    if(const auto octant = get_octant(node->bounds, e.box); octant.has_value()){
        insert_into_node(node->children[*octant].get(), e, depth + 1);
    }else{
        node->entries.push_back(e);
    }
}

template <class T>
void octree<T>::search_recursive(const octree_node* node, const bbox &query_box, std::vector<entry> &results) const {
    if((node == nullptr) || !node->bounds.intersects(query_box)) return;

    for(const auto &e : node->entries){
        if(e.box.intersects(query_box)){
            results.push_back(e);
        }
    }

    if(node->is_leaf) return;

    for(int i = 0; i < 8; ++i){
        if(node->children[i] != nullptr){
            search_recursive(node->children[i].get(), query_box, results);
        }
    }
}

template <class T>
void octree<T>::collect_all(const octree_node* node, std::vector<std::pair<T, entry>> &results, const vec3<T> &query_point) const {
    if(node == nullptr) return;

    for(const auto &e : node->entries){
        const T dist_sq = point_to_bbox_sq_dist(query_point, e.box);
        results.emplace_back(dist_sq, e);
    }

    if(node->is_leaf) return;

    for(int i = 0; i < 8; ++i){
        if(node->children[i] != nullptr){
            collect_all(node->children[i].get(), results, query_point);
        }
    }
}

template <class T>
void octree<T>::update_bounds(const vec3<T> &point) {
    update_bounds(bbox(point, point));
}

template <class T>
void octree<T>::update_bounds(const bbox &bb) {
    if(!bounds_initialized){
        bounds = bb;
        bounds_initialized = true;
    }else{
        bounds.expand(bb);
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
    insert(bbox(point, point), std::move(aux_data));
}

template <class T>
void octree<T>::insert(const bbox &bb) {
    insert(bb, std::any{});
}

template <class T>
void octree<T>::insert(const bbox &bb, std::any aux_data) {
    if(!bb.isfinite()){
        throw std::invalid_argument("Cannot insert non-finite bbox into octree");
    }

    entry e(bb, std::move(aux_data));

    update_bounds(bb);

    if(root == nullptr){
        const auto center = bb.center();
        const auto extent = bb.max - bb.min;
        T initial_half = std::max({ std::abs(center.x), std::abs(center.y), std::abs(center.z),
                                    extent.x, extent.y, extent.z, static_cast<T>(1) });
        initial_half *= static_cast<T>(2);
        vec3<T> half(initial_half, initial_half, initial_half);
        root = std::make_unique<octree_node>(bbox(center - half, center + half));
    }else{
        constexpr int max_expansion_iters = 128;
        int expansion_iters = 0;
        while(!root->bounds.contains(bb)){
            if(++expansion_iters > max_expansion_iters){
                throw std::runtime_error("Octree root expansion exceeded maximum iterations");
            }

            auto new_root = std::make_unique<octree_node>();

            vec3<T> new_min = root->bounds.min;
            vec3<T> new_max = root->bounds.max;
            const auto root_center = root->bounds.center();
            const auto bb_center = bb.center();
            const auto dx = new_max.x - new_min.x;
            const auto dy = new_max.y - new_min.y;
            const auto dz = new_max.z - new_min.z;

            if(bb_center.x < root_center.x) new_min.x -= dx;
            else new_max.x += dx;

            if(bb_center.y < root_center.y) new_min.y -= dy;
            else new_max.y += dy;

            if(bb_center.z < root_center.z) new_min.z -= dz;
            else new_max.z += dz;

            new_root->bounds = bbox(new_min, new_max);
            new_root->is_leaf = false;

            const auto old_octant = get_octant(new_root->bounds, root->bounds);
            if(!old_octant.has_value()){
                throw std::runtime_error("Unable to place octree root during expansion");
            }

            for(int i = 0; i < 8; ++i){
                if(i == *old_octant){
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
        if(!e.box.has_extent()){
            results.push_back(e.box.min);
        }
    }
    return results;
}

template <class T>
std::vector<typename octree<T>::bbox> octree<T>::search_bboxes(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<bbox> results;
    results.reserve(entries.size());
    for(const auto &e : entries){
        results.push_back(e.box);
    }
    return results;
}

template <class T>
std::vector<typename octree<T>::bbox> octree<T>::search_bboxes(const vec3<T> &query_point) const {
    auto entries = search(bbox(query_point, query_point));
    std::vector<bbox> results;
    results.reserve(entries.size());
    for(const auto &e : entries){
        if(e.box.contains(query_point)){
            results.push_back(e.box);
        }
    }
    return results;
}

template <class T>
std::vector<typename octree<T>::entry> octree<T>::search_radius(const vec3<T> &center, T radius) const {
    vec3<T> offset(radius, radius, radius);
    bbox query_box(center - offset, center + offset);

    auto candidates = search(query_box);

    std::vector<entry> results;
    const T radius_sq = radius * radius;
    for(const auto& e : candidates){
        if(point_to_bbox_sq_dist(center, e.box) <= radius_sq){
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
        if(!e.box.has_extent()){
            results.push_back(e.box.min);
        }
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

    if(all_entries.size() > count){
        std::nth_element(all_entries.begin(), all_entries.begin() + static_cast<std::ptrdiff_t>(count), all_entries.end(), cmp);
        all_entries.resize(count);
    }

    std::sort(all_entries.begin(), all_entries.end(), cmp);

    results.reserve(all_entries.size());
    for(const auto &pair : all_entries){
        results.push_back(pair.second);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> octree<T>::nearest_neighbors_points(const vec3<T> &query_point, size_t k) const {
    if(k == 0) return {};
    auto entries = nearest_neighbors(query_point, entry_count);
    std::vector<vec3<T>> results;
    results.reserve(std::min(k, entries.size()));
    for(const auto& e : entries){
        if(!e.box.has_extent()){
            results.push_back(e.box.min);
            if(results.size() == k) break;
        }
    }
    return results;
}

template <class T>
bool octree<T>::contains(const vec3<T> &point) const {
    return contains(bbox(point, point));
}

template <class T>
bool octree<T>::contains(const bbox &bb) const {
    if(root == nullptr) return false;

    auto results = search(bb);
    for(const auto &result : results){
        if(result.box == bb){
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
