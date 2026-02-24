//YgorIndexRTree.cc.

#include <algorithm>   //Needed for std::reverse.
#include <any>
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <functional>  //Needed for passing kernel functions to integration schemes.
#include <limits>      //Needed for std::numeric_limits::max().
#include <memory>
#include <stdexcept>
#include <utility>     //Needed for std::pair.
#include <vector>
#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndexRTree.h"

//#ifndef YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- rtree: R*-tree spatial indexing data structure -----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ entry --------------------------------------------------------------

template <class T>
rtree<T>::entry::entry() : point(), aux_data() { }

template <class T>
rtree<T>::entry::entry(const vec3<T> &p) : point(p), aux_data() { }

template <class T>
rtree<T>::entry::entry(const vec3<T> &p, std::any data) : point(p), aux_data(std::move(data)) { }

template <class T>
bool rtree<T>::entry::operator==(const entry &other) const {
    return point == other.point;
}

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

//------------------------------------------------------ internal_node ------------------------------------------------------

template <class T>
rtree<T>::internal_node::internal_node() : node_base() { }

template <class T>
bool rtree<T>::internal_node::is_leaf() const {
    return false;
}

//------------------------------------------------------ leaf_node ----------------------------------------------------------

template <class T>
rtree<T>::leaf_node::leaf_node() : node_base() { }

template <class T>
bool rtree<T>::leaf_node::is_leaf() const {
    return true;
}

//------------------------------------------------------ Constructors -------------------------------------------------------

template <class T>
rtree<T>::rtree() : root(std::make_unique<leaf_node>()), 
                    max_entries(8), min_entries(4), height(0), entry_count(0),
                    reinsert_count(3), in_reinsertion(false) { }  // ~30% of max_entries

template <class T>
rtree<T>::rtree(size_t max_node_entries) 
    : max_entries(max_node_entries), min_entries(max_node_entries / 2), 
      height(0), entry_count(0), in_reinsertion(false) {
    if(max_entries < 2) {
        throw std::invalid_argument("Maximum entries per node must be at least 2");
    }
    // R*-tree reinsertion: typically 30% of max_entries
    reinsert_count = std::max(static_cast<size_t>(1), static_cast<size_t>(max_entries * 0.3));
    root = std::make_unique<leaf_node>();
}

template <class T>
rtree<T>::~rtree() = default;

//------------------------------------------------------ Member functions ---------------------------------------------------

template <class T>
void rtree<T>::insert(const vec3<T> &point) {
    insert(point, std::any{});
}

template <class T>
void rtree<T>::insert(const vec3<T> &point, std::any aux_data) {
    // Reset the reinsertion flag for new top-level insert
    in_reinsertion = false;
    
    entry e(point, std::move(aux_data));
    insert_entry_at_leaf(e);
}

template <class T>
void rtree<T>::insert_entry_at_leaf(const entry &e) {
    // Find the appropriate leaf node using simple traversal
    leaf_node* leaf = choose_leaf(e.point);
    
    // Add the entry to the leaf
    leaf->entries.push_back(e);
    
    // Update the leaf's bounding box
    update_bounds(leaf);
    
    entry_count++;
    
    // Handle overflow using R*-tree overflow treatment
    bool is_root = (leaf->parent == nullptr);
    
    if(leaf->entries.size() > max_entries) {
        auto split_result = overflow_treatment(leaf, is_root);
        adjust_tree(leaf, std::move(split_result));
    } else {
        adjust_tree(leaf, nullptr);
    }
}

template <class T>
typename rtree<T>::node_base* rtree<T>::choose_subtree(node_base* node, const bbox &entry_box, 
                                                        size_t target_level, size_t current_level) {
    if(current_level == target_level) {
        return node;
    }
    
    if(node->is_leaf()) {
        return node;
    }
    
    internal_node* internal = static_cast<internal_node*>(node);
    
    // If children are leaves or this is the level above target, use minimum overlap increase
    // Otherwise, use minimum volume increase
    bool children_are_leaves = !internal->children.empty() && internal->children[0]->is_leaf();
    
    node_base* best_child = nullptr;
    
    if(children_are_leaves) {
        // Use minimum overlap increase (R*-tree optimization for leaf-level inserts)
        T min_overlap_increase = std::numeric_limits<T>::max();
        T min_volume_increase = std::numeric_limits<T>::max();
        T min_volume = std::numeric_limits<T>::max();
        
        for(auto& child_ptr : internal->children) {
            node_base* child = child_ptr.get();
            
            // Calculate current overlap with siblings
            T current_overlap = static_cast<T>(0);
            T new_overlap = static_cast<T>(0);
            bbox expanded_bounds = child->bounds.union_with(entry_box);
            
            for(auto& sibling_ptr : internal->children) {
                if(sibling_ptr.get() != child) {
                    bbox intersect = child->bounds.intersection_with(sibling_ptr->bounds);
                    if(child->bounds.intersects(sibling_ptr->bounds)) {
                        current_overlap += intersect.volume();
                    }
                    bbox new_intersect = expanded_bounds.intersection_with(sibling_ptr->bounds);
                    if(expanded_bounds.intersects(sibling_ptr->bounds)) {
                        new_overlap += new_intersect.volume();
                    }
                }
            }
            
            T overlap_increase = new_overlap - current_overlap;
            T volume_increase = child->bounds.volume_increase(entry_box);
            T volume = child->bounds.volume();
            
            if(overlap_increase < min_overlap_increase ||
               (overlap_increase == min_overlap_increase && volume_increase < min_volume_increase) ||
               (overlap_increase == min_overlap_increase && volume_increase == min_volume_increase && volume < min_volume)) {
                min_overlap_increase = overlap_increase;
                min_volume_increase = volume_increase;
                min_volume = volume;
                best_child = child;
            }
        }
    } else {
        // Use minimum volume increase
        T min_increase = std::numeric_limits<T>::max();
        T min_volume = std::numeric_limits<T>::max();
        
        for(auto& child_ptr : internal->children) {
            node_base* child = child_ptr.get();
            T increase = child->bounds.volume_increase(entry_box);
            T volume = child->bounds.volume();
            
            if(increase < min_increase || 
               (increase == min_increase && volume < min_volume)) {
                min_increase = increase;
                min_volume = volume;
                best_child = child;
            }
        }
    }
    
    return choose_subtree(best_child, entry_box, target_level, current_level + 1);
}

template <class T>
typename rtree<T>::leaf_node* rtree<T>::choose_leaf(const vec3<T> &point) {
    bbox point_box(point, point);
    return static_cast<leaf_node*>(choose_subtree(root.get(), point_box, height, 0));
}

template <class T>
std::unique_ptr<typename rtree<T>::node_base> rtree<T>::overflow_treatment(node_base* node, bool is_root) {
    // R*-tree forced reinsertion: if not the root and not already in reinsertion context,
    // perform reinsertion instead of splitting
    if(!is_root && !in_reinsertion && node->is_leaf()) {
        // Set flag to prevent recursive reinsertion
        in_reinsertion = true;
        
        // Perform reinsertion
        reinsert_leaf(static_cast<leaf_node*>(node));
        
        // After reinsertion, no split is needed
        return nullptr;
    }
    
    // Split the node
    if(node->is_leaf()) {
        return split_leaf_node(static_cast<leaf_node*>(node));
    } else {
        return split_internal_node(static_cast<internal_node*>(node));
    }
}

template <class T>
void rtree<T>::reinsert_leaf(leaf_node* node) {
    // Sort entries by distance from the center of the node's bounding box
    vec3<T> center = bbox_center(node->bounds);
    
    std::vector<std::pair<T, entry>> sorted_entries;
    for(const auto& e : node->entries) {
        T dist_sq = (e.point - center).sq_length();
        sorted_entries.emplace_back(dist_sq, e);
    }
    
    // Sort in descending order (farthest entries first)
    std::sort(sorted_entries.begin(), sorted_entries.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Remove the first reinsert_count entries (farthest from center)
    std::vector<entry> to_reinsert;
    size_t count = std::min(reinsert_count, sorted_entries.size());
    for(size_t i = 0; i < count; ++i) {
        to_reinsert.push_back(sorted_entries[i].second);
    }
    
    // Keep the remaining entries
    node->entries.clear();
    for(size_t i = count; i < sorted_entries.size(); ++i) {
        node->entries.push_back(sorted_entries[i].second);
    }
    
    // Update the node's bounding box
    update_bounds(node);
    
    // Adjust the tree upward to update parent bounding boxes
    adjust_tree(node, nullptr);
    
    // Reinsert the removed entries (decrement entry_count since they'll be re-added)
    entry_count -= to_reinsert.size();
    for(const auto& e : to_reinsert) {
        insert_entry_at_leaf(e);
    }
}

template <class T>
std::unique_ptr<typename rtree<T>::leaf_node> rtree<T>::split_leaf_node(leaf_node* node) {
    // Create a new leaf node
    auto new_leaf = std::make_unique<leaf_node>();
    
    // Determine the split axis and index using R*-tree algorithm
    std::vector<bbox> entry_boxes;
    for(const auto& e : node->entries) {
        entry_boxes.emplace_back(e.point, e.point);
    }
    
    int split_axis = choose_split_axis(entry_boxes);
    size_t split_index = choose_split_index(entry_boxes, split_axis);
    
    // Sort entries along the chosen axis
    std::vector<std::pair<T, size_t>> sorted_entries;
    for(size_t i = 0; i < node->entries.size(); ++i) {
        T coord;
        if(split_axis == 0) coord = node->entries[i].point.x;
        else if(split_axis == 1) coord = node->entries[i].point.y;
        else coord = node->entries[i].point.z;
        sorted_entries.emplace_back(coord, i);
    }
    std::sort(sorted_entries.begin(), sorted_entries.end());
    
    // Distribute entries between the two nodes
    std::vector<entry> entries1, entries2;
    for(size_t i = 0; i < sorted_entries.size(); ++i) {
        if(i < split_index) {
            entries1.push_back(node->entries[sorted_entries[i].second]);
        } else {
            entries2.push_back(node->entries[sorted_entries[i].second]);
        }
    }
    
    // Update the original node
    node->entries = std::move(entries1);
    update_bounds(node);
    
    // Update the new node
    new_leaf->entries = std::move(entries2);
    update_bounds(new_leaf.get());
    
    new_leaf->parent = node->parent;
    
    return new_leaf;
}

template <class T>
int rtree<T>::choose_split_axis(const std::vector<bbox> &entries) const {
    // R*-tree algorithm: choose axis with minimum sum of margin values
    T min_margin_sum = std::numeric_limits<T>::max();
    int best_axis = 0;
    
    for(int axis = 0; axis < 3; ++axis) {
        // Sort entries by lower value then by upper value along this axis
        std::vector<std::pair<T, size_t>> sorted_by_lower;
        std::vector<std::pair<T, size_t>> sorted_by_upper;
        
        for(size_t i = 0; i < entries.size(); ++i) {
            T lower_coord, upper_coord;
            if(axis == 0) {
                lower_coord = entries[i].min.x;
                upper_coord = entries[i].max.x;
            } else if(axis == 1) {
                lower_coord = entries[i].min.y;
                upper_coord = entries[i].max.y;
            } else {
                lower_coord = entries[i].min.z;
                upper_coord = entries[i].max.z;
            }
            sorted_by_lower.emplace_back(lower_coord, i);
            sorted_by_upper.emplace_back(upper_coord, i);
        }
        
        std::sort(sorted_by_lower.begin(), sorted_by_lower.end());
        std::sort(sorted_by_upper.begin(), sorted_by_upper.end());
        
        T margin_sum = static_cast<T>(0);
        
        // Try all valid split distributions
        for(size_t k = min_entries; k <= entries.size() - min_entries; ++k) {
            // For sorted_by_lower
            bbox b1, b2;
            bool first1 = true, first2 = true;
            for(size_t i = 0; i < k; ++i) {
                if(first1) {
                    b1 = entries[sorted_by_lower[i].second];
                    first1 = false;
                } else {
                    b1.expand(entries[sorted_by_lower[i].second]);
                }
            }
            for(size_t i = k; i < entries.size(); ++i) {
                if(first2) {
                    b2 = entries[sorted_by_lower[i].second];
                    first2 = false;
                } else {
                    b2.expand(entries[sorted_by_lower[i].second]);
                }
            }
            margin_sum += b1.margin() + b2.margin();
            
            // For sorted_by_upper
            first1 = true; first2 = true;
            for(size_t i = 0; i < k; ++i) {
                if(first1) {
                    b1 = entries[sorted_by_upper[i].second];
                    first1 = false;
                } else {
                    b1.expand(entries[sorted_by_upper[i].second]);
                }
            }
            for(size_t i = k; i < entries.size(); ++i) {
                if(first2) {
                    b2 = entries[sorted_by_upper[i].second];
                    first2 = false;
                } else {
                    b2.expand(entries[sorted_by_upper[i].second]);
                }
            }
            margin_sum += b1.margin() + b2.margin();
        }
        
        if(margin_sum < min_margin_sum) {
            min_margin_sum = margin_sum;
            best_axis = axis;
        }
    }
    
    return best_axis;
}

template <class T>
size_t rtree<T>::choose_split_index(const std::vector<bbox> &entries, int axis) const {
    // R*-tree algorithm: choose distribution with minimum overlap, then minimum volume
    T min_overlap = std::numeric_limits<T>::max();
    T min_volume = std::numeric_limits<T>::max();
    size_t best_k = (entries.size() + 1) / 2;
    
    // Sort entries by lower value along this axis
    std::vector<std::pair<T, size_t>> sorted_entries;
    for(size_t i = 0; i < entries.size(); ++i) {
        T coord;
        if(axis == 0) coord = entries[i].min.x;
        else if(axis == 1) coord = entries[i].min.y;
        else coord = entries[i].min.z;
        sorted_entries.emplace_back(coord, i);
    }
    std::sort(sorted_entries.begin(), sorted_entries.end());
    
    // Try all valid split distributions
    for(size_t k = min_entries; k <= entries.size() - min_entries; ++k) {
        bbox b1, b2;
        bool first1 = true, first2 = true;
        
        for(size_t i = 0; i < k; ++i) {
            if(first1) {
                b1 = entries[sorted_entries[i].second];
                first1 = false;
            } else {
                b1.expand(entries[sorted_entries[i].second]);
            }
        }
        for(size_t i = k; i < entries.size(); ++i) {
            if(first2) {
                b2 = entries[sorted_entries[i].second];
                first2 = false;
            } else {
                b2.expand(entries[sorted_entries[i].second]);
            }
        }
        
        // Calculate overlap
        T overlap = static_cast<T>(0);
        if(b1.intersects(b2)) {
            bbox intersect = b1.intersection_with(b2);
            overlap = intersect.volume();
        }
        
        T total_volume = b1.volume() + b2.volume();
        
        if(overlap < min_overlap || (overlap == min_overlap && total_volume < min_volume)) {
            min_overlap = overlap;
            min_volume = total_volume;
            best_k = k;
        }
    }
    
    return best_k;
}

template <class T>
void rtree<T>::adjust_tree(node_base* node, std::unique_ptr<node_base> split_node_ptr) {
    while(node != root.get()) {
        internal_node* parent = node->parent;
        
        // If there was a split, add the new node to the parent
        if(split_node_ptr != nullptr) {
            split_node_ptr->parent = parent;
            parent->children.push_back(std::move(split_node_ptr));
            split_node_ptr = nullptr;
        }
        
        // Update parent's bounding box
        update_bounds(parent);
        
        // Check if parent overflows
        if(parent->children.size() > max_entries) {
            bool parent_is_root = (parent->parent == nullptr);
            auto new_split = overflow_treatment(parent, parent_is_root);
            
            if(parent == root.get()) {
                // Handle root split below
                if(new_split != nullptr) {
                    split_node_ptr = std::move(new_split);
                }
                break;
            }
            
            node = parent;
            split_node_ptr = std::move(new_split);
            continue;
        }
        
        node = parent;
    }
    
    // If root was split, create a new root
    if(split_node_ptr != nullptr) {
        auto new_root = std::make_unique<internal_node>();
        
        root->parent = new_root.get();
        split_node_ptr->parent = new_root.get();
        
        new_root->children.push_back(std::move(root));
        new_root->children.push_back(std::move(split_node_ptr));
        
        update_bounds(new_root.get());
        
        root = std::move(new_root);
        height++;
    }
}

template <class T>
std::unique_ptr<typename rtree<T>::internal_node> rtree<T>::split_internal_node(internal_node* node) {
    auto new_internal = std::make_unique<internal_node>();
    
    // Determine split axis and index
    std::vector<bbox> child_boxes;
    for(const auto& child : node->children) {
        child_boxes.push_back(child->bounds);
    }
    
    int split_axis = choose_split_axis(child_boxes);
    size_t split_index = choose_split_index(child_boxes, split_axis);
    
    // Sort children along the chosen axis
    std::vector<std::pair<T, size_t>> sorted_indices;
    for(size_t i = 0; i < node->children.size(); ++i) {
        T coord;
        if(split_axis == 0) coord = (node->children[i]->bounds.min.x + node->children[i]->bounds.max.x) / static_cast<T>(2);
        else if(split_axis == 1) coord = (node->children[i]->bounds.min.y + node->children[i]->bounds.max.y) / static_cast<T>(2);
        else coord = (node->children[i]->bounds.min.z + node->children[i]->bounds.max.z) / static_cast<T>(2);
        sorted_indices.emplace_back(coord, i);
    }
    std::sort(sorted_indices.begin(), sorted_indices.end());
    
    // Collect children in sorted order
    std::vector<std::unique_ptr<node_base>> sorted_children;
    sorted_children.reserve(node->children.size());
    
    // We need to move children out in sorted order, but indices will become invalid
    // So first we build a mapping and then extract
    std::vector<std::unique_ptr<node_base>> all_children;
    all_children.reserve(node->children.size());
    for(auto& child : node->children) {
        all_children.push_back(std::move(child));
    }
    node->children.clear();
    
    // Now distribute
    for(size_t i = 0; i < sorted_indices.size(); ++i) {
        size_t idx = sorted_indices[i].second;
        if(i < split_index) {
            all_children[idx]->parent = node;
            node->children.push_back(std::move(all_children[idx]));
        } else {
            all_children[idx]->parent = new_internal.get();
            new_internal->children.push_back(std::move(all_children[idx]));
        }
    }
    
    // Update bounding boxes
    update_bounds(node);
    update_bounds(new_internal.get());
    
    new_internal->parent = node->parent;
    
    return new_internal;
}

template <class T>
std::vector<typename rtree<T>::entry> rtree<T>::search(const bbox &query_box) const {
    std::vector<entry> results;
    if(root != nullptr) {
        search_recursive(root.get(), query_box, results);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> rtree<T>::search_points(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries) {
        results.push_back(e.point);
    }
    return results;
}

template <class T>
void rtree<T>::search_recursive(const node_base* node, const bbox &query_box, std::vector<entry> &results) const {
    if(!node->bounds.intersects(query_box)) {
        return;
    }
    
    if(node->is_leaf()) {
        const leaf_node* leaf = static_cast<const leaf_node*>(node);
        for(const auto& e : leaf->entries) {
            if(query_box.contains(e.point)) {
                results.push_back(e);
            }
        }
    } else {
        const internal_node* internal = static_cast<const internal_node*>(node);
        for(const auto& child : internal->children) {
            search_recursive(child.get(), query_box, results);
        }
    }
}

template <class T>
std::vector<typename rtree<T>::entry> rtree<T>::search_radius(const vec3<T> &center, T radius) const {
    // Create a bounding box for the search sphere
    vec3<T> offset(radius, radius, radius);
    bbox query_box(center - offset, center + offset);
    
    // Get candidates from bounding box search
    auto candidates = search(query_box);
    
    // Filter by actual distance
    std::vector<entry> results;
    T radius_sq = radius * radius;
    for(const auto& e : candidates) {
        if((e.point - center).sq_length() <= radius_sq) {
            results.push_back(e);
        }
    }
    
    return results;
}

template <class T>
std::vector<vec3<T>> rtree<T>::search_radius_points(const vec3<T> &center, T radius) const {
    auto entries = search_radius(center, radius);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries) {
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename rtree<T>::entry> rtree<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    // Simple implementation: get all entries and sort by distance
    // A more efficient implementation would use a priority queue
    std::vector<std::pair<T, entry>> all_entries;
    
    std::function<void(const node_base*)> collect_all = [&](const node_base* node) {
        if(node->is_leaf()) {
            const leaf_node* leaf = static_cast<const leaf_node*>(node);
            for(const auto& e : leaf->entries) {
                T dist_sq = (e.point - query_point).sq_length();
                all_entries.emplace_back(dist_sq, e);
            }
        } else {
            const internal_node* internal = static_cast<const internal_node*>(node);
            for(const auto& child : internal->children) {
                collect_all(child.get());
            }
        }
    };
    
    if(root != nullptr) {
        collect_all(root.get());
    }
    
    // Sort by distance
    std::sort(all_entries.begin(), all_entries.end(), 
              [](const auto& a, const auto& b) { return a.first < b.first; });
    
    // Return k nearest
    std::vector<entry> results;
    size_t count = std::min(k, all_entries.size());
    for(size_t i = 0; i < count; ++i) {
        results.push_back(all_entries[i].second);
    }
    
    return results;
}

template <class T>
std::vector<vec3<T>> rtree<T>::nearest_neighbors_points(const vec3<T> &query_point, size_t k) const {
    auto entries = nearest_neighbors(query_point, k);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries) {
        results.push_back(e.point);
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
        if(std::abs(result.point.x - point.x) <= epsilon &&
           std::abs(result.point.y - point.y) <= epsilon &&
           std::abs(result.point.z - point.z) <= epsilon) {
            return true;
        }
    }
    return false;
}

template <class T>
void rtree<T>::clear() {
    root = std::make_unique<leaf_node>();
    height = 0;
    entry_count = 0;
    in_reinsertion = false;
}

template <class T>
void rtree<T>::update_bounds(node_base* node) {
    if(node->is_leaf()) {
        leaf_node* leaf = static_cast<leaf_node*>(node);
        if(!leaf->entries.empty()) {
            leaf->bounds = bbox(leaf->entries[0].point, leaf->entries[0].point);
            for(size_t i = 1; i < leaf->entries.size(); ++i) {
                leaf->bounds.expand(leaf->entries[i].point);
            }
        } else {
            leaf->bounds = bbox();
        }
    } else {
        internal_node* internal = static_cast<internal_node*>(node);
        if(!internal->children.empty()) {
            internal->bounds = internal->children[0]->bounds;
            for(size_t i = 1; i < internal->children.size(); ++i) {
                internal->bounds.expand(internal->children[i]->bounds);
            }
        } else {
            internal->bounds = bbox();
        }
    }
}

template <class T>
size_t rtree<T>::get_size() const {
    return entry_count;
}

template <class T>
size_t rtree<T>::get_height() const {
    return compute_height(root.get());
}

template <class T>
size_t rtree<T>::compute_height(const node_base* node) const {
    if(node == nullptr || node->is_leaf()) {
        return 0;
    }
    
    const internal_node* internal = static_cast<const internal_node*>(node);
    if(internal->children.empty()) {
        return 0;
    }
    
    return 1 + compute_height(internal->children.front().get());
}

template <class T>
typename rtree<T>::bbox rtree<T>::get_bounds() const {
    if(root != nullptr) {
        return root->bounds;
    }
    return bbox();
}

template <class T>
vec3<T> rtree<T>::bbox_center(const bbox &b) const {
    return vec3<T>(
        (b.min.x + b.max.x) / static_cast<T>(2),
        (b.min.y + b.max.y) / static_cast<T>(2),
        (b.min.z + b.max.z) / static_cast<T>(2)
    );
}

#ifndef YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
    template class rtree<float>;
    template class rtree<double>;
#endif


