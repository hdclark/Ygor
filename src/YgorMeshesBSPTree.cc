//YgorMeshesBSPTree.cc - Written by hal clark in 2026.
//
// Binary Space Partitioning tree for 3D solid volume representation.
// Uses Shewchuk-style adaptive predicates for robust geometric decisions.
// Boolean operations follow Naylor's BSP merge algorithm.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesAdaptivePredicates.h"
#include "YgorMeshesBSPTree.h"
#include "YgorMeshesOrient.h"
#include "YgorMeshesVerification.h"


namespace {

template <class T>
constexpr T plane_threshold() {
    return std::numeric_limits<T>::epsilon() * static_cast<T>(1024);
}

template <class T>
int classify_point(const bsp_plane<T> &P, const vec3<T> &v) {
    const T pa[3] = { P.anchors[0].x, P.anchors[0].y, P.anchors[0].z };
    const T pb[3] = { P.anchors[1].x, P.anchors[1].y, P.anchors[1].z };
    const T pc[3] = { P.anchors[2].x, P.anchors[2].y, P.anchors[2].z };
    const T pd[3] = { v.x, v.y, v.z };
    const double s = adaptive_predicate::orient3d(pa, pb, pc, pd);
    return (s > 0.0) ? +1 : ((s < 0.0) ? -1 : 0);
}

enum class TriClass : uint8_t {
    Front    = 0,
    Back     = 1,
    Coplanar = 2,
    Spanning = 3
};

template <class T>
TriClass classify_triangle(const bsp_plane<T> &P,
                           const vec3<T> &a,
                           const vec3<T> &b,
                           const vec3<T> &c) {
    const int s[3] = {
        classify_point(P, a),
        classify_point(P, b),
        classify_point(P, c)
    };
    if(s[0] == 0 && s[1] == 0 && s[2] == 0) return TriClass::Coplanar;
    bool has_pos = false, has_neg = false;
    for(int i = 0; i < 3; ++i) {
        if(s[i] > 0) has_pos = true;
        if(s[i] < 0) has_neg = true;
    }
    if(has_pos && has_neg) return TriClass::Spanning;
    if(has_neg) return TriClass::Back;
    return TriClass::Front;
}

template <class T>
vec3<T> intersect_edge_with_plane(const bsp_plane<T> &P,
                                  const vec3<T> &a,
                                  const vec3<T> &b) {
    const T pa[3] = { P.anchors[0].x, P.anchors[0].y, P.anchors[0].z };
    const T pb[3] = { P.anchors[1].x, P.anchors[1].y, P.anchors[1].z };
    const T pc[3] = { P.anchors[2].x, P.anchors[2].y, P.anchors[2].z };
    const T aa[3] = { a.x, a.y, a.z };
    const T ba[3] = { b.x, b.y, b.z };
    const T d0 = adaptive_predicate::orient3d(pa, pb, pc, aa);
    const T d1 = adaptive_predicate::orient3d(pa, pb, pc, ba);
    const T denom = d0 - d1;
    const T eps = std::numeric_limits<T>::epsilon() * (std::abs(d0) + std::abs(d1));
    if(std::abs(denom) < eps) {
        return (a + b) * static_cast<T>(0.5);
    }
    const T t = d0 / denom;
    return a + (b - a) * t;
}

template <class T>
void split_triangle(const bsp_plane<T> &P,
                    const vec3<T> &a,
                    const vec3<T> &b,
                    const vec3<T> &c,
                    std::vector<std::array<vec3<T>, 3>> &front,
                    std::vector<std::array<vec3<T>, 3>> &back) {
    const int sgn[3] = {
        classify_point(P, a),
        classify_point(P, b),
        classify_point(P, c)
    };
    if(sgn[0] >= 0 && sgn[1] >= 0 && sgn[2] >= 0) {
        if(sgn[0] > 0 || sgn[1] > 0 || sgn[2] > 0)
            front.push_back({{a, b, c}});
        return;
    }
    if(sgn[0] <= 0 && sgn[1] <= 0 && sgn[2] <= 0) {
        if(sgn[0] < 0 || sgn[1] < 0 || sgn[2] < 0)
            back.push_back({{a, b, c}});
        return;
    }
    std::vector<vec3<T>> pos, neg;
    if(sgn[0] >= 0) pos.push_back(a); else neg.push_back(a);
    if(sgn[1] >= 0) pos.push_back(b); else neg.push_back(b);
    if(sgn[2] >= 0) pos.push_back(c); else neg.push_back(c);
    if(pos.size() == 1u && neg.size() == 2u) {
        const vec3<T> i0 = intersect_edge_with_plane(P, pos[0], neg[0]);
        const vec3<T> i1 = intersect_edge_with_plane(P, pos[0], neg[1]);
        front.push_back({{pos[0], i0, i1}});
        back.push_back({{neg[0], neg[1], i1}});
        back.push_back({{neg[0], i1, i0}});
    } else {
        const vec3<T> i0 = intersect_edge_with_plane(P, neg[0], pos[0]);
        const vec3<T> i1 = intersect_edge_with_plane(P, neg[0], pos[1]);
        front.push_back({{pos[0], pos[1], i1}});
        front.push_back({{pos[0], i1, i0}});
        back.push_back({{neg[0], i0, i1}});
    }
}

template <class T>
bsp_plane<T> plane_from_triangle(const vec3<T> &a,
                                 const vec3<T> &b,
                                 const vec3<T> &c) {
    return bsp_plane<T>(a, b, c);
}

// ---- Ray-casting for IN/OUT classification ----

template <class T>
bool ray_intersects_triangle(const vec3<T> &origin,
                             const vec3<T> &direction,
                             const vec3<T> &v0,
                             const vec3<T> &v1,
                             const vec3<T> &v2) {
    const vec3<T> e1 = v1 - v0;
    const vec3<T> e2 = v2 - v0;
    const vec3<T> h = direction.Cross(e2);
    const T a = e1.Dot(h);
    if(std::abs(a) < plane_threshold<T>()) return false;
    const T f = static_cast<T>(1) / a;
    const vec3<T> s = origin - v0;
    const T u = f * s.Dot(h);
    if(u < plane_threshold<T>() || u > (static_cast<T>(1) - plane_threshold<T>())) return false;
    const vec3<T> q = s.Cross(e1);
    const T v = f * direction.Dot(q);
    if(v < plane_threshold<T>() || (u + v) > (static_cast<T>(1) - plane_threshold<T>())) return false;
    const T t = f * e2.Dot(q);
    return (t > plane_threshold<T>());
}

template <class T>
int64_t count_ray_intersections(
    const std::vector<std::array<vec3<T>, 3>> &tris,
    const vec3<T> &origin,
    const vec3<T> &direction) {
    int64_t count = 0;
    for(const auto &tri : tris) {
        if(ray_intersects_triangle(origin, direction,
                                   tri[0], tri[1], tri[2])) {
            ++count;
        }
    }
    return count;
}

template <class T>
bool point_is_inside_mesh(
    const std::vector<std::array<vec3<T>, 3>> &tris,
    const vec3<T> &pt) {
    const vec3<T> dir(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
    const int64_t cnt = count_ray_intersections(tris, pt, dir);
    return (cnt % 2) == 1;
}

// ---- BSP Tree: clone and complement ----

template <class T, class I>
using NodePtr = std::unique_ptr<typename bsp_tree_volume<T, I>::Node>;
template <class T, class I>
using NodeType = typename bsp_tree_volume<T, I>::NodeType;

template <class T, class I>
NodePtr<T, I> clone_node(const typename bsp_tree_volume<T, I>::Node *n) {
    if(!n) return nullptr;
    return NodePtr<T, I>(n->clone());
}

template <class T, class I>
NodePtr<T, I> complement_tree(NodePtr<T, I> node) {
    if(!node) return nullptr;
    if(node->type == NodeType<T, I>::In)
        return std::make_unique<typename bsp_tree_volume<T, I>::Node>(NodeType<T, I>::Out);
    if(node->type == NodeType<T, I>::Out)
        return std::make_unique<typename bsp_tree_volume<T, I>::Node>(NodeType<T, I>::In);

    auto fc = complement_tree<T, I>(std::move(node->front));
    auto bc = complement_tree<T, I>(std::move(node->back));

    if(fc && bc &&
       fc->type != NodeType<T, I>::Partition &&
       bc->type != NodeType<T, I>::Partition &&
       fc->type == bc->type) {
        return fc;
    }
    return std::make_unique<typename bsp_tree_volume<T, I>::Node>(
        node->partition_plane,
        std::move(fc),
        std::move(bc)
    );
}

// Partition a BSP tree by a plane.
// Returns (front_fragment, back_fragment).
template <class T, class I>
std::pair<NodePtr<T, I>, NodePtr<T, I>>
partition_tree(const bsp_plane<T> &P, NodePtr<T, I> node) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    if(!node) {
        return {nullptr, nullptr};
    }
    if(node->type != NT::Partition) {
        // IN or OUT leaf: both fragments inherit the leaf type.
        auto nt = node->type;
        return {std::make_unique<Node>(nt), std::make_unique<Node>(nt)};
    }

    const bsp_plane<T> &Q = node->partition_plane;

    auto [ff, fb] = partition_tree<T, I>(P, std::move(node->front));
    auto [bf, bb] = partition_tree<T, I>(P, std::move(node->back));

    // Build front fragment: Q with front=ff, back=bf
    NodePtr<T, I> front_result;
    {
        const bool ff_partition = ff && ff->type == NT::Partition;
        const bool bf_partition = bf && bf->type == NT::Partition;
        const bool ff_is_out = !ff || ff->type == NT::Out;
        const bool bf_is_out = !bf || bf->type == NT::Out;
        if(!ff_partition && !bf_partition && ff_is_out == bf_is_out) {
            front_result = (ff ? std::move(ff) : std::move(bf));
            if(!front_result) front_result = std::make_unique<Node>(NT::Out);
        } else {
            front_result = std::make_unique<Node>(Q, std::move(ff), std::move(bf));
        }
    }

    // Build back fragment: Q with front=fb, back=bb
    NodePtr<T, I> back_result;
    {
        const bool fb_partition = fb && fb->type == NT::Partition;
        const bool bb_partition = bb && bb->type == NT::Partition;
        const bool fb_is_out = !fb || fb->type == NT::Out;
        const bool bb_is_out = !bb || bb->type == NT::Out;
        if(!fb_partition && !bb_partition && fb_is_out == bb_is_out) {
            back_result = (fb ? std::move(fb) : std::move(bb));
            if(!back_result) back_result = std::make_unique<Node>(NT::Out);
        } else {
            back_result = std::make_unique<Node>(Q, std::move(fb), std::move(bb));
        }
    }

    return {std::move(front_result), std::move(back_result)};
}

// Collapse partition nodes where both subtrees evaluate to the same leaf type.
template <class T, class I>
NodePtr<T, I> collapse_uniform(NodePtr<T, I> node) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    if(!node) return nullptr;
    if(node->type != NT::Partition) return node;

    node->front = collapse_uniform<T, I>(std::move(node->front));
    node->back  = collapse_uniform<T, I>(std::move(node->back));

    const bool front_part = node->front && node->front->type == NT::Partition;
    const bool back_part  = node->back  && node->back->type  == NT::Partition;
    const bool front_is_out = !node->front || node->front->type == NT::Out;
    const bool back_is_out  = !node->back  || node->back->type  == NT::Out;

    if(!front_part && !back_part && front_is_out == back_is_out) {
        auto result = (node->front ? std::move(node->front) : std::move(node->back));
        if(!result) result = std::make_unique<Node>(NT::Out);
        return result;
    }
    return node;
}

// Evaluate the subtree rooted at |n| to determine if it is uniformly IN,
// uniformly OUT, or mixed.  Returns In/Out for uniform subtrees, or
// Partition for non-uniform ones.
template <class T, class I>
typename bsp_tree_volume<T, I>::NodeType
subtree_uniform_eval(const typename bsp_tree_volume<T, I>::Node *n) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!n) return NT::Out;
    if(n->type == NT::In || n->type == NT::Out) return n->type;
    const NT fe = subtree_uniform_eval<T, I>(n->front.get());
    if(fe == NT::Partition) return NT::Partition;
    const NT be = subtree_uniform_eval<T, I>(n->back.get());
    if(be == NT::Partition) return NT::Partition;
    return (fe == be) ? fe : NT::Partition;
}

// Deep collapse: collapse a partition node when BOTH children evaluate
// to the same uniform leaf type, even if the children themselves are
// still partition nodes.
template <class T, class I>
NodePtr<T, I> collapse_deep_uniform(NodePtr<T, I> node) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    if(!node) return nullptr;
    if(node->type != NT::Partition) return node;

    node->front = collapse_deep_uniform<T, I>(std::move(node->front));
    node->back  = collapse_deep_uniform<T, I>(std::move(node->back));

    const NT fe = subtree_uniform_eval<T, I>(node->front.get());
    if(fe == NT::Partition) return node;
    const NT be = subtree_uniform_eval<T, I>(node->back.get());
    if(be == NT::Partition) return node;
    if(fe != be) return node;

    auto result = (node->front ? std::move(node->front) : std::move(node->back));
    if(!result) result = std::make_unique<Node>(NT::Out);
    return result;
}

// Merge two BSP trees with a boolean operation.
// op: 0 = union, 1 = intersection, 2 = subtraction (A-B)
template <class T, class I>
NodePtr<T, I> merge_bsp(NodePtr<T, I> A, NodePtr<T, I> B, int op) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    // Handle leaf cases for A.
    if(!A || A->type != NT::Partition) {
        const bool a_in = (A && A->type == NT::In);
        switch(op) {
            case 0: // union: IN ∪ B = IN, OUT ∪ B = B
                return a_in ? std::make_unique<Node>(NT::In)
                            : (B ? clone_node<T, I>(B.get()) : nullptr);
            case 1: // intersection: IN ∩ B = B, OUT ∩ B = OUT
                return a_in ? (B ? clone_node<T, I>(B.get()) : nullptr)
                            : std::make_unique<Node>(NT::Out);
            case 2: // subtraction A-B: IN - B = !B, OUT - B = OUT
                return a_in ? complement_tree<T, I>(std::move(B))
                            : std::make_unique<Node>(NT::Out);
        }
    }

    // Handle leaf cases for B.
    if(!B || B->type != NT::Partition) {
        const bool b_in = (B && B->type == NT::In);
        switch(op) {
            case 0: // union: A ∪ IN = IN
                return b_in ? std::make_unique<Node>(NT::In)
                            : (A ? clone_node<T, I>(A.get()) : nullptr);
            case 1: // intersection: A ∩ IN = A
                return b_in ? (A ? clone_node<T, I>(A.get()) : nullptr)
                            : std::make_unique<Node>(NT::Out);
            case 2: // subtraction A-B: A - IN = OUT, A - OUT = A
                return b_in ? std::make_unique<Node>(NT::Out)
                            : (A ? clone_node<T, I>(A.get()) : nullptr);
        }
    }

    // Both are partition nodes. Use A's plane to partition B.
    const bsp_plane<T> &P = A->partition_plane;
    auto [B_front, B_back] = partition_tree<T, I>(P, std::move(B));

    auto new_front = merge_bsp<T, I>(clone_node<T, I>(A->front.get()),
                                     std::move(B_front), op);
    auto new_back  = merge_bsp<T, I>(clone_node<T, I>(A->back.get()),
                                     std::move(B_back), op);

    // Collapse redundant partitions.
    {
        const bool nf_partition = new_front && new_front->type == NT::Partition;
        const bool nb_partition = new_back && new_back->type == NT::Partition;
        const bool nf_is_out = !new_front || new_front->type == NT::Out;
        const bool nb_is_out = !new_back || new_back->type == NT::Out;
        if(!nf_partition && !nb_partition && nf_is_out == nb_is_out) {
            auto result = (new_front ? std::move(new_front) : std::move(new_back));
            if(!result) result = std::make_unique<Node>(NT::Out);
            return result;
        }
    }

    return std::make_unique<Node>(P, std::move(new_front), std::move(new_back));
}


// ---- Mesh -> BSP conversion helpers ----

template <class T>
struct TriangleRec {
    std::array<vec3<T>, 3> v;
    bsp_plane<T> pl;
};

// Compute an approximate extent of all input triangles, used to derive
// a mesh-scale-tuned offset for ray-cast test points so that they are not
// placed within machine epsilon of a boundary face.
template <class T>
T compute_all_tris_extent(const std::vector<std::array<vec3<T>, 3>> &tris) {
    vec3<T> bb_min( std::numeric_limits<T>::max(),
                     std::numeric_limits<T>::max(),
                     std::numeric_limits<T>::max());
    vec3<T> bb_max(-std::numeric_limits<T>::max(),
                   -std::numeric_limits<T>::max(),
                   -std::numeric_limits<T>::max());
    for(const auto &tri : tris) {
        for(const auto &v : tri) {
            bb_min.x = std::min(bb_min.x, v.x);
            bb_min.y = std::min(bb_min.y, v.y);
            bb_min.z = std::min(bb_min.z, v.z);
            bb_max.x = std::max(bb_max.x, v.x);
            bb_max.y = std::max(bb_max.y, v.y);
            bb_max.z = std::max(bb_max.z, v.z);
        }
    }
    const vec3<T> extent = bb_max - bb_min;
    return std::max({std::abs(extent.x), std::abs(extent.y), std::abs(extent.z), static_cast<T>(1)});
}

template <class T, class I>
NodePtr<T, I> build_bsp_from_triangles(
    std::vector<TriangleRec<T>> tris,
    const std::vector<std::array<vec3<T>, 3>> &all_tris,
    size_t depth);

template <class T, class I>
NodePtr<T, I> build_bsp_from_triangles(
    std::vector<TriangleRec<T>> tris,
    const std::vector<std::array<vec3<T>, 3>> &all_tris,
    size_t depth) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    constexpr size_t max_depth = 64;

    if(tris.empty() || depth > max_depth) {
        return std::make_unique<Node>(NT::Out);
    }

    if(depth == 0) {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::shuffle(tris.begin(), tris.end(), gen);
    }

    const bsp_plane<T> P = tris[0].pl;

    std::vector<TriangleRec<T>> front_tris, back_tris;
    size_t front_non_coplanar = 0;
    size_t back_non_coplanar = 0;
    size_t coplanar_cnt = 0;

    for(const auto &tri : tris) {
        const TriClass tc = classify_triangle(P, tri.v[0], tri.v[1], tri.v[2]);
        switch(tc) {
            case TriClass::Front:
                front_tris.push_back(tri);
                ++front_non_coplanar;
                break;
            case TriClass::Back:
                back_tris.push_back(tri);
                ++back_non_coplanar;
                break;
            case TriClass::Coplanar:
                front_tris.push_back(tri);
                ++coplanar_cnt;
                break;
            case TriClass::Spanning: {
                std::vector<std::array<vec3<T>, 3>> front_parts, back_parts;
                split_triangle(P, tri.v[0], tri.v[1], tri.v[2],
                               front_parts, back_parts);
                for(auto &fp : front_parts) {
                    TriangleRec<T> r;
                    r.v = fp;
                    r.pl = plane_from_triangle(fp[0], fp[1], fp[2]);
                    front_tris.push_back(r);
                    ++front_non_coplanar;
                }
                for(auto &bp : back_parts) {
                    TriangleRec<T> r;
                    r.v = bp;
                    r.pl = plane_from_triangle(bp[0], bp[1], bp[2]);
                    back_tris.push_back(r);
                    ++back_non_coplanar;
                }
                break;
            }
        }
    }

    // All triangles are coplanar with the partition plane: degenerate case.
    // The mesh lies entirely in this plane. Create a Partition node with
    // classified IN/OUT leaves on each side.
    if(coplanar_cnt == tris.size()) {
        const vec3<T> front_pt = P.centroid() + P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
        const vec3<T> back_pt  = P.centroid() - P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
        const bool front_inside = point_is_inside_mesh(all_tris, front_pt);
        const bool back_inside  = point_is_inside_mesh(all_tris, back_pt);

        auto fc = front_inside ? std::make_unique<Node>(NT::In)
                               : std::make_unique<Node>(NT::Out);
        auto bc = back_inside ? std::make_unique<Node>(NT::In)
                              : std::make_unique<Node>(NT::Out);

        if(fc->type == bc->type) {
            return fc;
        }
        return std::make_unique<Node>(P, std::move(fc), std::move(bc));
    }

    // Both sides have only coplanar triangles.
    // Ray-cast both sides to determine IN/OUT leaves.
    if(front_non_coplanar == 0 && back_non_coplanar == 0) {
        const vec3<T> front_pt = P.centroid() + P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
        const vec3<T> back_pt  = P.centroid() - P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
        const bool front_inside = point_is_inside_mesh(all_tris, front_pt);
        const bool back_inside  = point_is_inside_mesh(all_tris, back_pt);

        auto fc = front_inside ? std::make_unique<Node>(NT::In)
                               : std::make_unique<Node>(NT::Out);
        auto bc = back_inside ? std::make_unique<Node>(NT::In)
                              : std::make_unique<Node>(NT::Out);

        if(fc->type == bc->type) {
            return fc;
        }
        return std::make_unique<Node>(P, std::move(fc), std::move(bc));
    }

    // Front side has only coplanar triangles. Ray-cast front as leaf, recurse back.
    if(front_non_coplanar == 0) {
        auto b_child = [&]() -> NodePtr<T, I> {
            if(back_tris.empty()) {
                const vec3<T> back_pt = P.centroid() - P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
                const bool back_inside = point_is_inside_mesh(all_tris, back_pt);
                return std::make_unique<Node>(back_inside ? NT::In : NT::Out);
            }
            return build_bsp_from_triangles<T, I>(
                std::move(back_tris), all_tris, depth + 1);
        }();
        const vec3<T> front_pt = P.centroid() + P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
        const bool front_inside = point_is_inside_mesh(all_tris, front_pt);
        auto f_child = front_inside ? std::make_unique<Node>(NT::In)
                                    : std::make_unique<Node>(NT::Out);

        if(b_child->type != NT::Partition && b_child->type == f_child->type) {
            return b_child;
        }
        return std::make_unique<Node>(P, std::move(f_child), std::move(b_child));
    }

    // Back side has only coplanar triangles. Ray-cast back as leaf, recurse front.
    // Strip coplanar triangles from front_tris before recursing to avoid infinite chain.
    if(back_non_coplanar == 0) {
        const vec3<T> back_pt = P.centroid() - P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
        const bool back_inside = point_is_inside_mesh(all_tris, back_pt);
        auto b_child = back_inside ? std::make_unique<Node>(NT::In)
                                   : std::make_unique<Node>(NT::Out);

        // Remove coplanar triangles from front_tris — they are on the same plane
        // as the partition and would only cause degenerate recursion.
        front_tris.erase(
            std::remove_if(front_tris.begin(), front_tris.end(),
                [&](const TriangleRec<T> &tri) {
                    return classify_triangle(P, tri.v[0], tri.v[1], tri.v[2]) == TriClass::Coplanar;
                }),
            front_tris.end());

        auto f_child = [&]() -> NodePtr<T, I> {
            if(front_tris.empty()) {
                const vec3<T> front_pt = P.centroid() + P.unit_normal() * plane_threshold<T>() * static_cast<T>(2);
                const bool front_inside = point_is_inside_mesh(all_tris, front_pt);
                return std::make_unique<Node>(front_inside ? NT::In : NT::Out);
            }
            return build_bsp_from_triangles<T, I>(
                std::move(front_tris), all_tris, depth + 1);
        }();

        if(f_child->type != NT::Partition && f_child->type == b_child->type) {
            return f_child;
        }
        return std::make_unique<Node>(P, std::move(f_child), std::move(b_child));
    }

    // Both sides have non-coplanar triangles - recurse on both.
    auto f_child = build_bsp_from_triangles<T, I>(
        std::move(front_tris), all_tris, depth + 1);
    auto b_child = build_bsp_from_triangles<T, I>(
        std::move(back_tris), all_tris, depth + 1);

    if(f_child && b_child &&
       f_child->type != NT::Partition &&
       b_child->type != NT::Partition &&
       f_child->type == b_child->type) {
        return f_child;
    }

    return std::make_unique<Node>(P, std::move(f_child), std::move(b_child));
}


// ---- BSP -> Mesh conversion helpers ----

template <class T>
struct PolyFace {
    std::vector<vec3<T>> vertices;
    vec3<T> normal;
};

template <class T>
void clip_polygon_by_plane(const bsp_plane<T> &P,
                           const std::vector<vec3<T>> &poly,
                           std::vector<vec3<T>> &front,
                           std::vector<vec3<T>> &back) {
    if(poly.size() < 3) return;
    const size_t n = poly.size();
    for(size_t i = 0; i < n; ++i) {
        const vec3<T> &curr = poly[i];
        const vec3<T> &next = poly[(i + 1) % n];
        const int sc = classify_point(P, curr);
        const int sn = classify_point(P, next);
        if(sc >= 0) front.push_back(curr);
        if(sc <= 0) back.push_back(curr);
        if((sc > 0 && sn < 0) || (sc < 0 && sn > 0)) {
            const vec3<T> x = intersect_edge_with_plane(P, curr, next);
            front.push_back(x);
            back.push_back(x);
        }
    }
}

template <class T>
void triangulate_fan(const std::vector<vec3<T>> &poly,
                     std::vector<std::array<vec3<T>, 3>> &out) {
    if(poly.size() < 3) return;
    for(size_t i = 1; i + 1 < poly.size(); ++i) {
        const vec3<T> &a = poly[0];
        const vec3<T> &b = poly[i];
        const vec3<T> &c = poly[i + 1];
        const vec3<T> N = (b - a).Cross(c - a);
        const T scale = std::max({a.sq_length(), b.sq_length(), c.sq_length(), static_cast<T>(1)});
        if(N.sq_length() < std::numeric_limits<T>::epsilon() * scale * scale) continue;
        out.push_back({{a, b, c}});
    }
}

template <class T, class I>
T compute_tree_bbox_margin(const typename bsp_tree_volume<T, I>::Node *node) {
    T maxc = static_cast<T>(0);
    if(!node) return maxc;
    if(node->type == bsp_tree_volume<T, I>::NodeType::Partition) {
        const auto &p = node->partition_plane.centroid();
        maxc = std::max({maxc, std::abs(p.x), std::abs(p.y), std::abs(p.z)});
        maxc = std::max(maxc, compute_tree_bbox_margin<T, I>(node->front.get()));
        maxc = std::max(maxc, compute_tree_bbox_margin<T, I>(node->back.get()));
    }
    return maxc;
}

template <class T>
std::vector<vec3<T>> make_initial_polygon(const bsp_plane<T> &P, T size) {
    vec3<T> u;
    const vec3<T> x_axis(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
    const vec3<T> y_axis(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
    if(std::abs(P.unit_normal().Dot(x_axis)) < static_cast<T>(0.9)) {
        u = P.unit_normal().Cross(x_axis).unit();
    } else {
        u = P.unit_normal().Cross(y_axis).unit();
    }
    const vec3<T> v = P.unit_normal().Cross(u);
    const vec3<T> c = P.centroid();
    return {
        c + u * size + v * size,
        c + u * size - v * size,
        c - u * size - v * size,
        c - u * size + v * size
    };
}

// ============================================================================
// Helper: check whether a subtree contains the given leaf type.
// ============================================================================
template <class T, class I>
bool bsp_subtree_contains(const typename bsp_tree_volume<T, I>::Node *n,
                          typename bsp_tree_volume<T, I>::NodeType t) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!n) return (t == NT::Out);
    if(n->type == t) return true;
    if(n->type != NT::Partition) return false;
    return bsp_subtree_contains<T, I>(n->front.get(), t)
        || bsp_subtree_contains<T, I>(n->back.get(), t);
}

// Clip a polygon against a BSP subtree, keeping only the region where
// the subtree evaluates to |target|.
//
// Recursively walks the tree, clipping the polygon against each partition
// plane.  When the polygon lies on only one side of a plane the recursion
// follows that side, but the polygon is also clipped against the *
// sibling's* planes so that bounds from the opposite side are honoured.
template <class T, class I>
void clip_polygon_to_leaf(const typename bsp_tree_volume<T, I>::Node *sub,
                          std::vector<vec3<T>> &poly,
                          typename bsp_tree_volume<T, I>::NodeType target) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!sub || poly.size() < 3) return;
    if(sub->type == target) return;
    if(sub->type != NT::Partition) {
        poly.clear();
        return;
    }
    std::vector<vec3<T>> front_poly, back_poly;
    clip_polygon_by_plane(sub->partition_plane, poly, front_poly, back_poly);
    const bool front_valid = (front_poly.size() >= 3);
    const bool back_valid  = (back_poly.size() >= 3);

    if(front_valid && back_valid) {
        clip_polygon_to_leaf<T, I>(sub->front.get(), front_poly, target);
        clip_polygon_to_leaf<T, I>(sub->back.get(), back_poly, target);
        if(front_poly.size() >= 3 && back_poly.size() >= 3) {
            poly = (front_poly.size() >= back_poly.size())
                 ? std::move(front_poly)
                 : std::move(back_poly);
        } else if(front_poly.size() >= 3) {
            poly = std::move(front_poly);
        } else if(back_poly.size() >= 3) {
            poly = std::move(back_poly);
        } else {
            poly.clear();
        }
    } else if(front_valid) {
        poly = std::move(front_poly);
        clip_polygon_to_leaf<T, I>(sub->front.get(), poly, target);
    } else if(back_valid) {
        poly = std::move(back_poly);
        clip_polygon_to_leaf<T, I>(sub->back.get(), poly, target);
    } else {
        bool all_front = true, all_back = true;
        for(const auto &v : poly) {
            const int s = classify_point(sub->partition_plane, v);
            if(s < 0) all_front = false;
            if(s > 0) all_back  = false;
        }
        if(all_front && !all_back) {
            clip_polygon_to_leaf<T, I>(sub->front.get(), poly, target);
        } else if(all_back && !all_front) {
            clip_polygon_to_leaf<T, I>(sub->back.get(), poly, target);
        } else {
            const bool f_contains = bsp_subtree_contains<T, I>(sub->front.get(), target);
            const bool b_contains = bsp_subtree_contains<T, I>(sub->back.get(), target);
            if(f_contains && b_contains) {
                clip_polygon_to_leaf<T, I>(sub->front.get(), poly, target);
                clip_polygon_to_leaf<T, I>(sub->back.get(), poly, target);
            } else if(f_contains) {
                clip_polygon_to_leaf<T, I>(sub->front.get(), poly, target);
            } else if(b_contains) {
                clip_polygon_to_leaf<T, I>(sub->back.get(), poly, target);
            } else {
                poly.clear();
            }
        }
    }
}


// ============================================================================
// Classify a 3D point against a BSP subtree: return In/Out based on the
// leaf that is reached.
// ============================================================================
template <class T, class I>
typename bsp_tree_volume<T, I>::NodeType
classify_point_in_subtree(const typename bsp_tree_volume<T, I>::Node *n,
                          const vec3<T> &pt) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!n) return NT::Out;
    if(n->type != NT::Partition) return n->type;
    const int s = classify_point(n->partition_plane, pt);
    if(s >= 0) return classify_point_in_subtree<T, I>(n->front.get(), pt);
    return classify_point_in_subtree<T, I>(n->back.get(), pt);
}


template <class T, class I>
void extract_boundary_faces(
    const typename bsp_tree_volume<T, I>::Node *node,
    const std::vector<const typename bsp_tree_volume<T, I>::Node *> &ancestors,
    const std::vector<bool> &side_stack,
    T bbox_size,
    std::vector<PolyFace<T>> &faces) {
    using NT = NodeType<T, I>;
    enum class Eval : uint8_t { Out = 0, In = 1, Mixed = 2 };

    if(!node) return;
    if(node->type != NT::Partition) return;

    const auto eval_subtree = [](const typename bsp_tree_volume<T, I>::Node *n,
                                  auto &&recurse) -> Eval {
        if(!n) return Eval::Out;
        if(n->type == NT::In) return Eval::In;
        if(n->type == NT::Out) return Eval::Out;
        const Eval fe = recurse(n->front.get(), recurse);
        const Eval be = recurse(n->back.get(), recurse);
        if(fe == be) return fe;
        return Eval::Mixed;
    };

    const Eval front_eval = eval_subtree(node->front.get(), eval_subtree);
    const Eval back_eval  = eval_subtree(node->back.get(), eval_subtree);

    // Extract a boundary face whenever the two sides differ in their
    // IN/OUT classification.  This includes the classical IN-vs-OUT case
    // as well as Mixed-vs-leaf cases where part of the plane is a true
    // boundary of the solid.
    if((front_eval == Eval::In  && back_eval == Eval::Out)
    || (front_eval == Eval::Out && back_eval == Eval::In)
    || (front_eval == Eval::In  && back_eval == Eval::Mixed)
    || (front_eval == Eval::Out && back_eval == Eval::Mixed)
    || (front_eval == Eval::Mixed && back_eval == Eval::In)
    || (front_eval == Eval::Mixed && back_eval == Eval::Out)) {

        auto poly = make_initial_polygon<T>(node->partition_plane, bbox_size);

        // Clip against ancestor planes.
        for(size_t i = 0; i < ancestors.size(); ++i) {
            const bsp_plane<T> &anc_plane = ancestors[i]->partition_plane;
            const bool went_front = side_stack[i];
            std::vector<vec3<T>> keep, discard;
            clip_polygon_by_plane(anc_plane, poly, keep, discard);
            poly = went_front ? std::move(keep) : std::move(discard);
            if(poly.size() < 3) break;
        }

        // When one child is a leaf and the other is Mixed we also need to
        // clip the face polygon against the Mixed descendant planes so that
        // the face is bounded to the region where it really is a boundary.
        if(poly.size() >= 3) {
            if(front_eval == Eval::In && back_eval == Eval::Mixed) {
                clip_polygon_to_leaf<T, I>(node->back.get(), poly, NT::Out);
            } else if(front_eval == Eval::Out && back_eval == Eval::Mixed) {
                clip_polygon_to_leaf<T, I>(node->back.get(), poly, NT::In);
            } else if(front_eval == Eval::Mixed && back_eval == Eval::In) {
                clip_polygon_to_leaf<T, I>(node->front.get(), poly, NT::Out);
            } else if(front_eval == Eval::Mixed && back_eval == Eval::Out) {
                clip_polygon_to_leaf<T, I>(node->front.get(), poly, NT::In);
            }
        }

        if(poly.size() >= 3) {
            const vec3<T> face_normal = (front_eval == Eval::In)
                ? node->partition_plane.unit_normal()
                : node->partition_plane.unit_normal() * static_cast<T>(-1);
            faces.push_back({std::move(poly), face_normal});
        }
    }

    // Recurse front.
    {
        auto a2 = ancestors;
        auto s2 = side_stack;
        a2.push_back(node);
        s2.push_back(true);
        extract_boundary_faces<T, I>(node->front.get(), a2, s2,
                                      bbox_size, faces);
    }
    // Recurse back.
    {
        auto a2 = ancestors;
        auto s2 = side_stack;
        a2.push_back(node);
        s2.push_back(false);
        extract_boundary_faces<T, I>(node->back.get(), a2, s2,
                                      bbox_size, faces);
    }
}

} // anonymous namespace


// ============================================================================
// Node implementation
// ============================================================================
template <class T, class I>
bsp_tree_volume<T, I>::Node::Node()
    : type(NodeType::Out) {}

template <class T, class I>
bsp_tree_volume<T, I>::Node::Node(NodeType t)
    : type(t) {}

template <class T, class I>
bsp_tree_volume<T, I>::Node::Node(const bsp_plane<T> &p,
                                   std::unique_ptr<Node> f,
                                   std::unique_ptr<Node> b)
    : type(NodeType::Partition),
      partition_plane(p),
      front(std::move(f)),
      back(std::move(b)) {}

template <class T, class I>
typename bsp_tree_volume<T, I>::Node *
bsp_tree_volume<T, I>::Node::clone() const {
    auto *n = new Node(type);
    if(type == NodeType::Partition) {
        n->partition_plane = partition_plane;
        n->front.reset(front ? front->clone() : nullptr);
        n->back.reset(back ? back->clone() : nullptr);
    }
    return n;
}


// ============================================================================
// bsp_tree_volume implementation
// ============================================================================
template <class T, class I>
bsp_tree_volume<T, I>::bsp_tree_volume()
    : root(nullptr) {}

template <class T, class I>
bsp_tree_volume<T, I>::bsp_tree_volume(std::unique_ptr<Node> r)
    : root(std::move(r)) {}

template <class T, class I>
bsp_tree_volume<T, I>::bsp_tree_volume(const bsp_tree_volume &other)
    : root(other.root ? other.root->clone() : nullptr) {}

template <class T, class I>
bsp_tree_volume<T, I> &
bsp_tree_volume<T, I>::operator=(const bsp_tree_volume &other) {
    if(this != &other) {
        root.reset(other.root ? other.root->clone() : nullptr);
    }
    return *this;
}

template <class T, class I>
bool bsp_tree_volume<T, I>::empty() const {
    return (root == nullptr);
}

template <class T, class I>
const typename bsp_tree_volume<T, I>::Node *
bsp_tree_volume<T, I>::get_root() const {
    return root.get();
}


// ---- Boolean operations ----

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::boolean_union(const bsp_tree_volume &other) const {
    if(!root) return other;
    if(!other.root) return *this;
    auto r = merge_bsp<T, I>(clone_node<T, I>(root.get()),
                             clone_node<T, I>(other.root.get()), 0);
    r = collapse_uniform<T, I>(std::move(r));
    return bsp_tree_volume(std::move(r));
}

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::boolean_intersection(const bsp_tree_volume &other) const {
    if(!root || !other.root)
        return bsp_tree_volume();
    auto r = merge_bsp<T, I>(clone_node<T, I>(root.get()),
                             clone_node<T, I>(other.root.get()), 1);
    r = collapse_uniform<T, I>(std::move(r));
    return bsp_tree_volume(std::move(r));
}

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::boolean_subtraction(const bsp_tree_volume &other) const {
    if(!root) return bsp_tree_volume();
    if(!other.root) return *this;
    auto r = merge_bsp<T, I>(clone_node<T, I>(root.get()),
                             clone_node<T, I>(other.root.get()), 2);
    r = collapse_uniform<T, I>(std::move(r));
    return bsp_tree_volume(std::move(r));
}

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::boolean_exclusion(const bsp_tree_volume &other) const {
    if(!root) return other;
    if(!other.root) return *this;
    // A XOR B = (A - B) ∪ (B - A) = (A ∪ B) - (A ∩ B)
    auto a_sub_b = merge_bsp<T, I>(clone_node<T, I>(root.get()),
                                   clone_node<T, I>(other.root.get()), 2);
    auto b_sub_a = merge_bsp<T, I>(clone_node<T, I>(other.root.get()),
                                   clone_node<T, I>(root.get()), 2);
    auto r = merge_bsp<T, I>(std::move(a_sub_b), std::move(b_sub_a), 0);
    r = collapse_uniform<T, I>(std::move(r));
    return bsp_tree_volume(std::move(r));
}


// ---- Conversion: fv_surface_mesh -> bsp_tree_volume ----

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::from_fv_surface_mesh(
    const fv_surface_mesh<T, I> &mesh) {

    if(mesh.faces.empty() || mesh.vertices.empty())
        return bsp_tree_volume();

    fv_surface_mesh<T, I> working_mesh = mesh;
    working_mesh.convert_to_triangles();

    if(!HasOnlyFiniteVertices(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains non-finite vertices.");

    working_mesh.remove_degenerate_faces();

    if(working_mesh.faces.empty())
        return bsp_tree_volume();

    if(!IsTriangularMesh(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh must contain only triangular faces.");
    if(!HasValidFaceIndices(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains out-of-range face indices.");
    if(!HasNoDegenerateFaces(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains degenerate faces.");
    if(!IsClosedManifold(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh is not a closed manifold.");
    if(!HasConsistentOrientation(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh faces do not have a consistent orientation.");

    std::vector<TriangleRec<T>> tri_recs;
    std::vector<std::array<vec3<T>, 3>> all_tris;

    tri_recs.reserve(working_mesh.faces.size());
    all_tris.reserve(working_mesh.faces.size());

    for(size_t fi = 0; fi < working_mesh.faces.size(); ++fi) {
        const auto &f = working_mesh.faces[fi];
        if(f.size() != 3) continue;

        const I i0 = f[0], i1 = f[1], i2 = f[2];

        const vec3<T> &v0 = working_mesh.vertices[i0];
        const vec3<T> &v1 = working_mesh.vertices[i1];
        const vec3<T> &v2 = working_mesh.vertices[i2];

        const vec3<T> edge1 = v1 - v0;
        const vec3<T> edge2 = v2 - v0;
        const vec3<T> normal = edge1.Cross(edge2);
        const T sqlen = normal.sq_length();
        if(sqlen < plane_threshold<T>()) {
            const T pa[3] = { v0.x, v0.y, v0.z };
            const T pb[3] = { v1.x, v1.y, v1.z };
            const T pc[3] = { v2.x, v2.y, v2.z };
            vec3<T> dir = normal;
            if(dir.sq_length() < plane_threshold<T>()) {
                dir = vec3<T>(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
            }
            const T pd[3] = { v0.x + dir.x, v0.y + dir.y, v0.z + dir.z };
            if(adaptive_predicate::orient3d(pa, pb, pc, pd) == static_cast<T>(0)) continue;
        }

        all_tris.push_back({{v0, v1, v2}});

        TriangleRec<T> rec;
        rec.v = {{v0, v1, v2}};
        rec.pl = plane_from_triangle(v0, v1, v2);
        tri_recs.push_back(rec);
    }

    if(tri_recs.empty())
        return bsp_tree_volume();

    auto root = build_bsp_from_triangles<T, I>(
        std::move(tri_recs), all_tris, 0);

    return bsp_tree_volume<T, I>(std::move(root));
}


// ---- Conversion: bsp_tree_volume -> fv_surface_mesh ----

template <class T, class I>
fv_surface_mesh<T, I>
bsp_tree_volume<T, I>::to_fv_surface_mesh() const {
    fv_surface_mesh<T, I> mesh;
    if(!root) return mesh;

    if(root->type == NodeType::Out) return mesh;

    if(root->type == NodeType::In) {
        const T s = static_cast<T>(1);
        mesh.vertices = {
            vec3<T>(-s, -s, -s), vec3<T>( s, -s, -s),
            vec3<T>( s,  s, -s), vec3<T>(-s,  s, -s),
            vec3<T>(-s, -s,  s), vec3<T>( s, -s,  s),
            vec3<T>( s,  s,  s), vec3<T>(-s,  s,  s)
        };
        mesh.faces = {
            { 0, 2, 1 }, { 0, 3, 2 },
            { 4, 5, 6 }, { 4, 6, 7 },
            { 0, 1, 5 }, { 0, 5, 4 },
            { 2, 3, 7 }, { 2, 7, 6 },
            { 0, 4, 7 }, { 0, 7, 3 },
            { 1, 2, 6 }, { 1, 6, 5 }
        };
        OrientFaces(mesh);
        mesh.recreate_involved_face_index();
        return mesh;
    }

    T max_extent = compute_tree_bbox_margin<T, I>(root.get());
    if(max_extent < static_cast<T>(1)) {
        max_extent = static_cast<T>(1);
    }
    T bbox_size = max_extent
                + std::max(max_extent * static_cast<T>(0.1),
                           static_cast<T>(100));

    std::vector<const Node *> ancestors;
    std::vector<bool> side_stack;
    std::vector<PolyFace<T>> faces;
    extract_boundary_faces<T, I>(root.get(), ancestors, side_stack,
                                  bbox_size, faces);

    std::vector<std::array<vec3<T>, 3>> tris;
    for(auto &face : faces) {
        triangulate_fan(face.vertices, tris);
    }

    if(tris.empty()) return mesh;

    std::map<std::tuple<T, T, T>, I> vert_map;
    {
        vec3<T> bb_min = vec3<T>( std::numeric_limits<T>::max(),
                                   std::numeric_limits<T>::max(),
                                   std::numeric_limits<T>::max());
        vec3<T> bb_max = vec3<T>(-std::numeric_limits<T>::max(),
                                  -std::numeric_limits<T>::max(),
                                  -std::numeric_limits<T>::max());
        for(const auto &tri : tris) {
            for(const auto &v : tri) {
                bb_min.x = std::min(bb_min.x, v.x);
                bb_min.y = std::min(bb_min.y, v.y);
                bb_min.z = std::min(bb_min.z, v.z);
                bb_max.x = std::max(bb_max.x, v.x);
                bb_max.y = std::max(bb_max.y, v.y);
                bb_max.z = std::max(bb_max.z, v.z);
            }
        }
        const vec3<T> extent = bb_max - bb_min;
        const T scale = std::max({extent.x, extent.y, extent.z, static_cast<T>(1)});
        const T weld_eps = std::sqrt(std::numeric_limits<T>::epsilon()) * scale;

        const auto get_or_add_vertex = [&](const vec3<T> &v) -> I {
            auto key = std::make_tuple(
                std::round(v.x / weld_eps) * weld_eps,
                std::round(v.y / weld_eps) * weld_eps,
                std::round(v.z / weld_eps) * weld_eps);
            auto it = vert_map.find(key);
            if(it != vert_map.end()) return it->second;
            I idx = static_cast<I>(mesh.vertices.size());
            mesh.vertices.push_back(v);
            vert_map[key] = idx;
            return idx;
        };

        for(const auto &tri : tris) {
            std::vector<I> face;
            face.push_back(get_or_add_vertex(tri[0]));
            face.push_back(get_or_add_vertex(tri[1]));
            face.push_back(get_or_add_vertex(tri[2]));
            mesh.faces.push_back(std::move(face));
        }
    }

    OrientFaces(mesh);
    mesh.recreate_involved_face_index();
    return mesh;
}


// ============================================================================
// Explicit template instantiations
// ============================================================================

#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS

template class bsp_tree_volume<float,  uint32_t>;
template class bsp_tree_volume<float,  uint64_t>;
template class bsp_tree_volume<double, uint32_t>;
template class bsp_tree_volume<double, uint64_t>;

#endif // YGORMATH_DISABLE_ALL_SPECIALIZATIONS
