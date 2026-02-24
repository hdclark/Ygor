//YgorIndexRTree.cc.

#include <algorithm>   //Needed for std::reverse.
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <functional>  //Needed for passing kernel functions to integration schemes.
#include <limits>      //Needed for std::numeric_limits::max().
#include <stdexcept>
#include <utility>     //Needed for std::pair.
#include <vector>
#include <cstdint>
//#include <optional>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndexRTree.h"

//#ifndef YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- rtree: R*-tree spatial indexing data structure -----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ bbox ---------------------------------------------------------------

template <class T>
rtree<T>::bbox::bbox() : min(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))),
                         max(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))) { }

template <class T>
rtree<T>::bbox::bbox(const vec3<T> &min_corner, const vec3<T> &max_corner) 
    : min(min_corner), max(max_corner) { }

template <class T>
T rtree<T>::bbox::volume() const {
    const auto dx = max.x - min.x;
    const auto dy = max.y - min.y;
    const auto dz = max.z - min.z;
    return dx * dy * dz;
}

template <class T>
T rtree<T>::bbox::surface_area() const {
    const auto dx = max.x - min.x;
    const auto dy = max.y - min.y;
    const auto dz = max.z - min.z;
    return static_cast<T>(2) * (dx * dy + dy * dz + dz * dx);
}

template <class T>
T rtree<T>::bbox::margin() const {
    const auto dx = max.x - min.x;
    const auto dy = max.y - min.y;
    const auto dz = max.z - min.z;
    return dx + dy + dz;
}

template <class T>
bool rtree<T>::bbox::contains(const vec3<T> &point) const {
    return (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z);
}

template <class T>
bool rtree<T>::bbox::intersects(const bbox &other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y ||
             max.z < other.min.z || min.z > other.max.z);
}

template <class T>
typename rtree<T>::bbox rtree<T>::bbox::union_with(const bbox &other) const {
    bbox result;
    result.min.x = std::min(min.x, other.min.x);
    result.min.y = std::min(min.y, other.min.y);
    result.min.z = std::min(min.z, other.min.z);
    result.max.x = std::max(max.x, other.max.x);
    result.max.y = std::max(max.y, other.max.y);
    result.max.z = std::max(max.z, other.max.z);
    return result;
}

template <class T>
typename rtree<T>::bbox rtree<T>::bbox::intersection_with(const bbox &other) const {
    bbox result;
    result.min.x = std::max(min.x, other.min.x);
    result.min.y = std::max(min.y, other.min.y);
    result.min.z = std::max(min.z, other.min.z);
    result.max.x = std::min(max.x, other.max.x);
    result.max.y = std::min(max.y, other.max.y);
    result.max.z = std::min(max.z, other.max.z);
    return result;
}

template <class T>
T rtree<T>::bbox::volume_increase(const bbox &other) const {
    const auto combined = union_with(other);
    return combined.volume() - volume();
}

template <class T>
void rtree<T>::bbox::expand(const bbox &other) {
    min.x = std::min(min.x, other.min.x);
    min.y = std::min(min.y, other.min.y);
    min.z = std::min(min.z, other.min.z);
    max.x = std::max(max.x, other.max.x);
    max.y = std::max(max.y, other.max.y);
    max.z = std::max(max.z, other.max.z);
}

template <class T>
void rtree<T>::bbox::expand(const vec3<T> &point) {
    min.x = std::min(min.x, point.x);
    min.y = std::min(min.y, point.y);
    min.z = std::min(min.z, point.z);
    max.x = std::max(max.x, point.x);
    max.y = std::max(max.y, point.y);
    max.z = std::max(max.z, point.z);
}

//------------------------------------------------------ node_base ----------------------------------------------------------

template <class T>
rtree<T>::node_base::node_base() : parent(nullptr) { }

template <class T>
rtree<T>::node_base::~node_base() { }

//------------------------------------------------------ internal_node ------------------------------------------------------

template <class T>
rtree<T>::internal_node::internal_node() { }

template <class T>
rtree<T>::internal_node::~internal_node() {
    for(auto child : children) {
        delete child;
    }
}

template <class T>
bool rtree<T>::internal_node::is_leaf() const {
    return false;
}

//------------------------------------------------------ leaf_node ----------------------------------------------------------

template <class T>
rtree<T>::leaf_node::leaf_node() { }

template <class T>
bool rtree<T>::leaf_node::is_leaf() const {
    return true;
}

//------------------------------------------------------ Constructors -------------------------------------------------------

template <class T>
rtree<T>::rtree() : root(nullptr), max_entries(8), min_entries(4), height(0), size(0) {
    root = new leaf_node();
}

template <class T>
rtree<T>::rtree(size_t max_node_entries) 
    : root(nullptr), max_entries(max_node_entries), min_entries(max_node_entries / 2), 
      height(0), size(0) {
    if(max_entries < 2) {
        throw std::invalid_argument("Maximum entries per node must be at least 2");
    }
    root = new leaf_node();
}

template <class T>
rtree<T>::~rtree() {
    clear();
    delete root;
}

//------------------------------------------------------ Member functions ---------------------------------------------------

template <class T>
void rtree<T>::insert(const vec3<T> &point) {
    // Create a bounding box for the point (zero volume)
    bbox point_box(point, point);
    
    // Find the appropriate leaf node
    leaf_node* leaf = choose_leaf(point);
    
    // Add the point to the leaf
    leaf->entries.push_back(point);
    
    // Update the leaf's bounding box
    if(leaf->entries.size() == 1) {
        leaf->bounds = point_box;
    } else {
        leaf->bounds.expand(point);
    }
    
    size++;
    
    // Handle overflow
    if(leaf->entries.size() > max_entries) {
        node_base* new_node = split_leaf_node(leaf);
        adjust_tree(leaf, new_node);
    } else {
        adjust_tree(leaf, nullptr);
    }
}

template <class T>
typename rtree<T>::leaf_node* rtree<T>::choose_leaf(const vec3<T> &point) {
    bbox point_box(point, point);
    node_base* node = root;
    
    // Traverse down to a leaf
    while(!node->is_leaf()) {
        internal_node* internal = static_cast<internal_node*>(node);
        
        // Choose the child with minimum volume increase
        T min_increase = std::numeric_limits<T>::max();
        T min_volume = std::numeric_limits<T>::max();
        node_base* best_child = nullptr;
        
        for(auto child : internal->children) {
            T increase = child->bounds.volume_increase(point_box);
            T volume = child->bounds.volume();
            
            if(increase < min_increase || 
               (increase == min_increase && volume < min_volume)) {
                min_increase = increase;
                min_volume = volume;
                best_child = child;
            }
        }
        
        node = best_child;
    }
    
    return static_cast<leaf_node*>(node);
}

template <class T>
typename rtree<T>::leaf_node* rtree<T>::split_leaf_node(leaf_node* node) {
    // Create a new leaf node
    leaf_node* new_leaf = new leaf_node();
    
    // Determine the split axis and index using R*-tree algorithm
    std::vector<bbox> entry_boxes;
    for(const auto& entry : node->entries) {
        entry_boxes.emplace_back(entry, entry);
    }
    
    int split_axis = choose_split_axis(entry_boxes);
    size_t split_index = choose_split_index(entry_boxes, split_axis);
    
    // Sort entries along the chosen axis
    std::vector<std::pair<T, size_t>> sorted_entries;
    for(size_t i = 0; i < node->entries.size(); ++i) {
        T coord;
        if(split_axis == 0) coord = node->entries[i].x;
        else if(split_axis == 1) coord = node->entries[i].y;
        else coord = node->entries[i].z;
        sorted_entries.emplace_back(coord, i);
    }
    std::sort(sorted_entries.begin(), sorted_entries.end());
    
    // Distribute entries between the two nodes
    std::vector<vec3<T>> entries1, entries2;
    for(size_t i = 0; i < sorted_entries.size(); ++i) {
        if(i < split_index) {
            entries1.push_back(node->entries[sorted_entries[i].second]);
        } else {
            entries2.push_back(node->entries[sorted_entries[i].second]);
        }
    }
    
    // Update the original node
    node->entries = entries1;
    if(!entries1.empty()) {
        node->bounds = bbox(entries1[0], entries1[0]);
        for(size_t i = 1; i < entries1.size(); ++i) {
            node->bounds.expand(entries1[i]);
        }
    }
    
    // Update the new node
    new_leaf->entries = entries2;
    if(!entries2.empty()) {
        new_leaf->bounds = bbox(entries2[0], entries2[0]);
        for(size_t i = 1; i < entries2.size(); ++i) {
            new_leaf->bounds.expand(entries2[i]);
        }
    }
    
    new_leaf->parent = node->parent;
    
    return new_leaf;
}

template <class T>
int rtree<T>::choose_split_axis(const std::vector<bbox> &entries) const {
    // For simplicity, choose the axis with the largest extent
    T max_extent = static_cast<T>(0);
    int best_axis = 0;
    
    for(int axis = 0; axis < 3; ++axis) {
        T min_val = std::numeric_limits<T>::max();
        T max_val = std::numeric_limits<T>::lowest();
        
        for(const auto& box : entries) {
            T min_coord, max_coord;
            if(axis == 0) {
                min_coord = box.min.x;
                max_coord = box.max.x;
            } else if(axis == 1) {
                min_coord = box.min.y;
                max_coord = box.max.y;
            } else {
                min_coord = box.min.z;
                max_coord = box.max.z;
            }
            min_val = std::min(min_val, min_coord);
            max_val = std::max(max_val, max_coord);
        }
        
        T extent = max_val - min_val;
        if(extent > max_extent) {
            max_extent = extent;
            best_axis = axis;
        }
    }
    
    return best_axis;
}

template <class T>
size_t rtree<T>::choose_split_index(const std::vector<bbox> &entries, int axis) const {
    // Split at the median to ensure balanced distribution
    return (entries.size() + 1) / 2;
}

template <class T>
void rtree<T>::adjust_tree(node_base* node, node_base* split_node) {
    while(node != root) {
        internal_node* parent = static_cast<internal_node*>(node->parent);
        
        // If there was a split, add the new node to the parent first
        if(split_node != nullptr) {
            parent->children.push_back(split_node);
            split_node->parent = parent;
        }
        
        // Update parent's bounding box (after potentially adding split_node)
        if(!parent->children.empty()) {
            parent->bounds = parent->children[0]->bounds;
            for(size_t i = 1; i < parent->children.size(); ++i) {
                parent->bounds.expand(parent->children[i]->bounds);
            }
        }
        
        // Check if parent overflows
        if(split_node != nullptr && parent->children.size() > max_entries) {
            node_base* new_parent = split_internal_node(parent);
            adjust_tree(parent, new_parent);
            return;
        }
        
        node = parent;
        split_node = nullptr;
    }
    
    // If root was split, create a new root
    if(split_node != nullptr) {
        internal_node* new_root = new internal_node();
        new_root->children.push_back(root);
        new_root->children.push_back(split_node);
        root->parent = new_root;
        split_node->parent = new_root;
        
        new_root->bounds = root->bounds.union_with(split_node->bounds);
        
        root = new_root;
        height++;
    }
}

template <class T>
typename rtree<T>::internal_node* rtree<T>::split_internal_node(internal_node* node) {
    internal_node* new_internal = new internal_node();
    
    // Determine split axis and index
    std::vector<bbox> child_boxes;
    for(auto child : node->children) {
        child_boxes.push_back(child->bounds);
    }
    
    int split_axis = choose_split_axis(child_boxes);
    size_t split_index = choose_split_index(child_boxes, split_axis);
    
    // Sort children along the chosen axis
    std::vector<std::pair<T, node_base*>> sorted_children;
    for(auto child : node->children) {
        T coord;
        if(split_axis == 0) coord = (child->bounds.min.x + child->bounds.max.x) / static_cast<T>(2);
        else if(split_axis == 1) coord = (child->bounds.min.y + child->bounds.max.y) / static_cast<T>(2);
        else coord = (child->bounds.min.z + child->bounds.max.z) / static_cast<T>(2);
        sorted_children.emplace_back(coord, child);
    }
    std::sort(sorted_children.begin(), sorted_children.end());
    
    // Distribute children
    std::vector<node_base*> children1, children2;
    for(size_t i = 0; i < sorted_children.size(); ++i) {
        if(i < split_index) {
            children1.push_back(sorted_children[i].second);
        } else {
            children2.push_back(sorted_children[i].second);
        }
    }
    
    // Update original node
    node->children = children1;
    if(!children1.empty()) {
        node->bounds = children1[0]->bounds;
        for(size_t i = 1; i < children1.size(); ++i) {
            children1[i]->parent = node;
            node->bounds.expand(children1[i]->bounds);
        }
        children1[0]->parent = node;
    }
    
    // Update new node
    new_internal->children = children2;
    if(!children2.empty()) {
        new_internal->bounds = children2[0]->bounds;
        for(size_t i = 1; i < children2.size(); ++i) {
            children2[i]->parent = new_internal;
            new_internal->bounds.expand(children2[i]->bounds);
        }
        children2[0]->parent = new_internal;
    }
    
    new_internal->parent = node->parent;
    
    return new_internal;
}

template <class T>
std::vector<vec3<T>> rtree<T>::search(const bbox &query_box) const {
    std::vector<vec3<T>> results;
    if(root != nullptr) {
        search_recursive(root, query_box, results);
    }
    return results;
}

template <class T>
void rtree<T>::search_recursive(node_base* node, const bbox &query_box, std::vector<vec3<T>> &results) const {
    if(!node->bounds.intersects(query_box)) {
        return;
    }
    
    if(node->is_leaf()) {
        leaf_node* leaf = static_cast<leaf_node*>(node);
        for(const auto& entry : leaf->entries) {
            if(query_box.contains(entry)) {
                results.push_back(entry);
            }
        }
    } else {
        internal_node* internal = static_cast<internal_node*>(node);
        for(auto child : internal->children) {
            search_recursive(child, query_box, results);
        }
    }
}

template <class T>
std::vector<vec3<T>> rtree<T>::search_radius(const vec3<T> &center, T radius) const {
    // Create a bounding box for the search sphere
    vec3<T> offset(radius, radius, radius);
    bbox query_box(center - offset, center + offset);
    
    // Get candidates from bounding box search
    auto candidates = search(query_box);
    
    // Filter by actual distance
    std::vector<vec3<T>> results;
    T radius_sq = radius * radius;
    for(const auto& point : candidates) {
        if((point - center).sq_length() <= radius_sq) {
            results.push_back(point);
        }
    }
    
    return results;
}

template <class T>
std::vector<vec3<T>> rtree<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    // Simple implementation: get all points and sort by distance
    // A more efficient implementation would use a priority queue
    std::vector<std::pair<T, vec3<T>>> all_points;
    
    std::function<void(node_base*)> collect_all = [&](node_base* node) {
        if(node->is_leaf()) {
            leaf_node* leaf = static_cast<leaf_node*>(node);
            for(const auto& entry : leaf->entries) {
                T dist_sq = (entry - query_point).sq_length();
                all_points.emplace_back(dist_sq, entry);
            }
        } else {
            internal_node* internal = static_cast<internal_node*>(node);
            for(auto child : internal->children) {
                collect_all(child);
            }
        }
    };
    
    if(root != nullptr) {
        collect_all(root);
    }
    
    // Sort by distance
    std::sort(all_points.begin(), all_points.end());
    
    // Return k nearest
    std::vector<vec3<T>> results;
    size_t count = std::min(k, all_points.size());
    for(size_t i = 0; i < count; ++i) {
        results.push_back(all_points[i].second);
    }
    
    return results;
}

template <class T>
bool rtree<T>::contains(const vec3<T> &point) const {
    bbox point_box(point, point);
    auto results = search(point_box);
    
    // Use a tolerance for floating point comparison
    const T epsilon = std::numeric_limits<T>::epsilon() * static_cast<T>(10);
    
    for(const auto& result : results) {
        // Check if the points are approximately equal
        if(std::abs(result.x - point.x) <= epsilon &&
           std::abs(result.y - point.y) <= epsilon &&
           std::abs(result.z - point.z) <= epsilon) {
            return true;
        }
    }
    return false;
}

template <class T>
void rtree<T>::clear() {
    if(root != nullptr) {
        delete_tree(root);
        delete root;
        root = new leaf_node();
        height = 0;
        size = 0;
    }
}

template <class T>
void rtree<T>::delete_tree(node_base* node) {
    if(node != nullptr && !node->is_leaf()) {
        internal_node* internal = static_cast<internal_node*>(node);
        for(auto child : internal->children) {
            delete_tree(child);
            delete child;
        }
        internal->children.clear();
    }
}

template <class T>
size_t rtree<T>::get_size() const {
    return size;
}

template <class T>
size_t rtree<T>::get_height() const {
    return compute_height(root);
}

template <class T>
size_t rtree<T>::compute_height(node_base* node) const {
    if(node == nullptr || node->is_leaf()) {
        return 0;
    }
    
    internal_node* internal = static_cast<internal_node*>(node);
    if(internal->children.empty()) {
        return 0;
    }
    
    return 1 + compute_height(internal->children.front());
}

template <class T>
typename rtree<T>::bbox rtree<T>::get_bounds() const {
    if(root != nullptr) {
        return root->bounds;
    }
    return bbox();
}

#ifndef YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
    template class rtree<float>;
    template class rtree<double>;
#endif


