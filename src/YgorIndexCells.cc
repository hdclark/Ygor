//YgorIndexCells.cc.

#include <algorithm>
#include <any>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"
#include "YgorIndexCells.h"

//#ifndef YGOR_INDEX_CELLS_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_CELLS_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//--------------------------- cells_index: cell-based spatial indexing data structure ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ Private helpers ----------------------------------------------------

template <class T>
typename cells_index<T>::cell_t cells_index<T>::make_cell(const vec3<T> &v) const {
    return { static_cast<int64_t>(std::floor(v.x * inv_cell_size)),
             static_cast<int64_t>(std::floor(v.y * inv_cell_size)),
             static_cast<int64_t>(std::floor(v.z * inv_cell_size)) };
}

template <class T>
void cells_index<T>::update_bounds(const vec3<T> &point) {
    if(!bounds_initialized){
        bounds = bbox(point, point);
        bounds_initialized = true;
    }else{
        bounds.expand(point);
    }
}

//------------------------------------------------------ Constructors -------------------------------------------------------

template <class T>
cells_index<T>::cells_index() : cell_size(static_cast<T>(1)),
                                inv_cell_size(static_cast<T>(1)),
                                entry_count(0),
                                bounds_initialized(false) { }

template <class T>
cells_index<T>::cells_index(T cs) : cell_size(cs),
                                    entry_count(0),
                                    bounds_initialized(false) {
    if(!(static_cast<T>(0) < cs)){
        throw std::invalid_argument("Cell size must be positive");
    }
    inv_cell_size = static_cast<T>(1) / cs;
}

template <class T>
cells_index<T>::~cells_index() = default;

//------------------------------------------------------ Member functions ---------------------------------------------------

template <class T>
void cells_index<T>::insert(const vec3<T> &point) {
    insert(point, std::any{});
}

template <class T>
void cells_index<T>::insert(const vec3<T> &point, std::any aux_data) {
    entry e(point, std::move(aux_data));
    const auto c = make_cell(point);
    bins[c].push_back(std::move(e));
    ++entry_count;
    update_bounds(point);
}

template <class T>
std::vector<typename cells_index<T>::entry> cells_index<T>::search(const bbox &query_box) const {
    std::vector<entry> results;
    
    const auto c_min = make_cell(query_box.min);
    const auto c_max = make_cell(query_box.max);
    
    for(int64_t cx = c_min.x; cx <= c_max.x; ++cx){
        for(int64_t cy = c_min.y; cy <= c_max.y; ++cy){
            for(int64_t cz = c_min.z; cz <= c_max.z; ++cz){
                const cell_t c = { cx, cy, cz };
                const auto it = bins.find(c);
                if(it == std::end(bins)) continue;
                
                for(const auto &e : it->second){
                    if(query_box.contains(e.point)){
                        results.push_back(e);
                    }
                }
            }
        }
    }
    return results;
}

template <class T>
std::vector<vec3<T>> cells_index<T>::search_points(const bbox &query_box) const {
    auto entries = search(query_box);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename cells_index<T>::entry> cells_index<T>::search_radius(const vec3<T> &center, T radius) const {
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
std::vector<vec3<T>> cells_index<T>::search_radius_points(const vec3<T> &center, T radius) const {
    auto entries = search_radius(center, radius);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
std::vector<typename cells_index<T>::entry> cells_index<T>::nearest_neighbors(const vec3<T> &query_point, size_t k) const {
    if(k == 0 || entry_count == 0) return {};
    
    // Expand search radius until we find at least k results.
    // Start with one cell neighbourhood and double until sufficient.
    T search_radius = cell_size;
    std::vector<std::pair<T, entry>> candidates;
    
    for(int attempt = 0; attempt < 64; ++attempt){
        candidates.clear();
        
        vec3<T> offset(search_radius, search_radius, search_radius);
        bbox query_box(query_point - offset, query_point + offset);
        
        const auto c_min = make_cell(query_box.min);
        const auto c_max = make_cell(query_box.max);
        
        for(int64_t cx = c_min.x; cx <= c_max.x; ++cx){
            for(int64_t cy = c_min.y; cy <= c_max.y; ++cy){
                for(int64_t cz = c_min.z; cz <= c_max.z; ++cz){
                    const cell_t c = { cx, cy, cz };
                    const auto it = bins.find(c);
                    if(it == std::end(bins)) continue;
                    
                    for(const auto &e : it->second){
                        T dist_sq = (e.point - query_point).sq_length();
                        candidates.emplace_back(dist_sq, e);
                    }
                }
            }
        }
        
        if(candidates.size() >= k) break;
        search_radius *= static_cast<T>(2);
    }
    
    // If we still don't have enough, scan all bins.
    if(candidates.size() < k){
        candidates.clear();
        for(const auto &bin : bins){
            for(const auto &e : bin.second){
                T dist_sq = (e.point - query_point).sq_length();
                candidates.emplace_back(dist_sq, e);
            }
        }
    }
    
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });
    
    std::vector<entry> results;
    size_t count = std::min(k, candidates.size());
    for(size_t i = 0; i < count; ++i){
        results.push_back(candidates[i].second);
    }
    return results;
}

template <class T>
std::vector<vec3<T>> cells_index<T>::nearest_neighbors_points(const vec3<T> &query_point, size_t k) const {
    auto entries = nearest_neighbors(query_point, k);
    std::vector<vec3<T>> results;
    results.reserve(entries.size());
    for(const auto& e : entries){
        results.push_back(e.point);
    }
    return results;
}

template <class T>
bool cells_index<T>::contains(const vec3<T> &point) const {
    const auto c = make_cell(point);
    
    const T epsilon = std::numeric_limits<T>::epsilon() * static_cast<T>(10);
    
    // Scan 3x3x3 neighbourhood to handle boundary cases.
    for(int64_t dx = -1L; dx <= 1L; ++dx){
        for(int64_t dy = -1L; dy <= 1L; ++dy){
            for(int64_t dz = -1L; dz <= 1L; ++dz){
                const cell_t c_n = { c.x + dx, c.y + dy, c.z + dz };
                const auto it = bins.find(c_n);
                if(it == std::end(bins)) continue;
                
                for(const auto &e : it->second){
                    if(std::abs(e.point.x - point.x) <= epsilon &&
                       std::abs(e.point.y - point.y) <= epsilon &&
                       std::abs(e.point.z - point.z) <= epsilon){
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

template <class T>
void cells_index<T>::clear() {
    bins.clear();
    entry_count = 0;
    bounds_initialized = false;
    bounds = bbox();
}

template <class T>
size_t cells_index<T>::get_size() const {
    return entry_count;
}

template <class T>
typename cells_index<T>::bbox cells_index<T>::get_bounds() const {
    return bounds;
}

#ifndef YGOR_INDEX_CELLS_DISABLE_ALL_SPECIALIZATIONS
    template class cells_index<float>;
    template class cells_index<double>;
#endif

