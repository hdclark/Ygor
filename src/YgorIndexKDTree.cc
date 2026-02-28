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
    T dist_sq = static_cast<T>(0);
    
    // X axis.
    if(point.x < box.min.x){
        T d = box.min.x - point.x;
        dist_sq += d * d;
    }else if(point.x > box.max.x){
        T d = point.x - box.max.x;
        dist_sq += d * d;
    }
    
    // Y axis.
    if(point.y < box.min.y){
        T d = box.min.y - point.y;
        dist_sq += d * d;
    }else if(point.y > box.max.y){
        T d = point.y - box.max.y;
        dist_sq += d * d;
    }
    
    // Z axis.
    if(point.z < box.min.z){
        T d = box.min.z - point.z;
        dist_sq += d * d;
    }else if(point.z > box.max.z){
        T d = point.z - box.max.z;
        dist_sq += d * d;
    }
    
    return dist_sq;
}

template <class T>
std::unique_ptr<typename kdtree<T>::kdtree_node>
kdtree<T>::build(std::vector<entry> &entries, size_t begin, size_t end, int depth) {
    if(begin >= end) return nullptr;
    
    const int axis = depth % 3;
    
    // Use nth_element to partition around the median on the splitting axis.
    const size_t mid = begin + (end - begin) / 2;
    auto mid_it = entries.begin() + static_cast<std::ptrdiff_t>(mid);
    std::nth_element(entries.begin() + static_cast<std::ptrdiff_t>(begin),
                     mid_it,
                     entries.begin() + static_cast<std::ptrdiff_t>(end),
                     [axis, this](const entry &a, const entry &b){
                         return get_coord(a.point, axis) < get_coord(b.point, axis);
                     });
    
    auto node = std::make_unique<kdtree_node>(entries[mid], axis);
    node->left  = build(entries, begin, mid, depth + 1);
    node->right = build(entries, mid + 1, end, depth + 1);
    
    return node;
}

template <class T>
void kdtree<T>::ensure_built() {
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
    if(node == nullptr) return;
    
    // Check if the current node's point is inside the query box.
    if(query_box.contains(node->data.point)){
        results.push_back(node->data);
    }
    
    const int axis = node->split_axis;
    const T split_val = get_coord(node->data.point, axis);
    
    // Determine which subtrees may contain results.
    T query_min, query_max;
    if(axis == 0){
        query_min = query_box.min.x;
        query_max = query_box.max.x;
    }else if(axis == 1){
        query_min = query_box.min.y;
        query_max = query_box.max.y;
    }else{
        query_min = query_box.min.z;
        query_max = query_box.max.z;
    }
    
    // Left subtree contains points with coord < split_val (or equal).
    if(query_min <= split_val){
        search_recursive(node->left.get(), query_box, results);
    }
    // Right subtree contains points with coord > split_val.
    if(query_max >= split_val){
        search_recursive(node->right.get(), query_box, results);
    }
}

template <class T>
void kdtree<T>::nearest_recursive(const kdtree_node* node, const vec3<T> &query_point, size_t k,
                                   std::vector<std::pair<T, entry>> &best) const {
    if(node == nullptr) return;
    
    const T dist_sq = (node->data.point - query_point).sq_length();
    
    // Insert current node if it qualifies.
    if(best.size() < k){
        best.emplace_back(dist_sq, node->data);
        // Maintain a max-heap by distance so the farthest is at front.
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
    const T split_val = get_coord(node->data.point, axis);
    const T query_val = get_coord(query_point, axis);
    const T diff = query_val - split_val;
    
    // Decide which subtree to search first (the one containing the query point).
    const kdtree_node* near_subtree = (diff <= static_cast<T>(0)) ? node->left.get() : node->right.get();
    const kdtree_node* far_subtree  = (diff <= static_cast<T>(0)) ? node->right.get() : node->left.get();
    
    nearest_recursive(near_subtree, query_point, k, best);
    
    // Check if we need to search the far subtree.
    const T split_dist_sq = diff * diff;
    if(best.size() < k || split_dist_sq < best.front().first){
        nearest_recursive(far_subtree, query_point, k, best);
    }
}

template <class T>
void kdtree<T>::update_bounds(const vec3<T> &point) {
    if(!bounds_initialized){
        bounds = bbox(point, point);
        bounds_initialized = true;
    }else{
        bounds.expand(point);
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
    if(!point.isfinite()){
        throw std::invalid_argument("Cannot insert non-finite point into kdtree");
    }
    entry e(point, std::move(aux_data));
    
    update_bounds(point);
    pending_entries.push_back(std::move(e));
    ++entry_count;
    tree_built = false;
}

template <class T>
std::vector<typename kdtree<T>::entry> kdtree<T>::search(const bbox &query_box) const {
    std::vector<entry> results;

    if(tree_built) {
        if(root != nullptr){
            search_recursive(root.get(), query_box, results);
        }
    } else {
        // Fallback for unbuilt tree: linear scan of pending entries without mutating this object.
        for(const auto &e : pending_entries){
            const auto &p = e.point;
            if(p.x >= query_box.min.x && p.x <= query_box.max.x &&
               p.y >= query_box.min.y && p.y <= query_box.max.y &&
               p.z >= query_box.min.z && p.z <= query_box.max.z){
                results.push_back(e);
            }
        }
    }
    return results;
}

template <class T>
std::vector<vec3<T>> kdtree<T>::search_points(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename kdtree<T>::entry> kdtree<T>::search_radius(const vec3<T> &center, T radius) const {
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
std::vector<vec3<T>> kdtree<T>::search_radius_points(const vec3<T> &center, T radius) const {
    auto entries = search_radius(center, radius);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename kdtree<T>::entry> kdtree<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    // Build the tree if needed (logical const: deferred build).
    const_cast<kdtree*>(this)->ensure_built();
    
    std::vector<std::pair<T, entry>> best;

    std::vector<entry> results;
    if(k == 0 || root == nullptr){
        return results;
    }

    best.reserve(k + 1);
    nearest_recursive(root.get(), query_point, k, best);

    // Sort the results by distance (ascending).
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
    auto entries = nearest_neighbors(query_point, k);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
bool kdtree<T>::contains(const vec3<T> &point) const {
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

