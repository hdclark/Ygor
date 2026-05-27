//YgorIndexKDTree.cc.

#include <algorithm>
#include <any>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"
#include "YgorIndexKDTree.h"

//#ifndef YGOR_INDEX_KDTREE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_KDTREE_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------- kdtree: kd-tree spatial indexing data structure ----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ kdtree_node --------------------------------------------------------

template <class T>
kdtree<T>::kdtree_node::kdtree_node() : split_axis(0) { }

template <class T>
kdtree<T>::kdtree_node::kdtree_node(const entry &e, int axis) : data(e), split_axis(axis) { }

//------------------------------------------------------ Private helpers ----------------------------------------------------

template <class T>
T kdtree<T>::get_coord(const vec3<T> &point, int axis) {
    if(axis == 0) return point.x;
    if(axis == 1) return point.y;
    return point.z;
}

template <class T>
T kdtree<T>::point_to_bbox_sq_dist(const vec3<T> &point, const bbox &box) {
    return box.squared_distance_to(point);
}

template <class T>
std::unique_ptr<typename kdtree<T>::kdtree_node>
kdtree<T>::build(std::vector<entry> &entries, size_t begin, size_t end, int depth) const {
    if(begin >= end) return nullptr;

    const int axis = depth % 3;
    const size_t mid = begin + (end - begin) / 2;
    auto mid_it = entries.begin() + static_cast<std::ptrdiff_t>(mid);
    std::nth_element(entries.begin() + static_cast<std::ptrdiff_t>(begin),
                     mid_it,
                     entries.begin() + static_cast<std::ptrdiff_t>(end),
                     [axis](const entry &a, const entry &b){
                         return get_coord(a.box.center(), axis) < get_coord(b.box.center(), axis);
                     });

    auto node = std::make_unique<kdtree_node>(entries[mid], axis);

    node->node_bounds = entries[begin].box;
    for(size_t i = begin + 1; i < end; ++i){
        node->node_bounds.expand(entries[i].box);
    }

    node->left  = build(entries, begin, mid, depth + 1);
    node->right = build(entries, mid + 1, end, depth + 1);

    return node;
}

template <class T>
void kdtree<T>::ensure_built() const {
    if(tree_built) return;

    root.reset();

    if(!pending_entries.empty()){
        root = build(pending_entries, 0, pending_entries.size(), 0);
    }

    tree_built = true;
}

template <class T>
void kdtree<T>::search_recursive(const kdtree_node* node, const bbox &query_box,
                                  std::vector<entry> &results) const {
    if((node == nullptr) || !node->node_bounds.intersects(query_box)) return;

    if(node->data.box.intersects(query_box)){
        results.push_back(node->data);
    }

    search_recursive(node->left.get(), query_box, results);
    search_recursive(node->right.get(), query_box, results);
}

template <class T>
void kdtree<T>::nearest_recursive(const kdtree_node* node, const vec3<T> &query_point, size_t k,
                                   std::vector<std::pair<T, entry>> &best) const {
    if(node == nullptr) return;

    if(best.size() == k){
        const T min_dist_sq = point_to_bbox_sq_dist(query_point, node->node_bounds);
        if(min_dist_sq >= best.front().first) return;
    }

    const T dist_sq = node->data.box.squared_distance_to(query_point);

    if(best.size() < k){
        best.emplace_back(dist_sq, node->data);
        std::push_heap(best.begin(), best.end(),
                       [](const auto &a, const auto &b){ return a.first < b.first; });
    }else if(dist_sq < best.front().first){
        std::pop_heap(best.begin(), best.end(),
                      [](const auto &a, const auto &b){ return a.first < b.first; });
        best.back() = std::make_pair(dist_sq, node->data);
        std::push_heap(best.begin(), best.end(),
                       [](const auto &a, const auto &b){ return a.first < b.first; });
    }

    const int axis = node->split_axis;
    const T split_val = get_coord(node->data.box.center(), axis);
    const T query_val = get_coord(query_point, axis);
    const T diff = query_val - split_val;

    const kdtree_node* near_subtree = (diff <= static_cast<T>(0)) ? node->left.get() : node->right.get();
    const kdtree_node* far_subtree  = (diff <= static_cast<T>(0)) ? node->right.get() : node->left.get();

    nearest_recursive(near_subtree, query_point, k, best);

    const T split_dist_sq = diff * diff;
    if(best.size() < k || split_dist_sq < best.front().first){
        nearest_recursive(far_subtree, query_point, k, best);
    }
}

template <class T>
void kdtree<T>::update_bounds(const vec3<T> &point) {
    update_bounds(bbox(point, point));
}

template <class T>
void kdtree<T>::update_bounds(const bbox &bb) {
    if(!bounds_initialized){
        bounds = bb;
        bounds_initialized = true;
    }else{
        bounds.expand(bb);
    }
}

//------------------------------------------------------ Constructors -------------------------------------------------------

template <class T>
kdtree<T>::kdtree() : entry_count(0), bounds_initialized(false), tree_built(true) { }

template <class T>
kdtree<T>::~kdtree() = default;

//------------------------------------------------------ Member functions ---------------------------------------------------

template <class T>
void kdtree<T>::insert(const vec3<T> &point) {
    insert(point, std::any{});
}

template <class T>
void kdtree<T>::insert(const vec3<T> &point, std::any aux_data) {
    insert(bbox(point, point), std::move(aux_data));
}

template <class T>
void kdtree<T>::insert(const bbox &bb) {
    insert(bb, std::any{});
}

template <class T>
void kdtree<T>::insert(const bbox &bb, std::any aux_data) {
    if(!bb.isfinite()){
        throw std::invalid_argument("Cannot insert non-finite bbox into kdtree");
    }

    pending_entries.emplace_back(bb, std::move(aux_data));
    update_bounds(bb);
    ++entry_count;
    tree_built = false;
}

template <class T>
std::vector<typename kdtree<T>::entry> kdtree<T>::search(const bbox &query_box) const {
    ensure_built();

    std::vector<entry> results;
    if(root != nullptr){
        search_recursive(root.get(), query_box, results);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> kdtree<T>::search_points(const bbox &query_box) const {
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
std::vector<typename kdtree<T>::bbox> kdtree<T>::search_bboxes(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<bbox> results;
    results.reserve(entries.size());
    for(const auto &e : entries){
        results.push_back(e.box);
    }
    return results;
}

template <class T>
std::vector<typename kdtree<T>::bbox> kdtree<T>::search_bboxes(const vec3<T> &query_point) const {
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
std::vector<typename kdtree<T>::entry> kdtree<T>::search_radius(const vec3<T> &center, T radius) const {
    vec3<T> offset(radius, radius, radius);
    bbox query_box(center - offset, center + offset);

    auto candidates = search(query_box);

    std::vector<entry> results;
    const T radius_sq = radius * radius;
    for(const auto& e : candidates){
        if(e.box.squared_distance_to(center) <= radius_sq){
            results.push_back(e);
        }
    }
    return results;
}

template <class T>
std::vector<vec3<T>> kdtree<T>::search_radius_points(const vec3<T> &center, T radius) const {
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
std::vector<typename kdtree<T>::entry> kdtree<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    ensure_built();

    std::vector<std::pair<T, entry>> best;

    std::vector<entry> results;
    if(k == 0 || root == nullptr){
        return results;
    }

    best.reserve(k + 1);
    nearest_recursive(root.get(), query_point, k, best);

    std::sort(best.begin(), best.end(),
              [](const auto &a, const auto &b){ return a.first < b.first; });

    results.reserve(best.size());
    for(const auto &pair : best){
        results.push_back(pair.second);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> kdtree<T>::nearest_neighbors_points(const vec3<T> &query_point, size_t k) const {
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
bool kdtree<T>::contains(const vec3<T> &point) const {
    return contains(bbox(point, point));
}

template <class T>
bool kdtree<T>::contains(const bbox &bb) const {
    auto results = search(bb);
    for(const auto &result : results){
        if(result.box == bb){
            return true;
        }
    }
    return false;
}

template <class T>
void kdtree<T>::clear() {
    root.reset();
    pending_entries.clear();
    entry_count = 0;
    bounds_initialized = false;
    bounds = bbox();
    tree_built = true;
}

template <class T>
size_t kdtree<T>::get_size() const {
    return entry_count;
}

template <class T>
typename kdtree<T>::bbox kdtree<T>::get_bounds() const {
    return bounds;
}

#ifndef YGOR_INDEX_KDTREE_DISABLE_ALL_SPECIALIZATIONS
    template class kdtree<float>;
    template class kdtree<double>;
#endif
