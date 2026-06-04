//YgorMeshesBoolean4.cc - Written by hal clark in 2026.
//
// Surface mesh Boolean engine backed by the BSP tree volume class.

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesBSPTree.h"
#include "YgorMeshesBoolean4.h"
#include "YgorMeshesVerification.h"


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp4(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation4 op) {
    if(lhs.faces.empty() || lhs.vertices.empty()) {
        switch(op) {
            case MeshBooleanOperation4::Union:
            case MeshBooleanOperation4::Exclusion:
                return rhs;
            case MeshBooleanOperation4::Intersection:
            case MeshBooleanOperation4::Subtraction:
                return fv_surface_mesh<T, I>();
        }
    }
    if(rhs.faces.empty() || rhs.vertices.empty()) {
        switch(op) {
            case MeshBooleanOperation4::Union:
                return lhs;
            case MeshBooleanOperation4::Intersection:
            case MeshBooleanOperation4::Subtraction:
                return fv_surface_mesh<T, I>();
            case MeshBooleanOperation4::Exclusion:
                return lhs;
        }
    }

    const auto lhs_tree = bsp_tree_volume<T, I>::from_fv_surface_mesh(lhs);
    const auto rhs_tree = bsp_tree_volume<T, I>::from_fv_surface_mesh(rhs);

    bsp_tree_volume<T, I> result_tree;
    switch(op) {
        case MeshBooleanOperation4::Union:
            result_tree = lhs_tree.boolean_union(rhs_tree);
            break;
        case MeshBooleanOperation4::Intersection:
            result_tree = lhs_tree.boolean_intersection(rhs_tree);
            break;
        case MeshBooleanOperation4::Exclusion:
            result_tree = lhs_tree.boolean_exclusion(rhs_tree);
            break;
        case MeshBooleanOperation4::Subtraction:
            result_tree = lhs_tree.boolean_subtraction(rhs_tree);
            break;
    }

    return result_tree.to_fv_surface_mesh();
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion4(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs) {
    return BooleanMeshOp4(lhs, rhs, MeshBooleanOperation4::Union);
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection4(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs) {
    return BooleanMeshOp4(lhs, rhs, MeshBooleanOperation4::Intersection);
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion4(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs) {
    return BooleanMeshOp4(lhs, rhs, MeshBooleanOperation4::Exclusion);
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction4(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs) {
    return BooleanMeshOp4(lhs, rhs, MeshBooleanOperation4::Subtraction);
}


// Explicit template instantiations.
#ifndef YGOR_MESHES_BOOLEAN4_DISABLE_ALL_SPECIALIZATIONS

template fv_surface_mesh<float,  uint32_t> BooleanMeshOp4       (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &, MeshBooleanOperation4);
template fv_surface_mesh<float,  uint32_t> BooleanUnion4        (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);
template fv_surface_mesh<float,  uint32_t> BooleanIntersection4 (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);
template fv_surface_mesh<float,  uint32_t> BooleanExclusion4    (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);
template fv_surface_mesh<float,  uint32_t> BooleanSubtraction4  (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);

template fv_surface_mesh<float,  uint64_t> BooleanMeshOp4       (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &, MeshBooleanOperation4);
template fv_surface_mesh<float,  uint64_t> BooleanUnion4        (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);
template fv_surface_mesh<float,  uint64_t> BooleanIntersection4 (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);
template fv_surface_mesh<float,  uint64_t> BooleanExclusion4    (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);
template fv_surface_mesh<float,  uint64_t> BooleanSubtraction4  (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);

template fv_surface_mesh<double, uint32_t> BooleanMeshOp4       (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, MeshBooleanOperation4);
template fv_surface_mesh<double, uint32_t> BooleanUnion4        (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);
template fv_surface_mesh<double, uint32_t> BooleanIntersection4 (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);
template fv_surface_mesh<double, uint32_t> BooleanExclusion4    (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);
template fv_surface_mesh<double, uint32_t> BooleanSubtraction4  (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);

template fv_surface_mesh<double, uint64_t> BooleanMeshOp4       (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, MeshBooleanOperation4);
template fv_surface_mesh<double, uint64_t> BooleanUnion4        (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);
template fv_surface_mesh<double, uint64_t> BooleanIntersection4 (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);
template fv_surface_mesh<double, uint64_t> BooleanExclusion4    (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);
template fv_surface_mesh<double, uint64_t> BooleanSubtraction4  (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);

#endif // YGOR_MESHES_BOOLEAN4_DISABLE_ALL_SPECIALIZATIONS
