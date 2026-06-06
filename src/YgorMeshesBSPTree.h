//YgorMeshesBSPTree.h - Written by hal clark in 2026.
//
// Binary Space Partitioning (BSP) tree for representing solid 3D volumes.
// The tree partitions space using oriented planes and classifies each
// resulting convex subspace as either IN (solid) or OUT (empty).
//
// Boolean operations (union, intersection, subtraction, exclusion) are
// implemented using the standard BSP merge algorithm following Naylor (1990).
//
// Conversions to/from fv_surface_mesh<T,I> are provided for interoperability
// with the rest of the Ygor ecosystem.

#pragma once
#ifndef YGOR_MESHES_BSP_TREE_HDR_GRD_H
#define YGOR_MESHES_BSP_TREE_HDR_GRD_H

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesAdaptivePredicates.h"


template <class T>
struct bsp_plane {
    vec3<T> anchors[3];

    bsp_plane() = default;
    bsp_plane(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c)
        : anchors{a, b, c} {}

    vec3<T> normal() const {
        return (anchors[1] - anchors[0]).Cross(anchors[2] - anchors[0]);
    }
    vec3<T> unit_normal() const {
        return normal().unit();
    }
    vec3<T> centroid() const {
        return (anchors[0] + anchors[1] + anchors[2])
             * (static_cast<T>(1) / static_cast<T>(3));
    }
};


template <class T, class I>
class bsp_tree_volume {
    public:
        using value_type = T;
        using index_type = I;

        enum class NodeType : uint8_t {
            Partition = 0,
            In        = 1,
            Out       = 2
        };

        struct Node {
            NodeType type = NodeType::Out;
            bsp_plane<T> partition_plane;
            std::unique_ptr<Node> front;
            std::unique_ptr<Node> back;

            Node();
            explicit Node(NodeType t);
            Node(const bsp_plane<T> &p,
                 std::unique_ptr<Node> f,
                 std::unique_ptr<Node> b);
            Node(const Node &) = delete;
            Node &operator=(const Node &) = delete;
            Node(Node &&) = default;
            Node &operator=(Node &&) = default;

            Node *clone() const;
        };

    private:
        std::unique_ptr<Node> root;

    public:
        bsp_tree_volume();
        explicit bsp_tree_volume(std::unique_ptr<Node> r);
        bsp_tree_volume(const bsp_tree_volume &);
        bsp_tree_volume &operator=(const bsp_tree_volume &);
        bsp_tree_volume(bsp_tree_volume &&) = default;
        bsp_tree_volume &operator=(bsp_tree_volume &&) = default;

        bool empty() const;
        const Node *get_root() const;

        // ---- Boolean operations ----
        bsp_tree_volume boolean_union(const bsp_tree_volume &other) const;
        bsp_tree_volume boolean_intersection(const bsp_tree_volume &other) const;
        bsp_tree_volume boolean_subtraction(const bsp_tree_volume &other) const;
        bsp_tree_volume boolean_exclusion(const bsp_tree_volume &other) const;

        // ---- Conversions ----
        static bsp_tree_volume from_fv_surface_mesh(const fv_surface_mesh<T, I> &mesh,
                                                     std::optional<uint64_t> seed = std::nullopt);
        fv_surface_mesh<T, I> to_fv_surface_mesh() const;
};


#endif // YGOR_MESHES_BSP_TREE_HDR_GRD_H
