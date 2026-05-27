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
#include "YgorIndex.h"
#include "YgorIndexRTree.h"

//#ifndef YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- rtree: R*-tree spatial indexing data structure -----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ Constructors -------------------------------------------------------

// R*-tree reinsertion percentage (fraction of entries to reinsert on overflow).
// Beckmann et al. recommend 30% for optimal R*-tree performance.
constexpr double RTREE_REINSERT_FRACTION = 0.3;

template <class T>
rtree<T>::rtree() : root(std::make_unique<leaf_node>()),
                    max_entries(8), min_entries(4), height(0), entry_count(0),
                    reinsert_count(3), in_reinsertion(false) { }

template <class T>
rtree<T>::rtree(size_t max_node_entries)
    : max_entries(max_node_entries), min_entries(max_node_entries / 2),
      height(0), entry_count(0), in_reinsertion(false) {
    if(max_entries < 2) {
        throw std::invalid_argument("Maximum entries per node must be at least 2");
    }
    reinsert_count = std::max(static_cast<size_t>(1),
                              static_cast<size_t>(max_entries * RTREE_REINSERT_FRACTION));
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
    insert(bbox(point, point), std::move(aux_data));
}

template <class T>
void rtree<T>::insert(const bbox &bb) {
    insert(bb, std::any{});
}

template <class T>
void rtree<T>::insert(const bbox &bb, std::any aux_data) {
    if(!bb.isfinite()) {
        throw std::invalid_argument("Cannot insert non-finite bbox into rtree");
    }

    in_reinsertion = false;
    insert_entry_at_leaf(entry(bb, std::move(aux_data)));
}

template <class T>
void rtree<T>::insert_entry_at_leaf(const entry &e) {
    leaf_node* leaf = choose_leaf(e.box);

    leaf->entries.push_back(e);
    update_bounds(leaf);

    entry_count++;

    const bool is_root = (leaf->parent == nullptr);

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
    const bool children_are_leaves = !internal->children.empty() && internal->children[0]->is_leaf();

    node_base* best_child = nullptr;

    if(children_are_leaves) {
        T min_overlap_increase = std::numeric_limits<T>::max();
        T min_volume_increase = std::numeric_limits<T>::max();
        T min_volume = std::numeric_limits<T>::max();

        for(auto& child_ptr : internal->children) {
            node_base* child = child_ptr.get();

            T current_overlap = static_cast<T>(0);
            T new_overlap = static_cast<T>(0);
            const bbox expanded_bounds = child->bounds.union_with(entry_box);

            for(auto& sibling_ptr : internal->children) {
                if(sibling_ptr.get() == child) continue;

                if(child->bounds.intersects(sibling_ptr->bounds)) {
                    current_overlap += child->bounds.intersection_with(sibling_ptr->bounds).volume();
                }
                if(expanded_bounds.intersects(sibling_ptr->bounds)) {
                    new_overlap += expanded_bounds.intersection_with(sibling_ptr->bounds).volume();
                }
            }

            const T overlap_increase = new_overlap - current_overlap;
            const T volume_increase = child->bounds.volume_increase(entry_box);
            const T volume = child->bounds.volume();

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
        T min_increase = std::numeric_limits<T>::max();
        T min_volume = std::numeric_limits<T>::max();

        for(auto& child_ptr : internal->children) {
            node_base* child = child_ptr.get();
            const T increase = child->bounds.volume_increase(entry_box);
            const T volume = child->bounds.volume();

            if(increase < min_increase ||
               (increase == min_increase && volume < min_volume)) {
                min_increase = increase;
                min_volume = volume;
                best_child = child;
            }
        }
    }

    if(best_child == nullptr) {
        throw std::runtime_error("No children nodes present; unable to locate optimal child");
    }

    return choose_subtree(best_child, entry_box, target_level, current_level + 1);
}

template <class T>
typename rtree<T>::leaf_node* rtree<T>::choose_leaf(const bbox &entry_box) {
    return static_cast<leaf_node*>(choose_subtree(root.get(), entry_box, height, 0));
}

template <class T>
std::unique_ptr<typename rtree<T>::node_base> rtree<T>::overflow_treatment(node_base* node, bool is_root) {
    if(!is_root && !in_reinsertion && node->is_leaf()) {
        in_reinsertion = true;
        reinsert_leaf(static_cast<leaf_node*>(node));
        return nullptr;
    }

    if(node->is_leaf()) {
        return split_leaf_node(static_cast<leaf_node*>(node));
    }
    return split_internal_node(static_cast<internal_node*>(node));
}

template <class T>
void rtree<T>::reinsert_leaf(leaf_node* node) {
    const vec3<T> center = node->bounds.center();

    std::vector<std::pair<T, entry>> sorted_entries;
    sorted_entries.reserve(node->entries.size());
    for(const auto& e : node->entries) {
        const T dist_sq = (e.box.center() - center).sq_length();
        sorted_entries.emplace_back(dist_sq, e);
    }

    std::sort(sorted_entries.begin(), sorted_entries.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<entry> to_reinsert;
    const size_t count = std::min(reinsert_count, sorted_entries.size());
    for(size_t i = 0; i < count; ++i) {
        to_reinsert.push_back(sorted_entries[i].second);
    }

    node->entries.clear();
    for(size_t i = count; i < sorted_entries.size(); ++i) {
        node->entries.push_back(sorted_entries[i].second);
    }

    update_bounds(node);
    adjust_tree(node, nullptr);

    entry_count -= to_reinsert.size();
    for(const auto& e : to_reinsert) {
        insert_entry_at_leaf(e);
    }
}

template <class T>
std::unique_ptr<typename rtree<T>::leaf_node> rtree<T>::split_leaf_node(leaf_node* node) {
    auto new_leaf = std::make_unique<leaf_node>();

    std::vector<bbox> entry_boxes;
    entry_boxes.reserve(node->entries.size());
    for(const auto& e : node->entries) {
        entry_boxes.push_back(e.box);
    }

    const int split_axis = choose_split_axis(entry_boxes);
    const size_t split_index = choose_split_index(entry_boxes, split_axis);

    std::vector<std::pair<T, size_t>> sorted_entries;
    sorted_entries.reserve(node->entries.size());
    for(size_t i = 0; i < node->entries.size(); ++i) {
        sorted_entries.emplace_back(bbox_axis_center(node->entries[i].box, split_axis), i);
    }
    std::sort(sorted_entries.begin(), sorted_entries.end());

    std::vector<entry> entries1, entries2;
    for(size_t i = 0; i < sorted_entries.size(); ++i) {
        if(i < split_index) {
            entries1.push_back(node->entries[sorted_entries[i].second]);
        } else {
            entries2.push_back(node->entries[sorted_entries[i].second]);
        }
    }

    node->entries = std::move(entries1);
    update_bounds(node);

    new_leaf->entries = std::move(entries2);
    update_bounds(new_leaf.get());

    new_leaf->parent = node->parent;

    return new_leaf;
}

template <class T>
int rtree<T>::choose_split_axis(const std::vector<bbox> &entries) const {
    T min_margin_sum = std::numeric_limits<T>::max();
    int best_axis = 0;

    for(int axis = 0; axis < 3; ++axis) {
        std::vector<std::pair<T, size_t>> sorted_by_lower;
        std::vector<std::pair<T, size_t>> sorted_by_upper;
        sorted_by_lower.reserve(entries.size());
        sorted_by_upper.reserve(entries.size());

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

        for(size_t k = min_entries; k <= entries.size() - min_entries; ++k) {
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

            first1 = true;
            first2 = true;
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
    T min_overlap = std::numeric_limits<T>::max();
    T min_volume = std::numeric_limits<T>::max();
    size_t best_k = (entries.size() + 1) / 2;

    std::vector<std::pair<T, size_t>> sorted_entries;
    sorted_entries.reserve(entries.size());
    for(size_t i = 0; i < entries.size(); ++i) {
        T coord;
        if(axis == 0) coord = entries[i].min.x;
        else if(axis == 1) coord = entries[i].min.y;
        else coord = entries[i].min.z;
        sorted_entries.emplace_back(coord, i);
    }
    std::sort(sorted_entries.begin(), sorted_entries.end());

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

        T overlap = static_cast<T>(0);
        if(b1.intersects(b2)) {
            overlap = b1.intersection_with(b2).volume();
        }

        const T total_volume = b1.volume() + b2.volume();

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

        if(split_node_ptr != nullptr) {
            split_node_ptr->parent = parent;
            parent->children.push_back(std::move(split_node_ptr));
            split_node_ptr = nullptr;
        }

        update_bounds(parent);

        if(parent->children.size() > max_entries) {
            const bool parent_is_root = (parent->parent == nullptr);
            auto new_split = overflow_treatment(parent, parent_is_root);

            if(parent == root.get()) {
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

    std::vector<bbox> child_boxes;
    child_boxes.reserve(node->children.size());
    for(const auto& child : node->children) {
        child_boxes.push_back(child->bounds);
    }

    const int split_axis = choose_split_axis(child_boxes);
    const size_t split_index = choose_split_index(child_boxes, split_axis);

    std::vector<std::pair<T, size_t>> sorted_indices;
    sorted_indices.reserve(node->children.size());
    for(size_t i = 0; i < node->children.size(); ++i) {
        sorted_indices.emplace_back(bbox_axis_center(node->children[i]->bounds, split_axis), i);
    }
    std::sort(sorted_indices.begin(), sorted_indices.end());

    std::vector<std::unique_ptr<node_base>> all_children;
    all_children.reserve(node->children.size());
    for(auto& child : node->children) {
        all_children.push_back(std::move(child));
    }
    node->children.clear();

    for(size_t i = 0; i < sorted_indices.size(); ++i) {
        const size_t idx = sorted_indices[i].second;
        if(i < split_index) {
            all_children[idx]->parent = node;
            node->children.push_back(std::move(all_children[idx]));
        } else {
            all_children[idx]->parent = new_internal.get();
            new_internal->children.push_back(std::move(all_children[idx]));
        }
    }

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
        if(!e.box.has_extent()) {
            results.push_back(e.box.min);
        }
    }
    return results;
}

template <class T>
std::vector<typename rtree<T>::bbox> rtree<T>::search_bboxes(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<bbox> results;
    results.reserve(entries.size());
    for(const auto &e : entries) {
        results.push_back(e.box);
    }
    return results;
}

template <class T>
std::vector<typename rtree<T>::bbox> rtree<T>::search_bboxes(const vec3<T> &query_point) const {
    auto entries = search(bbox(query_point, query_point));
    std::vector<bbox> results;
    results.reserve(entries.size());
    for(const auto &e : entries) {
        if(e.box.contains(query_point)) {
            results.push_back(e.box);
        }
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
            if(e.box.intersects(query_box)) {
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
    vec3<T> offset(radius, radius, radius);
    bbox query_box(center - offset, center + offset);

    auto candidates = search(query_box);

    std::vector<entry> results;
    const T radius_sq = radius * radius;
    for(const auto& e : candidates) {
        if(e.box.squared_distance_to(center) <= radius_sq) {
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
        if(!e.box.has_extent()) {
            results.push_back(e.box.min);
        }
    }
    return results;
}

template <class T>
std::vector<typename rtree<T>::entry> rtree<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    std::vector<std::pair<T, entry>> all_entries;

    std::function<void(const node_base*)> collect_all = [&](const node_base* node) {
        if(node->is_leaf()) {
            const leaf_node* leaf = static_cast<const leaf_node*>(node);
            for(const auto& e : leaf->entries) {
                all_entries.emplace_back(e.box.squared_distance_to(query_point), e);
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

    std::sort(all_entries.begin(), all_entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<entry> results;
    const size_t count = std::min(k, all_entries.size());
    for(size_t i = 0; i < count; ++i) {
        results.push_back(all_entries[i].second);
    }

    return results;
}

template <class T>
std::vector<vec3<T>> rtree<T>::nearest_neighbors_points(const vec3<T> &query_point, size_t k) const {
    if(k == 0) return {};
    auto entries = nearest_neighbors(query_point, entry_count);
    std::vector<vec3<T>> results;
    results.reserve(std::min(k, entries.size()));
    for(const auto& e : entries) {
        if(!e.box.has_extent()) {
            results.push_back(e.box.min);
            if(results.size() == k) break;
        }
    }
    return results;
}

template <class T>
bool rtree<T>::contains(const vec3<T> &point) const {
    return contains(bbox(point, point));
}

template <class T>
bool rtree<T>::contains(const bbox &bb) const {
    auto results = search(bb);
    for(const auto& result : results) {
        if(result.box == bb) {
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
            leaf->bounds = leaf->entries[0].box;
            for(size_t i = 1; i < leaf->entries.size(); ++i) {
                leaf->bounds.expand(leaf->entries[i].box);
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
T rtree<T>::bbox_axis_center(const bbox &b, int axis) const {
    if(axis == 0) return (b.min.x + b.max.x) / static_cast<T>(2);
    if(axis == 1) return (b.min.y + b.max.y) / static_cast<T>(2);
    return (b.min.z + b.max.z) / static_cast<T>(2);
}

#ifndef YGOR_INDEX_RTREE_DISABLE_ALL_SPECIALIZATIONS
    template class rtree<float>;
    template class rtree<double>;
#endif
